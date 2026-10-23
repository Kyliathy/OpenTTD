/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_operations.h Native Station slots, physical zones and paid assembly.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups2/2B/8B.
 */
#ifndef LAUNCH_OPERATIONS_H
#define LAUNCH_OPERATIONS_H
#include "space_production.h"
#include "space_pod_type.h"

/** Disjoint physical zones rotated together inside the immutable center footprint. */
enum class LaunchFacilityZone : uint8_t { Pad, Workshop, Recovery, Storage, End };
inline constexpr size_t MAX_CENTER_CRAFT = 8;
/**
 * Station-owned operational reservations; native Vehicles retain paid hulls and work.
 * Workshop, service and pod assembly share one exclusive workshop. A rocket occupies
 * either the active pad or its bounded FIFO queue. References never own cargo or hulls.
 */
struct LaunchOperations {
	SpaceCraftRef workshop, service; ///< Exclusive assembly/service occupant, or an empty reference.
	SpacePodRef pod_workshop; ///< Paid passive shell being assembled in the same workshop.
	SpaceCraftRef pad_active; ///< Rocket retaining the pad through preparation, flight and turnaround.
	std::vector<SpaceCraftRef> pad_queue; ///< At most MAX_CENTER_CRAFT stored rockets, in dispatch order.
};
/** Copied LCOP transport; restoring references never creates another paid Vehicle. */
struct LaunchOperationsRecord { LaunchCenterRef center; LaunchOperations slots; };
/**
 * Locate one physical zone without changing either service circle.
 * @param anchor Immutable Surface build anchor.
 * @param rotation Quarter-turn layout index, 0..3.
 * @param zone Pad, workshop, recovery or storage zone to locate.
 * @return Surface extent, or no value for an invalid layout, zone or footprint.
 */
std::optional<TileArea> GetLaunchFacilityZone(TileIndex anchor, uint8_t rotation, LaunchFacilityZone zone);
/**
 * Read the precise orbital approach for workshop service.
 * @param center Native Station whose launch anchor supplies the rotated workshop location.
 * @return Layer-local Q16 approach, or no value when the facility/layout is absent.
 */
std::optional<SpacePoint> GetLaunchServiceApproach(const Station &center);
/**
 * Check the quarter-tile circular service tolerance, separately from the drop radius.
 * @param point Current layer-local Q16 spacecraft position.
 * @param center Native Station owning the service approach.
 * @return Whether the point lies within the inclusive precise approach.
 */
bool IsAtLaunchServiceApproach(SpacePoint point, const Station &center);
/**
 * Find paid native craft that still name this exact home identity.
 * @param center Station/generation pair; an empty generation never matches.
 * @return Whether a craft remains dependent on the center, including orbital craft.
 */
bool HasLaunchCraft(LaunchCenterRef center);
/** Advance unpaused paid hull work and reconcile slot ownership once per simulation tick. */
void TickLaunchOperations();
/**
 * Retire invalid queued references and place the FIFO head on a vacant pad.
 * @param center Native owner of the queue; remaining queued rockets use storage.
 */
void RefreshLaunchPad(Station &center);
/**
 * Release slots before a center identity disappears, retaining paid assets and work.
 * @param center Station being removed; active flight/descent owners receive target-loss notification.
 * @param cleaning_pool Whether world teardown already owns destruction of all pooled objects.
 */
void ForgetLaunchOperations(Station &center, bool cleaning_pool);
/**
 * Remove a craft's exact reference from all operational slots before Vehicle reuse.
 * @param vehicle Native paid craft whose reservations are being released.
 */
void ForgetLaunchCraft(SpaceVehicle &vehicle);
/** @return Copied Station-owned slot records in native station order, without moving any hull. */
std::vector<LaunchOperationsRecord> CaptureLaunchOperations();
/**
 * Validate bounded copied slots without resolving native pools; safe for previews.
 * @param records Copied LCOP records to check for malformed, duplicate or conflicting identities.
 * @return Whether queue bounds, exclusive workshop use and reference uniqueness hold.
 */
bool ValidateLaunchOperationRecords(std::span<const LaunchOperationsRecord> records);
/**
 * Validate copied production/slot relationships without mutating native owners.
 * @param craft Paid construction records, including unhomed retained work.
 * @param operations Center slot records whose assignments must match craft placement.
 * @param identities Generation-qualified identities from the native fleet transport.
 * @param centers Generation-qualified identities from the native center transport.
 * @return Whether every required slot and home resolves consistently and capacity is bounded.
 */
bool ValidateLaunchProductionLinks(std::span<const SpaceProductionRecord> craft, std::span<const LaunchOperationsRecord> operations,
	std::span<const SpaceCraftRef> identities, std::span<const LaunchCenterRef> centers);
/** @return Whether current slots agree with actual paid Vehicles, owners, placement and service work. */
bool ValidateLaunchOperations();
/**
 * Install copied slots only after structural and actual-world validation succeeds.
 * @param records Staged LCOP records; refusal leaves existing slots untouched.
 * @return Whether all records were validated and installed on their native Station owners.
 */
bool InstallLoadedLaunchOperations(std::span<const LaunchOperationsRecord> records);
/** @return Deterministic digest of paid production receipts, timers and Station-owned slots. */
uint64_t ComputeLaunchProductionDigest();
/**
 * Open the native center's assembly, cargo and flight controls.
 * @param center Generation-qualified launch center selected by the player.
 */
void ShowLaunchAssemblyWindow(LaunchCenterRef center);
#endif
