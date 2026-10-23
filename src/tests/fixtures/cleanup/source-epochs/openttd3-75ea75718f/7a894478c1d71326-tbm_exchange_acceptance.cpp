/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange_acceptance.cpp Lifecycle acceptance of the exchange (Plan 5 Group 12): releasing a hauler at every
 * hold the cycle can reach keeps every wagon, packet and set, and two complete cycles with real receivers bring the same
 * wagon units round and dispose of every tonne. Mixed Earth/Rock loads run throughout. A hauler short of empties at a
 * breakpoint before the last waits NoEmpties here; short initial empties, competing haulers and blocked lines have their
 * own cases in tbm_exchange_origin.cpp, tbm_wagon_sets.cpp and spoil_receiver_stall.cpp.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 12
 * @see docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-j-lifecycle-design.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"

#include "../safeguards.h"

/**
 * Step the rig's hauler into a hold, release it there, and check that nothing moved and the store still loads.
 * @param rig Rig whose hauler is about to hold.
 * @param wait Expected hold.
 * @return The released loaded set.
 */
static TBMWagonSetID ReleaseAt(TBMExchangeLoadRig &rig, TBMHaulerWait wait)
{
	const auto held = rig.StepHauler();
	CHECK(held.wait == wait);
	const auto chain = GetTBMChainIDs(rig.hauler);
	const auto packets = GetTBMPacketOwners(rig.hauler);
	const TBMPacketCustody custody;
	REQUIRE(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::None);
	CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index) == nullptr);
	REQUIRE(GetTBMWagonSet(held.set) != nullptr);
	CHECK(GetTBMWagonSet(held.set)->state == TBMWagonSetState::Loaded); // The spoil and its set stay where they are.
	CHECK(GetTBMChainIDs(rig.hauler) == chain);
	CHECK(GetTBMPacketOwners(rig.hauler) == packets);
	CHECK(custody.IsSole());
	CHECK(TBMExchangeSave{}.Reloads());
	return held.set;
}

/** Cancelling the assignment in any hold keeps every wagon and tonne; where the cause is gone, a new one recovers. */
TEST_CASE("Releasing a hauler in every reachable hold keeps its wagons and spoil", "[tbm-exchange-acceptance]")
{
	// The detached last Rock hopper must hold spoil: the former Soft Rock total overflows the first hopper (R2TBMF 5B.1).
	const TBMRigRock mixed_rock{GeologyHardness::Mixed};
	TBMExchangeLoadRig rig;

	/**
	 * Assign the hauler again and require it to adopt its set in the given phase.
	 * @param set Released set.
	 * @param phase Phase the adopted cycle resumes in.
	 */
	const auto recover = [&rig](TBMWagonSetID set, TBMHaulerPhase phase) {
		REQUIRE(AssignTBMHauler(rig.project.id, rig.hauler->index, true) == TBMExchangeError::None);
		CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->set == set);
		CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->phase == phase);
		CHECK(TBMExchangeRig::Aboard(rig.hauler) == rig.spoil);
	};

	SECTION("stopped on the way") {
		rig.hauler->vehstatus.Set(VehState::Stopped);
		const auto set = ReleaseAt(rig, TBMHaulerWait::Stopped);
		recover(set, TBMHaulerPhase::VisitOrigin);
	}
	SECTION("crashed") {
		rig.hauler->vehstatus.Set(VehState::Crashed);
		ReleaseAt(rig, TBMHaulerWait::Crashed); // A crashed train cannot be assigned again.
	}
	SECTION("a set wagon out of the train") {
		Train *last = rig.hauler->Last();
		last->Previous()->SetNext(nullptr);
		const auto set = ReleaseAt(rig, TBMHaulerWait::WrongWagons);
		CHECK(last->cargo.TotalCount() != 0); // The detached wagon keeps its own spoil.
		rig.hauler->Last()->SetNext(last);
		recover(set, TBMHaulerPhase::VisitOrigin);
	}
	SECTION("the origin is lost") {
		rig.fixture.origin->owner = OWNER_NONE;
		const auto set = ReleaseAt(rig, TBMHaulerWait::LostOrigin);
		rig.fixture.origin->owner = _current_company;
		recover(set, TBMHaulerPhase::VisitOrigin);
	}
	SECTION("the line is blocked") {
		rig.hauler->vehicle_flags.Set(VehicleFlag::PathfinderLost);
		const auto set = ReleaseAt(rig, TBMHaulerWait::BlockedLine);
		rig.hauler->vehicle_flags.Reset(VehicleFlag::PathfinderLost);
		recover(set, TBMHaulerPhase::VisitOrigin);
	}
	SECTION("no receiver at the origin has room") {
		rig.PlaceOnTrack(rig.hauler, rig.origin);
		REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::DisposeOrTransfer);
		const auto set = ReleaseAt(rig, TBMHaulerWait::SaturatedReceiver); // This rig has no receivers at all.
		recover(set, TBMHaulerPhase::DisposeOrTransfer); // The visit is kept; it need not be made twice.
	}
}

