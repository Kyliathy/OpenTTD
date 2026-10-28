/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_retirement.cpp Atomic installed field/index/knowledge retirement and original native cargo safety.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group28.
 * Projected body checkpoints and preexisting disclosure rows are declared geometry/receipt inputs. Native pursuit, freight and partial work are earned through original ticks; no normal-loop event or paid procurement is claimed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "probe_mining_fixture.h"
#include "space_return_fixture.h"
#include "allocation_failure_fixture.h"
#include "../asteroid_lifecycle.h"
#include "../space_mining_departure.h"
#include "../company_base.h"
#include "../vehicle_cmd.h"
#include "../safeguards.h"

/** Capture original owners before an explicit native transaction; scratch snapshots grant no gameplay authority. */
struct RetirementSnapshot {
	AsteroidField field = GetAsteroidField();
	std::vector<std::vector<OrbitalBodyID>> local_queries;
	uint64_t fleet = ComputeSpaceVehicleDigest(), orders = ComputeSpaceOrdersDigest(), clock = TimerGameTick::counter;
	Money money = Company::Get(_current_company)->money;
	std::array<uint32_t, 4> random{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	/** Capture each original body's actual spatial bucket answer before entering any allocation-fault scope. */
	RetirementSnapshot()
	{
		// Per-original-point answers detect partially moved/erased memberships;
		// merely counting all IDs would miss a changed bucket with the same count.
		for (const auto &body : this->field.bodies) this->local_queries.push_back(QueryAsteroidBodies(body.motion.position, body.motion.position));
	}
	/** Require field, derived spatial answers, native work/cargo, accounting, clock and all four global RNG words to retain their original values. */
	void CheckUnchanged() const
	{
		CHECK(GetAsteroidField() == this->field); CHECK(ComputeSpaceVehicleDigest() == this->fleet); CHECK(ComputeSpaceOrdersDigest() == this->orders);
		CHECK(TimerGameTick::counter == this->clock); CHECK(Company::Get(_current_company)->money == this->money);
		CHECK(std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == this->random);
		// Check each original identity's region independently after refusal so a
		// partially published index cannot hide behind unchanged field records.
		for (size_t index = 0; index < this->field.bodies.size(); ++index) {
			const auto point = this->field.bodies[index].motion.position;
			CHECK(QueryAsteroidBodies(point, point) == this->local_queries[index]);
		}
	}
};

/** @param bodies Existing original IDs whose geometry is explicitly projected to their last contained straight point. Publish no outside point and award no work/time/exit credit. */
static void StageRetirementGeometry(std::span<const OrbitalBodyID> bodies)
{
	auto field = GetAsteroidField(); const auto bounds = *GetSpacePlaneBounds(field.width, field.height);
	// This is a declared pre-translation component checkpoint. The native
	// retirement function must still prove every next step actually exits.
	for (const auto id : bodies) {
		auto *body = FindAsteroidBody(field, id); REQUIRE(body);
		const auto exit = ForecastAsteroidExit(*body, bounds); REQUIRE(exit); REQUIRE(exit->ticks_until_exit);
		body->motion = exit->last_safe_motion;
	}
	field.elapsed_ticks = std::max<uint64_t>(field.elapsed_ticks, 1); REQUIRE(InstallAsteroidField(std::move(field)));
}

/** Sorted batch publication earns exact original weights once and filters live disclosure in a single complete commit. */
TEST_CASE("Native asteroid retirement publishes exact batch credit index and knowledge", "[asteroid-retirement]")
{
	ProbeExtractionRig rig; auto field = GetAsteroidField();
	field.knowledge = {{OrbitalBodyID{0}, 1}, {OrbitalBodyID{1}, 1}, {OrbitalBodyID{2}, 1}, {OrbitalBodyID{3}, 1}}; // Declared existing receipts, no earned-scan claim.
	REQUIRE(InstallAsteroidField(std::move(field))); const std::array retired{OrbitalBodyID{0}, OrbitalBodyID{2}}; StageRetirementGeometry(retired);
	const auto original = GetAsteroidField(); std::vector<AsteroidExitRecord> exits;
	for (const auto id : retired) { const auto *body = FindAsteroidBody(original, id); REQUIRE(body); exits.push_back({id, {body->size, body->resource_class}}); }
	const auto expected = ApplyAsteroidExitCredit(original.renewal, exits, original.next_body_id); REQUIRE(expected);
	const auto clock = TimerGameTick::counter; const auto money = Company::Get(_current_company)->money;
	const auto result = RetireAsteroidBatch(retired); CHECK(result.status == AsteroidLifecycleStatus::Retired); CHECK(result.retired == retired.size()); CHECK_FALSE(result.attempt_closed);
	const auto &after = GetAsteroidField(); CHECK(after.renewal == *expected); CHECK(after.next_body_id == original.next_body_id);
	CHECK(after.bodies.size() + retired.size() == original.bodies.size()); CHECK(after.knowledge == std::vector<AsteroidKnowledgeRecord>{{OrbitalBodyID{1}, 1}, {OrbitalBodyID{3}, 1}});
	// Remaining original reserves and every live lookup/index identity retain
	// their own values after compact erasure, independent of the moved slot.
	for (const auto &body : after.bodies) {
		CHECK(body == *FindAsteroidBody(original, body.id));
		const auto nearby = QueryAsteroidBodies(body.motion.position, body.motion.position); CHECK(std::ranges::find(nearby, body.id) != nearby.end());
	}
	CHECK_FALSE(GetMutableAsteroidBody(retired[0])); CHECK_FALSE(GetMutableAsteroidBody(retired[1])); CHECK(ValidateAsteroidField(after));
	CHECK(TimerGameTick::counter == clock); CHECK(Company::Get(_current_company)->money == money);
	const RetirementSnapshot once; CHECK(RetireAsteroidBatch(retired).status == AsteroidLifecycleStatus::InvalidBatch); once.CheckUnchanged();
}

/** Invalid/partial batches never earn an exit or publish an index/native-work change. */
TEST_CASE("Native retirement refuses unsorted absent duplicate and nonterminal batches", "[asteroid-retirement]")
{
	ProbeExtractionRig rig; StageRetirementGeometry(std::array{OrbitalBodyID{0}, OrbitalBodyID{2}}); const RetirementSnapshot before;
	CHECK(RetireAsteroidBatch({}).status == AsteroidLifecycleStatus::NoChange);
	CHECK(RetireAsteroidBatch(std::array{OrbitalBodyID{2}, OrbitalBodyID{0}}).status == AsteroidLifecycleStatus::InvalidBatch);
	CHECK(RetireAsteroidBatch(std::array{OrbitalBodyID{0}, OrbitalBodyID{0}}).status == AsteroidLifecycleStatus::InvalidBatch);
	CHECK(RetireAsteroidBatch(std::array{OrbitalBodyID{0}, OrbitalBodyID{UINT32_MAX - 1}}).status == AsteroidLifecycleStatus::InvalidBatch);
	CHECK(RetireAsteroidBatch(std::array{OrbitalBodyID{1}}).status == AsteroidLifecycleStatus::InvalidBatch); before.CheckUnchanged();
}

/** Actual allocation failures across successive preparation stages cannot partially publish any original owner. */
TEST_CASE("Native retirement allocation refusal preserves all original publication owners", "[asteroid-retirement]")
{
	ProbeExtractionRig rig; const std::array retired{OrbitalBodyID{0}, OrbitalBodyID{2}}; StageRetirementGeometry(retired);
	const RetirementSnapshot before; uint32_t failures = 0; bool succeeded = false;
	// Move the single real failure through all preparation allocations until a
	// complete transaction succeeds. Diagnostics run only after scope disarming;
	// the finite bound prevents a missed failure path becoming an endless test.
	for (uint32_t allowance = 0; allowance < 256; ++allowance) {
		AsteroidLifecycleResult result; bool injected;
		{
			ScopedAllocationFailure fault(allowance); result = RetireAsteroidBatch(retired); injected = fault.Injected();
		}
		if (result.status == AsteroidLifecycleStatus::Retired) { CHECK_FALSE(injected); succeeded = true; break; }
		REQUIRE(injected); CHECK(result.status == AsteroidLifecycleStatus::AllocationRefused); ++failures; before.CheckUnchanged();
	}
	CHECK(succeeded); CHECK(failures > 5); CHECK(ValidateAsteroidField(GetAsteroidField()));
}

/** An actual stopped miner must release its own lease before erasure; earned packet history/cargo and work accounting survive. */
TEST_CASE("Native retirement retains original cargo after checked stopped mining release", "[asteroid-retirement]")
{
	ProbeExtractionRig rig; auto field = GetAsteroidField();
	// Canonical westbound geometry is declared before pursuit so the original
	// (+1,-0.5) attachment remains in-map through the body's own final point.
	field.bodies.front().motion.velocity_x = -16384; field.bodies.front().motion.velocity_y = 0; REQUIRE(InstallAsteroidField(std::move(field)));
	auto &craft = rig.Probe(); rig.Attach(craft);
	for (uint tick = 0; tick < 91; ++tick) rig.Step();
	REQUIRE(craft.cargo.TotalCount() == 2); REQUIRE(craft.mission.work_ticks != 0);
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded());
	const std::array retired{craft.mission.target}; StageRetirementGeometry(retired); UpdateAttachedSpaceCraftTransforms();
	const auto bounds = *GetSpacePlaneBounds(Map::SizeX(), Map::SizeY()); REQUIRE(bounds.Contains(craft.position));
	const RetirementSnapshot leased; CHECK(RetireAsteroidBatch(retired).status == AsteroidLifecycleStatus::ParticipantsActive); leased.CheckUnchanged();
	const auto position = craft.position; const auto packet = craft.cargo.Packets()->front()->index; const auto source = craft.cargo.Packets()->front()->GetSource();
	const auto money = Company::Get(craft.owner)->money; const auto cost = craft.mission.cost_remainder; const auto extracted = craft.extracted_units;
	REQUIRE(ReleaseUnsafeSpaceMiningAttachments() == 1); REQUIRE(ValidateNativeSpaceOrders());
	REQUIRE(RetireAsteroidBatch(retired).status == AsteroidLifecycleStatus::Retired);
	CHECK(craft.position == position); CHECK(craft.vehstatus.Test(VehState::Stopped)); CHECK(craft.cargo.TotalCount() == 2);
	CHECK(craft.cargo.Packets()->front()->index == packet); CHECK(craft.cargo.Packets()->front()->GetSource() == source);
	CHECK(craft.mission.cost_remainder == cost); CHECK(Company::Get(craft.owner)->money == money); CHECK(craft.extracted_units == extracted);
	CHECK(craft.mission.work_ticks == 0); CHECK(craft.mission.continuation.previous_body == retired.front()); CHECK(ValidateNativeSpaceOrders());
}

