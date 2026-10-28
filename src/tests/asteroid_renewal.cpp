/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_renewal.cpp Pure size-credit transitions, independent timer/candidate streams and pristine migration gate.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group11.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../gfx_type.h"
#include "../safeguards.h"

/** Restore the sole field after an isolated legacy-install check, without publishing an evolved renewal ledger. */
struct RenewalFieldRestore {
	AsteroidField original = GetAsteroidField();
	/** Restore original generated records/index through the same native install boundary. */
	~RenewalFieldRestore() { CHECK(InstallAsteroidField(std::move(this->original))); }
};

/**
 * Count the exact following simulation opportunities until an armed timer is due.
 * @param state Copied armed ledger; arming itself does not consume an opportunity.
 * @param issued Original issuance count, including any already retired IDs.
 * @return Due copied state with unchanged credit, history and both RNG streams.
 */
static AsteroidRenewalState RenewalDueAfterDelay(AsteroidRenewalState state, uint32_t issued)
{
	const auto original = state;
	REQUIRE(state.ticks_remaining > 0);
	// Only these explicit opportunities advance the pure countdown. Querying,
	// pausing or looking away does not call the step and cannot consume time/RNG.
	for (uint32_t tick = 0; tick < original.ticks_remaining; ++tick) {
		CHECK_FALSE(BeginAsteroidRenewalAttempt(state, issued));
		const auto next = AdvanceAsteroidRenewalTimer(state, issued);
		REQUIRE(next.has_value());
		state = *next;
		CHECK(state.ticks_remaining == original.ticks_remaining - tick - 1);
	}
	CHECK(state.credits == original.credits);
	CHECK(state.exited_by_size == original.exited_by_size);
	CHECK(state.last_exit == original.last_exit);
	CHECK(state.timer_rng.state[0] == original.timer_rng.state[0]);
	CHECK(state.timer_rng.state[1] == original.timer_rng.state[1]);
	CHECK(state.candidate_rng.state[0] == original.candidate_rng.state[0]);
	CHECK(state.candidate_rng.state[1] == original.candidate_rng.state[1]);
	CHECK(AdvanceAsteroidRenewalTimer(state, issued) == state);
	return state;
}

