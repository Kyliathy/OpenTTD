/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange.cpp The controlled wagon exchange: bays, hauler assignments and their bay turns, wagon sets, the
 * pure proposal, the atomic commit through the restricted train rearrangement primitive, the hauler cycle, the origin
 * leg of unloading (Group 8.1), the logistics waits that pause new cutting (Group 9.3), the staged acceptance of a
 * loaded store (Group 10) and the lifecycle rules for sale, refit, autoreplace, deletion, bays and companies (Group 11).
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Groups 1-11
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-j-lifecycle-design.md
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-h-exchange-design.md
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-i-receiver-design.md
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 13C (a face wait without a hauler reports NoHauler)
 */

#include "stdafx.h"
#include "tbm_exchange.h"
#include "transport_tube.h"
#include "tbm_hangar.h"
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
#include "window_func.h"
#include "vehicle_gui.h"

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

TBMExchangeError CheckTBMProjectTakesHauler(TBMProjectID project_id)
{
	const TBMProject *project = GetTBMProject(project_id);
	if (project == nullptr || project->company != _current_company || IsTBMProjectFinished(project->phase)) return TBMExchangeError::InvalidProject;
	if (!IsTBMOriginStation(*project)) return TBMExchangeError::MissingOrigin; // Every loaded leg must have its Surface station.
	if (GetTBMExchangeBay(*project).error != TBMExchangeError::None) return TBMExchangeError::InvalidBay;
	return TBMExchangeError::None;
}

/**
 * Identify a layer-local tube participant that cannot serve the mandatory conventional spoil leg.
 * @param hauler Any actual part; its existing native chain provides every semantic engine identity.
 * @return Whether any physical part is a canonical capsule, independently of sprites or broad compatibility masks.
 */
static bool TBMHaulerContainsTubeCapsule(const Train &hauler)
{
	// Every part must permit conventional wagon/Surface-origin travel. Checking
	// the complete native chain also refuses a capsule hidden behind an ordinary front.
	for (const Train *part = hauler.First(); part != nullptr; part = part->Next()) {
		if (GetTubeVehicleCapability(part->engine_type).has_value()) return true;
	}
	return false;
}

