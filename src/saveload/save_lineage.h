/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_lineage.h Detached compatibility evidence for parallel save histories.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-b-save-lineage.md
 * This boundary neither parses bytes nor initializes a world. The actual reader
 * must supply bounded same-file facts; filenames and live owners are not inputs.
 */
#ifndef SAVE_LINEAGE_H
#define SAVE_LINEAGE_H

#include <array>
#include <cstdint>
#include <optional>

/** Original wire interpretation; a merged result does not rewrite legacy versions. */
enum class SaveDialect : uint8_t { Shared, DestinationA, DonorB, Merged };

/** Explicit admission result; refused/ambiguous files have no capabilities. */
enum class SaveLineageStatus : uint8_t { Accepted, Ambiguous, Corrupt, Unsupported };

/** Distinguish omitted transport from a present family that failed bounded validation. */
enum class SaveEvidenceState : uint8_t { Absent, Valid, Malformed };

/** Wire types observed by the bounded inspector, not selected by a native handler. */
enum class SaveEvidenceKind : uint8_t { Riff, Table };

/** Variable-layout evidence is usable only after an exact detached owner decoder. */
enum class SavePayloadDialect : uint8_t { Common, DestinationA, DonorB, Malformed };

/** The two legacy CAPA extension columns are one complete, type-checked contract. */
enum class SaveSupplyColumns : uint8_t { Absent, Complete, Malformed };

/** Lineage-relevant families; the normal native loader still validates other owners. */
enum class SaveLineageFamily : uint8_t {
	SupplyHeader, SupplyOrders, GeneratorHeader, GeneratorRows,
	ConversionHeader, ConversionRows, TubeSourceHeader, TubeSourceRows,
	CargoDimensions, OrbitalRecipes, Marker, Count,
};

/** One detached chunk observation after bounded header/column/scalar validation. */
struct SaveChunkEvidence {
	SaveEvidenceState state = SaveEvidenceState::Absent; ///< Valid requires exact header/column types and bounded scalar fields, not only chunk existence.
	SaveEvidenceKind kind = SaveEvidenceKind::Riff; ///< Actual framing kind; a TABLE cannot stand in for an equally named RIFF.
	uint32_t length = 0; ///< Exact RIFF payload length; table row framing is checked by the inspector.
	uint32_t records = 0; ///< Declared header count or actual table rows, permitting independent pair joins.
	uint16_t schema = 0; ///< Actual versioned header schema; zero where the historical transport has none.
};

/** Proposed merged allocation; activation and complete serialization belong to Group44. */
inline constexpr uint16_t MERGED_SAVE_LINEAGE_VERSION = 441;
inline constexpr uint32_t MERGED_SAVE_LINEAGE_MAGIC = 0x4D4C4E31; ///< Network-order MLN1 payload marker inside MLLN.
inline constexpr uint16_t MERGED_SAVE_LINEAGE_SCHEMA = 1;

/** Detached evidence for exactly one original byte stream; no game references are retained. */
struct SaveWireFacts {
	uint16_t original_version = 0; ///< Unchanged native envelope version.
	std::array<uint8_t, 32> source_digest{}; ///< Exact original SHA256; mandatory only when an origin hint is needed.
	bool framing_complete = false; ///< Inspector reached the terminator without truncation, duplicate selected chunks or invalid record framing.
	bool common_transport_valid = false; ///< Same-file, source-era common metadata/CAPA/SDEF/SPST framing is complete; no current-world defaults.
	uint32_t feature_flags = 0; ///< Actual MLMD flags; zero for genuine older metadata absence.
	uint16_t space_content_version = 0; ///< Actual SDEF0/2/3, validated against its source-era common transport.
	SaveSupplyColumns supply_columns = SaveSupplyColumns::Absent; ///< Actual legacy B extension names and types, including empty CAPA.
	SavePayloadDialect payload_dialect = SavePayloadDialect::Common; ///< Optional positive evidence from a bounded versioned owner-layout decoder.
	std::array<SaveChunkEvidence, static_cast<uint8_t>(SaveLineageFamily::Count)> families{}; ///< Fixed-size inspector results, independent of native pools.
	uint32_t marker_magic = 0; ///< Original MLLN payload magic, never supplied by the running build's identity.
	uint16_t marker_schema = 0; ///< Original MLLN payload schema.
	uint16_t marker_writer_version = 0; ///< Marker must agree with the unchanged outer envelope.
	uint64_t merged_supply_next_id = 0; ///< Canonical merged issuer frontier; history may outlive pruned terminal rows.
	bool merged_supply_valid = false; ///< Set only by Group44's canonical tagged-supply transport validator; legacy SSOR decoding cannot set it.
	bool merged_cargo_valid = false; ///< Set only by the canonical merged CAPA descriptor validator after Group12/44.

