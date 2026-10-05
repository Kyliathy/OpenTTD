/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_portal_cmd.h Command that builds a pair of layer portal tiles. @see docs/zz-reach2/upgrades/2026-10/r2mlm7-layer-portal-foundation.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

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
 * @return Complete permission or refusal; ControllerUnavailable remains until Plan 4 supplies actual TBM role/consist authorization.
 */
LayerPortalWorkError CheckLayerPortalWorkAccess(TileIndex tile, VehicleID actor, ExcavationJobID project);

/** Portal-side commissioning request; the existing geological proposal owns route/model/origin/capacity meanings once. */
struct LayerPortalCommissionRequest {
	TileIndex portal = INVALID_TILE; ///< Actual accessible upper rail portal, never a viewport-selected proxy.
	ExcavationJob proposal; ///< Actual unallocated geological route, owner/model/Surface-origin references and frozen separate Earth/Rock pass capacities.
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
 * Validate portal-side commissioning and hand off actual purchase/work to its owning controller.
 * @param request Actual pair and canonical unallocated geological proposal.
 * @param execute False estimates; true may acquire only through the actual Plan 4/5 APIs after they exist.
 * @return Stable validated refusal until the real controller is available; never creates substitute trains, jobs, wagons, stations or cash.
 */
LayerPortalCommissionResult CommissionLayerPortalTBM(const LayerPortalCommissionRequest &request, bool execute);

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
 * Reassign one safe empty pair before the lifecycle owner updates both endpoint owners together.
 * @param tile Either actual endpoint.
 * @param new_owner Actual receiving company, or OWNER_NONE for a sealed unassigned ownerless road housing.
 * @return Complete unchanged refusal or nonallocating saved-owner update.
 */
LayerPortalWorkError TransferLayerPortalWorkPair(TileIndex tile, Owner new_owner);

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
