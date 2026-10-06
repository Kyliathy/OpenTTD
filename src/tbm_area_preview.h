/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_area_preview.h Exact read-only commissioning-area/material quote from the TBM owner. @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md and r2mlvs7-underground-construction-and-fleet.md */

#ifndef TBM_AREA_PREVIEW_H
#define TBM_AREA_PREVIEW_H

#include "tbm.h"

/** Transient quote; no identities, terrain or inventories are allocated. */
struct TBMCommissionAreaPreview {
	CommandCost purchase; ///< Full machinery/wagon command estimate, separate from later geological work charges.
	TBMAreaPreview area; ///< Canonical floor, route, turn space, pass terms and Earth/Rock/work/support costs.
	LayerPortalCommissionRequest request; ///< Owner-derived request; execution revalidates every authority.
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
 * @return Full unchanged quote or refusal; returned geometry grants no mutation permission.
 */
TBMCommissionAreaPreview PreviewTBMCommissionArea(CompanyID company, TileIndex portal, uint16_t model_id,
	uint8_t earth_wagons, uint8_t rock_wagons, StationID origin, TileIndex area_start, TileIndex area_end, bool accept_partial);

#endif /* TBM_AREA_PREVIEW_H */
