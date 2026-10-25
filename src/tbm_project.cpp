/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_project.cpp Typed project store, assignment validation, recoverable references, phase steps, player
 * controls and staged loading. A project is bound to one cutter by ID; ordinary train orders stay with the train and
 * never carry a project. The one tick step observes the face facts and runs the face work of Groups 6-8.
 * Recurring active and idle cost is charged from the saved duration buckets. The exchange edges (task 4.2B) read the
 * Plan 5 bay admission and the actual wagon contents.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 3-8 and 12, task 4.2B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Groups 1-5 (exchange bay)
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native phase and cargo ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Groups23/23CC (installed face wait and unfinished work-change command).
 */

#include "stdafx.h"
#include "tbm.h"
#include "tbm_hangar.h"
#include "pbs.h"
#include "tbm_cmd.h"
#include "tbm_replanning.h"
#include "tbm_area_preview.h"
#include "tbm_exchange.h"
#include "tbm_controls.h"
#include "train_cmd.h"
#include "command_func.h"
#include "train.h"
#include "station_base.h"
#include "company_base.h"
#include "company_func.h"
#include "layer_portal_map.h"
#include "rail_map.h"
#include "direction_func.h"
#include "map_func.h"
#include "debug.h"
#include "timer/timer_game_tick.h"
#include "timer/timer_game_calendar.h"
#include "core/backup_type.hpp"
#include "table/strings.h"

#include "safeguards.h"

static std::vector<TBMProject> _tbm_projects; ///< Live projects in strictly increasing ID order.

/** @copydoc GetTBMProjectForExchangeBay */
const TBMProject *GetTBMProjectForExchangeBay(TileIndex bay)
{
	if (bay == INVALID_TILE) return nullptr;
	const auto it = std::ranges::find(_tbm_projects, bay, &TBMProject::exchange_bay);
	return it == _tbm_projects.end() ? nullptr : &*it;
}
static TBMProjectID _next_tbm_project{0}; ///< Next identity; IDs are never reused.
static std::optional<std::pair<std::vector<TBMProject>, TBMProjectID>> _pending_tbm_projects; ///< Unpublished loaded store.

/**
 * @param id Exact identity.
 * @return Mutable live record, or nullptr.
 */
static TBMProject *FindTBMProject(TBMProjectID id)
{
	const auto it = std::ranges::lower_bound(_tbm_projects, id, {}, &TBMProject::id);
	return it == _tbm_projects.end() || it->id != id ? nullptr : &*it;
}

/**
 * @param id Exact project identity.
 * @return The live project, or nullptr.
 */
const TBMProject *GetTBMProject(TBMProjectID id)
{
	return FindTBMProject(id);
}

/**
 * @param cutter Actual front engine.
 * @return The project assigned to this cutter, or nullptr.
 */
const TBMProject *GetTBMProjectForCutter(VehicleID cutter)
{
	if (cutter == VehicleID::Invalid()) return nullptr;
	const auto it = std::ranges::find(_tbm_projects, cutter, &TBMProject::cutter);
	return it == _tbm_projects.end() ? nullptr : &*it;
}

/** @return Copy of every live project in increasing ID order. */
std::vector<TBMProject> CaptureTBMProjects()
{
	return _tbm_projects;
}

/** @return Next identity the store will allocate. */
TBMProjectID GetNextTBMProjectID()
{
	return _next_tbm_project;
}

/**
 * @param phase Saved phase.
 * @return Whether the project has ended and keeps only history.
 */
bool IsTBMProjectFinished(TBMPhase phase)
{
	return phase == TBMPhase::Completed || phase == TBMPhase::Cancelled;
}

/*
 * Live-reference predicates. Each answers "may this reference drive work right now?" against the real owner.
 * They never mutate; normalization and validation combine them with the saved blocked reason.
 */

/**
 * @param project Record to check.
 * @return The live cutter front engine of the project's company and model, or nullptr.
 */
const Train *GetLiveTBMCutter(const TBMProject &project)
{
	const Train *front = Train::GetIfValid(project.cutter);
	// Only this company's front engine of this exact cutter model can carry the assignment.
	if (front == nullptr || front->owner != project.company || !front->IsFrontEngine() || GetTBMEngineModel(front->engine_type) != project.model_id) return nullptr;
	return front;
}

/**
 * @param project Record to check.
 * @return Whether the live cutter's chain still matches the saved permanent machinery prefix.
 */
static bool IsTBMConsistLive(const TBMProject &project)
{
	const Train *front = GetLiveTBMCutter(project);
	// A project can drive only an intact model-sized permanent chain; missing or oversized machinery cannot resume.
	if (front == nullptr || project.permanent_parts.size() < 2 || project.permanent_parts.size() > MAX_TBM_PERMANENT_PARTS) return false;
	return InspectTBMConsist(*front, project).error == TBMBlockedReason::None;
}

/**
 * @param tile Saved portal reference.
 * @return Whether it names the canonical upper key of some pair (it may still be foreign or the wrong transport).
 */
static bool IsTBMPortalKey(TileIndex tile)
{
	// Invalid stored addresses have no physical portal owner and must not be read as map data.
	if (tile == INVALID_TILE || !IsValidTile(tile)) return false;
	const auto key = GetLayerPortalPairKey(tile);
	return key.has_value() && *key == tile;
}

/**
 * @param project Record to check.
 * @return The live geological job this project may work on, or nullptr.
 */
const ExcavationJob *GetLiveTBMJob(const TBMProject &project)
{
	if (project.job == ExcavationJobID{UINT32_MAX}) return nullptr;
	const auto *job = GetExcavationJob(project.job);
	// The job must belong to this company and model, and be unbound or bound to this same cutter.
	if (job == nullptr || job->company != project.company || job->model_id != project.model_id || (job->cutter != VehicleID::Invalid() && job->cutter != project.cutter)) return nullptr;
	// A cancelled or completed job no longer cuts, but it still anchors the project's return, final exchange and history
	// (Group 17.1); the phase step turns every forward phase of a cancelled job back to the bay.
	// When the portal is a canonical pair key, the job must start in that pair's rock-side housing; any other portal
	// value is the portal check's refusal, not evidence that the job is foreign.
	if (IsTBMPortalKey(project.portal) && job->commissioning_anchor != GetLayerPortalPairTile(project.portal)) return nullptr;
	return job;
}

/**
 * @param project Record to check.
 * @return Whether the portal is this company's rail pair and, if it is Commissioning this job, names this cutter;
 *         a project still in its Commissioning phase additionally needs exactly that binding.
 */
static bool IsTBMPortalLive(const TBMProject &project)
{
	if (!IsTBMPortalKey(project.portal)) return false;
	const auto *record = GetLayerPortalWorkRecord(project.portal);
	// Only the original company's rail pair can protect the machine's approach and return path.
	if (record == nullptr || record->company != project.company || GetLayerPortalTransportType(project.portal) != TransportType::Rail) return false;
	const bool binds = project.job != ExcavationJobID{UINT32_MAX} && record->state == LayerPortalWorkState::Commissioning && record->project == project.job;
	if (binds && record->cutter != project.cutter) return false; // The pair Commissioning this job belongs to another train.
	return project.phase != TBMPhase::Commissioning || binds;
}

/**
 * @param project Record to check.
 * @return Whether the origin is this company's Surface railway station.
 */
static bool IsTBMOriginLive(const TBMProject &project)
{
	const auto *origin = Station::GetIfValid(project.origin);
	// Exchange and haulage start at a real owned Surface railway facility, never an underground or foreign station.
	return origin != nullptr && origin->owner == project.company && IsValidTile(origin->xy) && GetTileLayer(origin->xy) == MapLayer::Surface && origin->facilities.Test(StationFacility::Train);
}

/**
 * @param project Record to check.
 * @return Whether the project's pair is live and its exchange bay resolves: the source side of the commissioning
 *         pair, or a valid excavated company depot bay (Plan 5 Group 1).
 */
static bool IsTBMExchangeLive(const TBMProject &project)
{
	return IsTBMPortalKey(project.portal) && GetTBMExchangeBay(project).error == TBMExchangeError::None;
}

/**
 * Find the first lost reference.
 * @param project Record to check against the live owners.
 * @return Matching recoverable reason, or None.
 */
TBMBlockedReason FindTBMProjectReferenceLoss(const TBMProject &project)
{
	if (GetLiveTBMCutter(project) == nullptr) return TBMBlockedReason::MissingCutter;
	if (!IsTBMConsistLive(project)) return TBMBlockedReason::InvalidConsist;
	if (GetLiveTBMJob(project) == nullptr) return TBMBlockedReason::MissingProject;
	if (!IsTBMPortalLive(project)) return TBMBlockedReason::MissingPortal;
	if (!IsTBMOriginLive(project)) return TBMBlockedReason::MissingOrigin;
	if (!IsTBMExchangeLive(project)) return TBMBlockedReason::MissingExchange;
	return TBMBlockedReason::None;
}

/**
 * Turn lost references into explicit recoverable state.
 * @param project Record to normalize in place.
 * @return Whether anything changed.
 */
bool NormalizeTBMProjectReferences(TBMProject &project)
{
	const TBMProject before = project;
	// IDs that resolve to nothing are cleared, so a later reuse of the same ID cannot silently rebind the project.
	if (project.cutter != VehicleID::Invalid() && !Train::IsValidID(project.cutter)) {
		project.cutter = VehicleID::Invalid();
		project.permanent_parts.clear();
	}
	if (project.job != ExcavationJobID{UINT32_MAX} && GetExcavationJob(project.job) == nullptr) project.job = ExcavationJobID{UINT32_MAX};
	if (project.origin != StationID::Invalid() && !Station::IsValidID(project.origin)) project.origin = StationID::Invalid();
	if (project.portal != INVALID_TILE && !IsTBMPortalKey(project.portal)) {
		if (project.exchange_bay == project.portal) project.exchange_bay = INVALID_TILE; // The commissioning exchange went with its pair.
		project.portal = INVALID_TILE;
	}
	if (project.exchange_bay != INVALID_TILE && !IsValidTile(project.exchange_bay)) project.exchange_bay = INVALID_TILE;
	// Only an unblocked project takes the loss as its reason; an existing block (such as a pause) is kept.
	if (project.blocked == TBMBlockedReason::None) project.blocked = FindTBMProjectReferenceLoss(project);
	return !(project == before);
}

/**
 * Check the complete physical consist before declaring it parked.
 * @param project Actual job/vehicle/bay association.
 * @return Whether every real part is stationary inside the bay and no cargo remains aboard.
 */
static bool IsTBMReadyToPark(const TBMProject &project)
{
	const Train *front = Train::GetIfValid(project.cutter);
	if (front == nullptr) return false;
	if (CheckTBMExchangeParticipant(*front, GetTBMExchangeBay(project), project.company) != TBMExchangeError::None) return IsTBMStored(project.phase) && IsTBMWholeInHangar(*front);
	// The head's arrival cannot conceal an occupied tail or a loaded wagon;
	// native bay admission has already checked every part's actual position.
	for (const Train *part = front; part != nullptr; part = part->Next()) {
		if (part->cargo.TotalCount() != 0) return false;
	}
	return true;
}

