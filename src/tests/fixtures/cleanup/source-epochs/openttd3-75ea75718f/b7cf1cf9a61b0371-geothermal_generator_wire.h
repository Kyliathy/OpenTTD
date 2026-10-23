/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_wire.h Bounded427 construction and appended429 operating reference/service transport; copied previews borrow no running-world quantities.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8C/8DC.
 */
#ifndef GEOTHERMAL_GENERATOR_WIRE_H
#define GEOTHERMAL_GENERATOR_WIRE_H

#include <cstdint>

/** Existing native construction/control record; this pure transport header introduces no live-world dependency. */
struct GeothermalGeneratorState;

/** Explicit20-byte427/428 construction plus26-byte429 operating table fields, independent of host padding; no material stock or energy is serialized twice. */
struct GeothermalGeneratorWire {
	uint16_t industry = UINT16_MAX; ///< Exact native machinery identity.
	uint32_t site = UINT32_MAX; ///< Retained consumed original Powerhouse blueprint.
	uint8_t owner = UINT8_MAX; ///< Actual controller or explicit abandonment.
	uint8_t model = UINT8_MAX; ///< Canonical implemented dated model.
	uint16_t work_ticks = 0; ///< Actual prepaid labour, bounded by this model's frozen bill.
	uint8_t materials_committed = 0; ///< Exact0/1 whole-bill receipt, validated before boolean conversion.
	uint8_t has_work_tick = 0; ///< Exact0/1 once-per-tick presence.
	uint64_t last_work_tick = 0; ///< Last actual construction opportunity, never preview progress.
	uint32_t heat_consumer = UINT32_MAX; ///<429 original shared physical HeatGenerator identity.
	uint32_t electrical_producer = UINT32_MAX; ///<429 original exact-conversion electrical source, or pending absence.
	uint8_t service_remainder = 0; ///<429 committed Money/74 numerator0..73.
	uint8_t has_operating_tick = 0; ///<429 exact0/1 actual operating receipt presence.
	uint64_t last_operating_tick = 0; ///<429 original synchronized operating opportunity.
	uint64_t active_service_ticks = 0; ///<429 committed service-duration history; no energy is transported here.
};

/** @param state Actual construction references/history. @return Explicit native transport; encoding changes no stock, work or identity. */
GeothermalGeneratorWire EncodeGeothermalGenerator(const GeothermalGeneratorState &state);
/** @param wire Scalar-validated copied native row. @return Detached unpublished construction state; native reference acceptance remains mandatory. */
GeothermalGeneratorState DecodeGeothermalGenerator(const GeothermalGeneratorWire &wire);
/** @param wire Exact raw fields, independent of the running world. @return Whether identity bounds, model, work and canonical booleans agree before decoding. */
bool ValidateGeothermalGeneratorWire(const GeothermalGeneratorWire &wire);

#endif /* GEOTHERMAL_GENERATOR_WIRE_H */
