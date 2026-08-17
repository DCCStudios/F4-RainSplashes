#include "RayCast.h"

#include "TerrainHeight.h"

#include "shim/MainShim.h"
#include "shim/NiCameraShim.h"
#include "shim/bhkPickData.h"
#include "RE/Bethesda/PlayerCharacter.h"
#include "RE/NetImmerse/NiAVObject.h"
#include "RE/NetImmerse/NiMatrix3.h"

#include <cmath>
#include <limits>

namespace
{
	// Cast extents around the player's Z: high enough to start above roofs
	// the player could plausibly see splashes on, low enough to reach ground
	// downhill of the player.  Without heightmap data (sample XY in a
	// neighboring cell) the ray must reach much deeper — neighbor terrain can
	// sit far below the player, and its collision is still loaded and pickable
	// even though our heightmap sampler only reads the player's cell.
	constexpr float kCastAbove = 2500.0f;
	constexpr float kCastBelow = 600.0f;
	constexpr float kCastBelowNoTerrain = 2500.0f;

	// Skip-and-recast discipline (FPGunplayOverhaul's LOS-ray contract):
	// step past a rejected hit and cast the remainder.  Actors are several
	// hknp bodies stacked (head/torso/pelvis/limbs, plus a char-controller
	// capsule), so actor hits use a coarser step and the attempt budget is
	// sized to clear one whole body plus a few helper volumes.
	constexpr int   kMaxAttempts = 8;
	constexpr float kSkipEpsilon = 4.0f;
	constexpr float kSkipEpsilonActor = 12.0f;

	thread_local std::mt19937 g_rng{ std::random_device{}() };

	bool PointUnderCellWaterPlane(RE::TESObjectCELL* a_cell, const RE::NiPoint3& a_pos)
	{
		if (!a_cell || !a_cell->HasWater()) {
			return false;
		}
		return a_pos.z < a_cell->waterHeight;
	}

	// ALLOW-list of collision layers rain splashes may land on: solid,
	// visible, upward-facing-capable geometry.  Everything else — triggers,
	// portals, invisible walls, avoid/collision boxes, camera spheres, char
	// controller capsules — is the invisible-collider class that registers on
	// ray queries and produced phantom hits in the original pick work.
	// kTrees is deliberately NOT allowed (unlike the FPGunplayOverhaul cover
	// list): tree collision is mostly a trunk cylinder under the canopy, and
	// a splash on a trunk top is invisible — re-casting through to the ground
	// beneath reads as rain dripping through the canopy.
	[[nodiscard]] bool IsRainSolidLayer(RE::COL_LAYER a_layer)
	{
		switch (a_layer) {
		case RE::COL_LAYER::kStatic:
		case RE::COL_LAYER::kAnimStatic:
		case RE::COL_LAYER::kTransparent:
		case RE::COL_LAYER::kClutter:
		case RE::COL_LAYER::kWeapon:
		case RE::COL_LAYER::kProps:
		case RE::COL_LAYER::kTerrain:
		case RE::COL_LAYER::kTrap:
		case RE::COL_LAYER::kGround:
		case RE::COL_LAYER::kDebrisLarge:
		case RE::COL_LAYER::kTransparentSmall:
		case RE::COL_LAYER::kTransparentSmallAnim:
		case RE::COL_LAYER::kClutterLarge:
			return true;
		default:
			return false;
		}
	}

	[[nodiscard]] bool IsActorBodyLayer(RE::COL_LAYER a_layer)
	{
		switch (a_layer) {
		case RE::COL_LAYER::kBiped:
		case RE::COL_LAYER::kBipedNoCC:
		case RE::COL_LAYER::kDeadBip:
			return true;
		default:
			return false;
		}
	}

	// LOS picks register actor bodies (FPGunplayOverhaul, verified in game
	// 2026-07-27: "skeleton.nif" blocked rays).  Walk the scene-graph parents
	// for either the actor-skeleton node name or one of the given roots.
	[[nodiscard]] bool HasAncestorOrSkeleton(
		RE::NiAVObject* a_obj,
		RE::NiAVObject* a_rootA,
		RE::NiAVObject* a_rootB,
		bool&           a_matchedRoot)
	{
		a_matchedRoot = false;
		for (auto* p = a_obj; p; p = p->parent) {
			if ((a_rootA && p == a_rootA) || (a_rootB && p == a_rootB)) {
				a_matchedRoot = true;
				return true;
			}
			if (p->name.c_str() && _stricmp(p->name.c_str(), "skeleton.nif") == 0) {
				return true;
			}
		}
		return false;
	}

