/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_roads.cpp Paid conventional freight infrastructure in the genuine Surface game.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BD.1.
 * Native placement, clearing and command finance own all infrastructure. The
 * search records candidate connections only; it cannot paint roads or terrain,
 * supply inventory, replace factories or advance native construction clocks.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../industry.h"
#include "../industry_map.h"
#include "../industrytype.h"
#include "../multilayer_industry.h"
#include "../station_base.h"
#include "../station_cmd.h"
#include "../station_map.h"
#include "../station_func.h"
#include "../road_cmd.h"
#include "../road_func.h"
#include "../road_map.h"
#include "../cargopacket.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <queue>
#include "../safeguards.h"

static std::function<void()> *surface_roads_observer = nullptr; ///< Loaded-world callback; native commands remain the sole infrastructure authority.

/** Invoke the current original-world road construction observer.
 * @param args Parameter-free native console command arguments.
 * @return True after the current callback ran.
 */
static bool ObserveSurfaceRoads(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_roads_observer != nullptr);
	(*surface_roads_observer)(); return true;
}

/** Save actual native owners without ticking or changing the source.
 * @param path Fresh invocation-owned checkpoint.
 */
static void SaveSurfaceRoads(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Execute one original synchronized, normally charged infrastructure command.
 * @tparam Tcmd Actual native command identity.
 * @tparam Targs Remaining typed native command arguments.
 * @param tile Explicit Surface action/command anchor.
 * @param args Original command arguments, with no private owner writes.
 * @return Original actual whole invoice.
 */
template <Commands Tcmd, typename... Targs>
static Money PaySurfaceRoadCommand(TileIndex tile, Targs... args)
{
	const auto cash = Company::Get(_current_company)->money;
	auto result = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false,
		tile, std::make_tuple(tile, args...));
	const auto &cost = ExtractCommandCost(result);
	INFO("road command=" << GetCommandName(Tcmd) << " tile=" << tile.base() << " error=" << cost.GetErrorMessage().base());
	REQUIRE(cost.Succeeded()); CHECK(Company::Get(_current_company)->money == cash - cost.GetCost());
	fmt::print("SURFACE_ENTRY_ROAD_INVOICE command={} tile={} cost={}\n", GetCommandName(Tcmd), tile.base(), cost.GetCost().base());
	return cost.GetCost();
}

/** Clip one physical adjacent step to the original Surface plane.
 * @param tile Original Surface tile.
 * @param direction Cardinal native map direction.
 * @return Real adjacent tile, or INVALID_TILE for an edge/wrap/layer escape.
 */
static TileIndex SurfaceRoadStep(TileIndex tile, DiagDirection direction)
{
	const auto next = AddTileIndexDiffCWrap(tile, TileIndexDiffCByDiagDir(direction));
	// Both ownership and adjacency are required: stacked storage and edge wraps
	// cannot provide free physical transport between projected coordinates.
	if (!MultiLayerMap::IsTileInLayer(next, MapLayer::Surface) ||
		std::abs(int(TileX(next)) - int(TileX(tile))) + std::abs(int(TileY(next)) - int(TileY(tile))) != 1) return INVALID_TILE;
	return next;
}

/** Admit only ordinary clear/tree/road terrain without demolishing existing gameplay owners.
 * @param tile Real candidate or INVALID_TILE.
 * @return Whether a road placement query may safely consider this candidate.
 */
static bool IsSurfaceRoadCandidate(TileIndex tile)
{
	if (tile == INVALID_TILE) return false;
	return IsTileType(tile, TileType::Clear) || IsTileType(tile, TileType::Trees) || IsNormalRoadTile(tile);
}

/** Test the exact road shape against original terrain and existing road bits.
 * @param tile Real Surface road candidate.
 * @param bits Required physical connections, including entrance/exit geometry.
 * @return Whether the shape already exists or the native command admits buying it.
 */
