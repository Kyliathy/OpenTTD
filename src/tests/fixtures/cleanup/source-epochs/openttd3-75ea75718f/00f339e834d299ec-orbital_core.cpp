/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_core.cpp Paid Core activation, physical stock and exact native continuation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
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

/** @return A copied fully consumed Core frame, without creating a native facility or packet. */
static SpaceStructureRecord CoreFrame()
{
	SpaceStructureRecord row; row.ref = {SpaceStructureID{0}, 1}; row.state.owner = Owner{0};
	row.state.x = 4; row.state.y = 4; row.state.capital_paid = 700000;
	row.state.phase = SpaceStructurePhase::FrameComplete; row.work_ticks = 740; row.consumed = {16, 24};
	return row;
}

TEST_CASE("Core activation and operational stock cannot bypass the paid construction receipt", "[orbital-core-contract]")
{
	auto row = CoreFrame(); CHECK(ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64));
	row.module.emplace(); row.module->activation_ticks = 74; CHECK(ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64));
	row.module->activation_ticks = 75; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64));
	row.module->activation_ticks = 0; row.module->quantities[0] = 8; row.module->stock[0] = {{CargoPacketID{1}, StationID::Invalid()}};
	CHECK(ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64));
	row.module->activation_ticks = 1; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64));
	row.module->activation_ticks = 0; row.module->quantities[0] = 1025; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64));
	row.module->quantities[0] = 8; row.module->quantities[1] = 8; row.module->stock[1] = row.module->stock[0];
	CHECK_FALSE(ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64));
	row = CoreFrame(); row.module.emplace(); row.consumed[1] = 23; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64));
	row = CoreFrame(); row.module.emplace(); row.state.phase = SpaceStructurePhase::Blueprint; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64));
}

TEST_CASE("Static module recipes retain native footprint and cannot masquerade as paid hulls", "[orbital-core-contract]")
{
	// Exercise every actual static definition rather than repeating one Core geometry under a different displayed name.
	for (const auto &definition : GetSpaceConstructionDefinitions()) {
		if (definition.width == 0) continue;
		auto row = CoreFrame(); row.state.kind = definition.kind; row.state.capital_paid = definition.capital;
		row.work_ticks = definition.work_ticks; row.consumed = {static_cast<uint16_t>(definition.inputs[0].quantity), static_cast<uint16_t>(definition.inputs[1].quantity)};
		CHECK(ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64));
		row.module.emplace();
		// Newly activated Power modules require their actual panel reference; unsupported later functions remain inert frames.
		if (definition.kind == SpaceBuildKind::Power) row.module->producer = UtilityProducerID{0};
		CHECK(ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64) == IsSupportedOrbitalModuleKind(definition.kind));
	}
	auto row = CoreFrame(); row.state.kind = SpaceBuildKind::Rocket; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{row}, 2, 64, 64));
}

static std::function<void()> *core_observer = nullptr;
/** @param args Console invocation. @return Whether the one registered native observer ran. */
static bool ObserveCore(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(core_observer != nullptr); (*core_observer)(); return true;
}

