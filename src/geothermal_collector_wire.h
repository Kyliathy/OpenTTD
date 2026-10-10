/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_collector_wire.h Independent construction and shared-meter reference transport; no private material or energy store.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 */

#ifndef GEOTHERMAL_COLLECTOR_WIRE_H
#define GEOTHERMAL_COLLECTOR_WIRE_H

/** Raw bounded scalar transport only; actual stock, native machinery and natural capacity are loaded and validated through their existing owners. */
struct GeothermalCollectorWire {
	uint16_t industry = UINT16_MAX; ///< Actual native industry pool identity.
	uint32_t site = UINT32_MAX; ///< Consumed retained cold blueprint identity, preserving original natural source/floor/endpoint.
	uint8_t owner = UINT8_MAX; ///< Actual current controller or explicit invalid-company abandonment.
	uint16_t work_ticks = 0; ///< Paid labour, bounded by the frozen296 ticks.
	uint8_t materials_committed = 0, has_work_tick = 0; ///< Exact checked booleans for one whole material receipt and once-only tick presence.
	uint64_t last_work_tick = 0; ///< Saved native construction tick, never preview/runtime wall-clock progress.
	uint32_t heat_producer = UINT32_MAX; ///< Appended391 reference to the sole shared native thermal meter; older files have no implicit association.
};

#endif /* GEOTHERMAL_COLLECTOR_WIRE_H */
