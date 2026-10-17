/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_repeat_native.cpp Actual paid descent, shell replacement and onboard service before repeated station-free mining.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group12
 * The exact accepted return-circle world supplies every hull, packet, shell, service part and fuel unit.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_transition.h"
#include "../space_structure.h"
#include "../asteroid.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../multilayer_space_content.h"
#include "../station_base.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <thread>
#include "../safeguards.h"

static std::function<void()> *repeat_observer = nullptr; ///< Process-local observer; owns no gameplay resource or progress.

/** @param args Parameter-free console invocation. @return Whether the exact loaded-world observer ran. */
static bool ObserveProbeRepeat(std::span<std::string_view> args) { REQUIRE(args.size() == 1); REQUIRE(repeat_observer); (*repeat_observer)(); return true; }

/** Earn one actual synchronized opportunity through the normal body, descent, manifest and vehicle owners. */
static void RepeatOpportunity()
{
	const auto before = GetAsteroidField().elapsed_ticks; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	AutoRestoreBackup pause(_pause_mode, PauseModes{});
	// Presentation pacing grants no descent, mining, travel or service progress; only the native synchronized opportunity satisfies this boundary.
	while (GetAsteroidField().elapsed_ticks == before) { REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
	REQUIRE(GetAsteroidField().elapsed_ticks == before + 1);
}

/** Commit one actual whole hold and qualify retry ownership without granting a second job.
 * @param miner Original paid orbital sender with actual held ore and carried shell.
 */
static void ReleaseRepeatHold(SpaceVehicle &miner)
{
	auto &manifest = *miner.manifest; const auto home = miner.construction.home; REQUIRE(miner.cargo.TotalCount() == 20); REQUIRE(!manifest.pods.empty());
	const auto shell = manifest.pods.front(); const auto next = GetNextSpaceDescentID(); const auto money = Company::Get(miner.owner)->money;
	const auto before = ComputeSpaceVehicleDigest(); const auto quote = CmdReleaseOrbitalCargo({}, miner.index, miner.generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{20});
	REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == 0); CHECK(ComputeSpaceVehicleDigest() == before); CHECK(CaptureSpaceDescents().empty());
	REQUIRE(Command<Commands::ReleaseOrbitalCargo>::Post(miner.index, miner.generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{20}));
	const auto *pod = ResolveSpacePod(shell); REQUIRE(pod); REQUIRE(pod->descent); CHECK(pod->descent->id == next); CHECK(pod->descent->quantity == 20); CHECK(pod->descent->packets.TotalCount() == 20);
	CHECK(miner.cargo.TotalCount() == 0); CHECK(manifest.pending_returns == 1); CHECK(GetNextSpaceDescentID() == next + 1); CHECK(Company::Get(miner.owner)->money == money);
	CHECK(CmdReleaseOrbitalCargo({}, miner.index, miner.generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{20}).Failed());
	CHECK_FALSE(Command<Commands::ReleaseOrbitalCargo>::Post(miner.index, miner.generation, home.station, home.generation, shell.pod, shell.generation, uint16_t{20}));
	CHECK(GetNextSpaceDescentID() == next + 1); CHECK(CaptureSpaceDescents().size() == 1);
	// A partly used allowance is not an early service purchase; pending recovery cannot earn service or consume the prepaid parts.
	CHECK(CmdServiceLaunchProbe({}, miner.index, miner.generation, false).Failed());
}

/** Settle only the original remaining countdown, then verify its shell cannot recover twice.
 * @param miner Original sender retaining the actual pending return claim.
 */
static void RecoverRepeatHold(SpaceVehicle &miner)
{
	const auto records = CaptureSpaceDescents(); REQUIRE(records.size() == 1); const auto &record = records.front(); const auto shell = record.pod;
	const auto remaining = record.remaining_ticks; REQUIRE(remaining > 0); const auto quota = miner.manifest->returns_remaining; const auto next = GetNextSpaceDescentID();
	// The saved native job alone determines remaining work. No timer reset, packet rewrite or installed credit may finance recovery.
	for (uint elapsed = 0; elapsed < remaining; ++elapsed) RepeatOpportunity();
	CHECK(CaptureSpaceDescents().empty()); CHECK(ResolveSpacePod(shell) == nullptr); CHECK(miner.manifest->pending_returns == 0); CHECK(miner.manifest->returns_remaining == quota - 1);
	CHECK(GetNextSpaceDescentID() == next); CHECK(miner.cargo.TotalCount() == 0);
}

/** Deliver two original stored shells through the same actual paid rocket and retain exhausted allowance.
 * @param miner Original empty receiver; onboard parts remain its own actual paid stock.
 */
