/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_vehicle.h Native conventional engine definitions and shared capability query. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Groups 7-10 */

#ifndef UNDERGROUND_VEHICLE_H
#define UNDERGROUND_VEHICLE_H

#include "command_type.h"
#include "engine_type.h"
#include "multilayer_content.h"

/**
 * Install conventional designs into the existing engine/EIDS owners after pinned content activation.
 * @param activated Exact loader configuration; no content is searched outside the accepted package.
 * @return Whether every definition can be reconstructed without overwriting original or colliding mappings.
 */
bool InitialiseUndergroundEngineDefinitions(const GRFConfigList &activated);

/**
 * Resolve a logical conventional design through the persisted EIDS owner.
 * @param model_id Catalogue model 2-8; the delivered Earth/Rock hoppers reuse the TBM producer's mappings.
 * @return Actual engine identity, or Invalid when the requested definition is absent/foreign.
 */
EngineID GetUndergroundEngine(uint16_t model_id);

/**
 * Identify the immutable conventional capability belonging to an actual native engine.
 * @param engine Real engine identity, never a logical ID cast to a pool index.
 * @return Catalogue row for a native hopper/freight design, or nullptr for other engines.
 */
const LayerVehicleCapability *GetUndergroundEngineCapability(EngineID engine);

/**
 * Check native conventional and capsule profile, technology, layer and exact cargo policy.
 * @param engine Actual engine definition. Ordinary external/original engines retain their existing rules.
 * @param layer Explicit operation layer; Invalid checks global profile/date availability without a location.
 * @param cargo Proposed cargo, or INVALID_CARGO to check presentation independent of refit selection.
 * @return The shared refusal, or success. This query never allocates, services or commissions a vehicle.
 */
CommandCost CheckUndergroundVehicleCapability(EngineID engine, MapLayer layer, CargoType cargo = INVALID_CARGO);

#endif /* UNDERGROUND_VEHICLE_H */
