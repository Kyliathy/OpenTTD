/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_lineage_policy.cpp Detached ambiguity, corruption and capability cases.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-b-save-lineage.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Groups 9-10; the closed Space fixes capability matrix.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 11; descriptor-level CAPA observations refuse when malformed.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 12; malformed or ambiguous SSOR book shapes refuse.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Groups 13 and 17B; merged 441 and 446 accepted, development-build 441-445 refused.
 * SAVE_LINEAGE_STANDALONE builds this pure Catch leaf without native game pools.
 * The ordinary native test target registers this leaf through src/tests/CMakeLists.txt.
 */
#ifdef SAVE_LINEAGE_STANDALONE
#define CATCH_CONFIG_MAIN
#else
#include "../stdafx.h"
#endif
#include "../3rdparty/catch2/catch.hpp"
#include "../saveload/save_lineage.h"

/** Access one explicitly selected test family without a native chunk handler.
 * @param facts Detached fixture being authored.
 * @param family Exact bounded family enum.
 * @return Mutable fixture evidence; this owns no gameplay state.
 */
static SaveChunkEvidence &Evidence(SaveWireFacts &facts, SaveLineageFamily family)
{
	return facts.families[static_cast<uint8_t>(family)];
}

/** Declare a completely bounded chunk fact for pure policy cases.
 * @param facts Detached fixture being authored.
 * @param family Existing owner family.
 * @param kind Actual conceptual wire kind.
 * @param length Exact RIFF length, or zero for a table.
 * @param schema Header revision, or zero for unversioned/table evidence.
 * @param records Header-declared or actual table count.
 */
static void AddChunk(SaveWireFacts &facts, SaveLineageFamily family, SaveEvidenceKind kind, uint32_t length, uint16_t schema = 0, uint32_t records = 0)
{
	Evidence(facts, family) = {SaveEvidenceState::Valid, kind, length, records, schema};
}

/** Build source-era common transport with a nonzero explicit original identity.
 * @param version Original envelope version; no real save bytes are modified.
 * @return Detached common facts with no positive branch evidence.
 */
static SaveWireFacts Common(uint16_t version)
{
	SaveWireFacts facts;
	facts.original_version = version;
	facts.framing_complete = facts.common_transport_valid = true;
	facts.space_content_version = 2;
	facts.source_digest[0] = 0x31;
	facts.source_digest[31] = 0x97;
	return facts;
}

/** Build A's empty mandatory transport for a particular known source era.
 * @param version Original A version, including source-defined439 without a native fixture claim.
 * @return Pure A facts; pre431 may intentionally remain ambiguous without a hint.
 */
static SaveWireFacts Destination(uint16_t version)
{
	auto facts = Common(version);
	if (version >= 431) facts.space_content_version = 3;
	if (version >= 437) AddChunk(facts, SaveLineageFamily::CargoDimensions, SaveEvidenceKind::Riff, 1);
	if (version >= 438) AddChunk(facts, SaveLineageFamily::OrbitalRecipes, SaveEvidenceKind::Riff, 11);
	if (version >= 440) {
		AddChunk(facts, SaveLineageFamily::SupplyOrders, SaveEvidenceKind::Riff, 14);
		facts.supply_book_shape = SupplyBookShape::Fixed; // The scanner reads A's empty fixed book by its own shape.
	}
	return facts;
}

/** Build B's complete empty families, preserving each original schema boundary.
 * @param version Original B version425 through434.
 * @return Detached B facts;425 uses positively decoded face-layout evidence.
 */
static SaveWireFacts Donor(uint16_t version)
{
	auto facts = Common(version);
	if (version == 425) facts.payload_dialect = SavePayloadDialect::DonorB;
	if (version >= 426) {
		facts.supply_columns = SaveSupplyColumns::Complete;
		AddChunk(facts, SaveLineageFamily::SupplyHeader, SaveEvidenceKind::Riff, 12, 1);
		AddChunk(facts, SaveLineageFamily::SupplyOrders, SaveEvidenceKind::Table, 0);
	}
	if (version >= 427) {
		AddChunk(facts, SaveLineageFamily::GeneratorHeader, SaveEvidenceKind::Riff, 8, version >= 429 ? 2 : 1);
		AddChunk(facts, SaveLineageFamily::GeneratorRows, SaveEvidenceKind::Table, 0);
	}
	if (version >= 428) {
		AddChunk(facts, SaveLineageFamily::ConversionHeader, SaveEvidenceKind::Riff, 32, 1);
		AddChunk(facts, SaveLineageFamily::ConversionRows, SaveEvidenceKind::Table, 0);
	}
	if (version >= 433) {
		AddChunk(facts, SaveLineageFamily::TubeSourceHeader, SaveEvidenceKind::Riff, 20, 1);
		AddChunk(facts, SaveLineageFamily::TubeSourceRows, SaveEvidenceKind::Table, 0);
	}
	return facts;
}

