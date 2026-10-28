/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_lineage.cpp Pure preflight policy for the two preserved save histories.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-b-save-lineage.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 10; explicit per-capability rules for every writer.
 */
#ifndef SAVE_LINEAGE_STANDALONE
#include "../stdafx.h"
#endif
#include "save_lineage.h"

#include <algorithm>
#include <utility>

static constexpr uint32_t SHARED_FEATURE_FLAGS = 0x007FFFFF;
static constexpr uint32_t DONOR_FEATURE_FLAGS = 0x07FFFFFF;

/** Check whether the original stream supplied a complete selected chunk.
 * @param facts Detached source facts.
 * @param family Selected original owner.
 * @return True only for bounded valid evidence, never an absent default.
 */
static bool Present(const SaveWireFacts &facts, SaveLineageFamily family)
{
	return facts.Get(family).state == SaveEvidenceState::Valid;
}

/** Join one complete mandatory RIFF-header/TABLE family, including an empty one.
 * @param facts Same-file observations.
 * @param header Header family with independently declared row count.
 * @param rows Table family with actual decoded row count.
 * @param length Exact historical header length.
 * @param schema Expected historical header schema.
 * @return Exact types, length, revision and cardinality agreement.
 */
static bool CompletePair(const SaveWireFacts &facts, SaveLineageFamily header, SaveLineageFamily rows, uint32_t length, uint16_t schema)
{
	const auto &h = facts.Get(header);
	const auto &r = facts.Get(rows);
	return h.state == SaveEvidenceState::Valid && r.state == SaveEvidenceState::Valid &&
		h.kind == SaveEvidenceKind::Riff && r.kind == SaveEvidenceKind::Table &&
		h.length == length && h.schema == schema && h.records == r.records;
}

/** Validate the A-derived fixed dimensions and variable processing family framing.
 * @param facts Same-file detached facts.
 * @return Correct RIFF kinds and bounded minimum/explicit-count lengths when present.
 */
static bool ValidSpaceFamilies(const SaveWireFacts &facts)
{
	const auto &dimensions = facts.Get(SaveLineageFamily::CargoDimensions);
	const auto &recipes = facts.Get(SaveLineageFamily::OrbitalRecipes);
	// The actual encoded dimension count is one byte. A TABLE or overflowed
	// length cannot impersonate the separate same-file cargo-mass definition.
	if (Present(facts, SaveLineageFamily::CargoDimensions) && (dimensions.kind != SaveEvidenceKind::Riff || dimensions.records > 255 || dimensions.length != 1 + 6 * dimensions.records)) return false;
	// Empty processing still has its explicit retirement/count header; an absent
	// or truncated header cannot erase paid unfinished-work history.
	if (Present(facts, SaveLineageFamily::OrbitalRecipes) && (recipes.kind != SaveEvidenceKind::Riff || recipes.length < 11)) return false;
	return true;
}

/** Join independent metadata history bits to their exact original family counts.
 * @param facts Same-file header counts and metadata bits.
 * @param include_supply Include B's dense supply book; the merged tagged book has its own validator.
 * @return Whether every selected native history declaration matches its rows.
 */
static bool ConsistentHistoryFlags(const SaveWireFacts &facts, bool include_supply)
{
	const std::array<std::pair<SaveLineageFamily, uint32_t>, 4> flags{{
		{SaveLineageFamily::SupplyHeader, 0x00800000}, {SaveLineageFamily::GeneratorHeader, 0x01000000},
		{SaveLineageFamily::ConversionHeader, 0x02000000}, {SaveLineageFamily::TubeSourceHeader, 0x04000000},
	}};
	// Inactive machinery still retains original history. A metadata bit cannot
	// hide those rows or conjure an owner absent from the complete transport.
	for (const auto &[family, flag] : flags) {
		if (!include_supply && family == SaveLineageFamily::SupplyHeader) continue; // Merged native/legacy supply uses the canonical tagged-book history rule.
		if (((facts.feature_flags & flag) != 0) != (facts.Get(family).records != 0)) return false;
	}
	return true;
}

