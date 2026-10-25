/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_area.cpp Read-only project area preview through the canonical geology planner (no second planner).
 * Shows passes, turn envelope, hardness, both spoil quantities and support cost before any command accepts the area.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical working-space ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23CB bounded approaches.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 4 (preview) and 5 (actual-consist adapter)
 */

#include "stdafx.h"
#include "tbm.h"
#include "tbm_area_preview.h"
#include "map_func.h"
#include "train.h"
#include "table/strings.h"

#include "safeguards.h"

/**
 * Add the explicitly quoted cutter-body ring without changing the requested floor.
 * @param anchor Existing housing or accepted route endpoint.
 * @param floor Canonical requested corridor centres or rectangular floor.
 * @param previous_work Existing authorization retained for a live job's approach and tail.
 * @param joined Whether the working domain must already be one connected piece. A floor away from its housing is not
 *               joined until an approach links them (r2df Group 9), so the approach builder asks for the cells only.
 * @return Floor and complete bounded working domain; no rock or claims change.
 */
static ExcavationOutline MakeTBMOutlineWithRing(TileIndex anchor, const std::vector<TileIndex> &floor, std::span<const TileIndex> previous_work, bool joined = true)
{
	const MapLayer layer = GetTileLayer(anchor);
	auto result = NormalizeExcavationOutline(layer, floor);
	if (result.error != ExcavationError::None) return result;
	std::vector<TileIndex> centres = floor;
	centres.push_back(anchor);
	std::vector<TileIndex> work(previous_work.begin(), previous_work.end());
	// The same explicitly displayed one-cell ring covers the 3x3 cutter body
	// for fresh and extended work; existing authorization is never discarded.
	for (TileIndex centre : centres) {
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dx = -1; dx <= 1; ++dx) {
				const TileIndex cell = TileAddWrap(centre, dx, dy);
				if (IsValidTile(cell) && GetTileLayer(cell) == layer) work.push_back(cell);
			}
		}
	}
	std::ranges::sort(work);
	work.erase(std::unique(work.begin(), work.end()), work.end());
	if (joined) {
		const auto accepted = NormalizeExcavationOutline(layer, work);
		if (accepted.error != ExcavationError::None) return accepted;
		result.authorized_tiles = accepted.requested_tiles;
	} else {
		// Canonical cells only: the approach joins the pieces, and AuthorizeExcavationOutline checks the joined whole.
		if (work.size() > MAX_EXCAVATION_JOB_TILES) return {ExcavationError::Limit, work.front()};
		result.authorized_tiles = std::move(work);
	}
	std::ranges::set_difference(result.authorized_tiles, result.requested_tiles, std::back_inserter(result.extra_tiles));
	return result;
}

/**
 * Form the rectangle's floor and ring around the floor and the anchor.
 * @param anchor Original rock-side housing, excluded from requested final-use floor.
 * @param area_start First explicit floor corner on the anchor's layer.
 * @param area_end Opposite floor corner.
 * @param previous_work Already accepted work retained when extending an existing job.
 * @param joined Whether the working domain must already be one connected piece.
 * @return Canonical floor/working outline or refusal.
 */
static ExcavationOutline MakeTBMWorkCells(TileIndex anchor, TileIndex area_start, TileIndex area_end, std::span<const TileIndex> previous_work, bool joined)
{
	// A rectangle cannot cross a stacked-map boundary or acquire permission
	// from a camera layer different from its actual commissioning housing.
	if (!IsValidTile(anchor) || !IsValidTile(area_start) || !IsValidTile(area_end)) return {ExcavationError::InvalidTile};
	const MapLayer layer = GetTileLayer(anchor);
	if (GetTileLayer(area_start) != layer || GetTileLayer(area_end) != layer) return {ExcavationError::MixedLayers};
	const uint x0 = std::min(TileX(area_start), TileX(area_end)), x1 = std::max(TileX(area_start), TileX(area_end));
	const uint y0 = std::min(TilePlaneY(area_start), TilePlaneY(area_end)), y1 = std::max(TilePlaneY(area_start), TilePlaneY(area_end));
	if (x1 - x0 >= 64 || y1 - y0 >= 64 || previous_work.size() > MAX_EXCAVATION_JOB_TILES) return {ExcavationError::Limit}; // Bound planning before allocating player-supplied working space.
	std::vector<TileIndex> floor;
	// Row-major floor selection stays independent of its future rail/mine use;
	// the existing housing itself is a facility, not new final-use floor.
	for (uint y = y0; y <= y1; ++y) {
		for (uint x = x0; x <= x1; ++x) {
			const TileIndex tile = TileXYL(x, y, layer);
			if (tile != anchor) floor.push_back(tile);
		}
	}
	return MakeTBMOutlineWithRing(anchor, floor, previous_work, joined);
}