	/** Read one fixed observation without an owner lookup.
	 * @param family Declared lineage family; callers must not pass Count.
	 * @return This file's immutable family evidence.
	 */
	const SaveChunkEvidence &Get(SaveLineageFamily family) const { return this->families[static_cast<uint8_t>(family)]; }
};

/** An explicitly selected ambiguous origin, bound to all bytes of one original save. */
struct SaveOriginHint {
	SaveDialect dialect = SaveDialect::Shared; ///< Only DestinationA or DonorB may disambiguate legacy transport.
	std::array<uint8_t, 32> source_digest{}; ///< Exact original SHA256; zero is never a wildcard.
};

/** Pure classification result; callers must inspect status before using the dialect. */
struct SaveLineageResult {
	SaveLineageStatus status = SaveLineageStatus::Corrupt;
	SaveDialect dialect = SaveDialect::Shared; ///< Inert unless status is Accepted.
	uint16_t original_version = 0; ///< Always the input envelope version, including refusals.
};

/** Divergent post-fork features; existing pre425 gates keep their ordinary numeric semantics. */
enum class SaveCapability : uint8_t {
	OrbitalAssemblyFacilities, OrbitalManufacturingReservations, OrbitalManufacturingMaterialization,
	OrbitalManufacturingClones, SpaceReplacementSettlement, SpaceOrderedActions,
	SpaceHeavyMinerCommissioning, SpaceHeavyMinerMissions, SpaceHeavyMinerSupplies,
	SpaceHeavyMinerDescents, SpaceHeavyMinerOnboardServices, SpaceHeavyMinerContainedServices,
	SpaceCargoDimensions, OrbitalRecipeProcessing, CompanySupplyPackets, SurfaceSupplyOrdersA,
	TBMFaceTransfer, SurfaceSupplyOrdersB, GeothermalGenerators, UtilityHeatConversion,
	GeothermalGeneratorOperation, GeothermalMagmaLoop, UtilityTownService,
	UtilityLocalElectricalFeed, TubeGeothermalSources, TubeGeothermalBindings, Count,
};

/** Classify one complete set of detached transport facts without parsing or mutation.
 * @param facts Evidence extracted from the original stream by a bounded inspector.
 * @param origin Optional exact-digest hint; consulted only when wire evidence is ambiguous.
 * @return Accepted dialect or explicit ambiguity/corruption/unsupported-version refusal.
 */
SaveLineageResult ClassifySaveDialect(const SaveWireFacts &facts, const std::optional<SaveOriginHint> &origin = std::nullopt);

/** Query a divergent feature without applying the other branch's numeric enum.
 * @param result Accepted classification retaining the original envelope version.
 * @param capability Exact known feature; unknown future enum values always refuse.
 * @return Whether this dialect/version supports the feature, never a grant of gameplay ownership.
 */
bool HasSaveCapability(const SaveLineageResult &result, SaveCapability capability);

#endif /* SAVE_LINEAGE_H */
