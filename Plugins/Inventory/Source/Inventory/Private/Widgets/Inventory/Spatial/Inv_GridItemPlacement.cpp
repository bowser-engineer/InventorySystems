#include "Widgets/Inventory/Spatial/Inv_GridItemPlacement.h"
#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Widgets/Inventory/SlottedItems/Inv_SlottedItem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Items/Fragments/Inv_FragmentTags.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Widgets/Utils/Inv_WidgetUtils.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Components/Inv_ItemComponent.h"

FInv_SlotAvailabilityResult UInv_GridItemPlacement::HasRoomForItem(const UInv_InventoryGrid* Grid, const FInv_ItemManifest& Manifest, const int32 StackAmountOverride)
{
	if (!IsValid(Grid)) return FInv_SlotAvailabilityResult{};

	FInv_SlotAvailabilityResult Result;

	const FInv_StackableFragment* StackableFragment = Manifest.GetFragmentOfType<FInv_StackableFragment>();
	Result.bStackable = StackableFragment != nullptr;
	
	const int32 MaxStackSize = StackableFragment ? StackableFragment->GetMaxStackSize() : 1;
	int32 AmountToFill = StackableFragment ? StackableFragment->GetStackCount() : 1;
	if (StackAmountOverride != -1 && Result.bStackable)
	{
		AmountToFill = StackAmountOverride;
	}

	TSet<int32> CheckedIndices;
	for (const auto& GridSlot : Grid->GridSlots)
	{
		if (AmountToFill == 0) break;
		
		if (IsIndexClaimed(CheckedIndices, GridSlot->GetIndex())) continue;

		if (!IsInGridBounds(Grid, GridSlot->GetIndex(), GetItemDimensions(Manifest))) continue;
		
		TSet<int32> TentativelyClaimed;
		if (!HasRoomAtIndex(Grid, GridSlot, GetItemDimensions(Manifest), CheckedIndices, TentativelyClaimed, Manifest.GetItemType(), MaxStackSize))
		{
			continue;
		}
		
		const int32 AmountToFillInSlot = DetermineFillAmountForSlot(Grid, Result.bStackable, MaxStackSize, AmountToFill, GridSlot);
		if (AmountToFillInSlot == 0) continue;

		CheckedIndices.Append(TentativelyClaimed);
		
		Result.TotalRoomToFill += AmountToFillInSlot;
		Result.SlotAvailabilities.Emplace(
			FInv_SlotAvailability{
				HasValidItem(GridSlot) ? GridSlot->GetUpperLeftIndex() : GridSlot->GetIndex(),
				Result.bStackable ? AmountToFillInSlot : 0,
				HasValidItem(GridSlot)
			}
		);

		AmountToFill -= AmountToFillInSlot;
		Result.Remainder = AmountToFill;
		
		if (AmountToFill == 0) return Result;
	}
	
	return Result;
}

bool UInv_GridItemPlacement::HasRoomAtIndex(const UInv_InventoryGrid* Grid, const UInv_GridSlot* GridSlot,
											const FIntPoint& Dimensions, const TSet<int32>& CheckedIndices,
											TSet<int32>& OutTentativelyClaimed, const FGameplayTag& ItemType,
											const int32 MaxStackSize)
{
	if (!IsValid(Grid) || !IsValid(GridSlot)) return false;

	bool bHasRoomAtIndex = true;
	UInv_InventoryStatics::ForEach2D(Grid->GridSlots, GridSlot->GetIndex(), Dimensions, Grid->Columns, [&](const UInv_GridSlot* SubGridSlot)
	{
		if (CheckSlotConstraints(Grid, GridSlot, SubGridSlot, CheckedIndices, OutTentativelyClaimed, ItemType, MaxStackSize))
			OutTentativelyClaimed.Add(SubGridSlot->GetIndex());
		else
			bHasRoomAtIndex = false;
	});

	return bHasRoomAtIndex;
}

bool UInv_GridItemPlacement::CheckSlotConstraints(const UInv_InventoryGrid* Grid, const UInv_GridSlot* GridSlot,
												const UInv_GridSlot* SubGridSlot, const TSet<int32>& CheckedIndices,
												TSet<int32>& OutTentativelyClaimed, const FGameplayTag& ItemType,
												const int32 MaxStackSize)
{
	if (!IsValid(Grid) || !IsValid(GridSlot) || !IsValid(SubGridSlot)) return false;

	if (IsIndexClaimed(CheckedIndices, SubGridSlot->GetIndex())) return false;
	
	if (!HasValidItem(SubGridSlot))
	{
		OutTentativelyClaimed.Add(SubGridSlot->GetIndex());
		return true;
	}

	if (!IsUpperLeftSlot(GridSlot, SubGridSlot)) return false;
	
	const UInv_InventoryItem* SubItem = SubGridSlot->GetInventoryItem().Get();
	if (!SubItem->IsStackable()) return false;
	
	if (!DoesItemTypeMatch(SubItem, ItemType)) return false;
	
	if (GridSlot->GetStackCount() >= MaxStackSize) return false;
	
	return true;
}

