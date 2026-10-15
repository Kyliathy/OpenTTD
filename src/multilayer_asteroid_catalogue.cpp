/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file multilayer_asteroid_catalogue.cpp Immutable Plan10 resource/population defaults.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Groups1-3/P1
 */
#include "stdafx.h"
#include "multilayer_content.h"
#include "multilayer_asteroid_catalogue.h"
#include "core/random_func.hpp"
#include "space.h"
#include "safeguards.h"

/** @return Canonical one-tonne-per-unit class definitions, in their saved class order. */
const std::array<MultiLayerAsteroidDefinition, 3> &GetMultiLayerAsteroidDefinitions()
{
	static const std::array<MultiLayerAsteroidDefinition, 3> definitions{{
		{CargoLabel{"NORE"}, 400, 800}, {CargoLabel{"WICE"}, 800, 1600}, {CargoLabel{"RARE"}, 100, 200},
	}};
	return definitions;
}

/** @return Canonical independent size weights, diameters, leases and finite reserve multipliers. */
const std::array<MultiLayerAsteroidSizeDefinition, 3> &GetMultiLayerAsteroidSizeDefinitions()
{
	static const std::array<MultiLayerAsteroidSizeDefinition, 3> definitions{{
		{50, SPACE_Q16 / 2, 1, 1}, {35, SPACE_Q16, 1, 2}, {15, 2 * SPACE_Q16, 1, 4},
	}};
	return definitions;
}

/**
 * Apply cumulative size weights without drawing additional randomness.
 * @param percentile Independent uniform content draw in [0, 100).
 * @return Small/Medium/Large saved size index.
 */
uint8_t SelectMultiLayerAsteroidSize(uint32_t percentile)
{
	assert(percentile < 100);
	const auto &sizes = GetMultiLayerAsteroidSizeDefinitions();
	// Stable size order assigns exactly each catalogue weight's share of the 100 possible draws.
	for (uint8_t index = 0; index < sizes.size(); ++index) {
		if (percentile < sizes[index].chance_percent) return index;
		percentile -= sizes[index].chance_percent;
	}
	NOT_REACHED();
}

/**
 * Include both reserve endpoints, then scale the sampled Small content by body size.
 * @param resource Selected immutable class definition.
 * @param size Selected immutable size definition, sampled independently of the class reserve.
 * @param random Feature-local generator whose next draw belongs to this reserve.
 * @return Whole-tonne initial reserve; one later extracted unit debits one tonne.
 */
uint32_t SampleMultiLayerAsteroidReserve(const MultiLayerAsteroidDefinition &resource,
	const MultiLayerAsteroidSizeDefinition &size, Randomizer &random)
{
	assert(resource.small_min <= resource.small_max);
	return (resource.small_min + random.Next(resource.small_max - resource.small_min + 1)) * size.reserve_multiplier;
}

/** @param profile Explicit selected profile identity. @return Immutable exact Industry population rows, or no rows for other profiles. */
std::span<const MultiLayerAsteroidPopulationDefinition> GetMultiLayerAsteroidPopulationDefinitions(MultiLayerGameplayProfileID profile)
{
	if (profile != MultiLayerGameplayProfileID::MultilayerIndustry) return {};
	static const std::array<MultiLayerAsteroidPopulationDefinition, 13> rows{{
		{12, {6, 10, 14}}, {13, {12, 20, 28}}, {14, {24, 40, 56}}, {15, {48, 80, 112}}, {16, {96, 160, 224}},
		{17, {192, 320, 448}}, {18, {384, 640, 896}}, {19, {768, 1280, 1792}}, {20, {1536, 2560, 3584}},
		{21, {2304, 3840, 5376}}, {22, {3072, 5120, 7168}}, {23, {4608, 7680, 10752}}, {24, {6144, 10240, 14336}},
	}};
	return rows;
}

/** @param profile Explicit selected profile identity. @return Immutable ordered class percentages, or no shares for other profiles. */
std::span<const MultiLayerAsteroidClassShare> GetMultiLayerAsteroidClassShares(MultiLayerGameplayProfileID profile)
{
	if (profile != MultiLayerGameplayProfileID::MultilayerIndustry) return {};
	static const std::array<MultiLayerAsteroidClassShare, 3> shares{{{0, 60}, {1, 30}, {2, 10}}};
	return shares;
}

/** @param profile Explicit selected profile identity. @return Immutable Industry placement defaults, or nullptr for other profiles. */
const MultiLayerAsteroidPlacementDefinition *GetMultiLayerAsteroidPlacementDefinition(MultiLayerGameplayProfileID profile)
{
	if (profile != MultiLayerGameplayProfileID::MultilayerIndustry) return nullptr;
	static const MultiLayerAsteroidPlacementDefinition definition{1, 4, 32};
	return &definition;
}

/** @param profile Explicit selected profile identity. @return Eight equally likely Industry headings, or no directions for another profile. */
std::span<const MultiLayerAsteroidHeadingDefinition> GetMultiLayerAsteroidHeadingDefinitions(MultiLayerGameplayProfileID profile)
{
	if (profile != MultiLayerGameplayProfileID::MultilayerIndustry) return {};
	static const std::array<MultiLayerAsteroidHeadingDefinition, 8> headings{{
		{65536, 0}, {46340, 46340}, {0, 65536}, {-46340, 46340},
		{-65536, 0}, {-46340, -46340}, {0, -65536}, {46340, -46340},
	}};
	return headings;
}

/** @param profile Explicit selected profile identity. @return Three equally likely Industry speeds, or no speeds for another profile. */
std::span<const MultiLayerAsteroidSpeedDefinition> GetMultiLayerAsteroidSpeedDefinitions(MultiLayerGameplayProfileID profile)
{
	if (profile != MultiLayerGameplayProfileID::MultilayerIndustry) return {};
	static const std::array<MultiLayerAsteroidSpeedDefinition, 3> speeds{{{4096}, {8192}, {16384}}};
	return speeds;
}

/**
 * Select the exact count for supported map geometry and the saved population column.
 * @param width Surface-plane width in tiles.
 * @param height Surface-plane height in tiles.
 * @param density Saved Sparse/Normal/Dense value (0/1/2).
 * @return Exact count, or zero when the requested generation contract is unsupported.
 */
uint32_t GetMultiLayerAsteroidPopulation(uint32_t width, uint32_t height, uint8_t density)
{
	// Unsupported geometry or an unknown density must refuse generation, never round to a different field.
	if (width < 64 || height < 64 || width > 4096 || height > 4096 ||
		!std::has_single_bit(width) || !std::has_single_bit(height) || density > 2) return 0;
	const auto rows = GetMultiLayerAsteroidPopulationDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	return rows[std::countr_zero(width) + std::countr_zero(height) - 12].counts[density];
}
