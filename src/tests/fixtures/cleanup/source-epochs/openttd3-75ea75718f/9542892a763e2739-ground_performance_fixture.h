/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file ground_performance_fixture.h Read-only timings around ordinary native application dispatch.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group21C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Explicit headless performance workload;not an alternative bootstrap acceptance producer.
 */
#ifndef TESTS_GROUND_PERFORMANCE_FIXTURE_H
#define TESTS_GROUND_PERFORMANCE_FIXTURE_H

#include "normal_ground_newgame_fixture.h"
#include "../video/null_v.h"
#include "../window_func.h"
#include "../industry.h"
#include "../station_base.h"
#include "../utility_power.h"
#include "../map_layer_gui.h"
#include "../window_gui.h"
#include "../viewport_type.h"
#include "../transparency.h"
#include <chrono>

inline bool _ground_performance_ready = false; ///< Test observation only;never simulation state.
inline uint64_t _ground_performance_last_tick = 0; ///< Last observed native tick,not a clock writer.
inline std::function<void(uint64_t, uint64_t, uint64_t)> *_ground_performance_frame = nullptr; ///< Full frame timing observer.
inline std::function<void()> *_ground_performance_prepare = nullptr; ///< Startup observation after native MakeNewGameDone has returned.

/** @return Steady-clock nanoseconds for elapsed observations only;never a game clock. */
inline uint64_t GroundPerformanceNow()
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

/** Native null rendering with the ordinary player startup capability and unmodified three frame dispatchers. */
class GroundPerformanceVideoDriver final : public VideoDriver_Null {
public:
	bool HasGUI() const override
	{
		// Generation retains ordinary player startup. Explicit headless view
		// comparisons load real existing companies through native OnStartGame.
		const char *view = std::getenv("OTTD_GROUND_PERFORMANCE_VIEW");
		return view == nullptr || std::string_view{view} != "headless";
	}
	std::string_view GetName() const override { return "ground-performance-null"; }
	void MainLoop() override
	{
		while (!_exit_game) {
			const auto begin = GroundPerformanceNow();
			::GameLoop(); const auto simulation = GroundPerformanceNow();
			::InputLoop(); const auto input = GroundPerformanceNow();
			::UpdateWindows(); const auto windows = GroundPerformanceNow();
			if (!_exit_game && _ground_performance_prepare != nullptr) (*_ground_performance_prepare)();
			if (!_exit_game && _ground_performance_ready && TimerGameTick::counter > _ground_performance_last_tick && _ground_performance_frame != nullptr) {
				REQUIRE(TimerGameTick::counter == _ground_performance_last_tick + 1);
				_ground_performance_last_tick = TimerGameTick::counter;
				(*_ground_performance_frame)(simulation - begin, input - simulation, windows - input);
			}
		}
	}
};

/** Register only with the test binary;normal application drivers and simulation ownership stay unchanged. */
class FGroundPerformanceVideoDriver final : public DriverFactoryBase {
public:
	FGroundPerformanceVideoDriver() : DriverFactoryBase(Driver::Type::Video, 0, "ground-performance-null", "Observed native ground performance loop") {}
	std::unique_ptr<Driver> CreateInstance() const override { return std::make_unique<GroundPerformanceVideoDriver>(); }
};
inline FGroundPerformanceVideoDriver _ground_performance_driver;

