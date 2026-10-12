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
#include "safeguards.h"

const std::array<MultiLayerAsteroidDefinition, 3> &GetMultiLayerAsteroidDefinitions()
{
	static const std::array<MultiLayerAsteroidDefinition, 3> definitions{{
		{CargoLabel{"NORE"}, 400, 800}, {CargoLabel{"WICE"}, 800, 1600}, {CargoLabel{"RARE"}, 100, 200},
	}};
	return definitions;
}

uint32_t GetMultiLayerAsteroidPopulation(uint32_t width, uint32_t height, uint8_t density)
{
	if (width < 64 || height < 64 || width > 4096 || height > 4096 ||
		!std::has_single_bit(width) || !std::has_single_bit(height) || density > 2) return 0;
	static constexpr uint32_t counts[][3] = {
		{6, 10, 14}, {12, 20, 28}, {24, 40, 56}, {48, 80, 112}, {96, 160, 224},
		{192, 320, 448}, {384, 640, 896}, {768, 1280, 1792}, {1536, 2560, 3584},
		{2304, 3840, 5376}, {3072, 5120, 7168}, {4608, 7680, 10752}, {6144, 10240, 14336},
	};
	return counts[std::countr_zero(width) + std::countr_zero(height) - 12][density];
}
