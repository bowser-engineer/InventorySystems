#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/Engine.h"
#include "Inv_GridPopupInteractions.generated.h"

class UInv_InventoryGrid;
class UInv_InventoryItem;

UCLASS(BlueprintType)
class INVENTORY_API UInv_GridPopupInteractions : public UObject
{
	GENERATED_BODY()

public:
	static void CreateItemPopUp(UInv_InventoryGrid* Grid, const int32 GridIndex);
	
	static void OnPopUpMenuSplit(UInv_InventoryGrid* Grid, int32 SplitAmount, int32 Index);
	
	static void OnPopUpMenuDrop(UInv_InventoryGrid* Grid, int32 Index);
	
	static void OnPopUpMenuConsume(UInv_InventoryGrid* Grid, int32 Index);

	static bool IsRightClick(const FPointerEvent& MouseEvent);
	
	static bool IsLeftClick(const FPointerEvent& MouseEvent);

	static bool IsSameStackable(const UInv_InventoryGrid* Grid, const UInv_InventoryItem* ClickedInventoryItem);

	static bool ShouldSwapStackCounts(const int32 RoomInClickedSlot, const int32 HoveredStackCount, const int32 MaxStackSize);
	
	static bool ShouldConsumeHoverItemStacks(const int32 HoveredStackCount, const int32 RoomInClickedSlot);
	
	static bool ShouldFillInStack(const int32 RoomInClickedSlot, const int32 HoveredStackCount);

	static void SwapStackCounts(UInv_InventoryGrid* Grid, const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index);
	
	static void ConsumeHoverItemStacks(UInv_InventoryGrid* Grid, const int32 ClickedStackCount, const int32 HoveredStackCount, const int32 Index);
	
	static void FillInStack(UInv_InventoryGrid* Grid, const int32 FillAmount, const int32 Remainder, const int32 Index);
};