/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm.h Train-owned cutter definitions, project authority, unpublished whole-consist acquisition, face work, the
 * actual-train cut participant, temporary support track, recurring cost and the work corridor.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 1-15; master AD-5
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-f-solid-starter-contract.md (Plan 4 consumer obligations)
 */

#ifndef TBM_H
#define TBM_H

#include "excavation.h"
#include "engine_type.h"
#include "layer_portal_cmd.h"
#include "core/random_func.hpp"
#include "track_type.h"
#include "rail_type.h"

struct Train;

/** Stable controller identity; geology continues to allocate and own its separate job identity. */
using TBMProjectID = StrongType::Typedef<uint32_t, struct TBMProjectIDTag, StrongType::Compare>;

/** Saved successful work phases; an interruption preserves the phase being resumed. */
enum class TBMPhase : uint8_t {
	Commissioning, TravelToFace, Bore, AdvanceSupport, ReturnForExchange, WaitForEmpties, Resume, Completed, Cancelled,
	ReturnToPark, ///< Return without cancelling the accepted job or unfinished geological work.
	WaitToPark, ///< Whole consist is at its bay; unload existing spoil through actual wagon exchange.
	Parked, ///< Whole empty consist is stopped in its actual bay, retaining resumable work.
	ReturnToRelease, ///< Cancellation requested; preserve geometry/claims until physical withdrawal and haulage settlement.
	WaitToRelease, ///< Whole cutter at its real bay, exchanging remaining actual spoil before cancellation.
	ReleasePending, ///< Withdrawn empty cutter, or a lost cutter, awaiting final cargo/occupancy settlement before claims can end.
	ReturnForHangar, ///< Ordinary halt order retained while loaded spoil returns to the real exchange bay.
	WaitForHangar, ///< Exchange remaining spoil before the native order drives to its selected destination.
	TravelToHangar, ///< Empty actual consist follows its ordinary depot order; cutting remains suspended.
	ParkedHangar, ///< Whole empty consist arrived at its selected actual hangar; explicit Resume retains the job.
	FollowOrders, ///< Native station/waypoint/depot list owns travel until an explicit work resume or hangar action.
	AwaitFaceTransfer, ///< Appended identity20: native face-exchange wait; original-file dialect admission remains separate.
};

/** @param phase Actual controller phase. @return Whether native travel returns to the exchange/hangar bay. */
inline bool IsTBMReturning(TBMPhase phase) { return phase == TBMPhase::ReturnForExchange || phase == TBMPhase::ReturnToPark || phase == TBMPhase::ReturnToRelease || phase == TBMPhase::ReturnForHangar; }
/** @param phase Original native controller identity. @return Whether the appended face-exchange wait is selected; this query performs no transition. */
inline bool IsTBMAwaitingFaceTransfer(TBMPhase phase) { return phase == TBMPhase::AwaitFaceTransfer; }
/** @param phase Actual controller phase. @return Whether actual empty wagons can complete the current bay exchange. */
inline bool IsTBMWaitingForExchange(TBMPhase phase) { return phase == TBMPhase::WaitForEmpties || phase == TBMPhase::WaitToPark || phase == TBMPhase::WaitToRelease || phase == TBMPhase::WaitForHangar; }
/** @param phase Actual controller phase. @return Whether cancellation is awaiting safe withdrawal and final haulage settlement. */
inline bool IsTBMFinishing(TBMPhase phase) { return phase == TBMPhase::ReturnToRelease || phase == TBMPhase::WaitToRelease || phase == TBMPhase::ReleasePending; }
/** @param phase Actual controller phase. @return Whether the machine is stored, optionally awaiting final release. */
inline bool IsTBMStored(TBMPhase phase) { return phase == TBMPhase::Parked || phase == TBMPhase::ReleasePending || phase == TBMPhase::ParkedHangar; }
/** @param phase Actual phase. @return Whether a native selected-hangar order has suspended work. */
inline bool IsTBMHangarTransfer(TBMPhase phase) { return phase == TBMPhase::ReturnForHangar || phase == TBMPhase::WaitForHangar || phase == TBMPhase::TravelToHangar; }
/** @param phase Actual controller phase. @return Whether unfinished work is suspended for return/parking, without a duplicate intent field. */
inline bool IsTBMParking(TBMPhase phase) { return phase == TBMPhase::ReturnToPark || phase == TBMPhase::WaitToPark || IsTBMStored(phase) || IsTBMFinishing(phase) || IsTBMHangarTransfer(phase); }

/** Saved resumable interruptions, independent of ordinary train orders and reversal flags. */
enum class TBMBlockedReason : uint8_t {
	None, Stopped, Breakdown, Servicing, MissingProject, MissingCutter, MissingOrigin, MissingPortal, MissingExchange, InvalidConsist, LockedModel, MissingSupport, EarthFull, RockFull, InsufficientFunds, Obstruction, AllocationFailure,
	Paused, ///< Player paused the project (CmdControlTBMProject); distinct from Stopped, which mirrors a stopped train.
	IllegalTurn, ///< The next segment does not advance one tile within 45 degrees of the saved cutting heading (Group 10.2).
	LogisticsStall, ///< A hauler of the project cannot dispose of or move its spoil (Plan 5 Group 9.3); new cutting waits, return and exchange go on.
	MissingHangar, ///< The selected ordinary depot order lost its actual compatible destination.
	OccupiedHangar, ///< Another real train currently occupies the selected portal magazine or active exchange bay.
	MissingOrders, ///< Native travel was selected but its list/current destination no longer supplies a usable order.
	End, ///< Validation bound; saved values are below it.
};

/** Immutable physical roles; only complete spoil wagon units are exchangeable. */
enum class TBMConsistRole : uint8_t { Invalid, Cutter, PermanentSupport, EarthWagon, RockWagon };

/** Feature-owned physical tuning of a profile-owned cutter model; this is never an EngineID. */
struct TBMCutterModel {
	uint16_t model_id; ///< Resolve layers, introduction and facility permissions through LayerVehicleCapability.
	uint16_t travel_speed; ///< Conventional train speed units; work speed uses the separate synchronized rate.
	uint16_t power_hp; ///< Installed traction power in the train engine's horsepower units.
	uint16_t work_rate_numerator; ///< Geological work ticks per simulation tick, divided by work_rate_denominator.
	uint16_t work_rate_denominator; ///< Fixed-point divisor; retain its remainder in project authority.
	Money purchase_cost; ///< Complete permanent machinery price before exchangeable wagon purchase.
	Money active_cost_per_year; ///< Recurring boring charge, separate from the geological cut/support fee.
	Money idle_cost_per_year; ///< Nonzero recurring stopped/service charge.
	uint16_t earth_wagon_model; ///< Profile-owned dedicated Earth hopper design.
	uint16_t rock_wagon_model; ///< Profile-owned dedicated Rock hopper design.
};

/**
 * Canonical saved train controller; it owns assignment, fixed parts, heading and state-duration accounting.
 * The referenced ExcavationJob exclusively owns route, face, cursor, partial geological work and material totals.
 * Wagon custody remains in the actual train chain; no cargo or mutable route cache is stored here.
 */
struct TBMProject {
	TBMProjectID id{UINT32_MAX}; ///< Invalid until this controller allocates a project.
	ExcavationJobID job{UINT32_MAX}; ///< Actual geological route/progress owner; absence is a recoverable block.
	CompanyID company = CompanyID::Invalid(); ///< Actual owner, never a copied cash account.
	VehicleID cutter = VehicleID::Invalid(); ///< Front engine of the commissioned train.
	uint16_t model_id = UINT16_MAX; ///< Immutable logical cutter model, independently of engine-pool allocation.
	std::vector<VehicleID> permanent_parts; ///< Ordered cutter followed by its articulated support parts; wagons never enter this list.
	Direction heading = Direction::Invalid; ///< Cutting orientation, set only by each committed advance; backing or reversing never changes it.
	StationID origin = StationID::Invalid(); ///< Required actual owned Surface railway station.
	TileIndex portal = INVALID_TILE; ///< Actual commissioning pair endpoint, resolved by the portal owner.
	TileIndex exchange_bay = INVALID_TILE; ///< Actual exchange facility, or source-side commissioning exchange until Plan 5.
	TBMPhase phase = TBMPhase::Commissioning; ///< Work phase retained while stopped, blocked or servicing.
	TBMBlockedReason blocked = TBMBlockedReason::None; ///< One precise saved interruption reason.
	uint64_t phase_ticks = 0; ///< One synchronized phase timer; movement substeps/rendering cannot advance it.
	uint16_t work_remainder = 0; ///< Fractional work-rate numerator, separate from geological partial work.
	uint64_t active_ticks = 0; ///< Saved duration awaiting recurring active-cost accounting.
	uint64_t idle_ticks = 0; ///< Saved duration awaiting recurring idle-cost accounting, including stopped ticks.
	uint64_t cost_remainder = 0; ///< Fractional recurring-money numerator carried across daily accounting.
	uint64_t revision = 0; ///< Controller stamp invalidating prepared physical participants.
	/** @param other Project to compare. @return Whether every saved field matches. */
	bool operator==(const TBMProject &other) const = default;
};

