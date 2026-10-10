/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file layer_mine_sl.cpp Bounded mine binding transport; geology and physical output retain their own chunks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 16
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../multilayer_industry.h"
#include "../multilayer_state.h"
#include "../fios.h"

#include "../safeguards.h"

/** Scalar transport; none of these fields is a second reserve or output inventory. */
struct LayerMineWire {
	uint16_t industry = IndustryID::Invalid().base(); ///< Actual industry identity.
	uint32_t deposit = UINT32_MAX; ///< Existing geology identity.
	uint8_t company = CompanyID::Invalid().base(); ///< Saved claimant.
	uint8_t phase = 255; ///< Known lifecycle value.
	uint64_t work_ticks = 0; ///< Bounded banked work.
	uint64_t extracted = 0; ///< Historical reserve debit.
	uint64_t dispatched = 0; ///< Historical station receipt.
	uint64_t revision = 0; ///< Saved change stamp.
	uint32_t work_fraction = 0; ///< Rational work remainder.
	uint16_t scale_fraction = 0; ///< Rate remainder modulo 200.
};

static const SaveLoad _layer_mine_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("industry", SLE_OBJECT_ADDRESS(LayerMineWire, industry)),
	SaveLoad::Variable<VarFileType::U32>("deposit", SLE_OBJECT_ADDRESS(LayerMineWire, deposit)),
	SaveLoad::Variable<VarFileType::U8>("company", SLE_OBJECT_ADDRESS(LayerMineWire, company)),
	SaveLoad::Variable<VarFileType::U8>("phase", SLE_OBJECT_ADDRESS(LayerMineWire, phase)),
	SaveLoad::Variable<VarFileType::U64>("work_ticks", SLE_OBJECT_ADDRESS(LayerMineWire, work_ticks)),
	SaveLoad::Variable<VarFileType::U64>("extracted", SLE_OBJECT_ADDRESS(LayerMineWire, extracted)),
	SaveLoad::Variable<VarFileType::U64>("dispatched", SLE_OBJECT_ADDRESS(LayerMineWire, dispatched)),
	SaveLoad::Variable<VarFileType::U64>("revision", SLE_OBJECT_ADDRESS(LayerMineWire, revision)),
	SaveLoad::Variable<VarFileType::U32>("work_fraction", SLE_OBJECT_ADDRESS(LayerMineWire, work_fraction)),
	SaveLoad::Variable<VarFileType::U16>("scale_fraction", SLE_OBJECT_ADDRESS(LayerMineWire, scale_fraction)),
};

static std::array<bool, 2> _layer_mine_presence{}; ///< Unpublished header/table presence.
static uint32_t _layer_mine_expected = 0; ///< Exact count declared by the header.
static std::vector<LayerMineState> _layer_mine_rows; ///< Parsed transport, never live bindings.

/** @copydoc ResetLayerMineLoadStaging */
void ResetLayerMineLoadStaging()
{
	_layer_mine_presence.fill(false);
	_layer_mine_expected = 0;
	_layer_mine_rows.clear();
}

/**
 * Require a complete header/table and agreement with the explicit metadata feature flag.
 * @param preview Read independent preview counts rather than loader transport.
 * @return Whether the family exists; only an older save can legitimately omit it.
 */
static bool CheckLayerMineFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.layer_mine_present : _layer_mine_presence;
	const auto expected = preview ? _load_check_data.layer_mine_expected : _layer_mine_expected;
	const auto count = preview ? _load_check_data.layer_mine_rows : _layer_mine_rows.size();
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : (metadata == nullptr ? 0 : metadata->feature_flags);
	const bool required = (flags & MULTILAYER_FEATURE_MINES) != 0;
	if (!presence[0]) {
		// Older absence grants no binding; a new version or stray rows/feature request must refuse.
		if (!IsSavegameVersionBefore(SaveLoadVersion::LayerMines) || presence[1] || required) SlErrorCorrupt("Missing mine header");
		return false;
	}
	// Every declared row must exist exactly once, and nonempty state must advertise its delivered descriptor family.
	if (!presence[1] || expected != count || required != (count != 0)) SlErrorCorrupt("Incomplete mine family");
	return true;
}

/** @copydoc ValidateLayerMineLoadPreview */
void ValidateLayerMineLoadPreview()
{
	CheckLayerMineFamily(true);
}

