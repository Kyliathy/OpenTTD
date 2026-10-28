/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_creation.cpp Native one-body publication, exact failed intervals and original ownership conservation.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group28B.
 * Due timers, last-safe geometry, lifetime history and isolated initial hull positions are declared component checkpoints. Real native transactions, earned extraction and allocation failures are exercised; elapsed waits, paid procurement and normal-loop births are not claimed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "probe_mining_fixture.h"
#include "space_return_fixture.h"
#include "allocation_failure_fixture.h"
#include "../asteroid_lifecycle.h"
#include "../safeguards.h"

/** Original owners captured outside a fault scope; comparing a closure allows only its exact independent saved ledger transition. */
struct CreationSnapshot {
	AsteroidField field = GetAsteroidField();
	std::vector<OrbitalBodyID> indexed = QueryAsteroidBodies({0, 0}, GetSpacePlaneBounds(field.width, field.height)->max);
	uint64_t fleet = ComputeSpaceVehicleDigest(), orders = ComputeSpaceOrdersDigest(), clock = TimerGameTick::counter;
	Money money = Company::Get(_current_company)->money;
	std::array<uint32_t, 4> random{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	/** Require the original native cargo/work, money, simulation clock and four global RNG words to remain unchanged. */
	void CheckNativeUnchanged() const
	{
		CHECK(ComputeSpaceVehicleDigest() == this->fleet); CHECK(ComputeSpaceOrdersDigest() == this->orders);
		CHECK(TimerGameTick::counter == this->clock); CHECK(Company::Get(_current_company)->money == this->money);
		CHECK(std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == this->random);
	}
	/** @param renewal Sole expected saved change, or the original ledger for a genuinely stale refusal. */
	void CheckClosure(const AsteroidRenewalState &renewal) const
	{
		auto expected = this->field; expected.renewal = renewal; CHECK(GetAsteroidField() == expected);
		CHECK(QueryAsteroidBodies({0, 0}, GetSpacePlaneBounds(field.width, field.height)->max) == this->indexed);
		this->CheckNativeUnchanged(); CHECK(ValidateAsteroidField(GetAsteroidField()));
	}
};

/** @param size Original size to retire for its own exact credit. @param avoid Optional active source whose native lease/work must survive. Declare only last-safe geometry, then use actual retirement and a declared due checkpoint. */
static void FundNativeCreation(AsteroidSize size = AsteroidSize::Large, OrbitalBodyID avoid = INVALID_ORBITAL_BODY)
{
	auto field = GetAsteroidField();
	/** @param body Original candidate. @return Whether its original size funds this component without touching the protected source. */
	const auto found = std::ranges::find_if(field.bodies, [&](const auto &body) { return body.size == size && body.id != avoid; });
	REQUIRE(found != field.bodies.end()); const auto id = found->id;
	const auto exit = ForecastAsteroidExit(*found, *GetSpacePlaneBounds(field.width, field.height)); REQUIRE(exit); REQUIRE(exit->ticks_until_exit);
	found->motion = exit->last_safe_motion; field.elapsed_ticks = std::max<uint64_t>(field.elapsed_ticks, 1);
	REQUIRE(InstallAsteroidField(std::move(field))); REQUIRE(RetireAsteroidBatch(std::array{id}).status == AsteroidLifecycleStatus::Retired);
	field = GetAsteroidField(); field.renewal.ticks_remaining = 0; REQUIRE(InstallAsteroidField(std::move(field)));
}

/** Validating/quoting cannot consume an interval; each actual successful transaction issues at most one fresh identity and pays exactly its own size. */
TEST_CASE("Native asteroid creation pays once and disarms exactly at zero credit", "[asteroid-creation]")
{
	ProbeExtractionRig rig; FundNativeCreation(); uint32_t births = 0;
	// Explicit due checkpoints exercise successive independent transactions,
	// not elapsed intervals. At most five Small-equivalent credits were earned.
	while (GetAsteroidField().renewal.credits != 0) {
		REQUIRE(births < 5); auto due = GetAsteroidField(); due.renewal.ticks_remaining = 0; REQUIRE(InstallAsteroidField(std::move(due)));
		const CreationSnapshot before; bool valid, injected;
		{ ScopedAllocationFailure fault(0); valid = ValidateAsteroidField(GetAsteroidField()); injected = fault.Injected(); }
		CHECK(valid); CHECK_FALSE(injected); const auto quote = PlanNativeAsteroidRenewal(); REQUIRE(quote.body); REQUIRE(quote.attempt);
		CHECK(quote == PlanNativeAsteroidRenewal()); before.CheckClosure(before.field.renewal);
		const auto expected = FinishAsteroidRenewalAttempt(before.field.renewal, *quote.attempt, true, before.field.next_body_id); REQUIRE(expected);
		const auto result = CommitAsteroidRenewal(quote); REQUIRE(result.status == AsteroidLifecycleStatus::Spawned); REQUIRE(result.spawned);
		CHECK(result.attempt_closed); CHECK(result.retired == 0); CHECK(result.spawned->base() == before.field.next_body_id);
		const auto &after = GetAsteroidField(); CHECK(after.next_body_id == before.field.next_body_id + 1); CHECK(after.bodies.size() == before.field.bodies.size() + 1);
		CHECK(after.renewal == *expected); CHECK(after.knowledge == before.field.knowledge);
		auto expected_body = *quote.body; expected_body.id = *result.spawned; CHECK(*GetMutableAsteroidBody(*result.spawned) == expected_body);
		// Every old resource retains its own finite stock/lease/motion. New IDs
		// neither inherit a departed receipt nor replace an old vector occupant.
		for (const auto &body : before.field.bodies) CHECK(*GetMutableAsteroidBody(body.id) == body);
		const auto bucket = QueryAsteroidBodies(expected_body.motion.position, expected_body.motion.position);
		CHECK(std::ranges::find(bucket, *result.spawned) != bucket.end()); before.CheckNativeUnchanged(); CHECK(ValidateAsteroidField(after));
		const CreationSnapshot closed; CHECK(CommitAsteroidRenewal(quote).status == AsteroidLifecycleStatus::NotDue); closed.CheckClosure(closed.field.renewal); ++births;
	}
	CHECK(births > 0); CHECK(GetAsteroidField().renewal.ticks_remaining == 0); CHECK(PlanNativeAsteroidRenewal().status == AsteroidRenewalProposalStatus::NotDue);
}

/** Stale ledger/cursor/tickets are observational; a modified body belonging to a recognized current ticket closes only one unsuccessful interval. */
TEST_CASE("Native renewal distinguishes stale authority from a changed current proposal", "[asteroid-creation]")
{
	ProbeExtractionRig rig; FundNativeCreation(); const auto quote = PlanNativeAsteroidRenewal(); REQUIRE(quote.attempt); REQUIRE(quote.body);
	SECTION("Modified ticket") {
		auto modified = quote; ++modified.attempt->ticket; const CreationSnapshot before;
		CHECK(CommitAsteroidRenewal(modified).status == AsteroidLifecycleStatus::StaleAttempt); before.CheckClosure(before.field.renewal);
	}
	SECTION("Modified copied ledger") {
		auto modified = quote; ++modified.source_renewal.credits; const CreationSnapshot before;
		CHECK(CommitAsteroidRenewal(modified).status == AsteroidLifecycleStatus::StaleAttempt); before.CheckClosure(before.field.renewal);
	}
	SECTION("Modified issued cursor") {
		auto modified = quote; ++modified.source_next_body_id; const CreationSnapshot before;
		CHECK(CommitAsteroidRenewal(modified).status == AsteroidLifecycleStatus::StaleAttempt); before.CheckClosure(before.field.renewal);
	}
	SECTION("Intervening original exit") {
		FundNativeCreation(AsteroidSize::Small); const CreationSnapshot before;
		CHECK(CommitAsteroidRenewal(quote).status == AsteroidLifecycleStatus::StaleAttempt); before.CheckClosure(before.field.renewal);
	}
	SECTION("Modified unissued reserve") {
		auto modified = quote; --modified.body->remaining_reserve; const CreationSnapshot before;
		const auto failed = FinishAsteroidRenewalAttempt(before.field.renewal, *quote.attempt, false, before.field.next_body_id); REQUIRE(failed);
		const auto result = CommitAsteroidRenewal(modified); CHECK(result.status == AsteroidLifecycleStatus::ProposalChanged); CHECK(result.attempt_closed); CHECK_FALSE(result.spawned);
		before.CheckClosure(*failed); CHECK(CommitAsteroidRenewal(modified).status == AsteroidLifecycleStatus::NotDue); before.CheckClosure(*failed);
	}
}

/** Actual bounded native occupancy must be re-read; no quote can move a hull or silently select a different entry when committing. */
TEST_CASE("Native renewal revalidates hull occupancy and closes blocked placement", "[asteroid-creation]")
{
	ProbeExtractionRig rig; FundNativeCreation(); const auto quote = PlanNativeAsteroidRenewal(); REQUIRE(quote.body); REQUIRE(quote.attempt);
	SECTION("A newly present hull invalidates the quoted entry") {
		rig.ProbeAt(quote.body->motion.position, false); const CreationSnapshot before;
		const auto failed = FinishAsteroidRenewalAttempt(before.field.renewal, *quote.attempt, false, before.field.next_body_id); REQUIRE(failed);
		const auto result = CommitAsteroidRenewal(quote); CHECK(result.status == AsteroidLifecycleStatus::ProposalChanged); CHECK(result.attempt_closed); before.CheckClosure(*failed);
	}
	SECTION("Every fixed alternative is occupied") {
		auto blocked = quote; uint32_t occupants = 0;
		// Declare distinct initial native hull points at successive bounded
		// candidates. No paid arrival is claimed; the real native occupancy owner
		// must exclude all at most16 alternatives without extra saved draws.
		while (blocked.status == AsteroidRenewalProposalStatus::Ready) {
			REQUIRE(occupants++ < 16); REQUIRE(blocked.body); rig.ProbeAt(blocked.body->motion.position, false); blocked = PlanNativeAsteroidRenewal();
		}
		REQUIRE(blocked.status == AsteroidRenewalProposalStatus::NoClearEntry); REQUIRE(blocked.attempt); const CreationSnapshot before;
		const auto failed = FinishAsteroidRenewalAttempt(before.field.renewal, *blocked.attempt, false, before.field.next_body_id); REQUIRE(failed);
		const auto result = CommitAsteroidRenewal(blocked); CHECK(result.status == AsteroidLifecycleStatus::NoClearEntry); CHECK(result.attempt_closed); before.CheckClosure(*failed);
	}
}

/** Fault the actual static-snapshot and field/index preparation allocations; refusal consumes the due interval but never credit, old resources or an identity. */
TEST_CASE("Native renewal allocation refusal closes exactly its saved failed attempt", "[asteroid-creation]")
{
	SpaceReturnRig rig; FundNativeCreation(); const auto due = GetAsteroidField(); const auto quote = PlanNativeAsteroidRenewal(); REQUIRE(quote.body); REQUIRE(quote.attempt);
	const auto failed = FinishAsteroidRenewalAttempt(due.renewal, *quote.attempt, false, due.next_body_id); REQUIRE(failed);
	SECTION("Read-only native quote allocation refusal") {
		const CreationSnapshot before; AsteroidRenewalProposal refused; bool injected;
		{ ScopedAllocationFailure fault(0); refused = PlanNativeAsteroidRenewal(); injected = fault.Injected(); }
		REQUIRE(injected); CHECK(refused.status == AsteroidRenewalProposalStatus::AllocationRefused); CHECK(refused.attempt == quote.attempt); before.CheckClosure(due.renewal);
		const auto result = CommitAsteroidRenewal(refused); CHECK(result.status == AsteroidLifecycleStatus::AllocationRefused); CHECK(result.attempt_closed); before.CheckClosure(*failed);
	}
	SECTION("Every successive commit preparation allocation") {
		uint32_t failures = 0; bool succeeded = false;
		// Each iteration is an independent declared due component. Restore its
		// original complete field before moving the single real failure forward;
		// no native cargo/work owner is rewound or manufactured by this adapter.
		for (uint32_t allowance = 0; allowance < 256; ++allowance) {
			REQUIRE(InstallAsteroidField(due)); const CreationSnapshot before; AsteroidLifecycleResult result; bool injected;
			{ ScopedAllocationFailure fault(allowance); result = CommitAsteroidRenewal(quote); injected = fault.Injected(); }
			if (result.status == AsteroidLifecycleStatus::Spawned) { CHECK_FALSE(injected); succeeded = true; before.CheckNativeUnchanged(); break; }
			REQUIRE(injected); CHECK(result.status == AsteroidLifecycleStatus::AllocationRefused); CHECK(result.attempt_closed); CHECK_FALSE(result.spawned); ++failures; before.CheckClosure(*failed);
			CHECK(CommitAsteroidRenewal(quote).status == AsteroidLifecycleStatus::NotDue); before.CheckClosure(*failed);
		}
		CHECK(succeeded); CHECK(failures > 5); CHECK(ValidateAsteroidField(GetAsteroidField()));
	}
}

/** Insufficient different-size credit and lifetime exhaustion close a valid attempt while retaining all original resources and credit. */
TEST_CASE("Native renewal closes ineligible and exhausted-ID intervals without creation", "[asteroid-creation]")
{
	ProbeExtractionRig rig;
	SECTION("One Small credit cannot buy a different size") { FundNativeCreation(AsteroidSize::Small); }
	SECTION("All lifetime IDs are issued") {
		FundNativeCreation(); auto field = GetAsteroidField(); const auto turnover = uint64_t{UINT32_MAX} - field.bodies.size() - 1;
		// Same bounded funded Small/Medium turnover component as Group28's
		// exhausted-cursor retirement case, with no lifetime-sized allocation.
		field.next_body_id = UINT32_MAX; field.renewal.exited_by_size = {(turnover + 1) / 2, turnover / 2, 1};
		field.renewal.last_exit = AsteroidExitMetadata{AsteroidSize::Medium, AsteroidClass::Metallic}; REQUIRE(InstallAsteroidField(std::move(field)));
	}
	const CreationSnapshot before; const auto quote = PlanNativeAsteroidRenewal(); REQUIRE(quote.attempt);
	const auto expected_status = before.field.next_body_id == UINT32_MAX ? AsteroidLifecycleStatus::IssuanceExhausted : AsteroidLifecycleStatus::NoEligibleSize;
	CHECK(CheckAsteroidRenewalAdmission(before.field, *quote.attempt) == expected_status);
	const auto failed = FinishAsteroidRenewalAttempt(before.field.renewal, *quote.attempt, false, before.field.next_body_id); REQUIRE(failed);
	const auto result = CommitAsteroidRenewal(quote); CHECK(result.status == expected_status); CHECK(result.attempt_closed); CHECK_FALSE(result.spawned); before.CheckClosure(*failed);
}

/** The exact native admission predicate must reject a genuine supported full live field; no fictitious huge map/native pool grants physical arrival or ownership. */
TEST_CASE("Renewal admission rejects a validated full supported live field", "[asteroid-creation]")
{
	auto made = GenerateAsteroidField(4096, 4096, 12345, AsteroidDensity::Dense); REQUIRE(made); auto field = std::move(*made); REQUIRE(field.bodies.size() == MAX_ORBITAL_BODIES);
	const auto large = std::ranges::find(field.bodies, AsteroidSize::Large, &AsteroidBody::size); REQUIRE(large != field.bodies.end());
	const auto id = large->id; const std::array exits{AsteroidExitRecord{id, {large->size, large->resource_class}}};
	const auto credited = ApplyAsteroidExitCredit(field.renewal, exits, field.next_body_id); REQUIRE(credited); field.renewal = *credited;
	REQUIRE(EraseAsteroidBody(field, id)); auto body = field.bodies.front(); body.id = OrbitalBodyID{field.next_body_id};
	body.size = AsteroidSize::Small; body.resource_class = AsteroidClass::Metallic; body.initial_reserve = body.remaining_reserve = 400;
	REQUIRE(InsertAsteroidBody(field, body)); --field.renewal.credits; field.renewal.ticks_remaining = 0; field.elapsed_ticks = 1;
	REQUIRE(ValidateAsteroidField(field)); const auto before = field; const auto attempt = BeginAsteroidRenewalAttempt(field.renewal, field.next_body_id); REQUIRE(attempt);
	CHECK(CheckAsteroidRenewalAdmission(field, *attempt) == AsteroidLifecycleStatus::Capacity); CHECK(field == before);
}

/** An actual native miner earns and retains its original partial work and packets while another original body's credit funds a replacement. */
TEST_CASE("Native creation preserves earned original mining cargo and partial work", "[asteroid-creation]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); rig.Attach(craft);
	// One complete74-opportunity batch plus partial next work must survive a
	// different body's publication; declaring the fixture supplies no freight.
	for (uint tick = 0; tick < 91; ++tick) rig.Step();
	REQUIRE(craft.cargo.TotalCount() == 2); REQUIRE(craft.mission.work_ticks != 0); FundNativeCreation(AsteroidSize::Medium, craft.mission.target);
	const CreationSnapshot before; const auto quote = PlanNativeAsteroidRenewal();
	INFO("native quote status=" << to_underlying(quote.status) << " candidates=" << unsigned{quote.candidates_examined} << " credit=" << before.field.renewal.credits);
	REQUIRE(quote.body); const auto packet = craft.cargo.Packets()->front()->index;
	const auto result = CommitAsteroidRenewal(quote); REQUIRE(result.status == AsteroidLifecycleStatus::Spawned); before.CheckNativeUnchanged();
	CHECK(craft.cargo.Packets()->front()->index == packet); CHECK(craft.cargo.TotalCount() == 2); CHECK(craft.mission.work_ticks != 0); CHECK(ValidateNativeSpaceOrders());
}
