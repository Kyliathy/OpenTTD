/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_failure_phase_native.cpp Genuine fuel/cancellation and two-rocket pad command ordering without constructed cargo or private pad changes.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group15.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../multilayer_space_content.h"
#include "../station_base.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
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

static std::function<void()> *phase_failure_observer = nullptr; ///< Observer only; actual commands retain the paid hull, pad and packets.

/** @param args Parameter-free loaded-world callback. @return Whether the installed observer ran. */
static bool ObserveLaunchPhaseFailure(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(phase_failure_observer != nullptr); (*phase_failure_observer)(); return true;
}

/** Verify one bounded real preburn cancellation/fuel refusal or deterministic two-paid-rocket pad exchange. */
TEST_CASE("Actual launch fuel and ordered pad failures preserve paid payload and charge each accepted mission once", "[.launch-phase-failure]")
{
	const char *input = std::getenv("OTTD_LAUNCH_PHASE_SOURCE"), *phase_env = std::getenv("OTTD_LAUNCH_PHASE_ACTION"); REQUIRE(input != nullptr); REQUIRE(phase_env != nullptr);
	const std::string phase{phase_env}; const auto paths = GetTestCaseArtifactPaths("space", "launch-phase-failure"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Use only an explicit genuine423 paid preburn/pad predecessor and propagate assertions through native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			SpaceVehicle *original = nullptr, *second = nullptr;
			// Resolve actual paid original/second rockets; source preparation has exactly one active preburn flight, without any fixture-owned pad identity.
			for (auto *craft : SpaceVehicle::Iterate()) {
				if (GetSpaceVehicleModel(*craft) != 14) continue;
				if (IsSpaceFlightActive(*craft)) { REQUIRE(original == nullptr); original = craft; }
				else { REQUIRE(second == nullptr); second = craft; }
			}
			REQUIRE(original != nullptr); REQUIRE(original->transition->phase == SpaceFlightPhase::Preparing); REQUIRE(original->manifest != nullptr);
			const auto target = original->transition->destination; const auto home = original->construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center != nullptr);
			/** @param craft Actual original or second paid rocket whose otherwise valid admission must refuse without changing cargo/fuel/money. */
			auto refuse = [&](SpaceVehicle &craft) {
				const auto digest = ComputeSpaceVehicleDigest(); const auto cash = Company::Get(_current_company)->money;
				CHECK(CmdBeginLaunchFlight({}, craft.index, craft.generation, target.kind, target.id, target.generation).Failed());
				CHECK(CmdBeginLaunchFlight({DoCommandFlag::Execute}, craft.index, craft.generation, target.kind, target.id, target.generation).Failed());
				CHECK(ComputeSpaceVehicleDigest() == digest); CHECK(Company::Get(_current_company)->money == cash);
			};
			/** @param craft Actual preburn rocket retaining all paid payload. */
			auto cancel = [&](SpaceVehicle &craft) {
				const auto cash = Company::Get(_current_company)->money; const auto freight = craft.cargo.TotalCount(); const auto fuel = craft.manifest->supplies[0].TotalCount();
				const auto burned = craft.manifest->fuel_burned, aborted = craft.transition->missions_canceled;
				const auto metal = craft.manifest->supplies[1].TotalCount(), electronics = craft.manifest->supplies[2].TotalCount();
				REQUIRE(Command<Commands::CancelLaunchFlight>::Post(craft.index, craft.generation));
				CHECK(craft.transition->phase == SpaceFlightPhase::GroundLoading); CHECK(craft.transition->missions_canceled == aborted + 1); CHECK(craft.cargo.TotalCount() == freight);
				CHECK(craft.manifest->supplies[0].TotalCount() == fuel); CHECK(craft.manifest->fuel_burned == burned); CHECK(Company::Get(_current_company)->money == cash);
				CHECK(craft.manifest->supplies[1].TotalCount() == metal); CHECK(craft.manifest->supplies[2].TotalCount() == electronics);
				CHECK(CmdCancelLaunchFlight({DoCommandFlag::Execute}, craft.index, craft.generation).Failed()); CHECK(Company::Get(_current_company)->money == cash);
			};
			/** @param craft Actual ready pad occupant whose next single mission is explicitly authorized. */
			auto begin = [&](SpaceVehicle &craft) {
				const auto cash = Company::Get(_current_company)->money;
				// A genuinely newly commissioned second rocket has no flight owner yet; its first synchronized launch creates that owner with zero prior missions.
				const auto started = craft.transition ? craft.transition->missions_started : uint64_t{0};
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(craft.index, craft.generation, target.kind, target.id, target.generation));
				CHECK(craft.transition->missions_started == started + 1); CHECK(Company::Get(_current_company)->money == cash - Money{GetMultiLayerSpaceBudget().mission_fee});
				CHECK(craft.transition->phase == SpaceFlightPhase::Preparing); refuse(craft);
			};
			if (phase == "fuel-cancel") {
				REQUIRE(second == nullptr); cancel(*original); REQUIRE(original->manifest->supplies[0].TotalCount() == 15);
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, original->index, original->generation, uint8_t{to_underlying(SpaceSupply::Propellant)}, uint16_t{1}, true));
				CHECK(original->manifest->supplies[0].TotalCount() == 14); refuse(*original); // Only round-trip fuel is unavailable; the real pad, payload and target are otherwise ready.
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, original->index, original->generation, uint8_t{to_underlying(SpaceSupply::Propellant)}, uint16_t{1}, false));
				begin(*original); cancel(*original); CHECK(original->manifest->supplies[0].TotalCount() == 15);
			} else {
				REQUIRE(phase == "pad-order"); REQUIRE(second == nullptr); cancel(*original);
				REQUIRE(Command<Commands::AssembleLaunchCraft>::Post(home.station, home.generation, uint16_t{14})); second = ResolveSpaceCraft(center->launch_operations->workshop); REQUIRE(second != nullptr);
				{
					AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
					// Commission the second actual material/capital-paid hull while the original remains GroundLoading; no fake queue slot or backward phase mutation is required.
					while (second->construction.placement == SpacePlacement::Assembling) {
						REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
						if (GetAsteroidField().elapsed_ticks == before) std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
				}
				REQUIRE(second->construction.placement == SpacePlacement::SurfaceStored);
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, second->index, second->generation, uint8_t{to_underlying(SpaceSupply::Propellant)}, uint16_t{15}, false));
				REQUIRE(Command<Commands::MoveLaunchFreight>::Post(home.station, home.generation, second->index, second->generation, original->cargo_type, uint16_t{1}, false));
				begin(*original); refuse(*second); cancel(*original);
				// Real pad queue order admits the waiting second hull after the original cancels; competing commands then reject the other hull without charging it.
				begin(*second); refuse(*original); cancel(*second); begin(*original); refuse(*second); cancel(*original);
			}
			REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / ("launch-failure-" + phase + "-423.sav")), false) == SaveLoadResult::Ok);
			fmt::print("LAUNCH_FAILURE native=true phase={} actual_paid_hulls=true fuel_preserved=true fee_once=true ordered_commands=true\n", phase);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(phase_failure_observer, &observe); IConsole::CmdRegister("launch_phase_test", ObserveLaunchPhaseFailure);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
