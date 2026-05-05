#pragma once

#include "RE/Bethesda/TESForms.h"

namespace RE
{
	class Sky
	{
	public:
		enum class Mode : std::uint32_t
		{
			kNone = 0,
			kInterior,
			kSkyDomeOnly,
			kFull,
			kTotal
		};

		[[nodiscard]] static Sky* GetSingleton()
		{
			using func_t = Sky* (*)();
			static REL::Relocation<func_t> func{ REL::ID(484694) };
			return func();
		}

		[[nodiscard]] Mode GetMode() const
		{
			return static_cast<Mode>(*reinterpret_cast<const std::uint32_t*>(reinterpret_cast<std::uintptr_t>(this) + 0x36C));
		}

		[[nodiscard]] TESWeather* GetCurrentWeather() const
		{
			return *reinterpret_cast<TESWeather* const*>(reinterpret_cast<std::uintptr_t>(this) + 0x48);
		}

		[[nodiscard]] TESWeather* GetLastWeather() const
		{
			return *reinterpret_cast<TESWeather* const*>(reinterpret_cast<std::uintptr_t>(this) + 0x50);
		}

		[[nodiscard]] TESWeather* GetDefaultWeather() const
		{
			return *reinterpret_cast<TESWeather* const*>(reinterpret_cast<std::uintptr_t>(this) + 0x58);
		}

		[[nodiscard]] TESWeather* GetOverrideWeather() const
		{
			return *reinterpret_cast<TESWeather* const*>(reinterpret_cast<std::uintptr_t>(this) + 0x60);
		}

		[[nodiscard]] float GetLastExtWetness() const
		{
			return *reinterpret_cast<const float*>(reinterpret_cast<std::uintptr_t>(this) + 0x300);
		}

		[[nodiscard]] float GetWindSpeed() const
		{
			return *reinterpret_cast<const float*>(reinterpret_cast<std::uintptr_t>(this) + 0x31C);
		}

	private:
		Sky() = delete;
	};
}
