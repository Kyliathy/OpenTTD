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

/** Exact B table scalars before semantic conversion; invalid defaults expose omitted paid-history values. */
struct LegacyBSupplyOrderWire {
	uint32_t id = 0; ///< Original dense positive identity; conversion widens without remapping.
	uint8_t owner = UINT8_MAX; ///< Original current company or explicit loss sentinel, never an invented purchaser.
	uint16_t station = UINT16_MAX; ///< Original source or explicit source-loss sentinel.
	CargoLabel label{}; ///< Original canonical four-byte material, independent of cargo-slot configuration.
	int32_t economy_day = INT32_MIN, calendar_date = INT32_MIN; ///< Separate original allowance and introduction-date facts.
	uint16_t quantity = 0, refunded = UINT16_MAX, dispatched = UINT16_MAX; ///< Whole paid amount and irreversible historical counters.
	int64_t unit_price = 0; ///< Frozen original tariff; no current-price or Q16 reconstruction is permitted.
	uint8_t closed = 2; ///< Only the original0/1 encoding is valid; do not coerce other values to true.
	/** @param other Original complete scalar row. @return Exact unchanged-input equality without native lookup. */
	bool operator==(const LegacyBSupplyOrderWire &other) const = default;
};

/** B's complete same-file SSOH/SSOR scalar transport; no instance is a live order owner or stock store. */
struct LegacyBSupplyTransport {
	uint32_t schema = 0; ///< Original header schema1; the default cannot manufacture a valid absent family.
	uint32_t count = UINT32_MAX; ///< Original declared row count, checked against the complete table before allocation.
	uint32_t next_order = 0; ///< Original never-issued frontier, exactly count+1 for B's retained dense history.
	std::vector<LegacyBSupplyOrderWire> orders; ///< Original table ordinal order; wire framing/column presence are checked by the caller.
	/** @param other Original detached family. @return Exact input equality, including declared bounds. */
	bool operator==(const LegacyBSupplyTransport &other) const = default;
};

/** Convert B's already framed complete family into the sole canonical detached receipt representation.
 * @param original Same-file header/table scalars; caller must establish both chunk presences, kinds and exact required columns first.
 * @param same_file Candidate-owned map/profile/company/station facts; never references borrowed from the active world.
 * @return Whole validated legacy snapshot with its original frontier, or no candidate. No packet, money, cap or live history changes.
 * This checks original schema/count/dense IDs/closed encoding before preserving every scalar and running shared receipt validation.
 * Cargo custody joins and publication remain separate mandatory afterload gates; a successful conversion grants neither.
 */
std::optional<SurfaceSupplyOrderSnapshot> ConvertLegacyBSupply(const LegacyBSupplyTransport &original, const SurfaceSupplyRecordContext &same_file);

/** Project a decoded original A440 family to native evidence without changing its price, identity or Pending rights.
 * @param original Exact A440 decoded transport; the caller retains saved_day for the independent same-file DATE join.
 * @param same_file Candidate-owned map/profile/company/station facts, never current-world defaults.
 * @return Whole validated native snapshot or no candidate; no installed book, stock, settlement or serialization version changes.
 * This is not a mixed-format decoder: legacy/unknown variants refuse, and full/copy loaders still own framing, clock and custody joins.
 */
std::optional<SurfaceSupplyOrderSnapshot> ConvertLegacyASupply(const SurfaceSupplyOrderTransport &original, const SurfaceSupplyRecordContext &same_file);

#endif /* SURFACE_SUPPLY_ORDER_WIRE_H */
