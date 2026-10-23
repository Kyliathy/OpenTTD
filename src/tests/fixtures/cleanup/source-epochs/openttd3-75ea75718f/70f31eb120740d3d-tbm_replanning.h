/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_replanning.h Read-only, revision-bound work extension for the actual paused cutter. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group3B; docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 9 (approaches), Group 12 (change work area) */

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
	TBMApproachSummary approach; ///< The approach and turning space the new cuts dig beyond the new floor, its ring and earlier work.
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

/** Transient change-work quote (r2df Group 12): the unfinished remainder replaced from the last completed cut; nothing is allocated. */
struct TBMWorkChangePreview {
	CommandCost result; ///< Zero immediate cost on success, or the player's refusal message.
	VehicleID cutter = VehicleID::Invalid(); ///< Actual machine quoted.
	ExcavationOutline outline; ///< New floor, ring and approach.
	ExcavationRouteProposal plan; ///< New cuts from the last completed cut, with their quantities.
	std::vector<TileIndex> working_space; ///< New swept cells outside the new floor.
	TBMApproachSummary approach; ///< Approach and turning space the new cuts dig beyond the new floor, its ring and the completed work.
	ExcavationLoadStart load; ///< Spoil aboard when quoted; the command measures it again when it executes.
	uint32_t cursor = 0; ///< Completed cuts kept; the quote is bound to this count.
	uint32_t dropped_cuts = 0; ///< Planned cuts that will no longer be made.
	uint64_t abandoned_work_ticks = 0; ///< Work on the cut in progress that a different first new cut discards, at quote time.
	uint64_t cut_work_ticks = 0; ///< Work the cut in progress needs in total, to state the abandoned share.
	TBMSupportPlan face_support; ///< Junction piece the face needs when its laid piece points away from the new first cut.
	uint64_t fingerprint = 0; ///< Reviewed geometry, quantities, cursor and participants; not the moving partial work.
};

/**
 * Quote replacing the unfinished remainder of a live project with a new area, without pausing it (r2df task 12.1).
 * Working, paused, returning and parked projects are accepted; finished or releasing ones, and one whose machine has
 * not completed a cut yet, are refused. Completed cuts, ground, cargo and money stay.
 * @param company Actual requesting company; the query restores the caller's command context.
 * @param project Actual existing controller.
 * @param area_start First new floor corner, or the last completed cut's position for a straight tunnel.
 * @param area_end Opposite floor corner, or the tunnel end.
 * @param intent Area coverage or straight tunnel.
 * @return Complete change quote or actionable refusal, with all saved owners unchanged.
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 12
 */
TBMWorkChangePreview PreviewTBMWorkChange(CompanyID company, TBMProjectID project, TileIndex area_start, TileIndex area_end, ExcavationPlanIntent intent = ExcavationPlanIntent::Area);

#endif /* TBM_REPLANNING_H */
