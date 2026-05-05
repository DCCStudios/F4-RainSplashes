#include "PCH.h"

#include "UI.h"

#include "F4SEMenuFramework.h"
#include "RainSplashes.h"
#include "RelSanity.h"
#include "Settings.h"
#include "WeatherOverrides.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <vector>

namespace
{
	const char* RainClassName(Settings::RainClass a_class)
	{
		switch (a_class) {
		case Settings::RainClass::kNone:   return "None";
		case Settings::RainClass::kLight:  return "Light";
		case Settings::RainClass::kMedium: return "Medium";
		case Settings::RainClass::kHeavy:  return "Heavy";
		default:                           return "?";
		}
	}

	constexpr std::size_t kPathBufSize = 512;

	struct TierBufs
	{
		char nifWorld[kPathBufSize]{};
		char nifActor[kPathBufSize]{};
	};

	static TierBufs g_tierBufs[3]{};

	void PathInput(const char* a_label, std::string& a_path, char* a_buf, std::size_t a_sz)
	{
		const ImGuiMCP::ImGuiID wid = ImGuiMCP::GetID(a_label);
		if (ImGuiMCP::GetActiveID() != wid) {
			(void)strncpy_s(a_buf, a_sz, a_path.c_str(), _TRUNCATE);
		}
		if (ImGuiMCP::InputText(a_label, a_buf, a_sz)) {
			a_path.assign(a_buf);
		}
	}

	// ── Settings page ────────────────────────────────────────────────────

	void DrawStatusBanner()
	{
		if (!RelSanity::Ok()) {
			ImGuiMCP::TextColored(
				ImGuiMCP::ImVec4{ 1.0f, 0.35f, 0.3f, 1.0f },
				"[!] Splashes disabled: Address Library mismatch");
			ImGuiMCP::TextWrapped(
				"Install/update Address Library for Fallout 4 so the version-*.bin matches "
				"your game exe. Details in RainSplashesF4SE.log.");
			ImGuiMCP::Separator();
			return;
		}

		auto& s = Settings::Get();
		float proxy = 0.0f;
		Settings::RainClass cls = Settings::RainClass::kNone;
		{
			std::scoped_lock lk(s.diagMutex);
			proxy = s.lastRainDensityProxy;
			cls   = s.lastRainClass;
		}

		if (cls != Settings::RainClass::kNone) {
			ImGuiMCP::TextColored(
				ImGuiMCP::ImVec4{ 0.3f, 1.0f, 0.5f, 1.0f },
				"Active: %s rain  (intensity %.1f)", RainClassName(cls), proxy);
		} else {
			ImGuiMCP::TextColored(
				ImGuiMCP::ImVec4{ 0.6f, 0.6f, 0.6f, 1.0f },
				"No rain detected right now");
		}
	}

	void DrawMainSettings()
	{
		DrawStatusBanner();

		auto& s = Settings::Get();

		ImGuiMCP::Separator();

		ImGuiMCP::Checkbox("Enable splashes", &s.global.masterEnabled);
		ImGuiMCP::SameLine();
		ImGuiMCP::Checkbox("Show debug markers", &s.global.debugSplashes);
		if (ImGuiMCP::IsItemHovered()) {
			ImGuiMCP::SetTooltip("Scales splashes up 3x and keeps the same mesh so hits are easier to see.");
		}

		ImGuiMCP::SliderFloat("Cover threshold (Z)", &s.global.coverThreshold, 32.0f, 800.0f);
		if (ImGuiMCP::IsItemHovered()) {
			ImGuiMCP::SetTooltip(
				"Skip splash if ground hit is this far above/below the player (units). "
				"Higher = stricter: fewer splashes under bridges / overhangs.");
		}

		ImGuiMCP::Separator();

		if (ImGuiMCP::Button("Save settings to INI")) {
			(void)s.SaveToFile(RainSplashes::GetIniPath());
		}
		ImGuiMCP::SameLine();
		if (ImGuiMCP::Button("Reload INI")) {
			(void)s.LoadFromFile(RainSplashes::GetIniPath());
		}
	}

