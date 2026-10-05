/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange.cpp The controlled wagon exchange: bays, hauler assignments and their bay turns, wagon sets, the
 * pure proposal, the atomic commit through the restricted train rearrangement primitive, the hauler cycle, the origin
 * leg of unloading (Group 8.1), the logistics waits that pause new cutting (Group 9.3) and the staged acceptance of a
 * loaded store (Group 10).
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Groups 1-10
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-h-exchange-design.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-i-receiver-design.md
 */

#include "stdafx.h"
#include "tbm_exchange.h"
#include "tbm_cmd.h"
#include "train.h"
#include "train_cmd.h"
#include "station_base.h"
#include "station_map.h"
#include "cargotype.h"
#include "rail_map.h"
#include "layer_portal_map.h"
#include "track_func.h"
#include "company_func.h"
#include "command_func.h"
#include "core/math_func.hpp"
#include "core/backup_type.hpp"
#include "spoil_receiver.h"

#include "table/strings.h"

#include "safeguards.h"

static std::vector<TBMExchangeAssignment> _tbm_exchange_assignments; ///< Live assignments in strictly increasing identity order.
static uint32_t _next_tbm_exchange_assignment = 0; ///< Next identity; identities are never reused within a game.
static std::vector<TBMWagonSet> _tbm_wagon_sets; ///< Live wagon sets in strictly increasing identity order.
static uint32_t _next_tbm_wagon_set = 0; ///< Next set identity.

/**
 * @param id Assignment identity.
 * @return The mutable assignment, or nullptr.
 */
static TBMExchangeAssignment *FindTBMExchangeAssignment(TBMExchangeAssignmentID id)
{
	for (auto &assignment : _tbm_exchange_assignments) {
		if (assignment.id == id) return &assignment;
	}
	return nullptr;
}

/**
 * @param hauler Front engine.
 * @return Its mutable assignment, or nullptr.
 */
static TBMExchangeAssignment *FindTBMExchangeAssignmentForHauler(VehicleID hauler)
{
	for (auto &assignment : _tbm_exchange_assignments) {
		if (assignment.hauler == hauler) return &assignment;
	}
	return nullptr;
}

/**
 * @param id Set identity.
 * @return The mutable set, or nullptr.
 */
static TBMWagonSet *FindTBMWagonSet(TBMWagonSetID id)
{
	for (auto &set : _tbm_wagon_sets) {
		if (set.id == id) return &set;
	}
	return nullptr;
}

/**
 * @param wagon Unit head.
 * @return The mutable set it belongs to, or nullptr.
 */
static TBMWagonSet *FindTBMWagonSetForWagon(VehicleID wagon)
{
	for (auto &set : _tbm_wagon_sets) {
		if (std::ranges::find(set.wagons, wagon) != set.wagons.end()) return &set;
	}
	return nullptr;
}

/**
 * @param set Set identity.
 * @return Whether some assignment's hauler carries it.
 */
static bool IsTBMWagonSetCarried(TBMWagonSetID set)
{
	return std::ranges::any_of(_tbm_exchange_assignments, [set](const TBMExchangeAssignment &assignment) { return assignment.set == set; });
}

TBMExchangeBay ResolveTBMExchangeBay(const TBMProject &project, TileIndex tile)
{
	TBMExchangeBay bay;
	if (tile == INVALID_TILE || !IsValidTile(tile)) return bay;
	// The source bay is the project's own portal key: the company's Surface assembly site of the cutter consist.
	if (tile == project.portal) {
		const auto key = GetLayerPortalPairKey(tile);
		if (!IsLayerPortalTile(tile) || GetTileOwner(tile) != project.company || !key.has_value() || *key != tile) return bay;
		bay.kind = TBMExchangeBayKind::Source;
		bay.tile = tile;
		bay.error = TBMExchangeError::None;
		return bay;
	}
	const Train *front = Train::GetIfValid(project.cutter);
	if (front == nullptr || !front->IsFrontEngine()) return bay; // The approach length and rail types come from the actual cutter consist.
	const MapLayer layer = GetTileLayer(tile);
	// A depot bay is the company's own rail depot on a rock layer, on open ground, in a rail type the consist runs on.
	if ((layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) || !IsRailDepotTile(tile) || GetTileOwner(tile) != project.company) return bay;
	const auto &terrain = GetGeologyWorldState().terrain;
	if (terrain.Get(tile) != ExcavationState::Excavated || !front->compatible_railtypes.Test(GetRailType(tile))) return bay;
	const DiagDirection door = GetRailDepotDirection(tile);
	const Track axis = DiagDirToDiagTrack(door);
	const auto step = TileIndexDiffCByDiagDir(door);
	const uint length = CeilDiv(front->gcache.cached_total_length, TILE_SIZE);
	TileIndex cell = tile;
	// The approach holds the whole cutter consist in a straight line in front of the door, so it can enter and leave.
	for (uint index = 0; index < length; ++index) {
		cell = TileAddWrap(cell, step.x, step.y);
		// Each approach cell is excavated plain company rail on the door axis, in the same layer and a usable rail type.
		if (!IsValidTile(cell) || GetTileLayer(cell) != layer || !IsPlainRailTile(cell) || GetTileOwner(cell) != project.company || !GetTrackBits(cell).Test(axis) || terrain.Get(cell) != ExcavationState::Excavated || !front->compatible_railtypes.Test(GetRailType(cell))) {
			bay.approach.clear();
			return bay;
		}
		bay.approach.push_back(cell);
	}
	bay.kind = TBMExchangeBayKind::Depot;
	bay.tile = tile;
	bay.error = TBMExchangeError::None;
	return bay;
}

TBMExchangeBay GetTBMExchangeBay(const TBMProject &project)
{
	return ResolveTBMExchangeBay(project, project.exchange_bay);
}

