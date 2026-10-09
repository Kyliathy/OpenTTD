/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file native_art_render_fixture.h Actual viewport raster capture for selected M1 runtime art checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups82/83/85
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#ifndef TESTS_NATIVE_ART_RENDER_FIXTURE_H
#define TESTS_NATIVE_ART_RENDER_FIXTURE_H

#include "test_artifact_paths.h"
#include "../viewport_func.h"
#include "../viewport_type.h"
#include "../gfx_func.h"
#include "../landscape.h"
#include "../zoom_func.h"
#include "../map_layer_gui.h"
#include "../layer_portal_map.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "../core/backup_type.hpp"
#include <filesystem>
#include <fstream>
#include <functional>

/**
 * Render one actual world viewport at the requested layer/zoom into a review artifact.
 * @param tile Actual world tile at the center; its layer supplies filtering independently of the main viewport.
 * @param zoom Native scale or another supported actual viewport zoom level.
 * @param path Repository-owned review PPM output; conversion to PNG preserves the rendered RGB pixels.
 * @return None; require a complete raster and successful output rather than queued screenshot intent.
 */
static void CaptureNativeArtViewport(TileIndex tile, ZoomLevel zoom, const std::filesystem::path &path)
{
	constexpr int width = 640;
	constexpr int height = 360;
	std::vector<uint8_t> pixels(width * height, 0);
	const Point centre = RemapCoords(TileX(tile) * TILE_SIZE + TILE_SIZE / 2, TileY(tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(tile));
	Viewport view{};
	view.width = width;
	view.height = height;
	view.zoom = zoom;
	view.layer = GetTileLayer(tile);
	view.virtual_width = ScaleByZoom(width, zoom);
	view.virtual_height = ScaleByZoom(height, zoom);
	view.virtual_left = centre.x - view.virtual_width / 2;
	view.virtual_top = centre.y - view.virtual_height / 2;
	DrawPixelInfo dpi{pixels.data(), 0, 0, width, height, width, zoom};
	AutoRestoreBackup screen(_screen, DrawPixelInfo{pixels.data(), 0, 0, width, height, width, ZoomLevel::Min});
	AutoRestoreBackup drawing(_cur_dpi, &dpi);
	AutoRestoreBackup animation(_screen_disable_anim, true);
	// Use the exact viewport tile/sprite sorting renderer with a screenshot-style
	// pixel target. No camera selection, vehicle movement or work step is injected.
	ViewportDoDraw(view, view.virtual_left, view.virtual_top,
		view.virtual_left + view.virtual_width, view.virtual_top + view.virtual_height);
	const auto capture_path = RequireTestArtifactWithin(path, FindTestRepositoryRoot() / "mlm/reviews");
	std::ofstream output(capture_path, std::ios::binary);
	REQUIRE(output.good());
	output << "P6\n" << width << " " << height << "\n255\n";
	// Retain every actual palette-resolved screen pixel. PNG conversion later
	// changes only the file container, so clipping and roof overlap stay reviewable.
	for (uint8_t pixel : pixels) {
		const auto &colour = _cur_palette.palette[pixel];
		const std::array<uint8_t, 3> rgb{colour.r, colour.g, colour.b};
		output.write(reinterpret_cast<const char *>(rgb.data()), rgb.size());
	}
	REQUIRE(output.good());
	output.flush();
	RecordNativeTestArtifact(capture_path, __func__);
}

/**
 * Render the actual loaded native mouth picker in every approach orientation.
 * @param layer Explicit source view selected through the existing viewport API.
 * @param target Actual destination choice supplied to the ordinary mouth preview.
 * @param path Repository-owned review PPM output for the four approach buttons.
 * @return None; preserve the main camera and retain exact native RGB pixels.
 */
static void CaptureNativeArtPortalPreviews(MapLayer layer, MapLayer target, const std::filesystem::path &path)
{
	constexpr int width = 640;
	constexpr int height = 160;
	std::vector<uint8_t> pixels(width * height, 0);
	Window *main = GetMainWindow();
	const MapLayer original_layer = main->viewport->layer;
	SetViewportLayer(main, layer);
	DrawPixelInfo dpi{pixels.data(), 0, 0, width, height, width, ZoomLevel::Min};
	AutoRestoreBackup screen(_screen, dpi);
	AutoRestoreBackup drawing(_cur_dpi, &dpi);
	// The real picker draws all four door choices using the selected source/target.
	// No portal tile, saved work phase, calendar gate or company state is edited.
	for (uint8_t view = 0; view < 4; ++view) {
		DrawLayerPortalPreview(80 + view * 160, 80, LayerPortalKind::RailTunnel, static_cast<DiagDirection>(view), target);
	}
	SetViewportLayer(main, original_layer);
	const auto capture_path = RequireTestArtifactWithin(path, FindTestRepositoryRoot() / "mlm/reviews");
	std::ofstream output(capture_path, std::ios::binary);
	REQUIRE(output.good());
	output << "P6\n" << width << " " << height << "\n255\n";
	// Preserve actual palette-resolved picker pixels so roof/mouth clipping can
	// be inspected beside the independently rendered world-layer scenes.
	for (uint8_t pixel : pixels) {
		const auto &colour = _cur_palette.palette[pixel];
		const std::array<uint8_t, 3> rgb{colour.r, colour.g, colour.b};
		output.write(reinterpret_cast<const char *>(rgb.data()), rgb.size());
	}
	REQUIRE(output.good());
	output.flush();
	RecordNativeTestArtifact(capture_path, __func__);
}

/**
 * Render actual UI drawing into an isolated indexed screen and retain its exact RGB pixels.
 * @param width Actual review screen width in pixels.
 * @param height Actual review screen height in pixels.
 * @param path Repository-owned disposable PPM output.
 * @param paint Real widget/engine/cargo renderer; it must not advance simulation owners.
 * @return None; restore the caller's screen/drawing target after the complete raster is written.
 */
static void CaptureNativeArtCanvas(int width, int height, const std::filesystem::path &path, const std::function<void()> &paint)
{
	REQUIRE(width > 0);
	REQUIRE(height > 0);
	REQUIRE(width <= 4096);
	REQUIRE(height <= 4096);
	std::vector<uint8_t> pixels(static_cast<size_t>(width) * height, 0);
	DrawPixelInfo dpi{pixels.data(), 0, 0, width, height, width, ZoomLevel::Min};
	AutoRestoreBackup screen(_screen, dpi);
	AutoRestoreBackup drawing(_cur_dpi, &dpi);
	AutoRestoreBackup animation(_screen_disable_anim, true);
	paint();
	const auto capture_path = RequireTestArtifactWithin(path, FindTestRepositoryRoot() / "mlm/reviews");
	std::ofstream output(capture_path, std::ios::binary);
	REQUIRE(output.good());
	output << "P6\n" << width << " " << height << "\n255\n";
	// Preserve every actual palette-resolved pixel; later conversion changes
	// only the container, keeping real text/icon placement and clipping visible.
	for (uint8_t pixel : pixels) {
		const auto &colour = _cur_palette.palette[pixel];
		const std::array<uint8_t, 3> rgb{colour.r, colour.g, colour.b};
		output.write(reinterpret_cast<const char *>(rgb.data()), rgb.size());
	}
	REQUIRE(output.good());
	output.flush();
	RecordNativeTestArtifact(capture_path, __func__);
}

/**
 * Capture one actual window with its existing widgets and current availability queries.
 * @param window Actual native window, temporarily positioned at the review canvas origin.
 * @param path Repository-owned disposable PPM output.
 * @return None; restore its normal position without creating commands or simulation ticks.
 */
static void CaptureNativeArtWindow(Window &window, const std::filesystem::path &path)
{
	AutoRestoreBackup left(window.left, 0);
	AutoRestoreBackup top(window.top, 0);
	/** Draw the real widget tree and its live read-only owner queries. */
	CaptureNativeArtCanvas(window.width, window.height, path, [&] { window.OnPaint(); });
}

#endif /* TESTS_NATIVE_ART_RENDER_FIXTURE_H */