TEST_CASE("An Earth-paid Core activates once and retains real delivered freight", "[.orbital-core]")
{
	const char *mode = std::getenv("OTTD_ORBITAL_CORE_ACTION"), *input = std::getenv("OTTD_ORBITAL_CORE_SOURCE");
	REQUIRE(mode != nullptr); REQUIRE(input != nullptr);
	const std::string_view action{mode}; const std::filesystem::path source{input};
	const auto paths = GetTestCaseArtifactPaths("space", "core"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Run inside the loaded native world; exceptions return through the normal game shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(ValidateSpaceVehicles());
			auto &site = *SpaceStructure::Get(SpaceStructureID{0}); REQUIRE(site.state.owner == _current_company);
			REQUIRE(site.state.phase == SpaceStructurePhase::FrameComplete); REQUIRE((site.job.consumed == std::array<uint16_t, 2>{16, 24}));
			SpaceVehicle *rocket = nullptr, *builder = nullptr;
			// Resolve actual paid native craft from the immutable two-delivery predecessor, never synthesize a construction inventory.
			for (auto *craft : SpaceVehicle::Iterate()) {
				if (GetSpaceVehicleModel(*craft) == 14) rocket = craft;
				if (GetSpaceVehicleModel(*craft) == 20) builder = craft;
			}
			REQUIRE(rocket != nullptr); REQUIRE(builder != nullptr); REQUIRE(builder->position == GetSpaceStructureApproach(site.state));
			const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}); auto *company = Company::Get(_current_company);
			/** @param ticks Synchronized native ticks, counted by the asteroid controller rather than wall time. */
			auto advance = [&](uint ticks) {
				AutoRestoreBackup pause(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(90);
				// Native fast-forward may yield without an economy tick; only real controller advancement counts toward activation/flight.
				for (uint done = 0; done < ticks;) {
					REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++done;
				}
			};
			/** @param name Native saved checkpoint beneath this isolated review run. */
			auto save = [&](std::string_view name) {
				REQUIRE(ValidateSpaceVehicles()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / std::string{name}), false) == SaveLoadResult::Ok);
			};
			if (const char *corrupt = std::getenv("OTTD_ORBITAL_CORE_CORRUPT")) {
				const auto digest = ComputeSpaceStructureDigest(); const auto money = company->money; const auto random = _random;
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeSpaceStructureDigest() == digest); CHECK(company->money == money);
				CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
				fmt::print("ORBITAL_CORE preview_rejected=true authority_unchanged=true\n"); observed = true; _exit_game = true; return;
			}
			if (action != "stored") {
				if (action == "prepare") {
					REQUIRE(site.module == nullptr); const auto digest = ComputeSpaceStructureDigest(); const auto money = company->money; const auto random = _random;
					CHECK(CmdActivateOrbitalCore({}, site.index, site.generation).Succeeded()); CHECK(CmdActivateOrbitalCore({}, site.index, site.generation + 1).Failed());
					CHECK(ComputeSpaceStructureDigest() == digest); CHECK(company->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
					CHECK(CmdBuildSpaceBlueprint({}, TileXYL(7, 4, MapLayer::Space), SpaceBuildKind::Storage).Failed());
					REQUIRE(Command<Commands::ActivateOrbitalCore>::Post(site.index, site.generation)); REQUIRE(site.module != nullptr);
					CHECK(GetUtilityPowerProducer(site.module->producer)->generated_lifetime_j == 0);
					CHECK_FALSE(IsOrbitalModuleOperational(site)); advance(37); CHECK(site.module->activation_ticks == 37); save("core-activating-418.sav");
					CHECK(GetUtilityPowerProducer(site.module->producer)->generated_lifetime_j == 0);
				}
				REQUIRE(site.module != nullptr); REQUIRE(site.module->activation_ticks == 37);
				CHECK(CmdActivateOrbitalCore({}, site.index, site.generation).Failed());
				CHECK(CmdMoveOrbitalFreight({}, site.index, site.generation, builder->index, builder->generation, goods, 1, true).Failed());
				advance(36); CHECK_FALSE(IsOrbitalModuleOperational(site)); advance(1); CHECK(IsOrbitalModuleOperational(site));
				CHECK(GetUtilityPowerProducer(site.module->producer)->generated_lifetime_j == 0);
				CHECK(GetOrbitalModuleCargoCapacity(site, goods) == 1024); CHECK(site.module->stock[goods].TotalCount() == 0);
				CHECK(CmdBuildSpaceBlueprint({}, TileXYL(7, 4, MapLayer::Space), SpaceBuildKind::Storage).Succeeded());
				CHECK(CmdBuildSpaceBlueprint({}, TileXYL(8, 8, MapLayer::Space), SpaceBuildKind::Storage).Failed());
				CHECK(CmdBuildSpaceBlueprint({}, TileXYL(7, 4, MapLayer::Space), SpaceBuildKind::Rocket).Failed());
				REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				ShowSpaceConstructionWindow(); auto *window = FindWindowById(WindowClass::SpaceConstruction, 0); REQUIRE(window != nullptr);
				window->OnDropdownSelect(11, to_underlying(SpaceBuildKind::Storage), 0);
				const auto cash = company->money; window->OnPlaceObject({}, TileXYL(7, 4, MapLayer::Space));
				REQUIRE(SpaceStructure::GetNumItems() == 2); const auto &extension = *SpaceStructure::Get(SpaceStructureID{1});
				CHECK(extension.state.kind == SpaceBuildKind::Storage); CHECK(extension.state.phase == SpaceStructurePhase::Blueprint);
				CHECK(extension.job.work_ticks == 0); CHECK(extension.module == nullptr); CHECK(company->money == cash - Money{120000});
				CaptureNativeArtWindow(*window, paths.images / "core-selector-ready.ppm");
				save("core-ready-418.sav");
				const auto home = rocket->construction.home;
				REQUIRE(Command<Commands::MoveLaunchFreight>::Post(home.station, home.generation, rocket->index, rocket->generation, goods, uint16_t{8}, false));
				REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
				REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, uint32_t{builder->index.base()}, builder->generation));
				advance(667); REQUIRE(builder->cargo.TotalCount() == 8); CHECK(rocket->transition->missions_completed == 3);
				const auto profit = builder->profit_this_year; const auto money = company->money;
				REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(site.index, site.generation, builder->index, builder->generation, goods, uint16_t{8}, false));
				CHECK(builder->profit_this_year == profit); CHECK(company->money == money); CHECK(builder->cargo.TotalCount() == 0); CHECK(site.module->stock[goods].TotalCount() == 8);
				save("core-stored-418.sav");
			}
			REQUIRE(IsOrbitalModuleOperational(site)); REQUIRE(site.module->stock[goods].TotalCount() == 8);
			const auto before = CaptureSpaceStructures()[0].module->stock; const auto digest = ComputeSpaceStructureDigest(); const auto money = company->money;
			CHECK(CmdMoveOrbitalFreight({}, site.index, site.generation, builder->index, builder->generation, goods, 9, true).Failed());
			CHECK(CmdMoveOrbitalFreight({}, site.index, site.generation + 1, builder->index, builder->generation, goods, 8, true).Failed());
			CHECK(ComputeSpaceStructureDigest() == digest); CHECK(company->money == money);
			REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(site.index, site.generation, builder->index, builder->generation, goods, uint16_t{8}, true));
			CHECK(site.module->stock[goods].TotalCount() == 0); CHECK(builder->cargo.TotalCount() == 8);
			REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(site.index, site.generation, builder->index, builder->generation, goods, uint16_t{8}, false));
			CHECK(CaptureSpaceStructures()[0].module->stock == before); CHECK((site.job.consumed == std::array<uint16_t, 2>{16, 24}));
			save("core-complete-418.sav"); fmt::print("ORBITAL_CORE native=true activated_once=true actual_paid_resupply=true stored_goods=8 no_delivery_payment=true\n");
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(core_observer, &observe); IConsole::CmdRegister("orbital_core_test", ObserveCore);
	const std::string source_path = source.string(), config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