/** Validate the legacy-A branch-specific framing without interpreting live owners.
 * @param facts Exact detached original facts.
 * @return Whether its A-era required/forbidden families and content revision agree.
 */
static bool CompatibleA(const SaveWireFacts &facts)
{
	const auto version = facts.original_version;
	const auto &supply = facts.Get(SaveLineageFamily::SupplyOrders);
	// B's columns/feature bits cannot be erased merely because its mandatory
	// families are missing. They retain their original lineage and must refuse A.
	if ((facts.feature_flags & ~SHARED_FEATURE_FLAGS) != 0 || facts.supply_columns != SaveSupplyColumns::Absent) return false;
	// Classic has no Space content; selected legacy/current Space keeps its own
	// exact revision. This never upgrades an old paid receipt on admission.
	if (facts.space_content_version != 0 && facts.space_content_version != (version >= 431 ? 3 : 2)) return false;
	if (Present(facts, SaveLineageFamily::CargoDimensions) != (version >= 437)) return false;
	if (Present(facts, SaveLineageFamily::OrbitalRecipes) != (version >= 438)) return false;
	if (!ValidSpaceFamilies(facts)) return false;
	if (supply.state == SaveEvidenceState::Valid) {
		// A's fixed little-endian book shape has neither a B header nor TABLE
		// rows. Bound multiplication before accepting a claimed record count.
		if (version != 440 || supply.kind != SaveEvidenceKind::Riff || supply.records > 16384 || supply.length != 14 + 53 * supply.records) return false;
	} else if (version >= 440) {
		return false;
	}
	return true;
}

/** Validate all version-required B transport without substituting empty defaults.
 * @param facts Exact detached original facts.
 * @return Whether every required pair/column set and source-era schema agrees.
 */
static bool CompatibleB(const SaveWireFacts &facts)
{
	const auto version = facts.original_version;
	// B never wrote a later A envelope, content3, CDIM or SPRC; refusing these
	// protects Space data from entering the donor's older interpretation.
	if (version > 434 || (facts.space_content_version != 0 && facts.space_content_version != 2) || Present(facts, SaveLineageFamily::CargoDimensions) || Present(facts, SaveLineageFamily::OrbitalRecipes)) return false;
	const uint32_t allowed = SHARED_FEATURE_FLAGS | (version >= 426 ? 0x00800000 : 0) | (version >= 427 ? 0x01000000 : 0) | (version >= 428 ? 0x02000000 : 0) | (version >= 433 ? 0x04000000 : 0);
	if ((facts.feature_flags & ~allowed) != 0) return false;
	if ((facts.supply_columns == SaveSupplyColumns::Complete) != (version >= 426)) return false;
	// B did not have any SSOR transport before426, including an empty table.
	if (version < 426 && Present(facts, SaveLineageFamily::SupplyOrders)) return false;
	if (version >= 426) {
		if (!CompletePair(facts, SaveLineageFamily::SupplyHeader, SaveLineageFamily::SupplyOrders, 12, 1)) return false;
	}
	if (version >= 427) {
		if (!CompletePair(facts, SaveLineageFamily::GeneratorHeader, SaveLineageFamily::GeneratorRows, 8, version >= 429 ? 2 : 1)) return false;
	}
	if (version >= 428) {
		if (!CompletePair(facts, SaveLineageFamily::ConversionHeader, SaveLineageFamily::ConversionRows, 32, 1)) return false;
	}
	if (version >= 433) {
		if (!CompletePair(facts, SaveLineageFamily::TubeSourceHeader, SaveLineageFamily::TubeSourceRows, 20, 1)) return false;
	}
	return ConsistentHistoryFlags(facts, true);
}

