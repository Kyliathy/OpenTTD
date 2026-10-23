/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_layout_fixture.h Paid modal layout on the actual normal-world irregular floor.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6DCDB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_NORMAL_GROUND_LAYOUT_FIXTURE_H
#define TESTS_NORMAL_GROUND_LAYOUT_FIXTURE_H

#include "normal_ground_irregular_fixture.h"
#include "../transport_tube.h"
#include "../transport_tube_cmd.h"

/**
 * Pay native track owners for a proposed physical route on already earned floor.
 * @param path Consecutive explicit world cells; actual stations and endpoint depots already exist.
 * @param railtype Native conventional/tube rail identity.
 */
inline void BuildNormalGroundModalPath(const std::vector<TileIndex> &path, RailType railtype)
{
	REQUIRE(path.size() >= 3);
	for (size_t index = 1; index + 1 < path.size(); ++index) {
		const auto tile = path[index];
		REQUIRE(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
		if (IsRailStationTile(tile)) { CHECK(GetRailType(tile) == railtype); continue; }
		REQUIRE(DistanceManhattan(tile, path[index - 1]) == 1);
		REQUIRE(DistanceManhattan(tile, path[index + 1]) == 1);
		const auto entry = DiagdirBetweenTiles(tile, path[index - 1]), exit = DiagdirBetweenTiles(tile, path[index + 1]);
		Track piece = Track::Invalid;
		for (const auto directed : DiagdirReachesTrackdirs(ReverseDiagDir(entry))) {
			if (TrackdirToExitdir(directed) == exit) piece = TrackdirToTrack(directed);
		}
		REQUIRE(piece != Track::Invalid);
		if (IsPlainRailTile(tile)) {
			if (GetRailType(tile) == railtype && GetTrackBits(tile) == TrackBits{piece}) continue;
			for (const auto old : GetTrackBits(tile)) PayNormalFreightCommand<Commands::RemoveRail>("modal_remove_old_piece", tile, old);
		}
		PayNormalFreightCommand<Commands::BuildRail>("modal_track", tile, railtype, piece, false);
	}
}

/** Release settled work and construct both real same-layer modal interchanges without advancing the game. */
inline void BuildNormalGroundModalLayout()
{
	REQUIRE(_game_mode == GameMode::Normal); REQUIRE_FALSE(_networking);
	REQUIRE(Company::IsValidID(_local_company)); AutoRestoreBackup actor{_current_company, _local_company};
	const auto *company = Company::Get(_current_company); const auto opening = company->money;
	const auto tick = TimerGameTick::counter;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto floor = CountNormalGroundRockStates(); CHECK(floor[1] == 0); CHECK(floor[2] == 112); CHECK(floor[3] == 0);
	const auto projects = CaptureTBMProjects(); REQUIRE(projects.size() == 1); const auto project = projects.front();
	REQUIRE(project.model_id == 0); REQUIRE(project.id == TBMProjectID{2});
	REQUIRE(GetTBMProjectReleaseProblem(project.id) == INVALID_STRING_ID);
	const auto assignments = CaptureTBMExchangeAssignments(); REQUIRE(assignments.size() == 1);
	const auto hauler_id = assignments.front().hauler;
	const auto fleet_count = Vehicle::GetNumItems();
	const bool paused = _pause_mode.Test(PauseMode::Normal);
	if (paused) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
	REQUIRE(_pause_mode.None());
	PayNormalFreightCommand<Commands::ControlTBMProject>("retire_settled_irregular", project.id, TBMProjectAction::Retire);
	REQUIRE(CaptureTBMProjects().empty()); REQUIRE(CaptureTBMExchangeAssignments().empty()); REQUIRE(CaptureTBMWagonSets().empty());
	auto *hauler = Train::Get(hauler_id);
	if (!hauler->vehstatus.Test(VehState::Stopped)) PayNormalFreightCommand<Commands::StartStopVehicle>("stop_released_irregular_hauler", hauler->tile, hauler_id, true);
	CHECK(Train::Get(project.cutter)->vehstatus.Test(VehState::Stopped));
	const auto supports = CaptureTBMSupport(); REQUIRE_FALSE(supports.empty());
	for (const auto &support : supports) {
		REQUIRE_FALSE(IsTBMSupportInUse(support.tile));
		for (const auto track : GetTrackBits(support.tile)) PayNormalFreightCommand<Commands::RemoveRail>("modal_remove_support", support.tile, track);
	}
	REQUIRE(CaptureTBMSupport().empty());
	const auto at = [](uint x, uint y) { return TileXYL(x, y, MapLayer::Underground); };
	const auto tube = GetTubeRailType(); REQUIRE(tube != INVALID_RAILTYPE);
	REQUIRE(company->avail_railtypes.Test(tube)); // Normal native availability,never a test-owned override.
	const std::array tube_depots{at(96, 66), at(103, 69)};
	const auto freight_depot = at(103, 70);
	PayNormalFreightCommand<Commands::BuildRailDepot>("modal_tube_home", tube_depots[0], tube, DiagDirection::SE);
	PayNormalFreightCommand<Commands::BuildRailDepot>("modal_tube_far", tube_depots[1], tube, DiagDirection::NE);
	PayNormalFreightCommand<Commands::BuildRailDepot>("modal_freight_far", freight_depot, RAILTYPE_RAIL, DiagDirection::NE);
	PayNormalFreightCommand<Commands::BuildRailStation>("modal_tube_origin", at(96, 68), tube, Axis::Y, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	const auto origin = GetStationIndex(at(96, 68));
	PayNormalFreightCommand<Commands::BuildRailStation>("modal_tube_transfer", at(101, 69), tube, Axis::X, uint8_t{1}, uint8_t{2}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	const auto destination = GetStationIndex(at(101, 69)); REQUIRE(destination != origin);
	// The commissioned portal requires a real plain-rail door at95,68.
	// One native hauler/resource wagon fits the next one-tile platform;the
	// diagonal adjacent tube platform supplies the same actual station owner.
	PayNormalFreightCommand<Commands::BuildRailStation>("modal_rail_origin", at(95, 69), RAILTYPE_RAIL, Axis::Y, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	PayNormalFreightCommand<Commands::BuildRailStation>("modal_rail_transfer", at(101, 70), RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{2}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	REQUIRE(GetStationIndex(at(95, 69)) == origin); REQUIRE(GetStationIndex(at(101, 70)) == destination);
	std::vector<TileIndex> tube_path, freight_path;
	for (uint y = 66; y <= 69; ++y) tube_path.push_back(at(96, y));
	for (uint x = 97; x <= 103; ++x) tube_path.push_back(at(x, 69));
	for (uint y = 67; y <= 70; ++y) freight_path.push_back(at(95, y));
	for (uint x = 96; x <= 103; ++x) freight_path.push_back(at(x, 70));
	BuildNormalGroundModalPath(tube_path, tube); BuildNormalGroundModalPath(freight_path, RAILTYPE_RAIL);
	PayNormalFreightCommand<Commands::ChangeLayerPortalConnection>("modal_ordinary_connection", project.portal, true);
	CHECK(GetLayerPortalWorkState(project.portal) == LayerPortalWorkState::Connected);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(CountNormalGroundRockStates() == floor);
	CHECK(TimerGameTick::counter == tick); CHECK(Vehicle::GetNumItems() == fleet_count);
	CHECK(company->current_loan == Money{300000}); CHECK(_settings_game.pf.forbid_90_deg);
	if (paused) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
	const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-layout");
	for (const auto name : {"modal-layout.sav", "modal-layout-repeat.sav"})
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", name)), false) == SaveLoadResult::Ok);
	fmt::print("NORMAL_GROUND_LAYOUT portal={} target={} cutter={} hauler={} origin={} destination={} tube_home={} tube_far={} freight_far={} supports={} fleet={} opening_cash={} cash={} loan={} tick={} excavated={} tube={}\n",
		project.portal.base(), GetLayerPortalPairTile(project.portal).base(), project.cutter.base(), hauler_id.base(), origin.base(), destination.base(), tube_depots[0].base(), tube_depots[1].base(), freight_depot.base(), supports.size(), fleet_count,
		opening.base(), company->money.base(), company->current_loan.base(), tick, floor[2], to_underlying(tube));
	for (const auto &path : {tube_path, freight_path}) for (const auto tile : path) fmt::print("NORMAL_GROUND_LAYOUT_CELL tile={} rail={} station={} depot={}\n",
		tile.base(), to_underlying(GetRailType(tile)), IsRailStationTile(tile) ? GetStationIndex(tile).base() : StationID::Invalid().base(), IsRailDepotTile(tile) ? 1 : 0);
	std::fflush(stdout);
}

/** Load the exact original and let native restored link-graph work finish before ordinary player construction. */
inline void RunNormalGroundModalLayout()
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
	bool requested = false, observed = false; std::exception_ptr failure;
	std::function<void()> prepare = [&]() { requested = true; };
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &prepare};
	IConsole::CmdRegister("normal_ground_layout", ObserveNormalGroundNewgame);
	const IntervalTimer<TimerGameRealtime> ready({std::chrono::milliseconds(1), TimerGameRealtime::Trigger::Always}, [&](auto) {
		if (!requested || observed || failure) return;
		auto pending = _pause_mode; pending.Reset(PauseMode::Normal); pending.Reset(PauseMode::CommandDuringPause);
		if (pending.Any()) return;
		try { BuildNormalGroundModalLayout(); observed = true; } catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	});
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(), "-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null", "-I", "original_dos", "-g", input};
	std::vector<std::string_view> arguments; for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); REQUIRE(observed);
}

#endif /* TESTS_NORMAL_GROUND_LAYOUT_FIXTURE_H */
