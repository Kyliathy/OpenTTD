/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange.h Controlled wagon exchange between a TBM consist and its assigned haulers: the bay where both
 * stand still, the hauler assignments that queue for it, the wagon sets they carry, the pure exchange proposal, the
 * one atomic commit, the hauler's saved collect/origin/dispose/return cycle, the Surface origin leg of unloading and
 * the published logistics waits. The store is saved with its identity cursors and validated after load (Group 10),
 * and kept consistent through sale, refit, autoreplace, deletion, bay removal and company changes (Group 11).
 * Real wagon units move between trains with their own cargo; no packet is copied or transferred.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Groups 1-11 (Public Handoff Contract)
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-j-lifecycle-design.md (Group 11 lifecycle)
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-h-exchange-design.md
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-i-receiver-design.md (origin leg, Plan 5 Groups 8-9)
 */

#ifndef TBM_EXCHANGE_H
#define TBM_EXCHANGE_H

#include "tbm.h"
#include "station_type.h"
#include "strings_type.h"
#include "vehicle_type.h"

/** Stable exchange refusal. Each value names the first failed rule; the world is unchanged after any refusal. */
enum class TBMExchangeError : uint8_t {
	None, ///< Every rule passed.
	InvalidProject, ///< No live, unfinished project of the actor's company.
	InvalidBay, ///< The bay is not the project's source portal or a valid excavated company depot bay.
	NotAtBay, ///< A participant is moving, crashed, foreign or not wholly on the bay's rearrangement tile.
	InvalidHauler, ///< The hauler is missing, not a front engine of the company, or a TBM cutter itself.
	Reserved, ///< The hauler, or one of its empty wagons, already belongs to another assignment or set.
	NotAssigned, ///< The hauler has no assignment for this project.
	OccupiedBay, ///< Another admitted hauler of the same bay has the earlier turn.
	WrongWagons, ///< A wagon of the hauler's set is no longer an empty spoil unit of that hauler.
	InvalidConsist, ///< A chain does not have the unit boundaries the exchange needs.
	UnstableCargo, ///< Loaded cargo is reserved for loading, transfer or delivery instead of being kept.
	NoSpoil, ///< The cutter consist carries no spoil, so there is nothing to collect.
	InsufficientEmpties, ///< The hauler's empty Earth or Rock capacity is below the project's frozen pass terms.
	CapacityLoss, ///< The rearranged consist would hold less of some cargo than it carries, so cargo would be lost.
	Coupling, ///< The shared attachment rules refuse a final chain (length or coupling callback); see the message.
	MissingOrigin, ///< The origin is not the company's Surface railway station.
	NoReceiver, ///< The origin does not accept both Earth and Rock, so loaded spoil would have nowhere to go.
	Limit, ///< The assignment or set store, or its identities, are full.
};

/** Where the two consists stand for an exchange. */
enum class TBMExchangeBayKind : uint8_t {
	Source, ///< The project's Surface source portal, where the cutter consist was assembled.
	Depot, ///< A company rail depot on a rock layer with an excavated, straight approach.
};

/** A resolved bay: its rearrangement tile and, for a depot, the approach it was validated with. */
struct TBMExchangeBay {
	TBMExchangeError error = TBMExchangeError::InvalidBay; ///< None for a usable bay.
	TBMExchangeBayKind kind = TBMExchangeBayKind::Source; ///< Source portal or depot.
	TileIndex tile = INVALID_TILE; ///< The one tile every part of both consists must stand on.
	std::vector<TileIndex> approach; ///< Depot approach from its door outwards; empty for the source bay.
};

/**
 * Resolve a candidate bay for a project without changing anything.
 * The source bay is the project's own portal key. A depot bay is a company rail depot on Underground or Deep
 * Underground: the depot and an approach of ceil(cutter consist length / TILE_SIZE) tiles in front of its door are
 * excavated plain rail of the company, on the door axis, in rail types the cutter consist can run on.
 * @param project Live project; its cutter gives the approach length and rail compatibility.
 * @param tile Candidate rearrangement tile.
 * @return The bay, or InvalidBay.
 */
TBMExchangeBay ResolveTBMExchangeBay(const TBMProject &project, TileIndex tile);

/**
 * @param project Live project.
 * @return The project's current bay (its saved exchange_bay), resolved as by ResolveTBMExchangeBay.
 */
