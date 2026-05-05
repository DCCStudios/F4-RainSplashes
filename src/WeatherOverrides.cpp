#include "PCH.h"

#include "WeatherOverrides.h"

#include "PluginPaths.h"

#include "RE/Bethesda/TESDataHandler.h"
#include "RE/Bethesda/TESFile.h"
#include "RE/Bethesda/TESForms.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

namespace
{
	constexpr std::int8_t kVanillaRainFlag = 0x04;
	constexpr int         kWeatherTypeByte = 11;

	std::mutex g_mutex;
	WeatherOverrides::GlobalActiveOverride g_global{};
	std::map<std::uint32_t, WeatherOverrides::PerFormEntry> g_perForm{};

	const char* RainToJson(WeatherOverrides::RainTriState v)
	{
		switch (v) {
		case WeatherOverrides::RainTriState::kInherit:    return "inherit";
		case WeatherOverrides::RainTriState::kForceRainy: return "rainy";
		case WeatherOverrides::RainTriState::kForceClear: return "clear";
		}
		return "inherit";
	}

	WeatherOverrides::RainTriState RainFromJson(std::string_view a_s)
	{
		if (a_s == "rainy") { return WeatherOverrides::RainTriState::kForceRainy; }
		if (a_s == "clear") { return WeatherOverrides::RainTriState::kForceClear; }
		return WeatherOverrides::RainTriState::kInherit;
	}

	const char* IntensityToJson(WeatherOverrides::IntensityTriState v)
	{
		switch (v) {
		case WeatherOverrides::IntensityTriState::kInherit: return "inherit";
		case WeatherOverrides::IntensityTriState::kLight:   return "light";
		case WeatherOverrides::IntensityTriState::kMedium:   return "medium";
		case WeatherOverrides::IntensityTriState::kHeavy:   return "heavy";
		}
		return "inherit";
	}

	WeatherOverrides::IntensityTriState IntensityFromJson(std::string_view a_s)
	{
		if (a_s == "light") { return WeatherOverrides::IntensityTriState::kLight; }
		if (a_s == "medium") { return WeatherOverrides::IntensityTriState::kMedium; }
		if (a_s == "heavy") { return WeatherOverrides::IntensityTriState::kHeavy; }
		return WeatherOverrides::IntensityTriState::kInherit;
	}

	Settings::RainClass MapIntensity(WeatherOverrides::IntensityTriState a_i)
	{
		switch (a_i) {
		case WeatherOverrides::IntensityTriState::kLight:  return Settings::RainClass::kLight;
		case WeatherOverrides::IntensityTriState::kMedium: return Settings::RainClass::kMedium;
		case WeatherOverrides::IntensityTriState::kHeavy:  return Settings::RainClass::kHeavy;
		default:                                          return Settings::RainClass::kNone;
		}
	}

	std::optional<std::uint32_t> ParseFormKey(std::string_view a_key)
	{
		const std::string s(a_key);
		char*             end = nullptr;
		const unsigned long v = std::strtoul(s.c_str(), &end, 0);
		if (end != s.c_str() && end == s.c_str() + s.size() && v <= 0xFFFFFFFFUL) {
			return static_cast<std::uint32_t>(v);
		}
		return std::nullopt;
	}
}

void WeatherOverrides::OnGameDataReady()
{
	const auto path = GetJsonPath();
	if (!std::filesystem::exists(path)) {
		std::scoped_lock lk(g_mutex);
		g_global = {};
		g_perForm.clear();
		return;
	}
	if (!LoadFromDefaultPath()) {
		logger::warn("RainSplashesF4SE: could not load weather overrides JSON '{}'", path.string());
	}
}

std::filesystem::path WeatherOverrides::GetJsonPath()
{
	const auto dir = PluginPaths::GetRainSplashesDataDirectory();
	if (!dir.empty()) {
		return dir / L"RainSplashesF4SE_WeatherOverrides.json";
	}
	return std::filesystem::path{ L"Data/F4SE/Plugins/RainSplashesF4SE/RainSplashesF4SE_WeatherOverrides.json" };
}

bool WeatherOverrides::LoadFromFile(const std::filesystem::path& a_path)
{
	std::ifstream in(a_path, std::ios::binary);
	if (!in) {
		return false;
	}
	nlohmann::json j;
	try {
		in >> j;
	} catch (...) {
		return false;
	}

	GlobalActiveOverride global{};
	std::map<std::uint32_t, PerFormEntry> perForm{};

	if (j.contains("globalForActiveWeather") && j["globalForActiveWeather"].is_object()) {
		const auto& g = j["globalForActiveWeather"];
		if (g.contains("rain")) {
			global.rain = RainFromJson(g["rain"].get<std::string>());
		}
		if (g.contains("intensity")) {
			global.intensity = IntensityFromJson(g["intensity"].get<std::string>());
		}
	}

	if (j.contains("forms") && j["forms"].is_object()) {
		for (auto it = j["forms"].begin(); it != j["forms"].end(); ++it) {
			const auto fid = ParseFormKey(it.key());
			if (!fid) {
				continue;
			}
			PerFormEntry e{};
			if (it.value().is_object()) {
				if (it.value().contains("rain")) {
					e.rain = RainFromJson(it.value()["rain"].get<std::string>());
				}
				if (it.value().contains("intensity")) {
					e.intensity = IntensityFromJson(it.value()["intensity"].get<std::string>());
				}
			}
			if (e.rain != RainTriState::kInherit || e.intensity != IntensityTriState::kInherit) {
				perForm[*fid] = e;
			}
		}
	}

	{
		std::scoped_lock lk(g_mutex);
		g_global = global;
		g_perForm = std::move(perForm);
	}
	return true;
}