/** Version/count header stages a complete family only after all chunks have been parsed. */
struct LMNHChunkHandler : ChunkHandler {
	/** Register the fixed mine header. */
	LMNHChunkHandler() : ChunkHandler("LMNH", ChunkType::Riff) {}
	/** Save the owner count without allocating claims or changing reserve. */
	void Save() const override
	{
		std::array<uint32_t, 2> header{1, static_cast<uint32_t>(CaptureLayerMines().size())};
		SlSetLength(8);
		SlCopy<VarFileType::U32>(header);
	}
	/** @param preview Copy structural bounds into preview state rather than loader staging. */
	void Read(bool preview) const
	{
		// A versioned header has exactly two uint32 fields and cannot appear in an older format.
		if (IsSavegameVersionBefore(SaveLoadVersion::LayerMines) || SlGetFieldLength() != 8) SlErrorCorrupt("Invalid mine header version/length");
		std::array<uint32_t, 2> header;
		SlCopy<VarFileType::U32>(header);
		if (header[0] != 1 || header[1] > MAX_GEOLOGY_DEPOSITS) SlErrorCorrupt("Invalid mine header bounds"); // One bounded binding per actual geological identity.
		auto &presence = preview ? _load_check_data.layer_mine_present : _layer_mine_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate mine header");
		presence[0] = true;
		(preview ? _load_check_data.layer_mine_expected : _layer_mine_expected) = header[1];
	}
	/** Parse unpublished full-load header. */
	void Load() const override { this->Read(false); }
	/** @param len The fixed reader validates exact payload length. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Stage complete transport; actual industry/geology validation follows their final rebuild. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!CheckLayerMineFamily(false)) return;
		StageLoadedLayerMines(_layer_mine_rows);
	}
};

/** Mandatory, ordered scalar rows with no inventory padding or transient dispatch tickets. */
struct LMNRChunkHandler : ChunkHandler {
	/** Register mine rows. */
	LMNRChunkHandler() : ChunkHandler("LMNR", ChunkType::Table) {}
	/** Serialize saved owner fields in increasing actual industry order. */
	void Save() const override
	{
		SlTableHeader(_layer_mine_desc);
		uint32_t ordinal = 0;
		// The owner supplies canonical industry order; table ordinals are transport only.
		for (const auto &mine : CaptureLayerMines()) {
			LayerMineWire wire;
			wire.industry = mine.industry.base();
			wire.deposit = mine.deposit.base();
			wire.company = mine.company.base();
			wire.phase = to_underlying(mine.phase);
			wire.work_ticks = mine.work_ticks;
			wire.extracted = mine.extracted;
			wire.dispatched = mine.dispatched;
			wire.revision = mine.revision;
			wire.work_fraction = mine.work_fraction;
			wire.scale_fraction = mine.scale_fraction;
			SlSetArrayIndex(ordinal++);
			SlObject(&wire, _layer_mine_desc);
		}
	}
	/** @param preview Check independent row counts without publishing or staging live bindings. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::LayerMines)) SlErrorCorrupt("Unexpected mine rows");
		const auto description = SlTableHeader(_layer_mine_desc);
		if (description.size() != std::size(_layer_mine_desc)) SlErrorCorrupt("Invalid mine field count");
		// Missing fields cannot default to renewed work, a released claim or forgotten reserve debit.
		for (const auto &field : _layer_mine_desc) {
			if (std::ranges::find(description, field.name, &SaveLoad::name) == description.end()) SlErrorCorrupt("Missing mine field");
		}
		uint32_t count = 0;
		uint16_t previous = 0;
		// Bound transport before allocating each row; runtime meaning and conservation are checked after industry rebuild.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_GEOLOGY_DEPOSITS || index != static_cast<int>(count)) SlErrorCorrupt("Invalid mine ordinal/count"); // No sparse or unbounded transport allocations.
			LayerMineWire wire;
			SlObject(&wire, description);
			// Known identities/phase, strict order, nonzero revisions and rate bounds prevent unsafe default activation.
			if (wire.industry == IndustryID::Invalid().base() || wire.deposit == UINT32_MAX ||
					(count != 0 && wire.industry <= previous) || wire.phase > to_underlying(LayerMinePhase::Abandoned) ||
					wire.revision == 0 || wire.scale_fraction >= 200 || wire.dispatched > wire.extracted) SlErrorCorrupt("Invalid mine fields");
			previous = wire.industry;
			if (!preview) {
				LayerMineState mine;
				mine.industry = IndustryID{wire.industry};
				mine.deposit = GeologyDepositID{wire.deposit};
				mine.company = CompanyID{wire.company};
				mine.phase = static_cast<LayerMinePhase>(wire.phase);
				mine.work_ticks = wire.work_ticks;
				mine.extracted = wire.extracted;
				mine.dispatched = wire.dispatched;
				mine.revision = wire.revision;
				mine.work_fraction = wire.work_fraction;
				mine.scale_fraction = wire.scale_fraction;
				_layer_mine_rows.push_back(mine);
			}
			++count;
		}
		auto &presence = preview ? _load_check_data.layer_mine_present : _layer_mine_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate mine rows");
		presence[1] = true;
		if (preview) _load_check_data.layer_mine_rows = count;
	}
	/** Parse unpublished binding rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns row boundaries. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const LMNHChunkHandler LMNH;
static const LMNRChunkHandler LMNR;
static const ChunkHandlerRef layer_mine_handlers[] = {LMNH, LMNR};
extern const ChunkHandlerTable _layer_mine_chunk_handlers(layer_mine_handlers);