void UInv_GridItemPlacement::AddItemAtIndex(UInv_InventoryGrid* Grid, UInv_InventoryItem* Item, const int32 Index, 
											const bool bStackable, const int32 StackAmount)
{
	if (!IsValid(Grid) || !IsValid(Item)) return;

	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(Item, FragmentTags::GridFragment);
	const FInv_ImageFragment* ImageFragment = GetFragment<FInv_ImageFragment>(Item, FragmentTags::IconFragment);
	if (!GridFragment || !ImageFragment) return;

	UInv_SlottedItem* SlottedItem = CreateSlottedItem(Grid, Item, bStackable, StackAmount, GridFragment, ImageFragment, Index);
	AddSlottedItemToCanvas(Grid, Index, GridFragment, SlottedItem);
	
	Grid->SlottedItems.Add(Index, SlottedItem);
}

void UInv_GridItemPlacement::UpdateGridSlots(UInv_InventoryGrid* Grid, UInv_InventoryItem* NewItem, const int32 Index, 
											bool bStackableItem, const int32 StackAmount)
{
	if (!IsValid(Grid) || !IsValid(NewItem) || !Grid->GridSlots.IsValidIndex(Index)) return;

	if (bStackableItem)
	{
		Grid->GridSlots[Index]->SetStackCount(StackAmount);
	}

	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(NewItem, FragmentTags::GridFragment);
	const FIntPoint Dimensions = GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);

	UInv_InventoryStatics::ForEach2D(Grid->GridSlots, Index, Dimensions, Grid->Columns, [&](UInv_GridSlot* GridSlot)
	{
		GridSlot->SetInventoryItem(NewItem);
		GridSlot->SetUpperLeftIndex(Index);
		GridSlot->SetOccupiedTexture();
		GridSlot->SetAvailable(false);
	});
}

void UInv_GridItemPlacement::RemoveItemFromGrid(UInv_InventoryGrid* Grid, UInv_InventoryItem* InventoryItem, const int32 GridIndex)
{
	if (!IsValid(Grid) || !IsValid(InventoryItem)) return;

	const FInv_GridFragment* GridFragment = GetFragment<FInv_GridFragment>(InventoryItem, FragmentTags::GridFragment);
	if (!GridFragment) return;

	UInv_InventoryStatics::ForEach2D(Grid->GridSlots, GridIndex, GridFragment->GetGridSize(), Grid->Columns, [&](UInv_GridSlot* GridSlot)
	{
		GridSlot->SetInventoryItem(nullptr);
		GridSlot->SetUpperLeftIndex(INDEX_NONE);
		GridSlot->SetUnoccupiedTexture();
		GridSlot->SetAvailable(true);
		GridSlot->SetStackCount(0);
	});

	if (Grid->SlottedItems.Contains(GridIndex))
	{
		TObjectPtr<UInv_SlottedItem> FoundSlottedItem;
		Grid->SlottedItems.RemoveAndCopyValue(GridIndex, FoundSlottedItem);
		FoundSlottedItem->RemoveFromParent();
	}
}

bool UInv_GridItemPlacement::IsInGridBounds(const UInv_InventoryGrid* Grid, const int32 StartIndex, const FIntPoint& ItemDimensions)
{
	if (!IsValid(Grid) || StartIndex < 0 || StartIndex >= Grid->GridSlots.Num()) return false;
	
	const int32 EndColumn = (StartIndex % Grid->Columns) + ItemDimensions.X;
	const int32 EndRow = (StartIndex / Grid->Columns) + ItemDimensions.Y;
	return EndColumn <= Grid->Columns && EndRow <= Grid->Rows;
}

FIntPoint UInv_GridItemPlacement::GetItemDimensions(const FInv_ItemManifest& Manifest)
{
	const FInv_GridFragment* GridFragment = Manifest.GetFragmentOfType<FInv_GridFragment>();
	return GridFragment ? GridFragment->GetGridSize() : FIntPoint(1, 1);
}

int32 UInv_GridItemPlacement::DetermineFillAmountForSlot(const UInv_InventoryGrid* Grid, const bool bStackable, 
														const int32 MaxStackSize, const int32 AmountToFill, 
														const UInv_GridSlot* GridSlot)
{
	if (!IsValid(Grid) || !IsValid(GridSlot)) return 0;
	
	const int32 RoomInSlot = MaxStackSize - GetStackAmount(Grid, GridSlot);
	return bStackable ? FMath::Min(AmountToFill, RoomInSlot) : 1;
}

int32 UInv_GridItemPlacement::GetStackAmount(const UInv_InventoryGrid* Grid, const UInv_GridSlot* GridSlot)
{
	if (!IsValid(Grid) || !IsValid(GridSlot)) return 0;

	int32 CurrentSlotStackCount = GridSlot->GetStackCount();
	if (const int32 UpperLeftIndex = GridSlot->GetUpperLeftIndex(); UpperLeftIndex != INDEX_NONE)
	{
		if (Grid->GridSlots.IsValidIndex(UpperLeftIndex))
		{
			UInv_GridSlot* UpperLeftGridSlot = Grid->GridSlots[UpperLeftIndex];
			CurrentSlotStackCount = UpperLeftGridSlot->GetStackCount();
		}
	}
	return CurrentSlotStackCount;
}

