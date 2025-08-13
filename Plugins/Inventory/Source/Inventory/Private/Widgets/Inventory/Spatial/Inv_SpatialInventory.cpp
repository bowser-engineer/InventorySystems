// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inventory/Spatial/Inv_SpatialInventory.h"

#include "Inventory.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Widgets/Inventory/Spatial/Inv_InventoryGrid.h"
#include "Widgets/Inventory/Spatial/Inv_GridInitialization.h"
#include "Widgets/Inventory/Spatial/Inv_GridHoverManagement.h"
#include "Widgets/Inventory/Spatial/Inv_GridItemPlacement.h"
#include "Inventory.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Items/Inv_InventoryItem.h"
#include "Widgets/ItemDescription/Inv_ItemDescription.h"
#include "Blueprint/WidgetTree.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Widgets/Inventory/GridSlots/Inv_EquippedGridSlot.h"
#include "Widgets/Inventory/HoverItem/Inv_HoverItem.h"
#include "Widgets/Inventory/SlottedItems/Inv_EquippedSlottedItem.h"
#include "Widgets/Inventory/Spatial/Inv_GridHoverManagement.h"

void UInv_SpatialInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Grid_Backpack->SetOwningCanvas(CanvasPanel);
	Grid_Satchel->SetOwningCanvas(CanvasPanel);
	Grid_Locked->SetOwningCanvas(CanvasPanel);

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		UInv_EquippedGridSlot* EquippedGridSlot = Cast<UInv_EquippedGridSlot>(Widget);
		if (IsValid(EquippedGridSlot))
		{
			EquippedGridSlots.Add(EquippedGridSlot);
			EquippedGridSlot->EquippedGridSlotClicked.AddDynamic(this, &ThisClass::EquippedGridSlotClicked);
		}
	});
}

void UInv_SpatialInventory::EquippedGridSlotClicked(UInv_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag)
{
	// Check to see if we can equip the Hover Item
	if (!CanEquipHoverItem(EquippedGridSlot, EquipmentTypeTag)) return;

	UInv_HoverItem* HoverItem = GetHoverItem();
	
	// Create an Equipped Slotted Item and add it to the Equipped Grid Slot (call EquippedGridSlot->OnItemEquipped())
	const float TileSize = UInv_InventoryStatics::GetInventoryWidget(GetOwningPlayer())->GetTileSize();
	UInv_EquippedSlottedItem* EquippedSlottedItem = EquippedGridSlot->OnItemEquipped(
		HoverItem->GetInventoryItem(),
		EquipmentTypeTag,
		TileSize
	);
	EquippedSlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	
	// If the item being equipped is the same one from the original equipped slot, 
	// it means we're putting it back - don't trigger unequip/equip, just clear the reference
	if (OriginalEquippedSlot.IsValid() && OriginalEquippedItem.IsValid() && 
		HoverItem->GetInventoryItem() == OriginalEquippedItem.Get() && EquippedGridSlot == OriginalEquippedSlot.Get())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquippedGridSlotClicked] Item %s placed back on original slot - no need to re-equip"), *HoverItem->GetInventoryItem()->GetName());
		OriginalEquippedSlot.Reset();
		OriginalEquippedItem.Reset();
	}
	else
	{
		// Inform the server that we've equipped an item (potentially unequipping an item as well)
		UInv_InventoryComponent* InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
		check(IsValid(InventoryComponent)); 

		InventoryComponent->Server_EquipSlotClicked(HoverItem->GetInventoryItem(), nullptr);
	}
	
	// Check if we need to remove from original equipped slot or inventory
	bool bWasPreviouslyEquipped = OriginalEquippedSlot.IsValid() && OriginalEquippedItem.IsValid() && 
								  HoverItem->GetInventoryItem() == OriginalEquippedItem.Get();
								  
	if (bWasPreviouslyEquipped)
	{
		// Item was previously equipped - remove it from original equipment slot
		UE_LOG(LogTemp, Warning, TEXT("[EquippedGridSlotClicked] Removing item %s from original equipped slot"), *OriginalEquippedItem->GetName());
		
		// Find and remove the equipped slotted item from the original slot
		if (UInv_EquippedSlottedItem* OldEquippedSlottedItem = OriginalEquippedSlot->GetEquippedSlottedItem())
		{
			RemoveEquippedSlottedItem(OldEquippedSlottedItem);
		}
		ClearSlotOfItem(OriginalEquippedSlot.Get());
		
		// Clear the references since we've now moved the item
		OriginalEquippedSlot.Reset();
		OriginalEquippedItem.Reset();
	}
	else
	{
		// This is a new item being equipped from inventory, so remove it from inventory
		UInv_InventoryGrid* HoverItemOwnerGrid = HoverItem->GetOwnerGrid();
		if (IsValid(HoverItemOwnerGrid))
		{
			int32 PreviousIndex = HoverItem->GetPreviousGridIndex();
			if (HoverItemOwnerGrid->GridSlots.IsValidIndex(PreviousIndex))
			{
				UE_LOG(LogTemp, Warning, TEXT("[EquippedGridSlotClicked] Removing equipped item %s from inventory at index %d"), 
					*HoverItem->GetInventoryItem()->GetName(), PreviousIndex);
				UInv_GridItemPlacement::RemoveItemFromGrid(HoverItemOwnerGrid, HoverItem->GetInventoryItem(), PreviousIndex);
			}
		}
	}
	
	// Clear the Hover Item from whichever grid actually owns it
	UInv_InventoryGrid* HoverItemOwnerGrid = HoverItem->GetOwnerGrid();
	if (IsValid(HoverItemOwnerGrid))
	{
		HoverItemOwnerGrid->ClearHoverItem();
	}
	else
	{
		// Fallback: clear from any grid that has a hover item
		if (Grid_Backpack->HasHoverItem()) Grid_Backpack->ClearHoverItem();
		else if (Grid_Locked->HasHoverItem()) Grid_Locked->ClearHoverItem();
		else if (Grid_Satchel->HasHoverItem()) Grid_Satchel->ClearHoverItem();
		else if (Grid_Quiver->HasHoverItem()) Grid_Quiver->ClearHoverItem();
	}
}

