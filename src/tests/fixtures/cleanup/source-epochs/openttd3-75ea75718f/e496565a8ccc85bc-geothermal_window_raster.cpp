/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_window_raster.cpp Actual ordinary loaded paid-world geothermal window/viewport captures.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15CDC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups84BB/84BC/84BD/84CB.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md and archi-viewport.md.
 * This observer creates no fixture world, cargo, work, power, facility or command; all inputs are retained real first-plant checkpoints.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../3rdparty/nlohmann/json.hpp"
#include "test_artifact_paths.h"
#include "layer_save_fixture.h"
#include "native_art_render_fixture.h"
#include "geothermal_machinery_view_fixture.h"
#include "geothermal_conduit_view_fixture.h"
#include "selected_control_view_fixture.h"
#include "selected_purchase_view_fixture.h"
#include "../openttd.h"
#include "../underground_gui.h"
#include "../geothermal_controls.h"
#include "../geothermal_collector.h"
#include "../geothermal_generator.h"
#include "../geothermal_sprite.h" // Plan14-84BB optional current actual generator-campus evidence; no fixture world is created.
#include "../geothermal_riser.h"
#include "../multilayer_sprite.h"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../tilehighlight_func.h"
#include "../industry.h"
#include "../saveload/saveload_func.h"
#include "../timer/timer_game_tick.h"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../blitter/factory.hpp"
#include "../core/random_func.hpp"
#include "../widgets/underground_widget.h"
#include "../table/strings.h"
#include <cstdlib>
#include "../safeguards.h"

static std::function<void()> *_geothermal_raster_observer = nullptr; ///< Isolated test startup callback, absent from normal game targets.

/** @param arguments Exact no-argument startup command. @return True after dispatching the current real loaded-world capture. */
static bool ObserveGeothermalRaster(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_geothermal_raster_observer != nullptr); (*_geothermal_raster_observer)(); return true;
}

