/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_lineage_sl.cpp Explicit merged writer identity; feature owners retain every gameplay quantity and receipt.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 * MLLNschema1names the closed delivered capability set at441. It grants no
 * cargo, finance, machinery or physical authority without the complete owners.
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../safeguards.h"

static_assert(to_underlying(SaveLoadVersion::MergedLineage) == MERGED_SAVE_LINEAGE_VERSION);

/** Unique fixed8-byte merged identity, validated by both detached and actual native readers. */
struct MLLNChunkHandler final : ChunkHandler {
	/** Register the sole explicit writer identity without allocating a gameplay owner. */
	MLLNChunkHandler() : ChunkHandler("MLLN", ChunkType::Riff) {}
	/** Emit network-order identity/schema/version; the complete441owner families remain mandatory separately. */
	void Save() const override
	{
		std::array<uint32_t, 1> magic{MERGED_SAVE_LINEAGE_MAGIC};
		std::array<uint16_t, 2> version{MERGED_SAVE_LINEAGE_SCHEMA, MERGED_SAVE_LINEAGE_VERSION};
		SlSetLength(8); SlCopy<VarFileType::U32>(magic); SlCopy<VarFileType::U16>(version);
	}
	/** Read this attempt's exact marker; original A/B formats cannot acquire merged capabilities by appending it. */
	void Read() const
	{
		const auto source = GetLoadCapabilities();
		// Only the explicitly admitted merged envelope and exact RIFF framing
		// may declare this closed capability contract; no legacy fallback exists.
		if (!source || source->dialect != SaveDialect::Merged || source->original_version != MERGED_SAVE_LINEAGE_VERSION ||
			SlGetChunkType() != ChunkType::Riff || SlGetFieldLength() != 8) SlErrorCorrupt("Invalid merged lineage marker framing");
		std::array<uint32_t, 1> magic{}; std::array<uint16_t, 2> version{};
		SlCopy<VarFileType::U32>(magic); SlCopy<VarFileType::U16>(version);
		// Exact values reject future schemas, downgraded versions and substituted
		// identities before any marker can stand in for missing owner evidence.
		if (magic[0] != MERGED_SAVE_LINEAGE_MAGIC || version[0] != MERGED_SAVE_LINEAGE_SCHEMA ||
			version[1] != MERGED_SAVE_LINEAGE_VERSION) SlErrorCorrupt("Invalid merged lineage marker identity");
	}
	/** Validate the admitted full-load marker without publishing any gameplay state. */
	void Load() const override { this->Read(); }
	/** @param length Framework extent; Read independently requires the fixed original8-byte marker. */
	void LoadCheck([[maybe_unused]] size_t length) const override { this->Read(); }
};

static const MLLNChunkHandler MLLN;
static const ChunkHandlerRef lineage_handlers[] = {MLLN};
extern const ChunkHandlerTable _save_lineage_chunk_handlers(lineage_handlers);
