#include "Widgets/Inventory/Spatial/Inv_GridPopupInteractions.h"
#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Widgets/Inventory/Spatial/Inv_GridItemPlacement.h"
#include "Widgets/Inventory/Spatial/Inv_GridHoverManagement.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Widgets/ItemPopUp/Inv_ItemPopUp.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Items/Inv_InventoryItem.h"
#include "Widgets/Utils/Inv_WidgetUtils.h"
#include "Widgets/Inventory/SlottedItems/Inv_SlottedItem.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"

void UInv_GridPopupInteractions::CreateItemPopUp(UInv_InventoryGrid* Grid, const int32 GridIndex)
{
	if (!IsValid(Grid) || !Grid->GridSlots.IsValidIndex(GridIndex)) return;

	UInv_InventoryItem* RightClickedItem = Grid->GridSlots[GridIndex]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	if (IsValid(Grid->GridSlots[GridIndex]->GetItemPopUp())) return;

	Grid->ItemPopUp = CreateWidget<UInv_ItemPopUp>(Grid, Grid->ItemPopUpClass);
	Grid->GridSlots[GridIndex]->SetItemPopUp(Grid->ItemPopUp);

	Grid->OwningCanvasPanel->AddChild(Grid->ItemPopUp);
	UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Grid->ItemPopUp);
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(Grid->GetOwningPlayer());
	CanvasSlot->SetPosition(MousePosition - Grid->ItemPopUpOffset);
	CanvasSlot->SetSize(Grid->ItemPopUp->GetBoxSize());

	const int32 SliderMax = Grid->GridSlots[GridIndex]->GetStackCount() - 1;
	if (RightClickedItem->IsStackable() && SliderMax > 0)
	{
		Grid->ItemPopUp->OnSplit.BindDynamic(Grid, &UInv_InventoryGrid::OnPopUpMenuSplit);
		Grid->ItemPopUp->SetSliderParams(SliderMax, FMath::Max(1, Grid->GridSlots[GridIndex]->GetStackCount() / 2));
	}
	else
	{
		Grid->ItemPopUp->CollapseSplitButton();
	}

	Grid->ItemPopUp->OnDrop.BindDynamic(Grid, &UInv_InventoryGrid::OnPopUpMenuDrop);

	if (RightClickedItem->IsConsumable())
	{
		Grid->ItemPopUp->OnConsume.BindDynamic(Grid, &UInv_InventoryGrid::OnPopUpMenuConsume);
	}
	else
	{
		Grid->ItemPopUp->CollapseConsumeButton();
	}
}

void UInv_GridPopupInteractions::OnPopUpMenuSplit(UInv_InventoryGrid* Grid, int32 SplitAmount, int32 Index)
{
	if (!IsValid(Grid) || !Grid->GridSlots.IsValidIndex(Index)) return;

	UInv_InventoryItem* RightClickedItem = Grid->GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;
	if (!RightClickedItem->IsStackable()) return;

	const int32 UpperLeftIndex = Grid->GridSlots[Index]->GetUpperLeftIndex();
	if (!Grid->GridSlots.IsValidIndex(UpperLeftIndex)) return;

	UInv_GridSlot* UpperLeftGridSlot = Grid->GridSlots[UpperLeftIndex];
	const int32 StackCount = UpperLeftGridSlot->GetStackCount();
	const int32 NewStackCount = StackCount - SplitAmount;

	UpperLeftGridSlot->SetStackCount(NewStackCount);
	if (Grid->SlottedItems.Contains(UpperLeftIndex))
	{
		Grid->SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);
	}

	UInv_GridHoverManagement::AssignHoverItem(Grid, RightClickedItem, UpperLeftIndex, UpperLeftIndex);
	if (IsValid(Grid->HoverItem))
	{
		Grid->HoverItem->UpdateStackCount(SplitAmount);
		Grid->HoverItem->SetIsFromSplitOperation(true);
	}
}

void UInv_GridPopupInteractions::OnPopUpMenuDrop(UInv_InventoryGrid* Grid, int32 Index)
{
	if (!IsValid(Grid) || !Grid->GridSlots.IsValidIndex(Index)) return;

	UInv_InventoryItem* RightClickedItem = Grid->GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;

	UInv_GridHoverManagement::PickUp(Grid, RightClickedItem, Index);
	UInv_GridHoverManagement::DropItem(Grid);
}

void UInv_GridPopupInteractions::OnPopUpMenuConsume(UInv_InventoryGrid* Grid, int32 Index)
{
	if (!IsValid(Grid) || !Grid->GridSlots.IsValidIndex(Index)) return;

	UInv_InventoryItem* RightClickedItem = Grid->GridSlots[Index]->GetInventoryItem().Get();
	if (!IsValid(RightClickedItem)) return;

	const int32 UpperLeftIndex = Grid->GridSlots[Index]->GetUpperLeftIndex();
	if (!Grid->GridSlots.IsValidIndex(UpperLeftIndex)) return;

	UInv_GridSlot* UpperLeftGridSlot = Grid->GridSlots[UpperLeftIndex];
	const int32 NewStackCount = UpperLeftGridSlot->GetStackCount() - 1;

	UpperLeftGridSlot->SetStackCount(NewStackCount);
	if (Grid->SlottedItems.Contains(UpperLeftIndex))
	{
		Grid->SlottedItems.FindChecked(UpperLeftIndex)->UpdateStackCount(NewStackCount);
	}

	Grid->InventoryComponent->Server_ConsumeItem(RightClickedItem);

	if (NewStackCount <= 0)
	{
		UInv_GridItemPlacement::RemoveItemFromGrid(Grid, RightClickedItem, Index);
	}
}

