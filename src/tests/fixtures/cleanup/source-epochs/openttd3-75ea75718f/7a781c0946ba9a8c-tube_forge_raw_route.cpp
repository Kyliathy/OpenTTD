/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_forge_raw_route.cpp Original-world paid conventional input preparation before genuine CFMT manufacture.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FCB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor/Connected shafts remain explicit boundaries; no raw shipment or manufactured RFMT is inferred.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "forge_raw_route_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

/** @param source Actual original raw carriers. @return Complete original physical poses/membership/cargo before command-only preparation, independent of the newly installed orders. */
static auto ForgeRawOriginalPositions(const ForgeSourceWorld &source)
{
	/** @param part Actual original native front or wagon. @return Exact immutable preparation-time physical/cargo observations. */
	const auto pose = [](const Train &part) { return std::tuple{part.index, part.First()->index, part.engine_type, part.owner, part.tile, part.x_pos, part.y_pos, part.z_pos, part.direction, part.track, part.cur_speed, part.cargo_type, part.cargo_cap, part.cargo.StoredCount()}; };
	return std::array{pose(*source.parts[0]), pose(*source.parts[1]), pose(*source.parts[2]), pose(*source.parts[3])};
}

/** @param world Original raw participants and actual newly paid facilities. @return Read-only source/queue/native eligibility observations for pure ready saving, without work or state repair. */
static std::string CaptureForgeRawReadyWorld(const ForgeRawRouteWorld &world)
{
	std::string result = CaptureForgeSourceWorld(world.source);
	/** @param name Unique real station field name. @param value Exact native whole integer bits. */
	const auto add = [&](std::string_view name, uint64_t value) { result += fmt::format("{}={},", name, value); };
	// All native service/eligibility metadata and queues must remain actual;
	// command-paid facilities cannot impersonate earned service or source cargo.
	for (size_t index = 0; index < world.stations.size(); ++index) {
		const auto &station = *world.stations[index]; const auto prefix = fmt::format("raw_station{}_", index);
		add(prefix + "id", station.index.base()); add(prefix + "tile", station.xy.base()); add(prefix + "owner", station.owner.base());
		for (const auto cargo : world.source.cargos) {
			const auto &entry = station.goods[cargo]; const auto name = prefix + fmt::format("cargo{}_", cargo);
			add(name + "units", entry.TotalCount()); add(name + "rating", entry.rating); add(name + "speed", entry.last_speed); add(name + "fraction", entry.amount_fract); add(name + "status", entry.status.base());
		}
	}
	return result;
}

