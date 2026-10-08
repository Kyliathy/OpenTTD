/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_control_runtime.cpp Actual loaded UI, purchase/cargo and active survey raster proof.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups83B/85B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoints8.7/8.8
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../multilayer_sprite.h"
#include "../underground_gui.h"
#include "../underground_vehicle.h"
#include "../tbm.h"
#include "../spoil_receiver.h"
#include "../spritecache.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../console_internal.h"
#include "../engine_gui.h"
#include "../engine_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../layer_portal_cmd.h"
#include "../widgets/underground_widget.h"
#include "../core/random_func.hpp"
#include <cstdlib>
#include "native_art_render_fixture.h"

#include "../safeguards.h"

static std::function<void()> *_control_art_observer = nullptr; ///< Isolated application-test callback, absent from production.

/**
 * Dispatch a fully loaded paused-world UI observer through the normal startup command hook.
 * @param arguments Exact no-argument observer command; no gameplay input is accepted.
 * @return True once its isolated native capture callback ran.
 */
static bool ObserveControlArt(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1);
	REQUIRE(_control_art_observer != nullptr);
	(*_control_art_observer)();
	return true;
}

/** The actual loader and real widget renderer must consume the baked UI families without changing any saved owner. */
TEST_CASE("M1 control raster consumes actual loaded windows engines cargo and seams", "[.m1-control-art]")
{
	const auto root = std::filesystem::current_path().parent_path();
	const auto source = root / "src/tests/fixtures/haulage/portal-native-deep-stopped.sav";
	const auto config = root / "build/native-art.cfg";
	const auto reviews = root / "docs/zz-reach2/upgrades/2026-10/assets/reviews";
	REQUIRE(std::filesystem::exists(source));
	REQUIRE(std::filesystem::exists(config));
	std::filesystem::create_directories(reviews);
	bool observed = false;
	std::exception_ptr failure;
	/** Capture actual loaded controls and pixels once, while the saved simulation remains paused. */
	std::function<void()> capture = [&] {
		if (observed) return;
		try {
			const auto policy = GetCurrentMultiLayerProfileSelection();
			REQUIRE(policy.id == MultiLayerGameplayProfileID::MultilayerIndustry);
			const auto projects = CaptureTBMProjects();
			const auto receivers = CaptureSpoilReceivers();
			const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
			REQUIRE(Company::IsValidID(_local_company));
			const Money money = Company::Get(_local_company)->money;
			SavedRandomSeeds before;
			SaveRandomSeeds(&before);
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			const auto records = GetGeologyWorldState().deposits.Records();
			// Each real rock-plane window owns its own survey cursor and marker
			// lookup; no camera switch or saved information/terrain edit is required.
			for (const MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
				Window *window = ShowUndergroundConstructionToolbar(layer, policy);
				REQUIRE(window != nullptr);
				// Headless video has no live screen pixel target. Refresh real
				// availability inside the screenshot canvas before arming its cursor.
				CaptureNativeArtWindow(*window, reviews / fmt::format("m1-controls-layer-{}.ppm", to_underlying(layer)));
				window->OnClick({}, WID_UC_SURVEY, 1);
				const auto seam = std::ranges::find(records, layer, &GeologyDeposit::layer);
				REQUIRE(seam != records.end());
				const TileIndex tile = TileXYL(seam->spans.front().first_x, seam->spans.front().y, layer);
				REQUIRE(GetUndergroundSurveyOverlaySprite(tile).has_value());
				window->OnPlaceObject({}, tile);
				CaptureNativeArtWindow(*window, reviews / fmt::format("m1-controls-layer-{}.ppm", to_underlying(layer)));
				CaptureNativeArtViewport(tile, ZoomLevel::Normal, reviews / fmt::format("m1-survey-layer-{}.ppm", to_underlying(layer)));
				window->OnPlaceObjectAbort();
				window->Close();
				Window::DeleteClosedWindows();
			}
			REQUIRE_FALSE(projects.empty());
			if (std::getenv("OTTD_M1_SURVEY_GALLERY") != nullptr) {
				// Every selected resource is represented by its actual retained seam
				// and active survey owner, never a fabricated label or reserve state.
				for (const CargoLabel label : {CargoLabel{"COAL"}, CargoLabel{"IORE"}, CargoLabel{"CORE"}, CargoLabel{"BAUX"}, CargoLabel{"SALT"},
					CargoLabel{"LITH"}, CargoLabel{"NORE"}, CargoLabel{"COBT"}, CargoLabel{"RARE"}, CargoLabel{"URAN"}}) {
					const std::string name = label.AsString();
					const auto seam = std::ranges::find(records, label, &GeologyDeposit::resource);
					REQUIRE(seam != records.end());
					Window *window = ShowUndergroundConstructionToolbar(seam->layer, policy);
					REQUIRE(window != nullptr);
					CaptureNativeArtWindow(*window, reviews / fmt::format("m1-survey-{}-controls.ppm", name));
					window->OnClick({}, WID_UC_SURVEY, 1);
					const TileIndex tile = TileXYL(seam->spans.front().first_x, seam->spans.front().y, seam->layer);
					REQUIRE(GetUndergroundSurveyOverlaySprite(tile).has_value());
					window->OnPlaceObject({}, tile);
					CaptureNativeArtWindow(*window, reviews / fmt::format("m1-survey-{}-controls.ppm", name));
					// Native and 2x world views preserve real seam boundaries and
					// the canonical marker anchor; survey selection writes no reserves.
					for (const ZoomLevel zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
						CaptureNativeArtViewport(tile, zoom, reviews / fmt::format("m1-survey-{}-zoom-{}.ppm", name, to_underlying(zoom)));
					}
					window->OnPlaceObjectAbort();
					window->Close();
					Window::DeleteClosedWindows();
				}
				fmt::print("M1_SURVEY_GALLERY resources=10 frames=30 actual_seams=true\n");
			}
			if (std::getenv("OTTD_M1_JOB_ART") != nullptr) {
				// Inspect the actual latest saved project through the real lower
				// housing and existing project cursor. Its canonical authorization
				// supplies overlays while the saved progress remains unchanged.
				const auto pair = GetLayerPortalPairKey(projects.back().portal);
				REQUIRE(pair.has_value());
				const TileIndex lower = GetLayerPortalPairTile(*pair);
				Window *window = ShowUndergroundConstructionToolbar(GetTileLayer(lower), policy);
				REQUIRE(window != nullptr);
				CaptureNativeArtWindow(*window, reviews / "m1-job-inspection-controls.ppm");
				window->OnClick({}, WID_UC_PROJECT, 1);
				window->OnPlaceObject({}, lower);
				CaptureNativeArtWindow(*window, reviews / "m1-job-inspection-controls.ppm");
				CaptureNativeArtViewport(lower, ZoomLevel::Normal, reviews / "m1-job-inspection-footprint.ppm");
				window->OnPlaceObjectAbort();
				window->Close();
				Window::DeleteClosedWindows();
				fmt::print("M1_JOB_RUNTIME frames=2 actual_job_cursor_and_partial_work=true actual_canonical_overlay=true\n");
			}
			ShowLayerPortalWindow(projects.back().portal);
			const auto pair = GetLayerPortalPairKey(projects.back().portal);
			REQUIRE(pair.has_value());
			Window *portal = FindWindowById(WindowClass::LayerPortal, pair->base());
			REQUIRE(portal != nullptr);
			CaptureNativeArtWindow(*portal, reviews / "m1-controls-portal.ppm");
			portal->Close();
			Window::DeleteClosedWindows();
			/** Actual ordinary purchase rendering plus actual loaded first-party cargo resolvers. */
			CaptureNativeArtCanvas(640, 240, reviews / "m1-purchase-cargo.ppm", [&] {
				// Draw each real engine through GetRailIcon's ordinary caller,
				// proving native mapping/availability reaches the loaded renderer.
				for (uint16_t model = 0; model < 5; ++model) {
					const EngineID engine = model < 4 ? GetTBMEngine(model) : GetUndergroundEngine(model);
					REQUIRE(Engine::IsValidID(engine));
					REQUIRE(GetMultiLayerEnginePurchaseSprite(engine, IsEngineBuildable(engine, VehicleType::Train, _local_company)).has_value());
					DrawTrainEngine(model * 128, model * 128 + 127, model * 128 + 24, 32, engine, PAL_NONE, EngineImageType::Purchase);
				}
				// Cargo definitions must actually belong to the loaded immutable
				// first-party package; matching resource labels alone are insufficient.
				for (const CargoLabel label : {CargoLabel{"ERTH"}, CargoLabel{"ROCK"}}) {
					const CargoType type = GetCargoTypeByLabel(label);
					REQUIRE(IsValidCargoType(type));
					const auto *cargo = CargoSpec::Get(type);
					const auto native = GetMultiLayerCargoSprite(*cargo);
					REQUIRE(native.has_value());
					CHECK(cargo->GetCargoIcon() == *native);
					DrawSpriteIgnorePadding(cargo->GetCargoIcon(), PAL_NONE,
						Rect{label == CargoLabel{"ERTH"} ? 96 : 416, 128, label == CargoLabel{"ERTH"} ? 223 : 543, 207}, {AlignmentH::Centre, AlignmentV::Middle});
				}
			});
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
			CHECK(Company::Get(_local_company)->money == money);
			fmt::print("M1_CONTROL_RUNTIME frames=6 actual_widgets=true engines=5 cargo_icons=2 active_survey_layers=2 owners_unchanged=true\n");
			observed = true;
		} catch (...) {
			// Complete native shutdown before reporting an observer assertion.
			failure = std::current_exception();
			observed = true;
		}
		_exit_game = true;
	};
	AutoRestoreBackup observer(_control_art_observer, &capture);
	IConsole::CmdRegister("m1_control_art", ObserveControlArt);
	const std::string config_path = config.string();
	const std::string source_path = source.string();
	std::array<std::string_view, 17> arguments = {"Debug/openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
