#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Inv_GridCrossOperations.generated.h"

class UInv_InventoryGrid;
class UInv_InventoryItem;
class UInv_HoverItem;

UCLASS(BlueprintType)
class INVENTORY_API UInv_GridCrossOperations : public UObject
{
	GENERATED_BODY()

public:
	static bool CanAcceptFromGrid(UInv_InventoryGrid* TargetGrid, UInv_InventoryGrid* SourceGrid, UInv_InventoryItem* Item, int32 StackAmount = -1);
	
	static bool TransferFromGrid(UInv_InventoryGrid* TargetGrid, UInv_InventoryGrid* SourceGrid, UInv_InventoryItem* Item, int32 StackAmount = -1);
	
	static bool HandleCrossGridTransfer(UInv_InventoryGrid* TargetGrid, UInv_InventoryGrid* SourceGrid, UInv_HoverItem* HoverItem, int32 ClickedGridIndex);
	
	static bool HandleCrossGridStacking(UInv_InventoryGrid* TargetGrid, UInv_HoverItem* HoverItem, int32 TargetIndex);
	
	static bool PlaceItemFromOtherGrid(UInv_InventoryGrid* TargetGrid, UInv_InventoryGrid* SourceGrid, UInv_HoverItem* HoverItem, int32 GridIndex);
	
	static bool HandleCrossGridSwap(UInv_InventoryGrid* SourceGrid, UInv_InventoryGrid* TargetGrid, UInv_HoverItem* HoverItem, UInv_InventoryItem* TargetItem, int32 TargetIndex);

	static bool MatchesCategory(const UInv_InventoryGrid* Grid, const UInv_InventoryItem* Item);
	
	static bool MatchesPreferredCategory(const UInv_InventoryGrid* Grid, const UInv_InventoryItem* Item);
	
	static bool AreItemsStackable(const UInv_InventoryItem* Item1, const UInv_InventoryItem* Item2);
};