	void DrawTier(const char* a_title, RainTierSettings& t, int a_idx)
	{
		if (!ImGuiMCP::TreeNodeEx(a_title, ImGuiMCP::ImGuiTreeNodeFlags_DefaultOpen)) {
			return;
		}
		ImGuiMCP::PushID(a_title);

		ImGuiMCP::Checkbox("Enabled", &t.enabled);

		ImGuiMCP::TextWrapped("Splash effect");
		ImGuiMCP::Checkbox("Spawn splash particles", &t.splashEnabled);

		int it = static_cast<int>(t.rayCastIterations);
		if (ImGuiMCP::SliderInt("Rays per tick", &it, 0, 40)) {
			t.rayCastIterations = static_cast<std::uint32_t>(std::clamp(it, 0, 40));
		}
		if (ImGuiMCP::IsItemHovered()) {
			ImGuiMCP::SetTooltip("How many raycasts each frame to find ground/object hit points. More = denser splashes, higher cost.");
		}
		ImGuiMCP::SliderFloat("Radius around player", &t.rayCastRadius, 64.0f, 4096.0f);
		if (ImGuiMCP::IsItemHovered()) {
			ImGuiMCP::SetTooltip("How far from the player splashes can spawn (game units). 1024 ~ half a cell width.");
		}
		ImGuiMCP::SliderFloat("Scale (ground)", &t.splashNifScale, 0.05f, 3.0f);
		ImGuiMCP::SliderFloat("Scale (on actors)", &t.splashNifScaleActor, 0.05f, 3.0f);
		ImGuiMCP::SliderFloat("Splash lifetime (sec)", &t.splashLifetime, 0.1f, 5.0f);
		if (ImGuiMCP::IsItemHovered()) {
			ImGuiMCP::SetTooltip("How long each splash instance stays attached before returning to the pool.");
		}

		if (ImGuiMCP::TreeNode("Mesh paths (advanced)")) {
			auto& nb = g_tierBufs[static_cast<std::size_t>(a_idx)];
			PathInput("Ground/terrain NIF##w", t.splashNif, nb.nifWorld, kPathBufSize);
			PathInput("Actor hit NIF##a", t.splashNifActor, nb.nifActor, kPathBufSize);
			ImGuiMCP::TreePop();
		}

		ImGuiMCP::PopID();
		ImGuiMCP::TreePop();
	}

	void __stdcall DrawSettingsPage()
	{
		DrawMainSettings();

		auto& s = Settings::Get();

		ImGuiMCP::Separator();
		ImGuiMCP::TextWrapped("Each rain tier has its own splash settings. The active tier depends on "
			"rain intensity (wetness + wind proxy vs. thresholds below).");

		if (ImGuiMCP::TreeNode("Intensity thresholds (advanced)")) {
			ImGuiMCP::SliderFloat("Light ceiling", &s.global.rainDensityLightThreshold, 0.5f, 20.0f);
			if (ImGuiMCP::IsItemHovered()) {
				ImGuiMCP::SetTooltip("Proxy values below this are Light rain. Above this is Medium.");
			}
			ImGuiMCP::SliderFloat("Heavy floor", &s.global.rainDensityHeavyThreshold, 0.5f, 40.0f);
			if (ImGuiMCP::IsItemHovered()) {
				ImGuiMCP::SetTooltip("Proxy values at or above this are Heavy rain.");
			}
			ImGuiMCP::SliderFloat("Base intensity", &s.global.rainProxyBase, 0.0f, 20.0f);
			ImGuiMCP::SliderFloat("Wetness weight", &s.global.rainProxyWetnessScale, 0.0f, 80.0f);
			ImGuiMCP::SliderFloat("Wind weight", &s.global.rainProxyWindScale, 0.0f, 20.0f);
			ImGuiMCP::TreePop();
		}

		ImGuiMCP::Separator();
		DrawTier("Light rain", s.light, 0);
		DrawTier("Medium rain", s.medium, 1);
		DrawTier("Heavy rain", s.heavy, 2);
	}

	// ── Weather overrides page ───────────────────────────────────────────

	static int RainComboIdx(WeatherOverrides::RainTriState r)
	{
		switch (r) {
		case WeatherOverrides::RainTriState::kForceRainy: return 1;
		case WeatherOverrides::RainTriState::kForceClear: return 2;
		default: return 0;
		}
	}

	static WeatherOverrides::RainTriState IdxToRain(int i)
	{
		switch (i) {
		case 1:  return WeatherOverrides::RainTriState::kForceRainy;
		case 2:  return WeatherOverrides::RainTriState::kForceClear;
		default: return WeatherOverrides::RainTriState::kInherit;
		}
	}

	static int IntComboIdx(WeatherOverrides::IntensityTriState i)
	{
		switch (i) {
		case WeatherOverrides::IntensityTriState::kLight:  return 1;
		case WeatherOverrides::IntensityTriState::kMedium: return 2;
		case WeatherOverrides::IntensityTriState::kHeavy:  return 3;
		default: return 0;
		}
	}

	static WeatherOverrides::IntensityTriState IdxToInt(int i)
	{
		switch (i) {
		case 1:  return WeatherOverrides::IntensityTriState::kLight;
		case 2:  return WeatherOverrides::IntensityTriState::kMedium;
		case 3:  return WeatherOverrides::IntensityTriState::kHeavy;
		default: return WeatherOverrides::IntensityTriState::kInherit;
		}
	}

