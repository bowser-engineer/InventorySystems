#include "Widgets/Inventory/Spatial/Inv_GridCrossOperations.h"
#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Widgets/Inventory/Spatial/Inv_GridItemPlacement.h"
#include "Widgets/Inventory/Spatial/Inv_GridInitialization.h"
#include "Widgets/Inventory/Spatial/Inv_SpatialInventory.h"
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

		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Checking slot %d, HasItem: %s"), 
			ClickedGridIndex, TargetSlot->GetInventoryItem().IsValid() ? TEXT("TRUE") : TEXT("FALSE"));

		if (TargetSlot->GetInventoryItem().IsValid())
		{
			UInv_InventoryItem* TargetItem = TargetSlot->GetInventoryItem().Get();

			UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Target slot %d already has item %s"),
				ClickedGridIndex, *TargetSlot->GetInventoryItem()->GetName());

			if (AreItemsStackable(Item, TargetItem)) 
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

bool UInv_GridCrossOperations::HandleCrossGridStacking(UInv_InventoryGrid* TargetGrid, UInv_HoverItem* LocalHoverItem, int32 TargetIndex)
{
	if (!IsValid(TargetGrid) || !IsValid(LocalHoverItem)) return false;

	UInv_InventoryItem* Item = LocalHoverItem->GetInventoryItem();
	if (!IsValid(Item) || !Item->IsStackable()) return false;

	// Check if the target grid accepts this item category (same validation as non-stackable items)
	if (!MatchesPreferredCategory(TargetGrid, Item)) return false;

	if (const FInv_StackableFragment* StackableFragment = Item->GetItemManifest().GetFragmentOfType<FInv_StackableFragment>()) 
	{
		const int32 MaxStackSize = StackableFragment->GetMaxStackSize();
		const int32 CurrentStack = TargetGrid->GridSlots[TargetIndex]->GetStackCount();
		const int32 RoomInSlot = MaxStackSize - CurrentStack;
		const int32 StackAmount = LocalHoverItem->GetStackCount();

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
				LocalHoverItem->UpdateStackCount(Remainder);
			}
			else 
			{
				// Remove the item from its original position since the entire stack was transferred
				UInv_InventoryGrid* OriginalGrid = LocalHoverItem->GetOwnerGrid();
				int32 OriginalIndex = LocalHoverItem->GetPreviousGridIndex();
				if (IsValid(OriginalGrid) && OriginalGrid->GridSlots.IsValidIndex(OriginalIndex))
				{
					UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Stacking: Removing entire stack from original position: Grid=%s, Index=%d"), 
						*GetNameSafe(OriginalGrid), OriginalIndex);
					UInv_GridItemPlacement::RemoveItemFromGrid(OriginalGrid, Item, OriginalIndex);
				}

				// Set the target grid's SourceGrid to point to the source grid for future transfers
				if (UInv_InventoryGrid* LocalHoverGrid = LocalHoverItem->GetOwnerGrid())
				{
					if (LocalHoverGrid->SourceGrid.IsValid() && LocalHoverGrid->SourceGrid.Get() != LocalHoverGrid)
					{
						TargetGrid->SourceGrid = LocalHoverGrid->SourceGrid;
						UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Stacking: Set TargetGrid SourceGrid to: %s"), *GetNameSafe(LocalHoverGrid->SourceGrid.Get()));
					}
					else
					{
						TargetGrid->SourceGrid = LocalHoverGrid;
						UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Stacking: Set TargetGrid SourceGrid to LocalHoverGrid: %s"), *GetNameSafe(LocalHoverGrid));
					}
				}

				// Clear hover item from the source grid that owns it
				if (UInv_InventoryGrid* LocalHoverGrid = LocalHoverItem->GetOwnerGrid())
				{
					if (LocalHoverGrid->GetHoverItem() == LocalHoverItem)
					{
						LocalHoverGrid->ClearHoverItem();
					}
				}
				else
				{
					// Fallback: find grid with hover item if owner reference is broken
					if (UInv_InventoryGrid* OtherHoverGrid = UInv_GridInitialization::GetGridWithHoverItem(TargetGrid))
					{
						if (OtherHoverGrid->GetHoverItem() == LocalHoverItem)
						{
							OtherHoverGrid->ClearHoverItem();
						}
					}
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

	// If item was previously equipped and is now placed in inventory, trigger unequipping
	bool bWasPreviouslyEquipped = HoverItem->WasPreviouslyEquipped();
	if (bWasPreviouslyEquipped)
	{
		// Find the spatial inventory widget to call the unequip method
		if (UInv_SpatialInventory* SpatialInventory = Cast<UInv_SpatialInventory>(UInv_InventoryStatics::GetInventoryWidget(TargetGrid->GetOwningPlayer())))
		{
			SpatialInventory->OnItemPlacedInInventory(Item);
		}
	}

	// Remove the item from its original position now that it's successfully placed
	// BUT NOT if the item was previously equipped - equipped items don't have an inventory position to remove from
	UInv_InventoryGrid* OriginalGrid = HoverItem->GetOwnerGrid();
	int32 OriginalIndex = HoverItem->GetPreviousGridIndex();
	
	if (!bWasPreviouslyEquipped && IsValid(OriginalGrid) && OriginalGrid->GridSlots.IsValidIndex(OriginalIndex))
	{
		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Removing item from original position: Grid=%s, Index=%d"), 
			*GetNameSafe(OriginalGrid), OriginalIndex);
		UInv_GridItemPlacement::RemoveItemFromGrid(OriginalGrid, Item, OriginalIndex);
	}
	else if (bWasPreviouslyEquipped)
	{
		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Item was previously equipped - not removing from inventory position"));
	}

	// Set the target grid's SourceGrid to point to the source grid for future transfers
	if (UInv_InventoryGrid* LocalHoverGrid = HoverItem->GetOwnerGrid())
	{
		if (LocalHoverGrid->SourceGrid.IsValid() && LocalHoverGrid->SourceGrid.Get() != LocalHoverGrid)
		{
			TargetGrid->SourceGrid = LocalHoverGrid->SourceGrid;
			UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Set TargetGrid SourceGrid to: %s"), *GetNameSafe(LocalHoverGrid->SourceGrid.Get()));
		}
		else
		{
			TargetGrid->SourceGrid = LocalHoverGrid;
			UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Set TargetGrid SourceGrid to LocalHoverGrid: %s"), *GetNameSafe(LocalHoverGrid));
		}
	}

	// Clear hover item from the source grid that owns it
	if (UInv_InventoryGrid* LocalHoverGrid = HoverItem->GetOwnerGrid())
	{
		if (LocalHoverGrid->GetHoverItem() == HoverItem)
		{
			LocalHoverGrid->ClearHoverItem();
		}
	}
	else
	{
		// Fallback: find grid with hover item if owner reference is broken
		if (UInv_InventoryGrid* OtherHoverGrid = UInv_GridInitialization::GetGridWithHoverItem(TargetGrid))
		{
			if (OtherHoverGrid->GetHoverItem() == HoverItem)
			{
				OtherHoverGrid->ClearHoverItem();
			}
		}
	}

	UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] SUCCESS: Item placed and hover cleared"));
	return true;
}

