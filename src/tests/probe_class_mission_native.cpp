/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_class_mission_native.cpp Actual paid missions for all three resources and separately declared inclusive-boundary release geometry.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group18
 * Paid inputs provide the original hull, shells, reserves and packets. Only the explicitly named boundary branch controls pose; it claims no earned travel.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../multilayer_content.h"
#include "../launch_center_cmd.h"
#include "../station_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <thread>
#include "../safeguards.h"

static std::function<void()> *class_mission_observer = nullptr; ///< Observes actual original paid owners, never an alternate mission controller.

/** @param args Parameter-free invocation. @return Whether the original-world observer ran. */
static bool ObserveProbeClassMission(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(class_mission_observer); (*class_mission_observer)(); return true;
}

/** Earn one actual synchronized opportunity, retaining native pursuit, finite extraction, return and independent descent ownership. */
static void ClassMissionOpportunity()
{
	const auto before = GetAsteroidField().elapsed_ticks; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10); AutoRestoreBackup pause(_pause_mode, PauseModes{});
	// Presentation pacing cannot earn resources or movement. Only the original synchronized opportunity may advance this paid mission.
	while (GetAsteroidField().elapsed_ticks == before) { REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
	REQUIRE(GetAsteroidField().elapsed_ticks == before + 1);
}