	static bool RowPassesFilter(const WeatherOverrides::CatalogRow& row, std::string_view filt)
	{
		if (filt.empty()) return true;
		auto lower = [](std::string s) { for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; };
		char hex[16];
		std::snprintf(hex, sizeof(hex), "%08x", static_cast<unsigned>(row.formID));
		return lower(row.editorID).find(filt) != std::string::npos ||
			lower(row.pluginName).find(filt) != std::string::npos ||
			std::string(hex).find(filt) != std::string::npos;
	}

	static std::vector<WeatherOverrides::CatalogRow> g_catalog;
	static char                                     g_filter[128]{};

	void __stdcall DrawWeatherOverridesPage()
	{
		ImGuiMCP::TextWrapped(
			"Override which weathers count as rainy (for splashes) and at what intensity. "
			"Changes here are saved to a JSON file alongside the plugin DLL.");

		if (ImGuiMCP::Button("Save overrides")) {
			(void)WeatherOverrides::SaveToDefaultPath();
		}
		ImGuiMCP::SameLine();
		if (ImGuiMCP::Button("Reload overrides")) {
			(void)WeatherOverrides::LoadFromDefaultPath();
		}
		ImGuiMCP::SameLine();
		if (ImGuiMCP::Button("Scan installed weathers")) {
			g_catalog = WeatherOverrides::BuildWeatherCatalog();
		}
		if (ImGuiMCP::IsItemHovered()) {
			ImGuiMCP::SetTooltip("Reads every WTHR record from your load order. Run once after loading a save.");
		}

		// ── Global override ──
		ImGuiMCP::Separator();
		ImGuiMCP::TextWrapped("Global override (applies to whatever weather is active right now):");

		auto snap = WeatherOverrides::GetSnapshot();
		int  gr = RainComboIdx(snap.globalActive.rain);
		int  gi = IntComboIdx(snap.globalActive.intensity);

		const char* kRainOpts =
			"Auto (use vanilla flag + per-weather overrides)\0"
			"Force rainy (always spawn splashes)\0"
			"Force clear (never spawn splashes)\0\0";
		const char* kIntOpts =
			"Auto (use wetness/wind proxy)\0"
			"Light\0Medium\0Heavy\0\0";

		if (ImGuiMCP::Combo("Rain##global", &gr, kRainOpts, 3)) {
			auto g = snap.globalActive;
			g.rain = IdxToRain(gr);
			WeatherOverrides::SetGlobalActive(g);
		}
		if (ImGuiMCP::Combo("Intensity##global", &gi, kIntOpts, 4)) {
			auto g = snap.globalActive;
			g.intensity = IdxToInt(gi);
			WeatherOverrides::SetGlobalActive(g);
		}

		// ── Per-weather list ──
		ImGuiMCP::Separator();
		ImGuiMCP::TextWrapped("Per-weather overrides (from installed plugins):");
		ImGuiMCP::InputText("Search", g_filter, sizeof(g_filter));

		std::string filterLow = g_filter;
		for (auto& c : filterLow) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		if (g_catalog.empty()) {
			ImGuiMCP::TextWrapped(
				"No weathers scanned yet. Load a save, then click \"Scan installed weathers\" above.");
			return;
		}

		ImGuiMCP::BeginChild("wlist", ImGuiMCP::ImVec2{ 0.0f, 400.0f }, 0);
		for (const auto& row : g_catalog) {
			if (!RowPassesFilter(row, filterLow)) continue;

			snap = WeatherOverrides::GetSnapshot();
			ImGuiMCP::PushID(static_cast<int>(row.formID));

			// Header line: EditorID [plugin] FormID [rainy/clear]
			const char* edid = row.editorID.empty() ? "(unnamed)" : row.editorID.c_str();

			if (row.vanillaRainy) {
				ImGuiMCP::TextColored(ImGuiMCP::ImVec4{ 0.4f, 0.8f, 1.0f, 1.0f }, "%s", edid);
			} else {
				ImGuiMCP::Text("%s", edid);
			}
			ImGuiMCP::SameLine();
			ImGuiMCP::TextDisabled("[%s]  %08X  %s",
				row.pluginName.c_str(),
				static_cast<unsigned>(row.formID),
				row.vanillaRainy ? "rainy" : "clear");

			// Override combos
			WeatherOverrides::PerFormEntry e{};
			if (auto it = snap.perForm.find(row.formID); it != snap.perForm.end()) {
				e = it->second;
			}

			ImGuiMCP::PushItemWidth(220.0f);
			int lr = RainComboIdx(e.rain);
			ImGuiMCP::PushID(1);
			if (ImGuiMCP::Combo("##r", &lr, kRainOpts, 3)) {
				e.rain = IdxToRain(lr);
				WeatherOverrides::SetPerForm(row.formID, e);
			}
			ImGuiMCP::PopID();
			ImGuiMCP::SameLine();
			int li = IntComboIdx(e.intensity);
			ImGuiMCP::PushID(2);
			if (ImGuiMCP::Combo("##i", &li, kIntOpts, 4)) {
				e.intensity = IdxToInt(li);
				WeatherOverrides::SetPerForm(row.formID, e);
			}
			ImGuiMCP::PopID();
			ImGuiMCP::PopItemWidth();

			ImGuiMCP::PopID();
			ImGuiMCP::Spacing();
		}
		ImGuiMCP::EndChild();
	}

