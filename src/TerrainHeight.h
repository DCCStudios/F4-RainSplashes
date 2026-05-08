#pragma once

namespace RE
{
	class TESObjectCELL;
}

namespace TerrainHeight
{
	// Bilinear sample of the loaded land heightmap at world XY. Returns
	// `a_fallbackZ` if the cell has no land data or sampling fails. Layout
	// matches CommonLib SSE/FO4 `TESObjectLAND::LoadedLandData` (heights[4][289]
	// at offset 0x20) — see TerrainHeight.cpp.
	[[nodiscard]] float SampleTerrainHeight(
		RE::TESObjectCELL* a_cell,
		float              a_worldX,
		float              a_worldY,
		float              a_fallbackZ) noexcept;
}
