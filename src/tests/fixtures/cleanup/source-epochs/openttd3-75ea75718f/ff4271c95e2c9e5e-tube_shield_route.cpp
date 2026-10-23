/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_shield_route.cpp Paid original-world Shield preparation and actual useful freight with continuing finite extraction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15ECB/15ECC/15EDB/15EDC/15EDE.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor/Connected shafts remain boundaries; only actual movement earns useful freight, never TBM bootstrap.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "shield_useful_route_fixture.h"
#include "shield_useful_world_snapshot.h"
#include "shield_useful_raster_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

/** @param source Exact original native source carrier. @return Both parts' complete physical pose; installing ordinary orders may not relocate a retained part. */
static auto ShieldOriginalPositions(const ShieldSourceWorld &source)
{
	/** @param part Actual original native part. @return Exact physical membership/pose, independent of its new ordinary order list. */
	const auto position = [](const Train &part) { return std::tuple{part.tile, part.x_pos, part.y_pos, part.z_pos, part.direction, part.track, part.cur_speed}; };
	return std::array{position(*source.parts[0]), position(*source.parts[1])};
}

/** @param world Actual paid same-world modal participants. @return Named complete-source/new-part/station/power observations for pure ready saving, with no work or state repair. */
static std::string CaptureShieldReadyWorld(const ShieldUsefulWorld &world)
{
	std::string result = CaptureShieldSourceWorld(world.source);
	/** @param name Unique additional native owner/field name. @param value Exact native integer bits, including signed native amounts. */
	const auto add = [&](std::string_view name, uint64_t value) { result += fmt::format("{}={},", name, value); };
	// Every new actual part accompanies the original carrier. Saving cannot
	// hide a lost trailer or new custody merely because the source tuple matches.
	for (size_t index = 0; index < world.parts.size(); ++index) {
		const auto &part = *world.parts[index]; const auto prefix = fmt::format("route_part{}_", index);
		add(prefix + "id", part.index.base()); add(prefix + "engine", part.engine_type.base()); add(prefix + "first", part.First()->index.base());
		add(prefix + "tile", part.tile.base()); add(prefix + "x", part.x_pos); add(prefix + "y", part.y_pos); add(prefix + "z", part.z_pos);
		add(prefix + "cargo", part.cargo_type); add(prefix + "units", part.cargo.StoredCount()); add(prefix + "capacity", part.cargo_cap);
		add(prefix + "speed", part.cur_speed); add(prefix + "status", part.vehstatus.base()); add(prefix + "flags", part.flags.base());
		add(prefix + "order", to_underlying(part.current_order.GetType())); add(prefix + "destination", part.current_order.GetDestination().base()); add(prefix + "order_index", part.cur_real_order_index);
	}
	// All joined native station owners and source eligibility remain actual;
	// no queue/rating/speed assignment may be disguised as paid preparation.
	for (size_t index = 0; index < world.stations.size(); ++index) {
		const auto &station = *world.stations[index]; const auto &entry = station.goods[world.source.cargo]; const auto prefix = fmt::format("route_station{}_", index);
		add(prefix + "id", station.index.base()); add(prefix + "tile", station.xy.base()); add(prefix + "owner", station.owner.base());
		add(prefix + "units", entry.TotalCount()); add(prefix + "rating", entry.rating); add(prefix + "speed", entry.last_speed); add(prefix + "fraction", entry.amount_fract); add(prefix + "status", entry.status.base());
	}
	add("route_source", world.power.base()); add("utility", ComputeUtilityPowerDigest()); add("binding", ComputeTubePowerVehicleDigest()); add("invoice", ComputeTubePowerDigest());
	return result;
}

