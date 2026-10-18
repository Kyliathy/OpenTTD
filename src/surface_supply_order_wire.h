/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order_wire.h Bounded original order metadata transport; decoding grants no native stock, finance or owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCC.
 */
#ifndef SURFACE_SUPPLY_ORDER_WIRE_H
#define SURFACE_SUPPLY_ORDER_WIRE_H

#include "surface_supply_order.h"

static constexpr size_t SURFACE_SUPPLY_WIRE_HEADER_SIZE = 14; ///< uint64 next ID, int32 native economy day, uint16 record count.
static constexpr size_t SURFACE_SUPPLY_WIRE_RECORD_SIZE = 53; ///< Fixed original scalar record; physical packets remain in CAPA/native containers.
static constexpr size_t MAX_SURFACE_SUPPLY_WIRE_SIZE = SURFACE_SUPPLY_WIRE_HEADER_SIZE + MAX_SURFACE_SUPPLY_RECORDS * SURFACE_SUPPLY_WIRE_RECORD_SIZE;

/** Same-file original family and clock stamp; adoption requires independent DATE/MAPS/PLYR/STNN evidence. */
struct SurfaceSupplyOrderTransport {
	SurfaceSupplyOrderSnapshot snapshot; ///< Detached original metadata, never another packet inventory.
	TimerGameEconomy::Date saved_day{0}; ///< Exact native economy date at original serialization, not the calendar date.
};

/** Encode complete already-admitted metadata without allocating stock/identities or touching financial owners.
 * @param transport Original native book and exact native economy-day stamp.
 * @return Canonical fixed little-endian bytes, or no value for oversized/identity-less/negative-charge transport; semantic owner validation belongs to the original full/copy joins.
 */
std::optional<std::vector<uint8_t>> EncodeSurfaceSupplyOrders(const SurfaceSupplyOrderTransport &transport);

/** Decode one bounded exact family before any vector allocation or native adoption.
 * @param bytes Entire same-file SSOR payload; no current-world data may fill truncation/trailing bytes.
 * @return Detached original values, or no value for malformed length/count/unsigned-money overflow; record/reference validation follows through the shared model.
 */
std::optional<SurfaceSupplyOrderTransport> DecodeSurfaceSupplyOrders(std::span<const uint8_t> bytes);

#endif /* SURFACE_SUPPLY_ORDER_WIRE_H */