TBMExchangeBay GetTBMExchangeBay(const TBMProject &project);

/**
 * Admit one participant to a bay: the company's front engine, not crashed, not moving (cur_speed 0) and with every
 * part on the bay's rearrangement tile in the depot-like Track::Depot state. The player's stop flag is not required:
 * the controller and the assignment hold both trains, and the exchange itself is one synchronous step.
 * @param head Participating front engine.
 * @param bay Resolved bay.
 * @param company Company both trains must belong to.
 * @return None, InvalidBay, NotAtBay or InvalidHauler.
 */
TBMExchangeError CheckTBMExchangeParticipant(const Train &head, const TBMExchangeBay &bay, CompanyID company);

/** Identity of one wagon set; it never doubles as a vehicle, project or assignment ID. */
using TBMWagonSetID = StrongType::Typedef<uint32_t, struct TBMWagonSetIDTag, StrongType::Compare>;

/** Where a wagon set is in the haulage loop (Plan 5 Group 7.1). */
enum class TBMWagonSetState : uint8_t {
	Available, ///< Empty and carried by no assignment, for example after a release.
	Assigned, ///< Serving the cutter consist, empty or filling.
	Loaded, ///< On a hauler with the spoil it collected at an exchange.
	ReturningEmpty, ///< On a hauler, empty, on its way to the bay.
	End, ///< End marker.
};

/**
 * One stable, ordered group of spoil wagon units that travels between the cutter and haulers as a whole. Its identity
 * is independent of the train that currently carries it; each wagon is in at most one set.
 */
struct TBMWagonSet {
	TBMWagonSetID id{UINT32_MAX}; ///< Store identity.
	TBMProjectID project{UINT32_MAX}; ///< Project the set serves.
	std::vector<VehicleID> wagons; ///< Unit heads in chain order.
	TBMWagonSetState state = TBMWagonSetState::Available; ///< Loop position.
	bool visited_origin = false; ///< A Loaded set reached the project's Surface origin on its current loaded cycle.
	uint64_t revision = 1; ///< Advances on every change of this record.
	/** @param other Set to compare. @return Whether every field matches. */
	bool operator==(const TBMWagonSet &other) const = default;
};

/** Saved phase of a hauler's cycle (Plan 5 Group 6.2). */
enum class TBMHaulerPhase : uint8_t {
	ReturnEmpties, ///< Bringing an empty set to the bay; also the first phase of a new assignment.
	AwaitExchange, ///< Wholly at the bay with the empty set, waiting for the cutter and the bay's turn.
	CollectLoaded, ///< Carrying the loaded set it just took, still at the bay.
	VisitOrigin, ///< On its way to the project's Surface origin station with the loaded set.
	DisposeOrTransfer, ///< At or after the origin, until disposal or an onward transfer has emptied the set.
	End, ///< End marker.
};

/** Recoverable reason a hauler's cycle holds; cargo is never touched while it holds. */
enum class TBMHaulerWait : uint8_t {
	None, ///< Not held.
	Stopped, ///< The player stopped the hauler away from the bay.
	Crashed, ///< The hauler is crashed.
	LostProject, ///< The project record is gone.
	LostOrigin, ///< The project's origin is no longer the company's Surface railway station.
	WrongWagons, ///< A wagon of the carried set is no longer in the hauler.
	NoEmpties, ///< The exchange refused because the empties cannot cover the next pass.
	OccupiedBay, ///< Another admitted hauler of the same bay has the earlier turn.
	SaturatedReceiver, ///< At the origin, no receiver there has room for the carried spoil (Plan 5 Group 9.3).
	BlockedLine, ///< Away from its bay, the hauler cannot find a route on its ordinary orders (its pathfinder is lost).
	End, ///< End marker.
};

/** Identity of one hauler assignment; it never doubles as a project, vehicle or station ID. */
using TBMExchangeAssignmentID = StrongType::Typedef<uint32_t, struct TBMExchangeAssignmentIDTag, StrongType::Compare>;

/**
 * One hauler's service of a project (Plan 5 Groups 1.2 and 6.1): the secondary locomotive, the project's bay it queues
 * for, the wagon set it carries and its saved cycle. A project may have several; each hauler has at most one. The
 * accepted request order is the identity order, which also orders the bay's turns.
 * Saved since version TBMExchange (Plan 5 Group 10); production commissioning stays guarded until master 6.6.
 */
