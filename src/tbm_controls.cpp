/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_controls.cpp The player's view of a rail portal pair and its boring project (R2MLVS3MP master 7.9): the
 * controls the portal window offers and the next step of a project's life, derived from the commands' own checks.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical managed destinations.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23B actual face-hold guidance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, tasks 16.1/16.3
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, task 13.4
 */

#include "stdafx.h"
#include "tbm_controls.h"
#include "tbm_gui.h"
#include "excavation.h"
#include "layer_portal_cmd.h"
#include "train.h"
#include "cargotype.h"
#include "company_func.h"
#include "core/backup_type.hpp"
#include "table/strings.h"

#include "safeguards.h"

/** @copydoc GetGeologyHardnessString */
StringID GetGeologyHardnessString(GeologyHardness hardness)
{
	switch (hardness) {
		case GeologyHardness::Soft: return STR_PORTAL_HARDNESS_SOFT;
		case GeologyHardness::Mixed: return STR_PORTAL_HARDNESS_MIXED;
		case GeologyHardness::Hard: return STR_PORTAL_HARDNESS_HARD;
		case GeologyHardness::Basalt: return STR_PORTAL_HARDNESS_BASALT;
		default: return STR_PORTAL_HARDNESS_UNKNOWN;
	}
}

/** @copydoc SumTBMCutterSpoil */
TBMSpoilLoad SumTBMCutterSpoil(const Train &cutter, const TBMProject &project)
{
	TBMSpoilLoad spoil;
	// Only whole attached units contribute to replacement counts and storage.
	// Articulated parts carry their own packets/capacity; planned pass limits
	// and historical dug totals must never stand in for this live inventory.
	for (const TBMConsistUnit &unit : InspectTBMConsist(cutter, project).exchangeable_units) {
		if (unit.role == TBMConsistRole::EarthWagon) ++spoil.earth_hoppers;
		if (unit.role == TBMConsistRole::RockWagon) ++spoil.rock_hoppers;
		const Train *head = Train::Get(unit.vehicle);
		const Train *next_unit = head->GetNextUnit();
		// Stop at this unit's boundary so each articulated part is counted
		// exactly once, including when physical exchange replaces the head.
		for (const Train *part = head; part != next_unit; part = part->Next()) {
			if (unit.role == TBMConsistRole::EarthWagon) {
				spoil.earth_loaded_t += part->cargo.TotalCount();
				spoil.earth_capacity_t += part->cargo_cap;
			} else {
				spoil.rock_loaded_t += part->cargo.TotalCount();
				spoil.rock_capacity_t += part->cargo_cap;
			}
		}
	}
	return spoil;
}

/** @copydoc SumTrainSpoil */
TBMSpoilLoad SumTrainSpoil(const Train &front)
{
	TBMSpoilLoad spoil;
	// Every part counts by its own cargo: a hauler's hoppers may be refitted either way and its engine carries nothing.
	for (const Train *part = &front; part != nullptr; part = part->Next()) {
		if (!IsValidCargoType(part->cargo_type)) continue;
		const CargoLabel label = CargoSpec::Get(part->cargo_type)->label;
		if (label == CargoLabel{"ERTH"}) {
			spoil.earth_loaded_t += part->cargo.TotalCount();
			spoil.earth_capacity_t += part->cargo_cap;
		} else if (label == CargoLabel{"ROCK"}) {
			spoil.rock_loaded_t += part->cargo.TotalCount();
			spoil.rock_capacity_t += part->cargo_cap;
		}
	}
	return spoil;
}


