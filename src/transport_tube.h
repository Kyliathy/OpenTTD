/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube.h Native tube rail capability; engine, construction and metered movement handoffs follow their producer groups.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-underground-transport-tubes.md Groups1-4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md task10.1
 */

#ifndef TRANSPORT_TUBE_H
#define TRANSPORT_TUBE_H

#include "rail_type.h"
#include "newgrf_config.h"
#include "engine_type.h"
#include "tile_type.h"
#include "command_type.h"
#include "direction_type.h"
#include "multilayer_content.h"

struct Train;

inline constexpr RailTypeLabel RAILTYPE_LABEL_TUBE{"UTUB"}; ///< Exact first-party semantic label; the canonical registry allocates its runtime slot.

/**
 * Allocate the native tube label through the existing railtype registry after exact package admission.
 * @param activated Actual active content configuration; missing/changed native package refuses before any registry write.
 * @return Actual native runtime railtype, or INVALID_RAILTYPE on collision, alternate-label claim or full registry.
 * Repeated initialization preserves the existing native identity. This foundation does not activate construction or engines.
 */
RailType InitialiseTubeRailType(const GRFConfigList &activated);

/** @return Exact canonical native tube railtype, or INVALID_RAILTYPE when absent; no runtime slot is cached or allocated by querying. */
RailType GetTubeRailType();

/**
 * Check actual canonical tube capability independently of compatibility/powered masks.
 * @param type Runtime railtype to inspect, including invalid/unallocated values.
 * @return Whether its exact UTUB registry row was admitted by the native producer; a label or alternate alone is insufficient.
 */
bool IsTubeRailType(RailType type);

/** Derived prototype capability; catalogue/EIDS/rail owners remain authoritative and this view is never saved. */
struct TubeVehicleCapability {
	EngineID engine = EngineID::Invalid(); ///< Actual persisted EIDS definition, never a logical model cast.
	const LayerVehicleCapability *catalogue = nullptr; ///< Borrowed immutable profile model, including dates/layers/exact refits.
	RailType guideway = INVALID_RAILTYPE; ///< Current canonical UTUB slot; invalid after registry retirement.
};

/**
 * Install the single Mole-70 prototype at its existing canonical EIDS identity.
 * @param activated Exact admitted content; the native tube registry must already exist.
 * @return Whether definition preflight and reconstruction succeed without collisions or missing cargo.
 * Loader/purchase-menu activation and electrical movement remain with later producer groups.
 */
bool InitialiseTubeEngineDefinitions(const GRFConfigList &activated);

/** @param model_id Prototype logical model9. @return Actual native EIDS engine, or Invalid for absent/foreign/unimplemented models. */
EngineID GetTubeEngine(uint16_t model_id);

/** @param engine Actual engine identity. @return Derived native capsule view, or no value for ordinary/foreign engines. */
std::optional<TubeVehicleCapability> GetTubeVehicleCapability(EngineID engine);

/**
 * Admit a whole actual train to its exclusive tube or conventional movement domain.
 * @param train Any physical part; the existing linked consist supplies every engine/cargo owner.
 * @param tile Actual destination world tile, independent of the active camera and compatibility masks.
 * @return Whether all parts share the destination domain and capsule layer/date rules; this never supplies energy.
 */
bool CanTubeTrainEnterTile(const Train &train, TileIndex tile);

/**
 * Validate a complete tube foundation/utility footprint through the existing geological owner.
 * @param footprint Explicit world floor and approach cells; all must occupy one permitted rock layer.
 * @return Pure construction permission or the existing profile/calendar/geological refusal; no tiles are excavated.
 */
CommandCost CheckTubeConstruction(std::span<const TileIndex> footprint);

/**
 * Validate tube station/depot geometry through the canonical rectangular construction helper.
 * @param tile Explicit world origin.
 * @param width Positive floor width, bounded by the ordinary owning constructor.
 * @param height Positive floor height, bounded by the ordinary owning constructor.
 * @param entrance Actual outward axis/door direction, or Invalid for no approach.
 * @param through Whether both opposing approaches are required.
 * @return Complete footprint permission; geometry/addressing stays with the existing owner.
 */
CommandCost CheckTubeConstructionArea(TileIndex tile, uint width, uint height,
	DiagDirection entrance = DiagDirection::Invalid, bool through = false);

/**
 * Validate actual train purchase/refit/clone location and cargo against the exclusive native domain.
 * @param engine Existing engine/EIDS definition.
 * @param tile Actual depot/refit world tile.
 * @param cargo Proposed exact cargo, or INVALID_CARGO for a location-only query.
 * @return Permission or unchanged refusal before any vehicle allocation/refit/charge.
 */
CommandCost CheckTubeVehicleAdmission(EngineID engine, TileIndex tile, CargoType cargo = INVALID_CARGO);

/**
 * Preflight every rail tile in a conversion before the ordinary command performs its first write.
 * @param tiles Complete selected world area, including nonrail cells used only for selection.
 * @param target Requested canonical railtype.
 * @param flags Estimate/execute policy passed unchanged to existing support guards.
 * @return Permission or complete refusal for domain, ground, paired portals, occupancy or reservations.
 */
CommandCost CheckTubeRailConversion(std::span<const TileIndex> tiles, RailType target, DoCommandFlags flags);

#endif /* TRANSPORT_TUBE_H */
