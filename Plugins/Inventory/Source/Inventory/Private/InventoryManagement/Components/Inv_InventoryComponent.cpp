// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Components/Inv_InventoryComponent.h"

#include "Items/Components/Inv_ItemComponent.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Net/UnrealNetwork.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include <InventoryManagement/Utils/Inv_InventoryStatics.h>


UInv_InventoryComponent::UInv_InventoryComponent() : InventoryList(this)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	bInventoryMenuOpen = false;
}

void UInv_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}
// Updated TryAddItem method - remove the separate Server_SetItemCategory call
void UInv_InventoryComponent::TryAddItem(UInv_ItemComponent* ItemComponent)
{
	TArray<EInv_ItemCategory> CategoriesToTry;
	EInv_ItemCategory ItemCategory = UInv_InventoryStatics::GetPreferredItemCategoryFromItemComp(ItemComponent);
	CategoriesToTry.Add(ItemCategory);
	if (ItemCategory != EInv_ItemCategory::Backpack && ItemCategory != EInv_ItemCategory::Locked)
	{
		CategoriesToTry.Add(EInv_ItemCategory::Backpack);
		CategoriesToTry.Add(EInv_ItemCategory::Locked);
	}

	FInv_SlotAvailabilityResult Result;
	UInv_InventoryItem* FoundItem = nullptr;
	bool bFoundRoom = false;
	EInv_ItemCategory ValidCategory = EInv_ItemCategory::Backpack; // Default fallback

	for (EInv_ItemCategory Category : CategoriesToTry)
	{
		// Test locally first (don't call server yet)
		ItemComponent->SetItemCategory(Category);
		Result = InventoryMenu->HasRoomForItem(ItemComponent);
		FoundItem = InventoryList.FindFirstItemByTypeInCategory(ItemComponent->GetItemManifest().GetItemType(), Category);
		Result.Item = FoundItem;

		if (Result.TotalRoomToFill > 0)
		{
			ValidCategory = Category;
			bFoundRoom = true;
			break;
		}
	}

	if (!bFoundRoom)
	{
		NoRoomInInventory.Broadcast();
		return;
	}

	// DEBUG: Log the result details
	UE_LOG(LogTemp, Warning, TEXT("TryAddItem Debug - Result.Item.IsValid(): %s, Result.bStackable: %s, Result.TotalRoomToFill: %d, ValidCategory: %s"),
		Result.Item.IsValid() ? TEXT("true") : TEXT("false"),
		Result.bStackable ? TEXT("true") : TEXT("false"),
		Result.TotalRoomToFill,
		*UEnum::GetValueAsString(ValidCategory));

	// Pass the ValidCategory directly to the server functions
	if (Result.Item.IsValid() && Result.bStackable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Taking STACKABLE path - adding to existing item"));
		OnStackChange.Broadcast(Result);
		Server_AddStacksToItem(ItemComponent, ValidCategory, Result.TotalRoomToFill, Result.Remainder);
	}
	else if (Result.TotalRoomToFill > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Taking NEW ITEM path - creating new stack"));
		Server_AddNewItem(ItemComponent, ValidCategory, Result.bStackable ? Result.TotalRoomToFill : 0, Result.Remainder);
	}
}