/** @copydoc ClassifySaveDialect */
SaveLineageResult ClassifySaveDialect(const SaveWireFacts &facts, const std::optional<SaveOriginHint> &origin)
{
	/** @param status Admission/refusal outcome. @param dialect Accepted interpretation, otherwise inert. @return Result retaining the unchanged original version. */
	const auto result = [&](SaveLineageStatus status, SaveDialect dialect = SaveDialect::Shared) {
		return SaveLineageResult{status, dialect, facts.original_version};
	};
	// Any malformed observation refuses before an origin hint or numeric range
	// can reinterpret it. Whole-envelope and common transport checks are mandatory.
	if (!facts.framing_complete || !facts.common_transport_valid || facts.supply_columns == SaveSupplyColumns::Malformed || facts.payload_dialect == SavePayloadDialect::Malformed) return result(SaveLineageStatus::Corrupt);
	// Every observed selected family participates, even one whose feature is
	// inactive: a malformed empty family cannot silently become an absent owner.
	for (const auto &family : facts.families) {
		if (family.state == SaveEvidenceState::Malformed) return result(SaveLineageStatus::Corrupt);
	}
	if (facts.original_version > MERGED_SAVE_LINEAGE_VERSION) return result(SaveLineageStatus::Unsupported);
	if ((facts.feature_flags & ~DONOR_FEATURE_FLAGS) != 0) return result(SaveLineageStatus::Corrupt);
	const auto &marker = facts.Get(SaveLineageFamily::Marker);
	// A marker is an explicit writer contract. Neither a wrong legacy placement
	// nor a partially assembled future family may fall back to old decoding.
	if (marker.state != SaveEvidenceState::Absent || facts.original_version == MERGED_SAVE_LINEAGE_VERSION) {
		if (facts.original_version != MERGED_SAVE_LINEAGE_VERSION || marker.state != SaveEvidenceState::Valid || marker.kind != SaveEvidenceKind::Riff || marker.length != 8 ||
			facts.marker_magic != MERGED_SAVE_LINEAGE_MAGIC || facts.marker_schema != MERGED_SAVE_LINEAGE_SCHEMA || facts.marker_writer_version != facts.original_version ||
			!facts.merged_supply_valid || !facts.merged_cargo_valid) return result(SaveLineageStatus::Corrupt);
		// New supply/cargo validators do not replace the other complete retained
		// families. A marker cannot erase geothermal or Space processing history.
		if ((facts.space_content_version != 0 && facts.space_content_version != 3) || !ValidSpaceFamilies(facts) ||
			!Present(facts, SaveLineageFamily::CargoDimensions) || !Present(facts, SaveLineageFamily::OrbitalRecipes) ||
			!Present(facts, SaveLineageFamily::SupplyOrders) || facts.Get(SaveLineageFamily::SupplyOrders).kind != SaveEvidenceKind::Riff || Present(facts, SaveLineageFamily::SupplyHeader) ||
			!CompletePair(facts, SaveLineageFamily::GeneratorHeader, SaveLineageFamily::GeneratorRows, 8, 2) ||
			!CompletePair(facts, SaveLineageFamily::ConversionHeader, SaveLineageFamily::ConversionRows, 32, 1) ||
			!CompletePair(facts, SaveLineageFamily::TubeSourceHeader, SaveLineageFamily::TubeSourceRows, 20, 1) || !ConsistentHistoryFlags(facts, false)) return result(SaveLineageStatus::Corrupt);
		// The sole canonical issuer retains paid history even after terminal rows
		// are pruned; row cardinality cannot clear or invent its metadata marker.
		if (facts.merged_supply_next_id == 0 || ((facts.feature_flags & (1U << 23)) != 0) != (facts.merged_supply_next_id != 1)) return result(SaveLineageStatus::Corrupt);
		return result(SaveLineageStatus::Accepted, SaveDialect::Merged);
	}
	const auto &supply = facts.Get(SaveLineageFamily::SupplyOrders);
	const bool a_evidence = facts.payload_dialect == SavePayloadDialect::DestinationA || facts.space_content_version == 3 ||
		Present(facts, SaveLineageFamily::CargoDimensions) || Present(facts, SaveLineageFamily::OrbitalRecipes) ||
		(supply.state == SaveEvidenceState::Valid && supply.kind == SaveEvidenceKind::Riff);
	const bool b_evidence = facts.payload_dialect == SavePayloadDialect::DonorB || (facts.feature_flags & ~SHARED_FEATURE_FLAGS) != 0 || facts.supply_columns == SaveSupplyColumns::Complete ||
		Present(facts, SaveLineageFamily::SupplyHeader) || (supply.state == SaveEvidenceState::Valid && supply.kind == SaveEvidenceKind::Table) ||
		Present(facts, SaveLineageFamily::GeneratorHeader) || Present(facts, SaveLineageFamily::GeneratorRows) ||
		Present(facts, SaveLineageFamily::ConversionHeader) || Present(facts, SaveLineageFamily::ConversionRows) ||
		Present(facts, SaveLineageFamily::TubeSourceHeader) || Present(facts, SaveLineageFamily::TubeSourceRows);
	if (a_evidence && b_evidence) return result(SaveLineageStatus::Corrupt); // Two incompatible owners cannot share one original envelope.
	// Families cannot appear before their branch introduced them, even when a
	// forged header claims an empty ledger that might otherwise look harmless.
	const std::array<std::pair<SaveLineageFamily, uint16_t>, 7> introductions{{
		{SaveLineageFamily::SupplyHeader, uint16_t{426}}, {SaveLineageFamily::GeneratorHeader, uint16_t{427}}, {SaveLineageFamily::GeneratorRows, uint16_t{427}},
		{SaveLineageFamily::ConversionHeader, uint16_t{428}}, {SaveLineageFamily::ConversionRows, uint16_t{428}},
		{SaveLineageFamily::TubeSourceHeader, uint16_t{433}}, {SaveLineageFamily::TubeSourceRows, uint16_t{433}},
	}};
	for (const auto &[family, version] : introductions) {
		if (Present(facts, family) && facts.original_version < version) return result(SaveLineageStatus::Corrupt); // Source-era absence cannot acquire a future owner.
	}
	if (facts.original_version <= 424) {
		if (a_evidence || b_evidence) return result(SaveLineageStatus::Corrupt); // Shared history predates every divergent transport meaning.
		return result(SaveLineageStatus::Accepted, SaveDialect::Shared);
	}
	if (b_evidence) return result(CompatibleB(facts) ? SaveLineageStatus::Accepted : SaveLineageStatus::Corrupt, SaveDialect::DonorB);
	// Versions435-440 are within A's pinned historical writer range. Their
	// required content/families still validate; a high number alone is insufficient.
	if (a_evidence || facts.original_version >= 435) return result(CompatibleA(facts) ? SaveLineageStatus::Accepted : SaveLineageStatus::Corrupt, SaveDialect::DestinationA);
	if (!CompatibleA(facts)) return result(SaveLineageStatus::Corrupt);
	/** @param byte One original SHA256 byte. @return Whether this byte prevents an unbound all-zero identity. */
	const auto nonzero = [](uint8_t byte) { return byte != 0; };
	const bool bound = std::ranges::any_of(facts.source_digest, nonzero);
	// Absent B families never prove A. Only an exact nonzero digest hint can
	// choose among common-layout legacy originals, and its dialect must still fit.
	if (origin && bound && origin->source_digest == facts.source_digest) {
		if (origin->dialect == SaveDialect::DestinationA) return result(SaveLineageStatus::Accepted, SaveDialect::DestinationA);
		if (origin->dialect == SaveDialect::DonorB) return result(CompatibleB(facts) ? SaveLineageStatus::Accepted : SaveLineageStatus::Corrupt, SaveDialect::DonorB);
	}
	return result(SaveLineageStatus::Ambiguous);
}

