/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geothermal_site_wire.h Copied cold planning coordinates for independent file preview, with no live map or energy authority.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-savegame-determinism-and-lifecycle.md
 */

#ifndef GEOTHERMAL_SITE_WIRE_H
#define GEOTHERMAL_SITE_WIRE_H

/** Fixed copied transport only; actual world tiles are reconstructed by the map controller after saved geometry loads. Every site is Deep-local. */
struct GeothermalSiteWire {
	uint32_t id = UINT32_MAX, vent = UINT32_MAX; ///< Retained planning identity and referenced original natural source.
	uint8_t owner = 0, kind = 0; ///< Exact actual company and frozen construction family.
	uint16_t x = 0, y = 0; ///< Layer-local facility origin in the saved map's geometry.
	uint16_t endpoint_x = 0, endpoint_y = 0; ///< Explicit boundary port in the same saved Deep plane.
	uint8_t access = 0, active = 0; ///< Checked access-side enum and cold reservation liveness; no completed-construction flag.
};

#endif /* GEOTHERMAL_SITE_WIRE_H */
