#include "Items/Inv_ItemTags.h"

namespace GameItems
{

	namespace Equipment
	{
		// Main weapons
		namespace MainWeapons
		{
			UE_DEFINE_GAMEPLAY_TAG(Axe, "GameItems.Equipment.Weapons.Axe");
			UE_DEFINE_GAMEPLAY_TAG(Sword, "GameItems.Equipment.Weapons.Sword");
			UE_DEFINE_GAMEPLAY_TAG(Knife, "GameItems.Equipment.Weapons.Knife");
			UE_DEFINE_GAMEPLAY_TAG(Staff, "GameItems.Equipment.Weapons.Staff");
		}

		// Offhand weapons
		namespace OffhandWeapons
		{
			UE_DEFINE_GAMEPLAY_TAG(Shield, "GameItems.Equipment.OffhandWeapons.Shield");
			UE_DEFINE_GAMEPLAY_TAG(Torch, "GameItems.Equipment.OffhandWeapons.Torch");
			UE_DEFINE_GAMEPLAY_TAG(Spellbook, "GameItems.Equipment.OffhandWeapons.Spellbook");
			UE_DEFINE_GAMEPLAY_TAG(Wand, "GameItems.Equipment.OffhandWeapons.Wand");
		}

		// Armor categories
		namespace Helmets
		{
			UE_DEFINE_GAMEPLAY_TAG(LeatherHelmet, "GameItems.Equipment.Helmets.LeatherHelmet")
				UE_DEFINE_GAMEPLAY_TAG(ChainmailHelmet, "GameItems.Equipment.Helmets.ChainmailHelmet")
				UE_DEFINE_GAMEPLAY_TAG(PlateHelmet, "GameItems.Equipment.Helmets.PlateHelmet")
		}

		namespace Chest
		{
			UE_DEFINE_GAMEPLAY_TAG(LeatherChest, "GameItems.Equipment.Chest.LeatherChest")
				UE_DEFINE_GAMEPLAY_TAG(ChainmailChest, "GameItems.Equipment.Chest.ChainmailChest")
				UE_DEFINE_GAMEPLAY_TAG(PlateChest, "GameItems.Equipment.Chest.PlateChest")
		}

		namespace Gloves
		{
			UE_DEFINE_GAMEPLAY_TAG(LeatherGloves, "GameItems.Equipment.Gloves.LeatherGloves")
				UE_DEFINE_GAMEPLAY_TAG(ChainmailGloves, "GameItems.Equipment.Gloves.ChainmailGloves")
				UE_DEFINE_GAMEPLAY_TAG(PlateGloves, "GameItems.Equipment.Gloves.PlateGloves")
		}

		namespace Legs
		{
			UE_DEFINE_GAMEPLAY_TAG(LeatherLegs, "GameItems.Equipment.Legs.LeatherLegs")
				UE_DEFINE_GAMEPLAY_TAG(ChainmailLegs, "GameItems.Equipment.Legs.ChainmailLegs")
				UE_DEFINE_GAMEPLAY_TAG(PlateLegs, "GameItems.Equipment.Legs.PlateLegs")
		}

		namespace Boots
		{
			UE_DEFINE_GAMEPLAY_TAG(LeatherBoots, "GameItems.Equipment.Boots.LeatherBoots")
				UE_DEFINE_GAMEPLAY_TAG(ChainmailBoots, "GameItems.Equipment.Boots.ChainmailBoots")
				UE_DEFINE_GAMEPLAY_TAG(PlateBoots, "GameItems.Equipment.Boots.PlateBoots")
		}

		// Accessories categories
		namespace Necklaces
		{
			UE_DEFINE_GAMEPLAY_TAG(WoodenNecklace, "GameItems.Equipment.Necklaces.WoodenNecklace")
				UE_DEFINE_GAMEPLAY_TAG(IronNecklace, "GameItems.Equipment.Necklaces.IronNecklace")
				UE_DEFINE_GAMEPLAY_TAG(SteelNecklace, "GameItems.Equipment.Necklaces.SteelNecklace")
				UE_DEFINE_GAMEPLAY_TAG(MagicNecklace, "GameItems.Equipment.Necklaces.MagicNecklace")
		}

