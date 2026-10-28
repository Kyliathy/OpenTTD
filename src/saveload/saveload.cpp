/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file saveload.cpp All actions handling saving and loading goes on in this file.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group16; historical natural freight retains a joined preview even after all Space hulls/sites disappear.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Groups 4 and 10; the interim writer and its capability context.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Groups 13 and 17B; files from a development build refuse with their own message.
 *
 * The general actions are as follows for saving a game (loading is analogous):
 * <ol>
 * <li>initialize the writer by creating a temporary memory-buffer for it
 * <li>go through all to-be saved elements, each 'chunk' (#ChunkHandler) prefixed by a label
 * <li>use their description array (#SaveLoad) to know what elements to save and in what version
 *    of the game it was active (used when loading)
 * <li>write all data byte-by-byte to the temporary buffer so it is endian-safe
 * <li>when the buffer is full; flush it to the output (eg save to file) (_sl.buf, _sl.bufp, _sl.bufe)
 * <li>repeat this until everything is done, and flush any remaining output to file
 * </ol>
 */

#include "../stdafx.h"
#include "../debug.h"
#include "../station_base.h"
#include "../thread.h"
#include "../town.h"
#include "../network/network.h"
#include "../window_func.h"
#include "../strings_func.h"
#include "../core/endian_func.hpp"
#include "../core/string_builder.hpp"
#include "../core/string_consumer.hpp"
#include "../vehicle_base.h"
#include "../company_func.h"
#include "../timer/timer_game_economy.h"
#include "../autoreplace_base.h"
#include "../roadstop_base.h"
#include "../linkgraph/linkgraph.h"
#include "../linkgraph/linkgraphjob.h"
#include "../statusbar_gui.h"
#include "../fileio_func.h"
#include "../gamelog.h"
#include "../string_func.h"
#include "../fios.h"
#include "../multilayer_content.h"
#include "../multilayer_state.h"
#include "../asteroid.h"
#include "../launch_center.h"
#include "../multilayer_space_content.h"
#include "../space_orders.h"
#include "space_cargo_preview.h"
#include "../excavation_state.h"
#include "../layer_portal_cmd.h"
#include "../tbm.h"
#include "../spoil_receiver.h"
#include "../multilayer_industry.h" // r2mlvs6-mines-and-resource-chains.md: independent preview and chunk registration.
#include "../layer_industry_processor.h"
#include "../tbm_exchange.h"
#include "../utility_power.h"
#include "../transport_tube.h"
#include "../geothermal_collector.h"
#include "../geothermal_generator.h"
#include "../transport_tube_geothermal.h"
#include "../geothermal_riser.h"
#include "../surface_supply_order_runtime.h"
#include "../geothermal_heat.h"
#include "../error.h"
#include "../strings_type.h"
#include "../newgrf_railtype.h"
#include "../newgrf_roadtype.h"
#include "../settings_internal.h"
#include "saveload_error.hpp"
#include "saveload_filter.h"
#include "saveload_func.h"
#include "saveload_internal.h"
#include "save_framing.h"
#include "save_envelope.h"

#include <atomic>
#ifdef __EMSCRIPTEN__
#	include <emscripten.h>
#endif

#ifdef WITH_LZO
#include <lzo/lzo1x.h>
#endif

#if defined(WITH_ZLIB)
#include <zlib.h>
#endif /* WITH_ZLIB */

#if defined(WITH_LIBLZMA)
#include <lzma.h>
#endif /* WITH_LIBLZMA */

#include "../table/control_codes.h"
#include "table/strings.h"

#include "../safeguards.h"

extern const SaveLoadVersion SAVEGAME_VERSION{to_underlying(SaveLoadVersion::MaxVersion) - 1}; ///< Current savegame version of OpenTTD.
/* m2o28-1-merge-oct28.md Group 22: the writer is merged 446, the last enumerator before MaxVersion. These checks fail
 * the build if an enumerator is ever appended without moving the writer and its MLLN tuple together. */
static_assert(SAVEGAME_VERSION == SaveLoadVersion::SpaceMiningLifecycle);
static_assert(to_underlying(SaveLoadVersion::SpaceMiningLifecycle) == CURRENT_MERGED_SAVE_VERSION);

SavegameType _savegame_type; ///< type of savegame we are loading
FileToSaveLoad _file_to_saveload; ///< File to save or load in the openttd loop.

uint32_t _ttdp_version;         ///< version of TTDP savegame (if applicable)
SaveLoadVersion _sl_version;  ///< the major savegame version identifier
uint8_t   _sl_minor_version;     ///< the minor savegame version, DO NOT USE!
std::string _savegame_format; ///< how to compress savegames
bool _do_autosave;            ///< are we doing an autosave at the moment?

/** What are we currently doing? */
enum class SaveLoadAction : uint8_t {
	Load, ///< loading
	Save, ///< saving
	Ptrs, ///< fixing pointers
	Null, ///< null all pointers (on loading error)
	LoadCheck, ///< partial loading into #_load_check_data
};

enum class NeedLength : uint8_t {
	None, ///< not working in NeedLength mode
	WantLength, ///< writing length and data
	CalcLength, ///< need to calculate the length
};

/** Save in chunks of 128 KiB. */
static const size_t MEMORY_CHUNK_SIZE = 128 * 1024;

/** A buffer for reading (and buffering) savegame data. */
struct ReadBuffer {
	uint8_t buf[MEMORY_CHUNK_SIZE]; ///< Buffer we're going to read from.
	uint8_t *bufp = nullptr; ///< Location we're at reading the buffer.
	uint8_t *bufe = nullptr; ///< End of the buffer we can read from.
	std::shared_ptr<LoadFilter> reader{}; ///< The filter used to actually read.
	size_t read = 0; ///< The amount of read bytes so far from the filter.

	/**
	 * Initialise our variables.
	 * @param reader The filter to actually read data.
	 */
	ReadBuffer(std::shared_ptr<LoadFilter> reader) : reader(std::move(reader))
	{
	}

	inline uint8_t ReadByte()
	{
		if (this->bufp == this->bufe) {
			size_t len = this->reader->Read(this->buf, lengthof(this->buf));
			if (len == 0) SlErrorCorrupt("Unexpected end of chunk");

			this->read += len;
			this->bufp = this->buf;
			this->bufe = this->buf + len;
		}

		return *this->bufp++;
	}

	/**
	 * Get the size of the memory dump made so far.
	 * @return The size.
	 */
	size_t GetSize() const
	{
		return this->read - (this->bufe - this->bufp);
	}
};


/** Container for dumping the savegame (quickly) to memory. */
struct MemoryDumper {
	std::vector<std::unique_ptr<uint8_t[]>> blocks{}; ///< Buffer with blocks of allocated memory.
	uint8_t *buf = nullptr; ///< Buffer we're going to write to.
	uint8_t *bufe = nullptr; ///< End of the buffer we write to.

	/**
	 * Write a single byte into the dumper.
	 * @param b The byte to write.
	 */
	inline void WriteByte(uint8_t b)
	{
		/* Are we at the end of this chunk? */
		if (this->buf == this->bufe) {
			this->buf = this->blocks.emplace_back(std::make_unique<uint8_t[]>(MEMORY_CHUNK_SIZE)).get();
			this->bufe = this->buf + MEMORY_CHUNK_SIZE;
		}

		*this->buf++ = b;
	}

	/**
	 * Flush this dumper into a writer.
	 * @param writer The filter we want to use.
	 */
	void Flush(std::shared_ptr<SaveFilter> writer)
	{
		uint i = 0;
		size_t t = this->GetSize();

		while (t > 0) {
			size_t to_write = std::min(MEMORY_CHUNK_SIZE, t);

			writer->Write(this->blocks[i++].get(), to_write);
			t -= to_write;
		}

		writer->Finish();
	}

	/**
	 * Get the size of the memory dump made so far.
	 * @return The size.
	 */
	size_t GetSize() const
	{
		return this->blocks.size() * MEMORY_CHUNK_SIZE - (this->bufe - this->buf);
	}
};

/** The saveload struct, containing reader-writer functions, buffer, version, etc. */
struct SaveLoadParams {
	SaveLoadAction action;               ///< are we doing a save or a load atm.
	NeedLength need_length;              ///< working in NeedLength (Autolength) mode?
	ChunkType chunk_type; ///< The type of chunk we are reading or writing.
	bool error;                          ///< did an error occur or not

	size_t obj_len;                      ///< the length of the current object we are busy with
	int array_index, last_array_index;   ///< in the case of an array, the current and last positions
	bool expect_table_header;            ///< In the case of a table, if the header is saved/loaded.

	std::unique_ptr<MemoryDumper> dumper; ///< Memory dumper to write the savegame to.
	std::shared_ptr<SaveFilter> sf; ///< Filter to write the savegame to.

	std::unique_ptr<ReadBuffer> reader; ///< Savegame reading buffer.
	std::shared_ptr<LoadFilter> lf; ///< Filter to read the savegame from.

	StringID error_str;                  ///< the translatable error message to show
	std::string extra_msg;               ///< the error message

	bool saveinprogress;                 ///< Whether there is currently a save in progress.
	std::optional<SaveLineageResult> lineage; ///< Accepted original interpretation, owned only by the current DoLoad scope.
};

static SaveLoadParams _sl; ///< Parameters used for/at saveload.

/** @copydoc GetLoadCapabilities */
std::optional<SaveLineageResult> GetLoadCapabilities()
{
	return _sl.lineage;
}

/** Identify the exact A feature denoted by an unchanged post-fork A version enumerator.
 * @param boundary Original destination-A version name, never a donor's conflicting numeric alias.
 * @return Its explicit A capability, or no mapping for shared/future version boundaries.
 */
static std::optional<SaveCapability> DestinationVersionCapability(SaveLoadVersion boundary)
{
	switch (boundary) {
		case SaveLoadVersion::OrbitalAssemblyFacilities: return SaveCapability::OrbitalAssemblyFacilities;
		case SaveLoadVersion::OrbitalManufacturingReservations: return SaveCapability::OrbitalManufacturingReservations;
		case SaveLoadVersion::OrbitalManufacturingMaterialization: return SaveCapability::OrbitalManufacturingMaterialization;
		case SaveLoadVersion::OrbitalManufacturingClones: return SaveCapability::OrbitalManufacturingClones;
		case SaveLoadVersion::SpaceReplacementSettlement: return SaveCapability::SpaceReplacementSettlement;
		case SaveLoadVersion::SpaceOrderedActions: return SaveCapability::SpaceOrderedActions;
		case SaveLoadVersion::SpaceHeavyMinerCommissioning: return SaveCapability::SpaceHeavyMinerCommissioning;
		case SaveLoadVersion::SpaceHeavyMinerMissions: return SaveCapability::SpaceHeavyMinerMissions;
		case SaveLoadVersion::SpaceHeavyMinerSupplies: return SaveCapability::SpaceHeavyMinerSupplies;
		case SaveLoadVersion::SpaceHeavyMinerDescents: return SaveCapability::SpaceHeavyMinerDescents;
		case SaveLoadVersion::SpaceHeavyMinerOnboardServices: return SaveCapability::SpaceHeavyMinerOnboardServices;
		case SaveLoadVersion::SpaceHeavyMinerContainedServices: return SaveCapability::SpaceHeavyMinerContainedServices;
		case SaveLoadVersion::SpaceCargoDimensions: return SaveCapability::SpaceCargoDimensions;
		case SaveLoadVersion::OrbitalRecipeProcessing: return SaveCapability::OrbitalRecipeProcessing;
		case SaveLoadVersion::CompanySupplyPackets: return SaveCapability::CompanySupplyPackets;
		case SaveLoadVersion::SurfaceSupplyOrders: return SaveCapability::SurfaceSupplyOrdersFixed;
		default: return std::nullopt;
	}
}

/** @copydoc SlHasSaveCapability */
bool SlHasSaveCapability(SaveCapability capability)
{
	if (_sl.lineage) return HasSaveCapability(*_sl.lineage, capability);
	// Outside a load, serialization and synthetic pointer cleanup describe the
	// current writer, never the format/version of a previously copied candidate.
	// The writer is merged 446 (the static_assert beside SAVEGAME_VERSION holds it),
	// so saving grants every A, B and Space capability (m2o28 Group 22).
	return HasSaveCapability({SaveLineageStatus::Accepted, SaveFormat::Unified, to_underlying(SAVEGAME_VERSION)}, capability);
}

/** @copydoc IsSavegameVersionBefore */
bool IsSavegameVersionBefore(SaveLoadVersion major, uint8_t minor)
{
	// The destination enum's425-440 names denote A features. An admitted B save
	// retains its actual numeric version but cannot activate those foreign layouts.
	if (_sl.lineage) {
		if (const auto capability = DestinationVersionCapability(major)) {
			if (!HasSaveCapability(*_sl.lineage, *capability)) return true;
		}
	}
	// Shared history and original minor-version distinctions keep their native
	// numeric semantics; no global or whole-world virtual version is installed.
	return _sl_version < major || (minor > 0 && _sl_version == major && _sl_minor_version < minor);
}

/** @copydoc IsSavegameVersionBeforeOrAt */
bool IsSavegameVersionBeforeOrAt(SaveLoadVersion major)
{
	// Inclusive A boundaries end just before their next explicit feature. B's
	// frozen Space prefix remains older than every such A addition, even at the
	// same number; pre-fork repair gates keep their original numeric semantics.
	if (_sl.lineage && DestinationVersionCapability(major)) {
		const auto next = static_cast<SaveLoadVersion>(to_underlying(major) + 1);
		return IsSavegameVersionBefore(next);
	}
	return _sl_version <= major;
}

/** One serial native attempt retains lineage through both chunk dispatch and afterload, then revokes it on every exit. */
class ScopedLoadLineage final {
public:
	/** Start with no retained choice from an earlier successful or refused load. */
	ScopedLoadLineage() { _sl.lineage.reset(); }
	/** Success, refusal and exception paths all revoke this attempt's capabilities. */
	~ScopedLoadLineage() { _sl.lineage.reset(); }
	/** Bind only a successful detached classification before native dispatch.
	 * @param result Accepted same-file original interpretation; no version renumbering occurs.
	 */
	void Bind(const SaveLineageResult &result)
	{
		assert(result.status == SaveLineageStatus::Accepted);
		_sl.lineage = result;
	}
};

/**
 * Allow scalar feature-reference validators to skip cleanup's synthetic current-version/nulling pass.
 * @return True only while actual loaded references are being fixed before gameplay resumes.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md
 */
bool SlIsFixingPointers()
{
	return _sl.action == SaveLoadAction::Ptrs;
}

/**
 * Read the stable handler registry, including the uniquely versioned MLMD scaffold.
 * @return Ordered legacy/feature handlers; no existing chunk identity or order is reinterpreted.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 */
static const std::vector<ChunkHandlerRef> &ChunkHandlers()
{
	/* These define the chunks */
	extern const ChunkHandlerTable _save_lineage_chunk_handlers;
	extern const ChunkHandlerTable _gamelog_chunk_handlers;
	extern const ChunkHandlerTable _map_chunk_handlers;
	extern const ChunkHandlerTable _misc_chunk_handlers;
	extern const ChunkHandlerTable _multilayer_chunk_handlers;
	extern const ChunkHandlerTable _asteroid_chunk_handlers;
	extern const ChunkHandlerTable _space_order_chunk_handlers;
	extern const ChunkHandlerTable _space_production_chunk_handlers;
	extern const ChunkHandlerTable _space_manifest_chunk_handlers;
	extern const ChunkHandlerTable _space_pod_chunk_handlers;
	extern const ChunkHandlerTable _space_structure_chunk_handlers;
	extern const ChunkHandlerTable _orbital_recipe_chunk_handlers;
	extern const ChunkHandlerTable _surface_supply_order_chunk_handlers;
	extern const ChunkHandlerTable _space_transition_chunk_handlers;
	extern const ChunkHandlerTable _space_descent_chunk_handlers;
	extern const ChunkHandlerTable _launch_center_chunk_handlers;
	extern const ChunkHandlerTable _geology_chunk_handlers;
	extern const ChunkHandlerTable _excavation_chunk_handlers;
	extern const ChunkHandlerTable _portal_work_chunk_handlers;
	extern const ChunkHandlerTable _tbm_project_chunk_handlers;
	extern const ChunkHandlerTable _spoil_receiver_chunk_handlers;
	extern const ChunkHandlerTable _layer_mine_chunk_handlers;
	extern const ChunkHandlerTable _layer_processor_chunk_handlers;
	extern const ChunkHandlerTable _tbm_exchange_chunk_handlers;
	extern const ChunkHandlerTable _utility_conversion_chunk_handlers;
	extern const ChunkHandlerTable _geothermal_generator_chunk_handlers;
	extern const ChunkHandlerTable _tube_geothermal_chunk_handlers;
	extern const ChunkHandlerTable _utility_power_chunk_handlers;
	extern const ChunkHandlerTable _tube_power_chunk_handlers;
	extern const ChunkHandlerTable _tube_vehicle_chunk_handlers;
	extern const ChunkHandlerTable _geothermal_site_chunk_handlers;
	extern const ChunkHandlerTable _geothermal_collector_chunk_handlers;
	extern const ChunkHandlerTable _geothermal_riser_chunk_handlers;
	extern const ChunkHandlerTable _geothermal_heat_route_chunk_handlers;
	extern const ChunkHandlerTable _name_chunk_handlers;
	extern const ChunkHandlerTable _cheat_chunk_handlers;
	extern const ChunkHandlerTable _setting_chunk_handlers;
	extern const ChunkHandlerTable _company_chunk_handlers;
	extern const ChunkHandlerTable _engine_chunk_handlers;
	extern const ChunkHandlerTable _veh_chunk_handlers;
	extern const ChunkHandlerTable _waypoint_chunk_handlers;
	extern const ChunkHandlerTable _depot_chunk_handlers;
	extern const ChunkHandlerTable _order_chunk_handlers;
	extern const ChunkHandlerTable _town_chunk_handlers;
	extern const ChunkHandlerTable _sign_chunk_handlers;
	extern const ChunkHandlerTable _station_chunk_handlers;
	extern const ChunkHandlerTable _industry_chunk_handlers;
	extern const ChunkHandlerTable _economy_chunk_handlers;
	extern const ChunkHandlerTable _subsidy_chunk_handlers;
	extern const ChunkHandlerTable _cargomonitor_chunk_handlers;
	extern const ChunkHandlerTable _goal_chunk_handlers;
	extern const ChunkHandlerTable _story_page_chunk_handlers;
	extern const ChunkHandlerTable _league_chunk_handlers;
	extern const ChunkHandlerTable _ai_chunk_handlers;
	extern const ChunkHandlerTable _game_chunk_handlers;
	extern const ChunkHandlerTable _animated_tile_chunk_handlers;
	extern const ChunkHandlerTable _newgrf_chunk_handlers;
	extern const ChunkHandlerTable _group_chunk_handlers;
	extern const ChunkHandlerTable _cargopacket_chunk_handlers;
	extern const ChunkHandlerTable _autoreplace_chunk_handlers;
	extern const ChunkHandlerTable _labelmaps_chunk_handlers;
	extern const ChunkHandlerTable _linkgraph_chunk_handlers;
	extern const ChunkHandlerTable _airport_chunk_handlers;
	extern const ChunkHandlerTable _object_chunk_handlers;
	extern const ChunkHandlerTable _persistent_storage_chunk_handlers;
	extern const ChunkHandlerTable _water_region_chunk_handlers;
	extern const ChunkHandlerTable _randomizer_chunk_handlers;

	/** List of all chunks in a savegame. */
	static const ChunkHandlerTable _chunk_handler_tables[] = {
		_save_lineage_chunk_handlers,
		_gamelog_chunk_handlers,
		_map_chunk_handlers,
		_misc_chunk_handlers,
		_multilayer_chunk_handlers,
		_asteroid_chunk_handlers,
		_space_order_chunk_handlers,
		_space_production_chunk_handlers,
		_space_manifest_chunk_handlers,
		_space_pod_chunk_handlers,
		_space_structure_chunk_handlers,
		_orbital_recipe_chunk_handlers,
		_surface_supply_order_chunk_handlers,
		_space_transition_chunk_handlers,
		_space_descent_chunk_handlers,
		_launch_center_chunk_handlers,
		_geology_chunk_handlers,
		_excavation_chunk_handlers,
		_portal_work_chunk_handlers,
		_tbm_project_chunk_handlers,
		_spoil_receiver_chunk_handlers,
		_layer_mine_chunk_handlers,
		_layer_processor_chunk_handlers,
		_tbm_exchange_chunk_handlers,
		_utility_conversion_chunk_handlers,
		_geothermal_generator_chunk_handlers,
		_tube_geothermal_chunk_handlers,
		_utility_power_chunk_handlers,
		_tube_power_chunk_handlers,
		_tube_vehicle_chunk_handlers,
		_geothermal_site_chunk_handlers,
		_geothermal_collector_chunk_handlers,
		_geothermal_riser_chunk_handlers,
		_geothermal_heat_route_chunk_handlers,
		_name_chunk_handlers,
		_cheat_chunk_handlers,
		_setting_chunk_handlers,
		_veh_chunk_handlers,
		_waypoint_chunk_handlers,
		_depot_chunk_handlers,
		_order_chunk_handlers,
		_industry_chunk_handlers,
		_economy_chunk_handlers,
		_subsidy_chunk_handlers,
		_cargomonitor_chunk_handlers,
		_goal_chunk_handlers,
		_story_page_chunk_handlers,
		_league_chunk_handlers,
		_engine_chunk_handlers,
		_town_chunk_handlers,
		_sign_chunk_handlers,
		_station_chunk_handlers,
		_company_chunk_handlers,
		_ai_chunk_handlers,
		_game_chunk_handlers,
		_animated_tile_chunk_handlers,
		_newgrf_chunk_handlers,
		_group_chunk_handlers,
		_cargopacket_chunk_handlers,
		_autoreplace_chunk_handlers,
		_labelmaps_chunk_handlers,
		_linkgraph_chunk_handlers,
		_airport_chunk_handlers,
		_object_chunk_handlers,
		_persistent_storage_chunk_handlers,
		_water_region_chunk_handlers,
		_randomizer_chunk_handlers,
	};

	static std::vector<ChunkHandlerRef> _chunk_handlers;

	if (_chunk_handlers.empty()) {
		// Stable table/row order preserves legacy handlers while adding each feature's one serialization owner.
		for (auto &chunk_handler_table : _chunk_handler_tables) {
			for (auto &chunk_handler : chunk_handler_table) {
				_chunk_handlers.push_back(chunk_handler);
			}
		}
	}

	return _chunk_handlers;
}

/** Null all pointers (convert index -> nullptr) */
static void SlNullPointers()
{
	_sl.action = SaveLoadAction::Null;
	// Failure cleanup describes the current in-memory schema rather than the
	// refused original wire format; no original capability survives into it.
	_sl.lineage.reset();

	/* We don't want any savegame conversion code to run
	 * during NULLing; especially those that try to get
	 * pointers from other pools. */
	_sl_version = SAVEGAME_VERSION;

	for (const ChunkHandler &ch : ChunkHandlers()) {
		Debug(Facility::Sl, Severity::Notice, "Nulling pointers for {}", ch.GetName());
		ch.FixPointers();
	}

	assert(_sl.action == SaveLoadAction::Null);
}

/**
 * Error handler. Sets everything up to show an error message and to clean
 * up the mess of a partial savegame load.
 * @param string The translatable error message to show.
 * @param extra_msg An extra error message coming from one of the APIs.
 * @note This function does never return as it throws an exception to
 *       break out of all the saveload code.
 */
[[noreturn]] void SlError(StringID string, const std::string &extra_msg)
{
	/* Distinguish between loading into _load_check_data vs. normal save/load. */
	if (_sl.action == SaveLoadAction::LoadCheck) {
		_load_check_data.error = string;
		_load_check_data.error_msg = extra_msg;
	} else {
		_sl.error_str = string;
		_sl.extra_msg = extra_msg;
	}

	/* We have to nullptr all pointers here; we might be in a state where
	 * the pointers are actually filled with indices, which means that
	 * when we access them during cleaning the pool dereferences of
	 * those indices will be made with segmentation faults as result. */
	if (_sl.action == SaveLoadAction::Load || _sl.action == SaveLoadAction::Ptrs) SlNullPointers();

	/* Logging could be active. */
	_gamelog.StopAnyAction();

	throw std::exception();
}

/**
 * Error handler for corrupt savegames. Sets everything up to show the
 * error message and to clean up the mess of a partial savegame load.
 * @param msg Location the corruption has been spotted.
 * @note This function does never return as it throws an exception to
 *       break out of all the saveload code.
 */
[[noreturn]] void SlErrorCorrupt(const std::string &msg)
{
	SlError(STR_GAME_SAVELOAD_ERROR_BROKEN_SAVEGAME, msg);
}


typedef void (*AsyncSaveFinishProc)();                      ///< Callback for when the savegame loading is finished.
static std::atomic<AsyncSaveFinishProc> _async_save_finish; ///< Callback to call when the savegame loading is finished.
static std::thread _save_thread;                            ///< The thread we're using to compress and write a savegame

/**
 * Called by save thread to tell we finished saving.
 * @param proc The callback to call when saving is done.
 */
static void SetAsyncSaveFinish(AsyncSaveFinishProc proc)
{
	if (_exit_game) return;
	while (_async_save_finish.load(std::memory_order_acquire) != nullptr) CSleep(10);

	_async_save_finish.store(proc, std::memory_order_release);
}

/**
 * Handle async save finishes.
 */
void ProcessAsyncSaveFinish()
{
	AsyncSaveFinishProc proc = _async_save_finish.exchange(nullptr, std::memory_order_acq_rel);
	if (proc == nullptr) return;

	proc();

	if (_save_thread.joinable()) {
		_save_thread.join();
	}
}

/**
 * Wrapper for reading a byte from the buffer.
 * @return The read byte.
 */
uint8_t SlReadByte()
{
	return _sl.reader->ReadByte();
}

/**
 * Wrapper for writing a byte to the dumper.
 * @param b The byte to write.
 */
void SlWriteByte(uint8_t b)
{
	_sl.dumper->WriteByte(b);
}

static inline int SlReadUint16()
{
	int x = SlReadByte() << 8;
	return x | SlReadByte();
}

static inline uint32_t SlReadUint32()
{
	uint32_t x = SlReadUint16() << 16;
	return x | SlReadUint16();
}

static inline uint64_t SlReadUint64()
{
	uint32_t x = SlReadUint32();
	uint32_t y = SlReadUint32();
	return static_cast<uint64_t>(x) << 32 | y;
}

static inline void SlWriteUint16(uint16_t v)
{
	SlWriteByte(GB(v, 8, 8));
	SlWriteByte(GB(v, 0, 8));
}

static inline void SlWriteUint32(uint32_t v)
{
	SlWriteUint16(GB(v, 16, 16));
	SlWriteUint16(GB(v,  0, 16));
}

static inline void SlWriteUint64(uint64_t x)
{
	SlWriteUint32(static_cast<uint32_t>(x >> 32));
	SlWriteUint32(static_cast<uint32_t>(x));
}

/**
 * Read the \c ChunkId.
 * @return The read \c ChunkId.
 */
static inline ChunkId SlReadChunkId()
{
	ChunkId label{};
	for (uint8_t &b : label) b = SlReadByte();
	return label;
}

/** Read the original prefix-width quantity shared by native array lengths and sparse indices.
 * @return Exact unsigned quantity; native reporting retains ordinary full-load failure semantics.
 */
static uint SlReadSimpleGamma()
{
	try {
		return ReadSaveGamma(SlReadByte);
	} catch (const SaveFramingError &error) {
		SlErrorCorrupt(error.what());
	}
}

/**
 * Write the header descriptor of an object or an array.
 * If the element is bigger than 127, use 2 bytes for saving
 * and use the highest byte of the first written one as a notice
 * that the length consists of 2 bytes, etc.. like this:
 * 0xxxxxxx
 * 10xxxxxx xxxxxxxx
 * 110xxxxx xxxxxxxx xxxxxxxx
 * 1110xxxx xxxxxxxx xxxxxxxx xxxxxxxx
 * 11110--- xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
 * We could extend the scheme ad infinum to support arbitrarily
 * large chunks, but as sizeof(size_t) == 4 is still very common
 * we don't support anything above 32 bits. That's why in the last
 * case the 3 most significant bits are unused.
 * @param i Index being written
 */

static void SlWriteSimpleGamma(size_t i)
{
	if (i >= (1 << 7)) {
		if (i >= (1 << 14)) {
			if (i >= (1 << 21)) {
				if (i >= (1 << 28)) {
					assert(i <= UINT32_MAX); // We can only support 32 bits for now.
					SlWriteByte(static_cast<uint8_t>(0xF0));
					SlWriteByte(static_cast<uint8_t>(i >> 24));
				} else {
					SlWriteByte(static_cast<uint8_t>(0xE0 | (i >> 24)));
				}
				SlWriteByte(static_cast<uint8_t>(i >> 16));
			} else {
				SlWriteByte(static_cast<uint8_t>(0xC0 | (i >> 16)));
			}
			SlWriteByte(static_cast<uint8_t>(i >> 8));
		} else {
			SlWriteByte(static_cast<uint8_t>(0x80 | (i >> 8)));
		}
	}
	SlWriteByte(static_cast<uint8_t>(i));
}

/**
 * Return how many bytes used to encode a gamma value.
 * @param i The value to encode with gamma encoding.
 * @return The length in bytes.
 */
static inline uint SlGetGammaLength(size_t i)
{
	return 1 + (i >= (1 << 7)) + (i >= (1 << 14)) + (i >= (1 << 21)) + (i >= (1 << 28));
}

static inline uint SlReadSparseIndex()
{
	return SlReadSimpleGamma();
}

static inline void SlWriteSparseIndex(uint index)
{
	SlWriteSimpleGamma(index);
}

static inline uint SlReadArrayLength()
{
	return SlReadSimpleGamma();
}

static inline void SlWriteArrayLength(size_t length)
{
	SlWriteSimpleGamma(length);
}

static inline uint SlGetArrayLength(size_t length)
{
	return SlGetGammaLength(length);
}

/**
 * Container/wrapper for the file type that is used in tables in the save game.
 * This is essentially \c VarFileType, but with a bit denoting that the type has a field length and a value denoting end-of-table.
 */
struct SavegameFileType {
	static constexpr uint8_t HAS_FIELD_LENGTH_BIT = SAVE_TABLE_FIELD_LENGTH_BIT; ///< Shared original table type/length bit allocation.
	uint8_t storage{}; ///< Actual storage of the file type.

	/** Create an end-of-table marker. */
	SavegameFileType() {}

	/**
	 * Create the type.
	 * @param file_type The file type.
	 * @param has_field_length Does this field have a length?
	 */
	SavegameFileType(VarFileType file_type, bool has_field_length = false) : storage(to_underlying(file_type))
	{
		/* 0 is not allowed as it's the end-of-table marker, larger is not allowed due to the field length bit. */
		assert(IsInsideMM(to_underlying(file_type), 1, 1 << HAS_FIELD_LENGTH_BIT));
		AssignBit(this->storage, HAS_FIELD_LENGTH_BIT, has_field_length);
	}

	/**
	 * Is this the end-of-table marker?
	 * @return \c true iff this is an end-of-table marker.
	 */
	constexpr bool IsEnd() const { return DecodeSaveTableField(this->storage).terminal; }

	/**
	 * Does this field have a length?
	 * @return \c true iff this field has a length.
	 */
	constexpr bool HasFieldLength() const
	{
		assert(!this->IsEnd());
		return DecodeSaveTableField(this->storage).has_length;
	}

	/**
	 * Get the \c VarType for this field.
	 * @return The \c VarType.
	 */
	constexpr VarFileType Type() const
	{
		assert(!this->IsEnd());
		return static_cast<VarFileType>(DecodeSaveTableField(this->storage).primitive);
	}
};

/**
 * Return the type as saved/loaded inside the savegame.
 * @param sld The save-load configuration for a single variable.
 * @return The type description part for the way the data is stored in the file.
 */
static SavegameFileType GetSavegameFileType(const SaveLoad &sld)
{
	switch (sld.cmd) {
		case SaveLoadType::Variable:
			return sld.conv.file;

		case SaveLoadType::String:
		case SaveLoadType::Array:
		case SaveLoadType::Vector:
			return { sld.conv.file, true };

		case SaveLoadType::Reference:
			return IsSavegameVersionBefore(SaveLoadVersion::MoreCargoPackets) ? VarFileType::U16 : VarFileType::U32;

		case SaveLoadType::ReferenceList:
		case SaveLoadType::ReferenceVector:
			return { IsSavegameVersionBefore(SaveLoadVersion::MoreCargoPackets) ? VarFileType::U16 : VarFileType::U32, true };

		case SaveLoadType::SaveByte:
			return VarFileType::U8;

		case SaveLoadType::Struct:
		case SaveLoadType::StructList:
			return { VarFileType::Struct, true };

		default: NOT_REACHED();
	}
}

/**
 * Return the size in bytes of a certain type of normal/atomic variable
 * as it appears in memory. See VarTypes
 * @param conv VarType type of variable that is used for calculating the size
 * @return Return the size of this type in bytes
 */
static inline uint SlCalcConvMemLen(VarMemType conv)
{
	switch (conv) {
		case VarMemType::Bool: return sizeof(bool);
		case VarMemType::I8: return sizeof(int8_t);
		case VarMemType::U8: return sizeof(uint8_t);
		case VarMemType::I16: return sizeof(int16_t);
		case VarMemType::U16: return sizeof(uint16_t);
		case VarMemType::I32: return sizeof(int32_t);
		case VarMemType::U32: return sizeof(uint32_t);
		case VarMemType::I64: return sizeof(int64_t);
		case VarMemType::U64: return sizeof(uint64_t);
		case VarMemType::Null: return 0;
		case VarMemType::Label: return sizeof(BaseLabel);

		case VarMemType::Str:
			return SlReadArrayLength();

		case VarMemType::Name:
		default:
			NOT_REACHED();
	}
}

/**
 * Return the size in bytes of a certain type of normal/atomic variable
 * as it appears in a saved game. See VarTypes
 * @param conv VarType type of variable that is used for calculating the size
 * @return Return the size of this type in bytes
 */
static inline uint8_t SlCalcConvFileLen(VarType conv)
{
	switch (conv.file) {
		case VarFileType::I8: return sizeof(int8_t);
		case VarFileType::U8: return sizeof(uint8_t);
		case VarFileType::I16: return sizeof(int16_t);
		case VarFileType::U16: return sizeof(uint16_t);
		case VarFileType::I32: return sizeof(int32_t);
		case VarFileType::U32: return sizeof(uint32_t);
		case VarFileType::I64: return sizeof(int64_t);
		case VarFileType::U64: return sizeof(uint64_t);
		case VarFileType::StringID: return sizeof(uint16_t);

		case VarFileType::String:
			return SlReadArrayLength();

		case VarFileType::Struct:
		default:
			NOT_REACHED();
	}
}

/**
 * Return the size in bytes of a reference (pointer).
 * @return Return the size of this type in bytes.
 */
static inline size_t SlCalcRefLen()
{
	return IsSavegameVersionBefore(SaveLoadVersion::MoreCargoPackets) ? 2 : 4;
}

void SlSetArrayIndex(uint index)
{
	_sl.need_length = NeedLength::WantLength;
	_sl.array_index = index;
}

static size_t _next_offs;

/**
 * Iterate through the elements of an array and read the whole thing
 * @return The index of the object, or -1 if we have reached the end of current block
 */
int SlIterateArray()
{
	// A prior row must be consumed in full before another native identity can
	// start; short/overread payloads would attach later vehicle/cargo fields to
	// the wrong serialized owner rather than merely skip display data.
	if (_next_offs != 0 && _sl.reader->GetSize() != _next_offs) {
		SlErrorCorruptFmt("Invalid chunk size iterating array - expected to be at position {}, actually at {}", _next_offs, _sl.reader->GetSize());
	}

	// Preserve each original ordinal, including empty contiguous entries; only
	// the explicit family terminator ends this owner's serialized identity set.
	for (;;) {
		const auto row = DecodeSaveArrayRow(SlReadArrayLength());
		if (row.terminal) {
			assert(!_sl.expect_table_header);
			_next_offs = 0;
			return -1;
		}

		const uint length = row.payload_bytes;
		_sl.obj_len = length;
		_next_offs = _sl.reader->GetSize() + length;

		if (_sl.expect_table_header) {
			_sl.expect_table_header = false;
			return INT32_MAX;
		}

		int index;
		switch (_sl.chunk_type) {
			case ChunkType::SparseTable:
			case ChunkType::SparseArray: index = static_cast<int>(SlReadSparseIndex()); break;
			case ChunkType::Table:
			case ChunkType::Array: index = _sl.array_index++; break;
			default:
				Debug(Facility::Sl, Severity::Critical, "SlIterateArray error");
				return -1; // error
		}

		if (length != 0) return index;
	}
}

/**
 * Skip an array or sparse array
 */
void SlSkipArray()
{
	while (SlIterateArray() != -1) {
		SlSkipBytes(_next_offs - _sl.reader->GetSize());
	}
}

/**
 * Sets the length of either a RIFF object or the number of items in an array.
 * This lets us load an object or an array of arbitrary size
 * @param length The length of the sought object/array
 */
void SlSetLength(size_t length)
{
	assert(_sl.action == SaveLoadAction::Save);

	switch (_sl.need_length) {
		case NeedLength::WantLength:
			_sl.need_length = NeedLength::None;
			if ((_sl.chunk_type == ChunkType::Table || _sl.chunk_type == ChunkType::SparseTable) && _sl.expect_table_header) {
				_sl.expect_table_header = false;
				SlWriteArrayLength(length + 1);
				break;
			}

			switch (_sl.chunk_type) {
				case ChunkType::Riff:
					/* Ugly encoding of >16M RIFF chunks
					 * The lower 24 bits are normal
					 * The uppermost 4 bits are bits 24:27 */
					assert(length < (1 << 28));
					SlWriteUint32((uint32_t)((length & 0xFFFFFF) | ((length >> 24) << 28)));
					break;
				case ChunkType::Table:
				case ChunkType::Array:
					assert(_sl.last_array_index <= _sl.array_index);
					while (++_sl.last_array_index <= _sl.array_index) {
						SlWriteArrayLength(1);
					}
					SlWriteArrayLength(length + 1);
					break;
				case ChunkType::SparseTable:
				case ChunkType::SparseArray:
					SlWriteArrayLength(length + 1 + SlGetArrayLength(_sl.array_index)); // Also include length of sparse index.
					SlWriteSparseIndex(_sl.array_index);
					break;
				default: NOT_REACHED();
			}
			break;

		case NeedLength::CalcLength:
			_sl.obj_len += static_cast<int>(length);
			break;

		default: NOT_REACHED();
	}
}

/**
 * Save/Load bytes. These do not need to be converted to Little/Big Endian
 * so directly write them or read them to/from file
 * @param ptr The source or destination of the object being manipulated
 * @param length number of bytes this fast CopyBytes lasts
 */
static void SlCopyBytes(void *ptr, size_t length)
{
	uint8_t *p = static_cast<uint8_t *>(ptr);

	switch (_sl.action) {
		case SaveLoadAction::LoadCheck:
		case SaveLoadAction::Load:
			for (; length != 0; length--) *p++ = SlReadByte();
			break;
		case SaveLoadAction::Save:
			for (; length != 0; length--) SlWriteByte(*p++);
			break;
		default: NOT_REACHED();
	}
}

/**
 * Get the length of the current object.
 * @return The length of the object in bytes.
 */
size_t SlGetFieldLength()
{
	return _sl.obj_len;
}

/** @copydoc SlGetChunkType */
ChunkType SlGetChunkType()
{
	return _sl.chunk_type;
}

/**
 * Return a signed-long version of the value of a setting
 * @param ptr pointer to the variable
 * @param conv type of variable, can be a non-clean
 * type, eg one with other flags because it is parsed
 * @return returns the value of the pointer-setting
 */
int64_t ReadValue(const void *ptr, VarMemType conv)
{
	switch (conv) {
		case VarMemType::Bool: return (*static_cast<const bool *>(ptr) != 0);
		case VarMemType::I8: return *static_cast<const int8_t *>(ptr);
		case VarMemType::U8: return *static_cast<const uint8_t *>(ptr);
		case VarMemType::I16: return *static_cast<const int16_t *>(ptr);
		case VarMemType::U16: return *static_cast<const uint16_t *>(ptr);
		case VarMemType::I32: return *static_cast<const int32_t *>(ptr);
		case VarMemType::U32: return *static_cast<const uint32_t *>(ptr);
		case VarMemType::I64: return *static_cast<const int64_t *>(ptr);
		case VarMemType::U64: return *static_cast<const uint64_t *>(ptr);
		case VarMemType::Null: return 0;
		default: NOT_REACHED();
	}
}

/**
 * Write the value of a setting
 * @param ptr pointer to the variable
 * @param conv type of variable, can be a non-clean type, eg
 *             with other flags. It is parsed upon read
 * @param val the new value being given to the variable
 */
void WriteValue(void *ptr, VarMemType conv, int64_t val)
{
	switch (conv) {
		case VarMemType::Bool: *static_cast<bool *>(ptr) = (val != 0); break;
		case VarMemType::I8: *static_cast<int8_t *>(ptr) = val; break;
		case VarMemType::U8: *static_cast<uint8_t *>(ptr) = val; break;
		case VarMemType::I16: *static_cast<int16_t *>(ptr) = val; break;
		case VarMemType::U16: *static_cast<uint16_t *>(ptr) = val; break;
		case VarMemType::I32: *static_cast<int32_t *>(ptr) = val; break;
		case VarMemType::U32: *static_cast<uint32_t *>(ptr) = val; break;
		case VarMemType::I64: *static_cast<int64_t *>(ptr) = val; break;
		case VarMemType::U64: *static_cast<uint64_t *>(ptr) = val; break;
		case VarMemType::Name: *reinterpret_cast<std::string *>(ptr) = CopyFromOldName(static_cast<StringID>(val)); break;
		case VarMemType::Null: break;
		default: NOT_REACHED();
	}
}

/**
 * Handle all conversion and typechecking of variables here.
 * In the case of saving, read in the actual value from the struct
 * and then write them to file, endian safely. Loading a value
 * goes exactly the opposite way
 * @param ptr The object being filled/read
 * @param conv VarType type of the current element of the struct
 */
static void SlSaveLoadConv(void *ptr, VarType conv)
{
	switch (_sl.action) {
		case SaveLoadAction::Save: {
			if (conv == VarTypes::LABEL) {
				/* Labels are written in reverse order as that is the way GrfIDs used to be written.
				 * Changing the order means changing external applications that extract this data. */
				BaseLabel *label = static_cast<BaseLabel *>(ptr);
				for (auto it = label->rbegin(); it != label->rend(); it++) SlWriteByte(*it);
				break;
			}

			int64_t x = ReadValue(ptr, conv.mem);

			/* Write the value to the file and check if its value is in the desired range */
			switch (conv.file) {
				case VarFileType::I8:
					assert(x >= -128 && x <= 127);
					SlWriteByte(x);
					break;

				case VarFileType::U8:
					assert(x >= 0 && x <= 255);
					SlWriteByte(x);
					break;

				case VarFileType::I16:
					assert(x >= -32768 && x <= 32767);
					SlWriteUint16(x);
					break;

				case VarFileType::StringID:
				case VarFileType::U16:
					assert(x >= 0 && x <= 65535);
					SlWriteUint16(x);
					break;

				case VarFileType::I32:
				case VarFileType::U32:
					SlWriteUint32(static_cast<uint32_t>(x));
					break;

				case VarFileType::I64:
				case VarFileType::U64:
					SlWriteUint64(x);
					break;

				default: NOT_REACHED();
			}
			break;
		}
		case SaveLoadAction::LoadCheck:
		case SaveLoadAction::Load: {
			if (conv == VarTypes::LABEL) {
				/* Labels are written in reverse order as that is the way GrfIDs used to be written.
				 * Changing the order means changing external applications that extract this data.
				 * The road/rail type labels were in forward order. They are fixed when needed in
				 * their respective loaders. */
				BaseLabel *label = static_cast<BaseLabel *>(ptr);
				for (auto it = label->rbegin(); it != label->rend(); it++) *it = SlReadByte();
				break;
			}

			int64_t x;
			/* Read a value from the file */
			switch (conv.file) {
				case VarFileType::I8: x = static_cast<int8_t>(SlReadByte()); break;
				case VarFileType::U8: x = static_cast<uint8_t>(SlReadByte()); break;
				case VarFileType::I16: x = static_cast<int16_t>(SlReadUint16()); break;
				case VarFileType::U16: x = static_cast<uint16_t>(SlReadUint16()); break;
				case VarFileType::I32: x = static_cast<int32_t>(SlReadUint32()); break;
				case VarFileType::U32: x = static_cast<uint32_t>(SlReadUint32()); break;
				case VarFileType::I64: x = static_cast<int64_t>(SlReadUint64()); break;
				case VarFileType::U64: x = static_cast<uint64_t>(SlReadUint64()); break;
				case VarFileType::StringID: x = RemapOldStringID(static_cast<StringID>(SlReadUint16())).base(); break;
				default: NOT_REACHED();
			}

			/* Write The value to the struct. These ARE endian safe. */
			WriteValue(ptr, conv.mem, x);
			break;
		}
		case SaveLoadAction::Ptrs: break;
		case SaveLoadAction::Null: break;
		default: NOT_REACHED();
	}
}

/**
 * Calculate the gross length of the string that it
 * will occupy in the savegame. This includes the real length, returned
 * by SlCalcNetStringLen and the length that the index will occupy.
 * @param ptr Pointer to the \c std::string.
 * @return The gross length of the string.
 */
static inline size_t SlCalcStdStringLen(const void *ptr)
{
	const std::string *str = reinterpret_cast<const std::string *>(ptr);

	size_t len = str->length();
	return len + SlGetArrayLength(len); // also include the length of the index
}


/**
 * Scan the string for old values of SCC_ENCODED and fix it to it's new, value.
 * Note that at the moment this runs, the string has not been validated yet
 * because the validation looks for SCC_ENCODED. If there is something invalid,
 * just bail out and do not continue trying to replace the tokens.
 * @param str the string to fix.
 * @param fix_code Whether to interpret/replace codes 0xE028 and 0xE02A as/with SCC_ENCODED.
 */
void FixSCCEncoded(std::string &str, bool fix_code)
{
	if (str.empty()) return;

	/* We need to convert from old escape-style encoding to record separator encoding.
	 * Initial `<SCC_ENCODED><STRINGID>` stays the same.
	 *
	 * `:<SCC_ENCODED><STRINGID>` becomes `<RS><SCC_ENCODED><STRINGID>`
	 * `:<HEX>`                   becomes `<RS><SCC_ENCODED_NUMERIC><HEX>`
	 * `:"<STRING>"`              becomes `<RS><SCC_ENCODED_STRING><STRING>`
	 */
	std::string result;
	StringBuilder builder(result);

	bool is_encoded = false; // Set if we determine by the presence of SCC_ENCODED that the string is an encoded string.
	bool in_string = false; // Set if we in a string, between double-quotes.
	bool need_type = true; // Set if a parameter type needs to be emitted.

	StringConsumer consumer(str);
	while (consumer.AnyBytesLeft()) {
		char32_t c;
		if (auto r = consumer.TryReadUtf8(); r.has_value()) {
			c = *r;
		} else {
			break;
		}
		if (c == SCC_ENCODED || (fix_code && (c == 0xE028 || c == 0xE02A))) {
			builder.PutUtf8(SCC_ENCODED);
			need_type = false;
			is_encoded = true;
			continue;
		}

		/* If the first character is not SCC_ENCODED then we don't have to do any conversion. */
		if (!is_encoded) return;

		if (c == '"') {
			in_string = !in_string;
			if (in_string && need_type) {
				/* Started a new string parameter. */
				builder.PutUtf8(SCC_ENCODED_STRING);
				need_type = false;
			}
			continue;
		}

		if (!in_string && c == ':') {
			builder.PutUtf8(SCC_RECORD_SEPARATOR);
			need_type = true;
			continue;
		}
		if (need_type) {
			/* Started a new numeric parameter. */
			builder.PutUtf8(SCC_ENCODED_NUMERIC);
			need_type = false;
		}

		builder.PutUtf8(c);
	}

	str = std::move(result);
}

/**
 * Scan the string for SCC_ENCODED_NUMERIC with negative values, and reencode them as uint64_t.
 * @param str the string to fix.
 */
void FixSCCEncodedNegative(std::string &str)
{
	if (str.empty()) return;

	StringConsumer consumer(str);

	/* Check whether this is an encoded string */
	if (!consumer.ReadUtf8If(SCC_ENCODED)) return;

	std::string result;
	StringBuilder builder(result);
	builder.PutUtf8(SCC_ENCODED);
	while (consumer.AnyBytesLeft()) {
		/* Copy until next record */
		builder.Put(consumer.ReadUntilUtf8(SCC_RECORD_SEPARATOR, StringConsumer::READ_ONE_SEPARATOR));

		/* Check whether this is a numeric parameter */
		if (!consumer.ReadUtf8If(SCC_ENCODED_NUMERIC)) continue;
		builder.PutUtf8(SCC_ENCODED_NUMERIC);

		/* First try unsigned */
		if (auto u = consumer.TryReadIntegerBase<uint64_t>(16); u.has_value()) {
			builder.PutIntegerBase<uint64_t>(*u, 16);
		} else {
			/* Read as signed, store as unsigned */
			auto s = consumer.ReadIntegerBase<int64_t>(16);
			builder.PutIntegerBase<uint64_t>(static_cast<uint64_t>(s), 16);
		}
	}

	str = std::move(result);
}

/**
 * Read the given amount of bytes from the buffer into the string.
 * @param str The string to write to.
 * @param length The amount of bytes to read into the string.
 * @note Does not perform any validation on validity of the string.
 */
void SlReadString(std::string &str, size_t length)
{
	str.resize(length);
	SlCopyBytes(str.data(), length);
}

/**
 * Save/Load a \c std::string.
 * @param ptr the string being manipulated
 * @param conv must be VarFileType::String
 */
static void SlStdString(void *ptr, VarType conv)
{
	std::string *str = reinterpret_cast<std::string *>(ptr);

	switch (_sl.action) {
		case SaveLoadAction::Save: {
			size_t len = str->length();
			SlWriteArrayLength(len);
			SlCopyBytes(const_cast<void *>(static_cast<const void *>(str->data())), len);
			break;
		}

		case SaveLoadAction::LoadCheck:
		case SaveLoadAction::Load: {
			size_t len = SlReadArrayLength();
			if (conv.mem == VarMemType::Null) {
				SlSkipBytes(len);
				return;
			}

			SlReadString(*str, len);

			StringValidationSettings settings = conv.string_validation_settings;
			settings.Set(StringValidationSetting::ReplaceWithQuestionMark);
			if (settings.Test(StringValidationSetting::AllowControlCode)) {
				if (IsSavegameVersionBefore(SaveLoadVersion::EncodedStringFormat)) FixSCCEncoded(*str, IsSavegameVersionBefore(SaveLoadVersion::MoveSccEncoded));
				if (IsSavegameVersionBefore(SaveLoadVersion::FixSccEncodedNegative)) FixSCCEncodedNegative(*str);
			}
			StrMakeValidInPlace(*str, settings);
		}

		case SaveLoadAction::Ptrs: break;
		case SaveLoadAction::Null: break;
		default: NOT_REACHED();
	}
}

/**
 * Internal function to save/Load a list of SaveLoadType::Variables.
 * SlCopy() and SlArray() are very similar, with the exception of the header.
 * This function represents the common part.
 * @param object The object being manipulated.
 * @param length The length of the object in elements
 * @param conv VarType type of the items.
 */
static void SlCopyInternal(void *object, size_t length, VarType conv)
{
	if (conv.mem == VarMemType::Null) {
		assert(_sl.action != SaveLoadAction::Save); // Use SaveLoadType::Null if you want to write null-bytes
		SlSkipBytes(length * SlCalcConvFileLen(conv));
		return;
	}

	/* NOTICE - handle some buggy stuff, in really old versions everything was saved
	 * as a byte-type. So detect this, and adjust object size accordingly */
	if (_sl.action != SaveLoadAction::Save && _sl_version == SaveLoadVersion::MinVersion) {
		/* all objects except difficulty settings */
		if (conv == VarTypes::I16 || conv == VarTypes::U16 || conv == VarTypes::STRINGID ||
				conv == VarTypes::I32 || conv == VarTypes::U32) {
			SlCopyBytes(object, length * SlCalcConvFileLen(conv));
			return;
		}
		/* used for conversion of Money 32bit->64bit */
		if (conv.file == VarFileType::I32 && conv.mem == VarMemType::I64) {
			for (uint i = 0; i < length; i++) {
				static_cast<int64_t *>(object)[i] = std::byteswap(SlReadUint32());
			}
			return;
		}
	}

	/* If the size of elements is 1 byte both in file and memory, no special
	 * conversion is needed, use specialized copy-copy function to speed up things */
	if (conv == VarTypes::I8 || conv == VarTypes::U8) {
		SlCopyBytes(object, length);
	} else {
		uint8_t *a = static_cast<uint8_t *>(object);
		uint8_t mem_size = SlCalcConvMemLen(conv.mem);

		for (; length != 0; length --) {
			SlSaveLoadConv(a, conv);
			a += mem_size; // get size
		}
	}
}

/**
 * Copy a list of SaveLoadType::Variables to/from a savegame.
 * These entries are copied as-is, and you as caller have to make sure things
 * like length-fields are calculated correctly.
 * @param object The object being manipulated.
 * @param length The length of the object in elements
 * @param conv VarType type of the items.
 */
void SlCopy(void *object, size_t length, VarType conv)
{
	assert(object != nullptr); // Use SlSkipBytes instead
	if (_sl.action == SaveLoadAction::Ptrs || _sl.action == SaveLoadAction::Null) return;

	/* Automatically calculate the length? */
	if (_sl.need_length != NeedLength::None) {
		SlSetLength(length * SlCalcConvFileLen(conv));
		/* Determine length only? */
		if (_sl.need_length == NeedLength::CalcLength) return;
	}

	SlCopyInternal(object, length, conv);
}

/**
 * Return the size in bytes of a certain type of atomic array
 * @param length The length of the array counted in elements
 * @param conv VarType type of the variable that is used in calculating the size
 * @return The size of this type in bytes.
 */
static inline size_t SlCalcArrayLen(size_t length, VarType conv)
{
	return SlCalcConvFileLen(conv) * length + SlGetArrayLength(length);
}

/**
 * Save/Load the length of the array followed by the array of SaveLoadType::Variable elements.
 * @param array The array being manipulated
 * @param length The length of the array in elements
 * @param conv VarType type of the atomic array (int, uint8_t, uint64_t, etc.)
 */
static void SlArray(void *array, size_t length, VarType conv)
{
	switch (_sl.action) {
		case SaveLoadAction::Save:
			SlWriteArrayLength(length);
			SlCopyInternal(array, length, conv);
			return;

		case SaveLoadAction::LoadCheck:
		case SaveLoadAction::Load: {
			if (!IsSavegameVersionBefore(SaveLoadVersion::SaveloadListLength)) {
				size_t sv_length = SlReadArrayLength();
				if (conv.mem == VarMemType::Null) {
					/* We don't know this field, so we assume the length in the savegame is correct. */
					length = sv_length;
				} else if (sv_length != length) {
					/* If the SLE_ARR changes size, a savegame bump is required
					 * and the developer should have written conversion lines.
					 * Error out to make this more visible. */
					SlErrorCorrupt("Fixed-length array is of wrong length");
				}
			}

			SlCopyInternal(array, length, conv);
			return;
		}

		case SaveLoadAction::Ptrs:
		case SaveLoadAction::Null:
			return;

		default:
			NOT_REACHED();
	}
}

/**
 * Pointers cannot be saved to a savegame, so this functions gets
 * the index of the item, and if not available, it hussles with
 * pointers (looks really bad :()
 * Remember that a nullptr item has value 0, and all
 * indices have +1, so vehicle 0 is saved as index 1.
 * @param obj The object that we want to get the index of
 * @param rt SLRefType type of the object the index is being sought of
 * @return Return the pointer converted to an index of the type pointed to
 */
static uint32_t ReferenceToInt(const void *obj, SLRefType rt)
{
	assert(_sl.action == SaveLoadAction::Save);

	if (obj == nullptr) return 0;

	switch (rt) {
		case SLRefType::OldVehicle: // Old vehicles we save as new ones
		case SLRefType::Vehicle: return static_cast<const Vehicle *>(obj)->index + 1;
		case SLRefType::Station: return static_cast<const Station *>(obj)->index + 1;
		case SLRefType::Town: return static_cast<const Town *>(obj)->index + 1;
		case SLRefType::RoadStop: return static_cast<const RoadStop *>(obj)->index + 1;
		case SLRefType::EngineRenew: return static_cast<const EngineRenew *>(obj)->index + 1;
		case SLRefType::CargoPacket: return static_cast<const CargoPacket *>(obj)->index + 1;
		case SLRefType::OrderList: return static_cast<const OrderList *>(obj)->index + 1;
		case SLRefType::Storage: return static_cast<const PersistentStorage *>(obj)->index + 1;
		case SLRefType::LinkGraph: return static_cast<const LinkGraph *>(obj)->index + 1;
		case SLRefType::LinkGraphJob: return static_cast<const LinkGraphJob *>(obj)->index + 1;
		default: NOT_REACHED();
	}
}

/**
 * Pointers cannot be loaded from a savegame, so this function
 * gets the index from the savegame and returns the appropriate
 * pointer from the already loaded base.
 * Remember that an index of 0 is a nullptr pointer so all indices
 * are +1 so vehicle 0 is saved as 1.
 * @param index The index that is being converted to a pointer
 * @param rt SLRefType type of the object the pointer is sought of
 * @return Return the index converted to a pointer of any type
 */
static void *IntToReference(size_t index, SLRefType rt)
{
	static_assert(sizeof(size_t) <= sizeof(void *));

	assert(_sl.action == SaveLoadAction::Ptrs);

	/* After version 4.3 SLRefType::OldVehicle is saved as SLRefType::Vehicle,
	 * and should be loaded like that */
	if (rt == SLRefType::OldVehicle && !IsSavegameVersionBefore(SaveLoadVersion::TownTolerancePauseMode, 4)) {
		rt = SLRefType::Vehicle;
	}

	/* No need to look up nullptr pointers, just return immediately */
	if (index == (rt == SLRefType::OldVehicle ? 0xFFFF : 0)) return nullptr;

	/* Correct index. Old vehicles were saved differently:
	 * invalid vehicle was 0xFFFF, now we use 0x0000 for everything invalid. */
	if (rt != SLRefType::OldVehicle) index--;

	switch (rt) {
		case SLRefType::OrderList:
			if (OrderList::IsValidID(index)) return OrderList::Get(index);
			SlErrorCorrupt("Referencing invalid OrderList");

		case SLRefType::OldVehicle:
		case SLRefType::Vehicle:
			if (Vehicle::IsValidID(index)) return Vehicle::Get(index);
			SlErrorCorrupt("Referencing invalid Vehicle");

		case SLRefType::Station:
			if (Station::IsValidID(index)) return Station::Get(index);
			SlErrorCorrupt("Referencing invalid Station");

		case SLRefType::Town:
			if (Town::IsValidID(index)) return Town::Get(index);
			SlErrorCorrupt("Referencing invalid Town");

		case SLRefType::RoadStop:
			if (RoadStop::IsValidID(index)) return RoadStop::Get(index);
			SlErrorCorrupt("Referencing invalid RoadStop");

		case SLRefType::EngineRenew:
			if (EngineRenew::IsValidID(index)) return EngineRenew::Get(index);
			SlErrorCorrupt("Referencing invalid EngineRenew");

		case SLRefType::CargoPacket:
			if (CargoPacket::IsValidID(index)) return CargoPacket::Get(index);
			SlErrorCorrupt("Referencing invalid CargoPacket");

		case SLRefType::Storage:
			if (PersistentStorage::IsValidID(index)) return PersistentStorage::Get(index);
			SlErrorCorrupt("Referencing invalid PersistentStorage");

		case SLRefType::LinkGraph:
			if (LinkGraph::IsValidID(index)) return LinkGraph::Get(index);
			SlErrorCorrupt("Referencing invalid LinkGraph");

		case SLRefType::LinkGraphJob:
			if (LinkGraphJob::IsValidID(index)) return LinkGraphJob::Get(index);
			SlErrorCorrupt("Referencing invalid LinkGraphJob");

		default: NOT_REACHED();
	}
}

/**
 * Handle conversion for references.
 * @param ptr The object being filled/read.
 * @param conv VarType type of the current element of the struct.
 */
void SlSaveLoadRef(void *ptr, VarType conv)
{
	switch (_sl.action) {
		case SaveLoadAction::Save:
			SlWriteUint32(ReferenceToInt(*static_cast<void **>(ptr), conv.ref));
			break;
		case SaveLoadAction::LoadCheck:
		case SaveLoadAction::Load:
			*static_cast<size_t *>(ptr) = IsSavegameVersionBefore(SaveLoadVersion::MoreCargoPackets) ? SlReadUint16() : SlReadUint32();
			break;
		case SaveLoadAction::Ptrs:
			*static_cast<void **>(ptr) = IntToReference(*static_cast<size_t *>(ptr), conv.ref);
			break;
		case SaveLoadAction::Null:
			*static_cast<void **>(ptr) = nullptr;
			break;
		default: NOT_REACHED();
	}
}

/**
 * Template class to help with list-like types.
 */
template <template <typename, typename> typename Tstorage, typename Tvar, typename Tallocator = std::allocator<Tvar>>
class SlStorageHelper {
	typedef Tstorage<Tvar, Tallocator> SlStorageT;
public:
	/**
	 * Internal templated helper to return the size in bytes of a list-like type.
	 * @param storage The storage to find the size of
	 * @param conv VarType type of variable that is used for calculating the size
	 * @param cmd The SaveLoadType ware are saving/loading.
	 * @return The size of this type in bytes.
	 */
	static size_t SlCalcLen(const void *storage, VarType conv, SaveLoadType cmd = SaveLoadType::Variable)
	{
		assert(cmd == SaveLoadType::Variable || cmd == SaveLoadType::Reference);

		const SlStorageT *list = static_cast<const SlStorageT *>(storage);

		int type_size = SlGetArrayLength(list->size());
		int item_size = SlCalcConvFileLen(cmd == SaveLoadType::Variable ? conv : VarType{VarFileType::U32, {}});
		return list->size() * item_size + type_size;
	}

	static void SlSaveLoadMember(SaveLoadType cmd, Tvar *item, VarType conv)
	{
		switch (cmd) {
			case SaveLoadType::Variable: SlSaveLoadConv(item, conv); break;
			case SaveLoadType::Reference: SlSaveLoadRef(item, conv); break;
			case SaveLoadType::String: SlStdString(item, conv); break;
			default:
				NOT_REACHED();
		}
	}

	/**
	 * Internal templated helper to save/load a list-like type.
	 * @param storage The storage being manipulated.
	 * @param conv VarType type of variable that is used for calculating the size.
	 * @param cmd The SaveLoadType ware are saving/loading.
	 */
	static void SlSaveLoad(void *storage, VarType conv, SaveLoadType cmd = SaveLoadType::Variable)
	{
		assert(cmd == SaveLoadType::Variable || cmd == SaveLoadType::Reference || cmd == SaveLoadType::String);

		SlStorageT *list = static_cast<SlStorageT *>(storage);

		switch (_sl.action) {
			case SaveLoadAction::Save:
				SlWriteArrayLength(list->size());

				for (auto &item : *list) {
					SlSaveLoadMember(cmd, &item, conv);
				}
				break;

			case SaveLoadAction::LoadCheck:
			case SaveLoadAction::Load: {
				size_t length;
				switch (cmd) {
					case SaveLoadType::Variable: length = IsSavegameVersionBefore(SaveLoadVersion::SaveloadListLength) ? SlReadUint32() : SlReadArrayLength(); break;
					case SaveLoadType::Reference: length = IsSavegameVersionBefore(SaveLoadVersion::MoreCargoPackets) ? SlReadUint16() : IsSavegameVersionBefore(SaveLoadVersion::SaveloadListLength) ? SlReadUint32() : SlReadArrayLength(); break;
					case SaveLoadType::String: length = SlReadArrayLength(); break;
					default: NOT_REACHED();
				}

				list->clear();
				if constexpr (std::is_same_v<SlStorageT, std::vector<Tvar, Tallocator>>) {
					list->reserve(length);
				}

				/* Load each value and push to the end of the storage. */
				for (size_t i = 0; i < length; i++) {
					Tvar &data = list->emplace_back();
					SlSaveLoadMember(cmd, &data, conv);
				}
				break;
			}

			case SaveLoadAction::Ptrs:
				for (auto &item : *list) {
					SlSaveLoadMember(cmd, &item, conv);
				}
				break;

			case SaveLoadAction::Null:
				list->clear();
				break;

			default: NOT_REACHED();
		}
	}
};

/**
 * Return the size in bytes of a list.
 * @param list The std::list to find the size of.
 * @param conv VarType type of variable that is used for calculating the size.
 * @return The size of this type in bytes.
 */
static inline size_t SlCalcRefListLen(const void *list, VarType conv)
{
	return SlStorageHelper<std::list, void *>::SlCalcLen(list, conv, SaveLoadType::Reference);
}

/**
 * Save/Load a list.
 * @param list The list being manipulated.
 * @param conv VarType type of variable that is used for calculating the size.
 */
static void SlRefList(void *list, VarType conv)
{
	/* Automatically calculate the length? */
	if (_sl.need_length != NeedLength::None) {
		SlSetLength(SlCalcRefListLen(list, conv));
		/* Determine length only? */
		if (_sl.need_length == NeedLength::CalcLength) return;
	}

	SlStorageHelper<std::list, void *>::SlSaveLoad(list, conv, SaveLoadType::Reference);
}

/**
 * Return the size in bytes of a vector.
 * @param vector The std::vector to find the size of.
 * @param conv VarType type of variable that is used for calculating the size.
 * @return The size of this type in bytes.
 */
static size_t SlCalcRefVectorLen(const void *vector, VarType conv)
{
	return SlStorageHelper<std::vector, void *>::SlCalcLen(vector, conv, SaveLoadType::Reference);
}

/**
 * Save/Load a vector.
 * @param vector The vector being manipulated.
 * @param conv VarType type of variable that is used for calculating the size.
 */
static void SlRefVector(void *vector, VarType conv)
{
	/* Automatically calculate the length? */
	if (_sl.need_length != NeedLength::None) {
		SlSetLength(SlCalcRefVectorLen(vector, conv));
		/* Determine length only? */
		if (_sl.need_length == NeedLength::CalcLength) return;
	}

	SlStorageHelper<std::vector, void *>::SlSaveLoad(vector, conv, SaveLoadType::Reference);
}

/**
 * Return the size in bytes of a std::vector.
 * @param vector The std::vector to find the size of
 * @param conv VarType type of variable that is used for calculating the size
 * @return The size of this type in bytes.
 */
static inline size_t SlCalcVectorLen(const void *vector, VarType conv)
{
	switch (conv.mem) {
		case VarMemType::Bool: NOT_REACHED(); // Not supported
		case VarMemType::I8: return SlStorageHelper<std::vector, int8_t>::SlCalcLen(vector, conv);
		case VarMemType::U8: return SlStorageHelper<std::vector, uint8_t>::SlCalcLen(vector, conv);
		case VarMemType::I16: return SlStorageHelper<std::vector, int16_t>::SlCalcLen(vector, conv);
		case VarMemType::U16: return SlStorageHelper<std::vector, uint16_t>::SlCalcLen(vector, conv);
		case VarMemType::I32: return SlStorageHelper<std::vector, int32_t>::SlCalcLen(vector, conv);
		case VarMemType::U32: return SlStorageHelper<std::vector, uint32_t>::SlCalcLen(vector, conv);
		case VarMemType::I64: return SlStorageHelper<std::vector, int64_t>::SlCalcLen(vector, conv);
		case VarMemType::U64: return SlStorageHelper<std::vector, uint64_t>::SlCalcLen(vector, conv);

		case VarMemType::Str:
			/* Strings are a length-prefixed field type in the savegame table format,
			 * these may not be directly stored in another length-prefixed container type. */
			NOT_REACHED();

		default: NOT_REACHED();
	}
}

/**
 * Save/load a std::vector.
 * @param vector The std::vector being manipulated
 * @param conv VarType type of variable that is used for calculating the size
 */
static void SlVector(void *vector, VarType conv)
{
	switch (conv.mem) {
		case VarMemType::Bool: NOT_REACHED(); // Not supported
		case VarMemType::I8: SlStorageHelper<std::vector, int8_t>::SlSaveLoad(vector, conv); break;
		case VarMemType::U8: SlStorageHelper<std::vector, uint8_t>::SlSaveLoad(vector, conv); break;
		case VarMemType::I16: SlStorageHelper<std::vector, int16_t>::SlSaveLoad(vector, conv); break;
		case VarMemType::U16: SlStorageHelper<std::vector, uint16_t>::SlSaveLoad(vector, conv); break;
		case VarMemType::I32: SlStorageHelper<std::vector, int32_t>::SlSaveLoad(vector, conv); break;
		case VarMemType::U32: SlStorageHelper<std::vector, uint32_t>::SlSaveLoad(vector, conv); break;
		case VarMemType::I64: SlStorageHelper<std::vector, int64_t>::SlSaveLoad(vector, conv); break;
		case VarMemType::U64: SlStorageHelper<std::vector, uint64_t>::SlSaveLoad(vector, conv); break;

		case VarMemType::Str:
			/* Strings are a length-prefixed field type in the savegame table format,
			 * these may not be directly stored in another length-prefixed container type.
			 * This is permitted for load-related actions, because invalid fields of this type are present
			 * from SaveLoadVersion::CompanyAllowList up to SaveLoadVersion::CompanyAllowListV2. */
			assert(_sl.action != SaveLoadAction::Save);
			SlStorageHelper<std::vector, std::string>::SlSaveLoad(vector, conv, SaveLoadType::String);
			break;

		default: NOT_REACHED();
	}
}

/** @copydoc SlIsObjectValidInSavegame */
bool SlIsObjectValidInSavegame(const SaveLoad &sld)
{
	// Branch-only columns require their own explicit capability, including empty
	// TABLE headers. A colliding numeric interval cannot expose a default value.
	if (sld.required_capability && !SlHasSaveCapability(*sld.required_capability)) return false;
	// Both endpoints use the same original-feature semantics as scalar decoders;
	// fixing only the handler's if-statements would leave its TABLE layout wrong.
	return !IsSavegameVersionBefore(sld.version_from) && IsSavegameVersionBefore(sld.version_to);
}

/**
 * Calculate the size of the table header.
 * @param slt The SaveLoad table with objects to save/load.
 * @return size of given object.
 */
static size_t SlCalcTableHeader(const SaveLoadTable &slt)
{
	size_t length = 0;

	for (auto &sld : slt) {
		if (!SlIsObjectValidInSavegame(sld)) continue;

		length += SlCalcConvFileLen(VarTypes::U8);
		length += SlCalcStdStringLen(&sld.name);
	}

	length += SlCalcConvFileLen(VarTypes::U8); // End-of-list entry.

	for (auto &sld : slt) {
		if (!SlIsObjectValidInSavegame(sld)) continue;
		if (sld.cmd == SaveLoadType::StructList || sld.cmd == SaveLoadType::Struct) {
			length += SlCalcTableHeader(sld.handler->GetDescription());
		}
	}

	return length;
}

/**
 * Calculate the size of an object.
 * @param object to be measured.
 * @param slt The SaveLoad table with objects to save/load.
 * @return size of given object.
 */
size_t SlCalcObjLength(const void *object, const SaveLoadTable &slt)
{
	size_t length = 0;

	/* Need to determine the length and write a length tag. */
	for (auto &sld : slt) {
		length += SlCalcObjMemberLength(object, sld);
	}
	return length;
}

size_t SlCalcObjMemberLength(const void *object, const SaveLoad &sld)
{
	assert(_sl.action == SaveLoadAction::Save);

	if (!SlIsObjectValidInSavegame(sld)) return 0;

	switch (sld.cmd) {
		case SaveLoadType::Variable: return SlCalcConvFileLen(sld.conv);
		case SaveLoadType::Reference: return SlCalcRefLen();
		case SaveLoadType::Array: return SlCalcArrayLen(sld.length, sld.conv);
		case SaveLoadType::ReferenceList: return SlCalcRefListLen(GetVariableAddress(object, sld), sld.conv);
		case SaveLoadType::ReferenceVector: return SlCalcRefVectorLen(GetVariableAddress(object, sld), sld.conv);
		case SaveLoadType::Vector: return SlCalcVectorLen(GetVariableAddress(object, sld), sld.conv);
		case SaveLoadType::String: return SlCalcStdStringLen(GetVariableAddress(object, sld));
		case SaveLoadType::SaveByte: return 1; // a byte is logically of size 1
		case SaveLoadType::Null: return SlCalcConvFileLen(sld.conv) * sld.length;

		case SaveLoadType::Struct:
		case SaveLoadType::StructList: {
			NeedLength old_need_length = _sl.need_length;
			size_t old_obj_len = _sl.obj_len;

			_sl.need_length = NeedLength::CalcLength;
			_sl.obj_len = 0;

			/* Pretend that we are saving to collect the object size. Other
			 * means are difficult, as we don't know the length of the list we
			 * are about to store. */
			sld.handler->Save(const_cast<void *>(object));
			size_t length = _sl.obj_len;

			_sl.obj_len = old_obj_len;
			_sl.need_length = old_need_length;

			if (sld.cmd == SaveLoadType::Struct) {
				length += SlGetArrayLength(1);
			}

			return length;
		}

		default: NOT_REACHED();
	}
	return 0;
}

static bool SlObjectMember(void *object, const SaveLoad &sld)
{
	if (!SlIsObjectValidInSavegame(sld)) return false;

	switch (sld.cmd) {
		case SaveLoadType::Variable:
		case SaveLoadType::Reference:
		case SaveLoadType::Array:
		case SaveLoadType::ReferenceList:
		case SaveLoadType::ReferenceVector:
		case SaveLoadType::Vector:
		case SaveLoadType::String: {
			void *ptr = GetVariableAddress(object, sld);

			switch (sld.cmd) {
				case SaveLoadType::Variable: SlSaveLoadConv(ptr, sld.conv); break;
				case SaveLoadType::Reference: SlSaveLoadRef(ptr, sld.conv); break;
				case SaveLoadType::Array: SlArray(ptr, sld.length, sld.conv); break;
				case SaveLoadType::ReferenceList: SlRefList(ptr, sld.conv); break;
				case SaveLoadType::ReferenceVector: SlRefVector(ptr, sld.conv); break;
				case SaveLoadType::Vector: SlVector(ptr, sld.conv); break;
				case SaveLoadType::String: SlStdString(ptr, sld.conv); break;
				default: NOT_REACHED();
			}
			break;
		}

		/* SaveLoadType::SaveByte writes a value to the savegame to identify the type of an object.
		 * When loading, the value is read explicitly with SlReadByte() to determine which
		 * object description to use. */
		case SaveLoadType::SaveByte: {
			void *ptr = GetVariableAddress(object, sld);

			switch (_sl.action) {
				case SaveLoadAction::Save: SlWriteByte(*static_cast<uint8_t *>(ptr)); break;
				case SaveLoadAction::LoadCheck:
				case SaveLoadAction::Load:
				case SaveLoadAction::Ptrs:
				case SaveLoadAction::Null: break;
				default: NOT_REACHED();
			}
			break;
		}

		case SaveLoadType::Null: {
			assert(sld.conv.mem == VarMemType::Null);

			switch (_sl.action) {
				case SaveLoadAction::LoadCheck:
				case SaveLoadAction::Load: SlSkipBytes(SlCalcConvFileLen(sld.conv) * sld.length); break;
				case SaveLoadAction::Save: for (int i = 0; i < SlCalcConvFileLen(sld.conv) * sld.length; i++) SlWriteByte(0); break;
				case SaveLoadAction::Ptrs:
				case SaveLoadAction::Null: break;
				default: NOT_REACHED();
			}
			break;
		}

		case SaveLoadType::Struct:
		case SaveLoadType::StructList:
			switch (_sl.action) {
				case SaveLoadAction::Save: {
					if (sld.cmd == SaveLoadType::Struct) {
						/* Store in the savegame if this struct was written or not. */
						SlSetStructListLength(SlCalcObjMemberLength(object, sld) > SlGetArrayLength(1) ? 1 : 0);
					}
					sld.handler->Save(object);
					break;
				}

				case SaveLoadAction::LoadCheck: {
					if (sld.cmd == SaveLoadType::Struct && !IsSavegameVersionBefore(SaveLoadVersion::SaveloadListLength)) {
						SlGetStructListLength(1);
					}
					sld.handler->LoadCheck(object);
					break;
				}

				case SaveLoadAction::Load: {
					if (sld.cmd == SaveLoadType::Struct && !IsSavegameVersionBefore(SaveLoadVersion::SaveloadListLength)) {
						SlGetStructListLength(1);
					}
					sld.handler->Load(object);
					break;
				}

				case SaveLoadAction::Ptrs:
					sld.handler->FixPointers(object);
					break;

				case SaveLoadAction::Null: break;
				default: NOT_REACHED();
			}
			break;

		default: NOT_REACHED();
	}
	return true;
}

/**
 * Set the length of this list.
 * @param length The length of the list.
 */
void SlSetStructListLength(size_t length)
{
	/* Automatically calculate the length? */
	if (_sl.need_length != NeedLength::None) {
		SlSetLength(SlGetArrayLength(length));
		if (_sl.need_length == NeedLength::CalcLength) return;
	}

	SlWriteArrayLength(length);
}

/**
 * Get the length of this list; if it exceeds the limit, error out.
 * @param limit The maximum size the list can be.
 * @return The length of the list.
 */
size_t SlGetStructListLength(size_t limit)
{
	size_t length = SlReadArrayLength();
	if (length > limit) SlErrorCorrupt("List exceeds storage size");

	return length;
}

/**
 * Main SaveLoad function.
 * @param object The object that is being saved or loaded.
 * @param slt The SaveLoad table with objects to save/load.
 */
void SlObject(void *object, const SaveLoadTable &slt)
{
	/* Automatically calculate the length? */
	if (_sl.need_length != NeedLength::None) {
		SlSetLength(SlCalcObjLength(object, slt));
		if (_sl.need_length == NeedLength::CalcLength) return;
	}

	for (auto &sld : slt) {
		SlObjectMember(object, sld);
	}
}

/**
 * Handler that is assigned when there is a struct read in the savegame which
 * is not known to the code. This means we are going to skip it.
 */
class SlSkipHandler : public SaveLoadHandler {
	void Save(void *) const override
	{
		NOT_REACHED();
	}

	void Load(void *object) const override
	{
		size_t length = SlGetStructListLength(UINT32_MAX);
		for (; length > 0; length--) {
			SlObject(object, this->GetLoadDescription());
		}
	}

	void LoadCheck(void *object) const override
	{
		this->Load(object);
	}

	SaveLoadTable GetDescription() const override
	{
		return {};
	}

	SaveLoadCompatTable GetCompatDescription() const override
	{
		NOT_REACHED();
	}
};

/**
 * Save or Load a table header.
 * @note a table-header can never contain more than 65535 fields.
 * @param slt The SaveLoad table with objects to save/load.
 * @return When loading, the ordered SaveLoad array to use; otherwise an empty list.
 */
std::vector<SaveLoad> SlTableHeader(const SaveLoadTable &slt)
{
	/* You can only use SlTableHeader if you are a ChunkType::Table or ChunkType::SparseTable. */
	assert(_sl.chunk_type == ChunkType::Table || _sl.chunk_type == ChunkType::SparseTable);

	switch (_sl.action) {
		case SaveLoadAction::LoadCheck:
		case SaveLoadAction::Load: {
			std::vector<SaveLoad> saveloads;

			/* Build a key lookup mapping based on the available fields. */
			std::map<std::string, const SaveLoad *> key_lookup;
			for (auto &sld : slt) {
				if (!SlIsObjectValidInSavegame(sld)) continue;

				/* Check that there is only one active SaveLoad for a given name. */
				assert(key_lookup.find(sld.name) == key_lookup.end());
				key_lookup[sld.name] = &sld;
			}

			while (true) {
				SavegameFileType type{};
				SlSaveLoadConv(&type.storage, VarTypes::U8);
				if (type.IsEnd()) break;

				std::string key;
				SlStdString(&key, VarTypes::STR);

				auto sld_it = key_lookup.find(key);
				if (sld_it == key_lookup.end()) {
					/* SLA_LOADCHECK triggers this debug statement a lot and is perfectly normal. */
					Debug(Facility::Sl, _sl.action == SaveLoadAction::Load ? Severity::Warning : Severity::Debug2, "Field '{}' of type 0x{:02x} not found, skipping", key, type.storage);

					std::shared_ptr<SaveLoadHandler> handler = nullptr;
					SaveLoadType saveload_type;
					switch (type.Type()) {
						case VarFileType::String:
							saveload_type = SaveLoadType::String;
							break;

						case VarFileType::Struct:
							saveload_type = SaveLoadType::StructList;
							handler = std::make_shared<SlSkipHandler>();
							break;

						default:
							saveload_type = type.HasFieldLength() ? SaveLoadType::Array : SaveLoadType::Variable;
							break;
					}

					/* We don't know this field, so read to nothing. */
					saveloads.emplace_back(std::move(key), saveload_type, VarType{type.Type(), VarMemType::Null}, 1, SaveLoadVersion::MinVersion, SaveLoadVersion::MaxVersion, nullptr, 0, std::move(handler));
					continue;
				}

				/* Validate the type of the field. If it is changed, the
				 * savegame should have been bumped so we know how to do the
				 * conversion. If this error triggers, that clearly didn't
				 * happen and this is a friendly poke to the developer to bump
				 * the savegame version and add conversion code. */
				SavegameFileType correct_type = GetSavegameFileType(*sld_it->second);
				if (correct_type.storage != type.storage) {
					Debug(Facility::Sl, Severity::Error, "Field type for '{}' was expected to be 0x{:02x} but 0x{:02x} was found", key, correct_type.storage, type.storage);
					SlErrorCorrupt("Field type is different than expected");
				}
				saveloads.emplace_back(*sld_it->second);
			}

			for (auto &sld : saveloads) {
				if (sld.cmd == SaveLoadType::StructList || sld.cmd == SaveLoadType::Struct) {
					sld.handler->load_description = SlTableHeader(sld.handler->GetDescription());
				}
			}

			return saveloads;
		}

		case SaveLoadAction::Save: {
			/* Automatically calculate the length? */
			if (_sl.need_length != NeedLength::None) {
				SlSetLength(SlCalcTableHeader(slt));
				if (_sl.need_length == NeedLength::CalcLength) break;
			}

			for (auto &sld : slt) {
				if (!SlIsObjectValidInSavegame(sld)) continue;
				/* Make sure we are not storing empty keys. */
				assert(!sld.name.empty());

				SavegameFileType type = GetSavegameFileType(sld);
				assert(!type.IsEnd());

				SlSaveLoadConv(&type.storage, VarTypes::U8);
				SlStdString(const_cast<std::string *>(&sld.name), VarTypes::STR);
			}

			/* Add an end-of-header marker. */
			SavegameFileType type{};
			SlSaveLoadConv(&type.storage, VarTypes::U8);

			/* After the table, write down any sub-tables we might have. */
			for (auto &sld : slt) {
				if (!SlIsObjectValidInSavegame(sld)) continue;
				if (sld.cmd == SaveLoadType::StructList || sld.cmd == SaveLoadType::Struct) {
					/* SlCalcTableHeader already looks in sub-lists, so avoid the length being added twice. */
					NeedLength old_need_length = _sl.need_length;
					_sl.need_length = NeedLength::None;

					SlTableHeader(sld.handler->GetDescription());

					_sl.need_length = old_need_length;
				}
			}

			break;
		}

		default: NOT_REACHED();
	}

	return std::vector<SaveLoad>();
}

/**
 * Load a table header in a savegame compatible way. If the savegame was made
 * before table headers were added, it will fall back to the
 * SaveLoadCompatTable for the order of fields while loading.
 *
 * @note You only have to call this function if the chunk existed as a
 * non-table type before converting it to a table. New chunks created as
 * table can call SlTableHeader() directly.
 *
 * @param slt The SaveLoad table with objects to save/load.
 * @param slct The SaveLoadCompat table the original order of the fields.
 * @return When loading, the ordered SaveLoad array to use; otherwise an empty list.
 */
std::vector<SaveLoad> SlCompatTableHeader(const SaveLoadTable &slt, const SaveLoadCompatTable &slct)
{
	assert(_sl.action == SaveLoadAction::Load || _sl.action == SaveLoadAction::LoadCheck);
	/* ChunkType::Table / ChunkType::SparseTable always have a header. */
	if (_sl.chunk_type == ChunkType::Table || _sl.chunk_type == ChunkType::SparseTable) return SlTableHeader(slt);

	std::vector<SaveLoad> saveloads;

	/* Build a key lookup mapping based on the available fields. */
	std::map<std::string_view, std::vector<const SaveLoad *>> key_lookup;
	for (auto &sld : slt) {
		/* All entries should have a name; otherwise the entry should just be removed. */
		assert(!sld.name.empty());

		key_lookup[sld.name].push_back(&sld);
	}

	for (auto &slc : slct) {
		if (slc.name.empty()) {
			/* In old savegames there can be data we no longer care for. We
			 * skip this by simply reading the amount of bytes indicated and
			 * send those to /dev/null. */
			saveloads.emplace_back("", SaveLoadType::Null, VarType{VarFileType::U8, VarMemType::Null}, slc.null_length, slc.version_from, slc.version_to, nullptr, 0, nullptr);
		} else {
			auto sld_it = key_lookup.find(slc.name);
			/* If this branch triggers, it means that an entry in the
			 * SaveLoadCompat list is not mentioned in the SaveLoad list. Did
			 * you rename a field in one and not in the other? */
			if (sld_it == key_lookup.end()) {
				/* This isn't an assert, as that leaves no information what
				 * field was to blame. This way at least we have breadcrumbs. */
				Debug(Facility::Sl, Severity::Critical, "internal error: saveload compatibility field '{}' not found", slc.name);
				SlErrorCorrupt("Internal error with savegame compatibility");
			}
			for (auto &sld : sld_it->second) {
				saveloads.push_back(*sld);
			}
		}
	}

	for (auto &sld : saveloads) {
		if (!SlIsObjectValidInSavegame(sld)) continue;
		if (sld.cmd == SaveLoadType::StructList || sld.cmd == SaveLoadType::Struct) {
			sld.handler->load_description = SlCompatTableHeader(sld.handler->GetDescription(), sld.handler->GetCompatDescription());
		}
	}

	return saveloads;
}

/**
 * Save or Load (a list of) global variables.
 * @param slt The SaveLoad table with objects to save/load.
 */
void SlGlobList(const SaveLoadTable &slt)
{
	SlObject(nullptr, slt);
}

/**
 * Do something of which I have no idea what it is :P
 * @param proc The callback procedure that is called
 * @param arg The variable that will be used for the callback procedure
 */
void SlAutolength(AutolengthProc *proc, int arg)
{
	assert(_sl.action == SaveLoadAction::Save);

	/* Tell it to calculate the length */
	_sl.need_length = NeedLength::CalcLength;
	_sl.obj_len = 0;
	proc(arg);

	/* Setup length */
	_sl.need_length = NeedLength::WantLength;
	SlSetLength(_sl.obj_len);

	size_t start_pos = _sl.dumper->GetSize();
	size_t expected_offs = start_pos + _sl.obj_len;

	/* And write the stuff */
	proc(arg);

	if (expected_offs != _sl.dumper->GetSize()) {
		SlErrorCorruptFmt("Invalid chunk size when writing autolength block, expected {}, got {}", _sl.obj_len, _sl.dumper->GetSize() - start_pos);
	}
}

void ChunkHandler::LoadCheck(size_t len) const
{
	switch (_sl.chunk_type) {
		case ChunkType::Table:
		case ChunkType::SparseTable:
			SlTableHeader({});
			[[fallthrough]];
		case ChunkType::Array:
		case ChunkType::SparseArray:
			SlSkipArray();
			break;
		case ChunkType::Riff:
			SlSkipBytes(len);
			break;
		default:
			NOT_REACHED();
	}
}

/**
 * Load a chunk of data (eg vehicles, stations, etc.)
 * @param ch The chunkhandler that will be used for the operation
 */
static void SlLoadChunk(const ChunkHandler &ch)
{
	uint8_t m = SlReadByte();

	_sl.chunk_type = static_cast<ChunkType>(m & to_underlying(ChunkType::FileTypeMask));
	_sl.obj_len = 0;
	_sl.expect_table_header = SaveChunkHasTableHeader(m);

	/* The header should always be at the start. Read the length; the
	 * Load() should as first action process the header. */
	if (_sl.expect_table_header) {
		if (SlIterateArray() != INT32_MAX) SlErrorCorrupt("Table chunk without header");
	}

	switch (_sl.chunk_type) {
		case ChunkType::Table:
		case ChunkType::Array:
			_sl.array_index = 0;
			ch.Load();
			if (_next_offs != 0) SlErrorCorrupt("Invalid array length");
			break;
		case ChunkType::SparseTable:
		case ChunkType::SparseArray:
			ch.Load();
			if (_next_offs != 0) SlErrorCorrupt("Invalid array length");
			break;
		case ChunkType::Riff: {
			/* Read length */
			const size_t len = ReadSaveRiffLength(m, SlReadByte);
			_sl.obj_len = len;
			size_t start_pos = _sl.reader->GetSize();
			size_t endoffs = start_pos + len;
			ch.Load();

			if (_sl.reader->GetSize() != endoffs) {
				SlErrorCorruptFmt("Invalid chunk size in RIFF in {} - expected {}, got {}", ch.GetName(), len, _sl.reader->GetSize() - start_pos);
			}
			break;
		}
		default:
			SlErrorCorrupt("Invalid chunk type");
			break;
	}

	if (_sl.expect_table_header) SlErrorCorrupt("Table chunk without header");
}

/**
 * Load a chunk of data for checking savegames.
 * If the chunkhandler is nullptr, the chunk is skipped.
 * @param ch The chunkhandler that will be used for the operation
 */
static void SlLoadCheckChunk(const ChunkHandler &ch)
{
	uint8_t m = SlReadByte();

	_sl.chunk_type = static_cast<ChunkType>(m & to_underlying(ChunkType::FileTypeMask));
	_sl.obj_len = 0;
	_sl.expect_table_header = SaveChunkHasTableHeader(m);

	/* The header should always be at the start. Read the length; the
	 * LoadCheck() should as first action process the header. */
	if (_sl.expect_table_header) {
		if (SlIterateArray() != INT32_MAX) SlErrorCorrupt("Table chunk without header");
	}

	switch (_sl.chunk_type) {
		case ChunkType::Table:
		case ChunkType::Array:
			_sl.array_index = 0;
			ch.LoadCheck();
			break;
		case ChunkType::SparseTable:
		case ChunkType::SparseArray:
			ch.LoadCheck();
			break;
		case ChunkType::Riff: {
			/* Read length */
			const size_t len = ReadSaveRiffLength(m, SlReadByte);
			_sl.obj_len = len;
			size_t start_pos = _sl.reader->GetSize();
			size_t endoffs = start_pos + len;
			ch.LoadCheck(len);

			if (_sl.reader->GetSize() != endoffs) {
				SlErrorCorruptFmt("Invalid chunk size in RIFF in {} - expected {}, got {}", ch.GetName(), len, _sl.reader->GetSize() - start_pos);
			}
			break;
		}
		default:
			SlErrorCorrupt("Invalid chunk type");
			break;
	}

	if (_sl.expect_table_header) SlErrorCorrupt("Table chunk without header");
}

/**
 * Save a chunk of data (eg. vehicles, stations, etc.). Each chunk is
 * prefixed by an ID identifying it, followed by data, and terminator where appropriate
 * @param ch The chunkhandler that will be used for the operation
 */
static void SlSaveChunk(const ChunkHandler &ch)
{
	if (ch.type == ChunkType::ReadOnly) return;

	for (uint8_t b : ch.id) SlWriteByte(b);
	Debug(Facility::Sl, Severity::Warning, "Saving chunk {}", ch.GetName());

	_sl.chunk_type = ch.type;
	_sl.expect_table_header = SaveChunkHasTableHeader(to_underlying(ch.type));

	_sl.need_length = (_sl.expect_table_header || _sl.chunk_type == ChunkType::Riff) ? NeedLength::WantLength : NeedLength::None;

	switch (_sl.chunk_type) {
		case ChunkType::Riff:
			ch.Save();
			break;
		case ChunkType::Table:
		case ChunkType::Array:
			_sl.last_array_index = 0;
			SlWriteByte(to_underlying(_sl.chunk_type));
			ch.Save();
			SlWriteArrayLength(0); // Terminate arrays
			break;
		case ChunkType::SparseTable:
		case ChunkType::SparseArray:
			SlWriteByte(to_underlying(_sl.chunk_type));
			ch.Save();
			SlWriteArrayLength(0); // Terminate arrays
			break;
		default: NOT_REACHED();
	}

	if (_sl.expect_table_header) SlErrorCorrupt("Table chunk without header");
}

/** Save all chunks */
static void SlSaveChunks()
{
	for (auto &ch : ChunkHandlers()) {
		SlSaveChunk(ch);
	}

	/* Terminator */
	SlWriteUint32(0);
}

/**
 * Find the ChunkHandler that will be used for processing the found
 * chunk in the savegame or in memory
 * @param id the chunk in question
 * @return returns the appropriate chunkhandler
 */
static const ChunkHandler *SlFindChunkHandler(ChunkId id)
{
	for (const ChunkHandler &ch : ChunkHandlers()) if (ch.id == id) return &ch;
	return nullptr;
}

/** Load all chunks */
static void SlLoadChunks()
{
	for (ChunkId id = SlReadChunkId(); !id.Empty(); id = SlReadChunkId()) {
		Debug(Facility::Sl, Severity::Warning, "Loading chunk {}", id.AsString());

		const ChunkHandler *ch = SlFindChunkHandler(id);
		if (ch == nullptr) SlErrorCorrupt("Unknown chunk type");
		SlLoadChunk(*ch);
	}
}

/** Load all chunks for savegame checking */
static void SlLoadCheckChunks()
{
	for (ChunkId id = SlReadChunkId(); !id.Empty(); id = SlReadChunkId()) {
		Debug(Facility::Sl, Severity::Warning, "Loading chunk {}", id.AsString());

		const ChunkHandler *ch = SlFindChunkHandler(id);
		if (ch == nullptr) SlErrorCorrupt("Unknown chunk type");
		SlLoadCheckChunk(*ch);
	}
}

/** Fix all pointers (convert index -> pointer) */
static void SlFixPointers()
{
	_sl.action = SaveLoadAction::Ptrs;

	for (const ChunkHandler &ch : ChunkHandlers()) {
		Debug(Facility::Sl, Severity::Notice, "Fixing pointers for {}", ch.GetName());
		ch.FixPointers();
	}

	assert(_sl.action == SaveLoadAction::Ptrs);
}


/** Yes, simply reading from a file. */
struct FileReader : LoadFilter {
	std::optional<FileHandle> file; ///< The file to read from.
	long begin; ///< The begin of the file.

	/**
	 * Create the file reader, so it reads from a specific file.
	 * @param file The file to read from.
	 */
	FileReader(FileHandle &&file) : LoadFilter(nullptr), file(std::move(file)), begin(ftell(*this->file))
	{
	}

	/** Make sure everything is cleaned up. */
	~FileReader() override
	{
		if (this->file.has_value()) {
			_game_session_stats.savegame_size = ftell(*this->file) - this->begin;
		}
	}

	size_t Read(uint8_t *buf, size_t size) override
	{
		/* We're in the process of shutting down, i.e. in "failure" mode. */
		if (!this->file.has_value()) return 0;

		return fread(buf, 1, size, *this->file);
	}

	void Reset() override
	{
		clearerr(*this->file);
		if (fseek(*this->file, this->begin, SEEK_SET)) {
			Debug(Facility::Sl, Severity::Error, "Could not reset the file reading");
		}
	}
};

/** Yes, simply writing to a file. */
struct FileWriter : SaveFilter {
	std::optional<FileHandle> file; ///< The file to write to.

	/**
	 * Create the file writer, so it writes to a specific file.
	 * @param file The file to write to.
	 */
	FileWriter(FileHandle &&file) : SaveFilter(nullptr), file(std::move(file))
	{
	}

	/** Make sure everything is cleaned up. */
	~FileWriter() override
	{
		this->Finish();
	}

	void Write(const uint8_t *buf, size_t size) override
	{
		/* We're in the process of shutting down, i.e. in "failure" mode. */
		if (!this->file.has_value()) return;

		if (fwrite(buf, 1, size, *this->file) != size) SlError(STR_GAME_SAVELOAD_ERROR_FILE_NOT_WRITEABLE);
	}

	void Finish() override
	{
		if (this->file.has_value()) {
			_game_session_stats.savegame_size = ftell(*this->file);
			this->file.reset();
		}
	}
};

/** Explicit codec failure ownership; detached preflight must never invoke native pointer cleanup. */
struct SaveCodecErrors {
	bool detached = false; ///< True only for a private preflight decoder, never inferred from global action.
	/** Report a codec failure through the caller's explicit error domain.
	 * @param message Existing native localized error identifier.
	 * @param detail Optional original decoder diagnostic.
	 * @throws SaveFramingError Detached failure without touching native save/load state.
	 */
	[[noreturn]] void Fail(StringID message, const std::string &detail = {}) const
	{
		if (this->detached) throw SaveFramingError(detail.empty() ? "Invalid compressed save stream" : detail.c_str());
		SlError(message, detail);
	}
	/** Preserve native corruption reporting while keeping inspection outside native cleanup.
	 * @param detail Original invalid-wire diagnostic.
	 */
	[[noreturn]] void Corrupt(const std::string &detail) const
	{
		if (this->detached) throw SaveFramingError(detail.c_str());
		SlErrorCorrupt(detail);
	}
};

/** @copydoc CreateSaveFileReader */
std::shared_ptr<LoadFilter> CreateSaveFileReader(FileHandle &&file)
{
	return std::make_shared<FileReader>(std::move(file));
}

/*******************************************
 ********** START OF LZO CODE **************
 *******************************************/

#ifdef WITH_LZO

/** Buffer size for the LZO compressor */
static const uint LZO_BUFFER_SIZE = 8192;

/** Filter using LZO compression. */
struct LZOLoadFilter : LoadFilter {
	SaveCodecErrors errors; ///< Explicit native or detached error ownership.
	bool ancient_byte_order; ///< Captured original version, never re-read from a running world.
	/**
	 * Initialise this filter.
	 * @param chain The next filter in this chain.
	 * @param errors Explicit failure policy; native callers retain ordinary save/load errors.
	 * @param version Original version selecting historical pre-envelope byte order.
	 */
	LZOLoadFilter(std::shared_ptr<LoadFilter> chain, SaveCodecErrors errors = {}, SaveLoadVersion version = _sl_version) : LoadFilter(std::move(chain)), errors(errors), ancient_byte_order(version == SaveLoadVersion::MinVersion)
	{
		if (lzo_init() != LZO_E_OK) this->errors.Fail(STR_GAME_SAVELOAD_ERROR_BROKEN_INTERNAL_ERROR, "cannot initialize decompressor");
	}

	/** Decode through the explicit failure domain.
	 * @param buf Caller-owned destination buffer.
	 * @param ssize Destination capacity, at least one native block for LZO.
	 * @return Actual decoded bytes, or zero only after a complete compressed stream.
	 */
	size_t Read(uint8_t *buf, size_t ssize) override
	{
		assert(ssize >= LZO_BUFFER_SIZE);

		/* Buffer size is from the LZO docs plus the chunk header size. */
		uint8_t out[LZO_BUFFER_SIZE + LZO_BUFFER_SIZE / 16 + 64 + 3 + sizeof(uint32_t) * 2];
		uint32_t tmp[2];
		uint32_t size;
		lzo_uint len = ssize;

		/* Read header*/
		const size_t header_bytes = this->chain->Read(reinterpret_cast<uint8_t *>(tmp), sizeof(tmp));
		// A complete final block may end at EOF; a partial block header is never a valid terminator.
		// Only the detached scanner probes for EOF after the chunk terminator;
		// an ordinary native request still requires its original complete block.
		if (header_bytes == 0 && this->errors.detached) return 0;
		if (header_bytes != sizeof(tmp)) this->errors.Fail(STR_GAME_SAVELOAD_ERROR_FILE_NOT_READABLE, "File read failed");

		/* Check if size is bad */
		((uint32_t*)out)[0] = size = tmp[1];

		if (!this->ancient_byte_order) {
			tmp[0] = TO_BE32(tmp[0]);
			size = TO_BE32(size);
		}

		if (size > sizeof(out) - sizeof(uint32_t)) this->errors.Corrupt("Inconsistent size");

		/* Read block */
		if (this->chain->Read(out + sizeof(uint32_t), size) != size) this->errors.Fail(STR_GAME_SAVELOAD_ERROR_FILE_NOT_READABLE);

		/* Verify checksum */
		if (tmp[0] != lzo_adler32(0, out, size + sizeof(uint32_t))) this->errors.Corrupt("Bad checksum");

		/* Decompress */
		int ret = lzo1x_decompress_safe(out + sizeof(uint32_t) * 1, size, buf, &len, nullptr);
		if (ret != LZO_E_OK) this->errors.Fail(STR_GAME_SAVELOAD_ERROR_FILE_NOT_READABLE);
		return len;
	}
};

/** Filter using LZO compression. */
struct LZOSaveFilter : SaveFilter {
	/**
	 * Initialise this filter.
	 * @param chain             The next filter in this chain.
	 */
	LZOSaveFilter(std::shared_ptr<SaveFilter> chain, uint8_t) : SaveFilter(std::move(chain))
	{
		if (lzo_init() != LZO_E_OK) SlError(STR_GAME_SAVELOAD_ERROR_BROKEN_INTERNAL_ERROR, "cannot initialize compressor");
	}

	void Write(const uint8_t *buf, size_t size) override
	{
		const lzo_bytep in = buf;
		/* Buffer size is from the LZO docs plus the chunk header size. */
		uint8_t out[LZO_BUFFER_SIZE + LZO_BUFFER_SIZE / 16 + 64 + 3 + sizeof(uint32_t) * 2];
		uint8_t wrkmem[LZO1X_1_MEM_COMPRESS];
		lzo_uint outlen;

		do {
			/* Compress up to LZO_BUFFER_SIZE bytes at once. */
			lzo_uint len = size > LZO_BUFFER_SIZE ? LZO_BUFFER_SIZE : static_cast<lzo_uint>(size);
			lzo1x_1_compress(in, len, out + sizeof(uint32_t) * 2, &outlen, wrkmem);
			((uint32_t*)out)[1] = TO_BE32(static_cast<uint32_t>(outlen));
			((uint32_t*)out)[0] = TO_BE32(lzo_adler32(0, out + sizeof(uint32_t), outlen + sizeof(uint32_t)));
			this->chain->Write(out, outlen + sizeof(uint32_t) * 2);

			/* Move to next data chunk. */
			size -= len;
			in += len;
		} while (size > 0);
	}
};

#endif /* WITH_LZO */

/*********************************************
 ******** START OF NOCOMP CODE (uncompressed)*
 *********************************************/

/** Filter without any compression. */
struct NoCompLoadFilter : LoadFilter {
	/**
	 * Initialise this filter.
	 * @param chain The next filter in this chain.
	 */
	NoCompLoadFilter(std::shared_ptr<LoadFilter> chain) : LoadFilter(std::move(chain))
	{
	}

	size_t Read(uint8_t *buf, size_t size) override
	{
		return this->chain->Read(buf, size);
	}
};

/** Filter without any compression. */
struct NoCompSaveFilter : SaveFilter {
	/**
	 * Initialise this filter.
	 * @param chain             The next filter in this chain.
	 */
	NoCompSaveFilter(std::shared_ptr<SaveFilter> chain, uint8_t) : SaveFilter(std::move(chain))
	{
	}

	void Write(const uint8_t *buf, size_t size) override
	{
		this->chain->Write(buf, size);
	}
};

/********************************************
 ********** START OF ZLIB CODE **************
 ********************************************/

#if defined(WITH_ZLIB)

/** Filter using Zlib compression. */
struct ZlibLoadFilter : LoadFilter {
	SaveCodecErrors errors; ///< Explicit native or detached error ownership.
	z_stream z{}; ///< Stream state we are reading from.
	uint8_t fread_buf[MEMORY_CHUNK_SIZE]; ///< Buffer for reading from the file.

	/**
	 * Initialise this filter.
	 * @param chain The next filter in this chain.
	 * @param errors Explicit failure policy; native callers retain ordinary save/load errors.
	 */
	ZlibLoadFilter(std::shared_ptr<LoadFilter> chain, SaveCodecErrors errors = {}) : LoadFilter(std::move(chain)), errors(errors)
	{
		if (inflateInit(&this->z) != Z_OK) this->errors.Fail(STR_GAME_SAVELOAD_ERROR_BROKEN_INTERNAL_ERROR, "cannot initialize decompressor");
	}

	/** Clean everything up. */
	~ZlibLoadFilter() override
	{
		inflateEnd(&this->z);
	}

	/** Decode through the explicit failure domain.
	 * @param buf Caller-owned destination buffer.
	 * @param size Destination capacity, at least one native block for LZO.
	 * @return Actual decoded bytes, or zero only after a complete compressed stream.
	 */
	size_t Read(uint8_t *buf, size_t size) override
	{
		this->z.next_out  = buf;
		this->z.avail_out = static_cast<uint>(size);

		do {
			/* read more bytes from the file? */
			if (this->z.avail_in == 0) {
				this->z.next_in = this->fread_buf;
				this->z.avail_in = static_cast<uint>(this->chain->Read(this->fread_buf, sizeof(this->fread_buf)));
			}

			/* A decoder must consume input or produce output on every continuation; malformed streams cannot monopolize preflight. */
			const auto before_in = this->z.avail_in, before_out = this->z.avail_out;
			int r = inflate(&this->z, 0);
			if (r == Z_STREAM_END) break;

			if (r != Z_OK || (before_in == this->z.avail_in && before_out == this->z.avail_out)) this->errors.Fail(STR_GAME_SAVELOAD_ERROR_BROKEN_INTERNAL_ERROR, "inflate() failed");
		} while (this->z.avail_out != 0);

		return size - this->z.avail_out;
	}
};

/** Filter using Zlib compression. */
struct ZlibSaveFilter : SaveFilter {
	z_stream z{}; ///< Stream state we are writing to.
	uint8_t fwrite_buf[MEMORY_CHUNK_SIZE]; ///< Buffer for writing to the file.

	/**
	 * Initialise this filter.
	 * @param chain             The next filter in this chain.
	 * @param compression_level The requested level of compression.
	 */
	ZlibSaveFilter(std::shared_ptr<SaveFilter> chain, uint8_t compression_level) : SaveFilter(std::move(chain))
	{
		if (deflateInit(&this->z, compression_level) != Z_OK) SlError(STR_GAME_SAVELOAD_ERROR_BROKEN_INTERNAL_ERROR, "cannot initialize compressor");
	}

	/** Clean up what we allocated. */
	~ZlibSaveFilter() override
	{
		deflateEnd(&this->z);
	}

	/**
	 * Helper loop for writing the data.
	 * @param p    The bytes to write.
	 * @param len  Amount of bytes to write.
	 * @param mode Mode for deflate.
	 */
	void WriteLoop(const uint8_t *p, size_t len, int mode)
	{
		uint n;
		this->z.next_in = const_cast<uint8_t *>(p); // zlib does not modify the data, but is non-const for legacy reasons
		this->z.avail_in = static_cast<uInt>(len);
		do {
			this->z.next_out = this->fwrite_buf;
			this->z.avail_out = sizeof(this->fwrite_buf);

			/**
			 * For the poor next soul who sees many valgrind warnings of the
			 * "Conditional jump or move depends on uninitialised value(s)" kind:
			 * According to the author of zlib it is not a bug and it won't be fixed.
			 * http://groups.google.com/group/comp.compression/browse_thread/thread/b154b8def8c2a3ef/cdf9b8729ce17ee2
			 * [Mark Adler, Feb 24 2004, 'zlib-1.2.1 valgrind warnings' in the newsgroup comp.compression]
			 */
			int r = deflate(&this->z, mode);

			/* bytes were emitted? */
			if ((n = sizeof(this->fwrite_buf) - this->z.avail_out) != 0) {
				this->chain->Write(this->fwrite_buf, n);
			}
			if (r == Z_STREAM_END) break;

			if (r != Z_OK) SlError(STR_GAME_SAVELOAD_ERROR_BROKEN_INTERNAL_ERROR, "zlib returned error code");
		} while (this->z.avail_in || !this->z.avail_out);
	}

	void Write(const uint8_t *buf, size_t size) override
	{
		this->WriteLoop(buf, size, 0);
	}

	void Finish() override
	{
		this->WriteLoop(nullptr, 0, Z_FINISH);
		this->chain->Finish();
	}
};

#endif /* WITH_ZLIB */

/********************************************
 ********** START OF LZMA CODE **************
 ********************************************/

#if defined(WITH_LIBLZMA)

/**
 * Have a copy of an initialised LZMA stream. We need this as it's
 * impossible to "re"-assign LZMA_STREAM_INIT to a variable in some
 * compilers, i.e. LZMA_STREAM_INIT can't be used to set something.
 * This var has to be used instead.
 */
static const lzma_stream _lzma_init = LZMA_STREAM_INIT;

/** Filter without any compression. */
struct LZMALoadFilter : LoadFilter {
	SaveCodecErrors errors; ///< Explicit native or detached error ownership.
	lzma_stream lzma;                  ///< Stream state that we are reading from.
	uint8_t fread_buf[MEMORY_CHUNK_SIZE]; ///< Buffer for reading from the file.

	/**
	 * Initialise this filter.
	 * @param chain The next filter in this chain.
	 * @param errors Explicit failure policy; native callers retain ordinary save/load errors.
	 */
	LZMALoadFilter(std::shared_ptr<LoadFilter> chain, SaveCodecErrors errors = {}) : LoadFilter(std::move(chain)), errors(errors), lzma(_lzma_init)
	{
		/* Allow saves up to 256 MB uncompressed */
		if (lzma_auto_decoder(&this->lzma, 1 << 28, 0) != LZMA_OK) this->errors.Fail(STR_GAME_SAVELOAD_ERROR_BROKEN_INTERNAL_ERROR, "cannot initialize decompressor");
	}

	/** Clean everything up. */
	~LZMALoadFilter() override
	{
		lzma_end(&this->lzma);
	}

	/** Decode through the explicit failure domain.
	 * @param buf Caller-owned destination buffer.
	 * @param size Destination capacity, at least one native block for LZO.
	 * @return Actual decoded bytes, or zero only after a complete compressed stream.
	 */
	size_t Read(uint8_t *buf, size_t size) override
	{
		this->lzma.next_out  = buf;
		this->lzma.avail_out = size;

		do {
			/* read more bytes from the file? */
			if (this->lzma.avail_in == 0) {
				this->lzma.next_in  = this->fread_buf;
				this->lzma.avail_in = this->chain->Read(this->fread_buf, sizeof(this->fread_buf));
			}

			/* Require actual decoder progress so malformed compressed streams cannot monopolize preflight. */
			const auto before_in = this->lzma.avail_in, before_out = this->lzma.avail_out;
			lzma_ret r = lzma_code(&this->lzma, LZMA_RUN);
			if (r == LZMA_STREAM_END) break;
			if (r != LZMA_OK || (before_in == this->lzma.avail_in && before_out == this->lzma.avail_out)) this->errors.Fail(STR_GAME_SAVELOAD_ERROR_BROKEN_INTERNAL_ERROR, "liblzma returned error code");
		} while (this->lzma.avail_out != 0);

		return size - this->lzma.avail_out;
	}
};

/** Filter using LZMA compression. */
struct LZMASaveFilter : SaveFilter {
	lzma_stream lzma; ///< Stream state that we are writing to.
	uint8_t fwrite_buf[MEMORY_CHUNK_SIZE]; ///< Buffer for writing to the file.

	/**
	 * Initialise this filter.
	 * @param chain             The next filter in this chain.
	 * @param compression_level The requested level of compression.
	 */
	LZMASaveFilter(std::shared_ptr<SaveFilter> chain, uint8_t compression_level) : SaveFilter(std::move(chain)), lzma(_lzma_init)
	{
		if (lzma_easy_encoder(&this->lzma, compression_level, LZMA_CHECK_CRC32) != LZMA_OK) SlError(STR_GAME_SAVELOAD_ERROR_BROKEN_INTERNAL_ERROR, "cannot initialize compressor");
	}

	/** Clean up what we allocated. */
	~LZMASaveFilter() override
	{
		lzma_end(&this->lzma);
	}

	/**
	 * Helper loop for writing the data.
	 * @param p      The bytes to write.
	 * @param len    Amount of bytes to write.
	 * @param action Action for lzma_code.
	 */
	void WriteLoop(const uint8_t *p, size_t len, lzma_action action)
	{
		size_t n;
		this->lzma.next_in = p;
		this->lzma.avail_in = len;
		do {
			this->lzma.next_out = this->fwrite_buf;
			this->lzma.avail_out = sizeof(this->fwrite_buf);

			lzma_ret r = lzma_code(&this->lzma, action);

			/* bytes were emitted? */
			if ((n = sizeof(this->fwrite_buf) - this->lzma.avail_out) != 0) {
				this->chain->Write(this->fwrite_buf, n);
			}
			if (r == LZMA_STREAM_END) break;
			if (r != LZMA_OK) SlError(STR_GAME_SAVELOAD_ERROR_BROKEN_INTERNAL_ERROR, "liblzma returned error code");
		} while (this->lzma.avail_in || !this->lzma.avail_out);
	}

	void Write(const uint8_t *buf, size_t size) override
	{
		this->WriteLoop(buf, size, LZMA_RUN);
	}

	void Finish() override
	{
		this->WriteLoop(nullptr, 0, LZMA_FINISH);
		this->chain->Finish();
	}
};

#endif /* WITH_LIBLZMA */

/*******************************************
 ************* END OF CODE *****************
 *******************************************/

/** Unique 4-letter tag for the different saveload formats. */
using SaveLoadFormatTag = Label<struct SaveLoadFormatLabelTag>;

/** The format for a reader/writer type of a savegame */
struct SaveLoadFormat {
	std::shared_ptr<LoadFilter> (*init_load)(std::shared_ptr<LoadFilter> chain); ///< Constructor for the load filter.
	std::shared_ptr<SaveFilter> (*init_write)(std::shared_ptr<SaveFilter> chain, uint8_t compression); ///< Constructor for the save filter.

	std::string_view name; ///< name of the compressor/decompressor (debug-only)
	SaveLoadFormatTag tag; ///< the 4-letter tag by which it is identified in the savegame

	uint8_t min_compression;                 ///< the minimum compression level of this format
	uint8_t default_compression;             ///< the default compression level of this format
	uint8_t max_compression;                 ///< the maximum compression level of this format
};

static const SaveLoadFormatTag SAVEGAME_TAG_LZO{"OTTD"}; ///< Tag for a game compressed with LZO
static const SaveLoadFormatTag SAVEGAME_TAG_NONE{"OTTN"}; ///< Tag for a game without compression.
static const SaveLoadFormatTag SAVEGAME_TAG_ZLIB{"OTTZ"}; ///< Tag for a game with zlib compression.
static const SaveLoadFormatTag SAVEGAME_TAG_LZMA{"OTTX"}; ///< Tag for a game with lzma compression.

/** The different saveload formats known/understood by OpenTTD. */
static const SaveLoadFormat _saveload_formats[] = {
#if defined(WITH_LZO)
	/* Roughly 75% larger than zlib level 6 at only ~7% of the CPU usage. */
	{CreateLoadFilter<LZOLoadFilter>, CreateSaveFilter<LZOSaveFilter>, "lzo", SAVEGAME_TAG_LZO, 0, 0, 0},
#else
	{nullptr, nullptr, "lzo", SAVEGAME_TAG_LZO, 0, 0, 0},
#endif
	/* Roughly 5 times larger at only 1% of the CPU usage over zlib level 6. */
	{CreateLoadFilter<NoCompLoadFilter>, CreateSaveFilter<NoCompSaveFilter>, "none", SAVEGAME_TAG_NONE, 0, 0, 0},
#if defined(WITH_ZLIB)
	/* After level 6 the speed reduction is significant (1.5x to 2.5x slower per level), but the reduction in filesize is
	 * fairly insignificant (~1% for each step). Lower levels become ~5-10% bigger by each level than level 6 while level
	 * 1 is "only" 3 times as fast. Level 0 results in uncompressed savegames at about 8 times the cost of "none". */
	{CreateLoadFilter<ZlibLoadFilter>, CreateSaveFilter<ZlibSaveFilter>, "zlib", SAVEGAME_TAG_ZLIB, 0, 6, 9},
#else
	{nullptr, nullptr, "zlib", SAVEGAME_TAG_ZLIB, 0, 0, 0},
#endif
#if defined(WITH_LIBLZMA)
	/* Level 2 compression is speed wise as fast as zlib level 6 compression (old default), but results in ~10% smaller saves.
	 * Higher compression levels are possible, and might improve savegame size by up to 25%, but are also up to 10 times slower.
	 * The next significant reduction in file size is at level 4, but that is already 4 times slower. Level 3 is primarily 50%
	 * slower while not improving the filesize, while level 0 and 1 are faster, but don't reduce savegame size much.
	 * It's OTTX and not e.g. OTTL because liblzma is part of xz-utils and .tar.xz is preferred over .tar.lzma. */
	{CreateLoadFilter<LZMALoadFilter>, CreateSaveFilter<LZMASaveFilter>, "lzma", SAVEGAME_TAG_LZMA, 0, 2, 9},
#else
	{nullptr, nullptr, "lzma", SAVEGAME_TAG_LZMA, 0, 0, 0},
#endif
};

/** @copydoc CreateSaveInspectionDecoder */
std::shared_ptr<LoadFilter> CreateSaveInspectionDecoder(std::shared_ptr<LoadFilter> raw, const std::array<uint8_t, 4> &tag, uint16_t version)
{
	SaveLoadFormatTag original{};
	std::ranges::copy(tag, original.begin());
	if (original == SAVEGAME_TAG_NONE) return std::make_shared<NoCompLoadFilter>(std::move(raw));
#ifdef WITH_LZO
	if (original == SAVEGAME_TAG_LZO) return std::make_shared<LZOLoadFilter>(std::move(raw), SaveCodecErrors{true}, static_cast<SaveLoadVersion>(version));
#endif
#ifdef WITH_ZLIB
	if (original == SAVEGAME_TAG_ZLIB) return std::make_shared<ZlibLoadFilter>(std::move(raw), SaveCodecErrors{true});
#endif
#ifdef WITH_LIBLZMA
	if (original == SAVEGAME_TAG_LZMA) return std::make_shared<LZMALoadFilter>(std::move(raw), SaveCodecErrors{true});
#endif
	throw SaveFramingError("Unsupported save compression identifier");
}

/**
 * Return the savegameformat of the game. Whether it was created with ZLIB compression
 * uncompressed, or another type
 * @param full_name Name of the savegame format. If empty it picks the first available one
 * @return Pair containing reference to SaveLoadFormat struct giving all characteristics of this type of savegame, and a compression level to use.
 */
static std::pair<const SaveLoadFormat &, uint8_t> GetSavegameFormat(std::string_view full_name)
{
	/* Find default savegame format, the highest one with which files can be written. */
	auto it = std::find_if(std::rbegin(_saveload_formats), std::rend(_saveload_formats), [](const auto &slf) { return slf.init_write != nullptr; });
	if (it == std::rend(_saveload_formats)) SlError(STR_GAME_SAVELOAD_ERROR_BROKEN_INTERNAL_ERROR, "no writeable savegame formats");

	const SaveLoadFormat &def = *it;

	if (!full_name.empty()) {
		/* Get the ":..." of the compression level out of the way */
		size_t separator = full_name.find(':');
		bool has_comp_level = separator != std::string::npos;
		std::string_view name = has_comp_level ? full_name.substr(0, separator) : full_name;

		for (const auto &slf : _saveload_formats) {
			if (slf.init_write != nullptr && name == slf.name) {
				if (has_comp_level) {
					auto complevel = full_name.substr(separator + 1);

					/* Get the level and determine whether all went fine. */
					auto level = ParseInteger<uint8_t>(complevel);
					if (!level.has_value() || *level != Clamp(*level, slf.min_compression, slf.max_compression)) {
						ShowErrorMessage(
							GetEncodedString(STR_CONFIG_ERROR),
							GetEncodedString(STR_CONFIG_ERROR_INVALID_SAVEGAME_COMPRESSION_LEVEL, complevel),
							WarningLevel::Critical);
					} else {
						return {slf, *level};
					}
				}
				return {slf, slf.default_compression};
			}
		}

		ShowErrorMessage(
			GetEncodedString(STR_CONFIG_ERROR),
			GetEncodedString(STR_CONFIG_ERROR_INVALID_SAVEGAME_COMPRESSION_ALGORITHM, name, def.name),
			WarningLevel::Critical);
	}
	return {def, def.default_compression};
}

/* actual loader/saver function */
void InitializeGame(uint size_x, uint size_y, bool reset_date, bool reset_settings);
extern bool AfterLoadGame();
extern bool LoadOldSaveGame(std::string_view file);

/**
 * Reset all settings to their default, so any settings missing in the savegame
 * are their default, and not "value of last game". AfterLoad might still fix
 * up values to become non-default, depending on the saveload version.
 */
static void ResetSettings()
{
	for (auto &desc : GetSaveLoadSettingTable()) {
		const SettingDesc *sd = GetSettingDesc(desc);
		if (sd->flags.Test(SettingFlag::NotInSave)) continue;
		if (sd->flags.Test(SettingFlag::NoNetworkSync) && _networking && !_network_server) continue;

		sd->ResetToDefault(&_settings_game);
	}
}

extern void ClearOldOrders();

/**
 * Clear temporary data that is passed between various saveload phases.
 */
static void ResetSaveloadData()
{
	ClearOldOrders();
	ResetTempEngineData();
	ClearRailTypeLabelList();
	ClearRoadTypeLabelList();
	ResetOldWaypoints();
	ResetSettings();
}

/**
 * Clear/free saveload state.
 */
static inline void ClearSaveLoadState()
{
	_sl.dumper = nullptr;
	_sl.sf = nullptr;
	_sl.reader = nullptr;
	_sl.lf = nullptr;
}

/** Update the gui accordingly when starting saving and set locks on saveload. */
static void SaveFileStart()
{
	SetMouseCursorBusy(true);

	InvalidateWindowData(WindowClass::Statusbar, 0, SBI_SAVELOAD_START);
	_sl.saveinprogress = true;
}

/** Update the gui accordingly when saving is done and release locks on saveload. */
static void SaveFileDone()
{
	SetMouseCursorBusy(false);

	InvalidateWindowData(WindowClass::Statusbar, 0, SBI_SAVELOAD_FINISH);
	_sl.saveinprogress = false;

#ifdef __EMSCRIPTEN__
	EM_ASM(if (window["openttd_syncfs"]) openttd_syncfs());
#endif
}

/**
 * Set the error message from outside of the actual loading/saving of the game (AfterLoadGame and friends).
 * @param str The string describing the error.
 */
void SetSaveLoadError(StringID str)
{
	_sl.error_str = str;
}

/**
 * Return the appropriate initial string for an error depending on whether we are saving or loading.
 * @return The encoded string with the error type.
 */
EncodedString GetSaveLoadErrorType()
{
	return GetEncodedString(_sl.action == SaveLoadAction::Save ? STR_ERROR_GAME_SAVE_FAILED : STR_ERROR_GAME_LOAD_FAILED);
}

/**
 * Return the description of the error.
 * @return The encoded string with the error message.
 */
EncodedString GetSaveLoadErrorMessage()
{
	return GetEncodedString(_sl.error_str, _sl.extra_msg);
}

/** Show a gui message when saving has failed */
static void SaveFileError()
{
	ShowErrorMessage(GetSaveLoadErrorType(), GetSaveLoadErrorMessage(), WarningLevel::Error);
	SaveFileDone();
}

/**
 * We have written the whole game into memory, _memory_savegame, now find
 * and appropriate compressor and start writing to file.
 * @param threaded Whether to run the compression in the background or not.
 * @return SaveLoadResult::Ok when everything went okay, otherwise an error.
 */
static SaveLoadResult SaveFileToDisk(bool threaded)
{
	try {
		auto [fmt, compression] = GetSavegameFormat(_savegame_format);

		/* We have written our stuff to memory, now write it to file! */
		_sl.sf->Write(fmt.tag.data(), fmt.tag.size());

		uint32_t version = TO_BE32(to_underlying(SAVEGAME_VERSION) << 16);
		_sl.sf->Write(reinterpret_cast<uint8_t *>(&version), sizeof(version));

		_sl.sf = fmt.init_write(_sl.sf, compression);
		_sl.dumper->Flush(_sl.sf);

		ClearSaveLoadState();

		if (threaded) SetAsyncSaveFinish(SaveFileDone);

		return SaveLoadResult::Ok;
	} catch (...) {
		ClearSaveLoadState();

		AsyncSaveFinishProc asfp = SaveFileDone;

		/* We don't want to shout when saving is just
		 * cancelled due to a client disconnecting. */
		if (_sl.error_str != STR_NETWORK_ERROR_LOSTCONNECTION) {
			Debug(Facility::Sl, Severity::Critical, "{} {}", GetSaveLoadErrorType().GetDecodedString(), GetSaveLoadErrorMessage().GetDecodedString());
			asfp = SaveFileError;
		}

		if (threaded) {
			SetAsyncSaveFinish(asfp);
		} else {
			asfp();
		}
		return SaveLoadResult::Error;
	}
}

void WaitTillSaved()
{
	if (!_save_thread.joinable()) return;

	_save_thread.join();

	/* Make sure every other state is handled properly as well. */
	ProcessAsyncSaveFinish();
}

/**
 * Actually perform the saving of the savegame.
 * General tactics is to first save the game to memory, then write it to file
 * using the writer, either in threaded mode if possible, or single-threaded.
 * @param writer   The filter to write the savegame to.
 * @param threaded Whether to try to perform the saving asynchronously.
 * @return Return the result of the action. #SaveLoadResult::Ok or #SaveLoadResult::Error
 */
static SaveLoadResult DoSave(std::shared_ptr<SaveFilter> writer, bool threaded)
{
	assert(!_sl.saveinprogress);

	_sl.dumper = std::make_unique<MemoryDumper>();
	_sl.sf = std::move(writer);

	_sl_version = SAVEGAME_VERSION;

	SaveViewportBeforeSaveGame();
	SlSaveChunks();

	SaveFileStart();

	if (!threaded || !StartNewThread(&_save_thread, "ottd:savegame", &SaveFileToDisk, true)) {
		if (threaded) Debug(Facility::Sl, Severity::Error, "Cannot create savegame thread, reverting to single-threaded mode...");

		SaveLoadResult result = SaveFileToDisk(false);
		SaveFileDone();

		return result;
	}

	return SaveLoadResult::Ok;
}

/**
 * Save the game using a (writer) filter.
 * @param writer   The filter to write the savegame to.
 * @param threaded Whether to try to perform the saving asynchronously.
 * @return Return the result of the action. #SaveLoadResult::Ok or #SaveLoadResult::Error
 */
SaveLoadResult SaveWithFilter(std::shared_ptr<SaveFilter> writer, bool threaded)
{
	try {
		_sl.action = SaveLoadAction::Save;
		return DoSave(std::move(writer), threaded);
	} catch (...) {
		ClearSaveLoadState();
		return SaveLoadResult::Error;
	}
}

/**
 * Determines the SaveLoadFormat that is connected to the given tag.
 * When the given tag is known, that format is chosen and a check on the validity of the version is performed.
 * Otherwise a fallback to an ancient buggy format using LZO is chosen.
 * @param tag The tag from the header describing the savegame compression/format.
 * @param raw_version The raw version from the savegame header.
 * @return The SaveLoadFormat to use for attempting to open the savegame.
 */
static const SaveLoadFormat *DetermineSaveLoadFormat(SaveLoadFormatTag tag, uint32_t raw_version)
{
	auto fmt = std::ranges::find(_saveload_formats, tag, &SaveLoadFormat::tag);
	if (fmt != std::end(_saveload_formats)) {
		/* Check version number */
		_sl_version = (SaveLoadVersion)(TO_BE32(raw_version) >> 16);
		/* Minor is not used anymore from version 18.0, but it is still needed
		 * in versions before that (4 cases) which can't be removed easy.
		 * Therefore it is loaded, but never saved (or, it saves a 0 in any scenario). */
		_sl_minor_version = (TO_BE32(raw_version) >> 8) & 0xFF;

		Debug(Facility::Sl, Severity::Error, "Loading savegame version {}", _sl_version);

		/* Is the version higher than the current? */
		if (_sl_version > SAVEGAME_VERSION) SlError(STR_GAME_SAVELOAD_ERROR_TOO_NEW_SAVEGAME);
		if (_sl_version >= SaveLoadVersion::StartPatchpacks && _sl_version <= SaveLoadVersion::EndPatchpacks) SlError(STR_GAME_SAVELOAD_ERROR_PATCHPACK);
		return fmt;
	}

	Debug(Facility::Sl, Severity::Critical, "Unknown savegame type, trying to load it as the buggy format");
	_sl.lf->Reset();
	_sl_version = SaveLoadVersion::MinVersion;
	_sl_minor_version = 0;

	/* Try to find the LZO savegame format; it uses 'OTTD' as tag. */
	fmt = std::ranges::find(_saveload_formats, SAVEGAME_TAG_LZO, &SaveLoadFormat::tag);
	if (fmt == std::end(_saveload_formats)) {
		/* Who removed the LZO savegame format definition? When built without LZO support,
		 * the formats must still list it just without a method to read the file.
		 * The caller of this function has to check for the existence of load function. */
		NOT_REACHED();
	}
	return fmt;
}

/** Report a refusal before native world mutation without invoking SlError's pointer cleanup.
 * @param load_check Route the diagnostic to copied preview as well as the ordinary error owner.
 * @param message Existing localized save/load error identifier.
 * @param detail Optional bounded parser/classifier diagnostic.
 * @return Error, explicitly preserving the existing live world rather than requesting ReInit.
 */
static SaveLoadResult ReportSavePreflightRefusal(bool load_check, StringID message, const std::string &detail = {})
{
	_sl.error_str = message; _sl.extra_msg = detail;
	if (load_check) {
		// An unreadable or refused next file cannot retain the preceding preview's
		// valid-looking paid receipts, utility grants or provider references.
		_load_check_data.Clear(); _load_check_data.checkable = true;
		_load_check_data.error = message; _load_check_data.error_msg = detail;
	}
	ClearSaveLoadState();
	return SaveLoadResult::Error;
}

/** Discard a refused copied candidate while retaining its useful native diagnostic.
 * No live owner is reset: preview errors never initialize or partially install a world.
 */
static void DiscardFailedLoadCheckData()
{
	const auto error = _load_check_data.error; const auto message = _load_check_data.error_msg;
	_load_check_data.Clear(); _load_check_data.checkable = true;
	_load_check_data.error = error; _load_check_data.error_msg = message;
}

/**
 * Actually perform the loading of a "non-old" savegame.
 * @param reader     The filter to read the savegame from.
 * @param load_check Whether to perform the checking ("preview") or actually load the game.
 * @param origin Optional exact-digest legacy origin for this attempt only.
 * @return Ok on success, Error for nonmutating preflight/copied refusal, or ReInit for a later native full-load failure.
 */
static SaveLoadResult DoLoad(std::shared_ptr<LoadFilter> reader, bool load_check, const std::optional<SaveOriginHint> &origin)
{
	ScopedLoadLineage lineage;
	const auto raw = std::move(reader);
	_sl.lf = raw;

	if (load_check) {
		_load_check_data.Clear();
		_load_check_data.checkable = true;
	}

	SaveLoadFormatTag tag{};
	uint32_t version = 0;
	SaveWireFacts facts;
	SaveLineageResult capabilities;
	try {
		if (raw == nullptr) throw SaveFramingError("Missing original save reader");
		raw->Reset();
		std::array<uint8_t, 8> original_header{};
		if (raw->Read(original_header.data(), original_header.size()) != original_header.size()) throw SaveFramingError("Truncated save envelope");
		facts = InspectSaveWireFacts(raw, {}, true);
		capabilities = ClassifySaveFormat(facts, origin);
		if (capabilities.status == SaveLineageStatus::Ambiguous) return ReportSavePreflightRefusal(load_check, STR_GAME_SAVELOAD_ERROR_AMBIGUOUS_LINEAGE);
		// Unsupported means either a later writer or a development build's own numbers
		// at or below the current writer; the player sees which one it is.
		if (capabilities.status == SaveLineageStatus::Unsupported) return ReportSavePreflightRefusal(load_check, facts.original_version > CURRENT_MERGED_SAVE_VERSION ? STR_GAME_SAVELOAD_ERROR_TOO_NEW_SAVEGAME : STR_GAME_SAVELOAD_ERROR_DEVELOPMENT_SAVEGAME);
		if (capabilities.status != SaveLineageStatus::Accepted) return ReportSavePreflightRefusal(load_check, STR_GAME_SAVELOAD_ERROR_INVALID_LINEAGE, "Its format markers are missing or do not agree");
		if (facts.original_version > to_underlying(SAVEGAME_VERSION)) return ReportSavePreflightRefusal(load_check, STR_GAME_SAVELOAD_ERROR_TOO_NEW_SAVEGAME);
		// Native patchpack reservations remain unsupported even if their outer
		// framing is readable; preflight refusal must not null the running world's pointers.
		if (facts.original_version >= to_underlying(SaveLoadVersion::StartPatchpacks) && facts.original_version <= to_underlying(SaveLoadVersion::EndPatchpacks)) return ReportSavePreflightRefusal(load_check, STR_GAME_SAVELOAD_ERROR_PATCHPACK);
		std::array<uint8_t, 8> replay_header{};
		if (raw->Read(replay_header.data(), replay_header.size()) != replay_header.size() || replay_header != original_header) throw SaveFramingError("Original save envelope changed before replay"); // Fresh native dispatch must retain exactly the inspected envelope, never a remembered version/tag.
		std::copy_n(replay_header.begin(), 4, tag.begin());
		std::copy_n(replay_header.data() + 4, sizeof(version), reinterpret_cast<uint8_t *>(&version));
	} catch (const std::exception &error) {
		return ReportSavePreflightRefusal(load_check, STR_GAME_SAVELOAD_ERROR_INVALID_LINEAGE, error.what());
	} catch (...) {
		return ReportSavePreflightRefusal(load_check, STR_GAME_SAVELOAD_ERROR_INVALID_LINEAGE, "Original save preflight failed");
	}

	// Only admitted bytes reach the original format/version owner. It retains
	// ancient headerless LZO handling and every original numeric version. The raw
	// source is at byte8 (or reset by its ancient fallback), with no reused codec.
	const SaveLoadFormat *fmt = DetermineSaveLoadFormat(tag, version);
	if (fmt->init_load == nullptr) return ReportSavePreflightRefusal(load_check, STR_GAME_SAVELOAD_ERROR_INVALID_LINEAGE, "Original save decoder is unavailable");
	lineage.Bind(capabilities);

	_sl.lf = fmt->init_load(_sl.lf);
	_sl.reader = std::make_unique<ReadBuffer>(_sl.lf);
	_next_offs = 0;

	if (!load_check) {
		ResetSaveloadData();

		/* Old maps were hardcoded to 256x256 and thus did not contain
		 * any mapsize information. Pre-initialize to 256x256 to not to
		 * confuse old games */
		InitializeGame(256, 256, true, true);

		_gamelog.Reset();

		if (IsSavegameVersionBefore(SaveLoadVersion::TownTolerancePauseMode)) {
			/*
			 * NewGRFs were introduced between 0.3,4 and 0.3.5, which both
			 * shared savegame version 4. Anything before that 'obviously'
			 * does not have any NewGRFs. Between the introduction and
			 * savegame version 41 (just before 0.5) the NewGRF settings
			 * were not stored in the savegame and they were loaded by
			 * using the settings from the main menu.
			 * So, to recap:
			 * - savegame version  <  4:  do not load any NewGRFs.
			 * - savegame version >= 41:  load NewGRFs from savegame, which is
			 *                            already done at this stage by
			 *                            overwriting the main menu settings.
			 * - other savegame versions: use main menu settings.
			 *
			 * This means that users *can* crash savegame version 4..40
			 * savegames if they set incompatible NewGRFs in the main menu,
			 * but can't crash anymore for savegame version < 4 savegames.
			 *
			 * Note: this is done here because AfterLoadGame is also called
			 * for TTO/TTD/TTDP savegames which have their own NewGRF logic.
			 */
			ClearGRFConfigList(_grfconfig);
		}
	}

	if (load_check) {
		/* Load chunks into _load_check_data.
		 * No pools are loaded. References are not possible, and thus do not need resolving. */
		SlLoadCheckChunks();
	} else {
		/* Load chunks and resolve references */
		SlLoadChunks();
		SlFixPointers();
	}

	ClearSaveLoadState();

	_savegame_type = SavegameType::OTTD;

	if (load_check) {
		/* The only part from AfterLoadGame() we need */
		_load_check_data.grf_compatibility = IsGoodGRFConfigList(_load_check_data.grfconfig);
		ValidateExcavationLoadPreview();
		ValidateAsteroidLoadPreview();
		ValidateLaunchCenterLoadPreview();
		ValidateSpaceContentLoadPreview();
		ValidateSpaceOrderLoadPreview();
		ValidateAsteroidKnowledgeLoadPreview();
		ValidateSpaceProductionLoadPreview();
		ValidateSpaceManifestLoadPreview();
		ValidateSpacePodLoadPreview();
		ValidateSpaceStructureLoadPreview();
		ValidateOrbitalRecipeLoadPreview(); // Same-file native structures, utility and clock supply processing ownership; the active world grants no work to preview.
		ValidateSurfaceSupplyOrderLoadPreview(); // Actual same-file native economy day, map/company/station facts validate pending rights; the open world's stock/clock grants none.
		ValidateSpaceTransitionLoadPreview();
		ValidateSpaceDescentLoadPreview();
		// Current files publish actual configured mass separately from job receipts; genuine older saves retain explicitly limited copied admission and their full native dimension checks.
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceCargoDimensions)) {
			if (!ValidateSpaceCargoPreviewDimensions(_load_check_data)) SlErrorCorrupt("Changed or missing configured cargo dimension preview");
		}
		/** @param row Original candidate packet. @return Whether historical natural provenance still needs field/mineral/custody validation, even without any surviving Space owner. */
		const auto has_natural_source = [](const auto &row) { return row.source.type == SourceType::Asteroid; };
		// Modern Space worlds and retained natural freight must join all actual
		// same-file containers after their readers finish. Removing the last Space
		// hull/site cannot bypass historical cargo validation on ordinary carriers.
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceHeavyMinerCommissioning) &&
			(!_load_check_data.native_space_vehicle_rows.empty() || !_load_check_data.space_structure_rows.empty() || !_load_check_data.space_descent_rows.empty() ||
			std::ranges::any_of(_load_check_data.native_cargo_packet_rows, has_natural_source))) {
			if (!ValidateSpaceCargoPreviewOwners(_load_check_data)) SlErrorCorrupt("Shared or missing native/Space cargo ownership preview");
		}
		ValidateLayerPortalWorkLoadPreview();
		// CAPA sources without typed origins gain full meaning only after this
		// file's own field and existing custody/mineral joins have admitted their
		// original rows (gated by capability, m2o28 Group 16). Preview changes
		// detached metadata alone, never the accepted world.
		if (!SlHasSaveCapability(SaveCapability::AsteroidCargoOrigins) && !NormalizeLegacyAsteroidCargoPreview(_load_check_data)) SlErrorCorrupt("Invalid copied legacy asteroid cargo origin");
		ValidateTBMProjectLoadPreview();
		ValidateSpoilReceiverLoadPreview();
		ValidateLayerMineLoadPreview();
		ValidateLayerProcessorLoadPreview();
		ValidateTBMExchangeLoadPreview();
		ValidateUtilityPowerLoadPreview();
		ValidateTubePowerLoadPreview();
		ValidateTubePowerVehicleLoadPreview();
		ValidateGeothermalSiteLoadPreview();
		ValidateGeothermalCollectorLoadPreview();
		ValidateGeothermalRiserLoadPreview();
		ValidateGeothermalHeatRouteLoadPreview();
		ValidateGeothermalGeneratorLoadPreview();
		ValidateTubeGeothermalPowerLoadPreview();
		// New-version previews require every bounded geology family, even for explicit inactive Classic/legacy terrain.
		if (!IsSavegameVersionBefore(SaveLoadVersion::GeologyExcavation)) {
			if (!_load_check_data.geology_header_present || !_load_check_data.geology_deposits_present || !_load_check_data.geology_spans_present) SlErrorCorrupt("Missing required geology preview chunks");
		}
		if (_load_check_data.geology_header_present) {
			const bool vents_required = (_load_check_data.multilayer_feature_flags & MULTILAYER_FEATURE_VENTS) != 0;
			// Explicit natural-source metadata requires a real bounded family; older geology-only absence stays valid.
			if (vents_required && !_load_check_data.geology_vents_present) SlErrorCorrupt("Missing required geothermal preview fields");
			if (vents_required != (_load_check_data.geology_vent_rows != 0)) SlErrorCorrupt("Geothermal preview feature mismatch");
			// Header/row cardinalities and feature identity must agree before a preview reports a loadable geological world.
			if (_load_check_data.geology_deposit_rows != _load_check_data.geology_expected_deposits || _load_check_data.geology_span_rows != _load_check_data.geology_expected_spans) SlErrorCorrupt("Inconsistent geology preview records");
			const bool required = (_load_check_data.multilayer_feature_flags & MULTILAYER_FEATURE_GEOLOGY) != 0;
			if (required != (_load_check_data.geology_generator_version != 0)) SlErrorCorrupt("Geology preview feature mismatch");
		}
		// New-format previews require metadata and exact profile content; legacy absence retains existing preview rules.
		if (!IsSavegameVersionBefore(SaveLoadVersion::MultiLayerMetadata)) {
			if (_load_check_data.multilayer_schema_version == 0) SlErrorCorrupt("Missing required multilayer metadata chunk");
		}
		const auto *profile = GetMultiLayerContentProfile(static_cast<MultiLayerGameplayProfileID>(_load_check_data.gameplay_profile_id));
		if (profile == nullptr) SlErrorCorrupt("Unsupported multilayer preview profile");
		if (profile->required_content != nullptr) {
			const auto package_error = CheckMultiLayerContentPackageIdentity(*profile->required_content, _load_check_data.grfconfig, false);
			if (package_error == MultiLayerContentIdentityError::MissingPackage) SlErrorCorrupt("Required Multilayer content package is missing");
			if (package_error == MultiLayerContentIdentityError::InactivePackage) SlErrorCorrupt("Required Multilayer content package is missing");
			if (package_error != MultiLayerContentIdentityError::None) SlErrorCorrupt("Required Multilayer content package differs from its saved identity/version");
		}
	} else {
		_gamelog.StartAction(GamelogActionType::Load);

		/* After loading fix up savegame for any internal changes that
		 * might have occurred since then. If it fails, load back the old game. */
		if (!AfterLoadGame()) {
			_gamelog.StopAction();
			return SaveLoadResult::ReInit;
		}

		_gamelog.StopAction();
	}

	return SaveLoadResult::Ok;
}

