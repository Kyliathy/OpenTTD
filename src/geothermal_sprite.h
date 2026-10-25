/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_sprite.h Pure authored generator/riser presentation from actual native paid machinery.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BB.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8/1CB-1CD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef GEOTHERMAL_SPRITE_H
#define GEOTHERMAL_SPRITE_H
#include "tile_type.h"
#include "sprite.h"
#include <optional>

/** @param tile Actual native generator cell on its original Deep footprint. @param front Authored foreground rather than ground/back part. @param visual_ticks Presentation-only lamp clock, never construction/service progress. @return Existing packed original model/view/cell/state sprite, or absence for foreign/missing native participants; queries change no quantity, binding, RNG or world clock. */
std::optional<SpriteID> GetGeothermalGeneratorSprite(TileIndex tile, bool front, uint64_t visual_ticks);

/** @param tile Actual paid native riser cell on its original rock-side footprint. @param front Authored empty front rather than the solid machinery back. @param visual_ticks Presentation-only equipment-glass clock, never paid work or electrical flow. @return Original packed fixed-view/cell/construction/connection/flow sprite, or absence for unrelated machinery; no energy, link or construction authority is acquired. */
std::optional<SpriteID> GetGeothermalRiserSprite(TileIndex tile, bool front, uint64_t visual_ticks);

/** @param tile Actual native riser cell. @return Per-cell sorting bounds for the original opaque riser back, whose source contract deliberately has an empty front; absence preserves the existing ground-back path for other industries. */
std::optional<SpriteBounds> GetGeothermalIndustryBackBounds(TileIndex tile);

/** @param tile Actual native generator cell with a valid authored selector. @return Per-cell foreground bounds covering its approved64-pixel native source canvas, or absence for unrelated legacy/foreign industries; riser fronts are empty, and these bounds change only sorting, never the physical footprint. */
std::optional<SpriteBounds> GetGeothermalIndustryFrontBounds(TileIndex tile);
#endif /* GEOTHERMAL_SPRITE_H */
