/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_production.cpp Exact native game-tick replay of genuine Surface products.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BD.3.
 * Group15BE reuses this actual tick owner after additional matching bank credit; the native debt must remain unchanged.
 * Native StateGameLoop owns clocks, jobs, production, vehicles and payments.
 * Restored routing workers may temporarily pause the game: wait for the real
 * owner to clear that pause, never fabricate ticks or reset its saved schedule.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../misc_cmd.h"
#include "../openttd.h"
#include "../timer/timer_game_tick.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <thread>
#include "../safeguards.h"

static std::function<void()> *surface_production_observer = nullptr; ///< Current loaded-world callback, never a simulation clock owner.

/** Dispatch this invocation's actual native tick replay.
 * @param args Parameter-free original console callback arguments.
 * @return True after the current native observer ran.
 */
static bool ObserveSurfaceProduction(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_production_observer != nullptr);
	(*surface_production_observer)(); return true;
}

/** Save actual original owners at an exact naturally reached game tick.
 * @param path Fresh invocation-owned output.
 */
static void SaveSurfaceProduction(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Exact native simulation work preserves real products across original and ordinary-resaved inputs. */
TEST_CASE("Authentic Surface products replay exact native game ticks after ordinary reload", "[.surface-entry-production]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-production");
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve native assertions through original shutdown without rewriting failed gameplay state. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			REQUIRE(_game_mode == GameMode::Normal); const auto debt = Company::Get(_current_company)->current_loan; REQUIRE(debt > 0);
			SaveSurfaceProduction(paths.saves / "surface-entry-before-replay-440.sav");
			// Clear only the player's ordinary pause through its original public
			// command. Routing, save/load and other native reasons retain ownership.
			if (_pause_mode.Test(PauseMode::Normal)) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
			const auto initial = TimerGameTick::counter; const auto target = initial + 3000;
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(85);
			uint32_t calls = 0, waits = 0;
			// Count actual earned game ticks, not driver calls. A restored native
			// routing job may need wall time; only StateGameLoop may join it and
			// resume simulation. No timer, worker, cargo or vehicle field is edited.
			while (TimerGameTick::counter < target) {
				REQUIRE(std::chrono::steady_clock::now() < deadline);
				StateGameLoop(); ++calls;
				if (_pause_mode.Any()) { ++waits; std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
			}
			CHECK(TimerGameTick::counter == target); CHECK(Company::Get(_current_company)->current_loan == debt);
			SaveSurfaceProduction(paths.saves / "surface-entry-replayed-440.sav");
			fmt::print("SURFACE_ENTRY_REPLAY_COMPLETE initial={} final={} earned=3000 calls={} waits={}\n", initial, target, calls, waits); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_production_observer, &inspect);
	IConsole::CmdRegister("surface_entry_production", ObserveSurfaceProduction);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
