/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_extraction_native.cpp Original paid probe earns finite work, packet provenance and full-hold detachment.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group10
 * The exact accepted interception supplies every input owner; no cargo, reserve, work or pose is installed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../vehicle_cmd.h"
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

static std::function<void()> *extraction_observer = nullptr; ///< Per-process exact loaded-world observer, never another material/work owner.

/** @param args Parameter-free actual native console callback. @return Whether this exact observer ran. */
static bool ObserveExtraction(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(extraction_observer); (*extraction_observer)(); return true; }

/** Offer one actual normal native field/vehicle opportunity; skipped presentation frames cannot earn extraction work. */
static void ExtractionOpportunity()
{
	const auto before = GetAsteroidField().elapsed_ticks; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10); AutoRestoreBackup pause(_pause_mode, PauseModes{});
	// Only the original synchronized schedule moves the finite body, charges upkeep and advances the actual leased hull.
	while (GetAsteroidField().elapsed_ticks == before) { REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
	REQUIRE(GetAsteroidField().elapsed_ticks == before + 1);
}

/** The original paid first mission must conserve finite resource, exact shared-body attachment and every emitted packet. */
TEST_CASE("Original paid probe conserves finite extraction through stopped partial and full holds", "[.probe-extraction-native]")
{
	const char *source = std::getenv("OTTD_PROBE_EXTRACTION_SOURCE"), *selected = std::getenv("OTTD_PROBE_EXTRACTION_PHASE"); REQUIRE(source); REQUIRE(selected);
	const std::string_view phase{selected}; const bool resave = std::getenv("OTTD_PROBE_EXTRACTION_RESAVE") != nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "probe-extraction"); const auto runtime = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Observe only the genuine original paid world and retain assertion failures across orderly application teardown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles()); SpaceVehicle *miner = nullptr;
			// Same-model similarity cannot replace the original Earth-paid first miner and its actual attachment history.
			for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 15) { REQUIRE(miner == nullptr); miner = craft; }
			REQUIRE(miner); REQUIRE(miner->index == VehicleID{16}); REQUIRE(miner->generation == 2); REQUIRE(miner->manifest);
			const auto *body = GetMutableAsteroidBody(OrbitalBodyID{0}); REQUIRE(body); REQUIRE(miner->mission.target == body->id);
			const auto before = ComputeSpaceVehicleDigest(); const auto fleet = GetSpaceVehicleCount();
			if (!resave) {
				uint ticks = 0;
				if (phase == "stopped") { REQUIRE(miner->mission.phase == SpaceMissionPhase::Mining); REQUIRE(miner->mission.work_ticks == 0); REQUIRE(Command<Commands::StartStopVehicle>::Post(miner->tile, miner->index, true)); ticks = 37; }
				else if (phase == "partial") { REQUIRE(miner->vehstatus.Test(VehState::Stopped)); REQUIRE(Command<Commands::StartStopVehicle>::Post(miner->tile, miner->index, true)); ticks = 37; }
				else if (phase == "first") { REQUIRE(miner->mission.work_ticks == 37); REQUIRE(miner->cargo.TotalCount() == 0); ticks = 37; }
				else if (phase == "second-partial") { REQUIRE(miner->cargo.TotalCount() == 2); REQUIRE(miner->mission.work_ticks == 0); ticks = 37; }
				else { REQUIRE(phase == "loaded"); REQUIRE(miner->cargo.TotalCount() == 2); REQUIRE(miner->mission.work_ticks == 37); ticks = 629; }
				// Compare this hull's exact native cost remainder on every actual opportunity. Stopped attachment still follows the body; only eligible work may earn a finite batch.
				for (uint elapsed = 0; elapsed < ticks; ++elapsed) {
					const auto profit = miner->profit_this_year; const uint accrual = miner->mission.cost_remainder + (IsSpaceMiningWorkTick(*miner) ? 500 : 100);
					ExtractionOpportunity(); CHECK(miner->profit_this_year == profit - Money{accrual / 74} * 256); CHECK(miner->mission.cost_remainder == accrual % 74);
					CHECK(body->remaining_reserve + miner->cargo.TotalCount() == 1760);
					if (miner->mission.phase == SpaceMissionPhase::Mining) CHECK(miner->position == SpacePoint{body->motion.position.x + miner->mission.attachment_offset.x, body->motion.position.y + miner->mission.attachment_offset.y});
				}
			}
			const auto quantity = phase == "loaded" ? 20 : phase == "first" || phase == "second-partial" ? 2 : 0;
			CHECK(miner->cargo.TotalCount() == quantity); CHECK(miner->extracted_units == quantity); CHECK(body->remaining_reserve == 1760 - quantity);
			CHECK(miner->mission.work_ticks == (phase == "partial" || phase == "second-partial" ? 37 : 0));
			CHECK(miner->manifest->returns_remaining == 2); CHECK(miner->manifest->pods.size() == 2); CHECK(GetSpaceVehicleCount() == fleet);
			if (phase == "loaded") { CHECK(miner->mission.phase == SpaceMissionPhase::Return); CHECK(body->mining_slot == VehicleID::Invalid()); }
			else { CHECK(miner->mission.phase == SpaceMissionPhase::Mining); CHECK(body->mining_slot == miner->index); }
			// Every real emitted packet retains the actual asteroid identity and original extraction plane; subsequent body movement cannot impersonate an Industry or Surface pickup.
			for (const auto *packet : *miner->cargo.Packets()) { CHECK(packet->GetSource().type == SourceType::Asteroid); CHECK(packet->GetSource().id == body->id.base()); CHECK(GetTileLayer(packet->GetSourceTile()) == MapLayer::Space); CHECK(packet->GetFirstStation() == StationID::Invalid()); }
			if (resave) CHECK(ComputeSpaceVehicleDigest() == before);
			REQUIRE(ValidateSpaceVehicles()); { AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("extraction-{}-438.sav", phase)), false) == SaveLoadResult::Ok); }
			fmt::print("PROBE_EXTRACTION native=true phase={} resave={} added_stock=false\n", phase, resave); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(extraction_observer, &observe); IConsole::CmdRegister("probe_extraction_test", ObserveExtraction); const std::string config = runtime.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
