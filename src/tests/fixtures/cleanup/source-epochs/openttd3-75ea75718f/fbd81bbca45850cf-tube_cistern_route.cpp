/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_cistern_route.cpp Original-world paid Cistern route preparation and actual useful receiver acquisition.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15DCB/15DCC/15DDB/15DDC/15DDE.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Every original upstream owner remains; prepared floor/machinery/shafts do not establish an operated route or paid TBM bootstrap.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "cistern_useful_route_fixture.h"
#include "cistern_useful_world_snapshot.h"
#include "cistern_useful_raster_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

static std::function<void()> *_cistern_route_observer = nullptr; ///< Invocation-local ordinary startup observer, never a gameplay or saved owner.

/** @param args Native console arguments. @return Command admission; run the current actual native observer only after ordinary loading. */
static bool ObserveCisternRoute(std::span<std::string_view> args)
{
	if (args.empty()) return true;
	if (_cistern_route_observer != nullptr) (*_cistern_route_observer)();
	return true;
}

/** @param world Actual command-built route and original source. @param expected Original useful-source facts from its accepted report. @param paths Invocation-owned still-stopped ready-world output. Verify complete participants, paid service and unchanged raw/useful source before native saving. */
static void SavePreparedCisternRoute(const CisternUsefulWorld &world, const std::vector<uint64_t> &expected, const TestArtifactPaths &paths)
{
	REQUIRE(TimerGameTick::counter == expected[0]); REQUIRE(Vehicle::GetNumItems() == 9); REQUIRE(Industry::GetNumItems() == 4); REQUIRE(Station::GetNumItems() == 6);
	REQUIRE(world.source.industries[1]->produced[0].waiting == expected[3]);
	REQUIRE(_settings_game.economy.multiple_industry_per_town);
	REQUIRE(CisternSourceInput(*world.source.industries[1], world.source.cargos[0]) == expected[4]);
	REQUIRE(CisternSourceInput(*world.source.industries[1], world.source.cargos[1]) == expected[5]);
	REQUIRE(GetGeologyWorldState().deposits.Get(world.source.deposit)->remaining_reserve == expected[9]);
	const auto &company = *Company::Get(_current_company); REQUIRE(company.current_loan.base() == static_cast<int64_t>(expected[8]));
	REQUIRE(company.money > Money{0}); REQUIRE(company.money.base() < static_cast<int64_t>(expected[7]));
	for (size_t role = 0; role < 2; ++role) {
		const auto account = CisternSourceAccounts(company, world.source.cargos[role]); REQUIRE(account.first.base() == static_cast<int64_t>(expected[6])); REQUIRE(account.second == expected[role + 1]);
	}
	REQUIRE(CisternSourceAccounts(company, world.source.cargos[2]).second == 0); REQUIRE(CargoPacket::GetNumItems() == 0);
	// Original source trains remain whole/stopped/empty. Every new actual modal
	// front and its trailers also stay genuinely stored until the next task;
	// purchase/coupling must not impersonate a loaded movement or useful arrival.
	for (const auto *front : {world.source.parts[0], world.source.parts[2], world.parts[0], world.parts[2], world.parts[3]}) {
		REQUIRE(front->IsChainInDepot()); REQUIRE(front->vehstatus.Test(VehState::Stopped)); REQUIRE(IsTrainOnItsTiles(front));
		for (const auto *part = front; part != nullptr; part = part->Next()) REQUIRE(part->cargo.StoredCount() == 0);
	}
	REQUIRE(world.parts[1]->First() == world.parts[0]); REQUIRE(world.parts[4]->First() == world.parts[3]);
	for (uint index : {1, 2, 4}) REQUIRE(world.parts[index]->cargo_type == world.source.cargos[2]);
	REQUIRE(world.parts[2]->engine_type == GetTubeEngine(11)); REQUIRE(GetTileLayer(world.parts[2]->tile) == MapLayer::Underground);
	const auto *binding = GetTubePowerVehicle(world.parts[2]->index); REQUIRE(binding != nullptr); REQUIRE(binding->source == world.power);
	const auto *producer = GetUtilityPowerProducer(world.power); REQUIRE(producer != nullptr); REQUIRE(producer->consumed_lifetime_j == 0);
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	for (const auto &slot : world.receiver->accepted) REQUIRE(slot.waiting == 0);
	for (const auto &slot : world.receiver->produced) REQUIRE(slot.waiting == 0);
	const auto path = paths.Output("saves", "cistern-route-prepared.sav");
	const auto before = std::tuple{company.money, TimerGameTick::counter, ComputeLayerProcessorDigest(),
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK((before == std::tuple{company.money, TimerGameTick::counter, ComputeLayerProcessorDigest(),
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}}));
	fmt::print("CISTERN_ROUTE_PREPARED tick={} cash={} loan={} source_chemicals=20 unused_oil=6 source_income={} consumed=0 chemical_deliveries=0 multiple_industry_per_town=true original_parts={},{},{},{} parts={},{},{},{},{} industries={},{},{},{} stations={},{},{},{},{},{} power={} path={}\n",
		TimerGameTick::counter, company.money.base(), company.current_loan.base(), CisternSourceAccounts(company, world.source.cargos[0]).first.base(),
		world.source.parts[0]->index.base(), world.source.parts[1]->index.base(), world.source.parts[2]->index.base(), world.source.parts[3]->index.base(),
		world.parts[0]->index.base(), world.parts[1]->index.base(), world.parts[2]->index.base(), world.parts[3]->index.base(), world.parts[4]->index.base(),
		world.source.industries[0]->index.base(), world.source.industries[1]->index.base(), world.source.industries[2]->index.base(), world.receiver->index.base(),
		world.source.stations[0]->index.base(), world.source.stations[1]->index.base(), world.source.stations[2]->index.base(), world.stations[0]->index.base(), world.stations[1]->index.base(), world.stations[2]->index.base(), world.power.base(), path.string());
}