/** Build the explicit merged detached contract independently of serialization.
 * @return Complete merged facts whose new supply/CAPA validators have admitted their own schemas.
 */
static SaveWireFacts Merged()
{
	auto facts = Donor(434);
	facts.original_version = MERGED_SAVE_LINEAGE_VERSION;
	facts.space_content_version = 3;
	facts.supply_columns = SaveSupplyColumns::Absent;
	Evidence(facts, SaveLineageFamily::SupplyHeader) = {};
	AddChunk(facts, SaveLineageFamily::SupplyOrders, SaveEvidenceKind::Riff, 1);
	facts.supply_book_shape = SupplyBookShape::Tagged;
	AddChunk(facts, SaveLineageFamily::CargoDimensions, SaveEvidenceKind::Riff, 1);
	AddChunk(facts, SaveLineageFamily::OrbitalRecipes, SaveEvidenceKind::Riff, 11);
	AddChunk(facts, SaveLineageFamily::Marker, SaveEvidenceKind::Riff, 8);
	facts.marker_magic = MERGED_SAVE_LINEAGE_MAGIC;
	facts.marker_schema = MERGED_SAVE_LINEAGE_SCHEMA;
	facts.marker_writer_version = MERGED_SAVE_LINEAGE_VERSION;
	facts.merged_supply_valid = facts.merged_cargo_valid = true;
	facts.merged_supply_next_id = 1;
	return facts;
}

/** Build the reserved combined writer's facts: merged 441 plus its marker schema 2 and typed CAPA origins.
 * @return Complete merged 446 facts, independent of any serializer.
 */
static SaveWireFacts Merged446()
{
	auto facts = Merged();
	facts.original_version = facts.marker_writer_version = CURRENT_MERGED_SAVE_VERSION;
	facts.marker_schema = CURRENT_MERGED_SAVE_LINEAGE_SCHEMA;
	facts.asteroid_origin_columns = SaveEvidenceState::Valid;
	return facts;
}

/** Build the facts of a file a development build wrote: A440's transport plus typed CAPA origins, no marker.
 * @param version Development-build number, 441-445.
 * @return Detached facts with the A440-derived fixed book and no B or merged evidence; the classifier must refuse them.
 */
static SaveWireFacts DevelopmentBuild(uint16_t version)
{
	auto facts = Destination(440);
	facts.original_version = version;
	facts.asteroid_origin_columns = SaveEvidenceState::Valid;
	return facts;
}

/** Independent source-enum expectation; do not derive thresholds from capability ordinals. */
struct CapabilityExpectation {
	SaveCapability capability; ///< Named historical feature, independent of declaration order.
	SaveDialect dialect; ///< Only this original branch introduced the feature.
	uint16_t first_version; ///< Explicit original SaveLoadVersion value in the audited handoff.
};

/** Source-defined thresholds from the historical writers and the merged writer.
 * Merged 441 contains the A and B union; merged 446 adds the five Space eras, listed with dialect Merged.
 */
