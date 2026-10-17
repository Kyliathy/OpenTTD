/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_clone.h Bounded copied configuration for one new material-paid native Space product.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BB
 */
#ifndef SPACE_CLONE_H
#define SPACE_CLONE_H
#include "space_purchase.h"
#include "space_orders.h"

/** Immutable request intent; the original incarnation is historical provenance, never a live cargo, service or order-sharing dependency. */
struct SpaceCloneIntent {
	SpaceCraftRef source; ///< Original real paid hull, retained even after its removal or numeric ID reuse.
	std::string name; ///< Original custom name; final uniqueness is prepared against the current actual fleet.
	std::vector<SpaceOrder> orders; ///< Instructions only: no cursor, partial work, mining lease or approach reservation.
};

inline constexpr size_t MAX_SPACE_CLONE_NAME_BYTES = (MAX_LENGTH_VEHICLE_NAME_CHARS - 1) * 4; ///< Bounded UTF-8 storage for an existing legal custom name.

/** Prepared storage moved into an actual newly paid product without allocating or copying any source assets during settlement. */
struct SpaceCloneProductConfiguration {
	std::string name; ///< New unique name prepared before hull allocation/material consumption.
	std::vector<SpaceOrder> orders; ///< Independent unstarted instructions, never a shared mutable queue.
};

/** Validate detached clone intent against this snapshot's issuance and immutable model policy; resolve no open-world owner.
 * @param intent Captured original identity, custom name and instructions.
 * @param model Actual requested canonical recipe role.
 * @param next_vehicle Same-file next native craft incarnation; removed original hulls remain valid historical sources.
 * @param next_structure Same-file next static incarnation, bounding copied module/blueprint instructions.
 * @return Whether every bounded field could have been captured from an already issued native source.
 */
bool ValidateSpaceCloneIntent(const SpaceCloneIntent &intent, uint16_t model, uint64_t next_vehicle, uint64_t next_structure);
/** Preflight current target rights without allocating a hull, acquiring an approach or mutating the original source.
 * @param intent Frozen instructions; the historical source is never re-resolved.
 * @param owner Actual current factory/product owner.
 * @return Whether surviving explicit targets retain compatible ownership; missing targets remain inert instructions.
 */
bool CanApplySpaceCloneIntent(const SpaceCloneIntent &intent, CompanyID owner);
/** Prepare all copied instruction/name storage before any physical product or final input commit.
 * @param intent Frozen valid request configuration.
 * @param owner Actual current factory/product owner.
 * @return Independent prepared configuration, or no result while a surviving target is foreign.
 */
std::optional<SpaceCloneProductConfiguration> PrepareSpaceCloneProductConfiguration(const SpaceCloneIntent &intent, CompanyID owner);
/** Move prepared configuration into an actual fresh paid product, preserving its original physical berth and construction receipt.
 * @param product Real newly allocated Earth/orbital hull; never an estimate or proxy.
 * @param configuration Already prepared storage; no packet, service or source action owner is transferred.
 */
void ApplySpaceCloneProductConfiguration(SpaceVehicle &product, SpaceCloneProductConfiguration &&configuration) noexcept;
/** Quote/admit a fresh physical purchase from supported immutable source configuration.
 * @param flags Ordinary clone estimate/NoTest execution; QueryCost remains pure even with Execute.
 * @param tile Actual destination workshop/Assembly world tile.
 * @param source Actual owned paid canonical native hull; its assets/progress remain unchanged.
 * @param share_orders Explicit shared-list request, refused because native Space queues have no shared owner.
 * @return Existing purchase cost and actual Earth hull or one queued factory job. Surface clones require empty instructions;
 *         nonempty native queues belong only to deployed orbital products. New hulls commission canonical default cargo;
 *         the source's dynamically assigned freight label and every actual packet remain with the original cargo owner.
 */
SpacePurchaseResult CloneSpaceVehicleAtFacility(DoCommandFlags flags, TileIndex tile, const SpaceVehicle &source, bool share_orders);
#endif