/** A second hauler waiting its turn at the bay can be released there with its empties untouched. */
TEST_CASE("Releasing a hauler waiting for an occupied bay keeps its empties", "[tbm-exchange-acceptance]")
{
	TBMExchangeRig rig;
	rig.MakeOriginStationTile();
	rig.BoreOneCut();
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties); // A waiting cutter makes each hauler in the bay ask for the exchange.
	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	Train *second = rig.BuildHauler(rig.depot, 2, 2);
	rig.PlaceAt(second, *rig.front);
	REQUIRE(AssignTBMHauler(rig.project.id, second->index, true) == TBMExchangeError::None);
	REQUIRE(TBMExchangeRig::StepHauler(*second).phase == TBMHaulerPhase::AwaitExchange);
	const auto waiting = TBMExchangeRig::StepHauler(*second); // Its request is refused: the first hauler has the turn.
	CHECK(waiting.wait == TBMHaulerWait::OccupiedBay);
	const auto chain = GetTBMChainIDs(second);
	REQUIRE(ReleaseTBMHauler(second->index, true) == TBMExchangeError::None);
	CHECK(GetTBMWagonSet(waiting.set)->state == TBMWagonSetState::Available);
	CHECK(GetTBMChainIDs(second) == chain);
	CHECK(TBMExchangeRig::Aboard(second) == 0);
	CHECK(GetTBMExchangeTurn(rig.Project().exchange_bay)->hauler == rig.hauler->index); // The first hauler keeps its turn.
	CHECK(TBMExchangeSave{}.Reloads());
}

