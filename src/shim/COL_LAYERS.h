#pragma once

namespace RE
{
	enum class COL_LAYER
	{
		kUnidentified = 0,
		kStatic = 1,
		kAnimStatic = 2,
		kTransparent = 3,
		kClutter = 4,
		kWeapon = 5,
		kProjectile = 6,
		kSpell = 7,
		kBiped = 8,
		kTrees = 9,
		kProps = 10,
		kWater = 11,
		kTrigger = 12,
		kTerrain = 13,
		kTrap = 14,
		kNonCollidable = 15,
		kCloudTrap = 16,
		kGround = 17,
		kPortal = 18,
		kDebrisSmall = 19,
		kDebrisLarge = 20,
		kAcousticSpace = 21,
		kActorZone = 22,
		kProjectileZone = 23,
		kGasTrap = 24,
		kShellCasing = 25,
		kTransparentSmall = 26,
		kInvisibleWall = 27,
		kTransparentSmallAnim = 28,
		kClutterLarge = 29,
		kCharController = 30,
		kStairHelper = 31,
		kDeadBip = 32,
		kBipedNoCC = 33,
		kAvoidBox = 34,
		kCollisionBox = 35,
		kCameraSphere = 36,
		kDoorDetection = 37,
		kConeProjectile = 38,
		kCamera = 39,
		kItemPicker = 40,
		kLOS = 41,
		kPathingPick = 42,
		kUnused0 = 43,
		kUnused1 = 44,
		kSpellExplosion = 45,
		kDroppingPick = 46,
		kNumFilterLayers = 47,
		kDataDefinedLayersBegin = kNumFilterLayers,
		kDataDefined1 = 48,
		kDataDefined2 = 49,
		kDataDefined3 = 50,
		kDataDefined4 = 51,
		kDataDefined5 = 52,
		kDataDefined6 = 53,
		kDataDefined7 = 54,
		kDataDefined8 = 55,
		kDataDefined9 = 56,
		kDataDefined10 = 57,
		kDataDefined11 = 58,
		kDataDefined12 = 59,
		kDataDefined13 = 60,
		kDataDefined14 = 61,
		kDataDefined15 = 62,
		kDataDefined16 = 63,
		kMaxLayerSize = 64
	};

	inline std::string_view CollisionLayerToString(COL_LAYER a_layer) noexcept
	{
		switch (a_layer) {
		case COL_LAYER::kUnidentified:
			return "Unidentified";
		case COL_LAYER::kStatic:
			return "Static";
		case COL_LAYER::kAnimStatic:
			return "AnimStatic";
		case COL_LAYER::kTransparent:
			return "Transparent";
		case COL_LAYER::kClutter:
			return "Clutter";
		case COL_LAYER::kWeapon:
			return "Weapon";
		case COL_LAYER::kProjectile:
			return "Projectile";
		case COL_LAYER::kSpell:
			return "Spell";
		case COL_LAYER::kBiped:
			return "Biped";
		case COL_LAYER::kTrees:
			return "Trees";
		case COL_LAYER::kProps:
			return "Props";
		case COL_LAYER::kWater:
			return "Water";
		case COL_LAYER::kTrigger:
			return "Trigger";
		case COL_LAYER::kTerrain:
			return "Terrain";
		case COL_LAYER::kTrap:
			return "Trap";
		case COL_LAYER::kNonCollidable:
			return "NonCollidable";
		case COL_LAYER::kCloudTrap:
			return "CloudTrap";
		case COL_LAYER::kGround:
			return "Ground";
		case COL_LAYER::kPortal:
			return "Portal";
		case COL_LAYER::kDebrisSmall:
			return "DebrisSmall";
		case COL_LAYER::kDebrisLarge:
			return "DebrisLarge";
		case COL_LAYER::kAcousticSpace:
			return "AcousticSpace";
		case COL_LAYER::kActorZone:
			return "ActorZone";
		case COL_LAYER::kProjectileZone:
			return "ProjectileZone";
		case COL_LAYER::kGasTrap:
			return "GasTrap";
		case COL_LAYER::kShellCasing:
			return "ShellCasing";
		case COL_LAYER::kTransparentSmall:
			return "TransparentSmall";
		case COL_LAYER::kInvisibleWall:
			return "InvisibleWall";
		case COL_LAYER::kTransparentSmallAnim:
			return "TransparentSmallAnim";
		case COL_LAYER::kClutterLarge:
			return "ClutterLarge";
		case COL_LAYER::kCharController:
			return "CharController";
		case COL_LAYER::kStairHelper:
			return "StairHelper";
		case COL_LAYER::kDeadBip:
			return "DeadBip";
		case COL_LAYER::kBipedNoCC:
			return "BipedNoCC";
		case COL_LAYER::kAvoidBox:
			return "AvoidBox";
		case COL_LAYER::kCollisionBox:
			return "CollisionBox";
		case COL_LAYER::kCameraSphere:
			return "CameraSphere";
		case COL_LAYER::kDoorDetection:
			return "DoorDetection";
		case COL_LAYER::kConeProjectile:
			return "ConeProjectile";
		case COL_LAYER::kCamera:
			return "Camera";
		case COL_LAYER::kItemPicker:
			return "ItemPicker";
		case COL_LAYER::kLOS:
			return "LOS";
		case COL_LAYER::kPathingPick:
			return "PathingPick";
		case COL_LAYER::kUnused0:
			return "Unused0";
		case COL_LAYER::kUnused1:
			return "Unused1";
		case COL_LAYER::kSpellExplosion:
			return "SpellExplosion";
		case COL_LAYER::kDroppingPick:
			return "DroppingPick";
		case COL_LAYER::kDataDefined1:
			return "DataDefined1";
		case COL_LAYER::kDataDefined2:
			return "DataDefined2";
		case COL_LAYER::kDataDefined3:
			return "DataDefined3";
		case COL_LAYER::kDataDefined4:
			return "DataDefined4";
		case COL_LAYER::kDataDefined5:
			return "DataDefined5";
		case COL_LAYER::kDataDefined6:
			return "DataDefined6";
		case COL_LAYER::kDataDefined7:
			return "DataDefined7";
		case COL_LAYER::kDataDefined8:
			return "DataDefined8";
		case COL_LAYER::kDataDefined9:
			return "DataDefined9";
		case COL_LAYER::kDataDefined10:
			return "DataDefined10";
		case COL_LAYER::kDataDefined11:
			return "DataDefined11";
		case COL_LAYER::kDataDefined12:
			return "DataDefined12";
		case COL_LAYER::kDataDefined13:
			return "DataDefined13";
		case COL_LAYER::kDataDefined14:
			return "DataDefined14";
		case COL_LAYER::kDataDefined15:
			return "DataDefined15";
		case COL_LAYER::kDataDefined16:
			return "DataDefined16";
		default:
			return "Unknown";
		}
	}
}

namespace std
{
	template <class CharT>
	struct formatter<RE::COL_LAYER, CharT> : std::formatter<std::string_view, CharT>
	{
		template <class FormatContext>
		auto format(RE::COL_LAYER a_layer, FormatContext& a_ctx) const
		{
			return formatter<std::string_view, CharT>::format(RE::CollisionLayerToString(a_layer), a_ctx);
		}
	};
}
