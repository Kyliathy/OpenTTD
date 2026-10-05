/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file spoil_receiver_sl.cpp Bounded transport of the spoil receivers' side state (Plan 5 Group 8). Rows are
 * structurally checked here and staged; they are validated against the rebuilt industries, which own the spoil stock
 * itself, only at the end of AfterLoadGame.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 8
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-i-receiver-design.md, Persistence
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../spoil_receiver.h"
#include "../multilayer_state.h"
#include "../fios.h"

#include "../safeguards.h"

static constexpr uint32_t MAX_SPOIL_RECEIVER_ROWS = 64000; ///< One row per industry at most.

/** Fixed scalar receiver row. */
struct SpoilReceiverWire {
	uint16_t industry = IndustryID::Invalid().base(); ///< Receiver industry.
	uint8_t kind = 255; ///< Spoil kind.
	uint8_t mode = 255; ///< Processing choice.
	uint16_t interval_ticks = 0; ///< Ticks into the interval.
	uint16_t interval_intake = 0; ///< Tonnes admitted this interval.
	uint16_t aggregate = 0; ///< Aggregate held.
	uint64_t disposed = 0; ///< Tonnes disposed.
	uint64_t crushed = 0; ///< Tonnes crushed.
	uint64_t aggregate_made = 0; ///< Aggregate tonnes made.
	uint64_t residue = 0; ///< Residue tonnes recorded.
	uint64_t fees = 0; ///< Fees charged.
	uint64_t revision = 0; ///< Change stamp.
};

static const SaveLoad _spoil_receiver_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("industry", SLE_OBJECT_ADDRESS(SpoilReceiverWire, industry)),
	SaveLoad::Variable<VarFileType::U8>("kind", SLE_OBJECT_ADDRESS(SpoilReceiverWire, kind)),
	SaveLoad::Variable<VarFileType::U8>("mode", SLE_OBJECT_ADDRESS(SpoilReceiverWire, mode)),
	SaveLoad::Variable<VarFileType::U16>("interval_ticks", SLE_OBJECT_ADDRESS(SpoilReceiverWire, interval_ticks)),
	SaveLoad::Variable<VarFileType::U16>("interval_intake", SLE_OBJECT_ADDRESS(SpoilReceiverWire, interval_intake)),
	SaveLoad::Variable<VarFileType::U16>("aggregate", SLE_OBJECT_ADDRESS(SpoilReceiverWire, aggregate)),
	SaveLoad::Variable<VarFileType::U64>("disposed", SLE_OBJECT_ADDRESS(SpoilReceiverWire, disposed)),
	SaveLoad::Variable<VarFileType::U64>("crushed", SLE_OBJECT_ADDRESS(SpoilReceiverWire, crushed)),
	SaveLoad::Variable<VarFileType::U64>("aggregate_made", SLE_OBJECT_ADDRESS(SpoilReceiverWire, aggregate_made)),
	SaveLoad::Variable<VarFileType::U64>("residue", SLE_OBJECT_ADDRESS(SpoilReceiverWire, residue)),
	SaveLoad::Variable<VarFileType::U64>("fees", SLE_OBJECT_ADDRESS(SpoilReceiverWire, fees)),
	SaveLoad::Variable<VarFileType::U64>("revision", SLE_OBJECT_ADDRESS(SpoilReceiverWire, revision)),
};

static std::array<bool, 2> _spoil_receiver_presence{}; ///< Unpublished header and row table presence.
static uint32_t _spoil_receiver_expected = 0; ///< Exact row count from the header.
static std::vector<SpoilReceiver> _spoil_receiver_rows; ///< Unpublished structurally checked rows; never a second live store.

void ResetSpoilReceiverLoadStaging()
{
	_spoil_receiver_presence.fill(false);
	_spoil_receiver_expected = 0;
	_spoil_receiver_rows.clear();
}

/**
 * Reconcile header, table and feature flag for load or preview.
 * @param preview Use the copied preview counts.
 * @return Whether a complete family exists; legitimate absence in older saves returns false.
 */
static bool CheckSpoilReceiverFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.spoil_receiver_present : _spoil_receiver_presence;
	const size_t expected = preview ? _load_check_data.spoil_receiver_expected : _spoil_receiver_expected;
	const size_t count = preview ? _load_check_data.spoil_receiver_rows : _spoil_receiver_rows.size();
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : (metadata == nullptr ? 0 : metadata->feature_flags);
	const bool required = (flags & MULTILAYER_FEATURE_SPOIL_RECEIVERS) != 0;
	if (!presence[0]) {
		// New saves always write the header; only older versions may lack the family, and then nothing may claim it.
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpoilReceivers) || presence[1] || required) SlErrorCorrupt("Missing spoil receiver header");
		return false;
	}
	// The table must match the header count, and the feature flag must match whether any receiver exists.
	if (!presence[1] || expected != count || required != (count != 0)) SlErrorCorrupt("Incomplete spoil receiver family");
	return true;
}

void ValidateSpoilReceiverLoadPreview()
{
	CheckSpoilReceiverFamily(true);
}

