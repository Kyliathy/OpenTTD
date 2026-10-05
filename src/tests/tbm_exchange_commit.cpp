/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exchange_commit.cpp One wagon exchange committed with complete recovery (Plan 5 Group 3): the same wagons
 * keep their own packets, both heads keep orders, groups and unit numbers, a refusal after the first leg restores
 * everything, and repeated mixed exchanges leave each wagon with exactly one head.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 3
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_exchange_fixture.h"
#include "../order_cmd.h"
#include "../core/backup_type.hpp"

#include "../table/strings.h"

#include "../safeguards.h"

/**
 * @param rig Exchange rig.
 * @return Packets of every part of both trains, keyed by part.
 */
static std::map<VehicleID, TBMPacketList> GetBothPacketOwners(const TBMExchangeRig &rig)
{
	auto owners = GetTBMPacketOwners(rig.front);
	owners.merge(GetTBMPacketOwners(rig.hauler));
	return owners;
}

/** Everything a refused or estimated exchange must leave exactly as it found it, including head-only state. */
struct TBMExchangeLedger {
	AcquisitionDigest digest; ///< Pools, money, random seeds, geology, portal work and counted trains/engines.
	TBMChainIDs cutter; ///< Cutter consist parts.
	TBMChainIDs hauler; ///< Hauler parts.
	std::map<VehicleID, TBMPacketList> packets; ///< Packets per part.
	std::vector<std::tuple<VehicleID, CargoType, uint8_t, uint16_t, GroupID>> refits; ///< Per part: cargo, subtype, refit capacity, group.
	std::array<uint16_t, 2> orders{}; ///< Order counts of both heads.
	std::array<UnitID, 2> units{}; ///< Unit numbers of both heads.
	std::vector<TBMExchangeAssignment> assignments; ///< Assignment store.
	std::vector<TBMWagonSet> sets; ///< Wagon set store.

	/**
	 * @param rig Exchange rig.
	 * @return The current ledger.
	 */
	static TBMExchangeLedger Capture(const TBMExchangeRig &rig)
	{
		TBMExchangeLedger ledger{rig.Digest(), GetTBMChainIDs(rig.front), GetTBMChainIDs(rig.hauler), GetBothPacketOwners(rig)};
		for (const Train *head : {rig.front, rig.hauler}) {
			for (const Train *part = head; part != nullptr; part = part->Next()) ledger.refits.emplace_back(part->index, part->cargo_type, part->cargo_subtype, part->refit_cap, part->group_id);
		}
		ledger.orders = {rig.front->GetNumOrders(), rig.hauler->GetNumOrders()};
		ledger.units = {rig.front->unitnumber, rig.hauler->unitnumber};
		ledger.assignments = CaptureTBMExchangeAssignments();
		ledger.sets = CaptureTBMWagonSets();
		return ledger;
	}

	/** @param other Ledger to compare. @return Whether everything matches. */
	bool operator==(const TBMExchangeLedger &other) const = default;
};

/** Give the hauler an ordinary travel order to the project's origin. */
static void AddHaulerOrder(const TBMExchangeRig &rig)
{
	Order order;
	order.MakeGoToStation(rig.fixture.origin->index);
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, rig.hauler->index, 0, order).Succeeded());
}

/** The committed exchange moves real wagons with their own packets and changes nothing else. */
TEST_CASE("One exchange moves real wagons with their own packets and nothing else", "[tbm-exchange-commit]")
{
	TBMExchangeRig rig;
	const auto estimate = rig.BoreAndMeet();
	AddHaulerOrder(rig);
	const auto packets = GetBothPacketOwners(rig);
	const auto packet_items = CargoPacket::GetNumItems();
	const auto before = rig.Digest();
	const UnitID cutter_unit = rig.front->unitnumber;
	const UnitID hauler_unit = rig.hauler->unitnumber;

	const auto proposal = ExecuteTBMWagonExchange(rig.Request());
	REQUIRE(proposal.error == TBMExchangeError::None);
	CHECK(GetTBMChainIDs(rig.front) == proposal.proposed_cutter);
	CHECK(GetTBMChainIDs(rig.hauler) == proposal.proposed_hauler);
	// Each packet is still aboard the same wagon; no packet was created, copied or deleted.
	CHECK(GetBothPacketOwners(rig) == packets);
	CHECK(CargoPacket::GetNumItems() == packet_items);
	CHECK(TBMExchangeRig::Aboard(rig.hauler) == estimate.earth_t + estimate.rock_t);
	CHECK(TBMExchangeRig::Aboard(rig.front) == 0);
	// Heads stay heads with their own orders, unit numbers and groups; every other part follows its head.
	CHECK(rig.front->IsFrontEngine());
	CHECK(rig.hauler->IsFrontEngine());
	CHECK(rig.front->unitnumber == cutter_unit);
	CHECK(rig.hauler->unitnumber == hauler_unit);
	CHECK(rig.hauler->GetNumOrders() == 1);
	for (const Train *head : {rig.front, rig.hauler}) {
		for (const Train *part = head; part != nullptr; part = part->Next()) {
			CHECK(part->First() == head);
			CHECK(part->group_id == head->group_id);
			if (part != head) CHECK_FALSE(part->IsFreeWagon());
		}
	}
	// The cutter consist keeps its fixed machinery and roles, now with empty Earth units before empty Rock units.
	const auto layout = InspectTBMConsist(*rig.front, rig.Project());
	REQUIRE(layout.error == TBMBlockedReason::None);
	REQUIRE(layout.exchangeable_units.size() == 4);
	CHECK(layout.exchangeable_units[0].role == TBMConsistRole::EarthWagon);
	CHECK(layout.exchangeable_units[3].role == TBMConsistRole::RockWagon);
	CHECK(rig.front->gcache.cached_total_length == 48);
	CHECK_FALSE(IsTBMExchangeDue(rig.Project()));
	// Cash, geology, portal work, random seeds and the counted fleet are unchanged; only the assignment and sets record it.
	CHECK(rig.Digest() == before);
	const auto *assignment = GetTBMExchangeAssignmentForHauler(rig.hauler->index);
	REQUIRE(assignment != nullptr);
	CHECK(assignment->phase == TBMHaulerPhase::CollectLoaded);
	CHECK(assignment->exchanges == 1);
	CHECK(assignment->revision == 2);
	const auto *loaded = GetTBMWagonSet(assignment->set);
	REQUIRE(loaded != nullptr);
	CHECK(loaded->wagons == proposal.loaded_units);
	CHECK(loaded->state == TBMWagonSetState::Loaded);
	CHECK_FALSE(loaded->visited_origin);
	const auto *working = GetTBMWagonSetForWagon(proposal.empty_units.front());
	REQUIRE(working != nullptr);
	CHECK(working->wagons == proposal.empty_units);
	CHECK(working->state == TBMWagonSetState::Assigned); // The hauler's empties now serve the cutter.
}