/** First post-fork envelope version of both historical branches; 424 and older is shared history. */
static constexpr uint16_t FIRST_DIVERGENT_VERSION = 425;
static constexpr uint16_t LAST_DESTINATION_A_VERSION = 440; ///< Last version the historical destination writer produced.
static constexpr uint16_t LAST_DONOR_B_VERSION = 434; ///< Last version the historical donor writer produced.
static constexpr uint16_t FIRST_SPACE_FIXES_VERSION = 441; ///< First Space fixes era; that writer descends from A440.
static constexpr uint16_t LAST_SPACE_FIXES_VERSION = 445; ///< Last version the Space fixes writer produced.

/** One capability's availability in every writer, looked up by capability and never by enum position.
 * A first version of zero means that writer never had the feature at any version.
 */
struct CapabilityRule {
	SaveCapability capability; ///< The feature this row describes.
	uint16_t first_a; ///< First historical DestinationA version, or 0.
	uint16_t first_b; ///< First historical DonorB version, or 0.
	uint16_t first_space; ///< First SpaceFixes version, or 0. A features start at 441 because that writer began from A440.
	bool merged441; ///< Carried by merged 441 with MLLN schema 1, whose meaning is frozen.
	bool merged446; ///< Carried by merged 446 with MLLN schema 2, the combined writer of m2o28 Group 22.
};