/** @copydoc AssignTBMHauler */
TBMExchangeError AssignTBMHauler(TBMProjectID project_id, VehicleID hauler_id, bool execute)
{
	if (const auto error = CheckTBMProjectTakesHauler(project_id); error != TBMExchangeError::None) return error;
	const TBMProject *project = GetTBMProject(project_id);
	const auto bay = GetTBMExchangeBay(*project);
	Train *hauler = Train::GetIfValid(hauler_id);
	// The hauler is a separate whole company train; no cutter, of this or another project, can haul spoil.
	if (hauler == nullptr || !hauler->IsFrontEngine() || hauler->owner != project->company || hauler->vehstatus.Test(VehState::Crashed)) return TBMExchangeError::InvalidHauler;
	if (hauler->index == project->cutter || IsTBMCutterEngine(hauler->engine_type) || GetTBMProjectForCutter(hauler->index) != nullptr) return TBMExchangeError::InvalidHauler;
	// Layer-local capsules cannot carry this project's mandatory real wagon/Surface-origin spoil leg, even behind a conventional front.
	if (TBMHaulerContainsTubeCapsule(*hauler)) return TBMExchangeError::InvalidHauler;
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

/**
 * End one assignment without an actor check (lifecycle owner), as a release does: its set stays registered, an empty
 * returning set becomes Available, and a loaded set keeps its spoil and its origin visit.
 * @param id Assignment to end; a missing one is ignored.
 */
static void EndTBMExchangeAssignment(TBMExchangeAssignmentID id)
{
	TBMExchangeAssignment *assignment = FindTBMExchangeAssignment(id);
	if (assignment == nullptr) return;
	if (TBMWagonSet *set = FindTBMWagonSet(assignment->set); set != nullptr && set->state == TBMWagonSetState::ReturningEmpty) {
		set->state = TBMWagonSetState::Available;
		++set->revision;
	}
	std::erase_if(_tbm_exchange_assignments, [id](const TBMExchangeAssignment &candidate) { return candidate.id == id; });
}

TBMExchangeError ReleaseTBMHauler(VehicleID hauler, bool execute)
{
	TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(hauler);
	if (assignment == nullptr) return TBMExchangeError::NotAssigned;
	const TBMProject *owner = GetTBMProject(assignment->project);
	// A released project may already be gone; only another company's live project is protected from this actor.
	if (owner != nullptr && owner->company != _current_company) return TBMExchangeError::NotAssigned;
	if (!execute) return TBMExchangeError::None;
	EndTBMExchangeAssignment(assignment->id); // The set stays where it is, with any spoil and visit it has.
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
	// The cutter needs a full set of empties for the next pass; after the final cut, or once its job is cancelled
	// (Plan 4 Group 17.1), it only gives its spoil away.
	const bool route_complete = job->cursor >= job->route.size() || job->state == ExcavationJobState::Cancelled || IsTBMFinishing(project->phase); // A requested final return needs only enough empties for safe withdrawal, even while geology retains claims until settlement.
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

/**
 * @param project Project whose stand is read.
 * @param hauler Hauler front engine.
 * @return Whether the hauler's moving front stands on the project's face stand, heading along it towards the cutter.
 */
static bool IsTBMHaulerFacingStand(const TBMProject &project, const Train &hauler)
{
	const Train *moving = hauler.GetMovingFront();
	if (!IsPlainRailTile(moving->tile)) return false; // Cheap refusal for every train away from support.
	const auto stand = FindTBMFaceStand(project);
	return stand.has_value() && moving->tile == stand->tile && GetTBMFaceStandTrackdirs(*stand).Test(moving->GetVehicleTrackdir());
}

/** @copydoc DescribeTBMFaceTransfer */
TBMExchangeProposal DescribeTBMFaceTransfer(const TBMExchangeRequest &request)
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
	/* Identities, as for the bay exchange. */
	const TBMProject *project = GetTBMProject(request.project);
	// Only the actor's own live project, still boring or returning, can take a transfer.
	if (project == nullptr || project->company != _current_company || IsTBMProjectFinished(project->phase)) return refuse(TBMExchangeError::InvalidProject);
	const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(request.hauler);
	// Only a hauler assigned to this very project serves its face.
	if (assignment == nullptr || assignment->project != request.project) return refuse(TBMExchangeError::NotAssigned);
	const Train *cutter = Train::GetIfValid(project->cutter);
	const Train *hauler = Train::GetIfValid(request.hauler);
	if (cutter == nullptr) return refuse(TBMExchangeError::InvalidProject);
	// The hauler is a live train of the project's company, addressed by its front engine.
	if (hauler == nullptr || !hauler->IsFrontEngine() || hauler->owner != project->company) return refuse(TBMExchangeError::InvalidHauler);
	proposal.cutter = cutter->index;
	proposal.hauler = hauler->index;

	/* The meeting: the face, after the dwell, with both trains standing and the hauler on the stand. */
	// The cutter meets at its face (rule F1) and waits there with spoil that is due to leave.
	if (GetTBMMeetingKind(*project) != TBMMeetingKind::Face || !IsTBMReadyForFaceTransfer(*project)) return refuse(TBMExchangeError::NotAtBay);
	if (project->phase_ticks < TBM_FACE_TRANSFER_TICKS) return refuse(TBMExchangeError::NotAtBay);
	const auto stand = FindTBMFaceStand(*project);
	if (stand.has_value()) proposal.bay = stand->tile;
	// Packets move only between intact stationary trains, the hauler's moving front on the stand heading towards the cutter.
	if (cutter->vehstatus.Test(VehState::Crashed) || hauler->vehstatus.Test(VehState::Crashed) ||
		cutter->cur_speed != 0 || hauler->cur_speed != 0 || !IsTBMHaulerFacingStand(*project, *hauler)) return refuse(TBMExchangeError::NotAtBay);
	const auto layout = InspectTBMConsist(*cutter, *project);
	if (layout.error != TBMBlockedReason::None) return refuse(TBMExchangeError::InvalidConsist);

	/* Source: every spoil unit of the cutter, each part keeping all of its cargo. */
	size_t parts = 0;
	for (const auto &unit : layout.exchangeable_units) {
		Train *head = Train::Get(unit.vehicle);
		const Train *last = head->GetLastEnginePart();
		uint64_t carried = 0;
		for (const Train *part = head; part != nullptr; part = part->Next()) {
			// Cargo claimed for loading, transfer or delivery belongs to a station action and cannot change trains.
			if (part->cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) != part->cargo.TotalCount()) return refuse(TBMExchangeError::UnstableCargo);
			carried += part->cargo.TotalCount();
			++parts;
			if (part == last) break;
		}
		proposal.loaded_units.push_back(unit.vehicle);
		(unit.role == TBMConsistRole::EarthWagon ? proposal.earth_t : proposal.rock_t) += carried;
	}
	if (proposal.earth_t + proposal.rock_t == 0) return refuse(TBMExchangeError::NoSpoil);

	/* Destination: exactly the wagons of the hauler's own set, each still a whole empty spoil unit of this hauler. */
	const TBMWagonSet *set = FindTBMWagonSet(assignment->set);
	// A hauler without its own hoppers has nowhere to take the spoil.
	if (set == nullptr || set->wagons.empty()) return refuse(TBMExchangeError::WrongWagons);
	for (VehicleID id : set->wagons) {
		Train *unit = Train::GetIfValid(id);
		uint64_t capacity;
		// A set wagon that left this hauler, or holds cargo already, breaks the set; it is not silently replaced.
		if (unit == nullptr || unit->First() != hauler || !IsEmptyTBMSpoilUnit(unit, capacity)) return refuse(TBMExchangeError::WrongWagons);
		(GetTBMSpoilRole(*unit) == TBMConsistRole::EarthWagon ? proposal.empty_earth_capacity_t : proposal.empty_rock_capacity_t) += capacity;
		proposal.empty_units.push_back(id);
		// Include every articulated destination part in the conservative split
		// allowance; unit metadata alone cannot reserve physical packet room.
		for (const Train *part = unit; part != nullptr; part = part->Next()) {
			++parts;
			if (part == unit->GetLastEnginePart()) break;
		}
	}
	// Each spoil goes only into its own kind of hopper, and the whole load must fit: nothing is left behind.
	if (proposal.empty_earth_capacity_t < proposal.earth_t || proposal.empty_rock_capacity_t < proposal.rock_t) return refuse(TBMExchangeError::InsufficientEmpties);
	// Each shift between one source part and one destination part may split one packet; there are fewer shifts than parts.
	if (!CargoPacket::CanAllocateItem(parts)) return refuse(TBMExchangeError::Limit);

	/* No part changes train: the proposed chains are the original ones. */
	for (const Train *part = cutter; part != nullptr; part = part->Next()) proposal.original_cutter.push_back(part->index);
	for (const Train *part = hauler; part != nullptr; part = part->Next()) proposal.original_hauler.push_back(part->index);
	proposal.proposed_cutter = proposal.original_cutter;
	proposal.proposed_hauler = proposal.original_hauler;
	proposal.error = TBMExchangeError::None;
	return proposal;
}

/** @copydoc ValidateTBMFaceTransfer */
TBMExchangeProposal ValidateTBMFaceTransfer(const TBMExchangeRequest &request)
{
	return DescribeTBMFaceTransfer(request);
}

/**
 * Refresh a train's cached mass and acceleration and its views after spoil moved on or off it.
 * @param train Front engine whose cargo changed.
 */
static void RefreshTBMTrainCargo(Train &train)
{
	// The spoil is physical train mass: both caches follow the packets, as after a cut, then the views.
	train.CargoChanged();
	train.UpdateAcceleration();
	SetWindowDirty(GetWindowClassForVehicleType(VehicleType::Train), train.owner);
	SetWindowDirty(WindowClass::VehicleDetails, train.index);
	train.MarkDirty();
}

