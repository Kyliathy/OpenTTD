/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_conservation.cpp Material conservation of boring (Plan 4 Group 18.1): every Earth and Rock tonne a job has
 * committed is in exactly one place, aboard a train, on a platform or with a receiver, through partial and committed
 * cuts, a reversal and a save during work, cancellation, the wagon exchange and delivery. A partial cut creates no
 * material.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 18
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"

#include "../safeguards.h"

/** Earth and Rock tonnes, kept apart. */
struct SpoilLedger {
	uint64_t earth = 0; ///< Earth tonnes.
	uint64_t rock = 0; ///< Rock tonnes.
	/** @param other Ledger to compare. @return Whether both amounts match. */
	bool operator==(const SpoilLedger &other) const = default;
};

/**
 * Count the spoil wherever it is.
 * @param earth_receiver Earth receiver.
 * @param rock_receiver Rock crusher.
 * @return Tonnes aboard every vehicle, on every platform, in receiver stock and processed by the receivers.
 */
static SpoilLedger CountSpoil(Industry &earth_receiver, Industry &rock_receiver)
{
	const CargoType earth = GetCargoTypeByLabel(CargoLabel{"ERTH"});
	const CargoType rock = GetCargoTypeByLabel(CargoLabel{"ROCK"});
	SpoilLedger ledger;
	// A vehicle carries one cargo; its whole list counts, whatever its loading state.
	for (const Vehicle *v : Vehicle::Iterate()) {
		if (v->cargo_type == earth) ledger.earth += v->cargo.TotalCount();
		if (v->cargo_type == rock) ledger.rock += v->cargo.TotalCount();
	}
	// Spoil refused by the receivers may be transferred onto a platform; it still exists there.
	for (const Station *st : Station::Iterate()) {
		if (st->goods[earth].HasData()) ledger.earth += st->goods[earth].GetData().cargo.TotalCount();
		if (st->goods[rock].HasData()) ledger.rock += st->goods[rock].GetData().cargo.TotalCount();
	}
	const SpoilReceiver *earth_state = GetSpoilReceiver(earth_receiver.index);
	const SpoilReceiver *rock_state = GetSpoilReceiver(rock_receiver.index);
	ledger.earth += SpoilReceiverRig::Stock(earth_receiver) + earth_state->disposed_t;
	ledger.rock += SpoilReceiverRig::Stock(rock_receiver) + rock_state->crushed_t + rock_state->disposed_t;
	return ledger;
}

/**
 * @param job Job of the project.
 * @return The tonnes the job has committed.
 */
static SpoilLedger Committed(const ExcavationJob &job)
{
	return {job.completed_earth_t, job.completed_rock_t};
}

/** Each tonne a cut commits exists exactly once, from the face to the receivers. */
TEST_CASE("Every committed tonne stays in one place from the face to the receivers", "[tbm-conservation]")
{
	AutoRestoreBackup<uint8_t> indicators{_settings_client.gui.loading_indicators, uint8_t{0}}; // No filling text effect without a GUI.
	SpoilReceiverRig rig(2);
	Industry &earth = *rig.BuildReceiver(SpoilReceiverKind::Earth, 15, 22);
	Industry &rock = *rig.BuildReceiver(SpoilReceiverKind::Rock, 14, 16);
	rig.RefreshOrigin();
	/** @return Whether the counted spoil equals what the job committed. */
	const auto conserved = [&]() { return CountSpoil(earth, rock) == Committed(rig.Job()); };
	REQUIRE(CountSpoil(earth, rock) == SpoilLedger{});
	REQUIRE(conserved());

	// A partial cut is work, not material: nothing is committed and nothing appears aboard.
	const auto estimate = EstimateExcavationCut(rig.project.job);
	REQUIRE(estimate.error == ExcavationError::None);
	REQUIRE(estimate.work_ticks >= 2);
	for (uint64_t tick = 0; tick < estimate.work_ticks / 2; ++tick) REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	CHECK(rig.Job().partial_work_ticks != 0);
	CHECK(CountSpoil(earth, rock) == SpoilLedger{});
	CHECK(conserved());

	// The commit moves the whole cut aboard at once, Earth and Rock apart.
	rig.BoreOneCut(); // The shared fixture uses the actual model rate and retained partial work.
	REQUIRE(rig.Job().cursor == 1);
	CHECK(CountSpoil(earth, rock) == SpoilLedger{estimate.earth_t, estimate.rock_t});
	CHECK(estimate.earth_t != 0);
	CHECK(estimate.rock_t != 0);
	CHECK(conserved());

	// Both cuts fit the wagons, so the first is no exchange breakpoint and the cutter goes on to its next support.
	REQUIRE(rig.Project().phase == TBMPhase::AdvanceSupport);

	// It cannot drive there yet (master Group 7B); a save and reload into Bore stands in for the drive. The reload and
	// half of the next cut keep both the committed totals and the spoil aboard.
	rig.Reload(TBMPhase::Bore);
	CHECK(conserved());
	// Reversing the working train changes train state only: no tonne moves, appears or goes.
	REQUIRE(Command<Commands::ReverseTrainDirection>::Do(DoCommandFlag::Execute, rig.front->index, false).Succeeded());
	CHECK(conserved());
	const auto second = EstimateExcavationCut(rig.project.job);
	REQUIRE(second.error == ExcavationError::None);
	for (uint64_t tick = 0; tick < second.work_ticks / 2; ++tick) REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	CHECK(rig.Job().partial_work_ticks != 0);
	CHECK(conserved());

	// Cancellation retains unfinished work until physical withdrawal and
	// delivery, without making or losing material while the spoil travels back.
	const SpoilLedger before_cancel = CountSpoil(earth, rock);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, rig.project.id, TBMProjectAction::Cancel).Succeeded());
	CHECK(rig.Job().partial_work_ticks != 0);
	CHECK(CountSpoil(earth, rock) == before_cancel);
	CHECK(conserved());
	REQUIRE(rig.Project().phase == TBMPhase::ReturnToRelease);

	// The exchange moves the wagons with their packets; no tonne is copied or lost.
	REQUIRE(rig.Tick().phase == TBMPhase::WaitToRelease);
	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded);
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == estimate.earth_t + estimate.rock_t);
	CHECK(conserved());

	// The hauler delivers everything; the receivers then hold what the face produced.
	rig.PlaceOnTrack(rig.hauler, TileXYL(25, 20, MapLayer::Surface));
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin);
	rig.PlaceOnTrack(rig.hauler, rig.origin_tile);
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::DisposeOrTransfer);
	// Each visit delivers one interval of intake per receiver; the receivers process it before the next visit.
	for (int visit = 0; visit < 24 && TBMExchangeRig::Aboard(rig.hauler) != 0; ++visit) {
		RunStationStop(rig.hauler, rig.fixture.origin, OrderUnloadType::UnloadIfPossible, OrderLoadType::NoLoad);
		SpoilReceiverRig::RunIntervals(earth, 1);
		SpoilReceiverRig::RunIntervals(rock, 1);
		CHECK(conserved());
	}
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == 0);
	CHECK(conserved());
}