static bool CanBuildSurfaceRoad(TileIndex tile, RoadBits bits)
{
	if (!IsSurfaceRoadCandidate(tile)) return false;
	if (IsNormalRoadTile(tile)) {
		const auto existing = GetRoadBits(tile, RoadTramType::Road);
		if ((existing | bits) == existing) return true;
	}
	return CmdBuildRoad(CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoad>()),
		tile, bits, ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded();
}

/** Normally buy the missing portion of one exact admitted road shape.
 * @param tile Real Surface tile, freshly revalidated before spending.
 * @param bits Connections required by the selected paid physical route.
 * @return Original actual invoice, or zero for existing complete road bits.
 */
static Money BuySurfaceRoad(TileIndex tile, RoadBits bits)
{
	if (IsNormalRoadTile(tile)) {
		const auto existing = GetRoadBits(tile, RoadTramType::Road);
		if ((existing | bits) == existing) return Money{0};
	}
	REQUIRE(CanBuildSurfaceRoad(tile, bits));
	return PaySurfaceRoadCommand<Commands::BuildRoad>(tile, bits, ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid());
}

/** Actual normally funded loading bay, with its sole physical road entrance. */
struct SurfaceFreightBay {
	TileIndex tile = INVALID_TILE; ///< Native station tile, never a virtual route endpoint.
	TileIndex entrance = INVALID_TILE; ///< Adjacent normal road tile outside the bay.
	DiagDirection direction = DiagDirection::NE; ///< Native bay opening direction.
	StationID station = StationID::Invalid(); ///< Native command-created station identity.
};

/** Find and normally fund one useful bay in the actual factory/raw-source catchment.
 * @param industry Real original raw producer or newly funded output factory.
 * @param invoice Running observation of actual infrastructure spending, never a financial owner.
 * @return Exact native bay and entrance identities.
 */
static SurfaceFreightBay BuySurfaceFreightBay(Industry &industry, Money &invoice)
{
	std::vector<TileIndex> sites;
	for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) sites.push_back(tile);
	/** Rank original candidate sites without changing industry footprint or catchment.
	 * @param tile Native Surface candidate.
	 * @return Manhattan distance and stable native tile tie breaker.
	 */
	const auto rank = [&industry](TileIndex tile) {
		return std::make_pair(std::abs(int(TileX(tile)) - int(TileX(industry.location.tile))) +
			std::abs(int(TileY(tile)) - int(TileY(industry.location.tile))), tile.base());
	};
	std::ranges::sort(sites, {}, rank);
	const auto flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoadStop>());
	// Real bay placement and catchment must both serve this exact industry.
	// Flat junction-capable entrances keep future road/depot access physical;
	// unbuildable original terrain is skipped, never replaced by a fixture slab.
	for (TileIndex tile : sites) {
		if (!IsTileType(tile, TileType::Clear)) continue;
		if (rank(tile).first > 6) break;
		for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			const auto entrance = SurfaceRoadStep(tile, direction);
			if (!CanBuildSurfaceRoad(entrance, ROAD_ALL)) continue;
			if (CmdBuildRoadStop(flags, tile, 1, 1, RoadStopType::Truck, false, direction, ROADTYPE_ROAD,
				ROADSTOP_CLASS_DFLT, 0, NEW_STATION, true).Failed()) continue;
			bool in_catchment = false;
			// Original machinery cells determine catchment. Empty rectangle holes
			// cannot authorize a station that never physically reaches its source.
			for (TileIndex cell : industry.location) {
				if (!industry.TileBelongsToIndustry(cell)) continue;
				if (std::max(std::abs(int(TileX(cell)) - int(TileX(tile))), std::abs(int(TileY(cell)) - int(TileY(tile)))) <= 3) in_catchment = true;
			}
			if (!in_catchment) continue;
			invoice += PaySurfaceRoadCommand<Commands::BuildRoadStop>(tile, uint8_t{1}, uint8_t{1}, RoadStopType::Truck,
				false, direction, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, NEW_STATION, true);
			auto *station = Station::Get(GetStationIndex(tile)); REQUIRE(station->owner == _local_company);
			CHECK(std::ranges::find(industry.stations_near, station) != industry.stations_near.end());
			fmt::print("SURFACE_ENTRY_FREIGHT_BAY industry={} station={} tile={} entrance={} direction={} radius={}\n",
				industry.index.base(), station->index.base(), tile.base(), entrance.base(), to_underlying(direction), station->GetCatchmentRadius());
			return {tile, entrance, direction, station->index};
		}
	}
	FAIL("No unchanged native bay site in actual industry catchment"); return {};
}

