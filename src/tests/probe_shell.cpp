/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_shell.cpp Model15 physical tuning, shared budget and frozen pursuit boundary contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9C; heavy21 shares physical pursuit while retaining distinct operating quantities.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_content.h"
#include "../multilayer_space_content.h"
#include "../space_vehicle.h"
#include "../space_mining_tuning.h"
#include "../safeguards.h"

/** Check that the commissioned miner and the mission consume one unchanged model and shared economic definition. */
TEST_CASE("Small probe tuning belongs to the existing paid model15", "[probe-shell]")
{
	const auto catalogue = GetMultiLayerVehicleCapabilities();
	const auto miner = std::ranges::find(catalogue, SPACE_PROBE_MODEL, &LayerVehicleCapability::model_id);
	REQUIRE(miner != catalogue.end());
	CHECK(miner->mining_tuning == &GetMultiLayerMiningProbeTuning());
	CHECK(miner->commissioning == MultiLayerCommissioningFacility::RocketPayload);
	CHECK(GetSpaceVehicleConstruction(SPACE_PROBE_MODEL)->kind == SpaceBuildKind::MiningProbe);
	const auto &tuning = *miner->mining_tuning;
	CHECK(tuning.travel_tiles_per_day == 4);
	CHECK(tuning.approach_tolerance_q16 == SPACE_Q16 / 4);
	CHECK(tuning.minimum_approach_ticks == 20 * GetMultiLayerSpaceTuning().ticks_per_economy_day);
	CHECK(tuning.approach_margin_ticks == 2 * GetMultiLayerSpaceTuning().ticks_per_economy_day);
	CHECK(tuning.closing_tiles_per_day == 3);
	// Both mining roles share physical pursuit; construction, survey, rocket and hauler designs retain their original non-mining roles and gain no extraction right.
	for (const auto &model : catalogue) {
		if (GetSpaceMiningTuning(model.model_id) == nullptr) CHECK(model.mining_tuning == nullptr);
		else CHECK(model.mining_tuning == &tuning);
	}
	const auto &budget = GetMultiLayerSpaceBudget();
	CHECK(budget.probe_hold_sixteenths == 20 * 16);
	CHECK(budget.mining_interval == 74); CHECK(budget.mining_units == 2);
	CHECK(budget.active_per_day == 500); CHECK(budget.idle_per_day == 100);
	CHECK(budget.starting_pods == 2); CHECK(budget.returns_per_service == 2); CHECK(budget.service_ticks == 148);
	const auto service = GetSpaceProbeServiceInputs(); REQUIRE(service.size() == 2);
	CHECK(service[0].label == CargoLabel{"RFMT"}); CHECK(service[0].quantity == 1);
	CHECK(service[1].label == CargoLabel{"ELEC"}); CHECK(service[1].quantity == 1);
	SpaceMission unassigned;
	CHECK(unassigned.target == INVALID_ORBITAL_BODY); CHECK(unassigned.phase == SpaceMissionPhase::Idle);
	CHECK(unassigned.approach_ticks_left == 0); CHECK(unassigned.work_ticks == 0);
}

/** Exercise exact ceiling-root boundaries where a rounded-down initial distance would underfund a long approach. */
TEST_CASE("Probe approach allowance freezes upward rounded actual attachment distance", "[probe-shell]")
{
	CHECK(GetSpaceMiningApproachTicks({}, {}) == 1480);
	CHECK(GetSpaceMiningApproachTicks({}, {32 * SPACE_Q16, 0}) == 1480);
	CHECK(GetSpaceMiningApproachTicks({}, {54 * SPACE_Q16, 0}) == 1480);
	CHECK(GetSpaceMiningApproachTicks({}, {54 * SPACE_Q16, 1}) == 1481);
	CHECK(GetSpaceMiningApproachTicks({}, {60 * SPACE_Q16, 0}) == 1628);
	CHECK(GetSpaceMiningApproachTicks({}, {60 * SPACE_Q16, 80 * SPACE_Q16}) == 2615);
	CHECK(GetSpaceMiningApproachTicks({60 * SPACE_Q16, 80 * SPACE_Q16}, {}) == 2615);
	const SpacePoint start{100 * SPACE_Q16, 100 * SPACE_Q16};
	CHECK(GetSpaceMiningApproachTicks(start, {start.x + 60 * SPACE_Q16, start.y + 80 * SPACE_Q16}) == 2615);
	// Integer movement must retain exactly four tiles after one day, including every possible saved fractional phase.
	for (uint8_t phase = 0; phase < GetMultiLayerSpaceTuning().ticks_per_economy_day; ++phase) {
		SpacePoint craft{}; uint8_t remainder = phase;
		for (uint tick = 0; tick < GetMultiLayerSpaceTuning().ticks_per_economy_day; ++tick) {
			CHECK_FALSE(MoveSpaceSurveyTowards(craft, {100 * SPACE_Q16, 0}, remainder));
		}
		CHECK(craft == SpacePoint{4 * SPACE_Q16, 0}); CHECK(remainder == phase);
	}
}