TBMExchangeError CheckTBMExchangeParticipant(const Train &head, const TBMExchangeBay &bay, CompanyID company)
{
	if (bay.error != TBMExchangeError::None) return TBMExchangeError::InvalidBay;
	// Only a whole company train takes part; a wagon chain or a foreign train never reaches the exchange.
	if (!head.IsFrontEngine() || head.First() != &head || head.owner != company) return TBMExchangeError::InvalidHauler;
	if (head.vehstatus.Test(VehState::Crashed) || head.cur_speed != 0) return TBMExchangeError::NotAtBay;
	// Every part stands on the one rearrangement tile, as trains do inside a depot; a train half outside is refused.
	for (const Train *part = &head; part != nullptr; part = part->Next()) {
		if (part->tile != bay.tile || part->track != Track::Depot) return TBMExchangeError::NotAtBay;
	}
	return TBMExchangeError::None;
}

/**
 * @param unit Unit head.
 * @return The spoil role its cargo gives it, or Invalid for any other cargo.
 */
static TBMConsistRole GetTBMSpoilRole(const Train &unit)
{
	if (!IsValidCargoType(unit.cargo_type)) return TBMConsistRole::Invalid;
	const auto label = CargoSpec::Get(unit.cargo_type)->label;
	if (label == CargoLabel{"ERTH"}) return TBMConsistRole::EarthWagon;
	if (label == CargoLabel{"ROCK"}) return TBMConsistRole::RockWagon;
	return TBMConsistRole::Invalid;
}

/**
 * @param unit Unit head of a wagon.
 * @param[out] capacity Receives the capacity of all its parts.
 * @return Whether it is a whole empty spoil unit: an Earth or Rock wagon whose every part carries that spoil and holds
 *         or expects no cargo.
 */
static bool IsEmptyTBMSpoilUnit(Train *unit, uint64_t &capacity)
{
	capacity = 0;
	if (unit->IsEngine() || unit->IsArticulatedPart() || GetTBMSpoilRole(*unit) == TBMConsistRole::Invalid) return false;
	const Train *last = unit->GetLastEnginePart();
	for (const Train *part = unit; part != nullptr; part = part->Next()) {
		if (part->cargo.TotalCount() != 0 || part->cargo_type != unit->cargo_type) return false;
		capacity += part->cargo_cap;
		if (part == last) break;
	}
	return true;
}

/**
 * @param project Live project.
 * @return Whether its origin is the company's own Surface railway station.
 */
static bool IsTBMOriginStation(const TBMProject &project)
{
	const Station *origin = Station::GetIfValid(project.origin);
	return origin != nullptr && origin->owner == project.company && IsValidTile(origin->xy) && GetTileLayer(origin->xy) == MapLayer::Surface && origin->facilities.Test(StationFacility::Train);
}

TBMExchangeError AssignTBMHauler(TBMProjectID project_id, VehicleID hauler_id, bool execute)
{
	const TBMProject *project = GetTBMProject(project_id);
	if (project == nullptr || project->company != _current_company || IsTBMProjectFinished(project->phase)) return TBMExchangeError::InvalidProject;
	if (!IsTBMOriginStation(*project)) return TBMExchangeError::MissingOrigin; // Every loaded leg must have its Surface station.
	const auto bay = GetTBMExchangeBay(*project);
	if (bay.error != TBMExchangeError::None) return TBMExchangeError::InvalidBay;
	Train *hauler = Train::GetIfValid(hauler_id);
	// The hauler is a separate whole company train; no cutter, of this or another project, can haul spoil.
	if (hauler == nullptr || !hauler->IsFrontEngine() || hauler->owner != project->company || hauler->vehstatus.Test(VehState::Crashed)) return TBMExchangeError::InvalidHauler;
	if (hauler->index == project->cutter || IsTBMCutterEngine(hauler->engine_type) || GetTBMProjectForCutter(hauler->index) != nullptr) return TBMExchangeError::InvalidHauler;
	if (FindTBMExchangeAssignmentForHauler(hauler_id) != nullptr) return TBMExchangeError::Reserved; // One assignment per hauler.

	/* The hauler's wagons either form one unowned set of this project, which it recovers, or empties for a new set. */
	TBMWagonSet *adopt = nullptr;
	std::vector<VehicleID> fresh;
	for (Train *unit = hauler->GetNextUnit(); unit != nullptr; unit = unit->GetNextUnit()) {
		if (unit->IsEngine()) continue;
		if (TBMWagonSet *set = FindTBMWagonSetForWagon(unit->index); set != nullptr) {
			// A wagon of another project's set, a carried set or the cutter's working set cannot be taken over.
			if (set->project != project_id || IsTBMWagonSetCarried(set->id) || set->state == TBMWagonSetState::Assigned) return TBMExchangeError::Reserved;
			if (adopt != nullptr && adopt != set) return TBMExchangeError::Reserved; // Two sets on one hauler have no single cycle.
			adopt = set;
			continue;
		}
		uint64_t capacity;
		if (IsEmptyTBMSpoilUnit(unit, capacity)) fresh.push_back(unit->index);
	}
	if (adopt != nullptr) {
		// A recovered set must still be wholly aboard, or its cycle cannot be verified.
		for (VehicleID wagon : adopt->wagons) {
			const Train *unit = Train::GetIfValid(wagon);
			if (unit == nullptr || unit->First() != hauler) return TBMExchangeError::WrongWagons;
		}
	}
	const bool new_set = adopt == nullptr && !fresh.empty();
	if (_tbm_exchange_assignments.size() >= MAX_TBM_EXCHANGE_ASSIGNMENTS || _next_tbm_exchange_assignment == UINT32_MAX) return TBMExchangeError::Limit;
	if (new_set && (_tbm_wagon_sets.size() >= MAX_TBM_WAGON_SETS || _next_tbm_wagon_set == UINT32_MAX)) return TBMExchangeError::Limit;
	if (!execute) return TBMExchangeError::None;

	TBMExchangeAssignment assignment;
	assignment.id = TBMExchangeAssignmentID{_next_tbm_exchange_assignment++};
	assignment.project = project_id;
	assignment.hauler = hauler_id;
	assignment.bay = bay.tile;
	if (new_set) {
		TBMWagonSet set;
		set.id = TBMWagonSetID{_next_tbm_wagon_set++};
		set.project = project_id;
		set.wagons = std::move(fresh);
		set.state = TBMWagonSetState::ReturningEmpty;
		assignment.set = set.id;
		_tbm_wagon_sets.push_back(std::move(set));
	} else if (adopt != nullptr) {
		assignment.set = adopt->id;
		// A loaded set resumes towards or at the origin; any other recovered set is brought back empty.
		if (adopt->state == TBMWagonSetState::Loaded) {
			assignment.phase = adopt->visited_origin ? TBMHaulerPhase::DisposeOrTransfer : TBMHaulerPhase::VisitOrigin;
		} else {
			adopt->state = TBMWagonSetState::ReturningEmpty;
			++adopt->revision;
		}
	}
	_tbm_exchange_assignments.push_back(std::move(assignment));
	return TBMExchangeError::None;
}