// Updated Server_AddNewItem to accept category parameter
void UInv_InventoryComponent::Server_AddNewItem_Implementation(UInv_ItemComponent* ItemComponent, EInv_ItemCategory Category, int32 StackCount, int32 Remainder)
{
	// Set the category first to ensure it's correct before adding to inventory
	ItemComponent->SetItemCategory(Category);

	// Log the category we're trying to set
	UE_LOG(LogTemp, Warning, TEXT("Setting ItemComponent category to: %s"), *UEnum::GetValueAsString(Category));

	// Verify ItemComponent has the correct category after setting
	UE_LOG(LogTemp, Warning, TEXT("ItemComponent category after setting: %s"),
		*UEnum::GetValueAsString(UInv_InventoryStatics::GetItemCategoryFromItemComp(ItemComponent)));

	// This just adds the item to the inventory list, which is a fast array.
	UInv_InventoryItem* NewItem = InventoryList.AddEntry(ItemComponent);

	// Verify the NewItem has the correct category
	UE_LOG(LogTemp, Warning, TEXT("NewItem category after AddEntry: %s"),
		*UEnum::GetValueAsString(NewItem->GetItemManifest().GetItemCategory()));

	NewItem->SetTotalStackCount(StackCount);

	if (GetOwner()->GetNetMode() == NM_ListenServer || GetOwner()->GetNetMode() == NM_Standalone)
	{
		// Log right before broadcasting
		UE_LOG(LogTemp, Warning, TEXT("About to broadcast NewItem with category: %s"),
			*UEnum::GetValueAsString(NewItem->GetItemManifest().GetItemCategory()));

		// This calls a delegate that will update all the grids that are listening to this inventory component. But we only want to add it to a specific grid.
		OnItemAdded.Broadcast(NewItem);

		// Log right after broadcasting
		UE_LOG(LogTemp, Warning, TEXT("After broadcast, NewItem category is: %s"),
			*UEnum::GetValueAsString(NewItem->GetItemManifest().GetItemCategory()));
	}

	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
	}
	else if (FInv_StackableFragment* StackableFragment = ItemComponent->GetItemManifestMutable().GetFragmentOfTypeMutable<FInv_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
	}
}

// Updated Server_AddStacksToItem to also accept category parameter
void UInv_InventoryComponent::Server_AddStacksToItem_Implementation(UInv_ItemComponent* ItemComponent, EInv_ItemCategory Category, int32 StacksToAdd, int32 Remainder)
{
	// Set the category to ensure consistency
	ItemComponent->SetItemCategory(Category);

	// Find the existing item in the inventory
	UInv_InventoryItem* ExistingItem = InventoryList.FindFirstItemByType(ItemComponent->GetItemManifest().GetItemType());

	if (ExistingItem)
	{
		// Ensure the existing item also has the correct category
		ExistingItem->GetItemManifestMutable().SetItemCategory(Category);

		// Add the stacks to the existing item
		int32 CurrentStacks = ExistingItem->GetTotalStackCount();
		ExistingItem->SetTotalStackCount(CurrentStacks + StacksToAdd);

		// Broadcast the change
		if (GetOwner()->GetNetMode() == NM_ListenServer || GetOwner()->GetNetMode() == NM_Standalone)
		{
			OnStackChange.Broadcast(FInv_SlotAvailabilityResult()); // You may need to construct this properly
		}
	}

	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
	}
	else if (FInv_StackableFragment* StackableFragment = ItemComponent->GetItemManifestMutable().GetFragmentOfTypeMutable<FInv_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
	}
}


void UInv_InventoryComponent::Client_UpdateItemCategory_Implementation(UInv_InventoryItem* Item, EInv_ItemCategory Category)
{
	if (Item)
	{
		auto& MutableManifest = Item->GetItemManifestMutable();
		MutableManifest.SetItemCategory(Category);
	}
}

void UInv_InventoryComponent::Server_DropItem_Implementation(UInv_InventoryItem* Item, int32 StackCount)
{
	const int32 NewStackCount = Item->GetTotalStackCount() - StackCount;
	if (NewStackCount <= 0)
	{
		InventoryList.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}

	SpawnDroppedItem(Item, StackCount);
}