/** @copydoc MakeTBMWorkOutline */
ExcavationOutline MakeTBMWorkOutline(TileIndex anchor, TileIndex area_start, TileIndex area_end, std::span<const TileIndex> previous_work)
{
	return MakeTBMWorkCells(anchor, area_start, area_end, previous_work, true);
}

/** @copydoc SummarizeTBMApproach */
TBMApproachSummary SummarizeTBMApproach(std::span<const ExcavationCutGeometry> route, std::span<const TileIndex> extra_tiles, uint16_t model_id, ExcavationJobID own_job)
{
	TBMApproachSummary summary;
	if (extra_tiles.empty()) return summary;
	// A cut that sweeps any extra cell uses the approach; each cell is counted once however many cuts sweep it.
	for (const auto &cut : route) {
		bool uses = false;
		for (TileIndex tile : cut.swept_tiles) {
			if (!std::ranges::binary_search(extra_tiles, tile)) continue;
			summary.tiles.push_back(tile);
			uses = true;
		}
		if (uses) ++summary.cuts;
	}
	std::ranges::sort(summary.tiles);
	summary.tiles.erase(std::unique(summary.tiles.begin(), summary.tiles.end()), summary.tiles.end());
	if (!summary.tiles.empty()) summary.estimate = QuoteExcavationCells(summary.tiles, model_id, own_job);
	return summary;
}

/** @copydoc GetTBMApproachRefusal */
StringID GetTBMApproachRefusal(ExcavationError error, TileIndex tile)
{
	switch (error) {
		case ExcavationError::InvalidTile: return STR_ERROR_TBM_APPROACH_MAP_EDGE; // Only the map's edge refuses a cut's cells as invalid.
		case ExcavationError::Conflict:
			// A planning conflict is a building or another company's seam; open rock with a conflict is the seam.
			return IsValidTile(tile) && IsTileType(tile, TileType::Clear) ? STR_ERROR_TBM_APPROACH_OTHER_WORK : STR_ERROR_TBM_APPROACH_BUILDING;
		case ExcavationError::ActiveCut:
		case ExcavationError::InvalidOwner: return STR_ERROR_TBM_APPROACH_OTHER_WORK;
		case ExcavationError::Limit: return STR_ERROR_TBM_APPROACH_TOO_FAR;
		default: return STR_ERROR_TBM_COMMISSION_ROUTE;
	}
}

