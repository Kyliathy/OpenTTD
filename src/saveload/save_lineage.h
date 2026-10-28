/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_lineage.h Detached compatibility evidence for parallel save histories.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-b-save-lineage.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Groups 9-10 and 17B; the Space eras as merged-446 capabilities, dev-build files refused.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 11; CAPA column encodings observed from the file's own descriptor.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 12; the RIFF SSOR book shape observed from its own bytes.
 * This boundary neither parses bytes nor initializes a world. The actual reader
 * must supply bounded same-file facts; filenames and live owners are not inputs.
 */
#ifndef SAVE_LINEAGE_H
#define SAVE_LINEAGE_H

#include <array>
#include <cstdint>
#include <optional>

/** Original wire interpretation; a merged result does not rewrite legacy versions.
 * Values are appended only: each one names a writer whose version numbers mean
 * something different from every other writer's same numbers.
 */
enum class SaveDialect : uint8_t {
	Shared, ///< Common history up to 424, before the branches diverged.
	DestinationA, ///< The destination's own historical writer, 425-440.
	DonorB, ///< The historical underground donor writer, 425-434.
	Merged, ///< The combined writer that emits MLLN: 441 with schema 1, later 446 with schema 2.
};

/** Explicit admission result; refused/ambiguous files have no capabilities. */
enum class SaveLineageStatus : uint8_t { Accepted, Ambiguous, Corrupt, Unsupported };

/** Distinguish omitted transport from a present family that failed bounded validation. */
enum class SaveEvidenceState : uint8_t { Absent, Valid, Malformed };

/** Wire types observed by the bounded inspector, not selected by a native handler. */
enum class SaveEvidenceKind : uint8_t { Riff, Table };

/** Variable-layout evidence is usable only after an exact detached owner decoder. */
enum class SavePayloadDialect : uint8_t { Common, DestinationA, DonorB, Malformed };

/** The two legacy CAPA extension columns are one complete, type-checked contract.
 * Kept for the existing classifier rules; `SupplyColumnEncoding` is the full observation.
 */
enum class SaveSupplyColumns : uint8_t { Absent, Complete, Malformed };

/** The CAPA supply-column pair as the file's own table descriptor declares it, never inferred from its version. */
enum class SupplyColumnEncoding : uint8_t {
	Absent, ///< Neither `company_supply_order` nor `company_supply_shipped`: shared, A or pre-426 B packets.
	LegacyU32Bool, ///< U32 order with Bool (I8) shipped: the historical donor B writer, 426-434.
	CanonicalU64U8, ///< U64 order with U8 shipped: the merged writer's canonical receipt link.
	Malformed, ///< Half a pair, or a pair with any other widths; the classifier refuses it.
};

/** How a RIFF SSOR payload reads, judged from its own bytes and never from the outer version or MLLN order.
 * The fixed A440 shape and the SSM1 tagged book are checked independently; the classifier decides which writer was meant.
 */
enum class SupplyBookShape : uint8_t {
	Absent, ///< No RIFF SSOR: genuinely absent, or the historical B TABLE.
	Fixed, ///< No SSM1 signature, and the fixed shape fits: 14-byte header, little-endian count, exact 14 + 53 * n extent.
	Tagged, ///< The SSM1 book validated completely, and the fixed shape does not fit.
	Ambiguous, ///< The SSM1 book validated and the fixed shape also fits the same bytes; the classifier refuses it.
	Malformed, ///< The SSM1 signature is present but its book is invalid, or no shape fits at all.
};

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

/** Frozen first merged allocation: the combined A+B writer, also `SaveLoadVersion::MergedLineage`.
 * Its meaning never changes. It has every A and B capability and none of the five Space eras that 446 adds.
 */
inline constexpr uint16_t MERGED_SAVE_LINEAGE_VERSION = 441;
inline constexpr uint32_t MERGED_SAVE_LINEAGE_MAGIC = 0x4D4C4E31; ///< Network-order MLN1 payload marker inside MLLN; shared by every merged schema.
inline constexpr uint16_t MERGED_SAVE_LINEAGE_SCHEMA = 1; ///< MLLN schema written by merged 441.

