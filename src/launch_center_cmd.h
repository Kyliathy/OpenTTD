/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
#ifndef LAUNCH_CENTER_CMD_H
#define LAUNCH_CENTER_CMD_H
#include "command_type.h"
#include "station_type.h"
CommandCost CmdBuildLaunchCenter(DoCommandFlags flags, TileIndex tile, uint8_t rotation, StationID to_join, bool adjacent);
CommandCost CmdRemoveLaunchCenter(DoCommandFlags flags, TileIndex tile);
/** Invalid center plus zero generation unlinks; otherwise select this exact center incarnation. */
CommandCost CmdSetLaunchHub(DoCommandFlags flags, StationID member, StationID center, uint64_t generation);
DEF_CMD_TRAIT(Commands::BuildLaunchCenter, CmdBuildLaunchCenter, CommandFlags({CommandFlag::Auto, CommandFlag::NoWater}), CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::RemoveLaunchCenter, CmdRemoveLaunchCenter, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::SetLaunchHub, CmdSetLaunchHub, {}, CommandType::RouteManagement)
#endif
