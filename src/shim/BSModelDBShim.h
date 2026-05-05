#pragma once

#include "RE/Bethesda/BSResource.h"
#include "RE/NetImmerse/NiNode.h"
#include "RE/NetImmerse/NiSmartPointer.h"
#include "REL/Relocation.h"

namespace RE
{
	namespace BSModelDBShim
	{
		// Matches BSModelDB::DBTraits::ArgsType (12 bytes).
		// Bitfield flags at byte 8 control post-load processing.
		struct ModelDemandArgs
		{
			std::int32_t  lodFadeMult{ 1 };  // ENUM_LOD_MULT::kObjects
			std::uint32_t loadLevel{ 0 };
			std::uint8_t  prepareAfterLoad : 1 { 1 };  // 08:0 — prepare controllers/materials
			std::uint8_t  faceGenModel     : 1 { 0 };  // 08:1
			std::uint8_t  useErrorMarker   : 1 { 0 };  // 08:2
			std::uint8_t  performProcess   : 1 { 1 };  // 08:3 — run post-processing
			std::uint8_t  createFadeNode   : 1 { 0 };  // 08:4
			std::uint8_t  loadTextures     : 1 { 1 };  // 08:5 — load textures for rendering
			std::uint8_t  pad08_67         : 2 { 0 };  // 08:6-7
			std::uint8_t  pad09{ 0 };
			std::uint16_t pad0A{ 0 };
		};
		static_assert(sizeof(ModelDemandArgs) == 0x0C);

		[[nodiscard]] inline BSResource::ErrorCode Demand(
			const char*               a_name,
			NiPointer<NiNode>*        a_result,
			const ModelDemandArgs&    a_args)
		{
			using func_t = BSResource::ErrorCode (*)(const char*, NiPointer<NiNode>*, const ModelDemandArgs&);
			static REL::Relocation<func_t> func{ REL::ID(1225688) };
			return func(a_name, a_result, a_args);
		}
	}
}
