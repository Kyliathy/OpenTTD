/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md; original B adapters use explicit lineage capabilities.
 * @file utility_conversion_sl.cpp Exact428 auxiliary source conversion metadata and bounded original thermal provenance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8DB.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../utility_power.h"
#include "../utility_power_conversion_wire.h"
#include "../multilayer_state.h"
#include "../fios.h"
#include "../safeguards.h"

static const SaveLoad _conversion_desc[] = {
	SaveLoad::Variable<VarFileType::U8>("kind", SLE_OBJECT_ADDRESS(UtilityHeatConversionWire, kind)),
	SaveLoad::Variable<VarFileType::U32>("output", SLE_OBJECT_ADDRESS(UtilityHeatConversionWire, output)),
	SaveLoad::Variable<VarFileType::U32>("input", SLE_OBJECT_ADDRESS(UtilityHeatConversionWire, input)),
	SaveLoad::Variable<VarFileType::U8>("efficiency", SLE_OBJECT_ADDRESS(UtilityHeatConversionWire, efficiency)),
	SaveLoad::Variable<VarFileType::U8>("numerator", SLE_OBJECT_ADDRESS(UtilityHeatConversionWire, numerator)),
	SaveLoad::Variable<VarFileType::U64>("capacity_w", SLE_OBJECT_ADDRESS(UtilityHeatConversionWire, capacity_w)),
	SaveLoad::Variable<VarFileType::U32>("origin", SLE_OBJECT_ADDRESS(UtilityHeatConversionWire, origin)),
	SaveLoad::Variable<VarFileType::U64>("heat_j", SLE_OBJECT_ADDRESS(UtilityHeatConversionWire, heat_j)),
};
static std::array<bool, 2> _conversion_present{}; ///< Exact unpublished full-load header/table presence.
static std::array<uint64_t, 4> _conversion_header{}; ///< Schema1/source count/origin count/original common epoch.
static std::vector<UtilityHeatConversionWire> _conversion_rows; ///< Copied references/history, never live spendable energy.

/** @copydoc ResetUtilityHeatConversionLoadStaging */
void ResetUtilityHeatConversionLoadStaging() { _conversion_present.fill(false); _conversion_header.fill(0); _conversion_rows.clear(); }

/** @copydoc JoinLoadedUtilityHeatConversions */
void JoinLoadedUtilityHeatConversions(UtilityPowerState &state, bool preview)
{
	const auto &presence = preview ? _load_check_data.utility_conversion_present : _conversion_present;
	const auto &header = preview ? _load_check_data.utility_conversion_header : _conversion_header;
	const auto &rows = preview ? _load_check_data.utility_conversion_rows : _conversion_rows;
	const auto *metadata = preview ? nullptr : GetPendingMultiLayerStateSchema();
	const uint32_t flags = preview ? _load_check_data.multilayer_feature_flags : metadata == nullptr ? 0 : metadata->feature_flags;
	const bool required = (flags & MULTILAYER_FEATURE_HEAT_CONVERSIONS) != 0;
	/** @param source Copied original native producer. @return Whether this exact source requires one auxiliary conversion metadata row. */
	const auto converted = [](const UtilityPowerProducer &source) { return source.kind == UtilityGenerationKind::HeatConversion; };
	const auto sources = std::ranges::count_if(state.producers, converted);
	if (!presence[0]) {
		// Current files carry both components even when empty; genuine older
		// absence cannot acquire a new source kind or borrow the live ledger.
		if (SlHasSaveCapability(SaveCapability::UtilityHeatConversion) || presence[1] || required || sources != 0) SlErrorCorrupt("Missing utility conversion header");
		return;
	}
	// Exact source coverage, original epoch and metadata/counts are one family;
	// omission cannot default a later ratio, input reference or fractional carry.
	if (!presence[1] || header[3] != state.interval || header[1] != static_cast<uint64_t>(sources) ||
		header[1] + header[2] != rows.size() || required != (sources != 0)) SlErrorCorrupt("Incomplete utility conversion family");
	// Repeated checks replace only this unpublished candidate's extensions,
	// just as original utility paths rebuild from copied transport each time.
	for (auto &source : state.producers) {
		source.conversion_input = UtilityConsumerID{UINT32_MAX}; source.conversion_efficiency = 0;
		source.conversion_capacity_w = 0; source.conversion_remainder = 0; source.conversion_origins.clear();
	}
	uint64_t metadata_rows = 0, origin_rows = 0;
	// Canonical rows resolve only against this file's original native source/
	// consumer identities; no live source, company, quantity or clock is consulted.
	for (const auto &row : rows) {
		if (row.output >= state.producers.size()) SlErrorCorrupt("Unknown utility conversion output");
		auto &output = state.producers[row.output];
		// Only this exact conversion producer can acquire the saved ratio and retained Heat attribution.
		if (output.meter.producer_id.base() != row.output || output.kind != UtilityGenerationKind::HeatConversion) SlErrorCorrupt("Foreign utility conversion output");
		if (row.kind == 0) {
			if (row.input >= state.consumers.size()) SlErrorCorrupt("Unknown utility conversion input");
			output.conversion_input = UtilityConsumerID{row.input}; output.conversion_efficiency = row.efficiency;
			output.conversion_capacity_w = row.capacity_w; output.conversion_remainder = row.numerator; ++metadata_rows;
			// An active copied source needs an actual company from this file;
			// historical inactive identities may outlive their company incarnation.
			if (preview && output.active && !_load_check_data.companies.contains(output.owner.base())) SlErrorCorrupt("Unknown utility conversion preview owner");
		} else {
			// Original Heat attribution requires a declared output and real original producer; missing roles cannot create energy.
			if (row.origin >= state.producers.size() || output.conversion_efficiency == 0) SlErrorCorrupt("Unknown utility conversion origin");
			output.conversion_origins.push_back({UtilityProducerID{row.origin}, row.heat_j}); ++origin_rows;
		}
	}
	if (metadata_rows != header[1] || origin_rows != header[2]) SlErrorCorrupt("Utility conversion row counts disagree"); // A row cannot change role while keeping the same total cardinality.
}

