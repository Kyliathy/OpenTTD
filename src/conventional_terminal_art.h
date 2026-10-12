/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file conventional_terminal_art.h Read-only native truck-stop geometry for original source authoring.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups25B/25.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef CONVENTIONAL_TERMINAL_ART_H
#define CONVENTIONAL_TERMINAL_ART_H

#include <array>
#include <optional>
#include "direction_type.h"
#include "sprite.h"
#include "tile_type.h"

/** Actual native default truck-stop layout; values project the renderer rather than a replacement registry. */
struct ConventionalRoadTerminalArtGeometry {
	uint8_t graphics = 0; ///< Native truck graphics0..5; fallback IDs are unsupported.
	bool drive_through = false; ///< Four one-ended bay layouts versus two through axes.
	std::optional<DiagDirection> entrance; ///< Actual bay entrance; absent for undirected through layouts.
	std::optional<Axis> axis; ///< Actual through axis; absent for directed bay layouts.
	std::array<std::optional<SpriteBounds>, 3> parents{}; ///< Native sequence order; through layouts have no third parent.
};

/**
 * Project the actual immutable default truck-stop drawing sequence.
 * @param graphics Native default truck graphics0..5; unsupported/fallback values refuse.
 * @return Exact cell/parent geometry, or absence on an unsupported native sequence.
 */
std::optional<ConventionalRoadTerminalArtGeometry> GetConventionalRoadTerminalArtGeometry(uint8_t graphics);

/**
 * Read eligible ordinary freight terminal geometry from one real native tile.
 * @param tile Actual truck-stop tile to inspect, including profile/layer/custom-spec ownership.
 * @return Native default geometry in an active rock-layer profile, or absence for other owners.
 * Rendering eligibility grants no construction, vehicle, loading or cargo permission.
 */
std::optional<ConventionalRoadTerminalArtGeometry> GetConventionalRoadTerminalTileArtGeometry(TileIndex tile);

#endif /* CONVENTIONAL_TERMINAL_ART_H */
