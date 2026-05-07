#include "RainSplashes.h"

#include "RayCast.h"
#include "Settings.h"

#include "F4SE/API.h"
#include "RE/Bethesda/PlayerCharacter.h"
#include "RE/Bethesda/TESForms.h"
#include "RE/NetImmerse/NiAVObject.h"
#include "RE/NetImmerse/NiNode.h"
#include "RE/NetImmerse/NiObject.h"
#include "PluginPaths.h"
#include "RelSanity.h"
#include "shim/SkyShim.h"
#include "shim/TempEffectShim.h"
#include "WeatherOverrides.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
	float SplashTimer = 0.0f;
	constexpr float kSplashInterval = 0.01f;

	constexpr std::int8_t kWeatherFlagRainy = 0x04;
	constexpr int         kWeatherDataTypeOffset = 11;

	[[nodiscard]] bool IsWeatherRainy(const RE::TESWeather* a_weather)
	{
		if (!a_weather) {
			return false;
		}
		return (a_weather->weatherData[kWeatherDataTypeOffset] & kWeatherFlagRainy) != 0;
	}

	float ComputeWetnessWindProxy(const Settings& s, RE::Sky* a_sky)
	{
		if (!a_sky || a_sky->GetMode() != RE::Sky::Mode::kFull) {
			return 0.0f;
		}
		if (!a_sky->GetCurrentWeather()) {
			return 0.0f;
		}
		return s.global.rainProxyBase +
			a_sky->GetLastExtWetness() * s.global.rainProxyWetnessScale +
			a_sky->GetWindSpeed() * s.global.rainProxyWindScale;
	}

	// ── BSTempEffectDebris approach — uses the game's managed effect system ──

	RE::TESObjectCELL* g_trackedCell{ nullptr };
	std::string        g_lastModelStatus{ "not loaded" };
	std::uint32_t      g_spawnCount{ 0 };

	void OnPlayerCellChanged()
	{
		g_spawnCount = 0;
	}

	// Spawns a visible temporary effect via BSTempEffectDebris + ProcessLists.
	// The game's own Update loop handles model loading, rendering, and cleanup.
	// NIF path should NOT include "Meshes\" prefix (the engine prepends it).
	[[nodiscard]] bool SpawnTempEffect(
		RE::TESObjectCELL*   a_cell,
		const RE::NiPoint3&  a_hitPos,
		const RE::NiMatrix3& a_rotation,
		float                a_scale,
		const char*          a_nifPath,
		float                a_lifetime)
	{
		auto* pl = RE::ProcessListsShim::GetSingleton();
		if (!pl) {
			static bool logged = false;
			if (!logged) {
				logger::warn("RainSplashesF4SE: ProcessLists singleton is null");
				logged = true;
			}
			return false;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		RE::NiPoint3 zeroVel{ 0.0f, 0.0f, 0.0f };

		auto* effect = RE::TempEffectShim::CreateDebris(
			a_cell,
			a_lifetime,
			a_nifPath,
			player,
			a_hitPos,
			a_rotation,
			zeroVel,
			zeroVel,
			a_scale,
			false,
			false,
			false);

		if (!effect) {
			static bool logged = false;
			if (!logged) {
				logger::warn("RainSplashesF4SE: CreateDebris returned null (allocation failure)");
				logged = true;
			}
			return false;
		}

		bool init = RE::TempEffectShim::IsInitialized(effect);
		auto* d3d = RE::TempEffectShim::GetDebris3D(effect);

		RE::NiPointer<RE::NiObject> ptr(effect);
		bool pushed = RE::ProcessListsShim::PushGlobalEffect(std::move(ptr));

		static std::uint32_t g_spawnLog = 0;
		if (g_spawnLog < 20) {
			auto* arr = RE::ProcessListsShim::GetGlobalTempEffects();
			logger::info(
				"RainSplashesF4SE: TempEffectDebris #{} at ({:.0f},{:.0f},{:.0f}) "
				"scale={:.2f} model='{}' lifetime={:.1f} debris3D={} init={} pushed={} plSize={}",
				g_spawnLog,
				a_hitPos.x, a_hitPos.y, a_hitPos.z,
				a_scale, a_nifPath, a_lifetime,
				d3d ? "yes" : "null",
				init,
				pushed,
				arr ? arr->size() : 0u);
			++g_spawnLog;
		}

		++g_spawnCount;
		g_lastModelStatus = std::string("OK(TempEffect): ") + a_nifPath;
		return true;
	}
}