struct TBMExchangeAssignment {
	TBMExchangeAssignmentID id{UINT32_MAX}; ///< Store identity and accepted request order.
	TBMProjectID project{UINT32_MAX}; ///< Served project.
	VehicleID hauler = VehicleID::Invalid(); ///< Secondary locomotive front engine; never a cutter.
	TileIndex bay = INVALID_TILE; ///< Rearrangement tile this hauler queues for.
	TBMWagonSetID set{UINT32_MAX}; ///< Set the hauler carries; invalid when it carries none.
	TBMHaulerPhase phase = TBMHaulerPhase::ReturnEmpties; ///< Saved cycle phase.
	TBMHaulerWait wait = TBMHaulerWait::None; ///< Current recoverable hold.
	uint64_t phase_ticks = 0; ///< Ticks in the current phase.
	uint32_t exchanges = 0; ///< Completed exchanges.
	uint64_t revision = 1; ///< Advances on every change of this record other than the timer.
	/** @param other Assignment to compare. @return Whether every field matches. */
	bool operator==(const TBMExchangeAssignment &other) const = default;
};

/** Upper bound of live assignments. */
static constexpr size_t MAX_TBM_EXCHANGE_ASSIGNMENTS = MAX_TBM_PROJECTS * 4;
/** Upper bound of live wagon sets. */
static constexpr size_t MAX_TBM_WAGON_SETS = MAX_TBM_EXCHANGE_ASSIGNMENTS * 2;

/**
 * Whether a project takes a hauler at all, whichever train it would be: the project side of AssignTBMHauler.
 * @param project Project to serve.
 * @return None, InvalidProject (missing, foreign to the actor or finished), MissingOrigin or InvalidBay.
 */
TBMExchangeError CheckTBMProjectTakesHauler(TBMProjectID project);

/**
 * Assign a hauler to a project and register its empty spoil units as the set it brings (ReturnEmpties), or adopt the
 * one unowned set its wagons already form, resuming that set's cycle (recovery after a release).
 * @param project Live, unfinished project of the actor's company with its Surface origin station and a valid bay.
 * @param hauler Non-TBM front engine of the same company with no assignment.
 * @param execute Store the assignment; otherwise only check.
 * @return None, InvalidProject, MissingOrigin, InvalidBay, InvalidHauler, Reserved, WrongWagons or Limit.
 */
TBMExchangeError AssignTBMHauler(TBMProjectID project, VehicleID hauler, bool execute);

/**
 * End a hauler's assignment (cancellation); the trains, wagons and packets are not touched. The carried set stays
 * registered without a carrier, so a later assignment of the same hauler recovers it.
 * @param hauler Assigned hauler.
 * @param execute Remove it; otherwise only check.
 * @return None, or NotAssigned.
 */
TBMExchangeError ReleaseTBMHauler(VehicleID hauler, bool execute);

/**
 * @param project Served project.
 * @return Its first assignment in accepted request order, or nullptr. References expire at the next store change.
 */
const TBMExchangeAssignment *GetTBMExchangeAssignment(TBMProjectID project);

/**
 * @param hauler Front engine.
 * @return Its assignment, or nullptr.
 */
const TBMExchangeAssignment *GetTBMExchangeAssignmentForHauler(VehicleID hauler);

/**
 * @param bay Rearrangement tile.
 * @return Its first assignment in accepted request order, or nullptr.
 */
const TBMExchangeAssignment *GetTBMExchangeAssignmentAtBay(TileIndex bay);

/**
 * The bay's turn: the earliest assignment of this bay whose hauler is admitted there and does not carry a Loaded set.
 * @param bay Rearrangement tile.
 * @return That assignment, or nullptr.
 */
const TBMExchangeAssignment *GetTBMExchangeTurn(TileIndex bay);

/** @return Every assignment in identity order. */
std::vector<TBMExchangeAssignment> CaptureTBMExchangeAssignments();

/**
 * @param id Set identity.
 * @return The set, or nullptr.
 */
const TBMWagonSet *GetTBMWagonSet(TBMWagonSetID id);

/**
 * @param wagon Unit head.
 * @return The set it belongs to, or nullptr.
 */
const TBMWagonSet *GetTBMWagonSetForWagon(VehicleID wagon);