	// ── Live debug page ──────────────────────────────────────────────────

	void __stdcall DrawDebugPage()
	{
		const auto d = RainSplashes::GetRainDebugInfo();

		ImGuiMCP::TextWrapped("Live data from the game engine. Updates every frame.");
		ImGuiMCP::Separator();

		// Player / Sky
		ImGuiMCP::Text("Location: %s", d.playerInInteriorCell ? "Interior cell" : "Exterior");
		if (!d.skyInstanceFound) {
			ImGuiMCP::TextColored(ImGuiMCP::ImVec4{ 1.0f, 0.4f, 0.4f, 1.0f }, "Sky not available");
			return;
		}
		ImGuiMCP::Text("Sky mode: %s", d.skyModeName.c_str());
		ImGuiMCP::Text("Wetness: %.3f    Wind: %.3f", d.lastExtWetness, d.windSpeed);

		// Current weather
		ImGuiMCP::Separator();
		if (d.hasCurrentWeather) {
			ImGuiMCP::Text("Current weather: %s  [%08X]",
				d.currentWeatherEditorID.empty() ? "(unnamed)" : d.currentWeatherEditorID.c_str(),
				d.currentWeatherFormID);

			const std::uint8_t f = d.weatherTypeFlags;
			ImGuiMCP::Text("  Flags: %s%s%s%s",
				(f & 0x01u) ? "Pleasant " : "",
				(f & 0x02u) ? "Cloudy "   : "",
				(f & 0x04u) ? "Rainy "    : "",
				(f & 0x08u) ? "Snowy "    : "");

			ImGuiMCP::Text("  Vanilla rainy: %s", d.currentWeatherIsRainy ? "Yes" : "No");
			ImGuiMCP::Text("  Effective (after overrides): %s",
				d.effectiveRainyAfterOverrides ? "Rainy" : "Clear");
		} else {
			ImGuiMCP::Text("Current weather: (none)");
		}

		if (d.hasLastWeather) {
			ImGuiMCP::Text("Previous: %s  [%08X]",
				d.lastWeatherEditorID.empty() ? "(unnamed)" : d.lastWeatherEditorID.c_str(),
				d.lastWeatherFormID);
		}
		if (d.hasOverrideWeather) {
			ImGuiMCP::Text("Override: %s  [%08X]",
				d.overrideWeatherEditorID.empty() ? "(unnamed)" : d.overrideWeatherEditorID.c_str(),
				d.overrideWeatherFormID);
		}

		// Rain result
		ImGuiMCP::Separator();
		ImGuiMCP::Text("Intensity proxy: %.2f", d.proxyDensity);
		ImGuiMCP::Text("Active rain tier: %s", RainClassName(d.rainClass));

		if (!d.globalOverrideSummary.empty()) {
			ImGuiMCP::TextDisabled("%s", d.globalOverrideSummary.c_str());
		}

		ImGuiMCP::Separator();
		ImGuiMCP::Text("Splashes (BSModelDB + cell scene):");
		ImGuiMCP::Text("  Active: %u    Pool: %u", d.splashActiveCount, d.splashPoolTotal);
		ImGuiMCP::Text("  Proto loaded: %s", d.splashTemplateReady ? "yes" : "no");
		if (!d.splashTemplateStatus.empty()) {
			ImGuiMCP::TextWrapped("  Status: %s", d.splashTemplateStatus.c_str());
		}
		ImGuiMCP::Text("  Cover threshold: %.0f", d.coverThreshold);
	}
}

void UI::Register()
{
	if (!F4SEMenuFramework::IsInstalled()) {
		logger::warn("RainSplashesF4SE: F4SE Menu Framework not found — in-game UI disabled.");
		return;
	}
	F4SEMenuFramework::SetSection("Rain Splashes");
	F4SEMenuFramework::AddSectionItem("Settings", DrawSettingsPage);
	F4SEMenuFramework::AddSectionItem("Weather Overrides", DrawWeatherOverridesPage);
	F4SEMenuFramework::AddSectionItem("Debug Info", DrawDebugPage);
}
