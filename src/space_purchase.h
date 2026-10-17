/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_purchase.h Ordinary purchase admission through actual paid Surface/orbital producers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8B
 */
#ifndef SPACE_PURCHASE_H
#define SPACE_PURCHASE_H
#include "command_type.h"
#include "space_production.h"

struct SpaceStructure;

/** Result view of existing physical owners; this creates no second job, hull, inventory or payment authority. */
struct SpacePurchaseResult {
	CommandCost cost{INVALID_STRING_ID}; ///< Whole unchanged refusal unless the actual producer admits the request; no command-dispatch header dependency.
	VehicleID vehicle = VehicleID::Invalid(); ///< Actual Earth-assembled hull on execution; a queued orbital request has no vehicle yet.
	SpaceStructureRef factory; ///< Original actual orbital factory, including for a pure successful quote.
	uint64_t serial = 0; ///< Actual issued job serial on orbital execution only; estimates issue nothing.
};

/** Resolve an actual Assembly footprint through world-layer geometry, without admission or ownership mutation.
 * @param tile Real world tile inside the module footprint; output approaches and camera-local coordinates do not identify a facility.
 * @return Unique actual Assembly, or nullptr for absent/ambiguous/wrong-layer geometry; the purchase producer separately checks ownership and operation.
 */
SpaceStructure *GetSpacePurchaseAssembly(TileIndex tile);
/** Identify an existing canonical Space Engine without registering a definition or a vehicle.
 * @param engine Actual native EngineID supplied by the ordinary purchase request.
 * @return Whether its native role agrees with the selected-profile mapping.
 */
bool IsSpacePurchaseEngine(EngineID engine);
/** Quote or execute one ordinary purchase through the existing paid physical producer.
 * @param flags Native command flags; estimates/query-cost never create owners. Replacement needs its separate old-hull settlement contract.
 * @param tile Actual Surface workshop or orbital Assembly footprint, independent of the selected camera.
 * @param engine Existing canonical native Engine; this boundary never invents/registers a mapping during lookup.
 * @param cargo Default cargo or INVALID_CARGO; unsupported refits never use temporary purchases to bypass the physical recipe.
 * @return Actual capital cost and existing issued owner view. Orbital admission requires a single unambiguous eligible actual stock source;
 *         explicit source selection remains available through CmdQueueOrbitalManufacture. Execution creates either a real Earth hull or one paid orbital job.
 */
SpacePurchaseResult PurchaseSpaceVehicleAtFacility(DoCommandFlags flags, TileIndex tile, EngineID engine, CargoType cargo);
#endif