/** @return Every set in identity order. */
std::vector<TBMWagonSet> CaptureTBMWagonSets();

/** Forget every assignment and set (new game, load, test teardown). */
void ResetTBMExchangeState();

/** The participants of one exchange: a project and one of its assigned haulers. */
struct TBMExchangeRequest {
	TBMProjectID project{UINT32_MAX}; ///< Project whose cutter consist gives up its spoil.
	VehicleID hauler = VehicleID::Invalid(); ///< Assigned hauler that brings the empties.
};

/**
 * The complete before/after description of one exchange (Plan 5 Group 2). Lists hold real IDs: every part of each
 * chain in order before and after, and the moved unit heads. It is a view; it owns no vehicle or cargo.
 */
struct TBMExchangeProposal {
	TBMExchangeError error = TBMExchangeError::InvalidProject; ///< None when the exchange may be committed.
	StringID message = INVALID_STRING_ID; ///< Shared attachment refusal text, for Coupling.
	TileIndex bay = INVALID_TILE; ///< Rearrangement tile of both trains.
	VehicleID cutter = VehicleID::Invalid(); ///< Cutter front engine; it stays the cutter consist's head.
	VehicleID hauler = VehicleID::Invalid(); ///< Hauler front engine; it stays the hauler's head.
	std::vector<VehicleID> loaded_units; ///< Loaded unit heads, cutter to hauler, in cutter chain order.
	std::vector<VehicleID> empty_units; ///< Empty unit heads of the hauler's set, hauler to cutter, Earth then Rock.
	std::vector<VehicleID> original_cutter; ///< Every part of the cutter consist before.
	std::vector<VehicleID> original_hauler; ///< Every part of the hauler before.
	std::vector<VehicleID> proposed_cutter; ///< Every part of the cutter consist after.
	std::vector<VehicleID> proposed_hauler; ///< Every part of the hauler after.
	uint64_t earth_t = 0; ///< Earth tonnes moving to the hauler.
	uint64_t rock_t = 0; ///< Rock tonnes moving to the hauler.
	uint64_t empty_earth_capacity_t = 0; ///< Earth capacity the empties give the cutter consist.
	uint64_t empty_rock_capacity_t = 0; ///< Rock capacity the empties give the cutter consist.
	/** @param other Proposal to compare. @return Whether every field matches. */
	bool operator==(const TBMExchangeProposal &other) const = default;
};

/**
 * Describe an exchange from the actual chains, without the trial rearrangement. Checks the project, the hauler's
 * assignment and the bay's turn, admission of both trains, unit boundaries, kept cargo, the hauler's set and the
 * empties' capacity against the frozen pass terms (not needed after the final cut).
 * @param request Participants.
 * @return The proposal; error is the first failed rule.
 */
TBMExchangeProposal DescribeTBMWagonExchange(const TBMExchangeRequest &request);

/**
 * The public estimate: DescribeTBMWagonExchange, then the trial rearrangement through the shared attachment and
 * capacity rules. Both chains, caches, groups, orders, sets and the random seed are exactly restored.
 * @param request Participants.
 * @return The proposal; None means ExecuteTBMWagonExchange will succeed now.
 */
TBMExchangeProposal ValidateTBMWagonExchange(const TBMExchangeRequest &request);

/**
 * Commit one exchange: the loaded units couple to the hauler and the hauler's empty set to the cutter consist, in one
 * step that cannot fail after validation. Wagon IDs and their cargo stay together. The cutter's units become the
 * hauler's Loaded set, the empties become the cutter's Assigned set, and the hauler moves on to CollectLoaded.
 * @param request Participants.
 * @return The executed proposal, or the validation refusal with nothing changed.
 */
TBMExchangeProposal ExecuteTBMWagonExchange(const TBMExchangeRequest &request);

/**
 * Origin and route admission for loaded spoil (Plan 5 Group 4.2): the project's origin is a company-owned Surface
 * railway station, and the station accepts both Earth and Rock, which only real receivers in its catchment give.
 * @param project Live project.
 * @return None, MissingOrigin or NoReceiver.
 */
TBMExchangeError CheckTBMOriginAdmission(const TBMProject &project);

