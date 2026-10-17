/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_portal_work.cpp One canonical saved work owner per reciprocal portal pair. @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Groups 2, 5 (task 5.3B: TBM role admission), 8 (commissioning binding), 8B (solid-start load binding) and 11.3/11.4 (withdrawal, merger and company removal of assigned pairs); docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-j-lifecycle-design.md; docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#include "stdafx.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "company_base.h"
#include "company_func.h"
#include "vehicle_base.h"
#include "station_base.h"
#include "debug.h"
#include "vehicle_func.h"
#include "train.h"
#include "train_cmd.h"
#include "aircraft.h"
#include "depot_base.h"
#include "tbm.h"
#include "tbm_hangar.h"
#include "pathfinder/yapf/yapf.h"
#include "tbm_exchange.h"
#include "utility_power.h"
#include "geothermal_riser.h"
#include "transport_tube.h"
#include "track_func.h"
#include "pbs.h"
#include "rail_map.h"
#include "landscape.h"
#include "timer/timer_game_calendar.h"

#include "safeguards.h"

static std::map<TileIndex, LayerPortalWorkRecord> _portal_work; ///< Sole accepted pair authority, independent of map infrastructure storage.
static std::optional<std::map<TileIndex, LayerPortalWorkRecord>> _pending_portal_work; ///< Validated unpublished loader candidate.

/**
 * Allocate only an unpublished actual-owner node; accepted state remains unchanged on allocation/capacity failure.
 * @param key Proposed lower physical endpoint.
 * @param company Actual future housing owner.
 * @return Prepared transferable Sealed node or empty refusal.
 */
LayerPortalWorkReservation PrepareLayerPortalWorkPair(TileIndex key, CompanyID company)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return {}; // Classic owns implicit Connected geometry and must never acquire new-profile saved work records.
	if (!IsValidTile(key) || !Company::IsValidID(company) || _portal_work.size() >= MAX_LAYER_PORTAL_WORK_PAIRS || _portal_work.contains(key)) return {}; // A full/duplicate/invalid future pair cannot mutate either housing before owner capacity is acquired.
	try {
		std::map<TileIndex, LayerPortalWorkRecord> prepared;
		LayerPortalWorkRecord record;
		record.pair_key = key; record.company = company;
		prepared.emplace(key, record);
		return LayerPortalWorkReservation{prepared.extract(key)};
	} catch (const std::bad_alloc &) {
		return {};
	}
}

/** @return Complete nonallocating publication after both actual housings exist, or unchanged failure. */
bool LayerPortalWorkReservation::Publish() noexcept
{
	if (node.empty()) return false;
	const auto &record = node.mapped();
	const auto key = GetLayerPortalPairKey(record.pair_key);
	if (!key || *key != record.pair_key || GetTileOwner(*key) != record.company.base() || _portal_work.contains(*key) || _portal_work.size() >= MAX_LAYER_PORTAL_WORK_PAIRS) return false; // Unexpected topology/ownership/capacity changes cannot publish a ghost work record.
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || !Company::IsValidID(record.company) || !MultiLayerMap::IsPortalPairAllowed(GetLayerPortalKind(*key), GetTileLayer(*key), GetLayerPortalTargetLayer(*key))) return false; // Publication rechecks actual profile/company/pair authority so abandoned preparation cannot publish after policy/owner changes or through unsupported topology.
	return _portal_work.insert(std::move(node)).inserted;
}

/**
 * Resolve exact reciprocal topology before invoking any asserting pair accessor.
 * @param tile Either actual endpoint.
 * @return Canonical existing lower endpoint, or invalid topology absence.
 */
std::optional<TileIndex> GetLayerPortalPairKey(TileIndex tile)
{
	if (!IsValidTile(tile)) return std::nullopt;
	if (!IsLayerPortalTile(tile)) return std::nullopt;
	const auto target = GetLayerPortalTargetLayer(tile);
	if (!MultiLayerMap::IsValidLayer(target) || target == GetTileLayer(tile)) return std::nullopt; // A missing/self-target layer cannot create a second physical endpoint.
	const auto pair = MultiLayerMap::TileInLayer(tile, target);
	if (!IsValidTile(pair)) return std::nullopt;
	if (!IsLayerPortalTile(pair)) return std::nullopt;
	// Both endpoints must agree on transport, shaft, reverse target and actual map ownership before sharing a state key.
	if (GetLayerPortalTargetLayer(pair) != GetTileLayer(tile) || GetLayerPortalTransportType(pair) != GetLayerPortalTransportType(tile) || GetLayerPortalShaftDirection(pair) != ReverseDiagDir(GetLayerPortalShaftDirection(tile)) || GetTileOwner(pair) != GetTileOwner(tile)) return std::nullopt;
	const auto transport = GetLayerPortalTransportType(tile);
	if (transport != TransportType::Rail && transport != TransportType::Road) return std::nullopt; // Unknown transport bits cannot inherit a legacy Connected fallback.
	if (transport == TransportType::Rail && GetRailType(tile) != GetRailType(pair)) return std::nullopt; // A half-converted pair cannot retain one shared usable rail connection.
	if (transport == TransportType::Road && GetRoadTypeRoad(tile) != GetRoadTypeRoad(pair)) return std::nullopt; // A half-converted road shaft cannot inherit the other end's type permission.
	return std::min(tile, pair);
}

/**
 * Read the single accepted record through actual physical topology.
 * @param tile Either endpoint.
 * @return Owner record or absence; references expire at reset/publication.
 */
const LayerPortalWorkRecord *GetLayerPortalWorkRecord(TileIndex tile)
{
	const auto key = GetLayerPortalPairKey(tile);
	if (!key) return nullptr;
	const auto it = _portal_work.find(*key);
	return it == _portal_work.end() ? nullptr : &it->second;
}

/**
 * Preserve explicit legacy routing while refusing missing native work authority safely.
 * @param tile Actual portal endpoint.
 * @return Sole saved phase, legacy Connected, or invalid topology.
 */
LayerPortalWorkState GetLayerPortalWorkState(TileIndex tile)
{
	if (!GetLayerPortalPairKey(tile)) return LayerPortalWorkState::Invalid;
	if (const auto *record = GetLayerPortalWorkRecord(tile)) return record->state;
	return GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::Classic ? LayerPortalWorkState::Connected : LayerPortalWorkState::Sealed;
}

/**
 * Apply the shared calendar authority to both directions without changing geological preview or legacy policy.
 * @param tile Actual existing endpoint.
 * @return Date-valid permission or safe topology/Deep refusal.
 */
