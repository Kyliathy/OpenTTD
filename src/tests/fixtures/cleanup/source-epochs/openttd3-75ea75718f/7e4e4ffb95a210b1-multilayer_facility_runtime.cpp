/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_facility_runtime.cpp Actual road-service gallery on ground opened by the retained paid TBM loop.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md task85B.2
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.8
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../console_internal.h"
#include "../command_func.h"
#include "../road_cmd.h"
#include "../rail_cmd.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../tbm.h"
#include "../spoil_receiver.h"
#include "../multilayer_sprite.h"
#include "../core/random_func.hpp"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../transparency.h"
#include "native_art_render_fixture.h"

#include "../safeguards.h"

static std::function<void()> *_facility_art_observer = nullptr; ///< Isolated application observer, absent from production.

/**
 * Invoke the loaded native facility observer through the ordinary startup hook.
 * @param arguments Exact no-argument command; no gameplay input is accepted.
 * @return Whether the isolated callback ran successfully.
 */
static bool ObserveFacilityArt(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1);
	REQUIRE(_facility_art_observer != nullptr);
	(*_facility_art_observer)();
	return true;
}

/** A real depot build on already paid-open ground must render all four native entrances without any render-time gameplay change. */
TEST_CASE("M1 road service gallery uses actual commands on the completed paid excavation", "[.m1-facility-art]")
{
	const auto root = FindTestRepositoryRoot();
	const auto source = ResolveTestInputPath(root / "src/tests/fixtures/haulage/spoil-loop-underground/m1-loop-underground-5-complete-loop.sav");
	const auto config = GetTestRuntimeConfig();
	const auto reviews = GetTestArtifactPaths("facilities", "service-depots").images;
	REQUIRE(std::filesystem::exists(source));
	REQUIRE(std::filesystem::exists(config));
	bool observed = false;
	std::exception_ptr failure;
	/** Build through actual commands before freezing the scene; the renderer itself never opens or builds a tile. */
	std::function<void()> capture = [&] {
		try {
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			REQUIRE(Company::IsValidID(_local_company));
			AutoRestoreBackup actor(_current_company, _local_company);
			std::vector<TileIndex> floor;
			// Only retained jobs of this owner supply bounded candidate cells.
			// No full-map scan or synthetic excavation authorizes depot construction.
			for (const auto &project : CaptureTBMProjects()) {
				if (project.company != _local_company) continue;
				const auto *job = GetExcavationJob(project.job);
				REQUIRE(job != nullptr);
				floor.insert(floor.end(), job->authorized_tiles.begin(), job->authorized_tiles.end());
			}
			std::ranges::sort(floor);
			floor.erase(std::unique(floor.begin(), floor.end()), floor.end());
			std::array<TileIndex, 4> depots{};
			// Each view is an actual distinct constructed tile. Command estimates
			// reject support, occupied cells and unsuitable approaches before debit.
			for (uint8_t view = 0; view < 4; ++view) {
				depots[view] = INVALID_TILE;
				for (const TileIndex tile : floor) {
					// Solid terrain and existing structures cannot be consumed merely
					// to display a building; use only genuinely opened clear floor.
					if (GetTileLayer(tile) != MapLayer::Underground || !IsTileType(tile, TileType::Clear)) continue;
					const auto direction = static_cast<DiagDirection>(view);
					if (CmdBuildRoadDepot({}, tile, ROADTYPE_ROAD, direction).Failed()) continue;
					REQUIRE(Command<Commands::BuildRoadDepot>::Do(DoCommandFlag::Execute, tile, ROADTYPE_ROAD, direction).Succeeded());
					depots[view] = tile;
					break;
				}
				REQUIRE(depots[view] != INVALID_TILE);
				REQUIRE(GetMultiLayerRoadDepotSprite(depots[view], false, 0).has_value());
				REQUIRE(GetMultiLayerRoadDepotSprite(depots[view], true, 0).has_value());
			}
			TileIndex rail_depot = INVALID_TILE;
			// An ordinary unassigned rail-service tile is distinct from the TBM's
			// assigned exchange bay even when they reuse the same source design.
			for (const TileIndex tile : floor) {
				// Both service families obey the original paid excavation boundary;
				// building a gallery never consumes solid or existing support cells.
				if (GetTileLayer(tile) != MapLayer::Underground || !IsTileType(tile, TileType::Clear)) continue;
				if (CmdBuildTrainDepot({}, tile, RAILTYPE_RAIL, DiagDirection::SE).Failed()) continue;
				REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, DiagDirection::SE).Succeeded());
				rail_depot = tile;
				break;
			}
			REQUIRE(rail_depot != INVALID_TILE);
			CHECK(GetMultiLayerRailDepotSprite(rail_depot, false, 0) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityRailService, {1, 0, 1, 0, 0}));
			const auto projects = CaptureTBMProjects();
			const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
			const auto receivers = CaptureSpoilReceivers();
			const Money money = Company::Get(_local_company)->money;
			SavedRandomSeeds before;
			SaveRandomSeeds(&before);
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			// Both genuine viewport zooms use ordinary road-tile rendering and
			// the existing sorter; direction labels identify the constructed entry.
			for (uint8_t view = 0; view < 4; ++view) {
				for (const ZoomLevel zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
					CaptureNativeArtViewport(depots[view], zoom, reviews / fmt::format("m1-road-service-direction-{}-zoom-{}.ppm", view, to_underlying(zoom)));
				}
			}
			{
				TransparencyOptions options = _transparency_opt;
				options.Set(TransparencyOption::Buildings);
				AutoRestoreBackup transparency(_transparency_opt, options);
				AutoRestoreBackup invisibility(_invisibility_opt, TransparencyOptions{});
				for (const ZoomLevel zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
					CaptureNativeArtViewport(depots[1], zoom, reviews / fmt::format("m1-road-service-transparent-zoom-{}.ppm", to_underlying(zoom)));
				}
			}
			// The ordinary depot uses its real native consumer, independently of
			// any assignment or TBM service transition in the other retained loops.
			for (const ZoomLevel zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
				CaptureNativeArtViewport(rail_depot, zoom, reviews / fmt::format("m1-rail-service-ready-zoom-{}.ppm", to_underlying(zoom)));
			}
			REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			SavedRandomSeeds after;
			SaveRandomSeeds(&after);
			CHECK(std::ranges::equal(before.random.state, after.random.state));
			CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
			CHECK(CaptureTBMProjects() == projects);
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			CHECK(CaptureSpoilReceivers() == receivers);
			CHECK(Company::Get(_local_company)->money == money);
			fmt::print("M1_FACILITY_RUNTIME frames=12 actual_road_depot_commands=4 actual_unassigned_rail_depot=true previously_paid_floor=true render_owners_unchanged=true\n");
		} catch (...) {
			// Native shutdown releases language/font ownership before reporting
			// any observer failure, matching the other isolated raster drivers.
			failure = std::current_exception();
		}
		observed = true;
		_exit_game = true;
	};
	AutoRestoreBackup observer(_facility_art_observer, &capture);
	IConsole::CmdRegister("m1_facility_art", ObserveFacilityArt);
	const std::string config_path = config.string();
	const std::string source_path = source.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
