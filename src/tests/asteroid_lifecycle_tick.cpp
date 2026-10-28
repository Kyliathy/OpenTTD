/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_lifecycle_tick.cpp Original native field opportunities joining motion, safety, retirement and saved renewal.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group29.
 * Last-contained geometry, explicit due timers and isolated initial worlds are component checkpoints. Native ticks/extraction remain actual; no elapsed interval or paid whole-world acceptance is claimed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "probe_mining_fixture.h"
#include "asteroid_snapshot_fixture.h"
#include "../asteroid_lifecycle.h"
#include "../space_mining_continuation.h"
#include "../space_mining_tuning.h"
#include "../vehicle_cmd.h"
#include "../safeguards.h"

/** @param ids Original native live identities whose final contained straight geometry is explicitly projected. Earn no work/time/credit. */
static void StageLifecycleExits(std::span<const OrbitalBodyID> ids)
{
	auto field = GetAsteroidField(); const auto bounds = *GetSpacePlaneBounds(field.width, field.height);
	// Only geometry changes here; the following actual native opportunity must
	// prove each outward step, release participants and credit each original once.
	for (const auto id : ids) {
		auto *body = FindAsteroidBody(field, id); REQUIRE(body); const auto exit = ForecastAsteroidExit(*body, bounds); REQUIRE(exit); REQUIRE(exit->ticks_until_exit);
		body->motion = exit->last_safe_motion;
	}
	field.elapsed_ticks = std::max<uint64_t>(field.elapsed_ticks, 1); REQUIRE(InstallAsteroidField(std::move(field)));
}

/** @param size Existing original size supplying its own native credit. @param timer Explicit component countdown after actual retirement, not elapsed gameplay. */
static void FundLifecycleTimer(AsteroidSize size, uint32_t timer)
{
	const auto found = std::ranges::find(GetAsteroidField().bodies, size, &AsteroidBody::size); REQUIRE(found != GetAsteroidField().bodies.end());
	const std::array ids{found->id}; StageLifecycleExits(ids); REQUIRE(RetireAsteroidBatch(ids).status == AsteroidLifecycleStatus::Retired);
	auto field = GetAsteroidField(); field.renewal.ticks_remaining = timer; REQUIRE(InstallAsteroidField(std::move(field)));
}

/** Native straight drift crosses the historical strip without a reversal or extra controller opportunity. */
TEST_CASE("Native field tick crosses the old placement strip with unchanged velocity", "[asteroid-lifecycle-tick]")
{
	ProbeExtractionRig rig; auto field = GetAsteroidField(); auto &body = field.bodies.front();
	body.motion.position = field.band.max; body.motion.velocity_x = 16384; body.motion.velocity_y = 0; body.motion.remainder_x = 73; field.elapsed_ticks = 1;
	const auto id = body.id; const auto original = body; REQUIRE(InstallAsteroidField(std::move(field)));
	const auto expected = StepSpaceMotionStraight(original.motion, *GetSpacePlaneBounds(Map::SizeX(), Map::SizeY())); REQUIRE(expected); REQUIRE_FALSE(expected->crossed_edges.Any());
	const auto before = GetAsteroidField(); TickAsteroidField(); const auto *moved = GetMutableAsteroidBody(id); REQUIRE(moved);
	CHECK(moved->motion == expected->proposed); CHECK(moved->motion.velocity_x == original.motion.velocity_x); CHECK(moved->remaining_reserve == original.remaining_reserve);
	CHECK(GetAsteroidField().elapsed_ticks == before.elapsed_ticks + 1); CHECK(GetAsteroidField().renewal == before.renewal); CHECK(ValidateAsteroidField(GetAsteroidField()));
}