/** The facts one hauler step reads; observed from the actual trains and owners, never from the previous phase. */
struct TBMHaulerInputs {
	bool project_live = false; ///< The project exists; a finished one still owns its final return's spoil.
	bool origin_live = false; ///< The project's origin is the company's Surface railway station.
	bool crashed = false; ///< The hauler is crashed.
	bool stopped = false; ///< The player's stop flag is set.
	bool at_bay = false; ///< The hauler is admitted at its bay.
	bool at_origin = false; ///< The hauler's head stands on a tile of the project's origin station.
	bool set_aboard = true; ///< Every wagon of the carried set is a unit of the hauler.
	bool set_empty = true; ///< No wagon of the carried set holds or expects cargo.
	bool cutter_waiting = false; ///< The project's cutter waits for empties (WaitForEmpties).
	bool exchange_finished = false; ///< Parked or finished work needs no further empties; actual haulers leave its bay for the origin.
	bool lost = false; ///< The hauler's pathfinder is lost.
	bool receiver_room = true; ///< At the origin, some receiver there has room for spoil the set carries.
};

/** Result of one pure hauler step. */
struct TBMHaulerStep {
	TBMHaulerPhase phase = TBMHaulerPhase::ReturnEmpties; ///< Next phase.
	TBMHaulerWait wait = TBMHaulerWait::None; ///< Next hold.
	bool exchange = false; ///< The step wants the exchange attempted now.
	bool visited_origin = false; ///< The step records the origin visit on the carried set.
};

/**
 * @param assignment Assignment to observe.
 * @return Its observed inputs; all false when the hauler is missing.
 */
TBMHaulerInputs ObserveTBMHaulerInputs(const TBMExchangeAssignment &assignment);

/**
 * Decide one hauler step without changing anything. Holds come first (lost project or origin, crash, missing set
 * wagons, the player's stop away from the bay, a lost route away from the bay). Edges: ReturnEmpties to AwaitExchange
 * once wholly at the bay with the set empty; AwaitExchange asks for the exchange while the cutter waits; CollectLoaded
 * to VisitOrigin once it has left the bay; VisitOrigin to DisposeOrTransfer at the origin, recording the visit;
 * DisposeOrTransfer to ReturnEmpties once the set is empty, holding SaturatedReceiver at the origin while no receiver
 * there has room.
 * @param assignment Current record.
 * @param inputs Observed facts.
 * @return The step.
 */
TBMHaulerStep StepTBMHaulerPhase(const TBMExchangeAssignment &assignment, const TBMHaulerInputs &inputs);

/**
 * One simulation step of a hauler's cycle (once per hauler tick, from Train::Tick): observe, step, and perform the
 * exchange when the step asks for it and the bay gives this hauler its turn.
 * @param id Assignment to advance.
 * @return None, or NotAssigned for an unknown assignment.
 */
TBMExchangeError AdvanceTBMHauler(TBMExchangeAssignmentID id);

/**
 * Whether ordinary movement must hold this train: an assigned hauler waiting wholly at its bay (AwaitExchange)
 * stays there until the exchange or a release, whatever its orders say.
 * @param front Front engine.
 * @return Whether ordinary movement must hold it.
 */
bool IsTBMHaulerHeld(const Train &front);

/**
 * Whether an assigned hauler may use the magazine of its Source bay at a portal key (R2MLVS3MP-M decision 3).
 * While its project is unfinished it is admitted in every phase, so it can always leave the bay it stands in. Once
 * the project is finished it is admitted only while a part of it still stands on the key, so it can take the last
 * load out but is never drawn back in. It never reaches the rock end.
 * @param front Front engine of the train.
 * @param tile Portal key.
 * @return True when the train's assignment names this tile as its project's Source bay, under the rule above.
 */
bool IsTBMHaulerAdmittedAtBay(const Train &front, TileIndex tile);

/**
 * Whether a project train standing wholly in its magazine may leave it now (R2MLVS3MP-M decision 2): the cutter only
 * to travel to its face, a hauler once it has no exchange to wait for, or at any time once its project is finished.
 * Any other train stays put.
 * @param front Front engine.
 * @return Whether it may start to leave.
 */
bool MayTBMTrainLeaveMagazine(const Train &front);

/**
 * Whether a train that reaches a line end stops there instead of reversing (R2MLVS3MP-M decisions 4 and 5): a
 * project's cutter always, because its controller chooses its way; a hauler in front of its busy bay, so it waits
 * there instead of driving off.
 * @param front Front engine.
 * @param next Tile beyond the line end.
 * @return Whether the train waits at the line end.
 */
