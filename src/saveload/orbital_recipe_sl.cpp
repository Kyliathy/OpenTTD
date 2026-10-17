/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_recipe_sl.cpp Bounded native/copy processing transport over original SPST module packet owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C
 * Independent same-file admission precedes private work restore; legacy migration supplies disabled empty metadata only.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../orbital_recipe.h"
#include "../space_structure.h"
#include "../fios.h"
#include "../company_func.h"
#include "../core/backup_type.hpp"
#include "../timer/timer_game_tick.h"
#include "../safeguards.h"

/** Detached bounded family; decoding grants no native module, work, stock or electrical identity. */
struct OrbitalRecipeTransport {
	std::map<MultiLayerRecipeID, uint64_t> retired_batches; ///< Same-authority spent recipe aggregate, never a retired-object registry.
	uint64_t retired_discarded_j = 0; ///< Actual already spent unfinished work; cannot become electrical supply.
	std::vector<OrbitalRecipeRecord> rows; ///< One ordered metadata record per existing actual processor module.
};
static std::optional<OrbitalRecipeTransport> recipe_candidate;

/** @copydoc ResetOrbitalRecipeLoadStaging */
void ResetOrbitalRecipeLoadStaging() { recipe_candidate.reset(); }

/** @copydoc AcceptLoadedOrbitalRecipes */
bool AcceptLoadedOrbitalRecipes()
{
	if (!recipe_candidate) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::OrbitalRecipeProcessing)) return false;
		// A genuine older passive buffer gains disabled zero-work metadata only. Loading never commissions an account or interprets retained stock as an earned batch.
		for (auto *site : SpaceStructure::Iterate()) {
			if (!site->module || !IsOrbitalRecipeKind(site->state.kind)) continue;
			AutoRestoreBackup actor(_current_company, site->state.owner); site->module->processing = CreateEmptyOrbitalRecipeState(*site);
			if (!site->module->processing) return false;
		}
		return ValidateOrbitalRecipes();
	}
	auto history = GetSpaceStructureRetirementHistory(); history.processing_batches = recipe_candidate->retired_batches; history.processing_discarded_paid_j = recipe_candidate->retired_discarded_j;
	if (!ValidateOrbitalRecipeRecords(recipe_candidate->rows, CaptureSpaceStructures(), CaptureUtilityPowerState(), TimerGameTick::counter, history)) return false;
	std::vector<std::unique_ptr<OrbitalRecipeState>> installed; installed.reserve(recipe_candidate->rows.size());
	// Prepare every private owner only after complete native module/utility joins pass. Any failed restore destroys these temporary metadata owners before publishing any replacement.
	for (const auto &record : recipe_candidate->rows) {
		auto state = std::make_unique<OrbitalRecipeState>(); static_cast<OrbitalRecipeMetadata &>(*state) = record;
		if (record.electrical) {
			const auto *site = ResolveSpaceStructure(record.site);
			state->electrical = LayerRecipeElectricalWork::Restore(GetOrbitalRecipeDefinition(site->state.kind, record.choice)->recipe, *record.electrical, MapLayer::Space);
			if (!state->electrical) return false;
		}
		installed.push_back(std::move(state));
	}
	if (!InstallLoadedSpaceStructureProcessingHistory(history)) return false;
	// Each already admitted module receives exactly its prepared private owner; packet custody and all native utility identities stay unchanged.
	for (size_t i = 0; i < installed.size(); ++i) ResolveSpaceStructure(recipe_candidate->rows[i].site)->module->processing = std::move(installed[i]);
	recipe_candidate.reset(); return ValidateOrbitalRecipes();
}

/** @copydoc ValidateOrbitalRecipeLoadPreview */
void ValidateOrbitalRecipeLoadPreview()
{
	const auto &data = _load_check_data;
	if (!data.orbital_recipes_present) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::OrbitalRecipeProcessing)) SlErrorCorrupt("Missing orbital processing family");
		return;
	}
	auto history = data.space_structure_retirement_history; history.processing_batches = data.orbital_retired_recipe_batches; history.processing_discarded_paid_j = data.orbital_retired_discarded_paid_j;
	// Nonempty processing state requires this file's actual profile and native clock; the active world's context supplies neither ownership nor elapsed work.
	if ((!data.orbital_recipe_rows.empty() || !history.processing_batches.empty() || history.processing_discarded_paid_j != 0) &&
		(data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry) || !data.native_tick_present)) SlErrorCorrupt("Missing orbital processing profile/clock");
	if (!ValidateOrbitalRecipeRecords(data.orbital_recipe_rows, data.space_structure_rows, data.utility_power_state, data.native_tick_counter, history)) SlErrorCorrupt("Invalid orbital processing ownership");
}

