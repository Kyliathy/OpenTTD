/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_freight_failure.cpp Bounded actual paid-world module contention, target retirement and cancelled recovery.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11C.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_approach.h"
#include "../orbital_freight.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../station_base.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../asteroid.h"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *failure_observer = nullptr; ///< Process-local callback, never another fleet/flight owner.

/** @param args Native console invocation without parameters. @return Whether the installed paid-world observer ran. */
static bool ObserveFreightFailure(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(failure_observer != nullptr); (*failure_observer)(); return true;
}

/** Real commands preserve the original loaded hull through occupied berths, exact target retirement and explicit cancelled recovery. */
TEST_CASE("Module flights retain genuine packets through contention and retired targets", "[.orbital-module-failure]")
{
	const char *input = std::getenv("OTTD_MODULE_FAILURE_SOURCE"), *mode = std::getenv("OTTD_MODULE_FAILURE_ACTION");
	REQUIRE(input != nullptr); REQUIRE(mode != nullptr); const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "module-failure"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Run only inside the actual loaded world; assertion failure survives orderly game shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			SpaceVehicle *rocket = nullptr; std::vector<SpaceVehicle *> builders;
			// Select only the original paid native fleet; no failure scenario creates substitute hulls or private target identities.
			for (auto *craft : SpaceVehicle::Iterate()) {
				if (GetSpaceVehicleModel(*craft) == 14) { REQUIRE(rocket == nullptr); rocket = craft; }
				if (GetSpaceVehicleModel(*craft) == 20) builders.push_back(craft);
			}
			REQUIRE(rocket != nullptr); REQUIRE(builders.size() == 2); REQUIRE(rocket->manifest != nullptr);
			auto &core = *SpaceStructure::Get(SpaceStructureID{0}), &staging = *SpaceStructure::Get(SpaceStructureID{2});
			const auto reference = GetSpaceCraftReference(*rocket);
			const auto center = rocket->construction.home;
			const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}); const auto count = GetSpaceVehicleCount();
			/** @param ticks Real synchronized controller opportunities, bounded independently of render callbacks. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Rendering/yields cannot advance the saved flight; count only actual native clock opportunities.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
				}
			};
			/** @param name Actual native checkpoint basename in this invocation's declared evidence home. */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			/** @param hull Actual paid builder. @param site Original native site whose precise approach is requested through synchronized edits. */
			auto visit = [&](SpaceVehicle &hull, const SpaceStructure &site) {
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(hull.index, hull.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(hull.index, hull.generation, SpaceOrderEdit::Insert, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Structure, uint32_t{site.index.base()}, site.generation));
			};
			/** @param hull Actual paid builder. @param site Original owned geometry. Wait for native motion/admission without injecting a position or berth. */
			auto arrive = [&](const SpaceVehicle &hull, const SpaceStructure &site) {
				// A bounded real arrival frees the old physical footprint, rather than treating an edited queue as actual departure.
				for (uint elapsed = 0; !GetSpaceStructureBerth(hull, site); ++elapsed) { REQUIRE(elapsed < 2000); advance(1); }
			};
			if (action == "pad-prepare") {
				REQUIRE(rocket->transition != nullptr); REQUIRE(rocket->transition->phase == SpaceFlightPhase::Preparing);
				const auto before_count = GetSpaceVehicleCount();
				REQUIRE(Command<Commands::AssembleLaunchCraft>::Post(center.station, center.generation, uint16_t{14}));
				SpaceVehicle *second = nullptr;
				// Resolve the new actually commissioned hull after its material/capital debit; no failure test fabricates another rocket or pad owner.
				for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 14 && craft != rocket) { REQUIRE(second == nullptr); second = craft; }
				REQUIRE(second != nullptr); advance(GetSpaceConstructionDefinition(SpaceBuildKind::Rocket)->work_ticks);
				REQUIRE(second->construction.placement == SpacePlacement::SurfaceStored); REQUIRE(IsSpaceFlightActive(*rocket));
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center.station, center.generation, second->index, second->generation, uint8_t{0}, uint16_t{15}, false));
				REQUIRE(Command<Commands::MoveLaunchFreight>::Post(center.station, center.generation, second->index, second->generation, steel, uint16_t{1}, false));
				const auto state = ComputeSpaceVehicleDigest(), manifests = ComputeSpaceManifestDigest(); const auto cash = Company::Get(_current_company)->money;
				CHECK(CmdBeginLaunchFlight({}, second->index, second->generation, SpaceOrderTargetKind::Structure, core.index.base(), core.generation).Failed());
				CHECK(CmdBeginLaunchFlight({DoCommandFlag::Execute}, second->index, second->generation, SpaceOrderTargetKind::Structure, core.index.base(), core.generation).Failed());
				// A31-unit requested hold cannot fit this real30-unit rocket; refusal preserves the one loaded unit and actual remaining Surface stock.
				CHECK(second->cargo_cap == 30); CHECK(CmdMoveLaunchFreight({}, center.station, center.generation, second->index, second->generation, steel, 30, false).Failed());
				CHECK(CmdMoveLaunchFreight({DoCommandFlag::Execute}, center.station, center.generation, second->index, second->generation, steel, 30, false).Failed());
				CHECK(ComputeSpaceVehicleDigest() == state); CHECK(ComputeSpaceManifestDigest() == manifests); CHECK(Company::Get(_current_company)->money == cash);
				CHECK(ResolveLaunchCenter(center)->launch_operations->pad_active == reference); CHECK(GetSpaceVehicleCount() == before_count + 1);
				CHECK(second->cargo.TotalCount() == 1); CHECK(second->manifest->supplies[0].TotalCount() == 15);
				save("module-flight-pad-blocked-423.sav"); fmt::print("MODULE_FAILURE native=true paid_second_rocket=true pad_contention=true hold_refused=true\n");
			// Contention owns a real shared berth wait; resolving it requires the original builder's physical departure rather than a queue-only shortcut.
			} else if (action == "contention-prepare" || action == "contention-continue") {
				if (action == "contention-prepare") {
					REQUIRE(rocket->transition != nullptr); REQUIRE(rocket->transition->phase == SpaceFlightPhase::Preparing);
					visit(*builders[0], core); visit(*builders[1], staging); arrive(*builders[0], core); arrive(*builders[1], staging);
					// Both real builder journeys fit inside this already paid departure; no second fee or private preparation timer is introduced.
					REQUIRE(rocket->transition->phase == SpaceFlightPhase::Preparing); advance(rocket->transition->remaining_ticks);
					REQUIRE(rocket->transition->phase == SpaceFlightPhase::Ascent); advance(rocket->transition->remaining_ticks); advance(2);
					REQUIRE(rocket->transition->phase == SpaceFlightPhase::OrbitalDelivery); REQUIRE(rocket->space_orders.approach);
					CHECK(rocket->space_orders.approach->slot == UINT8_MAX); CHECK(rocket->transition->blocked == SpaceFlightBlock::ReceiverBusy);
					save("module-flight-contention-423.sav");
					fmt::print("MODULE_FAILURE native=true actual_builder_contention=true payload_retained=true\n"); observed = true; _exit_game = true; return;
				}
				REQUIRE(rocket->transition->phase == SpaceFlightPhase::OrbitalDelivery); REQUIRE(rocket->space_orders.approach);
				const auto priority = rocket->space_orders.approach->request_tick; const auto stock = core.module->stock[steel].TotalCount();
				advance(37); CHECK(rocket->cargo.TotalCount() == 2); CHECK(rocket->manifest->supplies[1].TotalCount() == 1); CHECK(rocket->manifest->supplies[2].TotalCount() == 1);
				CHECK(core.module->stock[steel].TotalCount() == stock); CHECK(rocket->space_orders.approach->request_tick == priority); CHECK(rocket->space_orders.approach->slot == UINT8_MAX);
				visit(*builders[0], *SpaceStructure::Get(SpaceStructureID{1})); advance(1);
				// Reservation release alone grants no physical berth; the builder's actual departure must precede the original waiting rocket's admission.
				CHECK(rocket->transition->phase == SpaceFlightPhase::OrbitalDelivery); CHECK(rocket->cargo.TotalCount() == 2);
				bool admitted = false;
				for (uint elapsed = 0; rocket->transition->phase == SpaceFlightPhase::OrbitalDelivery; ++elapsed) {
					REQUIRE(elapsed < 4000); advance(1);
					if (rocket->space_orders.approach) { CHECK(rocket->space_orders.approach->request_tick == priority); if (rocket->space_orders.approach->slot == 0) admitted = true; }
				}
				CHECK(admitted); REQUIRE(rocket->transition->module_delivered); CHECK(core.module->stock[steel].TotalCount() == stock + 2);
				advance(rocket->transition->remaining_ticks); REQUIRE(rocket->transition->phase == SpaceFlightPhase::Turnaround); advance(rocket->transition->remaining_ticks);
				CHECK(rocket->transition->missions_completed == 7); CHECK(rocket->transition->missions_canceled == 0); CHECK(GetSpaceCraftReference(*rocket) == reference); CHECK(GetSpaceVehicleCount() == count);
				save("module-flight-contention-complete-423.sav"); fmt::print("MODULE_FAILURE native=true actual_departure=true original_priority=true whole_delivery=true\n");
			// Only explicit loss-preparation actions retire a destination; continuation consumes the already saved lost history instead of constructing another target.
			} else if (action.starts_with("lost-") && action != "lost-continue") {
				// Each phase starts from the same immutable genuine418 paid frame; material/fuel packing and safe removal are actual synchronized commands.
				REQUIRE(rocket->construction.placement == SpacePlacement::SurfaceStored); auto &storage = *SpaceStructure::Get(SpaceStructureID{1});
				REQUIRE(Command<Commands::ActivateOrbitalModule>::Post(storage.index, storage.generation)); advance(GetMultiLayerSpaceBudget().core_activation_ticks);
				visit(*builders[0], core); visit(*builders[1], staging); arrive(*builders[0], core); arrive(*builders[1], staging);
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center.station, center.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
				REQUIRE(Command<Commands::MoveLaunchFreight>::Post(center.station, center.generation, rocket->index, rocket->generation, steel, uint16_t{2}, false));
				// Service parts remain separate real packet owners; neither a test nor removal may fabricate another set or unpack a pod into this shipment.
				for (uint8_t supply : {1, 2}) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center.station, center.generation, rocket->index, rocket->generation, supply, uint16_t{1}, false));
				const auto destination = GetSpaceStructureReference(storage); const auto state = storage.state; const auto retired = GetSpaceStructureRetirementHistory();
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Structure, uint32_t{storage.index.base()}, storage.generation));
				if (action != "lost-preparing") advance(rocket->transition->remaining_ticks);
				// Ascent retains Surface custody; later delivery/approach cases enter Space through the same remaining native leg.
				if (action == "lost-delivery" || action == "lost-approach") advance(rocket->transition->remaining_ticks);
				if (action == "lost-approach") { advance(2); REQUIRE(rocket->space_orders.approach); }
				const auto manifest = ComputeSpaceManifestDigest(); const auto cargo = rocket->cargo.TotalCount();
				REQUIRE(Command<Commands::RemoveOrbitalModule>::Post(storage.index, storage.generation));
				CHECK(ResolveSpaceStructure(destination) == nullptr); CHECK(rocket->transition->blocked == SpaceFlightBlock::TargetLost); CHECK_FALSE(rocket->space_orders.approach);
				CHECK(ComputeSpaceManifestDigest() == manifest); CHECK(rocket->cargo.TotalCount() == cargo); CHECK(GetSpaceStructureRetirementHistory().structures_removed == retired.structures_removed + 1);
				// Reuse is a new paid blueprint with a higher issued generation; it cannot inherit the removed module's stock or this old flight's destination.
				REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(TileXYL(state.x, state.y, MapLayer::Space), state.kind));
				auto &replacement = *SpaceStructure::Get(destination.structure); CHECK(replacement.generation > destination.generation); CHECK(replacement.module == nullptr);
				CHECK(rocket->transition->destination.generation == destination.generation); CHECK(GetOrbitalModuleFreightBlock(*rocket, destination, false) == SpaceFlightBlock::TargetLost);
				save("module-flight-lost-423.sav"); fmt::print("MODULE_FAILURE native=true target_removed=true incarnation_reused=true payload_retained=true\n");
			} else {
				REQUIRE(action == "lost-continue"); REQUIRE(rocket->transition != nullptr); REQUIRE(rocket->transition->blocked == SpaceFlightBlock::TargetLost);
				const auto expected_fuel = rocket->transition->outbound_burned ? uint64_t{105} : uint64_t{90}; const auto expected_onboard = rocket->transition->outbound_burned ? uint{0} : uint{15};
				if (rocket->transition->phase == SpaceFlightPhase::Ascent) {
					// A burning ascent cannot be undone through preparation cancellation; it finishes its existing leg before explicit cancelled recovery in Space.
					CHECK(CmdCancelLaunchFlight({}, rocket->index, rocket->generation).Failed()); advance(rocket->transition->remaining_ticks);
				}
				CHECK(rocket->cargo.TotalCount() == 2); CHECK(rocket->manifest->supplies[1].TotalCount() == 1); CHECK(rocket->manifest->supplies[2].TotalCount() == 1);
				const auto history = GetSpaceStructureRetirementHistory(); const auto consumed = core.job.consumed; const auto before = ComputeSpaceTransitionDigest();
				CHECK(CmdCancelLaunchFlight({}, rocket->index, rocket->generation).Succeeded()); CHECK(ComputeSpaceTransitionDigest() == before);
				REQUIRE(Command<Commands::CancelLaunchFlight>::Post(rocket->index, rocket->generation));
				if (IsSpaceFlightActive(*rocket)) {
					REQUIRE(rocket->transition->phase == SpaceFlightPhase::ReturnRecovery); advance(rocket->transition->remaining_ticks);
					REQUIRE(rocket->transition->phase == SpaceFlightPhase::Turnaround); advance(rocket->transition->remaining_ticks);
				}
				CHECK_FALSE(IsSpaceFlightActive(*rocket)); CHECK(rocket->construction.placement == SpacePlacement::SurfaceStored);
				CHECK(rocket->cargo.TotalCount() == 2); CHECK(rocket->manifest->supplies[1].TotalCount() == 1); CHECK(rocket->manifest->supplies[2].TotalCount() == 1);
				CHECK(rocket->manifest->fuel_burned == expected_fuel); CHECK(rocket->manifest->supplies[0].TotalCount() == expected_onboard);
				CHECK(rocket->transition->missions_started == 7); CHECK(rocket->transition->missions_completed == 6); CHECK(rocket->transition->missions_canceled == 1);
				// Recovery advances real cargo age/utility clocks, while the paid replacement stays an inert blueprint and retired material never returns to stock.
				CHECK(GetSpaceStructureRetirementHistory() == history); CHECK(core.job.consumed == consumed);
				CHECK(SpaceStructure::Get(SpaceStructureID{1})->module == nullptr); CHECK(GetSpaceVehicleCount() == count); CHECK(GetSpaceCraftReference(*rocket) == reference);
				CHECK(CmdCancelLaunchFlight({}, rocket->index, rocket->generation).Failed()); save("module-flight-lost-complete-423.sav");
				fmt::print("MODULE_FAILURE native=true cancelled_recovery=true original_payload=true no_second_burn=true\n");
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(failure_observer, &observe); IConsole::CmdRegister("module_failure_test", ObserveFreightFailure);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
