#pragma once

#include "RE/Bethesda/MemoryManager.h"
#include "RE/Havok/hknpCollisionResult.h"
#include "RE/NetImmerse/NiPoint3.h"
#include "shim/COL_LAYERS.h"

namespace RE
{
	class hknpBody;
	class NiAVObject;

	struct bhkPickData
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

		void Reset()
		{
			using func_t = decltype(&bhkPickData::Reset);
			static REL::Relocation<func_t> func{ REL::ID(438299) };
			return func(this);
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

		std::int32_t GetAllCollectorRayHitSize()
		{
			using func_t = decltype(&bhkPickData::GetAllCollectorRayHitSize);
			static REL::Relocation<func_t> func{ REL::ID(1288513) };
			return func(this);
		}

		bool GetAllCollectorRayHitAt(std::uint32_t i, hknpCollisionResult& res)
		{
			using func_t = decltype(&bhkPickData::GetAllCollectorRayHitAt);
			static REL::Relocation<func_t> func{ REL::ID(583997) };
			return func(this, i, res);
		}

		void SortAllCollectorHits()
		{
			using func_t = decltype(&bhkPickData::SortAllCollectorHits);
			static REL::Relocation<func_t> func{ REL::ID(1274842) };
			return func(this);
		}

		NiAVObject* GetNiAVObject()
		{
			using func_t = decltype(&bhkPickData::GetNiAVObject);
			static REL::Relocation<func_t> func{ REL::ID(863406) };
			return func(this);
		}

		hknpBody* GetBody()
		{
			using func_t = decltype(&bhkPickData::GetBody);
			static REL::Relocation<func_t> func{ REL::ID(1223055) };
			return func(this);
		}

		void SetCollisionLayer(COL_LAYER a_layer)
		{
			constexpr std::size_t kCFilterOffset = 0x0A;
			auto* raw = reinterpret_cast<std::uint32_t*>(
				reinterpret_cast<std::byte*>(this) + kCFilterOffset);
			*raw = static_cast<std::uint32_t>(a_layer);
		}

		std::uint32_t GetCollisionFilterRaw() const
		{
			constexpr std::size_t kCFilterOffset = 0x0A;
			return *reinterpret_cast<const std::uint32_t*>(
				reinterpret_cast<const std::byte*>(this) + kCFilterOffset);
		}

		F4_HEAP_REDEFINE_NEW(bhkPickData);

		std::uint8_t _pad[0xE0]{};
	};
	static_assert(sizeof(bhkPickData) == 0xE0);
}
