/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_field_wire.cpp Pure original-owner ASTR checkpoints, legacy layouts and bounded malformed payloads.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group22.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 17; each writer era is lossless or refuses.
 * Aggregate sparse history and staged candidate bodies are declared codec inputs; no executed gameplay departure, spawn or paid procurement is claimed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../saveload/asteroid_field_sl.h"
#include "../safeguards.h"

/** @return Deterministic original generated component field, with no native hull, knowledge or installed gameplay publication. */
static AsteroidField CodecField()
{
	auto field = GenerateAsteroidField(64, 64, 53022769049978ULL, AsteroidDensity::Normal); REQUIRE(field); return *field;
}

/** @param field Copied complete field. Verify current bytes preserve every next-decision field and canonical body identity without live publication. */
static void CheckFieldRoundtrip(const AsteroidField &field)
{
	const auto encoded = EncodeAsteroidField(field); REQUIRE(encoded);
	const auto decoded = DecodeAsteroidField(*encoded, AsteroidFieldWireEra::Lifecycle); REQUIRE(decoded);
	CHECK(decoded->renewal == field.renewal); CHECK(decoded->next_body_id == field.next_body_id); CHECK(decoded->starter_input == field.starter_input);
	CHECK(decoded->generation_version == field.generation_version); CHECK(decoded->lifecycle_version == ASTEROID_LIFECYCLE_VERSION);
	CHECK(ComputeAsteroidFieldDigest(*decoded) == ComputeAsteroidFieldDigest(field));
	CHECK(EncodeAsteroidField(*decoded) == encoded);
	CHECK(AdvanceAsteroidRenewalTimer(decoded->renewal, decoded->next_body_id) == AdvanceAsteroidRenewalTimer(field.renewal, field.next_body_id));
	CHECK(BeginAsteroidRenewalAttempt(decoded->renewal, decoded->next_body_id) == BeginAsteroidRenewalAttempt(field.renewal, field.next_body_id));
}

/** @param field Copied component owner. @param id Original body to remove in this declared staged transaction; no native retirement is published. */
static void RetireCodecBody(AsteroidField &field, OrbitalBodyID id)
{
	const auto *body = FindAsteroidBody(field, id); REQUIRE(body);
	const auto next = ApplyAsteroidExitCredit(field.renewal, std::array{AsteroidExitRecord{id, {body->size, body->resource_class}}}, field.next_body_id); REQUIRE(next);
	field.renewal = *next; field.elapsed_ticks = 1; RemoveAsteroidKnowledge(field, id); REQUIRE(EraseAsteroidBody(field, id));
	REQUIRE(ValidateAsteroidField(field));
}

/** @param bytes Copied current transport buffer. @param offset Exact admitted scalar position. @param value Deliberately changed U32 value. Change only four bounded original bytes. */
static void PutFieldU32(std::vector<uint8_t> &bytes, size_t offset, uint32_t value)
{
	REQUIRE(offset + 4 <= bytes.size());
	for (uint index = 0; index < 4; ++index) bytes[offset + index] = static_cast<uint8_t>(value >> ((3 - index) * 8));
}

