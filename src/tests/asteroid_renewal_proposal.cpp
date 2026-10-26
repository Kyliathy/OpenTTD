/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_renewal_proposal.cpp Pure deterministic single-ticket entry/credit prerequisites.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group27.
 * Sizes, funded terminal/Small-birth history and RNG checkpoints are declared components, not executed lifecycle events or paid assets.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid_renewal.h"
#include "../multilayer_asteroid_catalogue.h"
#include <set>
#include "../safeguards.h"

/** @param latest Last declared exiting size. @param credits Exact desired positive/disarmed balance. @param checkpoint Explicit saved candidate RNG checkpoint. @return Valid finite component field with funded history and a due/disarmed ledger. */
static AsteroidField RenewalProposalField(AsteroidSize latest, uint32_t credits, uint32_t checkpoint = 17)
{
	auto made = GenerateAsteroidField(64, 64, 53022769049978ULL, AsteroidDensity::Normal); REQUIRE(made); auto field = *made;
	// Declare canonical Small sources except the greatest original identity.
	// Preserve each sampled Small-base reserve exactly while scaling its size;
	// this component supplies no live extraction or preserved paid-world claim.
	for (auto &body : field.bodies) {
		const auto base = body.initial_reserve / GetMultiLayerAsteroidSizeDefinitions()[to_underlying(body.size)].reserve_multiplier;
		body.size = body.id.base() == field.next_body_id - 1 ? latest : AsteroidSize::Small;
		body.initial_reserve = body.remaining_reserve = base * GetMultiLayerAsteroidSizeDefinitions()[to_underlying(body.size)].reserve_multiplier;
	}
	// Zero-credit Small history cannot be made by creating another Small after
	// its sole Small exit. The canonical pristine disarmed state needs no such
	// impossible history; latest-size metadata is irrelevant while no credit is due.
	if (latest == AsteroidSize::Small && credits == 0) {
		field.renewal.candidate_rng.SetSeed(checkpoint); REQUIRE(ValidateAsteroidField(field)); return field;
	}
	const auto cost = *GetAsteroidRenewalCost(latest); const uint32_t extras = credits > cost ? credits - cost : 0; REQUIRE(extras < field.bodies.size());
	std::vector<AsteroidExitRecord> exits;
	for (uint32_t index = 0; index < extras; ++index) exits.push_back({OrbitalBodyID{index}, {AsteroidSize::Small, field.bodies[index].resource_class}});
	const auto last = field.bodies.back(); exits.push_back({last.id, {latest, last.resource_class}});
	const auto renewal = ApplyAsteroidExitCredit(field.renewal, exits, field.next_body_id); REQUIRE(renewal); field.renewal = *renewal; field.elapsed_ticks = 1;
	// Retire the named declared originals once. Exact per-body size history,
	// class labels and issuance still validate after compact swap/pop removal.
	for (const auto &exit : exits) REQUIRE(EraseAsteroidBody(field, exit.body));
	// A low remaining balance after a Medium/Large exit is funded by actual
	// declared known Small descriptions; their precise1-unit costs cannot hide
	// behind aggregate bounds or retain every original before first credit.
	for (uint32_t spent = credits; spent < cost; ++spent) {
		auto body = field.bodies.front(); body.id = OrbitalBodyID{field.next_body_id};
		body.size = AsteroidSize::Small; body.initial_reserve = body.remaining_reserve = 400;
		body.resource_class = AsteroidClass::Metallic; REQUIRE(InsertAsteroidBody(field, body)); --field.renewal.credits;
	}
	field.renewal.ticks_remaining = 0; field.renewal.candidate_rng.SetSeed(checkpoint);
	REQUIRE(field.renewal.credits == credits); REQUIRE(ValidateAsteroidField(field)); return field;
}

