/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_freight_native.cpp Genuine paid rocket/module delivery and bounded interrupted recovery.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Public Handoff Contract.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../orbital_freight.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_structure.h"
#include "../space_approach.h"
#include "../space_vehicle_cmd.h"
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
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *freight_observer = nullptr; ///< Process-local observer, never another flight/controller owner.

/** @param args Native console command without parameters. @return Whether this loaded-world observer ran. */
static bool ObserveModuleFreight(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(freight_observer != nullptr); (*freight_observer)(); return true;
}

/** Deliver only original Earth packets to a genuine paid Core through workshop controls, preserving all emitted partial phases. */
TEST_CASE("One original paid rocket delivers whole freight to a native module and recovers", "[.orbital-module-flight]")
{
	const char *input = std::getenv("OTTD_MODULE_FLIGHT_SOURCE"), *mode = std::getenv("OTTD_MODULE_FLIGHT_ACTION");
	REQUIRE(input != nullptr); REQUIRE(mode != nullptr); const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "module-flight"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe only the real loaded game; retain assertion failures across orderly native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			// Independently accept the same valid file before exercising its installed world: copied native cargo references and module-flight requests must decode without borrowing live cargo or berth owners.
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			REQUIRE(ValidateSpaceVehicles());
			auto &core = *SpaceStructure::Get(SpaceStructureID{0}); REQUIRE(IsOrbitalModuleOperational(core));
			SpaceVehicle *rocket = nullptr;
			// Resolve the original paid rocket, retaining every other native hull and module from the immutable predecessor.
			for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == 14) { REQUIRE(rocket == nullptr); rocket = craft; }
			REQUIRE(rocket != nullptr); REQUIRE(rocket->manifest != nullptr);
			/** @param row Same-file copied native hull. @return Whether it is this original paid sender. */
			const auto copied = std::ranges::find_if(_load_check_data.native_space_vehicle_rows, [&](const auto &row) { return row.vehicle == rocket->index; });
			REQUIRE(copied != _load_check_data.native_space_vehicle_rows.end());
			CHECK(copied->cargo_packet_refs.size() == rocket->cargo.Packets()->size());
			const auto reference = GetSpaceCraftReference(*rocket);
			const auto target = GetSpaceStructureReference(core);
			const auto center = rocket->construction.home; const auto count = GetSpaceVehicleCount();
			const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), metal = GetSpaceSupplyCargo(SpaceSupply::RefinedMetal), electronics = GetSpaceSupplyCargo(SpaceSupply::Electronics);
			const auto construction = core.job.consumed; const auto retired = GetSpaceStructureRetirementHistory();
			if (const char *corrupt = std::getenv("OTTD_MODULE_FLIGHT_CORRUPT")) {
				const auto state = ComputeSpaceVehicleDigest(), power = ComputeUtilityPowerDigest(), orders = ComputeSpaceOrdersDigest();
				const auto money = Company::Get(_current_company)->money; const auto random = _random;
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceVehicleDigest() == state); CHECK(ComputeSpaceOrdersDigest() == orders); CHECK(ComputeUtilityPowerDigest() == power);
				CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("MODULE_FLIGHT native=true preview_rejected=true authority_unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			/** @param ticks Actual synchronized native opportunities; rendering/yields create no progress or energy. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Count only the authoritative simulation clock, bounding a stalled native controller independently of render callbacks.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
				}
			};
			/** @param name Checkpoint basename inside this invocation's isolated evidence home. */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			if (action == "prepare") {
				REQUIRE(rocket->construction.placement == SpacePlacement::SurfaceStored); REQUIRE(rocket->manifest->fuel_burned == 90);
				REQUIRE(rocket->manifest->supplies[0].TotalCount() == 0); REQUIRE(rocket->cargo.TotalCount() == 0);
				// Idle the two actual builders by synchronized edits; they keep their paid physical positions and never gain replacement berths or hulls.
				for (auto *craft : SpaceVehicle::Iterate()) {
					if (GetSpaceVehicleModel(*craft) != 20) continue;
					REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft->index, craft->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				}
				// The retained seed has unloaded its unused propellant back to actual Surface stock; pack precisely the next paid round trip through its native packet command.
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center.station, center.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
				REQUIRE(Command<Commands::MoveLaunchFreight>::Post(center.station, center.generation, rocket->index, rocket->generation, steel, uint16_t{2}, false));
				// All three deliverable labels remain real Surface stock until explicit packing into this same original rocket.
				for (uint8_t supply : {1, 2}) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(center.station, center.generation, rocket->index, rocket->generation, supply, uint16_t{1}, false));
				const auto state = ComputeSpaceVehicleDigest(), structures = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest();
				CHECK(GetOrbitalModuleFreightBlock(*rocket, target, false) == SpaceFlightBlock::None);
				CHECK(GetOrbitalModuleFreightBlock(*rocket, target, true) == SpaceFlightBlock::ReceiverBusy);
				CHECK_FALSE(CommitOrbitalModuleFreight(*rocket, target));
				CHECK(CmdBeginLaunchFlight({}, rocket->index, rocket->generation, SpaceOrderTargetKind::Structure, core.index.base(), core.generation).Succeeded());
				CHECK(CmdBeginLaunchFlight({}, rocket->index, rocket->generation, SpaceOrderTargetKind::Structure, core.index.base(), core.generation + 100).Failed());
				CHECK(ComputeSpaceVehicleDigest() == state); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == utility);
				ShowLaunchAssemblyWindow(center); auto *workshop = FindWindowById(WindowClass::LaunchAssembly, center.station.base()); REQUIRE(workshop != nullptr);
				CaptureNativeArtWindow(*workshop, paths.images / "module-workshop-unselected.ppm");
				const auto panel = workshop->GetWidget<NWidgetBase>(0)->GetCurrentRect(); const int line = GetCharacterHeight(FontSize::Normal) + 3;
				workshop->OnClick({panel.left + 8, panel.top + 8 + 6 * line}, 0, 1); // First real row follows the five recipes and status header.
				workshop->OnClick({}, 30, 1); CaptureNativeArtWindow(*workshop, paths.images / "module-workshop-ready.ppm");
				REQUIRE_FALSE(workshop->IsWidgetDisabled(31)); const auto money = Company::Get(_current_company)->money;
				workshop->OnClick({}, 31, 1); REQUIRE(IsSpaceFlightActive(*rocket));
				CHECK(Company::Get(_current_company)->money == money - Money{GetMultiLayerSpaceBudget().mission_fee});
				CHECK((rocket->transition->destination == SpaceOrderTarget{SpaceOrderTargetKind::Structure, core.index.base(), core.generation}));
				save("module-flight-preburn-423.sav"); advance(GetMultiLayerSpaceBudget().prepare_ticks);
				REQUIRE(rocket->transition->phase == SpaceFlightPhase::Ascent); CHECK(rocket->manifest->fuel_burned == 100); save("module-flight-ascent-423.sav");
				advance(GetMultiLayerSpaceBudget().flight_ticks); REQUIRE(rocket->transition->phase == SpaceFlightPhase::OrbitalDelivery);
				CHECK(rocket->position == rocket->transition->departure); CHECK(rocket->position != GetSpaceStructureApproach(core.state));
				save("module-flight-delivery-423.sav");
				fmt::print("MODULE_FLIGHT native=true workshop_launch=true real_space_entry=true departure_fee_once=true\n");
				observed = true; _exit_game = true; return;
			}
			REQUIRE(action == "continue"); REQUIRE(IsSpaceFlightActive(*rocket)); const auto &budget = GetMultiLayerSpaceBudget();
			// Advance only saved remaining phases. Continuations from preburn/ascent/delivery and post-unload snapshots converge through the same controller.
			if (rocket->transition->phase == SpaceFlightPhase::Preparing) advance(rocket->transition->remaining_ticks);
			if (rocket->transition->phase == SpaceFlightPhase::Ascent) advance(rocket->transition->remaining_ticks);
			if (rocket->transition->phase == SpaceFlightPhase::OrbitalDelivery) {
				REQUIRE(rocket->cargo.TotalCount() == 2); REQUIRE(rocket->manifest->supplies[1].TotalCount() == 1); REQUIRE(rocket->manifest->supplies[2].TotalCount() == 1);
				const auto stock = std::array{core.module->stock[steel].TotalCount(), core.module->stock[metal].TotalCount(), core.module->stock[electronics].TotalCount()};
				const auto source_tile = rocket->cargo.Packets()->front()->GetSourceTile();
				/** @param stock Actual physical stock. @return Whole feeder-share total, including packets that native append may legitimately merge. */
				auto shares = [](const StationCargoList &stock) {
					Money total = 0;
					// Existing routing buckets own all paid provenance; possible native merges preserve their aggregate feeder share.
					for (const auto &[hop, packets] : *stock.Packets()) { (void)hop; for (const auto *packet : packets) total += packet->GetFeederShare(); }
					return total;
				};
				Money incoming_share = 0;
				// Record every original freight packet before travel; unloading may merge compatible fragments while preserving their complete accounting.
				for (const auto *packet : *rocket->cargo.Packets()) incoming_share += packet->GetFeederShare();
				const auto expected_share = shares(core.module->stock[steel]) + incoming_share;
				bool checkpoint = false;
				// A shared admitted request must first move the real hull toward its precise berth; the flight cannot appear at the destination after ascent.
				for (uint elapsed = 0; rocket->transition->phase == SpaceFlightPhase::OrbitalDelivery; ++elapsed) {
					REQUIRE(elapsed < 4000); advance(1);
					if (!checkpoint && rocket->space_orders.approach && rocket->transition->phase == SpaceFlightPhase::OrbitalDelivery) { save("module-flight-approach-423.sav"); checkpoint = true; }
				}
				REQUIRE(rocket->transition->module_delivered); CHECK_FALSE(rocket->space_orders.approach); CHECK(rocket->position == GetSpaceStructureApproach(core.state));
				CHECK(core.module->stock[steel].TotalCount() == stock[0] + 2); CHECK(core.module->stock[metal].TotalCount() == stock[1] + 1); CHECK(core.module->stock[electronics].TotalCount() == stock[2] + 1);
				CHECK(shares(core.module->stock[steel]) == expected_share);
				bool source_retained = false;
				// Source provenance survives either original fragments or valid native merges; packet identity alone is not a promise of the container API.
				for (const auto &[hop, packets] : *core.module->stock[steel].Packets()) { (void)hop; for (const auto *packet : packets) if (packet->GetSourceTile() == source_tile) source_retained = true; }
				CHECK(source_retained);
				save("module-flight-unloaded-423.sav");
			}
			REQUIRE(rocket->transition->module_delivered); CHECK_FALSE(rocket->transition->resupplied); CHECK_FALSE(rocket->transition->deployed);
			CHECK(rocket->manifest->fuel_burned == 105); CHECK(rocket->manifest->supplies[0].TotalCount() == 0);
			CHECK(rocket->cargo.TotalCount() == 0); CHECK(rocket->manifest->supplies[1].TotalCount() == 0); CHECK(rocket->manifest->supplies[2].TotalCount() == 0);
			const auto structures = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
			CHECK_FALSE(CommitOrbitalModuleFreight(*rocket, target)); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(Company::Get(_current_company)->money == cash);
			if (rocket->transition->phase == SpaceFlightPhase::ReturnRecovery) {
				advance(rocket->transition->remaining_ticks); REQUIRE(rocket->transition->phase == SpaceFlightPhase::Turnaround);
				save("module-flight-turnaround-423.sav");
			}
			REQUIRE(rocket->transition->remaining_ticks <= budget.turnaround_ticks); advance(rocket->transition->remaining_ticks);
			CHECK_FALSE(IsSpaceFlightActive(*rocket)); CHECK(rocket->construction.placement == SpacePlacement::SurfaceStored);
			CHECK(GetSpaceCraftReference(*rocket) == reference); CHECK(GetSpaceVehicleCount() == count); CHECK(core.job.consumed == construction); CHECK(GetSpaceStructureRetirementHistory() == retired);
			CHECK(rocket->transition->missions_started == 7); CHECK(rocket->transition->missions_completed == 7); CHECK(rocket->transition->missions_canceled == 0);
			CHECK(ResolveLaunchCenter(center)->launch_operations->pad_active == SpaceCraftRef{});
			save("module-flight-complete-423.sav"); fmt::print("MODULE_FLIGHT native=true whole_delivery=true original_rocket=true once_only=true no_second_recipe=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(freight_observer, &observe); IConsole::CmdRegister("module_flight_test", ObserveModuleFreight);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
