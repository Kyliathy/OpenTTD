/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file map_layer_type.h Types for the stacked map layers. @see docs/zz-reach2/upgrades/2026-10/r2mlm12-deep-underground-tunnels-and-elevators.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#ifndef MAP_LAYER_TYPE_H
#define MAP_LAYER_TYPE_H

#include "core/enum_type.hpp"

/**
 * One horizontal map in the stacked world.
 *
 * The numeric values are the stacking order. They are implicit in every saved
 * TileIndex once layers exist, so they must never be reordered or renumbered.
 */
enum class MapLayer : uint8_t {
	Surface = 0, ///< The land map the game has always had.
	Underground = 1, ///< The map below the surface.
	Space = 2, ///< The map above the surface.
	DeepUnderground = 3, ///< The map below Underground. Appended after Space so saved TileIndex values keep their meaning.
	End, ///< Number of layers. Must stay last among the real layers.
	Invalid = 0xFF, ///< Not a layer.
};

/** How many real layers exist: four since r2mlm12 D-1 (AD-1 as amended). */
static constexpr uint MAX_MAP_LAYERS = to_underlying(MapLayer::End);
static_assert(MAX_MAP_LAYERS == 4);

#endif /* MAP_LAYER_TYPE_H */
