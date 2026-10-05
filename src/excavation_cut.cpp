/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_cut.cpp Pure complete-cut accounting and owner/geometry/capacity preflight. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Groups 11 and 14B (solid start); r2mlvs4-tunnel-boring-machines.md, Group 6 (per-tick work estimate); r2mlvs1-e-ground-and-space-budgets.md, Group 13; docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#include "stdafx.h"
#include "excavation.h"
#include "company_base.h"
#include "tile_map.h"

#include "safeguards.h"

/** Frozen per-new-cell terms; support material units and money remain separate external-owner quantities. */
struct ExcavationCellTerms {
	uint8_t earth, rock, standard_days, deep_days; ///< Tonnes and 74-tick economy work days per new cell.
	uint16_t support_cost; ///< Explicit once-only temporary rail/lining charge, excluding controller running costs.
};
static constexpr std::array<ExcavationCellTerms, 4> _cut_terms = {{{6, 12, 1, 1, 30}, {4, 24, 2, 1, 50}, {2, 40, 3, 2, 80}, {1, 48, 0, 4, 120}}};

/**
 * Quote explicitly previewed working cells through the actual terrain/seam owner and shared frozen terms.
 * @param tiles Bounded canonical cells; this creates no live reservation.
 * @param model_id Logical TBM whose layer/hardness capability applies.
 * @return Complete extra-work estimate, or stable zero-quantity refusal.
 */
/**
 * Cutting difficulty of one rock cell: its deposit's frozen hardness, otherwise the layer default.
 * @param geology Read-only geological owner.
 * @param tile Rock-layer cell.
 * @return Hardness used by quotes and actual cuts alike.
 */
GeologyHardness GetExcavationCellHardness(const GeologyWorldState &geology, TileIndex tile)
{
	const auto *deposit = geology.deposits.Get(geology.deposits.Find(tile));
	if (deposit != nullptr) return deposit->hardness;
	return GetTileLayer(tile) == MapLayer::DeepUnderground ? GeologyHardness::Hard : GeologyHardness::Soft;
}

