#include "Widgets/Inventory/Spatial/Inv_GridInitialization.h"
#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Widgets/Inventory/GridSlots/Inv_GridSlot.h"
#include "Widgets/Utils/Inv_WidgetUtils.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"



TArray<TWeakObjectPtr<UInv_InventoryGrid>> UInv_GridInitialization::RegisteredGrids;

void UInv_GridInitialization::InitializeGrid(UInv_InventoryGrid* Grid)
{
	if (!IsValid(Grid)) return;

	ConstructGrid(Grid);
	RegisterGrid(Grid);
	SetupInventoryComponent(Grid);
}

void UInv_GridInitialization::CleanupGrid(UInv_InventoryGrid* Grid)
{
	if (!IsValid(Grid)) return;

	if (Grid->HasHoverItem())
	{
		Grid->ClearHoverItem();
	}
	UnregisterGrid(Grid);
}

void UInv_GridInitialization::ConstructGrid(UInv_InventoryGrid* Grid)
{
	if (!IsValid(Grid) || !Grid->CanvasPanel) return;

	Grid->GridSlots.Reserve(Grid->Rows * Grid->Columns);

	for (int32 j = 0; j < Grid->Rows; ++j)
	{
		for (int32 i = 0; i < Grid->Columns; ++i)
		{
			UInv_GridSlot* GridSlot = CreateWidget<UInv_GridSlot>(Grid, Grid->GridSlotClass);
			Grid->CanvasPanel->AddChild(GridSlot);

			const FIntPoint TilePosition(i, j);
			GridSlot->SetTileIndex(UInv_WidgetUtils::GetIndexFromPosition(TilePosition, Grid->Columns));

			UCanvasPanelSlot* GridCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridSlot);
			GridCPS->SetSize(FVector2D(Grid->TileSize));
			GridCPS->SetPosition(TilePosition * Grid->TileSize);

			Grid->GridSlots.Add(GridSlot);
			GridSlot->GridSlotClicked.AddDynamic(Grid, &UInv_InventoryGrid::OnGridSlotClicked);
			GridSlot->GridSlotHovered.AddDynamic(Grid, &UInv_InventoryGrid::OnGridSlotHovered);
			GridSlot->GridSlotUnhovered.AddDynamic(Grid, &UInv_InventoryGrid::OnGridSlotUnhovered);
		}
	}
}

void UInv_GridInitialization::SetupInventoryComponent(UInv_InventoryGrid* Grid)
{
	if (!IsValid(Grid)) return;

	Grid->InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(Grid->GetOwningPlayer());
	if (!Grid->InventoryComponent.IsValid()) return;

	Grid->InventoryComponent->OnItemAdded.AddDynamic(Grid, &UInv_InventoryGrid::AddItem);
	Grid->InventoryComponent->OnStackChange.AddDynamic(Grid, &UInv_InventoryGrid::AddStacks);
	Grid->InventoryComponent->OnInventoryMenuToggled.AddDynamic(Grid, &UInv_InventoryGrid::OnInventoryMenuToggled);
}

void UInv_GridInitialization::RegisterGrid(UInv_InventoryGrid* Grid)
{
	if (!IsValid(Grid)) return;

	RegisteredGrids.RemoveAll([](const TWeakObjectPtr<UInv_InventoryGrid>& GridPtr) {
		return !GridPtr.IsValid();
	});

	if (!RegisteredGrids.Contains(Grid))
	{
		RegisteredGrids.Add(Grid);
	}
}

void UInv_GridInitialization::UnregisterGrid(UInv_InventoryGrid* Grid)
{
	if (!IsValid(Grid)) return;
	RegisteredGrids.Remove(Grid);
}

UInv_InventoryGrid* UInv_GridInitialization::GetGridWithHoverItem(const UObject* WorldContext)
{
	// Clean up invalid grid references first
	RegisteredGrids.RemoveAll([](const TWeakObjectPtr<UInv_InventoryGrid>& GridPtr) {
		return !GridPtr.IsValid();
	});

	for (auto& GridPtr : RegisteredGrids)
	{
		if (UInv_InventoryGrid* Grid = GridPtr.Get())
		{
			if (Grid->HasHoverItem())
			{
				UInv_HoverItem* HoverItem = Grid->GetHoverItem();
				if (!IsValid(HoverItem)) 
				{
					// Clean up invalid hover item reference
					Grid->ClearHoverItem();
					continue;
				}
				
				// Check if this grid actually owns the hover item
				if (HoverItem->GetOwnerGrid() == Grid)
				{
					return Grid;
				}
				else if (!HoverItem->GetOwnerGrid())
				{
					// If hover item has no owner, this grid likely owns it
					// but the reference got broken - fix it
					HoverItem->SetOwnerGrid(Grid);
					return Grid;
				}
			}
		}
	}
	return nullptr;
}