/** @copydoc BeginTBMHangarReturn */
bool BeginTBMHangarReturn(Train &front)
{
	const auto *current = GetTBMProjectForCutter(front.index);
	// A real depot order may interrupt only a live, revisable work assignment; final settlement already owns finishing travel.
	if (current == nullptr || IsTBMProjectFinished(current->phase) || IsTBMFinishing(current->phase) || !front.current_order.IsType(OT_GOTO_DEPOT) || current->revision >= UINT64_MAX - 1) return false;
	// Capture before mutation: current and the mutable record refer to the same live controller.
	const TBMPhase before = current->phase;
	TBMProject *project = FindTBMProject(current->id);
	bool loaded = false;
	for (const Train *part = &front; part != nullptr; part = part->Next()) loaded |= part->cargo.TotalCount() != 0; // Every attached packet must first return through its existing exchange owner.
	if (HasTBMHangarHaltOrder(front)) {
		project->phase = loaded ? (IsTBMWaitingForExchange(project->phase) ? TBMPhase::WaitForHangar : TBMPhase::ReturnForHangar) : TBMPhase::TravelToHangar;
	} else {
		// A replacement service order owns travel immediately and rejoins the
		// existing exchange/face afterward. It cannot leave an obsolete parking
		// phase waiting for a Halt that the native order no longer contains.
		project->phase = loaded ? TBMPhase::ReturnForExchange : TBMPhase::TravelToFace;
	}
	project->blocked = TBMBlockedReason::None;
	project->phase_ticks = 0;
	++project->revision;
	if (IsTBMReturning(project->phase)) {
		// At the held face the bay lies behind the cutter. Back along paid support while corridor admission still holds it for any hauler.
		if (IsTBMAwaitingFaceTransfer(before)) {
			FreeTrainTrackReservation(&front);
			BackUpTBMConsist(front);
		} else {
			ChooseTBMReturnDirection(front);
		}
	}
	return true;
}

/** @copydoc CompleteTBMHangarArrival */
bool CompleteTBMHangarArrival(const Train &front)
{
	const auto *current = GetTBMProjectForCutter(front.index);
	// The selected halt completes only after the whole actual consist arrives, never on a head-only or service-only visit.
	if (current == nullptr || current->phase != TBMPhase::TravelToHangar || !HasTBMHangarHaltOrder(front) || !IsTBMWholeInHangar(front)) return false;
	const auto destination = GetTBMOrderedHangar(front);
	// Arrival at some other depot cannot satisfy the player's retained destination.
	if (destination.error != TBMHangarError::None || destination.tile != front.tile) return false;
	TBMProject *project = FindTBMProject(current->id);
	project->phase = TBMPhase::ParkedHangar;
	project->blocked = TBMBlockedReason::None;
	project->phase_ticks = 0;
	++project->revision;
	return true;
}

/** @copydoc FollowTBMVehicleOrders */
bool FollowTBMVehicleOrders(Train &front)
{
	const auto *current = GetTBMProjectForCutter(front.index);
	// Ordinary orders may take over active work, but cannot reopen history or interrupt final claim/cargo settlement.
	if (current == nullptr || IsTBMProjectFinished(current->phase) || IsTBMFinishing(current->phase) || current->revision >= UINT64_MAX - 1) return false;
	TBMProject *project = FindTBMProject(current->id);
	FreeTrainTrackReservation(&front);
	if (front.current_order.IsType(OT_LOADING)) front.LeaveStation(); // Finish native station handling before selecting another real travel leg.
	front.current_order.Free();
	front.SetDestTile(INVALID_TILE);
	project->phase = TBMPhase::FollowOrders;
	project->blocked = TBMBlockedReason::None;
	project->phase_ticks = 0;
	++project->revision;
	return true;
}

/**
 * Validate one project against the live owners.
 * @param project Complete record with an allocated ID.
 * @return None or the first stable refusal.
 */
TBMProjectError ValidateTBMProject(const TBMProject &project)
{
	if (project.id == TBMProjectID{UINT32_MAX}) return TBMProjectError::InvalidProject;
	// Unknown phases or reasons cannot be resumed, and an unusable stamp cannot order later transitions.
	if (to_underlying(project.phase) > to_underlying(TBMPhase::AwaitFaceTransfer) || to_underlying(project.blocked) >= to_underlying(TBMBlockedReason::End) || !IsValidDirection(project.heading) || project.revision == 0 || project.revision == UINT64_MAX) return TBMProjectError::InvalidProject;
	const auto *model = GetTBMCutterModel(project.model_id);
	// The fractional work remainder is a numerator below the model's own divisor; anything else was never produced.
	if (model == nullptr || project.work_remainder >= model->work_rate_denominator) return TBMProjectError::InvalidProject;
	if (!Company::IsValidID(project.company)) return TBMProjectError::InvalidOwner;
	const bool blocked = project.blocked != TBMBlockedReason::None;

	/* Cutter and machinery. A cleared cutter carries no parts; a kept cutter carries a bounded part list. */
	if (project.cutter == VehicleID::Invalid()) {
		if (!project.permanent_parts.empty()) return TBMProjectError::InvalidConsist;
	} else {
		if (!Train::IsValidID(project.cutter)) return TBMProjectError::MissingCutter; // Dangling: normalization clears it.
		// Permanent machinery must fit the bounded native cutter layout before any phase can use it.
		if (project.permanent_parts.size() < 2 || project.permanent_parts.size() > MAX_TBM_PERMANENT_PARTS) return TBMProjectError::InvalidConsist;
	}
	// Recoverable blocked history may retain unavailable participants; runnable work must have both the live head and chain.
	if (!blocked && GetLiveTBMCutter(project) == nullptr) return TBMProjectError::MissingCutter;
	if (!blocked && !IsTBMConsistLive(project)) return TBMProjectError::InvalidConsist;

	/* Remaining references: dangling IDs never pass; anything not live needs a recoverable block. */
	if (project.job != ExcavationJobID{UINT32_MAX} && GetExcavationJob(project.job) == nullptr) return TBMProjectError::MissingJob;
	if (!blocked && GetLiveTBMJob(project) == nullptr) return TBMProjectError::MissingJob;
	if (project.origin != StationID::Invalid() && !Station::IsValidID(project.origin)) return TBMProjectError::MissingOrigin;
	if (!blocked && !IsTBMOriginLive(project)) return TBMProjectError::MissingOrigin;
	if (project.portal != INVALID_TILE && !IsTBMPortalKey(project.portal)) return TBMProjectError::MissingPortal;
	if (!blocked && !IsTBMPortalLive(project)) return TBMProjectError::MissingPortal;
	if (project.exchange_bay != INVALID_TILE && !IsValidTile(project.exchange_bay)) return TBMProjectError::MissingPortal;
	if (!blocked && !IsTBMExchangeLive(project)) return TBMProjectError::MissingPortal;
	if (IsTBMStored(project.phase) && project.cutter != VehicleID::Invalid() && !IsTBMReadyToPark(project)) return TBMProjectError::InvalidConsist; // A saved stored label cannot replace full physical withdrawal and empty wagons; a lost cutter retains only recoverable cancellation intent.
	return TBMProjectError::None;
}

/**
 * Validate a complete project set.
 * @param projects Strictly increasing IDs, each below next_id.
 * @param next_id Saved allocation cursor.
 * @return None or the first stable refusal.
 */
TBMProjectError ValidateTBMProjects(std::span<const TBMProject> projects, TBMProjectID next_id)
{
	// The saved store must leave a valid non-reused identity cursor within the native project limit.
	if (projects.size() > MAX_TBM_PROJECTS || next_id == TBMProjectID{UINT32_MAX}) return TBMProjectError::Limit;
	std::set<VehicleID> cutters;
	std::set<ExcavationJobID> jobs;
	// Identities increase strictly below the cursor; each kept cutter and each kept job has at most one project.
	for (size_t i = 0; i < projects.size(); ++i) {
		const auto &project = projects[i];
		if (project.id.base() >= next_id.base() || (i != 0 && project.id.base() <= projects[i - 1].id.base())) return TBMProjectError::Duplicate;
		if (project.cutter != VehicleID::Invalid() && !cutters.insert(project.cutter).second) return TBMProjectError::Duplicate;
		if (project.job != ExcavationJobID{UINT32_MAX} && !jobs.insert(project.job).second) return TBMProjectError::Duplicate;
		const auto error = ValidateTBMProject(project);
		if (error != TBMProjectError::None) return error;
	}
	return TBMProjectError::None;
}

/**
 * Allocate an ID for a prepared project and add it to the store.
 * @param project Prepared project with an invalid ID; receives the allocated ID and revision 1 on success.
 * @param execute False validates only.
 * @return None or an unchanged refusal.
 */
TBMProjectError PublishTBMProject(TBMProject &project, bool execute)
{
	if (project.id != TBMProjectID{UINT32_MAX}) return TBMProjectError::InvalidProject;
	// Only the acting live company may acquire a controller and its exclusive work participants.
	if (!Company::IsValidID(project.company) || project.company != _current_company) return TBMProjectError::InvalidOwner;
	if (project.blocked != TBMBlockedReason::None || IsTBMProjectFinished(project.phase)) return TBMProjectError::InvalidProject; // A fresh project starts unblocked.
	// Reject exhaustion before publishing either a project or its never-reused identity.
	if (_tbm_projects.size() >= MAX_TBM_PROJECTS || _next_tbm_project.base() >= UINT32_MAX - 1) return TBMProjectError::Limit;
	// One project per cutter and per job: a second assignment would let two controllers drive one train or one face.
	for (const auto &existing : _tbm_projects) {
		if (existing.cutter == project.cutter || existing.job == project.job) return TBMProjectError::Duplicate;
	}
	TBMProject candidate = project;
	candidate.id = _next_tbm_project;
	candidate.revision = 1;
	// Every reference must be live at publication; recoverable losses can only arise later.
	if (FindTBMProjectReferenceLoss(candidate) != TBMBlockedReason::None) {
		const auto error = ValidateTBMProject(candidate);
		return error != TBMProjectError::None ? error : TBMProjectError::InvalidProject;
	}
	const auto error = ValidateTBMProject(candidate);
	if (error != TBMProjectError::None) return error;
	if (!execute) return TBMProjectError::None;
	_tbm_projects.push_back(candidate);
	_next_tbm_project = TBMProjectID{_next_tbm_project.base() + 1};
	project = std::move(candidate);
	return TBMProjectError::None;
}

/**
 * @param front Cutter front engine.
 * @param project Its project.
 * @return Whether the whole consist stands withdrawn and empty: stationary, every part on one tile in the depot state,
 *         that tile being the project's Source bay or a genuine rail depot, and no cargo aboard any part.
 */
