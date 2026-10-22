/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange_sl.cpp Bounded transport of hauler assignments and wagon sets (Plan 5 Group 10). Rows are
 * structurally checked here and staged; they are validated against the rebuilt projects and vehicles only at the end
 * of AfterLoadGame. Wagon packets stay in the vehicles' own cargo lists; a set saves wagon identities only. The
 * exchange is one simulation action, so a save always sees the store before or after it, never a half swap.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 10
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-h-exchange-design.md
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../tbm_exchange.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../map_func.h"

#include "../safeguards.h"

/** Units of one set: a set is taken from one train, at most 64 tiles of eighth-tile units. */
static constexpr uint32_t MAX_TBM_SET_WAGONS = 64 * 8;
/** Bound of saved set wagon rows. */
static constexpr uint32_t MAX_TBM_SET_WAGON_ROWS = static_cast<uint32_t>(MAX_TBM_WAGON_SETS) * MAX_TBM_SET_WAGONS;

/** Fixed assignment row. */
struct TBMAssignmentWire {
	uint32_t id = UINT32_MAX; ///< Identity, also the accepted request order.
	uint32_t project = UINT32_MAX; ///< Served project.
	uint32_t hauler = VehicleID::Invalid().base(); ///< Hauler front engine.
	uint32_t bay = UINT32_MAX; ///< Queued bay tile.
	uint32_t set = UINT32_MAX; ///< Carried set, or invalid.
	uint8_t phase = 255; ///< Cycle phase.
	uint8_t wait = 255; ///< Hold.
	uint64_t phase_ticks = 0; ///< Ticks in the phase.
	uint32_t exchanges = 0; ///< Completed exchanges.
	uint64_t revision = 0; ///< Change stamp.
};

static const SaveLoad _tbm_assignment_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("id", SLE_OBJECT_ADDRESS(TBMAssignmentWire, id)),
	SaveLoad::Variable<VarFileType::U32>("project", SLE_OBJECT_ADDRESS(TBMAssignmentWire, project)),
	SaveLoad::Variable<VarFileType::U32>("hauler", SLE_OBJECT_ADDRESS(TBMAssignmentWire, hauler)),
	SaveLoad::Variable<VarFileType::U32>("bay", SLE_OBJECT_ADDRESS(TBMAssignmentWire, bay)),
	SaveLoad::Variable<VarFileType::U32>("set", SLE_OBJECT_ADDRESS(TBMAssignmentWire, set)),
	SaveLoad::Variable<VarFileType::U8>("phase", SLE_OBJECT_ADDRESS(TBMAssignmentWire, phase)),
	SaveLoad::Variable<VarFileType::U8>("wait", SLE_OBJECT_ADDRESS(TBMAssignmentWire, wait)),
	SaveLoad::Variable<VarFileType::U64>("phase_ticks", SLE_OBJECT_ADDRESS(TBMAssignmentWire, phase_ticks)),
	SaveLoad::Variable<VarFileType::U32>("exchanges", SLE_OBJECT_ADDRESS(TBMAssignmentWire, exchanges)),
	SaveLoad::Variable<VarFileType::U64>("revision", SLE_OBJECT_ADDRESS(TBMAssignmentWire, revision)),
};

/** Fixed set row; its wagons follow in the wagon table in set order. */
struct TBMSetWire {
	uint32_t id = UINT32_MAX; ///< Identity.
	uint32_t project = UINT32_MAX; ///< Served project.
	uint8_t state = 255; ///< Loop position.
	uint8_t visited = 255; ///< Origin visited on the current loaded cycle.
	uint64_t revision = 0; ///< Change stamp.
	uint32_t wagons = 0; ///< Wagon row count.
};

