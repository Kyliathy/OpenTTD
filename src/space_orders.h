/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_orders.h Native typed Space order ownership and physical work admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Groups6/11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group3.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group14; paid cargo descent uses an explicit center's circular region.
 */
#ifndef SPACE_ORDERS_H
#define SPACE_ORDERS_H
#include "space.h"
#include "vehicle_type.h"
#include <vector>
#include <span>

struct SpaceVehicle;
inline constexpr size_t MAX_SPACE_ORDERS = 32;
/** Append-only native action identities; unavailable feature owners retain explicit waits. */
enum class SpaceOrderAction : uint8_t { Visit, Survey, Mine, Construct, Unload, ReturnService, End };
/** Target domains remain distinct even when their numeric IDs coincide. */
enum class SpaceOrderTargetKind : uint8_t { Body, Structure, Blueprint, LaunchCenter, Craft, End };
/** Saved explicit operational waits; presentation never invents progress to clear one. */
enum class SpaceOrderBlock : uint8_t { None, Completed, TargetRemoved, ProducerUnavailable, HoldIncompatible, Timeout, ReturnsPending, ServiceRequired, PodsMissing, MaterialsMissing, BuilderBusy, ApproachBusy, End };
/** Synchronized bounded queue edits applied to an exact paid native craft incarnation. */
enum class SpaceOrderEdit : uint8_t { Insert, Replace, Erase, Clear, MoveUp, MoveDown, End };
/** Generation-qualified target; natural bodies use their monotonic body domain. */
struct SpaceOrderTarget {
	SpaceOrderTargetKind kind = SpaceOrderTargetKind::Body;
	uint32_t id = UINT32_MAX;
	uint64_t generation = 0; ///< Bodies are monotonic and use zero; reusable pool identities require nonzero generations.
	/** @param other Typed target to compare. @return Whether domain, ID and incarnation agree. */
	bool operator==(const SpaceOrderTarget &other) const = default;
};
/** One requested native action and its physical owner reference. */
struct SpaceOrder {
	SpaceOrderAction action = SpaceOrderAction::Visit;
	SpaceOrderTarget target;
	/** @param other Order to compare. @return Whether action and exact typed target agree. */
	bool operator==(const SpaceOrder &other) const = default;
};
/** One saved request owned by the asking native hull; no second vehicle or berth registry.
 * Structure and Blueprint navigation share the same physical Structure domain.
 * A waiting request holds no slot. Admission retains its slot until explicit
 * release, target loss or departure; rendering cannot resolve the queue.
 */
