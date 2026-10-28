/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_live_validation.cpp Dynamic copied-field validation and original survey/selection boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups14/24/28; sparse identity and actual size-credit conservation.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../asteroid_inspection.h"
#include "../company_base.h"
#include "probe_mining_fixture.h"
#include "../safeguards.h"

/** @return Original deterministic generated field for copied-state checks, without installing lifecycle state. */
static AsteroidField ValidationField()
{
	auto generated = GenerateAsteroidField(64, 64, 53022769049978ULL, AsteroidDensity::Normal);
	REQUIRE(generated);
	return *generated;
}

/**
 * Construct a detached terminal transaction for validator tests; this does not advance gameplay or claim safe native retirement.
 * @param field Copied generated owner with no actual native leases.
 * @param ids Explicit original live IDs to credit and remove together.
 */
static void RetireValidationBodies(AsteroidField &field, std::span<const OrbitalBodyID> ids)
{
	std::vector<AsteroidExitRecord> events;
	// Capture original terminal metadata before any swap/pop invalidates pointers.
	for (const auto id : ids) {
		const auto *body = FindAsteroidBody(field, id);
		REQUIRE(body != nullptr);
		events.push_back({id, {body->size, body->resource_class}});
	}
	const auto next = ApplyAsteroidExitCredit(field.renewal, events, field.next_body_id);
	REQUIRE(next);
	field.renewal = *next;
	field.elapsed_ticks = 1;
	// Knowledge is a live reference, so the staged transaction removes it before
	// erasing the corresponding original resource record. Cargo is not involved.
	for (const auto id : ids) {
		RemoveAsteroidKnowledge(field, id);
		REQUIRE(EraseAsteroidBody(field, id));
	}
}

/** Initial generation requirements remain exact, while counted exits may leave a smaller or empty current field. */
TEST_CASE("Asteroid validation separates initial history from counted live retirement", "[asteroid-live-validation]")
{
	auto field = ValidationField();
	const auto initial = field;
	field.bodies.front().remaining_reserve = 0;
	CHECK(ValidateAsteroidField(field));
	CHECK(field.renewal.credits == 0);
	REQUIRE(EraseAsteroidBody(field, field.bodies.front().id));
	CHECK_FALSE(ValidateAsteroidField(field)); // Erasure alone cannot forge terminal history.
	field = initial;
	const auto removed = field.bodies[1].id;
	RetireValidationBodies(field, std::array{removed});
	CHECK(ValidateAsteroidField(field));
	CHECK(field.bodies.size() == initial.bodies.size() - 1);
	CHECK(FindAsteroidBody(field, removed) == nullptr);
	std::vector<OrbitalBodyID> rest;
	for (const auto &body : field.bodies) rest.push_back(body.id);
	RetireValidationBodies(field, rest);
	CHECK(field.bodies.empty());
	CHECK(ValidateAsteroidField(field));
	CHECK_FALSE(ResolveAsteroidSelection(field, removed));
	CHECK(field.next_body_id == initial.next_body_id);
	auto bad = field; bad.elapsed_ticks = 0;
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; ++bad.next_body_id;
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; bad.renewal.credits = UINT64_MAX;
	CHECK_FALSE(ValidateAsteroidField(bad));
}

