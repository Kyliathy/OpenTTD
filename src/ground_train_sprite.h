/**
 * @file ground_train_sprite.h Read-only selected conventional rail source binding.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FE.
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
 */

#ifndef GROUND_TRAIN_SPRITE_H
#define GROUND_TRAIN_SPRITE_H

#include "ground_sprite.h"

/**
 * Resolve an actual selected native mineral wagon without granting operation.
 * @param train Actual EIDS-owned physical wagon; packets and capacity own load.
 * @param direction Existing renderer's body view, including its flip policy.
 * @param visual_ticks Synchronized presentation clock; drawing earns no movement or work.
 * @return Published model5/6 body, or absence outside qualified native capability/cargo/geometry.
 */
std::optional<SpriteID> GetGroundTrainSprite(const Train &train, Direction direction, uint64_t visual_ticks);

/**
 * Resolve one selected rail catalogue preview without changing purchase admission.
 * @param model Actual caller-resolved native model5 or6; other designs are refused.
 * @param available Existing company/engine purchase predicate, used only for its visible cue.
 * @return Available/locked authored preview, or absence for Classic/unselected families.
 */
std::optional<SpriteID> GetGroundRailPurchaseSprite(uint16_t model, bool available);

#endif /* GROUND_TRAIN_SPRITE_H */
