/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_attachment_safety.cpp Analytic attached-centre hazards without live movement or lease mutation.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group10.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_departure.h"
#include "../asteroid.h"
#include "../safeguards.h"

/**
 * Find the first unsafe point by iterating only pure native-equivalent proposals.
 * @param motion Copied original body motion, never installed in the field.
 * @param offset Original attachment translation, unchanged throughout the test.
 * @param bounds Explicit supported plane.
 * @param obstacles Copied original static footprint/approach geometry.
 */
static void CheckAttachmentIteration(SpaceMotion motion, SpacePoint offset, const SpacePlaneBounds &bounds, std::span<const SpaceAttachmentObstacle> obstacles)
{
	const auto original = motion;
	const auto body = ForecastSpaceMotionExit(motion, bounds);
	REQUIRE(body.has_value());
	const auto safety = ForecastSpaceAttachmentSafety(motion, *body, offset, bounds, obstacles);
	REQUIRE(safety.has_value());
	REQUIRE(safety->first_unsafe_tick.has_value());
	REQUIRE(*safety->first_unsafe_tick <= 2000);
	std::optional<SpacePoint> last_safe;
	bool unsafe = false;
	// The reference observes successive centres and original obstacles. It does
	// not reuse the analytic time-interval calculation under test.
	for (uint64_t tick = 0; tick <= 2000; ++tick) {
		const SpacePoint point{motion.position.x + offset.x, motion.position.y + offset.y};
		if (!bounds.Contains(motion.position) || !IsSpaceAttachmentPointClear(point, bounds, obstacles)) {
			CHECK(tick == *safety->first_unsafe_tick);
			CHECK(safety->last_safe_point == last_safe);
			unsafe = true;
			break;
		}
		last_safe = point;
		const auto next = StepSpaceMotionStraight(motion, bounds);
		REQUIRE(next.has_value());
		motion = next->proposed;
	}
	REQUIRE(unsafe);
	CHECK(motion.velocity_x == original.velocity_x);
	CHECK(motion.velocity_y == original.velocity_y);
}

/** Offset craft centres can become unsafe before their body, on every edge and corner. */
TEST_CASE("Attachment safety predicts offset map exits without reflecting a body", "[space-attachment-safety]")
{
	const auto bounds = *GetSpacePlaneBounds(64, 64);
	// Each signed corner uses its original offset. Equal diagonal components
	// retain fractional carry rather than rotating the attachment with heading.
	for (int32_t sx : {-1, 1}) for (int32_t sy : {-1, 1}) {
		SpaceMotion motion{{sx < 0 ? SPACE_Q16 + 1 : bounds.max.x - SPACE_Q16 - 1,
			sy < 0 ? SPACE_Q16 + 1 : bounds.max.y - SPACE_Q16 - 1}, sx * 11585, sy * 11585, 73, 37};
		const auto body = *ForecastSpaceMotionExit(motion, bounds);
		const SpacePoint offset{sx * SPACE_Q16, sy * SPACE_Q16};
		const auto safety = ForecastSpaceAttachmentSafety(motion, body, offset, bounds, {});
		REQUIRE(safety.has_value());
		CHECK(safety->first_unsafe_tick == 1);
		CHECK(safety->hazard == SpaceAttachmentHazard::MapEdge);
		CHECK(safety->last_safe_point == SpacePoint{motion.position.x + offset.x, motion.position.y + offset.y});
		CheckAttachmentIteration(motion, offset, bounds, {});
	}
	const SpaceMotion outward{{bounds.max.x, 20 * SPACE_Q16}, 74, 0, 0, 0};
	const auto body = *ForecastSpaceMotionExit(outward, bounds);
	const auto inward_attachment = ForecastSpaceAttachmentSafety(outward, body, {-SPACE_Q16, 0}, bounds, {});
	REQUIRE(inward_attachment.has_value());
	CHECK(inward_attachment->first_unsafe_tick == 1);
	CHECK(inward_attachment->hazard == SpaceAttachmentHazard::BodyExit);
	CheckAttachmentIteration(outward, {-SPACE_Q16, 0}, bounds, {});
}