/** @copydoc MakeTBMWorkOutlineWithApproach */
ExcavationOutline MakeTBMWorkOutlineWithApproach(const ExcavationPlanningPose &start, bool solid_start, uint16_t model_id, ExcavationJobID continuing_job,
	TileIndex ring_anchor, TileIndex area_start, TileIndex area_end, std::span<const TileIndex> previous_work, ExcavationApproachProposal &approach, uint32_t kept_cuts)
{
	approach = {};
	approach.end = start;
	auto base = MakeTBMWorkCells(ring_anchor, area_start, area_end, previous_work, false);
	if (base.error != ExcavationError::None) return base;
	approach = PlanExcavationApproach(start, base, model_id, {}, continuing_job, solid_start, false, kept_cuts);
	if (approach.error != ExcavationError::None) {
		// Keep the requested floor, so the refused quote can still say how much of it was reachable.
		base.error = approach.error;
		base.failure_tile = approach.failure_tile;
		return base;
	}
	// Floor, ring, housing and earlier work, plus the approach and turning cells, as one connected canonical domain. Like
	// MakeTBMWorkOutline this quotes nothing: earlier work may be mid-cut, and the commission quotes its own extra cells.
	std::vector<TileIndex> work = base.authorized_tiles;
	work.insert(work.end(), approach.extra_tiles.begin(), approach.extra_tiles.end());
	const auto joined = NormalizeExcavationOutline(base.layer, work);
	if (joined.error != ExcavationError::None) {
		base.error = joined.error;
		base.failure_tile = joined.failure_tile;
		return base;
	}
	base.authorized_tiles = joined.requested_tiles;
	base.extra_tiles.clear();
	std::ranges::set_difference(base.authorized_tiles, base.requested_tiles, std::back_inserter(base.extra_tiles));
	return base;
}

/** @copydoc MakeTBMTunnelOutline(TileIndex, Direction, TileIndex, std::span<const TileIndex>) */
ExcavationOutline MakeTBMTunnelOutline(TileIndex start, Direction heading, TileIndex end, std::span<const TileIndex> previous_work)
{
	// A straight route retains the actual heading and plane; clicking behind
	// or beside the machine never authorizes a hidden turn or reverse cut.
	if (!IsValidTile(start) || !IsValidTile(end) || !IsValidDirection(heading)) return {ExcavationError::InvalidTile};
	if (GetTileLayer(start) != GetTileLayer(end)) return {ExcavationError::MixedLayers};
	const int dx = int(TileX(end)) - int(TileX(start)), dy = int(TilePlaneY(end)) - int(TilePlaneY(start));
	const int steps = std::max(std::abs(dx), std::abs(dy));
	const auto delta = TileIndexDiffCByDir(heading);
	if (steps == 0 || dx != steps * delta.x || dy != steps * delta.y) return {ExcavationError::IllegalTurn};
	if (steps > 64 || previous_work.size() > MAX_EXCAVATION_JOB_TILES) return {ExcavationError::Limit}; // Match the bounded area-tool extent before allocating any proposed floor.
	std::vector<TileIndex> floor;
	std::vector<TileIndex> work(previous_work.begin(), previous_work.end());
	ExcavationPlanningPose pose{start, heading, std::vector<TileIndex>(3, start)};
	// Use the canonical full sweep, including the two outside corners of a
	// diagonal advance. A union of endpoint body rings misses those corners.
	// Existing authorization retains the actual longer tail on an extension.
	for (int index = 0; index < steps; ++index) {
		const auto step = AdvanceExcavationPlanningPose(pose, heading);
		if (step.error != ExcavationError::None) return {step.error, step.failure_tile};
		floor.push_back(step.next.position);
		work.insert(work.end(), step.cut.swept_tiles.begin(), step.cut.swept_tiles.end());
		pose = step.next;
	}
	auto result = NormalizeExcavationOutline(GetTileLayer(start), floor);
	if (result.error != ExcavationError::None) return result;
	std::ranges::sort(work);
	work.erase(std::unique(work.begin(), work.end()), work.end());
	const auto accepted = NormalizeExcavationOutline(result.layer, work);
	if (accepted.error != ExcavationError::None) return accepted;
	result.authorized_tiles = accepted.requested_tiles;
	std::ranges::set_difference(result.authorized_tiles, result.requested_tiles, std::back_inserter(result.extra_tiles));
	return result;
}