/** Find and purchase a physical road path between the actual bay entrances.
 * @param source Native funded source bay.
 * @param target Native funded receiving bay.
 * @param invoice Running observation of real native infrastructure spending.
 * @return Actual native road tiles, useful for selecting a real paid depot siding.
 */
static std::vector<TileIndex> BuySurfaceFreightRoad(const SurfaceFreightBay &source, const SurfaceFreightBay &target, Money &invoice)
{
	const uint32_t states = Map::PlaneSize() * 4;
	std::vector<int32_t> parent(states, -1); std::queue<uint32_t> pending;
	const uint32_t initial = source.entrance.base() * 4 + to_underlying(ReverseDiagDir(source.direction));
	parent[initial] = initial; pending.push(initial); uint32_t found = states;
	// Directional states keep both physical road bits of every turn. A tile-only
	// shortest path can falsely accept slopes that cannot support its real turn.
	while (!pending.empty()) {
		const uint32_t state = pending.front(); pending.pop();
		const auto tile = TileIndex{state / 4}; const auto back = static_cast<DiagDirection>(state % 4);
		if (tile == target.entrance) {
			const auto exit = ReverseDiagDir(target.direction);
			if (CanBuildSurfaceRoad(tile, DiagDirToRoadBits(back) | DiagDirToRoadBits(exit))) { found = state; break; }
		}
		// Only an exact admitted road shape can connect this entry to the next
		// physical neighbour; company command queries retain all terrain rules.
		for (const auto exit : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			if (exit == back) continue;
			const auto next = SurfaceRoadStep(tile, exit);
			if (!IsSurfaceRoadCandidate(next)) continue;
			const uint32_t candidate = next.base() * 4 + to_underlying(ReverseDiagDir(exit));
			if (parent[candidate] != -1) continue;
			if (!CanBuildSurfaceRoad(tile, DiagDirToRoadBits(back) | DiagDirToRoadBits(exit))) continue;
			parent[candidate] = state; pending.push(candidate);
		}
	}
	REQUIRE(found != states);
	std::vector<uint32_t> route;
	// Trace only the pure command-admitted predecessor chain; these local search
	// records do not move vehicles or create a second saved route authority.
	for (uint32_t state = found;; state = parent[state]) { route.push_back(state); if (state == initial) break; }
	std::ranges::reverse(route); std::vector<TileIndex> tiles;
	// Requery each selected shape immediately before payment. Earlier search
	// quotes cannot authorize spending after preceding road cells were built.
	for (size_t index = 0; index < route.size(); ++index) {
		const auto tile = TileIndex{route[index] / 4}; const auto back = static_cast<DiagDirection>(route[index] % 4);
		const auto exit = index + 1 == route.size() ? ReverseDiagDir(target.direction) : ReverseDiagDir(static_cast<DiagDirection>(route[index + 1] % 4));
		invoice += BuySurfaceRoad(tile, DiagDirToRoadBits(back) | DiagDirToRoadBits(exit)); tiles.push_back(tile);
	}
	fmt::print("SURFACE_ENTRY_FREIGHT_ROAD from={} to={} cells={}\n", source.station.base(), target.station.base(), tiles.size());
	return tiles;
}

/** Buy one actual depot with a command-built side connection to the paid road network.
 * @param route Original actual road tiles, not an invented vehicle path.
 * @param invoice Running total of normal infrastructure invoices.
 * @return Exact native depot tile.
 */
