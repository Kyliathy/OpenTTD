/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_approach.h Shared native approach arbitration and saved request ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group4B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11.
 */
#ifndef SPACE_APPROACH_H
#define SPACE_APPROACH_H
#include "space_orders.h"

struct SpaceVehicle;
struct SpaceStructure;
struct SpaceStructureState;
/** Detached request for pure arbitration; the native hull retains the actual saved owner. */
struct SpaceApproachRecord {
	VehicleID craft = VehicleID::Invalid(); ///< Native hull pool identity.
	uint64_t generation = 0; ///< Exact hull incarnation.
	SpaceApproachRequest request;
};
/** Derived physical slot, including pre-existing occupancy that admission may never push aside. */
struct SpaceApproachSlot {
	SpaceOrderTarget target;
	uint8_t slot = 0;
	SpacePoint point;
	VehicleID occupant = VehicleID::Invalid(); ///< Actual physical hull or existing body lease owner.
	bool blocked = false; ///< Multiple/foreign occupancy forbids admission without moving either participant.
};
/** Pure stable admission, preserving surviving reservations before considering waiting requests.
 * @param requests Copied bounded hull requests, in any input order.
 * @param slots Derived local target slots and their actual occupancy.
 * @return Copied requests with deterministic slots, or absence for ambiguous/duplicate input ownership.
 */
std::optional<std::vector<SpaceApproachRecord>> PlanSpaceApproaches(std::span<const SpaceApproachRecord> requests, std::span<const SpaceApproachSlot> slots);
/** Validate copied saved requests without borrowing live hulls or targets.
 * @param records Same-file native queues. @param native_tick Same-file clock, or UINT64_MAX for structural-only checks.
 * @param width Plane width, or zero when geometry is not yet available. @param height Plane height.
 * @return Whether priority, target identities, staging and exclusive slot ownership are bounded.
 */
bool ValidateSpaceApproachRecords(std::span<const SpaceOrderRecord> records, uint64_t native_tick = UINT64_MAX, uint32_t width = 0, uint32_t height = 0);
/** Request a real target without acquiring a slot during estimate.
 * @param craft Actual deployed owned hull or exact in-Space module-freight rocket. @param target Body or Structure/Blueprint destination.
 * @param execute False validates only; true saves one request without resolving simultaneous arrivals.
 * @return Whether this exact physical target can be requested without disturbing a retained service job.
 */
bool RequestSpaceApproach(SpaceVehicle &craft, SpaceOrderTarget target, bool execute);
/** Query immediate shared-slot eligibility without saving a request or preempting an existing owner.
 * @param craft Exact deployed native hull considering an automatic destination.
 * @param target Original body or generation-qualified static destination.
 * @return Whether the original native arbitration would admit this hull now, including physical occupancy and earlier requests. Later assignment must revalidate; this grants no lease or reservation.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group18.
 */
bool CanAcquireSpaceApproach(const SpaceVehicle &craft, SpaceOrderTarget target);
/** Release only this native hull's request; physical position, cargo and service parts remain unchanged.
 * @param craft Exact actual hull whose request is released.
 */
void ReleaseSpaceApproach(SpaceVehicle &craft);
/** Resolve all requests together once before native movement; target loss releases only affected reservations. */
void PrepareSpaceApproachTick();
/** Read the current admitted approach or saved waiting destination without granting work.
 * @param craft Actual requesting hull. @param point Receives the admitted slot or safe staging destination.
 * @return None for an admitted slot, ApproachBusy while queued, or TargetRemoved for a lost producer.
 */
SpaceOrderBlock GetSpaceApproachDestination(const SpaceVehicle &craft, SpacePoint &point);
/** @param site Native site. @return Paid physical approach capacity; activation is required for Dock's second berth, and later power/dependency loss retains its accountable occupancy. */
uint8_t GetSpaceStructureApproachCapacity(const SpaceStructure &site);
/** @param site Native site. @param slot Actual slot index. @return Its precise plane approach, or absence outside capacity. */
std::optional<SpacePoint> GetSpaceStructureApproachSlot(const SpaceStructure &site, uint8_t slot);
/** @param craft Exact hull. @param site Actual site. @return Its admitted physical slot only when it is actually there. */
std::optional<uint8_t> GetSpaceStructureBerth(const SpaceVehicle &craft, const SpaceStructure &site);
/** @return Whether native saved requests agree with actual owned targets, clocks and physical occupancy. */
bool ValidateNativeSpaceApproaches();
/** @param state Candidate static footprint. @return Whether actual hull/staging occupancy leaves its cells and approaches free. */
bool IsSpaceApproachSiteAvailable(const SpaceStructureState &state);
/** Check the actual Assembly slot before a paid request or whole input reservation.
 * @param site Exact native Assembly; its own saved manufacturing request is not a fictitious Vehicle occupant.
 * @return Whether physical footprint/approach and every previously admitted hull leave its one manufacturing berth free.
 */
bool IsSpaceStructureManufactureBerthFree(const SpaceStructure &site);
/** Bind only the freshly completed job's actual hull to its already preflighted native physical output berth.
 * @param craft Original newly issued native hull, physically at its factory output point.
 * @param site Actual completed Assembly whose exact paid job owns this product.
 * @return Whether the existing approach authority committed its one original berth; creates no proxy hull or cargo.
 */
bool BindSpaceManufactureBerth(SpaceVehicle &craft, const SpaceStructure &site);
#endif