/**
 * Load the game using a (reader) filter.
 * @param reader   The filter to read the savegame from.
 * @param origin Optional explicit legacy interpretation bound to the entire original byte stream.
 * @param load_check Parse the copied preview through the same network/filter boundary without initializing the live world.
 * @return Ok on success, Error for nonmutating preflight/copied refusal, or ReInit for a later native full-load failure.
 */
SaveLoadResult LoadWithFilter(std::shared_ptr<LoadFilter> reader, const std::optional<SaveOriginHint> &origin, bool load_check)
{
	try {
		_sl.action = load_check ? SaveLoadAction::LoadCheck : SaveLoadAction::Load;
		return DoLoad(std::move(reader), load_check, origin);
	} catch (...) {
		if (load_check) DiscardFailedLoadCheckData();
		ClearSaveLoadState();
		return load_check ? SaveLoadResult::Error : SaveLoadResult::ReInit;
	}
}

/**
 * Main Save or Load function where the high-level saveload functions are
 * handled. It opens the savegame, selects format and checks versions
 * @param filename The name of the savegame being created/loaded
 * @param fop Save or load mode. Load can also be a TTD(Patch) game.
 * @param dft The type of file to save or load.
 * @param sb The sub directory to save the savegame in
 * @param threaded True when threaded saving is allowed
 * @param origin Optional exact-original legacy origin, ignored for saving.
 * @return Return the result of the action. #SaveLoadResult::Ok, #SaveLoadResult::Error, or #SaveLoadResult::ReInit ("unload" the game)
 */
