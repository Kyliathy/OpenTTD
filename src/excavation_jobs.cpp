/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_jobs.cpp Canonical job geometry and rebuildable exclusive-cut claims. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Groups 10/10B */

#include "stdafx.h"
#include "excavation.h"
#include "company_base.h"
#include "vehicle_base.h"
#include "station_base.h"

#include "safeguards.h"

/**
 * Check strict tile ordering and explicit rock-layer bounds for saved/proposed geometry.
 * @param tiles Canonical nonempty area or envelope.
 * @param layer Authoritative rock layer for the job.
 * @param limit Maximum permitted cell count.
 * @return Stable first refusal, without reading camera or tile infrastructure meaning.
 */
static ExcavationFootprintResult CheckJobTiles(std::span<const TileIndex> tiles, MapLayer layer, size_t limit)
{
	if (tiles.empty()) return {ExcavationError::InvalidRoute, INVALID_TILE};
	if (tiles.size() > limit) return {ExcavationError::Limit, tiles.front()};
	TileIndex previous = INVALID_TILE;
	// Canonical unique areas ensure claim rebuilding and failure order never depend on input insertion order.
	for (TileIndex tile : tiles) {
		if (!IsValidTile(tile)) return {ExcavationError::InvalidTile, tile};
		if (GetTileLayer(tile) != layer) return {ExcavationError::MixedLayers, tile};
		if (previous != INVALID_TILE && tile <= previous) return {ExcavationError::InvalidRoute, tile}; // Duplicates cannot double-charge or acquire two claims.
		previous = tile;
	}
	return {};
}

/**
 * Validate the route owner's canonical geometry and progress bounds.
 * @param geology Actual prepared/live terrain and deposit records.
 * @param job Canonical proposed/restored authority.
 * @return First unchanged geometry/authorization refusal.
 */