/** A hauler with too few empties for the rest of the route waits NoEmpties at a mid-route exchange, and nothing moves. */
TEST_CASE("A hauler short of empties waits for them at a mid-route exchange", "[tbm-exchange-acceptance]")
{
	const TBMRigRock mixed_rock{GeologyHardness::Mixed}; // The overflow needs the former Soft Rock totals (R2TBMF 5B.1 halved yields).

	// The bend's diagonal cuts overflow one load, so its first breakpoint comes before the last cut.
	TBMExchangeRig rig({DirDiff::Same, DirDiff::Right45, DirDiff::Right45});
	rig.MakeOriginStationTile();
	REQUIRE(rig.Job().pass_terms.has_value());
	const uint16_t breakpoint = rig.Job().pass_terms->exchange_after.front();
	REQUIRE(breakpoint < rig.Job().route.size());
	// Each cut short of the breakpoint goes on to its next support; a reload into Bore stands in for that drive (master Group 7B).
	while (rig.Job().cursor < breakpoint) {
		if (rig.Project().phase == TBMPhase::AdvanceSupport) rig.Reload(TBMPhase::Bore);
		rig.BoreOneCut();
	}
	REQUIRE(rig.Project().phase == TBMPhase::ReturnForExchange);
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);

	Train *short_hauler = rig.BuildHauler(rig.depot, 1, 2); // One Earth hopper fewer than the cutter's two.
	rig.PlaceAt(short_hauler, *rig.front);
	REQUIRE(AssignTBMHauler(rig.project.id, short_hauler->index, true) == TBMExchangeError::None);
	const auto cutter = GetTBMChainIDs(rig.front);
	const auto hauler = GetTBMChainIDs(short_hauler);
	const uint64_t aboard = TBMExchangeRig::Aboard(rig.front);
	REQUIRE(TBMExchangeRig::StepHauler(*short_hauler).phase == TBMHaulerPhase::AwaitExchange);
	const auto waiting = TBMExchangeRig::StepHauler(*short_hauler); // Its request is refused: the route is not done.
	CHECK(waiting.wait == TBMHaulerWait::NoEmpties);
	CHECK(waiting.phase == TBMHaulerPhase::AwaitExchange);
	CHECK(GetTBMLogisticsWait(rig.Project()) == TBMLogisticsWait::NoEmpties);
	CHECK(rig.Project().phase == TBMPhase::WaitForEmpties);
	CHECK(GetTBMChainIDs(rig.front) == cutter);
	CHECK(GetTBMChainIDs(short_hauler) == hauler);
	CHECK(TBMExchangeRig::Aboard(rig.front) == aboard);
	CHECK(TBMExchangeSave{}.Reloads()); // The wait reloads like every other.

	// Releasing it in that hold keeps its empties, as in every other hold.
	REQUIRE(ReleaseTBMHauler(short_hauler->index, true) == TBMExchangeError::None);
	CHECK(GetTBMWagonSet(waiting.set)->state == TBMWagonSetState::Available);
	CHECK(GetTBMChainIDs(short_hauler) == hauler);
	CHECK(TBMExchangeSave{}.Reloads());
}

/**
 * @param chain Part IDs.
 * @return The same IDs in increasing order, to compare which units a train has regardless of their order.
 */
static std::vector<VehicleID> Sorted(std::vector<VehicleID> chain)
{
	std::ranges::sort(chain);
	return chain;
}

