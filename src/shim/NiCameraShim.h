#pragma once

#include "RE/NetImmerse/NiPoint3.h"

namespace RE
{
	class NiCamera
	{
	public:
		// Pre-NG Address Library lacks the BoundInFrustum ID; the
		// rayCastRadius setting already constrains splash placement,
		// so skipping the frustum cull is functionally harmless.
		bool PointInFrustum(const NiPoint3&, float) { return true; }

	private:
		NiCamera() = delete;
	};
}
