/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_facility_sprite.h Physical launch-center source cells read native station/work owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JB
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md task16.2
 */
#ifndef LAUNCH_FACILITY_SPRITE_H
#define LAUNCH_FACILITY_SPRITE_H

#include "launch_sprite.h"
#include "sprite.h"
#include "tile_type.h"

/** Existing native station owns its immutable launch anchor and operational slots. */
struct Station;
/** Existing viewport tile supplies physical world position and elevation. */
struct TileInfo;
/** Original native vehicle pool object; its existing flags and custody remain authoritative. */
struct Vehicle;

/** Read the qualified source cell and original workshop's own admitted-work cue.
 * @param center Actual station owning the launch facility.
 * @param tile Physical Surface cell inside its immutable rotated footprint.
 * @param part Original back0 or near-side front1; both keep their native cell anchor.
 * @return Transient source pose, or absence for an unrelated cell or undeclared part.
 */
std::optional<LaunchArtPose> GetLaunchFacilityArtPose(const Station &center, TileIndex tile, uint8_t part);

/** Derive sorting bounds from the original cell's own native visitor zone.
 * @param center Actual station owning the launch facility.
 * @param tile Physical Surface source cell.
 * @param part Back0 sorts behind its zone's visitor; front1 covers only that native zone.
 * @return Bounds with inverse offsets preserving the exact source contact, or no valid geometry.
 */
std::optional<SpriteBounds> GetLaunchFacilityArtBounds(const Station &center, TileIndex tile, uint8_t part);

/** Draw one real launch tile through the ordinary ground and sortable sprite owners.
 * @param tile Native viewport tile; no placement, actor, inventory or work is created.
 */
void DrawLaunchFacilityTile(const TileInfo &tile);

/** Admit the original hidden Surface rocket to drawing and native selection only.
 * @param vehicle Actual native vehicle; every unrelated role retains its existing Hidden behavior.
 * @return Whether the same model14 hull occupies its exact current pad/recovery cell with qualified source coverage.
 */
bool IsLaunchSurfaceCraftArtVisible(const Vehicle &vehicle);

/** Refresh only changed facility cues on the existing GUI update path, including paused commands.
 * Derived observer cache values grant no work, cargo, identity or simulation authority.
 */
void RefreshLaunchFacilityArt();

/** Keep the rocket's accepted contact at its native body position without changing sorting extents.
 * @param vehicle Existing native hull; other roles retain their exact original bounds.
 * @return Presentation bounds with only the qualified model14's inverse XY drawing offset.
 */
SpriteBounds GetLaunchCraftArtBounds(const Vehicle &vehicle);

#endif /* LAUNCH_FACILITY_SPRITE_H */
