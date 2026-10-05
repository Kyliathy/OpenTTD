/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_portal_cmd.h Command that builds a pair of layer portal tiles. @see docs/zz-reach2/upgrades/2026-10/r2mlm7-layer-portal-foundation.md, docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#ifndef LAYER_PORTAL_CMD_H
#define LAYER_PORTAL_CMD_H

#include "command_type.h"
#include "direction_type.h"
#include "map_layer_type.h"
#include "rail_type.h"
#include "road_type.h"
#include "transport_type.h"
#include "excavation.h"

/** One saved work phase per physical pair; topology and ordinary routing permissions remain distinct. */
enum class LayerPortalWorkState : uint8_t { Sealed, Commissioning, Connected, Invalid = 255 };

/** Stable unchanged portal-work refusal; no result grants cargo, ownership or excavation. */
enum class LayerPortalWorkError : uint8_t { None, InvalidPair, InvalidOwner, InvalidReference, Conflict, Limit, MissingExcavation, LockedDeep, ControllerUnavailable };

/** Canonical pair authority; only explicit IDs/phase are saved, never routing/reservation caches or duplicated endpoint state. */
struct LayerPortalWorkRecord {
	TileIndex pair_key = INVALID_TILE; ///< Lower existing endpoint index, resolved against actual reciprocal portal geometry.
	LayerPortalWorkState state = LayerPortalWorkState::Sealed; ///< Sole saved work phase for both endpoints.
	CompanyID company = CompanyID::Invalid(); ///< Actual company owner of both housings; Invalid is allowed only for explicitly ownerless unassigned Sealed road housing.
	ExcavationJobID project{UINT32_MAX}; ///< Actual geological work identity; absent outside commissioning.
	VehicleID cutter = VehicleID::Invalid(); ///< Actual assigned train reference; role admission additionally requires the later controller.
	StationID origin = StationID::Invalid(); ///< Actual optional saved Surface exchange reference, validated if present.
	uint64_t revision = 1; ///< Monotonic state stamp for unchanged prepare/commit checks.
};

/**
 * Resolve a reciprocal owned physical pair without asserting on malformed save/command input.
 * @param tile Either actual portal endpoint.
 * @return Existing lower endpoint key, or absence for invalid topology.
 */
std::optional<TileIndex> GetLayerPortalPairKey(TileIndex tile);

/**
 * Query the sole saved record through either actual endpoint.
 * @param tile Actual portal endpoint; viewport selection supplies no authority.
 * @return Read-only saved record, or nullptr for invalid/implicit legacy state.
 */
const LayerPortalWorkRecord *GetLayerPortalWorkRecord(TileIndex tile);

/**
 * Query explicit work state while preserving usable legacy portal policy.
 * @param tile Either actual endpoint.
 * @return Saved phase, Connected for an implicit Classic pair, or safe Sealed for absent native work authority.
 */
LayerPortalWorkState GetLayerPortalWorkState(TileIndex tile);

/**
 * Query the shared saved calendar policy for both ends of an existing physical pair.
 * @param tile Either actual endpoint; ascent and descent share the same gate.
 * @return None for date-valid topology, LockedDeep before unlock, or InvalidPair for broken geometry.
 */
LayerPortalWorkError CheckLayerPortalAvailability(TileIndex tile);

/**
 * Revalidate restricted work admission against the actual saved pair, job and vehicle.
 * @param tile Actual endpoint being entered.
 * @param actor Actual entering vehicle; a follower cannot inherit company-wide permission.
 * @param project Actual expected geological project identity.
 * @return None for a part of the live TBM project that owns this pair's cutter and job (Plan 4 role contract);
 *         ControllerUnavailable when no such project exists; otherwise a stable refusal.
 */
LayerPortalWorkError CheckLayerPortalWorkAccess(TileIndex tile, VehicleID actor, ExcavationJobID project);

/** Portal-side commissioning request; the existing geological proposal owns route/model/origin/capacity meanings once. */
struct LayerPortalCommissionRequest {
	TileIndex portal = INVALID_TILE; ///< Actual accessible upper rail portal, never a viewport-selected proxy.
	ExcavationJob proposal; ///< Actual unallocated geological route, owner/model/Surface-origin references and frozen separate Earth/Rock pass capacities.
	uint8_t earth_wagons = 0; ///< Earth hoppers of the consist to commission (Plan 4 Group 2.1); zero leaves a portal-only quote.
	uint8_t rock_wagons = 0; ///< Rock hoppers of the consist to commission, after the Earth ones.
};

