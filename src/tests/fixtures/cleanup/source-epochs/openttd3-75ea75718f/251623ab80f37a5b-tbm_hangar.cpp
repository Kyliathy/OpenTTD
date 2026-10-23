/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_hangar.cpp Admission for actual native depot and portal-magazine destinations.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group8
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "stdafx.h"
#include "tbm_hangar.h"
#include "tbm_cmd.h"
#include "tbm.h"
#include "tbm_exchange.h"
#include "train.h"
#include "depot_base.h"
#include "company_base.h"
#include "company_func.h"
#include "command_func.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "vehicle_func.h"
#include "viewport_func.h"
#include "train_cmd.h"
#include "window_func.h"
#include "pbs.h"
#include "pathfinder/yapf/yapf.h"
#include "timer/timer_game_calendar.h"
#include "table/strings.h"

#include "safeguards.h"

/** @copydoc DescribeTBMHangar */
TBMHangarInfo DescribeTBMHangar(const Train &front, TileIndex destination)
{
	TBMHangarInfo result;
	if (Train::GetIfValid(front.index) != &front || !front.IsFrontEngine() || front.First() != &front ||
			!Company::IsValidID(front.owner) || front.vehstatus.Test(VehState::Crashed) || !IsTBMCutterEngine(front.engine_type)) {
		result.error = TBMHangarError::InvalidMachine;
		return result;
	}
	if (!IsValidTile(destination)) return result;
	const bool portal = IsLayerPortalTile(destination) && GetLayerPortalTransportType(destination) == TransportType::Rail;
	if (portal) {
		const auto key = GetLayerPortalPairKey(destination);
		if (!key) return result;
		destination = *key; // The existing native magazine belongs to the upper housing, regardless of which endpoint the player clicks.
		result.kind = TBMHangarKind::PortalMagazine;
	} else if (!IsRailDepotTile(destination)) {
		return result;
	}
	result.tile = destination;
	if (GetTileOwner(destination) != front.owner) {
		result.error = TBMHangarError::WrongOwner;
		return result;
	}
	if (!front.compatible_railtypes.Test(GetRailType(destination))) {
		result.error = TBMHangarError::IncompatibleRail;
		return result;
	}
	if ((GetTileLayer(destination) == MapLayer::DeepUnderground &&
			!CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) ||
			(portal && CheckLayerPortalAvailability(destination) == LayerPortalWorkError::LockedDeep)) {
		result.error = TBMHangarError::LockedLayer;
		return result;
	}
	const auto model = GetTBMEngineModel(front.engine_type);
	const auto &policy = GetCurrentMultiLayerProfileSelection();
	// The same native model/layer/date capability used for commissioning
	// governs storage access. An upper housing cannot disguise an unsupported
	// lower layer merely because its rail gauge matches the cutter.
	if (!IsTBMCutterAvailable(model, GetTileLayer(destination), policy, TimerGameCalendar::date) ||
			(portal && !IsTBMCutterAvailable(model, GetTileLayer(GetLayerPortalPairTile(destination)), policy, TimerGameCalendar::date))) {
		result.error = TBMHangarError::InvalidDestination;
		return result;
	}
	const TBMProject *project = GetTBMProjectForCutter(front.index);
	if (portal) {
		const auto *work = GetLayerPortalWorkRecord(destination);
		if (work == nullptr) return result;
		if (IsLayerPortalHangarTile(destination)) {
			const Depot *depot = Depot::GetIfValid(GetLayerPortalHangarDepotIndex(destination));
			if (depot == nullptr || depot->xy != destination) return result;
			result.depot = depot->index;
		}
		if (work->company != front.owner) {
			result.error = TBMHangarError::WrongOwner;
			return result;
		}
		const bool own_work = project != nullptr && project->portal == destination && work->cutter == front.index && work->project == project->job;
		// The original working magazine remains usable before excavation is
		// complete. Another portal must have a genuinely open far approach and
		// no other live work binding; choosing a hangar never commissions rock.
		if (!own_work && (work->state == LayerPortalWorkState::Commissioning ||
				CheckExcavationConstruction(std::array{GetLayerPortalPairTile(destination), GetLayerPortalDoorTile(GetLayerPortalPairTile(destination))}).Failed())) {
			result.error = TBMHangarError::UnfinishedPortal;
			result.blocker = work->cutter;
			return result;
		}
	} else {
		const Depot *depot = Depot::GetIfValid(GetDepotIndex(destination));
		if (depot == nullptr || depot->xy != destination) return result;
		result.depot = depot->index; // Never derive a native destination ID from the numeric tile address.
		// A finished older controller can precede a live user of this depot.
		// Inspect all actual assignments, rather than accepting the first match.
		for (const auto &other : CaptureTBMProjects()) {
			if (other.exchange_bay != destination || other.cutter == front.index || IsTBMProjectFinished(other.phase)) continue;
			result.error = TBMHangarError::Occupied;
			result.blocker = other.cutter;
			return result;
		}
	}
	// A native depot may store several whole trains. The portal magazine
	// admits one cutter and its own exchange hauler. Ordinary trains on the
	// separate public track use normal movement occupancy, not storage space.
	for (const Vehicle *vehicle : VehiclesOnTile(destination)) {
		if (vehicle->type != VehicleType::Train) continue;
		const Train *other = Train::From(vehicle)->First();
		if (other == &front) continue;
		if (!portal && Train::From(vehicle)->track == Track::Depot) continue;
		if (portal && Train::From(vehicle)->track != Track::Depot && !IsTBMCutterEngine(other->engine_type)) continue;
		const auto *assignment = GetTBMExchangeAssignmentForHauler(other->index);
		if (portal && project != nullptr && assignment != nullptr && assignment->project == project->id && assignment->bay == destination) continue;
		result.error = TBMHangarError::Occupied;
		result.blocker = other->index;
		return result;
	}
	result.error = TBMHangarError::None;
	return result;
}