/** Source-defined availability of every capability. Appending a capability requires a row here; the
 * static_asserts below reject a missing or duplicated row, and each row chooses its merged 441 value explicitly.
 */
static constexpr CapabilityRule CAPABILITY_RULES[] = {
	{SaveCapability::OrbitalAssemblyFacilities, 425, 0, 441, true, true},
	{SaveCapability::OrbitalManufacturingReservations, 426, 0, 441, true, true},
	{SaveCapability::OrbitalManufacturingMaterialization, 427, 0, 441, true, true},
	{SaveCapability::OrbitalManufacturingClones, 428, 0, 441, true, true},
	{SaveCapability::SpaceReplacementSettlement, 429, 0, 441, true, true},
	{SaveCapability::SpaceOrderedActions, 430, 0, 441, true, true},
	{SaveCapability::SpaceHeavyMinerCommissioning, 431, 0, 441, true, true},
	{SaveCapability::SpaceHeavyMinerMissions, 432, 0, 441, true, true},
	{SaveCapability::SpaceHeavyMinerSupplies, 433, 0, 441, true, true},
	{SaveCapability::SpaceHeavyMinerDescents, 434, 0, 441, true, true},
	{SaveCapability::SpaceHeavyMinerOnboardServices, 435, 0, 441, true, true},
	{SaveCapability::SpaceHeavyMinerContainedServices, 436, 0, 441, true, true},
	{SaveCapability::SpaceCargoDimensions, 437, 0, 441, true, true},
	{SaveCapability::OrbitalRecipeProcessing, 438, 0, 441, true, true},
	{SaveCapability::CompanySupplyPackets, 439, 0, 441, true, true},
	{SaveCapability::SurfaceSupplyOrdersA, 440, 0, 441, true, true},
	{SaveCapability::TBMFaceTransfer, 0, 425, 0, true, true},
	{SaveCapability::SurfaceSupplyOrdersB, 0, 426, 0, true, true},
	{SaveCapability::GeothermalGenerators, 0, 427, 0, true, true},
	{SaveCapability::UtilityHeatConversion, 0, 428, 0, true, true},
	{SaveCapability::GeothermalGeneratorOperation, 0, 429, 0, true, true},
	{SaveCapability::GeothermalMagmaLoop, 0, 430, 0, true, true},
	{SaveCapability::UtilityTownService, 0, 431, 0, true, true},
	{SaveCapability::UtilityLocalElectricalFeed, 0, 432, 0, true, true},
	{SaveCapability::TubeGeothermalSources, 0, 433, 0, true, true},
	{SaveCapability::TubeGeothermalBindings, 0, 434, 0, true, true},
	{SaveCapability::AsteroidCargoOrigins, 0, 0, 441, false, true},
	{SaveCapability::AsteroidFieldLifecycle, 0, 0, 442, false, true},
	{SaveCapability::SpaceMiningContinuation, 0, 0, 443, false, true},
	{SaveCapability::SpaceLegacyDeparture, 0, 0, 444, false, true},
	{SaveCapability::SpaceSurveyDeparture, 0, 0, 445, false, true},
};