/** Read-only projected progress; geological fields are queried from their owner rather than saved twice. */
struct TBMWorkState {
	TBMPhase phase = TBMPhase::Commissioning; ///< Actual controller phase.
	TBMBlockedReason blocked = TBMBlockedReason::MissingProject; ///< Explicit invalid-reference default.
	TileIndex face = INVALID_TILE; ///< Next accepted geological face, absent after completion.
	uint32_t pass_cursor = 0; ///< Geological job's sole cursor.
	uint64_t partial_work_ticks = 0; ///< Geological job's sole partial accumulator.
	Direction heading = Direction::Invalid; ///< Saved forward cutting orientation.
	uint64_t phase_ticks = 0; ///< Controller's synchronized timer.
};

/** Actual whole-wagon boundary available to the later exchange owner; these are views, never another cargo store. */
struct TBMConsistUnit {
	VehicleID vehicle = VehicleID::Invalid(); ///< Real unit head; its articulated cargo parts move with it.
	TBMConsistRole role = TBMConsistRole::Invalid; ///< Dedicated Earth or Rock only.
};

/** Complete immutable-layout validation result, without changing links, cargo, reservations or funds. */
struct TBMConsistLayout {
	TBMBlockedReason error = TBMBlockedReason::InvalidConsist; ///< Success is None; malformed chains refuse entirely.
	std::vector<TBMConsistUnit> exchangeable_units; ///< Stable chain order of complete detachable wagons.
};

/** @return Both immutable physical cutter designs; profile capabilities remain the sole layer/date/facility source. */
std::span<const TBMCutterModel> GetTBMCutterModels();

/**
 * Resolve one physical cutter definition without allocating an engine or vehicle.
 * @param model_id Profile-owned logical design identity.
 * @return Immutable tuning, or nullptr for any non-cutter/unknown model.
 */
const TBMCutterModel *GetTBMCutterModel(uint16_t model_id);

/**
 * Validate cutter availability against explicit saved policy and synchronized calendar.
 * @param model_id Logical design, never a runtime engine ID.
 * @param layer Actual operation layer.
 * @param policy Accepted saved profile/version/override policy.
 * @param date Game-calendar date; economy and viewport state grant nothing.
 * @return Whether this cutter can operate here on this date under the selected profile.
 */
bool IsTBMCutterAvailable(uint16_t model_id, MapLayer layer, const MultiLayerProfileSelection &policy, TimerGameCalendar::Date date);

/**
 * Rebuild native machinery/hopper definitions through the existing engine pool and EIDS mapping after content loading.
 * @param activated Loader-owned configuration with pinned package and activated cargo meanings.
 * @return True after all four definitions exist, false for missing content or unavailable capacity.
 */
bool InitialiseTBMEngineDefinitions(const GRFConfigList &activated);

/**
 * Resolve a native design through the existing persisted engine mapping.
 * @param model_id Profile-owned cutter/hopper model (0..3).
 * @return Actual EngineID, or Invalid when unavailable.
 */
EngineID GetTBMEngine(uint16_t model_id);

/**
 * Classify an actual engine without interpreting an original/foreign engine number as a logical model.
 * @param engine Actual engine-pool identity.
 * @return Profile model 0..3, or UINT16_MAX for other/missing definitions.
 */
uint16_t GetTBMEngineModel(EngineID engine);

/**
 * Identify native articulated machinery for construction and purchase refusal.
 * @param engine Actual engine-pool identity.
 * @return Whether the native mapping identifies a cutter design.
 */
bool IsTBMCutterEngine(EngineID engine);

/**
 * Verify permanent cutter/support links and expose real detachable spoil units.
 * @param front Actual front train engine.
 * @param project Canonical immutable part IDs and company/cutter assignment.
 * @return Complete layout or unchanged refusal; uses GetNextUnit/GetPrevUnit for whole wagon boundaries.
 */
TBMConsistLayout InspectTBMConsist(const Train &front, const TBMProject &project);

/**
 * Classify one actual part against a project's immutable assignment (the Groups 3-5 role contract).
 * Role authority comes only from the saved part list and the actual chain; cargo labels never promote machinery.
 * @param project Canonical assignment: company, cutter and ordered permanent parts.
 * @param part Any actual train part.
 * @return Cutter, PermanentSupport, EarthWagon or RockWagon for a part of this company's assigned consist; Invalid otherwise.
 */
TBMConsistRole GetTBMConsistRole(const TBMProject &project, const Train &part);

/** Exchangeable spoil wagons requested behind the fixed machinery; the cutter model comes from the portal request. */
struct TBMConsistRequest {
	uint8_t earth_wagons = 0; ///< Dedicated Earth hopper units, coupled directly behind the permanent support.
	uint8_t rock_wagons = 0; ///< Dedicated Rock hopper units after the Earth units; never substitutes for Earth space.
	/** @param other Request to compare. @return Whether both wagon counts match. */
	bool operator==(const TBMConsistRequest &other) const = default;
};

/**
 * Choose the native version-1 commissioning presentation's separate spoil groups.
 * @param layer Actual working rock layer; camera state and the upper assembly layer do not choose capacity.
 * @return Two Earth hoppers and either two ordinary or four Deep Rock hoppers; invalid layers return an empty request.
 * The first twelve-cell Hard starter cut yields 480 tonnes of Rock, so Deep needs four native 128-tonne hoppers.
 * Native sizing, site, purchase and command checks remain authoritative; these defaults grant no construction permission.
 */
TBMConsistRequest GetDefaultTBMConsist(MapLayer layer);

/** Stable acquisition refusal; the detailed owner reason is kept beside it in the plan. */
enum class TBMAcquisitionError : uint8_t {
	None, ///< Every precondition passed; acquisition may allocate.
	Portal, ///< Portal commissioning or bootstrap refusal; see TBMAcquisitionPlan::portal_error.
	Geology, ///< Canonical starter, route, pass or reservation refusal; see TBMAcquisitionPlan::geology_error.
	UnavailableModel, ///< Definition missing; cutter locked by profile layer/date policy; hopper not buildable by the company.
	InvalidConsist, ///< No wagons, too long, or a route planned for a different consist length.
	VehicleLimit, ///< The vehicle pool cannot hold every part.
	UnitLimit, ///< The company has no free train unit number.
	InsufficientFunds, ///< The company cannot pay for the machinery and every wagon.
	Stale, ///< A fresh preparation no longer equals the accepted plan.
	AllocationFailure, ///< Construction or coupling failed after allocation began; everything was rolled back.
	NoReceiver, ///< Commissioning: the origin is not the company's Surface railway station, or no real receiver there accepts both Earth and Rock.
	ProjectLimit, ///< Commissioning: the project store, its identity cursor or the pair's revision is exhausted.
	RailTurns, ///< The canonical paid support path needs a physical bend forbidden by the current rail rule.
};

/** Physical size of a requested consist, from its definitions alone (shared by area preview and acquisition). */
struct TBMConsistDimensions {
	TBMAcquisitionError error = TBMAcquisitionError::InvalidConsist; ///< UnavailableModel, InvalidConsist or None.
	EngineID cutter_engine = EngineID::Invalid(); ///< Native cutter definition.
	EngineID earth_engine = EngineID::Invalid(); ///< Earth hopper definition named by the cutter model.
	EngineID rock_engine = EngineID::Invalid(); ///< Rock hopper definition named by the cutter model.
	uint32_t vehicle_parts = 0; ///< Pool items, articulated parts included.
	uint32_t consist_length = 0; ///< Whole length in vehicle units (TILE_SIZE per tile).
	uint8_t planning_points = 0; ///< Planner history size: ceil(length / TILE_SIZE) + 1, at least 3, at most 16.
	uint64_t earth_capacity_t = 0; ///< Combined Earth capacity.
	uint64_t rock_capacity_t = 0; ///< Combined Rock capacity.
};

/**
 * Size a consist from its definitions, before any site or company check.
 * @param model_id Cutter model; it names its own Earth and Rock hopper designs.
 * @param consist Requested wagon counts; both must be positive.
 * @return Dimensions, or UnavailableModel/InvalidConsist (missing definitions, a missing role, too long for the
 *         train-length setting, or more than 16 planning points).
 */
TBMConsistDimensions GetTBMConsistDimensions(uint16_t model_id, const TBMConsistRequest &consist);

