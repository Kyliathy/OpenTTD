/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_conservation_native.cpp Exact ordinary phase resaves and one actual native next opportunity over original paid owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group17
 * Original approach, attachment, mining, detachment and descent saves provide all resources and earned work. Nothing is installed or advanced artificially.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <thread>
#include "../safeguards.h"

static std::function<void()> *conservation_observer = nullptr; ///< Observes exact loaded owners without supplying work, cargo or movement.

/** @param args Parameter-free invocation. @return Whether the installed original-world observer ran. */
static bool ObserveProbeConservation(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(conservation_observer); (*conservation_observer)(); return true;
}

/** Earn one original synchronized opportunity, leaving all motion, extraction, descent and upkeep to their native owners. */
static void ProbeConservationOpportunity()
{
	const auto before = GetAsteroidField().elapsed_ticks; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	AutoRestoreBackup pause(_pause_mode, PauseModes{});
	// Presentation pacing cannot earn the saved next decision. Only one original synchronized opportunity may move the body or advance work/descent.
	while (GetAsteroidField().elapsed_ticks == before) { REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
	REQUIRE(GetAsteroidField().elapsed_ticks == before + 1);
}

/** Each original paid phase must preserve its ordinary saved owners and exact next native decision, including partial work and independent descent custody. */
TEST_CASE("Original paid mission phases preserve ordinary saves and one native next opportunity", "[.probe-conservation-native]")
{
	const char *source = std::getenv("OTTD_PROBE_CONSERVATION_SOURCE"), *selected = std::getenv("OTTD_PROBE_CONSERVATION_PHASE"); REQUIRE(source); REQUIRE(selected);
	const std::string_view phase{selected}; const auto paths = GetTestCaseArtifactPaths("space", "probe-conservation"); const auto runtime = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Qualify only the exact retained native phase; no setup mutation can replace original paid cargo, work or service. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles()); auto *miner = SpaceVehicle::GetIfValid(VehicleID{16}); REQUIRE(miner); REQUIRE(miner->generation == 2); REQUIRE(miner->manifest);
			const auto *body = GetMutableAsteroidBody(miner->mission.target); REQUIRE(body); const auto reserve = body->remaining_reserve; const auto quantity = miner->cargo.TotalCount(); const auto extracted = miner->extracted_units;
			const auto work = miner->mission.work_ticks; const auto allowance = miner->mission.approach_ticks_left; const auto mission = miner->mission.phase;
			if (phase == "approach") { REQUIRE(mission == SpaceMissionPhase::Approach); REQUIRE(quantity == 0); }
			else if (phase == "attach") { REQUIRE(mission == SpaceMissionPhase::Mining); REQUIRE(work == 0); REQUIRE(quantity == 0); }
			else if (phase == "mine") { REQUIRE(mission == SpaceMissionPhase::Mining); REQUIRE(work == 37); REQUIRE(quantity == 2); }
			else if (phase == "detach") { REQUIRE(mission == SpaceMissionPhase::Return); REQUIRE(quantity == 20); }
			else { REQUIRE(phase == "descent"); REQUIRE(mission == SpaceMissionPhase::Holding); REQUIRE(quantity == 0); REQUIRE(CaptureSpaceDescents().size() == 1); REQUIRE(CaptureSpaceDescents().front().remaining_ticks == 111); }
			const auto pods = miner->manifest->pods; const auto quota = miner->manifest->returns_remaining; const auto pending = miner->manifest->pending_returns; const auto services = miner->manifest->services_completed;
			const auto fleet = GetSpaceVehicleCount(); const auto next = GetNextSpaceVehicleGeneration(); const auto digest = ComputeSpaceVehicleDigest();
			std::vector<CargoPacketID> packets; for (const auto *packet : *miner->cargo.Packets()) packets.push_back(packet->index);
			/** @param suffix Explicit own or next checkpoint. Save the original paused native owners without earning any opportunity. */
			auto save = [&](std::string_view suffix) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("conservation-{}-{}-438.sav", phase, suffix)), false) == SaveLoadResult::Ok);
			};
			save("own"); CHECK(ComputeSpaceVehicleDigest() == digest); ProbeConservationOpportunity();
			CHECK(body->remaining_reserve == reserve); CHECK(miner->cargo.TotalCount() == quantity); CHECK(miner->extracted_units == extracted);
			CHECK(miner->mission.work_ticks == work + (mission == SpaceMissionPhase::Mining ? 1 : 0));
			if (phase == "approach") { CHECK(miner->mission.phase == mission); CHECK(miner->mission.approach_ticks_left == allowance - 1); }
			if (mission == SpaceMissionPhase::Mining) { CHECK(miner->mission.phase == mission); CHECK(body->mining_slot == miner->index); CHECK(miner->position == SpacePoint{body->motion.position.x + miner->mission.attachment_offset.x, body->motion.position.y + miner->mission.attachment_offset.y}); }
			if (phase == "descent") { const auto descents = CaptureSpaceDescents(); REQUIRE(descents.size() == 1); CHECK(descents.front().remaining_ticks == 110); }
			CHECK(miner->manifest->pods == pods); CHECK(miner->manifest->returns_remaining == quota); CHECK(miner->manifest->pending_returns == pending); CHECK(miner->manifest->services_completed == services);
			std::vector<CargoPacketID> retained; for (const auto *packet : *miner->cargo.Packets()) retained.push_back(packet->index); CHECK(retained == packets);
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == next); REQUIRE(ValidateSpaceVehicles()); save("next");
			fmt::print("PROBE_CONSERVATION native=true phase={} opportunities=1 added_stock=false\n", phase); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(conservation_observer, &observe); IConsole::CmdRegister("probe_conservation_test", ObserveProbeConservation); const std::string config = runtime.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
