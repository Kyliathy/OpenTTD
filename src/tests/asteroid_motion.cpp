/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_motion.cpp Canonical integer trajectory, attachment-envelope and numerical pursuit fixtures.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group3
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Shared Space geometry and motion defaults
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../multilayer_asteroid_catalogue.h"
#include "../3rdparty/fmt/format.h"
#include <set>
#include "../safeguards.h"

/** Calculate the declared conservative deadline, independently of a native mission's saved timer.
 * @param distance Upward-rounded initial Q16 approach distance.
 * @return At least20 days, or three-tile/day closing time plus two days.
 */
static uint32_t DeclaredApproachDeadline(uint32_t distance)
{
	return static_cast<uint32_t>(std::max<uint64_t>(1480,
		(uint64_t{distance} * 74 + 3 * 65536 - 1) / (3 * 65536) + 148));
}

/** Run geometry-only pursuit on the exact shared motion and four-tile/day travel boundaries.
 * @param body Authoritative moving body record, updated before pursuit each tick.
 * @param craft Actual geometry point; this fixture allocates no vehicle, lease or cargo.
 * @param band Shared body envelope.
 * @param deadline Frozen numerical allowance; current target movement cannot extend it.
 * @return Arrival tick, or zero when the declared allowance expires.
 */
static uint32_t PursuitTicks(SpaceMotion &body, SpacePoint &craft, const SpaceTravelBand &band, uint32_t deadline)
{
	uint8_t remainder = 0;
	// Model the shared schedule and translation offset without claiming the later paid pursuit/attachment controller.
	for (uint32_t tick = 1; tick <= deadline; ++tick) {
		AdvanceSpaceMotion(body, band);
		if (MoveSpaceSurveyTowards(craft, {body.position.x + SPACE_Q16, body.position.y - SPACE_Q16 / 2}, remainder)) return tick;
	}
	return 0;
}

/** Verify immutable motion definitions and a literal pre-change native generation record. */
TEST_CASE("Asteroid motion catalogue preserves the saved heading speed and feature stream", "[asteroid-motion]")
{
	constexpr int32_t expected[][2] = {{65536, 0}, {46340, 46340}, {0, 65536}, {-46340, 46340},
		{-65536, 0}, {-46340, -46340}, {0, -65536}, {46340, -46340}};
	const auto headings = GetMultiLayerAsteroidHeadingDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto speeds = GetMultiLayerAsteroidSpeedDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(headings.size() == 8); REQUIRE(speeds.size() == 3);
	// Every compass direction occupies one equal draw slot; speed is a separate equal three-way draw.
	for (size_t i = 0; i < headings.size(); ++i) {
		CHECK(headings[i].x_q16 == expected[i][0]); CHECK(headings[i].y_q16 == expected[i][1]);
	}
	CHECK(speeds[0].distance_q16_per_day == 4096); CHECK(speeds[1].distance_q16_per_day == 8192);
	CHECK(speeds[2].distance_q16_per_day == 16384);
	CHECK(GetMultiLayerAsteroidHeadingDefinitions(MultiLayerGameplayProfileID::Classic).empty());
	CHECK(GetMultiLayerAsteroidSpeedDefinitions(static_cast<MultiLayerGameplayProfileID>(255)).empty());
	const auto *profile = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(profile->space_tuning == &GetMultiLayerSpaceTuning());
	CHECK(GetMultiLayerContentProfile(MultiLayerGameplayProfileID::Classic)->space_tuning == nullptr);
	CHECK(profile->space_tuning->coordinate_units_per_tile == SPACE_Q16);
	CHECK(profile->space_tuning->ticks_per_economy_day == 74);
	CHECK(profile->space_tuning->body_center_margin_tiles == 2);
	const auto field = GenerateAsteroidField(64, 64, 53022769049978ULL, AsteroidDensity::Normal);
	REQUIRE(field);
	// Literal values come from the accepted pre-change424 native seed12345 Normal world, not a second generator oracle.
	CHECK(field->bodies[0].motion == SpaceMotion{{1966080, 1966080}, 5792, -5792, 0, 0});
	CHECK(field->bodies[1].motion == SpaceMotion{{2228224, 1966080}, 2896, -2896, 0, 0});
	CHECK(field->bodies[2].motion == SpaceMotion{{1966080, 2228224}, 0, -4096, 0, 0});
	CHECK(field->bodies[0].initial_reserve == 613); CHECK(field->bodies[1].initial_reserve == 856);
	CHECK(field->bodies[2].initial_reserve == 352);
}

/** Check all24 cardinal/diagonal speed combinations against an independent cumulative integer-distance oracle. */
TEST_CASE("Asteroid fractional travel is exact on every heading and speed", "[asteroid-motion]")
{
	constexpr int32_t headings[][2] = {{65536, 0}, {46340, 46340}, {0, 65536}, {-46340, 46340},
		{-65536, 0}, {-46340, -46340}, {0, -65536}, {46340, -46340}};
	const auto band = GetSpaceTravelBand(64, 64);
	const SpacePoint origin{32 * SPACE_Q16, 32 * SPACE_Q16};
	// Exhaust all declared combinations; even the slowest diagonal retains rather than discards its fractional step.
	for (const auto &heading : headings) for (int32_t speed : {4096, 8192, 16384}) {
		const int32_t vx = int64_t{speed} * heading[0] / 65536, vy = int64_t{speed} * heading[1] / 65536;
		SpaceMotion motion{origin, vx, vy, 0, 0};
		// Compare each tick to accumulated distance, rather than repeating the implementation's remainder update.
		for (int32_t ticks = 1; ticks <= 74; ++ticks) {
			AdvanceSpaceMotion(motion, band);
			CHECK(motion.position.x == origin.x + int64_t{vx} * ticks / 74);
			CHECK(motion.position.y == origin.y + int64_t{vy} * ticks / 74);
			CHECK(motion.remainder_x == std::abs(vx) * ticks % 74);
			CHECK(motion.remainder_y == std::abs(vy) * ticks % 74);
		}
		CHECK(motion.velocity_x == vx); CHECK(motion.velocity_y == vy);
	}
}

