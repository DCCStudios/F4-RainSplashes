#pragma once

namespace RayCast
{
	struct Output
	{
		RE::NiPoint3  hitPos{};
		RE::NiMatrix3 normal{};
		bool          hitActor{ false };
		bool          hitPlayer{ false };
		bool          hitWater{ false };
	};

	std::optional<RE::NiPoint3> RandomPointInDiskAround(float a_radius, const RE::NiPoint3& a_center, bool a_requireInCameraFrustum);

	std::optional<Output> CastVerticalCell(RE::TESObjectCELL* a_cell, const RE::NiPoint3& a_xyOrigin);
}
