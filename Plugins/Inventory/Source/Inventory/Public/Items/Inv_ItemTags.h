#pragma once

#include "NativeGameplayTags.h"

namespace GameItems
{
	namespace Equipment
	{
		// Weapon categories

		namespace Weapons
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Axe)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sword)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Knife)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bloodthorn)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(NightsEdge)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shadowbane)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Staff)
		}

		// Armor categories

		namespace Helmets
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeatherHelmet)
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(ChainmailHelmet)
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(PlateHelmet)
		}

		namespace Chestplates
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeatherChest)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ChainmailChest)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PlateChest)
		}

		namespace Gloves
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeatherGloves)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ChainmailGloves)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PlateGloves)
		}

		namespace Legs
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeatherLegs)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ChainmailLegs)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PlateLegs)
		}

		namespace Boots
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LeatherBoots)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ChainmailBoots)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PlateBoots)
		}

		// Accessories categories
		
		namespace Necklaces
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WoodenNecklace)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(IronNecklace)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SteelNecklace)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicNecklace)
		}

		namespace Rings
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WoodenRing)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(IronRing)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SteelRing)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicRing)
		}

		namespace Belts
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WoodenBelt)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(IronBelt)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SteelBelt)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicBelt)
		}

		// Containers categories

		namespace Backpacks
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WoodenBackpack)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(IronBackpack)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SteelBackpack)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicBackpack)
		}

		namespace Satchels
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WoodenSatchel)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(IronSatchel)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SteelSatchel)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicSatchel)
		}

		namespace Quivers
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WoodenQuiver)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(IronQuiver)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SteelQuiver)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicQuiver)
		}

	}

	namespace Consumables
	{
		namespace Potions
		{
			namespace Red
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Small)
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Large)
			}

			namespace Blue
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Small)
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Large)
			}
		}
	}

	namespace Craftables
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(FireFernFruit)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(LuminDaisy)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ScorchPetalBlossom)
	}
}