/** @copydoc GetTBMOrderProblem */
std::optional<StringID> GetTBMOrderProblem(const Train &front)
{
	const auto *cutter_project = GetTBMProjectForCutter(front.index);
	const auto *assignment = GetTBMExchangeAssignmentForHauler(front.index);
	if (cutter_project == nullptr && assignment == nullptr) return std::nullopt; // Ordinary trains retain normal manual-order review.
	const auto *project = cutter_project != nullptr ? cutter_project : GetTBMProject(assignment->project);
	if (project == nullptr) return STR_PORTAL_HOLD_MISSING_PROJECT;
	if (front.owner != project->company) return STR_TBM_OPERATION_WRONG_OWNER;
	if (cutter_project != nullptr) {
		const auto loss = FindTBMProjectReferenceLoss(*project);
		return loss == TBMBlockedReason::None ? INVALID_STRING_ID : GetTBMHoldString(loss);
	}
	const auto origin = CheckTBMOriginAdmission(*project);
	if (origin == TBMExchangeError::MissingOrigin) return STR_PORTAL_WAIT_LOST_ORIGIN;
	if (origin != TBMExchangeError::None) return STR_TBM_OPERATION_NO_RECEIVER;
	const auto bay = GetTBMExchangeBay(*project);
	if (bay.error != TBMExchangeError::None || bay.tile != assignment->bay) return STR_PORTAL_HOLD_MISSING_EXCHANGE; // A stale bay reference cannot substitute for the actual empty-return destination.
	const auto *set = GetTBMWagonSet(assignment->set);
	if (set == nullptr || set->project != project->id) return STR_PORTAL_WAIT_WRONG_WAGONS; // A different project's wagons do not complete this managed cycle.
	// Follow the real units, including every articulated part, so a detached
	// or transferred wagon cannot be hidden by a still-present assignment row.
	for (VehicleID id : set->wagons) {
		const Train *unit = Train::GetIfValid(id);
		if (unit == nullptr || unit->First() != &front) return STR_PORTAL_WAIT_WRONG_WAGONS;
		for (const Train *part = unit; part != unit->GetNextUnit(); part = part->Next()) {
			if (part->owner != project->company) return STR_TBM_OPERATION_WRONG_OWNER;
		}
	}
	/** @param order Actual manual order. @return Whether it supplies the mandatory Surface origin leg. */
	const auto visits_origin = [project](const Order &order) {
		return order.IsType(OT_GOTO_STATION) && order.GetDestination().ToStationID() == project->origin;
	};
	if (!std::ranges::any_of(front.Orders(), visits_origin)) return STR_TBM_OPERATION_ORIGIN_ORDER;
	// An admitted stationary magazine train or a hauler held at its real face meeting has reached its managed
	// destination. A stale path-search flag there must not ask the player to repair a healthy native logistics cycle.
	if (front.vehicle_flags.Test(VehicleFlag::PathfinderLost) && CheckTBMExchangeParticipant(front, bay, project->company) != TBMExchangeError::None && !IsTBMHaulerHeld(front)) return STR_PORTAL_WAIT_BLOCKED_LINE;
	return INVALID_STRING_ID;
}

/**
 * @param train Front engine.
 * @param tile Tile to look at.
 * @return Whether a part of the train stands on the tile.
 */
static bool IsTrainOnTile(const Train &train, TileIndex tile)
{
	for (const Train *part = &train; part != nullptr; part = part->Next()) {
		if (part->tile == tile) return true;
	}
	return false;
}

/**
 * @param train Front engine.
 * @param key Portal key.
 * @return Whether the whole train stands stopped in the depot state on the key, as a withdrawn train does.
 */
static bool IsTrainWithdrawnAt(const Train &train, TileIndex key)
{
	if (!train.vehstatus.Test(VehState::Stopped)) return false;
	// A withdrawn label requires every actual part in this bay's storage; a head alone cannot release occupied track.
	for (const Train *part = &train; part != nullptr; part = part->Next()) {
		if (part->tile != key || part->track != Track::Depot) return false;
	}
	return true;
}

/**
 * @param reason Saved hold of an unfinished project.
 * @return Whether only the player can remove it; a breakdown, a service visit or full hoppers clear by themselves.
 */
static bool IsTBMHoldForPlayer(TBMBlockedReason reason)
{
	switch (reason) {
		case TBMBlockedReason::None:
		case TBMBlockedReason::Stopped:
		case TBMBlockedReason::Breakdown:
		case TBMBlockedReason::Servicing:
		case TBMBlockedReason::EarthFull:
		case TBMBlockedReason::RockFull:
		case TBMBlockedReason::LogisticsStall:
		case TBMBlockedReason::Paused:
			return false;
		default:
			return true;
	}
}

/**
 * Choose the next step for a pair with a project, after the controls are known.
 * @param controls Controls of the project, every field but the step filled in.
 * @return The step.
 */
