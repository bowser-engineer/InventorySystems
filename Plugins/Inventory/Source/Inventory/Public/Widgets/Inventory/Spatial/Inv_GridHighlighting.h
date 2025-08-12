#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Inv_GridHighlighting.generated.h"

class UInv_InventoryGrid;
enum class EInv_GridSlotState : uint8;

UCLASS(BlueprintType)
class INVENTORY_API UInv_GridHighlighting : public UObject
{
	GENERATED_BODY()

public:
	static void HighlightSlots(UInv_InventoryGrid* Grid, const int32 Index, const FIntPoint& Dimensions);
	
	static void UnHighlightSlots(UInv_InventoryGrid* Grid, const int32 Index, const FIntPoint& Dimensions);
	
	static void ChangeHoverType(UInv_InventoryGrid* Grid, const int32 Index, const FIntPoint& Dimensions, EInv_GridSlotState GridSlotState);
	
	static void OnGridSlotHovered(UInv_InventoryGrid* Grid, int32 GridIndex);
	
	static void OnGridSlotUnhovered(UInv_InventoryGrid* Grid, int32 GridIndex);
};