void UInv_SpatialInventory::EquippedSlottedItemClicked(UInv_EquippedSlottedItem* EquippedSlottedItem)
{
	// Remove the Item Description
	UInv_InventoryStatics::ItemUnhovered(GetOwningPlayer());

	if (IsValid(GetHoverItem()) && GetHoverItem()->IsStackable()) return;
	
	// Get Item to Equip
	UInv_InventoryItem* ItemToEquip = IsValid(GetHoverItem()) ? GetHoverItem()->GetInventoryItem() : nullptr;
	
	// Get Item to Unequip
	UInv_InventoryItem* ItemToUnequip = EquippedSlottedItem->GetInventoryItem();
	
	// Get the Equipped Grid Slot holding this item
	UInv_EquippedGridSlot* EquippedGridSlot = FindSlotWithEquippedItem(ItemToUnequip);
	
	// If we have a hover item, validate it can be equipped in this slot
	if (ItemToEquip && !CanEquipHoverItemInSlot(EquippedGridSlot, ItemToEquip))
	{
		return;
	}
	
	// DON'T clear the equipped slot or broadcast unequipping yet
	// Instead, just make it available as a hover item without unequipping
	
	// Assign previously equipped item as the hover item
	UInv_GridHoverManagement::AssignHoverItem(Grid_Backpack, ItemToUnequip);
	
	// Mark this hover item as previously equipped AND track which slot it came from
	if (UInv_HoverItem* HoverItem = Grid_Backpack->GetHoverItem())
	{
		HoverItem->SetWasPreviouslyEquipped(true);
		// Store the original equipped slot and item for potential re-equipping
		OriginalEquippedSlot = EquippedGridSlot;
		OriginalEquippedItem = ItemToUnequip;
	}
	
	// Make a new equipped slotted item (for the item we held in HoverItem) if we're swapping
	if (ItemToEquip)
	{
		// Remove the old equipped item from UI and replace with new one
		RemoveEquippedSlottedItem(EquippedSlottedItem);
		ClearSlotOfItem(EquippedGridSlot);
		MakeEquippedSlottedItem(EquippedSlottedItem, EquippedGridSlot, ItemToEquip);
		
		// Only broadcast equipping of new item, not unequipping of old one yet
		UInv_InventoryComponent* InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
		check(IsValid(InventoryComponent));
		InventoryComponent->Server_EquipSlotClicked(ItemToEquip, nullptr);
	}
	// If just picking up the equipped item, DON'T remove it from UI yet
	// It should remain visible until successfully placed elsewhere
}

