/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_lineage_policy.cpp Detached ambiguity, corruption and capability cases.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-b-save-lineage.md
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
	if (version >= 440) AddChunk(facts, SaveLineageFamily::SupplyOrders, SaveEvidenceKind::Riff, 14);
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

/** Build the future merged detached contract without enabling a save writer.
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
	AddChunk(facts, SaveLineageFamily::CargoDimensions, SaveEvidenceKind::Riff, 1);
	AddChunk(facts, SaveLineageFamily::OrbitalRecipes, SaveEvidenceKind::Riff, 11);
	AddChunk(facts, SaveLineageFamily::Marker, SaveEvidenceKind::Riff, 8);
	facts.marker_magic = MERGED_SAVE_LINEAGE_MAGIC;
	facts.marker_schema = MERGED_SAVE_LINEAGE_SCHEMA;
	facts.marker_writer_version = MERGED_SAVE_LINEAGE_VERSION;
	facts.merged_supply_valid = facts.merged_cargo_valid = true;
	return facts;
}

/** Independent source-enum expectation; do not derive thresholds from capability ordinals. */
struct CapabilityExpectation {
	SaveCapability capability; ///< Named historical feature, independent of declaration order.
	SaveDialect dialect; ///< Only this original branch introduced the feature.
	uint16_t first_version; ///< Explicit original SaveLoadVersion value in the audited handoff.
};

/** Source-defined thresholds from both original enums; merged files contain their union. */
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
};
static_assert(std::size(CAPABILITY_EXPECTATIONS) == static_cast<uint8_t>(SaveCapability::Count));

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
	facts = Destination(440); facts.original_version = 442;
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Unsupported);
	CHECK_FALSE(HasSaveCapability(ClassifySaveDialect(Common(425)), SaveCapability::TBMFaceTransfer));
	CHECK_FALSE(HasSaveCapability(ClassifySaveDialect(Destination(440)), static_cast<SaveCapability>(255)));
}

/** A future marker grants only the fully supplied merged transport, never serialization permission. */
TEST_CASE("Save lineage validates the reserved merged marker and complete family boundary", "[save-lineage]")
{
	auto facts = Merged();
	const auto result = ClassifySaveDialect(facts);
	REQUIRE(result.status == SaveLineageStatus::Accepted);
	CHECK(result.dialect == SaveDialect::Merged);
	// Every currently known capability is supplied by the combined writer;
	// unknown enum values still refuse independently of the complete marker.
	for (uint8_t feature = 0; feature < static_cast<uint8_t>(SaveCapability::Count); ++feature) CHECK(HasSaveCapability(result, static_cast<SaveCapability>(feature)));
	CHECK_FALSE(HasSaveCapability(result, static_cast<SaveCapability>(255)));
	facts.marker_magic ^= 1; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.marker_schema = 2; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.marker_writer_version = 440; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.original_version = 440; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.merged_supply_valid = false; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.merged_cargo_valid = false; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); Evidence(facts, SaveLineageFamily::GeneratorRows) = {}; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
	facts = Merged(); facts.feature_flags = 0x01000000; CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Corrupt);
}
