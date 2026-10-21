/**
 * @file ground_header_contract.cpp Compile actual selected-ground producer dimensions and range ABI.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79D.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "ground_sprite_assets.h"
#include <cstddef>
#include <type_traits>

static_assert(std::is_same_v<std::underlying_type_t<GroundSpriteFamily>, uint8_t>);
static_assert(std::is_same_v<decltype(GroundSpriteRange::begin), uint16_t>);
static_assert(GROUND_SPRITE_START == LAUNCH_FLEET_SPRITE_END);
static_assert(GROUND_SPRITE_START == 7791 && GROUND_SPRITE_END == 22045);
static_assert(_ground_sprite_ranges.size() == 47);

/** Verify native cell/phase/parent dimensions and exact cardinality for a source family.
 * @param family Generated append-local identity, never a gameplay model/industry ID.
 * @param tiles Actual physical cell/variant count from the qualified producer.
 * @param states Native presentation states; they carry no simulation authority.
 * @param frames Genuine phase count including required static aliases.
 * @param directions Actual independently declared views.
 * @param parts Independently parented components in the source matrix.
 * @return Whether all typed dimensions and the range cardinality match their producer.
 */
constexpr bool Matches(GroundSpriteFamily family, uint16_t tiles, uint8_t states, uint8_t frames, uint8_t directions, uint8_t parts)
{
	const auto &range = _ground_sprite_ranges[static_cast<std::size_t>(family)];
	return range.tiles == tiles && range.states == states && range.frames == frames && range.directions == directions &&
		range.parts == parts && range.count == tiles * states * frames * directions * parts;
}

static_assert(Matches(GroundSpriteFamily::MineCOAL, 9, 4, 4, 1, 2));
static_assert(Matches(GroundSpriteFamily::FacilityElectronicsWorks, 16, 4, 4, 1, 2));
static_assert(Matches(GroundSpriteFamily::FacilityUtilityRiser, 4, 4, 4, 1, 2));
static_assert(Matches(GroundSpriteFamily::FacilityRailStation, 4, 4, 4, 2, 4));
static_assert(Matches(GroundSpriteFamily::TubeJunction, 54, 3, 1, 1, 2));