static bool IsTBMConsistWithdrawn(const Train &front, const TBMProject &project)
{
	if (IsTBMWholeInHangar(front)) return true;
	const TileIndex tile = front.tile;
	// Withdrawal requires a stationary head at an actual storage owner; a stopped train on running track still occupies work.
	if (front.cur_speed != 0 || (tile != project.portal && !IsRailDepotTile(tile))) return false;
	// Outside the work pocket and on one tile, with no spoil or other cargo leaving the project with the train.
	for (const Train *part = &front; part != nullptr; part = part->Next()) {
		if (part->tile != tile || part->track != Track::Depot || part->cargo.TotalCount() != 0) return false;
	}
	return true;
}

/** @copydoc GetTBMProjectReleaseProblem */
StringID GetTBMProjectReleaseProblem(TBMProjectID id)
{
	const auto *project = GetTBMProject(id);
	if (project == nullptr) return STR_ERROR_TBM_PROJECT_CANT_RETIRE;
	AutoRestoreBackup actor(_current_company, project->company); // Pure owner preflights must also work during native ticks; the actual command checks its caller separately.
	// Only ended or physically stored work can release claims, and any retained cutter must be wholly withdrawn.
	if (!IsTBMProjectFinished(project->phase) && !IsTBMStored(project->phase)) return STR_ERROR_TBM_RELEASE_RETURN;
	const Train *front = Train::GetIfValid(project->cutter);
	if (front != nullptr && !IsTBMConsistWithdrawn(*front, *project)) return STR_ERROR_TBM_RELEASE_RETURN;
	if (front == nullptr && project->phase == TBMPhase::Parked) return STR_ERROR_TBM_RELEASE_RETURN;
	if (GetTBMProjectSettlementBlocker(id).has_value()) return STR_ERROR_TBM_RELEASE_HAULAGE;
	if (IsTBMCorridorOccupied(*project)) return STR_ERROR_TBM_RELEASE_PORTAL; // Another real train on accepted support must clear before this operation releases its work protection.
	const auto *job = GetExcavationJob(project->job);
	const bool cancel_job = job != nullptr && job->state != ExcavationJobState::Completed && job->state != ExcavationJobState::Cancelled;
	// Still-open geology must admit cancellation before release can change any participant or portal binding.
	if (cancel_job && CancelExcavationJob(project->job, false) != ExcavationError::None) return STR_ERROR_TBM_PROJECT_CANT_RETIRE;
	const auto *record = IsTBMPortalKey(project->portal) ? GetLayerPortalWorkRecord(project->portal) : nullptr;
	const bool bound = record != nullptr && record->state == LayerPortalWorkState::Commissioning && (record->project == project->job || record->cutter == project->cutter);
	// A commissioning pair remains protected until its own physical withdrawal preflight also accepts the release.
	if (bound && CheckLayerPortalWorkWithdrawal(project->portal, project->job, project->cutter, cancel_job) != LayerPortalWorkError::None) return STR_ERROR_TBM_RELEASE_PORTAL;
	return INVALID_STRING_ID;
}

/**
 * Finish/cancel and release fully withdrawn, settled work while retaining the actual native machine.
 * @param project Record of the command actor.
 * @param execute False validates only.
 * @return None, or Unavailable with all owners unchanged until the whole release operation is admitted.
 */
static TBMProjectError RetireTBMProject(const TBMProject &project, bool execute)
{
	if (GetTBMProjectReleaseProblem(project.id) != INVALID_STRING_ID) return TBMProjectError::Unavailable;
	// The pair Commissioning this project is withdrawn with it (r2mlvs3 Group 11.3), once its rock side is empty;
	// otherwise the record would stay bound to an ordinary train that may then be sold.
	const auto *record = IsTBMPortalKey(project.portal) ? GetLayerPortalWorkRecord(project.portal) : nullptr;
	const bool bound = record != nullptr && record->state == LayerPortalWorkState::Commissioning && (record->project == project.job || record->cutter == project.cutter);
	if (!execute) return TBMProjectError::None;
	// Every cargo, vehicle, claim, owner and portal revision has passed before
	// any publication. These existing owner commits do not allocate or move a train.
	[[maybe_unused]] const bool settled = ReleaseSettledTBMProjectHaulers(project.id, true);
	assert(settled);
	if (Train *front = Train::GetIfValid(project.cutter); front != nullptr) front->vehstatus.Set(VehState::Stopped); // Explicit finish retains the stationary whole native machine in storage after its controller is gone.
	const auto *job = GetExcavationJob(project.job);
	// Cancellation changes only unfinished geology; completed/cancelled histories keep their existing paid cuts.
	if (job != nullptr && job->state != ExcavationJobState::Completed && job->state != ExcavationJobState::Cancelled) {
		[[maybe_unused]] const auto cancelled = CancelExcavationJob(project.job);
		assert(cancelled == ExcavationError::None);
	}
	if (bound) {
		[[maybe_unused]] const auto withdrawn = WithdrawLayerPortalWorkPair(project.portal, project.job, project.cutter, true);
		assert(withdrawn == LayerPortalWorkError::None);
	}
	const TBMProjectID id = project.id;
	ForgetTBMExchangeProject(id); // Only empty sets are left; their wagons simply stop being a set.
	/** @param candidate Original controller under consideration. @return Whether it is the one fully settled project. */
	std::erase_if(_tbm_projects, [id](const TBMProject &candidate) { return candidate.id == id; });
	return TBMProjectError::None;
}

/** @copydoc ControlTBMProject */
TBMProjectError ControlTBMProject(TBMProjectID id, TBMProjectAction action, bool execute)
{
	if (to_underlying(action) >= to_underlying(TBMProjectAction::End)) return TBMProjectError::InvalidProject;
	TBMProject *project = FindTBMProject(id);
	if (project == nullptr) return TBMProjectError::InvalidProject;
	if (project->company != _current_company) return TBMProjectError::InvalidOwner;
	if (action == TBMProjectAction::Retire) return RetireTBMProject(*project, execute); // Final release is separate from resumable parking, and also admits safely parked unfinished work.
	if (IsTBMProjectFinished(project->phase)) return TBMProjectError::Unavailable;
	TileIndex resume_bay = INVALID_TILE;
	// Handing native order travel back to work requires a real bay visit before the accepted cutting route resumes.
	if (action == TBMProjectAction::Resume && project->phase == TBMPhase::FollowOrders) {
		const auto bay = GetTBMExchangeBay(*project);
		// Quote the complete native travel admission before recording the resume intent.
		if (bay.error != TBMExchangeError::None || Command<Commands::SendTBMToHangar>::Do({}, project->cutter, bay.tile, true).Failed()) return TBMProjectError::Unavailable;
		resume_bay = bay.tile; // A real native service visit collects the whole train before its normal working departure.
	}
	TBMProject next = *project;
	switch (action) {
		case TBMProjectAction::Pause:
			if (project->blocked == TBMBlockedReason::Paused || IsTBMStored(project->phase)) return TBMProjectError::Unavailable; // Stored work is already suspended; its next deliberate action is Resume or final release.
			next.blocked = TBMBlockedReason::Paused; // The phase is kept.
			break;
		case TBMProjectAction::Resume:
			if (project->blocked != TBMBlockedReason::Paused && !IsTBMStored(project->phase) && project->phase != TBMPhase::FollowOrders) return TBMProjectError::Unavailable; // Native travel suspends work until this explicit handback.
			if (project->phase == TBMPhase::FollowOrders) {
				next.phase = TBMPhase::ReturnForExchange;
				next.phase_ticks = 0;
			}
			if (IsTBMStored(project->phase)) {
				if (!IsTBMReadyToPark(*project)) return TBMProjectError::Unavailable;
				next.phase = TBMPhase::Resume; // Reuse native bay departure and accepted-face rejoin, preserving partial work.
				next.phase_ticks = 0;
			}
			next.blocked = TBMBlockedReason::None;
			NormalizeTBMProjectReferences(next); // A loss that happened during the pause becomes the reason at once.
			break;
		case TBMProjectAction::ReturnAndPark:
			// Repeated requests do not reset a physical return, timer or exchange;
			// missing machinery/bay must be recovered rather than fabricated.
			if (IsTBMParking(project->phase) || GetLiveTBMCutter(*project) == nullptr || GetTBMExchangeBay(*project).error != TBMExchangeError::None) return TBMProjectError::Unavailable;
			next.phase = IsTBMWaitingForExchange(project->phase) ? TBMPhase::WaitToPark : TBMPhase::ReturnToPark;
			next.phase_ticks = 0;
			next.blocked = TBMBlockedReason::None;
			NormalizeTBMProjectReferences(next);
			break;
		case TBMProjectAction::Cancel: {
			const auto *job = project->job == ExcavationJobID{UINT32_MAX} ? nullptr : GetExcavationJob(project->job);
			const bool lost = project->cutter == VehicleID::Invalid();
			const bool open = job != nullptr && job->state != ExcavationJobState::Cancelled && job->state != ExcavationJobState::Completed;
			if (IsTBMFinishing(project->phase)) return TBMProjectError::Unavailable;
			// Final release must remain possible for the open job, or preserve a lost-cutter settlement obligation.
			if (open && CancelExcavationJob(project->job, false) != ExcavationError::None) return TBMProjectError::Unavailable;
			if (!open && !lost) return TBMProjectError::Unavailable;
			// Save only the player's final-release intent now. Work and future
			// claims remain with geology until the complete physical withdrawal
			// and real haulage settlement pass the same final-release preflight.
			next.phase = lost || IsTBMStored(project->phase) ? TBMPhase::ReleasePending :
				IsTBMWaitingForExchange(project->phase) ? TBMPhase::WaitToRelease : TBMPhase::ReturnToRelease;
			next.phase_ticks = 0;
			next.blocked = TBMBlockedReason::None;
			NormalizeTBMProjectReferences(next);
			break;
		}
		default:
			return TBMProjectError::Unavailable;
	}
	if (!execute) return TBMProjectError::None;
	const bool resume_orders = action == TBMProjectAction::Resume && project->phase == TBMPhase::FollowOrders;
	if (IsTBMStored(project->phase) || resume_orders || action == TBMProjectAction::ReturnAndPark || action == TBMProjectAction::Cancel) {
		// A requested return or departure explicitly starts the same machine;
		// ordinary pause/resume elsewhere never overrides a player's stop flag.
		Train *front = Train::GetIfValid(project->cutter);
		if (front != nullptr) {
			// This explicit work action supersedes a temporary native travel
			// order. Preserve the ordinary order list and the geological job.
			FreeTrainTrackReservation(front);
			if (front->current_order.IsType(OT_LOADING)) front->LeaveStation();
			front->current_order.MakeDummy();
			front->SetDestTile(INVALID_TILE);
			front->vehstatus.Reset(VehState::Stopped);
		}
	}
	const TBMPhase before = project->phase;
	++next.revision;
	*project = std::move(next);
	// Explicit park/cancel requests select a physical return direction only when their resulting phase actually travels back.
	if ((action == TBMProjectAction::ReturnAndPark || action == TBMProjectAction::Cancel) && IsTBMReturning(project->phase)) {
		if (Train *front = Train::GetIfValid(project->cutter); front != nullptr) {
			// A request at the face must back out of the dead end, even if the previous approach search preferred forward.
			// Travelling requests elsewhere keep their existing native two-end route choice.
			if (before == TBMPhase::Bore || before == TBMPhase::AdvanceSupport || IsTBMAwaitingFaceTransfer(before)) {
				FreeTrainTrackReservation(front);
				BackUpTBMConsist(*front);
			} else {
				ChooseTBMReturnDirection(*front);
			}
		}
	}
	if (resume_orders) {
		// The earlier estimate prepared the real facility/pool admission. Native
		// depot travel and arrival preserve every part; existing exchange/resume
		// phases then return the cutter to its face with its working end leading.
		[[maybe_unused]] const auto travel = Command<Commands::SendTBMToHangar>::Do(DoCommandFlag::Execute, project->cutter, resume_bay, true);
		assert(travel.Succeeded());
	}
	return TBMProjectError::None;
}