LayerPortalWorkError CheckLayerPortalAvailability(TileIndex tile)
{
	if (!GetLayerPortalPairKey(tile)) return LayerPortalWorkError::InvalidPair;
	// Any endpoint reaching Deep uses the same saved exception/date policy; an upper-layer click cannot bypass it.
	if ((GetTileLayer(tile) == MapLayer::DeepUnderground || GetLayerPortalTargetLayer(tile) == MapLayer::DeepUnderground) && !CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return LayerPortalWorkError::LockedDeep;
	return LayerPortalWorkError::None;
}

/**
 * Validate actual assignment without treating a train reference as a shipped TBM capability.
 * @param tile Actual entering endpoint.
 * @param actor Actual entering vehicle part.
 * @param project Expected real geological job identity.
 * @return Stable refusal; role/consist admission remains unavailable until its actual Plan 4 owner exists.
 */
LayerPortalWorkError CheckLayerPortalWorkAccess(TileIndex tile, VehicleID actor, ExcavationJobID project)
{
	const auto availability = CheckLayerPortalAvailability(tile);
	if (availability != LayerPortalWorkError::None) return availability;
	const auto *work = GetLayerPortalWorkRecord(tile);
	if (work == nullptr || work->state != LayerPortalWorkState::Commissioning || GetLayerPortalTransportType(tile) != TransportType::Rail) return LayerPortalWorkError::InvalidPair; // Only an explicitly assigned rail shaft can even request restricted work occupancy.
	if (!Vehicle::IsValidID(actor)) return LayerPortalWorkError::InvalidReference;
	const auto *vehicle = Vehicle::Get(actor);
	if (vehicle->type != VehicleType::Train || vehicle->owner != work->company || vehicle->First()->index != work->cutter || project != work->project) return LayerPortalWorkError::InvalidReference; // Each actual part must belong to the exact assigned consist/project, never another train behind it.
	const auto *job = GetExcavationJob(project);
	// Removed or foreign jobs cannot retain movement authority through a saved reference. A closed job, cancelled or with
	// its last cut done, keeps it while its own project lives, so the cutter backs out to its bay for the final return;
	// the project check below ends it once the project is finished (R2MLVS3MP-M).
	if (job == nullptr || job->company != work->company) return LayerPortalWorkError::InvalidReference;
	if (!GetLayerPortalWorkPocket(tile)) return LayerPortalWorkError::InvalidPair;
	// Movement authority comes only from the TBM controller: a live project must own this exact cutter and job.
	// Without one, the pair names a train that is not (yet) an assigned TBM, so admission stays unavailable.
	const TBMProject *tbm = GetTBMProjectForCutter(work->cutter);
	if (tbm == nullptr || tbm->job != project || tbm->phase == TBMPhase::Completed || tbm->phase == TBMPhase::Cancelled) return LayerPortalWorkError::ControllerUnavailable;
	// Every entering or retreating part needs a role in that consist; a coupled stranger or altered chain has none.
	if (GetTBMConsistRole(*tbm, *Train::From(vehicle)) == TBMConsistRole::Invalid || InspectTBMConsist(*Train::From(vehicle->First()), *tbm).error != TBMBlockedReason::None) return LayerPortalWorkError::InvalidReference;
	return LayerPortalWorkError::None;
}

/** @copydoc GetLayerPortalMagazineExit */
DiagDirection GetLayerPortalMagazineExit(TileIndex tile, const Train &front)
{
	if (!IsLayerPortalTile(tile) || GetLayerPortalTransportType(tile) != TransportType::Rail) return DiagDirection::Invalid;
	const auto key = GetLayerPortalPairKey(tile);
	const auto *work = GetLayerPortalWorkRecord(tile);
	const auto *orders_project = GetTBMProjectForCutter(front.index);
	const bool own_order_departure = orders_project != nullptr && orders_project->phase == TBMPhase::FollowOrders &&
		orders_project->portal == tile && CheckLayerPortalWorkAccess(tile, front.index, orders_project->job) == LayerPortalWorkError::None;
	if (key && *key == tile && (CanTBMUseHangarPortal(front, tile) || own_order_departure) && !IsTBMStagingHangarOrder(front)) {
		const auto *project = GetTBMProjectForCutter(front.index);
		const bool original_work = project != nullptr && project->portal == tile && project->phase != TBMPhase::FollowOrders && !front.current_order.IsType(OT_GOTO_DEPOT);
		if (!original_work) {
			const auto ordered = GetTBMOrderedHangar(front);
			if (ordered.error == TBMHangarError::None && ordered.tile == tile) return ReverseDiagDir(DirToDiagDir(front.GetMovingFront()->GetMovingDirection()));
			if (front.IsChainInDepot()) return YapfTrainChoosePortalExit(&front, tile);
			if (project != nullptr && project->portal == tile && front.vehicle_flags.Test(VehicleFlag::DrivingBackwards)) return GetLayerPortalShaftDirection(tile); // Collect a backing tail before changing the original magazine's exit side.
			// Stored parts retain the native departure orientation until each
			// leaves. The head can already be turning several cells away; using
			// its later heading here would strand a wagon inside this magazine.
			for (const Train *part = &front; part != nullptr; part = part->Next()) {
				if (part->tile == tile && part->track == Track::Depot) return DirToDiagDir(part->direction);
			}
			for (const Train *part = &front; part != nullptr; part = part->Next()) {
				if (part->tile == tile) return DirToDiagDir(part->direction); // Finish the last part's actual upper housing traversal.
			}
			return DiagDirection::Invalid;
		}
	}
	// Only the key of a pair that commissions work keeps a magazine; its rock end is a passage.
	if (!key.has_value() || *key != tile || work == nullptr || work->state != LayerPortalWorkState::Commissioning) return DiagDirection::Invalid;
	if (CheckLayerPortalWorkAccess(tile, front.index, work->project) == LayerPortalWorkError::None) {
		if (front.current_order.IsType(OT_GOTO_DEPOT) && !IsTBMStagingHangarOrder(front)) {
			// Backing from a face may leave trailing units in the work magazine.
			// Collect the complete real chain through the shaft before changing
			// its exit side; mixing an incoming shaft with a door-side release
			// would make a follower inherit another part's depot-only track state.
			if (front.vehicle_flags.Test(VehicleFlag::DrivingBackwards)) return GetLayerPortalShaftDirection(tile);
			// Once withdrawn, ordinary magazine departure releases the stored
			// whole chain through the outer door for the actual depot visit.
			for (const Train *part = front.First(); part != nullptr; part = part->Next()) {
				if (part->tile == tile && part->track == Track::Depot) return GetLayerPortalDoorDirection(tile);
			}
			return DiagDirection::Invalid;
		}
		return GetLayerPortalShaftDirection(tile);
	}
	if (IsTBMHaulerAdmittedAtBay(front, tile)) return GetLayerPortalDoorDirection(tile);
	return DiagDirection::Invalid;
}

bool IsLayerPortalMagazineBusy(TileIndex tile, const Train &front)
{
	const Train *own = front.First();
	/**
	 * @param v Vehicle on the tile.
	 * @return Whether it is a part of another train that moves on the housing rather than standing aside inside.
	 */
	return HasVehicleOnTile(tile, [own](const Vehicle *v) {
		return v->type == VehicleType::Train && Train::From(v)->First() != own && Train::From(v)->track != Track::Depot;
	});
}

/** @copydoc GetLayerPortalAdmittedTrackdirs */
TrackdirBits GetLayerPortalAdmittedTrackdirs(TileIndex tile, const Train &front, DiagDirection side)
{
	if (!IsLayerPortalTile(tile) || GetLayerPortalTransportType(tile) != TransportType::Rail) return {};
	// The same side rule as the ordinary track status: an end is reached through its door, or by a shaft step.
	if (side != DiagDirection::Invalid && side != GetLayerPortalDoorDirection(tile)) return {};
	if (CheckLayerPortalAvailability(tile) != LayerPortalWorkError::None) return {};
	const auto *work = GetLayerPortalWorkRecord(tile);
	if (CanTBMUseHangarPortal(front, tile)) {
		if (front.tile != tile && IsLayerPortalMagazineBusy(tile, front)) return {};
		return TrackBitsToTrackdirBits(DiagDirToDiagTrack(GetLayerPortalShaftDirection(tile)));
	}
	if (work == nullptr || work->state != LayerPortalWorkState::Commissioning) return {};
	const bool cutter = CheckLayerPortalWorkAccess(tile, front.index, work->project) == LayerPortalWorkError::None;
	// A hauler reaches only the key of its own Source bay, and only through the door: it never passes the cutter.
	const bool hauler = !cutter && side != DiagDirection::Invalid && IsTBMHaulerAdmittedAtBay(front, tile);
	if (!cutter && !hauler) return {};
	// A source housing occupied by another moving train stays unavailable,
	// including a servicing cutter that passes through instead of parking.
	if (front.tile != tile && work->pair_key == tile && IsLayerPortalMagazineBusy(tile, front)) return {};
	return TrackBitsToTrackdirBits(DiagDirToDiagTrack(GetLayerPortalShaftDirection(tile)));
}

/**
 * Validate the portal side, then hand the purchase to the TBM controller's commissioning transaction.
 * @param request Actual upper pair, canonical geological proposal and wagon counts.
 * @param execute False estimates; true commissions.
 * @return Typed pair/date/owner refusal, ControllerUnavailable for a controller refusal, or None with the allocated
 *         job and cutter after execution; no IDs are allocated on refusal.
 */
LayerPortalCommissionResult CommissionLayerPortalTBM(const LayerPortalCommissionRequest &request, bool execute)
{
	auto result = ValidateLayerPortalCommissioning(request);
	if (result.error != LayerPortalWorkError::None) return result;
	// The purchase belongs to the TBM controller (r2mlvs4 Group 2.1); a refusal there has allocated nothing.
	const auto commissioned = CommissionTBMProject(request, execute);
	if (commissioned.error != TBMAcquisitionError::None) {
		result.error = commissioned.error == TBMAcquisitionError::Portal ? commissioned.portal_error : LayerPortalWorkError::ControllerUnavailable;
		return result;
	}
	result.project = commissioned.job;
	result.cutter = commissioned.cutter;
	return result;
}

/**
 * Check ordinary ground occupancy, exempting only a prevalidated existing source consist.
 * @param tile Actual source/assembly cell; destination cells never receive a reuse exemption.
 * @param reused Exact healthy, empty withdrawn native train, or nullptr for ordinary fresh-site validation.
 * @return True when no unrelated ground vehicle occupies the construction footprint.
 */
static bool IsCommissioningGroundFree(TileIndex tile, const Train *reused)
{
	if (reused == nullptr) return EnsureNoVehicleOnGround(tile).Succeeded();
	const int z = GetTileMaxPixelZ(tile);
	// Match the native ground-occupancy exclusions. Only the exact eligible retired consist may remain in its
	// source magazine; all other physical vehicles retain the same construction protection as fresh purchases.
	for (const Vehicle *vehicle : VehiclesOnTile(tile)) {
		// Disaster entities and aircraft shadows are excluded by the ordinary owner; airborne vehicles do not occupy ground.
		if (vehicle->type == VehicleType::Disaster || (vehicle->type == VehicleType::Aircraft && vehicle->subtype == AIR_SHADOW) || vehicle->z_pos > z) continue;
		// The exemption belongs to one actual train chain, not all trains of its company or every vehicle on this tile.
		if (vehicle->type == VehicleType::Train && vehicle->First() == reused) continue;
		return false;
	}
	return true;
}

/** @copydoc ValidateLayerPortalCommissioning */
LayerPortalCommissionResult ValidateLayerPortalCommissioning(const LayerPortalCommissionRequest &request)
{
	LayerPortalCommissionResult result;
	const auto key = GetLayerPortalPairKey(request.portal);
	if (!key) return result;
	result.pair_key = *key;
	result.error = CheckLayerPortalAvailability(request.portal);
	if (result.error != LayerPortalWorkError::None) return result;
	const auto *work = GetLayerPortalWorkRecord(request.portal);
	if (work == nullptr || GetLayerPortalTransportType(request.portal) != TransportType::Rail || request.portal != *key) { result.error = LayerPortalWorkError::InvalidPair; return result; } // Commissioning originates at the actual accessible upper rail end, never a road/elevator or rock-side purchase bypass.
	if (!Company::IsValidID(request.proposal.company) || request.proposal.company != _current_company || request.proposal.company != work->company) { result.error = LayerPortalWorkError::InvalidOwner; return result; } // The real command actor and both owned housings must agree before project capacity is acquired.
	if (work->state != LayerPortalWorkState::Sealed || work->project != ExcavationJobID{UINT32_MAX} || work->cutter != VehicleID::Invalid()) { result.error = LayerPortalWorkError::Conflict; return result; } // Ordinary Connected traffic and already assigned work cannot share a new commissioning purchase.
	const Train *reused = nullptr;
	if (request.reuse_cutter != VehicleID::Invalid()) {
		reused = Train::GetIfValid(request.reuse_cutter);
		// Only the existing owner can reassign a completely withdrawn, empty native machine; stale, ordinary,
		// foreign, crashed, loaded or still-assigned trains cannot borrow a source-occupancy exemption.
		if (reused == nullptr || !IsTBMConsistReusableAtPortal(*reused, request.portal, work->company, request.proposal.model_id)) {
			result.error = LayerPortalWorkError::InvalidReference;
			return result;
		}
	}
	const auto destination = GetLayerPortalPairTile(request.portal);
	const auto models = GetMultiLayerVehicleCapabilities();
	const auto model = std::ranges::find(models, request.proposal.model_id, &LayerVehicleCapability::model_id);
	// Only the catalogue's portal-commissioned conventional rail designs may assemble here; tube and freight entries cannot inherit a cutter role.
	if (model == models.end() || model->commissioning != MultiLayerCommissioningFacility::RailPortalBay || model->transport != MultiLayerVehicleTransport::Rail || !model->conventional_portals || !model->supported_layers.Test(GetTileLayer(request.portal)) || !model->supported_layers.Test(GetTileLayer(destination))) { result.error = LayerPortalWorkError::InvalidReference; return result; }
	if (TimerGameCalendar::date < TimerGameCalendar::ConvertYMDToDate(model->introduction_year, 0, 1)) { result.error = LayerPortalWorkError::LockedDeep; return result; }
	// The first-party TBMs require conventional rail/electrified rail; future tube mappings cannot become eligible merely by sharing track bits.
	if (GetRailType(request.portal) != RAILTYPE_RAIL && GetRailType(request.portal) != RAILTYPE_ELECTRIC) { result.error = LayerPortalWorkError::InvalidReference; return result; }
	if (request.proposal.commissioning_anchor != destination || request.proposal.id != ExcavationJobID{UINT32_MAX} || request.proposal.cutter != VehicleID::Invalid()) { result.error = LayerPortalWorkError::InvalidReference; return result; } // Assembly must start at this real rock destination and cannot reuse an allocated foreign job/cutter.
	if (!request.proposal.pass_terms || request.proposal.pass_terms->earth_capacity_t == 0 || request.proposal.pass_terms->rock_capacity_t == 0) { result.error = LayerPortalWorkError::InvalidReference; return result; } // Separate positive Earth and Rock capacity is mandatory before the actual controller validates attached wagons.
	if (!Station::IsValidID(request.proposal.origin)) { result.error = LayerPortalWorkError::InvalidReference; return result; }
	const auto *origin = Station::Get(request.proposal.origin);
	if (origin->owner != work->company || !IsValidTile(origin->xy) || GetTileLayer(origin->xy) != MapLayer::Surface || !origin->facilities.Test(StationFacility::Train)) { result.error = LayerPortalWorkError::InvalidReference; return result; } // Pre-bay exchange requires an actual company-owned Surface railway origin, never a coincident underground station or invented reference.
	for (TileIndex end : {request.portal, destination}) {
		if (!IsCommissioningGroundFree(end, end == request.portal ? reused : nullptr) || HasLayerPortalReservation(end)) { result.error = LayerPortalWorkError::Conflict; return result; } // Both shafts stay free/resolved; only the exact prevalidated existing source consist may remain withdrawn for reassignment.
	}
	std::vector<TileIndex> assembly;
	// The conservative complete purchase footprint stays on accessible source ground; the restricted rock pocket is not painted open to make this check pass.
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			const auto cell = TileAddWrap(request.portal, x, y);
			if (!IsValidTile(cell)) { result.error = LayerPortalWorkError::InvalidPair; return result; }
			if (cell != request.portal && GetTileType(cell) != TileType::Clear && !IsPlainRailTile(cell)) { result.error = LayerPortalWorkError::Conflict; return result; } // Assembly cannot clear another structure or borrow incompatible infrastructure as hidden purchase space.
			if (IsPlainRailTile(cell) && (GetTileOwner(cell) != work->company.base() || GetRailType(cell) != GetRailType(request.portal))) { result.error = LayerPortalWorkError::Conflict; return result; } // Any rail assembly floor must remain the same company's compatible conventional rail.
			if (!IsCommissioningGroundFree(cell, reused)) { result.error = LayerPortalWorkError::Conflict; return result; }
			assembly.push_back(cell);
		}
	}
	if (CheckExcavationConstruction(assembly).Failed()) { result.error = LayerPortalWorkError::MissingExcavation; return result; }
	result.pair_revision = work->revision;
	result.error = LayerPortalWorkError::None;
	return result;
}