ExcavationCutResult QuoteExcavationCells(std::span<const TileIndex> tiles, uint16_t model_id)
{
	if (tiles.empty()) return {};
	if (tiles.size() > MAX_EXCAVATION_JOB_TILES) return {ExcavationError::Limit};
	const LayerVehicleCapability *model = nullptr;
	// Resolve the real immutable catalogue rather than importing another feature's private engine table.
	for (const auto &candidate : GetMultiLayerVehicleCapabilities()) {
		if (candidate.model_id == model_id) model = &candidate;
	}
	if (model == nullptr) return {ExcavationError::InvalidPolicy};
	if (model->commissioning != MultiLayerCommissioningFacility::RailPortalBay) return {ExcavationError::InvalidPolicy};
	const auto &geology = GetGeologyWorldState();
	if (!IsValidTile(tiles.front())) return {ExcavationError::InvalidTile, tiles.front()};
	const auto layer = GetTileLayer(tiles.front());
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return {ExcavationError::MixedLayers, tiles.front()}; // Preview cannot interpret Surface/Space as a rock-plane working domain.
	if (!model->supported_layers.Test(layer)) return {ExcavationError::InvalidPolicy, tiles.front()};
	if (layer == MapLayer::DeepUnderground) {
		if (!CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return {ExcavationError::LockedDeep, tiles.front()};
	}
	ExcavationCutResult result;
	TileIndex previous = INVALID_TILE;
	// Planning charges only actual solid extra cells; open legacy/cavity space produces no second spoil or support bill.
	for (TileIndex tile : tiles) {
		if (!IsValidTile(tile)) return {ExcavationError::InvalidTile, tile};
		if (GetTileLayer(tile) != layer) return {ExcavationError::MixedLayers, tile};
		if (previous != INVALID_TILE && tile <= previous) return {ExcavationError::InvalidRoute, tile}; // Canonical uniqueness prevents previews from double-charging duplicate extra cells.
		previous = tile;
		if (geology.generator_version == 0) continue;
		if (geology.job_claims.contains(tile)) return {ExcavationError::ActiveCut, tile};
		const auto terrain = geology.terrain.Get(tile);
		if (!terrain) return {ExcavationError::InvalidTile, tile};
		if (*terrain == ExcavationState::Excavating) return {ExcavationError::ActiveCut, tile};
		if (*terrain == ExcavationState::Excavated) continue;
		const auto &terms = _cut_terms[to_underlying(GetExcavationCellHardness(geology, tile))];
		const uint days = model_id == 0 ? terms.standard_days : terms.deep_days;
		if (days == 0) return {ExcavationError::InvalidPolicy, tile};
		++result.new_cells; ++result.support_units;
		result.earth_t += terms.earth; result.rock_t += terms.rock;
		result.cost += terms.support_cost; result.work_ticks += days * 74;
	}
	return result;
}

/**
 * Quote only the sole live owner route, without deriving permission from caller-supplied cutter geometry.
 * @param request Exact reserved identity/revision/cursor.
 * @param material Actual external snapshot, never another stock/cash owner.
 * @return Complete quote or unchanged zero-quantity refusal.
 */
ExcavationCutResult PrepareExcavationCut(const ExcavationCutRequest &request, const ExcavationMaterialSnapshot &material)
{
	const auto &geology = GetGeologyWorldState();
	const auto *job = GetExcavationJob(request.job);
	if (job == nullptr) return {ExcavationError::InvalidJob};
	// Closed or stale requests never replay old spoil/support charges or advance a different accepted pass.
	if (job->state == ExcavationJobState::Completed || job->state == ExcavationJobState::Cancelled) return {ExcavationError::AlreadyCompleted};
	if (job->revision != request.job_revision || job->cursor != request.cursor || job->cursor >= job->route.size() || job->revision == UINT64_MAX) return {ExcavationError::StaleParticipant};
	if (!ValidateExcavationJobReferences(*job)) return {ExcavationError::InvalidOwner};
	if (!Company::IsValidID(job->company)) return {ExcavationError::InvalidOwner};
	const auto &policy = GetCurrentMultiLayerProfileSelection();
	if (policy.id != MultiLayerGameplayProfileID::MultilayerIndustry) return {ExcavationError::InvalidPolicy};
	const auto layer = GetTileLayer(job->commissioning_anchor);
	if (layer == MapLayer::DeepUnderground) {
		if (!CheckDeepLayerAvailability(policy, TimerGameCalendar::date)) return {ExcavationError::LockedDeep};
	}
	if (job->state == ExcavationJobState::Paused) return {job->pause_reason};
	if (!job->pass_terms) return {ExcavationError::InvalidJob};
	const auto &cut = job->route[job->cursor];
	const auto geometry = ValidateExcavationJobGeometry(geology, *job);
	if (geometry.error != ExcavationError::None) return {geometry.error, geometry.failure_tile};
	// Only cut 0 can still sit inside an unopened portal pocket; its issuing pair must be Commissioning exactly this
	// job and route[0] must leave through the door, otherwise the pocket would be an unpaid or foreign starter room.
	const auto starter = job->cursor == 0 ? ResolveExcavationStarter(*job) : ExcavationStarterResult{};
	if (starter.error != ExcavationError::None) return {starter.error, starter.failure_tile};
	if (starter.required) {
		const auto first = CheckExcavationStarterGeometry(*job, starter.exit_heading, starter.pocket);
		if (first.error != ExcavationError::None) return {first.error, first.failure_tile};
	}
	/**
	 * Accept one unopened cell as solid-start support: it must be this job's reserved pocket rock, opened by this very cut.
	 * @param tile Candidate support/retreat cell.
	 * @return Whether the cell is open, or is reserved starter rock of this job at cut 0.
	 */
	const auto supported = [&](TileIndex tile) {
		if (geology.terrain.Get(tile) == ExcavationState::Excavated) return true;
		if (!starter.required || !std::binary_search(starter.pocket.begin(), starter.pocket.end(), tile)) return false;
		const auto claim = geology.job_claims.find(tile);
		return claim != geology.job_claims.end() && claim->second == job->id;
	};
	// A newly obstructed or contested backing approach invalidates the frozen route before any support, stock or cash write.
	for (TileIndex tile : GetExcavationRetreatPath(*job, job->cursor)) {
		if (!supported(tile)) return {ExcavationError::MissingSupport, tile};
		const auto owner = geology.job_claims.find(tile);
		if (owner != geology.job_claims.end() && owner->second != job->id) return {ExcavationError::Conflict, tile}; // Another accepted job cannot silently become this loaded return path.
		const auto type = GetTileType(tile);
		if (type != TileType::Clear && type != TileType::Railway && type != TileType::LayerPortal) return {ExcavationError::Conflict, tile}; // A converted building/road footprint needs a new retreat proposal rather than an implicit demolition.
		if (type != TileType::Clear && GetTileOwner(tile) != job->company) return {ExcavationError::InvalidOwner, tile}; // A later infrastructure-owner transfer invalidates this job's accepted backing before stock/cash changes.
	}
	// Every conservative cutter cell must belong to the accepted swept envelope before any material/cost proposal exists.
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			const auto tile = TileAddWrap(cut.cutter_position, x, y);
			if (!std::binary_search(cut.swept_tiles.begin(), cut.swept_tiles.end(), tile)) return {ExcavationError::InvalidRoute, tile};
		}
	}
	const auto previous = job->cursor == 0 ? job->commissioning_anchor : job->route[job->cursor - 1].cutter_position;
	// The old cutter envelope is backed open support, not permission to consume an unissued solid commissioning pocket.
	// An issued, reserved pocket is the single exception, and only for cut 0.
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			const auto tile = TileAddWrap(previous, x, y);
			if (!supported(tile)) return {ExcavationError::MissingSupport, tile};
		}
	}
	// Requested physical support floor must remain open; actual rail/vehicle/retreat hardware is additionally validated by the participant.
	for (TileIndex tile : cut.support_tiles) {
		if (!supported(tile)) return {ExcavationError::MissingSupport, tile};
	}
	ExcavationCutResult result;
	std::map<GeologyDepositID, uint32_t> seam_cells;
	// Charge/generate only newly reserved rock cells; open overlap contributes no second yield, reserve debit or lining bill.
	for (TileIndex tile : cut.swept_tiles) {
		const auto claim = geology.job_claims.find(tile);
		if (claim == geology.job_claims.end() || claim->second != job->id) return {ExcavationError::Conflict, tile}; // The complete cut has one actual exclusive owner.
		const auto terrain = geology.terrain.Get(tile);
		if (!terrain) return {ExcavationError::InvalidTile, tile};
		if (*terrain == ExcavationState::Excavated) {
			const auto type = GetTileType(tile);
			if (type != TileType::Clear && type != TileType::Railway && type != TileType::LayerPortal) return {ExcavationError::Conflict, tile}; // Already open overlap still belongs to the full cutter envelope; later buildings cannot be ignored as zero-yield cells.
			if (type != TileType::Clear && GetTileOwner(tile) != job->company) return {ExcavationError::InvalidOwner, tile}; // Physical support ownership changes invalidate the accepted sweep before any whole-cut receipt.
			continue;
		}
		if (*terrain != ExcavationState::Excavating) return {ExcavationError::Conflict, tile};
		// Excavation cannot overwrite existing infrastructure or an occupied/owned structure as a side effect of opening rock.
		// The issuing housing of a solid start is the only exception: cut 0 opens its rock and keeps its portal metadata.
		const bool housing = starter.required && tile == job->commissioning_anchor && GetTileType(tile) == TileType::LayerPortal && GetTileOwner(tile) == job->company;
		if (GetTileType(tile) != TileType::Clear && !housing) return {ExcavationError::Conflict, tile};
		const auto *deposit = geology.deposits.Get(geology.deposits.Find(tile));
		if (deposit != nullptr && deposit->claimant != CompanyID::Invalid() && deposit->claimant != job->company) return {ExcavationError::Conflict, tile}; // Mining/excavation share the finite owner; a later claim invalidates the earlier proposal.
		const auto &terms = _cut_terms[to_underlying(GetExcavationCellHardness(geology, tile))];
		const uint days = job->model_id == 0 ? terms.standard_days : terms.deep_days;
		if (days == 0) return {ExcavationError::InvalidPolicy, tile};
		++result.new_cells;
		result.earth_t += terms.earth;
		result.rock_t += terms.rock;
		++result.support_units;
		result.cost += terms.support_cost;
		result.work_ticks += days * 74;
		if (deposit != nullptr) ++seam_cells[deposit->id];
	}
	// Bound multiplication by dividing first; the validated exact per-cell reserve quantum cannot overflow when scaled by touched cells.
	for (const auto &[id, cells] : seam_cells) {
		const auto *deposit = geology.deposits.Get(id);
		uint64_t shape_cells = 0;
		for (const auto &span : deposit->spans) shape_cells += span.last_x - span.first_x + 1; // Canonical spans count each recoverable cell once.
		const uint64_t quantum = deposit->initial_reserve / shape_cells;
		const uint64_t loss = quantum * cells; // cells cannot exceed the validated complete seam shape, so this product is <= initial reserve.
		result.reserve_debits.push_back({id, deposit->resource, std::min(loss, deposit->remaining_reserve), deposit->remaining_reserve});
	}
	// Separate real Earth/Rock capacity and complete lining/funds are required; no partial sink or hidden internal hold is accepted.
	if (result.earth_t > material.earth_space_t || result.rock_t > material.rock_space_t) return {ExcavationError::InsufficientCapacity};
	if (result.earth_t > job->pass_terms->earth_capacity_t || result.rock_t > job->pass_terms->rock_capacity_t) return {ExcavationError::InsufficientCapacity}; // A larger live sink cannot silently change the accepted independent wagon limits.
	if (result.support_units > material.support_units) return {ExcavationError::MissingSupport};
	if (result.cost > material.available_funds) return {ExcavationError::InsufficientFunds};
	// Historical receipts are checked before addition; overflow must refuse before external cash/stock or terrain writes.
	if (job->completed_earth_t > UINT64_MAX - result.earth_t || job->completed_rock_t > UINT64_MAX - result.rock_t || job->completed_work_cost.base() > INT64_MAX - result.cost.base()) return {ExcavationError::Limit};
	return result;
}