/** @param phase Actual ready/final boundary. Report actual live owners outside all measured tick scopes. */
inline void GroundPerformanceCounts(std::string_view phase)
{
	const auto &geology = GetGeologyWorldState();
	const auto utility = CaptureUtilityPowerState();
	size_t fleet = 0, packets = CargoPacket::GetNumItems(), route = 0, swept = 0, support = 0;
	for (const auto *vehicle : Vehicle::Iterate()) if (vehicle->type < VehicleType::CompanyEnd) ++fleet;
	for (const auto &job : geology.jobs) {
		route += job.route.size();
		for (const auto &cut : job.route) { swept += cut.swept_tiles.size(); support += cut.support_tiles.size(); }
	}
	fmt::print("GROUND_PERFORMANCE_COUNTS phase={} width={} height={} map_tiles={} map_layers={} packed_tile_payload_estimate={} geology_terrain_bytes={} deposits={} deposit_index_payload_bytes={} vents={} jobs={} claims={} route_segments={} swept_refs={} support_refs={} companies={} fleet_parts={} effects_and_disasters={} packets={} industries={} stations={} utility_producers={} utility_consumers={} utility_links={} tick={}\n",
		phase, Map::SizeX(), Map::SizeY(), Map::Size(), Map::Size() / Map::PlaneSize(), uint64_t{Map::Size()} * 12,
		geology.terrain.StorageBytes(), geology.deposits.Records().size(), geology.deposits.IndexPayloadBytes(), geology.vents.size(),
		geology.jobs.size(), geology.job_claims.size(), route, swept, support, Company::GetNumItems(), fleet,
		Vehicle::GetNumItems() - fleet, packets, Industry::GetNumItems(), Station::GetNumItems(),
		utility.producers.size(), utility.consumers.size(), utility.links.size(), TimerGameTick::counter);
	// Packed TileBase8+TileExtended4 estimate is bound to current map_func.h/map.cpp;
	// allocator/temporary peaks are measured independently by the Windows controller.
	std::fflush(stdout);
}

/** @param phase Actual paused boundary. Measure synchronous full native serialization,then retain a pure independent repeat. */
inline void GroundPerformanceSave(std::string_view phase)
{
	const auto paths = GetTestCaseArtifactPaths("performance", "ground-maps");
	const auto tick = TimerGameTick::counter; const auto rng = std::array{_random.state[0], _random.state[1]};
	const auto begin = GroundPerformanceNow();
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", fmt::format("{}-world.sav", phase))), false) == SaveLoadResult::Ok);
	const auto elapsed = GroundPerformanceNow() - begin;
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", fmt::format("{}-repeat.sav", phase))), false) == SaveLoadResult::Ok);
	CHECK(TimerGameTick::counter == tick); CHECK((_random.state[0] == rng[0] && _random.state[1] == rng[1]));
	fmt::print("GROUND_PERFORMANCE_SAVE phase={} synchronous_ns={} tick={}\n", phase, elapsed, tick); std::fflush(stdout);
}