/** Known replacement sizes require actual dedicated credit cost, and a renewal history cannot start without an original exit. */
TEST_CASE("Asteroid lifecycle credit accounts for known surviving replacement sizes", "[asteroid-live-validation]")
{
	auto field = ValidationField();
	const auto medium = std::ranges::find(field.bodies, AsteroidSize::Medium, &AsteroidBody::size); REQUIRE(medium != field.bodies.end());
	auto body = *medium; const auto retired = body.id;
	RetireValidationBodies(field, std::array{retired}); field.renewal.ticks_remaining = 0;
	const auto attempt = BeginAsteroidRenewalAttempt(field.renewal, field.next_body_id); REQUIRE(attempt); REQUIRE(attempt->size == AsteroidSize::Small);
	const auto issued = field.next_body_id; body.id = OrbitalBodyID{issued}; body.size = AsteroidSize::Small;
	body.initial_reserve = body.remaining_reserve = GetMultiLayerAsteroidDefinitions()[to_underlying(body.resource_class)].small_min;
	REQUIRE(InsertAsteroidBody(field, body));
	const auto paid = FinishAsteroidRenewalAttempt(field.renewal, *attempt, true, issued); REQUIRE(paid); field.renewal = *paid;
	REQUIRE(ValidateAsteroidField(field));
	// With no retired replacement, this sole actual new Small must cost1.
	// Spending all3 of the Medium's credits fits coarse1..5 bounds but is false.
	auto overcharged = field; overcharged.renewal.credits = 0; overcharged.renewal.ticks_remaining = 0;
	CHECK_FALSE(ValidateAsteroidField(overcharged));
	auto bootstrap = ValidationField(); bootstrap.elapsed_ticks = 1; body.id = OrbitalBodyID{bootstrap.next_body_id};
	REQUIRE(InsertAsteroidBody(bootstrap, body)); ++bootstrap.next_body_id;
	bootstrap.renewal.exited_by_size = {0, 0, 1}; bootstrap.renewal.credits = 3;
	bootstrap.renewal.last_exit = AsteroidExitMetadata{AsteroidSize::Large, AsteroidClass::Metallic}; bootstrap.renewal.ticks_remaining = 741;
	// All originals still survive. The claimed terminal/new history balances
	// numerically but can never earn its first creation credit from a real exit.
	CHECK_FALSE(ValidateAsteroidField(bootstrap));
}

/** Aggregate refunds retain original size weight and size eligibility despite absent retired replacement records. */
TEST_CASE("Unknown retirement history cannot inflate original size weight or bypass size eligibility", "[asteroid-live-validation]")
{
	auto field = ValidationField(); const auto medium = std::ranges::find(field.bodies, AsteroidSize::Medium, &AsteroidBody::size); REQUIRE(medium != field.bodies.end());
	auto description = *medium; RetireValidationBodies(field, std::array{medium->id}); REQUIRE(ValidateAsteroidField(field));
	SECTION("cheap historical births cannot refund larger sizes") {
		field.next_body_id += 2; field.renewal.exited_by_size[2] += 2; field.renewal.credits += 8;
		field.renewal.last_exit = AsteroidExitMetadata{AsteroidSize::Large, AsteroidClass::Metallic};
		REQUIRE(ValidateAsteroidRenewalState(field.renewal, field.next_body_id)); CHECK_FALSE(ValidateAsteroidField(field));
	}
	SECTION("original weight must fit an original exit size bucket") {
		field.next_body_id += 2; field.renewal.exited_by_size = {2, 0, 1};
		field.renewal.last_exit = AsteroidExitMetadata{AsteroidSize::Large, AsteroidClass::Metallic};
		REQUIRE(field.renewal.credits == 3); REQUIRE(ValidateAsteroidRenewalState(field.renewal, field.next_body_id));
		CHECK_FALSE(ValidateAsteroidField(field)); // One retired original of weight3 must be Medium; this candidate has no Medium exit.
	}
	SECTION("known Large cannot cost only the Medium original budget") {
		description.id = OrbitalBodyID{field.next_body_id}; description.size = AsteroidSize::Large;
		description.initial_reserve = description.remaining_reserve = GetMultiLayerAsteroidDefinitions()[to_underlying(description.resource_class)].small_min * 4;
		REQUIRE(InsertAsteroidBody(field, description)); field.renewal.credits = 0; field.renewal.ticks_remaining = 0;
		REQUIRE(ValidateAsteroidRenewalState(field.renewal, field.next_body_id)); CHECK_FALSE(ValidateAsteroidField(field));
	}
	SECTION("one-color exits cannot fund historical same-color births") {
		field = ValidationField(); const auto initial = field.next_body_id; field.bodies.clear(); field.next_body_id = initial + 2; field.elapsed_ticks = 1;
		field.renewal.exited_by_size = {field.next_body_id, 0, 0}; field.renewal.credits = initial;
		field.renewal.last_exit = AsteroidExitMetadata{AsteroidSize::Small, AsteroidClass::Metallic}; field.renewal.ticks_remaining = 741;
		REQUIRE(RebuildAsteroidBodyLookup(field)); REQUIRE(ValidateAsteroidRenewalState(field.renewal, field.next_body_id));
		CHECK_FALSE(ValidateAsteroidField(field)); // No different-size exit ever made either alleged retired new Small eligible.
	}
}

