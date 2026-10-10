/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_generator_geometry.cpp Canonical passive world layouts for1980/2000 geothermal source authoring.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group8B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups55-56.
 */

#include "stdafx.h"
#include "geothermal_generator_geometry.h"
#include "industry_type.h"
#include "multilayer_bootstrap.h"

#include "safeguards.h"

/**
 * Construct the frozen sixteen-cell native floor without assigning any TIDS.
 * @return Row-major world offsets; native constructors and original sprite sections use the same cells.
 */
static constexpr std::array<TileIndexDiffC, 16> MakeGeneratorSourceLayout()
{
	std::array<TileIndexDiffC, 16> result{};
	// Rows are actual world Y, columns actual world X. Logical cell order stays
	// independent of camera projection and cannot be synthesized by raster rotation.
	for (uint8_t y = 0; y < 4; ++y) {
		for (uint8_t x = 0; x < 4; ++x) result[static_cast<size_t>(y) * 4 + x] = {x, y};
	}
	return result;
}

static constexpr auto _generator_source_layout = MakeGeneratorSourceLayout(); ///< Physical source layout only; canonical native managers retain all identities.

/** @copydoc GetGeothermalGeneratorGeometry */
std::optional<GeothermalGeneratorGeometry> GetGeothermalGeneratorGeometry(GeothermalGeneratorModel model)
{
	switch (model) {
		case GeothermalGeneratorModel::VentTap: {
			const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Powerhouse);
			// The existing paid first-build contract must match the frozen native
			// sixteen-cell source. A changed contract requires deliberate new art,
			// rather than silently stretching already approved machinery sections.
			if (terms == nullptr || terms->width != 4 || terms->height != 4) return std::nullopt;
			return GeothermalGeneratorGeometry{model, terms, terms->width, terms->height, terms->introduction_year,
				static_cast<uint8_t>(INDUSTRY_COMPLETED + 1), _generator_source_layout, "facility.vent-powerhouse"};
		}
		case GeothermalGeneratorModel::MagmaLoop:
			// The later design owns its own immutable physical floor/date. This
			// passive descriptor adds neither a saved construction ordinal nor a
			// native type/permission; operating balance and installation remain Group8.
			return GeothermalGeneratorGeometry{model, nullptr, 4, 4, TimerGameCalendar::Year{2000},
				static_cast<uint8_t>(INDUSTRY_COMPLETED + 1), _generator_source_layout, "facility.magma-powerhouse"};
		default:
			return std::nullopt;
	}
}
