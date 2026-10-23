/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_deep_fixture.h Actual Deep commissioning on Standard-earned accessible normal-world floor.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6CDD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_NORMAL_GROUND_DEEP_FIXTURE_H
#define TESTS_NORMAL_GROUND_DEEP_FIXTURE_H

#include "normal_ground_access_fixture.h"

/** Buy actual Deep readiness;all Underground assembly already exists from the exact original paid Standard job. */
inline void AcquireNormalGroundDeep()
{
	REQUIRE(_game_mode == GameMode::Normal); REQUIRE_FALSE(_networking);
	REQUIRE(Company::IsValidID(_local_company)); AutoRestoreBackup actor{_current_company, _local_company};
	const auto *company = Company::Get(_current_company); const auto opening = company->money;
	REQUIRE(CaptureTBMProjects().empty()); REQUIRE(CaptureTBMExchangeAssignments().empty());
	std::set<VehicleID> original_vehicles; for (const auto *vehicle : Train::Iterate()) original_vehicles.insert(vehicle->index);
	const auto before_rock = CountNormalGroundRockStates(); CHECK(before_rock[1] == 0); CHECK(before_rock[2] == 36);
	const auto access = TileXYL(95, 67, MapLayer::Surface);
	REQUIRE(GetLayerPortalWorkState(access) == LayerPortalWorkState::Connected);
	const auto origin = GetStationIndex(TileXYL(95, 61, MapLayer::Surface));
	const auto depot = TileXYL(95, 57, MapLayer::Surface);
	const auto portal = TileXYL(95, 73, MapLayer::Underground);
	const auto first = TileXYL(94, 75, MapLayer::DeepUnderground);
	const auto last = TileXYL(96, 83, MapLayer::DeepUnderground);
	const bool paused = _pause_mode.Test(PauseMode::Normal);
	if (paused) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
	// Native adjacent-station joining keeps the exact original acceptance owner.
	PayNormalFreightCommand<Commands::BuildRailStation>("deep_platform", TileXYL(95, 64, MapLayer::Surface), RAILTYPE_RAIL,
		Axis::Y, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	REQUIRE(GetStationIndex(TileXYL(95, 64, MapLayer::Surface)) == origin);
	REQUIRE(Station::Get(origin)->train_station.w * Station::Get(origin)->train_station.h >= 4);
	REQUIRE(GetGeologyWorldState().terrain.Get(portal) == ExcavationState::Excavated);
	// Existing ordinary track is removed through its own paid command;the
	// portal builder never silently replaces mid-route support or opens rock.
	REQUIRE(IsPlainRailTile(portal)); const auto tracks = GetTrackBits(portal);
	for (const auto track : tracks) PayNormalFreightCommand<Commands::RemoveRail>("deep_source_rail", portal, track);
	PayNormalFreightCommand<Commands::BuildLayerPortal>("deep_portal", portal, TransportType::Rail,
		MapLayer::DeepUnderground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD);
	const auto consist = GetDefaultTBMConsist(MapLayer::DeepUnderground);
	CHECK(consist.earth_wagons == 2); CHECK(consist.rock_wagons == 4);
	const auto engine = GetTBMEngine(1); REQUIRE(engine != EngineID::Invalid());
	REQUIRE(IsEngineBuildable(engine, VehicleType::Train, _current_company));
	const auto before_quote = std::tuple{company->money, company->current_loan, TimerGameTick::counter,
		_random.state[0], _random.state[1], ComputeGeologyStateDigest(GetGeologyWorldState()), Vehicle::GetNumItems()};
	const auto quote = PreviewTBMCommissionArea(_current_company, portal, uint16_t{1}, consist.earth_wagons, consist.rock_wagons,
		origin, first, last, false);
	fmt::print("NORMAL_GROUND_DEEP_QUOTE portal={} first={} last={} ok={} error={} purchase={} earth={} rock={} support={}\n",
		portal.base(), first.base(), last.base(), quote.purchase.Succeeded() ? 1 : 0, quote.purchase.GetErrorMessage().base(), quote.purchase.GetCost().base(),
		quote.area.totals.earth_t, quote.area.totals.rock_t, quote.area.totals.support_units); std::fflush(stdout);
	REQUIRE(quote.purchase.Succeeded());
	CHECK((std::tuple{company->money, company->current_loan, TimerGameTick::counter,
		_random.state[0], _random.state[1], ComputeGeologyStateDigest(GetGeologyWorldState()), Vehicle::GetNumItems()} == before_quote));
	auto *hauler = BuyNormalGroundUnit(depot, GetUndergroundEngine(4)); REQUIRE(hauler->IsFrontEngine());
	for (const auto [model, count] : {std::pair<uint16_t, uint8_t>{2, consist.earth_wagons}, {3, consist.rock_wagons}}) {
		for (uint index = 0; index < count; ++index) {
			auto *wagon = BuyNormalGroundUnit(depot, GetUndergroundEngine(model));
			if (wagon->First() != hauler) PayNormalFreightCommand<Commands::MoveRailVehicle>("deep_couple", depot, wagon->index, hauler->Last()->index, false);
		}
	}
	Order order; order.MakeGoToStation(origin);
	PayNormalFreightCommand<Commands::InsertOrder>("deep_origin_order", depot, hauler->index, VehicleOrderID{0}, order);
	PayNormalFreightCommand<Commands::CommissionTBM>("deep_commission", portal, uint16_t{1}, consist.earth_wagons, consist.rock_wagons,
		origin, first, last, false);
	const auto projects = CaptureTBMProjects(); REQUIRE(projects.size() == 1); const auto &project = projects.front();
	CHECK(project.portal == portal); CHECK(project.origin == origin); CHECK(project.model_id == 1);
	PayNormalFreightCommand<Commands::AssignTBMHauler>("deep_assign", project.id, hauler->index);
	PayNormalFreightCommand<Commands::ControlTBMProject>("pause_deep", project.id, TBMProjectAction::Pause);
	CHECK(GetTBMProject(project.id)->blocked == TBMBlockedReason::Paused);
	const auto *job = GetExcavationJob(project.job); REQUIRE(job != nullptr);
	CHECK(job->cursor == 0); CHECK(job->completed_earth_t == 0); CHECK(job->completed_rock_t == 0); CHECK(job->completed_work_cost == Money{0});
	for (const auto id : original_vehicles) CHECK(Train::IsValidID(id));
	const auto after_rock = CountNormalGroundRockStates();
	CHECK(after_rock[0] + after_rock[1] == before_rock[0]); CHECK(after_rock[2] == before_rock[2]); CHECK(after_rock[3] == 0);
	CHECK(after_rock[1] == GetGeologyWorldState().job_claims.size()); CHECK(company->current_loan == Money{300000});
	if (paused) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
	const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-deep");
	for (const auto name : {"deep-ready.sav", "deep-ready-repeat.sav"})
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", name)), false) == SaveLoadResult::Ok);
	fmt::print("NORMAL_GROUND_DEEP project={} job={} cutter={} hauler={} portal={} target={} access={} origin={} depot={} first={} last={} opening_cash={} cash={} loan={} tick={} original_solid={} solid={} excavating={} excavated={} reserved={} earth_wagons={} rock_wagons={}\n",
		project.id.base(), project.job.base(), project.cutter.base(), hauler->index.base(), portal.base(), GetLayerPortalPairTile(portal).base(), access.base(), origin.base(), depot.base(), first.base(), last.base(),
		opening.base(), company->money.base(), company->current_loan.base(), TimerGameTick::counter, before_rock[0], after_rock[0], after_rock[1], after_rock[2], after_rock[3], consist.earth_wagons, consist.rock_wagons);
	std::fflush(stdout);
}

/** Load the actual connected normal original and acquire Deep participants through public commands. */
inline void RunNormalGroundDeep()
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
	bool observed = false; std::exception_ptr failure;
	std::function<void()> acquire = [&]() {
		try { AcquireNormalGroundDeep(); observed = true; } catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &acquire};
	IConsole::CmdRegister("normal_ground_deep", ObserveNormalGroundNewgame);
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(),
		"-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null",
		"-I", "original_dos", "-d", "misc=4,sl=1", "-g", input};
	std::vector<std::string_view> arguments; for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	REQUIRE(observed);
}

#endif /* TESTS_NORMAL_GROUND_DEEP_FIXTURE_H */