std::string RainSplashes::GetIniPath()
{
	const auto dir = PluginPaths::GetDllContainingDirectory();
	if (!dir.empty()) {
		return (dir / L"RainSplashesF4SE.ini").string();
	}
	return std::filesystem::path{ L"Data/F4SE/Plugins/RainSplashesF4SE.ini" }.string();
}

void RainSplashes::OnGameDataReady()
{
	auto& s = Settings::Get();
	const auto path = GetIniPath();
	if (!s.LoadFromFile(path)) {
		logger::warn("RainSplashesF4SE: could not load '{}', using defaults", path);
	}
	WeatherOverrides::OnGameDataReady();
}

void RainSplashes::OnPostLoadGame()
{
	OnGameDataReady();
	g_trackedCell = nullptr;
	g_spawnCount = 0;
}

namespace
{
	const char* RainClassName(Settings::RainClass a_class)
	{
		switch (a_class) {
		case Settings::RainClass::kLight:
			return "Light";
		case Settings::RainClass::kMedium:
			return "Medium";
		case Settings::RainClass::kHeavy:
			return "Heavy";
		default:
			return "None";
		}
	}

	const char* SkyModeString(RE::Sky::Mode a_mode)
	{
		switch (a_mode) {
		case RE::Sky::Mode::kNone:
			return "None";
		case RE::Sky::Mode::kInterior:
			return "Interior";
		case RE::Sky::Mode::kSkyDomeOnly:
			return "Sky dome only";
		case RE::Sky::Mode::kFull:
			return "Full (exterior sky)";
		case RE::Sky::Mode::kTotal:
			return "Total";
		default:
			return "Unknown";
		}
	}

	void FillWeatherForm(const RE::TESWeather* a_w, bool& a_has, std::uint32_t& a_formID, std::string& a_editor)
	{
		a_has = false;
		a_formID = 0;
		a_editor.clear();
		if (!a_w) {
			return;
		}
		a_has = true;
		a_formID = const_cast<RE::TESWeather*>(a_w)->GetFormID();
		const char* ed = const_cast<RE::TESWeather*>(a_w)->GetFormEditorID();
		a_editor = ed ? ed : "";
	}
}

