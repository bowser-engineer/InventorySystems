#include "Widgets/Inventory/Spatial/Inv_GridHoverManagement.h"
#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Widgets/Inventory/Spatial/Inv_GridInitialization.h"
#include "Widgets/Inventory/Spatial/Inv_GridItemPlacement.h"
#include "Widgets/Inventory/Spatial/Inv_SpatialInventory.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Items/Fragments/Inv_FragmentTags.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Items/Inv_InventoryItem.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"


void UInv_GridHoverManagement::AssignHoverItem(UInv_InventoryGrid* Grid, UInv_InventoryItem* InventoryItem)
{
	if (!IsValid(Grid) || !IsValid(InventoryItem)) return;

	if (!IsValid(Grid->HoverItem))
	{
		Grid->HoverItem = CreateWidget<UInv_HoverItem>(Grid->GetOwningPlayer(), Grid->HoverItemClass);
	}

	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(InventoryItem, FragmentTags::GridFragment);
	const FInv_ImageFragment* ImageFragment = GetFragment<FInv_ImageFragment>(InventoryItem, FragmentTags::IconFragment);
	if (!GridFragment || !ImageFragment) return;

	const FVector2D DrawSize = GetDrawSize(Grid, GridFragment);

	FSlateBrush IconBrush;
	IconBrush.SetResourceObject(ImageFragment->GetIcon());
	IconBrush.DrawAs = ESlateBrushDrawType::Image;
	IconBrush.ImageSize = DrawSize * UWidgetLayoutLibrary::GetViewportScale(Grid);

	Grid->HoverItem->SetImageBrush(IconBrush);
	Grid->HoverItem->SetGridDimensions(GridFragment->GetGridSize());
	Grid->HoverItem->SetInventoryItem(InventoryItem);
	Grid->HoverItem->SetIsStackable(InventoryItem->IsStackable());
	Grid->HoverItem->SetOwnerGrid(Grid);

	Grid->GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, Grid->HoverItem);
}

void UInv_GridHoverManagement::AssignHoverItem(UInv_InventoryGrid* Grid, UInv_InventoryItem* InventoryItem, 
											  const int32 GridIndex, const int32 PreviousGridIndex)
{
	AssignHoverItem(Grid, InventoryItem);

	if (IsValid(Grid->HoverItem))
	{
		Grid->HoverItem->SetPreviousGridIndex(PreviousGridIndex);
		Grid->HoverItem->UpdateStackCount(InventoryItem->IsStackable() && Grid->GridSlots.IsValidIndex(GridIndex) ? 
										   Grid->GridSlots[GridIndex]->GetStackCount() : 0);
	}
}

void UInv_GridHoverManagement::PickUp(UInv_InventoryGrid* Grid, UInv_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	if (!IsValid(Grid) || !IsValid(ClickedInventoryItem)) return;

	UE_LOG(LogTemp, Warning, TEXT("[PickUp] DEBUG: Grid=%s, GridIndex=%d, Item=%s"), 
		*GetNameSafe(Grid), GridIndex, *GetNameSafe(ClickedInventoryItem));

	AssignHoverItem(Grid, ClickedInventoryItem, GridIndex, GridIndex);
	// Don't remove the item from grid yet - only remove it when successfully placed elsewhere

	// Only set SourceGrid to self if there's no existing valid SourceGrid (preserve the chain)
	if (!Grid->SourceGrid.IsValid())
	{
		Grid->SourceGrid = Grid;
		UE_LOG(LogTemp, Warning, TEXT("[PickUp] Set SourceGrid to self: %s"), *GetNameSafe(Grid));
	}
	else if (Grid->SourceGrid.Get() != Grid)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PickUp] Preserving existing SourceGrid: %s"), *GetNameSafe(Grid->SourceGrid.Get()));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PickUp] SourceGrid already points to self: %s"), *GetNameSafe(Grid));
	}
}

