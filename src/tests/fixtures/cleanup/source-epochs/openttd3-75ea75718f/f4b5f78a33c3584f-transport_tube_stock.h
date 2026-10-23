/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_stock.h Pure admission for the existing native interchange packet queue.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef TRANSPORT_TUBE_STOCK_H
#define TRANSPORT_TUBE_STOCK_H

#include "station_type.h"
#include "cargo_type.h"
#include <optional>

/** Native station packet list; this feature borrows its stock and reservations without another buffer. */
class StationCargoList;

/**
 * Inspect remaining native finite-supply queue capacity at an actual tube interchange.
 * @param station Native destination from the current unloading payment, independent of the carrier's mode.
 * @param cargo Actual cargo whose station queue receives transferred packets.
 * @param destination Exact native queue used by the unloading owner; mismatched lists are outside this contract.
 * @return Remaining whole-unit room including outstanding loading reservations, or no limit for other stations/lists.
 * Existing over-limit stock is retained with zero room. This allocates no packets, changes no stock and grants no income.
 */
std::optional<uint32_t> GetTubeInterchangeTransferRoom(StationID station, CargoType cargo, const StationCargoList *destination);

#endif /* TRANSPORT_TUBE_STOCK_H */
