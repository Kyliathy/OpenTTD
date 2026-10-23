/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_forge_route.cpp Paid Forge modal preparation and actual useful freight in the original manufactured-cargo world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15FDB/15FDC/15FEB/15FEC/15FEE.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * All raw/recipe/cargo/account/power owners remain actual; prepared floor/machinery/Connected shafts remain explicit boundaries.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "forge_useful_route_fixture.h"
#include "forge_useful_world_snapshot.h"
#include "forge_useful_raster_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

/** @param raw Actual original raw/CFMT participants. @return Every original material/physical/history/queue/packet/receipt observation that new paid commands must retain, excluding legitimate cash/expense, pool, new-processor and floor changes. */
static std::string CaptureForgeRetainedOwners(const ForgeRawRouteWorld &raw)
{
	const auto snapshot = CaptureForgeRawDelivery(raw); std::string result; size_t begin = 0;
	// This read-only selection protects actual original owners through new
	// command-funded construction. Native costs/new participants/prepared floor
	// have their own assertions; they cannot exempt original cargo or histories.
	while (begin < snapshot.size()) {
		const auto end = snapshot.find(',', begin); REQUIRE(end != std::string::npos); const std::string_view field{snapshot.data() + begin, end - begin};
		const bool retained = field.starts_with("part") || field.starts_with("industry0_") || field.starts_with("industry1_") || field.starts_with("industry2_") ||
			field.starts_with("mine0_") || field.starts_with("mine1_") || field.starts_with("raw_station") || field.starts_with("packet") || field.starts_with("reserve") ||
			field.starts_with("initial_reserve") || field.starts_with("deposit") || field.starts_with("home") || field.starts_with("quarter") || field.starts_with("current_income=") || field.starts_with("current_cargo");
		if (retained) { result.append(field); result += ','; }
		begin = end + 1;
	}
	return result;
}

/** @param world Real original source and newly paid modal owners. @return Named original/new participants, stations, receiver and electrical observations for pure ready saving. */
static std::string CaptureForgeReadyWorld(const ForgeUsefulWorld &world)
{
	std::string result = CaptureForgeRawDelivery(world.raw);
	/** @param name Unique native field name. @param value Exact integer bits, including losslessly transported signed Money. */
	const auto add = [&](std::string_view name, uint64_t value) { result += fmt::format("{}={},", name, value); };
	// All five newly purchased parts retain real native membership, physical
	// pose, ordinary order and hold/capacity owners through pure ready saving.
	for (size_t index = 0; index < world.parts.size(); ++index) {
		const auto &part = *world.parts[index]; const auto prefix = fmt::format("route_part{}_", index);
		add(prefix + "id", part.index.base()); add(prefix + "first", part.First()->index.base()); add(prefix + "engine", part.engine_type.base()); add(prefix + "owner", part.owner.base());
		add(prefix + "tile", part.tile.base()); add(prefix + "x", part.x_pos); add(prefix + "y", part.y_pos); add(prefix + "z", part.z_pos); add(prefix + "direction", to_underlying(part.direction)); add(prefix + "track", part.track.base());
		add(prefix + "cargo", part.cargo_type); add(prefix + "units", part.cargo.StoredCount()); add(prefix + "capacity", part.cargo_cap); add(prefix + "speed", part.cur_speed); add(prefix + "flags", part.flags.base()); add(prefix + "status", part.vehstatus.base());
		add(prefix + "order", to_underlying(part.current_order.GetType())); add(prefix + "destination", part.current_order.GetDestination().base()); add(prefix + "order_index", part.cur_real_order_index); add(prefix + "dest_tile", part.dest_tile.base());
	}
	for (size_t index = 0; index < world.stations.size(); ++index) {
		const auto &station = *world.stations[index]; const auto prefix = fmt::format("route_station{}_", index); add(prefix + "id", station.index.base()); add(prefix + "tile", station.xy.base()); add(prefix + "owner", station.owner.base());
		for (const auto cargo : world.raw.source.cargos) {
			const auto &entry = station.goods[cargo]; const auto name = prefix + fmt::format("cargo{}_", cargo);
			add(name + "units", entry.TotalCount()); add(name + "rating", entry.rating); add(name + "speed", entry.last_speed); add(name + "fraction", entry.amount_fract); add(name + "status", entry.status.base());
		}
	}
	add("receiver", world.receiver->index.base()); add("receiver_tile", world.receiver->location.tile.base()); add("receiver_type", world.receiver->type);
	// The actual new ELEC owns zero initial input/output. Its native processor
	// digest includes choice/work/history, independently of original CFMT stock.
	for (size_t index = 0; index < world.receiver->accepted.size(); ++index) add(fmt::format("receiver_input{}_units", index), world.receiver->accepted[index].waiting);
	for (size_t index = 0; index < world.receiver->produced.size(); ++index) add(fmt::format("receiver_output{}_units", index), world.receiver->produced[index].waiting);
	add("route_source", world.power.base()); add("route_utility", ComputeUtilityPowerDigest()); add("route_binding", ComputeTubePowerVehicleDigest()); add("route_invoice", ComputeTubePowerDigest());
	return result;
}