static constexpr CapabilityExpectation CAPABILITY_EXPECTATIONS[] = {
	{SaveCapability::OrbitalAssemblyFacilities, SaveDialect::DestinationA, 425},
	{SaveCapability::OrbitalManufacturingReservations, SaveDialect::DestinationA, 426},
	{SaveCapability::OrbitalManufacturingMaterialization, SaveDialect::DestinationA, 427},
	{SaveCapability::OrbitalManufacturingClones, SaveDialect::DestinationA, 428},
	{SaveCapability::SpaceReplacementSettlement, SaveDialect::DestinationA, 429},
	{SaveCapability::SpaceOrderedActions, SaveDialect::DestinationA, 430},
	{SaveCapability::SpaceHeavyMinerCommissioning, SaveDialect::DestinationA, 431},
	{SaveCapability::SpaceHeavyMinerMissions, SaveDialect::DestinationA, 432},
	{SaveCapability::SpaceHeavyMinerSupplies, SaveDialect::DestinationA, 433},
	{SaveCapability::SpaceHeavyMinerDescents, SaveDialect::DestinationA, 434},
	{SaveCapability::SpaceHeavyMinerOnboardServices, SaveDialect::DestinationA, 435},
	{SaveCapability::SpaceHeavyMinerContainedServices, SaveDialect::DestinationA, 436},
	{SaveCapability::SpaceCargoDimensions, SaveDialect::DestinationA, 437},
	{SaveCapability::OrbitalRecipeProcessing, SaveDialect::DestinationA, 438},
	{SaveCapability::CompanySupplyPackets, SaveDialect::DestinationA, 439},
	{SaveCapability::SurfaceSupplyOrdersA, SaveDialect::DestinationA, 440},
	{SaveCapability::TBMFaceTransfer, SaveDialect::DonorB, 425},
	{SaveCapability::SurfaceSupplyOrdersB, SaveDialect::DonorB, 426},
	{SaveCapability::GeothermalGenerators, SaveDialect::DonorB, 427},
	{SaveCapability::UtilityHeatConversion, SaveDialect::DonorB, 428},
	{SaveCapability::GeothermalGeneratorOperation, SaveDialect::DonorB, 429},
	{SaveCapability::GeothermalMagmaLoop, SaveDialect::DonorB, 430},
	{SaveCapability::UtilityTownService, SaveDialect::DonorB, 431},
	{SaveCapability::UtilityLocalElectricalFeed, SaveDialect::DonorB, 432},
	{SaveCapability::TubeGeothermalSources, SaveDialect::DonorB, 433},
	{SaveCapability::TubeGeothermalBindings, SaveDialect::DonorB, 434},
	{SaveCapability::AsteroidCargoOrigins, SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION},
	{SaveCapability::AsteroidFieldLifecycle, SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION},
	{SaveCapability::SpaceMiningContinuation, SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION},
	{SaveCapability::SpaceLegacyDeparture, SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION},
	{SaveCapability::SpaceSurveyDeparture, SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION},
};
static_assert(std::size(CAPABILITY_EXPECTATIONS) == static_cast<uint8_t>(SaveCapability::Count));

/** The five Space capabilities that merged 446 adds, in the column order of the m2o28 Group 9 matrix. */
static constexpr std::array<SaveCapability, 5> SPACE_CAPABILITIES{
	SaveCapability::AsteroidCargoOrigins, SaveCapability::AsteroidFieldLifecycle, SaveCapability::SpaceMiningContinuation,
	SaveCapability::SpaceLegacyDeparture, SaveCapability::SpaceSurveyDeparture,
};

/** @param expected One expectation row. @return Whether it is one of the five Space capabilities that only merged 446 carries. */
static bool IsSpaceCapability(const CapabilityExpectation &expected)
{
	return std::ranges::find(SPACE_CAPABILITIES, expected.capability) != SPACE_CAPABILITIES.end();
}

/** One source row of the closed matrix, written out literally rather than derived from the thresholds above. */
struct SourceMatrixRow {
	SaveDialect dialect; ///< Original writer of the source.
	uint16_t version; ///< Unchanged original envelope version.
	std::array<bool, 5> carries; ///< Typed origin, renewal, mining intent, unqueued departure, released survey return.
};

/** The closed source-format matrix (m2o28 Group 9, reduced in Group 17B): any other source grants none of the five. */
static constexpr SourceMatrixRow SOURCE_MATRIX[] = {
	{SaveDialect::Merged, MERGED_SAVE_LINEAGE_VERSION, {false, false, false, false, false}},
	{SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION, {true, true, true, true, true}},
};

/** Name an admitted source directly, for writers the classifier does not admit yet.
 * The capability policy is pure, so Groups 9-10 test it without wire facts; Group 13
 * composes the classifier that produces these results from real bytes.
 * @param dialect Original writer.
 * @param version Unchanged original envelope version.
 * @return An accepted classification of that writer and version.
 */