bool UInv_GridPopupInteractions::IsRightClick(const FPointerEvent& MouseEvent)
{
	return MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
}

bool UInv_GridPopupInteractions::IsLeftClick(const FPointerEvent& MouseEvent)
{
	return MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
}

bool UInv_GridPopupInteractions::IsSameStackable(const UInv_InventoryGrid* Grid, const UInv_InventoryItem* ClickedInventoryItem)
{
	if (!IsValid(Grid) || !IsValid(Grid->HoverItem) || !IsValid(ClickedInventoryItem)) return false;

	const bool bIsStackable = ClickedInventoryItem->IsStackable() && Grid->HoverItem->IsStackable();
	const bool bIsSameItemType = Grid->HoverItem->GetItemType().MatchesTagExact(ClickedInventoryItem->GetItemManifest().GetItemType());
	return bIsStackable && bIsSameItemType;
}

bool UInv_GridPopupInteractions::ShouldSwapStackCounts(const int32 RoomInClickedSlot, const int32 HoveredStackCount, const int32 MaxStackSize)
{
	// Safety checks: ensure valid values
	if (RoomInClickedSlot < 0 || HoveredStackCount <= 0 || MaxStackSize <= 0) return false;
	
	return RoomInClickedSlot == 0 && HoveredStackCount < MaxStackSize;
}

bool UInv_GridPopupInteractions::ShouldConsumeHoverItemStacks(const int32 HoveredStackCount, const int32 RoomInClickedSlot)
{
	// Safety checks: ensure valid values
	if (HoveredStackCount <= 0 || RoomInClickedSlot < 0) return false;
	
	return RoomInClickedSlot >= HoveredStackCount;
}

bool UInv_GridPopupInteractions::ShouldFillInStack(const int32 RoomInClickedSlot, const int32 HoveredStackCount)
{
	// Safety checks: ensure valid values  
	if (RoomInClickedSlot <= 0 || HoveredStackCount <= 0) return false;
	
	return RoomInClickedSlot < HoveredStackCount;
}

void UInv_GridPopupInteractions::SwapStackCounts(UInv_InventoryGrid* Grid, const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index)
{
	if (!IsValid(Grid) || !Grid->GridSlots.IsValidIndex(Index)) return;


	UInv_GridSlot* GridSlot = Grid->GridSlots[Index];
	GridSlot->SetStackCount(HoveredStackCount);

	if (Grid->SlottedItems.Contains(Index))
	{
		UInv_SlottedItem* ClickedSlottedItem = Grid->SlottedItems.FindChecked(Index);
		ClickedSlottedItem->UpdateStackCount(HoveredStackCount);
	}

	if (IsValid(Grid->HoverItem))
	{
		// Check if the clicked stack count is 0 (all items transferred to clicked slot)
		if (ClickedStackCount <= 0)
		{
			UInv_InventoryItem* HoverInventoryItem = Grid->HoverItem->GetInventoryItem();
			UInv_InventoryGrid* SourceGrid = Grid->HoverItem->GetOwnerGrid();
			int32 PreviousIndex = Grid->HoverItem->GetPreviousGridIndex();
			
			
			// Handle removal from source grid if needed
			if (IsValid(SourceGrid) && SourceGrid->GridSlots.IsValidIndex(PreviousIndex))
			{
				// Check if this was from a split operation (partial stack)
				if (Grid->HoverItem->IsFromSplitOperation())
				{
					// For split operations, we already reduced the original stack count in OnPopUpMenuSplit
					// No need to remove anything from source
				}
				// Check if this is a cross-grid transfer (different grid)
				else if (SourceGrid != Grid)
				{
					// Remove the entire item from the source grid since all stacks were transferred
					UInv_GridItemPlacement::RemoveItemFromGrid(SourceGrid, HoverInventoryItem, PreviousIndex);
				}
				// Same grid transfer - need to remove original item
				else if (SourceGrid == Grid)
				{
					UInv_GridItemPlacement::RemoveItemFromGrid(SourceGrid, HoverInventoryItem, PreviousIndex);
				}
			}
			
			// Clear the hover item since it's empty
			UInv_GridHoverManagement::ClearHoverItem(Grid);
			UInv_GridHoverManagement::ShowCursor(Grid);
		}
		else
		{
			// Update the hover item with the swapped stack count
			Grid->HoverItem->UpdateStackCount(ClickedStackCount);
		}
	}
}

