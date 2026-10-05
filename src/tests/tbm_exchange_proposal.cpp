/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange_proposal.cpp Both resulting consists described before any change (Plan 5 Group 2): real unit
 * boundaries, ordered ID lists, kept cargo, empty capacity against the frozen pass terms, the hauler rule, and an
 * estimate that tries the whole rearrangement and restores every chain, cache, group and random seed.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 2
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_exchange_fixture.h"

#include "../safeguards.h"

/** Everything an estimate must leave exactly as it found it. */
struct TBMExchangeSnapshot {
	AcquisitionDigest digest; ///< Pools, money, random seeds, geology, portal work and counted trains/engines.
	TBMChainIDs cutter; ///< Cutter consist parts in order.
	TBMChainIDs hauler; ///< Hauler parts in order.
	std::map<VehicleID, TBMPacketList> packets; ///< Packets on each part of both trains.
	std::vector<std::tuple<VehicleID, VehicleID, GroupID, uint16_t, uint16_t>> parts; ///< Per part: head, group, capacity, cached length.
	std::vector<TBMExchangeAssignment> assignments; ///< Assignment store.
	std::vector<TBMWagonSet> sets; ///< Wagon set store.

	/**
	 * @param rig Exchange rig.
	 * @return The current snapshot.
	 */
	static TBMExchangeSnapshot Capture(const TBMExchangeRig &rig)
	{
		TBMExchangeSnapshot snapshot{rig.Digest(), GetTBMChainIDs(rig.front), GetTBMChainIDs(rig.hauler), GetTBMPacketOwners(rig.front), {}, CaptureTBMExchangeAssignments(), CaptureTBMWagonSets()};
		snapshot.packets.merge(GetTBMPacketOwners(rig.hauler));
		for (const Train *head : {rig.front, rig.hauler}) {
			for (const Train *part = head; part != nullptr; part = part->Next()) snapshot.parts.emplace_back(part->index, part->First()->index, part->group_id, part->cargo_cap, part->gcache.cached_veh_length);
		}
		return snapshot;
	}

	/** @param other Snapshot to compare. @return Whether everything matches. */
	bool operator==(const TBMExchangeSnapshot &other) const = default;
};

/** The proposal names every moved unit and both complete chains, before and after, from the actual trains. */
TEST_CASE("The proposal lists both chains before and after with real unit boundaries", "[tbm-exchange-proposal]")
{
	TBMExchangeRig rig;
	const auto estimate = rig.BoreAndMeet();
	const auto cutter = GetTBMChainIDs(rig.front);
	const auto hauler = GetTBMChainIDs(rig.hauler);
	const auto proposal = DescribeTBMWagonExchange(rig.Request());
	REQUIRE(proposal.error == TBMExchangeError::None);
	CHECK(proposal.bay == rig.fixture.request.portal);
	CHECK(proposal.cutter == rig.front->index);
	CHECK(proposal.hauler == rig.hauler->index);
	CHECK(proposal.original_cutter == cutter);
	CHECK(proposal.original_hauler == hauler);
	TBMChainIDs loaded;
	for (const auto &unit : InspectTBMConsist(*rig.front, rig.Project()).exchangeable_units) loaded.push_back(unit.vehicle);
	CHECK(proposal.loaded_units == loaded);
	CHECK(proposal.earth_t == estimate.earth_t);
	CHECK(proposal.rock_t == estimate.rock_t);
	const TBMChainIDs empties(hauler.begin() + 1, hauler.end()); // Single-part hoppers, Earth then Rock, behind the locomotive.
	CHECK(proposal.empty_units == empties);
	CHECK(proposal.empty_earth_capacity_t == 128);
	CHECK(proposal.empty_rock_capacity_t == 256);
	// After: the fixed machinery and the empties; the locomotive and the loaded units.
	TBMChainIDs proposed_cutter = rig.Project().permanent_parts;
	proposed_cutter.insert(proposed_cutter.end(), empties.begin(), empties.end());
	CHECK(proposal.proposed_cutter == proposed_cutter);
	TBMChainIDs proposed_hauler{rig.hauler->index};
	proposed_hauler.insert(proposed_hauler.end(), loaded.begin(), loaded.end());
	CHECK(proposal.proposed_hauler == proposed_hauler);
	CHECK(GetTBMChainIDs(rig.front) == cutter); // Describing changes nothing.
	CHECK(GetTBMChainIDs(rig.hauler) == hauler);
}

