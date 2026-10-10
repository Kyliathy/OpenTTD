/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_electrical.h Deterministic physical electrical routes through installed cable and original paid native risers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-f-utility-and-transfer-contracts.md Group19
 */

#ifndef GEOTHERMAL_ELECTRICAL_H
#define GEOTHERMAL_ELECTRICAL_H

#include "geothermal_utility.h"

/** One original paid transition and the complete same-layer installed inlet path preceding it. */
struct GeothermalElectricalLeg {
	UtilityLinkID riser{UINT32_MAX}; ///< Shared original native paid-edge identity, never a derived replacement.
	GeothermalUtilityPath inlet; ///< Actual cable cells from the source port or prior arriving housing to this riser's declared machinery port.
};

/** Pure physical opportunity; no route owns generation, a reservation, fractional carry or delivered electricity. */
struct GeothermalElectricalPath {
	uint64_t topology_revision = 0; ///< Exact local installation snapshot for later whole-path revalidation.
	uint64_t capacity_w = 0; ///< Minimum frozen rate across every inlet cell and paid transition.
	uint64_t capacity_j = 0; ///< Conservative whole-joule opportunity for the explicitly quoted synchronized duration.
	std::vector<GeothermalElectricalLeg> legs; ///< Stable ordered paid transitions and actual intervening cable paths.
};

/**
 * Quote a bounded physical path without generating, allocating or transferring electricity.
 * @param source Actual active electrical reservation on the first sending plane.
 * @param destination Actual active same-company reservation on the final receiving plane.
 * @param source_port Explicit installed electrical cell on the source plane; matching account IDs alone establish no cable connection.
 * @param economy_ticks Proposed synchronized interval duration; later settlement must use the original source's actual published duration.
 * @param excluded Unavailable installed cells or paid risers in this transient allocation; no geometry is changed.
 * @return Deterministic shortest paid-edge route and conservative rate opportunity, or absence for incomplete physical connectivity.
 */
std::optional<GeothermalElectricalPath> FindGeothermalElectricalPath(UtilityConsumerID source, UtilityConsumerID destination,
	TileIndex source_port, uint32_t economy_ticks, std::span<const UtilityLinkID> excluded = {});

#endif /* GEOTHERMAL_ELECTRICAL_H */
