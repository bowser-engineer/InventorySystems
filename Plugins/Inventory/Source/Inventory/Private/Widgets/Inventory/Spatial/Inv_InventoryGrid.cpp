#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Widgets/Inventory/Spatial/Inv_GridCalculations.h"
#include "Widgets/Inventory/Spatial/Inv_GridCrossOperations.h"
#include "Widgets/Inventory/Spatial/Inv_GridHighlighting.h"
#include "Widgets/Inventory/Spatial/Inv_GridHoverManagement.h"
#include "Widgets/Inventory/Spatial/Inv_GridInitialization.h"
#include "Widgets/Inventory/Spatial/Inv_GridItemPlacement.h"
#include "Widgets/Inventory/Spatial/Inv_GridPopupInteractions.h"

#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"
#include "Widgets/Inventory/SlottedItems/Inv_SlottedItem.h"
#include "Widgets/ItemPopUp/Inv_ItemPopUp.h"

#include "Widgets/Utils/Inv_WidgetUtils.h"

#include "Inventory.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"

#include "Items/Components/Inv_ItemComponent.h"
#include "Items/Fragments/Inv_FragmentTags.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Manifest/Inv_ItemManifest.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"


void UInv_InventoryGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UInv_GridInitialization::InitializeGrid(this);
}

void UInv_InventoryGrid::NativeDestruct()
{
	// Check if HoverItem is still valid and put it back
	if (IsValid(HoverItem))
	{
		UInv_GridHoverManagement::PutHoverItemBack(this);
		HoverItem = nullptr;
	}
	UInv_GridInitialization::CleanupGrid(this);
	Super::NativeDestruct();
}

void UInv_InventoryGrid::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const FVector2D CanvasPosition = UInv_WidgetUtils::GetWidgetPosition(CanvasPanel);
	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer());

	UInv_InventoryGrid* GridWithHoverItem = UInv_GridInitialization::GetGridWithHoverItem(this);
	if (GridWithHoverItem && GridWithHoverItem != this)
	{
		const FVector2D ThisCanvasPos = UInv_WidgetUtils::GetWidgetPosition(CanvasPanel);
		const FVector2D ThisCanvasSize = UInv_WidgetUtils::GetWidgetSize(CanvasPanel);

		if (UInv_WidgetUtils::IsWithinBounds(ThisCanvasPos, ThisCanvasSize, MousePosition))
		{
			bMouseWithinCanvas = true;
			UInv_GridCalculations::UpdateTileParameters(this, ThisCanvasPos, MousePosition);
			HoverItem = GridWithHoverItem->GetHoverItem();
			OnTileParametersUpdated(TileParameters);
			return;
		}
		else
		{
			UInv_GridHighlighting::UnHighlightSlots(this, LastHighlightedIndex, LastHighlightedDimensions);
			return;
		}
	}

	if (UInv_GridCalculations::CursorExitedCanvas(this, CanvasPosition, UInv_WidgetUtils::GetWidgetSize(CanvasPanel), MousePosition))
	{
		UInv_GridHighlighting::UnHighlightSlots(this, LastHighlightedIndex, LastHighlightedDimensions);
		return;
	}

	UInv_GridCalculations::UpdateTileParameters(this, CanvasPosition, MousePosition);
}

void UInv_InventoryGrid::OnTileParametersUpdated(const FInv_TileParameters& Parameters)
{
	if (!IsValid(HoverItem)) return;

	const FIntPoint Dimensions = HoverItem->GetGridDimensions();
	const FIntPoint StartingCoordinate = UInv_GridCalculations::CalculateStartingCoordinate(Parameters.TileCoordinats, Dimensions, Parameters.TileQuadrant);
	ItemDropIndex = UInv_WidgetUtils::GetIndexFromPosition(StartingCoordinate, Columns);

	CurrentQueryResult = CheckHoverPosition(StartingCoordinate, Dimensions);

	if (CurrentQueryResult.bHasSpace)
	{
		UInv_GridHighlighting::HighlightSlots(this, ItemDropIndex, Dimensions);
		return;
	}
	UInv_GridHighlighting::UnHighlightSlots(this, LastHighlightedIndex, LastHighlightedDimensions);

	if (CurrentQueryResult.ValidItem.IsValid() && GridSlots.IsValidIndex(CurrentQueryResult.UpperLeftIndex))
	{
		const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(CurrentQueryResult.ValidItem.Get(), FragmentTags::GridFragment);
		if (!GridFragment) return;

		UInv_GridHighlighting::ChangeHoverType(this, CurrentQueryResult.UpperLeftIndex, GridFragment->GetGridSize(), EInv_GridSlotState::GrayedOut);
	}
}