/** @param world Original owners and actual new paid route. @param expected Original tick/cash/loan/RFMT/unusedCoal/reserves/income. @param paths Invocation-owned native ready-world output. Require actual paid, whole stopped empty storage and unchanged genuine original materials before pure saving. */
static void SavePreparedForgeRoute(const ForgeUsefulWorld &world, const std::vector<uint64_t> &expected, const TestArtifactPaths &paths)
{
	const auto &raw = world.raw; const auto &source = raw.source; const auto &company = *Company::Get(_current_company);
	REQUIRE(TimerGameTick::counter == expected[0]); REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{2005}); REQUIRE(Vehicle::GetNumItems() == 9); REQUIRE(Industry::GetNumItems() == 4); REQUIRE(Station::GetNumItems() == 5); REQUIRE(CargoPacket::GetNumItems() == 2);
	REQUIRE(company.money > Money{0}); REQUIRE(company.money.base() < static_cast<int64_t>(expected[1])); REQUIRE(company.current_loan.base() == static_cast<int64_t>(expected[2]));
	REQUIRE(ForgeRawAccounts(company, source.cargos[0]).first.base() == static_cast<int64_t>(expected[7]));
	const auto *processor = GetLayerProcessorState(source.industries[2]->index); REQUIRE(processor != nullptr); REQUIRE(processor->produced.at(CargoLabel{"RFMT"}) == expected[3]); REQUIRE(processor->dispatched == 0);
	REQUIRE(ForgeRawInput(*source.industries[2], source.cargos[1]) == expected[4]);
	for (size_t index = 0; index < source.deposits.size(); ++index) REQUIRE(GetGeologyWorldState().deposits.Get(source.deposits[index])->remaining_reserve == expected[index + 5]);
	CheckForgeRawDelivery(raw, std::array<uint64_t, 2>{32000, 32000});
	const std::array fronts{source.parts[0], source.parts[2], world.parts[0], world.parts[2], world.parts[3]}; const std::array homes{source.homes[0], source.homes[1], world.feed_depots[0], world.tube_depots[0], world.delivery_depots[0]};
	// The two original raw keepers and three new modal fronts must be whole
	// stopped/empty at actual homes. A front ahead of an outside or loaded tail
	// cannot impersonate storage, paid useful service or a native arrival.
	for (size_t index = 0; index < fronts.size(); ++index) {
		REQUIRE(fronts[index]->IsChainInDepot()); REQUIRE(fronts[index]->tile == homes[index]); REQUIRE(fronts[index]->vehstatus.Test(VehState::Stopped)); REQUIRE(IsTrainOnItsTiles(fronts[index]));
		for (const auto *part = fronts[index]; part != nullptr; part = part->Next()) REQUIRE(part->cargo.StoredCount() == 0);
	}
	REQUIRE(world.parts[2]->engine_type == GetTubeEngine(13)); REQUIRE(world.parts[2]->Next() == nullptr); REQUIRE(world.parts[2]->cargo_cap == 80); REQUIRE(GetTileLayer(world.parts[2]->tile) == MapLayer::Underground);
	for (const uint index : {1, 2, 4}) REQUIRE(world.parts[index]->cargo_type == source.cargos[2]);
	REQUIRE(world.parts[1]->First() == world.parts[0]); REQUIRE(world.parts[4]->First() == world.parts[3]);
	for (const auto &entry : world.receiver->accepted) REQUIRE(entry.waiting == 0);
	for (const auto &entry : world.receiver->produced) REQUIRE(entry.waiting == 0);
	const auto *receiver = GetLayerProcessorState(world.receiver->index); REQUIRE(receiver != nullptr); REQUIRE(receiver->recipe == MultiLayerRecipeID{"ELEC"});
	for (const auto &[recipe, batches] : receiver->batches) REQUIRE(batches == 0);
	const auto *producer = GetUtilityPowerProducer(world.power); REQUIRE(producer != nullptr); REQUIRE(producer->consumed_lifetime_j == 0);
	const auto *binding = GetTubePowerVehicle(world.parts[2]->index); REQUIRE(binding != nullptr); REQUIRE(binding->source == world.power); REQUIRE(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	const auto portals = CaptureLayerPortalWorkRecords(); REQUIRE(portals.size() == 4);
	for (const auto &portal : portals) REQUIRE(portal.state == LayerPortalWorkState::Connected);
	const auto before = CaptureForgeReadyWorld(world); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]}; const auto path = paths.Output("saves", "forge-route-prepared.sav");
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureForgeReadyWorld(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
	fmt::print("FORGE_ROUTE_PREPARED tick={} cash={} loan={} rfmt={} unused_coal={} copper_reserve={} coal_reserve={} income={} consumed=0 new_parts={},{},{},{},{} receiver={} stations={},{},{},{} power={} portals={},{},{},{} snapshot={} path={}\n",
		TimerGameTick::counter, company.money.base(), company.current_loan.base(), processor->produced.at(CargoLabel{"RFMT"}), ForgeRawInput(*source.industries[2], source.cargos[1]),
		GetGeologyWorldState().deposits.Get(source.deposits[0])->remaining_reserve, GetGeologyWorldState().deposits.Get(source.deposits[1])->remaining_reserve, ForgeRawAccounts(company, source.cargos[0]).first.base(),
		world.parts[0]->index.base(), world.parts[1]->index.base(), world.parts[2]->index.base(), world.parts[3]->index.base(), world.parts[4]->index.base(), world.receiver->index.base(),
		world.stations[0]->index.base(), world.stations[1]->index.base(), world.stations[2]->index.base(), world.stations[3]->index.base(), world.power.base(),
		portals[0].pair_key.base(), portals[1].pair_key.base(), portals[2].pair_key.base(), portals[3].pair_key.base(), before, path.string());
}

