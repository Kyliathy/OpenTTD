/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_dock_native.cpp Delivered Dock construction and two actual berths in bounded native phases.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4C.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlsp-space-master-plan.md prerequisite queue.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_approach.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../launch_center_cmd.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../asteroid.h"
#include "../map_func.h"
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

static std::function<void()> *dock_observer = nullptr;
/** @param args Native console invocation. @return Whether the currently installed loaded-world observer ran. */
static bool ObserveDock(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(dock_observer != nullptr); (*dock_observer)(); return true;
}

/** Paid imported packets, actual native queues and shared utility settlement are the only gameplay owners used by this observer. */
TEST_CASE("Delivered Dock uses two real berths and the original shared source without free stock", "[.orbital-dock]")
{
	const char *mode = std::getenv("OTTD_ORBITAL_DOCK_ACTION"), *input = std::getenv("OTTD_ORBITAL_DOCK_SOURCE");
	REQUIRE(mode != nullptr); REQUIRE(input != nullptr); const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "orbital-dock"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Inspect and drive only the ordinary loaded-world command/tick owners, propagating failures through native shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			auto &core = *SpaceStructure::Get(SpaceStructureID{0}); REQUIRE(IsOrbitalModuleOperational(core));
			SpaceStructure *dock = nullptr; SpaceVehicle *rocket = nullptr; std::vector<SpaceVehicle *> builders;
			// Select genuine paid owners from the loaded catalogue; no test hull, position, packet or receipt is injected.
			for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Dock) dock = site;
			for (auto *craft : SpaceVehicle::Iterate()) {
				if (GetSpaceVehicleModel(*craft) == 14) rocket = craft;
				if (GetSpaceVehicleModel(*craft) == 20 && craft->construction.placement == SpacePlacement::Orbital) builders.push_back(craft);
			}
			REQUIRE(rocket != nullptr); REQUIRE(builders.size() == 2);
			/** @param ticks Actual synchronized opportunities; rendering/yields do not advance a separate clock. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Count only the authoritative native controller, keeping each phase bounded even when the UI yields without simulation.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
				}
			};
			/** @param name Native checkpoint basename in this invocation's evidence directory. */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			/** @param craft Actual paid hull. @param site Exact native site. @param order Actual Visit or Construct command to execute. */
			auto order = [&](SpaceVehicle &craft, const SpaceStructure &site, SpaceOrderAction order) {
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				const auto target_kind = order == SpaceOrderAction::Construct ? SpaceOrderTargetKind::Blueprint : SpaceOrderTargetKind::Structure;
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, order, target_kind, uint32_t{site.index.base()}, site.generation));
			};
			/** @param receiver Existing deployed builder. @param cargo Real Surface freight label. @param quantity Whole freight units. @param parts Import two actual service pairs on this flight. */
			auto import_freight = [&](SpaceVehicle &receiver, CargoType cargo, uint16_t quantity, bool parts) {
				const auto home = rocket->construction.home; const auto missions = rocket->transition->missions_completed;
				// A waiting construction action cannot also promise its moving receiver to a flight; explicitly hold the real hull during resupply.
				REQUIRE(Command<Commands::EditSpaceOrder>::Post(receiver.index, receiver.generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
				REQUIRE(Command<Commands::MoveLaunchFreight>::Post(home.station, home.generation, rocket->index, rocket->generation, cargo, quantity, false));
				if (parts) {
					// Service stock is separately delivered physical cargo; the builder's embodied assembly parts cannot supply either job.
					for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, supply, uint16_t{2}, false));
				}
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, uint32_t{receiver.index.base()}, receiver.generation));
				// Await the same rocket's actual recovery, rather than teleporting its receiver or assuming a fixed flight duration.
				for (uint elapsed = 0; rocket->transition->missions_completed != missions + 1; ++elapsed) { REQUIRE(elapsed < 3000); advance(1); }
			};
			if (const char *corrupt = std::getenv("OTTD_ORBITAL_DOCK_CORRUPT")) {
				const auto structures = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest(), queues = ComputeSpaceOrdersDigest();
				const auto cash = Company::Get(_current_company)->money; const auto random = _random;
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeSpaceOrdersDigest() == queues);
				CHECK(Company::Get(_current_company)->money == cash); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("ORBITAL_DOCK native=true preview_rejected=true authority_unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			if (action == "steel") {
				REQUIRE(dock == nullptr); SpaceVehicle *receiver = nullptr;
				// The frame predecessor finishes its builders at Storage; choose an actual empty paid receiver and travel to Core explicitly.
				for (auto *craft : builders) if (craft->cargo.TotalCount() == 0) { receiver = craft; break; }
				REQUIRE(receiver != nullptr); REQUIRE(receiver->cargo.TotalCount() == 0);
				order(*receiver, core, SpaceOrderAction::Visit);
				for (uint elapsed = 0; !GetSpaceStructureBerth(*receiver, core); ++elapsed) { REQUIRE(elapsed < 2000); advance(1); }
				const auto tile = TileXYL(core.state.x + 1, core.state.y + 3, MapLayer::Space);
				REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(tile, SpaceBuildKind::Dock));
				for (auto *site : SpaceStructure::Iterate()) if (site->state.kind == SpaceBuildKind::Dock) dock = site;
				REQUIRE(dock != nullptr); import_freight(*receiver, GetCargoTypeByLabel(CargoLabel{"STEL"}), 8, true);
				order(*receiver, *dock, SpaceOrderAction::Construct);
				// Material leaves the real freight hold only after this builder acquires and reaches the native construction approach.
				for (uint elapsed = 0; dock->job.inputs[0].TotalCount() != 8; ++elapsed) { REQUIRE(elapsed < 2000); advance(1); }
				CHECK(dock->job.inputs[1].TotalCount() == 0); CHECK(dock->job.work_ticks == 0); CHECK_FALSE(IsOrbitalModuleOperational(*dock));
				save("dock-steel-422.sav");
			} else if (action == "goods") {
				REQUIRE(dock != nullptr); REQUIRE(dock->job.inputs[0].TotalCount() == 8); SpaceVehicle *receiver = nullptr;
				for (auto *craft : builders) if (GetSpaceStructureBerth(*craft, *dock)) receiver = craft;
				REQUIRE(receiver != nullptr); import_freight(*receiver, GetCargoTypeByLabel(CargoLabel{"GOOD"}), 12, false);
				order(*receiver, *dock, SpaceOrderAction::Construct);
				// Complete the existing delivered job through actual shared-power work; importing goods never activates the frame by itself.
				for (uint elapsed = 0; dock->state.phase != SpaceStructurePhase::FrameComplete; ++elapsed) { REQUIRE(elapsed < 2000); advance(1); }
				CHECK((dock->job.consumed == std::array<uint16_t, 2>{8, 12})); REQUIRE(dock->module == nullptr);
				REQUIRE(Command<Commands::ActivateOrbitalModule>::Post(dock->index, dock->generation)); advance(74);
				REQUIRE(IsOrbitalModuleOperational(*dock)); CHECK(dock->module->producer == UtilityProducerID{UINT32_MAX}); CHECK(GetSpaceStructureApproachCapacity(*dock) == 2);
				REQUIRE(Command<Commands::ConnectOrbitalServices>::Post(dock->index, dock->generation, core.index, core.generation));
				save("dock-ready-422.sav");
			} else {
				REQUIRE(dock != nullptr); REQUIRE(IsOrbitalModuleOperational(*dock)); REQUIRE(GetOrbitalServicePowerSite(*dock) == &core);
				if (action == "service") {
					SpaceVehicle *receiver = nullptr;
					for (auto *craft : builders) if (GetSpaceStructureBerth(*craft, *dock)) receiver = craft;
					REQUIRE(receiver != nullptr);
					// Unload only the two imported actual service pairs; each future job reserves its own original packets exclusively.
					for (uint8_t supply : {uint8_t{1}, uint8_t{2}}) REQUIRE(Command<Commands::MoveOrbitalSupply>::Post(dock->index, dock->generation, receiver->index, receiver->generation, supply, uint16_t{2}, false));
					for (auto *craft : builders) order(*craft, *dock, SpaceOrderAction::Visit);
					// Both hulls must physically arrive at distinct saved slots before either service can reserve parts.
					for (uint elapsed = 0; !GetSpaceStructureBerth(*builders[0], *dock) || !GetSpaceStructureBerth(*builders[1], *dock); ++elapsed) { REQUIRE(elapsed < 2000); advance(1); }
					for (auto *craft : builders) REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(dock->index, dock->generation, craft->index, craft->generation, OrbitalServiceControl::Start));
				}
				auto &first = dock->module->service; auto &second = dock->module->dock_service;
				/** @param job Actual contained job. @param control Explicit command on that original hull incarnation. */
				auto control = [&](const OrbitalServiceJob &job, OrbitalServiceControl control) { REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(dock->index, dock->generation, job.craft.vehicle, job.craft.generation, control)); };
				/** @return Current original source's settled lifetime joules. */
				auto used = [&] { return GetUtilityPowerProducer(core.module->producer)->consumed_lifetime_j; };
				if (action == "cancel") {
					const auto spent = used(); control(first, OrbitalServiceControl::Cancel); control(second, OrbitalServiceControl::Cancel);
					REQUIRE(Command<Commands::DisconnectOrbitalServices>::Post(dock->index, dock->generation));
					CHECK(used() == spent); CHECK(dock->module->stock[GetSpaceSupplyCargo(SpaceSupply::RefinedMetal)].TotalCount() == 2);
					CHECK(dock->module->stock[GetSpaceSupplyCargo(SpaceSupply::Electronics)].TotalCount() == 2); save("dock-cancelled-422.sav");
				} else {
					const auto first_ref = first.craft, second_ref = second.craft;
					const auto first_serial = first.services_at_start, second_serial = second.services_at_start;
					if (action == "service") {
						auto *first_hull = ResolveSpaceCraft(first_ref), *second_hull = ResolveSpaceCraft(second_ref);
						REQUIRE(first_hull != nullptr); REQUIRE(second_hull != nullptr);
						const auto first_profit = first_hull->profit_this_year, second_profit = second_hull->profit_this_year;
						const auto first_carry = first_hull->mission.cost_remainder, second_carry = second_hull->mission.cost_remainder;
						const auto before = used(); advance(37); CHECK(first.remaining_ticks == 111); CHECK(second.remaining_ticks == 148); CHECK(used() == before + 27750);
						CHECK(first_hull->profit_this_year == first_profit - Money{(first_carry + 37 * 500) / 74} * 256);
						CHECK(second_hull->profit_this_year == second_profit - Money{(second_carry + 37 * 100) / 74} * 256);
						CHECK(CmdDisconnectOrbitalServices({}, dock->index, dock->generation).Failed());
						CHECK(CmdEditSpaceOrder({}, first.craft.vehicle, first.craft.generation, SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Failed());
						control(first, OrbitalServiceControl::Pause); advance(37); CHECK(first.remaining_ticks == 111); CHECK(second.remaining_ticks == 111);
						control(first, OrbitalServiceControl::Resume); REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core.index, core.generation, true));
						const auto spent = used(); advance(37); CHECK(first.remaining_ticks == 111); CHECK(second.remaining_ticks == 111); CHECK(used() == spent);
						save("dock-work-422.sav");
					} else REQUIRE(action == "work");
					REQUIRE(first.remaining_ticks == 111); REQUIRE(second.remaining_ticks == 111);
					REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core.index, core.generation, false)); const auto spent = used();
					advance(111); CHECK(first.remaining_ticks == 0); CHECK(second.remaining_ticks == 111); CHECK(used() == spent + 83250);
					advance(111); CHECK(second.remaining_ticks == 0); CHECK(used() == spent + 166500); CHECK(dock->module->services_completed == 2);
					CHECK(ResolveSpaceCraft(first_ref)->manifest->services_completed == first_serial + 1); CHECK(ResolveSpaceCraft(first_ref)->manifest->returns_remaining == 2);
					CHECK(ResolveSpaceCraft(second_ref)->manifest->services_completed == second_serial + 1); CHECK(ResolveSpaceCraft(second_ref)->manifest->returns_remaining == 2);
					CHECK(dock->module->stock[GetSpaceSupplyCargo(SpaceSupply::RefinedMetal)].TotalCount() == 0);
					CHECK(dock->module->stock[GetSpaceSupplyCargo(SpaceSupply::Electronics)].TotalCount() == 0);
					REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
					ShowSpaceConstructionWindow(GetSpaceStructureReference(*dock)); auto *window = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(window != nullptr);
					CaptureNativeArtWindow(*window, paths.images / "dock-services.ppm"); save("dock-complete-422.sav");
				}
			}
			fmt::print("ORBITAL_DOCK native=true phase={} actual_paid_deliveries=true original_source=true\n", action);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(dock_observer, &observe); IConsole::CmdRegister("orbital_dock_test", ObserveDock);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