/** Sorted simultaneous actual exits credit once and retain the complete freshly armed first delay. */
TEST_CASE("Native field retirement arms first credit without immediately decrementing it", "[asteroid-lifecycle-tick]")
{
	ProbeExtractionRig rig; const std::array ids{OrbitalBodyID{0}, OrbitalBodyID{2}}; StageLifecycleExits(ids); const auto before = GetAsteroidField();
	std::vector<AsteroidExitRecord> exits;
	for (const auto id : ids) { const auto *body = FindAsteroidBody(before, id); REQUIRE(body); exits.push_back({id, {body->size, body->resource_class}}); }
	const auto expected = ApplyAsteroidExitCredit(before.renewal, exits, before.next_body_id); REQUIRE(expected); TickAsteroidField();
	CHECK(GetAsteroidField().renewal == *expected); CHECK(GetAsteroidField().next_body_id == before.next_body_id); CHECK(GetAsteroidField().bodies.size() + ids.size() == before.bodies.size());
	const auto bounds = *GetSpacePlaneBounds(before.width, before.height);
	// Surviving identities consume their separately tested pure step exactly
	// once, despite swap/pop retirement changing their dense storage positions.
	for (const auto &body : GetAsteroidField().bodies) {
		const auto *old = FindAsteroidBody(before, body.id); REQUIRE(old); CHECK(body.motion == StepSpaceMotionStraight(old->motion, bounds)->proposed);
	}
	for (const auto id : ids) CHECK_FALSE(GetMutableAsteroidBody(id));
	CHECK(ValidateAsteroidField(GetAsteroidField())); const auto ledger = GetAsteroidField().renewal; TickAsteroidField();
	CHECK(GetAsteroidField().renewal == *AdvanceAsteroidRenewalTimer(ledger, before.next_body_id));
}

/** A prior armed countdown advances once; one due creation retains its insertion pose until the next field opportunity. */
TEST_CASE("Native due renewal creates at most one body and first moves it next tick", "[asteroid-lifecycle-tick]")
{
	ProbeExtractionRig rig; FundLifecycleTimer(AsteroidSize::Large, 2); const auto issued = GetAsteroidField().next_body_id;
	TickAsteroidField(); REQUIRE(GetAsteroidField().renewal.ticks_remaining == 1); CHECK(GetAsteroidField().next_body_id == issued);
	auto expected = GetAsteroidField(); AdvanceContainedAsteroidSnapshot(expected); expected.renewal = *AdvanceAsteroidRenewalTimer(expected.renewal, expected.next_body_id);
	const auto quote = PlanAsteroidRenewal(expected, {}, {}); REQUIRE(quote.body); REQUIRE(quote.attempt);
	const auto renewed = FinishAsteroidRenewalAttempt(expected.renewal, *quote.attempt, true, expected.next_body_id); REQUIRE(renewed); TickAsteroidField();
	CHECK(GetAsteroidField().next_body_id == issued + 1); CHECK(GetAsteroidField().renewal == *renewed);
	auto body = *quote.body; body.id = OrbitalBodyID{issued}; REQUIRE(GetMutableAsteroidBody(body.id)); CHECK(*GetMutableAsteroidBody(body.id) == body);
	const auto moved = StepSpaceMotionStraight(body.motion, *GetSpacePlaneBounds(expected.width, expected.height)); REQUIRE(moved);
	TickAsteroidField(); CHECK(GetMutableAsteroidBody(body.id)->motion == moved->proposed); CHECK(GetAsteroidField().next_body_id == issued + 1);
	CHECK(GetAsteroidField().renewal.ticks_remaining + 1 == renewed->ticks_remaining); CHECK(ValidateAsteroidField(GetAsteroidField()));
}