/** A player's area selection for a project, ready for the canonical planner (Group 4.1). */
struct TBMAreaRequest {
	ExcavationOutline outline; ///< Normalized and authorized working floor (NormalizeExcavationOutline/AuthorizeExcavationOutline).
	ExcavationPlanningPose start; ///< Cutter start pose; its history length must match the consist's planning points.
	TBMConsistRequest consist; ///< Consist the area is planned for.
	uint16_t model_id = UINT16_MAX; ///< Cutter model.
	CompanyID company = CompanyID::Invalid(); ///< Owner of the resulting job.
	StationID origin = StationID::Invalid(); ///< Surface origin of the resulting job.
	std::vector<TileIndex> exchange_path; ///< Anchor-to-exchange backing path; empty selects the anchor.
	ExcavationPlanIntent intent = ExcavationPlanIntent::Area; ///< Reviewed area coverage or exact straight centreline.
	TileIndex tunnel_end = INVALID_TILE; ///< Exact endpoint for straight intent; ignored for area coverage.
};

/**
 * Working space beyond the floor, its ring, the housing and earlier work that a quoted route digs: the approach and the
 * turning space (r2df Groups 9-10).
 */
struct TBMApproachSummary {
	std::vector<TileIndex> tiles; ///< Canonical cells the route sweeps outside that existing authorization.
	uint32_t cuts = 0; ///< Cutter positions of the route whose sweep reaches one of those cells.
	ExcavationCutResult estimate; ///< QuoteExcavationCells of those cells; already part of the route's totals.
};

/** Read-only preview shown before any command accepts the area. */
struct TBMAreaPreview {
	ExcavationError error = ExcavationError::None; ///< Planner, pass or reservation refusal.
	TBMAcquisitionError consist_error = TBMAcquisitionError::None; ///< Consist sizing refusal, checked first.
	TileIndex failure_tile = INVALID_TILE; ///< Stable failing location.
	bool partial = false; ///< The planner covered only part of the floor; acceptance must be explicit.
	ExcavationRouteProposal plan; ///< Generated passes, covered/remaining floor and end pose.
	std::vector<TileIndex> turn_envelope; ///< Swept cells outside the requested floor (approach, turn and working space).
	std::array<uint32_t, 4> hardness{}; ///< Newly solid swept cells per GeologyHardness value.
	ExcavationCutResult totals; ///< Whole-route Earth, Rock, support units, cost and work.
	ExcavationPassTerms terms; ///< Frozen pass terms at the consist's capacities, including exchange breakpoints.
	ExcavationJob proposal; ///< Canonical job proposal; a partial plan requests only its covered floor.
	ExcavationJobID prospective_job{UINT32_MAX}; ///< Identity a reservation would allocate now; not reserved.
	TBMApproachSummary approach; ///< Actual approach cells and their existing share of the whole-route quote.
};

/**
 * Preview an area through the canonical planner, pass and reservation estimates; nothing is reserved or charged.
 * @param request Area, start pose, consist and owners.
 * @return Complete preview, or a refusal with the first failing stage.
 */
TBMAreaPreview PreviewTBMArea(const TBMAreaRequest &request);

/**
 * Complete pure preparation of one commissioning consist; nothing here is allocated or charged.
 * Equality compares every field, so acquisition can prove that nothing changed since the preview.
 */
struct TBMAcquisitionPlan {
	TBMAcquisitionError error = TBMAcquisitionError::InvalidConsist; ///< Overall result.
	LayerPortalWorkError portal_error = LayerPortalWorkError::None; ///< Portal owner's reason for a Portal refusal.
	ExcavationError geology_error = ExcavationError::None; ///< Geology owner's reason for a Geology refusal.
	TileIndex failure_tile = INVALID_TILE; ///< Stable failing location, when one exists.
	LayerPortalBootstrapProof proof; ///< Portal-issued source support, destination pocket and exit heading.
	TBMConsistRequest consist; ///< Requested wagon counts.
	EngineID cutter_engine = EngineID::Invalid(); ///< Actual native cutter definition.
	EngineID earth_engine = EngineID::Invalid(); ///< Actual Earth hopper definition named by the cutter model.
	EngineID rock_engine = EngineID::Invalid(); ///< Actual Rock hopper definition named by the cutter model.
	UnitID unit_number = 0; ///< Train unit number the front engine will take.
	uint32_t vehicle_parts = 0; ///< Pool items the whole consist allocates, articulated parts included.
	uint32_t consist_length = 0; ///< Whole length in vehicle units (TILE_SIZE per tile).
	uint8_t planning_points = 0; ///< Planner history size for this consist (3..16).
	uint64_t earth_capacity_t = 0; ///< Actual combined Earth hopper capacity.
	uint64_t rock_capacity_t = 0; ///< Actual combined Rock hopper capacity.
	Money machinery_cost = 0; ///< Permanent cutter/support price from its physical model.
	Money wagon_cost = 0; ///< Combined hopper price from the existing engine cost owner.
	ExcavationJob job; ///< Request proposal with pass terms frozen at the actual capacities.
	ExcavationJobID prospective_job{UINT32_MAX}; ///< Identity a reservation would allocate now; not reserved.
	/** @param other Plan to compare. @return Whether every prepared field matches. */
	bool operator==(const TBMAcquisitionPlan &other) const = default;
};

/**
 * Prepare a complete commissioning consist without allocating, charging or reserving anything.
 * Checks the portal proof, both hopper and cutter definitions, wagon layout, length, pool, unit number, funds,
 * the route's planning history and the canonical pass/reservation estimates at the actual capacities.
 * @param request Portal request whose proposal carries the canonical route, requested floor and exchange path.
 * @param consist Requested Earth/Rock wagon counts.
 * @return Complete plan, or a refusal with every world state unchanged.
 */
TBMAcquisitionPlan PrepareTBMAcquisition(const LayerPortalCommissionRequest &request, const TBMConsistRequest &consist);

/**
 * Check whether an existing native cutter can start another job at its original commissioning site.
 * @param front Actual whole train, with no live project or exchange custody.
 * @param portal Actual Sealed source site; its pair/owner/date checks remain with the portal owner.
 * @param company Requesting company; every real part must belong to it.
 * @param model_id Requested native cutter model.
 * @return True only for a healthy, empty, stopped whole consist withdrawn in this exact magazine.
 */
bool IsTBMConsistReusableAtPortal(const Train &front, TileIndex portal, CompanyID company, uint16_t model_id);

/**
 * Unpublished acquisition owner: stopped, hidden cutter/support/wagon parts on the source site plus their prepared
 * project. Destroying it without Release() deletes every acquired part (undoing unit number and group counts) and
 * restores the random seeds captured before construction, so a failed publication leaves nothing behind.
 * Reusing an eligible retired consist holds only its prospective assignment: rollback never deletes those already
 * owned parts or alters their packets/positions. Only AcquireTBMConsist fills it; it is not copyable.
 */
class TBMAcquisition {
public:
	/** Create an empty unpublished acquisition owner. */
	TBMAcquisition() = default;
	/** @param other Existing acquisition; unique publication ownership cannot be copied. */
	TBMAcquisition(const TBMAcquisition &other) = delete;
	/** @param other Existing acquisition; unique publication ownership cannot be assigned. @return Unavailable. */
	TBMAcquisition &operator=(const TBMAcquisition &other) = delete;
	/** Roll back anything still held. */
	~TBMAcquisition() { this->Rollback(); }

	/** @return Whether acquired parts are still held unpublished. */
	bool IsHeld() const { return this->held; }

	/** @return The assembled front engine, or nullptr when nothing is held. */
	Train *GetFront() const;

	/** @return The prepared, unallocated project (invalid id/job) describing the held consist. */
	const TBMProject &GetProject() const { return this->project; }

	/**
	 * Delete fresh held parts and restore captured random seeds; reused paid parts survive rollback unchanged.
	 * Any randomness consumed after acquisition in the same command is rewound too, which is correct
	 * because the whole command is being undone on every client.
	 */
	void Rollback();

	/**
	 * Hand the parts to a publishing owner; afterwards this object no longer rolls back.
	 * @return The prepared project; its id and job remain for the publishing owner to allocate.
	 */
	TBMProject Release();

private:
	friend TBMAcquisitionError AcquireTBMConsist(const LayerPortalCommissionRequest &request, const TBMAcquisitionPlan &plan, TBMAcquisition &out);

	VehicleID front = VehicleID::Invalid(); ///< Assembled chain head; deleting it deletes every coupled part.
	TBMProject project; ///< Prepared assignment; not allocated or published.
	SavedRandomSeeds seeds{}; ///< Seeds captured immediately before the first part was built.
	bool held = false; ///< Whether rollback still owns the parts.
	bool existing = false; ///< True holds only a prospective assignment for already owned parts; rollback must preserve them.
};