FInv_SpaceQueryResult UInv_InventoryGrid::CheckHoverPosition(const FIntPoint& Position, const FIntPoint& Dimensions)
{
	FInv_SpaceQueryResult Result;

	if (!UInv_GridItemPlacement::IsInGridBounds(this, UInv_WidgetUtils::GetIndexFromPosition(Position, Columns), Dimensions)) return Result;

	Result.bHasSpace = true;

	TSet<int32> OccupiedUpperLeftIndices;
	UInv_InventoryStatics::ForEach2D(GridSlots, UInv_WidgetUtils::GetIndexFromPosition(Position, Columns), Dimensions, Columns, [&](const UInv_GridSlot* GridSlot)
		{
			if (GridSlot->GetInventoryItem().IsValid())
			{
				OccupiedUpperLeftIndices.Add(GridSlot->GetUpperLeftIndex());
				Result.bHasSpace = false;
			}
		});

	if (OccupiedUpperLeftIndices.Num() == 1)
	{
		const int32 Index = *OccupiedUpperLeftIndices.CreateConstIterator();
		Result.ValidItem = GridSlots[Index]->GetInventoryItem();
		Result.UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	}

	return Result;
}

void UInv_InventoryGrid::AddStacks(const FInv_SlotAvailabilityResult& Result)
{
	if (!UInv_GridCrossOperations::MatchesCategory(this, Result.Item.Get())) return;

	for (const auto& Availability : Result.SlotAvailabilities)
	{
		if (Availability.bItemAtIndex)
		{
			const auto& GridSlot = GridSlots[Availability.Index];
			const auto& SlottedItem = SlottedItems.FindChecked(Availability.Index);
			SlottedItem->UpdateStackCount(GridSlot->GetStackCount() + Availability.AmountToFill);
			GridSlot->SetStackCount(GridSlot->GetStackCount() + Availability.AmountToFill);
		}
		else
		{
			UInv_GridItemPlacement::AddItemAtIndex(this, Result.Item.Get(), Availability.Index, Result.bStackable, Availability.AmountToFill);
			UInv_GridItemPlacement::UpdateGridSlots(this, Result.Item.Get(), Availability.Index, Result.bStackable, Availability.AmountToFill);
		}
	}
}

void UInv_InventoryGrid::OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	UInv_InventoryStatics::ItemUnhovered(GetOwningPlayer());

	check(GridSlots.IsValidIndex(GridIndex));
	UInv_InventoryItem* ClickedInventoryItem = GridSlots[GridIndex]->GetInventoryItem().Get();

	if (!IsValid(HoverItem) && UInv_GridPopupInteractions::IsLeftClick(MouseEvent))
	{
		UInv_GridHoverManagement::PickUp(this, ClickedInventoryItem, GridIndex);
		return;
	}

	if (UInv_GridPopupInteractions::IsRightClick(MouseEvent))
	{
		UInv_GridPopupInteractions::CreateItemPopUp(this, GridIndex);
		return;
	}

	if (UInv_GridPopupInteractions::IsSameStackable(this, ClickedInventoryItem))
	{
		HandleStackableItemInteraction(ClickedInventoryItem, GridIndex);
		return;
	}

	if (CurrentQueryResult.ValidItem.IsValid())
	{
		SwapWithHoverItem(ClickedInventoryItem, GridIndex);
	}
}

void UInv_InventoryGrid::HandleStackableItemInteraction(UInv_InventoryItem* ClickedInventoryItem, int32 GridIndex)
{
	const int32 ClickedStackCount = GridSlots[GridIndex]->GetStackCount();
	const FInv_StackableFragment* StackableFragment = ClickedInventoryItem->GetItemManifest().GetFragmentOfType<FInv_StackableFragment>();
	const int32 MaxStackSize = StackableFragment->GetMaxStackSize();
	const int32 RoomInClickedSlot = MaxStackSize - ClickedStackCount;
	const int32 HoveredStackCount = HoverItem->GetStackCount();

	if (UInv_GridPopupInteractions::ShouldSwapStackCounts(RoomInClickedSlot, HoveredStackCount, MaxStackSize))
	{
		UInv_GridPopupInteractions::SwapStackCounts(this, ClickedStackCount, HoveredStackCount, GridIndex);
		return;
	}

	if (UInv_GridPopupInteractions::ShouldConsumeHoverItemStacks(HoveredStackCount, RoomInClickedSlot))
	{
		UInv_GridPopupInteractions::ConsumeHoverItemStacks(this, ClickedStackCount, HoveredStackCount, GridIndex);
		return;
	}

	if (UInv_GridPopupInteractions::ShouldFillInStack(RoomInClickedSlot, HoveredStackCount))
	{
		UInv_GridPopupInteractions::FillInStack(this, RoomInClickedSlot, HoveredStackCount - RoomInClickedSlot, GridIndex);
		return;
	}
}

