/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_boundary_native.cpp Original paid phase cancellations, spent allowance and guarded home identity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group16
 * Exact accepted input saves supply every hull, packet, shell and earned interval. No geometry, reserve, stock or progress is installed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_pod.h"
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

static std::function<void()> *boundary_observer = nullptr; ///< Original loaded-world observer, with no alternate controller or inventory.

/** @param args Parameter-free invocation. @return Whether the original loaded-world observer ran. */
static bool ObserveProbeBoundary(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(boundary_observer); (*boundary_observer)(); return true;
}

/** Earn one actual synchronized opportunity for retained cargo, native return and independent descent. */
static void ProbeBoundaryOpportunity()
{
	const auto before = GetAsteroidField().elapsed_ticks; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	AutoRestoreBackup pause(_pause_mode, PauseModes{});
	// Presentation pacing cannot earn return, age or descent progress. Only the original simulation tick may satisfy this bounded continuation.
	while (GetAsteroidField().elapsed_ticks == before) { REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
	REQUIRE(GetAsteroidField().elapsed_ticks == before + 1);
}

/** Actual paid cancellation never rolls back extracted packets or adopts an independent pending descent; spent service and occupied home guards preserve ownership. */
TEST_CASE("Original paid probe boundaries retain independent cargo and recovery owners", "[.probe-boundary-native]")
{
	const char *source = std::getenv("OTTD_PROBE_BOUNDARY_SOURCE"), *selected = std::getenv("OTTD_PROBE_BOUNDARY_PHASE"); REQUIRE(source); REQUIRE(selected);
	const std::string_view phase{selected}; const bool resave = std::getenv("OTTD_PROBE_BOUNDARY_RESAVE") != nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "probe-boundary"); const auto runtime = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve all original paid owners while executing only the selected real command boundary and native continuation. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles()); auto *miner = SpaceVehicle::GetIfValid(VehicleID{16}); REQUIRE(miner); REQUIRE(miner->generation == 2); REQUIRE(miner->manifest);
			auto &manifest = *miner->manifest; const auto target = miner->mission.target; auto *body = GetMutableAsteroidBody(target); REQUIRE(body);
			const auto reserve = body->remaining_reserve; const auto extracted = miner->extracted_units; const auto quantity = miner->cargo.TotalCount(); const auto work = miner->mission.work_ticks;
			const auto pods = manifest.pods; const auto quota = manifest.returns_remaining; const auto claims = manifest.pending_returns; const auto services = manifest.services_completed;
			const auto fleet = GetSpaceVehicleCount(); const auto next = GetNextSpaceVehicleGeneration(); const auto home = miner->construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center);
			const auto descents = CaptureSpaceDescents(); const auto digest = ComputeSpaceVehicleDigest(); const auto cash = Company::Get(miner->owner)->money;
			std::vector<CargoPacketID> packets; for (const auto *packet : *miner->cargo.Packets()) packets.push_back(packet->index);
			if (!resave) {
				if (phase == "exhausted") {
					REQUIRE(quota == 0); REQUIRE(claims == 0); REQUIRE(quantity == 0); REQUIRE(extracted == 40); REQUIRE(pods.empty());
					// Real spent allowance, not copied service state, refuses every new pursuit form without touching retained production or earned history.
					for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) CHECK(CmdSetSpaceMission(flags, miner->index, target.base()).Failed());
					CHECK(ComputeSpaceVehicleDigest() == digest); CHECK(Company::Get(miner->owner)->money == cash);
				} else if (phase == "home") {
					REQUIRE(descents.size() == 1); const auto &job = descents.front(); REQUIRE(job.center == home);
					// Occupied paid home cannot be demolished. Wrong center or shell incarnations cannot redirect the same pending packets even when their numeric IDs match.
					CHECK(CmdRemoveLaunchCenter({}, center->launch_center->tile).Failed()); CHECK(CmdRemoveLaunchCenter(DoCommandFlag::Execute, center->launch_center->tile).Failed());
					CHECK(CmdRetargetSpaceDescent(DoCommandFlag::Execute, job.pod.pod, job.pod.generation, home.station, home.generation + 1).Failed());
					CHECK(CmdRetargetSpaceDescent(DoCommandFlag::Execute, job.pod.pod, job.pod.generation + 1, home.station, home.generation).Failed());
					CHECK(ResolveLaunchCenter(home) == center); CHECK(miner->construction.home == home); CHECK(ComputeSpaceVehicleDigest() == digest); CHECK(Company::Get(miner->owner)->money == cash);
				} else {
					if (phase == "approach") { REQUIRE(miner->mission.phase == SpaceMissionPhase::Approach); REQUIRE(quantity == 0); }
					else if (phase == "partial") { REQUIRE(miner->mission.phase == SpaceMissionPhase::Mining); REQUIRE(quantity == 2); REQUIRE(work == 37); }
					else if (phase == "detached") { REQUIRE(miner->mission.phase == SpaceMissionPhase::Return); REQUIRE(quantity == 20); REQUIRE(body->mining_slot == VehicleID::Invalid()); }
					else { REQUIRE(phase == "pending"); REQUIRE(descents.size() == 1); REQUIRE(quantity == 0); REQUIRE(descents.front().remaining_ticks == 111); }
					REQUIRE(Command<Commands::SetSpaceMission>::Post(miner->index, UINT32_MAX)); CHECK(body->mining_slot == VehicleID::Invalid()); CHECK(miner->mission.phase == SpaceMissionPhase::Return); CHECK(miner->mission.work_ticks == 0);
					// More than a whole extraction interval proves cancellation cannot replay another batch. The independent pending descent earns75 real ticks and remains its sole cargo owner.
					for (uint elapsed = 0; elapsed < 75; ++elapsed) ProbeBoundaryOpportunity();
					CHECK((miner->mission.phase == SpaceMissionPhase::Return || miner->mission.phase == SpaceMissionPhase::Holding));
				}
			}
			CHECK(body->remaining_reserve == reserve); CHECK(miner->cargo.TotalCount() == quantity); CHECK(miner->extracted_units == extracted);
			CHECK(manifest.pods == pods); CHECK(manifest.returns_remaining == quota); CHECK(manifest.pending_returns == claims); CHECK(manifest.services_completed == services);
			std::vector<CargoPacketID> retained; for (const auto *packet : *miner->cargo.Packets()) retained.push_back(packet->index); CHECK(retained == packets);
			const auto after = CaptureSpaceDescents(); REQUIRE(after.size() == descents.size());
			// A canceled sender has no authority over the already released job. Original shell/sender incarnation, recovery target, packet IDs and payload survive unchanged while only its actual clock advances.
			for (size_t index = 0; index < after.size(); ++index) {
				const auto &a = after[index], &b = descents[index]; CHECK(a.pod == b.pod); CHECK(a.id == b.id); CHECK(a.sender == b.sender); CHECK(a.sender_generation == b.sender_generation); CHECK(a.center == b.center); CHECK(a.packets == b.packets); CHECK(a.quantity == b.quantity);
				CHECK(a.remaining_ticks == b.remaining_ticks - (!resave && phase == "pending" ? 75 : 0));
			}
			if (resave) CHECK(ComputeSpaceVehicleDigest() == digest);
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == next); CHECK(miner->construction.home == home); REQUIRE(ValidateSpaceVehicles());
			{ AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"}); REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("boundary-{}-438.sav", phase)), false) == SaveLoadResult::Ok); }
			fmt::print("PROBE_BOUNDARY native=true phase={} resave={} added_stock=false\n", phase, resave); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(boundary_observer, &observe); IConsole::CmdRegister("probe_boundary_test", ObserveProbeBoundary); const std::string config = runtime.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
