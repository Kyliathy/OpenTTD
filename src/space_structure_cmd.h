/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_structure_cmd.h Synchronized orbital placement, activation and physical exchange.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1-6.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C explicit native recipe controls and original adjacent power connections.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12 explicit mass-bounded hauler exchanges.
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
/** Remove a fully preflighted empty paid site without refunding spent materials, capital or energy.
 * @param flags Estimate or execute; estimates change no native receipt, packet, account or reference.
 * @param site Exact native pool ID. @param generation Exact issued incarnation; old queues cannot target a replacement through ID reuse.
 * @return Whole empty removal or unchanged refusal; accountable jobs, stock, occupancy and dependencies require explicit evacuation first.
 */
CommandCost CmdRemoveOrbitalModule(DoCommandFlags flags, SpaceStructureID site, uint64_t generation);
/** Cancel retained jobs and evacuate every unconsumed module packet to one explicit safe adjacent native inventory.
 * @param flags Estimate or execute; whole estimates move no packet, clear no job and release no physical craft.
 * @param site Source native ID. @param generation Exact source incarnation.
 * @param destination Owned operational Core/Storage/Dock ID. @param destination_generation Exact destination incarnation.
 * @return Whole packet evacuation/cancellation or unchanged refusal; spent history/energy and hull holds/poses remain authoritative.
 */
CommandCost CmdEvacuateOrbitalModule(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, SpaceStructureID destination, uint64_t destination_generation);
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
/** Activate a fully consumed Core/Storage/Power/Dock/Assembly frame without another material debit, cargo grant or freight payment.
 * @param flags Estimate or execute; estimates reserve no identity, panel or stock.
 * @param site Existing native paid frame ID.
 * @param generation Exact frame incarnation; stale or foreign requests refuse unchanged.
 * @return Refusal or the once-only start of saved activation; extensions require an operational adjacent owned Core.
 */
CommandCost CmdActivateOrbitalModule(DoCommandFlags flags, SpaceStructureID site, uint64_t generation);
/** Exchange real freight at an operational module's precise approach or admitted Dock berth.
 * Constructors load STEL/GOOD; haulers load their canonical mass-bounded refits at an admitted berth.
 * Orbital CargoDist endpoints are unsupported: explicit typed navigation and this command own each exchange.
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
/** Explicitly connect one activated Dock/Assembly's bounded work consumer to an original adjacent paid panel.
 * @param flags Estimate or execute; estimates register no consumer or connection.
 * @param site Owned activated Dock/Assembly identity. @param generation Exact module incarnation.
 * @param source Owned operational adjacent Core/Power identity. @param source_generation Exact source incarnation.
 * @return Whole zero-energy connection registration or unchanged refusal; generation and allocation remain separate.
 */
CommandCost CmdConnectOrbitalServices(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, SpaceStructureID source, uint64_t source_generation);
/** Disconnect an idle paid Dock/Assembly without discarding parts, consumed history or another module's grants.
 * @param flags Estimate or execute. @param site Owned activated work module ID. @param generation Exact module incarnation.
 * @return Whole original consumer retirement/connection release or unchanged refusal; held jobs must be explicitly cancelled first.
 */
CommandCost CmdDisconnectOrbitalServices(DoCommandFlags flags, SpaceStructureID site, uint64_t generation);
/** @param flags Estimate or execute. @param site Native Core/Power panel owner. @param generation Exact incarnation.
 * @param paused True disables physical panel generation. @return Availability change or unchanged refusal.
 */
CommandCost CmdSetOrbitalPanelPaused(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, bool paused);
/** Start or control one exact contained Core job or either admitted Dock service job.
 * @param flags Estimate or execute. @param site Native Core/Dock ID. @param generation Exact module incarnation.
 * @param craft Native hull ID. @param craft_generation Exact hull incarnation. @param control Explicit job operation.
 * @return Whole reservation/control/return operation or unchanged refusal.
 */
