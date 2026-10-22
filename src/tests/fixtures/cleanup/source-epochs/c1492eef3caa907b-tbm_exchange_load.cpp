/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange_load.cpp Staged loading of hauler assignments and wagon sets (Plan 5 Group 10): a loaded store
 * stays unpublished until every set names live wagon units of its project's company, each in one set only, and every
 * assignment names one live non-cutter hauler and at most one carried set. Any refusal publishes nothing. Wagon packets
 * never pass through the store, and a save before or after the exchange step resumes the same cycle.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 10
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-h-exchange-design.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_exchange_fixture.h"
#include "../multilayer_state.h"

#include "../safeguards.h"

/** An intact store publishes unchanged, keeps the wagons' own packets and resumes the next step of the same cycle. */
TEST_CASE("A loaded exchange store publishes unchanged and resumes its cycle", "[tbm-exchange-load]")
{
	TBMExchangeLoadRig rig;
	const TBMExchangeSave saved;
	REQUIRE(saved.assignments.size() == 1);
	REQUIRE(saved.sets.size() == 2);
	const uint64_t digest = ComputeTBMExchangeDigest();
	const auto packets = GetTBMPacketOwners(rig.hauler);
	const auto cutter_chain = GetTBMChainIDs(rig.front);
	const auto schema = CaptureMultiLayerStateSchema();
	CHECK((schema.feature_flags & MULTILAYER_FEATURE_TBM_EXCHANGE) != 0);
	CHECK((schema.feature_flags & MULTILAYER_FEATURE_TBM_PROJECTS) != 0);
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::None);

	REQUIRE(saved.Load());
	CHECK(saved.IsLive());
	CHECK(ComputeTBMExchangeDigest() == digest);
	CHECK(GetTBMPacketOwners(rig.hauler) == packets); // Packets stay with their own wagons; the store names wagons only.
	CHECK(GetTBMChainIDs(rig.front) == cutter_chain);
	CHECK(AcceptLoadedTBMExchange()); // Nothing staged: the store stays as it is.
	CHECK(saved.IsLive());

	// Lookups are searches over the published store, so they answer for the actual consists at once.
	const TBMWagonSetID loaded = saved.assignments[0].set;
	CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->id == saved.assignments[0].id);
	CHECK(GetTBMWagonSetForWagon(rig.hauler->Next()->index)->id == loaded);
	CHECK(GetTBMWagonSet(loaded)->state == TBMWagonSetState::Loaded);
	rig.PlaceOnTrack(rig.hauler, rig.origin);
	const auto visited = rig.StepHauler();
	CHECK(visited.phase == TBMHaulerPhase::DisposeOrTransfer);
	CHECK(GetTBMWagonSet(loaded)->visited_origin);
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == rig.spoil);
}

/** Every phase and every hold survives the store unchanged; none is folded into another on load. */
TEST_CASE("Every hauler phase and wait round-trips", "[tbm-exchange-load]")
{
	TBMExchangeLoadRig rig;
	TBMExchangeSave saved;
	// The validator never repairs a phase/wait pair: each pair loads exactly as saved.
	for (uint8_t phase = 0; phase < to_underlying(TBMHaulerPhase::End); ++phase) {
		for (uint8_t wait = 0; wait < to_underlying(TBMHaulerWait::End); ++wait) {
			saved.assignments[0].phase = static_cast<TBMHaulerPhase>(phase);
			saved.assignments[0].wait = static_cast<TBMHaulerWait>(wait);
			saved.assignments[0].phase_ticks = 100 + phase * 16 + wait;
			INFO("phase " << int{phase} << " wait " << int{wait});
			REQUIRE(saved.Load());
			CHECK(saved.IsLive());
		}
	}
}

