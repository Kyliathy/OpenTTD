/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_preview_identity.cpp Candidate-only identity and knowledge joins, independent of installed resources.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group14B.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../fios.h"
#include "../safeguards.h"

/** Candidate identity transport is bounded by live rows and issuance, never a dense count-as-ID rule. */
TEST_CASE("Asteroid copied identities validate order class issuance and high IDs", "[asteroid-preview-identity]")
{
	const std::array rows{
		AsteroidIdentityRecord{OrbitalBodyID{3}, AsteroidClass::Rich},
		AsteroidIdentityRecord{OrbitalBodyID{70'000}, AsteroidClass::Metallic},
		AsteroidIdentityRecord{OrbitalBodyID{UINT32_MAX - 1}, AsteroidClass::Carbonaceous}};
	CHECK(ValidateAsteroidIdentityRecords(rows, UINT32_MAX));
	CHECK(ValidateAsteroidIdentityRecords({}, UINT32_MAX));
	CHECK(ValidateAsteroidIdentityRecords({}, 0));
	CHECK_FALSE(ValidateAsteroidIdentityRecords(rows, UINT32_MAX - 1));
	CHECK_FALSE(ValidateAsteroidIdentityRecords(rows, 3));
	auto bad = rows; std::swap(bad[0], bad[1]);
	CHECK_FALSE(ValidateAsteroidIdentityRecords(bad, UINT32_MAX));
	bad = rows; bad[1].id = bad[0].id;
	CHECK_FALSE(ValidateAsteroidIdentityRecords(bad, UINT32_MAX));
	bad = rows; bad[2].id = INVALID_ORBITAL_BODY;
	CHECK_FALSE(ValidateAsteroidIdentityRecords(bad, UINT32_MAX));
	bad = rows; bad[0].resource_class = static_cast<AsteroidClass>(3);
	CHECK_FALSE(ValidateAsteroidIdentityRecords(bad, UINT32_MAX));
	const auto *high = FindAsteroidIdentity(rows, OrbitalBodyID{70'000});
	REQUIRE(high != nullptr);
	CHECK(high->resource_class == AsteroidClass::Metallic);
	CHECK(FindAsteroidIdentity(rows, OrbitalBodyID{0}) == nullptr);
	CHECK(FindAsteroidIdentity(rows, OrbitalBodyID{69'999}) == nullptr);
	CHECK(FindAsteroidIdentity(rows, INVALID_ORBITAL_BODY) == nullptr);
	std::vector<AsteroidIdentityRecord> excess(MAX_ORBITAL_BODIES + 1);
	CHECK_FALSE(ValidateAsteroidIdentityRecords(excess, UINT32_MAX));
}

/** Completed knowledge joins to candidate IDs; legacy transport retains its separately declared dense restriction. */
TEST_CASE("Asteroid copied knowledge cannot borrow another body's identity or mask", "[asteroid-preview-identity]")
{
	const std::array bodies{
		AsteroidIdentityRecord{OrbitalBodyID{8}, AsteroidClass::Rich},
		AsteroidIdentityRecord{OrbitalBodyID{70'000}, AsteroidClass::Metallic}};
	const std::array known{AsteroidKnowledgeRecord{OrbitalBodyID{70'000}, 1}};
	CHECK(ValidateAsteroidKnowledgeShape(known));
	CHECK(ValidateAsteroidKnowledgeRecords(known, bodies));
	CHECK_FALSE(ValidateAsteroidKnowledgeRecords(known, MAX_ORBITAL_BODIES));
	auto bad = known; bad[0].body = OrbitalBodyID{1};
	CHECK(ValidateAsteroidKnowledgeShape(bad));
	CHECK_FALSE(ValidateAsteroidKnowledgeRecords(bad, bodies));
	bad = known; bad[0].surveyed_companies = 0;
	CHECK_FALSE(ValidateAsteroidKnowledgeRecords(bad, bodies));
	bad = known; bad[0].surveyed_companies = uint64_t{1} << MAX_COMPANIES;
	CHECK_FALSE(ValidateAsteroidKnowledgeRecords(bad, bodies));
	CHECK_FALSE(ValidateAsteroidKnowledgeRecords(std::array{known[0], known[0]}, bodies));
}

/** Restore native authority after contrasting it with candidate rows carrying different classes and IDs. */
struct PreviewIdentityFieldRestore {
	AsteroidField original = GetAsteroidField();
	/** Reinstall original resources and derive their lookup/index again. */
	~PreviewIdentityFieldRestore() { CHECK(InstallAsteroidField(std::move(this->original))); }
};

/** Candidate joins use their own rows even when another actual world is open; Clear discards all previous identity evidence. */
TEST_CASE("Asteroid preview identity is independent of installed field and clears fully", "[asteroid-preview-identity]")
{
	PreviewIdentityFieldRestore restore;
	const auto generated = GenerateAsteroidField(64, 64, 8, AsteroidDensity::Normal);
	REQUIRE(generated);
	REQUIRE(InstallAsteroidField(*generated));
	const auto live = GetAsteroidField();
	LoadCheckData candidate;
	candidate.asteroid_present = candidate.asteroid_active = candidate.asteroid_knowledge_present = true;
	candidate.asteroid_body_count = 2;
	candidate.asteroid_next_body_id = 70'001;
	candidate.asteroid_body_rows = {{OrbitalBodyID{0}, AsteroidClass::Rich}, {OrbitalBodyID{70'000}, AsteroidClass::Carbonaceous}};
	candidate.asteroid_knowledge_rows = {{OrbitalBodyID{70'000}, 1}};
	CHECK(FindAsteroidBody(live, OrbitalBodyID{0})->resource_class == AsteroidClass::Metallic);
	CHECK(FindAsteroidIdentity(candidate.asteroid_body_rows, OrbitalBodyID{0})->resource_class == AsteroidClass::Rich);
	CHECK(FindAsteroidBody(live, OrbitalBodyID{70'000}) == nullptr);
	CHECK(ValidateAsteroidIdentityRecords(candidate.asteroid_body_rows, candidate.asteroid_next_body_id));
	CHECK(ValidateAsteroidKnowledgeRecords(candidate.asteroid_knowledge_rows, candidate.asteroid_body_rows));
	CHECK_FALSE(ValidateAsteroidKnowledgeRecords(candidate.asteroid_knowledge_rows, live));
	CHECK(GetAsteroidField() == live);
	candidate.Clear();
	CHECK_FALSE(candidate.asteroid_present);
	CHECK_FALSE(candidate.asteroid_active);
	CHECK_FALSE(candidate.asteroid_knowledge_present);
	CHECK(candidate.asteroid_body_count == 0);
	CHECK(candidate.asteroid_next_body_id == 0);
	CHECK(candidate.asteroid_body_rows.empty());
	CHECK(candidate.asteroid_knowledge_rows.empty());
	CHECK(GetAsteroidField() == live);
	auto copied = live;
	std::ranges::reverse(copied.bodies);
	CHECK(GetAsteroidIdentityRecords(copied) == GetAsteroidIdentityRecords(live));
}