TBMExchangeError ReleaseTBMHauler(VehicleID hauler, bool execute)
{
	TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(hauler);
	if (assignment == nullptr) return TBMExchangeError::NotAssigned;
	const TBMProject *owner = GetTBMProject(assignment->project);
	// A released project may already be gone; only another company's live project is protected from this actor.
	if (owner != nullptr && owner->company != _current_company) return TBMExchangeError::NotAssigned;
	if (!execute) return TBMExchangeError::None;
	// The set stays registered without a carrier; an empty one becomes Available, a loaded one keeps its spoil and visit.
	if (TBMWagonSet *set = FindTBMWagonSet(assignment->set); set != nullptr && set->state == TBMWagonSetState::ReturningEmpty) {
		set->state = TBMWagonSetState::Available;
		++set->revision;
	}
	const TBMExchangeAssignmentID id = assignment->id;
	std::erase_if(_tbm_exchange_assignments, [id](const TBMExchangeAssignment &candidate) { return candidate.id == id; });
	return TBMExchangeError::None;
}

const TBMExchangeAssignment *GetTBMExchangeAssignment(TBMProjectID project)
{
	for (const auto &assignment : _tbm_exchange_assignments) {
		if (assignment.project == project) return &assignment;
	}
	return nullptr;
}

const TBMExchangeAssignment *GetTBMExchangeAssignmentForHauler(VehicleID hauler)
{
	return FindTBMExchangeAssignmentForHauler(hauler);
}

const TBMExchangeAssignment *GetTBMExchangeAssignmentAtBay(TileIndex bay)
{
	for (const auto &assignment : _tbm_exchange_assignments) {
		if (assignment.bay == bay) return &assignment;
	}
	return nullptr;
}

const TBMExchangeAssignment *GetTBMExchangeTurn(TileIndex bay)
{
	// Identity order is the accepted request order, so the first admitted, unloaded hauler of the bay goes first.
	for (const auto &assignment : _tbm_exchange_assignments) {
		if (assignment.bay != bay) continue;
		const TBMProject *project = GetTBMProject(assignment.project);
		const Train *hauler = Train::GetIfValid(assignment.hauler);
		if (project == nullptr || hauler == nullptr) continue;
		const TBMWagonSet *set = FindTBMWagonSet(assignment.set);
		if (set != nullptr && set->state == TBMWagonSetState::Loaded) continue; // A hauler carrying spoil has had its turn.
		const auto resolved = GetTBMExchangeBay(*project);
		if (resolved.tile == bay && CheckTBMExchangeParticipant(*hauler, resolved, project->company) == TBMExchangeError::None) return &assignment;
	}
	return nullptr;
}

std::vector<TBMExchangeAssignment> CaptureTBMExchangeAssignments()
{
	return _tbm_exchange_assignments;
}

const TBMWagonSet *GetTBMWagonSet(TBMWagonSetID id)
{
	return FindTBMWagonSet(id);
}

const TBMWagonSet *GetTBMWagonSetForWagon(VehicleID wagon)
{
	return FindTBMWagonSetForWagon(wagon);
}

std::vector<TBMWagonSet> CaptureTBMWagonSets()
{
	return _tbm_wagon_sets;
}

/** Loaded store awaiting AcceptLoadedTBMExchange. */
struct TBMExchangeStaging {
	std::vector<TBMExchangeAssignment> assignments; ///< Loaded assignments.
	uint32_t next_assignment = 0; ///< Loaded assignment cursor.
	std::vector<TBMWagonSet> sets; ///< Loaded sets.
	uint32_t next_set = 0; ///< Loaded set cursor.
};
static std::optional<TBMExchangeStaging> _staged_tbm_exchange; ///< Never a second live store.

void ResetTBMExchangeState()
{
	_staged_tbm_exchange.reset();
	ResetTBMExchangeLoadStaging();
	_tbm_exchange_assignments.clear();
	_next_tbm_exchange_assignment = 0;
	_tbm_wagon_sets.clear();
	_next_tbm_wagon_set = 0;
}

/**
 * Append every part of one whole unit (its head and articulated parts) in chain order.
 * @param out List to extend.
 * @param unit Unit head.
 */
static void AppendTBMUnitParts(std::vector<VehicleID> &out, Train *unit)
{
	const Train *last = unit->GetLastEnginePart();
	for (const Train *part = unit; part != nullptr; part = part->Next()) {
		out.push_back(part->index);
		if (part == last) break;
	}
}

