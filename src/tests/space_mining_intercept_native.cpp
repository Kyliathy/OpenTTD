/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_intercept_native.cpp Forecast timing against actual light/heavy native pursuit.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group18.
 * Native unit owners use declared factory inputs/content; this is not a paid procurement or whole-save acceptance trial.
 */
#include "../stdafx.h"
#include "probe_mining_fixture.h"
#include "space_mining_fixture.h"
#include "../space_mining_target.h"
#include "../space_mining_tuning.h"
#include "../safeguards.h"

/** Compare one clear remaining approach leg with the actual native controller.
 * @param craft Actual light/heavy miner already pursuing its original body.
 * @param step Callback to its fixture's original common field/approach/Vehicle driver.
 */
static void VerifyNativeMiningIntercept(SpaceVehicle &craft, const std::function<void()> &step)
{
	const auto bounds = GetSpacePlaneBounds(Map::SizeX(), Map::SizeY()); REQUIRE(bounds);
	const auto obstacles = GetSpaceAttachmentObstacles(); REQUIRE(obstacles);
	std::optional<SpaceMiningInterceptForecast> forecast;
	// A factory product starts at its own occupied module berth. Let its existing
	// native route leave that source geometry before comparing a clear remaining
	// leg. Neither its position nor an approach/work counter is assigned here.
	for (uint initial = 0; !forecast; ++initial) {
		REQUIRE(initial < 300); REQUIRE(craft.mission.phase == SpaceMissionPhase::Approach);
		const auto *body = FindAsteroidBody(GetAsteroidField(), craft.mission.target); REQUIRE(body != nullptr);
		auto next_body = *body;
		const auto next = StepSpaceMotionStraight(body->motion, *bounds); REQUIRE(next); REQUIRE(next->crossed_edges.None());
		next_body.motion = next->proposed; // The next real driver opportunity moves the field before this hull pursues.
		const auto original = ComputeSpaceVehicleDigest(); const auto original_field = GetAsteroidField();
		forecast = ForecastSpaceMiningIntercept(craft.position, craft.mission.travel_remainder, GetSpaceVehicleModel(craft), next_body, *bounds, *obstacles);
		CHECK(ComputeSpaceVehicleDigest() == original); CHECK(GetAsteroidField() == original_field);
		if (!forecast) step();
	}
	REQUIRE(forecast->arrival_ticks > 1);
	uint elapsed = 0;
	// Actual native ticks must earn the predicted arrival and sole body lease.
	// Comparing the final motion also proves this case never crossed the still-
	// legacy reflection boundary while the future straight forecast was used.
	while (craft.mission.phase != SpaceMissionPhase::Mining) {
		REQUIRE(elapsed < forecast->arrival_ticks + 1); step(); ++elapsed;
	}
	CHECK(elapsed == forecast->arrival_ticks);
	const auto *body = FindAsteroidBody(GetAsteroidField(), craft.mission.target); REQUIRE(body != nullptr);
	CHECK(body->motion == forecast->body_at_arrival); CHECK(body->mining_slot == craft.index);
	CHECK(craft.mission.travel_remainder == forecast->travel_remainder);
	CHECK(craft.position == SpacePoint{body->motion.position.x + SPACE_Q16, body->motion.position.y - SPACE_Q16 / 2});
	CHECK(PlanSpaceDeparture(craft).status == SpaceDepartureStatus::Ready);
}

/** The existing small native miner follows exactly the forecast's copied pursuit clock and travel carry. */
TEST_CASE("Light native mining pursuit matches interception forecast", "[space-mining-intercept-native]")
{
	ProbeExtractionRig rig; auto &craft = rig.Probe(); rig.Step();
	/** Offer the original light-miner common driver without changing its movement contract. */
	const auto step = [&] { rig.Step(); };
	VerifyNativeMiningIntercept(craft, step);
}

/** The actual factory-produced heavy role shares the original native movement; no relabelled light hull stands in for it. */
TEST_CASE("Heavy native mining pursuit matches interception forecast", "[space-mining-intercept-native]")
{
	FactoryMiningRig rig; auto &craft = rig.Miner(); rig.Step();
	REQUIRE(GetSpaceVehicleModel(craft) == SPACE_HEAVY_MINER_MODEL);
	/** Offer the original heavy product's common driver; the fixture owns no alternative movement state. */
	const auto step = [&] { rig.Step(); };
	VerifyNativeMiningIntercept(craft, step);
}
