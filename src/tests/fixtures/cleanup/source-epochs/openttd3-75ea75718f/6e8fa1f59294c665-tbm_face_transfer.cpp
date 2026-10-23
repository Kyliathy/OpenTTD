/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_transfer.cpp The spoil transfer at the face: whole kept packets move from the cutter's spoil wagons
 * into the hauler's own hoppers while both trains stand at the stand, conserving every tonne, its origin and money; and
 * every refusal, in rule order, changes nothing.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 5 (tasks 5.4 and 5.5)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-a-face-exchange-design.md (rule F3)
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "../company_base.h"
#include "../cargotype.h"
#include "../safeguards.h"

/**
 * @param heads Trains to read.
 * @param label Cargo label.
 * @return Tonnes of that cargo aboard the trains.
 */
static uint64_t Carried(std::initializer_list<const Train *> heads, CargoLabel label)
{
	uint64_t total = 0;
	for (const Train *head : heads) {
		for (const Train *part = head; part != nullptr; part = part->Next()) {
			if (IsValidCargoType(part->cargo_type) && CargoSpec::Get(part->cargo_type)->label == label) total += part->cargo.TotalCount();
		}
	}
	return total;
}

/**
 * @param units Unit heads.
 * @return Tonnes aboard the units and their articulated parts.
 */
static uint64_t CarriedBy(const std::vector<Train *> &units)
{
	uint64_t total = 0;
	for (Train *unit : units) {
		for (const Train *part = unit; part != nullptr; part = part->Next()) {
			total += part->cargo.TotalCount();
			if (part == unit->GetLastEnginePart()) break;
		}
	}
	return total;
}

/**
 * @param units Unit heads.
 * @return Capacity of the units and their articulated parts.
 */
static uint64_t CapacityOf(const std::vector<Train *> &units)
{
	uint64_t total = 0;
	for (Train *unit : units) {
		for (const Train *part = unit; part != nullptr; part = part->Next()) {
			total += part->cargo_cap;
			if (part == unit->GetLastEnginePart()) break;
		}
	}
	return total;
}

/**
 * Check that the description, the validation and the commit all refuse with one error and change nothing.
 * @param rig Rig whose request is refused.
 * @param error Expected refusal.
 */
static void CheckRefused(const TBMFaceTransferRig &rig, TBMExchangeError error)
{
	const auto before = TBMFaceSnapshot::Capture(rig);
	CHECK(DescribeTBMFaceTransfer(rig.Request()).error == error);
	CHECK(ValidateTBMFaceTransfer(rig.Request()).error == error);
	CHECK(ExecuteTBMFaceTransfer(rig.Request()).error == error);
	CHECK(TBMFaceSnapshot::Capture(rig) == before);
}