/**
 * Check a portal's public track while retaining whole trains in its existing native magazine.
 * @param tile Physical endpoint, with ground-level occupancy only.
 * @param company Owner of the requested connection.
 * @return Whether the through-route is empty; stored trains remain protected from demolition elsewhere.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group7B
 */
static bool IsPortalThroughTrackClear(TileIndex tile, CompanyID company)
{
	const int height = GetTileMaxPixelZ(tile);
	const auto key = GetLayerPortalPairKey(tile);
	for (const Vehicle *vehicle : VehiclesOnTile(tile)) {
		// Match ordinary ground occupancy; an overhead aircraft or disaster
		// graphic never occupies this track. Only the upper rail housing has
		// the existing depot-style magazine alongside its through passage.
		if (vehicle->type == VehicleType::Disaster || (vehicle->type == VehicleType::Aircraft && vehicle->subtype == AIR_SHADOW) || vehicle->z_pos > height) continue;
		if (vehicle->type != VehicleType::Train || !key || *key != tile || GetLayerPortalTransportType(tile) != TransportType::Rail) return false;
		const Train *front = Train::From(vehicle)->First();
		if (front->owner != company || front->vehstatus.Test(VehState::Crashed) || !IsTrainStoppedAtPortalSite(*front)) return false;
		if (GetTBMProjectForCutter(front->index) != nullptr || GetTBMExchangeAssignmentForHauler(front->index) != nullptr) return false; // Active work and logistics must settle through their owners before public connection.
	}
	return true;
}

