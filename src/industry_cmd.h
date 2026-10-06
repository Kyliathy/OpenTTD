/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file industry_cmd.h Command definitions related to industries. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md */

#ifndef INDUSTRY_CMD_H
#define INDUSTRY_CMD_H

#include "command_type.h"
#include "company_type.h"
#include "industry_type.h"
#include "industry.h"
#include "multilayer_industry.h"

/**
 * Build/fund an ordinary Surface industry through its shared availability/placement boundary.
 * @param flags Estimate/execute policy.
 * @param tile Explicit world anchor.
 * @param it Runtime industry definition.
 * @param first_layout First candidate layout.
 * @param fund Deity funding/prospecting choice.
 * @param seed Synchronized callback/layout seed.
 * @return Complete construction charge or unchanged refusal.
 */
CommandCost CmdBuildIndustry(DoCommandFlags flags, TileIndex tile, IndustryType it, uint32_t first_layout, bool fund, uint32_t seed);

/**
 * Fund one native geological mine at an explicitly selected seam and layout.
 * @param flags Estimate or complete atomic execution.
 * @param tile World machinery anchor.
 * @param it Exact native runtime mine definition.
 * @param layout Explicit installed layout; no random Surface prospecting.
 * @param deposit Selected finite geological identity.
 * @param seed Synchronized construction seed.
 * @return Complete machinery/clearing charge or refusal before tile/claim mutation.
 */
CommandCost CmdBuildLayerMine(DoCommandFlags flags, TileIndex tile, IndustryType it, uint32_t layout, GeologyDepositID deposit, uint32_t seed);

/**
 * Select one supported native alternative without moving or consuming any delivered/produced stock.
 * @param flags Estimate or execution.
 * @param industry Existing native processor identity.
 * @param recipe_value Content-owned four-byte recipe ID packed in big-endian order, not an allocated local ID.
 * @return Free complete choice change, or unchanged owner/date/definition refusal.
 */
CommandCost CmdSelectLayerIndustryRecipe(DoCommandFlags flags, IndustryID industry, uint32_t recipe_value);

/** @param flags Estimate/execute policy. @param ind_id Industry to manage. @param ctlflags New control flags. @return Command result. */
CommandCost CmdIndustrySetFlags(DoCommandFlags flags, IndustryID ind_id, IndustryControlFlags ctlflags);
/** @param flags Estimate/execute policy. @param ind_id Industry to manage. @param company_id Exclusive company. @param consumer Select outgoing rather than incoming exclusivity. @return Command result. */
CommandCost CmdIndustrySetExclusivity(DoCommandFlags flags, IndustryID ind_id, Owner company_id, bool consumer);
/** @param flags Estimate/execute policy. @param ind_id Industry to manage. @param text New player/script text. @return Command result. */
CommandCost CmdIndustrySetText(DoCommandFlags flags, IndustryID ind_id, const EncodedString &text);
/** @param flags Estimate/execute policy. @param ind_id Industry to manage. @param prod_level New production level. @param show_news Publish the change. @param text News text. @return Command result. */
CommandCost CmdIndustrySetProduction(DoCommandFlags flags, IndustryID ind_id, uint8_t prod_level, bool show_news, const EncodedString &text);

DEF_CMD_TRAIT(Commands::BuildIndustry, CmdBuildIndustry, CommandFlag::Deity, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::BuildLayerMine, CmdBuildLayerMine, {}, CommandType::LandscapeConstruction)
DEF_CMD_TRAIT(Commands::SelectLayerIndustryRecipe, CmdSelectLayerIndustryRecipe, CommandFlag::Deity, CommandType::OtherManagement)
DEF_CMD_TRAIT(Commands::IndustrySetFlags, CmdIndustrySetFlags, CommandFlag::Deity, CommandType::OtherManagement)
DEF_CMD_TRAIT(Commands::IndustrySetExclusivity, CmdIndustrySetExclusivity, CommandFlag::Deity, CommandType::OtherManagement)
DEF_CMD_TRAIT(Commands::IndustrySetText, CmdIndustrySetText, CommandFlags({CommandFlag::Deity, CommandFlag::StrCtrl}), CommandType::OtherManagement)
DEF_CMD_TRAIT(Commands::IndustrySetProduction, CmdIndustrySetProduction, CommandFlag::Deity, CommandType::OtherManagement)

#endif /* INDUSTRY_CMD_H */
