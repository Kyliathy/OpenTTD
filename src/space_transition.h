/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_transition.h Native saved rocket preparation, layer transition and recovery.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group14C; issued body intent is distinct from live navigation admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups10B/11.
 */
#ifndef SPACE_TRANSITION_H
#define SPACE_TRANSITION_H
#include "space_production.h"
#include "space_orders.h"
#include "space_structure_type.h"
#include "map_layer_type.h"

struct LaunchOperationsRecord;
/** Saved phase of the same paid native rocket; completion never commissions another hull. */
enum class SpaceFlightPhase : uint8_t { GroundLoading, Preparing, Ascent, OrbitalDelivery, ReturnRecovery, Turnaround, Holding, End };
/** Resumable physical flight admission; blocked cargo retains its current sole owner. */
enum class SpaceFlightBlock : uint8_t { None, TargetLost, HomeLost, PadBusy, ReceiverBusy, ReceiverFull, Approach, End };
/** Owned by the same native rocket Vehicle throughout a round trip. */
struct SpaceTransition {
	SpaceFlightPhase phase = SpaceFlightPhase::GroundLoading;
	SpaceFlightBlock blocked = SpaceFlightBlock::None;
	MapLayer layer = MapLayer::Surface;
	LaunchCenterRef source, recovery;
	SpaceOrderTarget destination;
	SpacePoint departure{};
	uint16_t remaining_ticks = 0;
	uint8_t pad_cost_remainder = 0;
	bool outbound_burned = false, recovery_burned = false, deployed = false;
	bool resupplied = false; ///< Separate414 receipt; an existing probe is not deployed again.
	bool module_delivered = false; ///< Separate423 once-only actual module freight receipt; no passenger deployment or craft resupply is implied.
	uint64_t missions_started = 0, missions_completed = 0, missions_canceled = 0;
	/** @param other Saved flight to compare. @return Whether all phase, payload, cost and identity receipts agree. */
	bool operator==(const SpaceTransition &other) const = default;
};
/** Detached transport references the existing native rocket and never owns a second ticking flight. */
struct SpaceTransitionRecord { SpaceCraftRef craft; SpaceTransition state; };
/** @param flight Same-file or actual flight. @param target Exact normalized static approach target.
 * @return Whether this flight alone may own a shared approach while its native hull is InTransit in Space.
 */
bool IsSpaceModuleFreightApproach(const SpaceTransition &flight, const SpaceOrderTarget &target);
/** @param craft Actual native hull. @return Whether its saved flight is outside GroundLoading. */
bool IsSpaceFlightActive(const SpaceVehicle &craft);
/** @param craft Native hull, including a carried passenger. @return Actual controller-owned layer, independent of camera selection. */
MapLayer GetSpaceVehicleLayer(const SpaceVehicle &craft);
/** @param craft Native hull or passenger. @return Whether Surface holding/lost-home custody permits explicit physical recovery. */
bool IsSpaceCraftAwaitingSurfaceRecovery(const SpaceVehicle &craft);
/** Advance actual saved flights once in the normal synchronized controller; queries earn no progress. */
void TickSpaceTransitions();
/** @param craft Actual native hull. @param center Exact removed home/recovery incarnation. Retain accountable payload while retiring that physical binding. */
void ForgetSpaceTransitionCenter(SpaceVehicle &craft, LaunchCenterRef center);
/** @param craft Actual native hull. @param site Exact removed module incarnation.
 * Mark only its unresolved module flight unavailable and release its request; actual payload remains aboard for explicit cancelled recovery.
 */
void ForgetSpaceTransitionStructure(SpaceVehicle &craft, SpaceStructureRef site);
/** @return Detached records for every existing native flight, retaining the original hull identity. */
std::vector<SpaceTransitionRecord> CaptureSpaceTransitions();
/** @param records Detached bounded flights. @param width Actual plane width. @param height Actual plane height.
 * @param next_body_id Same-owner issuance cursor; default validates only the 32-bit transport domain, while live/candidate joins supply their actual cursor.
 * @return Whether phase, identity, timers, layer and once-only payload receipts are structurally consistent.
 */
bool ValidateSpaceTransitionRecords(std::span<const SpaceTransitionRecord> records, uint32_t width = 4096, uint32_t height = 4096, uint32_t next_body_id = UINT32_MAX);
/** @param flight Same-file flights. @param craft Same-file paid hull states. @param operations Same-file physical pad owners.
 * @return Whether every active flight is the same paid in-transit rocket with its original recovery reservation.
 */
bool ValidateSpaceTransitionLinks(std::span<const SpaceTransitionRecord> flight, std::span<const SpaceProductionRecord> craft,
	std::span<const LaunchOperationsRecord> operations);
/** @return Whether installed flights agree with actual paid hulls, payload compartments, fuel and physical facilities. */
bool ValidateSpaceTransitions();
/** @param records Fully staged native flight transport. @return Whether whole preflight/installation succeeded without creating another hull. */
bool InstallLoadedSpaceTransitions(std::span<const SpaceTransitionRecord> records);
/** @return Stable digest of every saved native flight field and receipt in pool order. */
uint64_t ComputeSpaceTransitionDigest();
/** Discard detached decoder candidates without mutating an installed flight. */
void ResetSpaceTransitionLoadStaging();
/** @return Whether the staged family can attach to already restored native identities. */
bool AcceptLoadedSpaceTransitions();
/** Validate copied same-file flight ownership without borrowing live cargo, modules or facilities. */
void ValidateSpaceTransitionLoadPreview();
#endif