/** Check maximum carry, exact arrival and all corner signs while keeping the attachment offset unchanged. */
TEST_CASE("Asteroid reflection translates attachment envelopes without rotating offsets", "[asteroid-motion]")
{
	const auto band = GetSpaceTravelBand(64, 64);
	const SpacePoint offset{SPACE_Q16, -SPACE_Q16 / 2};
	SpaceMotion edge{{band.max.x, 32 * SPACE_Q16}, 16384, 0, 73, 0};
	AdvanceSpaceMotion(edge, band);
	CHECK(edge.position.x == band.max.x - 222); CHECK(edge.velocity_x == -16384); CHECK(edge.remainder_x == 29);
	SpaceMotion exact{{band.max.x - 1, 32 * SPACE_Q16}, 74, 0, 0, 0};
	AdvanceSpaceMotion(exact, band); CHECK(exact.position.x == band.max.x); CHECK(exact.velocity_x == -74);
	// Every corner mirrors both components; the same saved offset stays within the middle-half strip's reserved envelope.
	for (int32_t sx : {-1, 1}) for (int32_t sy : {-1, 1}) {
		const SpacePoint initial{sx < 0 ? band.min.x : band.max.x, sy < 0 ? band.min.y : band.max.y};
		SpaceMotion corner{initial, sx * 11585, sy * 11585, 73, 73};
		AdvanceSpaceMotion(corner, band);
		CHECK(corner.position == SpacePoint{initial.x - sx * 157, initial.y - sy * 157});
		CHECK(corner.velocity_x == -sx * 11585); CHECK(corner.velocity_y == -sy * 11585);
		CHECK(corner.remainder_x == 40); CHECK(corner.remainder_y == 40);
		const SpacePoint attached{corner.position.x + offset.x, corner.position.y + offset.y};
		CHECK(attached.x >= 0); CHECK(attached.x < 64 * SPACE_Q16);
		CHECK(attached.y >= 16 * SPACE_Q16); CHECK(attached.y < 48 * SPACE_Q16);
	}
}

/** Validate the declared conservative deadline and two-return travel cost without fabricating paid cargo or service acceptance. */
TEST_CASE("Asteroid maximum drift fits numerical starter pursuit and travel budgets", "[asteroid-motion]")
{
	const auto band = GetSpaceTravelBand(64, 64);
	SpaceMotion body{{52 * SPACE_Q16, 30 * SPACE_Q16}, 16384, 0, 0, 0};
	const SpacePoint home{21 * SPACE_Q16, 30 * SPACE_Q16 - SPACE_Q16 / 2};
	SpacePoint craft = home;
	CHECK(DeclaredApproachDeadline(32 * SPACE_Q16) == 1480);
	CHECK((32 * 74 + 3 - 1) / 3 == 790);
	const auto first = PursuitTicks(body, craft, band, 1480);
	CHECK(first == 632);
	uint32_t idle_ticks = first;
	// Two geometry collections each last740 work ticks; every return follows the same current-body schedule while flying home.
	for (uint32_t batch = 0; batch < 2; ++batch) {
		if (batch != 0) {
			const auto approach = PursuitTicks(body, craft, band, 1480);
			REQUIRE(approach != 0); idle_ticks += approach;
		}
		for (uint32_t tick = 0; tick < 740; ++tick) AdvanceSpaceMotion(body, band);
		craft = {body.position.x + SPACE_Q16, body.position.y - SPACE_Q16 / 2};
		uint8_t remainder = 0;
		bool returned = false;
		// Count the finite return's actual four-tile/day travel, including body drift while the detached craft flies.
		for (uint32_t tick = 0; tick < 1480; ++tick) {
			AdvanceSpaceMotion(body, band); ++idle_ticks;
			if (MoveSpaceSurveyTowards(craft, home, remainder)) { returned = true; break; }
		}
		REQUIRE(returned);
	}
	const uint32_t travel_cost = idle_ticks * 100 / 74, cost_remainder = idle_ticks * 100 % 74;
	CHECK(travel_cost > 0); CHECK(cost_remainder < 74);
	fmt::print("ASTEROID_MOTION first_32_ticks={} idle_ticks={} travel_cost={} cost_remainder={} two_return_total={}\n",
		first, idle_ticks, travel_cost, cost_remainder, 25900 + travel_cost);
	AsteroidStarterInput distant;
	distant.kind = AsteroidStarterKind::Site; distant.site_x = 10; distant.site_y = 10; distant.rotation = 0;
	distant.preference = {10 * SPACE_Q16, 10 * SPACE_Q16};
	const auto field = GenerateAsteroidField(64, 4096, 12345, AsteroidDensity::Normal, distant);
	REQUIRE(field);
	const auto bound = GetAsteroidInitialApproachDistance(field->starter_input); REQUIRE(bound);
	CHECK(*bound > 32 * SPACE_Q16);
	const uint32_t deadline = DeclaredApproachDeadline(*bound);
	CHECK(deadline > 1480);
	// All three actual generated starter positions fit the frozen bound even with fastest outward motion along the narrow band.
	for (uint32_t i = 0; i < 3; ++i) {
		SpaceMotion target{field->bodies[i].motion.position, 0, 16384, 0, 0};
		SpacePoint probe = distant.preference;
		const auto arrived = PursuitTicks(target, probe, field->band, deadline);
		CHECK(arrived != 0); CHECK(arrived <= deadline);
	}
}
