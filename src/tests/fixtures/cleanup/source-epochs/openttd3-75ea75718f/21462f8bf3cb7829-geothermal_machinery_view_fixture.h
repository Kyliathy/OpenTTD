/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_machinery_view_fixture.h Actual paid machinery viewport transparency, repetition and native logical industry picking.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BB.3.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * These helpers borrow ordinary loaded industries; they create no fixture world, cargo, machinery, work or utility authority.
 */
#ifndef TESTS_GEOTHERMAL_MACHINERY_VIEW_FIXTURE_H
#define TESTS_GEOTHERMAL_MACHINERY_VIEW_FIXTURE_H
#include "native_art_render_fixture.h"
#include "../3rdparty/nlohmann/json.hpp"
#include "../industry.h"
#include "../industry_map.h"
#include "../map_func.h"
#include "../multilayer_sprite.h"
#include "../openttd.h"
#include "../timer/timer_game_tick.h"

/** @param tile Actual physical cell whose centre is inspected. @param zoom Native screenshot scale. @return Real layer-aware viewport with the same geometry as the ordinary raster capture, never a new map or projected cross-layer owner. */
static Viewport GeothermalMachineryViewport(TileIndex tile, ZoomLevel zoom)
{
	Viewport view{}; view.width = 640; view.height = 360; view.zoom = zoom; view.layer = GetTileLayer(tile);
	const Point centre = RemapCoords(TileX(tile) * TILE_SIZE + TILE_SIZE / 2, TileY(tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(tile));
	view.virtual_width = ScaleByZoom(640, zoom); view.virtual_height = ScaleByZoom(360, zoom);
	view.virtual_left = centre.x - view.virtual_width / 2; view.virtual_top = centre.y - view.virtual_height / 2;
	return view;
}

/** Close only actual industry information windows opened by these native picking probes; no world owner or command is changed. */
static void CloseGeothermalMachineryInformation()
{
	// Correct- and wrong-layer clicks may legitimately open different native
	// industries. Close their view windows before the next pure world capture.
	while (Window *window = FindWindowByClass(WindowClass::IndustryView)) {
		window->Close(); Window::DeleteClosedWindows();
	}
}

/** @param industry Original actual paid industry. @param centre Its physical footprint cell at the viewport centre. @param zoom Native screenshot scale. Verify ordinary logical industry picking, including while artwork is transparent/hidden, and refusal to select that owner through Surface. */
static void PickGeothermalMachinery(const Industry &industry, TileIndex centre, ZoomLevel zoom)
{
	CloseGeothermalMachineryInformation(); AutoRestoreBackup control(_ctrl_pressed, false);
	const auto view = GeothermalMachineryViewport(centre, zoom); const auto point = TranslateXYToTileCoord(view, 320, 180);
	REQUIRE(point.x != -1); CHECK(TileVirtXY(point.x, point.y) == centre); REQUIRE(HandleViewportClicked(view, 320, 180));
	REQUIRE(FindWindowById(WindowClass::IndustryView, industry.index.base()) != nullptr); CloseGeothermalMachineryInformation();
	const auto surface = MultiLayerMap::TileInLayer(centre, MapLayer::Surface); const auto wrong = GeothermalMachineryViewport(surface, zoom);
	HandleViewportClicked(wrong, 320, 180); CHECK(FindWindowById(WindowClass::IndustryView, industry.index.base()) == nullptr);
	CloseGeothermalMachineryInformation();
}

/** @param industry Original ordinary loaded paid machinery. @param asset Its existing compiled native family. @param asset_id Sole public family name. @param paths Current native invocation's routed review destinations. Capture complete actual world sites repeatedly in each native industry display mode and both zooms, preserving logical owner/layer selection. */
static void CaptureGeothermalMachineryInteraction(const Industry &industry, MultiLayerSpriteAsset asset, std::string_view asset_id, const TestArtifactPaths &paths)
{
	const auto centre = TileAddWrap(industry.location.tile, 1, 1); const auto tick = TimerGameTick::counter;
	const auto &range = _multilayer_m1_sprite_ranges[static_cast<size_t>(asset)]; nlohmann::json cells = nlohmann::json::array();
	// Every original world cell/back-front selection remains authoritative
	// across display changes. No display mode or information window may choose
	// another native participant or imply construction/service progress.
	for (uint y = 0; y < industry.location.h; ++y) {
		for (uint x = 0; x < industry.location.w; ++x) {
			const auto tile = TileAddWrap(industry.location.tile, x, y); const auto back = GetMultiLayerIndustrySprite(tile, false, tick), front = GetMultiLayerIndustrySprite(tile, true, tick);
			REQUIRE(back.has_value()); REQUIRE(front.has_value()); const auto offset = *back - SPR_MAP_LAYER_BASE - range.offset;
			cells.push_back({{"tile", tile.base()}, {"x", TileX(tile)}, {"y", TilePlaneY(tile)}, {"z", TilePixelHeight(tile)}, {"cell", y * industry.location.w + x}, {"back", *back}, {"front", *front},
				{"state", offset / (range.frames * range.directions * range.tiles * range.parts)}, {"frame", offset / (range.directions * range.tiles * range.parts) % range.frames},
				{"view", offset / (range.tiles * range.parts) % range.directions}});
		}
	}
	DisplayOptions labels = _display_opt;
	// Real player display options hide labels so the original machinery's
	// pixels, rather than a station sign drawn over its floor, supply the
	// interaction witness. These client-only settings are restored on return.
	for (const auto flag : {DisplayOption::ShowStationNames, DisplayOption::ShowTownNames, DisplayOption::ShowSigns, DisplayOption::ShowWaypointNames}) labels.Reset(flag);
	AutoRestoreBackup display(_display_opt, labels);
	// Every whole campus must retain its real layer/owner at both native
	// scales while only client display policy changes its apparent opacity.
	for (const auto zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
		for (const std::string_view mode : {"opaque", "transparent", "hidden"}) {
			TransparencyOptions transparency = _transparency_opt, invisibility = _invisibility_opt;
			transparency.Reset(TransparencyOption::Industries); invisibility.Reset(TransparencyOption::Industries);
			if (mode != "opaque") transparency.Set(TransparencyOption::Industries);
			if (mode == "hidden") invisibility.Set(TransparencyOption::Industries);
			AutoRestoreBackup transparent(_transparency_opt, transparency); AutoRestoreBackup invisible(_invisibility_opt, invisibility);
			nlohmann::json images = nlohmann::json::array();
			// Repeating the complete native compositor and real tile click must
			// retain pixels/owners in every display mode. Hidden industries keep
			// their normal logical tile click; vehicle alpha picking is separate.
			for (uint repeat = 0; repeat < 2; ++repeat) {
				const auto path = paths.images / fmt::format("machinery-{}-{}-zoom-{}-{}.ppm", industry.index.base(), mode, to_underlying(zoom), repeat);
				CaptureNativeArtViewport(centre, zoom, path); PickGeothermalMachinery(industry, centre, zoom); images.push_back(path.generic_string());
			}
			const nlohmann::json record{{"asset_id", std::string{asset_id}}, {"industry", industry.index.base()}, {"layer", to_underlying(GetTileLayer(centre))},
				{"zoom", to_underlying(zoom)}, {"mode", std::string{mode}}, {"centre_x", TileX(centre)}, {"centre_y", TilePlaneY(centre)}, {"centre_z", TilePixelHeight(centre)}, {"cells", cells},
				{"paths", images}, {"native_industry_window", true}, {"logical_pick_while_hidden", true}, {"surface_same_owner_refused", true}};
			fmt::print("GEO_MACHINERY_INTERACTION {}\n", record.dump());
		}
	}
}
#endif /* TESTS_GEOTHERMAL_MACHINERY_VIEW_FIXTURE_H */
