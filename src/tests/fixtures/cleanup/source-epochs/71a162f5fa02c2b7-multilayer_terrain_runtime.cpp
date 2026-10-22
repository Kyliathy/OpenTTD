/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_terrain_runtime.cpp Real application load and raster evidence for the M1 geological/portal bindings.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group82
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../multilayer_sprite.h"
#include "../tbm.h"
#include "../spoil_receiver.h"
#include "../spritecache.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../console_func.h"
#include "../console_internal.h"
#include "native_art_render_fixture.h"
#include "layer_save_fixture.h"
#include "../saveload/saveload_func.h"
#include <cstdlib>

#include "../safeguards.h"

static std::function<void()> *_terrain_art_observer = nullptr; ///< Process-local loaded-world observer, absent from normal game builds.

/**
 * Observe the loaded world through its ordinary startup console dispatch, even while paused.
 * @param arguments Exact no-argument test command; no gameplay input is accepted.
 * @return True once the isolated render observer was dispatched.
 */
static bool ObserveTerrainArt(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1);
	REQUIRE(_terrain_art_observer != nullptr);
	(*_terrain_art_observer)();
	return true;
}

/** Isolated process: actual saved world, actual graphics loader, and actual extra-view raster rendering. */
TEST_CASE("M1 terrain portal raster uses loaded world state without advancing its owners", "[.m1-terrain-art]")
{
	const auto root = std::filesystem::current_path().parent_path();
	const char *input = std::getenv("OTTD_GEOLOGY_SOURCE");
	const auto source = input == nullptr ? root / "src/tests/fixtures/haulage/portal-native-deep-stopped.sav" : std::filesystem::path(input);
	const char *output = std::getenv("OTTD_GEOLOGY_OUTPUT");
	const std::string prefix = output == nullptr ? "m1-terrain" : std::filesystem::path(output).filename().string();
	const auto config = root / "build/native-art.cfg";
	const auto reviews = root / "docs/zz-reach2/upgrades/2026-10/assets/reviews";
	REQUIRE(std::filesystem::exists(source));
	REQUIRE(std::filesystem::exists(config));
	std::filesystem::create_directories(reviews);
	bool observed = false;
	std::exception_ptr failure;
	/** Render the fully loaded paused world without injecting a simulation tick. */
	std::function<void()> capture = [&] {
		if (observed) return;
		try {
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			const auto projects = CaptureTBMProjects();
			REQUIRE_FALSE(projects.empty());
			/** @param project Actual saved controller. @return Whether it is the current unfinished work owner. */
			const auto active = std::ranges::find_if(projects, [](const TBMProject &project) {
				// Finished history is not the active stopped machine's work face.
				return !IsTBMProjectFinished(project.phase);
			});
			REQUIRE(active != projects.end());
			const TileIndex face = GetTBMWorkState(active->id).face;
			REQUIRE(face != INVALID_TILE);
			REQUIRE(GetMultiLayerTerrainSprite(face).has_value());
			const uint64_t geology = ComputeGeologyStateDigest(GetGeologyWorldState());
			const auto receivers = CaptureSpoilReceivers();
			if (const char *bad = std::getenv("OTTD_GEOLOGY_BAD_PREVIEW")) {
				CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
				CHECK(CaptureTBMProjects() == projects);
				CHECK(CaptureSpoilReceivers() == receivers);
				fmt::print("REGIONAL_PREVIEW refused=true reason={} unchanged=true\n", _load_check_data.error_msg);
				observed = true;
				_exit_game = true;
				return;
			}
			REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			SavedRandomSeeds before;
			SaveRandomSeeds(&before);
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			// Co-located rock layers are rendered by independent explicit viewports.
			// The main camera remains untouched, reproducing the extra-view boundary.
			for (MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
				const TileIndex centre = MultiLayerMap::TileInLayer(face, layer);
				for (ZoomLevel zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
					const auto name = fmt::format("{}-layer-{}-zoom-{}.ppm", prefix, to_underlying(layer), to_underlying(zoom));
					CaptureNativeArtViewport(centre, zoom, reviews / name);
				}
			}
			// Actual native picker scenes cover both independent endpoints and all
			// four views of each depth family without inventing terrain/build rights.
			CaptureNativeArtPortalPreviews(MapLayer::Surface, MapLayer::Underground, reviews / (prefix + "-picker-basic-upper.ppm"));
			CaptureNativeArtPortalPreviews(MapLayer::Underground, MapLayer::Surface, reviews / (prefix + "-picker-basic-lower.ppm"));
			CaptureNativeArtPortalPreviews(MapLayer::Underground, MapLayer::DeepUnderground, reviews / (prefix + "-picker-deep-upper.ppm"));
			CaptureNativeArtPortalPreviews(MapLayer::DeepUnderground, MapLayer::Underground, reviews / (prefix + "-picker-deep-lower.ppm"));
			REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			SavedRandomSeeds after;
			SaveRandomSeeds(&after);
			CHECK(std::ranges::equal(before.random.state, after.random.state));
			CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			CHECK(CaptureTBMProjects() == projects);
			CHECK(CaptureSpoilReceivers() == receivers);
			if (output != nullptr) {
				const auto path = std::filesystem::path(output).concat("-resaved.sav");
				REQUIRE_FALSE(std::filesystem::exists(path));
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
				CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			}
			fmt::print("REGIONAL_NATIVE version={} seed={} digest={} source={}\n", GetGeologyWorldState().hardness_version,
				GetGeologyWorldState().hardness_seed, geology, source.filename().string());
			fmt::print("M1_TERRAIN_RUNTIME frames=8 picker_views=16 actual_viewport_render=true native_and_2x=true owners_unchanged=true\n");
			observed = true;
		} catch (...) {
			// Complete native shutdown before the test framework reports a failure.
			failure = std::current_exception();
			observed = true;
		}
		_exit_game = true;
	};
	AutoRestoreBackup observer(_terrain_art_observer, &capture);
	IConsole::CmdRegister("m1_terrain_art", ObserveTerrainArt);
	const std::string config_path = config.string();
	const std::string source_path = source.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