SaveLoadResult SaveOrLoad(std::string_view filename, SaveLoadOperation fop, DetailedFileType dft, Subdirectory sb, bool threaded, const std::optional<SaveOriginHint> &origin)
{
	/* An instance of saving is already active, so don't go saving again */
	if (_sl.saveinprogress && fop == SaveLoadOperation::Save && dft == DetailedFileType::GameFile && threaded) {
		/* if not an autosave, but a user action, show error message */
		if (!_do_autosave) ShowErrorMessage(GetEncodedString(STR_ERROR_SAVE_STILL_IN_PROGRESS), {}, WarningLevel::Error);
		return SaveLoadResult::Ok;
	}
	WaitTillSaved();

	try {
		/* Load a TTDLX or TTDPatch game */
		if (fop == SaveLoadOperation::Load && dft == DetailedFileType::OldGameFile) {
			ResetSaveloadData();

			InitializeGame(256, 256, true, true); // set a mapsize of 256x256 for TTDPatch games or it might get confused

			/* TTD/TTO savegames have no NewGRFs, TTDP savegame have them
			 * and if so a new NewGRF list will be made in LoadOldSaveGame.
			 * Note: this is done here because AfterLoadGame is also called
			 * for OTTD savegames which have their own NewGRF logic. */
			ClearGRFConfigList(_grfconfig);
			_gamelog.Reset();
			if (!LoadOldSaveGame(filename)) return SaveLoadResult::ReInit;
			_sl_version = SaveLoadVersion::MinVersion;
			_sl_minor_version = 0;
			_gamelog.StartAction(GamelogActionType::Load);
			if (!AfterLoadGame()) {
				_gamelog.StopAction();
				return SaveLoadResult::ReInit;
			}
			_gamelog.StopAction();
			return SaveLoadResult::Ok;
		}

		assert(dft == DetailedFileType::GameFile);
		switch (fop) {
			case SaveLoadOperation::Check:
				_sl.action = SaveLoadAction::LoadCheck;
				break;

			case SaveLoadOperation::Load:
				_sl.action = SaveLoadAction::Load;
				break;

			case SaveLoadOperation::Save:
				_sl.action = SaveLoadAction::Save;
				break;

			default: NOT_REACHED();
		}

		auto fh = (fop == SaveLoadOperation::Save) ? FioFOpenFile(filename, "wb", sb) : FioFOpenFile(filename, "rb", sb);

		/* Make it a little easier to load savegames from the console */
		if (!fh.has_value() && fop != SaveLoadOperation::Save) fh = FioFOpenFile(filename, "rb", Subdirectory::Save);
		if (!fh.has_value() && fop != SaveLoadOperation::Save) fh = FioFOpenFile(filename, "rb", Subdirectory::Base);
		if (!fh.has_value() && fop != SaveLoadOperation::Save) fh = FioFOpenFile(filename, "rb", Subdirectory::Scenario);

		if (!fh.has_value()) {
			// An unreadable input has not initialized or partially loaded a world;
			// report Error without invoking native pointer cleanup or requesting ReInit.
			if (fop != SaveLoadOperation::Save) return ReportSavePreflightRefusal(fop == SaveLoadOperation::Check, STR_GAME_SAVELOAD_ERROR_FILE_NOT_READABLE);
			SlError(STR_GAME_SAVELOAD_ERROR_FILE_NOT_WRITEABLE);
		}

		if (fop == SaveLoadOperation::Save) { // SAVE game
			Debug(Facility::Desync, Severity::Error, "save: {:08x}; {:02x}; {}", TimerGameEconomy::date, TimerGameEconomy::date_fract, filename);
			if (!_settings_client.gui.threaded_saves) threaded = false;

			return DoSave(std::make_shared<FileWriter>(std::move(*fh)), threaded);
		}

		/* LOAD game */
		assert(fop == SaveLoadOperation::Load || fop == SaveLoadOperation::Check);
		Debug(Facility::Desync, Severity::Error, "load: {}", filename);
		return DoLoad(CreateSaveFileReader(std::move(*fh)), fop == SaveLoadOperation::Check, origin);
	} catch (...) {
		/* This code may be executed both for old and new save games. */
		if (fop == SaveLoadOperation::Check) DiscardFailedLoadCheckData();
		ClearSaveLoadState();

		if (fop != SaveLoadOperation::Check) Debug(Facility::Sl, Severity::Critical, "{} {}", GetSaveLoadErrorType().GetDecodedString(), GetSaveLoadErrorMessage().GetDecodedString());

		/* A saver/loader exception!! reinitialize all variables to prevent crash! */
		return (fop == SaveLoadOperation::Load) ? SaveLoadResult::ReInit : SaveLoadResult::Error;
	}
}