/** @copydoc RegisterTBMHangarDepot */
DepotID RegisterTBMHangarDepot(const Train &front, TileIndex destination)
{
	const auto hangar = DescribeTBMHangar(front, destination);
	if (hangar.error != TBMHangarError::None) return DepotID::Invalid();
	if (hangar.depot != DepotID::Invalid()) return hangar.depot;
	if (hangar.kind != TBMHangarKind::PortalMagazine || !Depot::CanAllocateItem()) return DepotID::Invalid();
	Depot *depot = Depot::Create(hangar.tile);
	SetLayerPortalHangarDepot(hangar.tile, depot->index);
	MarkTileDirtyByTile(hangar.tile);
	return depot->index;
}

/** @copydoc HasTBMHangarHaltOrder */
bool HasTBMHangarHaltOrder(const Train &front)
{
	return IsTBMCutterEngine(front.engine_type) && front.current_order.IsType(OT_GOTO_DEPOT) && front.current_order.GetDepotActionType().Test(OrderDepotActionFlag::Halt);
}

/** @copydoc IsTBMStagingHangarOrder */
bool IsTBMStagingHangarOrder(const Train &front)
{
	const auto *project = GetTBMProjectForCutter(front.First()->index);
	return project != nullptr && (project->phase == TBMPhase::ReturnForHangar || project->phase == TBMPhase::WaitForHangar);
}

/** @copydoc GetTBMOrderedHangar */
TBMHangarInfo GetTBMOrderedHangar(const Train &front)
{
	if (!front.current_order.IsType(OT_GOTO_DEPOT)) return {};
	const auto *depot = Depot::GetIfValid(front.current_order.GetDestination().ToDepotID());
	if (depot == nullptr) return {};
	return DescribeTBMHangar(front, depot->xy);
}

/** @copydoc IsTBMWholeInHangar */
bool IsTBMWholeInHangar(const Train &front)
{
	const auto hangar = DescribeTBMHangar(front, front.tile);
	if (hangar.error != TBMHangarError::None || hangar.tile != front.tile || front.cur_speed != 0) return false;
	for (const Train *part = &front; part != nullptr; part = part->Next()) {
		if (part->tile != front.tile || part->track != Track::Depot || part->cargo.TotalCount() != 0) return false; // An arrived head cannot hide an outside or loaded tail.
	}
	return true;
}

