/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file map_sl.cpp Code handling saving and loading of map. @see docs/zz-reach2/architecture/savegame/archi-savegame.md, docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#include "../stdafx.h"

#include "saveload.h"
#include "saveload_error.hpp"
#include "compat/map_sl_compat.h"

#include "../map_func.h"
#include "../map_layer.h"
#include "../surface_supply_order_runtime.h"
#include "../core/bitmath_func.hpp"
#include "../fios.h"

#include "../safeguards.h"

static uint32_t _map_dim_x;
static uint32_t _map_dim_y;
static uint8_t _map_layers;

/** How many layers the file actually stored. Plane loaders must not read past that. */
uint8_t _map_file_layers = 1;

static const SaveLoad _map_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("dim_x", SLE_GLOBAL_ADDRESS(_map_dim_x), SaveLoadVersion::MultipleRoadStops),
	SaveLoad::Variable<VarFileType::U32>("dim_y", SLE_GLOBAL_ADDRESS(_map_dim_y), SaveLoadVersion::MultipleRoadStops),
	SaveLoad::Variable<VarFileType::U8>("layers", SLE_GLOBAL_ADDRESS(_map_layers), SaveLoadVersion::MapLayers),
};

/** Require original current-era geometry fields before either mode can use a MAPS row.
 * @param description Candidate's actual decoded MAPS schema; previous/default dimensions grant no order-reference permission.
 */
static void CheckSurfaceSupplyMapFields(const std::vector<SaveLoad> &description)
{
	if (!SlHasSaveCapability(SaveCapability::SurfaceSupplyOrdersA) && !SlHasSaveCapability(SaveCapability::SurfaceSupplyOrdersB)) return; // Either original paid-supply dialect requires its own complete MAPS evidence; unrelated older files retain native behavior.
	// Each current file must carry its own complete packed geometry; omitting a dimension/layer field cannot borrow a preceding file's static parser value.
	for (const auto field : {"dim_x", "dim_y", "layers"}) if (std::ranges::find(description, field, &SaveLoad::name) == description.end()) SlErrorCorrupt("Missing native supply map geometry field");
}

/**
 * Tiles the open file stored.
 * A RIFF length is 28 bits. A 4096 by 4096 map of four layers still fits.
 * @return Plane size times the file's layer count.
 */
static uint MapFileTileCount()
{
	return Map::PlaneSize() * _map_file_layers;
}

/** Original native map geometry; current paid-order references require explicit same-file dimensions in full/copied modes. */
struct MAPSChunkHandler : ChunkHandler {
	MAPSChunkHandler() : ChunkHandler("MAPS", ChunkType::Table) {}

	void Save() const override
	{
		SlTableHeader(_map_desc);

		_map_dim_x = Map::SizeX();
		_map_dim_y = Map::SizeY();
		_map_layers = static_cast<uint8_t>(Map::LayerCount());

		SlSetArrayIndex(0);
		SlGlobList(_map_desc);
	}

	/** Decode original geometry and retain independent order-reference evidence before allocating the native map. */
	void Load() const override
	{
		const std::vector<SaveLoad> slt = SlCompatTableHeader(_map_desc, _map_sl_compat);
		CheckSurfaceSupplyMapFields(slt);

		//Files written before MapLayers have no layers field, so they are one surface plane.
		_map_layers = 1;
		_map_file_layers = 1;
		if (!IsSavegameVersionBefore(SaveLoadVersion::RiffToArray) && SlIterateArray() == -1) return;
		SlGlobList(slt);
		if (!IsSavegameVersionBefore(SaveLoadVersion::RiffToArray) && SlIterateArray() != -1) SlErrorCorrupt("Too many MAPS entries");

		//H-3: the file's count sizes every plane read, so a count this build cannot store is a corrupt file.
		if (!MultiLayerMap::IsStorableLayerCount(_map_layers)) SlErrorCorrupt("Invalid map layer count");
		//Before Deep Underground a file holds at most Surface, Underground and Space.
		if (IsSavegameVersionBefore(SaveLoadVersion::DeepUnderground) && _map_layers > 3) SlErrorCorrupt("Invalid map layer count");
		_map_file_layers = _map_layers;
		// Observe only actual bounded original MAPS fields; independent supply adoption cannot borrow initialized/live geometry, and malformed dimensions fail before allocation.
		if (!ObserveLoadedSurfaceSupplyMap(_map_dim_x, _map_dim_y, _map_layers)) SlErrorCorrupt("Invalid/repeated native supply map geometry");
		//The live world always has every layer. Missing planes are filled in AfterLoadGame.
		Map::Allocate(_map_dim_x, _map_dim_y, MAX_MAP_LAYERS);
	}

	/** Copy only this file's original bounded geometry; no current map supplies absent order-reference fields.
	 * @param length Framework-owned chunk extent; native table framing supplies the actual field bounds.
	 */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		const std::vector<SaveLoad> slt = SlCompatTableHeader(_map_desc, _map_sl_compat);
		CheckSurfaceSupplyMapFields(slt);