/** Only successful original field publication wakes an actual matching full-load target wait; refusal remains observationally inert. */
TEST_CASE("Retirement resource notification follows successful publication only", "[asteroid-retirement]")
{
	SpaceReturnRig rig; auto &craft = rig.Miner();
	REQUIRE(CmdSetSpaceMiningPolicy(DoCommandFlag::Execute, craft.index, craft.generation, 0, SpaceMiningPolicy::FullLoadNearest).Succeeded());
	auto &first = rig.Attach(craft); const auto id = first.id; const auto mineral = first.resource_class; first.remaining_reserve = 2;
	// Explicit finite component inputs leave only this original batch. Neither
	// the test nor the controller supplies output packets or completed work.
	for (const auto &row : GetAsteroidField().bodies) if (row.id != id && row.resource_class == mineral) GetMutableAsteroidBody(row.id)->remaining_reserve = 0;
	// Original departure/selection opportunities must earn their named safe
	// wait. No mission phase, mineral pin, cargo or retry is forged here.
	for (uint tick = 0; craft.mission.continuation.phase != MiningContinuationPhase::Waiting; ++tick) { REQUIRE(tick < 5000); rig.Step(); }
	REQUIRE(craft.mission.continuation.waiting == MiningContinuationWait::NoCompatibleAsteroid); REQUIRE(craft.mission.continuation.retry_ticks != 0);
	StageRetirementGeometry(std::array{id}); const RetirementSnapshot before;
	CHECK(RetireAsteroidBatch(std::array{id, id}).status == AsteroidLifecycleStatus::InvalidBatch); before.CheckUnchanged();
	AsteroidLifecycleResult failed;
	{ ScopedAllocationFailure fault(0); failed = RetireAsteroidBatch(std::array{id}); }
	CHECK(failed.status == AsteroidLifecycleStatus::AllocationRefused); before.CheckUnchanged();
	const auto units = craft.cargo.TotalCount(); REQUIRE(units == 2); const auto position = craft.position;
	REQUIRE(RetireAsteroidBatch(std::array{id}).status == AsteroidLifecycleStatus::Retired);
	CHECK(craft.mission.continuation.retry_ticks == 0); CHECK(craft.mission.continuation.phase == MiningContinuationPhase::Waiting);
	CHECK(craft.space_orders.cursor == 0); CHECK(craft.position == position); CHECK(craft.cargo.TotalCount() == units); CHECK_FALSE(GetMutableAsteroidBody(id));
	CHECK(ValidateNativeSpaceOrders());
}