ExcavationFootprintResult ValidateExcavationJobGeometry(const GeologyWorldState &geology, const ExcavationJob &job)
{
	if (!IsValidTile(job.commissioning_anchor)) return {ExcavationError::InvalidTile, job.commissioning_anchor};
	const auto layer = GetTileLayer(job.commissioning_anchor);
	// Only rock jobs with bounded real phases/progress can hold geological claims.
	if ((layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) || geology.generator_version != 1 || job.route.empty() || job.route.size() > MAX_EXCAVATION_ROUTE || job.cursor > job.route.size() || job.partial_work_ticks > MAX_EXCAVATION_WORK_TICKS || to_underlying(job.state) > to_underlying(ExcavationJobState::Cancelled) || to_underlying(job.pause_reason) > to_underlying(ExcavationError::Limit) || job.completed_work_cost < 0) return {ExcavationError::InvalidJob, job.commissioning_anchor};
	const LayerVehicleCapability *capability = nullptr;
	// Resolve the immutable logical TBM design; no engine/vehicle surrogate is invented by the geometry owner.
	for (const auto &candidate : GetMultiLayerVehicleCapabilities()) {
		if (candidate.model_id == job.model_id) capability = &candidate;
	}
	if (capability == nullptr) return {ExcavationError::InvalidJob, job.commissioning_anchor};
	if (job.state == ExcavationJobState::Paused && job.pause_reason == ExcavationError::None) return {ExcavationError::InvalidJob, job.commissioning_anchor}; // A saved paused record must have an actual visible refusal; empty success cannot authorize a free cursor advance.
	if (job.pass_terms && job.state != ExcavationJobState::Proposed) { // Proposed capacities have no proof yet; only accepted/restored input must already be frozen.
		const auto pass = ValidateExcavationPassTerms(job, *job.pass_terms);
		if (pass.error != ExcavationError::None) return pass;
	}
	// Only actual TBM designs may reserve rock passes, with their own layer capability preserved.
	if (capability->commissioning != MultiLayerCommissioningFacility::RailPortalBay || !capability->supported_layers.Test(layer)) return {ExcavationError::InvalidPolicy, job.commissioning_anchor};
	// Both final-use targets and accepted working space must remain sorted, bounded domains on the same rock layer before claims can exist.
	for (const auto &tiles : {std::span<const TileIndex>{job.requested_tiles}, std::span<const TileIndex>{job.authorized_tiles}}) {
		const auto result = CheckJobTiles(tiles, layer, MAX_EXCAVATION_JOB_TILES);
		if (result.error != ExcavationError::None) return result;
	}
	// Requested final floor cannot extend beyond the explicitly accepted paid-work domain.
	for (TileIndex tile : job.requested_tiles) {
		if (!std::binary_search(job.authorized_tiles.begin(), job.authorized_tiles.end(), tile)) return {ExcavationError::OutsideAuthorization, tile};
	}
	TileIndex previous = job.commissioning_anchor;
	Direction previous_heading = Direction::Invalid;
	std::set<TileIndex> swept;
	// A pass advances one real tile at a time; headings may keep forward or turn at most 45 degrees while advancing.
	for (const auto &cut : job.route) {
		if (!IsValidDirection(cut.heading)) return {ExcavationError::IllegalTurn, cut.cutter_position};
		const auto offset = TileIndexDiffCByDir(cut.heading);
		if (TileAddWrap(previous, offset.x, offset.y) != cut.cutter_position) return {ExcavationError::InvalidRoute, cut.cutter_position};
		if (previous_heading != Direction::Invalid) {
			const uint difference = (to_underlying(cut.heading) - to_underlying(previous_heading) + 8) % 8;
			if (difference > 1 && difference < 7) return {ExcavationError::IllegalTurn, cut.cutter_position}; // Stationary/reversing/sharp pivots cannot authorize a swept pocket.
		}
		const auto result = CheckJobTiles(cut.swept_tiles, layer, 64);
		if (result.error != ExcavationError::None) return result;
		if (!std::binary_search(cut.swept_tiles.begin(), cut.swept_tiles.end(), cut.cutter_position)) return {ExcavationError::InvalidRoute, cut.cutter_position};
		// Every changed cell must be authorized even when it lies outside the requested final room.
		for (TileIndex tile : cut.swept_tiles) {
			if (!std::binary_search(job.authorized_tiles.begin(), job.authorized_tiles.end(), tile)) return {ExcavationError::OutsideAuthorization, tile};
			swept.insert(tile);
		}
		if (!cut.support_tiles.empty()) {
			const auto support = CheckJobTiles(cut.support_tiles, layer, 64);
			if (support.error != ExcavationError::None) return support;
		}
		previous = cut.cutter_position;
		previous_heading = cut.heading;
	}
	// The planner counts previously paid open floor as satisfied without sweeping it again. A further job may
	// request that existing floor beside its new cuts; every still-solid requested cell must remain in the actual
	// accepted route so completion cannot promise unpaid excavation or duplicate the first project's spoil.
	for (TileIndex tile : job.requested_tiles) {
		if (!swept.contains(tile) && geology.terrain.Get(tile) != ExcavationState::Excavated) return {ExcavationError::InvalidRoute, tile};
	}
	return {};
}

/**
 * Rebuild claims solely from actual unfinished route records, including already open swept cells.
 * @param geology Complete candidate; old derived lookup survives any refusal.
 * @return Complete acceptance or a duplicate/orphan/invalid terrain owner refusal.
 */
