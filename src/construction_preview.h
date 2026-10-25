/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file construction_preview.h Derived world-tile overlays owned by the current placement window. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md and docs/zz-reach2/architecture/gui/archi-viewport.md */

#ifndef CONSTRUCTION_PREVIEW_H
#define CONSTRUCTION_PREVIEW_H

#include "tile_type.h"

struct Window;

/** Presentation of canonical preview geometry; these roles grant no construction, claim or work permission. */
enum class ConstructionPreviewTile : uint8_t { None, Floor, WorkingSpace, Uncovered };

/**
 * Replace the active placement window's derived overlay using producer-supplied world tiles.
 * @param owner Actual window currently owning the cursor; an inactive window cannot replace another tool's preview.
 * @param floor Canonical requested/covered final-use floor.
 * @param working_space Actual additional swept approach/turn cells from the canonical planner.
 * @param uncovered Requested cells the planner could not cover; refusal remains with the command owner.
 * @param route Canonical successive cutter centres; displayed in order starting at1, without planning or reserving work.
 * @param blocked Actual refused location, drawn as uncovered with a "!" label; INVALID_TILE for accepted plans.
 */
void SetConstructionPreview(Window *owner, std::span<const TileIndex> floor, std::span<const TileIndex> working_space, std::span<const TileIndex> uncovered = {}, std::span<const TileIndex> route = {}, TileIndex blocked = INVALID_TILE);

/** Clear derived overlays when the cursor is cancelled, replaced or its viewport switches layers. */
void ClearConstructionPreview();

/** @param tile Actual world tile being drawn. @return Its presentation role for the active placement owner only. */
ConstructionPreviewTile GetConstructionPreviewTile(TileIndex tile);

/** @param tile Actual world tile. @return Read-only numbered visits for the active preview, or empty when cancelled/unrelated. */
std::string_view GetConstructionPreviewRouteLabel(TileIndex tile);

#endif /* CONSTRUCTION_PREVIEW_H */