static std::function<void()> *_forge_route_observer = nullptr; ///< Invocation-owned normal startup callback, never another saved source or cargo owner.

/** @param args Ordinary native console arguments after original manufactured-cargo loading. @return Admission; invoke only the current paid route observer. */
static bool ObserveForgeRoute(std::span<std::string_view> args)
{
	if (args.empty()) return true;
	REQUIRE(_forge_route_observer != nullptr); (*_forge_route_observer)(); return true;
}

/** Original finite sources/manufactured RFMT survive all paid modal purchases before any actual useful movement may be claimed. */
TEST_CASE("Original manufactured Forge world prepares paid stopped modal route", "[.tube-forge-route-prepare-native]")
{
	const char *input = std::getenv("OTTD_FORGE_ROUTE_INPUT"), *snapshot = std::getenv("OTTD_FORGE_ROUTE_SOURCE_SNAPSHOT"); REQUIRE(input != nullptr); REQUIRE(snapshot != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", "forge-route"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Ordinary original loading retains both finite sources/CFMT/raw carriers; native commands alone buy all new actual modal owners without advancing a source opportunity. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; AutoRestoreBackup pause{_pause_mode, PauseModes{}}; const auto raw = LoadForgeRawRouteWorld(); REQUIRE(CaptureForgeRawDelivery(raw) == snapshot);
			const auto retained = CaptureForgeRetainedOwners(raw); ForgeUsefulRouteBuilder builder(raw); REQUIRE(CaptureForgeRetainedOwners(raw) == retained);
			SavePreparedForgeRoute(builder.world, ShieldSourceValues("OTTD_FORGE_ROUTE_SOURCE_VALUES", 8), paths);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_forge_route_observer, &inspect}; IConsole::CmdRegister("forge_route_prepare_check", ObserveForgeRoute);
	const std::string config_path = config.string(); std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** @param world Actual original raw and paid modal owners. @param original_rfmt Genuine original manufactured stock, not a private production cap. @param unused_coal Original actual unused input retained while raw haulers stay stopped. Conserve every raw/RFMT unit, native recipe/account owner and original keeper during real movement. */
static void CheckForgeUsefulConservation(const ForgeUsefulWorld &world, uint64_t original_rfmt, uint64_t unused_coal)
{
	const auto &source = world.raw.source; const auto cargo = source.cargos[2]; const auto *processor = GetLayerProcessorState(source.industries[2]->index); REQUIRE(processor != nullptr);
	CheckForgeRawDelivery(world.raw, std::array<uint64_t, 2>{32000, 32000}, false);
	REQUIRE(processor->produced.at(CargoLabel{"RFMT"}) == original_rfmt); REQUIRE(ForgeRawInput(*source.industries[2], source.cargos[0]) == 0); REQUIRE(ForgeRawInput(*source.industries[2], source.cargos[1]) == unused_coal);
	// Only the new modal fleet moves. Both original raw keepers remain whole
	// stopped/empty, so no fresh raw input can impersonate this original RFMT.
	for (size_t index = 0; index < source.deposits.size(); ++index) {
		const auto *front = source.parts[index * 2]; REQUIRE(front->IsChainInDepot()); REQUIRE(front->tile == source.homes[index]); REQUIRE(front->vehstatus.Test(VehState::Stopped));
		REQUIRE(front->cargo.StoredCount() == 0); REQUIRE(source.parts[index * 2 + 1]->cargo.StoredCount() == 0);
	}
	const auto *receiver = GetLayerProcessorState(world.receiver->index); REQUIRE(receiver != nullptr); REQUIRE(receiver->recipe == MultiLayerRecipeID{"ELEC"});
	const auto rare = GetCargoTypeByLabel(CargoLabel{"RARE"}); REQUIRE(rare != INVALID_CARGO);
	REQUIRE(ForgeRawInput(*world.receiver, rare) == 0); REQUIRE(receiver->consumed.at(CargoLabel{"RFMT"}) == 0); REQUIRE(receiver->consumed.at(CargoLabel{"RARE"}) == 0);
	for (const auto &entry : world.receiver->produced) REQUIRE(entry.waiting == 0);
	REQUIRE(ForgeRawAccounts(*Company::Get(source.parts[0]->owner), cargo).second == ForgeRawInput(*world.receiver, cargo));
	// The new fronts retain native poses and the capsule remains local.
	// Ordinary locomotives alone carry both actual cross-layer freight legs.
	for (const uint index : {0, 2, 3}) { REQUIRE(IsTrainOnItsTiles(world.parts[index])); REQUIRE_FALSE(world.parts[index]->vehstatus.Test(VehState::Crashed)); }
	REQUIRE(GetTileLayer(world.parts[2]->tile) == MapLayer::Underground);
}

/** @param world Exact original paid modal participants. Advance one full native raw-source opportunity/all fronts, then dispatch only actual CFMT RFMT through earned Surface station service. */
static void RunForgeUsefulOpportunity(const ForgeUsefulWorld &world)
{
	RunForgeRawOpportunity(world.raw); DispatchLayerProcessorCargo(*world.raw.source.industries[2]);
}

/** @param world Actual original raw/modal participants. @param paths Invocation-owned original native outputs. @param scene Real loaded scene0..4 or useful final5. @param step Original0 or untouched complete next opportunity1. Pure-save every canonical owner and independently preserve unsaved client RNG. */
static void SaveForgeUsefulWorld(const ForgeUsefulWorld &world, const TestArtifactPaths &paths, uint scene, uint step)
{
	const auto before = CaptureForgeUsefulWorld(world); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]}; const auto path = paths.Output("saves", fmt::format("forge-useful-{}-{}.sav", scene, step));
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureForgeUsefulWorld(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
	fmt::print("FORGE_WORLD scene={} step={} tick={} snapshot={} path={}\n", scene, step, TimerGameTick::counter, before, path.string());
}

