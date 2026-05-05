#pragma once

#include "RE/Bethesda/BSTArray.h"
#include "RE/Bethesda/MemoryManager.h"
#include "RE/Bethesda/TESForms.h"
#include "RE/NetImmerse/NiAVObject.h"
#include "RE/NetImmerse/NiMatrix3.h"
#include "RE/NetImmerse/NiObject.h"
#include "RE/NetImmerse/NiPoint3.h"
#include "RE/NetImmerse/NiSmartPointer.h"
#include "REL/Relocation.h"

#include <cstring>

namespace RE
{
	namespace TempEffectShim
	{
		// Raw BSTempEffect fields at known offsets (relative to this pointer).
		// NiRefObject: +0x00 vtable, +0x08 refCount, +0x0C pad → 0x10
		// NiObject: no additional → 0x10
		// BSTempEffect: +0x10 lifetime, +0x18 cell, +0x20 age, +0x24 initialized, +0x28 effectID → 0x30
		// BSTempEffectDebris: +0x30 debris3D, +0x38 debrisFilename, +0x40 useDebrisCounter/forceDelete/firstPerson → 0x48
		constexpr std::size_t kDebris3D_Offset = 0x30;
		constexpr std::size_t kInitialized_Offset = 0x24;

		// Generous allocation for BSTempEffectDebris — 0x100 bytes covers any FO4 layout.
		constexpr std::size_t kDebrisAllocSize = 0x100;

		// Calls the game's BSTempEffectDebris constructor on raw memory.
		inline NiObject* CreateDebris(
			TESObjectCELL*   a_cell,
			float            a_lifetime,
			const char*      a_fileName,
			TESObjectREFR*   a_sourceRef,
			const NiPoint3&  a_position,
			const NiMatrix3& a_rotation,
			const NiPoint3&  a_startLinVel,
			const NiPoint3&  a_startAngVel,
			float            a_scale,
			bool             a_useCache,
			bool             a_addDebrisCount,
			bool             a_isFirstPerson)
		{
			void* mem = RE::malloc(kDebrisAllocSize);
			if (!mem) return nullptr;
			std::memset(mem, 0, kDebrisAllocSize);

			using ctor_t = void* (*)(
				void*,
				TESObjectCELL*, float, const char*, TESObjectREFR*,
				const NiPoint3&, const NiMatrix3&,
				const NiPoint3&, const NiPoint3&,
				float, bool, bool, bool);
			static REL::Relocation<ctor_t> ctor{ REL::ID(1075623) };

			ctor(mem,
				a_cell, a_lifetime, a_fileName, a_sourceRef,
				a_position, a_rotation,
				a_startLinVel, a_startAngVel,
				a_scale, a_useCache, a_addDebrisCount, a_isFirstPerson);

			return reinterpret_cast<NiObject*>(mem);
		}

		inline bool IsInitialized(const NiObject* a_obj)
		{
			auto* raw = reinterpret_cast<const char*>(a_obj);
			return *reinterpret_cast<const bool*>(raw + kInitialized_Offset);
		}

		inline NiAVObject* GetDebris3D(const NiObject* a_obj)
		{
			auto* raw = reinterpret_cast<const char*>(a_obj);
			auto* ptr = *reinterpret_cast<NiAVObject* const*>(raw + kDebris3D_Offset);
			return ptr;
		}
	}

	namespace ProcessListsShim
	{
		// ProcessLists layout for globalTempEffects access.
		// globalTempEffects is a BSTArray<NiPointer<BSTempEffect>> at offset 0x0F8.
		constexpr std::size_t kGlobalTempEffects_Offset = 0x0F8;

		inline void* GetSingleton()
		{
			static REL::Relocation<void**> singleton{ REL::ID(1569706) };
			return *singleton;
		}

		inline BSTArray<NiPointer<NiObject>>* GetGlobalTempEffects()
		{
			auto* pl = GetSingleton();
			if (!pl) return nullptr;
			auto* raw = reinterpret_cast<char*>(pl);
			return reinterpret_cast<BSTArray<NiPointer<NiObject>>*>(raw + kGlobalTempEffects_Offset);
		}

		inline bool PushGlobalEffect(NiPointer<NiObject> a_effect)
		{
			auto* arr = GetGlobalTempEffects();
			if (!arr) return false;
			arr->push_back(std::move(a_effect));
			return true;
		}
	}
}