/** Portal-side commissioning result; no prospective job/vehicle identity is represented as allocated on refusal. */
struct LayerPortalCommissionResult {
	LayerPortalWorkError error = LayerPortalWorkError::InvalidPair; ///< Stable pair/date/owner/controller refusal.
	TileIndex pair_key = INVALID_TILE; ///< Actual canonical validated pair, or invalid on topology refusal.
	ExcavationJobID project{UINT32_MAX}; ///< Actual allocated geometric identity only after the owning controller commits.
	VehicleID cutter = VehicleID::Invalid(); ///< Actual commissioned train only after Plan 4 acquisition succeeds.
	uint64_t pair_revision = 0; ///< Readonly preflight stamp for coordinated acquisition/revalidation.
};

/**
 * Validate portal-side commissioning, then hand the purchase to its owning controller (Plan 4 CommissionTBMProject),
 * which acquires the consist, reserves the job, binds this pair and publishes the project as one transaction.
 * @param request Actual pair, canonical unallocated geological proposal and wagon counts.
 * @param execute False estimates; true commissions.
 * @return The portal refusal; ControllerUnavailable when the controller refuses (its own result has the reason);
 *         otherwise None, with the allocated job and cutter after execution. Never creates substitute trains, jobs,
 *         wagons, stations or cash.
 */
LayerPortalCommissionResult CommissionLayerPortalTBM(const LayerPortalCommissionRequest &request, bool execute);

/**
 * Perform the portal owner's pure commissioning preflight separately from train acquisition.
 * @param request Actual accessible pair and canonical geological proposal.
 * @return None and a pair revision on valid source access; no vehicle/job identity is allocated.
 */
LayerPortalCommissionResult ValidateLayerPortalCommissioning(const LayerPortalCommissionRequest &request);

/**
 * Portal-issued bootstrap proof for a solid commissioning start (Plan 3 Group 8B).
 * It describes geometry and references only: no excavation credit, vehicle admission, purchase or payment.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-f-solid-starter-contract.md
 */
struct LayerPortalBootstrapProof {
	LayerPortalWorkError error = LayerPortalWorkError::InvalidPair; ///< None only for a complete issued proof.
	TileIndex pair_key = INVALID_TILE; ///< Canonical pair identity; also the accessible source endpoint.
	uint64_t pair_revision = 0; ///< Sealed work-record revision captured at issue.
	CompanyID company = CompanyID::Invalid(); ///< Actual owner of both housings and of the proposal.
	uint16_t model_id = UINT16_MAX; ///< Logical cutter whose layer and introduction date were checked.
	StationID origin = StationID::Invalid(); ///< Actual company-owned Surface railway origin.
	TileIndex source = INVALID_TILE; ///< Accessible upper endpoint where the consist assembles.
	std::vector<TileIndex> source_support; ///< Complete 3x3 source floor carrying the straddling consist and its retreat.
	TileIndex destination = INVALID_TILE; ///< Solid rock-side housing; the job's commissioning anchor.
	Direction exit_heading = Direction::Invalid; ///< Destination door heading; the only legal first cut heading.
	std::vector<TileIndex> pocket; ///< Restricted 3x3 destination occupancy, still unopened until the first paid cut.
	/** @param other Proof to compare. @return Whether every issued field matches. */
	bool operator==(const LayerPortalBootstrapProof &other) const = default;
};

/**
 * Issue the solid-start proof from the existing pure commissioning preflight.
 * Works before any route, train or job exists, so commissioning can be quoted before acquisition.
 * @param request Actual accessible upper rail pair and the unallocated geological proposal.
 * @return Complete proof, or a proof carrying only the stable preflight refusal; nothing is mutated.
 */
LayerPortalBootstrapProof IssueLayerPortalBootstrap(const LayerPortalCommissionRequest &request);

/**
 * Revalidate an earlier proof immediately before acquisition.
 * @param proof Proof issued earlier for this request.
 * @param request The same pair and proposal, re-read against live state.
 * @return None when a fresh issue equals the proof; otherwise the fresh refusal, or Conflict for a
 *         changed revision, source, pocket, model or origin.
 */
LayerPortalWorkError RevalidateLayerPortalBootstrap(const LayerPortalBootstrapProof &proof, const LayerPortalCommissionRequest &request);

