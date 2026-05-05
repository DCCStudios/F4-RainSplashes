#pragma once

#include "RE/Bethesda/BSTHashMap.h"
#include "RE/NetImmerse/NiObject.h"
#include "RE/NetImmerse/NiPoint3.h"

namespace RE
{
	// Layout matches alandtse CommonLibF4 NiCloningProcess (Creation Engine / NetImmerse).
	class NiCloningProcess
	{
	public:
		enum class CopyType : std::uint32_t
		{
			kNone = 0,
			kCopyExact,
			kCopyUnique,
		};

		// members
		BSTHashMap<NiObject*, NiObject*> cloneMap;  // 00
		BSTHashMap<NiObject*, bool>       processMap;  // 30
		std::uint32_t                    copyType{ 0 };  // 60
		char                             appendChar{ 0 };  // 64
		NiPoint3                         scale{ 1.0F, 1.0F, 1.0F };  // 68
	};
}
