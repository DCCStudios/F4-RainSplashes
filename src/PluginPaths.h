#pragma once

#include <filesystem>

namespace PluginPaths
{
	// Directory containing RainSplashesF4SE.dll (Data/F4SE/Plugins or .../RainSplashesF4SE).
	[[nodiscard]] std::filesystem::path GetDllContainingDirectory();

	// Data/F4SE/Plugins/RainSplashesF4SE — used for JSON when the DLL lives in Plugins root.
	[[nodiscard]] std::filesystem::path GetRainSplashesDataDirectory();
}
