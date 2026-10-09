/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_replanning.h Read-only, revision-bound work extension for the actual paused cutter. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group3B */

#ifndef TBM_REPLANNING_H
#define TBM_REPLANNING_H

#include "tbm.h"

/** Transient additional-work quote; the existing geological job and native train retain all saved authority. */
struct TBMExtensionPreview {
	CommandCost result; ///< Zero immediate cost on success; later cutting still pays its own work and running costs.
	VehicleID cutter = VehicleID::Invalid(); ///< Actual machine quoted, for explicit player-facing identity.
	ExcavationOutline outline; ///< Actual requested area and accepted old/new working space.
	ExcavationRouteProposal plan; ///< Only additional canonical cuts beyond the old accepted endpoint.
	std::vector<TileIndex> working_space; ///< Swept extension cells outside the additional final floor.
	uint64_t job_revision = 0; ///< Exact geological revision reviewed, including partial work.
	uint64_t fingerprint = 0; ///< Geometry, quantities and controller identity accepted by confirmation.
};

/**
 * Quote additional rectangular work for a real paused unfinished project.
 * @param company Actual requesting company; the query restores the caller's command context.
 * @param project Actual existing controller, never a request to buy another train.
 * @param area_start First explicit rock-layer floor corner.
 * @param area_end Opposite floor corner on that same layer.
 * @param intent Area coverage or straight continuation from the accepted endpoint supplied as area_start.
 * @return Complete additional-work quote or actionable refusal, with all saved owners unchanged.
 */
TBMExtensionPreview PreviewTBMExtension(CompanyID company, TBMProjectID project, TileIndex area_start, TileIndex area_end, ExcavationPlanIntent intent = ExcavationPlanIntent::Area);

#endif /* TBM_REPLANNING_H */
