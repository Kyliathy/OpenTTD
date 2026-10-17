/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_interception_native.cpp Declared edge geometry over original paid approach/attachment owners and exact saved next decisions.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group15
 * Geometry is an explicit isolated control, not paid travel. No body, craft, packet, reserve, quota, work or fuel is created.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../multilayer_asteroid_catalogue.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <thread>
#include "../safeguards.h"

static std::function<void()> *interception_observer = nullptr; ///< Exact loaded-world observer; owns no alternate physics, lease or inventory.

/** @param args Parameter-free console invocation. @return Whether the exact loaded-world observer ran. */
static bool ObserveProbeInterception(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(interception_observer); (*interception_observer)(); return true; }

/** Earn exactly one original synchronized body/arbitration/vehicle opportunity, without copying a movement or lease controller. */
static void InterceptionOpportunity()
{
	const auto before = GetAsteroidField().elapsed_ticks; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10); AutoRestoreBackup pause(_pause_mode, PauseModes{});
	// Presentation pacing cannot supply a reflected body step, pursuit decision or attachment work; only the original opportunity satisfies this checkpoint.
	while (GetAsteroidField().elapsed_ticks == before) { REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
	REQUIRE(GetAsteroidField().elapsed_ticks == before + 1);
}

/** Original real paid approach and attachment owners must preserve exact next-step decisions when their isolated edge geometry is saved/reloaded. */
TEST_CASE("Saved approaching and latched edge probes retain native reflected next steps", "[.probe-interception-native]")
{
	const char *source = std::getenv("OTTD_PROBE_INTERCEPTION_SOURCE"), *selected = std::getenv("OTTD_PROBE_INTERCEPTION_PHASE"); REQUIRE(source); REQUIRE(selected);
	const std::string_view phase{selected}; const bool latched = phase.starts_with("latched"), next_step = phase.ends_with("next"); const bool resave = std::getenv("OTTD_PROBE_INTERCEPTION_RESAVE") != nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "probe-interception"); const auto runtime = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Retain original paid custody and accountable history while qualifying a declared geometry-only boundary. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles()); auto *miner = SpaceVehicle::GetIfValid(VehicleID{16}); REQUIRE(miner); REQUIRE(miner->generation == 2); REQUIRE(miner->manifest);
			REQUIRE(miner->mission.target == OrbitalBodyID{0}); REQUIRE(miner->mission.phase == (latched ? SpaceMissionPhase::Mining : SpaceMissionPhase::Approach));
			const auto reserve = GetAsteroidField().bodies[0].remaining_reserve; const auto extracted = miner->extracted_units; const auto quantity = miner->cargo.TotalCount(); const auto work = miner->mission.work_ticks;
			const auto pods = miner->manifest->pods; const auto allowance = miner->manifest->returns_remaining; const auto claims = miner->manifest->pending_returns; const auto history = miner->manifest->services_completed;
			const auto fleet = GetSpaceVehicleCount(); const auto generation = GetNextSpaceVehicleGeneration(); const auto money = Company::Get(miner->owner)->money;
			std::vector<CargoPacketID> packets; for (const auto *packet : *miner->cargo.Packets()) packets.push_back(packet->index);
			const auto digest = ComputeSpaceVehicleDigest();
			if (!resave) {
				if (!next_step) {
					auto field = GetAsteroidField(); REQUIRE(field.elapsed_ticks > 0); auto &body = field.bodies[0];
					// Explicit component geometry changes only this existing body's pose/canonical motion. Original saved starter evidence, resources, history, IDs and earned work remain untouched.
					body.motion.position.x = field.band.max.x; body.motion.velocity_x = 16384; body.motion.velocity_y = 0; body.motion.remainder_x = 73; body.motion.remainder_y = 0;
					REQUIRE(InstallAsteroidField(std::move(field))); UpdateAttachedSpaceCraftTransforms(); SyncSpacePods(*miner);
				} else {
					const auto &body = GetAsteroidField().bodies[0]; REQUIRE(body.motion.position.x == GetAsteroidField().band.max.x); REQUIRE(body.motion.velocity_x == 16384); REQUIRE(body.motion.remainder_x == 73);
					InterceptionOpportunity(); const auto &reflected = GetAsteroidField().bodies[0];
					CHECK(reflected.motion.position.x == GetAsteroidField().band.max.x - 222); CHECK(reflected.motion.velocity_x == -16384); CHECK(reflected.motion.remainder_x == 29);
					CHECK(miner->mission.work_ticks == work + (latched ? 1 : 0));
				}
			}
			const auto &body = GetAsteroidField().bodies[0]; CHECK(body.remaining_reserve == reserve); CHECK(miner->cargo.TotalCount() == quantity); CHECK(miner->extracted_units == extracted);
			CHECK(miner->mission.phase == (latched ? SpaceMissionPhase::Mining : SpaceMissionPhase::Approach)); CHECK(body.mining_slot == (latched ? miner->index : VehicleID::Invalid()));
			if (latched) CHECK(miner->position == SpacePoint{body.motion.position.x + miner->mission.attachment_offset.x, body.motion.position.y + miner->mission.attachment_offset.y});
			CHECK(miner->manifest->pods == pods); CHECK(miner->manifest->returns_remaining == allowance); CHECK(miner->manifest->pending_returns == claims); CHECK(miner->manifest->services_completed == history);
			std::vector<CargoPacketID> retained; for (const auto *packet : *miner->cargo.Packets()) retained.push_back(packet->index); CHECK(retained == packets);
			if (resave) CHECK(ComputeSpaceVehicleDigest() == digest);
			if (!next_step) CHECK(Company::Get(miner->owner)->money == money);
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == generation); REQUIRE(ValidateSpaceVehicles());
			{ AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"}); REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("interception-{}-438.sav", phase)), false) == SaveLoadResult::Ok); }
			fmt::print("PROBE_INTERCEPTION native=true phase={} resave={} geometry_fixture=true added_stock=false\n", phase, resave); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(interception_observer, &observe); IConsole::CmdRegister("probe_interception_test", ObserveProbeInterception); const std::string config = runtime.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
