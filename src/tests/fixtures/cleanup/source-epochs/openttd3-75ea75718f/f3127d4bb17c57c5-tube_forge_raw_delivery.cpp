/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_forge_raw_delivery.cpp Actual finite CORE/Coal carriage, native CFMT manufacture and whole empty returns.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15FCC/15FCD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * All original raw sources continue normally; prepared floor/Connected shafts remain disclosed boundaries.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "forge_raw_delivery_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../vehicle_cmd.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

/** @param world Exact ordinary-loaded paid world. @param expected Accepted tick/cash/loan/CORE/Coal/remaining reserves/minimum RFMT. @param paths Invocation-owned output. Earn all physical input scenes, genuine native batches and whole empty home returns without repairing an owner. */
static void EarnForgeRawDelivery(const ForgeRawRouteWorld &world, const std::vector<uint64_t> &expected, const TestArtifactPaths &paths)
{
	const auto &source = world.source; const auto &company = *Company::Get(_current_company);
	const std::array reserves{expected[3] + expected[5], expected[4] + expected[6]};
	REQUIRE(TimerGameTick::counter == expected[0]); REQUIRE(company.money.base() == static_cast<int64_t>(expected[1])); REQUIRE(company.current_loan.base() == static_cast<int64_t>(expected[2]));
	REQUIRE(CargoPacket::GetNumItems() == 0); REQUIRE(ForgeRawAccounts(company, source.cargos[0]).first == Money{0}); CheckForgeRawDelivery(world, reserves);
	// Initial actual storage is only a starting boundary; native commands and
	// ordinary pickup/delivery/service orders must earn every later arrival.
	for (size_t index = 0; index < source.deposits.size(); ++index) {
		const auto *front = source.parts[index * 2]; REQUIRE(front->IsChainInDepot()); REQUIRE(front->tile == source.homes[index]); REQUIRE(front->vehstatus.Test(VehState::Stopped));
		REQUIRE(source.industries[index]->produced[0].waiting == expected[index + 3]); REQUIRE(ForgeRawAccounts(company, source.cargos[index]).second == 0);
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, front->index, true).Succeeded());
	}
	std::array<bool, 4> scenes{}; std::array<bool, 2> returned{}; uint ticks = 0; bool made = false;
	// This bound diagnoses an unreachable route or stalled real producer. It
	// neither caps extraction/manufacturing nor grants service, cargo or work.
	while (ticks < 24000) {
		RunForgeRawOpportunity(world); ++ticks; CheckForgeRawDelivery(world, reserves);
		// Record the first genuine loaded wagon in each real layer. Native
		// partial loads count as physical carriage without assigning a full hold.
		for (size_t scene = 0; scene < scenes.size(); ++scene) {
			const auto *wagon = source.parts[(scene / 2) * 2 + 1]; const auto layer = scene % 2 == 0 ? MapLayer::Underground : MapLayer::Surface;
			if (!scenes[scene] && wagon->cargo.StoredCount() > 0 && GetTileLayer(wagon->tile) == layer) {
				scenes[scene] = true; fmt::print("FORGE_RAW_SCENE scene={} tick={} cargo={} tile={} layer={}\n", scene, TimerGameTick::counter, wagon->cargo.StoredCount(), wagon->tile.base(), to_underlying(layer));
			}
		}
		const auto *processor = GetLayerProcessorState(source.industries[2]->index); const auto rfmt = processor->produced.at(CargoLabel{"RFMT"});
		// First output must follow actual native intake/recipe commits; the
		// marker observes manufacture and never credits a fixture-made product.
		if (!made && rfmt > 0) {
			made = true; fmt::print("FORGE_RAW_RECIPE tick={} rfmt={} batches={}\n", TimerGameTick::counter, rfmt, processor->batches.at(MultiLayerRecipeID{"CFMT"}));
		}
		// Useful genuine RFMT enables stopping only at a whole empty home
		// arrival after native payment settlement. Initial storage cannot count.
		if (rfmt >= expected[7]) {
			for (size_t index = 0; index < returned.size(); ++index) {
				const auto *front = source.parts[index * 2], *wagon = source.parts[index * 2 + 1];
				if (!returned[index] && front->IsChainInDepot() && front->tile == source.homes[index] && front->cargo.StoredCount() == 0 && wagon->cargo.StoredCount() == 0 && front->cargo_payment == nullptr) {
					if (!front->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, front->index, true).Succeeded());
					returned[index] = true; fmt::print("FORGE_RAW_RETURN role={} tick={}\n", index, TimerGameTick::counter);
				}
			}
			if (std::ranges::all_of(returned, std::identity{})) break;
		}
	}
	for (size_t index = 0; index < returned.size(); ++index) fmt::print("FORGE_RAW_FINAL {}\n", DescribeTrain(source.parts[index * 2]));
	REQUIRE(ticks < 24000); REQUIRE(std::ranges::all_of(scenes, std::identity{})); REQUIRE(std::ranges::all_of(returned, std::identity{})); REQUIRE(made);
	const std::array accounts{ForgeRawAccounts(company, source.cargos[0]), ForgeRawAccounts(company, source.cargos[1])}; REQUIRE(accounts[0].first > Money{0});
	// Stopped whole empty haulers cannot replay receipts/payments. Native
	// finite mine work, dispatch and any legitimate remaining CFMT batches
	// continue for two complete economy days; no quantity is normalized.
	for (uint empty = 0; empty < 148; ++empty) {
		RunForgeRawOpportunity(world); CheckForgeRawDelivery(world, reserves);
		for (size_t index = 0; index < returned.size(); ++index) {
			const auto *front = source.parts[index * 2]; REQUIRE(front->IsChainInDepot()); REQUIRE(front->vehstatus.Test(VehState::Stopped));
			REQUIRE(ForgeRawAccounts(company, source.cargos[index]) == accounts[index]); REQUIRE(source.parts[index * 2 + 1]->cargo.StoredCount() == 0);
		}
	}
	REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{2005}); REQUIRE(company.money > Money{0}); REQUIRE(company.current_loan == Money{0});
	const auto *processor = GetLayerProcessorState(source.industries[2]->index); REQUIRE(processor->produced.at(CargoLabel{"RFMT"}) >= expected[7]);
	const auto before = CaptureForgeRawDelivery(world); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
	const auto path = paths.Output("saves", "forge-rfmt-produced.sav");
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureForgeRawDelivery(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
	fmt::print("FORGE_RFMT year=2005 original_tick={} tick={} route_ticks={} rfmt={} batches={} income={} cash={} loan={} packets={} quarters={} scenes=4 returns=2 empty_ticks=148 snapshot={} path={}\n",
		expected[0], TimerGameTick::counter, ticks, processor->produced.at(CargoLabel{"RFMT"}), processor->batches.at(MultiLayerRecipeID{"CFMT"}), accounts[0].first.base(), company.money.base(), company.current_loan.base(), CargoPacket::GetNumItems(), company.num_valid_stat_ent, before, path.string());
	// Independent per-label facts expose every unused raw input and continued
	// source/queue owner instead of assuming equal trips or empty final packets.
	for (size_t index = 0; index < returned.size(); ++index) {
		const auto *mine = GetLayerMineState(source.industries[index]->index); uint64_t waiting = 0;
		for (const auto *station : Station::Iterate()) waiting += station->goods[source.cargos[index]].TotalCount();
		fmt::print("FORGE_RAW_MATERIAL role={} reserve={} extracted={} dispatched={} buffer={} waiting={} delivered={} unused={} consumed={}\n", index,
			GetGeologyWorldState().deposits.Get(source.deposits[index])->remaining_reserve, mine->extracted, mine->dispatched, source.industries[index]->produced[0].waiting,
			waiting, accounts[index].second, ForgeRawInput(*source.industries[2], source.cargos[index]), processor->consumed.at(CargoSpec::Get(source.cargos[index])->label));
	}
}