/** Each preparation rule refuses by name, before the trial, with nothing changed. */
TEST_CASE("Each preparation rule refuses with nothing changed", "[tbm-exchange-proposal]")
{
	SECTION("no hauler is assigned") {
		TBMExchangeRig rig;
		rig.BoreOneCut();
		rig.HaulerToSourceBay();
		const auto before = TBMExchangeSnapshot::Capture(rig);
		CHECK(DescribeTBMWagonExchange(rig.Request()).error == TBMExchangeError::NotAssigned);
		CHECK(TBMExchangeSnapshot::Capture(rig) == before);
	}
	SECTION("the hauler is still away from the bay") {
		TBMExchangeRig rig;
		rig.BoreOneCut();
		rig.Assign();
		const auto before = TBMExchangeSnapshot::Capture(rig);
		CHECK(DescribeTBMWagonExchange(rig.Request()).error == TBMExchangeError::NotAtBay);
		CHECK(TBMExchangeSnapshot::Capture(rig) == before);
	}
	SECTION("the cutter carries no spoil yet") {
		TBMExchangeRig rig;
		rig.HaulerToSourceBay();
		rig.Assign();
		CHECK(DescribeTBMWagonExchange(rig.Request()).error == TBMExchangeError::NoSpoil);
	}
	SECTION("loaded cargo reserved for loading cannot change trains") {
		TBMExchangeRig rig;
		rig.BoreAndMeet();
		REQUIRE(CargoPacket::CanAllocateItem());
		// A packet of another source cannot merge into the kept spoil, so it stays a separate loading reservation.
		CargoPacket *packet = CargoPacket::Create(StationID::Invalid(), 1, Source{Source::Invalid, SourceType::Town});
		rig.Wagons(TBMConsistRole::RockWagon).back()->cargo.Append(packet, VehicleCargoList::MoveToAction::Load);
		const auto before = TBMExchangeSnapshot::Capture(rig);
		CHECK(DescribeTBMWagonExchange(rig.Request()).error == TBMExchangeError::UnstableCargo);
		CHECK(TBMExchangeSnapshot::Capture(rig) == before);
	}
	SECTION("the cutter cannot be its own hauler") {
		TBMExchangeRig rig;
		rig.BoreOneCut();
		CHECK(AssignTBMHauler(rig.project.id, rig.front->index, true) == TBMExchangeError::InvalidHauler);
		CHECK(DescribeTBMWagonExchange({rig.project.id, rig.front->index}).error == TBMExchangeError::NotAssigned);
	}
}

/** Before the last cut the empties must cover the frozen pass terms of each spoil type separately. */
TEST_CASE("Empty Earth and Rock capacity is checked separately against the frozen terms", "[tbm-exchange-proposal]")
{
	SECTION("one Earth hopper is not enough Earth space") {
		TBMExchangeRig rig(2, 1, 2);
		rig.BoreOneCut(); // The first of two cuts: more cutting follows this exchange.
		rig.Meet();
		const auto proposal = DescribeTBMWagonExchange(rig.Request());
		CHECK(proposal.error == TBMExchangeError::InsufficientEmpties);
		CHECK(proposal.loaded_units.empty()); // A refusal never looks like a plan.
	}
	SECTION("one Rock hopper is not enough Rock space") {
		TBMExchangeRig rig(2, 2, 1);
		rig.BoreOneCut(); // The first of two cuts: more cutting follows this exchange.
		rig.Meet();
		CHECK(DescribeTBMWagonExchange(rig.Request()).error == TBMExchangeError::InsufficientEmpties);
	}
	SECTION("after the final cut the cutter only gives its spoil away") {
		TBMExchangeRig rig(1, 1, 1);
		rig.BoreAndMeet();
		REQUIRE(rig.Job().cursor == rig.Job().route.size());
		const auto proposal = DescribeTBMWagonExchange(rig.Request());
		CHECK(proposal.error == TBMExchangeError::None);
		CHECK(proposal.empty_units.size() == 2);
	}
}

/** The estimate runs the real rearrangement rules on the final chains and leaves every authoritative field unchanged. */
TEST_CASE("The estimate tries the whole rearrangement and restores everything", "[tbm-exchange-proposal]")
{
	TBMExchangeRig rig;
	rig.BoreAndMeet();
	const auto before = TBMExchangeSnapshot::Capture(rig);
	const auto described = DescribeTBMWagonExchange(rig.Request());
	const auto validated = ValidateTBMWagonExchange(rig.Request());
	REQUIRE(validated.error == TBMExchangeError::None);
	CHECK(validated == described); // A successful trial adds nothing to the description.
	CHECK(TBMExchangeSnapshot::Capture(rig) == before);
	CHECK(Command<Commands::ExchangeTBMWagons>::Do({}, rig.project.id, rig.hauler->index).Succeeded()); // The command estimate is the same trial.
	CHECK(TBMExchangeSnapshot::Capture(rig) == before);
	for (const Train *head : {rig.front, rig.hauler}) {
		for (const Train *part = head->Next(); part != nullptr; part = part->Next()) CHECK_FALSE(part->IsFreeWagon());
	}
}