UInv_SlottedItem* UInv_GridItemPlacement::CreateSlottedItem(UInv_InventoryGrid* Grid, UInv_InventoryItem* Item, 
															const bool bStackable, const int32 StackAmount, 
															const FInv_GridFragment* GridFragment, 
															const FInv_ImageFragment* ImageFragment, 
															const int32 Index)
{
	if (!IsValid(Grid) || !IsValid(Item)) return nullptr;

	UInv_SlottedItem* SlottedItem = CreateWidget<UInv_SlottedItem>(Grid->GetOwningPlayer(), Grid->SlottedItemClass);
	SlottedItem->SetInventoryItem(Item);
	SetSlottedItemImage(Grid, SlottedItem, GridFragment, ImageFragment);
	SlottedItem->SetGridIndex(Index);
	SlottedItem->SetIsStackable(bStackable);
	const int32 StackUpdateAmount = bStackable ? StackAmount : 0;
	SlottedItem->UpdateStackCount(StackUpdateAmount);
	SlottedItem->OnSlottedItemClicked.AddDynamic(Grid, &UInv_InventoryGrid::OnSlottedItemClicked);
	SlottedItem->OnSlottedItemReleased.AddDynamic(Grid, &UInv_InventoryGrid::OnSlottedItemReleased);

	return SlottedItem;
}

void UInv_GridItemPlacement::AddSlottedItemToCanvas(UInv_InventoryGrid* Grid, const int32 Index, 
													const FInv_GridFragment* GridFragment, 
													UInv_SlottedItem* SlottedItem)
{
	if (!IsValid(Grid) || !IsValid(SlottedItem) || !GridFragment) return;

	Grid->CanvasPanel->AddChild(SlottedItem);
	UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(SlottedItem);
	CanvasSlot->SetSize(GetDrawSize(Grid, GridFragment));
	const FVector2D DrawPos = UInv_WidgetUtils::GetPositionFromIndex(Index, Grid->Columns) * Grid->TileSize;
	const FVector2D DrawPosWithPadding = DrawPos + FVector2D(GridFragment->GetGridPadding());
	CanvasSlot->SetPosition(DrawPosWithPadding);
}

FVector2D UInv_GridItemPlacement::GetDrawSize(const UInv_InventoryGrid* Grid, const FInv_GridFragment* GridFragment)
{
	if (!IsValid(Grid) || !GridFragment) return FVector2D::ZeroVector;

	const float IconTileWidth = Grid->TileSize - GridFragment->GetGridPadding() * 2;
	return GridFragment->GetGridSize() * IconTileWidth;
}

void UInv_GridItemPlacement::SetSlottedItemImage(const UInv_InventoryGrid* Grid, const UInv_SlottedItem* SlottedItem, 
												const FInv_GridFragment* GridFragment, 
												const FInv_ImageFragment* ImageFragment)
{
	if (!IsValid(Grid) || !IsValid(SlottedItem) || !GridFragment || !ImageFragment) return;

	FSlateBrush Brush;
	Brush.SetResourceObject(ImageFragment->GetIcon());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = GetDrawSize(Grid, GridFragment);
	SlottedItem->SetImageBrush(Brush);
}

FInv_SlotAvailabilityResult UInv_GridItemPlacement::HasRoomForItem(const UInv_InventoryGrid* Grid, const UInv_ItemComponent* ItemComponent)
{
	if (!IsValid(Grid) || !IsValid(ItemComponent)) return FInv_SlotAvailabilityResult{};
	return HasRoomForItem(Grid, ItemComponent->GetItemManifest());
}

FInv_SlotAvailabilityResult UInv_GridItemPlacement::HasRoomForItem(const UInv_InventoryGrid* Grid, const UInv_InventoryItem* Item, const int32 StackAmountOverride)
{
	if (!IsValid(Grid) || !IsValid(Item)) return FInv_SlotAvailabilityResult{};
	return HasRoomForItem(Grid, Item->GetItemManifest(), StackAmountOverride);
}

bool UInv_GridItemPlacement::HasValidItem(const UInv_GridSlot* GridSlot)
{
	return IsValid(GridSlot) && GridSlot->GetInventoryItem().IsValid();
}

bool UInv_GridItemPlacement::IsUpperLeftSlot(const UInv_GridSlot* GridSlot, const UInv_GridSlot* SubGridSlot)
{
	return IsValid(GridSlot) && IsValid(SubGridSlot) && SubGridSlot->GetUpperLeftIndex() == GridSlot->GetIndex();
}

bool UInv_GridItemPlacement::DoesItemTypeMatch(const UInv_InventoryItem* SubItem, const FGameplayTag& ItemType)
{
	return IsValid(SubItem) && SubItem->GetItemManifest().GetItemType().MatchesTagExact(ItemType);
}

bool UInv_GridItemPlacement::IsIndexClaimed(const TSet<int32>& CheckedIndices, const int32 Index)
{
	return CheckedIndices.Contains(Index);
}