bool UInv_InventoryGrid::HasHoverItem() const
{
	return IsValid(HoverItem);
}

UInv_HoverItem* UInv_InventoryGrid::GetHoverItem() const
{
	return HoverItem;
}


void UInv_InventoryGrid::AddItem(UInv_InventoryItem* Item)
{
	if (!UInv_GridCrossOperations::MatchesCategory(this, Item)) return;

	FInv_SlotAvailabilityResult Result = UInv_GridItemPlacement::HasRoomForItem(this, Item);
	AddItemToIndices(Result, Item);
}

void UInv_InventoryGrid::AddItemToIndices(const FInv_SlotAvailabilityResult& Result, UInv_InventoryItem* NewItem)
{
	for (const auto& Availability : Result.SlotAvailabilities)
	{
		UInv_GridItemPlacement::AddItemAtIndex(this, NewItem, Availability.Index, Result.bStackable, Availability.AmountToFill);
		UInv_GridItemPlacement::UpdateGridSlots(this, NewItem, Availability.Index, Result.bStackable, Availability.AmountToFill);
	}
}

void UInv_InventoryGrid::SetOwningCanvas(UCanvasPanel* OwningCanvas)
{
	OwningCanvasPanel = OwningCanvas;
}

void UInv_InventoryGrid::OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	UInv_InventoryGrid* GridWithHoverItem = UInv_GridInitialization::GetGridWithHoverItem(this);

	UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] GridWithHoverItem: %s, This: %s, Same: %s"),
		GridWithHoverItem ? *GridWithHoverItem->GetName() : TEXT("NULL"),
		*GetName(),
		(GridWithHoverItem == this) ? TEXT("TRUE") : TEXT("FALSE"));

	if (GridWithHoverItem && GridWithHoverItem != this)
	{
		UInv_HoverItem* OtherHoverItem = GridWithHoverItem->GetHoverItem();

		if (!IsValid(OtherHoverItem)) return;

		UInv_InventoryItem* HoverInvItem = OtherHoverItem->GetInventoryItem();
		if (!IsValid(HoverInvItem))
		{
			GridWithHoverItem->ClearHoverItem();
			return;
		}

		if (UInv_GridCrossOperations::HandleCrossGridTransfer(this, GridWithHoverItem, OtherHoverItem, GridIndex))
		{
			UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Item transfer successful"));
			return;
		}
		else
		{
			UE_LOG(LogInventory, Warning, TEXT("[CrossGrid] Item transfer failed - returning early"));
			return;
		}
	}

	if (!IsValid(HoverItem)) return;
	if (!GridSlots.IsValidIndex(ItemDropIndex)) return;

	UE_LOG(LogInventory, Warning, TEXT("OnGridSlotClicked: GridIndex=%d, ItemDropIndex=%d"), GridIndex, ItemDropIndex);

	if (CurrentQueryResult.ValidItem.IsValid() && GridSlots.IsValidIndex(CurrentQueryResult.UpperLeftIndex))
	{
		OnSlottedItemClicked(CurrentQueryResult.UpperLeftIndex, MouseEvent);
		return;
	}

	if (!UInv_GridItemPlacement::IsInGridBounds(this, ItemDropIndex, HoverItem->GetGridDimensions())) return;

	auto GridSlot = GridSlots[ItemDropIndex];
	if (!GridSlot->GetInventoryItem().IsValid())
	{
		UInv_GridHoverManagement::PutDownOnIndex(this, ItemDropIndex);
	}
}

void UInv_InventoryGrid::OnInventoryMenuToggled(bool bOpen)
{
	if (!bOpen)
	{
		UInv_GridHoverManagement::PutHoverItemBack(this);
	}
}

UUserWidget* UInv_InventoryGrid::GetVisibleCursorWidget()
{
	if (!IsValid(GetOwningPlayer())) return nullptr;
	if (!IsValid(VisibleCursorWidget))
	{
		VisibleCursorWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), VisibleCursorWidgetClass);
	}
	return VisibleCursorWidget;
}