/** Geology-facing projection of the pair that may issue a solid commissioning start. */
struct LayerPortalStarterIssuer {
	LayerPortalWorkError error = LayerPortalWorkError::InvalidPair; ///< None only for a current issuer.
	TileIndex pair_key = INVALID_TILE; ///< Canonical issuing pair key.
	Direction exit_heading = Direction::Invalid; ///< Destination door heading.
};

/**
 * Resolve the current issuer of a solid start without reading or copying geology.
 * @param anchor Rock-side destination housing named by the geological job.
 * @param company Job owner, which must own the pair.
 * @param job Invalid for an unpublished proposal (pair must be Sealed and unassigned);
 *            otherwise the published job the pair must be Commissioning.
 * @return Issuer key and exit heading, or a stable pair/owner/date/phase refusal.
 */
LayerPortalStarterIssuer GetLayerPortalStarterIssuer(TileIndex anchor, CompanyID company, ExcavationJobID job);

/**
 * Connect or safely hide an empty pair without changing its physical topology or geology.
 * @param tile Either actual endpoint.
 * @param company Actual command owner, checked against both housings and saved authority.
 * @param connected Requested ordinary connection state; false seals an empty Connected pair.
 * @param execute False performs pure validation; true revalidates and commits once.
 * @return Complete permission/refusal, preserving assigned or occupied work unchanged.
 */
LayerPortalWorkError ChangeLayerPortalConnection(TileIndex tile, CompanyID company, bool connected, bool execute);

/**
 * Execute a synchronized ordinary portal connection/sealing command.
 * @param flags Actual estimate/execute command policy.
 * @param tile Either endpoint of the physical pair.
 * @param connected True connects real excavated compatible approaches; false safely seals an empty pair.
 * @return Zero-cost accepted transition or stable unchanged owner/occupancy/geology/date refusal.
 */
CommandCost CmdChangeLayerPortalConnection(DoCommandFlags flags, TileIndex tile, bool connected);

DEF_CMD_TRAIT(Commands::ChangeLayerPortalConnection, CmdChangeLayerPortalConnection, CommandFlags{}, CommandType::LandscapeConstruction)

/**
 * Invalidate both committed endpoints through the existing signal/pathfinder and viewport owners.
 * @param tile Actual endpoint of a still-valid physical pair.
 */
void NotifyLayerPortalRoutingChanged(TileIndex tile);

/**
 * Validate an empty lifecycle mutation without changing pair, project or vehicle state.
 * @param tile Either actual endpoint; legacy pairs retain existing lifecycle rules.
 * @return None for a safe empty native pair, otherwise an unchanged assignment/occupancy/reservation refusal.
 */
LayerPortalWorkError CheckLayerPortalLifecycle(TileIndex tile);

/**
 * Retire one safe empty work record immediately before both metadata housings are removed.
 * @param tile Either still-existing endpoint of the validated pair.
 * @return Complete unchanged refusal or nonallocating retirement; packed terrain is untouched.
 */
LayerPortalWorkError RetireLayerPortalWorkPair(TileIndex tile);

/**
 * Reassign one pair before the lifecycle owner updates both endpoint owners together. A receiving company takes the
 * pair with whatever is bound to it, because a merger moves its trains, stations, job and project in the same change
 * (r2mlvs3 Group 11.4); only the ownerless road retention of a bankruptcy needs a safe empty pair.
 * @param tile Either actual endpoint.
 * @param new_owner Actual receiving company, or OWNER_NONE for a sealed unassigned ownerless road housing.
 * @return Complete unchanged refusal or nonallocating saved-owner update.
 */
LayerPortalWorkError TransferLayerPortalWorkPair(TileIndex tile, Owner new_owner);

/**
 * Release a Commissioning pair once its actual controller has finished (r2mlvs3 Group 11.3): the pair must be
 * Commissioning exactly this job and cutter, the job must be closed (completed, cancelled or retired), and no train
 * part or claim may remain on the rock-side end or its door; the withdrawn consist may still stand at the Surface key.
 * The pair returns to Sealed with no project, cutter or origin, as one record change.
 * @param tile Either endpoint.
 * @param job Job the pair must be Commissioning.
 * @param cutter Cutter the pair must be bound to.
 * @param execute False validates only.
 * @return None, InvalidPair, Conflict (other binding, live job, occupied rock side) or Limit.
 */
LayerPortalWorkError WithdrawLayerPortalWorkPair(TileIndex tile, ExcavationJobID job, VehicleID cutter, bool execute);

