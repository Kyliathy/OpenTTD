/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file layer_processor_sl.cpp Bounded recipe choice/work/counter transport; physical cargo stays in ordinary industry/packet chunks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 16B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 2 (GCC-portable header bounds)
 */

#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../layer_industry_processor.h"
#include "../multilayer_state.h"
#include "../fios.h"

#include "../safeguards.h"

static constexpr uint32_t MAX_PROCESSOR_ROWS = 64000; ///< Existing industry pool bound.
static constexpr uint32_t MAX_PROCESSOR_COUNTERS = MAX_PROCESSOR_ROWS * 48; ///< At most 16 recipe/input/output meanings per owner.

/** Fixed scalar transport independent of physical input/output inventories. */
struct LayerProcessorWire {
	uint16_t industry = IndustryID::Invalid().base(); ///< Actual inventory identity.
	uint8_t controller = CompanyID::Invalid().base(); ///< Choice controller, optionally abandoned.
	MultiLayerRecipeID recipe{}; ///< Explicit four-byte content alternative.
	uint64_t work_ticks = 0; ///< Bounded synchronized work.
	uint16_t scale_fraction = 0; ///< Rate numerator modulo 200.
	uint64_t revision = 0; ///< Saved change stamp.
	uint64_t dispatched = 0; ///< Actual historical packet supply.
};

/** One historical key, including unused zero keys; never a second physical store. */
struct LayerProcessorCounterWire {
	uint16_t industry = IndustryID::Invalid().base(); ///< Scalar row owner.
	uint8_t kind = 255; ///< Batch=0, consumption=1, production=2.
	CargoLabel label{}; ///< Canonical four bytes; kind determines recipe versus cargo meaning.
	uint64_t count = 0; ///< Historical complete batches or material units.
};

static const SaveLoad _processor_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("industry", SLE_OBJECT_ADDRESS(LayerProcessorWire, industry)),
	SaveLoad::Variable<VarFileType::U8>("controller", SLE_OBJECT_ADDRESS(LayerProcessorWire, controller)),
	SaveLoad::Variable<VarFileType::Label>("recipe", SLE_OBJECT_ADDRESS(LayerProcessorWire, recipe)),
	SaveLoad::Variable<VarFileType::U64>("work_ticks", SLE_OBJECT_ADDRESS(LayerProcessorWire, work_ticks)),
	SaveLoad::Variable<VarFileType::U16>("scale_fraction", SLE_OBJECT_ADDRESS(LayerProcessorWire, scale_fraction)),
	SaveLoad::Variable<VarFileType::U64>("revision", SLE_OBJECT_ADDRESS(LayerProcessorWire, revision)),
	SaveLoad::Variable<VarFileType::U64>("dispatched", SLE_OBJECT_ADDRESS(LayerProcessorWire, dispatched)),
};
static const SaveLoad _counter_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("industry", SLE_OBJECT_ADDRESS(LayerProcessorCounterWire, industry)),
	SaveLoad::Variable<VarFileType::U8>("kind", SLE_OBJECT_ADDRESS(LayerProcessorCounterWire, kind)),
	SaveLoad::Variable<VarFileType::Label>("label", SLE_OBJECT_ADDRESS(LayerProcessorCounterWire, label)),
	SaveLoad::Variable<VarFileType::U64>("count", SLE_OBJECT_ADDRESS(LayerProcessorCounterWire, count)),
};

static std::array<bool, 3> _processor_presence{}; ///< Unpublished header/scalar/counter presence.
static std::array<uint32_t, 2> _processor_expected{}; ///< Declared scalar/counter cardinalities.
static std::vector<LayerProcessorState> _processor_rows; ///< Unpublished parsed scalar owners.
static std::vector<LayerProcessorCounterWire> _processor_counters; ///< Unpublished parsed historical keys.

/** @copydoc ResetLayerProcessorLoadStaging */
void ResetLayerProcessorLoadStaging()
{
	_processor_presence.fill(false);
	_processor_expected.fill(0);
	_processor_rows.clear();
	_processor_counters.clear();
}

/**
 * Reconcile bounded transport and explicit metadata without constructing gameplay inventory.
 * @param preview Use independent copied preview counts.
 * @return Whether the complete family exists; only older saves can omit it.
 */
static bool CheckLayerProcessorFamily(bool preview)
{
	const auto &presence = preview ? _load_check_data.layer_processor_present : _processor_presence;
	const auto &expected = preview ? _load_check_data.layer_processor_expected : _processor_expected;
	const std::array<uint32_t, 2> actual = preview ? _load_check_data.layer_processor_rows :
		std::array{static_cast<uint32_t>(_processor_rows.size()), static_cast<uint32_t>(_processor_counters.size())};
	const auto *metadata = GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : (metadata == nullptr ? 0 : metadata->feature_flags);
	const bool required = (flags & MULTILAYER_FEATURE_PROCESSORS) != 0;
	if (!presence[0]) {
		// Older absence grants no default choices; stray tables or a current format must refuse missing mandatory transport.
		if (!IsSavegameVersionBefore(SaveLoadVersion::LayerProcessors) || presence[1] || presence[2] || required) SlErrorCorrupt("Missing processor header");
		return false;
	}
	// Both exact tables must agree with the header; a feature bit advertises live owners, not fabricated counter-only state.
	if (!presence[1] || !presence[2] || expected != actual || required != (actual[0] != 0)) SlErrorCorrupt("Incomplete processor family");
	return true;
}

