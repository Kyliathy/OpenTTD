/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file conventional_terminal_art.cpp Preserve real default road-stop parents before raster authoring.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups25B/25.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "stdafx.h"
#include "conventional_terminal_art.h"
#include "station_func.h"
#include "station_map.h"
#include "multilayer_content.h"

/** @copydoc GetConventionalRoadTerminalArtGeometry */
std::optional<ConventionalRoadTerminalArtGeometry> GetConventionalRoadTerminalArtGeometry(uint8_t graphics)
{
	if (graphics >= 6) return std::nullopt; // The renderer's axis fallback cannot supply another genuinely authored native view.
	ConventionalRoadTerminalArtGeometry result;
	result.graphics = graphics;
	result.drive_through = graphics >= GFX_TRUCK_BUS_DRIVETHROUGH_OFFSET;
	if (result.drive_through) {
		result.axis = static_cast<Axis>(graphics - GFX_TRUCK_BUS_DRIVETHROUGH_OFFSET);
	} else {
		result.entrance = static_cast<DiagDirection>(graphics);
	}
	size_t parent = 0;
	// Native bay and through layouts have different independent sorting owners.
	// Preserve their actual sequence, refusing children or extra parents instead
	// of collapsing a U-shaped bay into a guessed two-part building silhouette.
	for (const auto &piece : GetStationTileLayout(StationType::Truck, graphics)->GetSequence()) {
		if (!piece.IsParentSprite() || parent >= result.parents.size()) return std::nullopt;
		result.parents[parent++] = static_cast<const SpriteBounds &>(piece);
	}
	if (parent != (result.drive_through ? 2U : 3U)) return std::nullopt;
	return result;
}

/** @copydoc GetConventionalRoadTerminalTileArtGeometry */
std::optional<ConventionalRoadTerminalArtGeometry> GetConventionalRoadTerminalTileArtGeometry(TileIndex tile)
{
	// Numeric graphics overlap among station families and profiles. Only the
	// ordinary first-party freight terminal in excavated rock layers belongs to
	// this source family; buses, custom stops and Surface/Classic keep their art.
	if (!IsValidTile(tile) || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	const auto layer = GetTileLayer(tile);
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return std::nullopt;
	if (!IsTileType(tile, TileType::Station) || !IsTruckStop(tile) || IsCustomRoadStopSpecIndex(tile)) return std::nullopt;
	return GetConventionalRoadTerminalArtGeometry(GetStationGfx(tile));
}
