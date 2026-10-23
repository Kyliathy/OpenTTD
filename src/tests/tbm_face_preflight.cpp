/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file tbm_face_preflight.cpp Complete native face-transfer estimates and unchanged ordered refusal.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md actual packet and train ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group21; mutation follows in21B.
 * Actual loaded-return components do not claim installed face-wait activation or driven route acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_fixture.h"
#include "../safeguards.h"

/**
 * @param units Unit heads.
 * @return Capacity of the units and their articulated parts.
 */
static uint64_t CapacityOf(const std::vector<Train *> &units)
{
	uint64_t total = 0;
	// Count each actual articulated part once; unit metadata cannot replace
	// physical hopper capacity during a whole-load refusal.
	for (Train *unit : units) {
		for (const Train *part = unit; part != nullptr; part = part->Next()) {
			total += part->cargo_cap;
			if (part == unit->GetLastEnginePart()) break;
		}
	}
	return total;
}

/** Require both public preflight entries to refuse before publishing even a partial movement proposal.
	* @param rig Actual native component owners.
	* @param error Independent expected first failed rule.
	*/
static void CheckPreflightRefused(const TBMFaceTransferRig &rig, TBMExchangeError error)
{
	const auto before = TBMFaceSnapshot::Capture(rig);
	const auto described = DescribeTBMFaceTransfer(rig.Request()); const auto validated = ValidateTBMFaceTransfer(rig.Request());
	// A refusal may retain display identities but never an actionable partial
	// source/destination chain or quantity; both public entries share one owner.
	for (const auto *proposal : {&described, &validated}) {
	 CHECK(proposal->error == error); CHECK(proposal->loaded_units.empty()); CHECK(proposal->empty_units.empty());
	 CHECK(proposal->original_cutter.empty()); CHECK(proposal->original_hauler.empty()); CHECK(proposal->proposed_cutter.empty()); CHECK(proposal->proposed_hauler.empty());
	 CHECK(proposal->earth_t == 0); CHECK(proposal->rock_t == 0);
	}
	CHECK(TBMFaceSnapshot::Capture(rig) == before);
}

