#include "Widgets/Inventory/Spatial/Inv_GridCrossOperations.h"
#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Widgets/Inventory/Spatial/Inv_GridItemPlacement.h"
#include "Widgets/Inventory/Spatial/Inv_GridInitialization.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Items/Fragments/Inv_FragmentTags.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Inventory.h"
#include "Widgets/Inventory/SlottedItems/Inv_SlottedItem.h"
#include "Items/Inv_InventoryItem.h"

bool UInv_GridCrossOperations::CanAcceptFromGrid(UInv_InventoryGrid* TargetGrid, UInv_InventoryGrid* SourceGrid, 
												UInv_InventoryItem* Item, int32 StackAmount)
{
	if (!IsValid(TargetGrid) || !IsValid(SourceGrid) || !IsValid(Item)) return false;

	if (!MatchesPreferredCategory(TargetGrid, Item)) return false;

	FInv_SlotAvailabilityResult Result = UInv_GridItemPlacement::HasRoomForItem(TargetGrid, Item, StackAmount);
	return Result.TotalRoomToFill > 0;
}

bool UInv_GridCrossOperations::TransferFromGrid(UInv_InventoryGrid* TargetGrid, UInv_InventoryGrid* SourceGrid, 
											   UInv_InventoryItem* Item, int32 StackAmount)
{
	if (!CanAcceptFromGrid(TargetGrid, SourceGrid, Item, StackAmount)) return false;

	FInv_SlotAvailabilityResult Result = UInv_GridItemPlacement::HasRoomForItem(TargetGrid, Item, StackAmount);
	Result.Item = Item;

	TargetGrid->AddStacks(Result);
	UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] TransferFromGrid: Item %s transferred to grid %s with stack amount %d"),
		*Item->GetName(), *TargetGrid->GetName(), StackAmount);
	return true;
}

bool UInv_GridCrossOperations::HandleCrossGridTransfer(UInv_InventoryGrid* TargetGrid, UInv_InventoryGrid* SourceGrid, 
													  UInv_HoverItem* HoverItem, int32 ClickedGridIndex)
{
	if (!IsValid(TargetGrid) || !IsValid(SourceGrid) || !IsValid(HoverItem)) return false;

	UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] HandleCrossGridTransfer: GridIndex=%d"), ClickedGridIndex);

	UInv_InventoryItem* Item = HoverItem->GetInventoryItem();
	int32 StackAmount = HoverItem->GetStackCount();

	if (!CanAcceptFromGrid(TargetGrid, SourceGrid, Item, StackAmount))
	{
		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Cannot transfer item to this grid - category mismatch or no space"));
		return false;
	}

	if (TargetGrid->GridSlots.IsValidIndex(ClickedGridIndex))
	{
		UInv_GridSlot* TargetSlot = TargetGrid->GridSlots[ClickedGridIndex];

		if (TargetSlot->GetInventoryItem().IsValid())
		{
			UInv_InventoryItem* TargetItem = TargetSlot->GetInventoryItem().Get();

			UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Target slot %d already has item %s"),
				ClickedGridIndex, *TargetSlot->GetInventoryItem()->GetName());

			if (Item == TargetItem && Item->IsStackable()) 
			{
				return HandleCrossGridStacking(TargetGrid, HoverItem, ClickedGridIndex);
			}
			else
			{
				if (CanAcceptFromGrid(SourceGrid, TargetGrid, TargetItem))
				{
					return HandleCrossGridSwap(SourceGrid, TargetGrid, HoverItem, TargetItem, ClickedGridIndex);
				}
			}
		}
		else
		{
			UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Target slot %d empty, placing item"), ClickedGridIndex);
			return PlaceItemFromOtherGrid(TargetGrid, SourceGrid, HoverItem, ClickedGridIndex);
		}
	}

	return false;
}

bool UInv_GridCrossOperations::HandleCrossGridStacking(UInv_InventoryGrid* TargetGrid, UInv_HoverItem* HoverItem, int32 TargetIndex)
{
	if (!IsValid(TargetGrid) || !IsValid(HoverItem)) return false;

	UInv_InventoryItem* Item = HoverItem->GetInventoryItem();
	if (!IsValid(Item) || !Item->IsStackable()) return false;

	if (const FInv_StackableFragment* StackableFragment = Item->GetItemManifest().GetFragmentOfType<FInv_StackableFragment>()) 
	{
		const int32 MaxStackSize = StackableFragment->GetMaxStackSize();
		const int32 CurrentStack = TargetGrid->GridSlots[TargetIndex]->GetStackCount();
		const int32 RoomInSlot = MaxStackSize - CurrentStack;
		const int32 StackAmount = HoverItem->GetStackCount();

		if (RoomInSlot > 0) 
		{
			const int32 AmountToTransfer = FMath::Min(StackAmount, RoomInSlot);
			const int32 NewStackCount = CurrentStack + AmountToTransfer;

			TargetGrid->GridSlots[TargetIndex]->SetStackCount(NewStackCount);
			if (TargetGrid->SlottedItems.Contains(TargetIndex)) 
			{
				TargetGrid->SlottedItems[TargetIndex]->UpdateStackCount(NewStackCount);
			}

			const int32 Remainder = StackAmount - AmountToTransfer;

			if (Remainder > 0) 
			{
				HoverItem->UpdateStackCount(Remainder);
			}
			else 
			{
				if (UInv_InventoryGrid* HoverGrid = UInv_GridInitialization::GetGridWithHoverItem(TargetGrid)) 
				{
					HoverGrid->ClearHoverItem();
				}
			}
			return true;
		}
	}

	return false;
}