/** @copydoc ExecuteTBMFaceTransfer */
TBMExchangeProposal ExecuteTBMFaceTransfer(const TBMExchangeRequest &request)
{
	auto proposal = ValidateTBMFaceTransfer(request);
	if (proposal.error != TBMExchangeError::None) return proposal;
	TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(request.hauler);
	TBMWagonSet *set = FindTBMWagonSet(assignment->set);
	Train *cutter = Train::Get(proposal.cutter);
	Train *hauler = Train::Get(proposal.hauler);

	// Allocate complete destination vectors before the first physical mutation.
	// The hauler keeps its own hoppers in stable set/part order; no wagon relinks.
	std::vector<Train *> earth_dest, rock_dest;
	for (VehicleID id : set->wagons) {
		Train *unit = Train::Get(id);
		auto &dest = GetTBMSpoilRole(*unit) == TBMConsistRole::EarthWagon ? earth_dest : rock_dest;
		for (Train *part = unit; part != nullptr; part = part->Next()) {
			if (part->cargo_cap != 0) dest.push_back(part);
			if (part == unit->GetLastEnginePart()) break;
		}
	}
	/* Source parts: the cutter's spoil units. Each part's packets fill the next hopper with room, in order. */
	size_t next_earth = 0, next_rock = 0;
	for (VehicleID id : proposal.loaded_units) {
		Train *head = Train::Get(id);
		const bool earth = GetTBMSpoilRole(*head) == TBMConsistRole::EarthWagon;
		auto &dest = earth ? earth_dest : rock_dest;
		size_t &next = earth ? next_earth : next_rock;
		for (Train *source = head; source != nullptr; source = source->Next()) {
			// Move this part's packets until it is empty; the description proved the hoppers hold them all.
			while (source->cargo.TotalCount() != 0) {
				// Skip the hoppers this spoil has already filled.
				while (next < dest.size() && dest[next]->cargo.TotalCount() >= dest[next]->cargo_cap) ++next;
				assert(next < dest.size());
				if (next >= dest.size()) NOT_REACHED(); // An internal preflight violation must never publish a partial transfer as complete.
				const uint room = dest[next]->cargo_cap - dest[next]->cargo.TotalCount();
				const uint before = source->cargo.TotalCount();
				const uint amount = std::min(room, before);
				const uint moved = source->cargo.Shift(amount, &dest[next]->cargo);
				assert(moved == amount && source->cargo.TotalCount() == before - amount);
				(void)moved; // The admitted pool bound forbids native whole-packet fallback.
			}
			if (source == head->GetLastEnginePart()) break;
		}
	}
	RefreshTBMTrainCargo(*cutter);
	RefreshTBMTrainCargo(*hauler);

	/* Bookkeeping, as after a bay exchange but without moving wagons: the hauler takes its own set away loaded. */
	set->state = TBMWagonSetState::Loaded;
	set->visited_origin = false;
	++set->revision;
	assignment->phase = TBMHaulerPhase::CollectLoaded;
	assignment->wait = TBMHaulerWait::None;
	assignment->phase_ticks = 0;
	++assignment->exchanges;
	++assignment->revision;
	return proposal;
}

/** @copydoc GetTBMHaulerRoomCells */
uint GetTBMHaulerRoomCells(TBMProjectID project)
{
	uint cells = 0;
	// Every current assignment contributes its actual complete consist length;
	// absent trains cannot lend clearance and no derived length is persisted.
	for (const auto &assignment : _tbm_exchange_assignments) {
		if (assignment.project != project) continue;
		if (const Train *hauler = Train::GetIfValid(assignment.hauler); hauler != nullptr) cells = std::max<uint>(cells, CeilDiv(hauler->gcache.cached_total_length, TILE_SIZE));
	}
	return cells;
}

/**
 * @param project Project whose pair is read.
 * @param corridor Its sorted corridor tiles (GetTBMWorkCorridor).
 * @param train Front engine of a train.
 * @return Whether a part of the train runs on the corridor or on either end of the pair. A part stored in a magazine
 *         or depot there, as a hauler queued at its bay is, is off the running line and does not count.
 */
static bool IsTrainInTBMCorridor(const TBMProject &project, const std::vector<TileIndex> &corridor, const Train &train)
{
	const TileIndex key = project.portal;
	const TileIndex housing = IsValidTile(key) && IsLayerPortalTile(key) ? GetLayerPortalPairTile(key) : INVALID_TILE;
	// A single running part holds the corridor until the complete hauler
	// clears it; stored depot/magazine parts remain off the running line.
	// A single stored/running follower changes the whole chain's departure contract.
	for (const Train *part = &train; part != nullptr; part = part->Next()) {
		if (part->track == Track::Depot) continue;
		// The pair's two ends join the corridor to the Surface: a hauler passing through the shaft is still inside.
		if ((key != INVALID_TILE && part->tile == key) || (housing != INVALID_TILE && part->tile == housing) || std::ranges::binary_search(corridor, part->tile)) return true;
	}
	return false;
}

/** @copydoc IsTBMHaulerInCorridor */
bool IsTBMHaulerInCorridor(const TBMProject &project, VehicleID hauler)
{
	const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(hauler);
	const Train *front = Train::GetIfValid(hauler);
	// Only this exact project assignment and a surviving native front may
	// hold its work corridor; a matching tile alone grants no participation.
	if (assignment == nullptr || assignment->project != project.id || front == nullptr) return false;
	return IsTrainInTBMCorridor(project, GetTBMWorkCorridor(project), *front);
}

/** @copydoc FindTBMHaulerInCorridor */
VehicleID FindTBMHaulerInCorridor(const TBMProject &project)
{
	VehicleID found = VehicleID::Invalid();
	std::vector<TileIndex> corridor;
	// The corridor is read once for all haulers; most projects have one or two.
	for (const auto &assignment : _tbm_exchange_assignments) {
		if (assignment.project != project.id) continue;
		const Train *front = Train::GetIfValid(assignment.hauler);
		// Missing trains confer no hold; a higher identity cannot replace an
		// already found lower occupant, keeping observation order deterministic.
		if (front == nullptr || (found != VehicleID::Invalid() && found < front->index)) continue;
		if (corridor.empty()) corridor = GetTBMWorkCorridor(project);
		if (IsTrainInTBMCorridor(project, corridor, *front)) found = front->index;
	}
	return found;
}

/**
 * @param project Project whose pair and corridor are read.
 * @param train Front engine of a hauler.
 * @return Whether a part of the train runs beyond the key: on the pair's housing or the corridor. Parts on the key
 *         itself, moving or stored, are where queuing haulers stand; parts stored elsewhere are off the running line.
 */
static bool IsTBMHaulerBeyondKey(const TBMProject &project, const Train &train)
{
	// A missing or replaced source cannot identify a safe exit corridor.
	if (!IsValidTile(project.portal) || !IsLayerPortalTile(project.portal)) return false;
	const TileIndex housing = GetLayerPortalPairTile(project.portal);
	const auto corridor = GetTBMWorkCorridor(project);
	// A single running part beyond the key still needs outbound clearance;
	// queued/stored key parts alone do not authorize a new shaft entry.
	for (const Train *part = &train; part != nullptr; part = part->Next()) {
		// Stored bodies and the queuing key remain outside the running interior.
		if (part->track == Track::Depot || part->tile == project.portal) continue;
		if (part->tile == housing || std::ranges::binary_search(corridor, part->tile)) return true;
	}
	return false;
}

