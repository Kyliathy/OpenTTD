/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_power_lineage.h Bounded original-source parent/segment validation in the sole shared reservation owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group8C
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5
 */

#ifndef UTILITY_POWER_LINEAGE_H
#define UTILITY_POWER_LINEAGE_H

#include "utility_power.h"

/** Copied serialized geometry only; validation never borrows the running world's map size for an independent preview. */
struct UtilityLineageGeometry {
	uint32_t width = 0; ///< This file's plane width; zero omits coordinate checks in a pure accounting fixture.
	uint32_t height = 0; ///< This file's plane height, separate from its stacked address range.
	uint32_t layers = MAX_MAP_LAYERS; ///< This file's explicit native plane count.
};

/** Bounded reference-only native path row; no energy quantity or independently issued identity exists in this transport. */
struct UtilityPowerPathWire {
	uint32_t share = 0; ///< Existing shared slice identity within this exact epoch.
	uint32_t ordinal = 0; ///< Consecutive position in that slice's newly traversed segment.
	uint32_t link = 0; ///< Existing installed cell or original native edge identity.
};

/**
 * Validate retained parent/segment identity, physical ordering and exact conserved direct flows without publishing any state.
 * @param state Complete shared reservation snapshot, including both domains and retired history.
 * @param geometry Optional explicit copied/native plane dimensions; no live map, pool, path cache or clock is consulted.
 * @return Whole structural lineage acceptance/refusal; native floor and original machinery readiness retain their existing full-load owners.
 */
UtilityPowerError ValidateUtilityPowerLineage(const UtilityPowerState &state, UtilityLineageGeometry geometry = {});

/**
 * Convert genuine earlier one-hop wire slices to the shared parent/segment representation, without creating a producer or quantity.
 * @param state Unpublished decoded older-file snapshot; roots are numbered before their retained one-hop children.
 * @return Complete bounded migration or unchanged refusal; already migrated snapshots validate without reissuing their identities.
 */
UtilityPowerError MigrateUtilityPowerLineage(UtilityPowerState &state);

#endif /* UTILITY_POWER_LINEAGE_H */