/** @copydoc MakeTBMTunnelOutline(const ExcavationPlanningPose &, bool, uint16_t, ExcavationJobID, TileIndex, std::span<const TileIndex>, ExcavationApproachProposal &, uint32_t) */
ExcavationOutline MakeTBMTunnelOutline(const ExcavationPlanningPose &start, bool solid_start, uint16_t model_id, ExcavationJobID continuing_job,
	TileIndex end, std::span<const TileIndex> previous_work, ExcavationApproachProposal &approach, uint32_t kept_cuts)
{
	approach = {};
	approach.end = start;
	if (!IsValidTile(start.position) || !IsValidTile(end) || !IsValidDirection(start.heading)) return {ExcavationError::InvalidTile};
	const MapLayer layer = GetTileLayer(start.position);
	if (GetTileLayer(end) != layer) return {ExcavationError::MixedLayers};
	const int dx = int(TileX(end)) - int(TileX(start.position)), dy = int(TilePlaneY(end)) - int(TilePlaneY(start.position));
	const int steps = std::max(std::abs(dx), std::abs(dy));
	const auto delta = TileIndexDiffCByDir(start.heading);
	// An end exactly ahead keeps the straight centreline and its own work ring.
	if (steps != 0 && dx == steps * delta.x && dy == steps * delta.y) return MakeTBMTunnelOutline(start.position, start.heading, end, previous_work);
	if (steps == 0) return {ExcavationError::IllegalTurn};
	if (steps > 64 || previous_work.size() > MAX_EXCAVATION_JOB_TILES) return {ExcavationError::Limit}; // The same extent as a straight tunnel.
	// An angled tunnel: the cutter centre reaches the end through an exact-goal approach of advancing 45-degree bends.
	// Its cutter centres become the requested floor and its complete sweeps the work, as the straight builder does.
	ExcavationOutline target;
	target.layer = layer;
	target.requested_tiles = {end};
	target.authorized_tiles.assign(previous_work.begin(), previous_work.end());
	approach = PlanExcavationApproach(start, target, model_id, {}, continuing_job, solid_start, true, kept_cuts);
	if (approach.error != ExcavationError::None) {
		ExcavationOutline refused = target;
		refused.error = approach.error;
		refused.failure_tile = approach.failure_tile;
		return refused;
	}
	std::vector<TileIndex> floor;
	std::vector<TileIndex> work(previous_work.begin(), previous_work.end());
	for (const auto &cut : approach.route) {
		floor.push_back(cut.cutter_position);
		work.insert(work.end(), cut.swept_tiles.begin(), cut.swept_tiles.end());
	}
	auto result = NormalizeExcavationOutline(layer, floor);
	if (result.error != ExcavationError::None) return result;
	std::ranges::sort(work);
	work.erase(std::unique(work.begin(), work.end()), work.end());
	const auto accepted = NormalizeExcavationOutline(layer, work);
	if (accepted.error != ExcavationError::None) return accepted;
	result.authorized_tiles = accepted.requested_tiles;
	std::ranges::set_difference(result.authorized_tiles, result.requested_tiles, std::back_inserter(result.extra_tiles));
	return result;
}

/**
 * Shared preview body for a consist whose dimensions are already known.
 * @param request Area, start pose, model and owners.
 * @param dimensions Definition-based or measured consist dimensions.
 * @return Complete preview, or a refusal with the first failing stage.
 */