RainSplashes::RainDebugInfo RainSplashes::GetRainDebugInfo()
{
	RainDebugInfo out;
	auto&       s = Settings::Get();
	out.thresholdLight = s.global.rainDensityLightThreshold;
	out.thresholdHeavy = s.global.rainDensityHeavyThreshold;
	out.coverThreshold = s.global.coverThreshold;
	out.splashActiveCount = g_spawnCount;
	out.splashPoolTotal = 0;
	out.splashTemplateReady = (g_spawnCount > 0);
	out.splashTemplateStatus = g_lastModelStatus;

	if (!RelSanity::Ok()) {
		out.globalOverrideSummary =
			"Address Library preflight failed — see RainSplashesF4SE.log; sky queries skipped.";
		return out;
	}

	if (const auto* player = RE::PlayerCharacter::GetSingleton()) {
		if (const auto* cell = player->GetParentCell()) {
			out.playerInInteriorCell = cell->IsInterior();
		}
	}

	auto* sky = RE::Sky::GetSingleton();
	const auto snap = WeatherOverrides::GetSnapshot();

	auto formatGlobal = [](const WeatherOverrides::Snapshot& snap2) -> std::string {
		const char* gr = snap2.globalActive.rain == WeatherOverrides::RainTriState::kForceRainy
			? "force rainy"
			: (snap2.globalActive.rain == WeatherOverrides::RainTriState::kForceClear ? "force clear" : "inherit");
		const char* gi = snap2.globalActive.intensity == WeatherOverrides::IntensityTriState::kLight
			? "light"
			: (snap2.globalActive.intensity == WeatherOverrides::IntensityTriState::kMedium
					? "medium"
					: (snap2.globalActive.intensity == WeatherOverrides::IntensityTriState::kHeavy ? "heavy" : "inherit"));
		return std::string("Global(active weather): rain=") + gr + ", intensity=" + gi;
	};

	if (!sky) {
		out.globalOverrideSummary = formatGlobal(snap);
		out.proxyDensity = 0.0f;
		out.effectiveRainyAfterOverrides = false;
		out.rainClass = Settings::RainClass::kNone;
		return out;
	}

	out.skyInstanceFound = true;
	const auto mode = sky->GetMode();
	out.skyModeRaw = static_cast<int>(mode);
	out.skyModeName = SkyModeString(mode);

	const auto* cw = sky->GetCurrentWeather();
	FillWeatherForm(cw, out.hasCurrentWeather, out.currentWeatherFormID, out.currentWeatherEditorID);
	if (cw) {
		out.weatherTypeFlags = static_cast<std::uint8_t>(
			static_cast<unsigned char>(cw->weatherData[kWeatherDataTypeOffset]));
		out.currentWeatherIsRainy = IsWeatherRainy(cw);
		out.currentWeatherHasPrecipitationData = (cw->precipitationData != nullptr);
	}

	FillWeatherForm(sky->GetLastWeather(), out.hasLastWeather, out.lastWeatherFormID, out.lastWeatherEditorID);
	FillWeatherForm(sky->GetOverrideWeather(), out.hasOverrideWeather, out.overrideWeatherFormID, out.overrideWeatherEditorID);

	out.lastExtWetness = sky->GetLastExtWetness();
	out.windSpeed = sky->GetWindSpeed();
	out.proxyDensity = ComputeWetnessWindProxy(s, sky);
	out.effectiveRainyAfterOverrides = WeatherOverrides::IsEffectiveRainy(cw, snap);
	if (out.effectiveRainyAfterOverrides) {
		out.rainClass = WeatherOverrides::EffectiveRainClass(cw, out.proxyDensity, s, snap);
	} else {
		out.rainClass = Settings::RainClass::kNone;
	}

	out.globalOverrideSummary = formatGlobal(snap);
	return out;
}

