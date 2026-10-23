/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_motion_native.cpp Isolated edge/corner save boundaries and pre-decision native lease translation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group3
 * Existing source identities and resources stay immutable. Staged motion is an
 * explicit geometry fixture, not new launch or paid mission acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../settings_type.h"
#include "../saveload/saveload_func.h"
#include "../window_gui.h"
#include "../window_func.h"
#include "../map_layer.h"
#include "../map_layer_gui.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *motion_observer = nullptr; ///< Invocation-owned native game-start observer.

/** @param args Exact registered console command tokens. @return Whether the invocation's native observer ran. */
static bool ObserveAsteroidMotion(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(motion_observer != nullptr); (*motion_observer)(); return true;
}

/** Stage only bounded motion in an invocation copy, publish the actual field step and retain ordinary before/after saves. */
TEST_CASE("Native asteroid boundary fixtures retain hidden motion and actual pre-decision attachments", "[.asteroid-motion-native]")
{
	const char *input = std::getenv("OTTD_ASTEROID_MOTION_SOURCE"); REQUIRE(input != nullptr);
	const char *mode = std::getenv("OTTD_ASTEROID_MOTION_CASE"); REQUIRE(mode != nullptr);
	const bool native = std::string_view{mode}.starts_with("native-");
	const bool corner = std::string_view{mode}.ends_with("corner");
	const bool stage = std::getenv("OTTD_ASTEROID_MOTION_RESUME") == nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "asteroid-motion-native");
	const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Retain actual application/save ownership while isolating geometry mutations to this invocation. */
	std::function<void()> observe = [&] {
		try {
			REQUIRE(ValidateAsteroidField(GetAsteroidField()));
			OrbitalBodyID target = INVALID_ORBITAL_BODY;
			SpaceVehicle *miner = nullptr;
			if (native) {
				// Use an already existing actual lease; this fixture cannot create a hull, supplies, resources or mission identity.
				for (auto *vehicle : SpaceVehicle::Iterate()) {
					if (vehicle->mission.phase != SpaceMissionPhase::Mining) continue;
					const auto *body = GetMutableAsteroidBody(vehicle->mission.target);
					// A saved target alone is insufficient; the retained field must prove this exact native lease.
					if (body == nullptr || body->mining_slot != vehicle->index) continue;
					miner = vehicle; target = body->id; break;
				}
				REQUIRE(miner != nullptr);
			} else {
				// The404 source supplies ownerless observation records; it grants no later paid mining-controller acceptance.
				for (const auto &survey : GetAsteroidField().surveys) {
					if (survey.phase == SpaceSurveyPhase::Attached) { target = survey.target; break; }
				}
				REQUIRE(target != INVALID_ORBITAL_BODY);
			}
			if (stage) {
				auto field = GetAsteroidField();
				auto &body = field.bodies[target.base()];
				body.motion.position.x = field.band.max.x;
				if (corner) body.motion.position.y = field.band.max.y;
				body.motion.velocity_x = corner ? 11585 : 16384; body.motion.velocity_y = corner ? 11585 : 0;
				body.motion.remainder_x = 73; body.motion.remainder_y = corner ? 73 : 0;
				// Translate existing observation attachments with their body; never allocate a replacement preview or lease.
				for (auto &survey : field.surveys) {
					if (survey.phase != SpaceSurveyPhase::Attached) continue;
					const auto point = field.bodies[survey.target.base()].motion.position;
					survey.position = {point.x + SPACE_Q16, point.y - SPACE_Q16 / 2};
				}
				REQUIRE(InstallAsteroidField(std::move(field)));
				UpdateAttachedSpaceCraftTransforms();
			}
			SetViewportLayer(GetMainWindow(), MapLayer::Surface);
			/** @param name Invocation-owned checkpoint name. Save actual native owners with paused lzma transport. */
			auto save = [&](std::string_view name) {
				AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("{}.sav", name)), false) == SaveLoadResult::Ok);
			};
			save("asteroid-motion-before-424");
			const auto before = GetAsteroidField();
			const auto original = before.bodies[target.base()];
			const uint32_t extracted = miner == nullptr ? 0 : miner->extracted_units;
			TickAsteroidField(); // Deliberately observe the producer boundary before CallVehicleTicks can perform any work.
			const auto &reflected = GetAsteroidField().bodies[target.base()];
			CHECK(reflected.motion.velocity_x == -original.motion.velocity_x);
			CHECK(reflected.motion.remainder_x == (corner ? 40 : 29));
			if (corner) CHECK(reflected.motion.velocity_y == -original.motion.velocity_y);
			if (miner != nullptr) {
				CHECK(miner->position == SpacePoint{reflected.motion.position.x + miner->mission.attachment_offset.x,
					reflected.motion.position.y + miner->mission.attachment_offset.y});
				CHECK(miner->extracted_units == extracted); CHECK(reflected.remaining_reserve == original.remaining_reserve);
			}
			// Expected body kinematics are independent of craft work and camera; advance exactly37 actual additional simulation ticks.
			auto expected = before;
			for (auto &body : expected.bodies) AdvanceSpaceMotion(body.motion, expected.band);
			AutoRestoreBackup running(_pause_mode, PauseModes{});
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
			while (GetAsteroidField().elapsed_ticks < before.elapsed_ticks + 38) {
				REQUIRE(std::chrono::steady_clock::now() < deadline);
				const auto tick = GetAsteroidField().elapsed_ticks; StateGameLoop();
				if (GetAsteroidField().elapsed_ticks == tick) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
				REQUIRE(GetAsteroidField().elapsed_ticks == tick + 1);
				// Stable body identity and fractional motion must match the shared step even when the real mission changes phase.
				for (size_t i = 0; i < expected.bodies.size(); ++i) {
					AdvanceSpaceMotion(expected.bodies[i].motion, expected.band);
					CHECK(GetAsteroidField().bodies[i].motion == expected.bodies[i].motion);
				}
			}
			CHECK(GetAsteroidField().starter_input == before.starter_input);
			REQUIRE(ValidateAsteroidField(GetAsteroidField()));
			save("asteroid-motion-after-424");
			fmt::print("ASTEROID_MOTION_NATIVE case={} stage={} steps=38 hidden=true predecision=true target={}\n", mode, stage, target.base());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(motion_observer, &observe); IConsole::CmdRegister("asteroid_motion_test", ObserveAsteroidMotion);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
