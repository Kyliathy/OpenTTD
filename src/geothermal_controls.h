/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_controls.h Read-only native geothermal control queries; no presentation-owned simulation registry.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups15B/15C.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 */
#ifndef GEOTHERMAL_CONTROLS_H
#define GEOTHERMAL_CONTROLS_H

#include "geothermal.h"

/** Native Industry owns all accepted construction stock. */
struct Industry;
/** Native collector construction/control participant. */
struct GeothermalCollectorState;
/** Native generator construction/service participant. */
struct GeothermalGeneratorState;
/** Native riser construction/original-portal participant. */
struct GeothermalRiserState;

/** Borrowed live inspection, valid only until the next owner mutation; callers re-query rather than retaining balances or permission. */
struct GeothermalControlInspection {
	const GeothermalVentField *vent = nullptr; ///< Original public natural field, including unchanged capacity.
	const GeothermalSiteClaim *site = nullptr; ///< Actual active or consumed original cold geometry.
	const Industry *industry = nullptr; ///< Actual construction-stock owner, if this tile contains selected machinery.
	const GeothermalCollectorState *collector = nullptr; ///< Actual collector work/history, never inferred from art.
	const GeothermalGeneratorState *generator = nullptr; ///< Actual dated generator work/service references.
	const GeothermalRiserState *riser = nullptr; ///< Actual paid original-portal construction references.
	const UtilityPowerProducer *heat = nullptr; ///< Sole original collector's shared source meter, without a copied capacity.
	const UtilityPowerConsumer *heat_input = nullptr; ///< Selected generator's actual demand/grant meter.
	const UtilityPowerProducer *electricity = nullptr; ///< Selected generator's one shared exact-conversion output.
};

/**
 * Filter facility tools by the live native profile, company, explicit layer and calendar introduction.
 * @param kind Exact frozen facility family; unsupported enum values refuse.
 * @param layer Explicit construction context; only risers may also use Underground.
 * @param company Actual company using the controls; spectators receive no construction admission.
 * @return Coarse tool availability only; the synchronized command still checks the full actual site and participants.
 */
bool IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind kind, MapLayer layer, CompanyID company);

/**
 * Inspect natural geology, a cold site or native selected machinery and its original shared meters without advancing work.
 * @param tile Explicit valid world tile; another layer at the same coordinates is a separate place.
 * @return Borrowed native owners, or an empty view; public surveys confer no company discovery bit or construction permission.
 */
GeothermalControlInspection GetGeothermalControlInspection(TileIndex tile);

#endif /* GEOTHERMAL_CONTROLS_H */
