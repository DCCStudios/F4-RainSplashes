#include "RayCast.h"

#include "TerrainHeight.h"

#include "shim/MainShim.h"
#include "shim/NiCameraShim.h"
#include "RE/NetImmerse/NiMatrix3.h"

#include <cmath>
#include <limits>

namespace
{
	void MatrixYawAroundZ(RE::NiMatrix3& a_mat, float a_yaw)
	{
		const float c = std::cos(a_yaw);
		const float s = std::sin(a_yaw);
		a_mat.entry[0].v = { c, -s, 0.0f, 0.0f };
		a_mat.entry[1].v = { s, c, 0.0f, 0.0f };
		a_mat.entry[2].v = { 0.0f, 0.0f, 1.0f, 0.0f };
	}

	thread_local std::mt19937 g_rng{ std::random_device{}() };

	bool PointUnderCellWaterPlane(RE::TESObjectCELL* a_cell, const RE::NiPoint3& a_pos)
	{
		if (!a_cell || !a_cell->HasWater()) {
			return false;
		}
		return a_pos.z < a_cell->waterHeight;
	}

	[[nodiscard]] bool VecFinite(const RE::NiPoint3& a_v)
	{
		return std::isfinite(a_v.x) && std::isfinite(a_v.y) && std::isfinite(a_v.z);
	}

	[[nodiscard]] bool BoundsReasonable(const RE::NiPoint3& a_min, const RE::NiPoint3& a_max)
	{
		if (!VecFinite(a_min) || !VecFinite(a_max)) {
			return false;
		}
		constexpr float kHuge = 100000.0f;
		if (a_max.x < a_min.x || a_max.y < a_min.y || a_max.z < a_min.z) {
			return false;
		}
		if ((a_max.x - a_min.x) > kHuge || (a_max.y - a_min.y) > kHuge || (a_max.z - a_min.z) > kHuge) {
			return false;
		}
		return true;
	}
}

namespace RayCast
{
	std::optional<RE::NiPoint3> RandomPointInDiskAround(float a_radius, const RE::NiPoint3& a_center, bool a_requireInCameraFrustum)
	{
		std::uniform_real_distribution<float> u01(0.0f, 1.0f);
		std::uniform_real_distribution<float> uTheta(0.0f, 2.0f * std::numbers::pi_v<float>);
		const float r = a_radius * std::sqrt(u01(g_rng));
		const float theta = uTheta(g_rng);
		RE::NiPoint3 p{
			a_center.x + r * std::cos(theta),
			a_center.y + r * std::sin(theta),
			a_center.z
		};
		if (!a_requireInCameraFrustum) return p;
		auto* cam = RE::Main::WorldRootCamera();
		if (!cam) return p;
		if (cam->PointInFrustum(p, 32.0f)) return p;
		return std::nullopt;
	}

	std::optional<Output> CastVerticalCell(RE::TESObjectCELL* a_cell, const RE::NiPoint3& a_xyOrigin)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || !a_cell || a_cell != player->GetParentCell()) {
			return std::nullopt;
		}

		const float playerZ = player->data.location.z;
		const float playerX = player->data.location.x;
		const float playerY = player->data.location.y;

		const float terrainZ = TerrainHeight::SampleTerrainHeight(a_cell, a_xyOrigin.x, a_xyOrigin.y, playerZ);

		Output out{};
		out.hitPos.x = a_xyOrigin.x;
		out.hitPos.y = a_xyOrigin.y;

		constexpr float kFootprintEps = 3.0f;
		constexpr float kVertVsPlayer = 500.0f;

		float                       bestTopZ = std::numeric_limits<float>::lowest();
		const RE::TESObjectREFR*  winnerRef = nullptr;

		auto& refs = a_cell->references;
		for (std::uint32_t i = 0; i < refs.size(); ++i) {
			auto* ref = refs[i].get();
			if (!ref) {
				continue;
			}

			const RE::NiPoint3 bmin = ref->GetBoundMin();
			const RE::NiPoint3 bmax = ref->GetBoundMax();
			if (!BoundsReasonable(bmin, bmax)) {
				continue;
			}

			const float lx = ref->data.location.x;
			const float ly = ref->data.location.y;
			const float lz = ref->data.location.z;

			const float wx0 = lx + bmin.x;
			const float wx1 = lx + bmax.x;
			const float wy0 = ly + bmin.y;
			const float wy1 = ly + bmax.y;

			if (a_xyOrigin.x < wx0 - kFootprintEps || a_xyOrigin.x > wx1 + kFootprintEps) {
				continue;
			}
			if (a_xyOrigin.y < wy0 - kFootprintEps || a_xyOrigin.y > wy1 + kFootprintEps) {
				continue;
			}

			float topZ = lz + bmax.z;

			if (ref->GetFormType() == RE::ENUM_FORM_ID::kACHR) {
				const float actorH = ref->GetActorHeightOrRefBound();
				if (std::isfinite(actorH) && actorH > 0.0f) {
					topZ = std::max(topZ, lz + actorH);
				}
			}

			if (std::fabs(topZ - playerZ) > kVertVsPlayer) {
				continue;
			}

			if (topZ > bestTopZ) {
				bestTopZ = topZ;
				winnerRef = ref;
			}
		}

		if (winnerRef != nullptr) {
			out.hitPos.z = std::max(terrainZ, bestTopZ);
			out.hitActor = winnerRef->GetFormType() == RE::ENUM_FORM_ID::kACHR;
			if (out.hitActor) {
				const float pdx2 = winnerRef->data.location.x - playerX;
				const float pdy2 = winnerRef->data.location.y - playerY;
				constexpr float kPlayerRadius = 50.0f;
				out.hitPlayer = (pdx2 * pdx2 + pdy2 * pdy2) < (kPlayerRadius * kPlayerRadius);
			} else {
				out.hitPlayer = false;
			}
		} else {
			out.hitPos.z = terrainZ;
			out.hitActor = false;
			out.hitPlayer = false;
		}

		if (PointUnderCellWaterPlane(a_cell, out.hitPos)) {
			out.hitWater = true;
			out.hitPos.z = a_cell->waterHeight;
		}

		MatrixYawAroundZ(out.normal, std::uniform_real_distribution<float>(
			-std::numbers::pi_v<float>, std::numbers::pi_v<float>)(g_rng));

		return out;
	}
}