FReply UInv_SpatialInventory::NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	//ActiveGrid->DropItem();
	return FReply::Handled();
}

FReply UInv_SpatialInventory::NativeOnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Let child widgets handle the mouse release first
	UE_LOG(LogInventory, Warning, TEXT("[SpatialInventory] Mouse released - allowing event propagation to child widgets"));
	
	// Return Unhandled to allow the event to propagate to child widgets (grid slots, slotted items)
	// Child widgets should handle their own stacking logic and clear hover items when appropriate
	return FReply::Unhandled();
}

void UInv_SpatialInventory::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(ItemDescription)) return;
	SetItemDescriptionSizeAndPosition(ItemDescription, CanvasPanel);
	SetEquippedItemDescriptionSizeAndPosition(ItemDescription, EquippedItemDescription, CanvasPanel);
}

void UInv_SpatialInventory::SetItemDescriptionSizeAndPosition(UInv_ItemDescription* Description, UCanvasPanel* Canvas) const
{
	UCanvasPanelSlot* ItemDescriptionCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(Description);
	if (!IsValid(ItemDescriptionCPS)) return;

	const FVector2D ItemDescriptionSize = Description->GetBoxSize();
	ItemDescriptionCPS->SetSize(ItemDescriptionSize);

	FVector2D ClampedPosition = UInv_WidgetUtils::GetClampedWidgetPosition(
		UInv_WidgetUtils::GetWidgetSize(Canvas),
		ItemDescriptionSize,
		UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer()));

	ItemDescriptionCPS->SetPosition(ClampedPosition);
}

void UInv_SpatialInventory::SetEquippedItemDescriptionSizeAndPosition(UInv_ItemDescription* Description, UInv_ItemDescription* EquippedDescription, UCanvasPanel* Canvas) const
{
	UCanvasPanelSlot* ItemDescriptionCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(Description);
	UCanvasPanelSlot* EquippedItemDescriptionCPS = UWidgetLayoutLibrary::SlotAsCanvasSlot(EquippedDescription);
	if (!IsValid(ItemDescriptionCPS) || !IsValid(EquippedItemDescriptionCPS)) return;

	const FVector2D ItemDescriptionSize = Description->GetBoxSize();
	const FVector2D EquippedItemDescriptionSize = EquippedDescription->GetBoxSize();

	FVector2D ClampedPosition = UInv_WidgetUtils::GetClampedWidgetPosition(
		UInv_WidgetUtils::GetWidgetSize(Canvas),
		ItemDescriptionSize,
		UWidgetLayoutLibrary::GetMousePositionOnViewport(GetOwningPlayer()));
	ClampedPosition.X -= EquippedItemDescriptionSize.X;

	EquippedItemDescriptionCPS->SetSize(EquippedItemDescriptionSize);
	EquippedItemDescriptionCPS->SetPosition(ClampedPosition);
}

bool UInv_SpatialInventory::CanEquipHoverItem(UInv_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag) const
{
	if (!IsValid(EquippedGridSlot)) return false;

	UInv_HoverItem* HoverItem = GetHoverItem();
	if (!IsValid(HoverItem)) return false;

	UInv_InventoryItem* HeldItem = HoverItem->GetInventoryItem();
	if (!IsValid(HeldItem)) return false;
	
	if (HoverItem->IsStackable()) return false;
	
	// Allow placing back if the slot is empty OR if this is the item's original slot
	bool bSlotEmpty = !EquippedGridSlot->GetInventoryItem().IsValid();
	bool bOriginalSlot = OriginalEquippedSlot.IsValid() && EquippedGridSlot == OriginalEquippedSlot.Get() && 
						 OriginalEquippedItem.IsValid() && HeldItem == OriginalEquippedItem.Get();
	
	if (!bSlotEmpty && !bOriginalSlot) return false;
	
	// Get the equipment fragment to check the equipment type
	const FInv_EquipmentFragment* EquipmentFragment = HeldItem->GetItemManifest().GetFragmentOfType<FInv_EquipmentFragment>();
	if (!EquipmentFragment) return false;

	const FGameplayTag ItemEquipmentType = EquipmentFragment->GetEquipmentType();
	return ItemEquipmentType.MatchesTag(EquipmentTypeTag);
}

