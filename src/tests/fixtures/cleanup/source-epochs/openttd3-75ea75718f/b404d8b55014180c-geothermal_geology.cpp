/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geothermal_geology.cpp Natural thermal geometry/capacity, separate from plant and allocation behavior. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 6 */

#include "stdafx.h"
#include "excavation.h"
#include "multilayer_utility.h"
#include "tile_map.h"

#include "safeguards.h"

/**
 * Validate natural source identities and physical capacity without discarding overlapping minerals.
 * @param fields Canonical bounded owner records.
 * @param saved_next_id Monotonic natural ID cursor.
 * @return Structural/rate refusal or success; no thermal allocation or world mutation occurs.
 */
GeologyError ValidateGeothermalVentFields(std::span<const GeothermalVentField> fields, uint32_t saved_next_id)
{
	if (fields.size() > MAX_GEOTHERMAL_VENT_FIELDS) return GeologyError::Limit;
	uint32_t minimum_id = 0;
	// Each actual natural source has its own identity/rate; geometry overlap cannot manufacture or erase a source.
	for (const auto &field : fields) {
		// Increasing IDs may contain deleted holes but cannot recycle or alias retained natural heat.
		if (field.id.base() < minimum_id || field.id.base() >= saved_next_id) return GeologyError::InvalidID;
		minimum_id = field.id.base() + 1;
		if (field.layer != MapLayer::DeepUnderground) return GeologyError::InvalidShape;
		if (!MultiLayerMap::IsValidLayer(field.layer)) return GeologyError::InvalidShape;
		if (field.generation_version != 1) return GeologyError::InvalidShape;
		// Sources share the utility owner's one-account rate ceiling; zero or unbounded capacity is not useful natural heat.
		if (field.thermal_capacity_w == 0 || field.thermal_capacity_w > MAX_UTILITY_RATE_W) return GeologyError::InvalidReserve;
		// Empty/oversized rectangles cannot define a stable collector footprint or safe bounded lookup.
		if (field.width == 0 || field.height == 0 || field.width > 16 || field.height > 16) return GeologyError::InvalidShape;
		// Full source shape stays inside the void fence; its cells remain normally excavatable rather than becoming holes.
		if (field.x + static_cast<uint32_t>(field.width) - 1 >= Map::MaxX() || field.y + static_cast<uint32_t>(field.height) - 1 >= Map::MaxY()) return GeologyError::InvalidShape;
		for (uint y = field.y; y < field.y + static_cast<uint32_t>(field.height); ++y) {
			for (uint x = field.x; x < field.x + static_cast<uint32_t>(field.width); ++x) {
				if (!IsValidTile(TileXYL(x, y, field.layer))) return GeologyError::InvalidShape;
			}
		}
	}
	return GeologyError::None;
}

/**
 * Select an actual field for planning; operational consumers must retain their explicit chosen source ID.
 * @param state Accepted/prepared geological owner, not an independent utility registry.
 * @param tile Explicit world tile.
 * @return Stable first overlap or nullptr; no capacity is summed by proximity.
 */
const GeothermalVentField *FindGeothermalVentField(const GeologyWorldState &state, TileIndex tile)
{
	if (!IsValidTile(tile)) return nullptr;
	if (GetTileLayer(tile) != MapLayer::DeepUnderground) return nullptr;
	const uint x = TileX(tile), y = TilePlaneY(tile);
	// Canonical bounded IDs resolve overlapping influence deterministically, independently of a mineral seam at the same tile.
	for (const auto &field : state.vents) {
		// Both axes must be inside the exact source rectangle; coincident Surface positions are excluded above.
		if (x >= field.x && y >= field.y && x < field.x + static_cast<uint32_t>(field.width) && y < field.y + static_cast<uint32_t>(field.height)) return &field;
	}
	return nullptr;
}
