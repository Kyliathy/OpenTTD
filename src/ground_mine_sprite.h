/**
 * @file ground_mine_sprite.h Source-backed selected mine presentation from actual native owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef GROUND_MINE_SPRITE_H
#define GROUND_MINE_SPRITE_H

#include "ground_sprite.h"

/**
 * Read one actual selected mine cell through its native capability, construction and finite-output owners.
 * @param tile Actual machinery cell; invalid, foreign, unbound or unsupported native geometry refuses.
 * @param front True for the exact aligned foreground, false for its source back/ground part.
 * @param visual_ticks Read-only shared visual phase; never production time, reserve debit or paid construction work.
 * @return Qualified native source part, or absence outside the seven delivered mine families.
 */
std::optional<SpriteID> GetGroundMineSprite(TileIndex tile, bool front, uint64_t visual_ticks);

#endif /* GROUND_MINE_SPRITE_H */