/** Dedicated renewal weights conserve a Large exit through separate Medium/Small replacement opportunities. */
TEST_CASE("Asteroid renewal spends five credits over three delayed different-size births", "[asteroid-renewal]")
{
	CHECK(GetAsteroidRenewalCost(AsteroidSize::Small) == 1);
	CHECK(GetAsteroidRenewalCost(AsteroidSize::Medium) == 3);
	CHECK(GetAsteroidRenewalCost(AsteroidSize::Large) == 5);
	CHECK_FALSE(GetAsteroidRenewalCost(static_cast<AsteroidSize>(3)));
	const auto bounds = GetAsteroidRenewalDelayBounds();
	CHECK(bounds == std::pair<uint32_t, uint32_t>{741, 2222});
	CHECK(bounds.first * MILLISECONDS_PER_TICK >= 20'000);
	CHECK((bounds.first - 1) * MILLISECONDS_PER_TICK < 20'000);
	CHECK(bounds.second * MILLISECONDS_PER_TICK <= 60'000);
	CHECK((bounds.second + 1) * MILLISECONDS_PER_TICK > 60'000);
	const auto initial = MakeAsteroidRenewalState(0);
	CHECK(initial.initialized);
	CHECK(ValidateAsteroidRenewalState(initial, 3));
	CHECK(AdvanceAsteroidRenewalTimer(initial, 3) == initial);
	CHECK_FALSE(BeginAsteroidRenewalAttempt(initial, 3));
	const std::array exit{AsteroidExitRecord{OrbitalBodyID{2}, {AsteroidSize::Large, AsteroidClass::Rich}}};
	const auto credited = ApplyAsteroidExitCredit(initial, exit, 3);
	REQUIRE(credited.has_value());
	CHECK(credited->credits == 5);
	CHECK(credited->exited_by_size == std::array<uint64_t, 3>{0, 0, 1});
	CHECK(credited->ticks_remaining >= bounds.first);
	CHECK(credited->ticks_remaining <= bounds.second);
	auto due = RenewalDueAfterDelay(*credited, 3);
	const auto medium = BeginAsteroidRenewalAttempt(due, 3);
	REQUIRE(medium.has_value());
	CHECK(medium->ticket == 3625017097U); // Literal domain5 seed0 first draw; generation streams are separate.
	CHECK(medium->size == AsteroidSize::Medium);
	auto next = FinishAsteroidRenewalAttempt(due, *medium, true, 3);
	REQUIRE(next.has_value());
	CHECK(next->credits == 2);
	CHECK_FALSE(FinishAsteroidRenewalAttempt(*next, *medium, true, 4));
	// The remaining two credits each buy Small only on a later full interval;
	// the latest exit remains Large, so two successive Small births are legal.
	for (uint32_t issued : {4U, 5U}) {
		due = RenewalDueAfterDelay(*next, issued);
		const auto small = BeginAsteroidRenewalAttempt(due, issued);
		REQUIRE(small.has_value());
		CHECK(small->size == AsteroidSize::Small);
		next = FinishAsteroidRenewalAttempt(due, *small, true, issued);
		REQUIRE(next.has_value());
		CHECK(next->credits == 5 - issued);
		CHECK(next->last_exit == exit[0].metadata);
		CHECK(next->exited_by_size == credited->exited_by_size);
	}
	CHECK(next->ticks_remaining == 0);
	CHECK(next->timer_rng.state[0] == due.timer_rng.state[0]);
	CHECK(next->timer_rng.state[1] == due.timer_rng.state[1]);
	CHECK_FALSE(BeginAsteroidRenewalAttempt(*next, 6));
	CHECK(ValidateAsteroidRenewalState(*next, 6));
}

/** Unaffordable or refused attempts retain credit while consuming exactly one independent candidate ticket and a fresh delay. */
TEST_CASE("Asteroid renewal retains blocked credits without coupling timer randomness", "[asteroid-renewal]")
{
	const auto initial = MakeAsteroidRenewalState(0);
	const std::array exit{AsteroidExitRecord{OrbitalBodyID{0}, {AsteroidSize::Small, AsteroidClass::Metallic}}};
	auto state = *ApplyAsteroidExitCredit(initial, exit, 3);
	// A single Small exit cannot afford a different size. Repeated due attempts
	// retain that one credit and never mint a refill for an otherwise empty field.
	for (uint attempt_number = 0; attempt_number < 3; ++attempt_number) {
		state = RenewalDueAfterDelay(state, 3);
		const auto before = state;
		const auto attempt = BeginAsteroidRenewalAttempt(state, 3);
		REQUIRE(attempt.has_value());
		CHECK_FALSE(attempt->size);
		CHECK(state == before);
		CHECK(BeginAsteroidRenewalAttempt(state, 3) == attempt);
		CHECK_FALSE(FinishAsteroidRenewalAttempt(state, *attempt, true, 3));
		Randomizer one_draw = state.candidate_rng;
		CHECK(attempt->ticket == one_draw.Next());
		const auto next = FinishAsteroidRenewalAttempt(state, *attempt, false, 3);
		REQUIRE(next.has_value());
		CHECK(next->credits == 1);
		CHECK(next->exited_by_size == before.exited_by_size);
		CHECK(next->candidate_rng.state[0] == one_draw.state[0]);
		CHECK(next->candidate_rng.state[1] == one_draw.state[1]);
		CHECK(next->ticks_remaining >= GetAsteroidRenewalDelayBounds().first);
		state = *next;
	}
	const std::array large{AsteroidExitRecord{OrbitalBodyID{2}, {AsteroidSize::Large, AsteroidClass::Rich}}};
	const auto due = RenewalDueAfterDelay(*ApplyAsteroidExitCredit(initial, large, 3), 3);
	const auto attempt = *BeginAsteroidRenewalAttempt(due, 3);
	const auto success = *FinishAsteroidRenewalAttempt(due, attempt, true, 3);
	const auto refusal = *FinishAsteroidRenewalAttempt(due, attempt, false, 3);
	CHECK(success.credits == 2);
	CHECK(refusal.credits == 5);
	CHECK(success.ticks_remaining == refusal.ticks_remaining);
	CHECK(success.timer_rng.state[0] == refusal.timer_rng.state[0]);
	CHECK(success.timer_rng.state[1] == refusal.timer_rng.state[1]);
	CHECK(success.candidate_rng.state[0] == refusal.candidate_rng.state[0]);
	CHECK(success.candidate_rng.state[1] == refusal.candidate_rng.state[1]);
	auto forged = attempt; ++forged.ticket;
	CHECK_FALSE(FinishAsteroidRenewalAttempt(due, forged, true, 3));
	forged = attempt; forged.size = AsteroidSize::Large;
	CHECK_FALSE(FinishAsteroidRenewalAttempt(due, forged, true, 3));
	CHECK_FALSE(FinishAsteroidRenewalAttempt(due, attempt, true, UINT32_MAX));
	CHECK(FinishAsteroidRenewalAttempt(due, attempt, false, UINT32_MAX).has_value());
}

/** Terminal batch order is stable by body ID; later exits never postpone an existing deadline. */
TEST_CASE("Asteroid exit batches preserve stable latest metadata and armed deadlines", "[asteroid-renewal]")
{
	const auto random = _random, interactive = _interactive_random;
	const auto initial = MakeAsteroidRenewalState(12345);
	const std::array events{
		AsteroidExitRecord{OrbitalBodyID{7}, {AsteroidSize::Small, AsteroidClass::Carbonaceous}},
		AsteroidExitRecord{OrbitalBodyID{3}, {AsteroidSize::Large, AsteroidClass::Metallic}},
		AsteroidExitRecord{OrbitalBodyID{9}, {AsteroidSize::Medium, AsteroidClass::Rich}}};
	const auto credited = ApplyAsteroidExitCredit(initial, events, 10);
	REQUIRE(credited.has_value());
	CHECK(credited->credits == 9);
	CHECK(credited->exited_by_size == std::array<uint64_t, 3>{1, 1, 1});
	CHECK(credited->last_exit == events[2].metadata);
	auto reverse = events; std::ranges::reverse(reverse);
	CHECK(ApplyAsteroidExitCredit(initial, reverse, 10) == credited);
	CHECK_FALSE(ApplyAsteroidExitCredit(initial, std::array{events[0], events[0]}, 10));
	CHECK_FALSE(ApplyAsteroidExitCredit(initial, events, 9));
	const std::array later{AsteroidExitRecord{OrbitalBodyID{4}, {AsteroidSize::Large, AsteroidClass::Metallic}}};
	const auto more = ApplyAsteroidExitCredit(*credited, later, 10);
	REQUIRE(more.has_value());
	CHECK(more->credits == 14);
	CHECK(more->last_exit == later[0].metadata);
	CHECK(more->ticks_remaining == credited->ticks_remaining);
	CHECK(more->timer_rng.state[0] == credited->timer_rng.state[0]);
	CHECK(more->timer_rng.state[1] == credited->timer_rng.state[1]);
	CHECK(more->candidate_rng.state[0] == initial.candidate_rng.state[0]);
	CHECK(more->candidate_rng.state[1] == initial.candidate_rng.state[1]);
	CHECK(ApplyAsteroidExitCredit(*more, {}, 10) == more);
	// Pure no-step inspection is the paused contract; only the later native
	// tick owner decides when to invoke Advance, independently of wall time.
	CHECK_FALSE(BeginAsteroidRenewalAttempt(*more, 10));
	CHECK(_random.state[0] == random.state[0]);
	CHECK(_random.state[1] == random.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]);
	CHECK(_interactive_random.state[1] == interactive.state[1]);
}

/** Issuance bounds reject forged history before any 64-bit addition can wrap or mint credit. */
TEST_CASE("Asteroid renewal rejects impossible history metadata and arithmetic", "[asteroid-renewal]")
{
	CHECK_FALSE(ValidateAsteroidRenewalState({}, 10));
	auto state = MakeAsteroidRenewalState(1);
	state.exited_by_size[2] = UINT32_MAX;
	state.credits = uint64_t{UINT32_MAX} * 5;
	state.last_exit = AsteroidExitMetadata{AsteroidSize::Large, AsteroidClass::Rich};
	state.ticks_remaining = GetAsteroidRenewalDelayBounds().second;
	REQUIRE(ValidateAsteroidRenewalState(state, UINT32_MAX));
	const auto original = state;
	const std::array extra{AsteroidExitRecord{OrbitalBodyID{0}, {AsteroidSize::Small, AsteroidClass::Metallic}}};
	CHECK_FALSE(ApplyAsteroidExitCredit(state, extra, UINT32_MAX));
	CHECK(state == original);
	state.exited_by_size[0] = UINT64_MAX;
	CHECK_FALSE(ValidateAsteroidRenewalState(state, UINT32_MAX));
	state = original; state.credits = UINT64_MAX;
	CHECK_FALSE(ValidateAsteroidRenewalState(state, UINT32_MAX));
	state = original; state.last_exit->size = AsteroidSize::Small;
	CHECK_FALSE(ValidateAsteroidRenewalState(state, UINT32_MAX));
	state = original; state.last_exit->resource_class = static_cast<AsteroidClass>(3);
	CHECK_FALSE(ValidateAsteroidRenewalState(state, UINT32_MAX));
	state = original; ++state.ticks_remaining;
	CHECK_FALSE(ValidateAsteroidRenewalState(state, UINT32_MAX));
	state = original; state.credits = 0;
	CHECK_FALSE(ValidateAsteroidRenewalState(state, UINT32_MAX));
}

/** Genuine legacy absence seeds exactly once; inconsistent accounting cannot become a valid current field. */
TEST_CASE("Asteroid fields seed missing legacy renewal without changing generation", "[asteroid-renewal]")
{
	RenewalFieldRestore restore;
	const auto generated = GenerateAsteroidField(64, 64, 53022769049978ULL, AsteroidDensity::Normal);
	REQUIRE(generated.has_value());
	CHECK(generated->renewal == MakeAsteroidRenewalState(generated->seed));
	CHECK(generated->bodies[0].motion == SpaceMotion{{1966080, 1966080}, 5792, -5792, 0, 0});
	CHECK(generated->bodies[0].initial_reserve == 613);
	auto legacy = *generated;
	legacy.lifecycle_version = 0; legacy.renewal = {};
	CHECK(ValidateAsteroidField(legacy));
	REQUIRE(InstallAsteroidField(legacy));
	CHECK(GetAsteroidField() == *generated);
	const auto accepted = GetAsteroidField();
	REQUIRE(InstallAsteroidField(accepted));
	CHECK(GetAsteroidField() == accepted);
	auto evolved = accepted;
	const auto &body = evolved.bodies.front();
	const std::array exit{AsteroidExitRecord{body.id, {body.size, body.resource_class}}};
	evolved.renewal = *ApplyAsteroidExitCredit(evolved.renewal, exit, static_cast<uint32_t>(evolved.bodies.size()));
	CHECK_FALSE(ValidateAsteroidField(evolved));
	CHECK_FALSE(InstallAsteroidField(evolved));
	CHECK(GetAsteroidField() == accepted);
	legacy.renewal.credits = 1;
	CHECK_FALSE(InstallAsteroidField(legacy));
	CHECK(GetAsteroidField() == accepted);
}
