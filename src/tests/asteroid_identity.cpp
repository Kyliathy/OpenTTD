/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_identity.cpp Sparse field identity tests without high-ID native cargo or live retirement.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group12.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-2-merge-oct28.md Group 23; every saved renewal field reaches the field diagnostic.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../safeguards.h"

/** @return Detached identity-only fixture with deliberately unordered high IDs and distinguishable reserves; it is not a playable field. */
static AsteroidField SparseIdentityFixture()
{
	AsteroidField field;
	field.next_body_id = 70'001;
	field.renewal = MakeAsteroidRenewalState(8);
	for (uint32_t id : {3U, 70'000U, 42U, 69'999U}) {
		AsteroidBody body;
		body.id = OrbitalBodyID{id}; body.initial_reserve = body.remaining_reserve = id + 10;
		field.bodies.push_back(body);
	}
	REQUIRE(RebuildAsteroidBodyLookup(field));
	return field;
}

/** Sparse lookup and swap/pop preserve resource identity, never the physical slot that once held it. */
TEST_CASE("Asteroid live storage erases first middle and last without reusing IDs", "[asteroid-identity]")
{
	// Every erasure position exercises the same owner against independent input;
	// lookups after a move must still return each survivor's original stock.
	for (uint32_t erased : {3U, 70'000U, 42U, 69'999U}) {
		auto field = SparseIdentityFixture();
		const auto ledger = field.renewal;
		CHECK(FindAsteroidBody(std::as_const(field), OrbitalBodyID{erased})->remaining_reserve == erased + 10);
		REQUIRE(EraseAsteroidBody(field, OrbitalBodyID{erased}));
		CHECK(field.bodies.size() == 3);
		CHECK(field.next_body_id == 70'001);
		CHECK(field.renewal == ledger);
		CHECK(FindAsteroidBody(field, OrbitalBodyID{erased}) == nullptr);
		CHECK_FALSE(EraseAsteroidBody(field, OrbitalBodyID{erased}));
		for (const auto &body : field.bodies) CHECK(FindAsteroidBody(field, body.id)->remaining_reserve == body.id.base() + 10);
		AsteroidBody fresh;
		fresh.id = OrbitalBodyID{field.next_body_id}; fresh.remaining_reserve = 999;
		REQUIRE(InsertAsteroidBody(field, fresh));
		CHECK(field.next_body_id == 70'002);
		CHECK(FindAsteroidBody(field, fresh.id)->remaining_reserve == 999);
		const auto before = field;
		CHECK_FALSE(InsertAsteroidBody(field, fresh));
		fresh.id = OrbitalBodyID{erased};
		CHECK_FALSE(InsertAsteroidBody(field, fresh));
		CHECK(field == before);
		CHECK(ValidateAsteroidBodyIdentities(field));
	}
}

/** Copied records are validated independently of stale caches; sorted wire/digest iteration follows stable IDs. */
TEST_CASE("Asteroid identity validation and diagnostics ignore stale derived layout", "[asteroid-identity]")
{
	const auto field = SparseIdentityFixture();
	auto copied = field;
	std::ranges::reverse(copied.bodies);
	CHECK(ValidateAsteroidBodyIdentities(copied));
	CHECK(ComputeAsteroidFieldDigest(copied) == ComputeAsteroidFieldDigest(field));
	for (const auto &body : field.bodies) CHECK(*FindAsteroidBody(copied, body.id) == body);
	CHECK(FindAsteroidBody(copied, OrbitalBodyID{65'536}) == nullptr);
	CHECK(FindAsteroidBody(copied, INVALID_ORBITAL_BODY) == nullptr);
	const auto before_rebuild = copied;
	REQUIRE(RebuildAsteroidBodyLookup(copied));
	CHECK(copied == before_rebuild);
	std::vector<uint32_t> ordered;
	for (const auto *body : GetAsteroidBodiesInIDOrder(copied)) ordered.push_back(body->id.base());
	CHECK(ordered == std::vector<uint32_t>{3, 42, 69'999, 70'000});
	copied.bodies[0].id = copied.bodies[1].id;
	CHECK_FALSE(ValidateAsteroidBodyIdentities(copied));
	CHECK_FALSE(RebuildAsteroidBodyLookup(copied));
	copied = field; copied.bodies[0].id = INVALID_ORBITAL_BODY;
	CHECK_FALSE(ValidateAsteroidBodyIdentities(copied));
	copied = field; copied.bodies[0].id = OrbitalBodyID{copied.next_body_id};
	CHECK_FALSE(ValidateAsteroidBodyIdentities(copied));
	copied = field; ++copied.next_body_id;
	CHECK(ComputeAsteroidFieldDigest(copied) != ComputeAsteroidFieldDigest(field));
}

/** Lifetime issuance can reach the last valid ID with only one live record; exhaustion is terminal even after erasure. */
TEST_CASE("Asteroid issuance exhausts without sentinel reuse or lifetime-sized storage", "[asteroid-identity]")
{
	AsteroidField field;
	field.next_body_id = UINT32_MAX - 1;
	REQUIRE(RebuildAsteroidBodyLookup(field));
	AsteroidBody final;
	final.id = OrbitalBodyID{UINT32_MAX - 1};
	REQUIRE(InsertAsteroidBody(field, final));
	CHECK(field.next_body_id == UINT32_MAX);
	CHECK(field.bodies.size() == 1);
	CHECK(ValidateAsteroidBodyIdentities(field));
	CHECK(FindAsteroidBody(field, final.id) != nullptr);
	final.id = INVALID_ORBITAL_BODY;
	CHECK_FALSE(InsertAsteroidBody(field, final));
	REQUIRE(EraseAsteroidBody(field, OrbitalBodyID{UINT32_MAX - 1}));
	CHECK(field.bodies.empty());
	CHECK(field.next_body_id == UINT32_MAX);
	final.id = OrbitalBodyID{0};
	CHECK_FALSE(InsertAsteroidBody(field, final));
	CHECK(ValidateAsteroidBodyIdentities(field));
}

/** The population cap bounds simultaneous live records, not the highest issued identity. */
TEST_CASE("Asteroid live cap refuses insertion without spending an ID", "[asteroid-identity]")
{
	AsteroidField field;
	field.next_body_id = MAX_ORBITAL_BODIES;
	field.bodies.resize(MAX_ORBITAL_BODIES);
	for (uint32_t id = 0; id < MAX_ORBITAL_BODIES; ++id) field.bodies[id].id = OrbitalBodyID{id};
	REQUIRE(RebuildAsteroidBodyLookup(field));
	AsteroidBody proposed;
	proposed.id = OrbitalBodyID{field.next_body_id};
	const auto before = field;
	CHECK_FALSE(InsertAsteroidBody(field, proposed));
	CHECK(field == before);
	REQUIRE(EraseAsteroidBody(field, OrbitalBodyID{500}));
	REQUIRE(InsertAsteroidBody(field, proposed));
	CHECK(field.bodies.size() == MAX_ORBITAL_BODIES);
	CHECK(field.next_body_id == MAX_ORBITAL_BODIES + 1);
	CHECK(FindAsteroidBody(field, OrbitalBodyID{500}) == nullptr);
	CHECK(FindAsteroidBody(field, proposed.id) != nullptr);
}

/** Restore actual live field/index authority after testing reset and legacy installation. */
struct IdentityFieldRestore {
	AsteroidField original = GetAsteroidField();
	/** Restore the previously accepted owner through the native install boundary. */
	~IdentityFieldRestore() { CHECK(InstallAsteroidField(std::move(this->original))); }
};

/** Installation reconstructs derived maps, while sparse live publication remains behind later consumer/persistence gates. */
TEST_CASE("Asteroid install reset and old-reader cursor migration rebuild lookup", "[asteroid-identity]")
{
	IdentityFieldRestore restore;
	const auto generated = GenerateAsteroidField(64, 64, 53022769049978ULL, AsteroidDensity::Normal);
	REQUIRE(generated);
	CHECK(generated->next_body_id == generated->bodies.size());
	REQUIRE(InstallAsteroidField(*generated));
	for (const auto &body : generated->bodies) CHECK(*GetMutableAsteroidBody(body.id) == body);
	const auto original_digest = ComputeAsteroidFieldDigest(*generated);
	ResetAsteroidField();
	CHECK(GetAsteroidField() == AsteroidField{});
	CHECK(GetMutableAsteroidBody(OrbitalBodyID{0}) == nullptr);
	auto legacy = *generated;
	legacy.lifecycle_version = 0; legacy.next_body_id = 0; legacy.renewal = {};
	REQUIRE(ValidateAsteroidField(legacy));
	REQUIRE(InstallAsteroidField(legacy));
	CHECK(GetAsteroidField() == *generated);
	CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == original_digest);
	auto invalid = *generated;
	invalid.bodies[0].id = invalid.bodies[1].id;
	CHECK_FALSE(InstallAsteroidField(invalid));
	invalid = *generated; invalid.next_body_id = 70'001;
	CHECK_FALSE(InstallAsteroidField(invalid));
	CHECK(GetAsteroidField() == *generated);
}

/** Each saved renewal ledger field, and the sparse issuer, changes the field diagnostic; an equal ledger does not (m2o28 Group 23). */
TEST_CASE("Asteroid renewal diagnostics change with every saved ledger field", "[asteroid-identity]")
{
	auto base = SparseIdentityFixture(); base.seed = 8;
	// The fixture's ledger is the pristine seeding for seed 8; one pending credit makes it an evolved ledger.
	REQUIRE(base.renewal == MakeAsteroidRenewalState(base.seed));
	base.renewal.credits = 1;
	const auto digest = ComputeAsteroidFieldDigest(base);
	CHECK(ComputeAsteroidFieldDigest(base) == digest); // Deterministic.
	/** @param mutate One saved-field change. @return Whether the diagnostic of the changed copy differs. */
	const auto changes = [&](auto mutate) { auto copy = base; mutate(copy); return ComputeAsteroidFieldDigest(copy) != digest; };
	for (size_t size = 0; size < base.renewal.exited_by_size.size(); ++size) {
		INFO(size); CHECK(changes([&](AsteroidField &f) { ++f.renewal.exited_by_size[size]; }));
	}
	CHECK(changes([](AsteroidField &f) { ++f.renewal.credits; }));
	CHECK(changes([](AsteroidField &f) { f.renewal.ticks_remaining += 1; }));
	CHECK(changes([](AsteroidField &f) { f.renewal.last_exit = AsteroidExitMetadata{AsteroidSize::Large, AsteroidClass::Rich}; }));
	for (size_t word = 0; word < 2; ++word) {
		INFO(word);
		CHECK(changes([&](AsteroidField &f) { f.renewal.timer_rng.state[word] ^= 1; }));
		CHECK(changes([&](AsteroidField &f) { f.renewal.candidate_rng.state[word] ^= 1; }));
	}
	CHECK(changes([](AsteroidField &f) { ++f.next_body_id; }));
	// Equivalent storage order is not saved meaning.
	auto reordered = base; std::ranges::reverse(reordered.bodies); REQUIRE(RebuildAsteroidBodyLookup(reordered));
	CHECK(ComputeAsteroidFieldDigest(reordered) == digest);
}

