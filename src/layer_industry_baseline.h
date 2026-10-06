/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_industry_baseline.h Explicit compatible original receivers without duplicate definitions or new recipe identities. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 11 */

#ifndef LAYER_INDUSTRY_BASELINE_H
#define LAYER_INDUSTRY_BASELINE_H

#include "industry_type.h"
#include "cargo_type.h"

struct Industry;

/** @param type Actual runtime definition. @return Whether the selected profile may reuse its exact one-to-one Iron Ore/Steel native path. */
bool IsLayerBaselineSteelDefinition(IndustryType type);
/** @param resource Coal or Iron Ore stable meaning. @return Existing compatible receiver identity, or IT_INVALID; Coal resolution alone grants no useful-power production. */
IndustryType GetLayerBaselineReceiverType(CargoLabel resource);
/** @param industry Existing validated steelworks. @param cargo Actual incoming slot. @return Room in its sole bounded input/history owner, or zero when unavailable. */
uint32_t GetLayerBaselineSteelInputRoom(const Industry &industry, CargoType cargo);
/** @param industry Existing steelworks. @param cargo Actual incoming meaning. @param offered Whole packet units. @return Exact stock/history credit; no material conversion occurs during intake. */
uint32_t ReceiveLayerBaselineSteelCargo(Industry &industry, CargoType cargo, uint32_t offered);
/** @param industry Existing supported steelworks. @return Exact one-to-one physical input/output conversion, with excess input retained. */
uint32_t ProcessLayerBaselineSteel(Industry &industry);
/** @param industry Existing supported steelworks. @return Exactly committed station units; refused finite Steel stays in its Industry output. */
uint32_t DispatchLayerBaselineSteel(Industry &industry);
/** @param industry Existing supported steelworks. @return Whether demolition would discard delivered Iron Ore or produced Steel. */
bool LayerBaselineSteelHasStoredCargo(const Industry &industry);

#endif /* LAYER_INDUSTRY_BASELINE_H */