static TileIndex BuySurfaceFreightDepot(const std::vector<TileIndex> &route, Money &invoice)
{
	const auto flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoadDepot>());
	// A depot must have both legal original foundations and a real connecting
	// road bit; an adjacent but closed wall is not physical depot access.
	for (TileIndex junction : route) for (const auto side : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
		const auto tile = SurfaceRoadStep(junction, side);
		if (!IsSurfaceRoadCandidate(tile)) continue;
		if (IsNormalRoadTile(tile)) continue;
		const auto entrance = ReverseDiagDir(side);
		if (CmdBuildRoadDepot(flags, tile, ROADTYPE_ROAD, entrance).Failed()) continue;
		const auto bits = GetRoadBits(junction, RoadTramType::Road) | DiagDirToRoadBits(side);
		if (!CanBuildSurfaceRoad(junction, bits)) continue;
		invoice += BuySurfaceRoad(junction, bits);
		invoice += PaySurfaceRoadCommand<Commands::BuildRoadDepot>(tile, ROADTYPE_ROAD, entrance);
		fmt::print("SURFACE_ENTRY_FREIGHT_DEPOT tile={} junction={} direction={}\n", tile.base(), junction.base(), to_underlying(entrance));
		return tile;
	}
	FAIL("No unchanged native depot siding on paid road"); return INVALID_TILE;
}

/** The genuine game acquires connected original freight infrastructure with exact native invoices and no stocks. */
TEST_CASE("Authentic Surface production buys connected conventional freight infrastructure", "[.surface-entry-roads]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-roads");
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve assertion failures through orderly native shutdown without compensating money/terrain writes. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			const auto *company = Company::Get(_current_company); const auto cash = company->money;
			REQUIRE(cash == 754026); REQUIRE(company->current_loan == 2000000); REQUIRE(CargoPacket::GetNumItems() == 0);
			std::array<Industry *, 3> industries{nullptr, nullptr, nullptr};
			const auto iron = GetCargoTypeByLabel(CargoLabel{"IORE"}), steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
			// Resolve only this actual world's natural iron source and its two
			// company-funded factories; generated output predecessors are excluded.
			for (auto *industry : Industry::Iterate()) {
				if (industry->accepted.empty()) for (const auto &output : industry->produced) if (output.cargo == iron) industries[0] = industry;
				if (industry->founder != _local_company) continue;
				for (const auto &output : industry->produced) {
					if (output.cargo == steel) industries[1] = industry;
					if (output.cargo == goods) industries[2] = industry;
				}
			}
			for (const auto *industry : industries) { REQUIRE(industry != nullptr); REQUIRE(IsLayerIndustryComplete(*industry)); }
			SaveSurfaceRoads(paths.saves / "surface-entry-before-roads-440.sav"); Money invoice{0};
			std::array<SurfaceFreightBay, 3> bays;
			for (size_t index = 0; index < industries.size(); ++index) bays[index] = BuySurfaceFreightBay(*industries[index], invoice);
			const auto first = BuySurfaceFreightRoad(bays[0], bays[1], invoice);
			const auto second = BuySurfaceFreightRoad(bays[1], bays[2], invoice);
			const auto depot = BuySurfaceFreightDepot(first, invoice); REQUIRE(depot != INVALID_TILE);
			CHECK(company->money == cash - invoice); CHECK(company->current_loan == 2000000);
			CHECK(Industry::GetNumItems() == 12); CHECK(CargoPacket::GetNumItems() == 0);
			SaveSurfaceRoads(paths.saves / "surface-entry-roads-440.sav");
			fmt::print("SURFACE_ENTRY_ROADS_COMPLETE expense={} cash={} debt={} depot={}\n", invoice.base(), company->money.base(), company->current_loan.base(), depot.base()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_roads_observer, &inspect);
	IConsole::CmdRegister("surface_entry_roads", ObserveSurfaceRoads);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