/** Every reachable affordability/latest-size combination retains Group11's one saved ticket and never invents an issuance. */
TEST_CASE("Renewal proposal preserves every eligible size subset and copied stream", "[asteroid-renewal-proposal]")
{
	std::set<uint8_t> subsets;
	// All thresholds are covered under each latest-size exclusion. Large alone
	// is impossible because a cheaper unexcluded size must also be eligible.
	for (const auto latest : {AsteroidSize::Small, AsteroidSize::Medium, AsteroidSize::Large}) for (uint32_t credits = 0; credits <= 8; ++credits) {
		auto field = RenewalProposalField(latest, credits); const auto original = field; const auto digest = ComputeAsteroidFieldDigest(field);
		const auto proposal = PlanAsteroidRenewal(field, {}, {}); CHECK(ComputeAsteroidFieldDigest(field) == digest); CHECK(field == original);
		if (credits == 0) { CHECK(proposal.status == AsteroidRenewalProposalStatus::NotDue); CHECK_FALSE(proposal.attempt); continue; }
		uint8_t mask = 0;
		// Expected eligibility follows only the public1/3/5 budget and latest-size
		// rule; no test duplicates ticket-local class/geometry generation.
		for (uint8_t size = 0; size < 3; ++size) if (static_cast<AsteroidSize>(size) != latest && *GetAsteroidRenewalCost(static_cast<AsteroidSize>(size)) <= credits) mask |= 1 << size;
		subsets.insert(mask); REQUIRE(proposal.attempt); CHECK(proposal.attempt == BeginAsteroidRenewalAttempt(field.renewal, field.next_body_id));
		if (mask == 0) { CHECK(proposal.status == AsteroidRenewalProposalStatus::NoEligibleSize); CHECK_FALSE(proposal.body); CHECK(proposal.candidates_examined == 0); }
		else {
			REQUIRE(proposal.body); CHECK(proposal.status == AsteroidRenewalProposalStatus::Ready); CHECK(proposal.body->id == INVALID_ORBITAL_BODY);
			CHECK(proposal.body->size != latest); CHECK(*GetAsteroidRenewalCost(proposal.body->size) <= credits);
			CHECK(proposal.body->mining_slot == VehicleID::Invalid()); CHECK(proposal.body->survey_slot == INVALID_SPACE_SURVEY);
		}
	}
	CHECK(subsets == std::set<uint8_t>{0, 1, 2, 3, 5, 6});
}

/** Fixed explicit checkpoints cover canonical minerals/headings/sides and exact replay without touching live random owners. */
TEST_CASE("Renewal entries are finite inward bounded and deterministic on every side", "[asteroid-renewal-proposal]")
{
	std::set<AsteroidClass> classes; std::set<SpacePlaneEdge> sides; std::set<std::pair<int32_t, int32_t>> velocities;
	// Independent declared saved checkpoints provide deterministic coverage,
	// rather than advancing either global RNG or relying on statistical luck.
	for (uint32_t checkpoint = 0; checkpoint < 128; ++checkpoint) {
		const auto field = RenewalProposalField(AsteroidSize::Large, 8, checkpoint * 0x9E3779B9U); const auto random = std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
		const auto proposal = PlanAsteroidRenewal(field, {}, {}); REQUIRE(proposal.body); CHECK(PlanAsteroidRenewal(field, {}, {}) == proposal);
		CHECK(std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random);
		const auto &body = *proposal.body; const auto bounds = *GetSpacePlaneBounds(field.width, field.height); const auto point = body.motion.position;
		classes.insert(body.resource_class); velocities.emplace(body.motion.velocity_x, body.motion.velocity_y);
		CHECK(body.initial_reserve == body.remaining_reserve); const auto factor = GetMultiLayerAsteroidSizeDefinitions()[to_underlying(body.size)].reserve_multiplier;
		const auto definition = GetMultiLayerAsteroidDefinitions()[to_underlying(body.resource_class)];
		CHECK(body.initial_reserve >= definition.small_min * factor); CHECK(body.initial_reserve <= definition.small_max * factor); CHECK(body.initial_reserve % factor == 0);
		CHECK(bounds.Contains(point)); CHECK(proposal.candidates_examined <= 16); CHECK(body.motion.remainder_x == 0); CHECK(body.motion.remainder_y == 0);
		if (point.x == 0) { CHECK(body.motion.velocity_x > 0); sides.insert(SpacePlaneEdge::West); }
		if (point.x == bounds.max.x) { CHECK(body.motion.velocity_x < 0); sides.insert(SpacePlaneEdge::East); }
		if (point.y == 0) { CHECK(body.motion.velocity_y > 0); sides.insert(SpacePlaneEdge::North); }
		if (point.y == bounds.max.y) { CHECK(body.motion.velocity_y < 0); sides.insert(SpacePlaneEdge::South); }
		const auto step = StepSpaceMotionStraight(body.motion, bounds); REQUIRE(step); CHECK_FALSE(step->crossed_edges.Any());
	}
	CHECK(classes.size() == 3); CHECK(sides.size() == 4); CHECK(velocities.size() == 24);
}