/**
 * Build the whole consist as unpublished parts. Re-prepares first and refuses Stale unless the result equals plan,
 * then assembles through BuildTBMConsistAtSite and verifies the actual layout, length, parts and capacities.
 * Eligible explicit reuse holds the existing retired chain and validates that same layout without allocating or
 * charging a second purchase. Charges nothing and reserves no job; publication pays, reserves and binds together.
 * @param request The same portal request the plan was prepared from.
 * @param plan Accepted successful preparation.
 * @param out Empty owner; on success it holds the parts, on failure it stays empty with everything rolled back.
 * @return None, Stale, a fresh preparation refusal, or AllocationFailure after a complete rollback.
 */
TBMAcquisitionError AcquireTBMConsist(const LayerPortalCommissionRequest &request, const TBMAcquisitionPlan &plan, TBMAcquisition &out);

/*
 * Project assignment (Group 3). A project is a typed record in its own store, keyed by TBMProjectID and bound to
 * exactly one cutter. It never lives in an order: Order::type packs a 4-bit OrderType with non-stop flags and
 * Order::dest is a 16-bit destination, so a 32-bit project ID cannot be stored there without changing saved
 * meanings. The cutter keeps ordinary train orders for travel and servicing. Sharing or copying those orders moves
 * no project, because assignment is looked up by cutter ID here, and CmdCloneVehicle already refuses cutters.
 */

static constexpr size_t MAX_TBM_PROJECTS = 4096; ///< Bounded live store; each project owns one cutter and one job.
static constexpr size_t MAX_TBM_PERMANENT_PARTS = 4; ///< Cutter plus its articulated support parts; natives use two.

/** Player controls for a project. Work phases themselves advance only through the controller. */
enum class TBMProjectAction : uint8_t {
	Pause, ///< Hold work in its current phase (saved as TBMBlockedReason::Paused).
	Resume, ///< Clear a player pause; any lost reference becomes the blocked reason at once.
	Cancel, ///< Request final withdrawal/settlement; unfinished claims and partial work remain until the real train and haulage are clear. Then cancel through the geological owner, retaining paid cavities and all vehicles/cargo.
	Retire, ///< Finish/cancel fully withdrawn settled work, release its haulers/claims and retain the actual native machine; parked unfinished work is admitted only after the complete release preflight.
	ReturnAndPark, ///< Preserve the accepted job and partial work; physically return, exchange existing spoil and stop the whole consist at its actual bay.
	End, ///< Validation bound.
};

/** Stable project store refusal. */
enum class TBMProjectError : uint8_t {
	None, ///< Valid.
	InvalidProject, ///< Unknown identity, malformed field or out-of-range phase/reason.
	InvalidOwner, ///< Company missing or not the command actor.
	MissingCutter, ///< Cutter is not a live front engine of the company with this model.
	InvalidConsist, ///< Saved permanent parts no longer match the actual chain.
	Duplicate, ///< Another project already owns this cutter, or identities are not strictly increasing.
	MissingJob, ///< Geological job missing, closed, foreign or bound to another cutter/anchor.
	MissingOrigin, ///< Origin is not the company's Surface railway station.
	MissingPortal, ///< Portal is not a canonical company pair, or a Commissioning pair binds another job/cutter.
	Limit, ///< Store or identity bound reached.
	Unavailable, ///< Action not delivered yet (Cancel until Group 17) or not applicable in this state.
};

/**
 * @param id Exact project identity.
 * @return The live project, or nullptr. References expire at the next store mutation.
 */
const TBMProject *GetTBMProject(TBMProjectID id);

/**
 * @param cutter Actual front engine.
 * @return The project assigned to this cutter, or nullptr; at most one exists.
 */
const TBMProject *GetTBMProjectForCutter(VehicleID cutter);

/**
 * Query the first saved project designating an actual exchange depot, without a second facility registry.
 * @param bay Actual depot tile; source portal magazines remain with their portal owner.
 * @return Read-only project in stable saved order, or nullptr when no project designates this depot.
 */
const TBMProject *GetTBMProjectForExchangeBay(TileIndex bay);

/** @return Copy of every live project in increasing ID order, for saving and diagnostics. */
std::vector<TBMProject> CaptureTBMProjects();

/**
 * @param phase Saved phase.
 * @return Whether a project in this phase has ended and keeps only history (Completed or Cancelled).
 */
bool IsTBMProjectFinished(TBMPhase phase);

/**
 * Move a project's exchange to another bay (Plan 5 Group 1). The caller has validated the bay through
 * ResolveTBMExchangeBay (tbm_exchange.h) and that no hauler reserves the old one.
 * @param id Project of the command actor, not finished.
 * @param bay New rearrangement tile: the project's portal key or a depot bay.
 * @param execute False validates only.
 * @return None, InvalidProject, InvalidOwner or Unavailable.
 */
TBMProjectError SetTBMProjectExchangeBay(TBMProjectID id, TileIndex bay, bool execute);

/**
 * Replace a project's origin with another company Surface railway station (Plan 5 Group 8.1). A finished project may
 * change it too, so its last load can still be hauled after its first origin is lost. Wagon sets that already visited
 * the old origin keep their visit; a MissingOrigin wait recovers on the project's next step.
 * @param id Project of the command actor.
 * @param station Replacement origin.
 * @param execute False validates only.
 * @return None, InvalidProject, InvalidOwner, MissingOrigin or Unavailable.
 */
TBMProjectError SetTBMProjectOrigin(TBMProjectID id, StationID station, bool execute);

/** @return Next identity the store will allocate; IDs are never reused, so this also counts retired projects. */
TBMProjectID GetNextTBMProjectID();

/**
 * Validate one project against the live owners (vehicles, geology, portal work, stations).
 * An unblocked project must have every reference live. A blocked project may keep references that no longer qualify
 * (foreign owner, lost Commissioning binding) or explicitly invalid ones, but never IDs that resolve to nothing.
 * @param project Complete published or loaded record with an allocated ID.
 * @return None or the first stable refusal.
 */
TBMProjectError ValidateTBMProject(const TBMProject &project);

/**
 * Validate a complete project set's structure and every project against the live owners.
 * @param projects Strictly increasing IDs, each below next_id.
 * @param next_id Saved allocation cursor.
 * @return None or the first stable refusal.
 */
TBMProjectError ValidateTBMProjects(std::span<const TBMProject> projects, TBMProjectID next_id);

/**
 * Allocate an ID for a prepared project and add it to the store. Publication (R2MLVS3MP Group 6) calls this in the
 * same command that pays, reserves the job and publishes the Commissioning pair; it has no command trait.
 * @param project Prepared project with an invalid ID; receives the allocated ID and revision 1 on success.
 * @param execute False validates only.
 * @return None or an unchanged refusal.
 */
TBMProjectError PublishTBMProject(TBMProject &project, bool execute);

/** Result of one commissioning purchase (Group 2.1, R2MLVS3MP master 6.6); refusals allocate nothing. */
struct TBMCommissionResult {
	TBMAcquisitionError error = TBMAcquisitionError::InvalidConsist; ///< Overall result; Portal and Geology carry their owner's reason.
	LayerPortalWorkError portal_error = LayerPortalWorkError::None; ///< Portal owner's reason for a Portal refusal.
	ExcavationError geology_error = ExcavationError::None; ///< Geology owner's reason for a Geology refusal.
	TileIndex failure_tile = INVALID_TILE; ///< Stable failing location, when one exists.
	Money cost = 0; ///< Machinery and every hopper; the command charges it once.
	TBMProjectID project{UINT32_MAX}; ///< Published project, after execution only.
	ExcavationJobID job{UINT32_MAX}; ///< Reserved geological job, after execution only.
	VehicleID cutter = VehicleID::Invalid(); ///< Commissioned front engine, after execution only.
};

/**
 * Commission one boring machine at a Sealed rail portal as one transaction (Group 2.1, r2mlvs3 Group 8). Every pure
 * check runs first: the portal proof, definitions, route replay, pass and reservation quotes, pool, unit number, funds,
 * origin receivers, store capacity and the pair binding. Execution then acquires the consist, reserves the job, binds
 * the pair Commissioning that job and cutter, and publishes the project in the Commissioning phase; nothing after
 * the acquisition can fail once those checks passed. Payment is the command's returned cost.
 * @param request Portal request whose proposal is the canonical route; its wagon counts size the consist.
 * @param execute False quotes only and changes nothing.
 * @return The quote or the published identities, or a refusal with vehicles, funds, geology, pair and RNG unchanged.
 */
TBMCommissionResult CommissionTBMProject(const LayerPortalCommissionRequest &request, bool execute);

/**
 * Apply one player control to a project owned by the command actor.
 * @param id Project to control.
 * @param action Requested control.
 * @param execute False validates only.
 * @return None or an unchanged refusal.
 */
TBMProjectError ControlTBMProject(TBMProjectID id, TBMProjectAction action, bool execute);

/**
 * Give an existing cutter's native order list control while preserving its exact geological job and cargo.
 * Called only by synchronized order commands after their own list/index mutation succeeds.
 * @param front Actual native train; ordinary or finished trains require no work-controller change.
 * @return Whether an unfinished controller adopted the native list; final withdrawal remains protected.
 */
