/**
 * @file launch_fleet_sprite_assets.h Generated launch/fleet native graphics ranges; do not edit.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62I
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */
#ifndef LAUNCH_FLEET_SPRITE_ASSETS_H
#define LAUNCH_FLEET_SPRITE_ASSETS_H

#include <array>
#include <cstdint>

/** Selected M1 graphics identities; saved engine/cargo/industry IDs stay with their existing owners. */
enum class LaunchFleetSpriteAsset : uint8_t {
	FacilityLaunchCenter, ///< facility.launch-center.
	VehicleModel14, ///< vehicle.model-14.
	End, ///< Number of selected family declarations.
};

/** One complete native family range. Pose order is state/frame/direction/tile/part. */
struct LaunchFleetSpriteRange {
	uint16_t offset; ///< Relative native Action5 type1C offset.
	uint16_t count; ///< Exact physical source pose count.
	uint8_t states; ///< Public state count.
	uint8_t frames; ///< Authored visual phase/texture count.
	uint8_t directions; ///< Engine view count.
	uint16_t tiles; ///< Physical tile variants or independent portal endpoints.
	uint8_t parts; ///< Physical back/front or vehicle part count.
};

static constexpr uint16_t LAUNCH_FLEET_SPRITE_START = 6143; ///< Every prior native sprite precedes this append.
static constexpr uint16_t LAUNCH_FLEET_SPRITE_END = 7791; ///< Exclusive selected native block bound.
static constexpr std::array<LaunchFleetSpriteRange, 2> _launch_fleet_sprite_ranges = {{
	{6143, 1536, 4, 4, 4, 12, 2}, // facility.launch-center
	{7679, 112, 7, 2, 8, 1, 1}, // vehicle.model-14
}};

#endif /* LAUNCH_FLEET_SPRITE_ASSETS_H */
