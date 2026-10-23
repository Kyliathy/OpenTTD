/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_irregular_fixture.h Public original-world irregular proposal and actual machinery reuse.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6DCB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_NORMAL_GROUND_IRREGULAR_FIXTURE_H
#define TESTS_NORMAL_GROUND_IRREGULAR_FIXTURE_H

#include "normal_ground_deep_fixture.h"
#include "../settings_cmd.h"

/** Pay for the missing piece named by the canonical preview,using its own public edge path. */
inline bool FundNormalGroundPreviewTrack(const TBMCommissionAreaPreview &quote)
{
	const auto tile = quote.area.failure_tile;
	REQUIRE(IsValidTile(tile)); REQUIRE(IsPlainRailTile(tile)); REQUIRE(GetTileOwner(tile) == _current_company);
	REQUIRE(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
	std::vector<size_t> ends;
	const auto path = GetTBMWorkRailPath(quote.area.proposal, quote.area.proposal.route.size(), ends);
	REQUIRE(ends.size() == quote.area.proposal.route.size());
	bool built = false;
	for (size_t index = 1; index < path.size(); ++index) if (path[index] == tile) {
		const auto entry = DiagdirBetweenTiles(tile, path[index - 1]);
		const auto exit = index + 1 < path.size() ? DiagdirBetweenTiles(tile, path[index + 1]) : ReverseDiagDir(entry);
		REQUIRE(entry != DiagDirection::Invalid); REQUIRE(exit != DiagDirection::Invalid);
		// Interpret the native edge path with the ordinary trackdir table;the
		// canonical planner above remains the sole owner of every path cell.
		for (const auto direction : DiagdirReachesTrackdirs(ReverseDiagDir(entry))) if (TrackdirToExitdir(direction) == exit) {
			const auto piece = TrackdirToTrack(direction);
			if (!GetTrackBits(tile).Test(piece)) {
				PayNormalFreightCommand<Commands::BuildRail>("irregular_support_piece", tile, GetRailType(tile), piece, false); built = true;
			}
			break;
		}
	}
	return built;
}

/** Prepare actual irregular work solely through supported quotes/commands on the exact normally earned original. */
inline void PrepareNormalGroundIrregular()
{
	REQUIRE(_game_mode == GameMode::Normal); REQUIRE_FALSE(_networking);
	REQUIRE(Company::IsValidID(_local_company)); AutoRestoreBackup actor{_current_company, _local_company};
	const auto *company = Company::Get(_current_company); const auto opening = company->money;
	const auto old_rock = CountNormalGroundRockStates(); CHECK(old_rock[2] == 72); CHECK(old_rock[1] == 0);
	const auto projects = CaptureTBMProjects(); REQUIRE(projects.size() == 1); const auto old_deep = projects.front();
	REQUIRE(old_deep.model_id == 1); REQUIRE(GetTBMProjectReleaseProblem(old_deep.id) == INVALID_STRING_ID);
	const bool paused = _pause_mode.Test(PauseMode::Normal);
	if (paused) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
	REQUIRE(_pause_mode.None());
	PayNormalFreightCommand<Commands::ControlTBMProject>("retire_settled_deep", old_deep.id, TBMProjectAction::Retire);
	REQUIRE(_pause_mode.None());
	const auto portal = TileXYL(95, 67, MapLayer::Surface);
	REQUIRE(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Connected);
	PayNormalFreightCommand<Commands::ChangeLayerPortalConnection>("reseal_standard_source", portal, false);
	const auto origin = GetStationIndex(TileXYL(95, 61, MapLayer::Surface));
	const auto depot = TileXYL(95, 57, MapLayer::Surface);
	const std::array targets{
		std::pair{TileXYL(101, 74, MapLayer::Underground), TileXYL(107, 78, MapLayer::Underground)},
		std::pair{TileXYL(98, 71, MapLayer::Underground), TileXYL(102, 73, MapLayer::Underground)},
		std::pair{TileXYL(97, 70, MapLayer::Underground), TileXYL(99, 72, MapLayer::Underground)},
	};
	size_t target = 0; auto [first, last] = targets[target];
	const auto consist = GetDefaultTBMConsist(MapLayer::Underground);
	const bool original_forbid = _settings_game.pf.forbid_90_deg;
	TBMCommissionAreaPreview quote;
	uint attempts = 0;
	for (; attempts < 12; ++attempts) {
		const auto before = std::tuple{company->money, TimerGameTick::counter, _random.state[0], _random.state[1],
			ComputeGeologyStateDigest(GetGeologyWorldState()), Vehicle::GetNumItems()};
		quote = PreviewTBMCommissionArea(_current_company, portal, uint16_t{0}, consist.earth_wagons, consist.rock_wagons, origin, first, last, false);
		CHECK((std::tuple{company->money, TimerGameTick::counter, _random.state[0], _random.state[1],
			ComputeGeologyStateDigest(GetGeologyWorldState()), Vehicle::GetNumItems()} == before));
		fmt::print("NORMAL_GROUND_IRREGULAR_QUOTE attempt={} first={} last={} ok={} error={} failure={} purchase={} earth={} rock={} support={} forbid90={}\n",
			attempts, first.base(), last.base(), quote.purchase.Succeeded() ? 1 : 0, quote.purchase.GetErrorMessage().base(), quote.area.failure_tile.base(),
			quote.purchase.GetCost().base(), quote.area.totals.earth_t, quote.area.totals.rock_t, quote.area.totals.support_units, _settings_game.pf.forbid_90_deg ? 1 : 0);
		std::fflush(stdout);
		if (quote.purchase.Succeeded()) break;
		if (quote.purchase.GetErrorMessage() == STR_ERROR_TBM_COMMISSION_RAIL_TURNS && _settings_game.pf.forbid_90_deg) {
			PayNormalFreightCommand<Commands::ChangeSetting>("allow_native_rail_turns", std::string{"pf.forbid_90_deg"}, int32_t{0});
		} else if (quote.purchase.GetErrorMessage() == STR_ERROR_TBM_COMMISSION_PORTAL) {
			// Ordinary native tree growth can reclaim the Surface assembly rim
			// during the previous months of real work. Pay its exact maintenance.
			bool cleared = false;
			for (const auto cell : GetExcavationCutterEnvelope(portal)) if (IsTileType(cell, TileType::Trees)) {
				PayNormalFreightCommand<Commands::LandscapeClear>("irregular_source_clear", cell); cleared = true;
			}
			REQUIRE(cleared);
		} else if (quote.purchase.GetErrorMessage() == STR_ERROR_TBM_COMMISSION_SUPPORT_PIECE) {
			REQUIRE(FundNormalGroundPreviewTrack(quote));
		} else if (quote.purchase.GetErrorMessage() == STR_ERROR_TBM_APPROACH_TOO_FAR && target + 1 < targets.size()) {
			// A bounded native search refusal is not permission to enlarge its
			// budget or fabricate a route. Review a nearer off-axis room instead.
			std::tie(first, last) = targets[++target];
		} else {
			FAIL("The canonical irregular proposal needs a separately characterized native prerequisite");
		}
	}
	REQUIRE(quote.purchase.Succeeded()); REQUIRE(quote.purchase.GetCost() == Money{0});
	REQUIRE(quote.request.reuse_cutter != VehicleID::Invalid());
	uint bends = 0;
	for (size_t index = 1; index < quote.request.proposal.route.size(); ++index) {
		const auto before = quote.request.proposal.route[index - 1].heading, after = quote.request.proposal.route[index].heading;
		if (before == after) continue;
		CHECK((ChangeDir(before, DirDiff::Left45) == after || ChangeDir(before, DirDiff::Right45) == after)); ++bends;
	}
	REQUIRE(bends > 0);
	PayNormalFreightCommand<Commands::CommissionTBM>("reuse_standard", portal, uint16_t{0}, consist.earth_wagons, consist.rock_wagons, origin, first, last, false);
	const auto current = CaptureTBMProjects(); REQUIRE(current.size() == 1); const auto &project = current.front();
	CHECK(project.cutter == quote.request.reuse_cutter);
	Train *hauler = nullptr;
	for (auto *front : Train::Iterate()) if (front->IsFrontEngine() && front->owner == _current_company && front->engine_type == GetUndergroundEngine(4) && front->IsStoppedInDepot() && front->tile == depot) {
		REQUIRE(hauler == nullptr); hauler = front;
	}
	REQUIRE(hauler != nullptr);
	PayNormalFreightCommand<Commands::AssignTBMHauler>("irregular_assign", project.id, hauler->index);
	PayNormalFreightCommand<Commands::ControlTBMProject>("pause_irregular", project.id, TBMProjectAction::Pause);
	const auto after_rock = CountNormalGroundRockStates(); CHECK(after_rock[2] == old_rock[2]); CHECK(after_rock[3] == 0);
	CHECK(after_rock[0] + after_rock[1] == old_rock[0]); CHECK(company->current_loan == Money{300000});
	if (paused) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
	const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-irregular");
	for (const auto name : {"irregular-ready.sav", "irregular-ready-repeat.sav"})
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", name)), false) == SaveLoadResult::Ok);
	fmt::print("NORMAL_GROUND_IRREGULAR project={} job={} cutter={} hauler={} portal={} origin={} depot={} first={} last={} bends={} attempts={} original_forbid90={} forbid90={} opening_cash={} cash={} loan={} tick={} solid={} excavating={} excavated={}\n",
		project.id.base(), project.job.base(), project.cutter.base(), hauler->index.base(), portal.base(), origin.base(), depot.base(), first.base(), last.base(), bends, attempts + 1,
		original_forbid ? 1 : 0, _settings_game.pf.forbid_90_deg ? 1 : 0, opening.base(), company->money.base(), company->current_loan.base(), TimerGameTick::counter, after_rock[0], after_rock[1], after_rock[2]);
	std::fflush(stdout);
}

/** Load the exact actually settled normal original through the native application. */
inline void RunNormalGroundIrregular()
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
	bool requested = false, observed = false, waiting_logged = false; std::exception_ptr failure;
	std::function<void()> prepare = [&]() { requested = true; };
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &prepare};
	IConsole::CmdRegister("normal_ground_irregular", ObserveNormalGroundNewgame);
	// AfterLoad_LinkGraphPauseControl can hold this exact saved date until a
	// native background job finishes. StateGameLoop clears that pause itself;
	// the saved Normal pause keeps simulation fixed while we wait for it.
	const IntervalTimer<TimerGameRealtime> ready({std::chrono::milliseconds(1), TimerGameRealtime::Trigger::Always}, [&](auto) {
		if (!requested || observed || failure) return;
		auto pending = _pause_mode; pending.Reset(PauseMode::Normal); pending.Reset(PauseMode::CommandDuringPause);
		if (pending.Any()) {
			if (!waiting_logged) { fmt::print("NORMAL_GROUND_IRREGULAR_LOAD_WAIT linkgraph={} tick={}\n", _pause_mode.Test(PauseMode::LinkGraph) ? 1 : 0, TimerGameTick::counter); std::fflush(stdout); waiting_logged = true; }
			return;
		}
		try { PrepareNormalGroundIrregular(); observed = true; } catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	});
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(),
		"-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null", "-I", "original_dos", "-g", input};
	std::vector<std::string_view> arguments; for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); REQUIRE(observed);
}

#endif /* TESTS_NORMAL_GROUND_IRREGULAR_FIXTURE_H */
