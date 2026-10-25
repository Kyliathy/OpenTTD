/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_area_preview.h Exact read-only commissioning-area/material quote from the TBM owner. @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md and r2mlvs7-underground-construction-and-fleet.md */

#ifndef TBM_AREA_PREVIEW_H
#define TBM_AREA_PREVIEW_H

#include "tbm.h"

/**
 * Add one identity or quoted quantity to a portable transient quote fingerprint.
 * @param hash Existing canonical geometry fingerprint, updated in place.
 * @param value Stable value encoded in an explicit byte order, independent of native structure padding.
 */
inline void MixTBMQuoteFingerprint(uint64_t &hash, uint64_t value)
{
	// Each peer must recognize the same reviewed plan. This presentation and
	// command token neither allocates an identity nor stores another work state.
	for (uint byte = 0; byte < 8; ++byte) {
		hash = (hash ^ (value & 0xff)) * 1099511628211ULL;
		value >>= 8;
	}
}

/**
 * Form the bounded rectangular request and its explicit one-cell working ring.
 * @param anchor Original rock-side housing, excluded from requested final-use floor.
 * @param area_start First explicit floor corner on the anchor's layer.
 * @param area_end Opposite floor corner.
 * @param previous_work Already accepted work retained when extending an existing job.
 * @return Canonical floor/working outline or refusal; material and physical admission stay with their normal quote owners.
 */
ExcavationOutline MakeTBMWorkOutline(TileIndex anchor, TileIndex area_start, TileIndex area_end, std::span<const TileIndex> previous_work = {});

/**
 * Form an exact straight centreline and its full cutter-body working ring.
 * @param start Actual proposed starting centre, excluded from additional final floor.
 * @param heading Current real or accepted-route heading; no implicit turn is allowed.
 * @param end Chosen centre endpoint ahead on that same heading and layer, at most64 tiles away.
 * @param previous_work Already accepted approach/tail authorization retained for extension.
 * @return Canonical requested floor/working outline or unchanged refusal.
 */
ExcavationOutline MakeTBMTunnelOutline(TileIndex start, Direction heading, TileIndex end, std::span<const TileIndex> previous_work = {});

/**
 * Summarize the approach a quoted route actually digs: the cells it sweeps among the approach proposal's extra cells.
 * The authorization may hold a few more cells the final route does not need; those are neither dug nor charged.
 * @param route The quote's route.
 * @param extra_tiles Canonical extra cells of the approach proposal.
 * @param model_id Cutter model, for the cells' quote.
 * @param own_job Job whose remainder is being replaced, or an invalid ID: its own unopened claimed cells are quoted as
 *                the rock they are (r2df task 12.4).
 * @return The cells, the cutter positions that use them and their quote; empty without extra cells.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23CB
 */
TBMApproachSummary SummarizeTBMApproach(std::span<const ExcavationCutGeometry> route, std::span<const TileIndex> extra_tiles, uint16_t model_id, ExcavationJobID own_job = ExcavationJobID{UINT32_MAX});

/**
 * Short reason for an approach the planner could not find.
 * @param error The approach refusal.
 * @param tile Its failing tile.
 * @return "Would leave the map", "a building is in the way", "another company's or project's work", "too far to plan",
 *         or the general route refusal.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23CB
 */
StringID GetTBMApproachRefusal(ExcavationError error, TileIndex tile);

