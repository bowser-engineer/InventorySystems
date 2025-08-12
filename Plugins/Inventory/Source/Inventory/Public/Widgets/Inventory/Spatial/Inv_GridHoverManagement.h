#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Inv_GridHoverManagement.generated.h"

class UInv_InventoryGrid;
class UInv_InventoryItem;
struct FInv_GridFragment;

UCLASS(BlueprintType)
class INVENTORY_API UInv_GridHoverManagement : public UObject
{
	GENERATED_BODY()

public:
	static void AssignHoverItem(UInv_InventoryGrid* Grid, UInv_InventoryItem* InventoryItem);
	
	static void AssignHoverItem(UInv_InventoryGrid* Grid, UInv_InventoryItem* InventoryItem, const int32 GridIndex, const int32 PreviousGridIndex);
	
	static void PickUp(UInv_InventoryGrid* Grid, UInv_InventoryItem* ClickedInventoryItem, const int32 GridIndex);
	
	static void ClearHoverItem(UInv_InventoryGrid* Grid);
	
	static void PutHoverItemBack(UInv_InventoryGrid* Grid);
	
	static void DropItem(UInv_InventoryGrid* Grid);
	
	static void PutDownOnIndex(UInv_InventoryGrid* Grid, const int32 Index);
	
	static void ShowCursor(UInv_InventoryGrid* Grid);
	
	static void HideCursor(UInv_InventoryGrid* Grid);

private:
	static FVector2D GetDrawSize(const UInv_InventoryGrid* Grid, const FInv_GridFragment* GridFragment);
};