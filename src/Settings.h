#pragma once

struct RainTierSettings
{
	bool   enabled{ true };
	float  rayCastRadius{ 1024.0f };
	std::uint32_t rayCastIterations{ 1 };

	bool   splashEnabled{ true };
	std::string splashNif{ R"(Meshes\Effects\Rain\rain_rainSplashGround_genericA.nif)" };
	std::string splashNifActor{ R"(Meshes\Effects\Rain\rain_rainSplashGround_genericA.nif)" };
	float  splashNifScale{ 0.5f };
	float  splashNifScaleActor{ 0.4f };
	float  splashLifetime{ 1.0f };
};

struct GlobalSettings
{
	bool masterEnabled{ true };
	bool debugSplashes{ false };
	float coverThreshold{ 300.0f };
	bool  spawnOnActors{ false };
	bool  spawnOnPlayer{ false };
	float rainDensityLightThreshold{ 5.0f };
	float rainDensityHeavyThreshold{ 9.0f };
	float rainProxyWetnessScale{ 20.0f };
	float rainProxyWindScale{ 2.0f };
	// Must sit below rainDensityLightThreshold or the Light tier is
	// unreachable: the proxy is base + wetness/wind terms, so a base at the
	// light ceiling starts every shower at Medium.
	float rainProxyBase{ 2.0f };
};

class Settings
{
public:
	static Settings& Get();

	bool LoadFromFile(const std::string& a_path);
	bool SaveToFile(const std::string& a_path) const;

	GlobalSettings global;
	RainTierSettings light;
	RainTierSettings medium;
	RainTierSettings heavy;

	enum class RainClass
	{
		kNone,
		kLight,
		kMedium,
		kHeavy
	};

	[[nodiscard]] const RainTierSettings* SelectTier(RainClass a_class) const;
	[[nodiscard]] RainClass ClassifyRainDensity(float a_density) const;

	// Last observed values (for ImGui diagnostics)
	mutable std::mutex diagMutex;
	float lastRainDensityProxy{ 0.0f };
	RainClass lastRainClass{ RainClass::kNone };

private:
	Settings() = default;

	static void LoadTier(CSimpleIniA& ini, const char* section, RainTierSettings& out);
	static void SaveTier(CSimpleIniA& ini, const char* section, const RainTierSettings& in);
};