static SaveLineageResult AcceptedSource(SaveDialect dialect, uint16_t version)
{
	return {SaveLineageStatus::Accepted, dialect, version};
}

/** Every original numeric collision must retain the selected branch's feature meanings. */
TEST_CASE("Save lineage separates every original A and B capability boundary", "[save-lineage]")
{
	// Exercise every original threshold independently; A439 is a source-defined
	// pure-policy case and deliberately does not claim a fabricated native save.
	for (uint16_t version = 425; version <= 440; ++version) {
		auto facts = Destination(version);
		const auto result = ClassifySaveDialect(facts, SaveOriginHint{SaveDialect::DestinationA, facts.source_digest});
		REQUIRE(result.status == SaveLineageStatus::Accepted);
		CHECK(result.dialect == SaveDialect::DestinationA);
		CHECK(result.original_version == version);
		// Check the entire divergent vocabulary so a B enum with the same number
		// cannot accidentally become enabled by an A version comparison.
		for (const auto &expected : CAPABILITY_EXPECTATIONS) {
			CHECK(HasSaveCapability(result, expected.capability) ==
				(expected.dialect == SaveDialect::DestinationA && version >= expected.first_version));
		}
	}
	// B's original thresholds enable only its delivered underground meanings;
	// the same numeric versions must not enable A's later Space features.
	for (uint16_t version = 425; version <= 434; ++version) {
		const auto result = ClassifySaveDialect(Donor(version));
		REQUIRE(result.status == SaveLineageStatus::Accepted);
		CHECK(result.dialect == SaveDialect::DonorB);
		// Cover the whole vocabulary, including every A capability that stays off.
		for (const auto &expected : CAPABILITY_EXPECTATIONS) {
			CHECK(HasSaveCapability(result, expected.capability) ==
				(expected.dialect == SaveDialect::DonorB && version >= expected.first_version));
		}
	}
}

/** Shared history is unchanged; ambiguous common-layout originals require an exact origin. */
TEST_CASE("Save lineage keeps shared history and refuses guessed legacy origins", "[save-lineage]")
{
	CHECK(ClassifySaveDialect(Common(424)).dialect == SaveDialect::Shared);
	// Empty/common owner layouts remain ambiguous throughout the colliding era;
	// lack of B's later family cannot itself prove that the source was A.
	for (uint16_t version = 425; version <= 430; ++version) {
		auto facts = Common(version);
		CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Ambiguous);
		auto hint = SaveOriginHint{SaveDialect::DestinationA, facts.source_digest};
		CHECK(ClassifySaveDialect(facts, hint).dialect == SaveDialect::DestinationA);
		hint.source_digest[31] ^= 1;
		CHECK(ClassifySaveDialect(facts, hint).status == SaveLineageStatus::Ambiguous);
		facts.source_digest.fill(0); hint.source_digest.fill(0);
		CHECK(ClassifySaveDialect(facts, hint).status == SaveLineageStatus::Ambiguous);
	}
	auto facts = Common(425);
	CHECK(ClassifySaveDialect(facts, SaveOriginHint{SaveDialect::DonorB, facts.source_digest}).dialect == SaveDialect::DonorB);
	facts.original_version = 426;
	CHECK(ClassifySaveDialect(facts, SaveOriginHint{SaveDialect::DonorB, facts.source_digest}).status == SaveLineageStatus::Corrupt);
}

/** A present B marker or field prevents a broken B save from falling through to A. */
TEST_CASE("Save lineage rejects missing or contradictory B families", "[save-lineage]")
{
	// Remove each independent mandatory component, including empty tables. The
	// remaining explicit B transport must expose corruption rather than default it.
	for (auto family : {SaveLineageFamily::SupplyHeader, SaveLineageFamily::SupplyOrders,
		SaveLineageFamily::GeneratorHeader, SaveLineageFamily::GeneratorRows,
		SaveLineageFamily::ConversionHeader, SaveLineageFamily::ConversionRows,
		SaveLineageFamily::TubeSourceHeader, SaveLineageFamily::TubeSourceRows}) {
		auto facts = Donor(434);
		Evidence(facts, family) = {};
		CHECK(ClassifySaveDialect(facts, SaveOriginHint{SaveDialect::DestinationA, facts.source_digest}).status == SaveLineageStatus::Corrupt);
	}
	auto facts = Donor(426);
	Evidence(facts, SaveLineageFamily::SupplyOrders).kind = SaveEvidenceKind::Riff;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Donor(434); facts.space_content_version = 3;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Donor(426); facts.supply_columns = SaveSupplyColumns::Malformed;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Donor(427); Evidence(facts, SaveLineageFamily::GeneratorHeader).schema = 2;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Donor(434); Evidence(facts, SaveLineageFamily::TubeSourceRows).records = 1;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Donor(426); facts.feature_flags |= 0x01000000;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Common(425); AddChunk(facts, SaveLineageFamily::SupplyOrders, SaveEvidenceKind::Table, 0);
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
}

