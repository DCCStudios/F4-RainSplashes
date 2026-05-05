#include "RayCast.h"

#include "shim/COL_LAYERS.h"
#include "shim/MainShim.h"
#include "shim/NiCameraShim.h"
#include "RE/Havok/hknpCollisionResult.h"
#include "RE/NetImmerse/NiMatrix3.h"

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

	RE::NiAVObject* CellPick(RE::TESObjectCELL* a_cell, RE::bhkPickData& a_pick)
	{
		using func_t = RE::NiAVObject* (*)(RE::TESObjectCELL*, RE::bhkPickData&);
		static REL::Relocation<func_t> func{ REL::ID(434717) };
		return func(a_cell, a_pick);
	}

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

		if (!a_requireInCameraFrustum) {
			return p;
		}
		auto* cam = RE::Main::WorldRootCamera();
		if (!cam) {
			return p;
		}
		if (cam->PointInFrustum(p, 32.0f)) {
			return p;
		}
		return std::nullopt;
	}

	std::optional<Output> CastVerticalCell(RE::TESObjectCELL* a_cell, const RE::NiPoint3& a_xyOrigin)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || !a_cell || a_cell != player->GetParentCell()) {
			return std::nullopt;
		}

		constexpr float kHeight = 9999.0f;
		RE::NiPoint3 rayStart = a_xyOrigin;
		RE::NiPoint3 rayEnd = a_xyOrigin;
		rayStart.z += kHeight;
		rayEnd.z -= kHeight;

		RE::bhkPickData pick{};
		pick.SetCollisionLayer(RE::COL_LAYER::kLOS);
		pick.SetStartEnd(rayStart, rayEnd);

		auto* niHit = CellPick(a_cell, pick);
		const bool hasHit = pick.HasHit();

		if (!hasHit && !niHit) {
			return std::nullopt;
		}

		Output out{};
		const float frac = pick.GetHitFraction();
		RE::NiPoint3 dir{};
		dir.x = rayEnd.x - rayStart.x;
		dir.y = rayEnd.y - rayStart.y;
		dir.z = rayEnd.z - rayStart.z;
		out.hitPos.x = rayStart.x + dir.x * frac;
		out.hitPos.y = rayStart.y + dir.y * frac;
		out.hitPos.z = rayStart.z + dir.z * frac;

		static bool g_loggedHit = false;
		if (!g_loggedHit) {
			logger::info("RainSplashesF4SE: first hit — frac={:.6f} pos=({:.0f},{:.0f},{:.0f})",
				frac, out.hitPos.x, out.hitPos.y, out.hitPos.z);
			g_loggedHit = true;
		}

		if (PointUnderCellWaterPlane(a_cell, out.hitPos)) {
			out.hitWater = true;
			out.hitPos.z = a_cell->waterHeight;
		}

		std::uniform_real_distribution<float> yaw(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
		MatrixYawAroundZ(out.normal, yaw(g_rng));

		return out;
	}
}