TBMExchangeProposal DescribeTBMWagonExchange(const TBMExchangeRequest &request)
{
	TBMExchangeProposal proposal;
	/**
	 * Finish a refusal; the partial lists are dropped so a refusal never looks like a plan.
	 * @param error First failed rule.
	 * @return The refused proposal.
	 */
	const auto refuse = [&proposal](TBMExchangeError error) {
		TBMExchangeProposal refused;
		refused.error = error;
		refused.bay = proposal.bay;
		refused.cutter = proposal.cutter;
		refused.hauler = proposal.hauler;
		return refused;
	};
	const TBMProject *project = GetTBMProject(request.project);
	if (project == nullptr || project->company != _current_company || IsTBMProjectFinished(project->phase)) return refuse(TBMExchangeError::InvalidProject);
	const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(request.hauler);
	if (assignment == nullptr || assignment->project != request.project) return refuse(TBMExchangeError::NotAssigned);
	const auto bay = GetTBMExchangeBay(*project);
	if (bay.error != TBMExchangeError::None || bay.tile != assignment->bay) return refuse(TBMExchangeError::InvalidBay);
	proposal.bay = bay.tile;
	Train *cutter = Train::GetIfValid(project->cutter);
	Train *hauler = Train::GetIfValid(request.hauler);
	if (cutter == nullptr) return refuse(TBMExchangeError::InvalidProject);
	if (hauler == nullptr) return refuse(TBMExchangeError::InvalidHauler);
	proposal.cutter = cutter->index;
	proposal.hauler = hauler->index;
	// Both trains are admitted to the bay before either chain is read for the exchange.
	for (const Train *participant : {cutter, hauler}) {
		const auto admitted = CheckTBMExchangeParticipant(*participant, bay, project->company);
		if (admitted != TBMExchangeError::None) return refuse(admitted);
	}
	// The bay serves one exchange at a time, in accepted request order among the haulers standing in it.
	const TBMExchangeAssignment *turn = GetTBMExchangeTurn(bay.tile);
	if (turn != nullptr && turn->id != assignment->id) return refuse(TBMExchangeError::OccupiedBay);
	const auto *job = project->job == ExcavationJobID{UINT32_MAX} ? nullptr : GetExcavationJob(project->job);
	if (job == nullptr || !job->pass_terms.has_value()) return refuse(TBMExchangeError::InvalidProject); // The empties are sized by the frozen terms.
	const auto layout = InspectTBMConsist(*cutter, *project);
	if (layout.error != TBMBlockedReason::None) return refuse(TBMExchangeError::InvalidConsist);

	/* Loaded side: every exchangeable unit of the cutter consist leaves, with all of its cargo kept aboard. */
	for (const auto &unit : layout.exchangeable_units) {
		Train *head = Train::Get(unit.vehicle);
		const Train *last = head->GetLastEnginePart();
		uint64_t carried = 0;
		for (const Train *part = head; part != nullptr; part = part->Next()) {
			// Cargo claimed for loading, transfer or delivery belongs to a station action and cannot change trains.
			if (part->cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) != part->cargo.TotalCount()) return refuse(TBMExchangeError::UnstableCargo);
			carried += part->cargo.TotalCount();
			if (part == last) break;
		}
		proposal.loaded_units.push_back(unit.vehicle);
		(unit.role == TBMConsistRole::EarthWagon ? proposal.earth_t : proposal.rock_t) += carried;
	}
	if (proposal.earth_t + proposal.rock_t == 0) return refuse(TBMExchangeError::NoSpoil);
	// The cutter's units become one set at the commit; a new set needs room in the store before anything changes.
	const TBMWagonSet *cutter_set = FindTBMWagonSetForWagon(proposal.loaded_units.front());
	if ((cutter_set == nullptr || cutter_set->wagons != proposal.loaded_units) && (_tbm_wagon_sets.size() >= MAX_TBM_WAGON_SETS || _next_tbm_wagon_set == UINT32_MAX)) return refuse(TBMExchangeError::Limit);

	/* Empty side: exactly the wagons of the hauler's set, each still a whole empty spoil unit of this hauler. */
	std::vector<VehicleID> earth_units;
	std::vector<VehicleID> rock_units;
	if (const TBMWagonSet *set = FindTBMWagonSet(assignment->set); set != nullptr) {
		for (VehicleID id : set->wagons) {
			Train *unit = Train::GetIfValid(id);
			uint64_t capacity;
			// A set wagon that left this hauler, or holds cargo again, breaks the set; it is not silently replaced.
			if (unit == nullptr || unit->First() != hauler || !IsEmptyTBMSpoilUnit(unit, capacity)) return refuse(TBMExchangeError::WrongWagons);
			if (GetTBMSpoilRole(*unit) == TBMConsistRole::EarthWagon) {
				earth_units.push_back(id);
				proposal.empty_earth_capacity_t += capacity;
			} else {
				rock_units.push_back(id);
				proposal.empty_rock_capacity_t += capacity;
			}
		}
	}
	// The cutter needs a full set of empties for the next pass; after the final cut it only gives its spoil away.
	const bool route_complete = job->cursor >= job->route.size();
	if (!route_complete && (proposal.empty_earth_capacity_t < job->pass_terms->earth_capacity_t || proposal.empty_rock_capacity_t < job->pass_terms->rock_capacity_t)) return refuse(TBMExchangeError::InsufficientEmpties);
	proposal.empty_units = earth_units;
	proposal.empty_units.insert(proposal.empty_units.end(), rock_units.begin(), rock_units.end());

	/* Ordered part lists: before, and after exactly as ExchangeRailWagonUnits links them. */
	for (const Train *part = cutter; part != nullptr; part = part->Next()) proposal.original_cutter.push_back(part->index);
	for (const Train *part = hauler; part != nullptr; part = part->Next()) proposal.original_hauler.push_back(part->index);
	Train *prefix_last = Train::Get(project->permanent_parts.back())->GetLastEnginePart();
	for (const Train *part = cutter; part != nullptr; part = part->Next()) {
		proposal.proposed_cutter.push_back(part->index);
		if (part == prefix_last) break;
	}
	for (VehicleID id : proposal.empty_units) AppendTBMUnitParts(proposal.proposed_cutter, Train::Get(id));
	Train *hauler_last = hauler->GetLastEnginePart();
	for (const Train *part = hauler; part != nullptr; part = part->Next()) {
		proposal.proposed_hauler.push_back(part->index);
		if (part == hauler_last) break;
	}
	for (VehicleID id : proposal.loaded_units) AppendTBMUnitParts(proposal.proposed_hauler, Train::Get(id));
	std::vector<VehicleID> moved_empty;
	for (VehicleID id : proposal.empty_units) AppendTBMUnitParts(moved_empty, Train::Get(id));
	// The hauler keeps every remaining part after its own unit, in order, except the empties that leave it.
	for (const Train *part = hauler_last->Next(); part != nullptr; part = part->Next()) {
		if (std::ranges::find(moved_empty, part->index) == moved_empty.end()) proposal.proposed_hauler.push_back(part->index);
	}
	proposal.error = TBMExchangeError::None;
	return proposal;
}

