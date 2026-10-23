/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file layer_portal_work_wire.h Existing portal work scalars shared with independent dependent-family preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md Group2
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5B
 */

#ifndef LAYER_PORTAL_WORK_WIRE_H
#define LAYER_PORTAL_WORK_WIRE_H

/** Existing26-byte copied transport; geometry and actor roles remain with their actual owners. */
struct PortalWorkWire {
	uint32_t key = UINT32_MAX; ///< Serialized canonical physical endpoint address in this file's map geometry.
	uint8_t state = 0, company = UINT8_MAX; ///< Checked work phase and actual company identity.
	uint32_t project = UINT32_MAX, cutter = UINT32_MAX, origin = UINT16_MAX; ///< Existing explicit job/vehicle/station references.
	uint64_t revision = 1; ///< Saved native transition stamp, never a preview-issued incarnation.
};

#endif /* LAYER_PORTAL_WORK_WIRE_H */
