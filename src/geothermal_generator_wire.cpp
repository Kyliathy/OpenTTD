/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md; original B adapters use explicit lineage capabilities.
 * @file geothermal_generator_wire.cpp Pure explicit powerhouse field conversion and independent copied-scalar validation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8C/8DC.
 */
#include "stdafx.h"
#include "geothermal_generator.h"
#include "geothermal_generator_wire.h"
#include "company_type.h"
#include "safeguards.h"

/** @copydoc EncodeGeothermalGenerator */
GeothermalGeneratorWire EncodeGeothermalGenerator(const GeothermalGeneratorState &state)
{
	return {state.industry.base(), state.site.base(), state.owner.base(), to_underlying(state.model), state.work_ticks,
		static_cast<uint8_t>(state.materials_committed), static_cast<uint8_t>(state.has_work_tick), state.last_work_tick,
		state.heat_consumer.base(), state.electrical_producer.base(), state.service_remainder, static_cast<uint8_t>(state.has_operating_tick),
		state.last_operating_tick, state.active_service_ticks};
}

/** @copydoc DecodeGeothermalGenerator */
GeothermalGeneratorState DecodeGeothermalGenerator(const GeothermalGeneratorWire &wire)
{
	return {IndustryID{wire.industry}, GeothermalSiteID{wire.site}, CompanyID{wire.owner}, static_cast<GeothermalGeneratorModel>(wire.model),
		wire.work_ticks, wire.materials_committed != 0, wire.has_work_tick != 0, wire.last_work_tick,
		UtilityConsumerID{wire.heat_consumer}, UtilityProducerID{wire.electrical_producer}, wire.service_remainder, wire.has_operating_tick != 0,
		wire.last_operating_tick, wire.active_service_ticks};
}

/** @copydoc ValidateGeothermalGeneratorWire */
bool ValidateGeothermalGeneratorWire(const GeothermalGeneratorWire &wire)
{
	const auto *operating = GetMultiLayerGeothermalGeneratorTerms(static_cast<GeothermalGeneratorModel>(wire.model));
	if (operating == nullptr) return false;
	const auto *terms = GetMultiLayerGeothermalConstruction(operating->construction);
	// Raw flags and service carry must validate before bool normalization.
	// Genuine older rows default to unbound, with no operating time or debt.
	if (wire.has_operating_tick > 1 || wire.service_remainder >= 74 ||
		wire.service_remainder != (wire.active_service_ticks % 74) * operating->service_cost_per_day % 74 ||
		(wire.has_operating_tick != 0) != (wire.active_service_ticks != 0) || (wire.has_operating_tick == 0 && wire.last_operating_tick != 0) ||
		(wire.electrical_producer == UINT32_MAX && wire.active_service_ticks != 0) ||
		(wire.heat_consumer == UINT32_MAX && wire.electrical_producer != UINT32_MAX)) return false;
	// Preview validates only immutable content and this file's exact scalars;
	// malformed booleans or receipts cannot normalize into valid paid progress.
	return terms != nullptr && wire.industry < 64000 && wire.site < MAX_GEOTHERMAL_SITE_CLAIMS &&
		(wire.owner < MAX_COMPANIES || wire.owner == CompanyID::Invalid().base()) && wire.work_ticks <= terms->work_ticks &&
		wire.materials_committed <= 1 && wire.has_work_tick <= 1 && (wire.materials_committed != 0) == (wire.work_ticks != 0) &&
		(wire.has_work_tick != 0) == (wire.work_ticks != 0) && (wire.has_work_tick != 0 || wire.last_work_tick == 0);
}