static TBMPlayerStep ChooseTBMProjectStep(const TBMPairControls &controls)
{
	const TBMProject &project = *controls.project;
	if (IsTBMProjectFinished(project.phase)) {
		if (controls.hauler.has_value()) {
			// The last load must reach the origin before the hauler goes, and it must clear the bay to be released.
			const TBMHaulerPhase phase = controls.hauler->phase;
			if (phase == TBMHaulerPhase::CollectLoaded || phase == TBMHaulerPhase::VisitOrigin || phase == TBMHaulerPhase::DisposeOrTransfer) return TBMPlayerStep::DeliverLast;
			return controls.release ? TBMPlayerStep::ReleaseHauler : TBMPlayerStep::WaitForHauler;
		}
		if (!controls.convertible.empty()) return TBMPlayerStep::ConvertTrack;
		if (controls.retire) return TBMPlayerStep::Retire;
		return controls.running ? TBMPlayerStep::StopMachine : TBMPlayerStep::FixHold;
	}
	// Stored machines and ordinary-order travel retain unfinished work until the player explicitly hands control back.
	if (project.phase == TBMPhase::Parked || project.phase == TBMPhase::ParkedHangar || project.phase == TBMPhase::FollowOrders) return TBMPlayerStep::Resume;
	if (project.phase == TBMPhase::ReleasePending) return controls.retire ? TBMPlayerStep::Retire : TBMPlayerStep::DeliverLast;
	if (project.blocked == TBMBlockedReason::Paused) return TBMPlayerStep::Resume;
	if (!controls.running) return TBMPlayerStep::StartMachine;
	if (controls.wait == TBMLogisticsWait::NoHauler) return TBMPlayerStep::AssignHauler;
	// A hauler short of empties, without a route on its orders or without an origin needs the player.
	if (controls.wait == TBMLogisticsWait::NoEmpties || controls.wait == TBMLogisticsWait::BlockedLine || controls.wait == TBMLogisticsWait::LostOrigin) return TBMPlayerStep::FixHold;
	return IsTBMHoldForPlayer(project.blocked) ? TBMPlayerStep::FixHold : TBMPlayerStep::Working;
}

/**
 * Fill in the controls of a pair whose project of the company is known.
 * @param controls Controls with the project set.
 */
static void DescribeTBMProject(TBMPairControls &controls)
{
	const TBMProject &project = *controls.project;
	controls.work = GetTBMWorkState(project.id);
	if (controls.work.face != INVALID_TILE) {
		controls.hardness = GetExcavationCellHardness(GetGeologyWorldState(), controls.work.face);
		controls.face_work_ticks = EstimateExcavationCut(project.job).work_ticks;
	}
	if (const TBMCutterModel *model = GetTBMCutterModel(project.model_id); model != nullptr) {
		controls.active_cost_per_year = model->active_cost_per_year;
		controls.idle_cost_per_year = model->idle_cost_per_year;
	}
	if (const ExcavationJob *job = GetExcavationJob(project.job); job != nullptr) {
		controls.cuts_done = job->cursor;
		controls.cuts_total = static_cast<uint32_t>(job->route.size());
		controls.earth_t = job->completed_earth_t;
		controls.rock_t = job->completed_rock_t;
		// Accepted exchange boundaries split the geological owner's sole cursor
		// into passes. At completion retain the last pass's completed progress;
		// while exchanging, the next pass is shown without advancing any work.
		if (job->pass_terms.has_value()) {
			const auto &ends = job->pass_terms->exchange_after;
			if (!ends.empty()) {
				const auto next = std::ranges::upper_bound(ends, job->cursor);
				const size_t index = next == ends.end() ? ends.size() - 1 : static_cast<size_t>(next - ends.begin());
				const uint32_t start = index == 0 ? 0 : ends[index - 1];
				controls.pass_number = static_cast<uint32_t>(index + 1);
				controls.pass_total = static_cast<uint32_t>(ends.size());
				controls.pass_cuts_total = ends[index] - start;
				controls.pass_cuts_done = std::min(job->cursor - start, controls.pass_cuts_total);
			}
		}
	}
	if (const Train *cutter = Train::GetIfValid(project.cutter); cutter != nullptr) {
		controls.running = !cutter->vehstatus.Test(VehState::Stopped);
		// The controls and work-change admission share the same actual attached packet measurement.
		const auto spoil = SumTBMCutterSpoil(*cutter, project);
		controls.earth_hoppers = spoil.earth_hoppers;
		controls.rock_hoppers = spoil.rock_hoppers;
		controls.earth_loaded_t = spoil.earth_loaded_t;
		controls.earth_capacity_t = spoil.earth_capacity_t;
		controls.rock_loaded_t = spoil.rock_loaded_t;
		controls.rock_capacity_t = spoil.rock_capacity_t;
	}
	controls.wait = GetTBMLogisticsWait(project);
	if (const TBMExchangeAssignment *assignment = GetTBMExchangeAssignment(project.id); assignment != nullptr) controls.hauler = *assignment;

	controls.pause = ControlTBMProject(project.id, TBMProjectAction::Pause, false) == TBMProjectError::None;
	controls.resume = ControlTBMProject(project.id, TBMProjectAction::Resume, false) == TBMProjectError::None;
	controls.park = ControlTBMProject(project.id, TBMProjectAction::ReturnAndPark, false) == TBMProjectError::None;
	controls.cancel = ControlTBMProject(project.id, TBMProjectAction::Cancel, false) == TBMProjectError::None;
	controls.retire = ControlTBMProject(project.id, TBMProjectAction::Retire, false) == TBMProjectError::None;
	controls.assign = CheckTBMProjectTakesHauler(project.id) == TBMExchangeError::None;
	if (controls.hauler.has_value()) {
		// A released hauler inside the bay would have no right to leave it, so it is only released once outside.
		const Train *hauler = Train::GetIfValid(controls.hauler->hauler);
		controls.release = hauler != nullptr && !IsTrainOnTile(*hauler, controls.hauler->bay) && ReleaseTBMHauler(hauler->index, false) == TBMExchangeError::None;
	}
	// The track this project laid becomes free to convert once the project is finished.
	for (const TBMSupportTile &support : CaptureTBMSupport()) {
		if (support.project == project.id && support.owner == project.company && !IsTBMSupportInUse(support.tile)) controls.convertible.push_back(support.tile);
	}
	controls.step = ChooseTBMProjectStep(controls);
}