/** @copydoc CanTBMUseHangarPortal */
bool CanTBMUseHangarPortal(const Train &front, TileIndex tile)
{
	const auto key = GetLayerPortalPairKey(tile);
	if (!key || !IsLayerPortalHangarTile(*key)) return false;
	const auto destination = DescribeTBMHangar(front, *key);
	if (destination.error != TBMHangarError::None) return false;
	const auto ordered = GetTBMOrderedHangar(front);
	if (!IsTBMStagingHangarOrder(front) && ordered.error == TBMHangarError::None && ordered.tile == *key) return true;
	// A saved actual tail preserves departure authority until the whole train
	// clears both housings. An unrelated approaching train has no such parts
	// and cannot acquire admission to a sealed portal by proximity alone.
	if (GetTBMProjectForCutter(front.index) == nullptr && !front.current_order.IsGotoOrder()) return false;
	const TileIndex pair = GetLayerPortalPairTile(*key);
	for (const Train *part = &front; part != nullptr; part = part->Next()) {
		if (part->tile == *key || part->tile == pair) return true;
	}
	return false;
}

/** @copydoc CmdSendTBMToHangar */
CommandCost CmdSendTBMToHangar(DoCommandFlags flags, VehicleID cutter, TileIndex destination, bool service)
{
	Train *front = Train::GetIfValid(cutter);
	if (front == nullptr) return CommandCost(STR_ERROR_TBM_HANGAR_UNAVAILABLE);
	const CommandCost ownership = CheckOwnership(front->owner);
	if (ownership.Failed()) return ownership;
	const auto hangar = DescribeTBMHangar(*front, destination);
	if (hangar.error != TBMHangarError::None) return CommandCost(hangar.error == TBMHangarError::Occupied ? STR_ERROR_TBM_HANGAR_OCCUPIED : STR_ERROR_TBM_HANGAR_UNAVAILABLE);
	const auto *project = GetTBMProjectForCutter(cutter);
	if (project != nullptr && (IsTBMFinishing(project->phase) || project->revision >= UINT64_MAX - 1)) return CommandCost(STR_ERROR_TBM_HANGAR_UNAVAILABLE);
	const TileIndex source = project != nullptr ? project->portal : front->IsChainInDepot() ? front->tile : INVALID_TILE;
	std::optional<TBMHangarInfo> source_hangar;
	if (source != hangar.tile && IsValidTile(source) && IsLayerPortalTile(source)) {
		source_hangar = DescribeTBMHangar(*front, source);
		if (source_hangar->error != TBMHangarError::None) return CommandCost(STR_ERROR_TBM_HANGAR_UNAVAILABLE);
	}
	const uint identities = (hangar.depot == DepotID::Invalid() ? 1 : 0) +
		(source_hangar.has_value() && source_hangar->depot == DepotID::Invalid() ? 1 : 0);
	if (identities != 0 && !Depot::CanAllocateItem(identities)) return CommandCost(STR_ERROR_TBM_HANGAR_UNAVAILABLE);
	if (!flags.Test(DoCommandFlag::Execute)) return CommandCost();
	// Everything which can refuse is prepared before allocating native storage
	// identities. The current Order is an existing fixed-size native object;
	// publishing its destination cannot create a second work assignment.
	if (source_hangar) {
		[[maybe_unused]] const DepotID source_id = RegisterTBMHangarDepot(*front, source_hangar->tile);
		assert(source_id != DepotID::Invalid());
	}
	const DepotID depot = RegisterTBMHangarDepot(*front, hangar.tile);
	assert(depot != DepotID::Invalid());
	FreeTrainTrackReservation(front);
	if (front->current_order.IsType(OT_LOADING)) front->LeaveStation(); // A travel interruption must finish native station handling before replacing its order.
	front->current_order.MakeGoToDepot(depot, {}, OrderNonStopFlag::NonStop, service ? OrderDepotActionFlags{} : OrderDepotActionFlag::Halt);
	front->SetDestTile(hangar.tile);
	front->vehstatus.Reset(VehState::Stopped);
	front->flags.Reset(VehicleRailFlag::Stuck);
	if (project != nullptr && !IsTBMProjectFinished(project->phase) && (!service || IsTBMHangarTransfer(project->phase))) {
		[[maybe_unused]] const bool adopted = BeginTBMHangarReturn(*front);
		assert(adopted);
	}
	// An empty cutter follows the real two-way native route search. Loaded
	// machinery has already selected its physical route to the working bay.
	if (!IsTBMStagingHangarOrder(*front) && !front->IsChainInDepot() && YapfTrainCheckReverse(front)) {
		Command<Commands::ReverseTrainDirection>::Do(DoCommandFlag::Execute, cutter, false);
	}
	InvalidateWindowData(WindowClass::VehicleView, cutter);
	SetWindowClassesDirty(WindowClass::TBMProjects);
	return CommandCost();
}