/**
 * Create an autosave or netsave.
 * @param counter A reference to the counter variable to be used for rotating the file name.
 */
void DoAutoOrNetsave(FiosNumberedSaveName &counter)
{
	std::string filename;

	if (_settings_client.gui.keep_all_autosave) {
		filename = GenerateDefaultSaveName() + counter.Extension();
	} else {
		filename = counter.Filename();
	}

	Debug(Facility::Sl, Severity::Warning, "Autosaving to '{}'", filename);
	if (SaveOrLoad(filename, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::Autosave) != SaveLoadResult::Ok) {
		ShowErrorMessage(GetEncodedString(STR_ERROR_AUTOSAVE_FAILED), {}, WarningLevel::Error);
	}
}


/** Do a save when exiting the game (_settings_client.gui.autosave_on_exit) */
void DoExitSave()
{
	SaveOrLoad("exit.sav", SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::Autosave);
}

/**
 * Get the default name for a savegame *or* screenshot.
 * @return The default name, i.e. company name and (play) time.
 */
std::string GenerateDefaultSaveName()
{
	/* Check if we have a name for this map, which is the name of the first
	 * available company. When there's no company available we'll use
	 * 'Spectator' as "company" name. */
	CompanyID cid = _local_company;
	if (!Company::IsValidID(cid)) {
		for (const Company *c : Company::Iterate()) {
			cid = c->index;
			break;
		}
	}

	std::array<StringParameter, 4> params{};
	auto it = params.begin();
	*it++ = cid;

	/* We show the current game time differently depending on the timekeeping units used by this game. */
	if (TimerGameEconomy::UsingWallclockUnits()) {
		/* Insert time played. */
		const auto play_time = TimerGameTick::counter / Ticks::TICKS_PER_SECOND;
		*it++ = STR_SAVEGAME_DURATION_REALTIME;
		*it++ = play_time / 60 / 60;
		*it++ = (play_time / 60) % 60;
	} else {
		/* Insert current date */
		switch (_settings_client.gui.date_format_in_default_names) {
			case 0: *it++ = STR_JUST_DATE_LONG; break;
			case 1: *it++ = STR_JUST_DATE_TINY; break;
			case 2: *it++ = STR_JUST_DATE_ISO; break;
			default: NOT_REACHED();
		}
		*it++ = TimerGameEconomy::date;
	}

	/* Get the correct string (special string for when there's not company) */
	std::string filename = GetStringWithArgs(!Company::IsValidID(cid) ? STR_SAVEGAME_NAME_SPECTATOR : STR_SAVEGAME_NAME_DEFAULT, params);
	SanitizeFilename(filename);
	return filename;
}

/**
 * Set the mode and file type of the file to save or load.
 * @param ft File type.
 * @param fop File operation being performed.
 */
void FileToSaveLoad::SetMode(const FiosType &ft, SaveLoadOperation fop)
{
	if (ft.abstract == AbstractFileType::Invalid || ft.abstract == AbstractFileType::None) {
		this->file_op = SaveLoadOperation::Invalid;
		this->ftype = FIOS_TYPE_INVALID;
		return;
	}

	this->file_op = fop;
	this->ftype = ft;
}

/**
 * Set the mode, title and name of the file.
 * @param item Container with mode, title and name.
 */
void FileToSaveLoad::Set(const FiosItem &item)
{
	this->SetMode(item.type);
	this->name = item.name;
	this->title = item.title;
}

SaveLoadTable SaveLoadHandler::GetLoadDescription() const
{
	assert(this->load_description.has_value());
	return *this->load_description;
}