static std::function<void()> *_forge_raw_delivery_observer = nullptr; ///< Invocation-owned startup observation; never another saved producer or cargo owner.

/** @param args Ordinary console arguments after native original loading. @return Admission; execute only the current invocation's real delivery observer. */
static bool ObserveForgeRawDelivery(std::span<std::string_view> args)
{
	if (args.empty()) return true;
	REQUIRE(_forge_raw_delivery_observer != nullptr); (*_forge_raw_delivery_observer)(); return true;
}

/** Actual original raw wagons physically feed native CFMT and return whole empty while both finite mines continue normally. */
TEST_CASE("Original Forge inputs physically manufacture native RFMT and return empty", "[.tube-forge-raw-delivery-native]")
{
	const char *input = std::getenv("OTTD_FORGE_RAW_ROUTE_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", "forge-raw-route"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Borrow exact original paid participants after ordinary startup; native commands/ticks alone earn cargo, manufacture, payment and whole arrivals. */
	std::function<void()> inspect = [&] {
		try { AutoRestoreBackup actor{_current_company, _local_company}; AutoRestoreBackup pause{_pause_mode, PauseModes{}}; const auto world = LoadForgeRawRouteWorld(); EarnForgeRawDelivery(world, ShieldSourceValues("OTTD_FORGE_RAW_DELIVERY_VALUES", 8), paths); }
		catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_forge_raw_delivery_observer, &inspect}; IConsole::CmdRegister("forge_raw_delivery_check", ObserveForgeRawDelivery);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** @param world Exact original manufactured-cargo world. @param path Invocation-owned native output. Save every named owner unchanged and separately protect unsaved client RNG. */
static void SaveForgeProducedWorld(const ForgeRawRouteWorld &world, const std::filesystem::path &path)
{
	const auto before = CaptureForgeRawDelivery(world); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureForgeRawDelivery(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
}

/** @param world Original real source, route and produced-stock owners. @param reserves Both original generated finite reserves. Require genuine manufactured output and actual whole stopped empty carriers before ordinary file observations. */
static void CheckForgeProducedWorld(const ForgeRawRouteWorld &world, const std::array<uint64_t, 2> &reserves)
{
	CheckForgeRawDelivery(world, reserves); REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{2005});
	const auto *processor = GetLayerProcessorState(world.source.industries[2]->index); REQUIRE(processor->produced.at(CargoLabel{"RFMT"}) >= 20);
	// Both real raw trains remain wholly at their original homes. File
	// observation cannot hide a loaded trailer or replay another paid arrival.
	for (size_t index = 0; index < world.source.deposits.size(); ++index) {
		const auto *front = world.source.parts[index * 2]; REQUIRE(front->IsChainInDepot()); REQUIRE(front->tile == world.source.homes[index]); REQUIRE(front->vehstatus.Test(VehState::Stopped));
		REQUIRE(front->cargo_payment == nullptr); REQUIRE(front->cargo.StoredCount() == 0); REQUIRE(world.source.parts[index * 2 + 1]->cargo.StoredCount() == 0);
	}
}

/** @param boundaries Write the exact original manufactured world and its untouched next full opportunity; false ordinarily previews, resaves or continues those exact original owners. */
static void ObserveForgeProducedFiles(bool boundaries)
{
	const char *input = std::getenv("OTTD_FORGE_PRODUCED_INPUT"), *snapshot_text = std::getenv("OTTD_FORGE_PRODUCED_SNAPSHOT"), *action_text = std::getenv("OTTD_FORGE_PRODUCED_ACTION");
	REQUIRE(input != nullptr); REQUIRE(snapshot_text != nullptr); const std::string expected{snapshot_text};
	const auto paths = GetTestCaseArtifactPaths("tubes", "forge-produced"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Borrow only actual ordinary-loaded owners; neither branch may call source/route builders or repair content, date, funds, packets, ratings or quantities. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; auto world = LoadForgeRawRouteWorld();
			const auto values = ShieldSourceValues("OTTD_FORGE_RAW_DELIVERY_VALUES", 8); const std::array reserves{values[3] + values[5], values[4] + values[6]};
			CheckForgeProducedWorld(world, reserves); const auto actual = CaptureForgeRawDelivery(world);
			if (actual != expected) fmt::print("FORGE_PRODUCED_SNAPSHOT actual={}\n", actual);
			REQUIRE(actual == expected);
			if (boundaries) {
				AutoRestoreBackup pause{_pause_mode, PauseModes{}};
				// The accepted original is saved without work, then the very next
				// complete native opportunity advances both real mines/industry
				// work. No reload or hidden tick separates these two boundaries.
				for (uint step = 0; step < 2; ++step) {
					if (step != 0) { RunForgeRawOpportunity(world); CheckForgeProducedWorld(world, reserves); }
					const auto path = paths.Output("saves", fmt::format("forge-produced-boundary-{}.sav", step)); SaveForgeProducedWorld(world, path);
					fmt::print("FORGE_PRODUCED_WORLD step={} tick={} snapshot={} path={}\n", step, TimerGameTick::counter, CaptureForgeRawDelivery(world), path.string());
				}
			} else {
				REQUIRE(action_text != nullptr); const std::string_view action{action_text};
				// Only the explicit complete next-opportunity action may advance
				// gameplay; ordinary preview/resaving cannot enter a producer.
				REQUIRE((action == "inspect" || action == "step")); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
				REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				world = LoadForgeRawRouteWorld(); REQUIRE(CaptureForgeRawDelivery(world) == expected); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
				const auto tick = TimerGameTick::counter;
				if (action == "step") { AutoRestoreBackup pause{_pause_mode, PauseModes{}}; RunForgeRawOpportunity(world); CheckForgeProducedWorld(world, reserves); REQUIRE(TimerGameTick::counter == tick + 1); }
				const auto path = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action)); SaveForgeProducedWorld(world, path);
				fmt::print("FORGE_PRODUCED_READ action={} preview_pure=true full_native_clock=true snapshot={} path={}\n", action, CaptureForgeRawDelivery(world), path.string());
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_forge_raw_delivery_observer, &inspect}; IConsole::CmdRegister(boundaries ? "forge_produced_boundaries_check" : "forge_produced_file_check", ObserveForgeRawDelivery);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}

/** Actual original manufactured cargo supplies untouched native boundary0 and precisely its next full clock/front/industry opportunity. */
TEST_CASE("Original manufactured Forge cargo writes complete native boundaries", "[.tube-forge-produced-boundaries-native]") { ObserveForgeProducedFiles(true); }

/** Ordinary loading, copied preview, repeated native saves and full continuation preserve both finite sources and genuine RFMT. */
TEST_CASE("Original manufactured Forge cargo survives ordinary native files", "[.tube-forge-produced-read-native]") { ObserveForgeProducedFiles(false); }