/** Tasks 5.4 and 5.5: the transfer moves every tonne into the hauler's own hoppers and changes nothing else. */
TEST_CASE("A face transfer moves the cutter's spoil into the hauler's own hoppers", "[demo-fix-face-transfer]")
{
	AutoRestoreBackup<decltype(_settings_game.vehicle.freight_trains)> freight_weight(_settings_game.vehicle.freight_trains, 1);
	for (const TBMPhase phase : {TBMPhase::ReturnToPark, TBMPhase::AwaitFaceTransfer}) {
		INFO("phase " << static_cast<int>(to_underlying(phase)));
		// The breakpoint wait runs on a longer route, so its breakpoint falls between cuts rather than at the route's end.
		TBMFaceTransferRig rig(phase, TBM_FACE_TRANSFER_TICKS, phase == TBMPhase::AwaitFaceTransfer ? 40 : 24);
		// Spoil with real mass, so a stale physical cache after the transfer would show (as in tbm_work.cpp). The rig
		// builds the cargo specs, so the weights are set after it.
		CargoSpec *earth_spec = CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"ERTH"}));
		CargoSpec *rock_spec = CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"ROCK"}));
		AutoRestoreBackup<decltype(earth_spec->weight)> earth_weight(earth_spec->weight, 16);
		AutoRestoreBackup<decltype(rock_spec->weight)> rock_weight(rock_spec->weight, 16);
		AutoRestoreBackup<bool> earth_freight(earth_spec->is_freight, true);
		AutoRestoreBackup<bool> rock_freight(rock_spec->is_freight, true);
		rig.front->CargoChanged();
		rig.hauler->CargoChanged();
		TBMPacketCustody custody;
		const auto totals = CountTBMPackets({rig.front, rig.hauler});
		const auto cutter_chain = GetTBMChainIDs(rig.front);
		const auto hauler_chain = GetTBMChainIDs(rig.hauler);
		const Money money = Company::Get(_current_company)->money;
		const uint64_t earth = Carried({rig.front}, CargoLabel{"ERTH"});
		const uint64_t rock = Carried({rig.front}, CargoLabel{"ROCK"});
		REQUIRE(earth + rock != 0);
		REQUIRE(TBMExchangeRig::Aboard(rig.hauler) == 0);
		const uint32_t hauler_weight = rig.hauler->gcache.cached_weight;
		const uint32_t cutter_weight = rig.front->gcache.cached_weight;
		const auto assignment = *GetTBMExchangeAssignmentForHauler(rig.hauler->index);
		const TBMWagonSet set = *GetTBMWagonSet(assignment.set);

		const auto described = DescribeTBMFaceTransfer(rig.Request());
		REQUIRE(described.error == TBMExchangeError::None);
		CHECK(described.earth_t == earth);
		CHECK(described.rock_t == rock);
		CHECK(described.bay == FindTBMFaceStand(rig.Project())->tile);
		CHECK(described.empty_units == set.wagons);
		// No part changes train: both chains stay as they are.
		CHECK(described.original_cutter == cutter_chain);
		CHECK(described.original_hauler == hauler_chain);
		CHECK(described.proposed_cutter == described.original_cutter);
		CHECK(described.proposed_hauler == described.original_hauler);

		const auto committed = ExecuteTBMFaceTransfer(rig.Request());
		REQUIRE(committed.error == TBMExchangeError::None);
		CHECK(committed == described);
		// Conservation: every tonne, its source, first station, age and feeder share; no money; one owner per packet.
		CHECK(CountTBMPackets({rig.front, rig.hauler}) == totals);
		CHECK(Company::Get(_current_company)->money == money);
		CHECK(custody.IsSole());
		CHECK(TBMExchangeRig::Aboard(rig.front) == 0);
		CHECK(CarriedBy(rig.SetWagons(TBMConsistRole::EarthWagon)) == earth);
		CHECK(CarriedBy(rig.SetWagons(TBMConsistRole::RockWagon)) == rock);
		CHECK(GetTBMChainIDs(rig.front) == cutter_chain);
		CHECK(GetTBMChainIDs(rig.hauler) == hauler_chain);
		// Every hopper holds at most its capacity, all of it kept aboard.
		for (const Train *part = rig.hauler; part != nullptr; part = part->Next()) {
			CHECK(part->cargo.TotalCount() <= part->cargo_cap);
			CHECK(part->cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) == part->cargo.TotalCount());
		}
		// The spoil is physical mass: both trains' caches follow it.
		CHECK(rig.hauler->gcache.cached_weight > hauler_weight);
		CHECK(rig.front->gcache.cached_weight < cutter_weight);

		// The hauler takes its own set away loaded, owing the origin visit.
		const auto after = *GetTBMExchangeAssignmentForHauler(rig.hauler->index);
		const TBMWagonSet *loaded = GetTBMWagonSet(assignment.set);
		REQUIRE(loaded != nullptr);
		CHECK(loaded->state == TBMWagonSetState::Loaded);
		CHECK_FALSE(loaded->visited_origin);
		CHECK(loaded->wagons == set.wagons);
		CHECK(loaded->revision == set.revision + 1);
		CHECK(after.phase == TBMHaulerPhase::CollectLoaded);
		CHECK(after.wait == TBMHaulerWait::None);
		CHECK(after.phase_ticks == 0);
		CHECK(after.exchanges == assignment.exchanges + 1);
		CHECK(after.revision == assignment.revision + 1);
		CHECK(after.set == assignment.set);
		// The origin rule still binds the spoil: it leaves only at the project's origin until the visit.
		for (const Train *part = rig.hauler; part != nullptr; part = part->Next()) {
			if (part->cargo.TotalCount() == 0) continue;
			CHECK(MayTBMSpoilLeaveVehicle(*rig.hauler, *part, rig.Project().origin));
			CHECK_FALSE(MayTBMSpoilLeaveVehicle(*rig.hauler, *part, StationID::Invalid()));
		}
		CHECK(rig.Project().origin == rig.fixture.origin->index);

		// With nothing aboard the cutter the exchange is no longer due, so a second commit is refused and changes nothing.
		CHECK_FALSE(IsTBMExchangeDue(rig.Project()));
		CheckRefused(rig, TBMExchangeError::NotAtBay);
	}
}

/** Task 5.5: describing and validating, repeated, return the same proposal and change nothing. */
TEST_CASE("Face transfer estimates leave the state unchanged", "[demo-fix-face-transfer]")
{
	TBMFaceTransferRig rig;
	const auto before = TBMFaceSnapshot::Capture(rig);
	const auto first = DescribeTBMFaceTransfer(rig.Request());
	REQUIRE(first.error == TBMExchangeError::None);
	CHECK(DescribeTBMFaceTransfer(rig.Request()) == first);
	CHECK(ValidateTBMFaceTransfer(rig.Request()) == first);
	CHECK(TBMFaceSnapshot::Capture(rig) == before);
}

