#pragma once

#include "Settings.h"

#include <cstdint>
#include <string>

namespace RainSplashes
{
	void OnGameDataReady();
	void OnPostLoadGame();

	// Called from RunActorUpdates hook — lightweight checks only (no Havok).
	// Raycasts + spawning are deferred to the main thread via AddTask.
	void TickOnMainThread(float a_delta);

	std::string GetIniPath();

	struct RainDebugInfo
	{
		bool   skyInstanceFound{ false };
		bool   playerInInteriorCell{ false };
		int    skyModeRaw{ -1 };
		std::string skyModeName;

		bool              hasCurrentWeather{ false };
		std::uint32_t     currentWeatherFormID{};
		std::string       currentWeatherEditorID;
		std::uint8_t      weatherTypeFlags{ 0 };  // DATA[11]: 0x01=Pleasant 0x02=Cloudy 0x04=Rainy 0x08=Snowy
		bool              currentWeatherIsRainy{ false };
		bool              currentWeatherHasPrecipitationData{ false };

		bool              hasLastWeather{ false };
		std::uint32_t     lastWeatherFormID{};
		std::string       lastWeatherEditorID;

		bool              hasOverrideWeather{ false };
		std::uint32_t     overrideWeatherFormID{};
		std::string       overrideWeatherEditorID;

		float             proxyDensity{};
		Settings::RainClass rainClass{ Settings::RainClass::kNone };
		bool              effectiveRainyAfterOverrides{ false };
		std::string       globalOverrideSummary;
		float             lastExtWetness{};
		float             windSpeed{};
		float             thresholdLight{};
		float             thresholdHeavy{};
		float             coverThreshold{};

		std::uint32_t splashActiveCount{ 0 };
		std::uint32_t splashPoolTotal{ 0 };
		bool          splashTemplateReady{ false };
		std::string   splashTemplateStatus;
	};

	// Snapshot for ImGui debug UI (reads Sky + player cell + current INI thresholds).
	[[nodiscard]] RainDebugInfo GetRainDebugInfo();
}