bool IsTBMWaitingAtLineEnd(const Train &front, TileIndex next);

/**
 * Where the pathfinder steers an assigned hauler instead of its current order (R2MLVS3MP-M decision 6). While it
 * returns empties its destination is its validated Source magazine or native underground depot, entered through the door; its orders are never written, so one
 * order to the origin carries the whole loop.
 * @param front Front engine.
 * @return The admitted Source bay or unfinished project's validated real depot during empty return, otherwise INVALID_TILE.
 */
TileIndex GetTBMHaulerSteeringTarget(const Train &front);

/** Why a project's spoil logistics wait (Plan 5 Group 9.3); a derived view, never saved. */
enum class TBMLogisticsWait : uint8_t {
	None, ///< Nothing waits.
	NoHauler, ///< The cutter returns or waits for empties, but no hauler is assigned.
	NoEmpties, ///< A hauler's empties cannot cover the next pass.
	OccupiedBay, ///< A hauler waits for another hauler's turn at the bay.
	BlockedLine, ///< A hauler cannot find a route on its ordinary orders.
	SaturatedReceiver, ///< A loaded hauler at the origin finds no receiver room for its spoil.
	LostOrigin, ///< The project's origin is no longer the company's Surface railway station.
	End, ///< End marker.
};

/**
 * The most severe logistics wait of a project: a lost origin, then its haulers' saturated receiver, lost route,
 * short empties and occupied bay (in that order, over every hauler), then a cutter that needs a hauler it lacks.
 * @param project Live project.
 * @return The wait, or None.
 */
TBMLogisticsWait GetTBMLogisticsWait(const TBMProject &project);

/**
 * Whether new excavation must pause because spoil already loaded cannot move on: a hauler of the project waits for a
 * saturated receiver, a lost route or a lost origin. Returning, exchanging and hauling go on.
 * @param project Live project.
 * @return Whether the face must not start more work.
 */
bool IsTBMLogisticsStalled(const TBMProject &project);

/**
 * Whether a vehicle part's cargo may be staged for unloading at a station under the Surface origin leg (Plan 5 Group
 * 8.1). A cutter never unloads spoil itself. A Loaded set's spoil leaves only from the hauler carrying that set, and
 * only at the project's origin or after visiting it. Any other cargo, and spoil outside a Loaded set (already
 * transferred onward from an origin), is unrestricted.
 * @param front Front of the unloading vehicle.
 * @param part One of its parts.
 * @param station Station being unloaded at.
 * @return Whether the part may be staged as usual; otherwise its cargo stays aboard.
 */
bool MayTBMSpoilLeaveVehicle(const Vehicle &front, const Vehicle &part, StationID station);

/** @return The next assignment identity; any value above zero means assignments were ever made (save family). */
uint32_t GetNextTBMExchangeAssignmentID();

/** @return The next wagon set identity. */
uint32_t GetNextTBMWagonSetID();

/**
 * Hold structurally checked loaded assignments and sets until projects and vehicles are rebuilt (Plan 5 Group 10).
 * @param assignments Loaded assignments in identity order.
 * @param next_assignment Loaded assignment cursor.
 * @param sets Loaded sets in identity order, each with its ordered wagons.
 * @param next_set Loaded set cursor.
 */
void StageLoadedTBMExchange(std::vector<TBMExchangeAssignment> assignments, uint32_t next_assignment, std::vector<TBMWagonSet> sets, uint32_t next_set);

/**
 * Publish the staged store after AfterLoadGame has rebuilt projects and vehicles. Every set names its project and
 * live wagon units of that company, each wagon in one set only, and the cutter's working set rides on the cutter.
 * Every assignment names its project, one live non-cutter front engine of that company, and at most one carried
 * set that no other assignment carries. Nothing is repaired: any mismatch refuses and publishes nothing.
 * @return Whether the store was accepted; nothing staged also succeeds.
 */
bool AcceptLoadedTBMExchange();

/** @return Diagnostic digest of every assignment and set field, in identity order. */
uint64_t ComputeTBMExchangeDigest();

/** Clear unpublished exchange transport (saveload owner). */
void ResetTBMExchangeLoadStaging();