bool UInv_GridCrossOperations::PlaceItemFromOtherGrid(UInv_InventoryGrid* TargetGrid, UInv_InventoryGrid* SourceGrid, 
													 UInv_HoverItem* HoverItem, int32 GridIndex)
{
	if (!TargetGrid->GridSlots.IsValidIndex(GridIndex) || !IsValid(HoverItem) || !IsValid(SourceGrid)) 
	{
		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Invalid parameters for placing item: GridIndex=%d, HoverItem=%s, SourceGrid=%s"),
			GridIndex, *GetNameSafe(HoverItem), *GetNameSafe(SourceGrid));
		return false;
	}

	UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Placing item at index %d"), GridIndex);

	UInv_InventoryItem* Item = HoverItem->GetInventoryItem();
	int32 StackAmount = HoverItem->GetStackCount();
	bool bIsStackable = HoverItem->IsStackable();

	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(Item, FragmentTags::GridFragment);
	if (!GridFragment) 
	{
		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] FAIL: Grid Fragment is false"));
		return false;
	}

	const FIntPoint ItemDimensions = GridFragment->GetGridSize();

	if (!UInv_GridItemPlacement::IsInGridBounds(TargetGrid, GridIndex, ItemDimensions)) 
	{
		if (!UInv_GridItemPlacement::IsInGridBounds(TargetGrid, TargetGrid->ItemDropIndex, ItemDimensions)) 
		{
			UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] FAIL: Out of bounds for start index %d"), GridIndex);
			return false;
		}
		GridIndex = TargetGrid->ItemDropIndex;
	}

	bool bAllSlotsFree = true;
	UInv_InventoryStatics::ForEach2D(TargetGrid->GridSlots, GridIndex, ItemDimensions, TargetGrid->Columns, [&](const UInv_GridSlot* GridSlot) 
	{
		if (GridSlot->GetInventoryItem().IsValid()) 
		{
			bAllSlotsFree = false;
		}
	});

	if (!bAllSlotsFree) 
	{
		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] FAIL: Required slots not free for index %d"), GridIndex);
		return false;
	}

	UInv_GridItemPlacement::AddItemAtIndex(TargetGrid, Item, GridIndex, bIsStackable, StackAmount);
	UInv_GridItemPlacement::UpdateGridSlots(TargetGrid, Item, GridIndex, bIsStackable, StackAmount);

	if (UInv_InventoryGrid* HoverGrid = UInv_GridInitialization::GetGridWithHoverItem(TargetGrid)) 
	{
		HoverGrid->ClearHoverItem();
	}

	UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] SUCCESS: Item placed and hover cleared"));
	return true;
}

bool UInv_GridCrossOperations::HandleCrossGridSwap(UInv_InventoryGrid* SourceGrid, UInv_InventoryGrid* TargetGrid,
												  UInv_HoverItem* HoverItem, UInv_InventoryItem* TargetItem, 
												  int32 TargetIndex)
{
	UE_LOG(LogInventory, Warning, TEXT("Cross-grid item swapping not yet implemented"));
	return false;
}

bool UInv_GridCrossOperations::MatchesCategory(const UInv_InventoryGrid* Grid, const UInv_InventoryItem* Item)
{
	if (!IsValid(Grid) || !IsValid(Item)) return false;
	
	UE_LOG(LogInventory, Warning, TEXT("Matching %s to %s"),
		*UEnum::GetValueAsString(Item->GetItemManifest().GetItemCategory()), 
		*UEnum::GetValueAsString(Grid->ItemCategory));

	return Item->GetItemManifest().GetItemCategory() == Grid->ItemCategory;
}

bool UInv_GridCrossOperations::MatchesPreferredCategory(const UInv_InventoryGrid* Grid, const UInv_InventoryItem* Item)
{
	if (!IsValid(Grid) || !IsValid(Item)) return false;
	
	if (Grid->ItemCategory == EInv_ItemCategory::Backpack || Grid->ItemCategory == EInv_ItemCategory::Locked) return true;

	UE_LOG(LogInventory, Warning, TEXT("Matching Preferred %s to %s"),
		*UEnum::GetValueAsString(Item->GetItemManifest().GetPreferredItemCategory()), 
		*UEnum::GetValueAsString(Grid->ItemCategory));

	UE_LOG(LogInventory, Warning, TEXT("Matching Preference: %s"),
			Item->GetItemManifest().GetItemCategory() == Grid->ItemCategory ? TEXT("true") : TEXT("false"));


	return Item->GetItemManifest().GetItemCategory() == Grid->ItemCategory;
}