/**
 * Release whatever Commissioning binding a pair has, for company removal only: the company's trains are already
 * deleted and its project erased, so nothing is left to withdraw (r2mlvs3 Group 11.4, via the TBM lifecycle owner).
 * @param tile Either endpoint.
 */
void ReleaseLayerPortalWorkBinding(TileIndex tile);

/**
 * Bind the Sealed pair a commissioning preflight saw to its newly reserved job and acquired cutter (Group 8.4, as the
 * pair half of the Plan 4 commissioning transaction). The pair becomes Commissioning with that job, cutter and origin,
 * as one record change, and routing is notified.
 * @param tile Either endpoint.
 * @param pair_revision Revision the preflight captured; any change since refuses.
 * @param job Reserved job; Invalid is accepted for an estimate.
 * @param cutter Acquired front engine; Invalid is accepted for an estimate.
 * @param origin Project origin station.
 * @param execute False validates only.
 * @return None, InvalidPair, Conflict (not the same unassigned Sealed pair) or Limit.
 */
LayerPortalWorkError BindLayerPortalWorkCommissioning(TileIndex tile, uint64_t pair_revision, ExcavationJobID job, VehicleID cutter, StationID origin, bool execute);

/**
 * Forget a deleted cutter in the pair Commissioning it (r2mlvs3 Group 11.4, via the TBM deletion hook). The pair stays
 * bound to its job, with no cutter, until the controller cancels the job and withdraws the pair.
 * @param tile Either endpoint.
 * @param cutter Deleted cutter front engine.
 */
void ForgetLayerPortalWorkCutter(TileIndex tile, VehicleID cutter);

/**
 * Forget a deleted origin station in every pair naming it (r2mlvs3 Group 11.4, via the TBM station hook). An explicit
 * origin reference is optional, so the pair keeps its binding and its record stays valid without one.
 * @param station Deleted station.
 */
void ForgetLayerPortalWorkOrigin(StationID station);

/**
 * Derive restricted assigned-cutter occupancy without opening any geological cell.
 * @param tile Either endpoint of the accepted commissioning pair.
 * @return Actual assigned footprint on the destination rock plane, or absence outside valid commissioning.
 */
std::optional<ExcavationCommissioningPocket> GetLayerPortalWorkPocket(TileIndex tile);

/** @return Copied canonical records for bounded save transport; no map pointers or private cache ownership is exported. */
std::vector<LayerPortalWorkRecord> CaptureLayerPortalWorkRecords();

/** @return Actual accepted pair count without allocating a copied save transport during command preflight. */
size_t GetLayerPortalWorkPairCount() noexcept;

/** @return Portable digest over actual pair phase/reference authority, excluding map pointers, pocket derivation and routing caches. */
uint64_t ComputeLayerPortalWorkDigest();

/** Clear portal authority/load staging during real world initialization, without creating geometry or cavities. */
void ResetLayerPortalWorkState();

/**
 * Validate copied saved records against actual reciprocal map geometry and declared geological references.
 * @param records Bounded strictly ordered canonical records.
 * @param profile Saved profile selection, independently of currently accepted policy.
 * @param geology Validated staged geological owner; references never resolve into copied surrogate pools.
 * @param check_references Resolve actual loaded company/train/station pools after all chunks exist.
 * @return Complete unchanged validation or stable refusal.
 */
LayerPortalWorkError ValidateLayerPortalWorkRecords(std::span<const LayerPortalWorkRecord> records, const MultiLayerProfileSelection &profile, const GeologyWorldState &geology, bool check_references = true);

/**
 * Stage only complete validated bounded records until shared profile/geology acceptance.
 * @param records Actual copied transport, already validated by the owner.
 */
void StageLoadedLayerPortalWork(std::vector<LayerPortalWorkRecord> records);

/** Publish validated staged portal records after content and geology acceptance. */
void AcceptLoadedLayerPortalWork();

/** Reset unpublished portal chunk transport; invoked with the actual world reset. */
void ResetLayerPortalWorkLoadStaging();

/** Validate independent bounded portal preview presence/counts without publishing actual work state. */
void ValidateLayerPortalWorkLoadPreview();

static constexpr size_t MAX_LAYER_PORTAL_WORK_PAIRS = 4096; ///< Explicit sparse native-work bound; implicit Classic pairs allocate no records.