static TBMAreaPreview PreviewTBMAreaWithDimensions(const TBMAreaRequest &request, const TBMConsistDimensions &dimensions)
{
	TBMAreaPreview preview;
	/**
	 * Finish a geology refusal.
	 * @param error Stable refusal.
	 * @param tile Failing location.
	 * @return The preview carrying the refusal.
	 */
	const auto refuse = [&preview](ExcavationError error, TileIndex tile) {
		preview.error = error;
		preview.failure_tile = tile;
		return preview;
	};

	/* The consist fixes the planner history and both capacities; nothing is planned for an impossible train. */
	if (dimensions.error != TBMAcquisitionError::None) {
		preview.consist_error = dimensions.error;
		return preview;
	}
	if (request.outline.error != ExcavationError::None) return refuse(request.outline.error, request.outline.failure_tile);
	// A start pose planned for a different consist length would sweep the wrong trailing cells.
	if (request.start.history.size() != dimensions.planning_points) return refuse(ExcavationError::InvalidRoute, request.start.position);

	/* Canonical planner: the only source of passes; a partial result is shown, never silently accepted as complete. */
	preview.plan = PlanExcavationRoute(request.outline, request.start, request.model_id, {}, request.intent, request.tunnel_end);
	if (preview.plan.error != ExcavationError::None) return refuse(preview.plan.error, preview.plan.failure_tile);
	preview.partial = preview.plan.partial;
	preview.totals = preview.plan.estimate;

	std::vector<TileIndex> swept;
	// Collect each swept cell once; the turn envelope and hardness are views of this one set.
	for (const auto &cut : preview.plan.route) swept.insert(swept.end(), cut.swept_tiles.begin(), cut.swept_tiles.end());
	std::ranges::sort(swept);
	swept.erase(std::unique(swept.begin(), swept.end()), swept.end());
	std::ranges::set_difference(swept, request.outline.requested_tiles, std::back_inserter(preview.turn_envelope));
	const auto &geology = GetGeologyWorldState();
	// Only still-solid cells are cut, so only they show a hardness; open cavity costs and yields nothing.
	for (TileIndex tile : swept) {
		const auto terrain = geology.terrain.Get(tile);
		if (terrain.has_value() && *terrain != ExcavationState::Excavated) ++preview.hardness[to_underlying(GetExcavationCellHardness(geology, tile))];
	}

	/* Canonical job proposal at the consist's real capacities, quoted through pass and reservation estimates. */
	ExcavationJob job;
	job.company = request.company;
	job.origin = request.origin;
	job.model_id = request.model_id;
	job.commissioning_anchor = request.start.position;
	if (preview.partial) {
		job.requested_tiles = preview.plan.covered_targets; // A partial plan may only request the floor it actually opens.
		std::ranges::sort(job.requested_tiles);
	} else {
		job.requested_tiles = request.outline.requested_tiles;
	}
	job.authorized_tiles = request.outline.authorized_tiles;
	job.route = preview.plan.route;
	ExcavationPassTerms terms;
	terms.earth_capacity_t = dimensions.earth_capacity_t;
	terms.rock_capacity_t = dimensions.rock_capacity_t;
	terms.exchange_path = request.exchange_path;
	const auto pass = PrepareExcavationPass(job, terms);
	if (pass.error != ExcavationError::None) return refuse(pass.error, pass.failure_tile);
	job.pass_terms = pass.terms;
	preview.terms = pass.terms;
	const auto reservation = ReserveExcavationJob(job, false);
	if (reservation.error != ExcavationError::None) return refuse(reservation.error, reservation.failure_tile);
	preview.prospective_job = reservation.id;
	preview.proposal = std::move(job);
	return preview;
}

/**
 * Preview an area through the canonical planner, pass and reservation estimates.
 * @param request Area, start pose, consist and owners.
 * @return Complete preview, or a refusal with the first failing stage.
 */
TBMAreaPreview PreviewTBMArea(const TBMAreaRequest &request)
{
	return PreviewTBMAreaWithDimensions(request, GetTBMConsistDimensions(request.model_id, request.consist));
}

/**
 * Adapt the canonical planner to an actual assigned consist.
 * @param project Assigned project with a live cutter.
 * @param area Outline, start pose and exchange path; consist and owner fields are replaced from the project.
 * @return Complete preview, or a refusal.
 */
TBMAreaPreview ValidateTBMAreaRoute(const TBMProject &project, const TBMAreaRequest &area)
{
	TBMAreaRequest request = area;
	request.model_id = project.model_id;
	request.company = project.company;
	request.origin = project.origin;
	const Train *front = Train::GetIfValid(project.cutter);
	TBMConsistDimensions dimensions;
	if (front != nullptr) dimensions = MeasureTBMConsist(*front, project); // A missing cutter keeps the default InvalidConsist refusal.
	return PreviewTBMAreaWithDimensions(request, dimensions);
}