/** Required versioned metadata family; original module stock remains exclusively serialized by SPST/CAPA. */
struct SPRCChunkHandler : ChunkHandler {
	/** Bind the unique native family tag; a copied preview never activates native processing. */
	SPRCChunkHandler() : ChunkHandler("SPRC", ChunkType::Riff) {}
	/** @param value Exact unsigned scalar. @param width Fixed native little-endian byte count. */
	static void Write(uint64_t value, uint width) { for (uint byte = 0; byte < width; ++byte) SlWriteByte(static_cast<uint8_t>(value >> (8 * byte))); }
	/** @param width Fixed admitted byte count. @return Exact native little-endian unsigned scalar. */
	static uint64_t Read(uint width) { uint64_t value = 0; for (uint byte = 0; byte < width; ++byte) value |= uint64_t{SlReadByte()} << (8 * byte); return value; }
	/** @param label Original stable four-byte cargo/recipe meaning. */
	static void WriteLabel(const BaseLabel &label) { for (const auto byte : label) SlWriteByte(byte); }
	/** @tparam Key Original cargo/recipe label type. @param values Canonical sorted bounded historical counters. */
	template <typename Key> static void WriteMap(const std::map<Key, uint64_t> &values)
	{
		Write(values.size(), 1);
		// Each original stable meaning owns exactly one historical quantity; no runtime ordinal or duplicated key becomes another material account.
		for (const auto &[label, quantity] : values) { WriteLabel(label); Write(quantity, 8); }
	}
	/** Serialize only fully admitted native metadata, private work and same-authority spent history. */
	void Save() const override
	{
		if (!ValidateOrbitalRecipes()) SlErrorCorrupt("Invalid orbital processing on save");
		const auto rows = CaptureOrbitalRecipes(); const auto &history = GetSpaceStructureRetirementHistory(); size_t length = 11 + 12 * history.processing_batches.size();
		// Exact lengths count every bounded history key and optional private receipt, never cargo quantities or native object addresses.
		for (const auto &row : rows) length += 74 + (row.electrical ? 52 : 0) + 12 * (row.batches.size() + row.consumed.size() + row.produced.size() + row.residue_mass_sixteenths.size());
		SlSetLength(length); Write(history.processing_discarded_paid_j, 8); WriteMap(history.processing_batches); Write(rows.size(), 2);
		for (const auto &row : rows) {
			Write(row.site.structure.base(), 2); Write(row.site.generation, 8); WriteLabel(row.choice); Write(row.source.structure.base(), 2); Write(row.source.generation, 8);
			Write(row.consumer.base(), 4); Write(row.revision, 8); Write(row.unpowered_work_ticks, 8); Write(row.enabled, 1); Write(row.last_native_tick, 8);
			Write(row.paid_lifetime_j, 8); Write(row.discarded_paid_j, 8); Write(row.electrical.has_value(), 1);
			if (row.electrical) {
				const auto &work = *row.electrical; WriteLabel(work.recipe); Write(work.consumer, 4); Write(work.work_per_batch, 4); Write(work.power_per_batch, 8);
				Write(work.paid_j, 8); Write(work.sequence, 8); Write(work.last_interval, 8); Write(work.interval_paid_j, 8);
			}
			WriteMap(row.batches); WriteMap(row.consumed); WriteMap(row.produced); WriteMap(row.residue_mass_sixteenths);
		}
	}
	/** Decode bounded detached data without resolving native pools or issuing work/accounts.
	 * @return Complete same-file family; native/copy owner admission follows after all original families have decoded.
	 */
	OrbitalRecipeTransport ReadRecords() const
	{
		const size_t length = SlGetFieldLength();
		// Earlier files cannot smuggle new processing metadata, and a bounded complete family must fit before any copied vector or map is allocated.
		if (IsSavegameVersionBefore(SaveLoadVersion::OrbitalRecipeProcessing) || length < 11 || length > 59 + 414 * SpaceStructureID::End().base()) SlErrorCorrupt("Invalid orbital processing length");
		size_t consumed = 0;
		/** @param width Requested fixed bytes. @return Exact scalar only after admitting its complete bytes within this same family. */
		auto read = [&](uint width) { if (width > length - consumed) SlErrorCorrupt("Truncated orbital processing"); consumed += width; return Read(width); };
		/** @param label Receives the exact original stable four-byte meaning, never a native runtime identity. */
		auto label = [&](BaseLabel &label) { for (auto &byte : label) byte = static_cast<uint8_t>(read(1)); };
		/** @param values Empty canonical history destination. @param cap Hard original role-count bound before allocating copied keys. */
		auto map = [&](auto &values, size_t cap) {
			const auto count = read(1); if (count > cap) SlErrorCorrupt("Oversized orbital processing history");
			// Canonical strictly increasing stable meanings exclude duplicates/reordered keys before semantic same-file admission.
			for (size_t i = 0; i < count; ++i) {
				typename std::decay_t<decltype(values)>::key_type key; label(key); const auto quantity = read(8);
				if (!values.empty() && key <= values.rbegin()->first) SlErrorCorrupt("Noncanonical orbital processing history");
				values.emplace(key, quantity);
			}
		};
		OrbitalRecipeTransport result; result.retired_discarded_j = read(8); map(result.retired_batches, 4); const auto count = read(2);
		if (count > SpaceStructureID::End().base()) SlErrorCorrupt("Oversized orbital processing family");
		result.rows.reserve(count);
		// Every record is detached metadata for one same-file module; decoding cannot allocate a native structure, packet or electrical consumer.
		for (size_t i = 0; i < count; ++i) {
			OrbitalRecipeRecord row; row.site = {SpaceStructureID{static_cast<uint16_t>(read(2))}, read(8)}; label(row.choice);
			row.source = {SpaceStructureID{static_cast<uint16_t>(read(2))}, read(8)}; row.consumer = UtilityConsumerID{static_cast<uint32_t>(read(4))};
			row.revision = read(8); row.unpowered_work_ticks = read(8); const auto enabled = read(1); row.last_native_tick = read(8);
			row.paid_lifetime_j = read(8); row.discarded_paid_j = read(8); const auto electrical = read(1);
			if (enabled > 1 || electrical > 1) SlErrorCorrupt("Invalid orbital processing flags");
			row.enabled = enabled != 0;
			if (electrical != 0) {
				auto &work = row.electrical.emplace(); label(work.recipe); work.consumer = static_cast<uint32_t>(read(4)); work.work_per_batch = static_cast<uint32_t>(read(4)); work.power_per_batch = read(8);
				work.paid_j = read(8); work.sequence = read(8); work.last_interval = read(8); work.interval_paid_j = read(8);
			}
			map(row.batches, 4); map(row.consumed, 8); map(row.produced, 8); map(row.residue_mass_sixteenths, 4); result.rows.push_back(std::move(row));
		}
		if (consumed != length) SlErrorCorrupt("Trailing orbital processing data");
		return result;
	}
	/** Stage one original authoritative family, refusing duplicate transport before native ownership changes. */
	void Load() const override { if (recipe_candidate) SlErrorCorrupt("Repeated orbital processing family"); recipe_candidate = ReadRecords(); }
	/** @param length Framework-admitted length; the bounded detached decoder independently checks every original field. */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.orbital_recipes_present) SlErrorCorrupt("Repeated orbital processing preview");
		auto data = ReadRecords(); _load_check_data.orbital_recipe_rows = std::move(data.rows); _load_check_data.orbital_retired_recipe_batches = std::move(data.retired_batches);
		_load_check_data.orbital_retired_discarded_paid_j = data.retired_discarded_j; _load_check_data.orbital_recipes_present = true;
	}
};
static const SPRCChunkHandler SPRC;
static const ChunkHandlerRef recipe_handlers[] = {SPRC};
extern const ChunkHandlerTable _orbital_recipe_chunk_handlers(recipe_handlers);
