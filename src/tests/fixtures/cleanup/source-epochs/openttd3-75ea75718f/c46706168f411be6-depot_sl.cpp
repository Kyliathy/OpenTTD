/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file depot_sl.cpp Native depot persistence, including registered portal magazines. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group8; docs/zz-reach2/architecture/savegame/archi-savegame.md */

#include "../stdafx.h"

#include "saveload.h"
#include "saveload_error.hpp"
#include "compat/depot_sl_compat.h"

#include "../depot_base.h"
#include "../town.h"
#include "../layer_portal_cmd.h"

#include "../safeguards.h"

static TownID _town_index;

static const SaveLoad _depot_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("xy", SLE_OBJECT_ADDRESS(Depot, xy), SaveLoadVersion::MinVersion, SaveLoadVersion::MultipleRoadStops),
	SaveLoad::Variable<VarFileType::U32>("xy", SLE_OBJECT_ADDRESS(Depot, xy), SaveLoadVersion::MultipleRoadStops),
	SaveLoad::Variable<VarFileType::U16>("town_index", SLE_GLOBAL_ADDRESS(_town_index), SaveLoadVersion::MinVersion, SaveLoadVersion::UniqueDepotNames),
	SaveLoad::Reference<SLRefType::Town>("town", SLE_OBJECT_ADDRESS(Depot, town), SaveLoadVersion::UniqueDepotNames),
	SaveLoad::Variable<VarFileType::U16>("town_cn", SLE_OBJECT_ADDRESS(Depot, town_cn), SaveLoadVersion::UniqueDepotNames),
	SaveLoad::String("name", SLE_OBJECT_ADDRESS(Depot, name), {}, SaveLoadVersion::UniqueDepotNames),
	SaveLoad::Variable<VarFileType::I32>("build_date", SLE_OBJECT_ADDRESS(Depot, build_date), SaveLoadVersion::NewGRFDepotBuildDate),
};

/** Native depot identities and town references; portal storage reuses this existing pool and wire format. */
struct DEPTChunkHandler : ChunkHandler {
	DEPTChunkHandler() : ChunkHandler("DEPT", ChunkType::Table) {}

	void Save() const override
	{
		SlTableHeader(_depot_desc);

		for (Depot *depot : Depot::Iterate()) {
			SlSetArrayIndex(depot->index);
			SlObject(depot, _depot_desc);
		}
	}

	void Load() const override
	{
		const std::vector<SaveLoad> slt = SlCompatTableHeader(_depot_desc, _depot_sl_compat);

		int index;

		while ((index = SlIterateArray()) != -1) {
			Depot *depot = Depot::CreateAtIndex(DepotID(index));
			SlObject(depot, slt);

			/* Set the town 'pointer' so we can restore it later. */
			if (IsSavegameVersionBefore(SaveLoadVersion::UniqueDepotNames)) depot->town = reinterpret_cast<Town *>(static_cast<size_t>(_town_index.base()));
		}
	}

	/** Restore native town pointers and validate both ends of a portal-backed depot identity after map loading. */
	void FixPointers() const override
	{
		for (Depot *depot : Depot::Iterate()) {
			SlObject(depot, _depot_desc);
			if (IsSavegameVersionBefore(SaveLoadVersion::UniqueDepotNames)) depot->town = Town::Get((size_t)depot->town);
			if (SlIsFixingPointers() && IsValidTile(depot->xy) && IsLayerPortalTile(depot->xy)) {
				const auto key = GetLayerPortalPairKey(depot->xy);
				// A depot may name only the real upper magazine with an explicit
				// matching map identity. Old files cannot assert this new meaning.
				if (IsSavegameVersionBefore(SaveLoadVersion::TBMHangarDepots) || !key || *key != depot->xy ||
						!IsLayerPortalHangarTile(depot->xy) || GetLayerPortalHangarDepotIndex(depot->xy) != depot->index) SlErrorCorrupt("Invalid portal hangar depot");
			}
		}
	}
};

static const DEPTChunkHandler DEPT;
static const ChunkHandlerRef depot_chunk_handlers[] = {
	DEPT,
};

extern const ChunkHandlerTable _depot_chunk_handlers(depot_chunk_handlers);