/**
 * Move a project's exchange to another bay that the exchange owner has validated.
 * @param id Project of the command actor, not finished.
 * @param bay New rearrangement tile.
 * @param execute False validates only.
 * @return None, InvalidProject, InvalidOwner or Unavailable.
 */
TBMProjectError SetTBMProjectExchangeBay(TBMProjectID id, TileIndex bay, bool execute)
{
	TBMProject *project = FindTBMProject(id);
	if (project == nullptr) return TBMProjectError::InvalidProject;
	if (project->company != _current_company) return TBMProjectError::InvalidOwner;
	// Historical work and unchanged bay requests cannot acquire a new exchange binding or revision.
	if (IsTBMProjectFinished(project->phase) || project->exchange_bay == bay) return TBMProjectError::Unavailable;
	if (!execute) return TBMProjectError::None;
	project->exchange_bay = bay;
	++project->revision;
	return TBMProjectError::None;
}

/** @param bay Removed exchange tile; affected projects fall back to their original Source when it still exists. */
void ReleaseTBMProjectExchangeBay(TileIndex bay)
{
	// Only projects using this removed physical bay lose the binding; unrelated work keeps its existing exchange.
	for (auto &project : _tbm_projects) {
		if (project.exchange_bay != bay) continue;
		// The Source bay is the project's own portal key; without one the project has no bay at all.
		project.exchange_bay = IsTBMPortalKey(project.portal) ? project.portal : INVALID_TILE;
		++project.revision;
	}
}

/**
 * Transfer original project ownership or settle the removed company's work without recycling project identities.
 * @param old_owner Company whose existing work is changing ownership.
 * @param new_owner Buyer, or INVALID_OWNER when the company has been removed.
 */
void ChangeTBMOwnership(Owner old_owner, Owner new_owner)
{
	if (new_owner != INVALID_OWNER) {
		ChangeExcavationJobOwnership(old_owner, new_owner);
		// A merger moves each project to the buyer; its vehicles, stations and track move in the same company change,
		// and its assignments and sets take their company from the project.
		for (auto &project : _tbm_projects) {
			if (project.company != old_owner) continue;
			project.company = new_owner;
			++project.revision;
		}
		return;
	}
	// A removed company's projects end with it; its trains are already deleted. No controller can ever resume their
	// work, so each job is cancelled (its unfinished cells return to rock, cavities stay) and its Commissioning pair is
	// released, so the company's tiles can be cleared. Its other jobs become explicit orphans below.
	std::vector<TBMProject> removed;
	for (const auto &project : _tbm_projects) {
		if (project.company == old_owner) removed.push_back(project);
	}
	AutoRestoreBackup actor(_current_company, OWNER_DEITY); // The lifecycle owner acts for the removed company.
	// Settle each removed project's original geology and commissioning association before erasing its controller.
	for (const auto &project : removed) {
		const auto *job = project.job == ExcavationJobID{UINT32_MAX} ? nullptr : GetExcavationJob(project.job);
		if (job != nullptr && job->state != ExcavationJobState::Completed && job->state != ExcavationJobState::Cancelled) {
			[[maybe_unused]] const auto cancelled = CancelExcavationJob(project.job);
		}
		const auto *record = IsTBMPortalKey(project.portal) ? GetLayerPortalWorkRecord(project.portal) : nullptr;
		if (record != nullptr && record->state == LayerPortalWorkState::Commissioning && record->project == project.job) ReleaseLayerPortalWorkBinding(project.portal);
		ForgetTBMExchangeProject(project.id);
	}
	/** @param project Original controller. @return Whether its removed company has just been settled. */
	std::erase_if(_tbm_projects, [old_owner](const TBMProject &project) { return project.company == old_owner; });
	ChangeExcavationJobOwnership(old_owner, new_owner);
}

/** @param part Actual train part being deleted; clear references before its pool identity can be reused. */
void OnTBMTrainPartDeleted(const Train &part)
{
	OnTBMExchangeTrainPartDeleted(part); // First: it still reads which project the cutter served.
	const auto it = std::ranges::find(_tbm_projects, part.index, &TBMProject::cutter);
	if (it == _tbm_projects.end()) return;
	it->cutter = VehicleID::Invalid();
	it->permanent_parts.clear();
	// An unfinished project shows the loss at once; a pause or an earlier block is kept, as normalization does.
	if (!IsTBMProjectFinished(it->phase) && it->blocked == TBMBlockedReason::None) it->blocked = TBMBlockedReason::MissingCutter;
	++it->revision;
	if (IsTBMPortalKey(it->portal)) ForgetLayerPortalWorkCutter(it->portal, part.index);
}

/** @param station Removed original origin; affected work retains a recoverable loss until a new origin is selected. */
void OnTBMStationDeleted(StationID station)
{
	// Invalidate only work tied to the removed origin, preserving unrelated deliveries and pre-existing pause reasons.
	for (auto &project : _tbm_projects) {
		if (project.origin != station) continue;
		project.origin = StationID::Invalid();
		// An unfinished project shows the loss at once; a pause or an earlier block is kept, as normalization does.
		if (!IsTBMProjectFinished(project.phase) && project.blocked == TBMBlockedReason::None) project.blocked = TBMBlockedReason::MissingOrigin;
		++project.revision;
	}
	ForgetLayerPortalWorkOrigin(station);
}

/**
 * Admit a replacement physical origin for the existing haulage obligation.
 * @param id Project owned by the command actor.
 * @param station Proposed owned Surface railway station.
 * @param execute False validates without changing the project.
 * @return None on admission, otherwise the stable reference/ownership refusal.
 */
TBMProjectError SetTBMProjectOrigin(TBMProjectID id, StationID station, bool execute)
{
	TBMProject *project = FindTBMProject(id);
	if (project == nullptr) return TBMProjectError::InvalidProject;
	if (project->company != _current_company) return TBMProjectError::InvalidOwner;
	if (project->origin == station) return TBMProjectError::Unavailable;
	TBMProject next = *project;
	next.origin = station;
	// A foreign or below-ground station, or one without rail, cannot end the loaded wagon leg.
	if (!IsTBMOriginLive(next)) return TBMProjectError::MissingOrigin;
	if (!execute) return TBMProjectError::None;
	project->origin = station;
	++project->revision;
	return TBMProjectError::None;
}

/**
 * Command entry for replacing a project's origin.
 * @param flags Command estimate/execute policy.
 * @param project Project of the command actor.
 * @param station Company-owned Surface railway station.
 * @return Success or a refusal with the project unchanged.
 */
CommandCost CmdSetTBMProjectOrigin(DoCommandFlags flags, TBMProjectID project, StationID station)
{
	if (SetTBMProjectOrigin(project, station, flags.Test(DoCommandFlag::Execute)) != TBMProjectError::None) return CMD_ERROR;
	return CommandCost();
}

/**
 * Network/script command entry for project controls.
 * @param flags Command estimate/execute policy.
 * @param project Project to control.
 * @param action Requested control.
 * @return Success or a refusal with the store unchanged.
 */
CommandCost CmdControlTBMProject(DoCommandFlags flags, TBMProjectID project, TBMProjectAction action)
{
	const TBMProjectError error = ControlTBMProject(project, action, flags.Test(DoCommandFlag::Execute));
	if (error == TBMProjectError::None) return CommandCost();
	if (error != TBMProjectError::Unavailable) return CMD_ERROR; // A missing or foreign project is no player choice.
	// An action refused in the project's current state names what it waits for.
	switch (action) {
		case TBMProjectAction::Pause: return CommandCost(STR_ERROR_TBM_PROJECT_CANT_PAUSE);
		case TBMProjectAction::Resume: return CommandCost(STR_ERROR_TBM_PROJECT_NOT_PAUSED);
		case TBMProjectAction::Cancel: return CommandCost(STR_ERROR_TBM_PROJECT_FINISHED);
		case TBMProjectAction::Retire: return CommandCost(GetTBMProjectReleaseProblem(project));
		default: return CMD_ERROR;
	}
}