		_map_layers = 1;
		if (!IsSavegameVersionBefore(SaveLoadVersion::RiffToArray) && SlIterateArray() == -1) return;
		SlGlobList(slt);
		if (!IsSavegameVersionBefore(SaveLoadVersion::RiffToArray) && SlIterateArray() != -1) SlErrorCorrupt("Too many MAPS entries");

		//H-3: the file's count sizes every plane read, so a count this build cannot store is a corrupt file.
		if (!MultiLayerMap::IsStorableLayerCount(_map_layers)) SlErrorCorrupt("Invalid map layer count");
		//Before Deep Underground a file holds at most Surface, Underground and Space.
		if (IsSavegameVersionBefore(SaveLoadVersion::DeepUnderground) && _map_layers > 3) SlErrorCorrupt("Invalid map layer count");
		_load_check_data.map_size_x = _map_dim_x;
		_load_check_data.map_size_y = _map_dim_y;
		_load_check_data.map_layers = _map_layers;
		if (!Map::DecodeStoredTile(TileIndex{0}, _map_dim_x, _map_dim_y, _map_layers)) SlErrorCorrupt("Invalid copied native map dimensions");
		if (_load_check_data.native_map_dimensions_present) SlErrorCorrupt("Repeated copied native map dimensions");
		_load_check_data.native_map_dimensions_present = true;
	}
};

static constexpr uint MAP_SL_BUF_SIZE = MIN_MAP_SIZE * MIN_MAP_SIZE; ///< Buffer size for saving/loading the map array. Sized to the smallest map.

struct MAPTChunkHandler : ChunkHandler {
	MAPTChunkHandler() : ChunkHandler("MAPT", ChunkType::Riff) {}

	void Load() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = MapFileTileCount();

		for (TileIndex i{}; i != size;) {
			SlCopy<VarFileType::U8>(buf);
			for (auto b : buf) Tile(i++).type() = b;
		}
	}

	void Save() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = Map::Size();

		SlSetLength(size);
		for (TileIndex i{}; i != size;) {
			for (auto &b : buf) b = Tile(i++).type();
			SlCopy<VarFileType::U8>(buf);
		}
	}
};

struct MAPHChunkHandler : ChunkHandler {
	MAPHChunkHandler() : ChunkHandler("MAPH", ChunkType::Riff) {}

	void Load() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = MapFileTileCount();

		for (TileIndex i{}; i != size;) {
			SlCopy<VarFileType::U8>(buf);
			for (auto b : buf) Tile(i++).height() = b;
		}
	}

	void Save() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = Map::Size();

		SlSetLength(size);
		for (TileIndex i{}; i != size;) {
			for (auto &b : buf) b = Tile(i++).height();
			SlCopy<VarFileType::U8>(buf);
		}
	}
};

struct MAPOChunkHandler : ChunkHandler {
	MAPOChunkHandler() : ChunkHandler("MAPO", ChunkType::Riff) {}

	void Load() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = MapFileTileCount();

		for (TileIndex i{}; i != size;) {
			SlCopy<VarFileType::U8>(buf);
			for (auto b : buf) Tile(i++).m1() = b;
		}
	}

	void Save() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = Map::Size();

		SlSetLength(size);
		for (TileIndex i{}; i != size;) {
			for (auto &b : buf) b = Tile(i++).m1();
			SlCopy<VarFileType::U8>(buf);
		}
	}
};

struct MAP2ChunkHandler : ChunkHandler {
	MAP2ChunkHandler() : ChunkHandler("MAP2", ChunkType::Riff) {}

	void Load() const override
	{
		std::array<uint16_t, MAP_SL_BUF_SIZE> buf;
		uint size = MapFileTileCount();

		for (TileIndex i{}; i != size;) {
			SlCopy(buf.data(), MAP_SL_BUF_SIZE,
				/* In those versions the m2 was 8 bits */
				IsSavegameVersionBefore(SaveLoadVersion::BigMap) ? VarType{VarFileType::U8, VarMemType::U16} : VarTypes::U16
			);
			for (auto b : buf) Tile(i++).m2() = b;
		}
	}

	void Save() const override
	{
		std::array<uint16_t, MAP_SL_BUF_SIZE> buf;
		uint size = Map::Size();

		SlSetLength(static_cast<uint32_t>(size) * sizeof(uint16_t));
		for (TileIndex i{}; i != size;) {
			for (auto &b : buf) b = Tile(i++).m2();
			SlCopy<VarFileType::U16>(buf);
		}
	}
};

struct M3LOChunkHandler : ChunkHandler {
	M3LOChunkHandler() : ChunkHandler("M3LO", ChunkType::Riff) {}

	void Load() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = MapFileTileCount();

		for (TileIndex i{}; i != size;) {
			SlCopy<VarFileType::U8>(buf);
			for (auto b : buf) Tile(i++).m3() = b;
		}
	}

	void Save() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = Map::Size();

		SlSetLength(size);
		for (TileIndex i{}; i != size;) {
			for (auto &b : buf) b = Tile(i++).m3();
			SlCopy<VarFileType::U8>(buf);
		}
	}
};