/** Ordinary native content/date commands must fund a useful compatible receiver, actual liquid modal fleet and paid tube service before movement. */
TEST_CASE("Cistern source prepares an actual paid stopped modal route and native PCHM receiver", "[.tube-cistern-route-prepare-native]")
{
	const char *input = std::getenv("OTTD_CISTERN_SOURCE_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", "cistern-route"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Acquire only the accepted original useful source; all new participants are real command-funded native owners in that same world. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; AutoRestoreBackup pause{_pause_mode, PauseModes{}};
			const auto source = LoadCisternSourceWorld(); const auto expected = CisternSourceValues("OTTD_CISTERN_READY_VALUES", 11);
			REQUIRE(TimerGameTick::counter == expected[0]); REQUIRE(Company::Get(_current_company)->money.base() == static_cast<int64_t>(expected[7]));
			CisternUsefulRouteBuilder builder(source); SavePreparedCisternRoute(builder.world, expected, paths);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_cistern_route_observer, &inspect}; IConsole::CmdRegister("cistern_route_prepare_check", ObserveCisternRoute);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** @param industry Actual native product inventory. @param cargo Exact existing product meaning. @return Its whole native stock, including a facility's nonprimary alternative output slot. */
static uint CisternRouteProduct(const Industry &industry, CargoType cargo)
{
	const auto slot = std::ranges::find(industry.produced, cargo, &Industry::ProducedCargo::cargo);
	return slot == industry.produced.end() ? 0 : slot->waiting;
}

/** @param world Original paid modal participants. Advance every actual native vehicle/clock/power owner and real processor work exactly once; source dispatch uses only genuinely earned output and ordinary station service. */
static void RunCisternUsefulOpportunity(const CisternUsefulWorld &world)
{
	RunUsefulTubeNativeOpportunity(); UpdateLandscapingLimits();
	AdvanceLayerProcessor(*world.source.industries[1], 1); AdvanceLayerProcessor(*world.receiver, 1);
	DispatchLayerProcessorCargo(*world.source.industries[1]);
	// Native pool order advances all three modal fronts and both retained raw
	// source keepers. No private front tick, arrival, pose or source work replay
	// can hide a lost trailer, a layer crossing by the capsule or a moving keeper.
	for (const auto *front : {world.source.parts[0], world.source.parts[2], world.parts[0], world.parts[2], world.parts[3]}) {
		REQUIRE(IsTrainOnItsTiles(front)); REQUIRE_FALSE(front->vehstatus.Test(VehState::Crashed));
	}
	for (uint index : {0, 2}) { REQUIRE(world.source.parts[index]->IsChainInDepot()); REQUIRE(world.source.parts[index]->vehstatus.Test(VehState::Stopped)); }
	REQUIRE(GetTileLayer(world.parts[2]->tile) == MapLayer::Underground);
}

/** @param world Exact original modal/source owners. @param expected Accepted ready tick/cash/loan/source income/chemical quantity/unused Oil/reserve/Oil and Salt delivered totals. Require disjoint physical chemical conservation, actual useful recipe output and untouched raw ownership. */
static void CheckCisternUsefulConservation(const CisternUsefulWorld &world, const std::vector<uint64_t> &expected)
{
	const auto chemical = world.source.cargos[2], propellant = GetCargoTypeByLabel(CargoLabel{"PROP"}); REQUIRE(propellant != INVALID_CARGO);
	const auto *source = GetLayerProcessorState(world.source.industries[1]->index), *receiver = GetLayerProcessorState(world.receiver->index);
	REQUIRE(source != nullptr); REQUIRE(receiver != nullptr); REQUIRE(source->produced.at(CargoLabel{"CHEM"}) == expected[4]);
	uint64_t physical = CisternSourceInput(*world.receiver, chemical) + receiver->consumed.at(CargoLabel{"CHEM"});
	// All real producer buffers, station queues and actual stored vehicle
	// loads are disjoint native owners. Historical PCHM consumption closes the
	// material equation; company payment is never counted as another cargo store.
	for (const auto *industry : Industry::Iterate()) physical += CisternRouteProduct(*industry, chemical);
	for (const auto *station : Station::Iterate()) physical += station->goods[chemical].TotalCount();
	for (const auto *part : Vehicle::Iterate()) if (part->cargo_type == chemical) physical += part->cargo.StoredCount();
	REQUIRE(physical == expected[4]); REQUIRE(receiver->produced.at(CargoLabel{"PROP"}) == receiver->consumed.at(CargoLabel{"CHEM"}));
	REQUIRE(receiver->dispatched == 0); REQUIRE(CisternRouteProduct(*world.receiver, propellant) == receiver->produced.at(CargoLabel{"PROP"}));
	REQUIRE(CisternSourceInput(*world.source.industries[1], world.source.cargos[0]) == expected[5]);
	REQUIRE(CisternSourceInput(*world.source.industries[1], world.source.cargos[1]) == 0);
	REQUIRE(GetGeologyWorldState().deposits.Get(world.source.deposit)->remaining_reserve == expected[6]);
	const auto &company = *Company::Get(_current_company);
	REQUIRE(CisternSourceAccounts(company, world.source.cargos[0]).second == expected[7]); REQUIRE(CisternSourceAccounts(company, world.source.cargos[1]).second == expected[8]);
	REQUIRE(CisternSourceAccounts(company, chemical).second <= expected[4]);
	// Every live packet still describes original CHEM from the actual works
	// and its original first Surface station, even after real loading splits
	// and both native modal transfers. PROP remains the undispatched useful output.
	for (const auto *packet : CargoPacket::Iterate()) {
		REQUIRE(packet->GetSource() == Source{world.source.industries[1]->index, SourceType::Industry});
		REQUIRE(packet->GetFirstStation() == world.source.stations[1]->index); REQUIRE(packet->GetCompanySupplyOrder() == 0);
	}
}

/** @param world Original native participants. @param paths Invocation-owned output locations. @param scene Actual loaded modal state0..4 or final useful state5. @param step Original boundary0 or its untouched next native opportunity1. Saving must preserve every named owner and both RNGs. */
static void SaveCisternUsefulWorld(const CisternUsefulWorld &world, const TestArtifactPaths &paths, uint scene, uint step)
{
	const auto before = CaptureCisternUsefulWorld(world);
	const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
	const auto path = paths.Output("saves", fmt::format("cistern-useful-{}-{}.sav", scene, step));
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureCisternUsefulWorld(world) == before);
	CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
	fmt::print("CISTERN_WORLD scene={} step={} tick={} parts={},{},{},{},{},{},{},{},{} references={},{},{},{},{},{},{},{},{},{},{},{} snapshot={} path={}\n",
		scene, step, TimerGameTick::counter,
		world.source.parts[0]->index.base(), world.source.parts[1]->index.base(), world.source.parts[2]->index.base(), world.source.parts[3]->index.base(),
		world.parts[0]->index.base(), world.parts[1]->index.base(), world.parts[2]->index.base(), world.parts[3]->index.base(), world.parts[4]->index.base(),
		world.source.industries[0]->index.base(), world.source.industries[1]->index.base(), world.source.industries[2]->index.base(), world.receiver->index.base(),
		world.source.stations[0]->index.base(), world.source.stations[1]->index.base(), world.source.stations[2]->index.base(), world.stations[0]->index.base(), world.stations[1]->index.base(), world.stations[2]->index.base(),
		world.power.base(), world.source.stations[0]->town->index.base(), before, path.string());
}