/**
 * Validate and commit a pair with vacated public track, releasing old reservations before hiding its edge.
 * @param tile Either actual physical endpoint.
 * @param company Actual command company.
 * @param connected Whether ordinary traffic should be admitted.
 * @param execute Pure check or immediate revalidated publication.
 * @return Complete unchanged refusal or one accepted phase transition.
 */
LayerPortalWorkError ChangeLayerPortalConnection(TileIndex tile, CompanyID company, bool connected, bool execute)
{
	const auto key = GetLayerPortalPairKey(tile);
	if (!key) return LayerPortalWorkError::InvalidPair;
	const auto it = _portal_work.find(*key);
	if (it == _portal_work.end()) return LayerPortalWorkError::InvalidPair;
	auto &work = it->second;
	if (!Company::IsValidID(company) || work.company != company || GetTileOwner(*key) != company.base()) return LayerPortalWorkError::InvalidOwner; // A stale/foreign actor cannot change shared pair connectivity.
	if (work.state == LayerPortalWorkState::Commissioning || work.project != ExcavationJobID{UINT32_MAX} || work.cutter != VehicleID::Invalid()) return LayerPortalWorkError::Conflict; // Assigned work must withdraw/cancel through its actual controller before ordinary topology changes.
	const auto pair = GetLayerPortalPairTile(*key);
	const auto availability = CheckLayerPortalAvailability(tile);
	if (availability != LayerPortalWorkError::None) return availability;
	// Both public endpoints and approaches must be vacated before changing a
	// routing edge. Whole withdrawn magazine trains retain their native storage.
	for (TileIndex end : {*key, pair}) {
		const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(end));
		const auto door = TileAddWrap(end, step.x, step.y);
		if (!IsValidTile(door)) return LayerPortalWorkError::InvalidPair;
		if (!IsPortalThroughTrackClear(end, company) || EnsureNoVehicleOnGround(door).Failed()) return LayerPortalWorkError::Conflict; // A moving head or any outside tail still prevents topology changes.
		if (connected && CheckExcavationConstruction(std::array{end, door}).Failed()) return LayerPortalWorkError::MissingExcavation; // Ordinary connection requires real open floor at both doors; shaft housing grants no clearing credit.

		if (GetGeologyWorldState().job_claims.contains(end) || GetGeologyWorldState().job_claims.contains(door)) return LayerPortalWorkError::Conflict; // No ordinary connection may steal an actual active geological work claim.
	}
	// Validate both geological floors before track compatibility so a sealed far face reports the required excavation, independently of source-road completion.
	for (TileIndex end : {*key, pair}) {
		const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(end));
		const auto door = TileAddWrap(end, step.x, step.y);
		if (connected) {
			const auto transport = GetLayerPortalTransportType(end);
			if (transport == TransportType::Rail) {
				// A legal curve immediately outside the door is a real approach,
				// just as for native train travel. Require a piece that joins the
				// door, rather than an unrelated or necessarily straight track.
				if (!IsPlainRailTile(door) || GetTileOwner(door) != company.base() || GetRailType(door) != GetRailType(end) ||
						(GetTrackBits(door) & DiagdirReachesTracks(GetLayerPortalDoorDirection(end))).None()) return LayerPortalWorkError::Conflict;
				if (work.state != LayerPortalWorkState::Connected && HasLayerPortalReservation(end)) return LayerPortalWorkError::Conflict; // Hidden/work reservations must be resolved by their controller before an ordinary edge appears.
			} else {
				if (!IsNormalRoadTile(door) || GetRoadOwner(door, RoadTramType::Road) != company.base() || GetRoadTypeRoad(door) != GetRoadTypeRoad(end) || GetRoadBits(door, RoadTramType::Road) != AxisToRoadBits(DiagDirToAxis(GetLayerPortalShaftDirection(end)))) return LayerPortalWorkError::Conflict; // Elevators/tunnels connect only actual same-owner road approaches, never an unexcavated or tube corridor.
			}
		}
	}
	const auto target_state = connected ? LayerPortalWorkState::Connected : LayerPortalWorkState::Sealed;
	if (work.state == target_state) return LayerPortalWorkError::None;
	if (work.revision >= UINT64_MAX - 1) return LayerPortalWorkError::Limit;
	if (!execute) return LayerPortalWorkError::None;
	// Sealing an admitted physical pair expires attached unused electrical
	// imports before the old Connected revision loses routing authority.
	if (!connected) {
		InvalidateTubePowerForPortal(*key);
		InvalidateUtilityPowerLinksForPortal(*key);
	}
	if (!connected && GetLayerPortalTransportType(*key) == TransportType::Rail) {
		// PBS asserts track visibility during release: drain each reserved endpoint while the old Connected state still exposes its axis.
		for (TileIndex end : {*key, pair}) {
			if (HasLayerPortalReservation(end)) UnreserveRailTrack(end, DiagDirToDiagTrack(GetLayerPortalShaftDirection(end)));
		}
	}
	const auto previous_revision = work.revision;
	work.state = target_state; ++work.revision;
	RevalidateGeothermalRiserPortal(tile, company, previous_revision); // The same actual pair may seal/reopen without repurchasing machinery or restoring expired utility grants.
	NotifyLayerPortalRoutingChanged(*key);
	return LayerPortalWorkError::None;
}

