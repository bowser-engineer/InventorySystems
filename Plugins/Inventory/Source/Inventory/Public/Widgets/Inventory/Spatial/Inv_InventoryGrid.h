// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/Inv_GridTypes.h"

#include "Inv_InventoryGrid.generated.h"

class UInv_ItemPopUp;
class UInv_HoverItem;
struct FInv_ImageFragment;
struct FInv_GridFragment;
class UInv_SlottedItem;
class UInv_ItemComponent;
struct FInv_ItemManifest;
class UCanvasPanel;
class UInv_GridSlot;
class UInv_InventoryComponent;
struct FGameplayTag;
enum class EInv_GridSlotState : uint8;
/**
 * 
 */
UCLASS()
class INVENTORY_API UInv_InventoryGrid : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	EInv_ItemCategory GetItemCategory() const { return ItemCategory; }
	FInv_SlotAvailabilityResult HasRoomForItem(const UInv_ItemComponent* ItemComponent);
	void SetOwningCanvas(UCanvasPanel* OwningCanvas);
	bool HasHoverItem() const;
	UInv_HoverItem* GetHoverItem() const;
	float GetTileSize() const { return TileSize; }
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearHoverItem();
	void OnHide();



	UFUNCTION()
	void AddItem(UInv_InventoryItem* Item);

	bool MatchesPreferredCategory(const UInv_InventoryItem* Item) const;
	// Override NativeDestruct to properly cleanup
	virtual void NativeDestruct() override;

	/** Static array to track all active grids for cross-grid operations */
	static TArray<TWeakObjectPtr<UInv_InventoryGrid>> RegisteredGrids;

	/** Track the grid that originally held the hover item */
	UPROPERTY()
	TWeakObjectPtr<UInv_InventoryGrid> SourceGrid;


	/* Default Grid */

	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<UCanvasPanel> OwningCanvasPanel;

	FInv_SlotAvailabilityResult HasRoomForItem(const UInv_InventoryItem* Item, const int32 StackAmountOverride = -1);
	FInv_SlotAvailabilityResult HasRoomForItem(const FInv_ItemManifest& Manifest, const int32 StackAmountOverride = -1);
	void AddItemToIndices(const FInv_SlotAvailabilityResult& Result, UInv_InventoryItem* NewItem);
	bool MatchesCategory(const UInv_InventoryItem* Item) const;


	void OnTileParametersUpdated(const FInv_TileParameters& Parameters);
	FInv_SpaceQueryResult CheckHoverPosition(const FIntPoint& Position, const FIntPoint& Dimensions);
	void HighlightSlots(const int32 Index, const FIntPoint& Dimensions);
	UUserWidget* GetVisibleCursorWidget();
	UUserWidget* GetHiddenCursorWidget();
	void SwapWithHoverItem(UInv_InventoryItem* ClickedInventoryItem, const int32 GridIndex);
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInv_ItemPopUp> ItemPopUpClass;

	UPROPERTY()
	TObjectPtr<UInv_ItemPopUp> ItemPopUp;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUserWidget> VisibleCursorWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UUserWidget> HiddenCursorWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> VisibleCursorWidget;

	UPROPERTY()
	TObjectPtr<UUserWidget> HiddenCursorWidget;
	
	UFUNCTION()
	void AddStacks(const FInv_SlotAvailabilityResult& Result);

	UFUNCTION()
	void OnSlottedItemClicked(int32 GridIndex, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void HandleStackableItemInteraction(UInv_InventoryItem* ClickedInventoryItem, int32 GridIndex);

	UFUNCTION()
	void OnGridSlotClicked(int32 GridIndex, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnGridSlotHovered(int32 GridIndex, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnGridSlotUnhovered(int32 GridIndex, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void OnPopUpMenuSplit(int32 SplitAmount, int32 Index);

	UFUNCTION()
	void OnPopUpMenuDrop(int32 Index);

	UFUNCTION()
	void OnPopUpMenuConsume(int32 Index);

	UFUNCTION()
	void OnInventoryMenuToggled(bool bOpen);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"),  Category = "Inventory")
	EInv_ItemCategory ItemCategory;

	UPROPERTY()
	TArray<TObjectPtr<UInv_GridSlot>> GridSlots;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInv_GridSlot> GridSlotClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInv_SlottedItem> SlottedItemClass;

	UPROPERTY()
	TMap<int32, TObjectPtr<UInv_SlottedItem>> SlottedItems;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 Rows;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 Columns;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float TileSize;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInv_HoverItem> HoverItemClass;

	UPROPERTY()
	TObjectPtr<UInv_HoverItem> HoverItem;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FVector2D ItemPopUpOffset;

	FInv_TileParameters TileParameters;
	FInv_TileParameters LastTileParameters;

	// Index where an item would be placed if we click on the grid at a valid location
	int32 ItemDropIndex{INDEX_NONE};
	FInv_SpaceQueryResult CurrentQueryResult;
	bool bMouseWithinCanvas;
	bool bLastMouseWithinCanvas;
	int32 LastHighlightedIndex;
	FIntPoint LastHighlightedDimensions;
};