/** @param world Original ordinary-loaded paid route. @param expected Accepted ready-world scalar observations. @param paths Invocation-owned original useful-world output. @param save_files Also retain six real worlds and their untouched next native opportunities. Earn all physical legs, native PCHM output and whole empty home returns, retaining original provenance/accounts. */
static void EarnCisternUsefulRoute(const CisternUsefulWorld &world, const std::vector<uint64_t> &expected, const TestArtifactPaths &paths, bool save_files)
{
	const auto chemical = world.source.cargos[2], propellant = GetCargoTypeByLabel(CargoLabel{"PROP"}); REQUIRE(propellant != INVALID_CARGO);
	REQUIRE(TimerGameTick::counter == expected[0]); REQUIRE(Company::Get(_current_company)->money.base() == static_cast<int64_t>(expected[1]));
	REQUIRE(Company::Get(_current_company)->current_loan.base() == static_cast<int64_t>(expected[2]));
	REQUIRE(CisternSourceAccounts(*Company::Get(_current_company), chemical).first.base() == static_cast<int64_t>(expected[3]));
	REQUIRE(CisternSourceAccounts(*Company::Get(_current_company), chemical).second == 0); REQUIRE(CargoPacket::GetNumItems() == 0);
	REQUIRE(CisternRouteProduct(*world.source.industries[1], chemical) == expected[4]);
	REQUIRE(GetUtilityPowerProducer(world.power)->consumed_lifetime_j == 0); REQUIRE(CisternRouteProduct(*world.receiver, propellant) == 0);
	CheckCisternUsefulConservation(world, expected);
	const std::array fronts{world.parts[0], world.parts[2], world.parts[3]};
	const std::array homes{world.feed_depots[0], world.tube_depots[0], world.delivery_depots[0]};
	for (size_t index = 0; index < fronts.size(); ++index) {
		REQUIRE(fronts[index]->IsChainInDepot()); REQUIRE(fronts[index]->tile == homes[index]); REQUIRE(fronts[index]->vehstatus.Test(VehState::Stopped));
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, fronts[index]->index, true).Succeeded());
	}
	std::array<bool, 5> scenes{}; std::array<bool, 3> returned{}; uint ticks = 0;
	std::array<bool, 5> saved{}; std::optional<uint> pending;
	// Actual normal station/service orders must earn all modal handling. Source
	// dispatch waits for normal service eligibility; no rating, packet, arrival
	// or order cursor is assigned to manufacture a first loaded movement.
	while (ticks++ < 24000) {
		RunCisternUsefulOpportunity(world); CheckCisternUsefulConservation(world, expected);
		// The pending scene's baseline is precisely the next ordinary shared
		// opportunity, before any new scene/return observation or Stop command.
		// File writing inserts no private tick or second allocation.
		if (pending.has_value()) { SaveCisternUsefulWorld(world, paths, *pending, 1); pending.reset(); }
		const auto accounts = CisternSourceAccounts(*Company::Get(_current_company), chemical);
		if (accounts.second == 0) REQUIRE(accounts.first.base() == static_cast<int64_t>(expected[3]));
		const std::array eligible{
			world.parts[1]->cargo.StoredCount() > 0 && GetTileLayer(world.parts[1]->tile) == MapLayer::Surface,
			world.parts[1]->cargo.StoredCount() > 0 && GetTileLayer(world.parts[1]->tile) == MapLayer::Underground,
			world.parts[2]->cargo.StoredCount() > 0 && world.parts[2]->cur_speed > 0,
			world.parts[4]->cargo.StoredCount() > 0 && GetTileLayer(world.parts[4]->tile) == MapLayer::Underground,
			world.parts[4]->cargo.StoredCount() > 0 && GetTileLayer(world.parts[4]->tile) == MapLayer::Surface};
		// Each scene is only an observation of the first real loaded native
		// state. It never moves a vehicle, chooses an arrived order or changes
		// body geometry, source art or the saved material owner.
		for (size_t scene = 0; scene < scenes.size(); ++scene) if (eligible[scene] && !scenes[scene]) {
			scenes[scene] = true; const auto *part = world.parts[scene < 2 ? 1 : scene == 2 ? 2 : 4];
			fmt::print("CISTERN_ROUTE_SCENE scene={} tick={} cargo={} tile={}\n", scene, TimerGameTick::counter, part->cargo.StoredCount(), part->tile.base());
		}
		if (save_files) {
			// Retain the first actual eligible state exactly once. An existing
			// baseline has priority if two modal scenes become eligible together.
			for (uint scene = 0; scene < saved.size(); ++scene) {
				if (saved[scene] || !eligible[scene] || pending.has_value()) continue;
				SaveCisternUsefulWorld(world, paths, scene, 0); saved[scene] = true; pending = scene;
			}
		}
		// An initial stored train or a front ahead of its outside tail cannot
		// establish return. Only actual final useful intake/output permits whole
		// empty home storage, followed by the ordinary player Stop command.
		if (accounts.second == expected[4] && CisternRouteProduct(*world.receiver, propellant) == expected[4]) {
			for (size_t index = 0; index < fronts.size(); ++index) {
				const auto *hold = world.parts[index == 0 ? 1 : index == 1 ? 2 : 4];
				if (!returned[index] && fronts[index]->IsChainInDepot() && fronts[index]->tile == homes[index] && hold->cargo.StoredCount() == 0 && fronts[index]->cargo_payment == nullptr) {
					if (!fronts[index]->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, fronts[index]->index, true).Succeeded());
					returned[index] = true; fmt::print("CISTERN_ROUTE_RETURN role={} tick={}\n", index, TimerGameTick::counter);
				}
			}
			if (std::ranges::all_of(returned, std::identity{})) break;
		}
	}
	for (const auto *front : fronts) INFO(DescribeTrain(front));
	INFO(fmt::format("source={} origin={} destination={} receiver_input={} propellant={}", CisternRouteProduct(*world.source.industries[1], chemical), world.stations[0]->goods[chemical].TotalCount(), world.stations[1]->goods[chemical].TotalCount(), CisternSourceInput(*world.receiver, chemical), CisternRouteProduct(*world.receiver, propellant)));
	REQUIRE(ticks < 24000); REQUIRE(std::ranges::all_of(scenes, std::identity{})); REQUIRE(std::ranges::all_of(returned, std::identity{}));
	const auto final = CisternSourceAccounts(*Company::Get(_current_company), chemical);
	REQUIRE(final.second == expected[4]); REQUIRE(final.first.base() > static_cast<int64_t>(expected[3])); REQUIRE(CargoPacket::GetNumItems() == 0);
	REQUIRE(GetUtilityPowerProducer(world.power)->consumed_lifetime_j > 0);
	// Two further native economy days with three stopped whole empty routes
	// cannot replay original Chemicals, useful Propellant, company income or
	// upstream extraction. Native running/service costs retain their real owner.
	for (uint empty = 0; empty < 148; ++empty) {
		RunCisternUsefulOpportunity(world); CheckCisternUsefulConservation(world, expected);
		for (const auto *front : fronts) { REQUIRE(front->IsChainInDepot()); REQUIRE(front->vehstatus.Test(VehState::Stopped)); }
	}
	REQUIRE(CisternSourceAccounts(*Company::Get(_current_company), chemical) == final); REQUIRE(CargoPacket::GetNumItems() == 0);
	// The useful modal loop cannot reset the original source's lifetime raw
	// consumption or its one real chemical dispatch. Verify those native
	// historical owners alongside the unchanged six unused Oil and reserve,
	// including the real quarter crossed by this complete physical route.
	const auto *source_history = GetLayerProcessorState(world.source.industries[1]->index); REQUIRE(source_history != nullptr);
	REQUIRE(source_history->consumed.at(CT_OIL) == expected[7] - expected[5]);
	REQUIRE(source_history->consumed.at(CargoLabel{"SALT"}) == expected[8]); REQUIRE(source_history->dispatched == expected[4]);
	REQUIRE(Company::Get(_current_company)->num_valid_stat_ent > 0); REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{1980});
	if (save_files) {
		REQUIRE(std::ranges::all_of(saved, std::identity{})); REQUIRE_FALSE(pending.has_value());
		SaveCisternUsefulWorld(world, paths, 5, 0);
		RunCisternUsefulOpportunity(world); CheckCisternUsefulConservation(world, expected);
		SaveCisternUsefulWorld(world, paths, 5, 1);
	}
	const auto path = paths.Output("saves", "cistern-useful.sav"); const auto &company = *Company::Get(_current_company);
	const auto before = std::tuple{company.money, TimerGameTick::counter, ComputeLayerProcessorDigest(), GetUtilityPowerProducer(world.power)->consumed_lifetime_j,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}}; REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK((before == std::tuple{company.money, TimerGameTick::counter, ComputeLayerProcessorDigest(), GetUtilityPowerProducer(world.power)->consumed_lifetime_j,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}}));
	fmt::print("CISTERN_USEFUL year={} original_tick={} tick={} route_ticks={} delivered={} propellant={} unused_oil={} reserve={} income={} total_income={} cash={} loan={} consumed={} quarter_history={} scenes=5 returns=3 empty_ticks=148 path={}\n",
		TimerGameCalendar::year.base(), expected[0], TimerGameTick::counter, ticks, final.second, CisternRouteProduct(*world.receiver, propellant), CisternSourceInput(*world.source.industries[1], world.source.cargos[0]),
		GetGeologyWorldState().deposits.Get(world.source.deposit)->remaining_reserve, final.first.base() - static_cast<int64_t>(expected[3]), final.first.base(), company.money.base(), company.current_loan.base(),
		GetUtilityPowerProducer(world.power)->consumed_lifetime_j, company.num_valid_stat_ent, path.string());
}

