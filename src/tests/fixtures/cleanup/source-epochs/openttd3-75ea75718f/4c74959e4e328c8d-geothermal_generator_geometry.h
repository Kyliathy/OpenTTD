/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_generator_geometry.h Passive physical generator source geometry before operating native registration.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups55-56.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef GEOTHERMAL_GENERATOR_GEOMETRY_H
#define GEOTHERMAL_GENERATOR_GEOMETRY_H

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "map_type.h"
#include "timer/timer_game_calendar.h"

struct MultiLayerGeothermalConstruction;

/** Logical dated generator designs; these ordinals are neither construction kinds nor native IndustryIDs. */
enum class GeothermalGeneratorModel : uint8_t {
	VentTap, ///<1980 first utility generator, borrowing the existing Powerhouse construction floor.
	MagmaLoop, ///<2000 closed-loop design; geometry does not install or fund an operating native type.
};

/**
 * Immutable native world-cell source contract for a dated generator design.
 * Native IIDS/TIDS, construction permission, stock/work, Heat and electricity
 * retain their existing owners. Raster canvases/anchors remain the Sprite Contract's.
 */
struct GeothermalGeneratorGeometry {
	GeothermalGeneratorModel model; ///< Logical source design, never a runtime industry reference.
	const MultiLayerGeothermalConstruction *first_build_contract; ///< Borrowed1980 bootstrap invoice/floor; advanced geometry does not redefine that first bill.
	uint8_t width; ///< Complete native world-floor width, excluding access cells.
	uint8_t height; ///< Complete native world-floor height.
	TimerGameCalendar::Year introduction_year; ///< Calendar metadata; querying a future design grants no availability.
	uint8_t construction_stages; ///< Native industry presentation stages; completed work remains a separate owner/flag.
	std::span<const TileIndexDiffC> cells; ///< Stable row-major relative native cell layout, shared by art and later registered machinery.
	std::string_view sprite_asset; ///< Exact first-party source family; no sprite IDs are allocated by this descriptor.
};

/**
 * Resolve the canonical physical source layout before actual generator activation.
 * @param model Logical1980/2000 source design, independent of runtime industry or construction IDs.
 * @return Immutable geometry for a known design, or absence for an unknown model
 * or changed first-bootstrap contract requiring source reconciliation. No native
 * definition, cold claim, funds, stock, work, clock or utility state is changed.
 */
std::optional<GeothermalGeneratorGeometry> GetGeothermalGeneratorGeometry(GeothermalGeneratorModel model);

#endif /* GEOTHERMAL_GENERATOR_GEOMETRY_H */