bool UInv_SpatialInventory::CanEquipHoverItemInSlot(UInv_EquippedGridSlot* EquippedGridSlot, UInv_InventoryItem* ItemToEquip) const
{
	if (!IsValid(EquippedGridSlot) || !IsValid(ItemToEquip)) return false;
	
	// Get the required equipment type from the slot
	const FGameplayTag SlotEquipmentType = EquippedGridSlot->GetEquipmentTypeTag();
	
	// Get the equipment fragment from the item we want to equip
	const FInv_EquipmentFragment* EquipmentFragment = ItemToEquip->GetItemManifest().GetFragmentOfType<FInv_EquipmentFragment>();
	if (!EquipmentFragment) return false;
	
	const FGameplayTag ItemEquipmentType = EquipmentFragment->GetEquipmentType();
	
	// Check if the item's equipment type matches the slot's required equipment type (hierarchical matching)
	return ItemEquipmentType.MatchesTag(SlotEquipmentType);
}

UInv_EquippedGridSlot* UInv_SpatialInventory::FindSlotWithEquippedItem(UInv_InventoryItem* EquippedItem) const
{
	auto* FoundEquippedGridSlot = EquippedGridSlots.FindByPredicate([EquippedItem](const UInv_EquippedGridSlot* GridSlot)
	{
		return GridSlot->GetInventoryItem() == EquippedItem;
	});
	return FoundEquippedGridSlot ? *FoundEquippedGridSlot : nullptr;
}

void UInv_SpatialInventory::ClearSlotOfItem(UInv_EquippedGridSlot* EquippedGridSlot)
{
	if (IsValid(EquippedGridSlot))
	{
		EquippedGridSlot->ClearEquippedItem();
	}
}

void UInv_SpatialInventory::RemoveEquippedSlottedItem(UInv_EquippedSlottedItem* EquippedSlottedItem)
{
	if (!IsValid(EquippedSlottedItem)) return;

	if (EquippedSlottedItem->OnEquippedSlottedItemClicked.IsAlreadyBound(this, &ThisClass::EquippedSlottedItemClicked))
	{
		EquippedSlottedItem->OnEquippedSlottedItemClicked.RemoveDynamic(this, &ThisClass::EquippedSlottedItemClicked);
	}
	EquippedSlottedItem->RemoveFromParent();
}

void UInv_SpatialInventory::MakeEquippedSlottedItem(UInv_EquippedSlottedItem* EquippedSlottedItem, UInv_EquippedGridSlot* EquippedGridSlot, UInv_InventoryItem* ItemToEquip)
{
	if (!IsValid(EquippedGridSlot)) return;

	UInv_EquippedSlottedItem* SlottedItem = EquippedGridSlot->OnItemEquipped(
		ItemToEquip,
		EquippedSlottedItem->GetEquipmentTypeTag(),
		UInv_InventoryStatics::GetInventoryWidget(GetOwningPlayer())->GetTileSize());
	if (IsValid(SlottedItem)) SlottedItem->OnEquippedSlottedItemClicked.AddDynamic(this, &ThisClass::EquippedSlottedItemClicked);

	EquippedGridSlot->SetEquippedSlottedItem(SlottedItem);
}

void UInv_SpatialInventory::BroadcastSlotClickedDelegates(UInv_InventoryItem* ItemToEquip, UInv_InventoryItem* ItemToUnequip) const
{
	UInv_InventoryComponent* InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
	check(IsValid(InventoryComponent));
	InventoryComponent->Server_EquipSlotClicked(ItemToEquip, ItemToUnequip);
}

