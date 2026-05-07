#include "Settings.h"

namespace
{
	// FO4's BSModelDB expects paths relative to Data\ (including the Meshes\ prefix).
	// Auto-prepend if the user's INI path doesn't already start with it.
	std::string EnsureMeshesPrefix(const std::string& a_path)
	{
		if (a_path.size() >= 7) {
			std::string lower7 = a_path.substr(0, 7);
			for (auto& c : lower7) {
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			}
			if (lower7 == "meshes\\" || lower7 == "meshes/") {
				return a_path;
			}
		}
		return "Meshes\\" + a_path;
	}
}

Settings& Settings::Get()
{
	static Settings s;
	return s;
}

void Settings::LoadTier(CSimpleIniA& ini, const char* section, RainTierSettings& out)
{
	out.enabled = ini.GetBoolValue(section, "Enabled", out.enabled);
	out.rayCastRadius = static_cast<float>(ini.GetDoubleValue(section, "RaycastRadius", out.rayCastRadius));
	out.rayCastIterations = static_cast<std::uint32_t>(ini.GetLongValue(section, "RaycastIterations", static_cast<long>(out.rayCastIterations)));

	out.splashEnabled = ini.GetBoolValue(section, "SplashEnabled", out.splashEnabled);
	out.splashNif = EnsureMeshesPrefix(ini.GetValue(section, "SplashNif", out.splashNif.c_str()));
	out.splashNifActor = EnsureMeshesPrefix(ini.GetValue(section, "SplashNifActor", out.splashNifActor.c_str()));
	out.splashNifScale = static_cast<float>(ini.GetDoubleValue(section, "SplashNifScale", out.splashNifScale));
	out.splashNifScaleActor = static_cast<float>(ini.GetDoubleValue(section, "SplashNifScaleActor", out.splashNifScaleActor));
	out.splashLifetime = static_cast<float>(ini.GetDoubleValue(section, "SplashLifetime", out.splashLifetime));
}

void Settings::SaveTier(CSimpleIniA& ini, const char* section, const RainTierSettings& in)
{
	ini.SetBoolValue(section, "Enabled", in.enabled);
	ini.SetDoubleValue(section, "RaycastRadius", in.rayCastRadius);
	ini.SetLongValue(section, "RaycastIterations", static_cast<long>(in.rayCastIterations));

	ini.SetBoolValue(section, "SplashEnabled", in.splashEnabled);
	ini.SetValue(section, "SplashNif", in.splashNif.c_str());
	ini.SetValue(section, "SplashNifActor", in.splashNifActor.c_str());
	ini.SetDoubleValue(section, "SplashNifScale", in.splashNifScale);
	ini.SetDoubleValue(section, "SplashNifScaleActor", in.splashNifScaleActor);
	ini.SetDoubleValue(section, "SplashLifetime", in.splashLifetime);
}

bool Settings::LoadFromFile(const std::string& a_path)
{
	CSimpleIniA ini;
	ini.SetUnicode();
	const SI_Error rc = ini.LoadFile(a_path.c_str());
	if (rc < SI_OK) {
		return false;
	}

	global.masterEnabled = ini.GetBoolValue("Global", "MasterEnabled", global.masterEnabled);
	global.debugSplashes = ini.GetBoolValue("Global", "DebugSplashes", global.debugSplashes);
	global.coverThreshold = static_cast<float>(ini.GetDoubleValue("Global", "CoverThreshold", global.coverThreshold));
	global.spawnOnActors = ini.GetBoolValue("Global", "SpawnOnActors", global.spawnOnActors);
	global.spawnOnPlayer = ini.GetBoolValue("Global", "SpawnOnPlayer", global.spawnOnPlayer);
	global.rainDensityLightThreshold = static_cast<float>(ini.GetDoubleValue("Global", "RainDensityLightThreshold", global.rainDensityLightThreshold));
	global.rainDensityHeavyThreshold = static_cast<float>(ini.GetDoubleValue("Global", "RainDensityHeavyThreshold", global.rainDensityHeavyThreshold));
	global.rainProxyWetnessScale = static_cast<float>(ini.GetDoubleValue("Global", "RainProxyWetnessScale", global.rainProxyWetnessScale));
	global.rainProxyWindScale = static_cast<float>(ini.GetDoubleValue("Global", "RainProxyWindScale", global.rainProxyWindScale));
	global.rainProxyBase = static_cast<float>(ini.GetDoubleValue("Global", "RainProxyBase", global.rainProxyBase));

	LoadTier(ini, "LightRain", light);
	LoadTier(ini, "MediumRain", medium);
	LoadTier(ini, "HeavyRain", heavy);

	return true;
}

bool Settings::SaveToFile(const std::string& a_path) const
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.SetBoolValue("Global", "MasterEnabled", global.masterEnabled);
	ini.SetBoolValue("Global", "DebugSplashes", global.debugSplashes);
	ini.SetDoubleValue("Global", "CoverThreshold", global.coverThreshold);
	ini.SetBoolValue("Global", "SpawnOnActors", global.spawnOnActors);
	ini.SetBoolValue("Global", "SpawnOnPlayer", global.spawnOnPlayer);
	ini.SetDoubleValue("Global", "RainDensityLightThreshold", global.rainDensityLightThreshold);
	ini.SetDoubleValue("Global", "RainDensityHeavyThreshold", global.rainDensityHeavyThreshold);
	ini.SetDoubleValue("Global", "RainProxyWetnessScale", global.rainProxyWetnessScale);
	ini.SetDoubleValue("Global", "RainProxyWindScale", global.rainProxyWindScale);
	ini.SetDoubleValue("Global", "RainProxyBase", global.rainProxyBase);

	SaveTier(ini, "LightRain", light);
	SaveTier(ini, "MediumRain", medium);
	SaveTier(ini, "HeavyRain", heavy);

	const SI_Error rc = ini.SaveFile(a_path.c_str());
	return rc >= SI_OK;
}

const RainTierSettings* Settings::SelectTier(RainClass a_class) const
{
	switch (a_class) {
	case RainClass::kLight:
		return &light;
	case RainClass::kMedium:
		return &medium;
	case RainClass::kHeavy:
		return &heavy;
	default:
		return nullptr;
	}
}

Settings::RainClass Settings::ClassifyRainDensity(float a_density) const
{
	if (a_density < 1.0f) {
		return RainClass::kNone;
	}
	if (a_density < global.rainDensityLightThreshold) {
		return RainClass::kLight;
	}
	if (a_density < global.rainDensityHeavyThreshold) {
		return RainClass::kMedium;
	}
	return RainClass::kHeavy;
}
