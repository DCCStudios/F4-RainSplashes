#include "PCH.h"

#include "RelSanity.h"

#include <fmt/format.h>
#include <mmio/mmio.hpp>

#include "REL/Relocation.h"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace
{
	struct Mapping
	{
		std::uint64_t id;
		std::uint64_t offset;
	};

	static bool                                    g_done{};
	static bool                                    g_ok{};
	static std::vector<Mapping>                    g_table;

	struct IdName
	{
		std::uint64_t  id;
		const char*    name;
	};

	// Every REL::ID used on splash / hook paths (see Hooks, RayCast, shims).
	// IDs are for the pre-NG Address Library (version-1-10-163-0.bin, max ~1.58M).
	constexpr IdName kRequired[] = {
		{ 556439,   "RunActorUpdates (hook call site)" },
		{ 484694,   "Sky::GetSingleton" },
		{ 384264,   "Main::WorldRootCamera" },
		{ 434717,   "TESObjectCELL pick / CellPick" },
		{ 526783,   "bhkPickData ctor" },
		{ 747470,   "bhkPickData::SetStartEnd" },
		{ 438299,   "bhkPickData::Reset" },
		{ 1181584,  "bhkPickData::HasHit" },
		{ 476687,   "bhkPickData::GetHitFraction" },
		{ 1288513,  "bhkPickData::GetAllCollectorRayHitSize" },
		{ 583997,   "bhkPickData::GetAllCollectorRayHitAt" },
		{ 1274842,  "bhkPickData::SortAllCollectorHits" },
		{ 863406,   "bhkPickData::GetNiAVObject" },
		{ 1223055,  "bhkPickData::GetBody" },
		{ 1225688,  "BSModelDB::Demand (splash mesh load)" },
		{ 1075623,  "BSTempEffectDebris ctor" },
		{ 1569706,  "ProcessLists singleton" },
	};

	[[nodiscard]] bool LoadTable()
	{
		const auto ver = REL::Module::get().version();
		const auto path = fmt::format(FMT_STRING("Data/F4SE/Plugins/version-{}.bin"), ver.string());
		mmio::mapped_file_source mmap;
		if (!mmap.open(std::filesystem::path{ path })) {
			logger::error("RainSplashesF4SE: RelSanity: could not open Address Library '{}'", path);
			return false;
		}
		const std::byte* bytes = mmap.data();
		const auto        nbytes = mmap.size();
		if (nbytes < sizeof(std::uint64_t)) {
			logger::error("RainSplashesF4SE: RelSanity: '{}' is too small", path);
			return false;
		}
		const auto count = *reinterpret_cast<const std::uint64_t*>(bytes);
		const auto need = sizeof(std::uint64_t) + count * sizeof(Mapping);
		if (count == 0 || nbytes < need) {
			logger::error("RainSplashesF4SE: RelSanity: '{}' has invalid entry count ({})", path, count);
			return false;
		}
		const auto* arr = reinterpret_cast<const Mapping*>(bytes + sizeof(std::uint64_t));
		g_table.assign(arr, arr + count);
		return true;
	}

	[[nodiscard]] bool HasId(std::uint64_t a_id)
	{
		const auto it = std::lower_bound(
			g_table.begin(), g_table.end(), a_id,
			[](const Mapping& a, std::uint64_t a_id2) { return a.id < a_id2; });
		return it != g_table.end() && it->id == a_id;
	}
}

void RelSanity::Init()
{
	if (g_done) {
		return;
	}
	g_done = true;

	if (!LoadTable()) {
		g_ok = false;
		return;
	}

	std::string missing;
	for (const auto& row : kRequired) {
		if (!HasId(row.id)) {
			if (!missing.empty()) {
				missing += ", ";
			}
			missing += fmt::format(FMT_STRING("{} (0x{:X})"), row.name, row.id);
		}
	}

	if (!missing.empty()) {
		logger::error(
			"RainSplashesF4SE: Address Library is missing required REL::ID(s) for this Fallout4.exe build — {}",
			missing);
		logger::error(
			"RainSplashesF4SE: Install/update Address Library for Fallout 4 so Data/F4SE/Plugins/version-{}.bin matches your game executable.",
			REL::Module::get().version().string());
		g_ok = false;
		return;
	}

	g_ok = true;
	logger::info("RainSplashesF4SE: RelSanity: all {} required Address Library IDs resolved",
		static_cast<unsigned>(sizeof(kRequired) / sizeof(kRequired[0])));
}

bool RelSanity::Ok()
{
	return g_ok;
}