FInv_SlotAvailabilityResult UInv_SpatialInventory::HasRoomForItem(UInv_ItemComponent* ItemComponent) const
{
	switch (UInv_InventoryStatics::GetItemCategoryFromItemComp(ItemComponent))
	{
		case EInv_ItemCategory::Backpack:
			return Grid_Backpack->HasRoomForItem(ItemComponent);
		case EInv_ItemCategory::Locked:
			return Grid_Locked->HasRoomForItem(ItemComponent);
		case EInv_ItemCategory::Satchel:
			return Grid_Satchel->HasRoomForItem(ItemComponent);
		case EInv_ItemCategory::Quiver:
			return Grid_Quiver->HasRoomForItem(ItemComponent);
		default:
			UE_LOG(LogInventory, Error, TEXT("ItemComponent doesn't have a valid Item Category."))
			return FInv_SlotAvailabilityResult();
	}
}

void UInv_SpatialInventory::OnItemHovered(UInv_InventoryItem* Item)
{
	const auto& Manifest = Item->GetItemManifest();
	UInv_ItemDescription* DescriptionWidget = GetItemDescription();
	DescriptionWidget->SetVisibility(ESlateVisibility::Collapsed);

	GetOwningPlayer()->GetWorldTimerManager().ClearTimer(DescriptionTimer);
	GetOwningPlayer()->GetWorldTimerManager().ClearTimer(EquippedDescriptionTimer);

	FTimerDelegate DescriptionTimerDelegate;
	DescriptionTimerDelegate.BindLambda([this, Item, &Manifest, DescriptionWidget]()
	{
		GetItemDescription()->SetVisibility(ESlateVisibility::HitTestInvisible);
		Manifest.AssimilateInventoryFragments(DescriptionWidget);
		
		// For the second item description, showing the equipped item of this type.
		FTimerDelegate EquippedDescriptionTimerDelegate;
		EquippedDescriptionTimerDelegate.BindUObject(this, &ThisClass::ShowEquippedItemDescription, Item);
		GetOwningPlayer()->GetWorldTimerManager().SetTimer(EquippedDescriptionTimer, EquippedDescriptionTimerDelegate, EquippedDescriptionTimerDelay, false);
	});

	GetOwningPlayer()->GetWorldTimerManager().SetTimer(DescriptionTimer, DescriptionTimerDelegate, DescriptionTimerDelay, false);
}

void UInv_SpatialInventory::OnItemUnHovered()
{
	GetItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	GetOwningPlayer()->GetWorldTimerManager().ClearTimer(DescriptionTimer);
	GetEquippedItemDescription()->SetVisibility(ESlateVisibility::Collapsed);
	GetOwningPlayer()->GetWorldTimerManager().ClearTimer(EquippedDescriptionTimer);
}

// There are 4 grids in the Spatial Inventory, so we check each one for a hover item
// but theres probably a better way to do this because there is only going to be one hover item at a time.

bool UInv_SpatialInventory::HasHoverItem() const
{
	if (Grid_Backpack->HasHoverItem()) return true;
	if (Grid_Satchel->HasHoverItem()) return true;
	if (Grid_Locked->HasHoverItem()) return true;
	if(Grid_Quiver->HasHoverItem()) return true;
	return false;
}

UInv_HoverItem* UInv_SpatialInventory::GetHoverItem() const
{
	if (Grid_Backpack->HasHoverItem()) return Grid_Backpack->GetHoverItem();
	if (Grid_Satchel->HasHoverItem()) return Grid_Satchel->GetHoverItem();
	if (Grid_Locked->HasHoverItem()) return Grid_Locked->GetHoverItem();
	if (Grid_Quiver->HasHoverItem()) return Grid_Quiver->GetHoverItem();
	return nullptr;
}

float UInv_SpatialInventory::GetTileSize() const
{
	return Grid_Backpack->GetTileSize();
}

