/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_merged_wire.h Explicit441tagged metadata; original A440and B tables retain their independent formats.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md
 */
#ifndef SURFACE_SUPPLY_MERGED_WIRE_H
#define SURFACE_SUPPLY_MERGED_WIRE_H
#include "surface_supply_order_wire.h"

inline constexpr uint32_t MERGED_SURFACE_SUPPLY_MAGIC = 0x53534D31; ///< Network-order SSM1; cannot resemble the original A440cursor header.
inline constexpr uint16_t MERGED_SURFACE_SUPPLY_SCHEMA = 1; ///< Closed Native1/LegacyB2receipt contract.
inline constexpr size_t MERGED_SURFACE_SUPPLY_HEADER_SIZE = 20; ///< U32magic, U16schema, U64frontier, I32economy day, U16count.
inline constexpr size_t MERGED_SURFACE_SUPPLY_NATIVE_SIZE = 54; ///< U8tag,21common bytes and32original native evidence bytes.
inline constexpr size_t MERGED_SURFACE_SUPPLY_LEGACY_SIZE = 39; ///< U8tag,21common bytes and17original B evidence bytes.
inline constexpr size_t MAX_MERGED_SURFACE_SUPPLY_WIRE_SIZE = MERGED_SURFACE_SUPPLY_HEADER_SIZE + MAX_LEGACY_SURFACE_SUPPLY_RECORDS * MERGED_SURFACE_SUPPLY_NATIVE_SIZE;

/** Bounded streaming facts only; no record collection, packet or gameplay owner exists during inspection. */
struct MergedSurfaceSupplyWireSummary {
	uint64_t next_id = 0; ///< Exact original canonical issuer frontier, retaining pruned terminal history.
	TimerGameEconomy::Date saved_day{0}; ///< Independent family clock that must later join native DATE.
	uint16_t records = 0; ///< Exact original tagged row count.
};

/** Encode the sole already-admitted canonical book with explicit provenance tags.
 * @param transport Original native/legacy receipts and exact saved economy day; no quantity or tariff is reconstructed.
 * @return Bounded network-order SSM1bytes, or no value for malformed tags/scalars/identity order/counts.
 */
std::optional<std::vector<uint8_t>> EncodeMergedSurfaceSupplyOrders(const SurfaceSupplyOrderTransport &transport);
/** Decode only441tagged metadata, independently of every original fixed/table codec.
 * @param bytes Exact admitted SSORpayload; trailing/truncated bytes cannot supply another receipt.
 * @return Detached canonical receipts, or no candidate for malformed schema/tags/scalars/counts; full ownership/custody joins remain mandatory.
 */
std::optional<SurfaceSupplyOrderTransport> DecodeMergedSurfaceSupplyOrders(std::span<const uint8_t> bytes);
/** Inspect the same pure tagged decoder without materializing payload bytes or a record vector.
 * @param length Exact original SSORextent, bounded before any field reads.
 * @param read Read one network-order unsigned scalar of the supplied1/2/4/8-byte width from this bounded detached stream; may throw the caller's detached framing error.
 * @return Exact header facts only after all tagged rows and the complete extent validate; no world/pool handler is invoked.
 */
std::optional<MergedSurfaceSupplyWireSummary> InspectMergedSurfaceSupplyOrders(size_t length, const std::function<uint64_t(size_t)> &read);
#endif /* SURFACE_SUPPLY_MERGED_WIRE_H */