/** Header: format revision and exact row count; it also stages the rows once all chunks exist. */
struct SPRHChunkHandler : ChunkHandler {
	/** Register the eight-byte header. */
	SPRHChunkHandler() : ChunkHandler("SPRH", ChunkType::Riff) {}
	/** Save the format revision and the live count. */
	void Save() const override
	{
		std::array<uint32_t, 2> header{1, static_cast<uint32_t>(CaptureSpoilReceivers().size())};
		SlSetLength(8);
		SlCopy<VarFileType::U32>(header);
	}
	/** @param preview Record copied bounds for the preview instead of the loader. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::SpoilReceivers) || SlGetFieldLength() != 8) SlErrorCorrupt("Invalid spoil receiver header version/length");
		std::array<uint32_t, 2> header;
		SlCopy<VarFileType::U32>(header);
		if (header[0] != 1 || header[1] > MAX_SPOIL_RECEIVER_ROWS) SlErrorCorrupt("Invalid spoil receiver header bounds");
		auto &presence = preview ? _load_check_data.spoil_receiver_present : _spoil_receiver_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate spoil receiver header");
		presence[0] = true;
		(preview ? _load_check_data.spoil_receiver_expected : _spoil_receiver_expected) = header[1];
	}
	/** Stage header bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader owns exact length checks. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Stage the complete family once every chunk is read; live validation waits for rebuilt industries. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!CheckSpoilReceiverFamily(false)) return;
		StageLoadedSpoilReceivers(_spoil_receiver_rows);
	}
};

/** Exact receiver rows, bounded and structurally checked before any industry is resolved. */
struct SPRRChunkHandler : ChunkHandler {
	/** Register the row table. */
	SPRRChunkHandler() : ChunkHandler("SPRR", ChunkType::Table) {}
	/** Save each receiver once in increasing industry order. */
	void Save() const override
	{
		SlTableHeader(_spoil_receiver_desc);
		uint32_t ordinal = 0;
		for (const auto &receiver : CaptureSpoilReceivers()) {
			SpoilReceiverWire wire;
			wire.industry = receiver.industry.base();
			wire.kind = to_underlying(receiver.kind);
			wire.mode = to_underlying(receiver.mode);
			wire.interval_ticks = receiver.interval_ticks;
			wire.interval_intake = receiver.interval_intake;
			wire.aggregate = receiver.aggregate;
			wire.disposed = receiver.disposed_t;
			wire.crushed = receiver.crushed_t;
			wire.aggregate_made = receiver.aggregate_t;
			wire.residue = receiver.residue_t;
			wire.fees = receiver.fees;
			wire.revision = receiver.revision;
			SlSetArrayIndex(ordinal++);
			SlObject(&wire, _spoil_receiver_desc);
		}
	}
	/** @param preview Validate copied rows without staging them. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::SpoilReceivers)) SlErrorCorrupt("Unexpected spoil receiver rows");
		const auto description = SlTableHeader(_spoil_receiver_desc);
		if (description.size() != std::size(_spoil_receiver_desc)) SlErrorCorrupt("Invalid spoil receiver field count");
		// A missing field cannot default to an empty interval, a zero total or an invented mode.
		for (const auto &field : _spoil_receiver_desc) {
			if (std::ranges::find(description, field.name, &SaveLoad::name) == description.end()) SlErrorCorrupt("Missing spoil receiver field");
		}
		uint32_t count = 0;
		uint16_t previous = 0;
		// Bounds, ordering and enums are checked before a row enters unpublished transport.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_SPOIL_RECEIVER_ROWS || index != static_cast<int>(count)) SlErrorCorrupt("Invalid spoil receiver ordinal/count");
			SpoilReceiverWire wire;
			SlObject(&wire, description);
			// Industries increase strictly; kind and mode are known values; the timer lies inside one interval.
			if (wire.industry == IndustryID::Invalid().base() || (count != 0 && wire.industry <= previous) || wire.kind >= to_underlying(SpoilReceiverKind::End) || wire.mode >= to_underlying(SpoilReceiverMode::End) || wire.interval_ticks >= SPOIL_RECEIVER_INTERVAL_TICKS || wire.revision == 0) SlErrorCorrupt("Invalid spoil receiver fields");
			previous = wire.industry;
			if (!preview) {
				SpoilReceiver receiver;
				receiver.industry = IndustryID{wire.industry};
				receiver.kind = static_cast<SpoilReceiverKind>(wire.kind);
				receiver.mode = static_cast<SpoilReceiverMode>(wire.mode);
				receiver.interval_ticks = wire.interval_ticks;
				receiver.interval_intake = wire.interval_intake;
				receiver.aggregate = wire.aggregate;
				receiver.disposed_t = wire.disposed;
				receiver.crushed_t = wire.crushed;
				receiver.aggregate_t = wire.aggregate_made;
				receiver.residue_t = wire.residue;
				receiver.fees = wire.fees;
				receiver.revision = wire.revision;
				_spoil_receiver_rows.push_back(receiver);
			}
			++count;
		}
		auto &presence = preview ? _load_check_data.spoil_receiver_present : _spoil_receiver_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate spoil receiver rows");
		presence[1] = true;
		if (preview) _load_check_data.spoil_receiver_rows = count;
	}
	/** Stage copied rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns row-boundary validation. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const SPRHChunkHandler SPRH;
static const SPRRChunkHandler SPRR;
static const ChunkHandlerRef spoil_receiver_handlers[] = {SPRH, SPRR};
extern const ChunkHandlerTable _spoil_receiver_chunk_handlers(spoil_receiver_handlers);