/** Fixed32-byte auxiliary bounds and original common accounting epoch; no previous utility header is reinterpreted. */
struct UCVHChunkHandler final : ChunkHandler {
	/** Register exact conversion metadata/provenance header. */
	UCVHChunkHandler() : ChunkHandler("UCVH", ChunkType::Riff) {}
	/** Serialize actual retained source/origin counts and their common original epoch, including an empty family. */
	void Save() const override
	{
		const auto state = CaptureUtilityPowerState(); std::array<uint64_t, 4> header{1, 0, 0, state.interval};
		// Include retired attribution as well as active conversion; losing a
		// facility cannot remove its already spent original thermal history.
		for (const auto &source : state.producers) {
			if (source.kind == UtilityGenerationKind::HeatConversion) { ++header[1]; header[2] += source.conversion_origins.size(); }
		}
		SlSetLength(32); SlCopy<VarFileType::U64>(header);
	}
	/** @param preview Decode only this independent preview or unpublished full load's exact bounds. */
	void Read(bool preview) const
	{
		if (!SlHasSaveCapability(SaveCapability::UtilityHeatConversion) || SlGetChunkType() != ChunkType::Riff || SlGetFieldLength() != 32) SlErrorCorrupt("Invalid utility conversion header version"); // Older rate-only source transport cannot acquire later conversion authority.
		std::array<uint64_t, 4> header; SlCopy<VarFileType::U64>(header);
		if (header[0] != 1 || header[1] > MAX_UTILITY_INTERVAL_ACCOUNTS || header[2] > MAX_UTILITY_CONVERSION_ORIGINS || header[3] == 0 || header[3] == UINT64_MAX) SlErrorCorrupt("Invalid utility conversion bounds"); // Validate revision, all allocation bounds and the nonterminal original epoch before any row storage.
		auto &presence = preview ? _load_check_data.utility_conversion_present : _conversion_present;
		if (presence[0]) SlErrorCorrupt("Duplicate utility conversion header");
		presence[0] = true; (preview ? _load_check_data.utility_conversion_header : _conversion_header) = header;
	}
	/** Decode unpublished full-load bounds. */
	void Load() const override { this->Read(false); }
	/** @param len Exact reader enforces its fixed32-byte header. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

/** Exact31-byte scalar metadata/origin rows, with every field mandatory and the prior utility tables unchanged. */
struct UCVDChunkHandler final : ChunkHandler {
	/** Register the auxiliary original conversion table. */
	UCVDChunkHandler() : ChunkHandler("UCVD", ChunkType::Table) {}
	/** Serialize exact original metadata first, followed by canonical original thermal contributions. */
	void Save() const override
	{
		SlTableHeader(_conversion_desc); const auto state = CaptureUtilityPowerState(); uint32_t ordinal = 0;
		// Source identity order is canonical and each conversion-only producer
		// emits exactly one metadata row, independent of current availability.
		for (const auto &source : state.producers) {
			if (source.kind != UtilityGenerationKind::HeatConversion) continue;
			UtilityHeatConversionWire row{0, source.meter.producer_id.base(), source.conversion_input.base(), source.conversion_efficiency, source.conversion_remainder, source.conversion_capacity_w};
			SlSetArrayIndex(ordinal++); SlObject(&row, _conversion_desc);
		}
		// Original contributions retain their owner/output identity and positive
		// lifetime use; no repeated copy becomes another spendable source balance.
		for (const auto &source : state.producers) {
			for (const auto &origin : source.conversion_origins) {
				UtilityHeatConversionWire row{1, source.meter.producer_id.base(), 0, 0, 0, 0, origin.producer.base(), origin.heat_j};
				SlSetArrayIndex(ordinal++); SlObject(&row, _conversion_desc);
			}
		}
	}
	/** @param preview Decode exact copied rows with bounded dense ordinals and immutable role/unused-field interpretation. */
	void Read(bool preview) const
	{
		// Conversion ratios and retained Heat attribution require the original TABLE framing before joining the sole utility owner.
		if (!SlHasSaveCapability(SaveCapability::UtilityHeatConversion) || SlGetChunkType() != ChunkType::Table) SlErrorCorrupt("Unexpected utility conversion rows");
		auto &presence = preview ? _load_check_data.utility_conversion_present : _conversion_present;
		if (presence[1]) SlErrorCorrupt("Duplicate utility conversion table");
		const auto desc = SlTableHeader(_conversion_desc);
		if (desc.size() != std::size(_conversion_desc)) SlErrorCorrupt("Invalid utility conversion fields");
		// Omitting any original identity, ratio or quantity must not normalize
		// into a different source, zero history or free initial fractional output.
		for (const auto &field : _conversion_desc) {
			if (std::ranges::count(desc, field.name, &SaveLoad::name) != 1) SlErrorCorrupt("Missing utility conversion field");
		}
		auto &rows = preview ? _load_check_data.utility_conversion_rows : _conversion_rows;
		uint32_t ordinal = 0; uint8_t prior_kind = 0; std::optional<std::pair<uint32_t, uint32_t>> previous;
		// Dense bounded canonical roles prevent sparse/duplicate metadata and
		// alternate ordering from concealing extra original thermal contributions.
		for (int index = SlIterateArray(); index != -1; index = SlIterateArray()) {
			if (index != static_cast<int>(ordinal++) || rows.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS + MAX_UTILITY_CONVERSION_ORIGINS) SlErrorCorrupt("Invalid utility conversion ordinal");
			UtilityHeatConversionWire row; SlObject(&row, desc);
			// Ordered known roles keep source metadata distinct from lifetime thermal contributions.
			if (row.kind > 1 || row.kind < prior_kind || row.output >= MAX_UTILITY_INTERVAL_ACCOUNTS) SlErrorCorrupt("Invalid utility conversion role");
			if (row.kind != prior_kind) previous.reset();
			prior_kind = row.kind; const auto key = std::pair{row.output, row.kind == 0 ? 0U : row.origin};
			// Each original output/origin pair appears once, preventing duplicated retained input history.
			if (previous.has_value() && key <= *previous) SlErrorCorrupt("Duplicate utility conversion identity");
			previous = key;
			// Each wire role has exactly its own scalar payload; another role's
			// unused fields cannot smuggle a hidden ratio, carry or provenance row.
			if (row.kind == 0) {
				if (row.input >= MAX_UTILITY_INTERVAL_ACCOUNTS || row.efficiency == 0 || row.efficiency > 100 || row.numerator >= 100 ||
					row.capacity_w == 0 || row.capacity_w > MAX_UTILITY_RATE_W || row.origin != 0 || row.heat_j != 0) SlErrorCorrupt("Invalid utility conversion metadata");
			} else if (row.input != 0 || row.efficiency != 0 || row.numerator != 0 || row.capacity_w != 0 ||
				row.origin >= MAX_UTILITY_INTERVAL_ACCOUNTS || row.heat_j == 0) SlErrorCorrupt("Invalid utility conversion origin fields");
			rows.push_back(row);
		}
		presence[1] = true;
	}
	/** Decode unpublished exact current rows. */
	void Load() const override { this->Read(false); }
	/** @param len Exact field/dense-row reader bounds all native records. */
	void LoadCheck([[maybe_unused]] size_t len) const override { this->Read(true); }
};

static const UCVHChunkHandler UCVH;
static const UCVDChunkHandler UCVD;
static const ChunkHandlerRef utility_conversion_handlers[] = {UCVH, UCVD};
extern const ChunkHandlerTable _utility_conversion_chunk_handlers(utility_conversion_handlers);