/** Task 5.5: describing and validating, repeated, return the same proposal and change nothing. */
TEST_CASE("Face transfer estimates leave the state unchanged", "[tbm-face-preflight]")
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
TEST_CASE("Face transfer refusals follow the rule order and change nothing", "[tbm-face-preflight]")
{
	SECTION("the dwell has not elapsed") {
		TBMFaceTransferRig rig(TBMPhase::ReturnToPark, TBM_FACE_TRANSFER_TICKS - 1);
		CheckPreflightRefused(rig, TBMExchangeError::NotAtBay);
	}
	SECTION("the hauler stopped short of the stand") {
		TBMFaceTransferRig rig;
		rig.HaulerOnStand(1);
		CheckPreflightRefused(rig, TBMExchangeError::NotAtBay);
	}
	SECTION("the hauler stands on the stand heading away from the cutter") {
		TBMFaceTransferRig rig;
		rig.HaulerOnStand(0, false);
		CheckPreflightRefused(rig, TBMExchangeError::NotAtBay);
	}
	SECTION("a train is still moving") {
		TBMFaceTransferRig rig;
		rig.hauler->cur_speed = 1;
		CheckPreflightRefused(rig, TBMExchangeError::NotAtBay);
		rig.hauler->cur_speed = 0;
		rig.front->cur_speed = 1;
		CheckPreflightRefused(rig, TBMExchangeError::NotAtBay);
	}
	SECTION("the cutter is boring, not waiting for the transfer") {
		TBMFaceTransferRig rig;
		/** @param project Original staged component; declare elapsed dwell without activating a face wait. */
		const auto dwell = [](TBMProject &project) { project.phase_ticks = TBM_FACE_TRANSFER_TICKS; };
		rig.Reload(TBMPhase::Bore, dwell);
		rig.Assign(); // The reload starts the exchange store afresh.
		CheckPreflightRefused(rig, TBMExchangeError::NotAtBay);
	}
	SECTION("the cutter has handed its spoil over already") {
		// With nothing aboard the exchange is not due, so the face is not ready: NoSpoil is never reached from here.
		TBMFaceTransferRig rig;
		for (Train *part = rig.front; part != nullptr; part = part->Next()) part->cargo.Truncate();
		CheckPreflightRefused(rig, TBMExchangeError::NotAtBay);
	}
	SECTION("cargo reserved for loading cannot change trains") {
		TBMFaceTransferRig rig;
		TBMFaceTransferRig::AddPacket(rig.Wagons(TBMConsistRole::RockWagon).back(), 1, VehicleCargoList::MoveToAction::Load);
		CheckPreflightRefused(rig, TBMExchangeError::UnstableCargo);
	}
	SECTION("a set wagon of the hauler carries cargo") {
		TBMFaceTransferRig rig;
		TBMFaceTransferRig::AddPacket(rig.SetWagons(TBMConsistRole::EarthWagon).front(), 1);
		CheckPreflightRefused(rig, TBMExchangeError::WrongWagons);
	}
	SECTION("the hauler's hoppers cannot take the whole load") {
		TBMFaceTransferRig rig;
		const uint64_t room = CapacityOf(rig.SetWagons(TBMConsistRole::EarthWagon));
		REQUIRE(room < CargoPacket::MAX_COUNT);
		TBMFaceTransferRig::AddPacket(rig.Wagons(TBMConsistRole::EarthWagon).front(), static_cast<uint16_t>(room + 1));
		CheckPreflightRefused(rig, TBMExchangeError::InsufficientEmpties);
	}
	SECTION("a hauler that is not assigned") {
		TBMFaceTransferRig rig;
		const Train *other = rig.BuildHauler(rig.depot, 2, 2);
		const auto before = TBMFaceSnapshot::Capture(rig);
		CHECK(DescribeTBMFaceTransfer({rig.project.id, other->index}).error == TBMExchangeError::NotAssigned);
		CHECK(TBMFaceSnapshot::Capture(rig) == before);
	}
	SECTION("two failed rules report the earlier one") {
		TBMFaceTransferRig rig;
		// Unstable cargo and a loaded set wagon: the cargo rule comes first.
		TBMFaceTransferRig::AddPacket(rig.Wagons(TBMConsistRole::RockWagon).back(), 1, VehicleCargoList::MoveToAction::Load);
		TBMFaceTransferRig::AddPacket(rig.SetWagons(TBMConsistRole::EarthWagon).front(), 1);
		CheckPreflightRefused(rig, TBMExchangeError::UnstableCargo);
		// A hauler short of the stand as well: the meeting rule comes before both.
		rig.HaulerOnStand(1);
		CheckPreflightRefused(rig, TBMExchangeError::NotAtBay);
	}
}

/** The complete proposal describes original chains and real whole-material capacity without relinking or settling anything. */
TEST_CASE("Face preflight names exact unchanged chains and whole actual cargo owners", "[tbm-face-preflight]")
{
	TBMFaceTransferRig rig; const auto before = TBMFaceSnapshot::Capture(rig); const auto proposal = DescribeTBMFaceTransfer(rig.Request());
	REQUIRE(proposal.error == TBMExchangeError::None); CHECK(proposal.bay == FindTBMFaceStand(rig.Project())->tile);
	CHECK(proposal.original_cutter == before.cutter); CHECK(proposal.proposed_cutter == before.cutter);
	CHECK(proposal.original_hauler == before.hauler); CHECK(proposal.proposed_hauler == before.hauler);
	const auto *assignment = GetTBMExchangeAssignmentForHauler(rig.hauler->index); REQUIRE(assignment != nullptr);
	CHECK(proposal.empty_units == GetTBMWagonSet(assignment->set)->wagons);
	CHECK(proposal.earth_t + proposal.rock_t == TBMExchangeRig::Aboard(rig.front)); REQUIRE(proposal.earth_t + proposal.rock_t > 0);
	CHECK(proposal.empty_earth_capacity_t == CapacityOf(rig.SetWagons(TBMConsistRole::EarthWagon)));
	CHECK(proposal.empty_rock_capacity_t == CapacityOf(rig.SetWagons(TBMConsistRole::RockWagon)));
	CHECK(TBMFaceSnapshot::Capture(rig) == before);
}

