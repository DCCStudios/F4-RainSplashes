#pragma once

namespace RE
{
	class TESObjectCELL;
}

namespace TerrainHeight
{
	// Bilinear sample of the loaded land heightmap at world XY. Succeeds only
	// when the XY lies inside `a_cell` and its land data is loaded and finite —
	// no worldspace-default or caller-Z fallbacks. Layout matches CommonLib
	// SSE/FO4 `TESObjectLAND::LoadedLandData` (heights[4][289] at offset 0x20)
	// — see TerrainHeight.cpp.
	[[nodiscard]] bool TrySampleTerrainHeight(
		RE::TESObjectCELL* a_cell,
		float              a_worldX,
		float              a_worldY,
		float&             a_outZ) noexcept;
}
