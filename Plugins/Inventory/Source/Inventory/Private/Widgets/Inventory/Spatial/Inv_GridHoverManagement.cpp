#include "Widgets/Inventory/Spatial/Inv_GridHoverManagement.h"
#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Widgets/Inventory/Spatial/Inv_GridInitialization.h"
#include "Widgets/Inventory/Spatial/Inv_GridItemPlacement.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Items/Fragments/Inv_FragmentTags.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Items/Inv_InventoryItem.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"

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

	AssignHoverItem(Grid, ClickedInventoryItem, GridIndex, GridIndex);
	UInv_GridItemPlacement::RemoveItemFromGrid(Grid, ClickedInventoryItem, GridIndex);

	Grid->SourceGrid = Grid;
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

	Grid->HoverItem->SetInventoryItem(nullptr);
	Grid->HoverItem->SetIsStackable(false);
	Grid->HoverItem->SetPreviousGridIndex(INDEX_NONE);
	Grid->HoverItem->UpdateStackCount(0);
	Grid->HoverItem->SetImageBrush(FSlateNoResource());
	Grid->HoverItem->SetOwnerGrid(nullptr);

	Grid->HoverItem->RemoveFromParent();
	Grid->HoverItem = nullptr;

	ShowCursor(Grid);
}

void UInv_GridHoverManagement::PutHoverItemBack(UInv_InventoryGrid* Grid)
{
	UInv_InventoryGrid* HoverGrid = UInv_GridInitialization::GetGridWithHoverItem(Grid);
	if (!IsValid(HoverGrid) || !IsValid(HoverGrid->HoverItem))
	{
		return;
	}

	UInv_HoverItem* LocalHoverItem = HoverGrid->HoverItem;

	if (!IsValid(LocalHoverItem->GetInventoryItem()))
	{
		ClearHoverItem(HoverGrid);
		return;
	}

	if (Grid->SourceGrid.IsValid() && Grid->SourceGrid.Get() != Grid)
	{
		UInv_InventoryGrid* SourceGridPtr = Grid->SourceGrid.Get();
		FInv_SlotAvailabilityResult Result = UInv_GridItemPlacement::HasRoomForItem(SourceGridPtr, LocalHoverItem->GetInventoryItem(),
			LocalHoverItem->GetStackCount());

		if (Result.TotalRoomToFill > 0)
		{
			Result.Item = LocalHoverItem->GetInventoryItem();
			SourceGridPtr->AddStacks(Result);
			ClearHoverItem(HoverGrid);
			return;
		}
	}

	FInv_SlotAvailabilityResult Result = UInv_GridItemPlacement::HasRoomForItem(Grid, LocalHoverItem->GetInventoryItem(),
		LocalHoverItem->GetStackCount());
	Result.Item = LocalHoverItem->GetInventoryItem();

	Grid->AddStacks(Result);
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

	UInv_GridItemPlacement::AddItemAtIndex(Grid, Grid->HoverItem->GetInventoryItem(), Index, 
										   Grid->HoverItem->IsStackable(), Grid->HoverItem->GetStackCount());
	UInv_GridItemPlacement::UpdateGridSlots(Grid, Grid->HoverItem->GetInventoryItem(), Index, 
											Grid->HoverItem->IsStackable(), Grid->HoverItem->GetStackCount());

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