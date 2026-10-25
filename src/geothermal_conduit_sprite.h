/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_conduit_sprite.h Pure original ground branches from actual paid native utility cells.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups60B/84BC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef GEOTHERMAL_CONDUIT_SPRITE_H
#define GEOTHERMAL_CONDUIT_SPRITE_H
#include "tile_type.h"
#include "gfx_type.h"
#include <array>
#include <optional>

/** Derived original ground parts only; shared native links exclusively retain installation, company, rate and resource ownership. */
struct GeothermalConduitArt {
	std::array<std::array<std::optional<SpriteID>, 4>, 2> domains{}; ///< Approved source order Heat0/Electricity1, each in native NE/SE/SW/NW direction order; absent installations have no parts.
};

/** @param tile Actual world cell, independent of camera or viewing company. @return Approved connected/capped parts for its actual installed rock-layer domains, or absence; construction previews never become installations and drawing grants no route, energy or work. */
std::optional<GeothermalConduitArt> GetGeothermalConduitArt(TileIndex tile);

/** @param tile Actual native ground cell. Draw Heat then Electricity beneath native vehicles and building parts, retaining both physical networks and all underlying map fields. */
void DrawGeothermalConduitGround(TileIndex tile);

/** @param tile Actual cell whose canonical installed topology or company just committed a change. Repaint only it and its four real same-layer neighbours so both ends of every changed branch/cap update; no installation, cache revision or resource owner changes here. */
void MarkGeothermalConduitArtDirty(TileIndex tile);
#endif /* GEOTHERMAL_CONDUIT_SPRITE_H */