void UInv_GridHoverManagement::ClearHoverItem(UInv_InventoryGrid* Grid)
{
	if (!IsValid(Grid) || !IsValid(Grid->HoverItem)) 
	{
		if (IsValid(Grid))
		{
			Grid->HoverItem = nullptr;
		}
		return;
	}

	// Safety check: only clear if this grid actually owns the hover item
	if (Grid->HoverItem->GetOwnerGrid() != Grid)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HoverManagement] Attempted to clear hover item from non-owning grid %s. Owner: %s"),
			*GetNameSafe(Grid), 
			*GetNameSafe(Grid->HoverItem->GetOwnerGrid()));
		// Just clear the reference, don't destroy the hover item
		Grid->HoverItem = nullptr;
		return;
	}

	Grid->HoverItem->SetInventoryItem(nullptr);
	Grid->HoverItem->SetIsStackable(false);
	Grid->HoverItem->SetPreviousGridIndex(INDEX_NONE);
	Grid->HoverItem->UpdateStackCount(0);
	Grid->HoverItem->SetImageBrush(FSlateNoResource());
	Grid->HoverItem->SetOwnerGrid(nullptr);
	Grid->HoverItem->SetWasPreviouslyEquipped(false);

	Grid->HoverItem->RemoveFromParent();
	Grid->HoverItem = nullptr;

	ShowCursor(Grid);
}

void UInv_GridHoverManagement::PutHoverItemBack(UInv_InventoryGrid* Grid)
{
	static double LastCallTime = 0.0;
	static int32 CallCount = 0;
	double CurrentTime = FPlatformTime::Seconds();
	
	// Reset counter if more than 0.5 seconds have passed
	if (CurrentTime - LastCallTime > 0.5)
	{
		CallCount = 0;
	}
	
	CallCount++;
	LastCallTime = CurrentTime;
	
	UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] Called for Grid: %s (Call #%d at time %.3f)"), 
		*GetNameSafe(Grid), CallCount, CurrentTime);
	
	// Guard against mass cleanup calls - only allow if there's actually a hover item somewhere
	UInv_InventoryGrid* ActualHoverGrid = UInv_GridInitialization::GetGridWithHoverItem(Grid);
	if (!ActualHoverGrid)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] GUARD: No hover item found anywhere - ignoring cleanup call for %s"), *GetNameSafe(Grid));
		return;
	}
	
	// Additional guard: only proceed if this grid is the one with the hover item OR if it's being called appropriately
	if (ActualHoverGrid != Grid && CallCount > 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] GUARD: Mass cleanup detected - Grid %s doesn't have hover item (belongs to %s), ignoring call #%d"), 
			*GetNameSafe(Grid), *GetNameSafe(ActualHoverGrid), CallCount);
		return;
	}
	
	UInv_InventoryGrid* HoverGrid = UInv_GridInitialization::GetGridWithHoverItem(Grid);
	if (!IsValid(HoverGrid) || !IsValid(HoverGrid->HoverItem))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] No valid hover grid or hover item found"));
		return;
	}

	UInv_HoverItem* LocalHoverItem = HoverGrid->HoverItem;

	if (!IsValid(LocalHoverItem->GetInventoryItem()))
	{
		ClearHoverItem(HoverGrid);
		return;
	}

	if (HoverGrid->SourceGrid.IsValid() && HoverGrid->SourceGrid.Get() != HoverGrid)
	{
		UInv_InventoryGrid* SourceGridPtr = HoverGrid->SourceGrid.Get();
		UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] Trying to return item to SourceGrid: %s"), *GetNameSafe(SourceGridPtr));
		
		FInv_SlotAvailabilityResult Result = UInv_GridItemPlacement::HasRoomForItem(SourceGridPtr, LocalHoverItem->GetInventoryItem(),
			LocalHoverItem->GetStackCount());

		UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] SourceGrid HasRoomForItem result: TotalRoomToFill=%d"), Result.TotalRoomToFill);

		if (Result.TotalRoomToFill > 0)
		{
			Result.Item = LocalHoverItem->GetInventoryItem();
			UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] Calling AddStacks on SourceGrid"));
			SourceGridPtr->AddStacks(Result);
			ClearHoverItem(HoverGrid);
			
			// UI refresh will happen automatically through the widget system
			
			UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] Successfully returned item to SourceGrid"));
			return;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] SourceGrid has no room, trying HoverGrid"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] No valid SourceGrid, using HoverGrid"));
	}

	FInv_SlotAvailabilityResult Result = UInv_GridItemPlacement::HasRoomForItem(HoverGrid, LocalHoverItem->GetInventoryItem(),
		LocalHoverItem->GetStackCount());
	Result.Item = LocalHoverItem->GetInventoryItem();

	UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] HoverGrid HasRoomForItem result: TotalRoomToFill=%d"), Result.TotalRoomToFill);
	
	if (Result.TotalRoomToFill > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] Calling AddStacks on HoverGrid"));
		HoverGrid->AddStacks(Result);
		
		// UI refresh will happen automatically through the widget system
		
		UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] Successfully returned item to HoverGrid"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PutHoverItemBack] ERROR: No room in any grid! Item will be lost!"));
	}
	
	ClearHoverItem(HoverGrid);
}