GeologyError RebuildExcavationJobClaims(GeologyWorldState &geology)
{
	if (geology.jobs.size() > MAX_EXCAVATION_JOBS) return GeologyError::Limit;
	std::map<TileIndex, ExcavationJobID> claims;
	ExcavationJobID previous{UINT32_MAX};
	size_t route_cells = 0;
	// Stable increasing IDs are saved authority; completed/cancelled jobs keep history but hold no unfinished cells.
	for (const auto &job : geology.jobs) {
		if (job.id.base() >= geology.next_job_id) return GeologyError::InvalidID;
		if (previous != ExcavationJobID{UINT32_MAX} && job.id <= previous) return GeologyError::InvalidID; // Sparse identities cannot acquire two records on reload.
		previous = job.id;
		if (ValidateExcavationJobGeometry(geology, job).error != ExcavationError::None) return GeologyError::InvalidShape;
		for (const auto &cut : job.route) route_cells += cut.swept_tiles.size() + cut.support_tiles.size(); // Bound aggregate saved geometry separately from sparse job count.
		if (route_cells > 262144) return GeologyError::Limit;
		if (job.state == ExcavationJobState::Proposed) return GeologyError::InvalidShape;
		// The completed prefix is actual persistent floor, including after cancellation; a saved cursor cannot invent paid completion.
		for (size_t i = 0; i < job.cursor; ++i) {
			for (TileIndex tile : job.route[i].swept_tiles) {
				if (geology.terrain.Get(tile) != ExcavationState::Excavated) return GeologyError::ClaimConflict;
			}
		}
		if (job.state == ExcavationJobState::Completed) {
			if (job.cursor != job.route.size() || job.partial_work_ticks != 0) return GeologyError::InvalidShape; // Complete jobs cannot hide unfinished paid work.
			continue;
		}
		if (job.state == ExcavationJobState::Cancelled) continue;
		if (job.cursor == job.route.size()) return GeologyError::InvalidShape;
		// Only uncompleted swept cells are exclusive cut claims; external controllers own physical retreat/track reservations.
		for (size_t i = job.cursor; i < job.route.size(); ++i) {
			for (TileIndex tile : job.route[i].swept_tiles) {
				const auto terrain = geology.terrain.Get(tile);
				if (!terrain || *terrain == ExcavationState::Solid) return GeologyError::ClaimConflict; // Every unfinished solid cell must carry its saved active reservation.
				const auto [entry, inserted] = claims.emplace(tile, job.id);
				if (!inserted && entry->second != job.id) return GeologyError::ClaimConflict; // Competing records cannot select an owner by load order.
			}
		}
		if (claims.size() > MAX_EXCAVATION_CLAIMS) return GeologyError::Limit;
	}
	// Scan packed bytes first: only sparse active cells need projection, keeping maximum-map reload work bounded.
	const auto packed = geology.terrain.Data();
	for (size_t byte = 0; byte < packed.size(); ++byte) {
		if ((packed[byte] & 0x55) == 0) continue;
		for (uint pair = 0; pair < 4; ++pair) {
			if (((packed[byte] >> (pair * 2)) & 3) != to_underlying(ExcavationState::Excavating)) continue;
			const uint32_t index = static_cast<uint32_t>(byte * 4 + pair);
			const uint32_t local = index % Map::PlaneSize();
			const auto layer = index < Map::PlaneSize() ? MapLayer::Underground : MapLayer::DeepUnderground;
			const TileIndex tile = TileXYL(local % Map::SizeX(), local / Map::SizeX(), layer);
			if (!claims.contains(tile)) return GeologyError::ClaimConflict;
		}
	}
	geology.job_claims.swap(claims);
	return GeologyError::None;
}

/**
 * Check actual optional bindings without copying a train/stock/station pool into geology.
 * @param job Actual accepted/saved job.
 * @return Whether it has a real company or explicit orphan pause and every supplied binding resolves.
 */
bool ValidateExcavationJobReferences(const ExcavationJob &job)
{
	if (!Company::IsValidID(job.company)) {
		// Removed ownership is explicit paused state; a dangling company value cannot silently attach to another company later.
		if (job.company != CompanyID::Invalid()) return false;
		const bool closed = job.state == ExcavationJobState::Completed || job.state == ExcavationJobState::Cancelled;
		if (!closed && (job.state != ExcavationJobState::Paused || job.pause_reason != ExcavationError::InvalidOwner)) return false; // Closed orphan history has no active controller; live work must carry its explicit recoverable pause.
	}
	if (job.cutter != VehicleID::Invalid()) {
		if (!Vehicle::IsValidID(job.cutter)) return false;
		if (Vehicle::Get(job.cutter)->type != VehicleType::Train) return false;
		// A supplied cutter must belong to this company; explicit orphan pauses preserve resolved bindings for later lifecycle recovery.
		if (job.company != CompanyID::Invalid() && Vehicle::Get(job.cutter)->owner != job.company) return false;
	}
	if (job.origin != StationID::Invalid()) {
		if (!Station::IsValidID(job.origin)) return false;
		if (job.company != CompanyID::Invalid() && Station::Get(job.origin)->owner != job.company) return false; // An origin reference cannot borrow another company's real exchange station.
	}
	return true;
}