struct SpaceApproachRequest {
	SpaceOrderTarget target; ///< Body or normalized generation-qualified Structure.
	uint64_t request_tick = 0; ///< Synchronized arrival in the arbitration queue.
	uint8_t slot = UINT8_MAX; ///< Waiting sentinel, otherwise the actual bounded target slot.
	SpacePoint staging; ///< Saved safe waiting destination; movement remains ordinary native pursuit.
	/** @param other Request to compare. @return Whether saved target, priority, slot and staging agree. */
	bool operator==(const SpaceApproachRequest &other) const = default;
};
/** Owned directly by one native Vehicle. No shared-order or clone commissioning path. */
struct SpaceOrderQueue {
	std::vector<SpaceOrder> entries;
	uint8_t cursor = 0;
	bool started = false;
	SpaceOrderBlock blocked = SpaceOrderBlock::None;
	uint32_t timer_ticks = 0;
	uint16_t work_ticks = 0; ///< Reserved for action-owned partial work; unavailable producers may not accrue it.
	std::optional<SpaceApproachRequest> approach; ///< Sole saved approach request for this actual hull.
	/** @param other Queue to compare. @return Whether every instruction, wait and action-owned timer agrees. */
	bool operator==(const SpaceOrderQueue &other) const = default;
};
/** Detached native queue transport; loading it cannot commission a hull or create a target. */
struct SpaceOrderRecord {
	VehicleID vehicle = VehicleID::Invalid();
	uint64_t generation = 0;
	SpaceOrderQueue queue;
};
/** @param model Canonical native hull model. @param order Typed requested action. @return Whether that role can perform the bounded action. */
bool IsSpaceOrderAllowed(uint16_t model, const SpaceOrder &order);
/** @param records Copied queues. @param next Saved next craft incarnation. @return Whether identities, timers and action-specific waits are bounded and consistent. */
bool ValidateSpaceOrderRecords(std::span<const SpaceOrderRecord> records, uint64_t next);
/** @return Copies of actual native queues and exact craft incarnations, without any work or target allocation. */
std::vector<SpaceOrderRecord> CaptureSpaceOrders();
/** @param records Validated copied queues. @param next Saved next incarnation. @return Whether all actual native hulls admitted installation before any queue changed. */
bool InstallLoadedSpaceOrders(std::span<const SpaceOrderRecord> records, uint64_t next);
/** @return Whether copied bounds also agree with actual paid hulls, owners, target generations and mining leases. */
bool ValidateNativeSpaceOrders();
/** @param vehicle Newly commissioned native hull. @return Whether one never-reused craft incarnation was issued. */
bool RegisterSpaceVehicleIdentity(SpaceVehicle &vehicle);
/** @return Next monotonic native craft incarnation; numeric vehicle pool IDs alone never identify a target. */
uint64_t GetNextSpaceVehicleGeneration();
/** @return Deterministic field-wise digest of actual typed queues, waits and action progress. */
uint64_t ComputeSpaceOrdersDigest();
/** Reset issuance and pending queue transport during world teardown. */
void ResetSpaceOrders();
/** Discard unpublished queue transport without changing a native hull. */
void ResetSpaceOrderLoadStaging();
/** @return Whether pending copied queues installed through their actual native hull owners. */
bool AcceptLoadedSpaceOrders();
/** Validate copied queues against this file's profile/native hull catalogue without changing the open world. */
void ValidateSpaceOrderLoadPreview();
/** Execute existing feature owners, including paid-pod center Unload at the actual current in-circle point.
 * An unavailable shell, quota or receiver keeps cargo aboard without granting service.
 * @param vehicle Actual paid orbital hull with its sole native queue and packet containers.
 * @return True when its queue handled this tick; false delegates Mine to the existing mining owner.
 */
bool TickSpaceOrders(SpaceVehicle &vehicle);
/** @param vehicle Actual hull whose queue and existing mining lease must be explicitly released. */
void CancelSpaceOrders(SpaceVehicle &vehicle);
/** @param vehicle Actual survey hull. @return Shared physical scan admission used by both work and upkeep. */
bool IsSpaceSurveyWorkTick(const SpaceVehicle &vehicle);
/** Resolve only actual producer coordinates for native orders and rocket delivery.
 * @param vehicle Asking native hull/owner. @param target Exact typed physical target. @param point Receives its current authoritative approach.
 * @return Explicit wait/refusal, or None with a physical position; allocates no proxy target.
 */
SpaceOrderBlock ResolveSpaceNavigationTarget(const SpaceVehicle &vehicle, const SpaceOrderTarget &target, SpacePoint &point);
/** @param vehicle Actual mining hull. @return Its real paid service/pod/return wait; retains bounded legacy behavior without fabricating supplies. */
SpaceOrderBlock GetSpaceMiningBlock(const SpaceVehicle &vehicle);
/** @param queue Native queue. @return Whether its current unstarted Mine is deliberately held for accountable return/service inventory. */
bool IsSpaceMiningServiceWait(const SpaceOrderQueue &queue);
/** @param vehicle Actual mining hull. @return Shared physical lease/reserve/hold admission used by mining work and upkeep. */
bool IsSpaceMiningWorkTick(const SpaceVehicle &vehicle);
#endif
