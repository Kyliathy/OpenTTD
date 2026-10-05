/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_area.cpp Read-only project area preview through the canonical geology planner (no second planner).
 * Shows passes, turn envelope, hardness, both spoil quantities and support cost before any command accepts the area.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 4 (preview) and 5 (actual-consist adapter)
 */

#include "stdafx.h"
#include "tbm.h"
#include "train.h"

#include "safeguards.h"

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
	preview.plan = PlanExcavationRoute(request.outline, request.start, request.model_id);
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
