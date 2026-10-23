/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_machinery_fixture.h Actual native hauler and portal-only cutter acquisition on normally paid originals.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6CDB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_NORMAL_GROUND_MACHINERY_FIXTURE_H
#define TESTS_NORMAL_GROUND_MACHINERY_FIXTURE_H

#include "normal_ground_yard_fixture.h"
#include "../tbm_area_preview.h"
#include "../tbm_cmd.h"
#include "../tbm_exchange.h"
#include "../underground_vehicle.h"

/** @param depot Actual owned native depot. @param engine Actual available definition. @return Newly bought complete unit head. */
inline Train *BuyNormalGroundUnit(TileIndex depot, EngineID engine)
{
	std::set<VehicleID> prior; for (const auto *vehicle : Train::Iterate()) prior.insert(vehicle->index);
	PayNormalFreightCommand<Commands::BuildVehicle>("machinery_unit", depot, engine, false, INVALID_CARGO, ClientID::Invalid);
	Train *result = nullptr;
	for (auto *vehicle : Train::Iterate()) if (!prior.contains(vehicle->index) && !vehicle->IsArticulatedPart()) {
		REQUIRE(result == nullptr); result = vehicle;
	}
	REQUIRE(result != nullptr); return result;
}

/** @return Actual inner rock state counts;observational only,never a construction authority. */
inline std::array<size_t, 4> CountNormalGroundRockStates()
{
	std::array<size_t, 4> result{};
	for (const auto layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		for (const auto tile : MultiLayerMap::IterateLayer(layer)) if (IsInnerTile(tile))
			++result[to_underlying(GetGeologyWorldState().terrain.Get(tile).value())];
	}
	return result;
}