bool WeatherOverrides::SaveToFile(const std::filesystem::path& a_path)
{
	GlobalActiveOverride global{};
	std::map<std::uint32_t, PerFormEntry> perForm{};
	{
		std::scoped_lock lk(g_mutex);
		global = g_global;
		perForm = g_perForm;
	}

	nlohmann::json j;
	j["version"] = 1;
	j["globalForActiveWeather"]["rain"]      = RainToJson(global.rain);
	j["globalForActiveWeather"]["intensity"] = IntensityToJson(global.intensity);

	nlohmann::json forms = nlohmann::json::object();
	for (const auto& [fid, e] : perForm) {
		if (e.rain == RainTriState::kInherit && e.intensity == IntensityTriState::kInherit) {
			continue;
		}
		char key[32];
		std::snprintf(key, sizeof(key), "0x%08X", static_cast<unsigned>(fid));
		forms[key] = {
			{ "rain",      RainToJson(e.rain) },
			{ "intensity", IntensityToJson(e.intensity) },
		};
	}
	j["forms"] = std::move(forms);

	const auto parent = a_path.parent_path();
	if (!parent.empty()) {
		std::error_code ec;
		std::filesystem::create_directories(parent, ec);
	}

	std::ofstream out(a_path, std::ios::binary | std::ios::trunc);
	if (!out) {
		return false;
	}
	out << j.dump(2);
	return true;
}

WeatherOverrides::Snapshot WeatherOverrides::GetSnapshot()
{
	std::scoped_lock lk(g_mutex);
	return Snapshot{ g_global, g_perForm };
}

void WeatherOverrides::SetGlobalActive(const GlobalActiveOverride& a_v)
{
	std::scoped_lock lk(g_mutex);
	g_global = a_v;
}

void WeatherOverrides::SetPerForm(std::uint32_t a_formID, PerFormEntry a_entry)
{
	std::scoped_lock lk(g_mutex);
	if (a_entry.rain == RainTriState::kInherit && a_entry.intensity == IntensityTriState::kInherit) {
		g_perForm.erase(a_formID);
	} else {
		g_perForm[a_formID] = a_entry;
	}
}

void WeatherOverrides::ClearPerForm(std::uint32_t a_formID)
{
	std::scoped_lock lk(g_mutex);
	g_perForm.erase(a_formID);
}

bool WeatherOverrides::IsVanillaDataRainy(const RE::TESWeather* a_weather)
{
	if (!a_weather) {
		return false;
	}
	return (a_weather->weatherData[kWeatherTypeByte] & kVanillaRainFlag) != 0;
}

bool WeatherOverrides::IsEffectiveRainy(const RE::TESWeather* a_weather, const Snapshot& snap)
{
	if (!a_weather) {
		return false;
	}
	if (const auto it = snap.perForm.find(a_weather->GetFormID()); it != snap.perForm.end()) {
		if (it->second.rain == RainTriState::kForceRainy) {
			return true;
		}
		if (it->second.rain == RainTriState::kForceClear) {
			return false;
		}
	}
	if (snap.globalActive.rain == RainTriState::kForceRainy) {
		return true;
	}
	if (snap.globalActive.rain == RainTriState::kForceClear) {
		return false;
	}
	return IsVanillaDataRainy(a_weather);
}

Settings::RainClass WeatherOverrides::EffectiveRainClass(
	const RE::TESWeather* a_weather,
	float                 a_proxyDensity,
	const Settings&       a_settings,
	const Snapshot&       snap)
{
	if (a_weather) {
		if (const auto it = snap.perForm.find(a_weather->GetFormID()); it != snap.perForm.end()) {
			if (it->second.intensity != IntensityTriState::kInherit) {
				return MapIntensity(it->second.intensity);
			}
		}
	}
	if (snap.globalActive.intensity != IntensityTriState::kInherit) {
		return MapIntensity(snap.globalActive.intensity);
	}
	return a_settings.ClassifyRainDensity(a_proxyDensity);
}

std::vector<WeatherOverrides::CatalogRow> WeatherOverrides::BuildWeatherCatalog()
{
	std::vector<CatalogRow> out;
	auto*                   dh = RE::TESDataHandler::GetSingleton();
	if (!dh) {
		return out;
	}

	auto& arr = dh->GetFormArray<RE::TESWeather>();
	for (auto* w : arr) {
		if (!w) {
			continue;
		}
		CatalogRow row{};
		row.formID = w->GetFormID();
		const char* ed = w->GetFormEditorID();
		row.editorID = ed ? ed : "";
		if (const auto* f = w->GetFile()) {
			row.pluginName = std::string{ f->GetFilename() };
		} else {
			row.pluginName = "?";
		}
		row.vanillaRainy = IsVanillaDataRainy(w);
		out.push_back(std::move(row));
	}

	std::sort(out.begin(), out.end(), [](const CatalogRow& a, const CatalogRow& b) {
		if (a.pluginName != b.pluginName) {
			return a.pluginName < b.pluginName;
		}
		return a.editorID < b.editorID;
	});
	return out;
}
