/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_generation_native.cpp Real save/preview/motion continuation for frozen starter inputs.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../settings_type.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *starter_observer = nullptr; ///< Actual game-start callback owned only by the current native invocation.

/**
 * Execute the native observer requested by the invocation's game-start script.
 * @param args Console command tokens; no extra argument can select a different field or source.
 * @return Whether the registered native observer ran.
 */
static bool ObserveAsteroidStarter(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(starter_observer != nullptr); (*starter_observer)(); return true;
}

/** Load actual fields, run native ticks and save before/after without re-authoring bodies, terrain, companies or cargo. */
TEST_CASE("Native asteroid fields retain initial access through preview and actual motion", "[.asteroid-generation-native]")
{
	const char *input = std::getenv("OTTD_ASTEROID_STARTER_SOURCE"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "asteroid-generation-native");
	const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Run the actual field owner and save writer; captured exceptions are rethrown after the application exits. */
	std::function<void()> observe = [&] {
		try {
			REQUIRE(ValidateAsteroidField(GetAsteroidField())); REQUIRE(GetAsteroidField().bodies.size() >= 3);
			const auto initial = GetAsteroidField();
			const auto digest = ComputeAsteroidFieldDigest(initial);
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(GetAsteroidField() == initial); CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == digest);
			if (const char *corrupt = std::getenv("OTTD_ASTEROID_STARTER_CORRUPT")) {
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(GetAsteroidField() == initial); CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == digest);
				fmt::print("ASTEROID_STARTER preview_reject=true unchanged=true\n");
			} else {
				/** Save actual gameplay owners with paused lzma transport. @param name Invocation-local before/after checkpoint name. */
				auto save = [&](std::string_view name) {
					AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
					AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
					REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("{}.sav", name)), false) == SaveLoadResult::Ok);
				};
				save("asteroid-starter-before-424");
				AutoRestoreBackup running(_pause_mode, PauseModes{});
				const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
				// Advance the real synchronized controller; temporary CargoDist pauses cannot count as completed motion ticks.
				while (GetAsteroidField().elapsed_ticks < initial.elapsed_ticks + 37) {
					REQUIRE(std::chrono::steady_clock::now() < deadline);
					const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1);
					CHECK(GetAsteroidField().starter_input == initial.starter_input);
				}
				REQUIRE(ValidateAsteroidField(GetAsteroidField()));
				CHECK(GetAsteroidField().bodies.size() == initial.bodies.size());
				CHECK(GetAsteroidField().bodies.front().motion.position != initial.bodies.front().motion.position);
				// Motion cannot create resources or change generated class/size/composition; legacy fields cannot invent initial history.
				for (size_t body = 0; body < initial.bodies.size(); ++body) {
					const auto &before = initial.bodies[body], &after = GetAsteroidField().bodies[body];
					CHECK(after.resource_class == before.resource_class); CHECK(after.size == before.size);
					CHECK(after.initial_reserve == before.initial_reserve); CHECK(after.remaining_reserve == before.remaining_reserve);
				}
				save("asteroid-starter-after-424");
				fmt::print("ASTEROID_STARTER native=true ticks=37 kind={} initial_bound={} bodies={} no_reroll=true\n",
					to_underlying(initial.starter_input.kind), initial.starter_input.initial_approach_distance_squared, initial.bodies.size());
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(starter_observer, &observe); IConsole::CmdRegister("asteroid_starter_test", ObserveAsteroidStarter);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