/**
 * Run the restricted rearrangement for an accepted description.
 * @param proposal Proposal from DescribeTBMWagonExchange with no error.
 * @param project Its project.
 * @param flags Execute to commit; otherwise only the trial.
 * @return The rearrangement result.
 */
static CommandCost RunTBMWagonExchange(const TBMExchangeProposal &proposal, const TBMProject &project, DoCommandFlags flags)
{
	std::vector<Train *> empties;
	std::vector<Train *> loaded;
	for (VehicleID id : proposal.empty_units) empties.push_back(Train::Get(id));
	for (VehicleID id : proposal.loaded_units) loaded.push_back(Train::Get(id));
	Train *prefix_last = Train::Get(project.permanent_parts.back())->GetLastEnginePart();
	return ExchangeRailWagonUnits(flags, Train::Get(proposal.cutter), prefix_last, empties, Train::Get(proposal.hauler), loaded);
}

/**
 * Turn a rearrangement refusal into the stable exchange refusal, keeping the shared message.
 * @param proposal Proposal to mark refused.
 * @param result Failed rearrangement result.
 */
static void RefuseTBMWagonExchange(TBMExchangeProposal &proposal, const CommandCost &result)
{
	proposal.message = result.GetErrorMessage();
	proposal.error = proposal.message == STR_ERROR_TBM_EXCHANGE_CAPACITY_LOSS ? TBMExchangeError::CapacityLoss : TBMExchangeError::Coupling;
}

TBMExchangeProposal ValidateTBMWagonExchange(const TBMExchangeRequest &request)
{
	auto proposal = DescribeTBMWagonExchange(request);
	if (proposal.error != TBMExchangeError::None) return proposal;
	const CommandCost trial = RunTBMWagonExchange(proposal, *GetTBMProject(request.project), {});
	if (trial.Failed()) RefuseTBMWagonExchange(proposal, trial);
	return proposal;
}

/**
 * Hand the sets over after a committed rearrangement: the cutter's units become the hauler's Loaded set, and the
 * hauler's set, now on the cutter, becomes its Assigned set. Store room was proven by the description.
 * @param proposal Executed proposal.
 * @param assignment The hauler's assignment.
 */
static void HandOverTBMWagonSets(const TBMExchangeProposal &proposal, TBMExchangeAssignment &assignment)
{
	const TBMWagonSet *existing = FindTBMWagonSetForWagon(proposal.loaded_units.front());
	TBMWagonSetID loaded_id{UINT32_MAX};
	if (existing != nullptr && existing->wagons == proposal.loaded_units) {
		loaded_id = existing->id;
	} else {
		// First exchange, or a cutter set that no longer matches its chain: the cutter's units form one new set.
		std::erase_if(_tbm_wagon_sets, [&proposal](const TBMWagonSet &set) {
			return std::ranges::any_of(set.wagons, [&proposal](VehicleID wagon) { return std::ranges::find(proposal.loaded_units, wagon) != proposal.loaded_units.end(); });
		});
		TBMWagonSet set;
		set.id = TBMWagonSetID{_next_tbm_wagon_set++};
		set.project = assignment.project;
		set.wagons = proposal.loaded_units;
		loaded_id = set.id;
		_tbm_wagon_sets.push_back(std::move(set));
	}
	TBMWagonSet *loaded = FindTBMWagonSet(loaded_id);
	loaded->state = TBMWagonSetState::Loaded;
	loaded->visited_origin = false;
	++loaded->revision;
	if (TBMWagonSet *empties = FindTBMWagonSet(assignment.set); empties != nullptr && empties->id != loaded_id) {
		empties->wagons = proposal.empty_units;
		empties->state = TBMWagonSetState::Assigned;
		empties->visited_origin = false;
		++empties->revision;
	}
	assignment.set = loaded_id;
	assignment.phase = TBMHaulerPhase::CollectLoaded;
	assignment.wait = TBMHaulerWait::None;
	assignment.phase_ticks = 0;
	++assignment.exchanges;
	++assignment.revision;
}

TBMExchangeProposal ExecuteTBMWagonExchange(const TBMExchangeRequest &request)
{
	auto proposal = ValidateTBMWagonExchange(request);
	if (proposal.error != TBMExchangeError::None) return proposal;
	const CommandCost result = RunTBMWagonExchange(proposal, *GetTBMProject(request.project), DoCommandFlag::Execute);
	// The trial above ran the same rules on the same chains in this same step, so the commit cannot be refused.
	if (result.Failed()) {
		RefuseTBMWagonExchange(proposal, result);
		return proposal;
	}
	HandOverTBMWagonSets(proposal, *FindTBMExchangeAssignmentForHauler(request.hauler));
	return proposal;
}

TBMExchangeError CheckTBMOriginAdmission(const TBMProject &project)
{
	// Loaded spoil always visits the company's own Surface railway station first; nothing below ground replaces it.
	if (!IsTBMOriginStation(project)) return TBMExchangeError::MissingOrigin;
	const Station *origin = Station::Get(project.origin);
	// Both spoil types need a real receiver in the station's catchment, which is what gives the station acceptance.
	for (const CargoLabel label : {CargoLabel{"ERTH"}, CargoLabel{"ROCK"}}) {
		const CargoType cargo = GetCargoTypeByLabel(label);
		if (!IsValidCargoType(cargo) || !origin->goods[cargo].status.Test(GoodsEntry::State::Acceptance)) return TBMExchangeError::NoReceiver;
	}
	return TBMExchangeError::None;
}

