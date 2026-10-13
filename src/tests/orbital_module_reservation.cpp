/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_module_reservation.cpp Actual imported stock, exclusive module reservations and inert continuation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group2.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../launch_center_cmd.h"
#include "../asteroid.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
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

static std::function<void()> *reservation_observer = nullptr;
/** @param args Native console invocation. @return Whether the installed loaded-world observer ran. */
static bool ObserveReservation(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(reservation_observer != nullptr); (*reservation_observer)(); return true;
}

TEST_CASE("Paid adjacent modules reserve imported materials exclusively without activating", "[.orbital-module-reservation]")
{
	const char *mode = std::getenv("OTTD_ORBITAL_RESERVATION_ACTION"), *input = std::getenv("OTTD_ORBITAL_RESERVATION_SOURCE");
	REQUIRE(mode != nullptr); REQUIRE(input != nullptr); const std::string_view action{mode};
	const auto paths = GetTestCaseArtifactPaths("space", "module-reservation"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe actual paid native owners; propagate failures through normal shutdown rather than synthesizing stock. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			auto &core = *SpaceStructure::Get(SpaceStructureID{0}); auto &site = *SpaceStructure::Get(SpaceStructureID{1});
			REQUIRE(IsOrbitalModuleOperational(core)); REQUIRE(site.state.kind == SpaceBuildKind::Storage);
			REQUIRE(site.state.phase == SpaceStructurePhase::Blueprint); REQUIRE(site.module == nullptr); REQUIRE(site.job.work_ticks == 0);
			REQUIRE(GetOrbitalConstructionCore(site.state) == &core);
			const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
			auto *company = Company::Get(_current_company); SpaceVehicle *rocket = nullptr, *builder = nullptr;
			// Reuse paid deployed builders and the same recovered Earth rocket from the immutable service predecessor.
			for (auto *craft : SpaceVehicle::Iterate()) {
				if (GetSpaceVehicleModel(*craft) == 14) rocket = craft;
				// Only a real empty builder at the Core approach can receive this physical resupply flight.
				if (GetSpaceVehicleModel(*craft) == 20 && craft->position == GetSpaceStructureApproach(core.state) && craft->cargo.TotalCount() == 0) builder = craft;
			}
			REQUIRE(rocket != nullptr); REQUIRE(builder != nullptr);
			/** @param ticks Real synchronized economy opportunities; rendering and fast-forward yields earn no work. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Count the authoritative controller, keeping native execution bounded without advancing a separate work clock.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
				}
			};
			/** @param name Isolated native checkpoint name. */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			/** @param target Paid output identity. @return Whole material reservation estimate, without execution. */
			auto estimate = [&](const SpaceStructure &target) { return CmdReserveOrbitalModuleInputs({}, target.index, target.generation, core.index, core.generation); };
			/** @param cargo Actual construction label. @param quantity Real whole units. @param load Whether stock loads the builder. */
			auto exchange = [&](CargoType cargo, uint16_t quantity, bool load) {
				REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(core.index, core.generation, builder->index, builder->generation, cargo, quantity, load));
			};
			if (action == "prepare") {
				REQUIRE(core.module->stock[steel].TotalCount() == 0); REQUIRE(core.module->stock[goods].TotalCount() == 8);
				const auto initial = ComputeSpaceStructureDigest(); CHECK(estimate(site).Failed()); CHECK(ComputeSpaceStructureDigest() == initial);
				CHECK(core.module->stock[goods].TotalCount() == 8); CHECK(site.job.inputs[1].TotalCount() == 0);
				auto geometry = site.state; geometry.owner = OWNER_NONE; CHECK(GetOrbitalConstructionCore(geometry) == nullptr);
				geometry = site.state; geometry.x = 8; geometry.y = 7; CHECK(GetOrbitalConstructionCore(geometry) == nullptr);
				CHECK(CmdBuildSpaceBlueprint({}, TileXYL(4, 2, MapLayer::Space), SpaceBuildKind::Storage).Failed()); // Approach enters Core footprint.
				CHECK(CmdBuildSpaceBlueprint({}, TileXYL(8, 8, MapLayer::Space), SpaceBuildKind::Storage).Failed()); // No shared Core edge.
				CHECK(CmdBuildSpaceBlueprint({}, TileXYL(2, 4, MapLayer::Space), SpaceBuildKind::Rocket).Failed()); // Carried hull cannot reserve a module footprint.
				const auto home = rocket->construction.home; const auto missions = rocket->transition->missions_completed;
				REQUIRE(Command<Commands::MoveLaunchFreight>::Post(home.station, home.generation, rocket->index, rocket->generation, steel, uint16_t{10}, false));
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, uint32_t{builder->index.base()}, builder->generation));
				advance(667); REQUIRE(builder->cargo.TotalCount() == 10); CHECK(rocket->transition->missions_completed == missions + 1);
				exchange(steel, 10, false); exchange(goods, 8, true);
				const auto steel_only = ComputeSpaceStructureDigest(); CHECK(estimate(site).Failed()); CHECK(ComputeSpaceStructureDigest() == steel_only);
				CHECK(core.module->stock[steel].TotalCount() == 10); CHECK(site.job.inputs[0].TotalCount() == 0); exchange(goods, 8, false);
				const auto cash = company->money;
				REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(TileXYL(2, 4, MapLayer::Space), SpaceBuildKind::Storage));
				CHECK(company->money == cash - Money{120000}); REQUIRE(SpaceStructure::GetNumItems() == 3);
				CHECK(GetOrbitalModuleCargoCapacity(site, goods) == 0); CHECK_FALSE(IsOrbitalModuleOperational(site));
				const auto digest = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest(); const auto money = company->money;
				const auto random = _random; const auto profit = builder->profit_this_year;
				CHECK(estimate(site).Succeeded()); CHECK(CmdReserveOrbitalModuleInputs({}, site.index, site.generation + 1, core.index, core.generation).Failed());
				CHECK(CmdReserveOrbitalModuleInputs({}, site.index, site.generation, core.index, core.generation + 1).Failed());
				{ AutoRestoreBackup foreign(_current_company, COMPANY_SPECTATOR); CHECK(estimate(site).Failed()); }
				CHECK(ComputeSpaceStructureDigest() == digest); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(company->money == money);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				const auto before = CaptureSpaceStructures(); REQUIRE(before[0].module->stock[steel].size() == 1);
				const auto original_id = before[0].module->stock[steel][0].first; const auto *original = CargoPacket::Get(original_id);
				const auto age = original->GetPeriodsInTransit(); const auto share = original->GetFeederShare(); const auto source = original->GetSource();
				REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				ShowSpaceConstructionWindow(GetSpaceStructureReference(site)); auto *window = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(window != nullptr);
				CaptureNativeArtWindow(*window, paths.images / "module-reservation-ready.ppm"); CHECK_FALSE(window->IsWidgetDisabled(27));
				window->OnClick({}, 27, 1); REQUIRE(site.job.inputs[0].TotalCount() == 8); REQUIRE(site.job.inputs[1].TotalCount() == 8);
				CHECK(company->money == money); CHECK(builder->profit_this_year == profit); CHECK(ComputeUtilityPowerDigest() == utility);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				const auto after = CaptureSpaceStructures(); REQUIRE(after[1].inputs[0].size() == 1);
				const auto *reserved = CargoPacket::Get(after[1].inputs[0][0].first); CHECK(reserved->index != original_id);
				CHECK(CargoPacket::Get(original_id)->Count() == 2); CHECK(reserved->Count() == 8); CHECK(reserved->GetPeriodsInTransit() == age);
				CHECK(reserved->GetSource() == source);
				CHECK(CargoPacket::Get(original_id)->GetFeederShare() + reserved->GetFeederShare() == share);
				CHECK(after[1].inputs[1] == before[0].module->stock[goods]); CHECK(core.module->stock[steel].TotalCount() == 2); CHECK(core.module->stock[goods].TotalCount() == 0);
				CaptureNativeArtWindow(*window, paths.images / "module-reservation-held.ppm"); CHECK(window->IsWidgetDisabled(27)); save("module-reserved-418.sav");
			}
			REQUIRE(site.job.inputs[0].TotalCount() == 8); REQUIRE(site.job.inputs[1].TotalCount() == 8);
			auto &competitor = *SpaceStructure::Get(SpaceStructureID{2}); const auto held = ComputeSpaceStructureDigest();
			CHECK(estimate(site).Failed()); CHECK(estimate(competitor).Failed()); CHECK(ComputeSpaceStructureDigest() == held);
			CHECK(competitor.job.inputs[0].TotalCount() == 0); CHECK(competitor.job.inputs[1].TotalCount() == 0);
			auto copied = CaptureSpaceStructures(); copied[2].quantities = copied[1].quantities; copied[2].inputs = copied[1].inputs;
			CHECK_FALSE(ValidateSpaceStructureRecords(copied, GetNextSpaceStructureGeneration(), Map::SizeX(), Map::SizeY()));
			const auto source_used = GetUtilityPowerProducer(core.module->producer)->consumed_lifetime_j;
			advance(74); CHECK(site.job.work_ticks == 0); CHECK(site.module == nullptr); CHECK_FALSE(IsOrbitalModuleOperational(site));
			CHECK(GetOrbitalModuleCargoCapacity(site, goods) == 0); CHECK(GetUtilityPowerProducer(core.module->producer)->consumed_lifetime_j == source_used);
			CHECK((site.job.consumed == std::array<uint16_t, 2>{0, 0})); save("module-inert-418.sav");
			fmt::print("ORBITAL_RESERVATION native=true actual_paid_import=true missing_either_unchanged=true exclusive=true split_conserved=true inert=true no_second_payment=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(reservation_observer, &observe); IConsole::CmdRegister("orbital_reservation_test", ObserveReservation);
	const std::string source_path{input}, config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
