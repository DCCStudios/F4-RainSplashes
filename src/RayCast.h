#pragma once

#include "shim/COL_LAYERS.h"

namespace RayCast
{
	struct Options
	{
		bool acceptActors{ false };
		bool acceptPlayer{ false };
	};

	struct Output
	{
		RE::NiPoint3  hitPos{};
		// Rotation aligning local +Z to the hit surface normal, random yaw.
		RE::NiMatrix3 rotation{};
		// Collision layer of the winning hit (kUnidentified for the heightmap
		// fallback) — diagnostics only.
		RE::COL_LAYER layer{ RE::COL_LAYER::kUnidentified };
		bool          hitActor{ false };
		bool          hitPlayer{ false };
		bool          hitWater{ false };
		// false when Z came from the land heightmap fallback or a
		// terrain-layer hit — cover-vs-playerZ is misleading on slopes there.
		bool          hitRefContributesZ{ false };
	};

	std::optional<RE::NiPoint3> RandomPointInDiskAround(float a_radius, const RE::NiPoint3& a_center, bool a_requireInCameraFrustum);

	// Vertical Havok pick (LOS layer) from above the player's Z down past the
	// terrain: the topmost solid hit is where rain lands, so roofs shield the
	// ground beneath them.  Non-solid helper volumes and unwanted actor hits
	// are skipped by re-casting past them; if nothing solid is hit, falls back
	// to the loaded land heightmap (and declines the sample when that isn't
	// available either).  MAIN THREAD ONLY — drives engine pick machinery.
	std::optional<Output> CastVerticalCell(
		RE::TESObjectCELL*  a_cell,
		const RE::NiPoint3& a_xyOrigin,
		const Options&      a_opts);
}