/** Each original resource class must fund a real full hold and recovery; the exact20-tile component separately verifies release geometry without claiming paid travel. */
TEST_CASE("Original paid small probe completes each asteroid class mission and inclusive boundary recovery", "[.probe-class-mission-native]")
{
	const char *source = std::getenv("OTTD_PROBE_CLASS_SOURCE"), *selected = std::getenv("OTTD_PROBE_CLASS_PHASE"), *target_env = std::getenv("OTTD_PROBE_CLASS_TARGET"); REQUIRE(source); REQUIRE(selected); REQUIRE(target_env);
	// The invocation names exactly one original starter class; reject arbitrary numeric text instead of using an unchecked legacy integer parser.
	REQUIRE(target_env[0] >= '0'); REQUIRE(target_env[0] <= '2'); REQUIRE(target_env[1] == '\0');
	const std::string_view phase{selected}; const uint32_t target = static_cast<uint32_t>(target_env[0] - '0');
	const auto paths = GetTestCaseArtifactPaths("space", "probe-class-mission"); const auto runtime = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Keep every resource and paid identity under its original owner while executing only this selected real mission segment. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles()); auto *miner = SpaceVehicle::GetIfValid(VehicleID{16}); REQUIRE(miner); REQUIRE(miner->generation == 2); REQUIRE(miner->manifest); REQUIRE(GetSpaceVehicleModel(*miner) == 15);
			auto *body = GetMutableAsteroidBody(OrbitalBodyID{target}); REQUIRE(body); REQUIRE(to_underlying(body->resource_class) == target);
			const auto cargo = GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[target].cargo); REQUIRE(IsValidCargoType(cargo)); REQUIRE(CargoSpec::Get(cargo)->weight == 16); REQUIRE(GetSpaceFreightCapacity(15, cargo) == 20);
			auto &manifest = *miner->manifest; const auto home = miner->construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center); const auto anchor = *GetLaunchCenterPlaneReference(center->launch_center->tile);
			const auto initial = body->initial_reserve; const auto history = manifest.services_completed; const auto fleet = GetSpaceVehicleCount(); const auto next = GetNextSpaceVehicleGeneration();
			/** @param suffix Actual loaded, region, pending or recovered original checkpoint. Persist paused owners without earning work. */
			auto save = [&](std::string_view suffix) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("class-{}-{}-438.sav", target, suffix)), false) == SaveLoadResult::Ok);
			};
			if (phase == "mine") {
				REQUIRE(miner->cargo.TotalCount() == 0); REQUIRE(miner->extracted_units == 0); REQUIRE(body->remaining_reserve == initial); REQUIRE(manifest.returns_remaining == 2); REQUIRE(manifest.pods.size() == 2);
				REQUIRE(Command<Commands::SetSpaceMission>::Post(miner->index, target)); const auto allowance = miner->mission.approach_ticks_left;
				// Actual native pursuit earns attachment and all740 work opportunities; the frozen approach budget bounds the entire full-hold segment without any installed interval or cargo.
				for (uint elapsed = 0; miner->mission.phase != SpaceMissionPhase::Return; ++elapsed) { REQUIRE(elapsed < allowance + 742); ClassMissionOpportunity(); }
				CHECK(miner->cargo.TotalCount() == 20); CHECK(miner->extracted_units == 20); CHECK(body->remaining_reserve == initial - 20); CHECK(body->mining_slot == VehicleID::Invalid()); save("loaded");
			} else if (phase == "own") {
				REQUIRE(miner->cargo.TotalCount() == 0); REQUIRE(miner->extracted_units == 20); REQUIRE(manifest.returns_remaining == 1); REQUIRE(manifest.pending_returns == 0); REQUIRE(CaptureSpaceDescents().empty()); save("recovered");
			} else {
				REQUIRE((phase == "return" || phase == "boundary")); REQUIRE(miner->mission.target == OrbitalBodyID{target}); REQUIRE(miner->cargo.TotalCount() == 20); REQUIRE(miner->extracted_units == 20); REQUIRE(manifest.pods.size() == 2);
				if (phase == "return") {
					// Only actual return movement can satisfy the paid mission's off-anchor circle entry. No position assignment is used on this path.
					for (uint elapsed = 0; miner->mission.phase == SpaceMissionPhase::Return; ++elapsed) { REQUIRE(elapsed < 3000); ClassMissionOpportunity(); }
					REQUIRE(miner->mission.phase == SpaceMissionPhase::Holding); REQUIRE(IsInsideLaunchCenterDropZone(miner->position, anchor)); CHECK(miner->position != anchor);
				} else {
					// Explicit component geometry uses the same real paid hold/shell. One Q16 unit outside must refuse; exact20 tiles must admit. This is not an earned paid route or an installed resource/work interval.
					REQUIRE(anchor.x + 20 * SPACE_Q16 + 1 <= int64_t{GetAsteroidField().width - 1} * SPACE_Q16);
					miner->position = {anchor.x + 20 * SPACE_Q16 + 1, anchor.y}; miner->UpdateSpacePosition(); const auto shell = manifest.pods.front(); const auto digest = ComputeSpaceVehicleDigest();
					CHECK(CmdReleaseOrbitalCargo({}, miner->index, miner->generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{20}).Failed());
					CHECK(CmdReleaseOrbitalCargo(DoCommandFlag::Execute, miner->index, miner->generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{20}).Failed()); CHECK(ComputeSpaceVehicleDigest() == digest);
					miner->position = {anchor.x + 20 * SPACE_Q16, anchor.y}; miner->UpdateSpacePosition(); REQUIRE(IsInsideLaunchCenterDropZone(miner->position, anchor));
				}
				save("region"); const auto point = miner->position; const auto shell = manifest.pods.front(); const auto money = Company::Get(miner->owner)->money; const auto stock = center->goods[cargo].TotalCount();
				REQUIRE(Command<Commands::ReleaseOrbitalCargo>::Post(miner->index, miner->generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{20}));
				REQUIRE(CaptureSpaceDescents().size() == 1); const auto job = CaptureSpaceDescents().front(); CHECK(job.pod == shell); CHECK(job.release_point == point); CHECK(job.quantity == 20); CHECK(job.cargo == cargo); CHECK(job.freight_mass == 320); CHECK(job.sender_model == 15);
				CHECK(miner->cargo.TotalCount() == 0); CHECK(manifest.pending_returns == 1); CHECK(manifest.returns_remaining == 2); CHECK(Company::Get(miner->owner)->money == money); save("pending");
				// The original148-tick descent owns its packets until actual recovery. No direct inventory move, countdown reset or replaced shell may satisfy delivery.
				for (uint elapsed = 0; elapsed < job.remaining_ticks; ++elapsed) ClassMissionOpportunity();
				CHECK(CaptureSpaceDescents().empty()); CHECK(ResolveSpacePod(shell) == nullptr); CHECK(manifest.pending_returns == 0); CHECK(manifest.returns_remaining == 1); CHECK(manifest.pods.size() == 1); CHECK(center->goods[cargo].TotalCount() == stock + 20);
				CHECK(CmdReleaseOrbitalCargo({}, miner->index, miner->generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{20}).Failed()); save("recovered");
			}
			CHECK(miner->cargo_type == cargo); CHECK(body->remaining_reserve == initial - 20); CHECK(manifest.services_completed == history); CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == next); REQUIRE(ValidateSpaceVehicles());
			fmt::print("PROBE_CLASS native=true target={} phase={} boundary_geometry={} added_stock=false\n", target, phase, phase == "boundary"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(class_mission_observer, &observe); IConsole::CmdRegister("probe_class_mission_test", ObserveProbeClassMission); const std::string config = runtime.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
