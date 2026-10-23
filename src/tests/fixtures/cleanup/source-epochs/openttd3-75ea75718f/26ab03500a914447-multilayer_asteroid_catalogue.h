/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file multilayer_asteroid_catalogue.h Immutable profile-owned population, class-share and initial placement records.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Groups2-4
 */
#ifndef MULTILAYER_ASTEROID_CATALOGUE_H
#define MULTILAYER_ASTEROID_CATALOGUE_H
#include "multilayer_content.h"

/** Exact population row for all supported rectangles with the same tile-area power. */
struct MultiLayerAsteroidPopulationDefinition {
	uint8_t area_log2; ///< Base-two logarithm of Surface tile area; supported rows12..24.
	std::array<uint32_t, 3> counts; ///< Exact Sparse/Normal/Dense counts, with no density approximation.
};

/** One saved class's immutable population share, separate from reserve and independently sampled size. */
struct MultiLayerAsteroidClassShare {
	uint8_t class_index; ///< Existing saved Metallic/Carbonaceous/Rich ordinal; never another resource ID registry.
	uint8_t percent; ///< Exact percentage; largest-remainder allocation retains at least one per class.
};

/** Immutable initial placement/version defaults; generated records remain authoritative after loading. */
struct MultiLayerAsteroidPlacementDefinition {
	uint16_t generation_version; ///< Saved generator contract, independent of the global save schema.
	uint8_t grid_spacing_tiles; ///< Minimum initial centre separation and native grid step.
	uint8_t starter_radius_tiles; ///< Preferred actual basic-probe approach bound when legal terrain permits it.
};

/** One equally likely initial compass heading; components retain the shared Q16 rounding contract. */
struct MultiLayerAsteroidHeadingDefinition {
	int32_t x_q16; ///< Signed unit-vector X component; diagonals use46340.
	int32_t y_q16; ///< Signed unit-vector Y component, independent of class and size.
};

/** One equally likely initial speed, sampled independently after the heading draw. */
struct MultiLayerAsteroidSpeedDefinition {
	uint32_t distance_q16_per_day; ///< Nominal whole Q16 magnitude per74-tick economy day.
};

/** @param profile Explicit selected immutable profile identity. @return Industry's eight equal-weight compass directions, or empty for Classic/unknown. */
std::span<const MultiLayerAsteroidHeadingDefinition> GetMultiLayerAsteroidHeadingDefinitions(MultiLayerGameplayProfileID profile);
/** @param profile Explicit selected immutable profile identity. @return Industry's three equal-weight speeds, or empty for Classic/unknown. */
std::span<const MultiLayerAsteroidSpeedDefinition> GetMultiLayerAsteroidSpeedDefinitions(MultiLayerGameplayProfileID profile);

/** @param profile Explicit selected immutable profile identity. @return Industry's exact rows, or empty for Classic/unknown identities. */
std::span<const MultiLayerAsteroidPopulationDefinition> GetMultiLayerAsteroidPopulationDefinitions(MultiLayerGameplayProfileID profile);
/** @param profile Explicit selected immutable profile identity. @return Industry's class shares, or empty for Classic/unknown identities. */
std::span<const MultiLayerAsteroidClassShare> GetMultiLayerAsteroidClassShares(MultiLayerGameplayProfileID profile);
/** @param profile Explicit selected immutable profile identity. @return Industry's placement contract, or nullptr for Classic/unknown identities. */
const MultiLayerAsteroidPlacementDefinition *GetMultiLayerAsteroidPlacementDefinition(MultiLayerGameplayProfileID profile);

#endif /* MULTILAYER_ASTEROID_CATALOGUE_H */
