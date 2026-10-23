/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_controls.cpp Live geothermal availability and inspection through existing native owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups15B/15C.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 */
#include "stdafx.h"
#include "geothermal_controls.h"
#include "geothermal_collector.h"
#include "geothermal_generator.h"
#include "geothermal_riser.h"
#include "company_base.h"
#include "industry.h"
#include "industry_map.h"
#include "openttd.h"
#include "safeguards.h"

/** @copydoc IsGeothermalFacilityToolAvailable */
bool IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind kind, MapLayer layer, CompanyID company)
{
	const auto &profile = GetCurrentMultiLayerProfileSelection();
	const auto *terms = GetMultiLayerGeothermalConstruction(kind);
	// Coarse controls offer only actual native company construction on allocated
	// rock planes; an editor/legacy policy or a camera projection grants none.
	if (_game_mode != GameMode::Normal || !Company::IsValidID(company) || terms == nullptr ||
		profile.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile.legacy || !MultiLayerMap::IsValidLayer(layer) ||
		TimerGameCalendar::date < TimerGameCalendar::ConvertYMDToDate(terms->introduction_year, 0, 1)) return false;
	if (layer == MapLayer::DeepUnderground) return CheckDeepLayerAvailability(profile, TimerGameCalendar::date);
	return kind == MultiLayerGeothermalConstructionKind::Riser && layer == MapLayer::Underground;
}

/** @copydoc GetGeothermalControlInspection */
GeothermalControlInspection GetGeothermalControlInspection(TileIndex tile)
{
	GeothermalControlInspection result;
	if (!IsValidTile(tile)) return result;
	result.site = GetGeothermalSiteClaimForTile(tile);
	if (IsTileType(tile, TileType::Industry)) {
		const auto id = GetIndustryIndex(tile);
		result.collector = GetGeothermalCollectorState(id);
		result.generator = GetGeothermalGeneratorState(id);
		result.riser = GetGeothermalRiserState(id);
		// An ordinary industry is not geothermal machinery just because a vent
		// happens to share its coordinates or a fallback sprite looks similar.
		if (result.collector != nullptr || result.generator != nullptr || result.riser != nullptr) result.industry = Industry::GetIfValid(id);
		if (result.collector != nullptr) result.site = GetGeothermalSiteClaim(result.collector->site);
		if (result.generator != nullptr) {
			result.site = GetGeothermalSiteClaim(result.generator->site);
			result.heat_input = GetUtilityPowerConsumer(result.generator->heat_consumer);
			result.electricity = GetUtilityPowerProducer(result.generator->electrical_producer);
		}
	}
	const auto &geology = GetGeologyWorldState();
	result.vent = FindGeothermalVentField(geology, tile);
	if (result.site != nullptr) {
		const auto found = std::ranges::find(geology.vents, result.site->site.vent, &GeothermalVentField::id);
		if (found != geology.vents.end()) result.vent = &*found;
	}
	if (result.vent != nullptr) {
		const auto *collector = GetGeothermalCollectorState(FindGeothermalCollectorForVent(result.vent->id));
		if (collector != nullptr) result.heat = GetUtilityPowerProducer(collector->heat_producer);
	}
	return result;
}
