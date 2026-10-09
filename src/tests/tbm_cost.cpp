/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_cost.cpp Active and idle recurring cost without tick duplication (Group 12): one bucket per tick, a day
 * of mixed intervals with a mid-day reload, a nonzero idle charge, no second running cost, and bankruptcy that stops
 * new cutting without touching wagons, cavities or retreat.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 12
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"
#include "../timer/timer_game_tick.h"
#include "../timer/timer_game_calendar.h"

#include "../safeguards.h"

/** Ticks in one year of daily accounting. */
static constexpr uint64_t TICKS_PER_YEAR = static_cast<uint64_t>(CalendarTime::DAYS_IN_YEAR) * Ticks::DAY_TICKS;

/**
 * Step the project with the same facts several times.
 * @param rig Work rig.
 * @param inputs Facts for every step.
 * @param ticks Number of steps.
 */
static void StepFor(const TBMWorkRig &rig, const TBMPhaseInputs &inputs, int ticks)
{
	for (int tick = 0; tick < ticks; ++tick) REQUIRE(AdvanceTBMProjectPhase(rig.project.id, inputs) == TBMProjectError::None);
}

/** Each tick lands in exactly one bucket; the daily charge uses the buckets, not the state at the moment of charging. */
TEST_CASE("A day of mixed intervals charges each tick once at its state's rate", "[tbm-cost]")
{
	TBMWorkRig rig;
	const auto &model = *GetTBMCutterModel(rig.Project().model_id);
	REQUIRE(model.idle_cost_per_year > 0);
	REQUIRE(rig.Project().active_ticks == 0);
	REQUIRE(rig.Project().idle_ticks == 0);
	TBMPhaseInputs active;
	TBMPhaseInputs waiting;
	waiting.work_wait = TBMBlockedReason::EarthFull;
	TBMPhaseInputs stopped;
	stopped.train_stopped = true;
	TBMPhaseInputs servicing;
	servicing.servicing = true;

	StepFor(rig, active, 20);
	StepFor(rig, waiting, 10);
	rig.Reload(); // A mid-day save keeps both buckets and the carried fraction.
	StepFor(rig, stopped, 15);
	StepFor(rig, servicing, 29);
	CHECK(rig.Project().active_ticks == 20);
	CHECK(rig.Project().idle_ticks == 54);
	CHECK(rig.Project().blocked == TBMBlockedReason::Servicing); // The state at charging time does not choose the rate.

	const uint64_t numerator = 20 * static_cast<uint64_t>(model.active_cost_per_year.base()) + 54 * static_cast<uint64_t>(model.idle_cost_per_year.base());
	const auto expected = ComputeTBMRecurringCharge(rig.Project());
	CHECK(expected.cost == Money(static_cast<int64_t>(numerator / TICKS_PER_YEAR)));
	CHECK(expected.remainder == numerator % TICKS_PER_YEAR);
	const Money cash = Company::Get(_current_company)->money;
	CHECK(ChargeTBMProjectCosts(rig.project.id) == expected.cost);
	CHECK(Company::Get(_current_company)->money == cash - expected.cost);
	CHECK(rig.Project().active_ticks == 0);
	CHECK(rig.Project().idle_ticks == 0);
	CHECK(rig.Project().cost_remainder == expected.remainder);

	/* A whole stopped day still costs money: the idle rate is charged explicitly, unlike ordinary running ticks. */
	StepFor(rig, stopped, static_cast<int>(Ticks::DAY_TICKS));
	const uint64_t idle_day = static_cast<uint64_t>(Ticks::DAY_TICKS) * static_cast<uint64_t>(model.idle_cost_per_year.base()) + expected.remainder;
	CHECK(ChargeTBMProjectCosts(rig.project.id) == Money(static_cast<int64_t>(idle_day / TICKS_PER_YEAR)));
	CHECK(idle_day / TICKS_PER_YEAR > 0);
	CHECK(rig.Project().cost_remainder == idle_day % TICKS_PER_YEAR);
	CHECK(ChargeTBMProjectCosts(TBMProjectID{99}) == 0);
}

/** The cutter's ordinary running cost is replaced while it has a project, so the train is never charged twice. */
TEST_CASE("No second running cost is charged for a cutter with a project", "[tbm-cost]")
{
	TBMWorkRig rig;
	CHECK(rig.front->GetRunningCost() == 0);
	// A completed cut charged its work once; neither the running cost nor the daily accrual repeats it.
	const auto estimate = EstimateExcavationCut(rig.project.job);
	const Money cash = Company::Get(_current_company)->money;
	rig.BoreOneCut();
	CHECK(Company::Get(_current_company)->money == cash - estimate.cost);
	CHECK(rig.front->GetRunningCost() == 0);
	const auto daily = ComputeTBMRecurringCharge(rig.Project());
	const uint64_t numerator = rig.Project().active_ticks * static_cast<uint64_t>(GetTBMCutterModel(0)->active_cost_per_year.base()) + rig.Project().idle_ticks * static_cast<uint64_t>(GetTBMCutterModel(0)->idle_cost_per_year.base()) + rig.Project().cost_remainder;
	CHECK(daily.cost == Money(static_cast<int64_t>(numerator / TICKS_PER_YEAR))); // Duration only; the cut charge is not in it.
}

/** Bankruptcy stops new cutting; loaded wagons, paid cavities and the supported retreat are kept. */
TEST_CASE("Bankruptcy stops new cutting and keeps wagons, cavities and retreat", "[tbm-cost]")
{
	TBMWorkRig rig(2);
	const auto first = rig.BoreOneCut();
	rig.Reload(TBMPhase::Bore); // Back at the face with the first cut's spoil still aboard.
	REQUIRE_FALSE(IsTBMExchangeDue(rig.Project()));
	const uint earth = rig.Spoil(TBMConsistRole::EarthWagon);
	REQUIRE(earth == first.earth_t);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	Company::Get(_current_company)->months_of_bankruptcy = 1;
	for (int tick = 0; tick < 5; ++tick) CHECK(rig.Tick().blocked == TBMBlockedReason::InsufficientFunds);
	CHECK(rig.Job().partial_work_ticks == 0);
	CHECK(rig.Job().cursor == 1);
	CHECK(rig.Spoil(TBMConsistRole::EarthWagon) == earth);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	for (TileIndex tile : GetExcavationRetreatPath(rig.Job(), rig.Job().cursor)) CHECK(IsTBMSupportUsable(rig.Project(), tile));
	// Recovered finances resume the same cut.
	Company::Get(_current_company)->months_of_bankruptcy = 0;
	CHECK(rig.Tick().blocked == TBMBlockedReason::None);
	CHECK(rig.Tick().blocked == TBMBlockedReason::None); // At half cutting speed, two recovered ticks earn one geological unit.
	CHECK(rig.Job().partial_work_ticks == 1);
}