/**
 * @param assignment Assignment of a hauler.
 * @param job Its project's live job.
 * @return Whether the hauler could serve the face now (rule F5): it returns empties or waits for an exchange without a
 *         hold, and its own set is wholly aboard, empty and covers the job's frozen pass terms for each spoil, so it can
 *         always take the cutter's whole load.
 */
static bool IsTBMFaceCandidate(const TBMExchangeAssignment &assignment, const ExcavationJob &job)
{
	// Only a current empty return or exchange wait can acquire the face turn;
	// a loaded/origin-bound leg must finish its existing delivery obligation.
	if (assignment.phase != TBMHaulerPhase::ReturnEmpties && assignment.phase != TBMHaulerPhase::AwaitExchange) return false;
	// A hold other than another hauler's turn at the bay (stopped, crashed, lost, short of empties) gives up the turn.
	if (assignment.wait != TBMHaulerWait::None && assignment.wait != TBMHaulerWait::OccupiedBay) return false;
	const Train *hauler = Train::GetIfValid(assignment.hauler);
	const TBMWagonSet *set = FindTBMWagonSet(assignment.set);
	// A live own-company intact hauler, actual set and frozen pass terms are
	// all required; missing/foreign facts cannot lend another train capacity.
	if (hauler == nullptr || hauler->owner != job.company || hauler->vehstatus.Test(VehState::Crashed) || set == nullptr || !job.pass_terms.has_value()) return false;
	uint64_t earth = 0, rock = 0;
	// Each set wagon must still be an empty spoil unit of this hauler; its parts' capacity counts for its spoil.
	for (VehicleID id : set->wagons) {
		const Train *unit = Train::GetIfValid(id);
		// A detached or absent set member cannot count as an empty hopper aboard
		// this candidate, even if the old set still names its identity.
		if (unit == nullptr || unit->First() != hauler) return false;
		const TBMConsistRole role = GetTBMSpoilRole(*unit);
		if (role == TBMConsistRole::Invalid) return false;
		for (const Train *part = unit; part != nullptr; part = part->Next()) {
			if (part->cargo.TotalCount() != 0) return false;
			(role == TBMConsistRole::EarthWagon ? earth : rock) += part->cargo_cap;
			if (!part->HasArticulatedPart()) break; // The unit ends with its last articulated part.
		}
	}
	return earth >= job.pass_terms->earth_capacity_t && rock >= job.pass_terms->rock_capacity_t;
}

/** @copydoc IsTBMFaceTurnHolder */
bool IsTBMFaceTurnHolder(const TBMExchangeAssignment &assignment)
{
	const TBMProject *project = GetTBMProject(assignment.project);
	const ExcavationJob *job = project == nullptr ? nullptr : GetLiveTBMJob(*project);
	if (job == nullptr) return false;
	// The store is in identity order, so the first eligible hauler of the project has the lowest assignment ID.
	/** @param candidate Actual retained assignment. @return Same-project whole-empty eligibility, without reserving a turn. */
	const auto eligible = [&assignment, job](const TBMExchangeAssignment &candidate) {
		return candidate.project == assignment.project && IsTBMFaceCandidate(candidate, *job);
	};
	const auto holder = std::ranges::find_if(_tbm_exchange_assignments, eligible);
	// No eligible assignment or a different earlier assignment leaves this
	// caller without a turn; observation never reorders the native store.
	if (holder == _tbm_exchange_assignments.end() || holder->id != assignment.id) return false;
	// One hauler in the corridor at a time: another of the project still inside, such as a loaded one on its way out,
	// keeps the turn closed, or the two would meet head on.
	std::vector<TileIndex> corridor;
	for (const auto &other : _tbm_exchange_assignments) {
		// Only another hauler of this same project competes for its single
		// corridor; the candidate itself is allowed to finish its own visit.
		if (other.project != assignment.project || other.id == assignment.id) continue;
		const Train *front = Train::GetIfValid(other.hauler);
		if (front == nullptr) continue;
		if (corridor.empty()) corridor = GetTBMWorkCorridor(*project);
		if (IsTrainInTBMCorridor(*project, corridor, *front)) return false;
	}
	return true;
}

/** @copydoc IsTBMHaulerAdmittedThroughPortal */
bool IsTBMHaulerAdmittedThroughPortal(const Train &front, TileIndex tile)
{
	const Train *head = front.First();
	const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(head->index);
	const TBMProject *project = assignment == nullptr ? nullptr : GetTBMProject(assignment->project);
	// Only an actual own-company assignment at a real portal can ask for
	// through passage; ordinary/foreign trains gain no project authority.
	if (project == nullptr || head->owner != project->company || !IsValidTile(tile) || !IsLayerPortalTile(tile)) return false;
	// Only the project's own pair, either end; a hauler never passes another project's pair.
	const auto key = GetLayerPortalPairKey(tile);
	if (!key.has_value() || *key != project->portal) return false;
	// On its way out: a part already beyond the key, on the housing or the corridor, always gets back to the Surface,
	// even once the project has finished.
	if (IsTBMHaulerBeyondKey(*project, *head)) return true;
	// On its way in: only the face turn holder, while the cutter works or waits at its face with a stand behind it.
	return GetTBMMeetingKind(*project) == TBMMeetingKind::Face && IsTBMFaceTurnHolder(*assignment);
}

/** @copydoc IsTBMHaulerFaceStand */
bool IsTBMHaulerFaceStand(const Train &front, TileIndex tile, Trackdir trackdir)
{
	const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(front.First()->index);
	// Cheap refusals first: path searches ask this for many candidate tiles of every train.
	if (assignment == nullptr || !IsValidTile(tile) || !IsPlainRailTile(tile)) return false;
	const TBMProject *project = GetTBMProject(assignment->project);
	if (project == nullptr) return false;
	const auto stand = FindTBMFaceStand(*project);
	// The stand counts on any of its pieces leaving towards the cutter, as the hold reads it.
	if (!stand.has_value() || stand->tile != tile || !GetTBMFaceStandTrackdirs(*stand).Test(trackdir)) return false;
	return GetTBMMeetingKind(*project) == TBMMeetingKind::Face && IsTBMFaceTurnHolder(*assignment);
}

/** Inspect actual storage before a physical reversal.
 * @param train Actual front engine.
 * @return Whether any follower remains in a native depot or magazine.
 */
static bool HasTBMStoredPart(const Train &train)
{
	// A single stored/running follower changes the whole chain's departure contract.
	for (const Train *part = &train; part != nullptr; part = part->Next()) {
		if (part->track == Track::Depot) return true;
	}
	return false;
}

/** Observe whether a saved lost-route flag describes any currently running part.
 * @param train Actual front engine.
 * @return Whether the chain has a part outside native storage.
 */
