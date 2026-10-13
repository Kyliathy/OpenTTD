/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_structure_cmd.h Synchronized orbital placement, activation and physical exchange.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1-6.
 */
#ifndef SPACE_STRUCTURE_CMD_H
#define SPACE_STRUCTURE_CMD_H
#include "command_type.h"
#include "space_structure_type.h"
#include "vehicle_type.h"
#include "cargo_type.h"
#include "multilayer_space_content.h"
#include "orbital_service.h"

/** Place an inert, material-paid site; no facility or material is created by estimation.
 * @param flags Estimate or execute. @param tile Explicit Space anchor. @param kind Canonical static module recipe.
 * @return Capital construction cost, or unchanged refusal.
 */
CommandCost CmdBuildSpaceBlueprint(DoCommandFlags flags, TileIndex tile, SpaceBuildKind kind = SpaceBuildKind::Core);
/** @param flags Estimate or execute. @param site Native site ID. @param generation Exact site incarnation. @return Refusal or retained-input cancellation. */
CommandCost CmdCancelSpaceBlueprint(DoCommandFlags flags, SpaceStructureID site, uint64_t generation);
/** Collect actual unconsumed material from a cancelled job.
 * @param flags Estimate or execute. @param site Native site ID. @param generation Exact site incarnation.
 * @param probe Native builder ID. @param probe_generation Exact builder incarnation.
 * @param cargo Actual recipe cargo. @param quantity Positive amount fitting the hold.
 * @return Refusal or a packet-preserving, unpaid physical move.
 */
CommandCost CmdCollectSpaceConstructionInputs(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, VehicleID probe, uint64_t probe_generation, CargoType cargo, uint16_t quantity);
/** Begin the one-time activation of a fully paid Core frame.
 * @param flags Estimate or execute. @param site Native site ID. @param generation Exact site incarnation.
 * @return Refusal or the start of the saved canonical activation countdown; creates no stock.
 */
CommandCost CmdActivateOrbitalCore(DoCommandFlags flags, SpaceStructureID site, uint64_t generation);
/** Exchange real freight at an operational Core's precise approach.
 * @param flags Estimate or execute. @param site Native module ID. @param generation Exact module incarnation.
 * @param craft Native orbital craft ID. @param craft_generation Exact craft incarnation.
 * @param cargo Actual physical cargo. @param quantity Positive bounded amount. @param load True loads the craft, false unloads it.
 * @return Refusal or a packet-preserving move with no delivery income.
 */
CommandCost CmdMoveOrbitalFreight(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, VehicleID craft, uint64_t craft_generation, CargoType cargo, uint16_t quantity, bool load);
/** Commission the actual panel of a migrated inventory-only Core, without stock or energy.
 * @param flags Estimate or execute. @param site Native Core ID. @param generation Exact site incarnation.
 * @return Once-only registration or unchanged refusal.
 */
CommandCost CmdCommissionOrbitalServices(DoCommandFlags flags, SpaceStructureID site, uint64_t generation);
/** @param flags Estimate or execute. @param site Native Core ID. @param generation Exact incarnation.
 * @param paused True disables physical panel generation. @return Availability change or unchanged refusal.
 */
CommandCost CmdSetOrbitalPanelPaused(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, bool paused);
/** Start/control the single precise Core service berth.
 * @param flags Estimate or execute. @param site Native Core ID. @param generation Exact Core incarnation.
 * @param craft Native hull ID. @param craft_generation Exact hull incarnation. @param control Explicit job operation.
 * @return Whole reservation/control/return operation or unchanged refusal.
 */
CommandCost CmdServiceOrbitalProbe(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, VehicleID craft, uint64_t craft_generation, OrbitalServiceControl control);
/** Move actual service supplies between an idle precise craft and the Core's stock.
 * @param flags Estimate or execute. @param site Native Core ID. @param generation Exact Core incarnation.
 * @param craft Native hull ID. @param craft_generation Exact hull incarnation. @param supply RFMT1 or ELEC2 compartment.
 * @param quantity Positive whole units. @param load True loads the craft; false unloads it.
 * @return Packet-preserving exchange without delivery income, or unchanged refusal.
 */
CommandCost CmdMoveOrbitalSupply(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, VehicleID craft, uint64_t craft_generation, uint8_t supply, uint16_t quantity, bool load);
/** Atomically reserve all still-missing common materials in an already paid native blueprint.
 * @param flags Estimate or execute. @param site Existing output identity/footprint. @param generation Exact blueprint incarnation.
 * @param core Owned adjacent Core containing actual delivered stock. @param core_generation Exact Core incarnation.
 * @return Whole all-input reservation or unchanged refusal; creates no facility and charges no second capital/payment.
 */
CommandCost CmdReserveOrbitalModuleInputs(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, SpaceStructureID core, uint64_t core_generation);
DEF_CMD_TRAIT(Commands::BuildSpaceBlueprint, CmdBuildSpaceBlueprint, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::CancelSpaceBlueprint, CmdCancelSpaceBlueprint, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::CollectSpaceConstructionInputs, CmdCollectSpaceConstructionInputs, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::ActivateOrbitalCore, CmdActivateOrbitalCore, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::MoveOrbitalFreight, CmdMoveOrbitalFreight, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::CommissionOrbitalServices, CmdCommissionOrbitalServices, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::SetOrbitalPanelPaused, CmdSetOrbitalPanelPaused, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::ServiceOrbitalProbe, CmdServiceOrbitalProbe, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::MoveOrbitalSupply, CmdMoveOrbitalSupply, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::ReserveOrbitalModuleInputs, CmdReserveOrbitalModuleInputs, {}, CommandType::LandscapeConstruction)
#endif
