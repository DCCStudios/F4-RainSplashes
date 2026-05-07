#include "RayCast.h"

#include "shim/MainShim.h"
#include "shim/NiCameraShim.h"
#include "RE/NetImmerse/NiMatrix3.h"

#include <cmath>

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

		// No Havok raycasting — scan loaded references for the best surface
		// height at this XY.  This avoids all bhkWorld threading issues.
		Output out{};
		out.hitPos.x = a_xyOrigin.x;
		out.hitPos.y = a_xyOrigin.y;
		out.hitPos.z = playerZ;

		constexpr float kRefScanRadius = 128.0f;
		constexpr float kRefScanRadiusSq = kRefScanRadius * kRefScanRadius;
		float bestDist2 = kRefScanRadiusSq;
		bool  foundRef = false;

		auto& refs = a_cell->references;
		for (std::uint32_t i = 0; i < refs.size(); ++i) {
			auto* ref = refs[i].get();
			if (!ref) continue;

			const float dx = ref->data.location.x - a_xyOrigin.x;
			const float dy = ref->data.location.y - a_xyOrigin.y;
			const float d2 = dx * dx + dy * dy;
			if (d2 >= bestDist2) continue;

			const float refZ = ref->data.location.z;
			const float dzFromPlayer = refZ - playerZ;
			if (dzFromPlayer < -500.0f || dzFromPlayer > 500.0f) continue;

			bestDist2 = d2;
			out.hitPos.z = refZ;
			foundRef = true;

			if (ref->GetFormType() == RE::ENUM_FORM_ID::kACHR) {
				out.hitActor = true;
				const float pdx = refZ - playerZ;
				const float pdx2 = ref->data.location.x - playerX;
				const float pdy2 = ref->data.location.y - playerY;
				constexpr float kPlayerRadius = 50.0f;
				out.hitPlayer = (pdx2 * pdx2 + pdy2 * pdy2) < (kPlayerRadius * kPlayerRadius);
			} else {
				out.hitActor = false;
				out.hitPlayer = false;
			}
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