/** A straight body may cross an outer construction strip while its attached hull must stop before the original footprint. */
TEST_CASE("Attachment forecasts preserve outer strip modules and exact approach points", "[space-attachment-safety]")
{
	const auto bounds = *GetSpacePlaneBounds(64, 64);
	const SpaceMotion motion{{20 * SPACE_Q16, 18 * SPACE_Q16}, 0, -16384, 0, 0};
	CHECK(GetSpaceTravelBand(64, 64).Contains(motion.position));
	const SpacePoint offset{SPACE_Q16, -SPACE_Q16 / 2};
	const std::array module{SpaceAttachmentObstacle{{21 * SPACE_Q16, 13 * SPACE_Q16}, {23 * SPACE_Q16 - 1, 15 * SPACE_Q16 - 1}}};
	const auto saved = module;
	const auto body = *ForecastSpaceMotionExit(motion, bounds);
	const auto safety = ForecastSpaceAttachmentSafety(motion, body, offset, bounds, module);
	REQUIRE(safety.has_value());
	CHECK(safety->first_unsafe_tick == 741);
	CHECK(safety->last_safe_point == SpacePoint{21 * SPACE_Q16, 15 * SPACE_Q16});
	CHECK(safety->hazard == SpaceAttachmentHazard::Footprint);
	CheckAttachmentIteration(motion, offset, bounds, module);
	CHECK(module == saved);
	const SpacePoint fractional_berth{motion.position.x + offset.x, motion.position.y + offset.y - 222};
	const std::array fractional_slot{SpaceAttachmentObstacle{fractional_berth, fractional_berth, SpaceAttachmentObstacleKind::Approach}};
	auto carried = motion; carried.remainder_y = 73;
	const auto carried_body = *ForecastSpaceMotionExit(carried, bounds);
	const auto fractional = ForecastSpaceAttachmentSafety(carried, carried_body, offset, bounds, fractional_slot);
	REQUIRE(fractional.has_value());
	CHECK(fractional->first_unsafe_tick == 1);
	CHECK(fractional->hazard == SpaceAttachmentHazard::Approach);
	CheckAttachmentIteration(carried, offset, bounds, fractional_slot);
	// With no carry the first step is221 units, then jumps past this exact berth.
	// The interval solver must not round that near miss into an admitted collision.
	CHECK(ForecastSpaceAttachmentSafety(motion, body, offset, bounds, fractional_slot)->hazard == SpaceAttachmentHazard::MapEdge);
	// Both signs must intersect X and Y during the same opportunity. Separate
	// visits to each axis range cannot invent a collision with the rectangle.
	for (int32_t sign : {-1, 1}) {
		const int32_t origin = sign > 0 ? bounds.max.x - 20 : 20;
		const SpaceMotion diagonal{{origin, origin}, sign * 74, sign * 74, 0, 0};
		const auto forecast = *ForecastSpaceMotionExit(diagonal, bounds);
		const int32_t low = std::min(origin + sign * 3, origin + sign * 8), high = std::max(origin + sign * 3, origin + sign * 8);
		const int32_t bottom = std::min(origin + sign * 7, origin + sign * 9), top = std::max(origin + sign * 7, origin + sign * 9);
		const std::array obstacle{SpaceAttachmentObstacle{{low, bottom}, {high, top}}};
		CHECK(ForecastSpaceAttachmentSafety(diagonal, forecast, {}, bounds, obstacle)->first_unsafe_tick == 7);
		CheckAttachmentIteration(diagonal, {}, bounds, obstacle);
		const SpacePoint berth{origin + sign * 3, origin + sign * 3};
		const std::array approach{SpaceAttachmentObstacle{berth, berth, SpaceAttachmentObstacleKind::Approach}};
		const auto exact = ForecastSpaceAttachmentSafety(diagonal, forecast, {}, bounds, approach);
		REQUIRE(exact.has_value());
		CHECK(exact->first_unsafe_tick == 3);
		CHECK(exact->hazard == SpaceAttachmentHazard::Approach);
		CheckAttachmentIteration(diagonal, {}, bounds, approach);
		auto overlap = std::array{approach[0], SpaceAttachmentObstacle{berth, berth}};
		CHECK(ForecastSpaceAttachmentSafety(diagonal, forecast, {}, bounds, overlap)->hazard == SpaceAttachmentHazard::Footprint);
		std::ranges::reverse(overlap);
		CHECK(ForecastSpaceAttachmentSafety(diagonal, forecast, {}, bounds, overlap)->hazard == SpaceAttachmentHazard::Footprint);
		const SpacePoint missed{origin + sign * 3, origin + sign * 4};
		const std::array miss{SpaceAttachmentObstacle{missed, missed, SpaceAttachmentObstacleKind::Approach}};
		CHECK(ForecastSpaceAttachmentSafety(diagonal, forecast, {}, bounds, miss)->hazard == SpaceAttachmentHazard::MapEdge);
		CheckAttachmentIteration(diagonal, {}, bounds, miss);
	}
}