/** Run an ordinary generated or explicitly selected original workload with exact actual tick sampling. */
TEST_CASE("Ground maps retain actual complete dispatch performance samples", "[.ground-performance-map]")
{
	const char *input = std::getenv("OTTD_GROUND_PERFORMANCE_INPUT");
	const char *profile = std::getenv("OTTD_GROUND_PERFORMANCE_PROFILE"); REQUIRE(profile != nullptr);
	const char *view = std::getenv("OTTD_GROUND_PERFORMANCE_VIEW");
	if (view != nullptr) {
		REQUIRE(input != nullptr);
		REQUIRE((std::string_view{view} == "surface" || std::string_view{view} == "underground" || std::string_view{view} == "hidden" || std::string_view{view} == "headless"));
	}
	REQUIRE((std::string_view{profile} == "classic" || std::string_view{profile} == "multilayer_industry"));
	bool requested = false, observed = false, complete = false; uint64_t opening_tick = 0, frames = 0, initial_observer_ns = 0; std::exception_ptr failure;
	const auto launch = GroundPerformanceNow();
	std::function<void()> request = [&] { requested = true; };
	std::function<void()> begin = [&] {
		if (!requested || observed) return;
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			REQUIRE(_game_mode == GameMode::Normal); REQUIRE_FALSE(_networking); REQUIRE(Company::IsValidID(_local_company));
			const bool enabled = std::string_view{profile} == "multilayer_industry";
			CHECK(GetCurrentMultiLayerProfileSelection().id == (enabled ? MultiLayerGameplayProfileID::MultilayerIndustry : MultiLayerGameplayProfileID::Classic));
			CHECK((GetGeologyWorldState().generator_version != 0) == enabled);
			if (view != nullptr) {
				Window *window = FindWindowById(WindowClass::MainWindow, 0);
				REQUIRE(window != nullptr); REQUIRE(window->viewport != nullptr);
				const auto layer = std::string_view{view} == "underground" ? MapLayer::Underground :
					std::string_view{view} == "hidden" ? MapLayer::DeepUnderground : MapLayer::Surface;
				if (std::string_view{view} != "headless") SetViewportLayer(window, layer);
				if (std::string_view{view} == "hidden") { _display_opt.Reset(DisplayOption::FullDetail); _display_opt.Reset(DisplayOption::FullAnimation); }
				REQUIRE(window->viewport->layer == layer);
				fmt::print("GROUND_PERFORMANCE_VIEW role={} layer={} gui_capability={} logical_viewport=1 raster=0\n", view, to_underlying(layer), VideoDriver::GetInstance()->HasGUI() ? 1 : 0);
			}
			fmt::print("GROUND_PERFORMANCE_READY startup_to_ready_ns={} mode={} profile={}\n", GroundPerformanceNow() - launch, input == nullptr ? "generation" : "load", profile);
			const auto inspection_begin = GroundPerformanceNow();
			REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
			GroundPerformanceCounts("ready"); GroundPerformanceSave("initial");
			opening_tick = TimerGameTick::counter;
			_ground_performance_last_tick = opening_tick;
			const auto command = GroundPerformanceNow();
			REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
			fmt::print("GROUND_PERFORMANCE_COMMAND operation=unpause synchronous_ns={}\n", GroundPerformanceNow() - command);
			initial_observer_ns = GroundPerformanceNow() - inspection_begin;
			observed = true; _ground_performance_ready = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	std::function<void(uint64_t, uint64_t, uint64_t)> sample = [&](uint64_t simulation, uint64_t input_ns, uint64_t windows) {
		try {
			++frames;
			REQUIRE(TimerGameTick::counter == opening_tick + frames);
			if (frames == 1) {
				const auto launch_to_first_tick_ns = GroundPerformanceNow() - launch;
				REQUIRE(launch_to_first_tick_ns >= initial_observer_ns);
				fmt::print("GROUND_PERFORMANCE_RESUMED launch_to_first_tick_ns={} initial_observer_ns={} load_and_first_work_ns={} first_simulation_ns={} tick={}\n",
					launch_to_first_tick_ns, initial_observer_ns, launch_to_first_tick_ns - initial_observer_ns, simulation, TimerGameTick::counter);
			}
			if (frames > 128) fmt::print("GROUND_PERFORMANCE_TICK index={} simulation_ns={} input_ns={} windows_ns={}\n", frames - 129, simulation, input_ns, windows);
			if (frames == 128 + 5 * 128) {
				AutoRestoreBackup actor{_current_company, _local_company};
				REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
				GroundPerformanceCounts("final"); GroundPerformanceSave("final"); complete = true; _exit_game = true;
			}
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	AutoRestoreBackup ready{_ground_performance_ready, false};
	AutoRestoreBackup frame{_ground_performance_frame, &sample};
	AutoRestoreBackup prepare{_ground_performance_prepare, &begin};
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &request};
	IConsole::CmdRegister("ground_performance_start", ObserveNormalGroundNewgame);
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(),
		"-v", "ground-performance-null", "-b", "null", "-s", "null", "-m", "null", "-I", "original_dos", "-d", "misc=4,map=2,sl=1"};
	if (input == nullptr) owned.insert(owned.end(), {"-G", "420", "-t", "1980", "-g"}); else owned.insert(owned.end(), {"-g", input});
	std::vector<std::string_view> arguments; for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure);
	REQUIRE(observed); REQUIRE(complete); REQUIRE(frames == 768);
	fmt::print("GROUND_PERFORMANCE_COMPLETE warmup=128 intervals=5 ticks_per_interval=128 measured_ticks=640 rendering=false\n");
}

#endif /* TESTS_GROUND_PERFORMANCE_FIXTURE_H */
