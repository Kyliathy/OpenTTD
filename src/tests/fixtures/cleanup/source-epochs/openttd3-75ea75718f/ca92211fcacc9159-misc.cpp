/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file misc.cpp Misc functions that shouldn't be here. @see docs/zz-reach2/architecture/game-loop/archi-game-loop.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md, docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#include "stdafx.h"
#include "multilayer_content.h"
#include "multilayer_state.h"
#include "excavation.h"
#include "layer_portal_cmd.h"
#include "tbm.h"
#include "spoil_receiver.h"
#include "landscape.h"
#include "map_layer.h"
#include "news_func.h"
#include "ai/ai.hpp"
#include "script/script_gui.h"
#include "newgrf.h"
#include "economy_func.h"
#include "timer/timer_game_calendar.h"
#include "timer/timer_game_economy.h"
#include "timer/timer_game_tick.h"
#include "texteff.hpp"
#include "gfx_func.h"
#include "gamelog.h"
#include "animated_tile_func.h"
#include "tilehighlight_func.h"
#include "network/network_func.h"
#include "window_func.h"
#include "core/pool_type.hpp"
#include "game/game.hpp"
#include "linkgraph/linkgraphschedule.h"
#include "town.h"
#include "viewport_kdtree.h"
#include "newgrf_profiling.h"
#include "3rdparty/monocypher/monocypher.h"

#include "safeguards.h"

extern TileIndex _cur_tileloop_tile;
extern void MakeNewgameSettingsLive();
extern uint8_t _map_file_layers;

void InitializeSound();
void InitializeMusic();
void InitializeVehicles();
void InitializeRailGui();
void InitializeRoadGui();
void InitializeAirportGui();
void InitializeDockGui();
void InitializeGraphGui();
void InitializeObjectGui();
void InitializeTownGui();
void InitializeIndustries();
void InitializeObjects();
void InitializeTrees();
void InitializeCompanies();
void InitializeCheats();
void InitializeOldNames();

/**
 * Generate an unique ID.
 *
 * It isn't as much of an unique ID but more a hashed digest of a random
 * string and a time. It is very likely to be unique, but it does not follow
 * any UUID standard.
 * @param subject What to create the ID for.
 * @return The generated ID.
 */
std::string GenerateUid(std::string_view subject)
{
	std::array<uint8_t, 32> random_bytes;
	RandomBytesWithFallback(random_bytes);

	auto current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	std::string coding_string = fmt::format("{}{}", current_time, subject);

	std::array<uint8_t, 16> digest;
	crypto_blake2b_ctx ctx;
	crypto_blake2b_init(&ctx, digest.size());
	crypto_blake2b_update(&ctx, random_bytes.data(), random_bytes.size());
	crypto_blake2b_update(&ctx, reinterpret_cast<const uint8_t *>(coding_string.data()), coding_string.size());
	crypto_blake2b_final(&ctx, digest.data());

	return FormatArrayAsHex(digest);
}

/**
 * Generate an unique savegame ID.
 */
void GenerateSavegameId()
{
	_game_session_stats.savegame_id = GenerateUid("OpenTTD Savegame ID");
}

/**
 * Initialize a candidate world without inheriting an unrelated accepted profile.
 * @param size_x Width of each map plane in tiles.
 * @param size_y Height of each map plane in tiles.
 * @param reset_date Initialize calendar/economy dates rather than retaining loaded values.
 * @param reset_settings Copy pending new-game settings into the candidate settings.
 */
void InitializeGame(uint size_x, uint size_y, bool reset_date, bool reset_settings)
{
	// A legacy save without profile metadata retains Classic behavior; new worlds activate after loading content.
	ResetMultiLayerProfileSelection();
	ResetMultiLayerStateSchema();
	ResetGeologyWorldState();
	ResetLayerPortalWorkState();
	ResetTBMProjectState();
	ResetSpoilReceiverState();
	/* Make sure there isn't any window that can influence anything
	 * related to the new game we're about to start/load. */
	UnInitWindowSystem();

	//AD-11: from this plan on, a live game stores every layer.
	Map::Allocate(size_x, size_y, MAX_MAP_LAYERS);
	//M-4: no file has been read for this game yet. A MAPS chunk sets the real count; an old TTD save has none, so its missing planes are initialised.
	_map_file_layers = 1;

	_pause_mode = {};
	_game_speed = 100;
	TimerGameTick::counter = 0;
	TimerGameEconomy::days_since_last_month = 0;
	_cur_tileloop_tile = TileIndex{1};
	_thd.redsq = INVALID_TILE;
	if (reset_settings) MakeNewgameSettingsLive();

	_newgrf_profilers.clear();

	if (reset_date) {
		TimerGameCalendar::Date new_date = TimerGameCalendar::ConvertYMDToDate(_settings_game.game_creation.starting_year, 0, 1);
		TimerGameCalendar::SetDate(new_date, 0);

		if (TimerGameEconomy::UsingWallclockUnits()) {
			/* If using wallclock units, start at year 1. */
			TimerGameEconomy::SetDate(TimerGameEconomy::ConvertYMDToDate(TimerGameEconomy::Year{1}, 0, 1), 0);
		} else {
			/* Otherwise, we always keep the economy date synced with the calendar date. */
			TimerGameEconomy::SetDate(TimerGameEconomy::Date{new_date.base()}, 0);
		}
		InitializeOldNames();
	}

	LinkGraphSchedule::Clear();
	PoolBase::Clean(PoolType::Normal);

	RebuildStationKdtree();
	RebuildTownKdtree();
	RebuildViewportKdtree();

	ResetPersistentNewGRFData();

	InitializeSound();
	InitializeMusic();

	InitializeVehicles();

	InitNewsItemStructs();
	InitializeLandscape();
	InitializeRailGui();
	InitializeRoadGui();
	InitializeAirportGui();
	InitializeDockGui();
	InitializeGraphGui();
	InitializeObjectGui();
	InitializeTownGui();
	InitializeScriptGui();
	InitializeTrees();
	InitializeIndustries();
	InitializeObjects();

	InitializeCompanies();
	AI::Initialize();
	Game::Initialize();
	InitializeCheats();

	InitTextEffects();
	NetworkInitChatMessage();
	InitializeAnimatedTiles();

	InitializeEconomy();

	ResetObjectToPlace();

	_gamelog.Reset();
	_gamelog.StartAction(GamelogActionType::Start);
	_gamelog.Revision();
	_gamelog.Mode();
	_gamelog.GRFAddList(_grfconfig);
	_gamelog.StopAction();
}