static bool HasTBMMovingPart(const Train &train)
{
	// A single stored/running follower changes the whole chain's departure contract.
	for (const Train *part = &train; part != nullptr; part = part->Next()) {
		if (part->track != Track::Depot) return true;
	}
	return false;
}

/** @copydoc ObserveTBMHaulerInputs */

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
		// Rule F4: the face turn holder meets the cutter at its stand, every other hauler at the bay (r2df2 task 4.2).
		const bool face = GetTBMMeetingKind(*project) == TBMMeetingKind::Face && IsTBMFaceTurnHolder(assignment);
		inputs.cutter_waiting = IsTBMWaitingForExchange(project->phase) || IsTBMReadyForFaceTransfer(*project);
		inputs.exchange_finished = IsTBMProjectFinished(project->phase) || IsTBMStored(project->phase);
		if (face) {
			inputs.at_bay = IsTBMHaulerFacingStand(*project, *hauler); // IsTBMHaulerHeld stops it there.
		} else {
			const auto bay = GetTBMExchangeBay(*project);
			inputs.at_bay = bay.tile == assignment.bay && CheckTBMExchangeParticipant(*hauler, bay, project->company) == TBMExchangeError::None;
		}
		// Rule F6: a hauler inside leaves the corridor when it may not serve the face, or when the cutter was sent back
		// with nothing aboard to collect.
		inputs.inside = IsTBMHaulerBeyondKey(*project, *hauler);
		if (inputs.inside && (!face || (IsTBMReturning(project->phase) && !IsTBMExchangeDue(*project)))) inputs.exchange_finished = true;
		// The origin visit is the head standing on a tile of the project's own origin station.
		inputs.at_origin = IsTileType(hauler->tile, TileType::Station) && GetStationIndex(hauler->tile) == project->origin;
	}
	inputs.lost = hauler->vehicle_flags.Test(VehicleFlag::PathfinderLost);
	// r2mlvs3mp task 10R.2: an empty hauler bound for its Source bay finds no path while another train still moves on
	// the bay's housing, because GetLayerPortalAdmittedTrackdirs hides a busy magazine. During the first cut that train
	// is the project's own cutter, whose tail stands on the key. That is a queue at the bay, not a broken line:
	// reporting BlockedLine would raise LogisticsStall and hold the very cut that frees the housing. The pathfinder
	// clears its lost flag once the housing is free again; a hauler lost for any other reason still reports it.
	if (inputs.lost && project != nullptr && assignment.phase == TBMHaulerPhase::ReturnEmpties &&
			IsLayerPortalTile(assignment.bay) && IsLayerPortalMagazineBusy(assignment.bay, *hauler)) {
		inputs.lost = false;
	}
	// A hauler standing wholly inside a magazine or depot has no route to be lost on: its flag dates from before it
	// arrived, and only its next path search, once it moves, sets or clears it. Read stale, the flag would report
	// BlockedLine to a face turn holder still in its bay, which gives up the turn, reads as at the bay again and so
	// flips back every tick without ever leaving (r2mlvs3mp task 10R.2).
	if (inputs.lost && !HasTBMMovingPart(*hauler)) inputs.lost = false;
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