/** Wire failures and premature later meaning refuse before normal native initialization. */
TEST_CASE("Save lineage rejects malformed A transport and future capability guesses", "[save-lineage]")
{
	auto facts = Destination(440); facts.framing_complete = false;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Destination(440); Evidence(facts, SaveLineageFamily::SupplyOrders).length = 15;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Destination(437); Evidence(facts, SaveLineageFamily::CargoDimensions).kind = SaveEvidenceKind::Table;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Destination(438); Evidence(facts, SaveLineageFamily::OrbitalRecipes).state = SaveEvidenceState::Malformed;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Common(425); facts.space_content_version = 3;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Destination(440); facts.original_version = 447;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Unsupported);
	// A markerless 442 is a development build's number, never read (m2o28 Group 17B).
	facts = Destination(440); facts.original_version = 442;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Unsupported);
	// Malformed CAPA descriptor observations win over every version or hint rule.
	facts = Destination(440); facts.cargo_supply_encoding = SupplyColumnEncoding::Malformed;
	CHECK(ClassifySaveDialect(facts, SaveOriginHint{SaveDialect::DestinationA, facts.source_digest}).status == SaveLineageStatus::Corrupt);
	facts = Destination(440); facts.asteroid_origin_columns = SaveEvidenceState::Malformed;
	CHECK(ClassifySaveDialect(facts, SaveOriginHint{SaveDialect::DestinationA, facts.source_digest}).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.asteroid_origin_columns = SaveEvidenceState::Malformed;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	// An SSOR that fits no book refuses even where the rest is complete. An
	// overlap of both books is settled by the writer: the marker selects the
	// tagged book, while no markerless writer ever wrote the SSM1 signature.
	facts = Merged(); facts.supply_book_shape = SupplyBookShape::Malformed;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Destination(440); facts.supply_book_shape = SupplyBookShape::Malformed;
	CHECK(ClassifySaveDialect(facts, SaveOriginHint{SaveDialect::DestinationA, facts.source_digest}).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.supply_book_shape = SupplyBookShape::Ambiguous;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Accepted);
	facts = Destination(440); facts.supply_book_shape = SupplyBookShape::Ambiguous;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = DevelopmentBuild(441); facts.supply_book_shape = SupplyBookShape::Ambiguous;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Unsupported); // Never read, whatever its book.
	facts = Merged(); facts.supply_book_shape = SupplyBookShape::Fixed;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt); // The marked writer needs its tagged book.
	facts = Destination(440); facts.supply_book_shape = SupplyBookShape::Tagged;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt); // A440 never wrote SSM1.
	CHECK_FALSE(HasSaveCapability(ClassifySaveDialect(Common(425)), SaveCapability::TBMFaceTransfer));
	CHECK_FALSE(HasSaveCapability(ClassifySaveDialect(Destination(440)), static_cast<SaveCapability>(255)));
}

/** The explicit marker grants only the fully supplied merged transport, never gameplay ownership. */
TEST_CASE("Save lineage validates the reserved merged marker and complete family boundary", "[save-lineage]")
{
	auto facts = Merged();
	const auto result = ClassifySaveDialect(facts);
	REQUIRE(result.status == SaveLineageStatus::Accepted);
	CHECK(result.dialect == SaveDialect::Merged);
	// The first combined writer supplies every A and B capability. The Space
	// eras came later with 446; the matrix case checks that 441 lacks them.
	for (const auto &expected : CAPABILITY_EXPECTATIONS) {
		if (!IsSpaceCapability(expected)) CHECK(HasSaveCapability(result, expected.capability));
	}
	// Unknown enum values still refuse independently of the complete marker.
	CHECK_FALSE(HasSaveCapability(result, static_cast<SaveCapability>(255)));
	facts.marker_magic ^= 1; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.marker_schema = 2; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.marker_writer_version = 440; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.original_version = 440; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.merged_supply_valid = false; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.merged_cargo_valid = false; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); Evidence(facts, SaveLineageFamily::GeneratorRows) = {}; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.feature_flags = 0x01000000; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	// Issuance history can outlive terminal rows. Neither an omitted marker nor
	// a fabricated paid-history bit may replace the actual canonical frontier.
	facts = Merged(); facts.merged_supply_next_id = 2; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts.feature_flags |= 1U << 23; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Accepted);
	facts.merged_supply_next_id = 1; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.merged_supply_next_id = 0; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.feature_flags |= 1U << 27; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);

}