void UInv_InventoryComponent::SpawnDroppedItem(UInv_InventoryItem* Item, int32 StackCount)
{
	const APawn* OwningPawn = OwningController->GetPawn();
	FVector RotatedForward = OwningPawn->GetActorForwardVector();
	RotatedForward = RotatedForward.RotateAngleAxis(FMath::FRandRange(DropSpawnAngleMin, DropSpawnAngleMax), FVector::UpVector);
	FVector SpawnLocation = OwningPawn->GetActorLocation() + RotatedForward * FMath::FRandRange(DropSpawnDistanceMin, DropSpawnDistanceMax);
	SpawnLocation.Z -= RelativeSpawnElevation;
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FInv_ItemManifest& ItemManifest = Item->GetItemManifestMutable();
	if (FInv_StackableFragment* StackableFragment = ItemManifest.GetFragmentOfTypeMutable<FInv_StackableFragment>())
	{
		StackableFragment->SetStackCount(StackCount);
	}
	ItemManifest.SpawnPickupActor(this, SpawnLocation, SpawnRotation);
}

void UInv_InventoryComponent::Server_ConsumeItem_Implementation(UInv_InventoryItem* Item)
{
	const int32 NewStackCount = Item->GetTotalStackCount() - 1;
	if (NewStackCount <= 0)
	{
		InventoryList.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}

	if (FInv_ConsumableFragment* ConsumableFragment = Item->GetItemManifestMutable().GetFragmentOfTypeMutable<FInv_ConsumableFragment>())
	{
		ConsumableFragment->OnConsume(OwningController.Get());
	}
}

void UInv_InventoryComponent::Server_EquipSlotClicked_Implementation(UInv_InventoryItem* ItemToEquip, UInv_InventoryItem* ItemToUnequip)
{
	Multicast_EquipSlotClicked(ItemToEquip, ItemToUnequip);
}

void UInv_InventoryComponent::Multicast_EquipSlotClicked_Implementation(UInv_InventoryItem* ItemToEquip, UInv_InventoryItem* ItemToUnequip)
{
	// Equipment Component will listen to these delegates
	OnItemEquipped.Broadcast(ItemToEquip);
	OnItemUnequipped.Broadcast(ItemToUnequip);
}

void UInv_InventoryComponent::ToggleInventoryMenu()
{
	if (bInventoryMenuOpen)
	{
		CloseInventoryMenu();
	}
	else
	{
		OpenInventoryMenu();
	}
	OnInventoryMenuToggled.Broadcast(bInventoryMenuOpen);
}

void UInv_InventoryComponent::AddRepSubObj(UObject* SubObj)
{
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObj))
	{
		AddReplicatedSubObject(SubObj);
	}
}

void UInv_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	ConstructInventory();
}

void UInv_InventoryComponent::ConstructInventory()
{
	OwningController = Cast<APlayerController>(GetOwner());
	checkf(OwningController.IsValid(), TEXT("Inventory Component should have a Player Controller as Owner."))
	if (!OwningController->IsLocalController()) return;

	InventoryMenu = CreateWidget<UInv_InventoryBase>(OwningController.Get(), InventoryMenuClass);
	InventoryMenu->AddToViewport();
	CloseInventoryMenu();
}

void UInv_InventoryComponent::OpenInventoryMenu()
{
	if (!IsValid(InventoryMenu)) return;

	InventoryMenu->SetVisibility(ESlateVisibility::Visible);
	bInventoryMenuOpen = true;

	if (!OwningController.IsValid()) return;

	FInputModeGameAndUI InputMode;
	OwningController->SetInputMode(InputMode);
	OwningController->SetShowMouseCursor(true);
}

void UInv_InventoryComponent::CloseInventoryMenu()
{
	if (!IsValid(InventoryMenu)) return;

	InventoryMenu->SetVisibility(ESlateVisibility::Collapsed);
	bInventoryMenuOpen = false;

	if (!OwningController.IsValid()) return;

	FInputModeGameOnly InputMode;
	OwningController->SetInputMode(InputMode);
	OwningController->SetShowMouseCursor(false);
}

void UInv_ItemComponent::SetItemCategory(EInv_ItemCategory Category)
{
	ItemManifest.SetItemCategory(Category);
}