/** Check at compile time that every declared capability has exactly one rule.
 * @return True when each value below Count occurs once in CAPABILITY_RULES.
 */
static constexpr bool EveryCapabilityHasOneRule()
{
	// Coverage only: walk the declared values and count their rows. The lookup in
	// HasSaveCapability still matches by capability, so row order carries no meaning.
	for (uint8_t value = 0; value < static_cast<uint8_t>(SaveCapability::Count); ++value) {
		if (std::ranges::count(CAPABILITY_RULES, static_cast<SaveCapability>(value), &CapabilityRule::capability) != 1) return false;
	}
	return true;
}
static_assert(std::size(CAPABILITY_RULES) == static_cast<size_t>(SaveCapability::Count));
static_assert(EveryCapabilityHasOneRule());

/** Check that a writer really produced an envelope version before any rule is consulted.
 * @param dialect Accepted original writer.
 * @param version Unchanged original envelope version.
 * @return True only inside that writer's closed range; Shared history has no post-fork capability.
 */
static bool WriterProducedVersion(SaveDialect dialect, uint16_t version)
{
	switch (dialect) {
		case SaveDialect::DestinationA: return version >= FIRST_DIVERGENT_VERSION && version <= LAST_DESTINATION_A_VERSION;
		case SaveDialect::DonorB: return version >= FIRST_DIVERGENT_VERSION && version <= LAST_DONOR_B_VERSION;
		case SaveDialect::SpaceFixes: return version >= FIRST_SPACE_FIXES_VERSION && version <= LAST_SPACE_FIXES_VERSION;
		// The merged writer has exactly two allocations; 442-445 are Space fixes numbers, never merged ones.
		case SaveDialect::Merged: return version == MERGED_SAVE_LINEAGE_VERSION || version == CURRENT_MERGED_SAVE_VERSION;
		case SaveDialect::Shared: return false;
	}
	return false; // A value outside the declared dialects names no writer.
}

/** @copydoc HasSaveCapability */
bool HasSaveCapability(const SaveLineageResult &result, SaveCapability capability)
{
	// Refusals grant nothing, even if a caller retained a previous successful
	// dialect alongside this rejected attempt's version. A version outside the
	// writer's own range would otherwise borrow another writer's meaning.
	if (result.status != SaveLineageStatus::Accepted || !WriterProducedVersion(result.dialect, result.original_version)) return false;
	const auto rule = std::ranges::find(CAPABILITY_RULES, capability, &CapabilityRule::capability);
	if (rule == std::end(CAPABILITY_RULES)) return false; // A future enum value has no rule yet and grants nothing.
	const auto version = result.original_version;
	switch (result.dialect) {
		// Historical writers gained features in order: a nonzero introduction at or
		// below this file's version means the file carries the feature's wire.
		case SaveDialect::DestinationA: return rule->first_a != 0 && version >= rule->first_a;
		case SaveDialect::DonorB: return rule->first_b != 0 && version >= rule->first_b;
		case SaveDialect::SpaceFixes: return rule->first_space != 0 && version >= rule->first_space;
		// Merged membership is a fixed set per allocation, not a numeric threshold.
		case SaveDialect::Merged: return version == MERGED_SAVE_LINEAGE_VERSION ? rule->merged441 : rule->merged446;
		case SaveDialect::Shared: return false;
	}
	return false;
}