void UInv_SpatialInventory::ShowEquippedItemDescription(UInv_InventoryItem* Item)
{
	const auto& Manifest = Item->GetItemManifest();
	const FInv_EquipmentFragment* EquipmentFragment = Manifest.GetFragmentOfType<FInv_EquipmentFragment>();
	if (!EquipmentFragment) return;

	const FGameplayTag HoveredEquipmentType = EquipmentFragment->GetEquipmentType();
	
	auto EquippedGridSlot = EquippedGridSlots.FindByPredicate([Item](const UInv_EquippedGridSlot* GridSlot)
	{
		return GridSlot->GetInventoryItem() == Item;
	});
	if (EquippedGridSlot != nullptr) return; // The hovered item is already equipped, we're already showing its Item Description

	// It's not equipped, so find the equipped item with the same equipment type
	auto FoundEquippedSlot = EquippedGridSlots.FindByPredicate([HoveredEquipmentType](const UInv_EquippedGridSlot* GridSlot)
	{
		UInv_InventoryItem* InventoryItem = GridSlot->GetInventoryItem().Get();
		return IsValid(InventoryItem) ? InventoryItem->GetItemManifest().GetFragmentOfType<FInv_EquipmentFragment>()->GetEquipmentType() == HoveredEquipmentType : false ;
	});
	UInv_EquippedGridSlot* EquippedSlot = FoundEquippedSlot ? *FoundEquippedSlot : nullptr;
	if (!IsValid(EquippedSlot)) return; // No equipped item with the same equipment type

	UInv_InventoryItem* EquippedItem = EquippedSlot->GetInventoryItem().Get();
	if (!IsValid(EquippedItem)) return;

	const auto& EquippedItemManifest = EquippedItem->GetItemManifest();
	UInv_ItemDescription* DescriptionWidget = GetEquippedItemDescription();

	auto EquippedDescriptionWidget = GetEquippedItemDescription();
	
	EquippedDescriptionWidget->Collapse();
	DescriptionWidget->SetVisibility(ESlateVisibility::HitTestInvisible);	
	EquippedItemManifest.AssimilateInventoryFragments(EquippedDescriptionWidget);
}

UInv_ItemDescription* UInv_SpatialInventory::GetItemDescription()
{
	if (!IsValid(ItemDescription))
	{
		ItemDescription = CreateWidget<UInv_ItemDescription>(GetOwningPlayer(), ItemDescriptionClass);
		CanvasPanel->AddChild(ItemDescription);
	}
	return ItemDescription;
}

UInv_ItemDescription* UInv_SpatialInventory::GetEquippedItemDescription()
{
	if (!IsValid(EquippedItemDescription))
	{
		EquippedItemDescription = CreateWidget<UInv_ItemDescription>(GetOwningPlayer(), EquippedItemDescriptionClass);
		CanvasPanel->AddChild(EquippedItemDescription);
	}
	return EquippedItemDescription;
}

void UInv_SpatialInventory::AddRecentlyUnequippedItem(UInv_InventoryItem* Item)
{
	if (IsValid(Item))
	{
		RecentlyUnequippedItems.Add(Item);
		UE_LOG(LogTemp, Warning, TEXT("[AddRecentlyUnequippedItem] Added item %s to recently unequipped list"), *Item->GetName());
	}
}

bool UInv_SpatialInventory::IsRecentlyUnequippedItem(UInv_InventoryItem* Item) const
{
	return IsValid(Item) && RecentlyUnequippedItems.Contains(Item);
}

void UInv_SpatialInventory::ClearRecentlyUnequippedItems()
{
	UE_LOG(LogTemp, Warning, TEXT("[ClearRecentlyUnequippedItems] Clearing recently unequipped items list"));
	RecentlyUnequippedItems.Empty();
}

void UInv_SpatialInventory::OnItemPlacedInInventory(UInv_InventoryItem* Item)
{
	if (!IsValid(Item)) return;

	// Check if this item was previously equipped and should now be unequipped
	if (OriginalEquippedItem.IsValid() && Item == OriginalEquippedItem.Get())
	{
		UE_LOG(LogTemp, Warning, TEXT("[OnItemPlacedInInventory] Unequipping item that was placed in inventory: %s"), *Item->GetName());
		
		// Remove the equipped item from its UI slot since it's now in inventory
		if (OriginalEquippedSlot.IsValid())
		{
			if (UInv_EquippedSlottedItem* OldEquippedSlottedItem = OriginalEquippedSlot->GetEquippedSlottedItem())
			{
				RemoveEquippedSlottedItem(OldEquippedSlottedItem);
			}
			ClearSlotOfItem(OriginalEquippedSlot.Get());
		}
		
		// Now actually unequip the item since it's been placed in inventory
		UInv_InventoryComponent* InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer());
		if (IsValid(InventoryComponent))
		{
			InventoryComponent->Server_EquipSlotClicked(nullptr, Item);
		}
		
		// Clear the original equipped slot and item references
		OriginalEquippedSlot.Reset();
		OriginalEquippedItem.Reset();
	}
}
