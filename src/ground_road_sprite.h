/**
 * @file ground_road_sprite.h Source-backed conventional road carrier presentation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FF.
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
 */

#ifndef GROUND_ROAD_SPRITE_H
#define GROUND_ROAD_SPRITE_H

#include "ground_sprite.h"

struct RoadVehicle;

/**
 * Resolve an actual native road carrier's current source view without changing its operation.
 * @param vehicle Actual EIDS-owned physical part; native packets/capacity retain load ownership.
 * @param direction Existing renderer's body direction.
 * @param visual_ticks Synchronized drawing phase; no movement, work or random draw is earned.
 * @return Published model7/8 source body, or absence outside qualified native capability/cargo/layers.
 */
std::optional<SpriteID> GetGroundRoadSprite(const RoadVehicle &vehicle, Direction direction, uint64_t visual_ticks);

/**
 * Select an authored road purchase cue without changing its existing admission predicate.
 * @param model Caller-resolved canonical road model7 or8.
 * @param available Existing company/engine purchase result.
 * @return Available/locked source preview, or absence for Classic/unselected designs.
 */
std::optional<SpriteID> GetGroundRoadPurchaseSprite(uint16_t model, bool available);

/**
 * Refresh derived art caches only on an actual ordinary front-engine tick.
 * @param head Existing native road consist front; other physical parts are refused.
 * @param visual_ticks Existing synchronized phase clock, sampled at authored eight-tick boundaries.
 */
void RefreshGroundRoadArt(RoadVehicle &head, uint64_t visual_ticks);

#endif /* GROUND_ROAD_SPRITE_H */