	// Build a rotation whose local +Z is a_normal, with a random yaw about it.
	// Column c of NiMatrix3 (entry[r].v[c]) is the image of local axis c.
	void MatrixAlignZToNormal(RE::NiMatrix3& a_mat, const RE::NiPoint3& a_normal, float a_yaw)
	{
		RE::NiPoint3 n = a_normal;
		const float  len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
		// Degenerate or downward/steep normals (a vertical ray grazing a wall
		// or railing): an upright splash reads better than a sideways one.
		if (!std::isfinite(len) || len < 1.0e-3f || (n.z / len) < 0.2f) {
			n = { 0.0f, 0.0f, 1.0f };
		} else {
			n = { n.x / len, n.y / len, n.z / len };
		}

		// Tangent: any unit vector perpendicular to n.
		RE::NiPoint3 t;
		if (std::fabs(n.z) < 0.999f) {
			// cross((0,0,1), n) normalized
			const float txy = std::sqrt(n.x * n.x + n.y * n.y);
			t = { -n.y / txy, n.x / txy, 0.0f };
		} else {
			t = { 1.0f, 0.0f, 0.0f };
		}
		// Bitangent b = n × t (right-handed, so identity falls out for n=+Z).
		RE::NiPoint3 b{
			n.y * t.z - n.z * t.y,
			n.z * t.x - n.x * t.z,
			n.x * t.y - n.y * t.x
		};

		const float c = std::cos(a_yaw);
		const float s = std::sin(a_yaw);
		const RE::NiPoint3 ty{ t.x * c + b.x * s, t.y * c + b.y * s, t.z * c + b.z * s };
		const RE::NiPoint3 by{ b.x * c - t.x * s, b.y * c - t.y * s, b.z * c - t.z * s };

		a_mat.entry[0].v = { ty.x, by.x, n.x, 0.0f };
		a_mat.entry[1].v = { ty.y, by.y, n.y, 0.0f };
		a_mat.entry[2].v = { ty.z, by.z, n.z, 0.0f };
	}

	[[nodiscard]] float RandomYaw()
	{
		return std::uniform_real_distribution<float>(
			-std::numbers::pi_v<float>, std::numbers::pi_v<float>)(g_rng);
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

	std::optional<Output> CastVerticalCell(
		RE::TESObjectCELL*  a_cell,
		const RE::NiPoint3& a_xyOrigin,
		const Options&      a_opts)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || !a_cell || a_cell != player->GetParentCell()) {
			return std::nullopt;
		}

		const float playerZ = player->data.location.z;

		float      terrainZ = 0.0f;
		const bool haveTerrain =
			TerrainHeight::TrySampleTerrainHeight(a_cell, a_xyOrigin.x, a_xyOrigin.y, terrainZ);

		const float startZ = playerZ + kCastAbove;
		const float endZ = haveTerrain
			? std::min(terrainZ, playerZ) - kCastBelow
			: playerZ - kCastBelowNoTerrain;
		const float segLen = startZ - endZ;
		if (segLen <= 1.0f) {
			return std::nullopt;
		}
		const float epsFrac = kSkipEpsilon / segLen;

		const RE::NiPoint3 rayEnd{ a_xyOrigin.x, a_xyOrigin.y, endZ };
		RE::NiAVObject*    playerRoot1st = player->Get3D(true);
		RE::NiAVObject*    playerRoot3rd = player->Get3D(false);

		float consumed = 0.0f;  // fraction of the original segment already skipped
		for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
			const RE::NiPoint3 start{ a_xyOrigin.x, a_xyOrigin.y, startZ - segLen * consumed };

			RE::bhkPickData pick{};
			pick.SetCollisionLayer(RE::COL_LAYER::kLOS);
			pick.SetStartEnd(start, rayEnd);
			RE::NiAVObject* hitObj = RE::CellPick(a_cell, pick);
			if (!pick.HasHit() && !hitObj) {
				break;  // nothing further down the ray → heightmap fallback
			}