/** Invalid public request identities/ownership and crashed physical participants cannot acquire a movement proposal. */
TEST_CASE("Face preflight refuses invalid identity foreign ownership and crashed participants", "[tbm-face-preflight]")
{
	TBMFaceTransferRig rig; const auto before = TBMFaceSnapshot::Capture(rig);
	CHECK(DescribeTBMFaceTransfer({TBMProjectID{UINT32_MAX}, rig.hauler->index}).error == TBMExchangeError::InvalidProject);
	CHECK(DescribeTBMFaceTransfer({rig.project.id, VehicleID::Invalid()}).error == TBMExchangeError::NotAssigned);
	{
	 AutoRestoreBackup actor(_current_company, CompanyID::Invalid()); CHECK(DescribeTBMFaceTransfer(rig.Request()).error == TBMExchangeError::InvalidProject);
	}
	CHECK(TBMFaceSnapshot::Capture(rig) == before);
	{
	 AutoRestoreBackup owner(rig.hauler->owner, Owner{OWNER_NONE}); CheckPreflightRefused(rig, TBMExchangeError::InvalidHauler);
	}
	rig.hauler->vehstatus.Set(VehState::Crashed); CheckPreflightRefused(rig, TBMExchangeError::NotAtBay); rig.hauler->vehstatus.Reset(VehState::Crashed);
	rig.front->vehstatus.Set(VehState::Crashed); CheckPreflightRefused(rig, TBMExchangeError::NotAtBay); rig.front->vehstatus.Reset(VehState::Crashed);
	CHECK(TBMFaceSnapshot::Capture(rig) == before);
}

/** Scoped native allocation-admission exhaustion tests the conservative eight-part bound without allocating sixteen million unrelated packets. */
TEST_CASE("Face preflight requires its whole conservative packet split allowance before admitting any movement", "[tbm-face-preflight]")
{
	TBMFaceTransferRig rig; const auto before = TBMFaceSnapshot::Capture(rig); const auto allocated = CargoPacket::GetNumItems();
	// This acquired component has four single-part source hoppers and four
	// actual destination hoppers, so the documented conservative allowance is8.
	REQUIRE(rig.Wagons(TBMConsistRole::EarthWagon).size() == 2); REQUIRE(rig.Wagons(TBMConsistRole::RockWagon).size() == 2);
	REQUIRE(rig.SetWagons(TBMConsistRole::EarthWagon).size() == 2); REQUIRE(rig.SetWagons(TBMConsistRole::RockWagon).size() == 2);
	for (const auto room : {size_t{0}, size_t{7}, size_t{8}}) {
	 // Only the native allocation-admission count is fault-injected. No pool
	 // allocation/deletion/iteration occurs until the scoped count is restored.
	 AutoRestoreBackup capacity(_cargopacket_pool.items, CargoPacketPool::MAX_SIZE - room);
	 const auto expected = room < 8 ? TBMExchangeError::Limit : TBMExchangeError::None;
	 CHECK(DescribeTBMFaceTransfer(rig.Request()).error == expected); CHECK(ValidateTBMFaceTransfer(rig.Request()).error == expected);
	}
	CHECK(CargoPacket::GetNumItems() == allocated); CHECK(TBMFaceSnapshot::Capture(rig) == before);
}
