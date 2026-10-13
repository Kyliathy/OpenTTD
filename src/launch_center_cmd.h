/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
#ifndef LAUNCH_CENTER_CMD_H
#define LAUNCH_CENTER_CMD_H
#include "command_type.h"
#include "station_type.h"
#include "cargo_type.h"
#include "vehicle_type.h"
#include "space_pod_type.h"
#include "space_orders.h"
CommandCost CmdBuildLaunchCenter(DoCommandFlags flags, TileIndex tile, uint8_t rotation, StationID to_join, bool adjacent);
CommandCost CmdRemoveLaunchCenter(DoCommandFlags flags, TileIndex tile);
/** Invalid center plus zero generation unlinks; otherwise select this exact center incarnation. */
CommandCost CmdSetLaunchHub(DoCommandFlags flags, StationID member, StationID center, uint64_t generation);
CommandCost CmdStartSurfaceHandling(DoCommandFlags flags, StationID member, uint64_t member_generation, StationID center, uint64_t generation, CargoType cargo, uint16_t quantity, bool reverse);
DEF_CMD_TRAIT(Commands::BuildLaunchCenter, CmdBuildLaunchCenter, CommandFlags({CommandFlag::Auto, CommandFlag::NoWater}), CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::RemoveLaunchCenter, CmdRemoveLaunchCenter, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::SetLaunchHub, CmdSetLaunchHub, {}, CommandType::RouteManagement)
DEF_CMD_TRAIT(Commands::StartSurfaceHandling, CmdStartSurfaceHandling, {}, CommandType::RouteManagement)
CommandCost CmdAssembleLaunchCraft(DoCommandFlags flags, StationID center, uint64_t generation, uint16_t model);
CommandCost CmdPauseLaunchAssembly(DoCommandFlags flags, VehicleID vehicle, uint64_t generation, bool pause);
CommandCost CmdRecoverLaunchCraft(DoCommandFlags flags, StationID center, uint64_t center_generation, VehicleID vehicle, uint64_t vehicle_generation);
DEF_CMD_TRAIT(Commands::AssembleLaunchCraft, CmdAssembleLaunchCraft, {}, CommandType::VehicleConstruction)
DEF_CMD_TRAIT(Commands::PauseLaunchAssembly, CmdPauseLaunchAssembly, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::RecoverLaunchCraft, CmdRecoverLaunchCraft, {}, CommandType::VehicleManagement)
CommandCost CmdMoveSpaceSupply(DoCommandFlags flags, StationID center, uint64_t center_generation, VehicleID craft, uint64_t craft_generation, uint8_t supply, uint16_t quantity, bool unload);
CommandCost CmdServiceLaunchProbe(DoCommandFlags flags, VehicleID craft, uint64_t generation, bool cancel);
CommandCost CmdCarryLaunchProbe(DoCommandFlags flags, VehicleID rocket, uint64_t rocket_generation, VehicleID probe, uint64_t probe_generation, bool unload);
DEF_CMD_TRAIT(Commands::MoveSpaceSupply, CmdMoveSpaceSupply, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::ServiceLaunchProbe, CmdServiceLaunchProbe, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::CarryLaunchProbe, CmdCarryLaunchProbe, {}, CommandType::VehicleManagement)
CommandCost CmdAssembleLaunchPod(DoCommandFlags flags, StationID center, uint64_t generation);
CommandCost CmdMoveLaunchPod(DoCommandFlags flags, StationID center, uint64_t center_generation, VehicleID craft, uint64_t craft_generation, SpacePodID pod, uint64_t pod_generation, bool unload);
CommandCost CmdPauseLaunchPod(DoCommandFlags flags, SpacePodID pod, uint64_t generation, bool pause);
CommandCost CmdRecoverLaunchPod(DoCommandFlags flags, StationID center, uint64_t center_generation, SpacePodID pod, uint64_t pod_generation);
DEF_CMD_TRAIT(Commands::AssembleLaunchPod, CmdAssembleLaunchPod, {}, CommandType::VehicleConstruction)
DEF_CMD_TRAIT(Commands::MoveLaunchPod, CmdMoveLaunchPod, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::PauseLaunchPod, CmdPauseLaunchPod, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::RecoverLaunchPod, CmdRecoverLaunchPod, {}, CommandType::VehicleManagement)
CommandCost CmdMoveLaunchFreight(DoCommandFlags flags, StationID center, uint64_t center_generation, VehicleID craft, uint64_t craft_generation, CargoType cargo, uint16_t quantity, bool unload);
DEF_CMD_TRAIT(Commands::MoveLaunchFreight, CmdMoveLaunchFreight, {}, CommandType::VehicleManagement)
CommandCost CmdBeginLaunchFlight(DoCommandFlags flags, VehicleID rocket, uint64_t generation, SpaceOrderTargetKind kind, uint32_t destination, uint64_t destination_generation);
CommandCost CmdCancelLaunchFlight(DoCommandFlags flags, VehicleID rocket, uint64_t generation);
CommandCost CmdDivertLaunchRecovery(DoCommandFlags flags, VehicleID rocket, uint64_t generation, StationID center, uint64_t center_generation);
DEF_CMD_TRAIT(Commands::BeginLaunchFlight, CmdBeginLaunchFlight, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::CancelLaunchFlight, CmdCancelLaunchFlight, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::DivertLaunchRecovery, CmdDivertLaunchRecovery, {}, CommandType::VehicleManagement)
CommandCost CmdReleaseOrbitalCargo(DoCommandFlags flags, VehicleID craft, uint64_t craft_generation, StationID center, uint64_t center_generation, SpacePodID pod, uint64_t pod_generation, uint16_t quantity);
CommandCost CmdRetargetSpaceDescent(DoCommandFlags flags, SpacePodID pod, uint64_t generation, StationID center, uint64_t center_generation);
DEF_CMD_TRAIT(Commands::ReleaseOrbitalCargo, CmdReleaseOrbitalCargo, {}, CommandType::VehicleManagement)
DEF_CMD_TRAIT(Commands::RetargetSpaceDescent, CmdRetargetSpaceDescent, {}, CommandType::VehicleManagement)
#endif