TBMHaulerInputs ObserveTBMHaulerInputs(const TBMExchangeAssignment &assignment)
{
	TBMHaulerInputs inputs;
	const TBMProject *project = GetTBMProject(assignment.project);
	const Train *hauler = Train::GetIfValid(assignment.hauler);
	if (hauler == nullptr) {
		inputs.set_aboard = false;
		return inputs;
	}
	inputs.crashed = hauler->vehstatus.Test(VehState::Crashed);
	inputs.stopped = hauler->vehstatus.Test(VehState::Stopped);
	// A finished project still owns the spoil of its final return, so only a missing project stops the cycle.
	if (project != nullptr) {
		inputs.project_live = true;
		inputs.origin_live = IsTBMOriginStation(*project);
		inputs.cutter_waiting = project->phase == TBMPhase::WaitForEmpties;
		const auto bay = GetTBMExchangeBay(*project);
		inputs.at_bay = bay.tile == assignment.bay && CheckTBMExchangeParticipant(*hauler, bay, project->company) == TBMExchangeError::None;
		// The origin visit is the head standing on a tile of the project's own origin station.
		inputs.at_origin = IsTileType(hauler->tile, TileType::Station) && GetStationIndex(hauler->tile) == project->origin;
	}
	inputs.lost = hauler->vehicle_flags.Test(VehicleFlag::PathfinderLost);
	if (const TBMWagonSet *set = FindTBMWagonSet(assignment.set); set != nullptr) {
		// The set is checked wagon by wagon against the actual chain, never trusted from the previous phase.
		for (VehicleID id : set->wagons) {
			Train *unit = Train::GetIfValid(id);
			if (unit == nullptr || unit->First() != hauler) {
				inputs.set_aboard = false;
				continue;
			}
			const Train *last = unit->GetLastEnginePart();
			for (const Train *part = unit; part != nullptr; part = part->Next()) {
				if (part->cargo.TotalCount() != 0) inputs.set_empty = false;
				if (part == last) break;
			}
		}
		// At the origin, the set is saturated when no receiver there has room for any spoil it still carries.
		const Station *origin = project == nullptr ? nullptr : Station::GetIfValid(project->origin);
		if (inputs.at_origin && origin != nullptr && !inputs.set_empty) {
			inputs.receiver_room = false;
			for (VehicleID id : set->wagons) {
				const Train *unit = Train::GetIfValid(id);
				if (unit != nullptr && unit->cargo.TotalCount() != 0 && GetSpoilReceiverRoomAt(*origin, unit->cargo_type) != 0) inputs.receiver_room = true;
			}
		}
	}
	return inputs;
}

TBMHaulerStep StepTBMHaulerPhase(const TBMExchangeAssignment &assignment, const TBMHaulerInputs &inputs)
{
	TBMHaulerStep step;
	step.phase = assignment.phase;
	/* Holds come first and keep the phase; the player's stop counts only away from the bay, where the cycle moves. */
	if (!inputs.project_live) {
		step.wait = TBMHaulerWait::LostProject;
	} else if (!inputs.origin_live) {
		step.wait = TBMHaulerWait::LostOrigin;
	} else if (inputs.crashed) {
		step.wait = TBMHaulerWait::Crashed;
	} else if (!inputs.set_aboard) {
		step.wait = TBMHaulerWait::WrongWagons;
	} else if (inputs.stopped && !inputs.at_bay) {
		step.wait = TBMHaulerWait::Stopped;
	} else if (inputs.lost && !inputs.at_bay) {
		step.wait = TBMHaulerWait::BlockedLine;
	}
	if (step.wait != TBMHaulerWait::None) return step;
	switch (assignment.phase) {
		case TBMHaulerPhase::ReturnEmpties:
			if (inputs.at_bay && inputs.set_empty) step.phase = TBMHaulerPhase::AwaitExchange;
			break;
		case TBMHaulerPhase::AwaitExchange:
			if (!inputs.at_bay) {
				step.phase = TBMHaulerPhase::ReturnEmpties; // It left the bay without exchanging; it must come back.
			} else if (inputs.cutter_waiting) {
				step.exchange = true;
			}
			break;
		case TBMHaulerPhase::CollectLoaded:
			if (!inputs.at_bay) step.phase = TBMHaulerPhase::VisitOrigin;
			break;
		case TBMHaulerPhase::VisitOrigin:
			if (inputs.at_origin) {
				step.visited_origin = true;
				step.phase = TBMHaulerPhase::DisposeOrTransfer;
			}
			break;
		case TBMHaulerPhase::DisposeOrTransfer:
			if (inputs.set_empty) {
				step.phase = TBMHaulerPhase::ReturnEmpties;
			} else if (inputs.at_origin && !inputs.receiver_room) {
				step.wait = TBMHaulerWait::SaturatedReceiver; // The spoil stays aboard until a receiver reopens or it is transferred.
			}
			break;
		default:
			break;
	}
	return step;
}