bool UInv_GridCrossOperations::HandleCrossGridSwap(UInv_InventoryGrid* SourceGrid, UInv_InventoryGrid* TargetGrid,
												  UInv_HoverItem* HoverItem, UInv_InventoryItem* TargetItem, 
												  int32 TargetIndex)
{
	if (!IsValid(SourceGrid) || !IsValid(TargetGrid) || !IsValid(HoverItem) || !IsValid(TargetItem))
	{
		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] HandleCrossGridSwap: Invalid parameters"));
		return false;
	}

	UInv_InventoryItem* HoverInvItem = HoverItem->GetInventoryItem();
	if (!IsValid(HoverInvItem))
	{
		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] HandleCrossGridSwap: Invalid hover item"));
		return false;
	}

	// Get item information
	const int32 HoverStackCount = HoverItem->GetStackCount();
	const bool bHoverIsStackable = HoverItem->IsStackable();
	
	// Get target item information
	const int32 TargetStackCount = TargetGrid->GridSlots[TargetIndex]->GetStackCount();
	const bool bTargetIsStackable = TargetItem->IsStackable();

	// Get fragments to determine item dimensions
	const FInv_GridFragment* HoverGridFragment = GetFragment<FInv_GridFragment>(HoverInvItem, FragmentTags::GridFragment);
	const FInv_GridFragment* TargetGridFragment = GetFragment<FInv_GridFragment>(TargetItem, FragmentTags::GridFragment);
	
	if (!HoverGridFragment || !TargetGridFragment)
	{
		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] HandleCrossGridSwap: Missing grid fragments"));
		return false;
	}

	const FIntPoint HoverDimensions = HoverGridFragment->GetGridSize();
	const FIntPoint TargetDimensions = TargetGridFragment->GetGridSize();

	// Find the upper-left index of the target item in its grid
	int32 TargetUpperLeftIndex = TargetIndex;
	if (TargetGrid->GridSlots.IsValidIndex(TargetIndex))
	{
		TargetUpperLeftIndex = TargetGrid->GridSlots[TargetIndex]->GetUpperLeftIndex();
	}

	// Check if hover item can fit in target position
	if (!UInv_GridItemPlacement::IsInGridBounds(TargetGrid, TargetIndex, HoverDimensions))
	{
		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] HandleCrossGridSwap: Hover item doesn't fit at target position"));
		return false;
	}

	// Check if target item can fit in source grid
	FInv_SlotAvailabilityResult SourceAvailability = UInv_GridItemPlacement::HasRoomForItem(SourceGrid, TargetItem, TargetStackCount);
	if (SourceAvailability.TotalRoomToFill <= 0)
	{
		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] HandleCrossGridSwap: Target item doesn't fit in source grid"));
		return false;
	}

	UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] HandleCrossGridSwap: Starting swap between grids"));

	// Step 1: Remove target item from target grid
	UInv_GridItemPlacement::RemoveItemFromGrid(TargetGrid, TargetItem, TargetUpperLeftIndex);

	// Step 2: Place hover item in target grid at target position
	UInv_GridItemPlacement::AddItemAtIndex(TargetGrid, HoverInvItem, TargetIndex, bHoverIsStackable, HoverStackCount);
	UInv_GridItemPlacement::UpdateGridSlots(TargetGrid, HoverInvItem, TargetIndex, bHoverIsStackable, HoverStackCount);

	// Step 2.5: Remove hover item from its original position
	UInv_InventoryGrid* OriginalGrid = HoverItem->GetOwnerGrid();
	int32 OriginalIndex = HoverItem->GetPreviousGridIndex();
	if (IsValid(OriginalGrid) && OriginalGrid->GridSlots.IsValidIndex(OriginalIndex))
	{
		UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Swap: Removing hover item from original position: Grid=%s, Index=%d"), 
			*GetNameSafe(OriginalGrid), OriginalIndex);
		UInv_GridItemPlacement::RemoveItemFromGrid(OriginalGrid, HoverInvItem, OriginalIndex);
	}

	// Step 3: Place target item in source grid using availability result
	SourceAvailability.Item = TargetItem;
	SourceGrid->AddStacks(SourceAvailability);

	// Step 4: Clear hover item with proper ownership handling
	if (UInv_InventoryGrid* HoverGrid = UInv_GridInitialization::GetGridWithHoverItem(TargetGrid))
	{
		// Ensure we're clearing from the correct grid
		if (HoverGrid->GetHoverItem() == HoverItem)
		{
			HoverGrid->ClearHoverItem();
		}
	}

	UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] HandleCrossGridSwap: Swap completed successfully"));
	return true;
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
	
	// Backpack and Locked grids accept any item category
	if (Grid->ItemCategory == EInv_ItemCategory::Backpack || Grid->ItemCategory == EInv_ItemCategory::Locked) 
	{
		UE_LOG(LogInventory, Warning, TEXT("Grid %s accepts any category - allowing transfer"), *UEnum::GetValueAsString(Grid->ItemCategory));
		return true;
	}

	// Satchel and Quiver grids only accept their specific preferred category
	UE_LOG(LogInventory, Warning, TEXT("Matching Preferred %s to %s"),
		*UEnum::GetValueAsString(Item->GetItemManifest().GetItemCategory()), 
		*UEnum::GetValueAsString(Grid->ItemCategory));

	bool bMatches = Item->GetItemManifest().GetItemCategory() == Grid->ItemCategory;
	UE_LOG(LogInventory, Warning, TEXT("Matching Preference: %s"), bMatches ? TEXT("true") : TEXT("false"));

	return bMatches;
}

bool UInv_GridCrossOperations::AreItemsStackable(const UInv_InventoryItem* Item1, const UInv_InventoryItem* Item2)
{
	if (!IsValid(Item1) || !IsValid(Item2)) return false;
	
	// Both items must be stackable
	if (!Item1->IsStackable() || !Item2->IsStackable()) return false;
	
	// Items must be of the same type (exact tag match)
	return Item1->GetItemManifest().GetItemType().MatchesTagExact(Item2->GetItemManifest().GetItemType());
}