/** @param world Actual original source plus paid modal participants. @param expected Original tick/cash/loan/stock/reserve/extracted/dispatched facts. @param paths Invocation-owned native ready-world output. Require full stopped ownership, unchanged original stock/history and pure saving. */
static void SavePreparedShieldRoute(const ShieldUsefulWorld &world, const std::vector<uint64_t> &expected, const TestArtifactPaths &paths)
{
	REQUIRE(TimerGameTick::counter == expected[0]); REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{1985});
	REQUIRE(Vehicle::GetNumItems() == 5); REQUIRE(Industry::GetNumItems() == 2); REQUIRE(Station::GetNumItems() == 4); REQUIRE(CargoPacket::GetNumItems() == 0);
	REQUIRE(world.source.industries[0]->produced[0].waiting == expected[3]); REQUIRE(GetGeologyWorldState().deposits.Get(world.source.deposit)->remaining_reserve == expected[4]);
	const auto *mine = GetLayerMineState(world.source.industries[0]->index); REQUIRE(mine != nullptr); REQUIRE(mine->extracted == expected[5]); REQUIRE(mine->dispatched == expected[6]);
	REQUIRE(mine->work_ticks == 0);
	// A real native receiver already belongs to this source. Preparation may
	// not consume an input or manufacture output while no freight has moved.
	for (const auto &entry : world.source.industries[1]->accepted) REQUIRE(entry.waiting == 0);
	for (const auto &entry : world.source.industries[1]->produced) REQUIRE(entry.waiting == 0);
	const auto &company = *Company::Get(_current_company); REQUIRE(company.money > Money{0}); REQUIRE(company.money.base() < static_cast<int64_t>(expected[1]));
	REQUIRE(company.current_loan.base() == static_cast<int64_t>(expected[2])); REQUIRE(company.cur_economy.income == Money{0}); REQUIRE(company.num_valid_stat_ent == 0);
	const std::array fronts{world.source.parts[0], world.parts[0], world.parts[1]};
	const std::array homes{world.feed_depots[0], world.tube_depots[0], world.delivery_depots[0]};
	// All three actual fronts and every coupled part must remain genuinely
	// stored/stopped/empty. Native purchase/rail/order/service commands cannot
	// impersonate any loaded departure, layer transition or useful arrival.
	for (size_t index = 0; index < fronts.size(); ++index) {
		REQUIRE(fronts[index]->IsChainInDepot()); REQUIRE(fronts[index]->tile == homes[index]); REQUIRE(fronts[index]->vehstatus.Test(VehState::Stopped)); REQUIRE(IsTrainOnItsTiles(fronts[index]));
		for (const auto *part = fronts[index]; part != nullptr; part = part->Next()) REQUIRE(part->cargo.StoredCount() == 0);
	}
	REQUIRE(world.parts[0]->engine_type == GetTubeEngine(12)); REQUIRE(world.parts[0]->Next() == nullptr); REQUIRE(GetTileLayer(world.parts[0]->tile) == MapLayer::Underground);
	REQUIRE(world.parts[2]->First() == world.parts[1]); REQUIRE(world.parts[2]->engine_type == GetUndergroundEngine(6));
	for (const auto *part : {world.source.parts[1], world.parts[0], world.parts[2]}) REQUIRE(part->cargo_type == world.source.cargo);
	const auto *binding = GetTubePowerVehicle(world.parts[0]->index); REQUIRE(binding != nullptr); REQUIRE(binding->source == world.power);
	const auto *producer = GetUtilityPowerProducer(world.power); REQUIRE(producer != nullptr); REQUIRE(producer->consumed_lifetime_j == 0);
	REQUIRE(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	const auto portals = CaptureLayerPortalWorkRecords(); REQUIRE(portals.size() == 3);
	for (const auto &portal : portals) REQUIRE(portal.state == LayerPortalWorkState::Connected);
	const auto before = CaptureShieldReadyWorld(world); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
	const auto path = paths.Output("saves", "shield-route-prepared.sav");
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureShieldReadyWorld(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
	fmt::print("SHIELD_ROUTE_PREPARED tick={} cash={} loan={} stock={} reserve={} extracted={} dispatched={} consumed=0 parts={},{},{},{},{} industries={},{} stations={},{},{},{} power={} portals={},{},{} path={}\n",
		TimerGameTick::counter, company.money.base(), company.current_loan.base(), world.source.industries[0]->produced[0].waiting, GetGeologyWorldState().deposits.Get(world.source.deposit)->remaining_reserve, mine->extracted, mine->dispatched,
		world.source.parts[0]->index.base(), world.source.parts[1]->index.base(), world.parts[0]->index.base(), world.parts[1]->index.base(), world.parts[2]->index.base(),
		world.source.industries[0]->index.base(), world.source.industries[1]->index.base(), world.stations[0]->index.base(), world.stations[1]->index.base(), world.stations[2]->index.base(), world.stations[3]->index.base(),
		world.power.base(), portals[0].pair_key.base(), portals[1].pair_key.base(), portals[2].pair_key.base(), path.string());
}

static std::function<void()> *_shield_route_observer = nullptr; ///< Invocation-owned ordinary startup observer, never a source or saved owner.

/** @param args Native console arguments after normal source loading. @return Command admission; invoke only the current actual native observer. */
static bool ObserveShieldRoute(std::span<std::string_view> args)
{
	if (args.empty()) return true;
	REQUIRE(_shield_route_observer != nullptr); (*_shield_route_observer)(); return true;
}

/** Actual original source identities/stock/history must survive all paid native modal preparation before a useful route may start. */
TEST_CASE("Original Shield source prepares paid stopped Deep Underground Surface modal participants", "[.tube-shield-route-prepare-native]")
{
	const char *input = std::getenv("OTTD_SHIELD_ROUTE_INPUT"), *snapshot = std::getenv("OTTD_SHIELD_ROUTE_SOURCE_SNAPSHOT"); REQUIRE(input != nullptr); REQUIRE(snapshot != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", "shield-route"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Load only the accepted original source. Every new participant is actually command-funded; the original receiver/carrier/mine/history remain unchanged. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; AutoRestoreBackup pause{_pause_mode, PauseModes{}};
			const auto source = LoadShieldSourceWorld(); REQUIRE(CaptureShieldSourceWorld(source) == snapshot);
			const auto expected = ShieldSourceValues("OTTD_SHIELD_ROUTE_SOURCE_VALUES", 7);
			REQUIRE(TimerGameTick::counter == expected[0]); REQUIRE(Company::Get(_current_company)->money.base() == static_cast<int64_t>(expected[1]));
			const auto original_positions = ShieldOriginalPositions(source); const auto original_mine = *GetLayerMineState(source.industries[0]->index);
			const auto original_receiver = *GetLayerProcessorState(source.industries[1]->index);
			ShieldUsefulRouteBuilder builder(source);
			CHECK(ShieldOriginalPositions(source) == original_positions); CHECK(*GetLayerMineState(source.industries[0]->index) == original_mine); CHECK(*GetLayerProcessorState(source.industries[1]->index) == original_receiver);
			SavePreparedShieldRoute(builder.world, expected, paths);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_shield_route_observer, &inspect}; IConsole::CmdRegister("shield_route_prepare_check", ObserveShieldRoute);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** @param company Actual native accounting owner. @param cargo Original RARE meaning. @return Exact native income/delivery totals across current and every valid historical quarter. */
static auto ShieldNativeAccounts(const Company &company, CargoType cargo)
{
	Money income = company.cur_economy.income; uint64_t delivered = company.cur_economy.delivered_cargo[cargo];
	// Real calendar rollover retains previous native buckets. Do not freeze
	// time or replace historical owners with a private accumulated balance.
	for (uint index = 0; index < company.num_valid_stat_ent; ++index) {
		income += company.old_economy[index].income; delivered += company.old_economy[index].delivered_cargo[cargo];
	}
	return std::pair{income, delivered};
}

/** @param industry Actual native receiving inventory. @param cargo Original accepted meaning. @return Existing whole input stock; no missing slot is constructed. */
static uint ShieldNativeInput(const Industry &industry, CargoType cargo)
{
	const auto slot = std::ranges::find(industry.accepted, cargo, &Industry::AcceptedCargo::cargo);
	return slot == industry.accepted.end() ? 0 : slot->waiting;
}

/** @param world Original real source/modal owners. @param initial_reserve Actual original generated reserve before native extraction. Conserve every physical unit and native receipt while source work continues normally. */
static void CheckShieldUsefulConservation(const ShieldUsefulWorld &world, uint64_t initial_reserve)
{
	const auto cargo = world.source.cargo; const auto *mine = GetLayerMineState(world.source.industries[0]->index);
	const auto *receiver = GetLayerProcessorState(world.source.industries[1]->index); REQUIRE(mine != nullptr); REQUIRE(receiver != nullptr);
	const auto *deposit = GetGeologyWorldState().deposits.Get(world.source.deposit); REQUIRE(deposit != nullptr); REQUIRE(deposit->initial_reserve == initial_reserve);
	uint64_t physical = 0;
	// All real industry buffers, queues and actual stored vehicle holds are
	// disjoint physical owners. Historical intake/consumption closes the native
	// equation; company payment is not counted as another cargo inventory.
	for (const auto *industry : Industry::Iterate()) {
		for (const auto &entry : industry->produced) if (entry.cargo == cargo) physical += entry.waiting;
		for (const auto &entry : industry->accepted) if (entry.cargo == cargo) physical += entry.waiting;
	}
	for (const auto *station : Station::Iterate()) physical += station->goods[cargo].TotalCount();
	for (const auto *part : Vehicle::Iterate()) if (part->cargo_type == cargo) physical += part->cargo.StoredCount();
	const auto consumed = receiver->consumed.at(CargoLabel{"RARE"});
	REQUIRE(deposit->remaining_reserve + physical + consumed == initial_reserve);
	REQUIRE(mine->extracted == initial_reserve - deposit->remaining_reserve);
	REQUIRE(mine->extracted == mine->dispatched + world.source.industries[0]->produced[0].waiting);
	const auto refined = GetCargoTypeByLabel(CargoLabel{"RFMT"}); REQUIRE(refined != INVALID_CARGO);
	REQUIRE(ShieldNativeInput(*world.source.industries[1], refined) == 0); REQUIRE(consumed == 0); REQUIRE(receiver->dispatched == 0);
	for (const auto &entry : world.source.industries[1]->produced) REQUIRE(entry.waiting == 0);
	const auto account = ShieldNativeAccounts(*Company::Get(world.source.parts[0]->owner), cargo);
	REQUIRE(ShieldNativeInput(*world.source.industries[1], cargo) == account.second);
	// Every live original and later actually produced packet retains the real
	// source/first-station/purpose through native splits, transfers and aging.
	// No replacement packet or nonpaying supply order may impersonate freight.
	for (const auto *packet : CargoPacket::Iterate()) {
		REQUIRE(packet->GetSource() == Source{world.source.industries[0]->index, SourceType::Industry});
		REQUIRE(packet->GetFirstStation() == world.stations[0]->index); REQUIRE(packet->GetCompanySupplyOrder() == 0);
	}
}

/** @param world Exact original paid route. Advance every original clock/front and both real industries once, then dispatch only genuine finite output through earned native station service. */
static void RunShieldUsefulOpportunity(const ShieldUsefulWorld &world)
{
	RunShieldSourceOpportunity(); DispatchLayerMineCargo(*world.source.industries[0]);
	// All three actual fronts and every real trailer advance in stable native
	// pool order. A private capsule tick cannot hide a lost conventional part
	// or a forbidden tube layer crossing while source extraction continues.
	for (const auto *front : {world.source.parts[0], world.parts[0], world.parts[1]}) {
		REQUIRE(IsTrainOnItsTiles(front)); REQUIRE_FALSE(front->vehstatus.Test(VehState::Crashed));
	}
	REQUIRE(GetTileLayer(world.parts[0]->tile) == MapLayer::Underground);
}

/** @param world Borrowed original native participants. @param paths Invocation-owned file outputs. @param scene Real loaded scene0..4 or completed useful world5. @param step Original boundary0 or precisely its next complete native opportunity1. Save without changing any named owner or either RNG. */
static void SaveShieldUsefulWorld(const ShieldUsefulWorld &world, const TestArtifactPaths &paths, uint scene, uint step)
{
	const auto before = CaptureShieldUsefulWorld(world); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
	const auto path = paths.Output("saves", fmt::format("shield-useful-{}-{}.sav", scene, step));
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureShieldUsefulWorld(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
	fmt::print("SHIELD_WORLD scene={} step={} tick={} parts={},{},{},{},{} references={},{},{},{},{},{},{},{} snapshot={} path={}\n", scene, step, TimerGameTick::counter,
		world.source.parts[0]->index.base(), world.source.parts[1]->index.base(), world.parts[0]->index.base(), world.parts[1]->index.base(), world.parts[2]->index.base(),
		world.source.industries[0]->index.base(), world.source.industries[1]->index.base(), world.stations[0]->index.base(), world.stations[1]->index.base(), world.stations[2]->index.base(), world.stations[3]->index.base(),
		world.power.base(), world.source.deposit.base(), before, path.string());
}

/** @param world Original ordinary-loaded paid participants. @param expected Accepted ready tick/cash/loan/original reserve/minimum useful receipt/source income. @param paths Invocation-owned useful-world output. @param save_files Retain real loaded/final worlds and their untouched next complete opportunities. Earn physical modal transit, useful intake/payment and whole empty home returns while conserving continuing extraction. */
static void EarnShieldUsefulRoute(const ShieldUsefulWorld &world, const std::vector<uint64_t> &expected, const TestArtifactPaths &paths, bool save_files)
{
	const auto &company = *Company::Get(_current_company); const auto cargo = world.source.cargo;
	REQUIRE(TimerGameTick::counter == expected[0]); REQUIRE(company.money.base() == static_cast<int64_t>(expected[1])); REQUIRE(company.current_loan.base() == static_cast<int64_t>(expected[2]));
	REQUIRE(ShieldNativeAccounts(company, cargo).first.base() == static_cast<int64_t>(expected[5])); REQUIRE(ShieldNativeAccounts(company, cargo).second == 0);
	REQUIRE(world.source.industries[0]->produced[0].waiting == expected[4]); REQUIRE(CargoPacket::GetNumItems() == 0);
	REQUIRE(GetUtilityPowerProducer(world.power)->consumed_lifetime_j == 0); CheckShieldUsefulConservation(world, expected[3]);
	const std::array fronts{world.source.parts[0], world.parts[0], world.parts[1]};
	const std::array homes{world.feed_depots[0], world.tube_depots[0], world.delivery_depots[0]};
	for (size_t index = 0; index < fronts.size(); ++index) {
		REQUIRE(fronts[index]->IsChainInDepot()); REQUIRE(fronts[index]->tile == homes[index]); REQUIRE(fronts[index]->vehstatus.Test(VehState::Stopped));
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, fronts[index]->index, true).Succeeded());
	}
	std::array<bool, 5> scenes{}, saved{}; std::array<bool, 3> returned{}; uint ticks = 0; std::optional<uint> pending;
	// Real orders/loading/source service must earn all five physical modal
	// scenes and final intake. The bounded loop is a diagnostic limit, never a
	// hidden work cap, rating grant, arrived order or replacement source packet.
	while (ticks++ < 24000) {
		RunShieldUsefulOpportunity(world); CheckShieldUsefulConservation(world, expected[3]);
		// A pending baseline is the next complete original opportunity before
		// any other scene observation or Stop command. Saving adds no private tick.
		if (pending.has_value()) { SaveShieldUsefulWorld(world, paths, *pending, 1); pending.reset(); }
		const auto account = ShieldNativeAccounts(company, cargo);
		if (account.second == 0) REQUIRE(account.first.base() == static_cast<int64_t>(expected[5]));
		const std::array eligible{
			world.source.parts[1]->cargo.StoredCount() > 0 && GetTileLayer(world.source.parts[1]->tile) == MapLayer::DeepUnderground,
			world.source.parts[1]->cargo.StoredCount() > 0 && GetTileLayer(world.source.parts[1]->tile) == MapLayer::Underground,
			world.parts[0]->cargo.StoredCount() > 0 && world.parts[0]->cur_speed > 0,
			world.parts[2]->cargo.StoredCount() > 0 && GetTileLayer(world.parts[2]->tile) == MapLayer::Underground,
			world.parts[2]->cargo.StoredCount() > 0 && GetTileLayer(world.parts[2]->tile) == MapLayer::Surface};
		// Scene labels only record the first genuine loaded native state,
		// including partial loading. They cannot move any body or cargo owner.
		for (size_t scene = 0; scene < scenes.size(); ++scene) if (eligible[scene] && !scenes[scene]) {
			scenes[scene] = true; const auto *part = scene < 2 ? world.source.parts[1] : world.parts[scene == 2 ? 0 : 2];
			fmt::print("SHIELD_ROUTE_SCENE scene={} tick={} cargo={} tile={}\n", scene, TimerGameTick::counter, part->cargo.StoredCount(), part->tile.base());
		}
		if (save_files) {
			// Preserve each first eligible original loaded state once, keeping
			// its next shared opportunity ahead of another boundary observation.
			for (uint scene = 0; scene < saved.size(); ++scene) {
				if (saved[scene] || !eligible[scene] || pending.has_value()) continue;
				SaveShieldUsefulWorld(world, paths, scene, 0); saved[scene] = true; pending = scene;
			}
		}
		// Only genuine useful intake/payment may enable return markers. Initial
		// depot presence or a front ahead of an outside/loaded trailer cannot
		// satisfy a whole empty home return; Stop follows actual stored arrival.
		if (account.second >= expected[4] && account.first.base() > static_cast<int64_t>(expected[5])) {
			for (size_t index = 0; index < fronts.size(); ++index) {
				const auto *hold = index == 0 ? world.source.parts[1] : world.parts[index == 1 ? 0 : 2];
				if (!returned[index] && fronts[index]->IsChainInDepot() && fronts[index]->tile == homes[index] && hold->cargo.StoredCount() == 0 && fronts[index]->cargo_payment == nullptr) {
					if (!fronts[index]->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, fronts[index]->index, true).Succeeded());
					returned[index] = true; fmt::print("SHIELD_ROUTE_RETURN role={} tick={}\n", index, TimerGameTick::counter);
				}
			}
			if (std::ranges::all_of(returned, std::identity{})) break;
		}
	}
	for (const auto *front : fronts) fmt::print("SHIELD_ROUTE_FINAL {}\n", DescribeTrain(front));
	REQUIRE(ticks < 24000); REQUIRE(std::ranges::all_of(scenes, std::identity{})); REQUIRE(std::ranges::all_of(returned, std::identity{}));
	const auto final = ShieldNativeAccounts(company, cargo); REQUIRE(final.second >= expected[4]); REQUIRE(final.first.base() > static_cast<int64_t>(expected[5]));
	REQUIRE(GetUtilityPowerProducer(world.power)->consumed_lifetime_j > 0);
	// Whole empty stopped routes cannot replay a receiver intake or payment.
	// Native mine work/dispatch still continue for two economy days; every new
	// unit must remain in the same actual reserve/buffer/queue equation.
	for (uint empty = 0; empty < 148; ++empty) {
		RunShieldUsefulOpportunity(world); CheckShieldUsefulConservation(world, expected[3]);
		for (const auto *front : fronts) { REQUIRE(front->IsChainInDepot()); REQUIRE(front->vehstatus.Test(VehState::Stopped)); }
	}
	REQUIRE(ShieldNativeAccounts(company, cargo) == final); REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{1985});
	if (save_files) {
		REQUIRE(std::ranges::all_of(saved, std::identity{})); REQUIRE_FALSE(pending.has_value());
		SaveShieldUsefulWorld(world, paths, 5, 0);
		RunShieldUsefulOpportunity(world); CheckShieldUsefulConservation(world, expected[3]);
		SaveShieldUsefulWorld(world, paths, 5, 1);
	}
	const auto before = CaptureShieldReadyWorld(world); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
	const auto path = paths.Output("saves", "shield-useful.sav");
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureShieldReadyWorld(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
	uint64_t waiting = 0; for (const auto *station : Station::Iterate()) waiting += station->goods[cargo].TotalCount();
	const auto *mine = GetLayerMineState(world.source.industries[0]->index);
	fmt::print("SHIELD_USEFUL year=1985 original_tick={} tick={} route_ticks={} delivered={} receiver={} income={} cash={} loan={} consumed={} reserve={} extracted={} dispatched={} source_buffer={} waiting={} packets={} quarter_history={} scenes=5 returns=3 empty_ticks=148 path={}\n",
		expected[0], TimerGameTick::counter, ticks, final.second, ShieldNativeInput(*world.source.industries[1], cargo), final.first.base() - static_cast<int64_t>(expected[5]), company.money.base(), company.current_loan.base(),
		GetUtilityPowerProducer(world.power)->consumed_lifetime_j, GetGeologyWorldState().deposits.Get(world.source.deposit)->remaining_reserve, mine->extracted, mine->dispatched,
		world.source.industries[0]->produced[0].waiting, waiting, CargoPacket::GetNumItems(), company.num_valid_stat_ent, path.string());
}

