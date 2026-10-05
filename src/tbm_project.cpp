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
 */

#include "stdafx.h"
#include "tbm.h"
#include "tbm_cmd.h"
#include "tbm_exchange.h"
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

#include "safeguards.h"

static std::vector<TBMProject> _tbm_projects; ///< Live projects in strictly increasing ID order.
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
static const Train *GetLiveTBMCutter(const TBMProject &project)
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
	if (front == nullptr || project.permanent_parts.size() < 2 || project.permanent_parts.size() > MAX_TBM_PERMANENT_PARTS) return false;
	return InspectTBMConsist(*front, project).error == TBMBlockedReason::None;
}

/**
 * @param tile Saved portal reference.
 * @return Whether it names the canonical upper key of some pair (it may still be foreign or the wrong transport).
 */
static bool IsTBMPortalKey(TileIndex tile)
{
	if (tile == INVALID_TILE || !IsValidTile(tile)) return false;
	const auto key = GetLayerPortalPairKey(tile);
	return key.has_value() && *key == tile;
}

/**
 * @param project Record to check.
 * @return The live geological job this project may work on, or nullptr.
 */
static const ExcavationJob *GetLiveTBMJob(const TBMProject &project)
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
 * Validate one project against the live owners.
 * @param project Complete record with an allocated ID.
 * @return None or the first stable refusal.
 */
TBMProjectError ValidateTBMProject(const TBMProject &project)
{
	if (project.id == TBMProjectID{UINT32_MAX}) return TBMProjectError::InvalidProject;
	// Unknown phases or reasons cannot be resumed, and an unusable stamp cannot order later transitions.
	if (to_underlying(project.phase) > to_underlying(TBMPhase::Cancelled) || to_underlying(project.blocked) >= to_underlying(TBMBlockedReason::End) || !IsValidDirection(project.heading) || project.revision == 0 || project.revision == UINT64_MAX) return TBMProjectError::InvalidProject;
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
		if (project.permanent_parts.size() < 2 || project.permanent_parts.size() > MAX_TBM_PERMANENT_PARTS) return TBMProjectError::InvalidConsist;
	}
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
	if (!Company::IsValidID(project.company) || project.company != _current_company) return TBMProjectError::InvalidOwner;
	if (project.blocked != TBMBlockedReason::None || IsTBMProjectFinished(project.phase)) return TBMProjectError::InvalidProject; // A fresh project starts unblocked.
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
 * @return Whether the whole consist stands withdrawn and empty: stopped, every part on one tile in the depot state,
 *         that tile being the project's Source bay or a genuine rail depot, and no cargo aboard any part.
 */
static bool IsTBMConsistWithdrawn(const Train &front, const TBMProject &project)
{
	const TileIndex tile = front.tile;
	if (!front.vehstatus.Test(VehState::Stopped) || (tile != project.portal && !IsRailDepotTile(tile))) return false;
	// Outside the work pocket and on one tile, with no spoil or other cargo leaving the project with the train.
	for (const Train *part = &front; part != nullptr; part = part->Next()) {
		if (part->tile != tile || part->track != Track::Depot || part->cargo.TotalCount() != 0) return false;
	}
	return true;
}

/**
 * Erase a finished project so its consist becomes an ordinary train (Group 17.2).
 * @param project Record of the command actor.
 * @param execute False validates only.
 * @return None, or Unavailable while the project is unfinished, a hauler serves it or a set of it carries spoil, its
 *         consist is not withdrawn and empty, or its Commissioning pair cannot be withdrawn yet.
 */
static TBMProjectError RetireTBMProject(const TBMProject &project, bool execute)
{
	if (!IsTBMProjectFinished(project.phase) || !IsTBMProjectExchangeIdle(project.id)) return TBMProjectError::Unavailable;
	if (const Train *front = Train::GetIfValid(project.cutter); front != nullptr && !IsTBMConsistWithdrawn(*front, project)) return TBMProjectError::Unavailable;
	// The pair Commissioning this project is withdrawn with it (r2mlvs3 Group 11.3), once its rock side is empty;
	// otherwise the record would stay bound to an ordinary train that may then be sold.
	const auto *record = IsTBMPortalKey(project.portal) ? GetLayerPortalWorkRecord(project.portal) : nullptr;
	const bool bound = record != nullptr && record->state == LayerPortalWorkState::Commissioning && (record->project == project.job || record->cutter == project.cutter);
	if (bound && WithdrawLayerPortalWorkPair(project.portal, project.job, project.cutter, false) != LayerPortalWorkError::None) return TBMProjectError::Unavailable;
	if (!execute) return TBMProjectError::None;
	if (bound) {
		[[maybe_unused]] const auto withdrawn = WithdrawLayerPortalWorkPair(project.portal, project.job, project.cutter, true);
		assert(withdrawn == LayerPortalWorkError::None);
	}
	const TBMProjectID id = project.id;
	ForgetTBMExchangeProject(id); // Only empty sets are left; their wagons simply stop being a set.
	std::erase_if(_tbm_projects, [id](const TBMProject &candidate) { return candidate.id == id; });
	return TBMProjectError::None;
}

