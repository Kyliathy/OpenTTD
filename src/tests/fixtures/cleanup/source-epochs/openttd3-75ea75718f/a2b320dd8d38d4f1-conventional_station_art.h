/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file conventional_station_art.h Read-only default rail station geometry for original source authoring.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef CONVENTIONAL_STATION_ART_H
#define CONVENTIONAL_STATION_ART_H

#include <array>
#include <optional>
#include "direction_type.h"
#include "sprite.h"
#include "tile_type.h"

/** Native default station roles; axis remains the low bit of StationGfx. */
enum class ConventionalRailStationArtRole : uint8_t {
	Platform, ///< Bare rear/front platform strips.
	LoadingBuilding, ///< Native small building on the rear platform.
	CanopyRear, ///< Native first canopy half with rear pillars and elevated roof.
	CanopyFront, ///< Native second canopy half with front pillars and elevated roof.
};

/** Independent native sorting owners; an elevated roof must not become a low foreground wall. */
enum class ConventionalRailStationArtPart : uint8_t {
	Back, ///< Rear platform, building or pillar parent.
	Front, ///< Near platform or pillar parent.
	Roof, ///< Elevated native canopy parent; its glass remains a separately described attached child.
};

/** Native screen-space child data; glass stays attached to its roof and owns no independent sorting box. */
struct ConventionalRailStationArtChild {
	Point screen_offset{}; ///< Actual signed pixel offset passed to AddChildSpriteScreen.
	bool transparent = false; ///< Actual native image transparency modifier, independent of source alpha.
	PaletteID palette = PAL_NONE; ///< Actual native glass palette retained by DrawCommonTileSeq.
};

/**
 * Value projection of the actual immutable default station drawing layout.
 * Bounds come from GetStationTileLayout, in native world units, rather than
 * another station footprint or a private replacement layout registry.
 */
struct ConventionalRailStationArtGeometry {
	uint8_t graphics = 0; ///< Actual native StationGfx0..7, including the axis bit.
	Axis axis = Axis::X; ///< Undirected railway axis; reversing a train creates no new view.
	ConventionalRailStationArtRole role = ConventionalRailStationArtRole::Platform; ///< Native graphics pair's physical role.
	std::array<std::optional<SpriteBounds>, 3> parts{}; ///< Actual back/front/optional elevated roof bounds.
	uint8_t roof_children = 0; ///< Native child sprites attached to the roof parent; no independent box or stock owner.
	std::optional<ConventionalRailStationArtChild> roof_glass; ///< Actual canopy glass child, absent on bare/building roles.
};

/**
 * Read the actual default rail layout without touching a world or its owners.
 * @param graphics Native default StationGfx0..7; custom/fallback values refuse.
 * @return Exact modular cell/part geometry, or absence if the native sequence
 * no longer matches these supported default roles. No sprites are activated.
 */
std::optional<ConventionalRailStationArtGeometry> GetConventionalRailStationArtGeometry(uint8_t graphics);

/**
 * Resolve eligible ordinary rock-layer station geometry from a real native tile.
 * @param tile Actual tile to inspect, with its native type, axis and custom spec.
 * @return Geometry for a default conventional rail station in the active new
 * profile's Underground/Deep layer, or absence for other owners. Rendering
 * selection grants no construction, movement, loading or cargo permission.
 */
std::optional<ConventionalRailStationArtGeometry> GetConventionalRailStationTileArtGeometry(TileIndex tile);

#endif /* CONVENTIONAL_STATION_ART_H */