UUserWidget* UInv_InventoryGrid::GetHiddenCursorWidget()
{
	if (!IsValid(GetOwningPlayer())) return nullptr;
	if (!IsValid(HiddenCursorWidget))
	{
		HiddenCursorWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), HiddenCursorWidgetClass);
	}
	return HiddenCursorWidget;
}

void UInv_InventoryGrid::SwapWithHoverItem(UInv_InventoryItem* ClickedInventoryItem, const int32 GridIndex)
{
	if (!IsValid(HoverItem)) return;

	UInv_InventoryItem* TempInventoryItem = HoverItem->GetInventoryItem();
	const int32 TempStackCount = HoverItem->GetStackCount();
	const bool bTempIsStackable = HoverItem->IsStackable();

	UInv_GridHoverManagement::AssignHoverItem(this, ClickedInventoryItem, GridIndex, HoverItem->GetPreviousGridIndex());
	UInv_GridItemPlacement::RemoveItemFromGrid(this, ClickedInventoryItem, GridIndex);
	UInv_GridItemPlacement::AddItemAtIndex(this, TempInventoryItem, ItemDropIndex, bTempIsStackable, TempStackCount);
	UInv_GridItemPlacement::UpdateGridSlots(this, TempInventoryItem, ItemDropIndex, bTempIsStackable, TempStackCount);
}

/************************************************************************************
 *  INVENTORY GRID - USER INTERACTION & ITEM HANDLING
 *
 *  This section defines the UInv_InventoryGrid methods responsible for:
 *    • Hover handling — showing, clearing, and restoring hover items.
 *    • Slot highlighting — responding to grid slot hover/unhover events.
 *    • Popup menu actions — splitting stacks, dropping items, and consuming items.
 *    • Category checks — validating item category compatibility with this grid.
 *    • Item placement checks — determining if the grid has enough room for an item.
 *
 *  Implementation notes:
 *    - All logic is delegated to specialized subsystem classes such as:
 *        UInv_GridHoverManagement, UInv_GridHighlighting,
 *        UInv_GridPopupInteractions, UInv_GridCrossOperations,
 *        and UInv_GridItemPlacement.
 *    - This keeps UInv_InventoryGrid lean and focused on event forwarding.
 ************************************************************************************/


void UInv_InventoryGrid::OnHide()
{
	UInv_GridHoverManagement::PutHoverItemBack(this);
}

void UInv_InventoryGrid::ClearHoverItem()
{
	UInv_GridHoverManagement::ClearHoverItem(this);
}

void UInv_InventoryGrid::OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	UInv_GridHighlighting::OnGridSlotHovered(this, GridIndex);
}

void UInv_InventoryGrid::OnGridSlotUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent)
{
	UInv_GridHighlighting::OnGridSlotUnhovered(this, GridIndex);
}

void UInv_InventoryGrid::OnPopUpMenuSplit(int32 SplitAmount, int32 Index)
{
	UInv_GridPopupInteractions::OnPopUpMenuSplit(this, SplitAmount, Index);
}

void UInv_InventoryGrid::OnPopUpMenuDrop(int32 Index)
{
	UInv_GridPopupInteractions::OnPopUpMenuDrop(this, Index);
}

void UInv_InventoryGrid::OnPopUpMenuConsume(int32 Index)
{
	UInv_GridPopupInteractions::OnPopUpMenuConsume(this, Index);
}

void UInv_InventoryGrid::HighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	UInv_GridHighlighting::HighlightSlots(this, Index, Dimensions);
}

bool UInv_InventoryGrid::MatchesCategory(const UInv_InventoryItem* Item) const
{
	return UInv_GridCrossOperations::MatchesCategory(this, Item);
}

bool UInv_InventoryGrid::MatchesPreferredCategory(const UInv_InventoryItem* Item) const
{
	return UInv_GridCrossOperations::MatchesPreferredCategory(this, Item);
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const UInv_ItemComponent* ItemComponent)
{
	return UInv_GridItemPlacement::HasRoomForItem(this, ItemComponent);
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const UInv_InventoryItem* Item, const int32 StackAmountOverride)
{
	return UInv_GridItemPlacement::HasRoomForItem(this, Item, StackAmountOverride);
}

FInv_SlotAvailabilityResult UInv_InventoryGrid::HasRoomForItem(const FInv_ItemManifest& Manifest, const int32 StackAmountOverride)
{
	return UInv_GridItemPlacement::HasRoomForItem(this, Manifest, StackAmountOverride);
}