bool FollowTBMVehicleOrders(Train &front);

/**
 * Explain final release using the same complete preflight as the synchronized Retire action.
 * @param id Actual project identity; this read-only query grants no command ownership.
 * @return INVALID_STRING_ID when ready, otherwise a localized return, settlement or portal-recovery instruction.
 */
StringID GetTBMProjectReleaseProblem(TBMProjectID id);

/**
 * Find the first reference this project has lost, in priority order cutter, consist, job, portal, origin, exchange.
 * @param project Record to check against the live owners.
 * @return The matching recoverable reason (MissingCutter, InvalidConsist, MissingProject, MissingPortal, MissingOrigin,
 *         MissingExchange), or None when every reference is live.
 */
TBMBlockedReason FindTBMProjectReferenceLoss(const TBMProject &project);

/**
 * Turn lost references into explicit recoverable state (Group 4.4). IDs that no longer resolve at all are cleared to
 * invalid, so a reused ID can never silently rebind; the first loss becomes the blocked reason unless the project is
 * already blocked (a player pause, for example, is kept and the loss is re-derived on Resume).
 * @param project Record to normalize in place.
 * @return Whether anything changed.
 */
bool NormalizeTBMProjectReferences(TBMProject &project);

/**
 * Facts observed for one controller step, gathered by AdvanceTBMWork from the real owners: the train state and face
 * position from the train, exchange need from the frozen pass terms and the wagons, and the cut and work wait from the
 * face step (Groups 6-8).
 */
struct TBMPhaseInputs {
	bool train_stopped = false; ///< The train is stopped (VehState::Stopped).
	bool broken_down = false; ///< The train is broken down.
	bool servicing = false; ///< The train follows an ordinary depot order for servicing.
	bool service_finished = false; ///< An actual depot order ended; rejoin the face/bay physically instead of resuming work at the depot.
	bool hangar_order = false; ///< A native halt order remains available after unloading at the original exchange bay.
	bool at_face = false; ///< The cutter reached the current face (ends TravelToFace).
	bool cut_committed = false; ///< The prepared cut at the face committed (ends Bore).
	bool support_advanced = false; ///< The consist moved onto the new support (ends AdvanceSupport).
	bool exchange_due = false; ///< Actual wagon space cannot take the next cut, per the frozen pass breakpoints.
	bool at_exchange = false; ///< The cutter consist is admitted at its bay (ends ReturnForExchange; Plan 5 Group 1).
	bool route_complete = false; ///< The job has no further cuts (also true once the job is cancelled).
	bool cancelled = false; ///< The job was cancelled (Group 17.1): forward phases turn back, and the return ends the project Cancelled.
	TBMBlockedReason work_wait = TBMBlockedReason::None; ///< Precise wait from the face work step (Group 6.3); holds the phase.
};

/** Result of one pure phase step. */
struct TBMPhaseStep {
	TBMProjectError error = TBMProjectError::None; ///< Refusal; nothing applies.
	TBMPhase phase = TBMPhase::Commissioning; ///< Next phase (unchanged while blocked).
	TBMBlockedReason blocked = TBMBlockedReason::None; ///< Next interruption.
	bool phase_changed = false; ///< Whether the phase timer restarts.
};

/**
 * Decide one phase step without changing anything (Group 4.2A and 4.2B edges, 4.3 resumable conditions).
 * Edges: Commissioning to TravelToFace; TravelToFace to Bore at the face; Bore to AdvanceSupport when the cut commits,
 * or to AwaitFaceTransfer/ReturnForExchange when an exchange is due first; AdvanceSupport back to Bore, or to the
 * same geometry-selected meeting. AwaitFaceTransfer waits for actual whole-load removal, then advances onto any
 * new support before boring; a lost stand or completed route returns to the bay. Group 4.2B, through the Plan 5
 * exchange (tbm_exchange.h): ReturnForExchange to WaitForEmpties once
 * the consist is admitted at its bay; WaitForEmpties to Resume once no exchange is due, which only an executed exchange
 * makes true (the actual wagons then hold no spoil); Resume to TravelToFace, or to Completed after the final return.
 * Conditions: a lost reference, a player pause, a stopped, broken-down or servicing train, or a face work wait each
 * hold the phase. Group 17.1: with a cancelled job every forward phase (Commissioning, TravelToFace, Bore, AdvanceSupport)
 * turns to ReturnForExchange, the exchange gives away whatever spoil is aboard, and Resume ends the project Cancelled.
 * Heading is never read from the train, so reversing or backing cannot change the cutting direction.
 * @param project Current record.
 * @param inputs Observed facts.
 * @return The step, or a refusal.
 */
TBMPhaseStep StepTBMProjectPhase(const TBMProject &project, const TBMPhaseInputs &inputs);

/**
 * Apply one controller step to a live project: normalize references, step the phase, advance the one phase timer
 * (restarted on a phase change) and the active or idle duration. Call once per simulation tick per project.
 * @param id Project to step.
 * @param inputs Observed facts.
 * @return None, or a refusal with the project unchanged.
 */
TBMProjectError AdvanceTBMProjectPhase(TBMProjectID id, const TBMPhaseInputs &inputs);

/**
 * Read the train-side and face facts for a project: stopped, broken down, servicing, at the face approach (the housing
 * before the first cut, else the last committed cutter position), route complete and exchange due. The cut commit and
 * work wait come from the face step in AdvanceTBMWork.
 * @param project Project whose cutter is observed.
 * @return Observed inputs; all false when the cutter is missing.
 */
TBMPhaseInputs ObserveTBMPhaseInputs(const TBMProject &project);

/*
 * Controller boundary (Group 5). Owned fields: TBMProject owns assignment, permanent parts, heading, phase, blocked
 * reason, the one phase timer and duration accounting. The geological job owns route, face, cursor and partial work;
 * TBMWorkState only projects them. Wagons and their cargo stay in the actual train chain.
 *
 * Call sites and separation:
 * - Simulation: AdvanceTBMWork is called from exactly one place, Train::Tick, once per front engine per tick.
 * - Commands: CmdControlTBMProject records player intent only (pause/resume); it never advances work.
 * - Estimates: PreviewTBMArea, ValidateTBMAreaRoute and PrepareTBMAcquisition are pure.
 *
 * Tested by Groups 3-4: assignment, references, phase edges and conditions, timer and reload. Groups 6-8 add the
 * work facts: face arrival and support advance are observed from the cutter's tile; in Bore the face step adds
 * bounded work and commits paid cuts that lay support and load real spoil, or names a precise wait. Ordinary train
 * ticks drive the physical chain through its work pocket and paid support under the project corridor's admission
 * rules; GetTBMCutterSteeringTarget names the existing approach for TravelToFace/AdvanceSupport. IsTBMHoldingTrain
 * holds only face work and the wait for empty wagons. Plan 5 owns actual source exchange/disposal/empty return;
 * recurring underground-bay and complete occupied-file acceptance remain separate scopes in the master plan.
 */

/**
 * Project the work state of one project from its own record and its geological job.
 * @param id Project to read.
 * @return Phase, reason, face, cursor, partial work, heading and timer; MissingProject defaults when absent.
 */
TBMWorkState GetTBMWorkState(TBMProjectID id);

/**
 * Measure an actual assigned consist from its chain (real lengths and capacities, not definitions).
 * @param front Actual front engine.
 * @param project Assignment whose immutable layout must match the chain.
 * @return Dimensions, or InvalidConsist when the layout or planner history does not fit.
 */
TBMConsistDimensions MeasureTBMConsist(const Train &front, const TBMProject &project);

/**
 * Adapt the canonical planner to an actual assigned consist: the area preview with the train's measured length and
 * capacities, the project's model and owners. It is the same planner as PreviewTBMArea, not a second one.
 * @param project Assigned project with a live cutter.
 * @param area Outline, start pose and exchange path; its consist and owner fields are replaced from the project.
 * @return Complete preview, or a refusal.
 */
TBMAreaPreview ValidateTBMAreaRoute(const TBMProject &project, const TBMAreaRequest &area);

/**
 * One simulation step of the controller: observe the train and face, run the face step when the project is unblocked
 * in Bore, save its work remainder, then advance the phase with the cut and wait facts.
 * @param id Project to step; called only from Train::Tick.
 * @return None, or a refusal; a refused phase step leaves the project unchanged.
 */
TBMProjectError AdvanceTBMWork(TBMProjectID id);

/*
 * Temporary support track (Group 8). Each cut lays ordinary railway pieces of the project's company along the cutter
 * path: the new cutter cell and, for a diagonal advance, the side cell it passes through. A registry keeps the metadata
 * of every such tile. The excavation state beneath stays with geology, so removing the rail later leaves the cavity
 * open. Only the cut transaction may lay support; ordinary rail tools still refuse work cells, and conversion or removal
 * of laid support is Group 9.
 */