/** Each kind of mismatch between the store and the rebuilt vehicles or projects refuses and publishes nothing. */
TEST_CASE("Loaded exchange stores refuse missing, reused or shared references", "[tbm-exchange-load]")
{
	TBMExchangeLoadRig rig;
	TBMExchangeSave saved;
	const VehicleID missing{50000};

	SECTION("a missing hauler") {
		saved.assignments[0].hauler = missing;
		saved.Refuse();
	}
	SECTION("a hauler ID reused by the cutter") {
		saved.assignments[0].hauler = rig.front->index;
		saved.Refuse();
	}
	SECTION("a hauler ID reused by a wagon") {
		saved.assignments[0].hauler = rig.hauler->Next()->index;
		saved.Refuse();
	}
	SECTION("two assignments of one hauler") {
		TBMExchangeAssignment twin = saved.assignments[0];
		twin.id = TBMExchangeAssignmentID{saved.next_assignment};
		twin.set = TBMWagonSetID{UINT32_MAX};
		saved.assignments.push_back(twin);
		++saved.next_assignment;
		saved.Refuse();
	}
	SECTION("one set carried by two haulers") {
		// Two heads may never claim one set's wagons (Plan 14 10.5), even when both are real haulers of the project.
		TBMExchangeAssignment twin = saved.assignments[0];
		twin.id = TBMExchangeAssignmentID{saved.next_assignment};
		twin.hauler = rig.BuildHauler(rig.depot, 1, 0)->index;
		saved.assignments.push_back(twin);
		++saved.next_assignment;
		saved.Refuse();
	}
	SECTION("a missing project") {
		saved.assignments[0].project = TBMProjectID{999};
		saved.Refuse();
	}
	SECTION("a missing wagon") {
		saved.Set(TBMWagonSetState::Loaded).wagons[0] = missing;
		saved.Refuse();
	}
	SECTION("a wagon ID reused by the hauler's engine") {
		saved.Set(TBMWagonSetState::Loaded).wagons[0] = rig.hauler->index;
		saved.Refuse();
	}
	SECTION("a wagon in two sets") {
		TBMWagonSet copy = saved.Set(TBMWagonSetState::Loaded);
		copy.id = TBMWagonSetID{saved.next_set};
		copy.state = TBMWagonSetState::Available;
		copy.wagons.resize(1);
		saved.sets.push_back(copy);
		++saved.next_set;
		saved.Refuse();
	}
	SECTION("a wagon of another company") {
		Train *unit = Train::Get(saved.Set(TBMWagonSetState::Loaded).wagons[0]);
		const Owner was = unit->owner;
		unit->owner = OWNER_NONE;
		saved.Refuse();
		unit->owner = was;
	}
	SECTION("the cutter's set off the cutter") {
		// Swapping the wagon lists puts the Assigned set on the hauler; the carried set's location is its hold's business.
		std::swap(saved.Set(TBMWagonSetState::Assigned).wagons, saved.Set(TBMWagonSetState::Loaded).wagons);
		saved.Refuse();
	}
	SECTION("a carried Assigned set") {
		saved.assignments[0].set = saved.Set(TBMWagonSetState::Assigned).id;
		saved.Refuse();
	}
	SECTION("a carried set that does not exist") {
		saved.assignments[0].set = TBMWagonSetID{saved.next_set + 7};
		saved.Refuse();
	}
	SECTION("identities at or beyond their cursor") {
		saved.next_set = saved.sets.back().id.base();
		saved.Refuse();
	}
	SECTION("sets out of identity order") {
		std::swap(saved.sets[0], saved.sets[1]);
		saved.Refuse();
	}
	SECTION("an empty set") {
		saved.Set(TBMWagonSetState::Assigned).wagons.clear();
		saved.Refuse();
	}
}

/** A save on either side of the exchange step reloads consistently: the swap is one action, never half done. */
TEST_CASE("Saves before and after the exchange step both resume", "[tbm-exchange-load]")
{
	TBMExchangeRig rig;
	rig.MakeOriginStationTile();
	const auto estimate = rig.BoreOneCut();
	REQUIRE(rig.Tick().phase == TBMPhase::WaitForEmpties);
	rig.Meet();
	REQUIRE(rig.StepHauler().phase == TBMHaulerPhase::AwaitExchange);

	const TBMExchangeSave before;
	REQUIRE(before.Load());
	CHECK(before.IsLive());
	const auto collected = rig.StepHauler(); // The whole exchange happens inside this one step.
	CHECK(collected.phase == TBMHaulerPhase::CollectLoaded);
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == estimate.earth_t + estimate.rock_t);

	const TBMExchangeSave after;
	REQUIRE(after.sets.size() == 2);
	REQUIRE(after.Load());
	CHECK(after.IsLive());
	CHECK(rig.StepHauler().phase == TBMHaulerPhase::CollectLoaded); // Still standing in the bay.
}

/** The metadata flag follows the identity cursors: a released hauler keeps it, an empty history clears it. */
TEST_CASE("The exchange feature flag follows identity history", "[tbm-exchange-load]")
{
	TBMExchangeRig rig;
	rig.MakeOriginStationTile();
	CHECK((CaptureMultiLayerStateSchema().feature_flags & MULTILAYER_FEATURE_TBM_EXCHANGE) == 0);
	rig.Assign();
	CHECK((CaptureMultiLayerStateSchema().feature_flags & MULTILAYER_FEATURE_TBM_EXCHANGE) != 0);
	REQUIRE(ReleaseTBMHauler(rig.hauler->index, true) == TBMExchangeError::None);
	CHECK(CaptureTBMExchangeAssignments().empty());
	CHECK((CaptureMultiLayerStateSchema().feature_flags & MULTILAYER_FEATURE_TBM_EXCHANGE) != 0);
	const TBMExchangeSave released;
	REQUIRE(released.Load()); // An unowned Available set and the cursors load without any assignment.
	CHECK(released.IsLive());
	ResetTBMExchangeState();
	CHECK((CaptureMultiLayerStateSchema().feature_flags & MULTILAYER_FEATURE_TBM_EXCHANGE) == 0);
}
