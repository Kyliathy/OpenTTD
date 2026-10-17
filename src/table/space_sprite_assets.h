/** @file space_sprite_assets.h Generated final Space native ranges.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG
 */
#ifndef SPACE_SPRITE_ASSETS_H
#define SPACE_SPRITE_ASSETS_H

#include <array>
#include <cstdint>

/** Existing source families in fixed append order; no saved gameplay identity. */
enum class SpaceSpriteAsset : uint8_t {
	VehicleModel15, ///< vehicle.model-15
	VehicleModel19, ///< vehicle.model-19
	VehicleModel20, ///< vehicle.model-20
	VehicleModel18, ///< vehicle.model-18
	AsteroidNickel, ///< asteroid.nickel
	AsteroidIce, ///< asteroid.ice
	AsteroidRareEarth, ///< asteroid.rare-earth
	End, ///< Number of final Space art families.
};

/** Pure native range; pose order is state/frame/direction/saved-size. */
struct SpaceSpriteRange {
	uint16_t offset; ///< Relative native Map Layer block offset.
	uint16_t count; ///< Complete logical source pose count.
	uint8_t states; ///< Published visual states.
	uint8_t frames; ///< Authored phases and explicit static aliases.
	uint8_t directions; ///< Genuine original engine views.
	uint8_t sizes; ///< Saved asteroid size variants, or one for craft.
};

static constexpr uint16_t SPACE_SPRITE_START = 3599; ///< All original M0/M1/preview slots precede this append.
static constexpr uint16_t SPACE_SPRITE_END = 5167; ///< Exclusive final native Space sprite bound.
static constexpr std::array<SpaceSpriteRange, 7> _space_sprite_ranges = {{
	{3599, 128, 8, 2, 8, 1}, // vehicle.model-15
	{3727, 96, 6, 2, 8, 1}, // vehicle.model-19
	{3823, 128, 8, 2, 8, 1}, // vehicle.model-20
	{3951, 64, 4, 2, 8, 1}, // vehicle.model-18
	{4015, 384, 4, 4, 8, 3}, // asteroid.nickel
	{4399, 384, 4, 4, 8, 3}, // asteroid.ice
	{4783, 384, 4, 4, 8, 3}, // asteroid.rare-earth
}};

#endif /* SPACE_SPRITE_ASSETS_H */