			// Fraction sanity (FPGunplayOverhaul): ≤0 means a hit exactly at
			// the ray start — a stale/garbage result, not geometry.
			const float frac = pick.GetHitFraction();
			if (!std::isfinite(frac) || frac <= 0.001f || frac > 1.0f) {
				break;
			}

			const float         overall = consumed + (1.0f - consumed) * frac;
			const float         hitZ = startZ - segLen * overall;
			const RE::COL_LAYER layer = pick.GetResultLayer();

			// Actor hits: by body layer, or by skeleton walk (LOS picks
			// register actor bodies whose capsule layer varies).
			bool       matchedPlayerRoot = false;
			const bool actorByGraph =
				hitObj && HasAncestorOrSkeleton(hitObj, playerRoot1st, playerRoot3rd, matchedPlayerRoot);
			const bool isActorHit = IsActorBodyLayer(layer) || actorByGraph;

			if (isActorHit) {
				const bool accepted = matchedPlayerRoot ? a_opts.acceptPlayer : a_opts.acceptActors;
				if (!accepted) {
					consumed = overall + kSkipEpsilonActor / segLen;
					if (consumed >= 1.0f) {
						break;
					}
					continue;  // splash lands on whatever is beneath the actor
				}
				Output out{};
				out.hitPos = { a_xyOrigin.x, a_xyOrigin.y, hitZ };
				out.layer = layer;
				if (PointUnderCellWaterPlane(a_cell, out.hitPos)) {
					// Swimming actor: suppress like any other underwater hit.
					out.hitWater = true;
					out.hitPos.z = a_cell->waterHeight;
					MatrixAlignZToNormal(out.rotation, { 0.0f, 0.0f, 1.0f }, RandomYaw());
					return out;
				}
				out.hitActor = true;
				out.hitPlayer = matchedPlayerRoot;
				out.hitRefContributesZ = true;
				// Actor surfaces are curved and animated; an upright splash
				// with random yaw reads better than the contact normal.
				MatrixAlignZToNormal(out.rotation, { 0.0f, 0.0f, 1.0f }, RandomYaw());
				return out;
			}

			if (layer == RE::COL_LAYER::kWater) {
				Output out{};
				out.hitPos = { a_xyOrigin.x, a_xyOrigin.y, hitZ };
				out.layer = layer;
				out.hitWater = true;  // caller skips: the game does its own ripples
				MatrixAlignZToNormal(out.rotation, { 0.0f, 0.0f, 1.0f }, RandomYaw());
				return out;
			}

			if (!IsRainSolidLayer(layer)) {
				// Helper volume / foliage-class hit: re-cast past it.
				consumed = overall + epsFrac;
				if (consumed >= 1.0f) {
					break;
				}
				continue;
			}

			Output out{};
			out.hitPos = { a_xyOrigin.x, a_xyOrigin.y, hitZ };
			out.layer = layer;
			out.hitRefContributesZ =
				(layer != RE::COL_LAYER::kTerrain && layer != RE::COL_LAYER::kGround);
			if (PointUnderCellWaterPlane(a_cell, out.hitPos)) {
				out.hitWater = true;
				out.hitPos.z = a_cell->waterHeight;
			}
			MatrixAlignZToNormal(out.rotation, pick.GetResultNormal(), RandomYaw());
			return out;
		}

		// Heightmap fallback: no solid collision on the ray (streamed-out
		// physics, or everything was skipped).  Decline the sample entirely
		// when the XY has no loaded land data — a worldspace default height
		// would float or sink the splash.
		if (!haveTerrain) {
			return std::nullopt;
		}
		Output out{};
		out.hitPos = { a_xyOrigin.x, a_xyOrigin.y, terrainZ };
		out.layer = RE::COL_LAYER::kUnidentified;
		out.hitRefContributesZ = false;
		if (PointUnderCellWaterPlane(a_cell, out.hitPos)) {
			out.hitWater = true;
			out.hitPos.z = a_cell->waterHeight;
		}
		MatrixAlignZToNormal(out.rotation, { 0.0f, 0.0f, 1.0f }, RandomYaw());
		return out;
	}
}