/**
 * Protect actual assignments/parts before removal, conversion or ownership mutation.
 * @param tile Either physical endpoint.
 * @return Safe empty-pair permission or unchanged refusal; the unavailable controller must recover assigned work first.
 */
LayerPortalWorkError CheckLayerPortalLifecycle(TileIndex tile)
{
	const auto key = GetLayerPortalPairKey(tile);
	if (!key) return LayerPortalWorkError::InvalidPair;
	if (IsLayerPortalHangarTile(*key)) {
		const Depot *depot = Depot::GetIfValid(GetLayerPortalHangarDepotIndex(*key));
		if (depot == nullptr || depot->xy != *key) return LayerPortalWorkError::InvalidReference;
	}
	if (GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::Classic) return LayerPortalWorkError::None;
	const auto *work = GetLayerPortalWorkRecord(tile);
	if (work == nullptr) return LayerPortalWorkError::InvalidReference;
	if (work->state == LayerPortalWorkState::Commissioning || work->project != ExcavationJobID{UINT32_MAX} || work->cutter != VehicleID::Invalid() || work->origin != StationID::Invalid()) return LayerPortalWorkError::Conflict; // Assigned work remains recoverable until the actual controller stops, withdraws and cancels it.
	// Either endpoint, door, active geological claim or ordinary reservation can protect a recoverable actual train path.
	for (TileIndex end : {tile, GetLayerPortalPairTile(tile)}) {
		const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(end));
		const auto door = TileAddWrap(end, step.x, step.y);
		if (!IsValidTile(door)) return LayerPortalWorkError::InvalidPair;
		if (EnsureNoVehicleOnGround(end).Failed() || EnsureNoVehicleOnGround(door).Failed() || GetGeologyWorldState().job_claims.contains(end) || GetGeologyWorldState().job_claims.contains(door)) return LayerPortalWorkError::Conflict; // No lifecycle command may erase a part's support or steal unfinished work.
		if (GetLayerPortalTransportType(end) == TransportType::Rail && HasLayerPortalReservation(end)) return LayerPortalWorkError::Conflict; // Reserved ordinary paths remain intact until an explicit safe release/withdrawal.
	}
	return LayerPortalWorkError::None;
}

/**
 * Remove saved authority only while its empty actual housings still resolve.
 * @param tile One validated endpoint.
 * @return Nonallocating retirement or unchanged lifecycle refusal.
 */
LayerPortalWorkError RetireLayerPortalWorkPair(TileIndex tile)
{
	const auto error = CheckLayerPortalLifecycle(tile);
	if (error != LayerPortalWorkError::None) return error;
	// Actual demolition retires metered routes while physical endpoint
	// identity still resolves; consumed electricity is never refunded.
	InvalidateUtilityPowerLinksForPortal(tile);
	InvalidateTubePowerForPortal(tile);
	RetireGeothermalRiserPortal(tile); // Original-housing loss is permanent even if a rebuilt pair reuses the same coordinates and numeric stamp.
	const TileIndex key = *GetLayerPortalPairKey(tile);
	if (IsLayerPortalHangarTile(key)) {
		// Retire the genuine depot while its physical marker still resolves,
		// so native destruction invalidates every ordinary order and backup.
		delete Depot::Get(GetLayerPortalHangarDepotIndex(key));
		SetLayerPortalHangarDepot(key, DepotID::Invalid());
	}
	_portal_work.erase(key);
	return LayerPortalWorkError::None;
}

/**
 * Update saved empty-pair ownership before its physical owner publishes both map ends.
 * @param tile One actual endpoint.
 * @param new_owner Actual receiving company or explicit ownerless-road destination.
 * @return Nonallocating accepted owner update or unchanged refusal.
 */