/** The closed matrix: merged 441 carries none of the five Space eras and 446 carries all; both keep all of A and B. */
TEST_CASE("Save lineage freezes the merged 441 and 446 capability matrix", "[save-lineage]")
{
	for (const auto &row : SOURCE_MATRIX) {
		const auto source = AcceptedSource(row.dialect, row.version);
		// The five Space capabilities follow the literal row, not a version comparison.
		for (size_t column = 0; column < SPACE_CAPABILITIES.size(); ++column) {
			CHECK(HasSaveCapability(source, SPACE_CAPABILITIES[column]) == row.carries[column]);
		}
		// Both merged allocations carry every older A and B capability.
		for (const auto &expected : CAPABILITY_EXPECTATIONS) {
			if (!IsSpaceCapability(expected)) CHECK(HasSaveCapability(source, expected.capability));
		}
	}
}

/** The Space eras exist only in merged 446; no historical writer, other number or refused attempt has them. */
TEST_CASE("Save lineage grants the Space eras only to merged 446", "[save-lineage]")
{
	const auto merged441 = AcceptedSource(SaveDialect::Merged, MERGED_SAVE_LINEAGE_VERSION);
	const auto merged446 = AcceptedSource(SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION);
	// Each Space capability against both merged allocations and the last historical files.
	for (const auto capability : SPACE_CAPABILITIES) {
		CHECK_FALSE(HasSaveCapability(merged441, capability));
		CHECK(HasSaveCapability(merged446, capability));
		CHECK_FALSE(HasSaveCapability(AcceptedSource(SaveDialect::DestinationA, 440), capability));
		CHECK_FALSE(HasSaveCapability(AcceptedSource(SaveDialect::DonorB, 434), capability));
	}
	// A version outside its writer's closed range borrows no other writer's meaning:
	// merged 442-445 and 447, A441 and B435 were never written by those writers.
	const SaveLineageResult foreign[] = {
		AcceptedSource(SaveDialect::Merged, 442), AcceptedSource(SaveDialect::Merged, 445),
		AcceptedSource(SaveDialect::Merged, 447), AcceptedSource(SaveDialect::DestinationA, 441),
		AcceptedSource(SaveDialect::DonorB, 435), AcceptedSource(SaveDialect::Shared, 441),
		AcceptedSource(static_cast<SaveDialect>(200), 446),
	};
	for (const auto &source : foreign) {
		for (const auto &expected : CAPABILITY_EXPECTATIONS) CHECK_FALSE(HasSaveCapability(source, expected.capability));
	}
	// A refused attempt grants nothing even when it names merged 446.
	for (auto status : {SaveLineageStatus::Ambiguous, SaveLineageStatus::Corrupt, SaveLineageStatus::Unsupported}) {
		CHECK_FALSE(HasSaveCapability({status, SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION}, SaveCapability::AsteroidCargoOrigins));
	}
	// The next appended value and any unknown value have no rule, so a future
	// append cannot silently become available in old 441 or in 446.
	for (const auto unknown : {static_cast<SaveCapability>(SaveCapability::Count), static_cast<SaveCapability>(255)}) {
		CHECK_FALSE(HasSaveCapability(merged441, unknown));
		CHECK_FALSE(HasSaveCapability(merged446, unknown));
	}
}