/** Exhausting lifetime issuance does not prevent already issued original bodies from retiring or earning their own exact credit. */
TEST_CASE("Native retirement remains available at the exhausted issued-ID cursor", "[asteroid-retirement]")
{
	ProbeExtractionRig rig; const auto large = std::ranges::find(GetAsteroidField().bodies, AsteroidSize::Large, &AsteroidBody::size); REQUIRE(large != GetAsteroidField().bodies.end());
	const std::array first{large->id}; StageRetirementGeometry(first); REQUIRE(RetireAsteroidBatch(first).status == AsteroidLifecycleStatus::Retired);
	auto field = GetAsteroidField(); const auto small_medium = uint64_t{UINT32_MAX} - field.bodies.size() - 1;
	// Declared complete Small/Medium turnover spends and refunds the same
	// original Large's five credits. Three initial births (Small/Small/Medium)
	// followed by balanced Small/Medium pairs justify the odd lifetime count;
	// no billions-of-events execution or additional earned credit is claimed.
	field.next_body_id = UINT32_MAX; field.renewal.exited_by_size = {(small_medium + 1) / 2, small_medium / 2, 1};
	field.renewal.last_exit = AsteroidExitMetadata{AsteroidSize::Medium, AsteroidClass::Metallic}; REQUIRE(ValidateAsteroidField(field));
	REQUIRE(InstallAsteroidField(std::move(field))); const std::array next{GetAsteroidField().bodies.front().id}; StageRetirementGeometry(next);
	const auto old = GetAsteroidField().renewal; const auto cost = *GetAsteroidRenewalCost(GetMutableAsteroidBody(next.front())->size);
	REQUIRE(RetireAsteroidBatch(next).status == AsteroidLifecycleStatus::Retired);
	CHECK(GetAsteroidField().next_body_id == UINT32_MAX); CHECK(GetAsteroidField().renewal.credits == old.credits + cost);
	CHECK(GetAsteroidField().renewal.ticks_remaining == old.ticks_remaining); CHECK(ValidateAsteroidField(GetAsteroidField()));
}
