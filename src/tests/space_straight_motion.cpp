/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_straight_motion.cpp Physical-edge proposals and independent repeated-step forecast checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group9.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../multilayer_asteroid_catalogue.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/**
 * Compare an analytic exit with repeated one-step proposals, without publishing a body.
 * @param original Copied real-format body whose signed heading and carry must persist.
 * @param bounds Explicit supported physical plane.
 */
static void CheckRepeatedStraightExit(const AsteroidBody &original, const SpacePlaneBounds &bounds)
{
	const auto forecast = ForecastAsteroidExit(original, bounds);
	REQUIRE(forecast.has_value());
	REQUIRE(forecast->ticks_until_exit.has_value());
	REQUIRE(*forecast->ticks_until_exit <= 2000);
	auto current = original.motion;
	bool exited = false;
	// Walk actual integer proposals until the first outside point. This checks
	// the analytic boundary against iteration, including the entire last-safe
	// motion/carry state, rather than restating its ceiling-division formula.
	for (uint64_t tick = 1; tick <= 2000; ++tick) {
		const auto step = StepSpaceMotionStraight(current, bounds);
		REQUIRE(step.has_value());
		if (step->crossed_edges.Any()) {
			CHECK(tick == *forecast->ticks_until_exit);
			CHECK(current == forecast->last_safe_motion);
			CHECK(step->crossed_edges == forecast->crossed_edges);
			CHECK_FALSE(bounds.Contains(step->proposed.position));
			CHECK(step->proposed.velocity_x == original.motion.velocity_x);
			CHECK(step->proposed.velocity_y == original.motion.velocity_y);
			exited = true;
			break;
		}
		current = step->proposed;
	}
	REQUIRE(exited);
	CHECK(bounds.Contains(forecast->last_safe_motion.position));
}

/** Exact last-centre arrival remains legal and never reflects, independent of the old placement strip. */
TEST_CASE("Straight Space motion leaves the actual plane after its last valid centre", "[space-straight-motion]")
{
	const auto bounds = GetSpacePlaneBounds(64, 64);
	REQUIRE(bounds.has_value());
	const auto band = GetSpaceTravelBand(64, 64);
	const SpaceMotion old_edge{{band.max.x, 32 * SPACE_Q16}, 16384, 0, 73, 0};
	const auto forward = StepSpaceMotionStraight(old_edge, *bounds);
	REQUIRE(forward.has_value());
	CHECK(forward->crossed_edges.None());
	CHECK(forward->proposed.position.x == band.max.x + 222);
	CHECK(forward->proposed.velocity_x == 16384);
	CHECK(forward->proposed.remainder_x == 29);
	const SpaceMotion exact{{bounds->max.x - 1, 20 * SPACE_Q16}, 74, 0, 0, 73};
	const auto arrived = StepSpaceMotionStraight(exact, *bounds);
	REQUIRE(arrived.has_value());
	CHECK(arrived->proposed.position.x == bounds->max.x);
	CHECK(arrived->crossed_edges.None());
	CHECK(arrived->proposed.remainder_y == 73);
	const auto outside = StepSpaceMotionStraight(arrived->proposed, *bounds);
	REQUIRE(outside.has_value());
	CHECK(outside->proposed.position.x == bounds->max.x + 1);
	CHECK(outside->proposed.velocity_x == 74);
	CHECK(outside->crossed_edges == SpacePlaneEdges{SpacePlaneEdge::East});
	const auto forecast = ForecastSpaceMotionExit(exact, *bounds);
	REQUIRE(forecast.has_value());
	CHECK(forecast->ticks_until_exit == 2);
	CHECK(forecast->last_safe_motion == arrived->proposed);
	// Every corner can cross two independent signed components in one tick;
	// visual art size does not rotate, reflect or delay that shared geometry.
	for (int32_t sx : {-1, 1}) for (int32_t sy : {-1, 1}) {
		SpaceMotion corner{{sx < 0 ? 0 : bounds->max.x, sy < 0 ? 0 : bounds->max.y}, sx * 11585, sy * 11585, 73, 73};
		const auto step = StepSpaceMotionStraight(corner, *bounds);
		REQUIRE(step.has_value());
		CHECK(step->proposed.position == SpacePoint{corner.position.x + sx * 157, corner.position.y + sy * 157});
		CHECK(step->proposed.remainder_x == 40);
		CHECK(step->proposed.remainder_y == 40);
		CHECK(step->crossed_edges == SpacePlaneEdges{sx < 0 ? SpacePlaneEdge::West : SpacePlaneEdge::East, sy < 0 ? SpacePlaneEdge::North : SpacePlaneEdge::South});
		CHECK(step->proposed.velocity_x == corner.velocity_x);
		CHECK(step->proposed.velocity_y == corner.velocity_y);
	}
}