		namespace Rings
		{
			UE_DEFINE_GAMEPLAY_TAG(WoodenRing, "GameItems.Equipment.Rings.WoodenRing")
				UE_DEFINE_GAMEPLAY_TAG(IronRing, "GameItems.Equipment.Rings.IronRing")
				UE_DEFINE_GAMEPLAY_TAG(SteelRing, "GameItems.Equipment.Rings.SteelRing")
				UE_DEFINE_GAMEPLAY_TAG(MagicRing, "GameItems.Equipment.Rings.MagicRing")
		}

		namespace Belts
		{
			UE_DEFINE_GAMEPLAY_TAG(WoodenBelt, "GameItems.Equipment.Belts.WoodenBelt")
				UE_DEFINE_GAMEPLAY_TAG(IronBelt, "GameItems.Equipment.Belts.IronBelt")
				UE_DEFINE_GAMEPLAY_TAG(SteelBelt, "GameItems.Equipment.Belts.SteelBelt")
				UE_DEFINE_GAMEPLAY_TAG(MagicBelt, "GameItems.Equipment.Belts.MagicBelt")
		}

		// Containers
		namespace Backpacks
		{
			UE_DEFINE_GAMEPLAY_TAG(WoodenBackpack, "GameItems.Equipment.Backpacks.WoodenBackpack")
				UE_DEFINE_GAMEPLAY_TAG(IronBackpack, "GameItems.Equipment.Backpacks.IronBackpack")
				UE_DEFINE_GAMEPLAY_TAG(SteelBackpack, "GameItems.Equipment.Backpacks.SteelBackpack")
				UE_DEFINE_GAMEPLAY_TAG(MagicBackpack, "GameItems.Equipment.Backpacks.MagicBackpack")
		}

		namespace Satchels
		{
			UE_DEFINE_GAMEPLAY_TAG(WoodenSatchel, "GameItems.Equipment.Satchels.WoodenSatchel")
				UE_DEFINE_GAMEPLAY_TAG(IronSatchel, "GameItems.Equipment.Satchels.IronSatchel")
				UE_DEFINE_GAMEPLAY_TAG(SteelSatchel, "GameItems.Equipment.Satchels.SteelSatchel")
				UE_DEFINE_GAMEPLAY_TAG(MagicSatchel, "GameItems.Equipment.Satchels.MagicSatchel")
		}

		namespace Quivers
		{
			UE_DEFINE_GAMEPLAY_TAG(WoodenQuiver, "GameItems.Equipment.Quivers.WoodenQuiver")
				UE_DEFINE_GAMEPLAY_TAG(IronQuiver, "GameItems.Equipment.Quivers.IronQuiver")
				UE_DEFINE_GAMEPLAY_TAG(SteelQuiver, "GameItems.Equipment.Quivers.SteelQuiver")
				UE_DEFINE_GAMEPLAY_TAG(MagicQuiver, "GameItems.Equipment.Quivers.MagicQuiver")
		}

	}

	namespace Consumables
	{
		namespace Potions
		{
			namespace Red
			{
				UE_DEFINE_GAMEPLAY_TAG(Small, "GameItems.Consumables.Potions.Red.Small")
				UE_DEFINE_GAMEPLAY_TAG(Large, "GameItems.Consumables.Potions.Red.Large")
			}

			namespace Blue
			{
				UE_DEFINE_GAMEPLAY_TAG(Small, "GameItems.Consumables.Potions.Blue.Small")
				UE_DEFINE_GAMEPLAY_TAG(Large, "GameItems.Consumables.Potions.Blue.Large")
			}
		}
	}

	namespace Craftables
	{
		UE_DEFINE_GAMEPLAY_TAG(FireFernFruit, "GameItems.Craftables.FireFernFruit")
		UE_DEFINE_GAMEPLAY_TAG(LuminDaisy, "GameItems.Craftables.LuminDaisy")
		UE_DEFINE_GAMEPLAY_TAG(ScorchPetalBlossom, "GameItems.Craftables.ScorchPetalBlossom")
	}
}