/** Tasks 5.2 and 5.5: each rule refuses on its own, and the earlier rule wins when two fail; nothing changes. */
TEST_CASE("Face transfer refusals follow the rule order and change nothing", "[demo-fix-face-transfer]")
{
	SECTION("the dwell has not elapsed") {
		TBMFaceTransferRig rig(TBMPhase::ReturnToPark, TBM_FACE_TRANSFER_TICKS - 1);
		CheckRefused(rig, TBMExchangeError::NotAtBay);
	}
	SECTION("the hauler stopped short of the stand") {
		TBMFaceTransferRig rig;
		rig.HaulerOnStand(1);
		CheckRefused(rig, TBMExchangeError::NotAtBay);
	}
	SECTION("the hauler stands on the stand heading away from the cutter") {
		TBMFaceTransferRig rig;
		rig.HaulerOnStand(0, false);
		CheckRefused(rig, TBMExchangeError::NotAtBay);
	}
	SECTION("a train is still moving") {
		TBMFaceTransferRig rig;
		rig.hauler->cur_speed = 1;
		CheckRefused(rig, TBMExchangeError::NotAtBay);
		rig.hauler->cur_speed = 0;
		rig.front->cur_speed = 1;
		CheckRefused(rig, TBMExchangeError::NotAtBay);
	}
	SECTION("the cutter is boring, not waiting for the transfer") {
		TBMFaceTransferRig rig;
		rig.Reload(TBMPhase::Bore, [](TBMProject &project) { project.phase_ticks = TBM_FACE_TRANSFER_TICKS; });
		rig.Assign(); // The reload starts the exchange store afresh.
		CheckRefused(rig, TBMExchangeError::NotAtBay);
	}
	SECTION("the cutter has handed its spoil over already") {
		// With nothing aboard the exchange is not due, so the face is not ready: NoSpoil is never reached from here.
		TBMFaceTransferRig rig;
		for (Train *part = rig.front; part != nullptr; part = part->Next()) part->cargo.Truncate();
		CheckRefused(rig, TBMExchangeError::NotAtBay);
	}
	SECTION("cargo reserved for loading cannot change trains") {
		TBMFaceTransferRig rig;
		TBMFaceTransferRig::AddPacket(rig.Wagons(TBMConsistRole::RockWagon).back(), 1, VehicleCargoList::MoveToAction::Load);
		CheckRefused(rig, TBMExchangeError::UnstableCargo);
	}
	SECTION("a set wagon of the hauler carries cargo") {
		TBMFaceTransferRig rig;
		TBMFaceTransferRig::AddPacket(rig.SetWagons(TBMConsistRole::EarthWagon).front(), 1);
		CheckRefused(rig, TBMExchangeError::WrongWagons);
	}
	SECTION("the hauler's hoppers cannot take the whole load") {
		TBMFaceTransferRig rig;
		const uint64_t room = CapacityOf(rig.SetWagons(TBMConsistRole::EarthWagon));
		REQUIRE(room < CargoPacket::MAX_COUNT);
		TBMFaceTransferRig::AddPacket(rig.Wagons(TBMConsistRole::EarthWagon).front(), static_cast<uint16_t>(room + 1));
		CheckRefused(rig, TBMExchangeError::InsufficientEmpties);
	}
	SECTION("a hauler that is not assigned") {
		TBMFaceTransferRig rig;
		const Train *other = rig.BuildHauler(rig.depot, 2, 2);
		const auto before = TBMFaceSnapshot::Capture(rig);
		CHECK(DescribeTBMFaceTransfer({rig.project.id, other->index}).error == TBMExchangeError::NotAssigned);
		CHECK(ExecuteTBMFaceTransfer({rig.project.id, other->index}).error == TBMExchangeError::NotAssigned);
		CHECK(TBMFaceSnapshot::Capture(rig) == before);
	}
	SECTION("two failed rules report the earlier one") {
		TBMFaceTransferRig rig;
		// Unstable cargo and a loaded set wagon: the cargo rule comes first.
		TBMFaceTransferRig::AddPacket(rig.Wagons(TBMConsistRole::RockWagon).back(), 1, VehicleCargoList::MoveToAction::Load);
		TBMFaceTransferRig::AddPacket(rig.SetWagons(TBMConsistRole::EarthWagon).front(), 1);
		CheckRefused(rig, TBMExchangeError::UnstableCargo);
		// A hauler short of the stand as well: the meeting rule comes before both.
		rig.HaulerOnStand(1);
		CheckRefused(rig, TBMExchangeError::NotAtBay);
	}
}