TBMExchangeError AdvanceTBMHauler(TBMExchangeAssignmentID id)
{
	TBMExchangeAssignment *assignment = FindTBMExchangeAssignment(id);
	if (assignment == nullptr) return TBMExchangeError::NotAssigned;
	const auto inputs = ObserveTBMHaulerInputs(*assignment);
	auto step = StepTBMHaulerPhase(*assignment, inputs);
	if (step.exchange) {
		const CompanyID owner = GetTBMProject(assignment->project)->company;
		Backup<CompanyID> actor(_current_company, owner); // The exchange acts for the company that owns both trains.
		const auto proposal = ExecuteTBMWagonExchange({assignment->project, assignment->hauler});
		actor.Restore();
		if (proposal.error == TBMExchangeError::None) return TBMExchangeError::None; // The commit moved the cycle on.
		// A refusal holds the hauler with its precise reason; any other refusal is the cutter not being ready yet.
		if (proposal.error == TBMExchangeError::InsufficientEmpties) step.wait = TBMHaulerWait::NoEmpties;
		if (proposal.error == TBMExchangeError::OccupiedBay) step.wait = TBMHaulerWait::OccupiedBay;
	}
	const bool phase_changed = step.phase != assignment->phase;
	if (TBMWagonSet *set = FindTBMWagonSet(assignment->set); set != nullptr) {
		if (step.visited_origin && !set->visited_origin) {
			set->visited_origin = true;
			++set->revision;
		}
		// The set is empty again once disposal or transfer is done; it now returns as the next empties.
		if (phase_changed && step.phase == TBMHaulerPhase::ReturnEmpties && set->state == TBMWagonSetState::Loaded) {
			set->state = TBMWagonSetState::ReturningEmpty;
			set->visited_origin = false;
			++set->revision;
		}
	}
	if (phase_changed || step.wait != assignment->wait) ++assignment->revision;
	assignment->phase_ticks = phase_changed ? 0 : std::min<uint64_t>(assignment->phase_ticks + 1, UINT64_MAX - 1);
	assignment->phase = step.phase;
	assignment->wait = step.wait;
	return TBMExchangeError::None;
}

bool IsTBMHaulerHeld(const Train &front)
{
	const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(front.index);
	return assignment != nullptr && assignment->phase == TBMHaulerPhase::AwaitExchange && assignment->wait != TBMHaulerWait::LostProject;
}

TBMLogisticsWait GetTBMLogisticsWait(const TBMProject &project)
{
	if (!IsTBMOriginStation(project)) return TBMLogisticsWait::LostOrigin;
	bool any = false, saturated = false, blocked = false, short_empties = false, occupied = false;
	// The most severe wait of any hauler of this project counts, whatever the haulers' order.
	for (const auto &assignment : _tbm_exchange_assignments) {
		if (assignment.project != project.id) continue;
		any = true;
		saturated |= assignment.wait == TBMHaulerWait::SaturatedReceiver;
		blocked |= assignment.wait == TBMHaulerWait::BlockedLine;
		short_empties |= assignment.wait == TBMHaulerWait::NoEmpties;
		occupied |= assignment.wait == TBMHaulerWait::OccupiedBay;
	}
	if (saturated) return TBMLogisticsWait::SaturatedReceiver;
	if (blocked) return TBMLogisticsWait::BlockedLine;
	if (short_empties) return TBMLogisticsWait::NoEmpties;
	if (occupied) return TBMLogisticsWait::OccupiedBay;
	// A cutter on its way back for, or waiting for, empties needs a hauler to bring them.
	if (!any && (project.phase == TBMPhase::ReturnForExchange || project.phase == TBMPhase::WaitForEmpties)) return TBMLogisticsWait::NoHauler;
	return TBMLogisticsWait::None;
}

bool IsTBMLogisticsStalled(const TBMProject &project)
{
	const TBMLogisticsWait wait = GetTBMLogisticsWait(project);
	return wait == TBMLogisticsWait::SaturatedReceiver || wait == TBMLogisticsWait::BlockedLine || wait == TBMLogisticsWait::LostOrigin;
}

uint32_t GetNextTBMExchangeAssignmentID()
{
	return _next_tbm_exchange_assignment;
}

uint32_t GetNextTBMWagonSetID()
{
	return _next_tbm_wagon_set;
}

void StageLoadedTBMExchange(std::vector<TBMExchangeAssignment> assignments, uint32_t next_assignment, std::vector<TBMWagonSet> sets, uint32_t next_set)
{
	_staged_tbm_exchange = TBMExchangeStaging{std::move(assignments), next_assignment, std::move(sets), next_set};
}

/**
 * @param staged Loaded store.
 * @return Whether every set is a valid, exclusive group of live wagon units of its project's company.
 */
static bool IsLoadedTBMWagonSetStoreValid(const TBMExchangeStaging &staged)
{
	std::set<VehicleID> wagons;
	uint32_t previous = 0;
	// Identities increase below the cursor; each set is known, non-empty and stamped.
	for (const auto &set : staged.sets) {
		if (set.id.base() >= staged.next_set || (&set != &staged.sets.front() && set.id.base() <= previous)) return false;
		previous = set.id.base();
		const TBMProject *project = GetTBMProject(set.project);
		if (project == nullptr || set.state >= TBMWagonSetState::End || set.wagons.empty() || set.revision == 0) return false;
		for (VehicleID id : set.wagons) {
			const Train *unit = Train::GetIfValid(id);
			// A wagon is a live unit head of the project's company and belongs to this set alone.
			if (unit == nullptr || unit->IsEngine() || unit->IsArticulatedPart() || unit->owner != project->company || !wagons.insert(id).second) return false;
			// The cutter's working set rides on the cutter; any other set may be anywhere, which its hauler's hold reports.
			if (set.state == TBMWagonSetState::Assigned && unit->First()->index != project->cutter) return false;
		}
	}
	return true;
}

/**
 * @param staged Loaded store with valid sets.
 * @return Whether every assignment is a valid, exclusive binding of one live hauler and at most one set.
 */
static bool IsLoadedTBMAssignmentStoreValid(const TBMExchangeStaging &staged)
{
	std::set<VehicleID> haulers;
	std::set<TBMWagonSetID> carried;
	uint32_t previous = 0;
	for (const auto &assignment : staged.assignments) {
		if (assignment.id.base() >= staged.next_assignment || (&assignment != &staged.assignments.front() && assignment.id.base() <= previous)) return false;
		previous = assignment.id.base();
		const TBMProject *project = GetTBMProject(assignment.project);
		if (project == nullptr || assignment.phase >= TBMHaulerPhase::End || assignment.wait >= TBMHaulerWait::End || assignment.revision == 0) return false;
		const Train *hauler = Train::GetIfValid(assignment.hauler);
		// The hauler is a live front engine of the project's company, no cutter, with exactly one assignment.
		if (hauler == nullptr || !hauler->IsFrontEngine() || hauler->owner != project->company || IsTBMCutterEngine(hauler->engine_type) || GetTBMProjectForCutter(hauler->index) != nullptr) return false;
		if (!haulers.insert(assignment.hauler).second) return false;
		if (assignment.set == TBMWagonSetID{UINT32_MAX}) continue;
		// A carried set exists, serves the same project, is not the cutter's, and has one carrier.
		const auto set = std::ranges::find(staged.sets, assignment.set, &TBMWagonSet::id);
		if (set == staged.sets.end() || set->project != assignment.project || set->state == TBMWagonSetState::Assigned || !carried.insert(assignment.set).second) return false;
	}
	return true;
}