/** Reserved combined writer that adds the five Space eras; m2o28-1-merge-oct28.md Group 22 activates it.
 * Until then nothing writes 446 and the writer context stays at merged 441. The Space eras are capabilities
 * of merged 446, never `SaveLoadVersion` values: `MergedLineage = 441` keeps its meaning, and Group 22
 * deletes the pending enumerators that `saveload.h` keeps after `MaxVersion` in the meantime. Files that a
 * development build wrote at 441-445 without a marker are not read (m2o28 Group 17B).
 */
inline constexpr uint16_t CURRENT_MERGED_SAVE_VERSION = 446;
inline constexpr uint16_t CURRENT_MERGED_SAVE_LINEAGE_SCHEMA = 2; ///< MLLN schema reserved for merged 446; schema 1 stays bound to 441.

/** Detached evidence for exactly one original byte stream; no game references are retained. */
struct SaveWireFacts {
	uint16_t original_version = 0; ///< Unchanged native envelope version.
	std::array<uint8_t, 32> source_digest{}; ///< Exact original SHA256; mandatory only when an origin hint is needed.
	bool framing_complete = false; ///< Inspector reached the terminator without truncation, duplicate selected chunks or invalid record framing.
	bool common_transport_valid = false; ///< Same-file, source-era common metadata/CAPA/SDEF/SPST framing is complete; no current-world defaults.
	uint32_t feature_flags = 0; ///< Actual MLMD flags; zero for genuine older metadata absence.
	uint16_t space_content_version = 0; ///< Actual SDEF0/2/3, validated against its source-era common transport.
	SaveSupplyColumns supply_columns = SaveSupplyColumns::Absent; ///< Actual legacy B extension names and types, including empty CAPA.
	SupplyColumnEncoding cargo_supply_encoding = SupplyColumnEncoding::Absent; ///< Actual CAPA supply pair encoding, recorded even when CAPA has zero rows.
	SaveEvidenceState asteroid_origin_columns = SaveEvidenceState::Absent; ///< Valid only for exactly `asteroid_body` and `asteroid_mineral`, both U32; any other shape is Malformed.
	SupplyBookShape supply_book_shape = SupplyBookShape::Absent; ///< Actual RIFF SSOR shape; independent of the version and of chunk order.
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

/** Divergent post-fork features; existing pre425 gates keep their ordinary numeric semantics.
 * Values are appended only, and none is derived from its position: each has an explicit rule in
 * `HasSaveCapability`, and `src/tests/save_lineage_policy.cpp` asserts one expectation per value.
 */
enum class SaveCapability : uint8_t {
	OrbitalAssemblyFacilities, OrbitalManufacturingReservations, OrbitalManufacturingMaterialization,
	OrbitalManufacturingClones, SpaceReplacementSettlement, SpaceOrderedActions,
	SpaceHeavyMinerCommissioning, SpaceHeavyMinerMissions, SpaceHeavyMinerSupplies,
	SpaceHeavyMinerDescents, SpaceHeavyMinerOnboardServices, SpaceHeavyMinerContainedServices,
	SpaceCargoDimensions, OrbitalRecipeProcessing, CompanySupplyPackets, SurfaceSupplyOrdersA,
	TBMFaceTransfer, SurfaceSupplyOrdersB, GeothermalGenerators, UtilityHeatConversion,
	GeothermalGeneratorOperation, GeothermalMagmaLoop, UtilityTownService,
	UtilityLocalElectricalFeed, TubeGeothermalSources, TubeGeothermalBindings,
	AsteroidCargoOrigins, ///< Merged 446: CAPA packets carry the full asteroid body ID and mineral label.
	AsteroidFieldLifecycle, ///< Merged 446: ASTR/SPKN sparse issuance, lifecycle revision and renewal state.
	SpaceMiningContinuation, ///< Merged 446: SPOR mining policy and VEHS contained mining intent.
	SpaceLegacyDeparture, ///< Merged 446: an unordered legacy miner's local departure.
	SpaceSurveyDeparture, ///< Merged 446: an ownerless survey's Return/Waiting after release.
	Count,
};

/** Classify one complete set of detached transport facts without parsing or mutation.
 * Malformed evidence refuses first. A present marker then selects only its own merged
 * allocation (441/schema 1 or 446/schema 2); a markerless 441-445 was written by a development
 * build and is Unsupported; an unmarked 446 refuses; anything later is Unsupported; 440 and
 * below use the legacy rules.
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
