/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_lineage_sl.cpp Explicit merged writer identity; feature owners retain every gameplay quantity and receipt.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 * MLLNschema1names the closed delivered capability set at441. It grants no
 * cargo, finance, machinery or physical authority without the complete owners.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 22; the writer emits (446, schema 2) and readers accept exactly (441, 1) or (446, 2).
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include <optional>
#include "../safeguards.h"

static_assert(to_underlying(SaveLoadVersion::MergedLineage) == MERGED_SAVE_LINEAGE_VERSION);
static_assert(to_underlying(SaveLoadVersion::SpaceMiningLifecycle) == CURRENT_MERGED_SAVE_VERSION);

/** @param version Admitted merged envelope version. @return The one MLLN schema that version wrote, or no value for any other version. */
static std::optional<uint16_t> MergedLineageSchema(uint16_t version)
{
	if (version == MERGED_SAVE_LINEAGE_VERSION) return MERGED_SAVE_LINEAGE_SCHEMA;
	if (version == CURRENT_MERGED_SAVE_VERSION) return CURRENT_MERGED_SAVE_LINEAGE_SCHEMA;
	return std::nullopt;
}

/** Unique fixed8-byte merged identity, validated by both detached and actual native readers. */
struct MLLNChunkHandler final : ChunkHandler {
	/** Register the sole explicit writer identity without allocating a gameplay owner. */
	MLLNChunkHandler() : ChunkHandler("MLLN", ChunkType::Riff) {}
	/** Emit the current writer's network-order identity, schema 2 and version 446; the complete owner families remain mandatory separately. */
	void Save() const override
	{
		std::array<uint32_t, 1> magic{MERGED_SAVE_LINEAGE_MAGIC};
		std::array<uint16_t, 2> version{CURRENT_MERGED_SAVE_LINEAGE_SCHEMA, CURRENT_MERGED_SAVE_VERSION};
		SlSetLength(8); SlCopy<VarFileType::U32>(magic); SlCopy<VarFileType::U16>(version);
	}
	/** Read this attempt's exact marker; original A/B formats cannot acquire merged capabilities by appending it. */
	void Read() const
	{
		const auto source = GetLoadCapabilities();
		// Only an admitted merged envelope at 441 or 446 and exact RIFF framing may
		// declare a closed capability contract; no legacy fallback and no range exist.
		const auto schema = source ? MergedLineageSchema(source->original_version) : std::nullopt;
		if (!source || source->format != SaveFormat::Unified || !schema ||
			SlGetChunkType() != ChunkType::Riff || SlGetFieldLength() != 8) SlErrorCorrupt("Invalid save format marker framing");
		std::array<uint32_t, 1> magic{}; std::array<uint16_t, 2> version{};
		SlCopy<VarFileType::U32>(magic); SlCopy<VarFileType::U16>(version);
		// The tuple is exactly (441, 1) or (446, 2) and names the envelope's own
		// version: future or crossed schemas, downgraded versions and substituted
		// identities refuse before any marker can stand in for missing owner evidence.
		if (magic[0] != MERGED_SAVE_LINEAGE_MAGIC || version[0] != *schema ||
			version[1] != source->original_version) SlErrorCorrupt("Invalid save format marker identity");
	}
	/** Validate the admitted full-load marker without publishing any gameplay state. */
	void Load() const override { this->Read(); }
	/** @param length Framework extent; Read independently requires the fixed original8-byte marker. */
	void LoadCheck([[maybe_unused]] size_t length) const override { this->Read(); }
};

static const MLLNChunkHandler MLLN;
static const ChunkHandlerRef lineage_handlers[] = {MLLN};
extern const ChunkHandlerTable _save_lineage_chunk_handlers(lineage_handlers);