/** @param save_files Retain actual modal worlds and untouched next opportunities in addition to the original useful-route proof. Normal application loading alone acquires all original participants. */
static void ObserveCisternUsefulRoute(bool save_files)
{
	const char *input = std::getenv("OTTD_CISTERN_ROUTE_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", save_files ? "cistern-files" : "cistern-route"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Load the accepted original paid ready world; construction, raw-source work and registry setup are forbidden in this movement observer. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; AutoRestoreBackup pause{_pause_mode, PauseModes{}};
			const auto world = LoadCisternUsefulWorld(); const auto expected = CisternSourceValues("OTTD_CISTERN_ROUTE_VALUES", 9);
			EarnCisternUsefulRoute(world, expected, paths, save_files);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_cistern_route_observer, &inspect}; IConsole::CmdRegister(save_files ? "cistern_useful_files_check" : "cistern_useful_check", ObserveCisternRoute);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** All genuinely earned original Chemicals physically change modes through paid Cistern service into native PCHM output before whole empty returns. */
TEST_CASE("Original Chemicals earn useful native Cistern modal transport and PCHM output", "[.tube-cistern-useful-native]") { ObserveCisternUsefulRoute(false); }

/** Pure native saving retains six actual modal/final original worlds and precisely their next complete shared opportunities. */
TEST_CASE("Original useful Cistern route writes complete native worlds and untouched baselines", "[.tube-cistern-useful-files-native]") { ObserveCisternUsefulRoute(true); }

/** @param world Exact original native participants. @param expected Original writer's complete named observation. Preserve a diagnosable changed field rather than repairing a loader's state. */
static void CheckCisternUsefulSnapshot(const CisternUsefulWorld &world, std::string_view expected)
{
	const auto actual = CaptureCisternUsefulWorld(world);
	if (actual != expected) fmt::print("CISTERN_SNAPSHOT actual={}\n", actual);
	REQUIRE(actual == expected);
}

/** Ordinary application loading, copied preview and pure resave must retain every original source/route owner before the optional next complete native opportunity. */
TEST_CASE("Original useful Cistern files preserve native owners preview and complete continuation", "[.tube-cistern-useful-native-file]")
{
	const char *input = std::getenv("OTTD_CISTERN_USEFUL_INPUT"), *action_text = std::getenv("OTTD_CISTERN_USEFUL_ACTION"), *snapshot_text = std::getenv("OTTD_CISTERN_USEFUL_SNAPSHOT");
	REQUIRE(input != nullptr); REQUIRE(action_text != nullptr); REQUIRE(snapshot_text != nullptr);
	const std::string_view action{action_text}; const std::string expected{snapshot_text};
	// File inspection cannot accidentally select construction or another work
	// mode. Only an explicitly requested shared opportunity may change owners.
	REQUIRE((action == "inspect" || action == "step" || action == "raster"));
	const auto paths = GetTestCaseArtifactPaths("tubes", "cistern-files"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe only original normal-loaded pools; copied preview cannot consume simulation or client entropy, and optional continuation shares the exact writer driver. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			auto world = LoadCisternUsefulWorld(); CheckCisternUsefulSnapshot(world, expected);
			const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			world = LoadCisternUsefulWorld(); CheckCisternUsefulSnapshot(world, expected);
			CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
			const auto values = CisternSourceValues("OTTD_CISTERN_ROUTE_VALUES", 9); CheckCisternUsefulConservation(world, values);
			const auto tick = TimerGameTick::counter;
			if (action == "step") {
				AutoRestoreBackup pause{_pause_mode, PauseModes{}};
				RunCisternUsefulOpportunity(world); CheckCisternUsefulConservation(world, values); CHECK(TimerGameTick::counter == tick + 1);
			}
			if (action == "raster") {
				/** Advance every original owner through exactly the accepted shared useful-route opportunity, retaining raw/chemical/PROP conservation. */
				CaptureUsefulCistern(world, paths, [&] { RunCisternUsefulOpportunity(world); CheckCisternUsefulConservation(world, values); });
			}
			// Both raw keepers and every actual modal front must occupy their
			// original real map membership; a trailer cannot disappear on load
			// merely because the observed capsule count or power digest matches.
			for (const auto *front : {world.source.parts[0], world.source.parts[2], world.parts[0], world.parts[2], world.parts[3]}) {
				REQUIRE(IsTrainOnItsTiles(front)); REQUIRE_FALSE(front->vehstatus.Test(VehState::Crashed));
			}
			const auto before = CaptureCisternUsefulWorld(world);
			const std::array save_interactive{_interactive_random.state[0], _interactive_random.state[1]};
			const auto path = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action));
			{
				AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
				AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
			}
			CHECK(CaptureCisternUsefulWorld(world) == before);
			CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == save_interactive));
			fmt::print("CISTERN_READ action={} original_parts=9 fronts=5 full_native_clock=true preview_pure=true snapshot={} path={}\n", action, before, path.string());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_cistern_route_observer, &inspect}; IConsole::CmdRegister("cistern_useful_file_check", ObserveCisternRoute);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