/** Armed, due, unsuccessful, paid-credit and changed-last-exit checkpoints keep their exact next proposals across original transport. */
TEST_CASE("ASTR lifecycle transport preserves complete renewal checkpoints", "[asteroid-field-wire]")
{
	auto field = CodecField(); CheckFieldRoundtrip(field);
	const auto large = std::ranges::find(field.bodies, AsteroidSize::Large, &AsteroidBody::size); REQUIRE(large != field.bodies.end());
	RetireCodecBody(field, large->id); CheckFieldRoundtrip(field);
	field.renewal.ticks_remaining = 0; CheckFieldRoundtrip(field); // Explicit pure due-boundary input, not an elapsed gameplay claim.
	const auto attempt = BeginAsteroidRenewalAttempt(field.renewal, field.next_body_id); REQUIRE(attempt);
	const auto failed = FinishAsteroidRenewalAttempt(field.renewal, *attempt, false, field.next_body_id); REQUIRE(failed);
	const auto balance = field.renewal.credits; field.renewal = *failed; CHECK(field.renewal.credits == balance); CheckFieldRoundtrip(field);
	// Each copied successful component proposal also installs its one staged
	// original record and fresh ID before the field is validated. This is no
	// native spawn/placement claim; production atomic publication is Group28.
	for (uint index = 0; field.renewal.credits != 0; ++index) {
		REQUIRE(index < 5); field.renewal.ticks_remaining = 0;
		const auto proposal = BeginAsteroidRenewalAttempt(field.renewal, field.next_body_id); REQUIRE(proposal); REQUIRE(proposal->size);
		const auto issued = field.next_body_id; auto body = field.bodies.front(); body.id = OrbitalBodyID{issued}; body.size = *proposal->size;
		const auto &definition = GetMultiLayerAsteroidDefinitions()[to_underlying(body.resource_class)];
		body.initial_reserve = body.remaining_reserve = definition.small_min * GetMultiLayerAsteroidSizeDefinitions()[to_underlying(body.size)].reserve_multiplier;
		REQUIRE(InsertAsteroidBody(field, body));
		const auto committed = FinishAsteroidRenewalAttempt(field.renewal, *proposal, true, issued); REQUIRE(committed); field.renewal = *committed;
		REQUIRE(ValidateAsteroidField(field)); CheckFieldRoundtrip(field);
	}
	CHECK(field.renewal.ticks_remaining == 0); CheckFieldRoundtrip(field);
	RetireCodecBody(field, field.bodies.front().id); CheckFieldRoundtrip(field);
}