LayerPortalWorkError TransferLayerPortalWorkPair(TileIndex tile, Owner new_owner)
{
	// Ownerless retention needs a safe empty pair; a receiving company takes the pair with everything bound to it.
	if (new_owner == OWNER_NONE) {
		const auto error = CheckLayerPortalLifecycle(tile);
		if (error != LayerPortalWorkError::None) return error;
	}
	const auto key = GetLayerPortalPairKey(tile);
	if (!key) return LayerPortalWorkError::InvalidPair;
	const auto it = _portal_work.find(*key);
	if (it == _portal_work.end()) return LayerPortalWorkError::None;
	if (new_owner != OWNER_NONE && !Company::IsValidID(new_owner)) return LayerPortalWorkError::InvalidOwner; // A transfer cannot bind a record to an invented/deleted company.
	if (new_owner == OWNER_NONE && GetLayerPortalTransportType(tile) != TransportType::Road) return LayerPortalWorkError::InvalidOwner; // Bankruptcy removes rail housings; only existing road policy retains ownerless shafts.
	if (it->second.revision >= UINT64_MAX - 1) return LayerPortalWorkError::Limit;
	const auto previous_owner = it->second.company;
	const auto previous_revision = it->second.revision;
	it->second.company = new_owner == OWNER_NONE ? CompanyID::Invalid() : CompanyID{new_owner};
	if (new_owner == OWNER_NONE) it->second.state = LayerPortalWorkState::Sealed;
	++it->second.revision;
	RevalidateGeothermalRiserPortal(tile, previous_owner, previous_revision); // Native takeover keeps paid stock/work on this same physical pair; removed or stale attachments remain retired.
	return LayerPortalWorkError::None;
}

/**
 * Clear a record's assignment, returning it to Sealed, and tell the routing owner.
 * @param key Pair key.
 * @param record Record to clear.
 */
static void UnbindLayerPortalWork(TileIndex key, LayerPortalWorkRecord &record)
{
	record.state = LayerPortalWorkState::Sealed;
	record.project = ExcavationJobID{UINT32_MAX};
	record.cutter = VehicleID::Invalid();
	record.origin = StationID::Invalid();
	++record.revision;
	NotifyLayerPortalRoutingChanged(key);
}

/** @copydoc CheckLayerPortalWorkWithdrawal */
LayerPortalWorkError CheckLayerPortalWorkWithdrawal(TileIndex tile, ExcavationJobID job, VehicleID cutter, bool after_cancel)
{
	const auto key = GetLayerPortalPairKey(tile);
	if (!key) return LayerPortalWorkError::InvalidPair;
	const auto it = _portal_work.find(*key);
	// Only the pair Commissioning exactly this job and cutter is released, and only by that job's controller.
	if (it == _portal_work.end() || it->second.state != LayerPortalWorkState::Commissioning || it->second.project != job || it->second.cutter != cutter) return LayerPortalWorkError::Conflict;
	const auto &geology = GetGeologyWorldState();
	const auto found = std::ranges::find(geology.jobs, job, &ExcavationJob::id);
	// The work is over: a retired, completed or cancelled job no longer needs the pocket.
	if (found != geology.jobs.end() && found->state != ExcavationJobState::Completed && found->state != ExcavationJobState::Cancelled) {
		if (!after_cancel || CancelExcavationJob(job, false) != ExcavationError::None) return LayerPortalWorkError::Conflict; // A prepared cancellation must be admitted by the sole geological owner.
	}
	const TileIndex rock = GetLayerPortalPairTile(*key);
	const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(rock));
	const TileIndex door = TileAddWrap(rock, step.x, step.y);
	if (!IsValidTile(door)) return LayerPortalWorkError::InvalidPair;
	// The rock side is empty, part and claim alike; the withdrawn consist may still stand in its Surface assembly state.
	for (TileIndex cell : {rock, door}) {
		if (EnsureNoVehicleOnGround(cell).Failed()) return LayerPortalWorkError::Conflict;
		const auto claim = geology.job_claims.find(cell);
		if (claim != geology.job_claims.end() && (!after_cancel || claim->second != job)) return LayerPortalWorkError::Conflict; // Only this operation's own cancelled claims may disappear; another project's claim is never released.
	}
	if (it->second.revision >= UINT64_MAX - 1) return LayerPortalWorkError::Limit;
	return LayerPortalWorkError::None;
}

/** @copydoc WithdrawLayerPortalWorkPair */
LayerPortalWorkError WithdrawLayerPortalWorkPair(TileIndex tile, ExcavationJobID job, VehicleID cutter, bool execute)
{
	const auto check = CheckLayerPortalWorkWithdrawal(tile, job, cutter, false);
	if (check != LayerPortalWorkError::None) return check;
	if (!execute) return LayerPortalWorkError::None;
	const auto key = GetLayerPortalPairKey(tile);
	UnbindLayerPortalWork(*key, _portal_work.at(*key));
	return LayerPortalWorkError::None;
}

void ForgetLayerPortalWorkCutter(TileIndex tile, VehicleID cutter)
{
	const auto key = GetLayerPortalPairKey(tile);
	if (!key) return;
	const auto it = _portal_work.find(*key);
	if (it == _portal_work.end() || it->second.state != LayerPortalWorkState::Commissioning || it->second.cutter != cutter || it->second.revision >= UINT64_MAX - 1) return;
	it->second.cutter = VehicleID::Invalid();
	++it->second.revision;
	NotifyLayerPortalRoutingChanged(*key);
}

void ForgetLayerPortalWorkOrigin(StationID station)
{
	// Only a Commissioning pair names an origin; every pair naming this one forgets it.
	for (auto &[key, record] : _portal_work) {
		if (record.origin != station || record.revision >= UINT64_MAX - 1) continue;
		record.origin = StationID::Invalid();
		++record.revision;
	}
}

LayerPortalWorkError BindLayerPortalWorkCommissioning(TileIndex tile, uint64_t pair_revision, ExcavationJobID job, VehicleID cutter, StationID origin, bool execute)
{
	const auto key = GetLayerPortalPairKey(tile);
	if (!key) return LayerPortalWorkError::InvalidPair;
	const auto it = _portal_work.find(*key);
	if (it == _portal_work.end()) return LayerPortalWorkError::InvalidPair;
	auto &record = it->second;
	// Only the unassigned Sealed pair the preflight saw is bound; any change since then refuses the whole purchase.
	if (record.state != LayerPortalWorkState::Sealed || record.revision != pair_revision || record.project != ExcavationJobID{UINT32_MAX} || record.cutter != VehicleID::Invalid() || record.origin != StationID::Invalid()) return LayerPortalWorkError::Conflict;
	if (record.revision >= UINT64_MAX - 1) return LayerPortalWorkError::Limit;
	if (!execute) return LayerPortalWorkError::None;
	record.state = LayerPortalWorkState::Commissioning;
	record.project = job;
	record.cutter = cutter;
	record.origin = origin;
	++record.revision;
	NotifyLayerPortalRoutingChanged(*key);
	return LayerPortalWorkError::None;
}

