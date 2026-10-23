/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file conventional_station_art.cpp Read native station part bounds before dependent raster authoring.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "stdafx.h"
#include "conventional_station_art.h"
#include "station_func.h"
#include "station_map.h"
#include "multilayer_content.h"
#include "transport_tube.h"

/** @copydoc GetConventionalRailStationArtGeometry */
std::optional<ConventionalRailStationArtGeometry> GetConventionalRailStationArtGeometry(uint8_t graphics)
{
	if (graphics >= 8) return std::nullopt; // Do not inherit GetStationTileLayout's axis-only fallback as a genuine source role.
	ConventionalRailStationArtGeometry result;
	result.graphics = graphics;
	result.axis = (graphics & 1) != 0 ? Axis::Y : Axis::X;
	result.role = static_cast<ConventionalRailStationArtRole>(graphics / 2);
	const auto sequence = GetStationTileLayout(StationType::Rail, graphics)->GetSequence();
	size_t parents = 0;
	// Native sequence order owns rear/front/elevated roof sorting. Child glass
	// belongs to that roof, rather than becoming a fourth bounding box or a low
	// wall that could cover a real train. Refuse an unknown native layout change.
	for (const auto &piece : sequence) {
		if (!piece.IsParentSprite()) {
			// This source family supports the actual single glass child attached
			// to the elevated roof. An extra or differently attached child needs
			// an explicit source-contract revision before original art selection.
			if (parents != 3 || result.roof_glass.has_value()) return std::nullopt;
			result.roof_glass = ConventionalRailStationArtChild{{piece.origin.x, piece.origin.y},
				HasBit(piece.image.sprite, PALETTE_MODIFIER_TRANSPARENT), piece.image.pal};
			++result.roof_children;
			continue;
		}
		if (parents >= result.parts.size()) return std::nullopt;
		result.parts[parents++] = static_cast<const SpriteBounds &>(piece);
	}
	const bool canopy = graphics >= 4;
	// Bare/building cells have two low parents; canopy cells add exactly one
	// elevated parent and its native glass child. Partial matches cannot prove
	// complete source coverage after an upstream renderer/layout change.
	if (parents != (canopy ? 3U : 2U) || result.roof_children != (canopy ? 1 : 0)) return std::nullopt;
	return result;
}

/** @copydoc GetConventionalRailStationTileArtGeometry */
std::optional<ConventionalRailStationArtGeometry> GetConventionalRailStationTileArtGeometry(TileIndex tile)
{
	// This first-party family must not replace Classic/Surface scenery, custom
	// stations, tube terminals or other station types merely because a numeric
	// graphics value happens to share the same low axis bit.
	if (!IsValidTile(tile) || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	const auto layer = GetTileLayer(tile);
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return std::nullopt;
	if (!IsRailStationTile(tile) || IsCustomStationSpecIndex(tile) || IsTubeRailType(GetRailType(tile))) return std::nullopt;
	return GetConventionalRailStationArtGeometry(GetStationGfx(tile));
}
