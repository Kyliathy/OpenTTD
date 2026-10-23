/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file ground_network_fixture.h Read-only complete native saves at actual agreed network frames.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Groups17B/18C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Only the test executable registers this observer; production ticks and command dispatch remain unchanged.
 */
#ifndef TESTS_GROUND_NETWORK_FIXTURE_H
#define TESTS_GROUND_NETWORK_FIXTURE_H

#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../openttd.h"
#include "../network/network.h"
#include "../network/network_internal.h"
#include "../timer/timer.h"
#include "../timer/timer_game_tick.h"
#include "../timer/timer_game_calendar.h"
#include "../timer/timer_game_economy.h"
#include "../core/random_func.hpp"
#include "../map_layer_gui.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "../viewport_type.h"
#include "../transparency.h"
#include "../saveload/saveload_func.h"

/**
 * Read one required explicit native peer argument without finding replacement worlds.
 * @param name Invocation-owned environment field naming the exact input or peer role.
 * @return Nonempty argument whose lifetime belongs to this process environment.
 */
inline const char *GroundNetworkArgument(const char *name)
{
	const char *value = std::getenv(name);
	REQUIRE(value != nullptr);
	REQUIRE(*value != '\0');
	return value;
}

/**
 * Save an actual agreed native opportunity twice without changing gameplay authority.
 * @param server Actual dedicated-host versus independently joining-client role.
 * @param stage Explicit joined or joined-next checkpoint basename.
 */
