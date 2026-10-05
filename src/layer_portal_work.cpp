/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_portal_work.cpp One canonical saved work owner per reciprocal portal pair. @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 2 */

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
	if (job == nullptr || job->company != work->company || job->state == ExcavationJobState::Cancelled || job->state == ExcavationJobState::Completed) return LayerPortalWorkError::InvalidReference; // Removed/closed foreign jobs cannot retain movement authority through a saved reference.
	if (!GetLayerPortalWorkPocket(tile)) return LayerPortalWorkError::InvalidPair;
	return LayerPortalWorkError::ControllerUnavailable;
}

/**
 * Publish the portal-side producer boundary without inventing a TBM purchase/controller fallback.
 * @param request Actual upper pair and canonical geological proposal.
 * @param execute Actual estimate/execute intent; acquisition remains unavailable until its owner exists.
 * @return Typed pair/date/owner/refusal and a real preflight stamp, with no allocated IDs on refusal.
 */
LayerPortalCommissionResult CommissionLayerPortalTBM(const LayerPortalCommissionRequest &request, [[maybe_unused]] bool execute)
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
		if (EnsureNoVehicleOnGround(end).Failed() || HasLayerPortalReservation(end)) { result.error = LayerPortalWorkError::Conflict; return result; } // Both real shaft ends must be free before any project/vehicle capacity acquisition.
	}
	std::vector<TileIndex> assembly;
	// The conservative complete purchase footprint stays on accessible source ground; the restricted rock pocket is not painted open to make this check pass.
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			const auto cell = TileAddWrap(request.portal, x, y);
			if (!IsValidTile(cell)) { result.error = LayerPortalWorkError::InvalidPair; return result; }
			if (cell != request.portal && GetTileType(cell) != TileType::Clear && !IsPlainRailTile(cell)) { result.error = LayerPortalWorkError::Conflict; return result; } // Assembly cannot clear another structure or borrow incompatible infrastructure as hidden purchase space.
			if (IsPlainRailTile(cell) && (GetTileOwner(cell) != work->company.base() || GetRailType(cell) != GetRailType(request.portal))) { result.error = LayerPortalWorkError::Conflict; return result; } // Any rail assembly floor must remain the same company's compatible conventional rail.
			if (EnsureNoVehicleOnGround(cell).Failed()) { result.error = LayerPortalWorkError::Conflict; return result; }
			assembly.push_back(cell);
		}
	}
	if (CheckExcavationConstruction(assembly).Failed()) { result.error = LayerPortalWorkError::MissingExcavation; return result; }
	result.pair_revision = work->revision;
	result.error = LayerPortalWorkError::ControllerUnavailable;
	return result;
}

/**
 * Validate and commit an empty pair state, releasing old reservations before hiding their track.
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
	// Both physical ends and approaches must be free before a cached path/reservation can lose or acquire an edge.
	for (TileIndex end : {*key, pair}) {
		const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(end));
		const auto door = TileAddWrap(end, step.x, step.y);
		if (!IsValidTile(door)) return LayerPortalWorkError::InvalidPair;
		if (EnsureNoVehicleOnGround(end).Failed() || EnsureNoVehicleOnGround(door).Failed()) return LayerPortalWorkError::Conflict; // Any occupying part preserves its current recoverable position and reservation.
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
				if (!IsPlainRailTile(door) || GetTileOwner(door) != company.base() || GetRailType(door) != GetRailType(end) || !GetTrackBits(door).Test(DiagDirToDiagTrack(GetLayerPortalShaftDirection(end)))) return LayerPortalWorkError::Conflict; // Open rock is insufficient: both actual owned rail approaches must supply the compatible shaft axis.
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
	if (!connected && GetLayerPortalTransportType(*key) == TransportType::Rail) {
		// PBS asserts track visibility during release: drain each reserved endpoint while the old Connected state still exposes its axis.
		for (TileIndex end : {*key, pair}) {
			if (HasLayerPortalReservation(end)) UnreserveRailTrack(end, DiagDirToDiagTrack(GetLayerPortalShaftDirection(end)));
		}
	}
	work.state = target_state; ++work.revision;
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
	if (!GetLayerPortalPairKey(tile)) return LayerPortalWorkError::InvalidPair;
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
	_portal_work.erase(*GetLayerPortalPairKey(tile));
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
	const auto error = CheckLayerPortalLifecycle(tile);
	if (error != LayerPortalWorkError::None) return error;
	const auto key = GetLayerPortalPairKey(tile);
	const auto it = _portal_work.find(*key);
	if (it == _portal_work.end()) return LayerPortalWorkError::None;
	if (new_owner != OWNER_NONE && !Company::IsValidID(new_owner)) return LayerPortalWorkError::InvalidOwner; // A transfer cannot bind a record to an invented/deleted company.
	if (new_owner == OWNER_NONE && GetLayerPortalTransportType(tile) != TransportType::Road) return LayerPortalWorkError::InvalidOwner; // Bankruptcy removes rail housings; only existing road policy retains ownerless shafts.
	if (it->second.revision >= UINT64_MAX - 1) return LayerPortalWorkError::Limit;
	it->second.company = new_owner == OWNER_NONE ? CompanyID::Invalid() : CompanyID{new_owner};
	if (new_owner == OWNER_NONE) it->second.state = LayerPortalWorkState::Sealed;
	++it->second.revision;
	return LayerPortalWorkError::None;
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
	ExcavationCommissioningPocket pocket{record->cutter, destination, {}};
	// The actual conservative cutter footprint must fit inside this rock plane; a void boundary cannot become free commissioning space.
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			const auto cell = TileAddWrap(destination, x, y);
			if (!IsValidTile(cell) || GetTileLayer(cell) != GetTileLayer(destination)) return std::nullopt; // Wrapped/void footprint cells cannot grant occupancy in another plane.
			pocket.tiles.push_back(cell);
		}
	}
	return pocket;
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
		if (job == geology.jobs.end() || job->company != record.company || job->state == ExcavationJobState::Cancelled || job->state == ExcavationJobState::Completed || record.cutter == VehicleID::Invalid()) return LayerPortalWorkError::InvalidReference; // Commissioning binds an actual live geological project and train, never surrogate IDs.
		if (job->commissioning_anchor != pair || (job->cutter != VehicleID::Invalid() && job->cutter != record.cutter)) return LayerPortalWorkError::InvalidReference; // Saved work must start at this physical destination and preserve any actual controller binding already owned by geology.
		if (check_references) {
			if (!Vehicle::IsValidID(record.cutter)) return LayerPortalWorkError::InvalidReference;
			const auto *cutter = Vehicle::Get(record.cutter);
			if (cutter->type != VehicleType::Train || cutter->owner != record.company) return LayerPortalWorkError::InvalidReference; // Role/model admission remains with the actual controller; a stored reference alone never makes a TBM.
			if (record.origin != StationID::Invalid()) {
				if (!Station::IsValidID(record.origin)) return LayerPortalWorkError::InvalidReference;
				const auto *station = Station::Get(record.origin);
				if (station->owner != record.company || GetTileLayer(station->xy) != MapLayer::Surface) return LayerPortalWorkError::InvalidReference; // Any explicit exchange reference must resolve to the company's Surface origin.
			}
		}
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
}