/** Check the copied exchange family of a save preview without touching live owners (saveload owner). */
void ValidateTBMExchangeLoadPreview();

/**
 * Keep the store free of a deleted train part (Plan 5 Group 11, every deletion path but pool cleaning): a deleted
 * hauler's assignment is released, a deleted wagon leaves its set, an emptied set is erased together with its
 * carrier's assignment, and a deleted cutter's Assigned set becomes Loaded (spoil aboard) or Available.
 * Called from Vehicle::PreDestructor, while the part is still linked in its chain.
 * @param part Train part about to be deleted.
 */
void OnTBMExchangeTrainPartDeleted(const Train &part);

/**
 * Drop every assignment and set of an erased project (company removal, Plan 5 Group 11.4).
 * @param project Erased project.
 */
void ForgetTBMExchangeProject(TBMProjectID project);

/**
 * @param project Project to retire or erase.
 * @return Whether no hauler serves the project and no set of it carries spoil, so erasing it strands nothing (Plan 4
 *         Group 17.2).
 */
bool IsTBMProjectExchangeIdle(TBMProjectID project);

/**
 * Identify actual custody or occupancy preventing final project release.
 * @param project Live project to settle.
 * @return No value when every set is empty and every hauler is safely outside its bay after settlement;
 *         otherwise the blocking native train, or Invalid for a missing custody reference.
 */
std::optional<VehicleID> GetTBMProjectSettlementBlocker(TBMProjectID project);

/**
 * Release all settled hauler assignments as part of an already prepared final project operation.
 * @param project Project whose real empty haulers are outside their bays.
 * @param execute False validates only; true ends assignments without moving or deleting their trains/packets.
 * @return Whether complete settlement passed, leaving everything unchanged on refusal.
 */
bool ReleaseSettledTBMProjectHaulers(TBMProjectID project, bool execute);

/**
 * Admit an ordinary rearrangement (CmdMoveRailVehicle): no part may leave or join the cutter consist of a project.
 * @param src_head Head of the source chain.
 * @param dst_head Head of the destination chain, or nullptr for a new chain.
 * @return Success or STR_ERROR_TBM_CONSIST_LOCKED.
 */
CommandCost CheckTBMTrainRearrangement(const Train &src_head, const Train *dst_head);

/**
 * Admit an ordinary sale (CmdSellRailWagon): no part of a cutter consist is sold, and no wagon-set unit that carries
 * spoil, because nothing would preserve that spoil.
 * @param unit First engine part of the unit to sell.
 * @param sell_chain Whether every unit behind it is sold too.
 * @return Success, STR_ERROR_TBM_CONSIST_LOCKED or STR_ERROR_TBM_SPOIL_ABOARD.
 */
CommandCost CheckTBMRailVehicleSale(const Train &unit, bool sell_chain);

/**
 * Admit a manual or automatic refit of one unit: cutter consist parts and wagon-set units keep their capacity.
 * @param unit Any part of the unit to refit.
 * @return Success, STR_ERROR_TBM_CONSIST_LOCKED or STR_ERROR_TBM_SET_REFIT.
 */
CommandCost CheckTBMRefit(const Train &unit);

/**
 * @param part Any train part.
 * @return Whether autoreplace/autorenew must skip its train: a cutter consist part, an assigned hauler or a wagon-set
 *         unit, whose vehicle identities a replacement would renew.
 */
bool IsTBMReplacementLocked(const Train &part);

/**
 * Admit removal of a depot that may be a project's exchange bay (Plan 5 Group 11.3). The depot's own vehicle check
 * runs first, so an occupied bay is already refused before any consist or cargo changes. Bankruptcy and flooding
 * never refuse.
 * @param tile Depot tile.
 * @param flags Removal flags.
 * @return Success, or STR_ERROR_TBM_EXCHANGE_BAY_IN_USE while an unfinished project using the bay has no usable
 *         Source bay to fall back on.
 */
CommandCost CheckTBMExchangeBayRemoval(TileIndex tile, DoCommandFlags flags);

/**
 * After a depot is removed, move every project using it to its Source bay and every hauler queued for it to its
 * project's new bay. No hauler stood in the depot, so only reservations without a train change.
 * @param tile Removed depot tile.
 */
void ReleaseTBMExchangeBay(TileIndex tile);

#endif /* TBM_EXCHANGE_H */