/**
 * Form the rectangle's floor and ring as MakeTBMWorkOutline does, then add the approach and turning space the cutter
 * needs from its start pose. A floor away from the housing is allowed: the approach joins them.
 * @param start Cutter start pose: the housing with the measured planning points, or the pose after an accepted route.
 * @param solid_start Whether the start may be an unopened portal housing (a commission).
 * @param model_id Cutter model.
 * @param continuing_job The job being extended, or an invalid ID for a commission.
 * @param ring_anchor Housing whose 3x3 belongs to the working space.
 * @param area_start First explicit floor corner on the anchor's layer.
 * @param area_end Opposite floor corner.
 * @param previous_work Already accepted work retained when extending an existing job.
 * @param[out] approach The approach proposal: its extra cells, route and expansions, or its refusal and blocking tile.
 * @param kept_cuts Accepted cuts of the continuing job that precede the new work: all for an extension, the completed
 *                  ones for a change of work area (r2df task 12.4).
 * @return Floor and connected authorization (floor, ring, housing, earlier work and approach), unquoted like
 *         MakeTBMWorkOutline, or a refusal that keeps the requested floor; nothing is reserved or charged.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23CB
 */
ExcavationOutline MakeTBMWorkOutlineWithApproach(const ExcavationPlanningPose &start, bool solid_start, uint16_t model_id, ExcavationJobID continuing_job,
	TileIndex ring_anchor, TileIndex area_start, TileIndex area_end, std::span<const TileIndex> previous_work, ExcavationApproachProposal &approach, uint32_t kept_cuts = UINT32_MAX);

/**
 * Form a tunnel from a start pose to an end: straight when the end lies exactly ahead (as the overload above),
 * otherwise an angled tunnel along an exact-goal approach, whose cutter centres are the floor and whose sweeps the work.
 * @param start Cutter start pose: the housing with the measured planning points, or the pose after an accepted route.
 * @param solid_start Whether the start may be an unopened portal housing (a commission).
 * @param model_id Cutter model.
 * @param continuing_job The job being extended, or an invalid ID for a commission.
 * @param end Chosen centre endpoint on the start's layer, at most 64 tiles away.
 * @param previous_work Already accepted approach/tail authorization retained for extension.
 * @param[out] approach The angled tunnel's approach; empty for a straight one.
 * @param kept_cuts Accepted cuts of the continuing job that precede the new work, as for the area builder.
 * @return Canonical requested floor/working outline or refusal.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23CB
 */
ExcavationOutline MakeTBMTunnelOutline(const ExcavationPlanningPose &start, bool solid_start, uint16_t model_id, ExcavationJobID continuing_job,
	TileIndex end, std::span<const TileIndex> previous_work, ExcavationApproachProposal &approach, uint32_t kept_cuts = UINT32_MAX);

/** Transient quote; no identities, terrain or inventories are allocated. */
struct TBMCommissionAreaPreview {
	CommandCost purchase; ///< Full machinery/wagon command estimate, separate from later geological work charges.
	TBMAreaPreview area; ///< Canonical floor, route, turn space, pass terms and Earth/Rock/work/support costs.
	LayerPortalCommissionRequest request; ///< Owner-derived request; execution revalidates every authority.
	uint64_t fingerprint = 0; ///< Exact reviewed geometry, price and reuse identity; no saved state.
};

/**
 * Quote the same rectangular floor and whole consist as the commissioning command.
 * @param company Actual requesting company; must match the current command actor, independently of the camera.
 * @param portal Actual upper rail housing used for purchase.
 * @param model_id Profile-owned cutter model.
 * @param earth_wagons Earth hopper count.
 * @param rock_wagons Rock hopper count.
 * @param origin Actual owned Surface station with spoil receivers.
 * @param area_start First selected world-tile corner on the portal's rock layer.
 * @param area_end Opposite world-tile corner on that same layer.
 * @param accept_partial Explicit acceptance policy for a partially coverable floor.
 * @param intent Area coverage or exact straight tunnel; straight input starts at the rock-side housing.
 * @return Full unchanged quote or refusal; returned geometry grants no mutation permission.
 */
TBMCommissionAreaPreview PreviewTBMCommissionArea(CompanyID company, TileIndex portal, uint16_t model_id,
	uint8_t earth_wagons, uint8_t rock_wagons, StationID origin, TileIndex area_start, TileIndex area_end, bool accept_partial, ExcavationPlanIntent intent = ExcavationPlanIntent::Area);

#endif /* TBM_AREA_PREVIEW_H */