void ReleaseLayerPortalWorkBinding(TileIndex tile)
{
	const auto key = GetLayerPortalPairKey(tile);
	if (!key) return;
	const auto it = _portal_work.find(*key);
	if (it == _portal_work.end() || it->second.state != LayerPortalWorkState::Commissioning || it->second.revision >= UINT64_MAX - 1) return;
	UnbindLayerPortalWork(*key, it->second);
}

/**
 * Derive the restricted destination footprint from saved actual assignment and physical pair geometry.
 * @param tile Either accepted endpoint.
 * @return Assigned 3x3 occupancy pocket; no packed terrain or inventory is modified.
 */
std::optional<ExcavationCommissioningPocket> GetLayerPortalWorkPocket(TileIndex tile)
{
	const auto *record = GetLayerPortalWorkRecord(tile);
	if (record == nullptr || record->state != LayerPortalWorkState::Commissioning || record->cutter == VehicleID::Invalid()) return std::nullopt; // A sealed/connected/unassigned pair cannot grant cutter-only rock occupancy.
	const auto destination = GetLayerPortalPairTile(record->pair_key);
	// The shared conservative cutter envelope must fit inside this rock plane; a void boundary cannot become free commissioning space.
	auto tiles = GetExcavationCutterEnvelope(destination);
	if (tiles.empty()) return std::nullopt;
	return ExcavationCommissioningPocket{record->cutter, destination, std::move(tiles)};
}

/** @return Canonical actual records without duplicating state at either endpoint. */
std::vector<LayerPortalWorkRecord> CaptureLayerPortalWorkRecords()
{
	std::vector<LayerPortalWorkRecord> records;
	records.reserve(_portal_work.size());
	for (const auto &[key, record] : _portal_work) records.push_back(record);
	return records;
}

/** @return Actual owner count without allocation, suitable for readonly command capacity preflight. */
size_t GetLayerPortalWorkPairCount() noexcept
{
	return _portal_work.size();
}

/** @return Explicit portable pair-state digest; compiler padding/cache order and derived pockets cannot alter it. */
uint64_t ComputeLayerPortalWorkDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/**
	 * Fold one actual fixed-width field into the canonical saved-state hash.
	 * @param value Actual unsigned scalar/reference payload.
	 * @param width Explicit portable byte width.
	 */
	const auto add = [&hash](uint64_t value, uint width) {
		for (uint i = width; i != 0; --i) {
			hash ^= (value >> ((i - 1) * 8)) & 255;
			hash *= 1099511628211ULL;
		}
	};
	add(0x504F5254414C3031ULL, 8);
	add(_portal_work.size(), 4);
	// Exact actual keys/phase/owner/references/stamps define saved work; map geometry remains in its own normal tile digest.
	for (const auto &[key, record] : _portal_work) {
		add(key.base(), 4); add(to_underlying(record.state), 1); add(record.company.base(), 1);
		add(record.project.base(), 4); add(record.cutter.base(), 4); add(record.origin.base(), 2); add(record.revision, 8);
	}
	return hash;
}

/** Reset accepted/unpublished state when the real world owner initializes. */
void ResetLayerPortalWorkState()
{
	_portal_work.clear();
	_pending_portal_work.reset();
	ResetLayerPortalWorkLoadStaging();
}

/**
 * Validate complete copied authority before publication, using actual map and geological references.
 * @param records Strictly ordered bounded transport.
 * @param profile Saved explicit gameplay policy.
 * @param geology Validated staged geological authority.
 * @param check_references Resolve real loaded company/train/station pools.
 * @return Full unchanged validation or stable refusal.
 */