static void ResupplyRepeatPods(SpaceVehicle &miner)
{
	const auto home = miner.construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center); REQUIRE(center->launch_operations); REQUIRE(!center->launch_operations->pad_queue.empty());
	// The original physical queue admits the existing paid19/gen5 rocket before15; selecting another hull by model or bypassing the queue would invent pad rights.
	auto *rocket = ResolveSpaceCraft(center->launch_operations->pad_queue.front()); REQUIRE(rocket); REQUIRE(rocket->index == VehicleID{19}); REQUIRE(rocket->generation == 5); REQUIRE(GetSpaceVehicleModel(*rocket) == 14); REQUIRE(rocket->manifest);
	REQUIRE(rocket->manifest->pods.empty()); REQUIRE(miner.manifest->pods.empty());
	// The admitted rocket retains the original paid constructor18. Its body/payload must return to actual Surface storage before the same carrier can serve a Craft resupply destination.
	const auto passenger = rocket->manifest->passenger; REQUIRE(passenger == SpaceCraftRef{VehicleID{18}, uint64_t{4}});
	REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, passenger.vehicle, passenger.generation, true));
	REQUIRE(ResolveSpaceCraft(passenger)); CHECK(ResolveSpaceCraft(passenger)->construction.placement == SpacePlacement::SurfaceStored);
	const auto missions = rocket->transition ? rocket->transition->missions_completed : 0; const auto burned = rocket->manifest->fuel_burned; const auto fuel_cargo = GetSpaceSupplyCargo(SpaceSupply::Propellant);
	const auto stock = center->goods[fuel_cargo].GetOrCreateData().cargo.TotalCount(); std::vector<SpacePodRef> selected;
	// Quote admission selects only actual paid CenterStored shells; iteration cannot borrow another craft's shell or create a replacement receipt.
	for (auto *pod : SpacePod::Iterate()) {
		if (selected.size() == 2) break;
		if (CmdMoveLaunchPod({}, home.station, home.generation, rocket->index, rocket->generation, pod->index, pod->generation, false).Failed()) continue;
		selected.push_back(GetSpacePodReference(*pod)); REQUIRE(Command<Commands::MoveLaunchPod>::Post(home.station, home.generation, rocket->index, rocket->generation, pod->index, pod->generation, false));
	}
	REQUIRE(selected.size() == 2); const auto fuel = static_cast<uint16_t>(GetMultiLayerSpaceBudget().outbound_fuel + GetMultiLayerSpaceBudget().recovery_fuel); REQUIRE(fuel == 15);
	// This exact miner already spent its first paid service parts. Only original Surface packet transfers and actual flight may replace both inputs.
	for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, supply, uint16_t{1}, false));
	REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, uint8_t{0}, fuel, false));
	REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, uint32_t{miner.index.base()}, miner.generation));
	// Only complete native delivery and recovery settle this mission. The original receiver remains orbital and receives no new return quota from freight transport.
	for (uint elapsed = 0; rocket->transition->missions_completed == missions; ++elapsed) { REQUIRE(elapsed < 3000); RepeatOpportunity(); }
	CHECK(rocket->transition->missions_completed == missions + 1); CHECK(rocket->construction.placement == SpacePlacement::SurfaceStored); CHECK(rocket->manifest->fuel_burned == burned + fuel);
	CHECK(center->goods[fuel_cargo].GetOrCreateData().cargo.TotalCount() == stock - fuel); CHECK(rocket->manifest->pods.empty()); CHECK(miner.manifest->pods == selected); CHECK(miner.manifest->returns_remaining == 0);
	CHECK(miner.manifest->supplies[1].TotalCount() == 1); CHECK(miner.manifest->supplies[2].TotalCount() == 1);
}

/** Earn paid service work from the actual derived current-state predicate, retaining exact per-opportunity upkeep.
 * @param miner Original paid small probe with an admitted onboard countdown and actual two service parts.
 * @param count Number of native service opportunities, never an installed timer delta.
 */
static void WorkRepeatService(SpaceVehicle &miner, uint count)
{
	const auto profit = miner.profit_this_year;
	// Each real active opportunity charges the model's500/day with its original74-tick remainder; completion consumes the two actual parts once.
	for (uint elapsed = 0; elapsed < count; ++elapsed) {
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); REQUIRE(IsSpaceServiceWorkTick(miner)); const auto before = miner.profit_this_year; const uint accrual = miner.mission.cost_remainder + 500;
		RepeatOpportunity(); CHECK(miner.profit_this_year == before - Money{accrual / 74} * 256); CHECK(miner.mission.cost_remainder == accrual % 74);
	}
	CHECK(miner.profit_this_year == profit - Money{count * 500 / 74} * 256);
}