/** Merged 441 is read from its marker and proofs; a markerless 441 is a development build's file and is not read. */
TEST_CASE("Save lineage reads merged 441 and refuses a markerless 441", "[save-lineage]")
{
	const auto merged = ClassifySaveDialect(Merged());
	REQUIRE(merged.status == SaveLineageStatus::Accepted);
	CHECK(merged.dialect == SaveDialect::Merged); CHECK(merged.original_version == 441);
	CHECK_FALSE(HasSaveCapability(merged, SaveCapability::AsteroidCargoOrigins));
	CHECK(ClassifySaveDialect(DevelopmentBuild(441)).status == SaveLineageStatus::Unsupported);
	// Typed origins inside a marked 441 contradict the frozen allocation, and a
	// marker cannot adopt a development build's fixed book either.
	auto facts = Merged(); facts.asteroid_origin_columns = SaveEvidenceState::Valid;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = DevelopmentBuild(441); AddChunk(facts, SaveLineageFamily::Marker, SaveEvidenceKind::Riff, 8);
	facts.marker_magic = MERGED_SAVE_LINEAGE_MAGIC; facts.marker_schema = MERGED_SAVE_LINEAGE_SCHEMA; facts.marker_writer_version = 441;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	// Without its marker even the complete merged transport is just a 441 number.
	facts = Merged(); Evidence(facts, SaveLineageFamily::Marker) = {};
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Unsupported);
}

/** Every markerless 441-445 is Unsupported whatever it contains; malformed evidence still refuses first as Corrupt. */
TEST_CASE("Save lineage refuses every markerless 441-445 as a development build", "[save-lineage]")
{
	for (uint16_t version = 441; version <= 445; ++version) {
		INFO(version);
		CHECK(ClassifySaveDialect(DevelopmentBuild(version)).status == SaveLineageStatus::Unsupported);
		auto classic = DevelopmentBuild(version); classic.space_content_version = 0;
		CHECK(ClassifySaveDialect(classic).status == SaveLineageStatus::Unsupported);
		// No legacy origin hint can turn these numbers into a historical writer.
		auto hinted = DevelopmentBuild(version);
		for (const auto dialect : {SaveDialect::DestinationA, SaveDialect::DonorB}) {
			CHECK(ClassifySaveDialect(hinted, SaveOriginHint{dialect, hinted.source_digest}).status == SaveLineageStatus::Unsupported);
		}
		// Changing what the file carries does not make it readable.
		hinted.asteroid_origin_columns = SaveEvidenceState::Absent;
		CHECK(ClassifySaveDialect(hinted).status == SaveLineageStatus::Unsupported);
	}
	// Malformed evidence is reported as corruption before the number is considered.
	auto facts = DevelopmentBuild(443); facts.asteroid_origin_columns = SaveEvidenceState::Malformed;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = DevelopmentBuild(443); facts.framing_complete = false;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	// A legacy file at 440 or below cannot carry the merged-446 columns either.
	facts = Destination(440); facts.asteroid_origin_columns = SaveEvidenceState::Valid;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
}

/** The reserved combined writer is accepted only with its own marker tuple and typed origins; nothing unmarked or later. */
TEST_CASE("Save lineage admits marked 446 and refuses every other modern tuple", "[save-lineage]")
{
	const auto current = ClassifySaveDialect(Merged446());
	REQUIRE(current.status == SaveLineageStatus::Accepted);
	CHECK(current.dialect == SaveDialect::Merged); CHECK(current.original_version == CURRENT_MERGED_SAVE_VERSION);
	CHECK(HasSaveCapability(current, SaveCapability::SpaceSurveyDeparture));
	CHECK(HasSaveCapability(current, SaveCapability::TubeGeothermalBindings));
	auto facts = Merged446(); facts.asteroid_origin_columns = SaveEvidenceState::Absent;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt); // 446 always declares typed origins.
	facts = Merged446(); facts.marker_schema = MERGED_SAVE_LINEAGE_SCHEMA;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt); // Schema 1 belongs to 441.
	facts = Merged446(); Evidence(facts, SaveLineageFamily::Marker) = {};
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt); // Unmarked 446.
	// A marker on any Space fixes era belongs to no writer, whatever it claims.
	for (uint16_t version = 442; version <= 445; ++version) {
		facts = Merged446(); facts.original_version = facts.marker_writer_version = version;
		CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	}
	facts = Merged446(); facts.original_version = facts.marker_writer_version = 447;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Unsupported);
	facts = DevelopmentBuild(445); facts.original_version = 447;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Unsupported);
}

