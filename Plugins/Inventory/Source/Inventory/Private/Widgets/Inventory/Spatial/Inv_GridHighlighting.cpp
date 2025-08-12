#include "Widgets/Inventory/Spatial/Inv_GridHighlighting.h"
#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"

void UInv_GridHighlighting::HighlightSlots(UInv_InventoryGrid* Grid, const int32 Index, const FIntPoint& Dimensions)
{
	if (!IsValid(Grid) || !Grid->bMouseWithinCanvas) return;
	
	UnHighlightSlots(Grid, Grid->LastHighlightedIndex, Grid->LastHighlightedDimensions);
	
	UInv_InventoryStatics::ForEach2D(Grid->GridSlots, Index, Dimensions, Grid->Columns, [&](UInv_GridSlot* GridSlot)
	{
		GridSlot->SetOccupiedTexture();
	});
	
	Grid->LastHighlightedDimensions = Dimensions;
	Grid->LastHighlightedIndex = Index;
}

void UInv_GridHighlighting::UnHighlightSlots(UInv_InventoryGrid* Grid, const int32 Index, const FIntPoint& Dimensions)
{
	if (!IsValid(Grid)) return;
	
	UInv_InventoryStatics::ForEach2D(Grid->GridSlots, Index, Dimensions, Grid->Columns, [&](UInv_GridSlot* GridSlot)
	{
		GridSlot->SetUnoccupiedTexture();
	});
}

void UInv_GridHighlighting::ChangeHoverType(UInv_InventoryGrid* Grid, const int32 Index, const FIntPoint& Dimensions, EInv_GridSlotState GridSlotState)
{
	if (!IsValid(Grid)) return;
	
	UnHighlightSlots(Grid, Grid->LastHighlightedIndex, Grid->LastHighlightedDimensions);
	
	UInv_InventoryStatics::ForEach2D(Grid->GridSlots, Index, Dimensions, Grid->Columns, [State = GridSlotState](UInv_GridSlot* GridSlot)
	{
		switch (State)
		{
		case EInv_GridSlotState::Occupied:
			GridSlot->SetOccupiedTexture();
			break;
		case EInv_GridSlotState::Unoccupied:
			GridSlot->SetUnoccupiedTexture();
			break;
		case EInv_GridSlotState::GrayedOut:
			GridSlot->SetGrayedOutTexture();
			break;
		case EInv_GridSlotState::Selected:
			GridSlot->SetSelectedTexture();
			break;
		}
	});

	Grid->LastHighlightedIndex = Index;
	Grid->LastHighlightedDimensions = Dimensions;
}

void UInv_GridHighlighting::OnGridSlotHovered(UInv_InventoryGrid* Grid, int32 GridIndex)
{
	if (!IsValid(Grid) || !Grid->HoverItem) return;

	if (Grid->GridSlots.IsValidIndex(GridIndex))
	{
		UInv_GridSlot* GridSlot = Grid->GridSlots[GridIndex];
		if (GridSlot->IsAvailable())
		{
			GridSlot->SetOccupiedTexture();
		}
	}
}

void UInv_GridHighlighting::OnGridSlotUnhovered(UInv_InventoryGrid* Grid, int32 GridIndex)
{
	if (!IsValid(Grid) || !Grid->HoverItem) return;

	if (Grid->GridSlots.IsValidIndex(GridIndex))
	{
		UInv_GridSlot* GridSlot = Grid->GridSlots[GridIndex];
		if (GridSlot->IsAvailable())
		{
			GridSlot->SetUnoccupiedTexture();
		}
	}
}