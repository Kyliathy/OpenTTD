/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_sprite.h Read-only launch graphics selected from existing native owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62J
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md task16.2
 */
#ifndef LAUNCH_SPRITE_H
#define LAUNCH_SPRITE_H

#include "direction_type.h"
#include "table/sprites.h"
#include <optional>

/** Actual native hull; graphics never own a second flight or packet compartment. */
struct SpaceVehicle;

/** Transient source indices in the qualified state/frame/view/cell/part order. */
struct LaunchArtPose {
	LaunchFleetSpriteAsset asset = LaunchFleetSpriteAsset::VehicleModel14; ///< Qualified source family.
	uint8_t state = 0; ///< Original controller state, without saved art state.
	uint8_t frame = 0; ///< Existing countdown phase or explicit static source alias.
	uint8_t direction = 0; ///< Source view index; facility views and rocket headings have different counts.
	uint8_t tile = 0; ///< Physical source cell within the existing footprint.
	uint8_t part = 0; ///< Original back/front part, or the rocket's sole whole body.
};

/** Resolve only a declared pose inside its qualified packed range.
 * @param pose Explicit source family and bounded presentation indices.
 * @return Exact native sprite, or no sprite for undeclared source coverage.
 */
inline std::optional<SpriteID> ResolveLaunchSprite(const LaunchArtPose &pose)
{
	const auto index = static_cast<size_t>(pose.asset);
	if (index >= _launch_fleet_sprite_ranges.size()) return std::nullopt;
	const auto &range = _launch_fleet_sprite_ranges[index];
	// Missing poses must not wrap into neighbouring cells or another family;
	// a valid-looking native ID cannot replace the accepted physical coverage.
	if (pose.state >= range.states || pose.frame >= range.frames || pose.direction >= range.directions || pose.tile >= range.tiles || pose.part >= range.parts) return std::nullopt;
	const uint32_t offset = ((((pose.state * range.frames + pose.frame) * range.directions + pose.direction) * range.tiles + pose.tile) * range.parts + pose.part);
	if (offset >= range.count) return std::nullopt;
	return SPR_MAP_LAYER_BASE + range.offset + offset;
}

/** Select the genuine rocket heading from actual custody and native movement geometry.
 * @param craft Original native model14 hull; other roles retain their native facing.
 * @return Source heading, without changing direction, position or flight state.
 */
Direction GetLaunchRocketArtDirection(const SpaceVehicle &craft);

/** Select model14 source state and frame from the same paid flight owner.
 * @param craft Actual native hull with its original optional transition.
 * @param direction Genuine source heading requested by the current image context.
 * @return Qualified whole-body pose, or absence for another role or invalid enum.
 */
std::optional<LaunchArtPose> GetLaunchRocketArtPose(const SpaceVehicle &craft, Direction direction);

#endif /* LAUNCH_SPRITE_H */