/** @param save_files Also retain actual modal/final worlds and precisely their next complete native opportunities. Normal application loading alone acquires every original participant. */
static void ObserveShieldUsefulRoute(bool save_files)
{
	const char *input = std::getenv("OTTD_SHIELD_ROUTE_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", save_files ? "shield-files" : "shield-route"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Load exact original ready participants only; the paid construction builder is forbidden in this movement observer. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; AutoRestoreBackup pause{_pause_mode, PauseModes{}};
			const auto world = LoadShieldUsefulWorld(); const auto expected = ShieldSourceValues("OTTD_SHIELD_USEFUL_VALUES", 6);
			EarnShieldUsefulRoute(world, expected, paths, save_files);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_shield_route_observer, &inspect}; IConsole::CmdRegister(save_files ? "shield_useful_files_check" : "shield_useful_check", ObserveShieldRoute);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** Actual real freight legs, useful native ELEC intake/payment and whole empty returns come from full native movement while finite extraction continues. */
TEST_CASE("Original Shield earns real Deep Underground Surface freight with conserved native extraction", "[.tube-shield-useful-native]") { ObserveShieldUsefulRoute(false); }

/** Pure native saving retains six original loaded/final worlds and precisely their next complete common native opportunities. */
TEST_CASE("Original useful Shield writes complete modal worlds and untouched native baselines", "[.tube-shield-useful-files-native]") { ObserveShieldUsefulRoute(true); }

/** @param world Exact ordinary-loaded original participants. @param expected Original writer's complete canonical observations. Diagnose any changed field without repairing a native loader or its owner state. */
static void CheckShieldUsefulSnapshot(const ShieldUsefulWorld &world, std::string_view expected)
{
	const auto actual = CaptureShieldUsefulWorld(world);
	if (actual != expected) fmt::print("SHIELD_SNAPSHOT actual={}\n", actual);
	REQUIRE(actual == expected);
}

/** Normal application loading and copied preview preserve all original owners before an explicitly requested complete shared opportunity. */
TEST_CASE("Original useful Shield files preserve native owners preview and complete continuation", "[.tube-shield-useful-native-file]")
{
	const char *input = std::getenv("OTTD_SHIELD_USEFUL_INPUT"), *action_text = std::getenv("OTTD_SHIELD_USEFUL_ACTION"), *snapshot_text = std::getenv("OTTD_SHIELD_USEFUL_SNAPSHOT");
	REQUIRE(input != nullptr); REQUIRE(action_text != nullptr); REQUIRE(snapshot_text != nullptr);
	const std::string_view action{action_text}; const std::string expected{snapshot_text};
	// Only the named file modes may execute. Ordinary inspection cannot select
	// construction or accidentally consume a native movement/source opportunity.
	REQUIRE((action == "inspect" || action == "step" || action == "raster"));
	const auto paths = GetTestCaseArtifactPaths("tubes", "shield-files"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Borrow original normal-loaded pools, preserve preview/client entropy and share exactly the writer's continuation driver. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; auto world = LoadShieldUsefulWorld(); CheckShieldUsefulSnapshot(world, expected);
			const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			world = LoadShieldUsefulWorld(); CheckShieldUsefulSnapshot(world, expected);
			CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
			const auto values = ShieldSourceValues("OTTD_SHIELD_USEFUL_VALUES", 6); CheckShieldUsefulConservation(world, values[3]); const auto tick = TimerGameTick::counter;
			if (action == "step") {
				AutoRestoreBackup pause{_pause_mode, PauseModes{}};
				RunShieldUsefulOpportunity(world); CheckShieldUsefulConservation(world, values[3]); CHECK(TimerGameTick::counter == tick + 1);
			}
			if (action == "raster") {
				/** Advance every original clock/front/industry and genuine finite dispatch exactly once, preserving the accepted useful-route conservation equation. */
				CaptureUsefulShield(world, paths, [&] { RunShieldUsefulOpportunity(world); CheckShieldUsefulConservation(world, values[3]); });
			}
			// Each complete real front/tail must retain map membership; matching
			// packet or power totals cannot hide an absent conventional participant.
			for (const auto *front : {world.source.parts[0], world.parts[0], world.parts[1]}) {
				REQUIRE(IsTrainOnItsTiles(front)); REQUIRE_FALSE(front->vehstatus.Test(VehState::Crashed));
			}
			const auto before = CaptureShieldUsefulWorld(world); const std::array save_interactive{_interactive_random.state[0], _interactive_random.state[1]};
			const auto path = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action));
			{
				AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
				AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
			}
			CHECK(CaptureShieldUsefulWorld(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == save_interactive));
			fmt::print("SHIELD_READ action={} original_parts=5 fronts=3 full_native_clock=true preview_pure=true snapshot={} path={}\n", action, before, path.string());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_shield_route_observer, &inspect}; IConsole::CmdRegister("shield_useful_file_check", ObserveShieldRoute);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
