#pragma once

namespace RE
{
	class NiCamera;

	class Main
	{
	public:
		[[nodiscard]] static NiCamera* WorldRootCamera()
		{
			using func_t = NiCamera* (*)();
			static REL::Relocation<func_t> func{ REL::ID(384264) };
			return func();
		}

	private:
		Main() = delete;
	};
}
