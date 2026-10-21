/**
 * @file ground_sprite.h Pure typed selection of qualified selected-ground source poses.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Gameplay, inventory, capability, construction and visual-state authority remain
 * with the caller's existing native owner. This API resolves published art only.
 */

#ifndef GROUND_SPRITE_H
#define GROUND_SPRITE_H

#include "multilayer_sprite.h"

/**
 * Resolve one exact qualified source tuple without querying or changing a world.
 * @param family Published append-local art family; it is never a gameplay model or industry ID.
 * @param pose Existing transient state/frame/view/cell/part indices, derived by the actual consumer.
 * @return Exact published native sprite, or absence for an undeclared family or pose axis.
 */
inline std::optional<SpriteID> GetGroundSprite(GroundSpriteFamily family, const MultiLayerSpritePose &pose)
{
	const size_t index = static_cast<size_t>(family);
	if (index >= _ground_sprite_ranges.size()) return std::nullopt;
	const auto &range = _ground_sprite_ranges[index];
	// Invalid source axes must not wrap into another cell, phase, parent or
	// family. Exact bounds preserve native foregrounds and meaningful load cues;
	// resolving available art never grants movement or operating permission.
	if (pose.state >= range.states || pose.frame >= range.frames || pose.direction >= range.directions ||
		pose.tile >= range.tiles || pose.part >= range.parts) return std::nullopt;
	const uint offset = ((((pose.state * range.frames + pose.frame) * range.directions + pose.direction) * range.tiles + pose.tile) * range.parts + pose.part);
	return SPR_MAP_LAYER_BASE + range.begin + offset;
}

#endif /* GROUND_SPRITE_H */
