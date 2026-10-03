/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file station_kdtree.h Declarations for accessing the k-d tree of stations. */

#ifndef STATION_KDTREE_H
#define STATION_KDTREE_H

#include "core/kdtree.hpp"
#include "station_base.h"
#include "map_func.h"
#include "map_layer.h"

struct Kdtree_StationXYFunc {
	inline uint16_t operator()(StationID stid, int dim)
	{
		return (dim == 0) ? TileX(BaseStation::Get(stid)->xy) : TileY(BaseStation::Get(stid)->xy);
	}
};

using StationKdtree = Kdtree<StationID, Kdtree_StationXYFunc, uint16_t, int>;
extern StationKdtree _station_kdtree;

/**
 * Call a function on all stations whose sign is within a radius of a center tile.
 * @param center  Central tile to search around.
 * @param radius  Distance in both X and Y to search within.
 * @param func    The function to call, must take a single parameter which is Station*.
 */
template <typename Func>
void ForAllStationsRadius(TileIndex center, uint radius, Func func)
{
	MapLayer layer = GetTileLayer(center);
	int plane_y = static_cast<int>(TilePlaneY(center));
	int y_start = std::max<int>(0, plane_y - static_cast<int>(radius));
	int y_end = std::min<int>(static_cast<int>(Map::SizeY()), plane_y + static_cast<int>(radius) + 1);

	uint16_t x1, y1, x2, y2;
	x1 = (uint16_t)std::max<int>(0, TileX(center) - radius);
	x2 = (uint16_t)std::min<int>(TileX(center) + radius + 1, Map::SizeX());
	//C6: the tree stores stacked Y, so the box is this layer's band, expressed through TileXYL.
	y1 = static_cast<uint16_t>(TileY(TileXYL(0, static_cast<uint>(y_start), layer)));
	if (y_end == static_cast<int>(Map::SizeY())) {
		uint next = to_underlying(layer) + 1;
		//The exclusive end is the next layer's first row, or the end of storage for the last layer.
		y2 = (next < Map::LayerCount())
			? static_cast<uint16_t>(TileY(MultiLayerMap::GetLayerBase(static_cast<MapLayer>(next))))
			: static_cast<uint16_t>(Map::Size() >> Map::LogX());
	} else {
		y2 = static_cast<uint16_t>(TileY(TileXYL(0, static_cast<uint>(y_end), layer)));
	}

	_station_kdtree.FindContained(x1, y1, x2, y2, [&](StationID id) {
		Station *st = Station::Get(id);
		//A station sign in another layer is not inside this search.
		if (GetTileLayer(st->xy) != layer) return;
		func(st);
	});
}

#endif