/** Sparse issued IDs and an empty current field remain finite original owners, not dense slots or invisible tombstones. */
TEST_CASE("ASTR lifecycle transport accepts sparse and empty current fields", "[asteroid-field-wire]")
{
	auto field = CodecField(); const auto initial = field.next_body_id;
	const auto original = field.bodies.front(); field.bodies = {original}; field.bodies.front().id = OrbitalBodyID{70'000};
	field.elapsed_ticks = 1; field.next_body_id = 70'001;
	const uint64_t medium_exits = (70'000 - initial) / 2;
	field.renewal.exited_by_size = {70'000 - medium_exits, medium_exits, 0}; field.renewal.credits = initial - *GetAsteroidRenewalCost(original.size);
	field.renewal.last_exit = AsteroidExitMetadata{AsteroidSize::Medium, AsteroidClass::Rich}; field.renewal.ticks_remaining = 741;
	REQUIRE(RebuildAsteroidBodyLookup(field)); REQUIRE(ValidateAsteroidField(field)); CheckFieldRoundtrip(field);
	const auto encoded = EncodeAsteroidField(field); REQUIRE(encoded); const auto decoded = DecodeAsteroidField(*encoded, AsteroidFieldWireEra::Lifecycle); REQUIRE(decoded);
	CHECK(FindAsteroidBody(*decoded, OrbitalBodyID{70'000})); CHECK_FALSE(FindAsteroidBody(*decoded, OrbitalBodyID{0}));
	RetireCodecBody(field, OrbitalBodyID{70'000}); REQUIRE(field.bodies.empty()); CheckFieldRoundtrip(field);
	CHECK(ValidateAsteroidKnowledgeRecords({}, field));
	const auto inactive = EncodeAsteroidField(AsteroidField{}); REQUIRE(inactive);
	CHECK(DecodeAsteroidField(*inactive, AsteroidFieldWireEra::Lifecycle) == AsteroidField{});
}

/** Synthetic component buffers exercise each exact historic layout; genuine immutable native-file migrations have separate application checks. */
TEST_CASE("ASTR legacy decoding preserves original bodies and initializes only missing history", "[asteroid-field-wire]")
{
	const auto field = CodecField(); const auto encoded = EncodeAsteroidField(field); REQUIRE(encoded);
	for (const auto era : {AsteroidFieldWireEra::Preview, AsteroidFieldWireEra::NativeMining, AsteroidFieldWireEra::StarterInputs}) {
		std::vector<uint8_t> legacy(encoded->begin(), encoded->begin() + 51);
		// Remove only fields absent in the declared historic component schema;
		// no native file header is rewritten or passed off as genuine history.
		for (size_t index = 0; index < field.bodies.size(); ++index) {
			const auto first = encoded->begin() + 51 + index * 40;
			legacy.insert(legacy.end(), first, first + (era == AsteroidFieldWireEra::Preview ? 36 : 40));
		}
		if (era == AsteroidFieldWireEra::StarterInputs) legacy.insert(legacy.end(), encoded->end() - ASTEROID_LIFECYCLE_WIRE_BYTES - 26, encoded->end() - ASTEROID_LIFECYCLE_WIRE_BYTES);
		const auto decoded = DecodeAsteroidField(legacy, era); REQUIRE(decoded);
		CHECK(decoded->bodies == field.bodies); CHECK(decoded->seed == field.seed); CHECK(decoded->elapsed_ticks == field.elapsed_ticks);
		CHECK(decoded->generation_version == field.generation_version); CHECK(decoded->next_body_id == field.bodies.size());
		CHECK(decoded->renewal == MakeAsteroidRenewalState(field.seed)); CHECK(decoded->lifecycle_version == ASTEROID_LIFECYCLE_VERSION);
		CHECK(decoded->starter_input == (era == AsteroidFieldWireEra::StarterInputs ? field.starter_input : AsteroidStarterInput{}));
		CHECK_FALSE(DecodeAsteroidField(legacy, AsteroidFieldWireEra::Lifecycle));
	}
	CHECK_FALSE(DecodeAsteroidField(*encoded, AsteroidFieldWireEra::StarterInputs));
}

/** Length/count, canonical scalar, history and ID checks reject malformed bytes before any native field/index publication. */
TEST_CASE("ASTR current transport rejects malformed counts identities and renewal fields", "[asteroid-field-wire]")
{
	const auto field = CodecField(); const auto encoded = EncodeAsteroidField(field); REQUIRE(encoded);
	const auto footer = encoded->size() - ASTEROID_LIFECYCLE_WIRE_BYTES;
	SECTION("truncated") { auto bad = *encoded; bad.pop_back(); CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("trailing") { auto bad = *encoded; bad.push_back(0); CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("oversized") { std::vector<uint8_t> bad(GetAsteroidFieldWireMaximum(AsteroidFieldWireEra::Lifecycle) + 1); CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("body count") { auto bad = *encoded; PutFieldU32(bad, 27, UINT32_MAX); CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("duplicate ID") { auto bad = *encoded; PutFieldU32(bad, 91, 0); CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("future ID") { auto bad = *encoded; PutFieldU32(bad, 51, field.next_body_id); CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("zero issuance") { auto bad = *encoded; PutFieldU32(bad, footer + 2, 0); CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("unknown lifecycle") { auto bad = *encoded; bad[footer + 1] = 2; CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("absent initialized ledger") { auto bad = *encoded; bad[footer + 6] = 0; CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("invalid presence") { auto bad = *encoded; bad[footer + 39] = 2; CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("hidden optional payload") { auto bad = *encoded; bad[footer + 40] = 1; CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("unearned credit") { auto bad = *encoded; bad[footer + 38] = 1; CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("timer bound") { auto bad = *encoded; PutFieldU32(bad, footer + 42, 2223); CHECK_FALSE(DecodeAsteroidField(bad, AsteroidFieldWireEra::Lifecycle)); }
	SECTION("second timer RNG word") {
		auto changed = *encoded; changed[footer + 50] ^= 1;
		const auto decoded = DecodeAsteroidField(changed, AsteroidFieldWireEra::Lifecycle); REQUIRE(decoded);
		CHECK(decoded->renewal.timer_rng.state[1] != field.renewal.timer_rng.state[1]);
		CHECK(ComputeAsteroidFieldDigest(*decoded) != ComputeAsteroidFieldDigest(field));
	}
	SECTION("second candidate RNG word") {
		auto changed = *encoded; changed[footer + 58] ^= 1;
		const auto decoded = DecodeAsteroidField(changed, AsteroidFieldWireEra::Lifecycle); REQUIRE(decoded);
		CHECK(decoded->renewal.candidate_rng.state[1] != field.renewal.candidate_rng.state[1]);
		CHECK(ComputeAsteroidFieldDigest(*decoded) != ComputeAsteroidFieldDigest(field));
	}
}

/** A writer emits only its own layout: an older one is accepted only when it reproduces the field exactly. */
TEST_CASE("ASTR writer layouts are lossless or refuse", "[asteroid-field-wire]")
{
	auto field = CodecField();
	const auto current = EncodeAsteroidField(field); REQUIRE(current);
	CHECK(EncodeAsteroidField(field, AsteroidFieldWireEra::SurveyDeparture) == current); // The default is the current layout.
	CHECK(EncodeAsteroidField(field, AsteroidFieldWireEra::Lifecycle) == current); // Same bytes; only no-target surveys differ in admission.
	// A freshly generated field is dense, in band and carries the seed's renewal
	// state, so the StarterInputs layout (merged 441) holds it completely.
	const auto starter = EncodeAsteroidField(field, AsteroidFieldWireEra::StarterInputs); REQUIRE(starter);
	CHECK(starter->size() == current->size() - ASTEROID_LIFECYCLE_WIRE_BYTES);
	CHECK(std::equal(starter->begin(), starter->end(), current->begin())); // The older layout is the current one minus its trailer.
	const auto reread = DecodeAsteroidField(*starter, AsteroidFieldWireEra::StarterInputs); REQUIRE(reread);
	CHECK(*reread == field);
	// Survey knowledge belongs to SPKN, not ASTR: it never blocks the older layout.
	auto surveyed = field; surveyed.knowledge.push_back({field.bodies.front().id, uint64_t{1}});
	CHECK(EncodeAsteroidField(surveyed, AsteroidFieldWireEra::StarterInputs) == starter);
	CHECK(reread->renewal == MakeAsteroidRenewalState(field.seed)); // Nothing regenerated: the seed already gave this state.
	// Elapsed opportunities alone change nothing: the timer stays unarmed until a
	// body exits, so the field remains representable in StarterInputs.
	auto ticking = field;
	const auto advanced = AdvanceAsteroidRenewalTimer(ticking.renewal, ticking.next_body_id); REQUIRE(advanced);
	ticking.renewal = *advanced;
	CHECK(ticking.renewal == field.renewal);
	CHECK(EncodeAsteroidField(ticking, AsteroidFieldWireEra::StarterInputs) == starter);
	// The first exit earns credit and leaves a gap in the dense IDs: StarterInputs
	// cannot hold that, so the writer refuses instead of dropping it.
	auto retired = field;
	RetireCodecBody(retired, retired.bodies.front().id);
	CHECK_FALSE(EncodeAsteroidField(retired, AsteroidFieldWireEra::StarterInputs));
	const auto lifecycle = EncodeAsteroidField(retired, AsteroidFieldWireEra::Lifecycle); REQUIRE(lifecycle);
	CHECK(DecodeAsteroidField(*lifecycle, AsteroidFieldWireEra::Lifecycle)->renewal == retired.renewal);
	// No writer emits a pre-StarterInputs layout, and an unknown era writes nothing.
	for (const auto era : {AsteroidFieldWireEra::Preview, AsteroidFieldWireEra::NativeMining, AsteroidFieldWireEra::End}) {
		CHECK_FALSE(EncodeAsteroidField(field, era));
	}
	// The inactive field is representable in every writer layout.
	for (const auto era : {AsteroidFieldWireEra::StarterInputs, AsteroidFieldWireEra::Lifecycle, AsteroidFieldWireEra::SurveyDeparture}) {
		const auto inactive = EncodeAsteroidField(AsteroidField{}, era); REQUIRE(inactive);
		CHECK(DecodeAsteroidField(*inactive, era) == AsteroidField{});
	}
}