static const SaveLoad _tbm_set_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("id", SLE_OBJECT_ADDRESS(TBMSetWire, id)),
	SaveLoad::Variable<VarFileType::U32>("project", SLE_OBJECT_ADDRESS(TBMSetWire, project)),
	SaveLoad::Variable<VarFileType::U8>("state", SLE_OBJECT_ADDRESS(TBMSetWire, state)),
	SaveLoad::Variable<VarFileType::U8>("visited", SLE_OBJECT_ADDRESS(TBMSetWire, visited)),
	SaveLoad::Variable<VarFileType::U64>("revision", SLE_OBJECT_ADDRESS(TBMSetWire, revision)),
	SaveLoad::Variable<VarFileType::U32>("wagons", SLE_OBJECT_ADDRESS(TBMSetWire, wagons)),
};

/** One set wagon, in set order and then chain order. */
struct TBMSetWagonWire {
	uint32_t set = UINT32_MAX; ///< Owning set.
	uint32_t wagon = VehicleID::Invalid().base(); ///< Unit head.
};

static const SaveLoad _tbm_set_wagon_desc[] = {
	SaveLoad::Variable<VarFileType::U32>("set", SLE_OBJECT_ADDRESS(TBMSetWagonWire, set)),
	SaveLoad::Variable<VarFileType::U32>("wagon", SLE_OBJECT_ADDRESS(TBMSetWagonWire, wagon)),
};

static std::array<bool, 4> _tbm_exchange_presence{}; ///< Unpublished header, assignment, set and wagon table presence.
static std::array<uint32_t, 5> _tbm_exchange_header{}; ///< Assignment count and cursor, set count and cursor, wagon rows.
static std::vector<TBMExchangeAssignment> _tbm_assignment_rows; ///< Unpublished assignments.
static std::vector<TBMWagonSet> _tbm_set_rows; ///< Unpublished sets, wagons attached at staging.
static std::vector<uint32_t> _tbm_set_wagon_counts; ///< Saved wagon count per set row.
static std::vector<TBMSetWagonWire> _tbm_set_wagon_rows; ///< Unpublished wagon rows.

void ResetTBMExchangeLoadStaging()
{
	_tbm_exchange_presence.fill(false);
	_tbm_exchange_header.fill(0);
	_tbm_assignment_rows.clear();
	_tbm_set_rows.clear();
	_tbm_set_wagon_counts.clear();
	_tbm_set_wagon_rows.clear();
}

/**
 * Check one table's field list against its descriptor.
 * @param description Loaded table header.
 * @param desc Expected descriptor.
 * @param what Name for the corruption message.
 */
static void CheckTBMExchangeFields(const std::vector<SaveLoad> &description, std::span<const SaveLoad> desc, const char *what)
{
	if (description.size() != desc.size()) SlErrorCorruptFmt("Invalid {} field count", what);
	// A missing field cannot default to an invented hauler, set, phase or wagon.
	for (const auto &field : desc) {
		if (std::ranges::find(description, field.name, &SaveLoad::name) == description.end()) SlErrorCorruptFmt("Missing {} field", what);
	}
}

/**
 * Reconcile header, tables and feature flag for load or preview.
 * @param preview Use the copied preview counts.
 * @return Whether a complete family exists; legitimate absence in older saves returns false.
 */
static bool CheckTBMExchangeFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.tbm_exchange_present : _tbm_exchange_presence;
	const auto &header = preview ? _load_check_data.tbm_exchange_header : _tbm_exchange_header;
	const auto &rows = preview ? _load_check_data.tbm_exchange_rows : std::array<uint32_t, 3>{static_cast<uint32_t>(_tbm_assignment_rows.size()), static_cast<uint32_t>(_tbm_set_rows.size()), static_cast<uint32_t>(_tbm_set_wagon_rows.size())};
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : (metadata == nullptr ? 0 : metadata->feature_flags);
	const bool required = (flags & MULTILAYER_FEATURE_TBM_EXCHANGE) != 0;
	if (!presence[0]) {
		// New saves always write the header; only older versions may lack the family, and then nothing may claim it.
		if (!IsSavegameVersionBefore(SaveLoadVersion::TBMExchange) || presence[1] || presence[2] || presence[3] || required) SlErrorCorrupt("Missing TBM exchange header");
		return false;
	}
	// Every table matches its header count, and the flag matches whether any identity was ever allocated.
	if (!presence[1] || !presence[2] || !presence[3] || rows[0] != header[0] || rows[1] != header[2] || rows[2] != header[4] || required != (header[1] != 0 || header[3] != 0)) SlErrorCorrupt("Incomplete TBM exchange family");
	return true;
}