/** @param path Current invocation's review save. Preserve complete actual native owners, without changing the source's pause or injecting a fixture package. */
static void SaveGeothermalRasterWorld(const std::filesystem::path &path)
{
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/**
 * Capture the actual selected widget/status tree and require its complete wrapped text to fit.
 * @param window Existing actual construction window.
 * @param path Invocation-owned actual PPM output.
 * @param name Semantic page name for the producer-bound receipt.
 */
static void CaptureGeothermalRasterWindow(Window &window, const std::filesystem::path &path, std::string_view name)
{
	CaptureNativeArtWindow(window, path);
	const auto *widget = window.GetWidget<NWidgetBase>(WID_UC_GEO_STATUS);
	const auto text = window.GetWidgetString(WID_UC_GEO_STATUS, STR_GEO_HELP);
	const int text_height = GetStringHeight(text, widget->current_x);
	CHECK(text_height <= static_cast<int>(widget->current_y)); CHECK(window.height <= 720);
	// An information view has one visible selector row. Hidden site/recipient
	// pages must not leave their former maximum height as a blank text header.
	if (name == "stock" || name == "energy" || name == "town-info") {
		const auto *selector = window.GetWidget<NWidgetBase>(WID_UC_GEO_INFORMATION);
		CHECK(widget->pos_y <= selector->pos_y + static_cast<int>(selector->current_y) + ScaleGUITrad(4));
	}
	const nlohmann::json record{{"page", name}, {"path", path.generic_string()}, {"width", window.width}, {"height", window.height},
		{"status_text", text}, {"status_text_height", text_height}, {"status_height", widget->current_y}};
	fmt::print("GEO_RASTER_WINDOW {}\n", record.dump());
}

/** Ordinary loader/real graphics/widget/viewport paths must preserve all actual paid owners while revealing only the active natural survey. */
TEST_CASE("Loaded paid geothermal controls render actual windows and natural vent survey", "[.geothermal-window-raster]")
{
	const char *input_text = std::getenv("OTTD_GEO_RASTER_INPUT"), *scene_text = std::getenv("OTTD_GEO_RASTER_SCENE");
	REQUIRE(input_text != nullptr); REQUIRE(scene_text != nullptr);
	const auto input = ResolveTestInputPath(input_text), config = GetTestRuntimeConfig();
	const auto paths = GetTestArtifactPaths("geothermal", "window-raster");
	const std::string scene{scene_text}; bool observed = false; std::exception_ptr failure;
	/** Capture only paused ordinary afterload state; every simulation owner remains original. */
	std::function<void()> capture = [&] {
		if (observed) return;
		try {
			REQUIRE(_pause_mode.Any()); REQUIRE(Company::IsValidID(_local_company));
			const char *riser_capture = std::getenv("OTTD_GEO_RISER_CAPTURE");
			const bool capture_risers = riser_capture != nullptr && std::string_view{riser_capture} == "1";
			AutoRestoreBackup actor{_current_company, _local_company}; // Native startup may use a system actor; presentation queries borrow the actual player's context and restore it afterward.
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			REQUIRE_FALSE(GetGeologyWorldState().vents.empty());
			const auto vent = GetGeologyWorldState().vents.front(); const auto tile = TileXYL(vent.x, vent.y, vent.layer);
			const auto terrain = GetGeologyWorldState().terrain.Get(tile); REQUIRE(terrain.has_value());
			CHECK((*terrain == ExcavationState::Excavated) == (scene != "solid"));
			const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), claims = ComputeGeothermalSiteDigest();
			const auto collectors = ComputeGeothermalCollectorDigest(), generators = ComputeGeothermalGeneratorDigest(), risers = ComputeGeothermalRiserDigest();
			const auto power = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_local_company)->money;
			const auto tick = TimerGameTick::counter; SavedRandomSeeds before; SaveRandomSeeds(&before);
			SaveGeothermalRasterWorld(paths.Output("saves", "world-before.sav"));
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			Window *window = ShowUndergroundConstructionToolbar(MapLayer::DeepUnderground, GetCurrentMultiLayerProfileSelection()); REQUIRE(window != nullptr);
			// Headless startup has no live pixel target. Real first painting must
			// occur inside the actual indexed canvas before enabling a tool.
			CaptureNativeArtWindow(*window, paths.images / "collapsed.ppm");
			window->OnClick({}, WID_UC_GEOTHERMAL, 1);
			CaptureGeothermalRasterWindow(*window, paths.images / "sites.ppm", "sites");
			window->OnDropdownSelect(WID_UC_GEO_PAGE, 1, 0);
			CaptureGeothermalRasterWindow(*window, paths.images / "utilities.ppm", "utilities");
			// An already funded original sender provides a real unchanged
			// duplicate-funding refusal, without creating another fixture portal.
			for (const auto &riser : CaptureGeothermalRisers()) {
				if (riser.site.owner != _local_company || GetTileLayer(riser.site.tile) != MapLayer::DeepUnderground) continue; // Only the actual owned Deep geometry belongs to this window's explicit placement context.
				window->OnDropdownSelect(WID_UC_GEO_ACCESS, to_underlying(riser.site.access), 0);
				window->OnClick({}, WID_UC_GEO_RISER, 1); window->OnPlaceObject({}, riser.site.portal);
				window->OnPlaceObject({}, riser.site.tile); window->OnPlaceObject({}, riser.site.port);
				CaptureGeothermalRasterWindow(*window, paths.images / "riser-refusal.ppm", "riser-refusal"); ResetObjectToPlace(); break;
			}
			TileIndex inspected = tile;
			IndustryID generator = IndustryID::Invalid();
			const auto model = scene == "vent-tap" ? GeothermalGeneratorModel::VentTap : GeothermalGeneratorModel::MagmaLoop;
			// Original riser construction precedes the later generator model.
			// Its optional body capture inspects those real earlier machinery
			// owners without requiring a generator that did not yet exist.
			if (scene != "solid" && !capture_risers) {
				// Resolve only real owned machinery from this original save. No
				// assumed numeric pool identity or predicted blueprint is used.
				for (const auto &state : CaptureGeothermalGenerators()) {
					if (state.owner != _local_company || state.model != model) continue; // Another controller or model is not this scene's original paid machine.
					REQUIRE(generator == IndustryID::Invalid()); generator = state.industry;
				}
				REQUIRE(Industry::IsValidID(generator)); inspected = GetGeothermalGeneratorPort(generator);
				const auto *state = GetGeothermalGeneratorState(generator); REQUIRE(state != nullptr);
				if (scene == "funded") { CHECK(state->work_ticks == 0); CHECK_FALSE(state->materials_committed); }
				else if (scene == "partial") { CHECK(state->work_ticks > 0); CHECK(state->work_ticks < GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::MagmaPowerhouse)->work_ticks); }
				else CHECK(IsGeothermalGeneratorConstructionComplete(generator));
				if (IsGeothermalGeneratorConstructionComplete(generator)) window->OnDropdownSelect(WID_UC_GEO_GENERATOR, generator.base(), 0);
			}
			window->OnDropdownSelect(WID_UC_GEO_PAGE, 3, 0); window->OnClick({}, WID_UC_GEO_SURVEY, 1); window->OnPlaceObject({}, inspected);
			// Native material/work and energy views use their actual current
			// source; every frame retains exact pixels and wrapped-text bounds.
			for (int information = 0; information < 2; ++information) {
				window->OnDropdownSelect(WID_UC_GEO_INFORMATION, information, 0);
				CaptureGeothermalRasterWindow(*window, paths.images / (information == 0 ? "stock.ppm" : "energy.ppm"), information == 0 ? "stock" : "energy");
			}
			const UtilityPowerConsumer *town = nullptr;
			for (const auto &consumer : GetUtilityPowerConsumers()) {
				if (!consumer.active || consumer.owner != _local_company || consumer.town == TownID::Invalid()) continue; // Only a real retained owned town account can supply the useful-export observation.
				REQUIRE(town == nullptr); town = &consumer;
			}
			if (town != nullptr) {
				const auto source = GetGeothermalControlInspection(town->town_source_port);
				if (source.generator != nullptr) window->OnDropdownSelect(WID_UC_GEO_GENERATOR, source.generator->industry.base(), 0);
				// These originals contain one paid sender per rock plane. Select
				// their actual issued machinery through the same menus a player uses.
				for (const auto &riser : CaptureGeothermalRisers()) {
					if (riser.site.owner != _local_company || !IsGeothermalRiserOperational(riser.industry)) continue; // Unfinished or foreign senders are not actual selectable route participants.
					window->OnDropdownSelect(GetTileLayer(riser.site.tile) == MapLayer::DeepUnderground ? WID_UC_GEO_FIRST_RISER : WID_UC_GEO_SECOND_RISER, riser.industry.base() + 1, 0);
				}
				window->OnDropdownSelect(WID_UC_GEO_TERMINAL, town->town_terminal.base(), 0);
				window->OnDropdownSelect(WID_UC_GEO_TOWN, town->town.base(), 0); window->OnDropdownSelect(WID_UC_GEO_INFORMATION, 2, 0);
				CaptureGeothermalRasterWindow(*window, paths.images / "town-info.ppm", "town-info");
				window->OnDropdownSelect(WID_UC_GEO_PAGE, 2, 0);
				CaptureGeothermalRasterWindow(*window, paths.images / "town-tools.ppm", "town-tools");
			}
			ResetObjectToPlace(); CaptureNativeArtViewport(tile, ZoomLevel::In2x, paths.images / "vent-inactive.ppm");
			window->OnClick({}, WID_UC_GEO_SURVEY, 1); window->OnPlaceObject({}, tile);
			const auto expected = GetMultiLayerSprite(MultiLayerSpriteAsset::GeothermalVent,
				{static_cast<uint8_t>(scene == "solid" ? 1 : 2), GetMultiLayerVisualFrame(MultiLayerSpriteAsset::GeothermalVent, tick, false)});
			CHECK(GetUndergroundGeothermalSurveyOverlaySprite(tile) == expected);
			CaptureNativeArtViewport(tile, ZoomLevel::In2x, paths.images / "vent-active.ppm");
			window->OnClick({}, WID_UC_SURVEY, 1); CHECK_FALSE(GetUndergroundGeothermalSurveyOverlaySprite(tile).has_value());
			ResetObjectToPlace(); CaptureNativeArtViewport(tile, ZoomLevel::In2x, paths.images / "vent-cancelled.ppm");
			window->OnClick({}, WID_UC_GEOTHERMAL, 1); window->OnClick({}, WID_UC_TUBES, 1);
			CaptureNativeArtWindow(*window, paths.images / "original-tubes.ppm");
			window->Close(); Window::DeleteClosedWindows();
			const char *machinery = std::getenv("OTTD_GEO_MACHINERY_CAPTURE");
			// The selected art integration opts into whole actual generator sites,
			// rather than treating a distant vent-centred viewport or window text
			// as evidence that every approved native machinery cell was rendered.
			if (machinery != nullptr && std::string_view{machinery} == "1") {
				for (const auto &state : CaptureGeothermalGenerators()) {
					if (state.owner != _local_company) continue;
					const auto *industry = Industry::Get(state.industry); const auto geometry = GetGeothermalGeneratorGeometry(state.model); REQUIRE(geometry.has_value());
					const auto asset = state.model == GeothermalGeneratorModel::VentTap ? MultiLayerSpriteAsset::FacilityVentPowerhouse : MultiLayerSpriteAsset::FacilityMagmaPowerhouse;
					const auto &range = _multilayer_m1_sprite_ranges[static_cast<size_t>(asset)]; const auto centre = TileAddWrap(industry->location.tile, 1, 1);
					nlohmann::json cells = nlohmann::json::array();
					// Every source cell is borrowed from actual native machinery. The
					// independent Python observer matches original packed atlas pixels
					// at this real world projection; no painted or relocated model passes.
					for (uint y = 0; y < industry->location.h; ++y) {
						for (uint x = 0; x < industry->location.w; ++x) {
							const auto cell = TileAddWrap(industry->location.tile, x, y); const auto back = GetGeothermalGeneratorSprite(cell, false, tick), front = GetGeothermalGeneratorSprite(cell, true, tick);
							REQUIRE(back.has_value()); REQUIRE(front.has_value()); CHECK(GetMultiLayerIndustrySprite(cell, false, tick) == back); CHECK(GetMultiLayerIndustrySprite(cell, true, tick) == front);
							const auto offset = *back - SPR_MAP_LAYER_BASE - range.offset;
							cells.push_back({{"tile", cell.base()}, {"x", TileX(cell)}, {"y", TilePlaneY(cell)}, {"z", TilePixelHeight(cell)}, {"cell", y * industry->location.w + x}, {"back", *back}, {"front", *front},
								{"state", offset / (range.frames * range.directions * range.tiles * range.parts)}, {"frame", offset / (range.directions * range.tiles * range.parts) % range.frames},
								{"view", offset / (range.tiles * range.parts) % range.directions}});
						}
					}
					for (const auto zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
						const auto path = paths.images / fmt::format("generator-{}-zoom-{}.ppm", to_underlying(state.model), to_underlying(zoom)); CaptureNativeArtViewport(centre, zoom, path);
						const nlohmann::json record{{"asset_id", geometry->sprite_asset}, {"industry", state.industry.base()}, {"site", state.site.base()}, {"work", state.work_ticks}, {"service", state.active_service_ticks},
							{"zoom", to_underlying(zoom)}, {"centre_x", TileX(centre)}, {"centre_y", TilePlaneY(centre)}, {"centre_z", TilePixelHeight(centre)}, {"cells", cells}, {"path", path.generic_string()}};
						fmt::print("GEO_MACHINERY_FRAME {}\n", record.dump());
					}
				}
			}
			if (capture_risers) {
				for (const auto &state : CaptureGeothermalRisers()) { // Only actual owned native risers from the ordinary loaded paid world supply these campus observations.
					if (state.site.owner != _local_company) continue;
					const auto *industry = Industry::Get(state.industry); const auto centre = TileAddWrap(industry->location.tile, 1, 1);
					const auto &range = _multilayer_m1_sprite_ranges[static_cast<size_t>(MultiLayerSpriteAsset::FacilityUtilityRiser)];
					nlohmann::json cells = nlohmann::json::array(), links = nlohmann::json::array();
					// All four fixed source cells retain their exact compiled part
					// selection and actual rock-plane projection; no fixture, pose
					// assignment or overlay provides a missing paid native machine.
					for (uint y = 0; y < industry->location.h; ++y) {
						for (uint x = 0; x < industry->location.w; ++x) {
							const auto tile = TileAddWrap(industry->location.tile, x, y); const auto back = GetGeothermalRiserSprite(tile, false, tick), front = GetGeothermalRiserSprite(tile, true, tick);
							REQUIRE(back.has_value()); REQUIRE(front.has_value()); CHECK(GetMultiLayerIndustrySprite(tile, false, tick) == back); CHECK(GetMultiLayerIndustrySprite(tile, true, tick) == front);
							const auto offset = *back - SPR_MAP_LAYER_BASE - range.offset;
							cells.push_back({{"tile", tile.base()}, {"x", TileX(tile)}, {"y", TilePlaneY(tile)}, {"z", TilePixelHeight(tile)}, {"cell", y * industry->location.w + x}, {"back", *back}, {"front", *front},
								{"state", offset / (range.frames * range.directions * range.tiles * range.parts)}, {"frame", offset / (range.directions * range.tiles * range.parts) % range.frames}, {"view", 0}});
						}
					}
					for (const auto &link : GetUtilityPowerLinks()) { // Retain this original machinery's actual electrical edges and terminal history, never a copied transfer account.
						if (link.kind != UtilityLinkKind::ElectricalRiser || link.riser != state.industry) continue; // Other machinery and local cells cannot prove this original sender's transfer.
						links.push_back({{"id", link.id.base()}, {"active", link.active}, {"flow", link.flow_j}});
					}
					for (const auto zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) { // Both ordinary native zooms must show each whole original2x2 campus on its real layer.
						const auto path = paths.images / fmt::format("riser-{}-zoom-{}.ppm", state.industry.base(), to_underlying(zoom)); CaptureNativeArtViewport(centre, zoom, path);
						const nlohmann::json record{{"asset_id", "facility.utility-riser"}, {"industry", state.industry.base()}, {"layer", to_underlying(GetTileLayer(state.site.tile))},
							{"work", state.work_ticks}, {"materials_committed", state.materials_committed}, {"operational", IsGeothermalRiserOperational(state.industry)}, {"portal_retired", state.portal_retired}, {"links", links},
							{"zoom", to_underlying(zoom)}, {"centre_x", TileX(centre)}, {"centre_y", TilePlaneY(centre)}, {"centre_z", TilePixelHeight(centre)}, {"cells", cells}, {"path", path.generic_string()}};
						fmt::print("GEO_RISER_FRAME {}\n", record.dump());
					}
				}
			}
			const char *interaction = std::getenv("OTTD_GEO_MACHINERY_INTERACTION");
			// This optional verification reuses the original paid collectors,
			// both generator models and both rock-plane risers; it never funds or
			// edits a participant to supply a convenient interaction scene.
			if (interaction != nullptr && std::string_view{interaction} == "1") {
				for (const auto *industry : Industry::Iterate()) {
					const auto *generator = GetGeothermalGeneratorState(industry->index); const auto *riser = GetGeothermalRiserState(industry->index); const auto *collector = GetGeothermalCollectorState(industry->index);
					if (generator != nullptr) {
						if (generator->owner != _local_company) continue;
						const auto geometry = GetGeothermalGeneratorGeometry(generator->model); REQUIRE(geometry.has_value());
						CaptureGeothermalMachineryInteraction(*industry, generator->model == GeothermalGeneratorModel::VentTap ? MultiLayerSpriteAsset::FacilityVentPowerhouse : MultiLayerSpriteAsset::FacilityMagmaPowerhouse, geometry->sprite_asset, paths);
					} else if (riser != nullptr) {
						if (riser->site.owner != _local_company) continue;
						CaptureGeothermalMachineryInteraction(*industry, MultiLayerSpriteAsset::FacilityUtilityRiser, "facility.utility-riser", paths);
					} else if (collector != nullptr) {
						if (collector->owner != _local_company) continue;
						CaptureGeothermalMachineryInteraction(*industry, MultiLayerSpriteAsset::FacilityVentCollector, "facility.vent-collector", paths);
					} else {
						// Actual unrelated industries keep their existing receiver or
						// foreign/native resolver. No new geothermal selector/bounds
						// can appropriate their real world cells.
						for (const auto tile : industry->location) {
							if (!industry->TileBelongsToIndustry(tile)) continue;
							CHECK_FALSE(GetGeothermalGeneratorSprite(tile, false, tick).has_value()); CHECK_FALSE(GetGeothermalRiserSprite(tile, false, tick).has_value());
							CHECK_FALSE(GetGeothermalIndustryBackBounds(tile).has_value()); CHECK_FALSE(GetGeothermalIndustryFrontBounds(tile).has_value());
							CHECK(GetMultiLayerIndustrySprite(tile, false, tick) == GetMultiLayerReceiverSprite(tile, false, tick));
						}
						fmt::print("GEO_MACHINERY_FALLBACK industry={} unchanged_native_resolver=true\n", industry->index.base());
					}
				}
			}
			const char *conduits = std::getenv("OTTD_GEO_CONDUIT_CAPTURE");
			if (conduits != nullptr && std::string_view{conduits} == "1") CaptureGeothermalConduitViews(paths); // Explicit current art proof borrows the actual paid installations; normal window capture retains its original scope.
			const char *controls = std::getenv("OTTD_GEO_CONTROL_CAPTURE");
			if (controls != nullptr && std::string_view{controls} == "1") CaptureSelectedControlViews(paths); // Actual widget expansion/player policy supplies original icon states; no command or paid-world owner is changed.
			const char *purchase = std::getenv("OTTD_SELECTED_PURCHASE_CAPTURE");
			// Only the explicitly requested art observer adds purchase canvases;
			// ordinary paid-world capture retains its existing scope and owners.
			if (purchase != nullptr && std::string_view{purchase} == "1") CaptureSelectedPurchaseViews(paths);
			REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			SaveGeothermalRasterWorld(paths.Output("saves", "world-after.sav"));
			SavedRandomSeeds after; SaveRandomSeeds(&after);
			CHECK(std::ranges::equal(before.random.state, after.random.state)); CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeGeothermalSiteDigest() == claims);
			CHECK(ComputeGeothermalCollectorDigest() == collectors); CHECK(ComputeGeothermalGeneratorDigest() == generators); CHECK(ComputeGeothermalRiserDigest() == risers);
			CHECK(ComputeUtilityPowerDigest() == power); CHECK(Company::Get(_local_company)->money == cash); CHECK(TimerGameTick::counter == tick);
			fmt::print("GEO_RASTER_READY scene={} native_owners_unchanged=true rng_unchanged=true actual_widgets=true actual_viewport=true\n", scene);
			observed = true;
		} catch (...) { failure = std::current_exception(); observed = true; }
		_exit_game = true;
	};
	AutoRestoreBackup observer{_geothermal_raster_observer, &capture}; IConsole::CmdRegister("geothermal_window_raster", ObserveGeothermalRaster);
	const std::string config_path = config.string(), input_path = input.string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", input_path};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