LayerPortalWorkError ValidateLayerPortalWorkRecords(std::span<const LayerPortalWorkRecord> records, const MultiLayerProfileSelection &profile, const GeologyWorldState &geology, bool check_references)
{
	if (records.size() > MAX_LAYER_PORTAL_WORK_PAIRS) return LayerPortalWorkError::Limit;
	if (!records.empty() && profile.id != MultiLayerGameplayProfileID::MultilayerIndustry) return LayerPortalWorkError::InvalidReference; // Classic stores implicit connected topology rather than acquiring new-profile project authority.
	/** @param anchor Loaded initial work centre. @return Whether its complete pocket still needs the issuing portal's restricted paid start. */
	const auto needs_starter = [&geology](TileIndex anchor) {
		const auto state = geology.terrain.Get(anchor);
		if (!state) return false;
		if (*state != ExcavationState::Excavated) return true;
		if (!IsLayerPortalTile(anchor)) return false;
		const auto pocket = GetExcavationCutterEnvelope(anchor);
		/** @param tile One staged pocket cell. @return Whether this cell still depends on the first paid cut. */
		const auto unopened = [&geology](TileIndex tile) { return geology.terrain.Get(tile) != ExcavationState::Excavated; };
		return pocket.empty() || std::ranges::any_of(pocket, unopened);
	};
	TileIndex previous = INVALID_TILE;
	// Every typed saved key must resolve to exactly one existing reciprocal pair, without duplicate identity or hidden owner repair.
	for (const auto &record : records) {
		const auto key = GetLayerPortalPairKey(record.pair_key);
		if (!key || *key != record.pair_key || (previous != INVALID_TILE && record.pair_key <= previous)) return LayerPortalWorkError::InvalidPair;
		previous = record.pair_key;
		const auto pair = GetLayerPortalPairTile(record.pair_key);
		if (!MultiLayerMap::IsPortalPairAllowed(GetLayerPortalKind(record.pair_key), GetTileLayer(record.pair_key), GetTileLayer(pair), profile.id)) return LayerPortalWorkError::InvalidPair;
		if (to_underlying(record.state) > to_underlying(LayerPortalWorkState::Connected) || record.revision == 0 || record.revision == UINT64_MAX) return LayerPortalWorkError::InvalidReference; // Unknown phases or unusable stamps cannot authorize a replayed transition.
		const bool ownerless_road = record.company == CompanyID::Invalid() && GetTileOwner(record.pair_key) == OWNER_NONE && GetLayerPortalTransportType(record.pair_key) == TransportType::Road && record.state == LayerPortalWorkState::Sealed; // Bankruptcy retains only empty sealed road housing, with no invented company identity or traffic permission.
		if (!ownerless_road && record.company.base() != GetTileOwner(record.pair_key)) return LayerPortalWorkError::InvalidOwner; // Ordinary records must still match both actual map owners.
		if (check_references && !ownerless_road && !Company::IsValidID(record.company)) return LayerPortalWorkError::InvalidOwner; // Assigned/Connected native authority must resolve to a real loaded company.
		if (record.state != LayerPortalWorkState::Commissioning) {
			if (record.project != ExcavationJobID{UINT32_MAX} || record.cutter != VehicleID::Invalid() || record.origin != StationID::Invalid()) return LayerPortalWorkError::InvalidReference; // Only commissioning owns a live project; empty/connected pairs cannot conceal actor/cargo bindings.
			if (record.state == LayerPortalWorkState::Connected) {
				auto terrain_policy = profile;
				terrain_policy.editor_override = true; // Saved phase validation checks physical excavation; the actual saved date remains an independent operation gate.
				const std::array source_floor{record.pair_key, GetLayerPortalDoorTile(record.pair_key)};
				const std::array target_floor{pair, GetLayerPortalDoorTile(pair)};
				if (CheckExcavatedFootprint(geology, terrain_policy, TimerGameCalendar::date, source_floor).error != ExcavationError::None || CheckExcavatedFootprint(geology, terrain_policy, TimerGameCalendar::date, target_floor).error != ExcavationError::None) return LayerPortalWorkError::MissingExcavation; // A saved Connected edge cannot invent an excavated destination or erase a blocked approach.
			}
			continue;
		}
		if (GetLayerPortalTransportType(record.pair_key) != TransportType::Rail) return LayerPortalWorkError::InvalidReference;
		const auto job = std::ranges::find(geology.jobs, record.project, &ExcavationJob::id);
		// Commissioning binds an actual geological job, never a surrogate ID. A closed (completed or cancelled) job still
		// binds its cutter while the consist returns, until the pair is withdrawn (r2mlvs4 Group 17); a deleted cutter
		// leaves the binding without one (r2mlvs3 Group 11.4), never a dangling or reused vehicle ID.
		if (job == geology.jobs.end() || job->company != record.company) return LayerPortalWorkError::InvalidReference;
		if (job->commissioning_anchor != pair || (job->cutter != VehicleID::Invalid() && job->cutter != record.cutter)) return LayerPortalWorkError::InvalidReference; // Saved work must start at this physical destination and preserve any actual controller binding already owned by geology.
		// A commissioned job that has not yet opened its housing must still leave through the door and pay for the
		// whole pocket in its first cut; a tampered first segment could otherwise skip unpaid starter rock.
		if (job->cursor == 0 && needs_starter(pair)) {
			const auto envelope = GetExcavationCutterEnvelope(pair);
			if (envelope.empty() || CheckExcavationStarterGeometry(*job, DiagDirToDir(GetLayerPortalDoorDirection(pair)), envelope).error != ExcavationError::None) return LayerPortalWorkError::InvalidReference;
		}
		if (check_references) {
			if (record.cutter != VehicleID::Invalid()) {
				if (!Vehicle::IsValidID(record.cutter)) return LayerPortalWorkError::InvalidReference;
				const auto *cutter = Vehicle::Get(record.cutter);
				if (cutter->type != VehicleType::Train || cutter->owner != record.company) return LayerPortalWorkError::InvalidReference; // Role/model admission remains with the actual controller; a stored reference alone never makes a TBM.
			}
			if (record.origin != StationID::Invalid()) {
				if (!Station::IsValidID(record.origin)) return LayerPortalWorkError::InvalidReference;
				const auto *station = Station::Get(record.origin);
				if (station->owner != record.company || GetTileLayer(station->xy) != MapLayer::Surface) return LayerPortalWorkError::InvalidReference; // Any explicit exchange reference must resolve to the company's Surface origin.
			}
		}
	}
	// Reverse binding includes reused portals with an open centre but solid
	// pocket edges. Their first cut still requires this exact issuer; a stale
	// pair cannot let saved work resume using unsupported surrounding rock.
	for (const auto &job : geology.jobs) {
		if (job.state == ExcavationJobState::Completed || job.state == ExcavationJobState::Cancelled || job.cursor != 0) continue;
		if (!needs_starter(job.commissioning_anchor)) continue;
		const auto key = GetLayerPortalPairKey(job.commissioning_anchor);
		if (!key || *key == job.commissioning_anchor) return LayerPortalWorkError::InvalidReference;
		const auto issuer = std::ranges::find(records, *key, &LayerPortalWorkRecord::pair_key);
		if (issuer == records.end() || issuer->state != LayerPortalWorkState::Commissioning || issuer->project != job.id) return LayerPortalWorkError::InvalidReference;
	}
	return LayerPortalWorkError::None;
}

/**
 * Allocate all loader candidate nodes before any accepted state changes.
 * @param records Complete owner-validated copied transport.
 */
void StageLoadedLayerPortalWork(std::vector<LayerPortalWorkRecord> records)
{
	std::map<TileIndex, LayerPortalWorkRecord> prepared;
	for (const auto &record : records) prepared.emplace(record.pair_key, record);
	_pending_portal_work = std::move(prepared);
}

/**
 * Report actual ordinary-edge usability after loaded profile, geology and work authority are accepted.
 * This load-boundary diagnostic queries the real tile dispatch rather than reproducing
 * portal permission in a fixture. It owns no connectivity cache or saved state.
 */
static void LogLoadedLayerPortalConnectivity()
{
	std::array<uint32_t, 3> pairs{}, usable{};
	uint32_t invalid = 0;
	// Count each actual reciprocal pair once at its canonical lower end; ordinary legacy and Native edges use exactly the same runtime track-status dispatch.
	for (uint32_t index = 0; index < Map::Size(); ++index) {
		const TileIndex tile{index};
		if (!IsLayerPortalTile(tile)) continue;
		const auto key = GetLayerPortalPairKey(tile);
		if (!key) { ++invalid; continue; }
		if (*key != tile) continue;
		const auto pair = GetLayerPortalPairTile(tile);
		const auto kind = GetLayerPortalKind(tile);
		if (!MultiLayerMap::IsPortalPairAllowed(kind, GetTileLayer(tile), GetTileLayer(pair))) { ++invalid; continue; }
		const auto slot = to_underlying(kind);
		++pairs[slot];
		const auto transport = GetLayerPortalTransportType(tile);
		const auto road_kind = transport == TransportType::Road ? RoadTramType::Road : RoadTramType::Invalid;
		// A usable connection needs a real ordinary edge at both housings; a saved physical neighbour alone does not prove phase/date admission.
		if (GetTileTrackStatus(tile, transport, road_kind).trackdirs.Any() && GetTileTrackStatus(pair, transport, road_kind).trackdirs.Any()) ++usable[slot];
	}
	Debug(Facility::Misc, Severity::Info, "MLVS_PORTAL_CONNECTIVITY rail={}/{} road={}/{} elevator={}/{} invalid={}", usable[0], pairs[0], usable[1], pairs[1], usable[2], pairs[2], invalid);
}

/** Publish once only after shared content/geological acceptance. */
void AcceptLoadedLayerPortalWork()
{
	if (!_pending_portal_work) return;
	_portal_work.swap(*_pending_portal_work);
	_pending_portal_work.reset();
	Debug(Facility::Misc, Severity::Info, "MLVS_PORTAL_WORK_LOADED pairs={}", _portal_work.size());
	// Diagnostics name actual accepted phase/reference authority only after profile/geology publication.
	for (const auto &[key, record] : _portal_work) {
		Debug(Facility::Misc, Severity::Info, "MLVS_PORTAL_WORK key={} state={} company={} project={} cutter={} revision={}", key.base(), to_underlying(record.state), record.company.base(), record.project.base(), record.cutter.base(), record.revision);
	}
	LogLoadedLayerPortalConnectivity();
}