/** Preallocated owner node for a future Sealed pair; destruction discards only the unpublished candidate. */
class LayerPortalWorkReservation {
	using Store = std::map<TileIndex, LayerPortalWorkRecord>; ///< Sole-owner storage shape used only to prepare a transferable node.
	Store::node_type node; ///< Allocated unpublished node; not another accepted state store.
	/** @param prepared Fully allocated Sealed node supplied only by the actual owner factory. */
	explicit LayerPortalWorkReservation(Store::node_type prepared) : node(std::move(prepared)) {}
	friend LayerPortalWorkReservation PrepareLayerPortalWorkPair(TileIndex key, CompanyID company);
public:
	/** Construct an empty refused/legacy reservation without allocation or gameplay mutation. */
	LayerPortalWorkReservation() = default;
	/** @param other Unpublished capacity transferred without allocation. */
	LayerPortalWorkReservation(LayerPortalWorkReservation &&other) noexcept = default;
	/** @param other Unpublished capacity replacing this pending reservation. @return This reservation. */
	LayerPortalWorkReservation &operator=(LayerPortalWorkReservation &&other) noexcept = default;
	/** @return Whether actual owner capacity was fully allocated before map writes. */
	bool Ready() const { return !node.empty(); }
	/** @return True after nonallocating publication into valid actual reciprocal geometry, otherwise unchanged refusal. */
	bool Publish() noexcept;
};

/**
 * Allocate a Sealed record before native housing mutation without touching accepted state.
 * @param key Canonical proposed lower endpoint, checked again against physical topology at publication.
 * @param company Actual company owning both future housings.
 * @return Fully allocated unpublished node, or unchanged capacity/reference refusal.
 */
LayerPortalWorkReservation PrepareLayerPortalWorkPair(TileIndex key, CompanyID company);

/**
 * Infrastructure pieces one portal end counts as.
 * A rail end counts as one piece, the same as a rail depot.
 * A road end counts as two pieces, the same as a road depot.
 * @param transport Transport of the end.
 * @return Piece count for that one end.
 */
inline uint GetLayerPortalPieceCount(TransportType transport)
{
	return transport == TransportType::Rail ? 1 : 2;
}

/**
 * Build a validated reciprocal layer-portal pair through the existing command boundary.
 * @param flags Estimate/execute command policy.
 * @param tile Clicked source endpoint.
 * @param transport_type Actual rail/road transport family.
 * @param target_layer Explicit destination layer, validated against the profile pair matrix.
 * @param shaft_dir Source shaft direction; the opposite endpoint uses its reverse.
 * @param railtype Actual compatible rail type for rail housing.
 * @param roadtype Actual compatible road type for road housing; tram is excluded.
 * @return Complete construction quote or unchanged validation refusal.
 */
CommandCost CmdBuildLayerPortal(DoCommandFlags flags, TileIndex tile, TransportType transport_type, MapLayer target_layer, DiagDirection shaft_dir, RailType railtype, RoadType roadtype);

DEF_CMD_TRAIT(Commands::BuildLayerPortal, CmdBuildLayerPortal, CommandFlags({CommandFlag::Auto, CommandFlag::NoWater}), CommandType::LandscapeConstruction)

/**
 * Apply the existing construction feedback after a successful command, without granting gameplay permission.
 * @param cmd Completed command identity.
 * @param result Validated command result.
 * @param tile Clicked feedback location.
 * @param transport_type Original transport argument.
 * @param target_layer Original layer argument.
 * @param shaft_dir Original shaft-direction argument.
 * @param railtype Original rail-type argument.
 * @param roadtype Original road-type argument.
 */
void CcBuildLayerPortal(Commands cmd, const CommandCost &result, TileIndex tile, TransportType transport_type, MapLayer target_layer, DiagDirection shaft_dir, RailType railtype, RoadType roadtype);

/**
 * Move a vehicle that has stepped across a shaft edge onto the pair's entry edge.
 * @param portal End whose shaft was just crossed.
 * @param x World X, updated in place.
 * @param y World Y, updated in place.
 */
void TranslateIntoLayerPortalPair(TileIndex portal, int &x, int &y);

/**
 * Whether a vehicle at a point on a portal end is inside the shaft, so it is not drawn.
 * @param shaft Shaft direction of the end.
 * @param x World X of the point.
 * @param y World Y of the point.
 * @return True from the tunnel visibility frame of that shaft to the shaft edge.
 */
bool IsHiddenInLayerPortal(DiagDirection shaft, int x, int y);

#endif /* LAYER_PORTAL_CMD_H */