void ValidateTBMExchangeLoadPreview()
{
	CheckTBMExchangeFamily(true);
}

/** Header: format revision, row counts and both cursors; it also stages the store once all chunks exist. */
struct TBXHChunkHandler : ChunkHandler {
	/** Register the twenty-four-byte header. */
	TBXHChunkHandler() : ChunkHandler("TBXH", ChunkType::Riff) {}
	/** Save the format revision, the live counts and both cursors. */
	void Save() const override
	{
		uint32_t wagons = 0;
		for (const auto &set : CaptureTBMWagonSets()) wagons += static_cast<uint32_t>(set.wagons.size());
		std::array<uint32_t, 6> header{1, static_cast<uint32_t>(CaptureTBMExchangeAssignments().size()), GetNextTBMExchangeAssignmentID(), static_cast<uint32_t>(CaptureTBMWagonSets().size()), GetNextTBMWagonSetID(), wagons};
		SlSetLength(24);
		SlCopy<VarFileType::U32>(header);
	}
	/** @param preview Record copied bounds for the preview instead of the loader. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::TBMExchange) || SlGetFieldLength() != 24) SlErrorCorrupt("Invalid TBM exchange header version/length");
		std::array<uint32_t, 6> header;
		SlCopy<VarFileType::U32>(header);
		// Unknown formats, oversized stores and cursors that cannot cover their rows refuse first.
		if (header[0] != 1 || header[1] > MAX_TBM_EXCHANGE_ASSIGNMENTS || header[3] > MAX_TBM_WAGON_SETS || header[5] > MAX_TBM_SET_WAGON_ROWS || header[1] > header[2] || header[3] > header[4]) SlErrorCorrupt("Invalid TBM exchange header bounds");
		auto &presence = preview ? _load_check_data.tbm_exchange_present : _tbm_exchange_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate TBM exchange header");
		presence[0] = true;
		auto &target = preview ? _load_check_data.tbm_exchange_header : _tbm_exchange_header;
		for (size_t i = 0; i < target.size(); ++i) target[i] = header[i + 1];
	}
	/** Stage header bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader owns exact length checks. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Attach each set's wagons in order and stage the store; live validation waits for rebuilt vehicles. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!CheckTBMExchangeFamily(false)) return;
		size_t next = 0;
		// Wagon rows follow the set rows: each set takes exactly its saved count, and every row names that set.
		for (size_t i = 0; i < _tbm_set_rows.size(); ++i) {
			for (uint32_t k = 0; k < _tbm_set_wagon_counts[i]; ++k, ++next) {
				if (next >= _tbm_set_wagon_rows.size() || _tbm_set_wagon_rows[next].set != _tbm_set_rows[i].id.base()) SlErrorCorrupt("Invalid TBM set wagon order");
				_tbm_set_rows[i].wagons.push_back(VehicleID{_tbm_set_wagon_rows[next].wagon});
			}
		}
		if (next != _tbm_set_wagon_rows.size()) SlErrorCorrupt("Invalid TBM set wagon count");
		StageLoadedTBMExchange(_tbm_assignment_rows, _tbm_exchange_header[1], _tbm_set_rows, _tbm_exchange_header[3]);
	}
};

/** Assignment rows, bounded and structurally checked before any reference is resolved. */
struct TBXAChunkHandler : ChunkHandler {
	/** Register the assignment table. */
	TBXAChunkHandler() : ChunkHandler("TBXA", ChunkType::Table) {}
	/** Save each assignment once in identity order. */
	void Save() const override
	{
		SlTableHeader(_tbm_assignment_desc);
		uint32_t ordinal = 0;
		for (const auto &assignment : CaptureTBMExchangeAssignments()) {
			TBMAssignmentWire wire;
			wire.id = assignment.id.base();
			wire.project = assignment.project.base();
			wire.hauler = assignment.hauler.base();
			wire.bay = assignment.bay.base();
			wire.set = assignment.set.base();
			wire.phase = to_underlying(assignment.phase);
			wire.wait = to_underlying(assignment.wait);
			wire.phase_ticks = assignment.phase_ticks;
			wire.exchanges = assignment.exchanges;
			wire.revision = assignment.revision;
			SlSetArrayIndex(ordinal++);
			SlObject(&wire, _tbm_assignment_desc);
		}
	}
	/** @param preview Validate copied rows without staging them. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::TBMExchange)) SlErrorCorrupt("Unexpected TBM assignment rows");
		const auto description = SlTableHeader(_tbm_assignment_desc);
		CheckTBMExchangeFields(description, _tbm_assignment_desc, "TBM assignment");
		uint32_t count = 0;
		// Ordering against the cursor and the references are checked at acceptance; bounds and enums here.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_TBM_EXCHANGE_ASSIGNMENTS || index != static_cast<int>(count)) SlErrorCorrupt("Invalid TBM assignment ordinal/count");
			TBMAssignmentWire wire;
			SlObject(&wire, description);
			const uint64_t size = preview ? uint64_t{_load_check_data.map_size_x} * _load_check_data.map_size_y * _load_check_data.map_layers : Map::Size();
			if (wire.id == UINT32_MAX || wire.project == UINT32_MAX || wire.hauler == VehicleID::Invalid().base() || wire.bay >= size || wire.phase >= to_underlying(TBMHaulerPhase::End) || wire.wait >= to_underlying(TBMHaulerWait::End) || wire.revision == 0) SlErrorCorrupt("Invalid TBM assignment fields");
			if (!preview) {
				TBMExchangeAssignment assignment;
				assignment.id = TBMExchangeAssignmentID{wire.id};
				assignment.project = TBMProjectID{wire.project};
				assignment.hauler = VehicleID{wire.hauler};
				assignment.bay = TileIndex{wire.bay};
				assignment.set = TBMWagonSetID{wire.set};
				assignment.phase = static_cast<TBMHaulerPhase>(wire.phase);
				assignment.wait = static_cast<TBMHaulerWait>(wire.wait);
				assignment.phase_ticks = wire.phase_ticks;
				assignment.exchanges = wire.exchanges;
				assignment.revision = wire.revision;
				_tbm_assignment_rows.push_back(assignment);
			}
			++count;
		}
		auto &presence = preview ? _load_check_data.tbm_exchange_present : _tbm_exchange_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate TBM assignment rows");
		presence[1] = true;
		if (preview) _load_check_data.tbm_exchange_rows[0] = count;
	}
	/** Stage copied rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns row-boundary validation. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

/** Set rows; their wagons are attached from the wagon table when the family is staged. */
struct TBXSChunkHandler : ChunkHandler {
	/** Register the set table. */
	TBXSChunkHandler() : ChunkHandler("TBXS", ChunkType::Table) {}
	/** Save each set once in identity order. */
	void Save() const override
	{
		SlTableHeader(_tbm_set_desc);
		uint32_t ordinal = 0;
		for (const auto &set : CaptureTBMWagonSets()) {
			TBMSetWire wire;
			wire.id = set.id.base();
			wire.project = set.project.base();
			wire.state = to_underlying(set.state);
			wire.visited = set.visited_origin ? 1 : 0;
			wire.revision = set.revision;
			wire.wagons = static_cast<uint32_t>(set.wagons.size());
			SlSetArrayIndex(ordinal++);
			SlObject(&wire, _tbm_set_desc);
		}
	}
	/** @param preview Validate copied rows without staging them. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::TBMExchange)) SlErrorCorrupt("Unexpected TBM set rows");
		const auto description = SlTableHeader(_tbm_set_desc);
		CheckTBMExchangeFields(description, _tbm_set_desc, "TBM set");
		uint32_t count = 0;
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_TBM_WAGON_SETS || index != static_cast<int>(count)) SlErrorCorrupt("Invalid TBM set ordinal/count");
			TBMSetWire wire;
			SlObject(&wire, description);
			// Known state, a boolean visit, a stamp, and a non-empty bounded wagon list.
			if (wire.id == UINT32_MAX || wire.project == UINT32_MAX || wire.state >= to_underlying(TBMWagonSetState::End) || wire.visited > 1 || wire.revision == 0 || wire.wagons == 0 || wire.wagons > MAX_TBM_SET_WAGONS) SlErrorCorrupt("Invalid TBM set fields");
			if (!preview) {
				TBMWagonSet set;
				set.id = TBMWagonSetID{wire.id};
				set.project = TBMProjectID{wire.project};
				set.state = static_cast<TBMWagonSetState>(wire.state);
				set.visited_origin = wire.visited != 0;
				set.revision = wire.revision;
				_tbm_set_rows.push_back(std::move(set));
				_tbm_set_wagon_counts.push_back(wire.wagons);
			}
			++count;
		}
		auto &presence = preview ? _load_check_data.tbm_exchange_present : _tbm_exchange_presence;
		if (presence[2]) SlErrorCorrupt("Duplicate TBM set rows");
		presence[2] = true;
		if (preview) _load_check_data.tbm_exchange_rows[1] = count;
	}
	/** Stage copied rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns row-boundary validation. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

/** Set wagon rows in set order and chain order. */
struct TBXWChunkHandler : ChunkHandler {
	/** Register the wagon table. */
	TBXWChunkHandler() : ChunkHandler("TBXW", ChunkType::Table) {}
	/** Save every set's wagons in set order, each in chain order. */
	void Save() const override
	{
		SlTableHeader(_tbm_set_wagon_desc);
		uint32_t ordinal = 0;
		for (const auto &set : CaptureTBMWagonSets()) {
			for (VehicleID wagon : set.wagons) {
				TBMSetWagonWire wire{set.id.base(), wagon.base()};
				SlSetArrayIndex(ordinal++);
				SlObject(&wire, _tbm_set_wagon_desc);
			}
		}
	}
	/** @param preview Validate copied rows without staging them. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::TBMExchange)) SlErrorCorrupt("Unexpected TBM set wagon rows");
		const auto description = SlTableHeader(_tbm_set_wagon_desc);
		CheckTBMExchangeFields(description, _tbm_set_wagon_desc, "TBM set wagon");
		uint32_t count = 0;
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_TBM_SET_WAGON_ROWS || index != static_cast<int>(count)) SlErrorCorrupt("Invalid TBM set wagon ordinal/count");
			TBMSetWagonWire wire;
			SlObject(&wire, description);
			if (wire.set == UINT32_MAX || wire.wagon == VehicleID::Invalid().base()) SlErrorCorrupt("Invalid TBM set wagon fields");
			if (!preview) _tbm_set_wagon_rows.push_back(wire);
			++count;
		}
		auto &presence = preview ? _load_check_data.tbm_exchange_present : _tbm_exchange_presence;
		if (presence[3]) SlErrorCorrupt("Duplicate TBM set wagon rows");
		presence[3] = true;
		if (preview) _load_check_data.tbm_exchange_rows[2] = count;
	}
	/** Stage copied rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns row-boundary validation. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const TBXHChunkHandler TBXH;
static const TBXAChunkHandler TBXA;
static const TBXSChunkHandler TBXS;
static const TBXWChunkHandler TBXW;
static const ChunkHandlerRef tbm_exchange_handlers[] = {TBXH, TBXA, TBXS, TBXW};
extern const ChunkHandlerTable _tbm_exchange_chunk_handlers(tbm_exchange_handlers);
