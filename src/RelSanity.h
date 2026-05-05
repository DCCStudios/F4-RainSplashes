#pragma once

namespace RelSanity
{
	// Loads Address Library `version-*.bin` and verifies every REL::ID this plugin needs.
	// Call once from kGameDataReady before constructing REL::Relocation for splash paths.
	void Init();

	[[nodiscard]] bool Ok();
}