/**
 * Estimate the job's next cut from the shared per-cell terms, without geometry or material checks.
 * @param id Exact job.
 * @return Quantities of the still-unopened swept cells, or InvalidJob/AlreadyCompleted.
 */
ExcavationCutResult EstimateExcavationCut(ExcavationJobID id)
{
	const auto *job = GetExcavationJob(id);
	if (job == nullptr) return {ExcavationError::InvalidJob};
	// A closed job, or one whose cursor is past its route, has no next cut to work toward.
	if (job->state == ExcavationJobState::Completed || job->state == ExcavationJobState::Cancelled || job->cursor >= job->route.size()) return {ExcavationError::AlreadyCompleted};
	const auto &geology = GetGeologyWorldState();
	ExcavationCutResult result;
	// Open overlap was paid by an earlier cut; only still-unopened cells cost work, spoil, support and money.
	for (TileIndex tile : job->route[job->cursor].swept_tiles) {
		const auto terrain = geology.terrain.Get(tile);
		if (!terrain.has_value() || *terrain == ExcavationState::Excavated) continue;
		const auto &terms = _cut_terms[to_underlying(GetExcavationCellHardness(geology, tile))];
		++result.new_cells;
		++result.support_units;
		result.earth_t += terms.earth;
		result.rock_t += terms.rock;
		result.cost += terms.support_cost;
		result.work_ticks += (job->model_id == 0 ? terms.standard_days : terms.deep_days) * 74;
	}
	return result;
}