TBMProjectError ControlTBMProject(TBMProjectID id, TBMProjectAction action, bool execute)
{
	if (to_underlying(action) >= to_underlying(TBMProjectAction::End)) return TBMProjectError::InvalidProject;
	TBMProject *project = FindTBMProject(id);
	if (project == nullptr) return TBMProjectError::InvalidProject;
	if (project->company != _current_company) return TBMProjectError::InvalidOwner;
	if (action == TBMProjectAction::Retire) return RetireTBMProject(*project, execute); // The one control of a finished project.
	if (IsTBMProjectFinished(project->phase)) return TBMProjectError::Unavailable;
	TBMProject next = *project;
	switch (action) {
		case TBMProjectAction::Pause:
			if (project->blocked == TBMBlockedReason::Paused) return TBMProjectError::Unavailable;
			next.blocked = TBMBlockedReason::Paused; // The phase is kept.
			break;
		case TBMProjectAction::Resume:
			if (project->blocked != TBMBlockedReason::Paused) return TBMProjectError::Unavailable; // Controller-derived waits are not the player's to clear.
			next.blocked = TBMBlockedReason::None;
			NormalizeTBMProjectReferences(next); // A loss that happened during the pause becomes the reason at once.
			break;
		case TBMProjectAction::Cancel: {
			const auto *job = project->job == ExcavationJobID{UINT32_MAX} ? nullptr : GetExcavationJob(project->job);
			const bool lost = project->cutter == VehicleID::Invalid();
			const bool open = job != nullptr && job->state != ExcavationJobState::Cancelled && job->state != ExcavationJobState::Completed;
			// An open job must be the project's own and still stampable. With its cutter live, a project without an open job
			// has nothing to cancel; a project that lost its cutter can always be ended, whatever its job.
			if (open && (job->company != project->company || job->revision == UINT64_MAX)) return TBMProjectError::Unavailable;
			if (!open && !lost) return TBMProjectError::Unavailable;
			// Without a cutter there is nothing to bring back: its train and anything aboard went when it was deleted.
			// Otherwise a forward phase turns back to the bay; a return or an exchange already under way simply goes on.
			if (lost) {
				next.phase = TBMPhase::Cancelled;
				next.phase_ticks = 0;
			} else if (project->phase == TBMPhase::Commissioning || project->phase == TBMPhase::TravelToFace || project->phase == TBMPhase::Bore || project->phase == TBMPhase::AdvanceSupport) {
				next.phase = TBMPhase::ReturnForExchange;
				next.phase_ticks = 0;
			}
			next.blocked = TBMBlockedReason::None; // A pause gives way to the return.
			if (execute && open) {
				// Unfinished claimed cells return to rock; cavities, committed totals and the spoil aboard stay.
				[[maybe_unused]] const auto cancelled = CancelExcavationJob(project->job);
				assert(cancelled == ExcavationError::None);
			}
			NormalizeTBMProjectReferences(next); // A loss that happened meanwhile is the reason at once.
			break;
		}
		default:
			return TBMProjectError::Unavailable;
	}
	if (!execute) return TBMProjectError::None;
	++next.revision;
	*project = std::move(next);
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
	if (IsTBMProjectFinished(project->phase) || project->exchange_bay == bay) return TBMProjectError::Unavailable;
	if (!execute) return TBMProjectError::None;
	project->exchange_bay = bay;
	++project->revision;
	return TBMProjectError::None;
}

void ReleaseTBMProjectExchangeBay(TileIndex bay)
{
	for (auto &project : _tbm_projects) {
		if (project.exchange_bay != bay) continue;
		// The Source bay is the project's own portal key; without one the project has no bay at all.
		project.exchange_bay = IsTBMPortalKey(project.portal) ? project.portal : INVALID_TILE;
		++project.revision;
	}
}

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
	for (const auto &project : removed) {
		const auto *job = project.job == ExcavationJobID{UINT32_MAX} ? nullptr : GetExcavationJob(project.job);
		if (job != nullptr && job->state != ExcavationJobState::Completed && job->state != ExcavationJobState::Cancelled) {
			[[maybe_unused]] const auto cancelled = CancelExcavationJob(project.job);
		}
		const auto *record = IsTBMPortalKey(project.portal) ? GetLayerPortalWorkRecord(project.portal) : nullptr;
		if (record != nullptr && record->state == LayerPortalWorkState::Commissioning && record->project == project.job) ReleaseLayerPortalWorkBinding(project.portal);
		ForgetTBMExchangeProject(project.id);
	}
	std::erase_if(_tbm_projects, [old_owner](const TBMProject &project) { return project.company == old_owner; });
	ChangeExcavationJobOwnership(old_owner, new_owner);
}

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

