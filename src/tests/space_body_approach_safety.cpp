/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_body_approach_safety.cpp Current-point body-slot filtering before native shared arbitration.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group10C.
 * Extreme bodies and slot records are explicit copied geometry, never published
 * into the still-legacy live field or claimed as paid native commissioning.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_departure.h"
#include "../space_approach.h"
#include "../asteroid.h"
#include "../safeguards.h"

/** Every body approach preserves the original offset while refusing unsafe edge centres. */
TEST_CASE("Body approach slots require both the live centre and offset point inside the plane", "[space-body-approach-safety]")
{
	const auto bounds = *GetSpacePlaneBounds(64, 64);
	CHECK_FALSE(GetSafeSpaceBodyApproachPoint({bounds.max.x, 20 * SPACE_Q16}, bounds, {}));
	CHECK_FALSE(GetSafeSpaceBodyApproachPoint({20 * SPACE_Q16, 0}, bounds, {}));
	CHECK_FALSE(GetSafeSpaceBodyApproachPoint({bounds.max.x, 0}, bounds, {}));
	CHECK_FALSE(GetSafeSpaceBodyApproachPoint({-1, SPACE_Q16}, bounds, {}));
	CHECK(GetSafeSpaceBodyApproachPoint({0, SPACE_Q16 / 2}, bounds, {}) == SpacePoint{SPACE_Q16, 0});
	CHECK(GetSafeSpaceBodyApproachPoint({bounds.max.x - SPACE_Q16, bounds.max.y}, bounds, {}) == SpacePoint{bounds.max.x, bounds.max.y - SPACE_Q16 / 2});
	CHECK_FALSE(GetSafeSpaceBodyApproachPoint({0, 0}, SpacePlaneBounds{}, {}));
}

/** Static geometry cannot be moved or ignored to admit a body's translated craft point. */
TEST_CASE("Body approach filtering preserves copied static footprints and berths", "[space-body-approach-safety]")
{
	const auto bounds = *GetSpacePlaneBounds(64, 64);
	AsteroidBody body;
	body.motion = {{3 * SPACE_Q16, 5 * SPACE_Q16 + SPACE_Q16 / 2}, 16384, -11585, 73, 37};
	const auto original = body;
	const SpacePoint point{4 * SPACE_Q16, 5 * SPACE_Q16};
	const std::array berth{SpaceAttachmentObstacle{point, point, SpaceAttachmentObstacleKind::Approach}};
	CHECK_FALSE(GetSafeSpaceBodyApproachPoint(body.motion.position, bounds, berth));
	const std::array footprint{SpaceAttachmentObstacle{{4 * SPACE_Q16, 4 * SPACE_Q16}, {6 * SPACE_Q16 - 1, 6 * SPACE_Q16 - 1}}};
	CHECK_FALSE(GetSafeSpaceBodyApproachPoint(body.motion.position, bounds, footprint));
	CHECK(GetSafeSpaceBodyApproachPoint(body.motion.position, bounds, {}) == point);
	CHECK(body == original);
	CHECK(berth[0].min == point);
	CHECK(berth[0].max == point);
}

/** Unsafe moving slots disappear from the same shared planner while an existing static occupant retains its saved admission. */
TEST_CASE("Withholding an unsafe body slot preserves a static berth and its owner", "[space-body-approach-safety]")
{
	const auto bounds = *GetSpacePlaneBounds(64, 64);
	const SpacePoint static_point{4 * SPACE_Q16, 5 * SPACE_Q16};
	const SpaceOrderTarget site{SpaceOrderTargetKind::Structure, 7, 10}, body{SpaceOrderTargetKind::Body, 7, 0};
	const std::array requests{
		SpaceApproachRecord{VehicleID{3}, 30, {site, 11, 0, {2 * SPACE_Q16, 2 * SPACE_Q16}}},
		SpaceApproachRecord{VehicleID{8}, 80, {body, 12, 0, {10 * SPACE_Q16, 10 * SPACE_Q16}}}};
	const auto original = requests;
	const std::array obstacles{SpaceAttachmentObstacle{static_point, static_point, SpaceAttachmentObstacleKind::Approach}};
	const auto unsafe_body_point = GetSafeSpaceBodyApproachPoint({static_point.x - SPACE_Q16, static_point.y + SPACE_Q16 / 2}, bounds, obstacles);
	REQUIRE_FALSE(unsafe_body_point);
	const std::array slots{SpaceApproachSlot{site, 0, static_point, VehicleID{3}, false}};
	const auto plan = PlanSpaceApproaches(requests, slots);
	REQUIRE(plan.has_value());
	CHECK((*plan)[0].request == original[0].request);
	CHECK((*plan)[0].craft == VehicleID{3});
	CHECK((*plan)[1].request.slot == UINT8_MAX);
	CHECK((*plan)[1].request.request_tick == 12);
	CHECK((*plan)[1].request.target == body);
	CHECK(requests[0].request == original[0].request);
	CHECK(requests[1].request == original[1].request);
	CHECK(slots[0].occupant == VehicleID{3});
	CHECK(slots[0].point == static_point);
	// Input enumeration cannot preempt the retained static occupant or rescue an
	// unsafe body request through an unrelated equal numeric target ID.
	auto reverse = requests;
	std::ranges::reverse(reverse);
	const auto reordered = PlanSpaceApproaches(reverse, slots);
	REQUIRE(reordered.has_value());
	CHECK((*reordered)[0].request.slot == UINT8_MAX);
	CHECK((*reordered)[1].request == original[0].request);
}