/** @copydoc StepTBMHaulerPhase */
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
			if (inputs.at_bay && inputs.set_empty) {
				step.phase = inputs.exchange_finished ? TBMHaulerPhase::VisitOrigin : TBMHaulerPhase::AwaitExchange;
			} else if (inputs.inside && inputs.exchange_finished) {
				step.phase = TBMHaulerPhase::VisitOrigin; // Rule F6: inside, but no longer serving the face; it backs out.
			}
			break;
		case TBMHaulerPhase::AwaitExchange:
			if (inputs.exchange_finished) {
				step.phase = TBMHaulerPhase::VisitOrigin; // Empty queued haulers physically clear parked/finished work; no packet or wagon is fabricated by departure.
			} else if (!inputs.at_bay) {
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

/**
 * Rule F6 (r2df2 task 4.4): a hauler leaving the face turns round on the stand, where its hold stopped it facing the
 * cutter, and backs out the way it came. Turned, it no longer faces the stand, so this happens once per visit; a part
 * still stored in the magazine could not follow a reversal, so a hauler only turns once wholly out.
 * @param assignment Assignment after this tick's step or transfer.
 */
static void TurnTBMHaulerAtStand(const TBMExchangeAssignment &assignment)
{
	const bool leaving = assignment.phase == TBMHaulerPhase::CollectLoaded || assignment.phase == TBMHaulerPhase::VisitOrigin || assignment.phase == TBMHaulerPhase::DisposeOrTransfer;
	if (!leaving) return;
	Train *hauler = Train::GetIfValid(assignment.hauler);
	if (hauler == nullptr || HasTBMStoredPart(*hauler)) return;
	if (const TBMProject *project = GetTBMProject(assignment.project); project != nullptr && IsTBMHaulerFacingStand(*project, *hauler)) BackUpTBMHauler(*hauler);
}

/** @copydoc AdvanceTBMHauler */
TBMExchangeError AdvanceTBMHauler(TBMExchangeAssignmentID id)
{
	TBMExchangeAssignment *assignment = FindTBMExchangeAssignment(id);
	if (assignment == nullptr) return TBMExchangeError::NotAssigned;
	const auto inputs = ObserveTBMHaulerInputs(*assignment);
	auto step = StepTBMHaulerPhase(*assignment, inputs);
	if (step.exchange) {
		const TBMProject *project = GetTBMProject(assignment->project);
		Backup<CompanyID> actor(_current_company, project->company); // The exchange acts for the company that owns both trains.
		// Rule F3 (r2df2 task 6.4): a cutter ready at its face hands its spoil over there; any other cutter waits in its bay.
		const TBMExchangeRequest request{assignment->project, assignment->hauler};
		const auto proposal = IsTBMReadyForFaceTransfer(*project) ? ExecuteTBMFaceTransfer(request) : ExecuteTBMWagonExchange(request);
		actor.Restore();
		if (proposal.error == TBMExchangeError::None) {
			TurnTBMHaulerAtStand(*assignment); // The commit moved the cycle on; at the face the loaded hauler turns at once.
			return TBMExchangeError::None;
		}
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
	TurnTBMHaulerAtStand(*assignment);
	if (inputs.lost) {
		if (Train *hauler = Train::GetIfValid(assignment->hauler); hauler != nullptr) ChooseTBMHaulerRouteDirection(*hauler);
	} // A closed directed chamber loop has no line end; native two-end routing can recover the existing work/ordinary destination without changing cargo or work.
	return TBMExchangeError::None;
}

/** @copydoc IsTBMHaulerHeld */
bool IsTBMHaulerHeld(const Train &front)
{
	const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(front.index);
	if (assignment == nullptr) return false;
	const auto *project = GetTBMProject(assignment->project);
	if (project != nullptr && (IsTBMProjectFinished(project->phase) || IsTBMStored(project->phase))) return false; // The origin order clears the real bay after work stops needing empties.
	// A hauler reaching the stand heading towards the cutter stops there, one clear cell behind it, whatever its phase:
	// the turn holder waits for the transfer, any other hauler turns round there (rules F4 and F6).
	if (project != nullptr && IsTBMHaulerFacingStand(*project, front)) return true;
	return assignment->phase == TBMHaulerPhase::AwaitExchange && assignment->wait != TBMHaulerWait::LostProject;
}

/** @copydoc IsTBMHaulerAdmittedAtBay */
bool IsTBMHaulerAdmittedAtBay(const Train &front, TileIndex tile)
{
	const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(front.First()->index);
	if (assignment == nullptr || assignment->bay != tile) return false;
	const TBMProject *project = GetTBMProject(assignment->project);
	// Only a Source bay is a magazine; a depot bay is a genuine depot with the ordinary rules.
	if (project == nullptr || front.owner != project->company) return false;
	const TBMExchangeBay bay = ResolveTBMExchangeBay(*project, tile);
	if (bay.error != TBMExchangeError::None || bay.kind != TBMExchangeBayKind::Source) return false;
	// A hauler with a part on the key may always finish leaving the bay it stands in, or finish entering it.
	for (const Train *part = front.First(); part != nullptr; part = part->Next()) {
		if (part->tile == tile) return true;
	}
	// A finished project has no exchange left: the hauler is never drawn back in for one.
	if (IsTBMProjectFinished(project->phase) || IsTBMStored(project->phase)) return false;
	// r2mlvs5 task 13D.2: through the door, only a hauler bound for its bay or waiting there is admitted. A loaded
	// hauler on its way to the origin has no business in the bay. Admitted, it saw a track into the magazine rather
	// than a line end at the door, so path reservation found no safe waiting position between the origin platform and
	// the cutter, and a hauler turning back at the origin could never reserve its way out of its depot.
	return assignment->phase == TBMHaulerPhase::ReturnEmpties || assignment->phase == TBMHaulerPhase::AwaitExchange;
}

/** @copydoc MayTBMTrainLeaveMagazine */
bool MayTBMTrainLeaveMagazine(const Train &front)
{
	if (IsTBMCutterEngine(front.engine_type) && front.current_order.IsType(OT_GOTO_DEPOT) && !IsTBMStagingHangarOrder(front)) return GetTBMOrderedHangar(front).error == TBMHangarError::None;
	if (const TBMProject *project = GetTBMProjectForCutter(front.index); project != nullptr) {
		if (project->phase == TBMPhase::FollowOrders) return front.current_order.IsGotoOrder() && GetTBMTravelHoldReason(front) == TBMBlockedReason::None;
		return project->phase == TBMPhase::TravelToFace && project->blocked == TBMBlockedReason::None; // Off to the face.
	}
	const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(front.index);
	if (assignment == nullptr) return false;
	// The face turn holder queued here leaves towards the face; the exit rule sends it through the shaft (r2df2 task 3.4).
	if (IsTBMHaulerAdmittedThroughPortal(front, front.tile)) return true;
	// A finished project has no exchange to wait for, so its hauler leaves whatever its phase.
	if (const TBMProject *project = GetTBMProject(assignment->project); project != nullptr && (IsTBMProjectFinished(project->phase) || IsTBMStored(project->phase))) return true;
	// A hauler waits in the bay while it returns empties or awaits its exchange; afterwards it takes the load out.
	return assignment->phase != TBMHaulerPhase::ReturnEmpties && assignment->phase != TBMHaulerPhase::AwaitExchange;
}

/** @copydoc IsTBMWaitingAtLineEnd */
bool IsTBMWaitingAtLineEnd(const Train &front, TileIndex next)
{
	if (GetTBMProjectForCutter(front.index) != nullptr) return true;
	const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(front.index);
	if (assignment == nullptr) return false;
	const auto goal = GetTBMHaulerSteeringTarget(front);
	// Only an actual managed bay destination suppresses ordinary line-end
	// reversal. A loaded leg or a parked/finished project's empty hauler follows
	// its Surface order, even while a part still traverses the old Source key.
	return goal.has_value() && goal->tile == next && assignment->bay == next && IsTBMHaulerAdmittedAtBay(front, next);
}

/** @copydoc GetTBMHaulerSteeringTarget */
std::optional<TBMHaulerSteeringTarget> GetTBMHaulerSteeringTarget(const Train &front)
{
	const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(front.First()->index);
	// Only the empty return is steered; every other phase follows the player's orders, the origin visit included.
	if (assignment == nullptr || assignment->phase != TBMHaulerPhase::ReturnEmpties) return std::nullopt; // Only a real assigned empty-return obligation can override the ordinary origin order.
	const auto *project = GetTBMProject(assignment->project);
	if (project == nullptr) return std::nullopt;
	// Once work parks or finishes, its empty hauler follows its ordinary origin
	// order. An actual part still on the Source key may finish that physical
	// traversal, but cannot revive the now-ended bay destination (Plan18 8FA).
	if (IsTBMProjectFinished(project->phase) || IsTBMStored(project->phase)) return std::nullopt;
	// The face turn holder drives through the pair to the stand behind the cutter, arriving towards it (r2df2 task 4.1).
	if (GetTBMMeetingKind(*project) == TBMMeetingKind::Face && IsTBMFaceTurnHolder(*assignment)) {
		if (const auto stand = FindTBMFaceStand(*project); stand.has_value()) return TBMHaulerSteeringTarget{stand->tile, GetTBMFaceStandTrackdirs(*stand)};
	}
	const auto bay = GetTBMExchangeBay(*project);
	if (bay.error != TBMExchangeError::None) return std::nullopt;
	if (bay.kind == TBMExchangeBayKind::Source) {
		if (!IsTBMHaulerAdmittedAtBay(front, assignment->bay)) return std::nullopt;
	}
	// Every other hauler queues at the bay, entering through its door.
	const DiagDirection door = bay.kind == TBMExchangeBayKind::Source ? GetLayerPortalDoorDirection(assignment->bay) : GetRailDepotDirection(assignment->bay);
	return TBMHaulerSteeringTarget{assignment->bay, TrackdirToTrackdirBits(DiagDirToDiagTrackdir(ReverseDiagDir(door)))};
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
	// A cutter on its way back for, or waiting for, empties needs a hauler to bring them. So does a cutter whose load
	// waits at its face for a transfer: with no hauler assigned, none of these waits can end until the player assigns one.
	if (!any && (IsTBMReturning(project.phase) || IsTBMWaitingForExchange(project.phase) || IsTBMAwaitingFaceTransfer(project.phase))) return TBMLogisticsWait::NoHauler;
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
		// Loaded assignments must obey the same conventional spoil domain as new commands; saved references cannot authorize a capsule substitute.
		if (TBMHaulerContainsTubeCapsule(*hauler)) return false;
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

/**
 * @param unit Unit head.
 * @return Whether the unit or any of its articulated parts carries cargo.
 */
static bool IsTBMUnitLoaded(const Train &unit)
{
	for (const Train *part = &unit; part != nullptr; part = part->Next()) {
		if (part->cargo.TotalCount() != 0) return true;
		if (!part->HasArticulatedPart()) break; // The unit ends with its last articulated part.
	}
	return false;
}

/**
 * @param part Any train part.
 * @return Whether its train is the cutter consist of a project record, finished or not.
 */
static bool IsTBMCutterConsistPart(const Train &part)
{
	return GetTBMProjectForCutter(part.First()->index) != nullptr;
}

void OnTBMExchangeTrainPartDeleted(const Train &part)
{
	if (_tbm_exchange_assignments.empty() && _tbm_wagon_sets.empty()) return; // Every other train deletion costs nothing.
	const VehicleID id = part.index;
	// A deleted hauler ends its assignment; its set and any spoil in it stay where they are.
	if (const TBMExchangeAssignment *assignment = FindTBMExchangeAssignmentForHauler(id); assignment != nullptr) EndTBMExchangeAssignment(assignment->id);
	// A deleted wagon leaves its set. A set without wagons is gone, and so is the cycle of the hauler carrying it.
	if (TBMWagonSet *set = FindTBMWagonSetForWagon(id); set != nullptr) {
		std::erase(set->wagons, id);
		++set->revision;
		if (set->wagons.empty()) {
			const TBMWagonSetID gone = set->id;
			std::erase_if(_tbm_wagon_sets, [gone](const TBMWagonSet &candidate) { return candidate.id == gone; });
			std::vector<TBMExchangeAssignmentID> carriers;
			for (const auto &assignment : _tbm_exchange_assignments) {
				if (assignment.set == gone) carriers.push_back(assignment.id);
			}
			for (TBMExchangeAssignmentID carrier : carriers) EndTBMExchangeAssignment(carrier);
		}
	}
	// A deleted cutter leaves its working set on no cutter: with spoil aboard it waits, Loaded, for a hauler to adopt
	// it; empty it is simply Available. Normally its wagons are deleted with it and the set goes as above.
	if (const TBMProject *project = GetTBMProjectForCutter(id); project != nullptr) {
		for (auto &set : _tbm_wagon_sets) {
			if (set.project != project->id || set.state != TBMWagonSetState::Assigned) continue;
			const bool loaded = std::ranges::any_of(set.wagons, [](VehicleID wagon) {
				const Train *unit = Train::GetIfValid(wagon);
				return unit != nullptr && IsTBMUnitLoaded(*unit);
			});
			set.state = loaded ? TBMWagonSetState::Loaded : TBMWagonSetState::Available;
			set.visited_origin = false;
			++set.revision;
		}
	}
}

/** @copydoc GetTBMProjectSettlementBlocker */
std::optional<VehicleID> GetTBMProjectSettlementBlocker(TBMProjectID project)
{
	const auto *owner = GetTBMProject(project);
	if (owner == nullptr) return VehicleID::Invalid();
	// Recorded loaded sets survive manual assignment release. They must also
	// settle before retirement, including every articulated part's real load.
	for (const auto &set : _tbm_wagon_sets) {
		if (set.project != project) continue;
		for (VehicleID id : set.wagons) {
			const Train *unit = Train::GetIfValid(id);
			if (unit == nullptr) return VehicleID::Invalid();
			if (IsTBMUnitLoaded(*unit)) return unit->First()->index;
			if (unit->First()->index != owner->cutter) {
				for (const Train *part = unit->First(); part != nullptr; part = part->Next()) {
					if (part->tile == owner->exchange_bay) return part->First()->index; // Manual assignment release cannot make a recorded hauler's remaining tail disappear from the bay.
				}
			}
		}
	}
	for (const auto &assignment : _tbm_exchange_assignments) {
		if (assignment.project != project) continue;
		const Train *front = Train::GetIfValid(assignment.hauler);
		if (front == nullptr) return VehicleID::Invalid();
		if (front->owner != owner->company || front->vehstatus.Test(VehState::Crashed) || assignment.phase != TBMHaulerPhase::ReturnEmpties) return front->index;
		// Empty sets still in the paid corridor or pair retain their actual exit authority and keep final claims protected.
		if (IsTBMHaulerInCorridor(*owner, front->index)) return front->index;
		// Releasing one hidden head must not remove an outside tail's magazine
		// exit authority. The complete actual chain must already have left.
		for (const Train *part = front; part != nullptr; part = part->Next()) {
			if (part->tile == assignment.bay || part->cargo.TotalCount() != 0) return front->index;
		}
	}
	return std::nullopt;
}

/** @copydoc ReleaseSettledTBMProjectHaulers */
bool ReleaseSettledTBMProjectHaulers(TBMProjectID project, bool execute)
{
	const auto *owner = GetTBMProject(project);
	if (owner == nullptr || owner->company != _current_company || GetTBMProjectSettlementBlocker(project).has_value()) return false;
	if (!execute) return true;
	// Validation covered the whole set before the first removal. Ending one
	// assignment cannot change another train's position or empty packet custody.
	while (const auto *assignment = GetTBMExchangeAssignment(project)) EndTBMExchangeAssignment(assignment->id);
	return true;
}

/** @copydoc IsTBMProjectExchangeIdle */
bool IsTBMProjectExchangeIdle(TBMProjectID project)
{
	if (std::ranges::any_of(_tbm_exchange_assignments, [project](const TBMExchangeAssignment &assignment) { return assignment.project == project; })) return false;
	// A set of the project still holding spoil, carried or not, would lose the only record of its origin leg.
	for (const auto &set : _tbm_wagon_sets) {
		if (set.project != project) continue;
		for (VehicleID wagon : set.wagons) {
			const Train *unit = Train::GetIfValid(wagon);
			if (unit != nullptr && IsTBMUnitLoaded(*unit)) return false;
		}
	}
	return true;
}

void ForgetTBMExchangeProject(TBMProjectID project)
{
	// A removed company's wagons were deleted first, and a retired project has no spoil in its sets; nothing is dropped.
	std::erase_if(_tbm_exchange_assignments, [project](const TBMExchangeAssignment &assignment) { return assignment.project == project; });
	std::erase_if(_tbm_wagon_sets, [project](const TBMWagonSet &set) { return set.project == project; });
}

CommandCost CheckTBMTrainRearrangement(const Train &src_head, const Train *dst_head)
{
	// A part may neither leave a cutter consist nor join one; only the project's restricted owners rearrange it.
	if (IsTBMCutterConsistPart(src_head) || (dst_head != nullptr && IsTBMCutterConsistPart(*dst_head))) return CommandCost(STR_ERROR_TBM_CONSIST_LOCKED);
	return CommandCost();
}

CommandCost CheckTBMRailVehicleSale(const Train &unit, bool sell_chain)
{
	if (IsTBMCutterConsistPart(unit)) return CommandCost(STR_ERROR_TBM_CONSIST_LOCKED);
	// Each unit the sale deletes: the chosen one, and with a chain sale every unit behind it. A set unit with spoil
	// aboard refuses; an empty one may go, and its set shrinks when it is deleted.
	for (const Train *sold = &unit; sold != nullptr; sold = sell_chain ? sold->GetNextUnit() : nullptr) {
		if (FindTBMWagonSetForWagon(sold->index) != nullptr && IsTBMUnitLoaded(*sold)) return CommandCost(STR_ERROR_TBM_SPOIL_ABOARD);
	}
	return CommandCost();
}

CommandCost CheckTBMRefit(const Train &unit)
{
	if (IsTBMCutterConsistPart(unit)) return CommandCost(STR_ERROR_TBM_CONSIST_LOCKED);
	if (FindTBMWagonSetForWagon(unit.GetFirstEnginePart()->index) != nullptr) return CommandCost(STR_ERROR_TBM_SET_REFIT);
	return CommandCost();
}

bool IsTBMReplacementLocked(const Train &part)
{
	return IsTBMCutterConsistPart(part) || FindTBMExchangeAssignmentForHauler(part.index) != nullptr || FindTBMWagonSetForWagon(part.index) != nullptr;
}

CommandCost CheckTBMExchangeBayRemoval(TileIndex tile, DoCommandFlags flags)
{
	// Company removal and flooding clear whatever stands there; the release afterwards still keeps the stores consistent.
	if (flags.Test(DoCommandFlag::Bankrupt) || _current_company == OWNER_WATER) return CommandCost();
	for (const TBMProject &project : CaptureTBMProjects()) {
		// An unfinished project using this bay must have its Source bay to fall back on; otherwise it keeps the depot.
		if (project.exchange_bay != tile || IsTBMProjectFinished(project.phase)) continue;
		if (ResolveTBMExchangeBay(project, project.portal).error != TBMExchangeError::None) return CommandCost(STR_ERROR_TBM_EXCHANGE_BAY_IN_USE);
	}
	return CommandCost();
}

void ReleaseTBMExchangeBay(TileIndex tile)
{
	ReleaseTBMProjectExchangeBay(tile);
	// A hauler queued for the removed bay now queues for its project's new bay; none stood in the depot.
	for (auto &assignment : _tbm_exchange_assignments) {
		if (assignment.bay != tile) continue;
		const TBMProject *project = GetTBMProject(assignment.project);
		if (project == nullptr || project->exchange_bay == INVALID_TILE) continue; // No bay left: the hauler simply never reaches one.
		assignment.bay = project->exchange_bay;
		++assignment.revision;
	}
}

CommandCost CmdAssignTBMHauler(DoCommandFlags flags, TBMProjectID project, VehicleID hauler)
{
	// Each refusal names what the player must change: the project, the chosen train or the store bound.
	switch (AssignTBMHauler(project, hauler, flags.Test(DoCommandFlag::Execute))) {
		case TBMExchangeError::None: return CommandCost();
		case TBMExchangeError::InvalidProject: return CommandCost(STR_ERROR_TBM_HAULER_PROJECT);
		case TBMExchangeError::MissingOrigin: return CommandCost(STR_ERROR_TBM_HAULER_ORIGIN);
		case TBMExchangeError::InvalidBay: return CommandCost(STR_ERROR_TBM_HAULER_BAY);
		case TBMExchangeError::InvalidHauler: return CommandCost(STR_ERROR_TBM_HAULER_TRAIN);
		case TBMExchangeError::Reserved: return CommandCost(STR_ERROR_TBM_HAULER_RESERVED);
		case TBMExchangeError::WrongWagons: return CommandCost(STR_ERROR_TBM_HAULER_WAGONS);
		default: return CommandCost(STR_ERROR_TBM_COMMISSION_LIMIT);
	}
}

CommandCost CmdReleaseTBMHauler(DoCommandFlags flags, VehicleID hauler)
{
	if (ReleaseTBMHauler(hauler, flags.Test(DoCommandFlag::Execute)) != TBMExchangeError::None) return CommandCost(STR_ERROR_TBM_HAULER_NOT_ASSIGNED);
	return CommandCost();
}

CommandCost CmdSetTBMExchangeBay(DoCommandFlags flags, TBMProjectID project, TileIndex tile)
{
	const TBMProject *record = GetTBMProject(project);
	if (record == nullptr || record->company != _current_company || IsTBMProjectFinished(record->phase)) return CMD_ERROR;
	// A working bay with queued haulers cannot move under them; release them first. A lost bay (Plan 4 Group 17.3) may
	// be replaced under them, and its queue follows to the new bay.
	const bool lost = GetTBMExchangeBay(*record).error != TBMExchangeError::None;
	if ((!lost && GetTBMExchangeAssignment(project) != nullptr) || ResolveTBMExchangeBay(*record, tile).error != TBMExchangeError::None) return CMD_ERROR;
	if (SetTBMProjectExchangeBay(project, tile, flags.Test(DoCommandFlag::Execute)) != TBMProjectError::None) return CMD_ERROR;
	if (!flags.Test(DoCommandFlag::Execute)) return CommandCost();
	for (auto &assignment : _tbm_exchange_assignments) {
		if (assignment.project != project || assignment.bay == tile) continue;
		assignment.bay = tile;
		++assignment.revision;
	}
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
