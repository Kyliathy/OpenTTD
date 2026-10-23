/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_planning_seed.cpp Native planning seeds and playable underground demo preparation/reload. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group3C; docs/zz-reach2/architecture/savegame/archi-savegame.md */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include "layer_save_fixture.h"
#include "../company_cmd.h"
#include "../saveload/saveload_func.h"
#include "../table/sprites.h"
#include <cstdlib>
#include "native_art_render_fixture.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../industry_cmd.h"
#include "../industry_map.h"
#include "../geothermal_collector.h"
#include "../rail_cmd.h"
#include "../rail_map.h"
#include "../signal_func.h"
#include "../station_cmd.h"
#include "../station_base.h"
#include "../newgrf_station.h"
#include "../vehicle_cmd.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../excavation_state.h"
#include "../order_cmd.h"
#include "../underground_vehicle.h"
#include "../layer_portal_cmd.h"
#include "../safeguards.h"

/** Write a funded native logistics world whose target rock and uncommissioned portal remain untouched by any TBM. */
TEST_CASE("Fresh TBM planning writes a real uncommissioned rock-layer scenario", "[.tbm-planning-seed]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const char *selected = std::getenv("OTTD_TBM_FEEDBACK_FRESH");
	REQUIRE(selected != nullptr);
	const std::string_view name{selected};
	REQUIRE((name == "underground" || name == "deep"));
	const MapLayer layer = name == "deep" ? MapLayer::DeepUnderground : MapLayer::Underground;
	UndergroundWorkflowWorld world(layer, true, true);
	LayerSaveContent content(world.ActivatedContent(), true);
	// Area coverage in this declared scenario permits ordinary sharp rail
	// pieces; the cutter's own advancing bends remain limited to45 degrees.
	AutoRestoreBackup turns(_settings_game.pf.forbid_90_deg, false);
	AutoRestoreBackup player(_local_company, _current_company);
	// This unstarted scenario has no video driver for CmdPause's UI hook.
	// Declare its initial paused state before writing; no simulation ran here.
	AutoRestoreBackup initial_pause(_pause_mode, PauseModes{PauseMode::Normal});
	REQUIRE(Command<Commands::RenameCompany>::Do(DoCommandFlag::Execute, "Underground Mines").Succeeded());
	REQUIRE(CaptureTBMProjects().empty());
	REQUIRE(GetGeologyWorldState().terrain.Get(world.rock) == ExcavationState::Solid);
	const auto path = GetTestCaseArtifactPaths("tbm", "planning").saves / fmt::format("tbm-planning-{}-393.sav", name);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto money = Company::Get(_current_company)->money;
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE); // The real save writer temporarily displays its busy cursor, as in existing native save fixtures.
	AutoRestoreBackup no_ui(_game_mode, GameMode::Menu);
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == money);
	fmt::print("PLANNING_SEED layer={} portal={} rock={} origin={} hauler={} setup={} saved={}\n", name,
		world.portal.base(), world.rock.base(), world.origin->index.base(), world.hauler->index.base(), world.setup_cost.base(), path.string());
}

/** Playable finite-mine tour preparation; see d2um-underground-mines.md. */
static std::function<void()> *_underground_demo_observer = nullptr;

/** Invoke only the test-owned startup observer after ordinary full-world loading. */
static bool PrepareUndergroundDemo(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1);
	REQUIRE(_underground_demo_observer != nullptr);
	(*_underground_demo_observer)();
	return true;
}

