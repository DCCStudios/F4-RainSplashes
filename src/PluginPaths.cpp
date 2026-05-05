#include "PCH.h"

#include "PluginPaths.h"

#include <Windows.h>

namespace
{
	static void ModuleAnchor() noexcept {}
}

std::filesystem::path PluginPaths::GetDllContainingDirectory()
{
	HMODULE module{};
	if (!::GetModuleHandleExW(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(&ModuleAnchor),
			&module)) {
		return {};
	}
	wchar_t buf[MAX_PATH]{};
	if (::GetModuleFileNameW(module, buf, MAX_PATH) == 0) {
		return {};
	}
	const std::filesystem::path dll{ buf };
	return dll.parent_path().lexically_normal();
}

std::filesystem::path PluginPaths::GetRainSplashesDataDirectory()
{
	const auto parent = GetDllContainingDirectory();
	if (parent.empty()) {
		return {};
	}
	if (parent.filename() == L"RainSplashesF4SE") {
		return parent;
	}
	return (parent / L"RainSplashesF4SE").lexically_normal();
}