inline void SaveGroundNetworkOpportunity(bool server, std::string_view stage)
{
	const auto before = std::tuple{_frame_counter, TimerGameTick::counter, TimerGameCalendar::date,
		TimerGameCalendar::date_fract, TimerGameEconomy::date, TimerGameEconomy::date_fract,
		_random.state[0], _random.state[1], _current_company, _local_company, _pause_mode};
	const auto artifacts = GetTestCaseArtifactPaths("network", "paid-ground");
	// Repeated full native serialization exposes any observation-induced saved
	// owner change;the external verifier compares every one of the126 chunks.
	for (const std::string_view suffix : {"", "-repeat"}) {
		const auto path = artifacts.Output("saves", fmt::format("ground-{}-{}{}.sav", server ? "server" : "client", stage, suffix));
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK((std::tuple{_frame_counter, TimerGameTick::counter, TimerGameCalendar::date,
		TimerGameCalendar::date_fract, TimerGameEconomy::date, TimerGameEconomy::date_fract,
		_random.state[0], _random.state[1], _current_company, _local_company, _pause_mode} == before));
	fmt::print("GROUND_NETWORK_JOIN role={} stage={} frame={} tick={}\n", server ? "server" : "client", stage,
		_frame_counter, TimerGameTick::counter);
	std::fflush(stdout);
}

/**
 * Run the real application with a test-only observer inside its native game-tick phase.
 * @param action Optional actual-frame observer/poster;public commands still use normal native networking.
 * @param capture_join Also write complete first/next unpaused native opportunities for delivered-family joins.
 * @param sample_by_tick Select later samples by unpaused simulation opportunities,allowing real network pause frames between them.
 * @param tick_offsets Four bounded later opportunity offsets;longer paid recovery cases retain the same complete writer.
 * @return None; application errors and observer failures propagate after native shutdown.
 */
inline void RunGroundNetworkPeer(const std::function<void(uint32_t, bool)> &action = {}, bool capture_join = false, bool sample_by_tick = false,
	std::array<uint64_t, 4> tick_offsets = {128, 256, 384, 512}, uint64_t trailing_ticks = 64)
{
	REQUIRE(std::ranges::is_sorted(tick_offsets));
	REQUIRE(tick_offsets.back() <= 3072);
	REQUIRE(trailing_ticks >= 16); REQUIRE(trailing_ticks <= 64);
	const std::string role = GroundNetworkArgument("OTTD_GROUND_NETWORK_ROLE");
	REQUIRE((role == "server" || role == "client"));
	const bool server = role == "server";
	const auto artifacts = GetTestCaseArtifactPaths("network", "paid-ground");
	std::vector<std::string> owned = {"openttd_test.exe", "-b", "null", "-s", "null", "-m", "null", "-x", "-X",
		"-I", "original_dos", "-c", GetTestRuntimeConfig().string(), "-d", "net=9,misc=4,desync=4,sl=1"};
	if (server) {
		owned.insert(owned.end(), {"-D", "-g", GroundNetworkArgument("OTTD_GROUND_NETWORK_INPUT")});
	} else {
		owned.insert(owned.end(), {"-v", "win32:no_threads", "-b", "8bpp-simple", "-r", "640x480",
			"-n", GroundNetworkArgument("OTTD_GROUND_NETWORK_ADDRESS")});
	}
	std::vector<std::string_view> arguments;
	// Keep every native argument's storage alive across the actual application
	// loop; shell parsing or a temporary borrowed string could change peer input.
	for (const auto &value : owned) arguments.push_back(value);
	std::exception_ptr failure;
	size_t observations = 0;
	std::optional<uint32_t> first_join_frame;
	std::optional<uint64_t> first_join_tick;
	std::array<uint32_t, 4> frames = {256, 384, 512, 640};
	uint32_t exit_frame = static_cast<uint32_t>(frames.back() + trailing_ticks);
	/**
	 * Observe after native agreed commands and calendar/economy/tick advancement,
	 * immediately before RunTileLoop/vehicle work in StateGameLoop.
	 * @param count Number of actual game-tick callbacks;one is required per frame.
	 */
	IntervalTimer<TimerGameTick> observer({TimerGameTick::Priority::None, 1}, [&](uint count) {
		if (!_networking) return;
		if (_game_mode != GameMode::Normal) return;
		try {
			REQUIRE(count == 1);
			REQUIRE(_network_server == server);
			// The rendered client deliberately views Underground with reduced
			// detail while the dedicated host has no camera. View changes may
			// never change simulation clocks,orders,packets or allocation choices.
			if (!server) {
				SetViewportLayer(GetMainWindow(), MapLayer::Underground);
				_display_opt.Reset(DisplayOption::FullDetail);
				_display_opt.Reset(DisplayOption::FullAnimation);
			}
			if (action) action(_frame_counter, server);
			if (capture_join) {
				if (!first_join_frame.has_value()) {
					first_join_frame = _frame_counter;
					first_join_tick = TimerGameTick::counter;
					// Slow real snapshot loading may finish after an absolute
					// checkpoint. Select the same four future agreed frames from
					// the actual first unpaused opportunity on both peers instead.
					// Actual paid recovery may need more travel time;the caller's
					// bounded offsets select full shared snapshots,never fake work.
					for (size_t index = 0; index < frames.size(); ++index) frames[index] = _frame_counter + static_cast<uint32_t>(tick_offsets[index]);
					exit_frame = _frame_counter + static_cast<uint32_t>(tick_offsets.back() + trailing_ticks);
					SaveGroundNetworkOpportunity(server, "joined");
				} else if (_frame_counter == *first_join_frame + 1) {
					SaveGroundNetworkOpportunity(server, "joined-next");
				}
			}
			bool checkpoint = std::ranges::find(frames, _frame_counter) != frames.end();
			if (sample_by_tick) {
				REQUIRE(first_join_tick.has_value());
				checkpoint = std::ranges::find(tick_offsets, TimerGameTick::counter - *first_join_tick) != tick_offsets.end();
				if (checkpoint) {
					REQUIRE(observations < frames.size());
					frames[observations] = _frame_counter;
				}
			}
			if (checkpoint) {
				const auto before = std::tuple{_frame_counter, TimerGameTick::counter, TimerGameCalendar::date,
					TimerGameCalendar::date_fract, TimerGameEconomy::date, TimerGameEconomy::date_fract,
					_random.state[0], _random.state[1], _current_company, _local_company, _pause_mode};
				// Two consecutive full native writes expose any serialized-owner
				// mutation by observation itself. Python compares every complete
				// chunk of this pair before comparing separate host/client worlds.
				for (const std::string_view suffix : {"", "-repeat"}) {
					const auto path = artifacts.Output("saves", fmt::format("ground-{}-f{}{}.sav", role, _frame_counter, suffix));
					REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
				}
				CHECK((std::tuple{_frame_counter, TimerGameTick::counter, TimerGameCalendar::date,
					TimerGameCalendar::date_fract, TimerGameEconomy::date, TimerGameEconomy::date_fract,
					_random.state[0], _random.state[1], _current_company, _local_company, _pause_mode} == before));
				observations++;
				fmt::print("GROUND_NETWORK_STATE role={} frame={} tick={} rng0={} rng1={} observations={}\n",
					role, _frame_counter, TimerGameTick::counter, _random.state[0], _random.state[1], observations);
				const auto *window = GetMainWindow();
				REQUIRE(window != nullptr);
				REQUIRE(window->viewport != nullptr);
				CHECK(window->viewport->layer == (server ? MapLayer::Surface : MapLayer::Underground));
				fmt::print("GROUND_NETWORK_VIEW role={} frame={} layer={} detail={} animation={}\n", role,
					_frame_counter, to_underlying(window->viewport->layer), _display_opt.Test(DisplayOption::FullDetail),
					_display_opt.Test(DisplayOption::FullAnimation));
				std::fflush(stdout);
			}
			// The independent client exits only after all captured frames and a
			// further ACK interval. The wrapper then asks its owned host to quit
			// through the actual dedicated console,retaining clean test outcomes.
			const bool complete = sample_by_tick ? TimerGameTick::counter - *first_join_tick >= tick_offsets.back() + trailing_ticks : _frame_counter >= exit_frame;
			if (!server && complete) {
				_exit_game = true;
			}
		} catch (...) {
			failure = std::current_exception();
			_exit_game = true;
		}
	});
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observations == frames.size());
}

#endif /* TESTS_GROUND_NETWORK_FIXTURE_H */