/** Vehicle classes that may run on temporary support track (Group 8.1). */
enum class TBMSupportClass : uint8_t {
	Cutter, ///< The assigned cutter and its permanent support parts.
	SpoilWagon, ///< The project's own dedicated Earth and Rock wagons.
	Hauler, ///< A Plan 5 hauler collecting this project's wagons.
	End, ///< Validation bound.
};
/** Set of allowed support traffic classes. */
using TBMSupportClasses = EnumBitSet<TBMSupportClass, uint8_t, TBMSupportClass::End>;

/** Saved metadata of one temporary support track tile (Group 8.1). */
struct TBMSupportTile {
	TileIndex tile = INVALID_TILE; ///< Rock-layer tile carrying the support track.
	TBMProjectID project{UINT32_MAX}; ///< Project that laid it; kept after the project ends, until Group 9 conversion or removal.
	CompanyID owner = CompanyID::Invalid(); ///< Track owner, the project's company.
	TrackBits track{}; ///< Pieces the project laid on this tile.
	TBMSupportClasses allowed{}; ///< Vehicle classes that may traverse it; movement enforcement is Group 14.
	uint16_t max_speed = 0; ///< Speed limit on temporary support, in train speed units.
	Money conversion_price = 0; ///< Price to convert the laid pieces into permanent track (Group 9).
	/** @param other Tile to compare. @return Whether every saved field matches. */
	bool operator==(const TBMSupportTile &other) const = default;
};

static constexpr uint32_t MAX_TBM_SUPPORT_TILES = 65536; ///< World-wide registry bound, the same as the geology claim bound.
static constexpr uint16_t TBM_SUPPORT_MAX_SPEED = 40; ///< Speed limit on temporary support track.

/**
 * @param tile Any tile.
 * @return The support metadata on this tile, or nullptr.
 */
const TBMSupportTile *GetTBMSupportTile(TileIndex tile);

/** @return Copy of the support registry in increasing tile order. */
std::vector<TBMSupportTile> CaptureTBMSupport();

/** One support piece prepared for a cut. */
struct TBMSupportPiece {
	TileIndex tile = INVALID_TILE; ///< Tile to lay on: a clear cell open after the cut, or this project's own support.
	Track track = Track::Invalid; ///< Piece connecting the previous path tile to the next one.
};

/** Complete support placement for one cut, prepared before any write. */
struct TBMSupportPlan {
	TBMBlockedReason error = TBMBlockedReason::None; ///< None, MissingSupport (registry full) or Obstruction.
	TileIndex failure_tile = INVALID_TILE; ///< First failing tile.
	RailType railtype = INVALID_RAILTYPE; ///< Rail type of the project's pair.
	std::vector<TBMSupportPiece> pieces; ///< Pieces to lay, in path order.
};

/**
 * Prepare the support pieces of the job's next cut without changing anything. The path runs from the housing through
 * every cutter position; a diagonal advance passes through the side cell that turns away from the entry edge, so
 * successive diagonal advances alternate corner pieces like ordinary diagonal track (straight after the housing it
 * passes straight on first, so the piece at the portal mouth is the axis track that joins it). Each piece already
 * points at the next planned position, so a later cut never has to change track under the train. A route that passes
 * over its own support again reuses the piece there or adds a junction piece. Existing owned compatible permanent
 * rail on paid open floor is reused only when the exact required piece is already present; adding ordinary track
 * remains a paid construction command before assignment. Other infrastructure refuses.
 * @param project Assigned project.
 * @param job Its live job, with the cursor at the cut to support.
 * @return The pieces, or a refusal naming the first tile that cannot take support.
 */
TBMSupportPlan PrepareTBMSupport(const TBMProject &project, const ExcavationJob &job);

/**
 * Prepare the piece the face needs after a change of work area (r2df Group 12). The face piece was laid by the last
 * completed cut, aimed at the next planned cut; a new remainder may leave the face another way. Changes nothing.
 * @param project Assigned project.
 * @param job Its job with the new remainder after the completed cuts.
 * @return No pieces when the laid track already leads to the next cut; else one junction piece at the face; or an
 *         Obstruction refusal at the face when a train stands on it or the cell cannot take the piece.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23CB
 */
TBMSupportPlan PrepareTBMFaceSupport(const TBMProject &project, const ExcavationJob &job);



/**
 * Read the canonical edge-connected track path used by native cut/support planning.
 * @param job Canonical job whose accepted route and commissioning anchor supply every position.
 * @param positions Number of route positions to include, bounded by the actual route.
 * @param[out] ends Path index of each included cutter position.
 * @return Actual edge-adjacent path from the housing, or empty for invalid cross-layer/diagonal start geometry.
 */
std::vector<TileIndex> GetTBMWorkRailPath(const ExcavationJob &job, size_t positions, std::vector<size_t> &ends);

/**
 * Lay prepared support pieces and register their metadata; cannot fail once prepared.
 * Called only inside TBMCutParticipant::Commit, before geology opens the cut's cells.
 * @param project Assigned project.
 * @param plan Successful preparation for the current cut.
 */
void PlaceTBMSupport(const TBMProject &project, const TBMSupportPlan &plan);

/**
 * Whether a retreat cell carries rail this project's consist can use: its company's pair housing, or railway track of
 * its company (laid support or ordinary track).
 * @param project Assigned project.
 * @param tile Retreat cell.
 * @return Whether the cell supports the consist.
 */
bool IsTBMSupportUsable(const TBMProject &project, TileIndex tile);

/**
 * Whether a vehicle class may run on a tile. Tiles without support metadata allow everything here; the movement
 * check that applies this to actual train parts is Group 14.
 * @param tile Any tile.
 * @param vehicle_class Class of the moving vehicle.
 * @return Whether the class is allowed.
 */
bool IsTBMSupportTraversable(TileIndex tile, TBMSupportClass vehicle_class);

/**
 * Stage loaded support rows with the staged projects; AcceptLoadedTBMProjects validates and publishes both.
 * @param rows Strictly increasing loaded tiles.
 */
void StageLoadedTBMSupport(std::vector<TBMSupportTile> rows);

/**
 * Validate staged support rows against the loaded map and publish them; called by AcceptLoadedTBMProjects.
 * @param next_project Project allocation cursor; every row was laid by an earlier project.
 * @return False when a row refuses; the live registry is then left empty. No staged rows publish an empty registry.
 */
bool AcceptLoadedTBMSupport(TBMProjectID next_project);

/** Clear the support registry and its load staging; called by ResetTBMProjectState. */
void ResetTBMSupportState();

/*
 * Temporary support rules (Group 9).
 * - In use: the laying project still exists and is unfinished. Ordinary rail tools may then not remove, add to,
 *   signal or re-type the tile, so a cutter can never be stranded. Company removal is the one exception: it clears the
 *   track and the metadata leaves with it.
 * - Released: once the project has finished, been cancelled or gone, ordinary removal works and drops the metadata
 *   of each removed piece, and CmdConvertTBMSupport turns the tile into permanent track for its conversion price.
 * - Traversal: only parts of the owner's trains with an allowed class may run on support, at most at its speed
 *   limit. The speed limit applies at movement now (Train::GetCurrentMaxSpeed); Group 14 applies the class check to
 *   moving parts.
 */

/**
 * @param tile Any tile.
 * @return Whether actual occupancy, a geological claim or an unfinished project's retreat still needs this support.
 */
bool IsTBMSupportInUse(TileIndex tile);

/**
 * Check an ordinary rail tool's change to a tile against the support rules.
 * @param tile Tile the tool changes.
 * @param flags Command flags; company removal (Bankrupt) is always allowed.
 * @return Success, or the in-use refusal.
 */
CommandCost CheckTBMSupportChange(TileIndex tile, DoCommandFlags flags);

/**
 * Drop the metadata of one removed piece; the entry goes once no laid piece is left.
 * @param tile Tile whose piece was removed.
 * @param track Removed piece.
 */
void ReleaseTBMSupportPiece(TileIndex tile, Track track);

/**
 * Quote replacement of a completed tunnel's vacated terminal support by a real rail portal.
 * Requires the whole actual cutter stopped on the retained approach side; all
 * other work claims, route users, occupancy and reservations retain their guards.
 * @param tile Proposed rock-layer endpoint containing this project's straight support.
 * @param shaft Direction into the proposed shaft from that endpoint.
 * @param railtype Requested portal gauge, which must match the existing support.
 * @return Exact conversion/removal charge or an unchanged, actionable refusal.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group9
 */
CommandCost CheckTBMExitHandover(TileIndex tile, DiagDirection shaft, RailType railtype);

/**
 * Complete the prevalidated terminal-support handover after the reciprocal portal has published.
 * @param tile The same endpoint, now a real owned portal of the quoted gauge.
 * @return Nothing; remove only its old support metadata and old single rail infrastructure count.
 */
