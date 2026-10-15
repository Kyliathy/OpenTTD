/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_depletion_native.cpp Isolated zero-reserve lease boundary and actual ordinary persistence.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group5
 * The invocation stages an explicit depleted record in an existing405 copy.
 * It does not claim production of its historical missing reserve or a new paid mission.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_orders.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../company_func.h"
#include "../openttd.h"
#include "../settings_type.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *depletion_observer = nullptr; ///< Invocation-owned native game-start callback.

/** @param args Exact registered console command tokens. @return Whether the native observer ran. */
static bool ObserveAsteroidDepletion(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(depletion_observer != nullptr); (*depletion_observer)(); return true;
}

/** Stage only zero reserve, let the actual controller release its existing lease, and preserve before/after owners. */
TEST_CASE("Native depleted-body boundaries retain cargo and refuse identity reuse", "[.asteroid-depletion-native]")
{
	const char *input = std::getenv("OTTD_ASTEROID_DEPLETION_SOURCE"); REQUIRE(input != nullptr);
	const bool stage = std::getenv("OTTD_ASTEROID_DEPLETION_RESUME") == nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "asteroid-depletion-native");
	const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Exercise the real load/tick/save owner; preserve caught assertions until application teardown completes. */
	std::function<void()> observe = [&] {
		try {
			REQUIRE(ValidateAsteroidField(GetAsteroidField())); REQUIRE(ValidateSpaceVehicles());
			SpaceVehicle *miner = nullptr;
			// Select only an existing actual lease; the boundary fixture creates no hull, packet, resource or company identity.
			for (auto *vehicle : SpaceVehicle::Iterate()) {
				if (vehicle->mission.phase != SpaceMissionPhase::Mining) continue;
				const auto *body = GetMutableAsteroidBody(vehicle->mission.target);
				// An inert target reference cannot supply the native lease required for the depletion boundary.
				if (body == nullptr || body->mining_slot != vehicle->index) continue;
				miner = vehicle; break;
			}
			REQUIRE(miner != nullptr);
			const auto target = miner->mission.target;
			if (stage) {
				auto field = GetAsteroidField(); field.bodies[target.base()].remaining_reserve = 0;
				REQUIRE(InstallAsteroidField(std::move(field)));
			}
			REQUIRE(GetMutableAsteroidBody(target)->remaining_reserve == 0);
			AutoRestoreBackup actor(_current_company, miner->owner);
			const auto cargo = miner->cargo.TotalCount(); const auto extracted = miner->extracted_units;
			SpacePoint point{123, 456};
			CHECK(ResolveSpaceNavigationTarget(*miner, {SpaceOrderTargetKind::Body, UINT32_MAX, 0}, point) == SpaceOrderBlock::TargetRemoved);
			CHECK(ResolveSpaceNavigationTarget(*miner, {SpaceOrderTargetKind::Body, static_cast<uint32_t>(GetAsteroidField().bodies.size()), 0}, point) == SpaceOrderBlock::TargetRemoved);
			CHECK(point == SpacePoint{123, 456});
			CHECK(ResolveSpaceNavigationTarget(*miner, {SpaceOrderTargetKind::Body, target.base(), 0}, point) == SpaceOrderBlock::None);
			CHECK(CmdSetSpaceMission({}, miner->index, target.base()).Failed());
			/** @param name Invocation-owned checkpoint name. Persist real owners with paused lzma transport. */
			auto save = [&](std::string_view name) {
				AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("{}.sav", name)), false) == SaveLoadResult::Ok);
			};
			save("asteroid-depletion-before-424");
			const auto initial = GetAsteroidField();
			AutoRestoreBackup running(_pause_mode, PauseModes{});
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
			// Exactly one real working tick must leave the exhausted body alive while releasing its actual mining lease.
			while (GetAsteroidField().elapsed_ticks == initial.elapsed_ticks) {
				REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop();
				if (GetAsteroidField().elapsed_ticks == initial.elapsed_ticks) std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			CHECK(GetAsteroidField().elapsed_ticks == initial.elapsed_ticks + 1);
			const auto *body = GetMutableAsteroidBody(target); REQUIRE(body != nullptr);
			CHECK(body->id == target); CHECK(body->remaining_reserve == 0);
			CHECK(body->initial_reserve == initial.bodies[target.base()].initial_reserve);
			CHECK(body->motion.position != initial.bodies[target.base()].motion.position);
			CHECK(body->mining_slot == VehicleID::Invalid()); CHECK(miner->mission.phase == SpaceMissionPhase::Return);
			CHECK(miner->cargo.TotalCount() == cargo); CHECK(miner->extracted_units == extracted);
			CHECK(GetAsteroidField().bodies.size() == initial.bodies.size());
			CHECK(CmdSetSpaceMission({}, miner->index, target.base()).Failed());
			REQUIRE(ValidateSpaceVehicles());
			save("asteroid-depletion-after-424");
			fmt::print("ASTEROID_DEPLETION native=true record_boundary=true ticks=1 target={} zero=true retained=true cargo_unchanged=true\n", target.base());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(depletion_observer, &observe); IConsole::CmdRegister("asteroid_depletion_test", ObserveAsteroidDepletion);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
