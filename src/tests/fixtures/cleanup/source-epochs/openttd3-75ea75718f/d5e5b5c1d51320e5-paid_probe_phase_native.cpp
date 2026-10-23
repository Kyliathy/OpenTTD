/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file paid_probe_phase_native.cpp Actual paid carried-probe launch phase pairs and expense witnesses.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group17.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_transition.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../multilayer_space_content.h"
#include "../asteroid.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include <filesystem>
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *paid_phase_observer = nullptr; ///< Read/advance observer; the original Vehicle and its transition retain all authority.

/** @param args Parameter-free loaded-world invocation. @return Whether the installed observer ran. */
static bool ObservePaidProbePhase(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(paid_phase_observer != nullptr); (*paid_phase_observer)(); return true;
}

/** Advance exactly one original paid flight phase; capture canonical owner state before and immediately after its legal transition. */
TEST_CASE("Paid carried probe phase pairs retain original identities fuel and expense receipts", "[.paid-probe-phase]")
{
	const char *input = std::getenv("OTTD_PAID_PROBE_PHASE_SOURCE"), *stage = std::getenv("OTTD_PAID_PROBE_PHASE_STAGE");
	REQUIRE(input != nullptr); REQUIRE(stage != nullptr); const std::string phase{stage};
	const auto paths = GetTestCaseArtifactPaths("space", "paid-probe-phase"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Observe the actual loaded paid flight and propagate assertion failures after normal native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			SpaceVehicle *rocket = nullptr, *probe = nullptr;
			// The immutable launch predecessor has one paid rocket and one paid miner; pool IDs are discovered through their actual model roles.
			for (auto *craft : SpaceVehicle::Iterate()) {
				if (GetSpaceVehicleModel(*craft) == 14) { REQUIRE(rocket == nullptr); rocket = craft; }
				if (GetSpaceVehicleModel(*craft) == 15) { REQUIRE(probe == nullptr); probe = craft; }
			}
			REQUIRE(rocket != nullptr); REQUIRE(probe != nullptr); REQUIRE(rocket->transition != nullptr); REQUIRE(rocket->manifest != nullptr); REQUIRE(probe->manifest != nullptr);
			// The original carried probe retains its historical inactive Holding state; neither it nor Idle starts extraction or service while proving launch-only expenses.
			REQUIRE((probe->mission.phase == SpaceMissionPhase::Idle || probe->mission.phase == SpaceMissionPhase::Holding)); REQUIRE(probe->space_orders.entries.empty());
			const auto rocket_ref = GetSpaceCraftReference(*rocket), probe_ref = GetSpaceCraftReference(*probe); const auto home = rocket->construction.home;
			REQUIRE(ResolveLaunchCenter(home) != nullptr); const auto shells = probe->manifest->pods;
			const auto rocket_cargo = rocket->cargo.TotalCount(), probe_cargo = probe->cargo.TotalCount();
			const auto total_fuel = rocket->manifest->supplies[0].TotalCount() + rocket->manifest->fuel_burned;
			const auto started = rocket->transition->missions_started, canceled = rocket->transition->missions_canceled; const auto fleet_count = GetSpaceVehicleCount();
			const std::array<std::pair<std::string_view, SpaceFlightPhase>, 6> stages{{
				{"preparing", SpaceFlightPhase::Preparing}, {"ascent", SpaceFlightPhase::Ascent}, {"delivery", SpaceFlightPhase::OrbitalDelivery},
				{"return", SpaceFlightPhase::ReturnRecovery}, {"turnaround", SpaceFlightPhase::Turnaround}, {"complete", SpaceFlightPhase::GroundLoading}}};
			/** @param entry Declared canonical phase. @return Whether it is the explicit requested boundary. */
			const auto selected = std::find_if(stages.begin(), stages.end(), [&](const auto &entry) { return entry.first == phase; });
			REQUIRE(selected != stages.end()); REQUIRE(rocket->transition->phase == selected->second);
			const auto next_phase = selected + 1 == stages.end() ? selected->second : (selected + 1)->second;
			/** @param edge Before or after the single phase transition; every number comes from an actual canonical owner. */
			auto capture = [&](std::string_view edge) {
				REQUIRE(ValidateSpaceVehicles());
				const auto digest = ComputeSpaceVehicleDigest(); const auto money = Company::Get(rocket->owner)->money;
				CHECK(CmdBeginLaunchFlight({}, rocket->index, rocket->generation, SpaceOrderTargetKind::Body, 0, 0).Failed());
				const bool cancelable = rocket->transition->phase == SpaceFlightPhase::Preparing || rocket->transition->phase == SpaceFlightPhase::OrbitalDelivery;
				CHECK(CmdCancelLaunchFlight({}, rocket->index, rocket->generation).Succeeded() == cancelable);
				CHECK(ComputeSpaceVehicleDigest() == digest); CHECK(Company::Get(rocket->owner)->money == money);
				AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("probe-flight-{}-{}-423.sav", phase, edge)), false) == SaveLoadResult::Ok);
				fmt::print("PAID_PROBE_LEDGER edge={} phase={} rocket={} rocket_generation={} probe={} probe_generation={} money={} rocket_profit={} probe_profit={} fuel={} burned={} started={} completed={} canceled={} cargo={} pods={} pad_remainder={} probe_remainder={}\n",
					edge, to_underlying(rocket->transition->phase), rocket_ref.vehicle.base(), rocket_ref.generation, probe_ref.vehicle.base(), probe_ref.generation,
					static_cast<int64_t>(money), static_cast<int64_t>(rocket->profit_this_year), static_cast<int64_t>(probe->profit_this_year), rocket->manifest->supplies[0].TotalCount(), rocket->manifest->fuel_burned,
					rocket->transition->missions_started, rocket->transition->missions_completed, rocket->transition->missions_canceled, probe->cargo.TotalCount(), probe->manifest->pods.size(), rocket->transition->pad_cost_remainder, probe->mission.cost_remainder);
			};
			capture("before"); const auto initial_phase = rocket->transition->phase; uint ticks = 0;
			AutoRestoreBackup running(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90); const auto &budget = GetMultiLayerSpaceBudget();
			// Actual opportunities advance only the requested native phase. Vehicle upkeep runs before the existing flight controller, so a newly deployed probe starts idle expense on the next opportunity.
			while (initial_phase == SpaceFlightPhase::GroundLoading ? ticks < 3 : rocket->transition->phase == initial_phase) {
				REQUIRE(std::chrono::steady_clock::now() < deadline); REQUIRE(ticks < 1000); const auto elapsed = GetAsteroidField().elapsed_ticks;
				const auto rocket_profit = rocket->profit_this_year, probe_profit = probe->profit_this_year;
				const bool pad = rocket->transition->phase == SpaceFlightPhase::Preparing || rocket->transition->phase == SpaceFlightPhase::Turnaround;
				const uint pad_accrual = rocket->transition->pad_cost_remainder + (pad ? budget.pad_per_day : 0);
				const bool orbital = probe->construction.placement == SpacePlacement::Orbital; const uint probe_accrual = probe->mission.cost_remainder + (orbital ? budget.idle_per_day : 0);
				StateGameLoop();
				if (GetAsteroidField().elapsed_ticks == elapsed) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
				REQUIRE(GetAsteroidField().elapsed_ticks == elapsed + 1); ++ticks;
				CHECK(rocket->profit_this_year == rocket_profit - Money{pad_accrual / 74} * 256); CHECK(rocket->transition->pad_cost_remainder == pad_accrual % 74);
				CHECK(probe->profit_this_year == probe_profit - Money{probe_accrual / 74} * 256); CHECK(probe->mission.cost_remainder == probe_accrual % 74);
				CHECK(rocket->manifest->fuel_burned + rocket->manifest->supplies[0].TotalCount() == total_fuel);
			}
			CHECK(rocket->transition->phase == next_phase); CHECK(GetSpaceCraftReference(*rocket) == rocket_ref); CHECK(GetSpaceCraftReference(*probe) == probe_ref); CHECK(rocket->construction.home == home);
			CHECK(GetSpaceVehicleCount() == fleet_count); CHECK(rocket->cargo.TotalCount() == rocket_cargo); CHECK(probe->cargo.TotalCount() == probe_cargo); CHECK(probe->manifest->pods == shells);
			CHECK(rocket->transition->missions_started == started); CHECK(rocket->transition->missions_canceled == canceled);
			// Preparing/ascent preserve the original reciprocal passenger binding. Delivery clears it exactly once and keeps the same paid probe orbital throughout recovery and later idle opportunities.
			if (rocket->transition->phase == SpaceFlightPhase::Ascent || rocket->transition->phase == SpaceFlightPhase::OrbitalDelivery) {
				CHECK(rocket->manifest->passenger == probe_ref); CHECK(probe->manifest->carrier == rocket_ref); CHECK(probe->construction.placement == SpacePlacement::Carried); CHECK_FALSE(probe->construction.deployed);
			} else {
				CHECK(rocket->manifest->passenger == SpaceCraftRef{}); CHECK(probe->manifest->carrier == SpaceCraftRef{}); CHECK(probe->construction.placement == SpacePlacement::Orbital); CHECK(probe->construction.deployed);
			}
			capture("after"); fmt::print("PAID_PROBE_PHASE native=true phase={} ticks={} no_new_fee=true no_cargo_income=true\n", phase, ticks);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(paid_phase_observer, &observe); IConsole::CmdRegister("paid_probe_phase_test", ObservePaidProbePhase);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