/**
 * Fill in the controls of a pair with no project of the company.
 * @param key Canonical pair key.
 * @param company Company asking.
 * @param controls Controls to fill in.
 */
static void DescribeTBMPair(TileIndex key, CompanyID company, TBMPairControls &controls)
{
	// A company train left withdrawn in the portal with no project or assignment: a retired machine, or a hauler.
	for (const Train *train : Train::Iterate()) {
		if (!train->IsFrontEngine() || train->owner != company || train->tile != key || !IsTrainWithdrawnAt(*train, key)) continue;
		if (GetTBMProjectForCutter(train->index) != nullptr || GetTBMExchangeAssignmentForHauler(train->index) != nullptr) continue;
		controls.sellable = train->index;
		break;
	}
	// Released track whose project has retired no longer belongs to any pair, so every such piece of the company is offered.
	for (const TBMSupportTile &support : CaptureTBMSupport()) {
		if (support.owner == company && GetTBMProject(support.project) == nullptr) controls.convertible.push_back(support.tile);
	}
	controls.connect = ChangeLayerPortalConnection(key, company, true, false) == LayerPortalWorkError::None;
	// Selling a retained machine is optional. Cleared public track can be
	// connected while the complete stopped native consist stays in its magazine.
	if (!controls.convertible.empty()) {
		controls.step = TBMPlayerStep::ConvertTrack;
	} else if (GetLayerPortalWorkState(key) == LayerPortalWorkState::Connected) {
		controls.step = TBMPlayerStep::Connected;
	} else {
		controls.step = controls.connect ? TBMPlayerStep::Connect : TBMPlayerStep::Commission;
	}
}

/**
 * Derive controls from original participants without changing work or advancing simulation.
 * @param key Actual canonical rail-pair key.
 * @param company Company whose authorized actions the view describes.
 * @return Current controls and physical lifecycle guidance for this pair.
 */
TBMPairControls GetTBMPairControls(TileIndex key, CompanyID company)
{
	// The command checks below answer for their actor, so they run as the asking company.
	AutoRestoreBackup actor(_current_company, company);
	TBMPairControls controls;
	// A pair binds at most one project at a time; it is the company's own if it is offered at all.
	for (const TBMProject &project : CaptureTBMProjects()) {
		if (project.portal == key && project.company == company) controls.project = project;
	}
	if (controls.project.has_value()) {
		DescribeTBMProject(controls);
	} else {
		DescribeTBMPair(key, company, controls);
	}
	return controls;
}
