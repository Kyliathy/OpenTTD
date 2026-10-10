/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_riser_wire.h Copied local-coordinate machinery transport for independent version400 preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5B
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 */

#ifndef GEOTHERMAL_RISER_WIRE_H
#define GEOTHERMAL_RISER_WIRE_H

/** Actual paid control/work association; the wire below owns no native material or energy quantities. */
struct GeothermalRiserState;

/** Exact43-byte scalar construction transport; coordinates use the saved planes, never the live map during preview. */
struct GeothermalRiserWire {
	uint16_t industry = UINT16_MAX; ///< Actual native machinery identity.
	uint8_t owner = UINT8_MAX, layer = UINT8_MAX; ///< Current controller/abandonment and original sending rock plane.
	uint16_t x = 0, y = 0; ///< Original complete2x2 machinery anchor.
	uint16_t portal_x = 0, portal_y = 0, port_x = 0, port_y = 0; ///< Original sending housing and adjoining machinery cell.
	uint8_t destination_layer = UINT8_MAX; ///< Original receiving plane; unrelated intervening planes stay untouched.
	uint16_t destination_x = 0, destination_y = 0; ///< Original reciprocal housing coordinates.
	uint8_t access = UINT8_MAX; ///< Checked construction/loading direction.
	uint64_t portal_revision = 0; ///< Accepted original-pair stamp, retained through physical retirement.
	uint8_t portal_retired = 0; ///< Exact permanent housing-loss/abandonment boolean.
	uint16_t work_ticks = 0; ///< Paid labour, bounded by the frozen296 ticks.
	uint8_t materials_committed = 0, has_work_tick = 0; ///< Exact whole-bill receipt and once-only tick presence.
	uint64_t last_work_tick = 0; ///< Saved synchronized work credit, never wall-clock or preview progress.
};

/** @param row Copied construction scalars. @param width Saved plane width. @param height Saved plane height. @param layers Saved plane count. @return Complete scalar/local-geometry permission using immutable terms only; no live world is read. */
bool ValidateGeothermalRiserWire(const GeothermalRiserWire &row, uint32_t width, uint32_t height, uint32_t layers);
/** @param state Actual validated live construction participant. @return Exact copied scalar encoding; native stock/history are not copied here. */
GeothermalRiserWire EncodeGeothermalRiser(const GeothermalRiserState &state);
/** @param row Scalar-validated parsed construction participant. @return Runtime references resolved through the now-loaded native map controller, without creating machinery or stock. */
GeothermalRiserState DecodeGeothermalRiser(const GeothermalRiserWire &row);

#endif /* GEOTHERMAL_RISER_WIRE_H */