/** An estimate, and a refusal raised after the cutter's leg passed, leave every authoritative field unchanged. */
TEST_CASE("An estimate and a refusal after the first leg change nothing", "[tbm-exchange-commit]")
{
	// One Earth and one Rock hopper: after the final cut the cutter needs no space, but the hauler grows by two units.
	TBMExchangeRig rig(1, 1, 1);
	rig.BoreAndMeet();
	AddHaulerOrder(rig);
	const auto before = TBMExchangeLedger::Capture(rig);

	SECTION("estimate") {
		const auto proposal = ValidateTBMWagonExchange(rig.Request());
		CHECK(proposal.error == TBMExchangeError::None);
		CHECK(TBMExchangeLedger::Capture(rig) == before);
	}
	SECTION("the hauler's leg is too long after the cutter's leg passed") {
		// Two tiles hold the cutter's new 32 units exactly; the hauler's new 40 units do not.
		AutoRestoreBackup shorter(_settings_game.vehicle.max_train_length, uint8_t{2});
		const auto proposal = ExecuteTBMWagonExchange(rig.Request());
		CHECK(proposal.error == TBMExchangeError::Coupling);
		CHECK(proposal.message == STR_ERROR_TRAIN_TOO_LONG);
		CHECK(TBMExchangeLedger::Capture(rig) == before);
		CHECK(Command<Commands::ExchangeTBMWagons>::Do(DoCommandFlag::Execute, rig.project.id, rig.hauler->index).GetErrorMessage() == STR_ERROR_TRAIN_TOO_LONG);
		CHECK(TBMExchangeLedger::Capture(rig) == before);
	}
	// Either way the chains are whole: every part's head is its original one.
	for (VehicleID id : before.cutter) CHECK(Train::Get(id)->First() == rig.front);
	for (VehicleID id : before.hauler) CHECK(Train::Get(id)->First() == rig.hauler);
}

/** Two mixed exchanges in a row: each wagon has exactly one head and each packet stays on its own wagon throughout. */
TEST_CASE("Repeated mixed exchanges keep exactly one head per wagon", "[tbm-exchange-commit]")
{
	TBMExchangeRig rig;
	rig.BoreAndMeet();
	REQUIRE(ExecuteTBMWagonExchange(rig.Request()).error == TBMExchangeError::None);
	// Stand-ins for the hauler's disposal (Plan 5 Group 8) and the cutter's next cuts: different Earth and Rock loads.
	for (Train *part = rig.hauler; part != nullptr; part = part->Next()) part->cargo.Truncate();
	const auto earth = rig.Wagons(TBMConsistRole::EarthWagon);
	const auto rock = rig.Wagons(TBMConsistRole::RockWagon);
	rig.Preload(earth[0], 10);
	rig.Preload(rock[1], 30);
	const auto packets = GetBothPacketOwners(rig);
	const auto second = ExecuteTBMWagonExchange(rig.Request());
	REQUIRE(second.error == TBMExchangeError::None);
	CHECK(second.earth_t == 10);
	CHECK(second.rock_t == 30);
	CHECK(GetBothPacketOwners(rig) == packets);
	CHECK(GetTBMExchangeAssignmentForHauler(rig.hauler->index)->exchanges == 2);
	CHECK(CaptureTBMWagonSets().size() == 2); // The same two sets swap places; no new set was made.
	// Every train part in the pool belongs to exactly one of the two chains.
	const auto cutter = GetTBMChainIDs(rig.front);
	const auto hauler = GetTBMChainIDs(rig.hauler);
	for (const Train *part : Train::Iterate()) {
		const bool in_cutter = std::ranges::find(cutter, part->index) != cutter.end();
		const bool in_hauler = std::ranges::find(hauler, part->index) != hauler.end();
		CHECK(in_cutter != in_hauler);
	}
	CHECK(cutter.size() + hauler.size() == Vehicle::GetNumItems());
	CHECK(InspectTBMConsist(*rig.front, rig.Project()).error == TBMBlockedReason::None);
}