/** A current exit supplies latest-size metadata before this opportunity's due selection; expired time cannot use yesterday's exclusion. */
TEST_CASE("Native same-tick exit and expiry select from the newly published ledger", "[asteroid-lifecycle-tick]")
{
	ProbeExtractionRig rig; FundLifecycleTimer(AsteroidSize::Large, 1);
	const auto small = std::ranges::find(GetAsteroidField().bodies, AsteroidSize::Small, &AsteroidBody::size); REQUIRE(small != GetAsteroidField().bodies.end());
	const std::array ids{small->id}; StageLifecycleExits(ids); const auto before = GetAsteroidField(); const auto *departed = FindAsteroidBody(before, ids.front()); REQUIRE(departed);
	const std::array events{AsteroidExitRecord{departed->id, {departed->size, departed->resource_class}}};
	const auto credited = ApplyAsteroidExitCredit(before.renewal, events, before.next_body_id); REQUIRE(credited);
	const auto due = AdvanceAsteroidRenewalTimer(*credited, before.next_body_id); REQUIRE(due); const auto attempt = BeginAsteroidRenewalAttempt(*due, before.next_body_id); REQUIRE(attempt); REQUIRE(attempt->size);
	const auto renewed = FinishAsteroidRenewalAttempt(*due, *attempt, true, before.next_body_id); REQUIRE(renewed); TickAsteroidField();
	CHECK_FALSE(GetMutableAsteroidBody(ids.front())); const auto *born = GetMutableAsteroidBody(OrbitalBodyID{before.next_body_id}); REQUIRE(born);
	CHECK(born->size == *attempt->size); CHECK(born->size != departed->size); CHECK(GetAsteroidField().renewal == *renewed);
	CHECK(GetAsteroidField().bodies.size() == before.bodies.size()); CHECK(GetAsteroidField().next_body_id == before.next_body_id + 1); CHECK(ValidateAsteroidField(GetAsteroidField()));
}

/** Direct lifecycle calls obey the same simulation gate as the ordinary game loop, even with an outward body and due interval ready. */
TEST_CASE("Native lifecycle remains frozen in paused editor menu and bootstrap modes", "[asteroid-lifecycle-tick]")
{
	ProbeExtractionRig rig; FundLifecycleTimer(AsteroidSize::Large, 0); const std::array ids{GetAsteroidField().bodies.front().id}; StageLifecycleExits(ids);
	const auto field = GetAsteroidField(); const auto fleet = ComputeSpaceVehicleDigest(); const auto orders = ComputeSpaceOrdersDigest(); const auto tick = TimerGameTick::counter;
	// Each non-running mode is an independent callback check against the same
	// actual unchanged native owners; no editor/menu opportunity earns credit.
	for (const auto mode : {GameMode::Editor, GameMode::Menu, GameMode::Bootstrap}) {
		AutoRestoreBackup selected(_game_mode, mode); TickAsteroidField(); CHECK(GetAsteroidField() == field);
	}
	{ AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal}); TickAsteroidField(); CHECK(GetAsteroidField() == field); }
	CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeSpaceOrdersDigest() == orders); CHECK(TimerGameTick::counter == tick);
}

/** Pre-translation safety releases a stopped real miner before its target leaves, retaining its bounded existing point and original earned packets. */
TEST_CASE("Native field exit detaches stopped mining before attachment translation", "[asteroid-lifecycle-tick]")
{
	ProbeExtractionRig rig; auto field = GetAsteroidField(); field.bodies.front().motion.velocity_x = -16384; field.bodies.front().motion.velocity_y = 0; REQUIRE(InstallAsteroidField(std::move(field)));
	auto &craft = rig.Probe(); rig.Attach(craft);
	// Earn a complete original batch and a partial next interval before the
	// declared terminal geometry; neither is supplied by the geometry adapter.
	for (uint tick = 0; tick < 91; ++tick) rig.Step();
	REQUIRE(craft.cargo.TotalCount() == 2); REQUIRE(craft.mission.work_ticks != 0); REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft.index, true).Succeeded());
	const std::array ids{craft.mission.target}; StageLifecycleExits(ids); UpdateAttachedSpaceCraftTransforms();
	const auto point = craft.position; const auto packet = craft.cargo.Packets()->front()->index; const auto source = craft.cargo.Packets()->front()->GetSource();
	const auto money = Company::Get(craft.owner)->money; const auto cost = craft.mission.cost_remainder; TickAsteroidField();
	CHECK_FALSE(GetMutableAsteroidBody(ids.front())); CHECK(craft.position == point); CHECK(craft.vehstatus.Test(VehState::Stopped)); CHECK(craft.mission.work_ticks == 0);
	CHECK(craft.cargo.Packets()->front()->index == packet); CHECK(craft.cargo.Packets()->front()->GetSource() == source); CHECK(craft.cargo.TotalCount() == 2);
	CHECK(Company::Get(craft.owner)->money == money); CHECK(craft.mission.cost_remainder == cost); CHECK(HasSpaceMiningContinuationOrder(craft)); CHECK(ValidateNativeSpaceOrders());
	const auto orders = craft.space_orders; const auto intent = craft.mission.continuation;
	const auto idle = GetSpaceMiningTuning(GetSpaceVehicleModel(craft))->idle_per_day; const auto day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
	REQUIRE(craft.Tick());
	// The field producer earns no upkeep. The later ordinary stopped hull tick
	// retains its original one idle-cost accrual while local departure is frozen.
	CHECK(craft.position == point); CHECK(craft.space_orders == orders); CHECK(craft.mission.continuation == intent);
	CHECK(craft.mission.cost_remainder == (cost + idle) % day); CHECK(Company::Get(craft.owner)->money == money - (cost + idle) / day);
}