struct M3HIChunkHandler : ChunkHandler {
	M3HIChunkHandler() : ChunkHandler("M3HI", ChunkType::Riff) {}

	void Load() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = MapFileTileCount();

		for (TileIndex i{}; i != size;) {
			SlCopy<VarFileType::U8>(buf);
			for (auto b : buf) Tile(i++).m4() = b;
		}
	}

	void Save() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = Map::Size();

		SlSetLength(size);
		for (TileIndex i{}; i != size;) {
			for (auto &b : buf) b = Tile(i++).m4();
			SlCopy<VarFileType::U8>(buf);
		}
	}
};

struct MAP5ChunkHandler : ChunkHandler {
	MAP5ChunkHandler() : ChunkHandler("MAP5", ChunkType::Riff) {}

	void Load() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = MapFileTileCount();

		for (TileIndex i{}; i != size;) {
			SlCopy<VarFileType::U8>(buf);
			for (auto b : buf) Tile(i++).m5() = b;
		}
	}

	void Save() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = Map::Size();

		SlSetLength(size);
		for (TileIndex i{}; i != size;) {
			for (auto &b : buf) b = Tile(i++).m5();
			SlCopy<VarFileType::U8>(buf);
		}
	}
};

struct MAPEChunkHandler : ChunkHandler {
	MAPEChunkHandler() : ChunkHandler("MAPE", ChunkType::Riff) {}

	void Load() const override
	{
		uint size = MapFileTileCount();

		if (IsSavegameVersionBefore(SaveLoadVersion::BridgeWormhole)) {
			/* Since this loads 4 tiles per read byte, amend the buffer size to suit. */
			std::array<uint8_t, MAP_SL_BUF_SIZE / 4> buf;
			for (TileIndex i{}; i != size;) {
				SlCopy<VarFileType::U8>(buf);
				for (auto b : buf) {
					Tile(i++).m6() = GB(b, 0, 2);
					Tile(i++).m6() = GB(b, 2, 2);
					Tile(i++).m6() = GB(b, 4, 2);
					Tile(i++).m6() = GB(b, 6, 2);
				}
			}
		} else {
			std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
			for (TileIndex i{}; i != size;) {
				SlCopy<VarFileType::U8>(buf);
				for (auto b : buf) Tile(i++).m6() = b;
			}
		}
	}

	void Save() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = Map::Size();

		SlSetLength(size);
		for (TileIndex i{}; i != size;) {
			for (auto &b : buf) b = Tile(i++).m6();
			SlCopy<VarFileType::U8>(buf);
		}
	}
};

struct MAP7ChunkHandler : ChunkHandler {
	MAP7ChunkHandler() : ChunkHandler("MAP7", ChunkType::Riff) {}

	void Load() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = MapFileTileCount();

		for (TileIndex i{}; i != size;) {
			SlCopy<VarFileType::U8>(buf);
			for (auto b : buf) Tile(i++).m7() = b;
		}
	}

	void Save() const override
	{
		std::array<uint8_t, MAP_SL_BUF_SIZE> buf;
		uint size = Map::Size();

		SlSetLength(size);
		for (TileIndex i{}; i != size;) {
			for (auto &b : buf) b = Tile(i++).m7();
			SlCopy<VarFileType::U8>(buf);
		}
	}
};

struct MAP8ChunkHandler : ChunkHandler {
	MAP8ChunkHandler() : ChunkHandler("MAP8", ChunkType::Riff) {}

	void Load() const override
	{
		std::array<uint16_t, MAP_SL_BUF_SIZE> buf;
		uint size = MapFileTileCount();

		for (TileIndex i{}; i != size;) {
			SlCopy<VarFileType::U16>(buf);
			for (auto b : buf) Tile(i++).m8() = b;
		}
	}

	void Save() const override
	{
		std::array<uint16_t, MAP_SL_BUF_SIZE> buf;
		uint size = Map::Size();

		SlSetLength(static_cast<uint32_t>(size) * sizeof(uint16_t));
		for (TileIndex i{}; i != size;) {
			for (auto &b : buf) b = Tile(i++).m8();
			SlCopy<VarFileType::U16>(buf);
		}
	}
};

static const MAPSChunkHandler MAPS;
static const MAPTChunkHandler MAPT;
static const MAPHChunkHandler MAPH;
static const MAPOChunkHandler MAPO;
static const MAP2ChunkHandler MAP2;
static const M3LOChunkHandler M3LO;
static const M3HIChunkHandler M3HI;
static const MAP5ChunkHandler MAP5;
static const MAPEChunkHandler MAPE;
static const MAP7ChunkHandler MAP7;
static const MAP8ChunkHandler MAP8;
static const ChunkHandlerRef map_chunk_handlers[] = {
	MAPS,
	MAPT,
	MAPH,
	MAPO,
	MAP2,
	M3LO,
	M3HI,
	MAP5,
	MAPE,
	MAP7,
	MAP8,
};

extern const ChunkHandlerTable _map_chunk_handlers(map_chunk_handlers);