/** Continue the exact retained tour after normal loading; no construction supplies or cargo are injected. */
TEST_CASE("Retained underground demos continue actual freight and collector construction after reload", "[.underground-demo-reload]")
{
	const char *input = std::getenv("OTTD_UNDERGROUND_DEMO_SOURCE");
	REQUIRE(input != nullptr);
	const auto source = RequireTestArtifactWithin(std::filesystem::path{input}, FindTestRepositoryRoot());
	const auto config = GetTestRuntimeConfig();
	bool observed = false;
	std::exception_ptr failure;
	std::function<void()> resume = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			AutoRestoreBackup pause(_pause_mode, PauseModes{});
			UpdateSignalsInBuffer();
			const auto collectors = CaptureGeothermalCollectors();
			uint ticks = 0;
			if (!collectors.empty()) {
				REQUIRE(collectors.size() == 1);
				const auto initial = collectors.front();
				REQUIRE(initial.work_ticks == 74);
				REQUIRE(initial.materials_committed);
				while (ticks++ < 1000 && !IsGeothermalCollectorOperational(initial.industry)) StateGameLoop();
				REQUIRE(IsGeothermalCollectorOperational(initial.industry));
				CHECK(CaptureGeothermalCollectors().front().work_ticks == 296);
				fmt::print("DEMO_RELOAD collector_work_before=74 collector_work_after=296 supplied_materials=0 ticks={}\n", ticks);
			} else {
				Industry *mine = nullptr;
				for (Industry *industry : Industry::Iterate()) {
					const auto *state = GetLayerMineState(industry->index);
					if (state != nullptr && GetGeologyWorldState().deposits.Get(state->deposit)->resource == CargoLabel{"COAL"}) mine = industry;
				}
				REQUIRE(mine != nullptr);
				const auto deposit = GetLayerMineState(mine->index)->deposit;
				const auto reserve = GetGeologyWorldState().deposits.Get(deposit)->remaining_reserve;
				Train *shuttle = nullptr;
				for (Train *train : Train::Iterate()) {
					if (train->IsPrimaryVehicle() && train->GetNextUnit() != nullptr &&
							train->GetNextUnit()->engine_type == GetUndergroundEngine(5) &&
							train->GetNextUnit()->cargo_type == GetCargoTypeByLabel(CargoLabel{"COAL"})) shuttle = train;
				}
				REQUIRE(shuttle != nullptr);
				const auto profit = shuttle->profit_this_year;
				const auto origin = shuttle->GetOrder(VehicleOrderID{0})->GetDestination().ToStationID();
				bool returned = false;
				while (ticks++ < 15000) {
					StateGameLoop();
					returned |= shuttle->last_station_visited == origin;
					if (returned && shuttle->profit_this_year > profit && shuttle->last_station_visited != origin) break;
				}
				REQUIRE(returned);
				REQUIRE(shuttle->profit_this_year > profit);
				REQUIRE(GetGeologyWorldState().deposits.Get(deposit)->remaining_reserve < reserve);
				fmt::print("DEMO_RELOAD reserve_before={} reserve_after={} profit_before={} profit_after={} empty_return=true supplied_cargo=0 ticks={}\n",
					reserve, GetGeologyWorldState().deposits.Get(deposit)->remaining_reserve, profit, shuttle->profit_this_year, ticks);
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true;
		_exit_game = true;
	};
	AutoRestoreBackup observer(_underground_demo_observer, &resume);
	IConsole::CmdRegister("underground_demo", PrepareUndergroundDemo);
	const std::string source_path = source.string(), config_path = config.string();
	std::array<std::string_view, 17> arguments = {"RelWithDebInfo/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple",
		"-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}

/** Execute through the synchronized command kernel, retaining its ordinary charge and refusal. */
template <Commands Tcmd, typename... Args>
static auto DemoCommand(Args... args)
{
	auto result = Command<Tcmd>::Do(DoCommandFlag::Execute, args...);
	INFO("demo command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(result).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(result).Succeeded());
	return result;
}

/** Save a genuinely reached paused state; retained source worlds are never overwritten. */
static void SaveMineDemo(TileIndex centre, std::string_view state)
{
	SetViewportLayer(GetMainWindow(), MapLayer::Underground);
	ScrollMainWindowToTile(centre, true);
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	const auto path = GetTestArtifactPaths("mines", "underground-demo").saves / fmt::format("underground-mine-{}.sav", state);
	REQUIRE(!std::filesystem::exists(path));
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

TEST_CASE("Underground demo funds a native mine and delivers its finite coal through a public portal", "[.underground-demo]")
{
	const auto source = FindTestRepositoryRoot() / "src/tests/fixtures/haulage/far-exit/underground-demo-tunnel-399.sav";
	const auto config = GetTestRuntimeConfig();
	bool observed = false;
	std::exception_ptr failure;
	std::function<void()> prepare = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			AutoRestoreBackup pause(_pause_mode, PauseModes{});
			const auto resource = CargoLabel{"COAL"};
			const auto type = GetLayerMineIndustryType(resource);
			REQUIRE(type != IT_INVALID);
			LayerMineRequest request;
			request.company = _local_company;
			request.type = type;
			// Choose a real natural seam with enough unoccupied room for the
			// disclosed workshop, reciprocal portal and Surface receiver.
			for (const auto &deposit : GetGeologyWorldState().deposits.Records()) {
				if (deposit.resource != resource || deposit.layer != MapLayer::Underground ||
						deposit.claimant != CompanyID::Invalid() || deposit.remaining_reserve == 0) continue;
				for (const auto &span : deposit.spans) {
					for (uint x = span.first_x; x <= span.last_x; ++x) {
						if (x < 6 || x + 14 >= Map::SizeX() - 1 || span.y < 6 || uint(span.y) + 11 >= Map::SizeY() - 1) continue;
						bool free = true;
						for (int y = 0; y < 6 && free; ++y) for (int dx = -1; dx <= 4; ++dx) {
							if (!IsTileType(TileXYL(x + dx, span.y + y, MapLayer::Underground), TileType::Clear)) free = false;
						}
						for (int y = 2; y < 11 && free; ++y) for (int dx = 4; dx < 14; ++dx) {
							const auto tile = TileXYL(x + dx, span.y + y, MapLayer::Surface);
							if (!IsTileType(tile, TileType::Clear) || GetTileSlope(tile) != SLOPE_FLAT) free = false;
						}
						if (free) {
							request.tile = TileXYL(x, span.y, MapLayer::Underground);
							request.deposit = deposit.id;
							break;
						}
					}
					if (request.tile != INVALID_TILE) break;
				}
				if (request.tile != INVALID_TILE) break;
			}
			REQUIRE(request.tile != INVALID_TILE);
			const auto x = TileX(request.tile), y = TilePlaneY(request.tile);
			// Explicit scenario-author floor, through the existing geological
			// restoration owner. No natural seam, reserve, claim or cargo is invented.
			auto snapshot = CaptureGeologyState();
			GeologyWorldState candidate;
			REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
			for (int dy = 0; dy < 6; ++dy) for (int dx = -1; dx <= 4; ++dx) {
				REQUIRE(candidate.terrain.Set(TileXYL(x + dx, y + dy, MapLayer::Underground), ExcavationState::Excavated));
			}
			snapshot.packed.assign(candidate.terrain.Data().begin(), candidate.terrain.Data().end());
			REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
			StageLoadedGeologyState(std::move(candidate));
			AcceptLoadedGeologyState();
			REQUIRE(ValidateMineDepositClaim(request).error == LayerIndustryError::None);
			const auto reserve = GetGeologyWorldState().deposits.Get(request.deposit)->remaining_reserve;
			DemoCommand<Commands::BuildLayerMine>(request.tile, type, uint32_t{0}, request.deposit, uint32_t{17});
			Industry *mine = nullptr;
			for (Industry *industry : Industry::Iterate()) if (industry->location.tile == request.tile) mine = industry;
			REQUIRE(mine != nullptr);
			const auto depot = TileXYL(x - 1, y + 3, MapLayer::Underground);
			const auto loading = TileXYL(x, y + 3, MapLayer::Underground);
			DemoCommand<Commands::BuildRailDepot>(depot, RAILTYPE_RAIL, DiagDirection::SW);
			DemoCommand<Commands::BuildRailStation>(loading, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{2}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
			const auto origin = Station::GetByTile(loading)->index;
			for (uint dx : {2U, 3U}) DemoCommand<Commands::BuildRail>(TileXYL(x + dx, y + 3, MapLayer::Underground), RAILTYPE_RAIL, Track::X, false);
			const auto portal = TileXYL(x + 4, y + 3, MapLayer::Underground);
			DemoCommand<Commands::BuildLayerPortal>(portal, TransportType::Rail, MapLayer::Surface, DiagDirection::SW, RAILTYPE_RAIL, INVALID_ROADTYPE);
			for (uint dx : {5U, 6U}) DemoCommand<Commands::BuildRail>(TileXYL(x + dx, y + 3, MapLayer::Surface), RAILTYPE_RAIL, Track::X, false);
			const auto unloading = TileXYL(x + 7, y + 3, MapLayer::Surface);
			DemoCommand<Commands::BuildRailStation>(unloading, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{2}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
			const auto destination = Station::GetByTile(unloading)->index;
			DemoCommand<Commands::BuildIndustry>(TileXYL(x + 7, y + 5, MapLayer::Surface), IndustryType{1}, uint32_t{0}, true, uint32_t{23});
			Station::Get(destination)->RecomputeCatchment();
			DemoCommand<Commands::ChangeLayerPortalConnection>(portal, true);
			const auto coal = GetCargoTypeByLabel(resource);
			const auto engine = DemoCommand<Commands::BuildVehicle>(depot, GetUndergroundEngine(4), false, INVALID_CARGO, ClientID::Invalid);
			const auto wagon = DemoCommand<Commands::BuildVehicle>(depot, GetUndergroundEngine(5), false, coal, ClientID::Invalid);
			const auto train_id = std::get<1>(engine);
			DemoCommand<Commands::MoveRailVehicle>(std::get<1>(wagon), train_id, false);
			Order collect;
			collect.MakeGoToStation(origin);
			collect.SetLoadType(OrderLoadType::FullLoadAny);
			collect.SetUnloadType(OrderUnloadType::NoUnload);
			DemoCommand<Commands::InsertOrder>(train_id, VehicleOrderID{0}, collect);
			Order deliver;
			deliver.MakeGoToStation(destination);
			deliver.SetLoadType(OrderLoadType::NoLoad);
			deliver.SetUnloadType(OrderUnloadType::Unload);
			DemoCommand<Commands::InsertOrder>(train_id, VehicleOrderID{1}, deliver);
			DemoCommand<Commands::StartStopVehicle>(train_id, true);
			UpdateSignalsInBuffer();
			SaveMineDemo(request.tile, "construction");
			uint ticks = 0;
			while (ticks++ < 40000 && Train::Get(train_id)->profit_this_year <= 0) StateGameLoop();
			INFO("mine demo ticks=" << ticks << " reserve=" << GetGeologyWorldState().deposits.Get(request.deposit)->remaining_reserve
				<< " train tile=" << Train::Get(train_id)->tile.base());
			REQUIRE(Train::Get(train_id)->profit_this_year > 0);
			REQUIRE(GetGeologyWorldState().deposits.Get(request.deposit)->remaining_reserve < reserve);
			SaveMineDemo(request.tile, "haulage");
			fmt::print("UNDERGROUND_DEMO mine_x={} mine_y={} centre={} train={} loading={} unloading={} reserve_before={} reserve_after={} profit={} ticks={} fixture_floor=true fixture_cargo=false\n",
				x, y, request.tile.base(), train_id.base(), origin.base(), destination.base(), reserve,
				GetGeologyWorldState().deposits.Get(request.deposit)->remaining_reserve, Train::Get(train_id)->profit_this_year, ticks);
		} catch (...) { failure = std::current_exception(); }
		observed = true;
		_exit_game = true;
	};
	AutoRestoreBackup observer(_underground_demo_observer, &prepare);
	IConsole::CmdRegister("underground_demo", PrepareUndergroundDemo);
	const std::string source_path = source.string(), config_path = config.string();
	std::array<std::string_view, 17> arguments = {"RelWithDebInfo/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple",
		"-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
