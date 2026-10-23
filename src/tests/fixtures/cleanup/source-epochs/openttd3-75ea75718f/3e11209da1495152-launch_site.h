/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_site.h Company-independent geometry shared by real launch placement and field generation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group3
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
 */
#ifndef LAUNCH_SITE_H
#define LAUNCH_SITE_H
#include "tile_type.h"

/** Native command result; geometry queries return no construction or clearing charge. */
class CommandCost;

inline constexpr uint8_t LAUNCH_CENTER_LONG_EDGE = 4; ///< Native long footprint extent, shared with saved initial-site bounds.
inline constexpr uint8_t LAUNCH_CENTER_SHORT_EDGE = 3; ///< Native short footprint extent, shared with saved initial-site bounds.

/**
 * Check one footprint tile using the actual launch command's terrain/obstruction rules.
 * This grants no company, technology, town, station-joining or clearing permission.
 * @param tile Explicit Surface footprint tile; invalid/cross-layer tiles refuse before inspection.
 * @param reference_height Required flat land elevation shared by the complete footprint.
 * @return Native placement refusal, or success with zero cost and no mutation.
 */
CommandCost GetLaunchCenterTerrainTileBlock(TileIndex tile, int reference_height);

/**
 * Check an entire rotated site before companies exist during new-world generation.
 * Uses the same guarded footprint, orthogonal navigable-water and terrain producers as real placement.
 * Later construction still requires all command-specific ownership, date, town and clearing checks.
 * @param anchor Surface build anchor, never a camera tile or another layer's projection.
 * @param rotation Quarter-turn layout index in [0, 4).
 * @return Native geometry refusal, or success with zero cost and no mutation.
 */
CommandCost GetLaunchCenterSiteGeometryBlock(TileIndex anchor, uint8_t rotation);

#endif /* LAUNCH_SITE_H */