void CompleteTBMExitHandover(TileIndex tile);

/**
 * Follow a change of the track's owner (company merger).
 * @param tile Tile whose owner changes.
 * @param new_owner New owner.
 */
void ChangeTBMSupportOwner(TileIndex tile, Owner new_owner);

/**
 * Move every project whose exchange is at a removed depot bay back to its Source bay (its portal key), or to no bay
 * when its portal is gone; the project then waits MissingExchange (Plan 5 Group 11.3). Lifecycle owner: no actor check.
 * @param bay Removed rearrangement tile.
 */
void ReleaseTBMProjectExchangeBay(TileIndex bay);

/**
 * Apply a company merger or removal to the TBM projects, their excavation jobs and their exchange records (common
 * lifecycle contract, Plan 5 Group 11.4). Called after the company's vehicles moved or were deleted, before its tiles.
 * A merger moves each project and job to the buyer. A removal erases the company's projects (identities are never
 * reused) and their exchange records, and leaves their jobs as explicit orphans.
 * @param old_owner Company that goes away.
 * @param new_owner Buyer, or INVALID_OWNER when the company is removed.
 */
void ChangeTBMOwnership(Owner old_owner, Owner new_owner);

/**
 * Common deletion hook of the TBM owners (Plan 4/5, Plan 15 16.1), called from Vehicle::PreDestructor for every train
 * part on every deletion path but pool cleaning. The exchange store forgets the part first; a deleted cutter is then
 * cleared from its project (MissingCutter) and from its Commissioning pair at once, so a later train that reuses the
 * ID never inherits the project, its consist lock or its pair.
 * @param part Train part about to be deleted, still linked in its chain.
 */
void OnTBMTrainPartDeleted(const Train &part);

/**
 * Station deletion hook of the TBM owners (Plan 4 Group 17, Plan 15 16.1), called from Station::~Station on every
 * deletion but pool cleaning. A project whose origin it was loses it at once (MissingOrigin), and so does a pair
 * naming it, so a later station that reuses the ID never becomes an origin and the pair's record still loads.
 * @param station Station about to be deleted.
 */
void OnTBMStationDeleted(StationID station);

/**
 * Traffic class of one train part on support.
 * @param part Any train part.
 * @return Cutter for the machinery of a project consist, SpoilWagon for its dedicated wagons; nothing for other
 *         trains (Plan 5 haulers will declare Hauler).
 */
std::optional<TBMSupportClass> GetTBMSupportClass(const Train &part);

/**
 * Whether a train part may run on a tile under the support rules; tiles without support allow every part.
 * @param part Moving train part.
 * @param tile Tile it enters.
 * @return Whether the part is the owner's and of an allowed class.
 */
bool MayTBMSupportCarry(const Train &part, TileIndex tile);

/*
 * Work corridor (Group 14). A project's corridor is its supported return path back to the housing plus every support
 * tile it laid. Ordinary freight is kept out of it twice: path finding does not follow support for a train with a
 * part the support may not carry, and the rail tile refuses such a train when its moving front enters, so neither a
 * route nor force-proceed can take it in. The pair housing itself stays sealed by the portal owner while it is
 * Commissioning. Support is released only when its project ends, and released support under a train is still not
 * removable, so withdrawal must come first.
 */

/**
 * @param project Assigned project.
 * @return Its retreat path and laid support tiles, sorted and unique.
 */
std::vector<TileIndex> GetTBMWorkCorridor(const TBMProject &project);

/** Derived next meeting; never serialized or owned by a viewport. */
enum class TBMMeetingKind : uint8_t {
	Bay, ///< Original source magazine or actual underground depot.
	Face, ///< Validated stand behind a live cutter at its eligible face.
};

/** Derive Source-only Face versus Bay from original references, phase, body and hauler clearance.
	* @param project Actual native project; a copy may ask about a phase without publishing it.
	* @return Face only for a safe live Source project: working at current/previous approach,
	* or returning with due spoil exactly at the face (previous approach only for an already-inside hauler).
	* Invalid references, a real underground-depot bay, insufficient whole-hauler room or a missing stand return Bay.
	* This reads no camera and changes no geometry, phase, train, admission or RNG state.
	*/
TBMMeetingKind GetTBMMeetingKind(const TBMProject &project);

/** Observe readiness without performing a transfer or activating a controller edge.
	* @param project Original native project in a face wait or return phase.
	* @return Whether its meeting is Face and actual spoil/frozen pass terms require exchange.
	* Stand orientation, whole hauler capacity and dwell remain the transfer owner's independent checks.
	*/
bool IsTBMReadyForFaceTransfer(const TBMProject &project);

/** The tile where the face turn holder's front stops behind the cutter, and its direction of travel there. */
struct TBMFaceStand {
	TileIndex tile = INVALID_TILE; ///< Plain support rail on the work path, one clear tile behind the cutter's rearmost part.
	Trackdir trackdir = Trackdir::Invalid; ///< Laid piece on the stand, in the path's forward direction (towards the cutter).
	/** @param other Stand to compare. @return Whether both fields match. */
	bool operator==(const TBMFaceStand &other) const = default;
};

/**
 * Find the stand behind the cutter: two work-path cells behind its rearmost part, so one clear tile separates the
 * hauler's front from the cutter. Only the path tail is searched, bounded by the consist's part count (I-9).
 * @param project Project whose live cutter and job are read.
 * @return The stand, or nothing when a part stands off the last path cells (in the housing's shaft, on another layer or
 *         further back), when the stand would be the housing, lie in the next cut's sweep or not be plain rail, or when
 *         its tile does not carry the laid path piece.
 */
std::optional<TBMFaceStand> FindTBMFaceStand(const TBMProject &project);

/**
 * The trackdirs a hauler may stand on at a face stand: every piece laid on the stand tile that leaves it on the same
 * side as the stand's own trackdir, towards the cutter. A chamber tile can carry a straight and a diagonal piece at
 * once, and a hauler arrives on whichever its way in reaches (r2df2 Group 6).
 * @param stand Found stand.
 * @return Its trackdirs; they include the stand's own trackdir.
 */
TrackdirBits GetTBMFaceStandTrackdirs(const TBMFaceStand &stand);

/** Shared original live-cutter predicate, also consumed by pure face observations.
 * @param project Actual project record whose company/model/front reference is checked.
 * @return Original live cutter front of that company/model, or nullptr; no repair or saved-state mutation.
 */
const Train *GetLiveTBMCutter(const TBMProject &project);

/** Shared original live-job predicate; completed/cancelled history still anchors safe return.
 * @param project Actual project record whose job/company/model/binding/housing is checked.
 * @return Its original job or nullptr; this reference alone authorizes no new cut.
 */
const ExcavationJob *GetLiveTBMJob(const TBMProject &project);

/** Observe an assigned hauler's actual running parts in the project's paid corridor or either portal end.
 * @param project Original project whose exact assignment and geometry are consumed.
 * @param hauler Actual front identity; unassigned/foreign-project/missing trains do not count.
 * @return Whether any running part is inside; stored Track::Depot parts in a magazine or depot are excluded.
 */
bool IsTBMHaulerInCorridor(const TBMProject &project, VehicleID hauler);

/** Select the deterministic existing occupant without admitting a new hauler or changing its route.
 * @param project Original project whose assigned native trains are observed.
 * @return Lowest actual assigned vehicle ID with a running corridor/portal part, or Invalid.
 */
VehicleID FindTBMHaulerInCorridor(const TBMProject &project);


/**
 * @param project Assigned project.
 * @return Whether any train part stands on a corridor tile; release and cancellation (Group 17) wait for withdrawal.
 */
bool IsTBMCorridorOccupied(const TBMProject &project);

/**
 * Hold work/wait phases and unsafe or paused project travel. Native depot orders retain service travel;
 * after the visit, the controller physically rejoins its face or exchange bay before resuming work.
 * @param front Actual front engine and attached consist.
 * @return Whether both ordinary locomotive substeps must hold every attached part still.
 */
bool IsTBMHoldingTrain(const Train &front);

/**
 * Derive a recoverable work-travel hold from current controller and rail authority.
 * @param front Actual train head; ordinary trains and native depot/service travel retain their normal handling.
 * @return Player pause, lost reference or unusable work target; None means this query imposes no travel hold.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group4
 */
TBMBlockedReason GetTBMTravelHoldReason(const Train &front);

/** Derived pathfinding destination for a cutter travelling on its own canonical support, never an ordinary saved order. */
struct TBMCutterSteeringTarget {
	TileIndex tile = INVALID_TILE; ///< Current face approach or real exchange bay.
	TrackdirBits trackdirs{}; ///< Actual laid face piece in its canonical entry/exit direction, or the bay's required physical door direction.
};

/**
 * Resolve the cutter's next physical destination from its actual project/job/support owners.
 * @param front Ordinary train head; noncutters and ordinary service orders retain existing routing.
 * @return Current canonical travel target, or no override for ordinary trains and held work phases.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 9B
 */