/** @copydoc PreviewTBMExtension */
TBMExtensionPreview PreviewTBMExtension(CompanyID company, TBMProjectID id, TileIndex area_start, TileIndex area_end, ExcavationPlanIntent intent)
{
	TBMExtensionPreview preview;
	/** @param message Exact player-facing admission failure. @return Unaccepted quote retaining any useful preview geometry. */
	const auto refuse = [&preview](StringID message) {
		preview.result = CommandCost(message);
		return preview;
	};
	const auto *project = GetTBMProject(id);
	// A browser row cannot lend another company's machine or an orphaned
	// project to the local player, even when its old ID still exists.
	if (!Company::IsValidID(company) || project == nullptr || project->company != company) return refuse(STR_ERROR_TBM_EXTEND_REFERENCES);
	AutoRestoreBackup actor(_current_company, company);
	if (ValidateTBMProject(*project) != TBMProjectError::None) return refuse(STR_ERROR_TBM_EXTEND_REFERENCES);
	if (project->revision >= UINT64_MAX - 1) return refuse(STR_ERROR_TBM_EXTEND_REFERENCES);
	if (FindTBMProjectReferenceLoss(*project) != TBMBlockedReason::None) return refuse(STR_ERROR_TBM_EXTEND_REFERENCES);
	const auto *job = GetLiveTBMJob(*project);
	// Completed/cancelled history and an actively changing cut are separate
	// workflows. Pause preserves the current partial cut for a stable quote.
	if (job == nullptr || IsTBMProjectFinished(project->phase) || job->state == ExcavationJobState::Completed || job->state == ExcavationJobState::Cancelled) return refuse(STR_ERROR_TBM_PROJECT_FINISHED);
	if (project->blocked != TBMBlockedReason::Paused) return refuse(STR_ERROR_TBM_EXTEND_PAUSE);
	if (!IsTBMCutterAvailable(project->model_id, GetTileLayer(job->commissioning_anchor), GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return refuse(STR_ERROR_TBM_COMMISSION_MODEL);
	if (CheckTBMOriginAdmission(*project) != TBMExchangeError::None) return refuse(STR_ERROR_TBM_COMMISSION_RECEIVER);
	const Train *front = GetLiveTBMCutter(*project);
	const auto dimensions = MeasureTBMConsist(*front, *project);
	// An exchange or refit cannot silently shrink either frozen cargo group
	// while a new plan assumes the old return schedule is still safe.
	if (dimensions.error != TBMAcquisitionError::None || !job->pass_terms || dimensions.earth_capacity_t != job->pass_terms->earth_capacity_t ||
		dimensions.rock_capacity_t != job->pass_terms->rock_capacity_t) return refuse(STR_ERROR_TBM_COMMISSION_CONSIST);
	std::vector<size_t> ends;
	const auto retreat = GetTBMWorkRailPath(*job, job->cursor, ends);
	if (retreat.empty()) return refuse(STR_PORTAL_HOLD_MISSING_SUPPORT);
	// Every already paid piece of the original return path must remain usable;
	// extending work cannot fix or hide missing/foreign support behind the cutter.
	for (TileIndex tile : retreat) {
		if (!IsTBMSupportUsable(*project, tile)) return refuse(STR_PORTAL_HOLD_MISSING_SUPPORT);
	}
	preview.job_revision = job->revision;
	preview.cutter = project->cutter;
	if (intent > ExcavationPlanIntent::StraightTunnel) return refuse(STR_ERROR_TBM_COMMISSION_AREA);
	// Straight intent starts exactly where the accepted cutter path ends; it
	// cannot smuggle an unrelated drag corner into an implied approach turn.
	if (intent == ExcavationPlanIntent::StraightTunnel && area_start != job->route.back().cutter_position) return refuse(STR_ERROR_TBM_TUNNEL_ALIGNMENT);
	// New work starts from the pose after every accepted cut, with the approach and turning space it needs (r2df task 9.3).
	const auto replay = ReplayExcavationRoute(*job, dimensions.planning_points, job->route.size());
	if (replay.error != ExcavationError::None) return refuse(STR_ERROR_TBM_COMMISSION_ROUTE);
	ExcavationApproachProposal approach;
	preview.outline = intent == ExcavationPlanIntent::Area ?
		MakeTBMWorkOutlineWithApproach(replay.next, false, job->model_id, job->id, job->commissioning_anchor, area_start, area_end, job->authorized_tiles, approach) :
		MakeTBMTunnelOutline(replay.next, false, job->model_id, job->id, area_end, job->authorized_tiles, approach);
	if (approach.error != ExcavationError::None) {
		// No legal approach: the quote names the blocking tile and why (r2df task 10.4), and none of the new floor is reachable.
		preview.plan.error = approach.error;
		preview.plan.failure_tile = approach.failure_tile;
		preview.plan.remaining_targets = preview.outline.requested_tiles;
		return refuse(GetTBMApproachRefusal(approach.error, approach.failure_tile));
	}
	if (preview.outline.error == ExcavationError::IllegalTurn) return refuse(STR_ERROR_TBM_TUNNEL_ALIGNMENT);
	if (preview.outline.error != ExcavationError::None) return refuse(STR_ERROR_TBM_COMMISSION_AREA);
	preview.plan = PlanExcavationJobExtension(job->id, preview.outline, dimensions.planning_points, {}, intent, area_end);
	preview.approach = SummarizeTBMApproach(preview.plan.route, approach.extra_tiles, job->model_id);
	// A planner refusal reads exactly as in the commissioning quote (r2df task 4.5): an unreachable request is a route
	// refusal there, so it must not be reported as partial cover here merely because targets remain.
	if (preview.plan.error == ExcavationError::InsufficientCapacity) return refuse(STR_ERROR_TBM_COMMISSION_CONSIST);
	if (preview.plan.error != ExcavationError::None) return refuse(STR_ERROR_TBM_COMMISSION_ROUTE);
	// Partial or empty extensions must not be accepted as the player's whole
	// requested area or merely bump the saved revision without useful work.
	if (preview.plan.partial || !preview.plan.remaining_targets.empty()) return refuse(STR_ERROR_TBM_COMMISSION_PARTIAL);
	if (preview.plan.route.empty()) return refuse(STR_ERROR_TBM_EXTEND_EMPTY);
	ExcavationJob combined = *job;
	combined.route.insert(combined.route.end(), preview.plan.route.begin(), preview.plan.route.end());
	if (!CheckTBMSupportRailTurns(combined)) return refuse(STR_ERROR_TBM_COMMISSION_RAIL_TURNS);
	// r2df task 7.3: the cuts still to come, old and new, must be able to lay or reuse their support, as the commissioning
	// quote checks; the refused cell is marked like a planner refusal.
	if (const auto blocker = FindTBMSupportRouteBlocker(combined, project->id, company); blocker.tile != INVALID_TILE) {
		preview.plan.failure_tile = blocker.tile;
		return refuse(blocker.fundable ? STR_ERROR_TBM_COMMISSION_SUPPORT_PIECE : STR_ERROR_TBM_COMMISSION_SUPPORT);
	}
	const auto admitted = AppendExcavationJob(job->id, job->revision, preview.outline, preview.plan.route, false);
	if (admitted.error == ExcavationError::InsufficientCapacity) return refuse(STR_ERROR_TBM_COMMISSION_CONSIST);
	if (admitted.error != ExcavationError::None) return refuse(STR_ERROR_TBM_EXTEND_ROUTE);
	std::set<TileIndex> swept;
	// The preview shows the actual additional body/tail sweep, not the entire
	// previous job's work area or a rectangle mistaken for cutter movement.
	for (const auto &cut : preview.plan.route) swept.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
	std::ranges::set_difference(swept, preview.outline.requested_tiles, std::back_inserter(preview.working_space));
	combined.requested_tiles.insert(combined.requested_tiles.end(), preview.outline.requested_tiles.begin(), preview.outline.requested_tiles.end());
	combined.authorized_tiles = preview.outline.authorized_tiles;
	std::ranges::sort(combined.requested_tiles);
	combined.requested_tiles.erase(std::unique(combined.requested_tiles.begin(), combined.requested_tiles.end()), combined.requested_tiles.end());
	preview.fingerprint = ComputeExcavationPassFingerprint(combined, *job->pass_terms);
	MixTBMQuoteFingerprint(preview.fingerprint, to_underlying(intent));
	// Bind reviewed geometry/quantities to stable participants. Idle-cost and
	// phase timers keep running while paused and must not invalidate a quote.
	for (uint64_t value : {uint64_t{id.base()}, uint64_t{project->cutter.base()}, uint64_t{project->origin.base()}, uint64_t{project->portal.base()},
		uint64_t{project->exchange_bay.base()}, job->revision, preview.plan.estimate.earth_t, preview.plan.estimate.rock_t,
		preview.plan.estimate.work_ticks, uint64_t(preview.plan.estimate.cost.base()),
		uint64_t{preview.approach.tiles.size()}, uint64_t{preview.approach.cuts}, uint64_t(preview.approach.estimate.cost.base())}) MixTBMQuoteFingerprint(preview.fingerprint, value); // r2df task 9.5: the approach is reviewed too.
	for (VehicleID part : project->permanent_parts) MixTBMQuoteFingerprint(preview.fingerprint, part.base());
	preview.result = CommandCost();
	return preview;
}

/** @copydoc CmdExtendTBMProject */
CommandCost CmdExtendTBMProject(DoCommandFlags flags, TBMProjectID id, TileIndex area_start, TileIndex area_end, uint64_t expected_revision, uint64_t expected_fingerprint, ExcavationPlanIntent intent)
{
	const auto preview = PreviewTBMExtension(_current_company, id, area_start, area_end, intent);
	if (preview.result.Failed()) return preview.result;
	// A stale player quote never replans silently around changed terrain or
	// acquires different work after the first confirmation already succeeded.
	if (preview.job_revision != expected_revision || preview.fingerprint != expected_fingerprint) return CommandCost(STR_ERROR_TBM_EXTEND_STALE);
	if (!flags.Test(DoCommandFlag::Execute)) return preview.result;
	TBMProject *project = FindTBMProject(id);
	const auto appended = AppendExcavationJob(project->job, expected_revision, preview.outline, preview.plan.route, true);
	if (appended.error != ExcavationError::None) return CommandCost(STR_ERROR_TBM_EXTEND_STALE);
	++project->revision; // Prepared geology is now published; this non-allocating controller stamp cannot leave a partial commit.
	return CommandCost();
}

/** @copydoc PreviewTBMWorkChange */
TBMWorkChangePreview PreviewTBMWorkChange(CompanyID company, TBMProjectID id, TileIndex area_start, TileIndex area_end, ExcavationPlanIntent intent)
{
	TBMWorkChangePreview preview;
	/** @param message Exact player-facing admission failure. @return Unaccepted quote retaining any useful preview geometry. */
	const auto refuse = [&preview](StringID message) {
		preview.result = CommandCost(message);
		return preview;
	};
	const auto *project = GetTBMProject(id);
	// Only the company's own project may be replanned, whatever row or portal the request came from.
	if (!Company::IsValidID(company) || project == nullptr || project->company != company) return refuse(STR_ERROR_TBM_EXTEND_REFERENCES);
	AutoRestoreBackup actor(_current_company, company);
	if (ValidateTBMProject(*project) != TBMProjectError::None) return refuse(STR_ERROR_TBM_EXTEND_REFERENCES);
	if (project->revision >= UINT64_MAX - 1) return refuse(STR_ERROR_TBM_EXTEND_REFERENCES);
	if (FindTBMProjectReferenceLoss(*project) != TBMBlockedReason::None) return refuse(STR_ERROR_TBM_EXTEND_REFERENCES);
	const auto *job = GetLiveTBMJob(*project);
	// Finished work, or a project being released, has nothing left to replace (task 12.5 offers release and new work).
	// Working, paused, returning and parked projects are all accepted: the quote binds the completed-cut count, not a pause.
	if (job == nullptr || IsTBMProjectFinished(project->phase) || IsTBMFinishing(project->phase) || job->state == ExcavationJobState::Completed || job->state == ExcavationJobState::Cancelled) return refuse(STR_ERROR_TBM_PROJECT_FINISHED);
	if (job->cursor == 0) return refuse(STR_ERROR_TBM_CHANGE_NO_CUT); // The first cut out of the housing belongs to the commission.
	if (!IsTBMCutterAvailable(project->model_id, GetTileLayer(job->commissioning_anchor), GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return refuse(STR_ERROR_TBM_COMMISSION_MODEL);
	if (CheckTBMOriginAdmission(*project) != TBMExchangeError::None) return refuse(STR_ERROR_TBM_COMMISSION_RECEIVER);
	const Train *front = GetLiveTBMCutter(*project);
	const auto dimensions = MeasureTBMConsist(*front, *project);
	// An exchange or refit cannot silently shrink either frozen cargo group under a new plan's exchange schedule.
	if (dimensions.error != TBMAcquisitionError::None || !job->pass_terms || dimensions.earth_capacity_t != job->pass_terms->earth_capacity_t ||
		dimensions.rock_capacity_t != job->pass_terms->rock_capacity_t) return refuse(STR_ERROR_TBM_COMMISSION_CONSIST);
	std::vector<size_t> ends;
	const auto retreat = GetTBMWorkRailPath(*job, job->cursor, ends);
	if (retreat.empty()) return refuse(STR_PORTAL_HOLD_MISSING_SUPPORT);
	// The paid return path behind the completed cuts must stay usable; a change cannot hide missing support.
	for (TileIndex tile : retreat) {
		if (!IsTBMSupportUsable(*project, tile)) return refuse(STR_PORTAL_HOLD_MISSING_SUPPORT);
	}
	preview.cutter = project->cutter;
	preview.cursor = job->cursor;
	preview.dropped_cuts = static_cast<uint32_t>(job->route.size()) - job->cursor;
	if (intent > ExcavationPlanIntent::StraightTunnel) return refuse(STR_ERROR_TBM_COMMISSION_AREA);
	// A straight tunnel starts where the last completed cut ended, the face the cutter works from.
	if (intent == ExcavationPlanIntent::StraightTunnel && area_start != job->route[job->cursor - 1].cutter_position) return refuse(STR_ERROR_TBM_TUNNEL_ALIGNMENT);
	// New work starts from the pose after the completed cuts, with the approach and turning space it needs (task 12.4).
	const auto replay = ReplayExcavationRoute(*job, dimensions.planning_points, job->cursor);
	if (replay.error != ExcavationError::None) return refuse(STR_ERROR_TBM_COMMISSION_ROUTE);
	// Retain paid sweeps and delivered floor as usable access. Unopened cells authorized only by abandoned cuts must be claimed and charged again.
	std::vector<TileIndex> kept;
	for (uint32_t index = 0; index < job->cursor; ++index) kept.insert(kept.end(), job->route[index].swept_tiles.begin(), job->route[index].swept_tiles.end());
	// Only genuinely excavated old authorization may enlarge the new approach domain.
	for (TileIndex tile : job->authorized_tiles) {
		if (GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated) kept.push_back(tile);
	}
	std::ranges::sort(kept);
	kept.erase(std::unique(kept.begin(), kept.end()), kept.end());
	ExcavationApproachProposal approach;
	preview.outline = intent == ExcavationPlanIntent::Area ?
		MakeTBMWorkOutlineWithApproach(replay.next, false, job->model_id, job->id, job->commissioning_anchor, area_start, area_end, kept, approach, job->cursor) :
		MakeTBMTunnelOutline(replay.next, false, job->model_id, job->id, area_end, kept, approach, job->cursor);
	if (approach.error != ExcavationError::None) {
		// No legal approach: the quote names the blocking tile and why, and none of the new floor is reachable.
		preview.plan.error = approach.error;
		preview.plan.failure_tile = approach.failure_tile;
		preview.plan.remaining_targets = preview.outline.requested_tiles;
		return refuse(GetTBMApproachRefusal(approach.error, approach.failure_tile));
	}
	if (preview.outline.error == ExcavationError::IllegalTurn) return refuse(STR_ERROR_TBM_TUNNEL_ALIGNMENT);
	if (preview.outline.error != ExcavationError::None) return refuse(STR_ERROR_TBM_COMMISSION_AREA);
	preview.plan = PlanExcavationJobReplacement(job->id, preview.outline, dimensions.planning_points, {}, intent, area_end);
	preview.approach = SummarizeTBMApproach(preview.plan.route, approach.extra_tiles, job->model_id, job->id);
	// Planner refusals read exactly as in the commissioning and extension quotes.
	if (preview.plan.error == ExcavationError::InsufficientCapacity) return refuse(STR_ERROR_TBM_COMMISSION_CONSIST);
	if (preview.plan.error != ExcavationError::None) return refuse(STR_ERROR_TBM_COMMISSION_ROUTE);
	// Replacing a whole unfinished assignment requires full coverage; partial work cannot masquerade as the reviewed area.
	if (preview.plan.partial || !preview.plan.remaining_targets.empty()) return refuse(STR_ERROR_TBM_COMMISSION_PARTIAL);
	if (preview.plan.route.empty()) return refuse(STR_ERROR_TBM_EXTEND_EMPTY);
	ExcavationJob combined = *job;
	combined.route.resize(job->cursor);
	combined.route.insert(combined.route.end(), preview.plan.route.begin(), preview.plan.route.end());
	if (!CheckTBMSupportRailTurns(combined)) return refuse(STR_ERROR_TBM_COMMISSION_RAIL_TURNS);
	// The new cuts must be able to lay or reuse their support, as the commissioning quote checks; the cell is marked.
	if (const auto blocker = FindTBMSupportRouteBlocker(combined, project->id, company); blocker.tile != INVALID_TILE) {
		preview.plan.failure_tile = blocker.tile;
		return refuse(blocker.fundable ? STR_ERROR_TBM_COMMISSION_SUPPORT_PIECE : STR_ERROR_TBM_COMMISSION_SUPPORT);
	}
	// The face piece was aimed at the old next cut. A different exit takes a junction piece, laid by the change itself;
	// a train standing on the face cannot switch to it, so then the player must move the machine first.
	preview.face_support = PrepareTBMFaceSupport(*project, combined);
	if (preview.face_support.error != TBMBlockedReason::None) {
		preview.plan.failure_tile = preview.face_support.failure_tile;
		return refuse(STR_ERROR_TBM_CHANGE_FACE);
	}
	// The spoil aboard now seeds the first new exchange (task 4.2's helper measures it); the command measures it again.
	const TBMSpoilLoad spoil = SumTBMCutterSpoil(*front, *project);
	preview.load = {spoil.earth_loaded_t, spoil.rock_loaded_t};
	const auto admitted = ReplaceExcavationJobRemainder(job->id, job->cursor, preview.outline, preview.plan.route, preview.load, false);
	if (admitted.error == ExcavationError::InsufficientCapacity) return refuse(STR_ERROR_TBM_COMMISSION_CONSIST);
	if (admitted.error != ExcavationError::None) return refuse(STR_ERROR_TBM_CHANGE_ROUTE);
	// Face work is lost only when the new remainder does not start with the cut in progress.
	if (preview.plan.route.front() != job->route[job->cursor]) preview.abandoned_work_ticks = job->partial_work_ticks;
	const auto current = EstimateExcavationCut(job->id);
	if (current.error == ExcavationError::None) preview.cut_work_ticks = current.work_ticks;
	std::set<TileIndex> swept;
	// Display only the new body/tail sweep outside requested floor; completed history is not new work.
	for (const auto &cut : preview.plan.route) swept.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
	std::ranges::set_difference(swept, preview.outline.requested_tiles, std::back_inserter(preview.working_space));
	combined.requested_tiles = preview.outline.requested_tiles;
	combined.authorized_tiles = preview.outline.authorized_tiles;
	preview.fingerprint = ComputeExcavationPassFingerprint(combined, *job->pass_terms);
	MixTBMQuoteFingerprint(preview.fingerprint, to_underlying(intent));
	// Bind reviewed geometry and quantities to the completed-cut count and stable participants. The job revision and
	// partial work change every work tick, so a running machine's quote stays valid until another cut completes.
	for (uint64_t value : {uint64_t{id.base()}, uint64_t{project->cutter.base()}, uint64_t{project->origin.base()}, uint64_t{project->portal.base()},
		uint64_t{project->exchange_bay.base()}, uint64_t{job->cursor}, uint64_t{preview.dropped_cuts}, preview.plan.estimate.earth_t, preview.plan.estimate.rock_t,
		preview.plan.estimate.work_ticks, uint64_t(preview.plan.estimate.cost.base()),
		uint64_t{preview.approach.tiles.size()}, uint64_t{preview.approach.cuts}, uint64_t(preview.approach.estimate.cost.base()),
		uint64_t{preview.face_support.pieces.size()}}) MixTBMQuoteFingerprint(preview.fingerprint, value);
	// Bind every existing machinery part so a changed consist requires a fresh review.
	for (VehicleID part : project->permanent_parts) MixTBMQuoteFingerprint(preview.fingerprint, part.base());
	preview.result = CommandCost();
	return preview;
}

/** @copydoc CmdChangeTBMWork */
CommandCost CmdChangeTBMWork(DoCommandFlags flags, TBMProjectID id, TileIndex area_start, TileIndex area_end, uint32_t expected_cursor, uint64_t expected_fingerprint, ExcavationPlanIntent intent)
{
	const auto preview = PreviewTBMWorkChange(_current_company, id, area_start, area_end, intent);
	if (preview.result.Failed()) return preview.result;
	// Another completed cut, or any change to the reviewed work, makes the player's quote stale; nothing is replanned silently.
	if (preview.cursor != expected_cursor || preview.fingerprint != expected_fingerprint) return CommandCost(STR_ERROR_TBM_CHANGE_STALE);
	if (!flags.Test(DoCommandFlag::Execute)) return preview.result;
	TBMProject *project = FindTBMProject(id);
	const auto replaced = ReplaceExcavationJobRemainder(project->job, expected_cursor, preview.outline, preview.plan.route, preview.load, true);
	if (replaced.error != ExcavationError::None) return CommandCost(STR_ERROR_TBM_CHANGE_STALE);
	// Preflight required the paid retreat, so this is an admitted junction on existing support.
	// It changes no occupied piece and needs no new support record after publishing the remainder.
	if (!preview.face_support.pieces.empty()) PlaceTBMSupport(*project, preview.face_support);
	// The face the cutter works from is unchanged, so its phase, heading and work remainder stay; only the stamp moves.
	++project->revision;
	return CommandCost();
}

/**
 * Decide one phase step without changing anything.
 * @param project Current record.
 * @param inputs Observed facts.
 * @return The step, or a refusal.
 */
TBMPhaseStep StepTBMProjectPhase(const TBMProject &project, const TBMPhaseInputs &inputs)
{
	TBMPhaseStep step;
	step.phase = project.phase;
	if (IsTBMProjectFinished(project.phase)) {
		step.error = TBMProjectError::Unavailable;
		return step;
	}
	// A service trip leaves the accepted face/bay. Record the rejoin phase as
	// soon as the order takes control: a later project pause can replace the
	// displayed Servicing reason, but must never erase the required return trip.
	if ((inputs.servicing || inputs.service_finished) && project.phase != TBMPhase::FollowOrders) {
		// A face transfer interrupted by service must physically rejoin its accepted face before working again.
		const bool face = project.phase == TBMPhase::Commissioning || project.phase == TBMPhase::TravelToFace || project.phase == TBMPhase::Bore || project.phase == TBMPhase::AdvanceSupport || IsTBMAwaitingFaceTransfer(project.phase);
		step.phase = IsTBMFinishing(project.phase) ? TBMPhase::ReturnToRelease : IsTBMParking(project.phase) ? TBMPhase::ReturnToPark : face ? TBMPhase::TravelToFace : TBMPhase::ReturnForExchange;
		step.phase_changed = step.phase != project.phase;
	}
	/* Conditions hold the phase: reference loss first, then the player's pause, then the train's own state. */
	const auto loss = FindTBMProjectReferenceLoss(project);
	if (project.blocked == TBMBlockedReason::Paused) {
		step.blocked = TBMBlockedReason::Paused;
	} else if (loss != TBMBlockedReason::None) {
		step.blocked = loss;
	} else if (inputs.train_stopped) {
		step.blocked = TBMBlockedReason::Stopped;
	} else if (inputs.broken_down) {
		step.blocked = TBMBlockedReason::Breakdown;
	} else if (inputs.servicing) {
		step.blocked = TBMBlockedReason::Servicing;
	} else if (inputs.work_wait != TBMBlockedReason::None) {
		step.blocked = inputs.work_wait; // The face is waiting for wagon space, support, cash or a clear path.
	}
	if (step.blocked != TBMBlockedReason::None) return step;

	/* A cancelled job turns every forward phase back to the bay along the supported retreat (Group 17.1). */
	const bool forward = project.phase == TBMPhase::Commissioning || project.phase == TBMPhase::TravelToFace || project.phase == TBMPhase::Bore || project.phase == TBMPhase::AdvanceSupport || IsTBMAwaitingFaceTransfer(project.phase);
	if (inputs.cancelled && forward) {
		step.phase = TBMPhase::ReturnForExchange;
		step.phase_changed = true;
		return step;
	}

	/* Edges. The ReturnForExchange entry guard needs the job's frozen pass terms, never a guess. */
	const auto *job = GetLiveTBMJob(project);
	const bool exchange_due = inputs.exchange_due && job != nullptr && job->pass_terms.has_value();
	switch (step.phase) {
		case TBMPhase::Commissioning:
			step.phase = TBMPhase::TravelToFace; // Published, unblocked and started: leave the source site for the face.
			break;
		case TBMPhase::TravelToFace:
			if (inputs.at_face) step.phase = TBMPhase::Bore;
			break;
		case TBMPhase::Bore:
			// An exchange that is due must happen before the next cut; otherwise a committed cut moves on to support.
			// Actual paid geometry chooses a face meeting; a cramped approach retains the original bay settlement.
			if (exchange_due) {
				step.phase = GetTBMMeetingKind(project) == TBMMeetingKind::Face ? TBMPhase::AwaitFaceTransfer : TBMPhase::ReturnForExchange;
			} else if (inputs.cut_committed) {
				step.phase = TBMPhase::AdvanceSupport;
			}
			break;
		case TBMPhase::AdvanceSupport:
			if (!inputs.support_advanced) break;
			if (exchange_due) {
				step.phase = GetTBMMeetingKind(project) == TBMMeetingKind::Face ? TBMPhase::AwaitFaceTransfer : TBMPhase::ReturnForExchange;
			} else if (inputs.route_complete) {
				step.error = TBMProjectError::InvalidProject; // Frozen terms always end with a final return; finishing without one is inconsistent.
			} else {
				step.phase = TBMPhase::Bore;
			}
			break;
		case TBMPhase::AwaitFaceTransfer:
			// Only actual removal of the due load releases this wait. A cut can leave the cutter behind the new
			// support end, so its physical advance must finish before more boring; a finished route returns empty.
			if (!inputs.exchange_due) {
				step.phase = inputs.route_complete ? TBMPhase::ReturnForExchange : inputs.at_face ? TBMPhase::Bore : TBMPhase::AdvanceSupport;
			} else if (GetTBMMeetingKind(project) != TBMMeetingKind::Face) {
				step.phase = TBMPhase::ReturnForExchange; // Lost stand: use the existing safe bay and withdrawal holds.
			}
			break;
		case TBMPhase::ReturnForExchange:
			if (inputs.at_exchange) step.phase = TBMPhase::WaitForEmpties; // The consist stands wholly in its bay.
			break;
		case TBMPhase::ReturnToPark:
			if (inputs.at_exchange) step.phase = TBMPhase::WaitToPark;
			break;
		case TBMPhase::ReturnForHangar:
			if (inputs.at_exchange) step.phase = TBMPhase::WaitForHangar;
			break;
		case TBMPhase::WaitForHangar:
			// Hangar travel starts only after physical bay arrival and complete spoil settlement; the retained halt owns its target.
			if (inputs.at_exchange && !inputs.exchange_due) {
				if (inputs.hangar_order) step.phase = TBMPhase::TravelToHangar;
				else step.blocked = TBMBlockedReason::MissingHangar;
			}
			break;
		case TBMPhase::TravelToHangar:
		case TBMPhase::ParkedHangar:
		case TBMPhase::FollowOrders:
			break; // Native whole-consist depot arrival, rather than a caller-supplied flag, completes the order.
		case TBMPhase::WaitToPark:
			if (inputs.at_exchange && !inputs.exchange_due) step.phase = TBMPhase::Parked; // Parking requires whole physical arrival and the actual load already exchanged.
			break;
		case TBMPhase::ReturnToRelease:
			if (inputs.at_exchange) step.phase = TBMPhase::WaitToRelease;
			break;
		case TBMPhase::WaitToRelease:
			// Final release cannot advance while the cutter is still outside storage or owns an unsettled spoil load.
			if (inputs.at_exchange && !inputs.exchange_due) step.phase = TBMPhase::ReleasePending;
			break;
		case TBMPhase::ReleasePending:
			break; // Actual whole-project settlement, not a caller-supplied arrival flag, admits cancellation below.
		case TBMPhase::Parked:
			break; // Only an explicit Resume or cancellation releases this work suspension.
		case TBMPhase::WaitForEmpties:
			// Only an executed exchange removes the spoil from the actual wagons, so "not due" means it happened.
			if (!inputs.exchange_due) step.phase = TBMPhase::Resume;
			break;
		case TBMPhase::Resume:
			// The final return ends the project: Completed after the last cut, Cancelled after a cancellation.
			step.phase = inputs.route_complete ? (inputs.cancelled ? TBMPhase::Cancelled : TBMPhase::Completed) : TBMPhase::TravelToFace;
			break;
		default:
			break;
	}
	step.phase_changed = step.phase != project.phase;
	return step;
}

/**
 * Apply one controller step to a live project.
 * @param id Project to step.
 * @param inputs Observed facts.
 * @return None, or a refusal with the project unchanged.
 */
TBMProjectError AdvanceTBMProjectPhase(TBMProjectID id, const TBMPhaseInputs &inputs)
{
	TBMProject *project = FindTBMProject(id);
	if (project == nullptr) return TBMProjectError::InvalidProject;
	TBMProject next = *project;
	NormalizeTBMProjectReferences(next);
	// A player pause survives normalization; any other block is re-derived from scratch by the step.
	if (next.blocked != TBMBlockedReason::Paused) next.blocked = TBMBlockedReason::None;
	const auto step = StepTBMProjectPhase(next, inputs);
	if (step.error != TBMProjectError::None) return step.error;
	if (IsTBMStored(step.phase) && next.cutter != VehicleID::Invalid() && !IsTBMReadyToPark(next)) return TBMProjectError::Unavailable; // A caller's arrival flag cannot replace actual full withdrawal and empty custody at publication.
	const bool changed = step.phase != project->phase || step.blocked != project->blocked || next.cutter != project->cutter || next.job != project->job || next.origin != project->origin || next.portal != project->portal || next.exchange_bay != project->exchange_bay;
	next.phase = step.phase;
	next.blocked = step.blocked;
	// The one phase timer restarts on a phase change; durations accrue as idle while blocked, active otherwise.
	next.phase_ticks = step.phase_changed ? 0 : std::min<uint64_t>(next.phase_ticks + 1, UINT64_MAX - 1);
	if (step.blocked != TBMBlockedReason::None) {
		next.idle_ticks = std::min<uint64_t>(next.idle_ticks + 1, UINT64_MAX - 1);
	} else {
		next.active_ticks = std::min<uint64_t>(next.active_ticks + 1, UINT64_MAX - 1);
	}
	if (changed) ++next.revision;
	*project = std::move(next);
	return TBMProjectError::None;
}

/**
 * Read the train-side and face facts for a project. The face approach is the housing before the first cut and the
 * last committed cutter position afterwards; the cutter standing there ends TravelToFace and, after a cut, ends
 * AdvanceSupport. Cut commits and work waits come from the face step in AdvanceTBMWork, not from here.
 * @param project Project whose cutter is observed.
 * @return Observed inputs; all false when the cutter is missing.
 */
TBMPhaseInputs ObserveTBMPhaseInputs(const TBMProject &project)
{
	TBMPhaseInputs inputs;
	const Train *front = GetLiveTBMCutter(project);
	if (front == nullptr) return inputs;
	inputs.train_stopped = front->vehstatus.Test(VehState::Stopped);
	inputs.broken_down = front->breakdown_ctr == 1;
	inputs.servicing = front->current_order.IsType(OT_GOTO_DEPOT) && !IsTBMHangarTransfer(project.phase);
	inputs.hangar_order = HasTBMHangarHaltOrder(*front);
	inputs.service_finished = project.blocked == TBMBlockedReason::Servicing && !inputs.servicing;
	inputs.work_wait = GetTBMTravelHoldReason(*front);
	// Native reservation retries still run while this visible work interruption
	// is reported. Treating it as a physical hold query would suppress the retry
	// that clears Stuck after another train leaves or the track is repaired.
	if (inputs.work_wait == TBMBlockedReason::None && front->flags.Test(VehicleRailFlag::Stuck) && GetTBMCutterSteeringTarget(*front).has_value()) inputs.work_wait = TBMBlockedReason::Obstruction;
	const auto *job = GetLiveTBMJob(project);
	// Only a live bounded geological cursor can define the actual face/support approach used by native movement.
	if (job != nullptr && job->cursor <= job->route.size()) {
		const TileIndex approach = job->cursor == 0 ? job->commissioning_anchor : job->route[job->cursor - 1].cutter_position;
		inputs.at_face = front->tile == approach;
		inputs.support_advanced = inputs.at_face; // Read only in AdvanceSupport, where the approach is the new support end.
		inputs.cancelled = job->state == ExcavationJobState::Cancelled;
		inputs.route_complete = job->cursor == job->route.size() || inputs.cancelled; // A cancelled route has no further cut.
	}
	inputs.exchange_due = IsTBMExchangeDue(project);
	inputs.at_exchange = CheckTBMExchangeParticipant(*front, GetTBMExchangeBay(project), project.company) == TBMExchangeError::None;
	return inputs;
}

/**
 * Project the work state of one project from its own record and its geological job.
 * @param id Project to read.
 * @return Projected state; MissingProject defaults when the project is absent.
 */
TBMWorkState GetTBMWorkState(TBMProjectID id)
{
	TBMWorkState state;
	const auto *project = FindTBMProject(id);
	if (project == nullptr) return state;
	state.phase = project->phase;
	state.blocked = project->blocked;
	state.heading = project->heading;
	state.phase_ticks = project->phase_ticks;
	const auto *job = project->job == ExcavationJobID{UINT32_MAX} ? nullptr : GetExcavationJob(project->job);
	if (job == nullptr) return state; // Face, cursor and partial work belong to the job; without it they stay empty.
	state.pass_cursor = job->cursor;
	state.partial_work_ticks = job->partial_work_ticks;
	if (job->cursor < job->route.size()) state.face = job->route[job->cursor].cutter_position;
	return state;
}

/**
 * One simulation step of the controller.
 * @param id Project to step.
 * @return None, or a refusal with the project unchanged.
 */
TBMProjectError AdvanceTBMWork(TBMProjectID id)
{
	TBMProject *project = FindTBMProject(id);
	if (project == nullptr) return TBMProjectError::InvalidProject;
	if (IsTBMProjectFinished(project->phase)) return TBMProjectError::None; // Historical controllers never reacquire a work assignment from a native travel order.
	if (Train *front = Train::GetIfValid(project->cutter); front != nullptr && HasTBMHangarHaltOrder(*front) &&
			!IsTBMHangarTransfer(project->phase) && project->phase != TBMPhase::ParkedHangar && !IsTBMFinishing(project->phase)) BeginTBMHangarReturn(*front); // Adopt a normal player/list halt order before this tick can begin another atomic cut.
	auto inputs = ObserveTBMPhaseInputs(*project);
	// The face works only when this step leaves the project unblocked in Bore with no exchange due, so a paused,
	// stopped, broken-down, servicing or reference-lost project never cuts. An earlier work wait is retried.
	const bool works = project->phase == TBMPhase::Bore && project->blocked != TBMBlockedReason::Paused && FindTBMProjectReferenceLoss(*project) == TBMBlockedReason::None && !inputs.train_stopped && !inputs.broken_down && !inputs.servicing && !inputs.service_finished && !inputs.exchange_due && !inputs.cancelled;
	if (works) {
		const auto work = AdvanceTBMFaceWork(*project);
		inputs.cut_committed = work.cut_committed;
		inputs.work_wait = work.wait;
		project->work_remainder = work.work_remainder; // Face work never touches the store, so the pointer is still valid.
		if (work.cut_committed) {
			project->heading = work.heading; // Only a committed advance turns the cutter.
			// Re-read actual loaded custody after the paid commit. The phase owner chooses the face wait or bay
			// return before another advance; no stale pre-cut observation may permit another cut.
			inputs.exchange_due = IsTBMExchangeDue(*project);
		}
	}
	const TBMPhase previous_phase = project->phase;
	const TBMProjectError error = AdvanceTBMProjectPhase(id, inputs);
	// The return backs the consist along its support to the bay, cutter last and facing the face (R2MLVS3MP-M decision 5).
	if (error == TBMProjectError::None) {
		const TBMProject *stepped = FindTBMProject(id);
		if (Train *front = Train::GetIfValid(stepped->cutter); front != nullptr) {
			// Only an automatic departure from the working face mandates backing.
			// Explicit travel interruptions already selected their direction from
			// both actual ends; later ticks must not force them backwards again.
			const bool leaving_face = previous_phase == TBMPhase::Bore || previous_phase == TBMPhase::AdvanceSupport ||
				previous_phase == TBMPhase::TravelToFace || previous_phase == TBMPhase::Commissioning || IsTBMAwaitingFaceTransfer(previous_phase);
			if (leaving_face && IsTBMReturning(stepped->phase) && stepped->blocked == TBMBlockedReason::None) BackUpTBMConsist(*front);
			if (IsTBMStored(stepped->phase) && IsTBMReadyToPark(*stepped)) front->vehstatus.Set(VehState::Stopped); // Stop only after every actual part and its cargo passed parking admission.
		}
		if (stepped->phase == TBMPhase::ReleasePending && GetTBMProjectReleaseProblem(id) == INVALID_STRING_ID) {
			// Cancellation is now safe. Keep the controller until the explicit
			// final release action, so no train tick erases its own live owner.
			const auto *job = GetExcavationJob(stepped->job);
			if (job != nullptr && job->state != ExcavationJobState::Completed && job->state != ExcavationJobState::Cancelled) {
				AutoRestoreBackup actor(_current_company, stepped->company);
				[[maybe_unused]] const auto cancelled = CancelExcavationJob(stepped->job);
				assert(cancelled == ExcavationError::None);
			}
			auto *pending = FindTBMProject(id);
			pending->phase = job != nullptr && job->state == ExcavationJobState::Completed ? TBMPhase::Completed : TBMPhase::Cancelled;
			pending->phase_ticks = 0;
			pending->blocked = TBMBlockedReason::None;
			++pending->revision;
		}
	}
	return error;
}

/**
 * @param project Project whose buckets are read.
 * @return Money for the buckets plus the carried fraction.
 */
TBMRecurringCharge ComputeTBMRecurringCharge(const TBMProject &project)
{
	TBMRecurringCharge charge;
	const auto *model = GetTBMCutterModel(project.model_id);
	if (model == nullptr) return charge;
	const uint64_t year = static_cast<uint64_t>(CalendarTime::DAYS_IN_YEAR) * Ticks::DAY_TICKS;
	// Daily charging keeps the buckets small; the cap only keeps a crafted save from overflowing the product.
	const uint64_t active = std::min<uint64_t>(project.active_ticks, UINT32_MAX);
	const uint64_t idle = std::min<uint64_t>(project.idle_ticks, UINT32_MAX);
	const uint64_t numerator = active * static_cast<uint64_t>(model->active_cost_per_year.base()) + idle * static_cast<uint64_t>(model->idle_cost_per_year.base()) + project.cost_remainder % year;
	charge.cost = static_cast<int64_t>(numerator / year);
	charge.remainder = numerator % year;
	return charge;
}

/**
 * @param id Project to charge.
 * @return Money debited.
 */
Money ChargeTBMProjectCosts(TBMProjectID id)
{
	TBMProject *project = FindTBMProject(id);
	if (project == nullptr) return 0;
	const auto charge = ComputeTBMRecurringCharge(*project);
	project->active_ticks = 0;
	project->idle_ticks = 0;
	project->cost_remainder = charge.remainder;
	if (charge.cost != 0) SubtractMoneyFromCompany(project->company, CommandCost(ExpensesType::TrainRun, charge.cost));
	return charge.cost;
}

/** @return Stable digest over every saved project field and the allocation cursor. */
uint64_t ComputeTBMProjectDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/**
	 * Fold one fixed-width field into the hash, most significant byte first.
	 * @param value Unsigned field value.
	 * @param width Portable byte width.
	 */
	const auto add = [&hash](uint64_t value, uint width) {
		for (uint i = width; i != 0; --i) {
			hash ^= (value >> ((i - 1) * 8)) & 255;
			hash *= 1099511628211ULL;
		}
	};
	add(0x54424D50524F4A31ULL, 8);
	add(_next_tbm_project.base(), 4);
	add(_tbm_projects.size(), 4);
	// Every saved field defines the store; caches and order lists stay with their own owners and digests.
	for (const auto &project : _tbm_projects) {
		add(project.id.base(), 4); add(project.job.base(), 4); add(project.company.base(), 1); add(project.cutter.base(), 4);
		add(project.model_id, 2); add(project.permanent_parts.size(), 1);
		for (VehicleID part : project.permanent_parts) add(part.base(), 4);
		add(to_underlying(project.heading), 1); add(project.origin.base(), 2); add(project.portal.base(), 4); add(project.exchange_bay.base(), 4);
		add(to_underlying(project.phase), 1); add(to_underlying(project.blocked), 1); add(project.phase_ticks, 8); add(project.work_remainder, 2);
		add(project.active_ticks, 8); add(project.idle_ticks, 8); add(project.cost_remainder, 8); add(project.revision, 8);
	}
	const auto support = CaptureTBMSupport();
	add(support.size(), 4);
	// Support metadata is saved with the projects, so it belongs to the same digest.
	for (const auto &tile : support) {
		add(tile.tile.base(), 4); add(tile.project.base(), 4); add(tile.owner.base(), 1); add(tile.track.base(), 1);
		add(tile.allowed.base(), 1); add(tile.max_speed, 2); add(static_cast<uint64_t>(tile.conversion_price.base()), 8);
	}
	return hash;
}

/** Clear the live store and unpublished load staging when the world initializes. */
void ResetTBMProjectState()
{
	_tbm_projects.clear();
	_next_tbm_project = TBMProjectID{0};
	_pending_tbm_projects.reset();
	ResetTBMSupportState();
	ResetTBMExchangeState(); // Assignments name projects; none survives the store.
	ResetTBMProjectLoadStaging();
}

/**
 * Keep structurally checked loaded projects unpublished until vehicles are rebuilt after load.
 * @param projects Strictly increasing loaded records.
 * @param next_id Saved allocation cursor.
 */
void StageLoadedTBMProjects(std::vector<TBMProject> projects, TBMProjectID next_id)
{
	_pending_tbm_projects.emplace(std::move(projects), next_id);
}

/**
 * Normalize lost references, validate staged projects against rebuilt owners, then publish them.
 * Normalization is deterministic on every client, because it reads only the loaded game state.
 * @return False when the staged projects refuse; the live store is then left empty.
 */
bool AcceptLoadedTBMProjects()
{
	if (!_pending_tbm_projects.has_value()) return AcceptLoadedTBMSupport(_next_tbm_project); // Older saves carry no project family; staged support without it refuses.
	auto [projects, next_id] = std::move(*_pending_tbm_projects);
	_pending_tbm_projects.reset();
	// A station, job or train lost between the last controller step and the save becomes a recoverable block.
	for (auto &project : projects) {
		if (project.id == TBMProjectID{UINT32_MAX} || !Company::IsValidID(project.company)) continue; // Left for validation to refuse.
		if (NormalizeTBMProjectReferences(project)) ++project.revision;
	}
	const auto error = ValidateTBMProjects(projects, next_id);
	if (error != TBMProjectError::None) {
		Debug(Facility::Misc, Severity::Error, "MLVS_TBM_PROJECTS_REFUSED error={}", to_underlying(error));
		return false;
	}
	_tbm_projects = std::move(projects);
	_next_tbm_project = next_id;
	// Support rows name their projects and real track; a refusal leaves both stores empty.
	if (!AcceptLoadedTBMSupport(next_id)) {
		_tbm_projects.clear();
		_next_tbm_project = TBMProjectID{0};
		Debug(Facility::Misc, Severity::Error, "MLVS_TBM_SUPPORT_REFUSED");
		return false;
	}
	Debug(Facility::Misc, Severity::Info, "MLVS_TBM_PROJECTS_LOADED projects={} next={} support={}", _tbm_projects.size(), _next_tbm_project.base(), CaptureTBMSupport().size());
	return true;
}