bool AcceptLoadedTBMExchange()
{
	if (!_staged_tbm_exchange) return true;
	TBMExchangeStaging staged = std::move(*_staged_tbm_exchange);
	_staged_tbm_exchange.reset();
	if (!IsLoadedTBMWagonSetStoreValid(staged) || !IsLoadedTBMAssignmentStoreValid(staged)) return false;
	_tbm_exchange_assignments = std::move(staged.assignments);
	_next_tbm_exchange_assignment = staged.next_assignment;
	_tbm_wagon_sets = std::move(staged.sets);
	_next_tbm_wagon_set = staged.next_set;
	return true;
}

/**
 * Append one value's explicit big-endian bytes to an FNV-1a diagnostic hash.
 * @param digest Accumulator.
 * @param value Field value.
 */
static void HashTBMExchangeField(uint64_t &digest, uint64_t value)
{
	for (int shift = 56; shift >= 0; shift -= 8) {
		digest ^= static_cast<uint8_t>(value >> shift);
		digest *= 1099511628211ULL;
	}
}

uint64_t ComputeTBMExchangeDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	for (uint64_t value : {uint64_t{_next_tbm_exchange_assignment}, uint64_t{_next_tbm_wagon_set}, uint64_t{_tbm_exchange_assignments.size()}, uint64_t{_tbm_wagon_sets.size()}}) HashTBMExchangeField(digest, value);
	// Both stores are kept in identity order, so the digest never depends on insertion history.
	for (const auto &a : _tbm_exchange_assignments) {
		for (uint64_t value : {uint64_t{a.id.base()}, uint64_t{a.project.base()}, uint64_t{a.hauler.base()}, uint64_t{a.bay.base()}, uint64_t{a.set.base()}, uint64_t{to_underlying(a.phase)}, uint64_t{to_underlying(a.wait)}, a.phase_ticks, uint64_t{a.exchanges}, a.revision}) HashTBMExchangeField(digest, value);
	}
	for (const auto &set : _tbm_wagon_sets) {
		for (uint64_t value : {uint64_t{set.id.base()}, uint64_t{set.project.base()}, uint64_t{to_underlying(set.state)}, uint64_t{set.visited_origin}, set.revision, uint64_t{set.wagons.size()}}) HashTBMExchangeField(digest, value);
		for (VehicleID wagon : set.wagons) HashTBMExchangeField(digest, wagon.base());
	}
	return digest;
}

bool MayTBMSpoilLeaveVehicle(const Vehicle &front, const Vehicle &part, StationID station)
{
	// Only spoil carried by trains is bound to the leg; any other cargo and every other carrier stage as usual.
	if (GetSpoilReceiverKindForCargo(part.cargo_type) == SpoilReceiverKind::End || front.type != VehicleType::Train) return true;
	if (GetTBMProjectForCutter(front.index) != nullptr) return false; // The cutter cannot satisfy its own haulage leg.
	const Train *unit = Train::From(&part);
	if (unit->IsArticulatedPart()) unit = unit->GetFirstEnginePart();
	const TBMWagonSet *set = FindTBMWagonSetForWagon(unit->index);
	if (set == nullptr || set->state != TBMWagonSetState::Loaded) return true;
	// A Loaded set unloads only from its own carrier, and only at the origin or once it has visited the origin.
	const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(front.index);
	if (assignment == nullptr || assignment->set != set->id) return false;
	const TBMProject *project = GetTBMProject(set->project);
	return set->visited_origin || (project != nullptr && project->origin == station);
}

CommandCost CmdAssignTBMHauler(DoCommandFlags flags, TBMProjectID project, VehicleID hauler)
{
	if (AssignTBMHauler(project, hauler, flags.Test(DoCommandFlag::Execute)) != TBMExchangeError::None) return CMD_ERROR;
	return CommandCost();
}

CommandCost CmdReleaseTBMHauler(DoCommandFlags flags, VehicleID hauler)
{
	if (ReleaseTBMHauler(hauler, flags.Test(DoCommandFlag::Execute)) != TBMExchangeError::None) return CMD_ERROR;
	return CommandCost();
}

CommandCost CmdSetTBMExchangeBay(DoCommandFlags flags, TBMProjectID project, TileIndex tile)
{
	const TBMProject *record = GetTBMProject(project);
	if (record == nullptr || record->company != _current_company || IsTBMProjectFinished(record->phase)) return CMD_ERROR;
	// A bay with queued haulers cannot move under them; release them first.
	if (GetTBMExchangeAssignment(project) != nullptr || ResolveTBMExchangeBay(*record, tile).error != TBMExchangeError::None) return CMD_ERROR;
	if (SetTBMProjectExchangeBay(project, tile, flags.Test(DoCommandFlag::Execute)) != TBMProjectError::None) return CMD_ERROR;
	return CommandCost();
}

CommandCost CmdExchangeTBMWagons(DoCommandFlags flags, TBMProjectID project, VehicleID hauler)
{
	const TBMExchangeRequest request{project, hauler};
	const auto proposal = flags.Test(DoCommandFlag::Execute) ? ExecuteTBMWagonExchange(request) : ValidateTBMWagonExchange(request);
	if (proposal.error == TBMExchangeError::None) return CommandCost();
	if (proposal.message != INVALID_STRING_ID) return CommandCost(proposal.message);
	return CMD_ERROR;
}