/** @param world Exact ordinary-loaded ready participants. @param expected Ready tick/cash/loan/original RFMT/unusedCoal/reserves/prior income. @param paths Invocation-owned final useful world. @param save_files Retain actual loaded/final originals and precisely their next complete native opportunities. Earn five loaded scenes, actual intake/payment and whole empty homes while finite sources continue normally. */
static void EarnForgeUsefulRoute(const ForgeUsefulWorld &world, const std::vector<uint64_t> &expected, const TestArtifactPaths &paths, bool save_files)
{
	const auto &company = *Company::Get(_current_company); const auto cargo = world.raw.source.cargos[2];
	REQUIRE(TimerGameTick::counter == expected[0]); REQUIRE(company.money.base() == static_cast<int64_t>(expected[1])); REQUIRE(company.current_loan.base() == static_cast<int64_t>(expected[2]));
	REQUIRE(ForgeRawAccounts(company, cargo).first.base() == static_cast<int64_t>(expected[7])); REQUIRE(ForgeRawAccounts(company, cargo).second == 0);
	REQUIRE(GetUtilityPowerProducer(world.power)->consumed_lifetime_j == 0); CheckForgeUsefulConservation(world, expected[3], expected[4]);
	const std::array fronts{world.parts[0], world.parts[2], world.parts[3]}; const std::array homes{world.feed_depots[0], world.tube_depots[0], world.delivery_depots[0]}; const std::array holds{world.parts[1], world.parts[2], world.parts[4]};
	// Native player commands start only the three new real fronts. Original
	// raw keepers, all inventory and every ordinary order retain their owners.
	for (size_t index = 0; index < fronts.size(); ++index) {
		REQUIRE(fronts[index]->IsChainInDepot()); REQUIRE(fronts[index]->tile == homes[index]); REQUIRE(fronts[index]->vehstatus.Test(VehState::Stopped)); REQUIRE(holds[index]->cargo.StoredCount() == 0);
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, fronts[index]->index, true).Succeeded());
	}
	std::array<bool, 5> scenes{}, saved{}; std::array<bool, 3> returned{}; uint ticks = 0; std::optional<uint> pending;
	// This diagnostic bound never manufactures cargo, grants service, moves a
	// body or caps work. Full native clocks/fronts/source/recipe dispatch must
	// earn actual conventional crossings, local powered travel and final intake.
	while (ticks < 24000) {
		RunForgeUsefulOpportunity(world); ++ticks; CheckForgeUsefulConservation(world, expected[3], expected[4]);
		// Saving never adds a private tick. A pending baseline is this next
		// complete ordinary opportunity before another observation or Stop.
		if (pending.has_value()) { SaveForgeUsefulWorld(world, paths, *pending, 1); pending.reset(); }
		const auto account = ForgeRawAccounts(company, cargo);
		if (account.second == 0) REQUIRE(account.first.base() == static_cast<int64_t>(expected[7]));
		const std::array eligible{
			world.parts[1]->cargo.StoredCount() > 0 && GetTileLayer(world.parts[1]->tile) == MapLayer::Surface,
			world.parts[1]->cargo.StoredCount() > 0 && GetTileLayer(world.parts[1]->tile) == MapLayer::Underground,
			world.parts[2]->cargo.StoredCount() > 0 && world.parts[2]->cur_speed > 0,
			world.parts[4]->cargo.StoredCount() > 0 && GetTileLayer(world.parts[4]->tile) == MapLayer::Underground,
			world.parts[4]->cargo.StoredCount() > 0 && GetTileLayer(world.parts[4]->tile) == MapLayer::Surface};
		// First native loaded observations include legitimate partial holds.
		// Scene bookkeeping cannot alter the underlying packet or physical pose.
		for (size_t scene = 0; scene < scenes.size(); ++scene) if (eligible[scene] && !scenes[scene]) {
			const auto *part = world.parts[scene < 2 ? 1 : scene == 2 ? 2 : 4]; scenes[scene] = true;
			fmt::print("FORGE_ROUTE_SCENE scene={} tick={} cargo={} tile={} layer={}\n", scene, TimerGameTick::counter, part->cargo.StoredCount(), part->tile.base(), to_underlying(GetTileLayer(part->tile)));
		}
		if (save_files) {
			// Retain each first eligible original once. Pending next baselines
			// take precedence so another save cannot hide an intervening stop.
			for (uint scene = 0; scene < saved.size(); ++scene) {
				if (saved[scene] || !eligible[scene] || pending.has_value()) continue;
				SaveForgeUsefulWorld(world, paths, scene, 0); saved[scene] = true; pending = scene;
			}
		}
		// Real useful intake and new final freight revenue enable stopping only
		// after whole empty home storage and payment settlement. Initial home
		// presence or a front ahead of an outside trailer never counts as return.
		if (account.second >= 20 && account.first.base() > static_cast<int64_t>(expected[7])) {
			for (size_t index = 0; index < returned.size(); ++index) {
				if (!returned[index] && fronts[index]->IsChainInDepot() && fronts[index]->tile == homes[index] && fronts[index]->cargo.StoredCount() == 0 && holds[index]->cargo.StoredCount() == 0 && fronts[index]->cargo_payment == nullptr) {
					if (!fronts[index]->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, fronts[index]->index, true).Succeeded());
					returned[index] = true; fmt::print("FORGE_ROUTE_RETURN role={} tick={}\n", index, TimerGameTick::counter);
				}
			}
			if (std::ranges::all_of(returned, std::identity{})) break;
		}
	}
	for (const auto *front : fronts) fmt::print("FORGE_ROUTE_FINAL {}\n", DescribeTrain(front));
	REQUIRE(ticks < 24000); REQUIRE(std::ranges::all_of(scenes, std::identity{})); REQUIRE(std::ranges::all_of(returned, std::identity{}));
	const auto final = ForgeRawAccounts(company, cargo); REQUIRE(final.second >= 20); REQUIRE(final.first.base() > static_cast<int64_t>(expected[7])); REQUIRE(GetUtilityPowerProducer(world.power)->consumed_lifetime_j > 0);
	// After three real empty stops, two more economy days must retain exact
	// useful intake/payment. Both finite mines, raw queues and legitimate work
	// still run normally; no stock, counter or source eligibility is normalized.
	for (uint empty = 0; empty < 148; ++empty) {
		RunForgeUsefulOpportunity(world); CheckForgeUsefulConservation(world, expected[3], expected[4]);
		REQUIRE(ForgeRawAccounts(company, cargo) == final);
		for (size_t index = 0; index < fronts.size(); ++index) { REQUIRE(fronts[index]->IsChainInDepot()); REQUIRE(fronts[index]->tile == homes[index]); REQUIRE(fronts[index]->vehstatus.Test(VehState::Stopped)); REQUIRE(holds[index]->cargo.StoredCount() == 0); }
	}
	REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{2005}); REQUIRE(company.money > Money{0}); REQUIRE(company.current_loan == Money{0});
	if (save_files) { REQUIRE(std::ranges::all_of(saved, std::identity{})); REQUIRE_FALSE(pending.has_value()); SaveForgeUsefulWorld(world, paths, 5, 0); }
	const auto before = CaptureForgeReadyWorld(world); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]}; const auto path = paths.Output("saves", "forge-useful.sav");
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureForgeReadyWorld(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive)); CheckForgeUsefulConservation(world, expected[3], expected[4]);
	uint64_t waiting = 0; for (const auto *station : Station::Iterate()) waiting += station->goods[cargo].TotalCount();
	uint64_t source_buffer = 0; for (const auto &entry : world.raw.source.industries[2]->produced) if (entry.cargo == cargo) source_buffer += entry.waiting;
	fmt::print("FORGE_USEFUL year=2005 original_tick={} tick={} route_ticks={} rfmt={} delivered={} receiver={} source_buffer={} waiting={} income={} total_income={} cash={} loan={} consumed={} packets={} quarters={} scenes=5 returns=3 empty_ticks=148 snapshot={} path={}\n",
		expected[0], TimerGameTick::counter, ticks, expected[3], final.second, ForgeRawInput(*world.receiver, cargo), source_buffer, waiting, final.first.base() - static_cast<int64_t>(expected[7]), final.first.base(), company.money.base(), company.current_loan.base(),
		GetUtilityPowerProducer(world.power)->consumed_lifetime_j, CargoPacket::GetNumItems(), company.num_valid_stat_ent, before, path.string());
	// Retain separate actual raw material totals after continued extraction;
	// old queues and unused Coal are legitimate owners, not disposable leftovers.
	const auto *processor = GetLayerProcessorState(world.raw.source.industries[2]->index);
	for (size_t index = 0; index < world.raw.source.deposits.size(); ++index) {
		const auto *mine = GetLayerMineState(world.raw.source.industries[index]->index); uint64_t raw_waiting = 0;
		for (const auto *station : Station::Iterate()) raw_waiting += station->goods[world.raw.source.cargos[index]].TotalCount();
		fmt::print("FORGE_USEFUL_RAW role={} reserve={} extracted={} dispatched={} buffer={} waiting={} delivered={} unused={} consumed={}\n", index,
			GetGeologyWorldState().deposits.Get(world.raw.source.deposits[index])->remaining_reserve, mine->extracted, mine->dispatched, world.raw.source.industries[index]->produced[0].waiting, raw_waiting,
			ForgeRawAccounts(company, world.raw.source.cargos[index]).second, ForgeRawInput(*world.raw.source.industries[2], world.raw.source.cargos[index]), processor->consumed.at(CargoSpec::Get(world.raw.source.cargos[index])->label));
	}
	// The original useful output/metrics above stay at the accepted final
	// tick. Only the explicit writer now earns its untouched final baseline.
	if (save_files) { RunForgeUsefulOpportunity(world); CheckForgeUsefulConservation(world, expected[3], expected[4]); SaveForgeUsefulWorld(world, paths, 5, 1); }
}