/** Buy native participants and publish the real paused Standard project;no simulation/cut or fixture stock is substituted. */
inline void AcquireNormalGroundMachinery()
{
	REQUIRE(_game_mode == GameMode::Normal); REQUIRE_FALSE(_networking);
	REQUIRE(Company::IsValidID(_local_company)); AutoRestoreBackup actor{_current_company, _local_company};
	const auto *company = Company::Get(_current_company); const auto opening = company->money;
	const bool paused = _pause_mode.Test(PauseMode::Normal);
	if (paused) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
	REQUIRE(_pause_mode.None());
	const char *site_text = std::getenv("OTTD_NORMAL_GROUND_ENTRANCE"); REQUIRE(site_text != nullptr);
	const std::string_view argument{site_text}; uint32_t value = 0;
	const auto parsed = std::from_chars(argument.data(), argument.data() + argument.size(), value);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == argument.data() + argument.size());
	const auto portal = TileIndex{value}; REQUIRE(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Sealed);
	const uint x = TileX(portal), y = TilePlaneY(portal);
	const auto depot = TileXYL(x, y - 10, MapLayer::Surface);
	const auto origin = GetStationIndex(TileXYL(x, y - 6, MapLayer::Surface));
	const auto first = TileXYL(x - 1, y + 2, MapLayer::Underground);
	const auto last = TileXYL(x + 1, y + 10, MapLayer::Underground);
	const auto consist = GetDefaultTBMConsist(MapLayer::Underground);
	// The portal command only cleared its exact source/door. The canonical
	// commissioning preflight also needs the complete accessible 3x3 source
	// assembly footprint. Pay for each actual Surface tree before quoting it.
	for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
		const auto cell = TileAddWrap(portal, dx, dy);
		if (IsTileType(cell, TileType::Trees)) PayNormalFreightCommand<Commands::LandscapeClear>("assembly_clear", cell);
	}
	for (const uint16_t model : {uint16_t{4}, uint16_t{2}, uint16_t{3}}) {
		const auto engine = GetUndergroundEngine(model); REQUIRE(engine != EngineID::Invalid());
		const bool available = IsEngineBuildable(engine, VehicleType::Train, _current_company);
		fmt::print("NORMAL_GROUND_ENGINE model={} engine={} available={} cost={}\n", model, engine.base(), available ? 1 : 0, Engine::Get(engine)->GetCost().base());
		REQUIRE(available);
	}
	REQUIRE(CaptureTBMProjects().empty());
	const auto before_rock = CountNormalGroundRockStates();
	CHECK(before_rock[1] == 0); CHECK(before_rock[2] == 0); CHECK(before_rock[3] == 0);
	const auto before_quote = std::tuple{company->money, company->current_loan, TimerGameTick::counter,
		_random.state[0], _random.state[1], ComputeGeologyStateDigest(GetGeologyWorldState()), Vehicle::GetNumItems()};
	const auto quote = PreviewTBMCommissionArea(_current_company, portal, uint16_t{0}, consist.earth_wagons, consist.rock_wagons,
		origin, first, last, false);
	fmt::print("NORMAL_GROUND_COMMISSION_QUOTE first={} last={} ok={} error={} purchase={} earth={} rock={} support={}\n", first.base(), last.base(),
		quote.purchase.Succeeded() ? 1 : 0, quote.purchase.GetErrorMessage().base(), quote.purchase.GetCost().base(),
		quote.area.totals.earth_t, quote.area.totals.rock_t, quote.area.totals.support_units);
	std::fflush(stdout);
	REQUIRE(quote.purchase.Succeeded());
	CHECK((std::tuple{company->money, company->current_loan, TimerGameTick::counter,
		_random.state[0], _random.state[1], ComputeGeologyStateDigest(GetGeologyWorldState()), Vehicle::GetNumItems()} == before_quote));
	auto *hauler = BuyNormalGroundUnit(depot, GetUndergroundEngine(4)); REQUIRE(hauler->IsFrontEngine());
	for (const auto [model, count] : {std::pair<uint16_t, uint8_t>{2, consist.earth_wagons}, {3, consist.rock_wagons}}) {
		for (uint index = 0; index < count; ++index) {
			auto *wagon = BuyNormalGroundUnit(depot, GetUndergroundEngine(model));
			if (wagon->First() != hauler) PayNormalFreightCommand<Commands::MoveRailVehicle>("machinery_couple", depot, wagon->index, hauler->Last()->index, false);
		}
	}
	Order order; order.MakeGoToStation(origin);
	PayNormalFreightCommand<Commands::InsertOrder>("spoil_origin_order", depot, hauler->index, VehicleOrderID{0}, order);
	PayNormalFreightCommand<Commands::CommissionTBM>("standard_commission", portal, uint16_t{0}, consist.earth_wagons, consist.rock_wagons,
		origin, first, last, false);
	const auto projects = CaptureTBMProjects(); REQUIRE(projects.size() == 1); const auto &project = projects.front();
	CHECK(project.portal == portal); CHECK(project.origin == origin); CHECK(project.model_id == 0);
	REQUIRE(Train::IsValidID(project.cutter));
	PayNormalFreightCommand<Commands::AssignTBMHauler>("assign_hauler", project.id, hauler->index);
	REQUIRE(GetTBMExchangeAssignmentForHauler(hauler->index) != nullptr);
	PayNormalFreightCommand<Commands::ControlTBMProject>("pause_standard", project.id, TBMProjectAction::Pause);
	CHECK(GetTBMProject(project.id)->blocked == TBMBlockedReason::Paused);
	CHECK(GetTBMWorkState(project.id).pass_cursor == 0);
	CHECK(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Commissioning);
	const auto after_rock = CountNormalGroundRockStates();
	// ReserveExcavationJob marks its uncut claimed solids Excavating. Only
	// committed cuts become Excavated; the fourth terrain encoding is invalid.
	CHECK(after_rock[0] + after_rock[1] == before_rock[0]);
	CHECK(after_rock[2] == 0); CHECK(after_rock[3] == 0);
	CHECK(after_rock[1] == GetGeologyWorldState().job_claims.size());
	const auto *job = GetExcavationJob(project.job); REQUIRE(job != nullptr);
	CHECK(job->cursor == 0); CHECK(job->completed_earth_t == 0); CHECK(job->completed_rock_t == 0);
	CHECK(company->current_loan == Money{300000});
	if (paused) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
	const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-machinery");
	for (const auto name : {"standard-ready.sav", "standard-ready-repeat.sav"}) {
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", name)), false) == SaveLoadResult::Ok);
	}
	fmt::print("NORMAL_GROUND_MACHINERY project={} job={} cutter={} hauler={} portal={} origin={} depot={} first={} last={} opening_cash={} cash={} loan={} tick={} original_solid={} solid={} reserved={} excavating={} excavated={}\n",
		project.id.base(), project.job.base(), project.cutter.base(), hauler->index.base(), portal.base(), origin.base(), depot.base(), first.base(), last.base(),
		opening.base(), company->money.base(), company->current_loan.base(), TimerGameTick::counter, before_rock[0], after_rock[0], after_rock[3], after_rock[1], after_rock[2]);
	std::fflush(stdout);
}

/** Load the actual paid yard through the native application and acquire participants through public commands. */
inline void RunNormalGroundMachinery()
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
	bool observed = false; std::exception_ptr failure;
	std::function<void()> acquire = [&]() {
		try { AcquireNormalGroundMachinery(); observed = true; } catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &acquire};
	IConsole::CmdRegister("normal_ground_machinery", ObserveNormalGroundNewgame);
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(),
		"-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null",
		"-I", "original_dos", "-d", "misc=4,sl=1", "-g", input};
	std::vector<std::string_view> arguments; for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	REQUIRE(observed);
}

#endif /* TESTS_NORMAL_GROUND_MACHINERY_FIXTURE_H */
