#include "PCH.h"

#include "TerrainHeight.h"

#include <cstdint>

namespace
{
	// Game cell edge length in world units (FO4 / Creation Engine).
	constexpr float kCellWorldSize = 4096.0f;
	// One landscape quadrant is 17x17 vertices (Skyrim/FO4 runtime layout).
	constexpr int   kVertsPerEdge = 17;
	constexpr int   kVertsPerQuad = 289;  // 17 * 17
	// Offset of `heights[0][0]` in `LoadedLandData` — follows NiNode*[4] mesh.
	constexpr std::size_t kHeightsOffset = 0x20;

	[[nodiscard]] std::int32_t CellGridX(RE::TESObjectCELL* a_cell)
	{
		using func_t = std::int32_t (*)(RE::TESObjectCELL*);
		static REL::Relocation<func_t> func{ REL::ID(445210) };
		return func(a_cell);
	}

	[[nodiscard]] std::int32_t CellGridY(RE::TESObjectCELL* a_cell)
	{
		using func_t = std::int32_t (*)(RE::TESObjectCELL*);
		static REL::Relocation<func_t> func{ REL::ID(1322816) };
		return func(a_cell);
	}

	// Mirrors CommonLibSSE `TESObjectLAND::LoadedLandData` through `heights`.
	struct LoadedLandDataHeightsOnly
	{
		std::byte pad0[kHeightsOffset]{};
		float     heights[4][kVertsPerQuad]{};
	};

	[[nodiscard]] bool FloatFinite(float a_v)
	{
		return std::isfinite(a_v) && a_v > -500000.0f && a_v < 500000.0f;
	}

	[[nodiscard]] float BilinearQuad(const float* a_quad, float a_fu, float a_fv)
	{
		// a_fu, a_fv in [0, kVertsPerEdge - 1] continuous (maps across 17 verts).
		const float clampedU = std::clamp(a_fu, 0.0f, static_cast<float>(kVertsPerEdge - 1));
		const float clampedV = std::clamp(a_fv, 0.0f, static_cast<float>(kVertsPerEdge - 1));

		const int x0 = static_cast<int>(std::floor(clampedU));
		const int y0 = static_cast<int>(std::floor(clampedV));
		const int x1 = std::min(x0 + 1, kVertsPerEdge - 1);
		const int y1 = std::min(y0 + 1, kVertsPerEdge - 1);

		const float tx = clampedU - static_cast<float>(x0);
		const float ty = clampedV - static_cast<float>(y0);

		const float h00 = a_quad[y0 * kVertsPerEdge + x0];
		const float h10 = a_quad[y0 * kVertsPerEdge + x1];
		const float h01 = a_quad[y1 * kVertsPerEdge + x0];
		const float h11 = a_quad[y1 * kVertsPerEdge + x1];

		const float h0 = h00 * (1.0f - tx) + h10 * tx;
		const float h1 = h01 * (1.0f - tx) + h11 * tx;
		return h0 * (1.0f - ty) + h1 * ty;
	}
}

float TerrainHeight::SampleTerrainHeight(
	RE::TESObjectCELL* a_cell,
	float              a_worldX,
	float              a_worldY,
	float              a_fallbackZ) noexcept
{
	if (!a_cell || a_cell->IsInterior()) {
		return a_fallbackZ;
	}

	auto* land = a_cell->cellLand;
	if (!land || !land->loadedData) {
		if (a_cell->worldSpace != nullptr && FloatFinite(a_cell->worldSpace->defaultLandHeight)) {
			return a_cell->worldSpace->defaultLandHeight;
		}
		return a_fallbackZ;
	}

	const auto* ld = reinterpret_cast<const LoadedLandDataHeightsOnly*>(land->loadedData);

	const std::int32_t cx = CellGridX(a_cell);
	const std::int32_t cy = CellGridY(a_cell);

	const float cellOriginX = static_cast<float>(cx) * kCellWorldSize;
	const float cellOriginY = static_cast<float>(cy) * kCellWorldSize;

	const float lx = a_worldX - cellOriginX;
	const float ly = a_worldY - cellOriginY;

	if (lx < -0.5f || lx >= kCellWorldSize + 0.5f || ly < -0.5f || ly >= kCellWorldSize + 0.5f) {
		if (a_cell->worldSpace != nullptr && FloatFinite(a_cell->worldSpace->defaultLandHeight)) {
			return a_cell->worldSpace->defaultLandHeight;
		}
		return a_fallbackZ;
	}

	const float clampedLx = std::clamp(lx, 0.0f, kCellWorldSize - 1.0e-3f);
	const float clampedLy = std::clamp(ly, 0.0f, kCellWorldSize - 1.0e-3f);

	const int halfX = clampedLx >= kCellWorldSize * 0.5f ? 1 : 0;
	const int halfY = clampedLy >= kCellWorldSize * 0.5f ? 1 : 0;
	const int quad = halfY * 2 + halfX;

	const float lxq = clampedLx - static_cast<float>(halfX) * (kCellWorldSize * 0.5f);
	const float lyq = clampedLy - static_cast<float>(halfY) * (kCellWorldSize * 0.5f);

	const float fu = (lxq / (kCellWorldSize * 0.5f)) * static_cast<float>(kVertsPerEdge - 1);
	const float fv = (lyq / (kCellWorldSize * 0.5f)) * static_cast<float>(kVertsPerEdge - 1);

	const float* quadHeights = ld->heights[quad];
	const float    h = BilinearQuad(quadHeights, fu, fv);

	if (!FloatFinite(h)) {
		if (a_cell->worldSpace != nullptr && FloatFinite(a_cell->worldSpace->defaultLandHeight)) {
			return a_cell->worldSpace->defaultLandHeight;
		}
		return a_fallbackZ;
	}

	return h;
}
