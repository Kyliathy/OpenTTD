/**
 * @file ground_processor_sprite.h Selected Surface processor parts from actual recipe/inventory owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef GROUND_PROCESSOR_SPRITE_H
#define GROUND_PROCESSOR_SPRITE_H

#include "ground_sprite.h"

/**
 * Query one actual selected processor's construction and native operating availability without earning work.
 * @param tile Actual native Surface machinery cell; foreign, unbound or unsupported geometry refuses.
 * @param front True for its aligned foreground, false for the original source back/ground part.
 * @param visual_ticks Shared read-only source phase, never recipe work, delivered cargo or construction progress.
 * @return Qualified original source part for one of the four selected16-cell facilities, or absence.
 */
std::optional<SpriteID> GetGroundProcessorSprite(TileIndex tile, bool front, uint64_t visual_ticks);

#endif /* GROUND_PROCESSOR_SPRITE_H */
