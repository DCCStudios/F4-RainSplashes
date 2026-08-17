#pragma once

#include "RE/Bethesda/MemoryManager.h"
#include "RE/NetImmerse/NiPoint3.h"
#include "shim/COL_LAYERS.h"

namespace RE
{
	class NiAVObject;
	class TESObjectCELL;

	// Minimal shim over the engine's bhkPickData (0xE0 bytes).  Field offsets
	// mirror CommonLibF4-NG's declared layout (see FPGunplayOverhaul's
	// lib/commonlibf4, verified in game there 2026-07):
	//   +0x00  hknpRayCastQuery castQuery
	//          +0x08 hknpQueryFilterData m_filterData (CFilter word at +0x04)
	//   +0x60  hknpRayCastQueryResult result (: hknpCollisionResult)
	//          +0x00 hkVector4f position   (Havok-scaled — do not use raw)
	//          +0x10 hkVector4f normal     (unit direction, scale-free)
	//          +0x20 fraction              (unitless, scale-free)
	//          +0x40 BodyInfo hitBodyInfo  (+0x0C CFilter shapeCollisionFilterInfo)
	//   +0xD0  collector / +0xD8 collectorType — the ctor never configures a
	//          collector, so collector enumeration is empty on EVERY runtime;
	//          the embedded `result` is the only hit data a pick produces.
	// alignas(16): the engine writes hkVector4 members with aligned SIMD
	// stores; a byte-aligned pad blob on the stack is not guaranteed 16-byte
	// alignment.
	struct alignas(16) bhkPickData
	{
	public:
		bhkPickData()
		{
			typedef bhkPickData* func_t(bhkPickData*);
			static REL::Relocation<func_t> func{ REL::ID(526783) };
			func(this);
		}

		void SetStartEnd(const NiPoint3& start, const NiPoint3& end)
		{
			using func_t = decltype(&bhkPickData::SetStartEnd);
			static REL::Relocation<func_t> func{ REL::ID(747470) };
			return func(this, start, end);
		}

		bool HasHit()
		{
			using func_t = decltype(&bhkPickData::HasHit);
			static REL::Relocation<func_t> func{ REL::ID(1181584) };
			return func(this);
		}

		float GetHitFraction()
		{
			using func_t = decltype(&bhkPickData::GetHitFraction);
			static REL::Relocation<func_t> func{ REL::ID(476687) };
			return func(this);
		}

		// Writes the query's collision-layer filter word:
		// castQuery.m_filterData.m_collisionFilterInfo at +0x0C.  An older
		// revision wrote +0x0A, which straddled hknpMaterialId padding and
		// the real CFilter word (bug catalogued in FPGunplayOverhaul).
		void SetCollisionLayer(COL_LAYER a_layer)
		{
			constexpr std::size_t kFilterOffset = 0x0C;
			*reinterpret_cast<std::uint32_t*>(
				reinterpret_cast<std::byte*>(this) + kFilterOffset) =
				static_cast<std::uint32_t>(a_layer);
		}

		// Surface normal of the embedded (closest) hit.  A direction, so it
		// carries no Havok world scale; the hit POSITION at +0x60 does and is
		// deliberately not exposed — derive world positions from the fraction.
		[[nodiscard]] NiPoint3 GetResultNormal() const
		{
			constexpr std::size_t kNormalOffset = 0x70;
			const float* v = reinterpret_cast<const float*>(
				reinterpret_cast<const std::byte*>(this) + kNormalOffset);
			return NiPoint3{ v[0], v[1], v[2] };
		}

		// Collision layer of the embedded hit's body
		// (result.hitBodyInfo.shapeCollisionFilterInfo, low 7 bits per CFilter).
		[[nodiscard]] COL_LAYER GetResultLayer() const
		{
			constexpr std::size_t kHitFilterOffset = 0xAC;
			const auto raw = *reinterpret_cast<const std::uint32_t*>(
				reinterpret_cast<const std::byte*>(this) + kHitFilterOffset);
			return static_cast<COL_LAYER>(raw & 0x7F);
		}

		// Aligned variant: plain RE::malloc gives no 16-byte guarantee, and a
		// heap-allocated pick would hand the engine's SIMD stores an 8-aligned
		// block.  Current usage is stack-only; this keeps `new` safe anyway.
		F4_HEAP_REDEFINE_ALIGNED_NEW(bhkPickData);

		std::uint8_t _pad[0xE0]{};
	};
	static_assert(sizeof(bhkPickData) == 0xE0);

	// TESObjectCELL::Pick — runs the query against the loaded physics world
	// (not just this cell's refs) and returns the scene-graph object of the
	// closest hit.  Main thread only.
	[[nodiscard]] inline NiAVObject* CellPick(TESObjectCELL* a_cell, bhkPickData& a_pick)
	{
		using func_t = NiAVObject* (*)(TESObjectCELL*, bhkPickData&);
		static REL::Relocation<func_t> func{ REL::ID(434717) };
		return func(a_cell, a_pick);
	}
}
