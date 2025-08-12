#include "Widgets/Inventory/Spatial/Inv_GridCalculations.h"
#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Widgets/Utils/Inv_WidgetUtils.h"
#include "Components/CanvasPanel.h"

#include "Widgets/Inventory/Spatial/Inv_GridPopupInteractions.h"
#include "Widgets/Inventory/Spatial/Inv_GridInitialization.h"
#include "Widgets/Inventory/Spatial/Inv_GridItemPlacement.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"

#include "Components/Widget.h"


void UInv_GridCalculations::UpdateTileParameters(UInv_InventoryGrid* Grid, const FVector2D& CanvasPosition, const FVector2D& MousePosition)
{
	if (!IsValid(Grid) || !Grid->bMouseWithinCanvas) return;
	
	const FIntPoint HoveredTileCoordinates = CalculateHoveredCoordinates(Grid, CanvasPosition, MousePosition);
	
	Grid->LastTileParameters = Grid->TileParameters;
	Grid->TileParameters.TileCoordinates = HoveredTileCoordinates;
	Grid->TileParameters.TileIndex = UInv_WidgetUtils::GetIndexFromPosition(HoveredTileCoordinates, Grid->Columns);
	Grid->TileParameters.TileQuadrant = CalculateTileQuadrant(Grid, CanvasPosition, MousePosition);
	
	Grid->OnTileParametersUpdated(Grid->TileParameters);
}

FIntPoint UInv_GridCalculations::CalculateHoveredCoordinates(const UInv_InventoryGrid* Grid, const FVector2D& CanvasPosition, const FVector2D& MousePosition)
{
	if (!IsValid(Grid)) return FIntPoint::ZeroValue;

	return FIntPoint{
		static_cast<int32>(FMath::FloorToInt((MousePosition.X - CanvasPosition.X) / Grid->TileSize)),
		static_cast<int32>(FMath::FloorToInt((MousePosition.Y - CanvasPosition.Y) / Grid->TileSize))
	};
}

EInv_TileQuadrant UInv_GridCalculations::CalculateTileQuadrant(const UInv_InventoryGrid* Grid, const FVector2D& CanvasPosition, const FVector2D& MousePosition)
{
	if (!IsValid(Grid)) return EInv_TileQuadrant::None;

	const float TileLocalX = FMath::Fmod(MousePosition.X - CanvasPosition.X, Grid->TileSize);
	const float TileLocalY = FMath::Fmod(MousePosition.Y - CanvasPosition.Y, Grid->TileSize);

	const bool bIsTop = TileLocalY < Grid->TileSize / 2.f;
	const bool bIsLeft = TileLocalX < Grid->TileSize / 2.f;

	EInv_TileQuadrant HoveredTileQuadrant{EInv_TileQuadrant::None};
	if (bIsTop && bIsLeft) HoveredTileQuadrant = EInv_TileQuadrant::TopLeft;
	else if (bIsTop && !bIsLeft) HoveredTileQuadrant = EInv_TileQuadrant::TopRight;
	else if (!bIsTop && bIsLeft) HoveredTileQuadrant = EInv_TileQuadrant::BottomLeft;
	else if (!bIsTop && !bIsLeft) HoveredTileQuadrant = EInv_TileQuadrant::BottomRight;

	return HoveredTileQuadrant;
}

FIntPoint UInv_GridCalculations::CalculateStartingCoordinate(const FIntPoint& Coordinate, const FIntPoint& Dimensions, const EInv_TileQuadrant Quadrant)
{
	const int32 HasEvenWidth = Dimensions.X % 2 == 0 ? 1 : 0;
	const int32 HasEvenHeight = Dimensions.Y % 2 == 0 ? 1 : 0;

	FIntPoint StartingCoord;
	switch (Quadrant)
	{
		case EInv_TileQuadrant::TopLeft:
			StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5f * Dimensions.X);
			StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y);
			break;
		case EInv_TileQuadrant::TopRight:
			StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5f * Dimensions.X) + HasEvenWidth;
			StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y);
			break;
		case EInv_TileQuadrant::BottomLeft:
			StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5f * Dimensions.X);
			StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y) + HasEvenHeight;
			break;
		case EInv_TileQuadrant::BottomRight:
			StartingCoord.X = Coordinate.X - FMath::FloorToInt(0.5f * Dimensions.X) + HasEvenWidth;
			StartingCoord.Y = Coordinate.Y - FMath::FloorToInt(0.5f * Dimensions.Y) + HasEvenHeight;
			break;
		default:
			return FIntPoint(-1, -1);
	}
	return StartingCoord;
}

bool UInv_GridCalculations::CursorExitedCanvas(UInv_InventoryGrid* Grid, const FVector2D& BoundaryPos, const FVector2D& BoundarySize, const FVector2D& Location)
{
	if (!IsValid(Grid)) return true;

	Grid->bLastMouseWithinCanvas = Grid->bMouseWithinCanvas;
	Grid->bMouseWithinCanvas = UInv_WidgetUtils::IsWithinBounds(BoundaryPos, BoundarySize, Location);
	
	if (!Grid->bMouseWithinCanvas && Grid->bLastMouseWithinCanvas)
	{
		return true;
	}
	return false;
}

bool UInv_GridCalculations::IsMouseOverGrid(const UInv_InventoryGrid* Grid)
{
	if (!IsValid(Grid) || !IsValid(Grid->GetOwningPlayer()) || !Grid->CanvasPanel) return false;

	const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(Grid->GetOwningPlayer());


	const FVector2D CanvasPosition = UInv_WidgetUtils::GetWidgetPosition(Cast<UWidget>(Grid->CanvasPanel));
	const FVector2D CanvasSize = UInv_WidgetUtils::GetWidgetSize(Cast<UWidget>(Grid->CanvasPanel));

	return UInv_WidgetUtils::IsWithinBounds(CanvasPosition, CanvasSize, MousePosition);
}