/** Enumerate the immutable heading/speed catalogue, all art sizes, both narrow orientations and unequal carries. */
TEST_CASE("Analytic asteroid exits match repeated straight proposals", "[space-straight-motion]")
{
	const auto headings = GetMultiLayerAsteroidHeadingDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto speeds = GetMultiLayerAsteroidSpeedDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(headings.size() == 8);
	REQUIRE(speeds.size() == 3);
	// The complete canonical catalogue and both skinny maps expose axial,
	// diagonal and simultaneous-edge cases without depending on random draws.
	for (const auto [width, height] : {std::pair{64U, 4096U}, std::pair{4096U, 64U}}) {
		const auto bounds = GetSpacePlaneBounds(width, height);
		REQUIRE(bounds.has_value());
		for (const auto &heading : headings) for (const auto &speed : speeds) {
			const int32_t vx = static_cast<int32_t>(int64_t{speed.distance_q16_per_day} * heading.x_q16 / SPACE_Q16);
			const int32_t vy = static_cast<int32_t>(int64_t{speed.distance_q16_per_day} * heading.y_q16 / SPACE_Q16);
			for (const auto size : {AsteroidSize::Small, AsteroidSize::Medium, AsteroidSize::Large}) for (const uint8_t carry : {uint8_t{0}, uint8_t{73}}) {
				AsteroidBody body;
				body.size = size;
				body.motion = {{vx < 0 ? SPACE_Q16 : bounds->max.x - SPACE_Q16, vy < 0 ? SPACE_Q16 : bounds->max.y - SPACE_Q16}, vx, vy, carry, uint8_t{37}};
				const auto saved = body;
				CheckRepeatedStraightExit(body, *bounds);
				CHECK(body == saved);
			}
		}
	}
}

/** Large horizons, zero components, malformed inputs and full signed component limits use defined integer arithmetic. */
TEST_CASE("Straight forecasts retain long horizons and refuse invalid motion", "[space-straight-motion]")
{
	const auto bounds = GetSpacePlaneBounds(4096, 4096);
	REQUIRE(bounds.has_value());
	const SpaceMotion slow{{0, 0}, 1, 0, 0, 73};
	const auto forecast = ForecastSpaceMotionExit(slow, *bounds);
	REQUIRE(forecast.has_value());
	CHECK(forecast->ticks_until_exit == uint64_t{19859374154});
	CHECK(forecast->last_safe_motion.position == SpacePoint{268369920, 0});
	CHECK(forecast->last_safe_motion.remainder_x == 73);
	CHECK(forecast->last_safe_motion.remainder_y == 73);
	const SpaceMotion stopped{{SPACE_Q16, SPACE_Q16}, 0, 0, 17, 73};
	const auto stationary = ForecastSpaceMotionExit(stopped, *bounds);
	REQUIRE(stationary.has_value());
	CHECK_FALSE(stationary->ticks_until_exit.has_value());
	CHECK(stationary->last_safe_motion == stopped);
	CHECK(stationary->crossed_edges.None());
	CHECK(StepSpaceMotionStraight(stopped, *bounds)->proposed == stopped);
	AsteroidBody extreme;
	extreme.motion = {{bounds->max.x, 0}, INT32_MIN, INT32_MAX, 73, 73};
	CheckRepeatedStraightExit(extreme, *bounds);
	CHECK_FALSE(GetSpacePlaneBounds(0, 64));
	CHECK_FALSE(GetSpacePlaneBounds(64, 63));
	CHECK_FALSE(GetSpacePlaneBounds(96, 64));
	CHECK_FALSE(GetSpacePlaneBounds(8192, 64));
	CHECK_FALSE(StepSpaceMotionStraight(slow, SpacePlaneBounds{}));
	CHECK_FALSE(ForecastSpaceMotionExit(slow, SpacePlaneBounds{{INT32_MAX, INT32_MAX}}));
	auto invalid = slow;
	invalid.position.x = -1;
	CHECK_FALSE(StepSpaceMotionStraight(invalid, *bounds));
	invalid = slow; invalid.remainder_y = 74;
	CHECK_FALSE(ForecastSpaceMotionExit(invalid, *bounds));
}

/** Pure queries leave the original generation band, frozen starter inputs, resource records and global streams unchanged. */
TEST_CASE("Exit forecasting does not regenerate or advance an asteroid field", "[space-straight-motion]")
{
	const auto field = GenerateAsteroidField(64, 64, 12345, AsteroidDensity::Normal);
	REQUIRE(field.has_value());
	const auto saved = *field;
	const auto random = _random;
	const auto bounds = GetSpacePlaneBounds(field->width, field->height);
	REQUIRE(bounds.has_value());
	// Inspect every actual generated body without installing or publishing a new
	// position. Lifecycle, persistence and safe detachment remain separate gates.
	for (const auto &body : field->bodies) {
		REQUIRE(ForecastAsteroidExit(body, *bounds).has_value());
		REQUIRE(StepSpaceMotionStraight(body.motion, *bounds).has_value());
	}
	CHECK(*field == saved);
	CHECK(field->band == GetSpaceTravelBand(64, 64));
	CHECK(_random.state[0] == random.state[0]);
	CHECK(_random.state[1] == random.state[1]);
}