/** Invalid offsets/forecasts and stationary conflicts cannot fabricate a safe historical point. */
TEST_CASE("Attachment queries distinguish stationary safety and already unsafe inputs", "[space-attachment-safety]")
{
	const auto bounds = *GetSpacePlaneBounds(64, 64);
	const SpaceMotion stopped{{SPACE_Q16, SPACE_Q16}, 0, 0, 17, 73};
	const auto body = *ForecastSpaceMotionExit(stopped, bounds);
	const auto clear = ForecastSpaceAttachmentSafety(stopped, body, {}, bounds, {});
	REQUIRE(clear.has_value());
	CHECK_FALSE(clear->first_unsafe_tick);
	CHECK(clear->last_safe_point == stopped.position);
	CHECK(clear->hazard == SpaceAttachmentHazard::None);
	const std::array obstacle{SpaceAttachmentObstacle{stopped.position, stopped.position}};
	const auto blocked = ForecastSpaceAttachmentSafety(stopped, body, {}, bounds, obstacle);
	REQUIRE(blocked.has_value());
	CHECK(blocked->first_unsafe_tick == 0);
	CHECK_FALSE(blocked->last_safe_point);
	CheckAttachmentIteration(stopped, {}, bounds, obstacle);
	const auto outside = ForecastSpaceAttachmentSafety(stopped, body, {INT32_MAX, INT32_MIN}, bounds, {});
	REQUIRE(outside.has_value());
	CHECK(outside->first_unsafe_tick == 0);
	CHECK_FALSE(outside->last_safe_point);
	CHECK(outside->hazard == SpaceAttachmentHazard::MapEdge);
	auto stale = body; stale.last_safe_motion.position.x += 1;
	CHECK_FALSE(ForecastSpaceAttachmentSafety(stopped, stale, {}, bounds, {}));
	const std::array malformed{SpaceAttachmentObstacle{{10, 10}, {9, 9}}};
	CHECK_FALSE(ForecastSpaceAttachmentSafety(stopped, body, {}, bounds, malformed));
	CHECK_FALSE(IsSpaceAttachmentPointClear(stopped.position, bounds, malformed));
	CHECK_FALSE(IsSpaceAttachmentPointClear(stopped.position, SpacePlaneBounds{{INT32_MAX, INT32_MAX}}, {}));
	const std::array invalid_berth{SpaceAttachmentObstacle{{0, 0}, {10, 10}, SpaceAttachmentObstacleKind::Approach}};
	CHECK_FALSE(ForecastSpaceAttachmentSafety(stopped, body, {}, bounds, invalid_berth));
	// Long stationary projections retain carry without overflowing their horizon;
	// moving projections stop at the exact first outside opportunity.
	CHECK(ProjectSpaceMotionStraight(stopped, bounds, UINT64_MAX)->proposed == stopped);
	const SpaceMotion moving{{bounds.max.x - 1, 0}, 74, 0, 0, 0};
	CHECK(ProjectSpaceMotionStraight(moving, bounds, 1)->crossed_edges.None());
	CHECK(ProjectSpaceMotionStraight(moving, bounds, 2)->crossed_edges.Test(SpacePlaneEdge::East));
	CHECK_FALSE(ProjectSpaceMotionStraight(moving, bounds, 3));
	const auto wide = *GetSpacePlaneBounds(4096, 4096);
	const SpaceMotion slow{{0, 0}, 1, 0, 0, 0};
	CHECK(ProjectSpaceMotionStraight(slow, wide, uint64_t{19859374153})->proposed.position.x == 268369920);
	CHECK(ProjectSpaceMotionStraight(slow, wide, uint64_t{19859374154})->crossed_edges.Test(SpacePlaneEdge::East));
	CHECK_FALSE(ProjectSpaceMotionStraight(slow, wide, uint64_t{19859374155}));
}
