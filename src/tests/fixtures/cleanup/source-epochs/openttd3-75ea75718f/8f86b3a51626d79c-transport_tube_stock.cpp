/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_stock.cpp Reuse canonical finite station capacity for every mode sharing a tube interchange.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "stdafx.h"
#include "transport_tube_stock.h"
#include "transport_tube.h"
#include "station_base.h"
#include "station_map.h"
#include "station_supply.h"
#include "rail_map.h"
#include "safeguards.h"

/** @copydoc GetTubeInterchangeTransferRoom */
std::optional<uint32_t> GetTubeInterchangeTransferRoom(StationID station, CargoType cargo, const StationCargoList *destination)
{
	const Station *owner = Station::GetIfValid(station);
	// Only the exact actual queue can borrow this admission rule. Invalid IDs,
	// unrelated lists and ordinary stations acquire no replacement stock owner.
	if (owner == nullptr || cargo >= NUM_CARGO || !owner->goods[cargo].HasData() ||
		&owner->goods[cargo].GetData().cargo != destination) return std::nullopt;
	// Inspect only the native bounded station rectangle. A joined road/ordinary
	// rail berth shares the same queue bound when an actual tube platform exists;
	// adjacent independent stations and same-XY cells on other layers do not.
	for (TileIndex tile : owner->train_station) {
		// Rectangular station extents can contain gaps and foreign facilities;
		// exact installed membership precedes the semantic railtype check.
		if (!IsRailStationTile(tile) || GetStationIndex(tile) != station || !IsTubeRailType(GetRailType(tile))) continue;
		const uint32_t waiting = owner->goods[cargo].TotalCount();
		return waiting >= EXACT_STATION_SUPPLY_CAPACITY ? 0 : EXACT_STATION_SUPPLY_CAPACITY - waiting;
	}
	return std::nullopt;
}
