#include "PCH.h"

#include "Hooks.h"

#include "RainSplashes.h"

#include "logger.h"

namespace
{
	using RunActorUpdatesFn = void (*)(void*, float, bool);
	REL::Relocation<std::uintptr_t> kRunActorUpdates{ REL::ID(556439), 0x17 };
	RunActorUpdatesFn               g_origRunActorUpdates{ nullptr };
	std::atomic<bool>               g_installed{ false };

	void HookedRunActorUpdates(void* a_list, float a_delta, bool a_instant)
	{
		if (g_origRunActorUpdates) {
			g_origRunActorUpdates(a_list, a_delta, a_instant);
		}
		RainSplashes::TickOnMainThread(a_delta);
	}
}

void Hooks::Install()
{
	bool expected = false;
	if (!g_installed.compare_exchange_strong(expected, true)) {
		return;
	}

	auto& trampoline = F4SE::GetTrampoline();
	g_origRunActorUpdates = reinterpret_cast<RunActorUpdatesFn>(
		trampoline.write_call<5>(kRunActorUpdates.address(), &HookedRunActorUpdates));

	logger::info("RainSplashesF4SE: hooked RunActorUpdates @ REL::ID(556439)+0x17");
}
