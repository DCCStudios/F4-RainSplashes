// Rain splashes for Fallout 4 — inspired by powerof_three's Splashes of Storms (Skyrim, MIT).

#include "PCH.h"

#include "Hooks.h"
#include "RainSplashes.h"
#include "RelSanity.h"
#include "UI.h"

#include "logger.h"

namespace Plugin
{
	static constexpr auto NAME = "RainSplashesF4SE"sv;
	static const REL::Version VERSION{ 1, 0, 0 };
}

void MessageCallback(F4SE::MessagingInterface::Message* a_msg)
{
	if (!a_msg) {
		return;
	}
	switch (a_msg->type) {
	case F4SE::MessagingInterface::kGameDataReady:
		RelSanity::Init();
		RainSplashes::OnGameDataReady();
		UI::Register();
		if (!RelSanity::Ok()) {
			logger::error("RainSplashesF4SE: splashes/hooks disabled until Address Library matches this EXE (menu still opens).");
			break;
		}
		F4SE::AllocTrampoline(128);
		Hooks::Install();
		break;
	case F4SE::MessagingInterface::kPostLoadGame:
		RainSplashes::OnPostLoadGame();
		break;
	default:
		break;
	}
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = static_cast<std::uint32_t>(Plugin::VERSION[0]);

	if (!a_f4se || a_f4se->IsEditor()) {
		return false;
	}
	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		return false;
	}
	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	SetupLog();
	logger::info("{} v{}.{}.{}", Plugin::NAME, Plugin::VERSION[0], Plugin::VERSION[1], Plugin::VERSION[2]);

	F4SE::Init(a_f4se);

	const auto* messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageCallback)) {
		return false;
	}

	return true;
}