/** @param world Actual original source and paid input route owners. @param expected Original tick/cash/loan/CORE/Coal/remaining reserves. @param paths Invocation-owned complete native ready output. Require unchanged source quantities and whole stopped empty carriers before pure saving. */
static void SavePreparedForgeRawRoute(const ForgeRawRouteWorld &world, const std::vector<uint64_t> &expected, const TestArtifactPaths &paths)
{
	REQUIRE(TimerGameTick::counter == expected[0]); REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{2005});
	REQUIRE(Vehicle::GetNumItems() == 4); REQUIRE(Industry::GetNumItems() == 3); REQUIRE(Station::GetNumItems() == 3); REQUIRE(CargoPacket::GetNumItems() == 0);
	const auto &company = *Company::Get(_current_company); REQUIRE(company.money > Money{0}); REQUIRE(company.money.base() < static_cast<int64_t>(expected[1]));
	REQUIRE(company.current_loan.base() == static_cast<int64_t>(expected[2])); REQUIRE(company.cur_economy.income == Money{0}); REQUIRE(company.num_valid_stat_ent == 0);
	// Both original finite quantities and complete actual raw chains retain
	// the accepted stock world; new tracks/orders cannot earn cargo or an arrival.
	for (size_t index = 0; index < world.source.deposits.size(); ++index) {
		const auto *mine = GetLayerMineState(world.source.industries[index]->index); REQUIRE(mine != nullptr);
		REQUIRE(world.source.industries[index]->produced[0].waiting == expected[index + 3]); REQUIRE(mine->extracted == expected[index + 3]); REQUIRE(mine->dispatched == 0);
		REQUIRE(GetGeologyWorldState().deposits.Get(world.source.deposits[index])->remaining_reserve == expected[index + 5]);
		const auto *front = world.source.parts[index * 2]; REQUIRE(front->IsChainInDepot()); REQUIRE(front->tile == world.source.homes[index]); REQUIRE(front->vehstatus.Test(VehState::Stopped)); REQUIRE(IsTrainOnItsTiles(front));
	}
	for (const auto *part : world.source.parts) REQUIRE(part->cargo.StoredCount() == 0);
	for (const auto &entry : world.source.industries[2]->accepted) REQUIRE(entry.waiting == 0);
	for (const auto &entry : world.source.industries[2]->produced) REQUIRE(entry.waiting == 0);
	for (const auto *station : world.stations) for (const auto cargo : world.source.cargos) REQUIRE(station->goods[cargo].TotalCount() == 0);
	const auto portals = CaptureLayerPortalWorkRecords(); REQUIRE(portals.size() == 2);
	for (const auto &portal : portals) REQUIRE(portal.state == LayerPortalWorkState::Connected);
	const auto before = CaptureForgeRawReadyWorld(world); const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
	const auto path = paths.Output("saves", "forge-raw-route-prepared.sav");
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CaptureForgeRawReadyWorld(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
	fmt::print("FORGE_RAW_PREPARED tick={} cash={} loan={} copper={} coal={} copper_reserve={} coal_reserve={} parts={},{},{},{} industries={},{},{} stations={},{},{} portals={},{} path={}\n",
		TimerGameTick::counter, company.money.base(), company.current_loan.base(), world.source.industries[0]->produced[0].waiting, world.source.industries[1]->produced[0].waiting,
		GetGeologyWorldState().deposits.Get(world.source.deposits[0])->remaining_reserve, GetGeologyWorldState().deposits.Get(world.source.deposits[1])->remaining_reserve,
		world.source.parts[0]->index.base(), world.source.parts[1]->index.base(), world.source.parts[2]->index.base(), world.source.parts[3]->index.base(),
		world.source.industries[0]->index.base(), world.source.industries[1]->index.base(), world.source.industries[2]->index.base(), world.stations[0]->index.base(), world.stations[1]->index.base(), world.stations[2]->index.base(),
		portals[0].pair_key.base(), portals[1].pair_key.base(), path.string());
}

static std::function<void()> *_forge_raw_route_observer = nullptr; ///< Invocation-owned ordinary startup callback, never another source or saved owner.

/** @param args Actual ordinary console arguments after original source loading. @return Console admission; invoke only this native preparation observer. */
static bool ObserveForgeRawRoute(std::span<std::string_view> args)
{
	if (args.empty()) return true;
	REQUIRE(_forge_raw_route_observer != nullptr); (*_forge_raw_route_observer)(); return true;
}

/** Actual native commands prepare routes without moving originals, consuming stocks or replacing any original geological/processor owner. */
TEST_CASE("Original Forge raw world pays for stopped conventional refinery routes", "[.tube-forge-raw-route-prepare-native]")
{
	const char *input = std::getenv("OTTD_FORGE_RAW_ROUTE_INPUT"), *snapshot_text = std::getenv("OTTD_FORGE_RAW_SOURCE_SNAPSHOT"); REQUIRE(input != nullptr); REQUIRE(snapshot_text != nullptr);
	const auto paths = GetTestCaseArtifactPaths("tubes", "forge-raw-route"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Borrow only original normal-loaded source owners; all new topology/facilities/orders are explicit native command or disclosed Connected-boundary preparation. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; const auto source = LoadForgeSourceWorld(); REQUIRE(CaptureForgeSourceWorld(source) == snapshot_text);
			const auto expected = ShieldSourceValues("OTTD_FORGE_RAW_PREPARE_VALUES", 7); const auto positions = ForgeRawOriginalPositions(source);
			const std::array mines{*GetLayerMineState(source.industries[0]->index), *GetLayerMineState(source.industries[1]->index)}; const auto refinery = *GetLayerProcessorState(source.industries[2]->index);
			ForgeRawRouteBuilder builder(source); CHECK(ForgeRawOriginalPositions(source) == positions);
			CHECK(*GetLayerMineState(source.industries[0]->index) == mines[0]); CHECK(*GetLayerMineState(source.industries[1]->index) == mines[1]); CHECK(*GetLayerProcessorState(source.industries[2]->index) == refinery);
			SavePreparedForgeRawRoute(builder.world, expected, paths);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_forge_raw_route_observer, &inspect}; IConsole::CmdRegister("forge_raw_route_prepare_check", ObserveForgeRawRoute);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