/** Original paid mining must transfer two actual holds, exhaust its budget and earn service before another rendezvous. */
TEST_CASE("Original paid probe repeats only after actual descent shell resupply and paid onboard service", "[.probe-repeat-native]")
{
	const char *source = std::getenv("OTTD_PROBE_REPEAT_SOURCE"), *selected = std::getenv("OTTD_PROBE_REPEAT_PHASE"); REQUIRE(source); REQUIRE(selected);
	const std::string_view phase{selected}; const bool resave = std::getenv("OTTD_PROBE_REPEAT_RESAVE") != nullptr;
	const auto paths = GetTestCaseArtifactPaths("space", "probe-repeat"); const auto runtime = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Observe the exact accepted paid world and retain assertion failures until orderly application shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles()); auto *miner = SpaceVehicle::GetIfValid(VehicleID{16}); REQUIRE(miner); REQUIRE(miner->generation == 2); REQUIRE(GetSpaceVehicleModel(*miner) == 15); REQUIRE(miner->manifest);
			auto &manifest = *miner->manifest; const auto fleet = GetSpaceVehicleCount(); const auto generation = GetNextSpaceVehicleGeneration(); const auto before = ComputeSpaceVehicleDigest();
			REQUIRE(CaptureSpaceStructures().empty()); REQUIRE(miner->construction.placement == SpacePlacement::Orbital);
			if (!resave) {
				if (phase == "first-pending") {
					REQUIRE(manifest.returns_remaining == 2); REQUIRE(manifest.pods.size() == 2); ReleaseRepeatHold(*miner);
					// Save genuine interrupted descent after37 opportunities; allowance remains reserved until actual recovery.
					for (uint elapsed = 0; elapsed < 37; ++elapsed) RepeatOpportunity();
					CHECK(manifest.returns_remaining == 2); CHECK(manifest.pending_returns == 1);
				// Both recovery boundaries use the same native job owner; the second additionally exhausts the last reserved allowance rather than earning free service.
				} else if (phase == "first-recovered" || phase == "exhausted") {
					RecoverRepeatHold(*miner);
				} else if (phase == "second-pending") {
					REQUIRE(manifest.returns_remaining == 1); REQUIRE(manifest.pods.size() == 1); REQUIRE(Command<Commands::SetSpaceMission>::Post(miner->index, uint32_t{0}));
					// The original mission must mine finite reserve, leave its real lease and physically reenter its own circle; no pose or work is installed.
					for (uint elapsed = 0; miner->mission.phase != SpaceMissionPhase::Holding; ++elapsed) { REQUIRE(elapsed < 4000); RepeatOpportunity(); }
					CHECK(miner->extracted_units == 40); CHECK(GetMutableAsteroidBody(OrbitalBodyID{0})->remaining_reserve == 1720); ReleaseRepeatHold(*miner);
					CHECK(CmdSetSpaceMission({}, miner->index, uint32_t{0}).Failed());
				} else if (phase == "resupplied") {
					REQUIRE(manifest.returns_remaining == 0); CHECK(CmdSetSpaceMission({}, miner->index, uint32_t{0}).Failed()); ResupplyRepeatPods(*miner);
				} else if (phase == "service-partial") {
					REQUIRE(manifest.returns_remaining == 0); REQUIRE(manifest.pods.size() == 2); REQUIRE(manifest.supplies[1].TotalCount() == 1); REQUIRE(manifest.supplies[2].TotalCount() == 1);
					const auto quote = CmdServiceLaunchProbe({}, miner->index, miner->generation, false); REQUIRE(quote.Succeeded()); CHECK(quote.GetCost() == 0);
					REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(miner->index, miner->generation, false)); REQUIRE(manifest.service_ticks == 148);
					CHECK(CmdServiceLaunchProbe({}, miner->index, miner->generation, false).Failed()); WorkRepeatService(*miner, 74);
					CHECK(manifest.service_ticks == 74); CHECK(manifest.returns_remaining == 0); CHECK(manifest.supplies[1].TotalCount() == 1); CHECK(manifest.supplies[2].TotalCount() == 1); CHECK(CmdSetSpaceMission({}, miner->index, uint32_t{0}).Failed());
				} else if (phase == "service-complete") {
					REQUIRE(manifest.service_ticks == 74); const auto completed = manifest.services_completed; WorkRepeatService(*miner, 74);
					CHECK(manifest.service_ticks == 0); CHECK(manifest.services_completed == completed + 1); CHECK(manifest.returns_remaining == 2); CHECK(manifest.supplies[1].TotalCount() == 0); CHECK(manifest.supplies[2].TotalCount() == 0);
					CHECK(CmdServiceLaunchProbe({}, miner->index, miner->generation, false).Failed());
				} else {
					REQUIRE(phase == "third-assigned"); REQUIRE(manifest.returns_remaining == 2); REQUIRE(manifest.pods.size() == 2); REQUIRE(manifest.services_completed == 2);
					REQUIRE(Command<Commands::SetSpaceMission>::Post(miner->index, uint32_t{0})); CHECK(miner->mission.phase == SpaceMissionPhase::Approach); CHECK(miner->extracted_units == 40); CHECK(miner->cargo.TotalCount() == 0);
				}
			}
			if (resave) CHECK(ComputeSpaceVehicleDigest() == before);
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(miner->construction.placement == SpacePlacement::Orbital); REQUIRE(ValidateSpaceVehicles());
			{ AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"}); REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("repeat-{}-438.sav", phase)), false) == SaveLoadResult::Ok); }
			fmt::print("PROBE_REPEAT native=true phase={} resave={} added_stock=false quota={} pending={} service_ticks={} services={} pods={} cargo={} extracted={}\n", phase, resave, manifest.returns_remaining, manifest.pending_returns, manifest.service_ticks, manifest.services_completed, manifest.pods.size(), miner->cargo.TotalCount(), miner->extracted_units); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup observer(repeat_observer, &observe); IConsole::CmdRegister("probe_repeat_test", ObserveProbeRepeat); const std::string config = runtime.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