/** Two complete cycles move only the same wagon units between the trains, and the receivers account for every tonne. */
TEST_CASE("Two full cycles return the same wagons and dispose of every tonne", "[tbm-exchange-acceptance]")
{
	const TBMRigRock mixed_rock{GeologyHardness::Mixed}; // The overflow needs the former Soft Rock totals (R2TBMF 5B.1 halved yields).

	AutoRestoreBackup<uint8_t> indicators{_settings_client.gui.loading_indicators, uint8_t{0}}; // No filling text effect without a GUI.
	// A straight cut after the pocket opens little new ground; the bend's diagonal cuts overflow one load of the cutter's wagons.
	SpoilReceiverRig rig({DirDiff::Same, DirDiff::Right45, DirDiff::Right45});
	Industry *earth = rig.BuildReceiver(SpoilReceiverKind::Earth, 15, 22);
	Industry *rock = rig.BuildReceiver(SpoilReceiverKind::Rock, 14, 16);
	rig.RefreshOrigin();
	REQUIRE(rig.Job().pass_terms.has_value());
	const std::vector<uint16_t> breakpoints = rig.Job().pass_terms->exchange_after;
	REQUIRE(breakpoints.size() >= 2); // At least one exchange is due before the final one.
	const auto cutter_units = Sorted(GetTBMChainIDs(rig.front));
	const auto hauler_units = Sorted(GetTBMChainIDs(rig.hauler));
	const size_t vehicles = Vehicle::GetNumItems();
	const TBMPacketCustody custody; // Each tonne in one list until a receiver takes it (Plan 15 10.3).
	uint64_t earth_cut = 0;
	uint64_t rock_cut = 0;

	/**
	 * Stand in for the drive onto the next support, which needs master Group 7B: reload the project into Bore, as the
	 * other multi-cut rigs do, carrying the exchange store exactly as a save and reload would carry it.
	 */
	const auto reenter_bore = [&]() {
		const TBMExchangeSave keep;
		rig.Reload(TBMPhase::Bore);
		REQUIRE(keep.Load());
		CHECK(custody.IsSole());
	};
	/**
	 * Bore up to the next frozen exchange breakpoint, wait for empties and let the hauler waiting in the bay take the load.
	 * @param breakpoint Cursor after which the exchange is due.
	 * @param exchanges Exchanges the hauler has made after this one.
	 */
	const auto cut_and_exchange = [&](uint16_t breakpoint, uint32_t exchanges) {
		uint64_t load = 0;
		// A cut short of the breakpoint keeps its spoil aboard, and the cutter goes on to its next support.
		while (rig.Job().cursor < breakpoint) {
			if (rig.Project().phase == TBMPhase::AdvanceSupport) reenter_bore();
			const auto cut = rig.BoreOneCut();
			earth_cut += cut.earth_t;
			rock_cut += cut.rock_t;
			load += cut.earth_t + cut.rock_t;
		}
		REQUIRE(rig.Project().phase == TBMPhase::ReturnForExchange);
		REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
		const auto collected = rig.StepHauler();
		REQUIRE(collected.phase == TBMHaulerPhase::CollectLoaded);
		CHECK(collected.exchanges == exchanges);
		CHECK(TBMExchangeRig::Aboard(rig.hauler) == load);
		CHECK(custody.IsSole());
	};
	/** Take the load to the origin, let the receivers take all of it, and bring the empties back to the bay. */
	const auto haul = [&]() {
		rig.PlaceOnTrack(rig.hauler, TileXYL(25, 20, MapLayer::Surface));
		REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::VisitOrigin);
		rig.PlaceOnTrack(rig.hauler, rig.origin_tile);
		REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::DisposeOrTransfer);
		// Each visit delivers one interval of intake per receiver; the receivers process it before the next visit.
		for (int visit = 0; visit < 24 && TBMExchangeRig::Aboard(rig.hauler) != 0; ++visit) {
			RunStationStop(rig.hauler, rig.fixture.origin, OrderUnloadType::UnloadIfPossible, OrderLoadType::NoLoad);
			SpoilReceiverRig::RunIntervals(*earth, 1);
			SpoilReceiverRig::RunIntervals(*rock, 1);
		}
		REQUIRE(TBMExchangeRig::Aboard(rig.hauler) == 0);
		CHECK(custody.IsSole()); // Delivered packets are gone, not left behind in some list.
		REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::ReturnEmpties);
		rig.HaulerToSourceBay();
		REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	};

	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);
	cut_and_exchange(breakpoints[0], 1);
	std::vector<TBMWagonSetID> sets;
	for (const auto &set : CaptureTBMWagonSets()) sets.push_back(set.id);
	REQUIRE(sets.size() == 2);
	CHECK(Sorted(GetTBMChainIDs(rig.front)) != cutter_units); // The trains now carry each other's wagons.
	haul();

	reenter_bore(); // Nor can the cutter yet drive from the bay back to its face.
	cut_and_exchange(breakpoints[1], 2);
	haul();

	// The same units went round twice: each train has its own wagons back, and no vehicle or set was made.
	CHECK(Sorted(GetTBMChainIDs(rig.front)) == cutter_units);
	CHECK(Sorted(GetTBMChainIDs(rig.hauler)) == hauler_units);
	CHECK(Vehicle::GetNumItems() == vehicles);
	std::vector<TBMWagonSetID> after;
	for (const auto &set : CaptureTBMWagonSets()) after.push_back(set.id);
	CHECK(after == sets);
	// Every tonne cut is with the receivers, processed or still in stock; none is left aboard either train.
	CHECK(GetSpoilReceiver(earth->index)->disposed_t + SpoilReceiverRig::Stock(*earth) == earth_cut);
	CHECK(GetSpoilReceiver(rock->index)->crushed_t + SpoilReceiverRig::Stock(*rock) == rock_cut);
	CHECK(TBMExchangeRig::Aboard(rig.front) == 0);
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == 0);
}