void UInv_GridPopupInteractions::ConsumeHoverItemStacks(UInv_InventoryGrid* Grid, const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index)
{
	if (!IsValid(Grid) || !Grid->GridSlots.IsValidIndex(Index)) return;


	const int32 AmountToTransfer = HoveredStackCount;
	const int32 NewClickedStackCount = ClickedStackCount + AmountToTransfer;

	Grid->GridSlots[Index]->SetStackCount(NewClickedStackCount);
	if (Grid->SlottedItems.Contains(Index))
	{
		Grid->SlottedItems.FindChecked(Index)->UpdateStackCount(NewClickedStackCount);
	}
	
	// When consuming all hover item stacks, we need to remove the original item from its source grid
	// if it came from a split operation or cross-grid transfer
	if (IsValid(Grid->HoverItem))
	{
		UInv_InventoryItem* HoverInventoryItem = Grid->HoverItem->GetInventoryItem();
		UInv_InventoryGrid* SourceGrid = Grid->HoverItem->GetOwnerGrid();
		int32 PreviousIndex = Grid->HoverItem->GetPreviousGridIndex();
		
		
		// If the hover item came from a split operation or different grid, remove the original stack
		if (IsValid(SourceGrid) && SourceGrid->GridSlots.IsValidIndex(PreviousIndex))
		{
			bool bShouldRemoveFromSource = false;
			
			// Check if this was from a split operation (partial stack)
			if (Grid->HoverItem->IsFromSplitOperation())
			{
				// For split operations, we already reduced the original stack count in OnPopUpMenuSplit
				// No need to remove anything from source
				bShouldRemoveFromSource = false;
			}
			// Check if this is a cross-grid transfer (different grid)
			else if (SourceGrid != Grid)
			{
				// Remove the entire item from the source grid since all stacks were transferred
				UInv_GridItemPlacement::RemoveItemFromGrid(SourceGrid, HoverInventoryItem, PreviousIndex);
				bShouldRemoveFromSource = true;
			}
			// Same grid transfer - need to remove original item
			else if (SourceGrid == Grid)
			{
				UInv_GridItemPlacement::RemoveItemFromGrid(SourceGrid, HoverInventoryItem, PreviousIndex);
				bShouldRemoveFromSource = true;
			}
		}
	}
	
	UInv_GridHoverManagement::ClearHoverItem(Grid);
	UInv_GridHoverManagement::ShowCursor(Grid);

	if (IsValid(Grid->GridSlots[Index]->GetInventoryItem().Get()))
	{
		const FInv_GridFragment* GridFragment = Grid->GridSlots[Index]->GetInventoryItem()->GetItemManifest().GetFragmentOfType<FInv_GridFragment>();
		const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
		Grid->HighlightSlots(Index, Dimensions);
	}
}

void UInv_GridPopupInteractions::FillInStack(UInv_InventoryGrid* Grid, const int32 FillAmount, const int32 Remainder, const int32 Index)
{
	if (!IsValid(Grid) || !Grid->GridSlots.IsValidIndex(Index)) return;


	UInv_GridSlot* GridSlot = Grid->GridSlots[Index];
	const int32 NewStackCount = GridSlot->GetStackCount() + FillAmount;

	GridSlot->SetStackCount(NewStackCount);

	if (Grid->SlottedItems.Contains(Index))
	{
		UInv_SlottedItem* ClickedSlottedItem = Grid->SlottedItems.FindChecked(Index);
		ClickedSlottedItem->UpdateStackCount(NewStackCount);
	}

	if (IsValid(Grid->HoverItem))
	{
		// If there's no remainder, the hover item stack is fully depleted
		if (Remainder <= 0)
		{
			UInv_InventoryItem* HoverInventoryItem = Grid->HoverItem->GetInventoryItem();
			UInv_InventoryGrid* SourceGrid = Grid->HoverItem->GetOwnerGrid();
			int32 PreviousIndex = Grid->HoverItem->GetPreviousGridIndex();
			
			
			// Handle removal from source grid if needed
			if (IsValid(SourceGrid) && SourceGrid->GridSlots.IsValidIndex(PreviousIndex))
			{
				// Check if this was from a split operation (partial stack)
				if (Grid->HoverItem->IsFromSplitOperation())
				{
					// For split operations, we already reduced the original stack count in OnPopUpMenuSplit
					// No need to remove anything from source
				}
				// Check if this is a cross-grid transfer (different grid)
				else if (SourceGrid != Grid)
				{
					// Remove the entire item from the source grid since all stacks were transferred
					UInv_GridItemPlacement::RemoveItemFromGrid(SourceGrid, HoverInventoryItem, PreviousIndex);
				}
				// Same grid transfer - need to remove original item
				else if (SourceGrid == Grid)
				{
					UInv_GridItemPlacement::RemoveItemFromGrid(SourceGrid, HoverInventoryItem, PreviousIndex);
				}
			}
			
			// Clear the hover item since it's fully depleted
			UInv_GridHoverManagement::ClearHoverItem(Grid);
			UInv_GridHoverManagement::ShowCursor(Grid);
		}
		else
		{
			// Update the hover item with the remaining stack count
			Grid->HoverItem->UpdateStackCount(Remainder);
		}
	}
}