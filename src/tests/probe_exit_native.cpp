/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_exit_native.cpp Original paid recall retains actual cargo and safe departure geometry, ending uncommitted attachment work.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group13
 * Exact accepted partial/full/service saves supply every hull, packet, reserve and paid service owner.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_departure.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_structure.h"
#include "../asteroid.h"
#include "../vehicle_cmd.h"
#include "../launch_center_cmd.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <thread>
#include "../safeguards.h"

static std::function<void()> *exit_observer = nullptr; ///< Loaded-world observer, never an alternative mission or stock owner.

/** @param args Parameter-free native console invocation. @return Whether the exact loaded-world observer ran. */
static bool ObserveProbeExit(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(exit_observer); (*exit_observer)(); return true; }

/** Offer one genuine native opportunity; only the original simulation may move the canceled hull or its former target. */
static void ExitOpportunity()
{
	const auto before = GetAsteroidField().elapsed_ticks; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10); AutoRestoreBackup pause(_pause_mode, PauseModes{});
	// Presentation pacing cannot supply a departure, release a lease or consume the retained service countdown.
	while (GetAsteroidField().elapsed_ticks == before) { REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
	REQUIRE(GetAsteroidField().elapsed_ticks == before + 1);
}

/** Commands used by the inspector must refuse incompatible work/refits and preserve all accountable paid owners through cancellation/reload. */
TEST_CASE("Original paid recall preserves partial cargo work and service ownership", "[.probe-exit-native]")
{
	const char *source = std::getenv("OTTD_PROBE_EXIT_SOURCE"), *selected = std::getenv("OTTD_PROBE_EXIT_PHASE"); REQUIRE(source); REQUIRE(selected);
	const std::string_view phase{selected}; const bool resave = std::getenv("OTTD_PROBE_EXIT_RESAVE") != nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "probe-exits"); const auto runtime = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Inspect only the original accepted paid world, retaining failures through orderly native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles()); auto *miner = SpaceVehicle::GetIfValid(VehicleID{16}); REQUIRE(miner); REQUIRE(miner->generation == 2); REQUIRE(miner->manifest);
			auto &manifest = *miner->manifest; const auto *body = GetMutableAsteroidBody(miner->mission.target); REQUIRE(body); const auto target = body->id;
			const auto reserve = body->remaining_reserve, extracted = miner->extracted_units; const auto work = miner->mission.work_ticks; const auto quantity = miner->cargo.TotalCount();
			const auto quota = manifest.returns_remaining, pending = manifest.pending_returns; const auto pods = manifest.pods; const auto services = manifest.services_completed; const auto ticks = manifest.service_ticks;
			std::vector<CargoPacketID> packets; for (const auto *packet : *miner->cargo.Packets()) packets.push_back(packet->index);
			const auto fleet = GetSpaceVehicleCount(); const auto next = GetNextSpaceVehicleGeneration(); const auto before = ComputeSpaceVehicleDigest();
			if (!resave) {
				// Ordinary depot refits are never a route to relabel accountable Space packets; both estimate forms must refuse before mutation.
				for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) {
					CHECK(std::get<0>(CmdRefitVehicle(flags, miner->index, miner->cargo_type, uint8_t{0}, false, true, uint8_t{1})).Failed());
				}
				if (phase == "service") {
					REQUIRE(ticks == 74); CHECK(CmdSetSpaceMission({}, miner->index, UINT32_MAX).Failed());
					CHECK(CmdEditSpaceOrder({}, miner->index, miner->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}).Failed());
					CHECK(CmdServiceLaunchProbe({}, miner->index, miner->generation, false).Failed()); CHECK(ComputeSpaceVehicleDigest() == before);
				} else {
					REQUIRE(ticks == 0); REQUIRE(quantity == (phase == "loaded" ? 20 : 2)); CHECK(CmdSetSpaceMission({}, miner->index, UINT32_MAX - 1).Failed());
					if (phase == "loaded") CHECK(CmdSetSpaceMission({}, miner->index, target.base()).Failed());
					// Loaded actual ore may not change resource meaning through a different natural target; the command retains the whole old hold.
					const AsteroidBody *other = nullptr;
					for (const auto &candidate : GetAsteroidField().bodies) if (candidate.resource_class != body->resource_class) { other = &candidate; break; }
					REQUIRE(other); CHECK(CmdSetSpaceMission({}, miner->index, other->id.base()).Failed());
					// The inspector uses this same pure quote for Recall. A mixed estimate retains the old lease, partial interval and packets; actual cancellation later discards only uncommitted work belonging to that attachment.
					for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) CHECK(CmdSetSpaceMission(flags, miner->index, UINT32_MAX).Succeeded());
					CHECK(ComputeSpaceVehicleDigest() == before); REQUIRE(Command<Commands::SetSpaceMission>::Post(miner->index, UINT32_MAX));
					CHECK(miner->mission.target == target); CHECK(miner->mission.phase == SpaceMissionPhase::Return); CHECK(body->mining_slot == VehicleID::Invalid());
					if (phase == "partial") { const auto exit = GetSpaceMiningDepartureDestination(*miner, miner->home); REQUIRE(exit); CHECK(*exit != miner->position); }
					// The real canceled hull must clear shared departure geometry and reach its own cargo circle; no forced pose or elapsed-time guess can declare recovery.
					for (uint elapsed = 0; miner->mission.phase == SpaceMissionPhase::Return; ++elapsed) { REQUIRE(elapsed < 3000); ExitOpportunity(); }
					CHECK(miner->mission.phase == SpaceMissionPhase::Holding);
				}
			}
			CHECK(miner->cargo.TotalCount() == quantity); CHECK(miner->extracted_units == extracted); CHECK(miner->mission.work_ticks == (resave || phase == "service" ? work : 0)); CHECK(body->remaining_reserve == reserve);
			CHECK(manifest.returns_remaining == quota); CHECK(manifest.pending_returns == pending); CHECK(manifest.pods == pods); CHECK(manifest.services_completed == services); CHECK(manifest.service_ticks == ticks);
			std::vector<CargoPacketID> retained; for (const auto *packet : *miner->cargo.Packets()) retained.push_back(packet->index); CHECK(retained == packets);
			if (resave) CHECK(ComputeSpaceVehicleDigest() == before);
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == next); CHECK(CaptureSpaceDescents().empty()); CHECK(CaptureSpaceStructures().empty()); REQUIRE(ValidateSpaceVehicles());
			{ AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"}); REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("exit-{}-438.sav", phase)), false) == SaveLoadResult::Ok); }
			fmt::print("PROBE_EXIT native=true phase={} resave={} added_stock=false\n", phase, resave); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(exit_observer, &observe); IConsole::CmdRegister("probe_exit_test", ObserveProbeExit); const std::string config = runtime.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
