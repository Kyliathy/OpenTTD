/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file landscape_cmd.h Command definitions related to landscape (slopes etc.). @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md */

#ifndef LANDSCAPE_CMD_H
#define LANDSCAPE_CMD_H

#include "command_type.h"

CommandCost CmdLandscapeClear(DoCommandFlags flags, TileIndex tile);
std::tuple<CommandCost, Money> CmdClearArea(DoCommandFlags flags, TileIndex tile, TileIndex start_tile, bool diagonal);

/**
 * Paint an atomic scenario rock rectangle under the saved explicit editor override.
 * @param flags Estimate/execute policy; ordinary gameplay cannot use this command.
 * @param tile Explicit first corner.
 * @param end_tile Explicit opposite corner in the same rock layer.
 * @param excavated True for open floor, false for solid terrain.
 * @return Empty cost or unchanged refusal; editor painting produces no spoil or paid work.
 */
CommandCost CmdPaintExcavationTerrain(DoCommandFlags flags, TileIndex tile, TileIndex end_tile, bool excavated);

DEF_CMD_TRAIT(Commands::LandscapeClear, CmdLandscapeClear, CommandFlag::Deity, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::ClearArea, CmdClearArea, CommandFlag::NoTest, CommandType::LandscapeConstruction) // destroying multi-tile houses makes town rating differ between test and execution
DEF_CMD_TRAIT(Commands::PaintExcavationTerrain, CmdPaintExcavationTerrain, CommandFlags({CommandFlag::Offline, CommandFlag::Deity}), CommandType::LandscapeConstruction)

#endif /* LANDSCAPE_CMD_H */
