/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file transport_tube_facility_sprite.h Retained native geometry and the donor's read-only facility animation clock.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CC.
 * Merge46imports the existing animation lifecycle needed by original ANIT; packed sprite selection remains Merge51.
 */
#ifndef TRANSPORT_TUBE_FACILITY_SPRITE_H
#define TRANSPORT_TUBE_FACILITY_SPRITE_H
#include "transport_tube.h"

/** Inspect existing native facility geometry independently of today's construction date.
 * @param tile Actual excavated rock-layer platform/depot, never projected through a view.
 * @param company Actual current owner; foreign/orphan/ordinary identities refuse.
 * @return Existing physical geometry only; no construction, operation, cargo or power permission.
 */
std::optional<TubeTerminal> GetBuiltTubeTerminal(TileIndex tile, CompanyID company);

/** Join the existing bounded animation owner only for actual default tube facilities.
 * @param tile Native world cell revalidated through the sole physical geometry owner.
 */
void RegisterTubeFacilityArtAnimation(TileIndex tile);

/** Refresh the donor's existing eight-tick display clock without a gameplay opportunity.
 * @param tile Original animated facility cell, checked again after removal/conversion.
 * @return True for supported native geometry; false lets its tile owner retire obsolete display registration.
 */
bool AnimateTubeFacilityArt(TileIndex tile);
#endif /* TRANSPORT_TUBE_FACILITY_SPRITE_H */