void UInv_GridHoverManagement::DropItem(UInv_InventoryGrid* Grid)
{
	if (!IsValid(Grid)) return;

	UInv_InventoryGrid* GridWithHoverItem = UInv_GridInitialization::GetGridWithHoverItem(Grid);
	if (!GridWithHoverItem) return;

	UInv_HoverItem* ActualHoverItem = GridWithHoverItem->GetHoverItem();
	if (!IsValid(ActualHoverItem)) return;
	
	UInv_InventoryItem* InventoryItem = ActualHoverItem->GetInventoryItem();
	if (!IsValid(InventoryItem))
	{
		ClearHoverItem(GridWithHoverItem);
		return;
	}

	Grid->InventoryComponent->Server_DropItem(Grid->HoverItem->GetInventoryItem(), Grid->HoverItem->GetStackCount());

	ClearHoverItem(GridWithHoverItem);
	ShowCursor(Grid);
}

void UInv_GridHoverManagement::PutDownOnIndex(UInv_InventoryGrid* Grid, const int32 Index)
{
	if (!IsValid(Grid) || !IsValid(Grid->HoverItem)) return;

	UE_LOG(LogTemp, Warning, TEXT("Putting down item at index %d"), Index);

	// Store hover item info before clearing
	UInv_InventoryItem* HoverInventoryItem = Grid->HoverItem->GetInventoryItem();
	UInv_InventoryGrid* OriginalGrid = Grid->HoverItem->GetOwnerGrid();
	int32 OriginalIndex = Grid->HoverItem->GetPreviousGridIndex();
	bool bIsStackable = Grid->HoverItem->IsStackable();
	int32 StackCount = Grid->HoverItem->GetStackCount();
	bool bWasPreviouslyEquipped = Grid->HoverItem->WasPreviouslyEquipped();

	// Add the item to the new position
	UInv_GridItemPlacement::AddItemAtIndex(Grid, HoverInventoryItem, Index, bIsStackable, StackCount);
	UInv_GridItemPlacement::UpdateGridSlots(Grid, HoverInventoryItem, Index, bIsStackable, StackCount);

	// Now remove the item from its original position (only if successful placement)
	// BUT NOT if the item was previously equipped - equipped items don't have an inventory position to remove from
	if (!bWasPreviouslyEquipped && IsValid(OriginalGrid) && OriginalGrid->GridSlots.IsValidIndex(OriginalIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("Removing item from original position: Grid=%s, Index=%d"), 
			*GetNameSafe(OriginalGrid), OriginalIndex);
		UInv_GridItemPlacement::RemoveItemFromGrid(OriginalGrid, HoverInventoryItem, OriginalIndex);
	}
	else if (bWasPreviouslyEquipped)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item was previously equipped - not removing from inventory position"));
	}

	// If item was previously equipped and is now placed in inventory, trigger unequipping
	if (bWasPreviouslyEquipped)
	{
		// Find the spatial inventory widget to call the unequip method
		if (UInv_SpatialInventory* SpatialInventory = Cast<UInv_SpatialInventory>(UInv_InventoryStatics::GetInventoryWidget(Grid->GetOwningPlayer())))
		{
			SpatialInventory->OnItemPlacedInInventory(HoverInventoryItem);
		}
	}

	if (UInv_InventoryGrid* HoverGrid = UInv_GridInitialization::GetGridWithHoverItem(Grid)) 
	{
		ClearHoverItem(HoverGrid);
	}
}

void UInv_GridHoverManagement::ShowCursor(UInv_InventoryGrid* Grid)
{
	if (!IsValid(Grid) || !IsValid(Grid->GetOwningPlayer())) return;
	Grid->GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, Grid->GetVisibleCursorWidget());
}

void UInv_GridHoverManagement::HideCursor(UInv_InventoryGrid* Grid)
{
	if (!IsValid(Grid) || !IsValid(Grid->GetOwningPlayer())) return;
	Grid->GetOwningPlayer()->SetMouseCursorWidget(EMouseCursor::Default, Grid->GetHiddenCursorWidget());
}

FVector2D UInv_GridHoverManagement::GetDrawSize(const UInv_InventoryGrid* Grid, const FInv_GridFragment* GridFragment)
{
	if (!IsValid(Grid) || !GridFragment) return FVector2D::ZeroVector;

	const float IconTileWidth = Grid->TileSize - GridFragment->GetGridPadding() * 2;
	return GridFragment->GetGridSize() * IconTileWidth;
}