/** @copydoc ValidateLayerProcessorLoadPreview */
void ValidateLayerProcessorLoadPreview() { CheckLayerProcessorFamily(true); }

/** Version/count header; final reference acceptance waits until actual industries and cargo meanings exist. */
struct LPCHChunkHandler : ChunkHandler {
	/** Register the twelve-byte header. */
	LPCHChunkHandler() : ChunkHandler("LPCH", ChunkType::Riff) {}
	/** Save exact scalar and historical key counts from the sole side-state owner. */
	void Save() const override
	{
		const auto rows = CaptureLayerProcessors();
		uint32_t counters = 0;
		// Count actual keys only; no per-owner padding creates future or unimplemented recipe meanings.
		for (const auto &state : rows) counters += static_cast<uint32_t>(state.batches.size() + state.consumed.size() + state.produced.size());
		std::array<uint32_t, 3> header{1, static_cast<uint32_t>(rows.size()), counters};
		SlSetLength(12);
		SlCopy<VarFileType::U32>(header);
	}
	/** @param preview Copy independent bounds rather than loader transport. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::LayerProcessors) || SlGetFieldLength() != 12) SlErrorCorrupt("Invalid processor header version/length"); // Only this version owns exactly three uint32 fields.
		std::array<uint32_t, 3> header;
		SlCopy<VarFileType::U32>(header);
		if (header[0] != 1 || header[1] > MAX_PROCESSOR_ROWS || header[2] > MAX_PROCESSOR_COUNTERS || header[2] > header[1] * 48) SlErrorCorrupt("Invalid processor header bounds"); // Count ceilings bound both actual owners and per-owner meanings.
		auto &presence = preview ? _load_check_data.layer_processor_present : _processor_presence;
		if (presence[0]) SlErrorCorrupt("Duplicate processor header");
		presence[0] = true;
		// Bind the destination first: GCC 13 rejects a braced-list assignment to a conditional lvalue.
		auto &expected = preview ? _load_check_data.layer_processor_expected : _processor_expected;
		expected = {header[1], header[2]};
	}
	/** Parse unpublished header bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Fixed reader owns exact payload length. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
	/** Assemble complete unpublished side records; no physical inventories are touched before final owner validation. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!CheckLayerProcessorFamily(false)) return;
		std::vector<LayerProcessorState> rows = _processor_rows;
		// Every historical key must name a parsed scalar owner; orphan counters cannot invent a processor.
		for (const auto &counter : _processor_counters) {
			auto state = std::ranges::lower_bound(rows, IndustryID{counter.industry}, std::less{}, &LayerProcessorState::industry);
			if (state == rows.end() || state->industry != IndustryID{counter.industry}) SlErrorCorrupt("Orphan processor counter"); // Existing actual owner required, never a synthesized default row.
			if (counter.kind == 0) {
				MultiLayerRecipeID recipe;
				std::copy(counter.label.begin(), counter.label.end(), recipe.begin());
				state->batches.emplace(recipe, counter.count);
			} else if (counter.kind == 1) {
				state->consumed.emplace(counter.label, counter.count);
			} else {
				state->produced.emplace(counter.label, counter.count);
			}
		}
		StageLoadedLayerProcessors(std::move(rows));
	}
};

/** Processor scalar rows in canonical actual-industry order. */
struct LPCRChunkHandler : ChunkHandler {
	/** Register scalar table. */
	LPCRChunkHandler() : ChunkHandler("LPCR", ChunkType::Table) {}
	/** Save choice/work/history stamps without copying incoming or outgoing cargo. */
	void Save() const override
	{
		SlTableHeader(_processor_desc);
		uint32_t ordinal = 0;
		// Explicit content labels survive runtime cargo/industry mapping changes.
		for (const auto &state : CaptureLayerProcessors()) {
			LayerProcessorWire wire{state.industry.base(), state.controller.base(), state.recipe, state.work_ticks, state.scale_fraction, state.revision, state.dispatched};
			SlSetArrayIndex(ordinal++);
			SlObject(&wire, _processor_desc);
		}
	}
	/** @param preview Check only independent preview counts, without staging choices. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::LayerProcessors)) SlErrorCorrupt("Unexpected processor rows");
		const auto description = SlTableHeader(_processor_desc);
		if (description.size() != std::size(_processor_desc)) SlErrorCorrupt("Invalid processor field count");
		// A missing choice/work/counter field cannot default into new production.
		for (const auto &field : _processor_desc) {
			if (std::ranges::find(description, field.name, &SaveLoad::name) == description.end()) SlErrorCorrupt("Missing processor field");
		}
		uint32_t count = 0;
		uint16_t previous = 0;
		// Validate bounded dense transport and strictly ordered actual IDs before allocating loader rows.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_PROCESSOR_ROWS || index != static_cast<int>(count)) SlErrorCorrupt("Invalid processor ordinal/count"); // A file cannot force sparse or unbounded allocations.
			LayerProcessorWire wire;
			SlObject(&wire, description);
			// Known identity, nonempty choice, strict order and valid work/revision bounds precede runtime meaning checks.
			if (wire.industry == IndustryID::Invalid().base() || (count != 0 && wire.industry <= previous) ||
					wire.recipe.Empty() || wire.revision == 0 || wire.scale_fraction >= 200 || wire.work_ticks > uint64_t{UINT32_MAX} * UINT16_MAX) SlErrorCorrupt("Invalid processor fields");
			previous = wire.industry;
			if (!preview) {
				LayerProcessorState state;
				state.industry = IndustryID{wire.industry};
				state.controller = CompanyID{wire.controller};
				state.recipe = wire.recipe;
				state.work_ticks = wire.work_ticks;
				state.scale_fraction = wire.scale_fraction;
				state.revision = wire.revision;
				state.dispatched = wire.dispatched;
				_processor_rows.push_back(std::move(state));
			}
			++count;
		}
		auto &presence = preview ? _load_check_data.layer_processor_present : _processor_presence;
		if (presence[1]) SlErrorCorrupt("Duplicate processor rows");
		presence[1] = true;
		if (preview) _load_check_data.layer_processor_rows[0] = count;
	}
	/** Parse unpublished scalar rows. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns exact row boundaries. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

/** Separate bounded historical key table avoids padded per-owner arrays and preserves explicit zero keys. */
struct LPCCChunkHandler : ChunkHandler {
	/** Register historical counters. */
	LPCCChunkHandler() : ChunkHandler("LPCC", ChunkType::Table) {}
	/** Serialize owner/category/label order so repeated or omitted keys cannot hide in restoration. */
	void Save() const override
	{
		SlTableHeader(_counter_desc);
		uint32_t ordinal = 0;
		// Every canonical map includes unused zero keys; commits must never allocate after changing physical stock.
		for (const auto &state : CaptureLayerProcessors()) {
			/** Write one category's existing keys. @param kind Saved historical role. @param values Owner's canonical counters. */
			const auto write = [&](uint8_t kind, const auto &values) {
				for (const auto &[key, value] : values) {
					LayerProcessorCounterWire wire;
					wire.industry = state.industry.base();
					wire.kind = kind;
					std::copy(key.begin(), key.end(), wire.label.begin());
					wire.count = value;
					SlSetArrayIndex(ordinal++);
					SlObject(&wire, _counter_desc);
				}
			};
			write(0, state.batches);
			write(1, state.consumed);
			write(2, state.produced);
		}
	}
	/** @param preview Validate independent counter counts without staging historical totals. */
	void Read(bool preview) const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::LayerProcessors)) SlErrorCorrupt("Unexpected processor counters");
		const auto description = SlTableHeader(_counter_desc);
		if (description.size() != std::size(_counter_desc)) SlErrorCorrupt("Invalid processor counter field count");
		// Missing owner/category/label/count is corruption, never permission to create a zeroed historical key.
		for (const auto &field : _counter_desc) {
			if (std::ranges::find(description, field.name, &SaveLoad::name) == description.end()) SlErrorCorrupt("Missing processor counter field");
		}
		uint32_t count = 0;
		std::tuple<uint16_t, uint8_t, CargoLabel> previous{};
		// Ordering removes duplicate historical keys before unpublished rows can become an inventory participant.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (count >= MAX_PROCESSOR_COUNTERS || index != static_cast<int>(count)) SlErrorCorrupt("Invalid processor counter ordinal/count"); // Bound every allocation and require dense transport.
			LayerProcessorCounterWire wire;
			SlObject(&wire, description);
			const auto key = std::tuple{wire.industry, wire.kind, wire.label};
			if (wire.industry == IndustryID::Invalid().base() || wire.kind > 2 || wire.label.Empty() || (count != 0 && key <= previous)) SlErrorCorrupt("Invalid processor counter fields"); // Strict owner/category/label order excludes duplicates and unknown roles.
			previous = key;
			if (!preview) _processor_counters.push_back(wire);
			++count;
		}
		auto &presence = preview ? _load_check_data.layer_processor_present : _processor_presence;
		if (presence[2]) SlErrorCorrupt("Duplicate processor counters");
		presence[2] = true;
		if (preview) _load_check_data.layer_processor_rows[1] = count;
	}
	/** Parse unpublished historical keys. */
	void Load() const override { this->Read(false); }
	/** @param len Table reader owns exact row boundaries. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const LPCHChunkHandler LPCH;
static const LPCRChunkHandler LPCR;
static const LPCCChunkHandler LPCC;
static const ChunkHandlerRef layer_processor_handlers[] = {LPCH, LPCR, LPCC};
extern const ChunkHandlerTable _layer_processor_chunk_handlers(layer_processor_handlers);
