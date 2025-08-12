#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Inv_GridInitialization.generated.h"

class UInv_InventoryGrid;

UCLASS(BlueprintType)
class INVENTORY_API UInv_GridInitialization : public UObject
{
	GENERATED_BODY()

public:
	static void InitializeGrid(UInv_InventoryGrid* Grid);
	static void CleanupGrid(UInv_InventoryGrid* Grid);
	
	static void RegisterGrid(UInv_InventoryGrid* Grid);
	static void UnregisterGrid(UInv_InventoryGrid* Grid);
	static UInv_InventoryGrid* GetGridWithHoverItem(const UObject* WorldContext);

private:
	static void ConstructGrid(UInv_InventoryGrid* Grid);
	static void SetupInventoryComponent(UInv_InventoryGrid* Grid);

	static TArray<TWeakObjectPtr<UInv_InventoryGrid>> RegisteredGrids;
};