/** High sparse IDs retain their own resource/knowledge identity, with no inheritance through dense slots. */
TEST_CASE("Asteroid sparse field history knowledge and selection use live IDs", "[asteroid-live-validation]")
{
	auto field = ValidationField();
	const uint32_t initial_count = field.next_body_id;
	const auto original = field.bodies.front();
	// A declared aggregate history exercises lifetime-ID arithmetic; this is a
	// copied validator fixture, not evidence of 70,000 executed native departures.
	field.elapsed_ticks = 1;
	field.bodies = {original};
	field.bodies.front().id = OrbitalBodyID{70'000};
	field.next_body_id = 70'001;
	const uint64_t medium_exits = (70'000 - initial_count) / 2;
	field.renewal.exited_by_size = {70'000 - medium_exits, medium_exits, 0};
	field.renewal.credits = initial_count - *GetAsteroidRenewalCost(field.bodies.front().size);
	field.renewal.last_exit = AsteroidExitMetadata{AsteroidSize::Medium, AsteroidClass::Rich};
	field.renewal.ticks_remaining = GetAsteroidRenewalDelayBounds().first;
	REQUIRE(RebuildAsteroidBodyLookup(field));
	REQUIRE(ValidateAsteroidField(field));
	field.knowledge = {{OrbitalBodyID{70'000}, 1}};
	CHECK(ValidateAsteroidKnowledgeRecords(field.knowledge, field));
	CHECK(ValidateAsteroidField(field));
	CHECK(ResolveAsteroidSelection(field, OrbitalBodyID{0}) == OrbitalBodyID{70'000});
	CHECK(ResolveAsteroidSelection(field, OrbitalBodyID{70'000}) == OrbitalBodyID{70'000});
	const auto before = field;
	auto large = field;
	large.renewal.credits -= *GetAsteroidRenewalCost(AsteroidSize::Large) - *GetAsteroidRenewalCost(original.size);
	large.bodies.front().size = AsteroidSize::Large;
	large.bodies.front().initial_reserve = large.bodies.front().remaining_reserve = GetMultiLayerAsteroidDefinitions()[to_underlying(original.resource_class)].small_min *
		GetMultiLayerAsteroidSizeDefinitions()[to_underlying(AsteroidSize::Large)].reserve_multiplier;
	REQUIRE(ValidateAsteroidField(large));
	auto inflated = large; inflated.renewal.credits += 5 * initial_count;
	CHECK_FALSE(ValidateAsteroidField(inflated)); // Unknown retired replacements cannot fund more live weight/credit than every original could contain.
	CHECK_FALSE(FindAsteroidBody(field, OrbitalBodyID{0}));
	CHECK(field == before);
	auto bad = field;
	bad.knowledge.front().body = OrbitalBodyID{0};
	CHECK_FALSE(ValidateAsteroidKnowledgeRecords(bad.knowledge, bad));
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; bad.knowledge.front().surveyed_companies = uint64_t{1} << MAX_COMPANIES;
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; bad.knowledge.front().surveyed_companies = 0;
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; bad.knowledge.push_back(bad.knowledge.front());
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; ++bad.bodies.front().remaining_reserve;
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; bad.bodies.front().id = OrbitalBodyID{field.next_body_id};
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; ++bad.renewal.credits; // This would spend an impossible even cost on an odd-cost spawn count.
	CHECK_FALSE(ValidateAsteroidField(bad));
	RemoveAsteroidKnowledge(field, OrbitalBodyID{0});
	CHECK(field.knowledge == before.knowledge);
	RemoveAsteroidKnowledge(field, OrbitalBodyID{70'000});
	CHECK(field.knowledge.empty());
	CHECK(field.renewal == before.renewal);
	CHECK(field.bodies == before.bodies);
}

/** Canonical physical bounds apply after generation; stale identities and arbitrary motion still fail. */
TEST_CASE("Asteroid continuing field geometry and canonical identity remain validated", "[asteroid-live-validation]")
{
	auto field = ValidationField();
	field.elapsed_ticks = 1;
	field.bodies.front().motion.position = {0, 0};
	CHECK(ValidateAsteroidField(field));
	auto bad = field; bad.elapsed_ticks = 0;
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; bad.bodies.front().motion.position.x = -1;
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; bad.bodies.front().motion.remainder_y = 74;
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; bad.bodies.front().motion.velocity_x = 123;
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; bad.bodies[1].id = bad.bodies[0].id;
	CHECK_FALSE(ValidateAsteroidField(bad));
	bad = field; bad.bodies.front().initial_reserve = 0; bad.bodies.front().remaining_reserve = 0;
	CHECK_FALSE(ValidateAsteroidField(bad));
	std::ranges::reverse(field.bodies);
	CHECK(ValidateAsteroidField(field)); // Copied cache is stale; raw IDs still determine validity.
	CHECK(ResolveAsteroidSelection(field, std::nullopt) == OrbitalBodyID{0});
	CHECK(ResolveAsteroidSelection(field, OrbitalBodyID{2}) == OrbitalBodyID{2});
	SpaceSurveyPreview preview;
	preview.target = OrbitalBodyID{70'000};
	field.surveys.push_back(preview);
	CHECK_FALSE(ValidateAsteroidField(field));
}

/** Current lifecycle installation accepts validated sparse geometry; disclosure still requires an actual company receipt. */
TEST_CASE("Asteroid lifecycle installation and native disclosure retain original authority", "[asteroid-live-validation]")
{
	ProbeExtractionRig rig;
	const auto initial = GetAsteroidField();
	REQUIRE(Company::IsValidID(_current_company));
	const auto id = initial.bodies.front().id;
	CHECK_FALSE(GetAsteroidQuantityDisclosure(_current_company, id));
	const std::array receipt{AsteroidKnowledgeRecord{id, uint64_t{1} << _current_company.base()}};
	REQUIRE(InstallLoadedAsteroidKnowledge(receipt));
	const auto quantity = GetAsteroidQuantityDisclosure(_current_company, id);
	REQUIRE(quantity);
	CHECK(quantity->remaining == initial.bodies.front().remaining_reserve);
	CHECK_FALSE(GetAsteroidQuantityDisclosure(CompanyID::Invalid(), id));
	CHECK_FALSE(GetAsteroidQuantityDisclosure(_current_company, OrbitalBodyID{70'000}));
	CHECK_FALSE(GetAsteroidInspectionDisclosure(_current_company, OrbitalBodyID{70'000}));
	auto removed = GetAsteroidField();
	RetireValidationBodies(removed, std::array{id});
	REQUIRE(ValidateAsteroidField(removed));
	auto legacy_removed = removed; legacy_removed.lifecycle_version = 0;
	CHECK_FALSE(InstallAsteroidField(legacy_removed));
	REQUIRE(InstallAsteroidField(removed));
	CHECK_FALSE(GetAsteroidQuantityDisclosure(_current_company, id));
	CHECK_FALSE(GetMutableAsteroidBody(id));
	auto edge = initial;
	edge.elapsed_ticks = 1; edge.bodies.front().motion.position = {0, 0};
	REQUIRE(ValidateAsteroidField(edge));
	auto legacy_edge = edge; legacy_edge.lifecycle_version = 0;
	CHECK_FALSE(InstallAsteroidField(legacy_edge));
	REQUIRE(InstallAsteroidField(edge));
	CHECK(GetMutableAsteroidBody(id)->motion.position == SpacePoint{0, 0});
	CHECK(ValidateAsteroidField(GetAsteroidField()));
	CHECK(ValidateAsteroidKnowledge());
}