CommandCost CmdServiceOrbitalProbe(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, VehicleID craft, uint64_t craft_generation, OrbitalServiceControl control);
/** Move actual service supplies between an idle precise craft and its Core/Dock's separate stock.
 * @param flags Estimate or execute. @param site Native Core/Dock ID. @param generation Exact module incarnation.
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
/** Pay for one typed request at a completed connected Assembly, without immediately reserving packets or producing a vehicle.
 * @param flags Estimate or execute. @param site Actual Assembly pool identity. @param generation Exact facility incarnation.
 * @param source Actual adjacent Core/Storage inventory. @param source_generation Exact stock-owner incarnation.
 * @param model Immutable supported orbital vehicle model, retaining every declared recipe input.
 * @return Capital charge and one queued job, or unchanged whole refusal; only normal grant-qualified reservation moves material.
 */
CommandCost CmdQueueOrbitalManufacture(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, SpaceStructureID source, uint64_t source_generation, uint16_t model);
/** Cancel one unfinished manufacturing job or acknowledge its completed receipt, preserving unconsumed materials, actual products and spent work/capital.
 * @param flags Estimate or execute. @param site Actual Assembly identity. @param generation Exact facility incarnation.
 * @param destination Adjacent owned Core/Storage receiving the complete unconsumed batch, including after paid work; ignored for queued/complete packet-free jobs.
 * @param destination_generation Exact destination incarnation, never a reused inventory identity.
 * @return Whole packet-preserving cancellation or completed-receipt acknowledgement, or unchanged refusal. Paid work/capital remain spent and an existing product stays alive.
 */
CommandCost CmdCancelOrbitalManufacture(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, SpaceStructureID destination, uint64_t destination_generation);
/** Detach an unfinished Earth replacement while retaining its actual paid body as an ordinary purchase.
 * @param flags Quote or execute; QueryCost is pure. @param product Actual paid new body index. @param generation Exact new incarnation.
 * @return Zero-refund cancellation or unchanged refusal; the old hull and every original asset survive.
 */
CommandCost CmdCancelEarthSpaceReplacement(DoCommandFlags flags, VehicleID product, uint64_t generation);
/** Control the actual native module's explicitly selected processing recipe.
 * @param flags Estimate/execute; QueryCost changes nothing. @param site Existing processor identity. @param generation Exact module incarnation.
 * @param recipe_value Big-endian canonical four-byte recipe ID for Start; zero selects its default and other controls retain the choice.
 * @param control_value Exact OrbitalRecipeControl value: Start, Pause, Resume or Cancel.
 * @return Whole control admission or unchanged refusal; cancellation retains materials and spent electrical history.
 */
CommandCost CmdControlOrbitalRecipe(DoCommandFlags flags, SpaceStructureID site, uint64_t generation, uint32_t recipe_value, uint8_t control_value);
DEF_CMD_TRAIT(Commands::BuildSpaceBlueprint, CmdBuildSpaceBlueprint, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::CancelSpaceBlueprint, CmdCancelSpaceBlueprint, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::RemoveOrbitalModule, CmdRemoveOrbitalModule, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::EvacuateOrbitalModule, CmdEvacuateOrbitalModule, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::CollectSpaceConstructionInputs, CmdCollectSpaceConstructionInputs, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::ActivateOrbitalCore, CmdActivateOrbitalCore, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::ActivateOrbitalModule, CmdActivateOrbitalModule, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::MoveOrbitalFreight, CmdMoveOrbitalFreight, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::CommissionOrbitalServices, CmdCommissionOrbitalServices, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::ConnectOrbitalServices, CmdConnectOrbitalServices, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::DisconnectOrbitalServices, CmdDisconnectOrbitalServices, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::SetOrbitalPanelPaused, CmdSetOrbitalPanelPaused, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::ServiceOrbitalProbe, CmdServiceOrbitalProbe, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::MoveOrbitalSupply, CmdMoveOrbitalSupply, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::ReserveOrbitalModuleInputs, CmdReserveOrbitalModuleInputs, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::QueueOrbitalManufacture, CmdQueueOrbitalManufacture, {}, CommandType::VehicleConstruction)
DEF_CMD_TRAIT(Commands::CancelOrbitalManufacture, CmdCancelOrbitalManufacture, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::CancelEarthSpaceReplacement, CmdCancelEarthSpaceReplacement, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::ControlOrbitalRecipe, CmdControlOrbitalRecipe, {}, CommandType::OtherManagement)
#endif