/** Ownerless transforms must be consistent before due creation validates the original field; no copied stale attachment blocks a legitimate birth. */
TEST_CASE("Native renewal remains valid with an actually attached ownerless preview", "[asteroid-lifecycle-tick]")
{
	ProbeExtractionRig rig; REQUIRE(PrepareSpaceSurveyPreview()); bool attached = false;
	// Let the original observation controller earn its own attachment; no saved
	// phase, scan countdown or attachment lease is assigned by the test.
	for (uint tick = 0; !attached; ++tick) {
		REQUIRE(tick < 3000); TickAsteroidField(); const auto &surveys = GetAsteroidField().surveys;
		attached = std::ranges::find(surveys, SpaceSurveyPhase::Attached, &SpaceSurveyPreview::phase) != surveys.end();
	}
	auto field = GetAsteroidField();
	/** @param body Original non-preview source. @return Whether its actual size can fund a different Small without disturbing the three retained observation targets. */
	const auto funding = std::ranges::find_if(field.bodies, [](const auto &body) { return body.id.base() >= 3 && body.size == AsteroidSize::Medium; }); REQUIRE(funding != field.bodies.end());
	const std::array ids{funding->id}; StageLifecycleExits(ids); REQUIRE(RetireAsteroidBatch(ids).status == AsteroidLifecycleStatus::Retired);
	field = GetAsteroidField(); field.renewal.ticks_remaining = 1; REQUIRE(InstallAsteroidField(std::move(field))); const auto before = GetAsteroidField(); TickAsteroidField();
	CHECK(GetAsteroidField().next_body_id == before.next_body_id + 1); CHECK(GetAsteroidField().bodies.size() == before.bodies.size() + 1); CHECK(ValidateAsteroidField(GetAsteroidField()));
}

/** Terminal retirement can leave no live resources; the fresh delay remains complete and the later due interval inserts only one body. */
TEST_CASE("Native all-body exit preserves an empty field until one delayed replacement", "[asteroid-lifecycle-tick]")
{
	ProbeExtractionRig rig; std::vector<OrbitalBodyID> ids;
	for (const auto *body : GetAsteroidBodiesInIDOrder(GetAsteroidField())) ids.push_back(body->id);
	StageLifecycleExits(ids); const auto issued = GetAsteroidField().next_body_id; TickAsteroidField();
	CHECK(GetAsteroidField().bodies.empty()); CHECK(GetAsteroidField().next_body_id == issued); CHECK(ValidateAsteroidField(GetAsteroidField()));
	const auto ledger = GetAsteroidField().renewal; REQUIRE(ledger.credits >= ids.size()); REQUIRE(ledger.ticks_remaining >= GetAsteroidRenewalDelayBounds().first);
	TickAsteroidField(); CHECK(GetAsteroidField().bodies.empty()); CHECK(GetAsteroidField().renewal == *AdvanceAsteroidRenewalTimer(ledger, issued));
	auto field = GetAsteroidField(); field.renewal.ticks_remaining = 1; REQUIRE(InstallAsteroidField(std::move(field))); TickAsteroidField();
	CHECK(GetAsteroidField().bodies.size() == 1); CHECK(GetAsteroidField().next_body_id == issued + 1); CHECK(ValidateAsteroidField(GetAsteroidField()));
}