/** @param save_files Retain each original real loaded/final world and untouched next opportunity; ordinary loading alone obtains all existing participants. */
static void ObserveForgeUsefulRoute(bool save_files)
{
	const char *input = std::getenv("OTTD_FORGE_ROUTE_INPUT"); REQUIRE(input != nullptr); const auto paths = GetTestCaseArtifactPaths("tubes", save_files ? "forge-files" : "forge-route"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Borrow exact ordinary-loaded prepared owners; no raw/modal builder or packet/rating/pose/order/clock/account repair may run in actual useful movement. */
	std::function<void()> inspect = [&] {
		try { AutoRestoreBackup actor{_current_company, _local_company}; AutoRestoreBackup pause{_pause_mode, PauseModes{}}; const auto world = LoadForgeUsefulWorld(); EarnForgeUsefulRoute(world, ShieldSourceValues("OTTD_FORGE_USEFUL_VALUES", 8), paths, save_files); }
		catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_forge_route_observer, &inspect}; IConsole::CmdRegister(save_files ? "forge_useful_files_check" : "forge_useful_check", ObserveForgeRoute);
	const std::string config_path = config.string(); std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** Native commands/full shared opportunities alone earn original RFMT useful freight, paid capsule travel and three whole empty returns. */
TEST_CASE("Original Forge RFMT earns actual useful modal freight and empty returns", "[.tube-forge-useful-native]") { ObserveForgeUsefulRoute(false); }

/** Genuine original modal/final states retain pure native saves and exactly their next complete shared opportunities. */
TEST_CASE("Original Forge useful route writes actual modal native worlds", "[.tube-forge-useful-files-native]") { ObserveForgeUsefulRoute(true); }

/** @param world Actual ordinary-loaded original participants, including native saved visual effects. @param expected Original writer's complete canonical observations. Diagnose changed owners without repairing a loader, effect lifetime or inventory. */
static void CheckForgeUsefulSnapshot(const ForgeUsefulWorld &world, std::string_view expected)
{
	const auto actual = CaptureForgeUsefulWorld(world);
	if (actual != expected) fmt::print("FORGE_SNAPSHOT actual={}\n", actual);
	REQUIRE(actual == expected);
}

/** Ordinary loading/copied preview retains all original train/effect/industry/packet owners before an explicit complete native opportunity. */
TEST_CASE("Original useful Forge files preserve native owners preview and full continuation", "[.tube-forge-useful-native-file]")
{
	const char *input = std::getenv("OTTD_FORGE_USEFUL_INPUT"), *action_text = std::getenv("OTTD_FORGE_USEFUL_ACTION"), *snapshot_text = std::getenv("OTTD_FORGE_USEFUL_SNAPSHOT");
	REQUIRE(input != nullptr); REQUIRE(action_text != nullptr); REQUIRE(snapshot_text != nullptr); const std::string_view action{action_text}; const std::string expected{snapshot_text};
	// Ordinary inspect/resave cannot enter construction or consume a source,
	// movement, effect-animation or power opportunity. Explicit step or raster
	// travel may advance; viewport drawing and picking themselves remain pure.
	REQUIRE((action == "inspect" || action == "step" || action == "raster"));
	const auto paths = GetTestCaseArtifactPaths("tubes", "forge-files"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Borrow real ordinary-loaded pools, preserve copied preview/client entropy, and share precisely the original writer's full native continuation driver. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; auto world = LoadForgeUsefulWorld(); CheckForgeUsefulSnapshot(world, expected);
			const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			world = LoadForgeUsefulWorld(); CheckForgeUsefulSnapshot(world, expected); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
			const auto values = ShieldSourceValues("OTTD_FORGE_USEFUL_VALUES", 8); CheckForgeUsefulConservation(world, values[3], values[4]); const auto tick = TimerGameTick::counter;
			if (action == "step") {
				AutoRestoreBackup pause{_pause_mode, PauseModes{}}; RunForgeUsefulOpportunity(world); CheckForgeUsefulConservation(world, values[3], values[4]); CHECK(TimerGameTick::counter == tick + 1);
			}
			if (action == "raster") {
				/** Advance every original clock/front/effect/source and genuine CFMT dispatch once, retaining both finite raw equations, original RFMT and actual paid power. */
				CaptureUsefulForge(world, paths, [&] { RunForgeUsefulOpportunity(world); CheckForgeUsefulConservation(world, values[3], values[4]); });
			}
			const auto before = CaptureForgeUsefulWorld(world); const std::array save_interactive{_interactive_random.state[0], _interactive_random.state[1]};
			const auto path = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action));
			{
				AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
			}
			CHECK(CaptureForgeUsefulWorld(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == save_interactive));
			fmt::print("FORGE_READ action={} original_parts=9 fronts=5 native_effects=true full_native_clock=true preview_pure=true snapshot={} path={}\n", action, before, path.string());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_forge_route_observer, &inspect}; IConsole::CmdRegister("forge_useful_file_check", ObserveForgeRoute);
	const std::string config_path = config.string(); std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
