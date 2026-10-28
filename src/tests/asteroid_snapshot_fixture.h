/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_snapshot_fixture.h Test-only bounded straight-motion/ownerless snapshot adapter; it grants no native lifecycle authority.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group29.
 * Tests inspecting a contained short horizon reuse the pure motion producer. Native retirement, renewal and hull safety must use TickAsteroidField and their real owners.
 */
#ifndef TESTS_ASTEROID_SNAPSHOT_FIXTURE_H
#define TESTS_ASTEROID_SNAPSHOT_FIXTURE_H
#include "../asteroid.h"
#include "../space_work_cost.h"
#include "../3rdparty/catch2/catch.hpp"

/** Advance one explicitly contained copied-field opportunity without retirement or renewal.
 * @param field Detached component copy with no outward next step. The adapter refuses an installed owner or a terminal horizon before changing any record; it cannot freeze or replace an outward native body.
 */
inline void AdvanceContainedAsteroidSnapshot(AsteroidField &field)
{
	REQUIRE(&field != &GetAsteroidField()); const auto bounds = GetSpacePlaneBounds(field.width, field.height); REQUIRE(bounds);
	// Establish the short horizon before publication. End-of-map behavior needs
	// the separately tested native lifecycle and cannot be supplied by this copy.
	for (const auto &body : field.bodies) {
		const auto step = StepSpaceMotionStraight(body.motion, *bounds); REQUIRE(step); REQUIRE_FALSE(step->crossed_edges.Any());
	}
	ReleaseUnsafeSpaceSurveyPreviews(field, {}); ++field.elapsed_ticks;
	// Only contained pure motions and original ownerless observation advance;
	// no native hull, source packet, credit, ID or saved random stream is written.
	for (auto &body : field.bodies) { body.motion = StepSpaceMotionStraight(body.motion, *bounds)->proposed; RecordSpaceWork(SpaceWorkKind::BodySteps); }
	AdvanceSpaceSurveyPreviews(field);
}
#endif /* TESTS_ASTEROID_SNAPSHOT_FIXTURE_H */