std::optional<TBMCutterSteeringTarget> GetTBMCutterSteeringTarget(const Train &front);

/**
 * Check whether the canonical support path obeys the actual railtype and current sharp-turn preference.
 * @param job Validated, portal-anchored proposal; this query allocates no vehicle, job or track.
 * @return Whether every physical support bend can be traversed under the same rule as the train controller.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 9B
 */
bool CheckTBMSupportRailTurns(const ExcavationJob &job);

/** The first cell where a route's support would be refused (r2df task 7.3). */
struct TBMSupportRouteBlocker {
	TileIndex tile = INVALID_TILE; ///< The refused cell, or INVALID_TILE when the whole route can be supported.
	bool fundable = false; ///< The cell is the company's own permanent track lacking only the needed piece, which it can build.
};

/**
 * Find the first cell from the job's cursor onwards where a cut could not lay or reuse its support, with the same rule
 * PrepareTBMSupport applies cut by cut (r2df task 7.3). Quotes use it so they never accept a route whose cut is
 * then refused.
 * @param job Portal-anchored proposal or job, with its cursor at the first cut still to be supported.
 * @param project The project that will lay the support; a quote before commissioning passes one that matches no record.
 * @param company Company laying the support.
 * @return The first cell with infrastructure the support may not merge with or replace, if any.
 */
TBMSupportRouteBlocker FindTBMSupportRouteBlocker(const ExcavationJob &job, TBMProjectID project, CompanyID company);

/** What holds a project's cutter for the status line (r2df task 7.5). */
struct TBMObstruction {
	VehicleID vehicle = VehicleID::Invalid(); ///< Train in the way, if a train is.
	TileIndex tile = INVALID_TILE; ///< Existing track or structure in the way, if no train is.
};

/**
 * Name what keeps the cutter from moving on, for display only: a train in the next cut's sweep, a cell the next cut's
 * support may not use, or another train on the pair while the cutter waits in or at its housing.
 * @param project Live project.
 * @return The blocker; both fields invalid when none can be named.
 */
TBMObstruction FindTBMObstruction(const TBMProject &project);

/** Result of one work step at the face (Groups 6-7). */
struct TBMFaceWork {
	TBMBlockedReason wait = TBMBlockedReason::None; ///< Precise limiting reason; accumulated work and cursor are kept.
	bool cut_committed = false; ///< One complete cut committed this step (at most one per tick).
	uint64_t work_added = 0; ///< Geological work ticks added this step.
	uint16_t work_remainder = 0; ///< New fractional work-rate numerator for the project to save.
	Direction heading = Direction::Invalid; ///< Cutting heading after the committed cut, when cut_committed.
	ExcavationCutResult receipt; ///< The committed receipt, when cut_committed.
};

/*
 * Spoil provenance (Group 7.2). Earth and Rock from a cut become new cargo packets with:
 * - source: unknown (Source::Invalid); a project or job ID is not an industry, town or headquarters ID;
 * - loading coordinate (source_xy): the cut's cutter position, the canonical work-site tile;
 * - first station: invalid; the required Surface destination is reached later through Plan 5 and is never pre-stamped;
 * - age and feeder share: zero.
 * Packets go only into dedicated wagons of the matching role, in stable chain order, appended as kept cargo.
 */

/**
 * Actual-train transaction participant for one cut (Groups 6-8): the assigned consist's real wagons, support capacity
 * and company funds. Capture, Validate and IsSupported change nothing. Commit first allocates every cargo packet, then
 * places the support track, appends the spoil and debits the work charge; a refusal leaves the world unchanged.
 */
class TBMCutParticipant final : public ExcavationMaterialParticipant, public ExcavationWorkParticipant {
public:
	/**
	 * Bind the participant to one project.
	 * @param project Assigned project (copied; the store is not referenced).
	 * @param packet_budget Most cargo packets one commit may allocate. Production passes no limit; the pool's own
	 *                      capacity is always checked as well. Tests pass a small budget to prove allocation refusal.
	 */
	explicit TBMCutParticipant(const TBMProject &project, size_t packet_budget = SIZE_MAX) : project(project), packet_budget(packet_budget) {}
	ExcavationMaterialSnapshot Capture() const override;
	bool Validate(const ExcavationMaterialSnapshot &expected, const ExcavationCutResult &delta) const override;
	bool Commit(const ExcavationMaterialSnapshot &expected, const ExcavationCutResult &delta) noexcept override;
	bool IsSupported(const ExcavationJob &job, const ExcavationCutGeometry &cut) const override;
	/** @return Why the last Validate, Commit or IsSupported refused; None after a success. */
	TBMBlockedReason GetRefusal() const { return this->refusal; }

private:
	TBMProject project; ///< Assignment whose consist, job and company take part.
	size_t packet_budget; ///< Allocation ceiling for one commit.
	mutable TBMBlockedReason refusal = TBMBlockedReason::None; ///< Last refusal, for a precise wait.
};

/**
 * Real material snapshot of an assigned consist: free Earth and Rock wagon space, support capacity and company funds.
 * A wagon with a loading reservation offers no space, because spoil may only be appended to a quiet cargo list.
 * @param project Assigned project.
 * @return Snapshot; all zero when the cutter or consist is not live.
 */
ExcavationMaterialSnapshot CaptureTBMMaterial(const TBMProject &project);

/**
 * Whether the frozen pass terms require an exchange now: the job's completed-cut count is a breakpoint and the
 * wagons actually carry spoil. After a real exchange the wagons are empty, so the same breakpoint is not due again.
 * @param project Assigned project.
 * @return Whether ReturnForExchange is due before any further cut.
 */
bool IsTBMExchangeDue(const TBMProject &project);

/**
 * Advance the face once (Groups 6, 7 and 10): check that the next segment advances one tile within 45 degrees of the
 * saved cutting heading, add rate-limited work up to the cut's requirement, then, at the completed-work boundary,
 * commit exactly that cut through the geology owner with the actual wagons, support and funds, or report the precise
 * wait. At most one cut commits per call. It does not change the project record itself; the caller saves the returned
 * work remainder and, after a commit, the new heading. Called by AdvanceTBMWork only while unblocked in Bore.
 * @param project Live project.
 * @return What happened; a refusal keeps geology, wagons and funds unchanged and keeps the accumulated work.
 */
TBMFaceWork AdvanceTBMFaceWork(const TBMProject &project);

/*
 * Recurring cost (Group 12). Every controller step adds exactly one tick to one saved bucket: active while unblocked,
 * idle while held for any reason (stopped, broken down, servicing, paused, a work wait or a lost reference). At each
 * economy day the buckets become money at the model's yearly active and idle rates, whatever the state is at that
 * moment, and the fraction is carried in cost_remainder. While a cutter has a project its ordinary running cost is
 * replaced by this accrual, and the per-cut work charge belongs to the cut transaction alone, so nothing is charged
 * twice. A company in bankruptcy cuts nothing new; its wagons, cavities and retreat stay as they are.
 */

/** One daily conversion of a project's duration buckets into money. */
struct TBMRecurringCharge {
	Money cost = 0; ///< Whole money to debit now.
	uint64_t remainder = 0; ///< Fraction carried to the next day, below one year of ticks.
};

/**
 * Convert the buckets without changing anything.
 * @param project Project whose buckets are read.
 * @return Money for the buckets plus the carried fraction; zero for an unknown model.
 */
TBMRecurringCharge ComputeTBMRecurringCharge(const TBMProject &project);

/**
 * Debit one day's recurring cost: empty both buckets, keep the fraction, debit the company as train running cost.
 * Called once per economy day for the cutter, from Train::OnNewEconomyDay.
 * @param id Project to charge.
 * @return Money debited (zero for an unknown project).
 */
Money ChargeTBMProjectCosts(TBMProjectID id);

/** @return Stable digest over every saved project field and the allocation cursor. */
uint64_t ComputeTBMProjectDigest();

/** Clear the live store and unpublished load staging when the world initializes. */
void ResetTBMProjectState();

/**
 * Keep structurally checked loaded projects unpublished until vehicles have been rebuilt after load.
 * @param projects Strictly increasing loaded records.
 * @param next_id Saved allocation cursor.
 */
void StageLoadedTBMProjects(std::vector<TBMProject> projects, TBMProjectID next_id);

/**
 * Validate staged projects against the rebuilt vehicles, geology and portal work, then publish them.
 * @return False when staged projects refuse; the live store is then left empty.
 */
bool AcceptLoadedTBMProjects();

/** Clear unpublished project save transport (saveload/tbm_sl.cpp) when the world initializes. */
void ResetTBMProjectLoadStaging();

/** Check the copied project family of a save preview without touching live owners. */
void ValidateTBMProjectLoadPreview();

#endif /* TBM_H */