/** Blocked geometry uses deterministic alternatives with unchanged content/stream, and complete refusal retains every owner. */
TEST_CASE("Renewal blocked entry alternatives consume no additional saved randomness", "[asteroid-renewal-proposal]")
{
	const auto field = RenewalProposalField(AsteroidSize::Medium, 4); const auto original = PlanAsteroidRenewal(field, {}, {}); REQUIRE(original.body);
	const std::array point_blocker{original.body->motion.position};
	const auto relocated = PlanAsteroidRenewal(field, {}, point_blocker); REQUIRE(relocated.body);
	CHECK(relocated.body->motion.position != original.body->motion.position); CHECK(relocated.attempt == original.attempt);
	auto same_content = *relocated.body; same_content.motion.position = original.body->motion.position; CHECK(same_content == *original.body);
	const auto bounds = *GetSpacePlaneBounds(field.width, field.height);
	const std::array obstructed{SpaceAttachmentObstacle{{0, 0}, bounds.max, SpaceAttachmentObstacleKind::Footprint}};
	const auto blocked = PlanAsteroidRenewal(field, obstructed, {}); CHECK(blocked.status == AsteroidRenewalProposalStatus::NoClearEntry); CHECK_FALSE(blocked.body);
	CHECK(blocked.attempt == original.attempt); CHECK(blocked.candidates_examined > 0); CHECK(blocked.candidates_examined <= 16);
	CHECK(PlanAsteroidRenewal(field, obstructed, {}) == blocked);
	const auto failed = FinishAsteroidRenewalAttempt(field.renewal, *blocked.attempt, false, field.next_body_id); REQUIRE(failed);
	CHECK(failed->credits == field.renewal.credits); CHECK(failed->candidate_rng.state[0] == blocked.attempt->next_candidate_rng.state[0]);
	CHECK(failed->candidate_rng.state[1] == blocked.attempt->next_candidate_rng.state[1]);
}

/** Every physical corner enters both touching edges; thin supported maps and malformed copied input retain exact bounds. */
TEST_CASE("Renewal proposals handle corners narrow planes and invalid copied occupancy", "[asteroid-renewal-proposal]")
{
	auto field = RenewalProposalField(AsteroidSize::Large, 8); const auto bounds = *GetSpacePlaneBounds(field.width, field.height);
	std::set<std::pair<int32_t, int32_t>> corners;
	// Enumerate fixed complete-word checkpoints to cover all four corners. The
	// production planner itself remains bounded to16 nonrandom alternatives.
	for (uint32_t checkpoint = 0; checkpoint < 4096; ++checkpoint) {
		field.renewal.candidate_rng.SetSeed(checkpoint * 0x9E3779B9U);
		const auto proposal = PlanAsteroidRenewal(field, {}, {}); REQUIRE(proposal.body); const auto &motion = proposal.body->motion;
		if ((motion.position.x == 0 || motion.position.x == bounds.max.x) && (motion.position.y == 0 || motion.position.y == bounds.max.y)) {
			corners.emplace(motion.position.x, motion.position.y);
			CHECK((motion.position.x == 0 ? motion.velocity_x : -motion.velocity_x) > 0);
			CHECK((motion.position.y == 0 ? motion.velocity_y : -motion.velocity_y) > 0);
		}
	}
	CHECK(corners.size() == 4);
	const std::array outside{SpacePoint{-1, 0}};
	CHECK(PlanAsteroidRenewal(field, {}, outside).status == AsteroidRenewalProposalStatus::InvalidInput);
	field.renewal.ticks_remaining = 74; CHECK(PlanAsteroidRenewal(field, {}, {}).status == AsteroidRenewalProposalStatus::NotDue);
	const auto thin = GenerateAsteroidField(64, 256, 77, AsteroidDensity::Normal); REQUIRE(thin); auto narrow = *thin;
	std::vector<AsteroidExitRecord> exits;
	// Declare funded complete retirement in this narrow component. Its empty
	// live set is valid and never gains an automatic minimum population credit.
	for (const auto *body : GetAsteroidBodiesInIDOrder(narrow)) exits.push_back({body->id, {body->size, body->resource_class}});
	const auto renewal = ApplyAsteroidExitCredit(narrow.renewal, exits, narrow.next_body_id); REQUIRE(renewal); narrow.renewal = *renewal;
	for (const auto &exit : exits) REQUIRE(EraseAsteroidBody(narrow, exit.body));
	narrow.elapsed_ticks = 1; narrow.renewal.ticks_remaining = 0; REQUIRE(ValidateAsteroidField(narrow));
	const auto proposed = PlanAsteroidRenewal(narrow, {}, {}); REQUIRE(proposed.body);
	CHECK(GetSpacePlaneBounds(64, 256)->Contains(proposed.body->motion.position)); CHECK(narrow.bodies.empty());
}