void OnTBMStationDeleted(StationID station)
{
	for (auto &project : _tbm_projects) {
		if (project.origin != station) continue;
		project.origin = StationID::Invalid();
		// An unfinished project shows the loss at once; a pause or an earlier block is kept, as normalization does.
		if (!IsTBMProjectFinished(project.phase) && project.blocked == TBMBlockedReason::None) project.blocked = TBMBlockedReason::MissingOrigin;
		++project.revision;
	}
	ForgetLayerPortalWorkOrigin(station);
}

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
	if (ControlTBMProject(project, action, flags.Test(DoCommandFlag::Execute)) != TBMProjectError::None) return CMD_ERROR;
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
	const bool forward = project.phase == TBMPhase::Commissioning || project.phase == TBMPhase::TravelToFace || project.phase == TBMPhase::Bore || project.phase == TBMPhase::AdvanceSupport;
	if (inputs.cancelled && forward) {
		step.phase = TBMPhase::ReturnForExchange;
		step.phase_changed = true;
		return step;
	}

	/* Edges. The ReturnForExchange entry guard needs the job's frozen pass terms, never a guess. */
	const auto *job = GetLiveTBMJob(project);
	const bool exchange_due = inputs.exchange_due && job != nullptr && job->pass_terms.has_value();
	switch (project.phase) {
		case TBMPhase::Commissioning:
			step.phase = TBMPhase::TravelToFace; // Published, unblocked and started: leave the source site for the face.
			break;
		case TBMPhase::TravelToFace:
			if (inputs.at_face) step.phase = TBMPhase::Bore;
			break;
		case TBMPhase::Bore:
			// An exchange that is due must happen before the next cut; otherwise a committed cut moves on to support.
			if (exchange_due) {
				step.phase = TBMPhase::ReturnForExchange;
			} else if (inputs.cut_committed) {
				step.phase = TBMPhase::AdvanceSupport;
			}
			break;
		case TBMPhase::AdvanceSupport:
			if (!inputs.support_advanced) break;
			if (exchange_due) {
				step.phase = TBMPhase::ReturnForExchange;
			} else if (inputs.route_complete) {
				step.error = TBMProjectError::InvalidProject; // Frozen terms always end with a final return; finishing without one is inconsistent.
			} else {
				step.phase = TBMPhase::Bore;
			}
			break;
		case TBMPhase::ReturnForExchange:
			if (inputs.at_exchange) step.phase = TBMPhase::WaitForEmpties; // The consist stands wholly in its bay.
			break;
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
	inputs.servicing = front->current_order.IsType(OT_GOTO_DEPOT);
	const auto *job = GetLiveTBMJob(project);
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
	if (IsTBMProjectFinished(project->phase)) return TBMProjectError::None; // Finished projects keep their history and do not step.
	auto inputs = ObserveTBMPhaseInputs(*project);
	// The face works only when this step leaves the project unblocked in Bore with no exchange due, so a paused,
	// stopped, broken-down, servicing or reference-lost project never cuts. An earlier work wait is retried.
	const bool works = project->phase == TBMPhase::Bore && project->blocked != TBMBlockedReason::Paused && FindTBMProjectReferenceLoss(*project) == TBMBlockedReason::None && !inputs.train_stopped && !inputs.broken_down && !inputs.servicing && !inputs.exchange_due && !inputs.cancelled;
	if (works) {
		const auto work = AdvanceTBMFaceWork(*project);
		inputs.cut_committed = work.cut_committed;
		inputs.work_wait = work.wait;
		project->work_remainder = work.work_remainder; // Face work never touches the store, so the pointer is still valid.
		if (work.cut_committed) {
			project->heading = work.heading; // Only a committed advance turns the cutter.
			// A commit that makes the exchange due (wagons full, or the final return) sends the consist straight back,
			// rather than first onto support it would not use before the exchange.
			inputs.exchange_due = IsTBMExchangeDue(*project);
		}
	}
	return AdvanceTBMProjectPhase(id, inputs);
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
