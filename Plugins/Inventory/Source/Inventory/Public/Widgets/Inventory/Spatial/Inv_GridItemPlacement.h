#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <Types/Inv_GridTypes.h>
#include "Inv_InventoryGrid.h"
#include "Inv_GridItemPlacement.generated.h"

class UInv_InventoryGrid;
class UInv_InventoryItem;
class UInv_ItemComponent;
class UInv_GridSlot;
class UInv_SlottedItem;
struct FInv_GridFragment;
struct FInv_ImageFragment;
struct FInv_ItemManifest;

UCLASS(BlueprintType)
class INVENTORY_API UInv_GridItemPlacement : public UObject
{
	GENERATED_BODY()

public:
	static FInv_SlotAvailabilityResult HasRoomForItem(const UInv_InventoryGrid* Grid, const UInv_ItemComponent* ItemComponent);
	
	static FInv_SlotAvailabilityResult HasRoomForItem(const UInv_InventoryGrid* Grid, const UInv_InventoryItem* Item, const int32 StackAmountOverride = -1);
	
	static FInv_SlotAvailabilityResult HasRoomForItem(const UInv_InventoryGrid* Grid, const FInv_ItemManifest& Manifest, const int32 StackAmountOverride = -1);

	static bool HasRoomAtIndex(const UInv_InventoryGrid* Grid, const UInv_GridSlot* GridSlot, const FIntPoint& Dimensions, 
							  const TSet<int32>& CheckedIndices, TSet<int32>& OutTentativelyClaimed, 
							  const FGameplayTag& ItemType, const int32 MaxStackSize);

	static bool CheckSlotConstraints(const UInv_InventoryGrid* Grid, const UInv_GridSlot* GridSlot, const UInv_GridSlot* SubGridSlot, 
									const TSet<int32>& CheckedIndices, TSet<int32>& OutTentativelyClaimed, 
									const FGameplayTag& ItemType, const int32 MaxStackSize);

	static void AddItemAtIndex(UInv_InventoryGrid* Grid, UInv_InventoryItem* Item, const int32 Index, const bool bStackable, const int32 StackAmount);
	
	static void UpdateGridSlots(UInv_InventoryGrid* Grid, UInv_InventoryItem* NewItem, const int32 Index, bool bStackableItem, const int32 StackAmount);
	
	static void RemoveItemFromGrid(UInv_InventoryGrid* Grid, UInv_InventoryItem* InventoryItem, const int32 GridIndex);

	static bool IsInGridBounds(const UInv_InventoryGrid* Grid, const int32 StartIndex, const FIntPoint& ItemDimensions);

private:
	static FIntPoint GetItemDimensions(const FInv_ItemManifest& Manifest);
	static bool HasValidItem(const UInv_GridSlot* GridSlot);
	static bool IsUpperLeftSlot(const UInv_GridSlot* GridSlot, const UInv_GridSlot* SubGridSlot);
	static bool DoesItemTypeMatch(const UInv_InventoryItem* SubItem, const FGameplayTag& ItemType);
	static bool IsIndexClaimed(const TSet<int32>& CheckedIndices, const int32 Index);
	static int32 DetermineFillAmountForSlot(const UInv_InventoryGrid* Grid, const bool bStackable, const int32 MaxStackSize, const int32 AmountToFill, const UInv_GridSlot* GridSlot);
	static int32 GetStackAmount(const UInv_InventoryGrid* Grid, const UInv_GridSlot* GridSlot);

	static UInv_SlottedItem* CreateSlottedItem(UInv_InventoryGrid* Grid, UInv_InventoryItem* Item, const bool bStackable, 
											  const int32 StackAmount, const FInv_GridFragment* GridFragment, 
											  const FInv_ImageFragment* ImageFragment, const int32 Index);
	
	static void AddSlottedItemToCanvas(UInv_InventoryGrid* Grid, const int32 Index, const FInv_GridFragment* GridFragment, UInv_SlottedItem* SlottedItem);
	
	static FVector2D GetDrawSize(const UInv_InventoryGrid* Grid, const FInv_GridFragment* GridFragment);
	
	static void SetSlottedItemImage(const UInv_InventoryGrid* Grid, const UInv_SlottedItem* SlottedItem, const FInv_GridFragment* GridFragment, const FInv_ImageFragment* ImageFragment);
};