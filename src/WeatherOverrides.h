#pragma once

#include "Settings.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace RE
{
	class TESWeather;
}

namespace WeatherOverrides
{
	enum class RainTriState : std::uint8_t
	{
		kInherit,
		kForceRainy,
		kForceClear
	};

	enum class IntensityTriState : std::uint8_t
	{
		kInherit,
		kLight,
		kMedium,
		kHeavy
	};

	struct PerFormEntry
	{
		RainTriState      rain{ RainTriState::kInherit };
		IntensityTriState intensity{ IntensityTriState::kInherit };
	};

	// Applies to whichever weather is currently active on the sky (each frame).
	// Per-form overrides take precedence; this is a fallback before vanilla DATA[11].
	struct GlobalActiveOverride
	{
		RainTriState      rain{ RainTriState::kInherit };
		IntensityTriState intensity{ IntensityTriState::kInherit };
	};

	void OnGameDataReady();

	[[nodiscard]] std::filesystem::path GetJsonPath();

	bool LoadFromFile(const std::filesystem::path& a_path);
	bool SaveToFile(const std::filesystem::path& a_path);

	inline bool LoadFromDefaultPath()
	{
		return LoadFromFile(GetJsonPath());
	}

	inline bool SaveToDefaultPath()
	{
		return SaveToFile(GetJsonPath());
	}

	// Thread-safe snapshot for game tick (avoid holding mutex across engine calls).
	struct Snapshot
	{
		GlobalActiveOverride              globalActive{};
		std::map<std::uint32_t, PerFormEntry> perForm;
	};
	[[nodiscard]] Snapshot GetSnapshot();

	void SetGlobalActive(const GlobalActiveOverride& a_v);
	void SetPerForm(std::uint32_t a_formID, PerFormEntry a_entry);
	void ClearPerForm(std::uint32_t a_formID);

	[[nodiscard]] bool IsVanillaDataRainy(const RE::TESWeather* a_weather);

	[[nodiscard]] bool IsEffectiveRainy(const RE::TESWeather* a_weather, const Snapshot& snap);

	[[nodiscard]] Settings::RainClass EffectiveRainClass(
		const RE::TESWeather* a_weather,
		float                 a_proxyDensity,
		const Settings&       a_settings,
		const Snapshot&       snap);

	struct CatalogRow
	{
		std::uint32_t formID{};
		std::string   editorID;
		std::string   pluginName;
		bool          vanillaRainy{ false };
	};

	[[nodiscard]] std::vector<CatalogRow> BuildWeatherCatalog();
}