void RainSplashes::TickOnMainThread(float a_delta)
{
	if (!RelSanity::Ok()) {
		return;
	}

	auto& s = Settings::Get();
	if (!s.global.masterEnabled) {
		return;
	}

	auto* player = RE::PlayerCharacter::GetSingleton();
	if (!player) {
		return;
	}
	auto* cell = player->GetParentCell();
	if (!cell || cell->IsInterior()) {
		if (g_trackedCell) {
			OnPlayerCellChanged();
			g_trackedCell = nullptr;
		}
		return;
	}

	if (cell != g_trackedCell) {
		OnPlayerCellChanged();
		g_trackedCell = cell;
	}

	auto* sky = RE::Sky::GetSingleton();
	if (!sky || sky->GetMode() != RE::Sky::Mode::kFull) {
		return;
	}

	const auto*               weather = sky->GetCurrentWeather();
	const auto                snap = WeatherOverrides::GetSnapshot();
	const float               density = ComputeWetnessWindProxy(s, sky);
	const bool                rainy = WeatherOverrides::IsEffectiveRainy(weather, snap);
	const Settings::RainClass rainClass =
		rainy ? WeatherOverrides::EffectiveRainClass(weather, density, s, snap) : Settings::RainClass::kNone;

	{
		std::scoped_lock lk(s.diagMutex);
		s.lastRainDensityProxy = rainy ? density : 0.0f;
		s.lastRainClass = rainClass;
	}

	if (rainClass == Settings::RainClass::kNone) {
		return;
	}

	const auto* tier = s.SelectTier(rainClass);
	if (!tier || !tier->enabled) {
		return;
	}

	const RE::NiPoint3 playerPos{
		player->data.location.x,
		player->data.location.y,
		player->data.location.z
	};

	if (!tier->splashEnabled) {
		return;
	}

	float dt = a_delta > 0.0f ? a_delta : (1.0f / 60.0f);
	SplashTimer += dt;
	if (SplashTimer <= kSplashInterval) {
		return;
	}
	SplashTimer = 0.0f;

	const auto    iterations = tier->rayCastIterations;
	const float   radius = tier->rayCastRadius;
	const bool    debugMarker = s.global.debugSplashes;
	const float   coverThresh = s.global.coverThreshold;
	const float   life = std::max(0.05f, tier->splashLifetime);

	static bool g_loggedSplashPath = false;
	if (!g_loggedSplashPath) {
		logger::info("RainSplashesF4SE: spawn path (TempEffectDebris) — tier={} iter={} radius={:.0f} debug={}",
			RainClassName(rainClass), iterations, radius, debugMarker);
		g_loggedSplashPath = true;
	}

	// CastVerticalCell no longer uses Havok/CellPick — it scans loaded
	// references for surface height.  Safe to call on any thread.
	// Only SpawnTempEffect is deferred to the main thread (model loading).
	for (std::uint32_t i = 0; i < iterations; ++i) {
		const auto origin = RayCast::RandomPointInDiskAround(radius, playerPos, true);
		if (!origin) {
			continue;
		}

		auto out = RayCast::CastVerticalCell(cell, *origin);
		if (!out) {
			continue;
		}
		if (out->hitWater) {
			continue;
		}
		if (out->hitPlayer && !s.global.spawnOnPlayer) {
			continue;
		}
		if (out->hitActor && !out->hitPlayer && !s.global.spawnOnActors) {
			continue;
		}

		static bool g_loggedFirstHit = false;
		if (!g_loggedFirstHit) {
			logger::info(
				"RainSplashesF4SE: first hit — pos=({:.0f},{:.0f},{:.0f}) player=({:.0f},{:.0f},{:.0f})",
				out->hitPos.x, out->hitPos.y, out->hitPos.z,
				playerPos.x, playerPos.y, playerPos.z);
			g_loggedFirstHit = true;
		}

		const float dzCover = std::fabs(out->hitPos.z - playerPos.z);
		if (dzCover > coverThresh) {
			static std::uint32_t g_coverDrops = 0;
			if (g_coverDrops < 10) {
				logger::info("RainSplashesF4SE: cover-filtered dz={:.0f} thresh={:.0f} hitZ={:.0f} playerZ={:.0f}",
					dzCover, coverThresh, out->hitPos.z, playerPos.z);
				++g_coverDrops;
			}
			continue;
		}

		std::string nifPathStr;
		float       scale;
		if (debugMarker) {
			nifPathStr = "MarkerX.nif";
			scale = 1.0f;
		} else {
			static auto stripMeshesPrefix = [](const std::string& p) -> std::string {
				if (p.size() > 7) {
					auto prefix = p.substr(0, 7);
					for (auto& c : prefix) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
					if (prefix == "meshes\\" || prefix == "meshes/") {
						return p.substr(7);
					}
				}
				return p;
			};
			nifPathStr = stripMeshesPrefix(out->hitActor ? tier->splashNifActor : tier->splashNif);
			scale = out->hitActor ? tier->splashNifScaleActor : tier->splashNifScale;
		}

		RE::NiPoint3  capturedPos    = out->hitPos;
		RE::NiMatrix3 capturedNormal = out->normal;
		float         capturedScale  = scale;
		float         capturedLife   = life;

		auto* taskIface = F4SE::GetTaskInterface();
		if (taskIface) {
			taskIface->AddTask([=]() {
				auto* player2 = RE::PlayerCharacter::GetSingleton();
				if (!player2) return;
				auto* cell2 = player2->GetParentCell();
				if (!cell2 || cell2->IsInterior()) return;
				(void)SpawnTempEffect(cell2, capturedPos, capturedNormal, capturedScale, nifPathStr.c_str(), capturedLife);
			});
		}
	}
}
