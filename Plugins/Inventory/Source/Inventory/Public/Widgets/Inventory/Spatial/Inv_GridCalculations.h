#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <Types/Inv_GridTypes.h>
#include "Inv_GridCalculations.generated.h"

class UInv_InventoryGrid;

UCLASS(BlueprintType)
class INVENTORY_API UInv_GridCalculations : public UObject
{
	GENERATED_BODY()

public:
	static void UpdateTileParameters(UInv_InventoryGrid* Grid, const FVector2D& CanvasPosition, const FVector2D& MousePosition);
	
	static FIntPoint CalculateHoveredCoordinates(const UInv_InventoryGrid* Grid, const FVector2D& CanvasPosition, const FVector2D& MousePosition);
	
	static EInv_TileQuadrant CalculateTileQuadrant(const UInv_InventoryGrid* Grid, const FVector2D& CanvasPosition, const FVector2D& MousePosition);
	
	static FIntPoint CalculateStartingCoordinate(const FIntPoint& Coordinate, const FIntPoint& Dimensions, const EInv_TileQuadrant Quadrant);
	
	static bool CursorExitedCanvas(UInv_InventoryGrid* Grid, const FVector2D& BoundaryPos, const FVector2D& BoundarySize, const FVector2D& Location);
	
	static bool IsMouseOverGrid(const UInv_InventoryGrid* Grid);
};