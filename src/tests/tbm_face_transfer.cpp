/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file tbm_face_transfer.cpp Native whole-load packet commit, stable hoppers and physical train cache refresh.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md actual custody and paid work.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23 installed face-wait transaction replay.
 * Loaded-return poses and additional packet cases are declared components, not original-save or driven gameplay proof.
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
	// Actual native part labels select the delivered material; no set record
	// or proposed count substitutes for a packet owner.
	for (const Train *head : heads) {
		for (const Train *part = head; part != nullptr; part = part->Next()) {
			// Only this material in an actual valid cargo compartment contributes
			// to whole-load conservation; empty non-cargo machinery is irrelevant.
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
	// Include every actual articulated hopper part in its original unit;
	// the transfer must not leave hidden cargo behind a counted unit head.
	for (Train *unit : units) {
		for (const Train *part = unit; part != nullptr; part = part->Next()) {
			total += part->cargo.TotalCount();
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
TEST_CASE("A face transfer moves the cutter's spoil into the hauler's own hoppers", "[tbm-face-transfer]")
{
	AutoRestoreBackup<decltype(_settings_game.vehicle.freight_trains)> freight_weight(_settings_game.vehicle.freight_trains, 1);
	// Exercise identical native custody and physical-cache guarantees in the historical loaded return and
	// the installed face wait. Component poses remain separate from Group23B's driven disposal loop.
	for (const TBMPhase phase : {TBMPhase::ReturnToPark, TBMPhase::AwaitFaceTransfer}) {
		INFO("phase " << static_cast<int>(to_underlying(phase)));
		TBMFaceTransferRig rig(phase, TBM_FACE_TRANSFER_TICKS, 40);
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


/** A real boundary split fills the existing first hopper before the second, preserving proportional credits and every packet provenance field. */
TEST_CASE("Face transfer splits the original packet only at stable destination hopper capacity", "[tbm-face-transfer]")
{
	TBMFaceTransferRig rig;
	// Replace only this component's source input packets through native list
	// destruction. Two separate historical fragments then force one exact split.
	for (Train *part = rig.front; part != nullptr; part = part->Next()) part->cargo.Truncate();
	const auto sources = rig.Wagons(TBMConsistRole::EarthWagon); const auto targets = rig.SetWagons(TBMConsistRole::EarthWagon);
	REQUIRE(sources.size() == 2); REQUIRE(targets.size() == 2); const uint16_t cap = targets[0]->cargo_cap;
	REQUIRE(cap > 2); REQUIRE(targets[1]->cargo_cap == cap); REQUIRE(sources[0]->cargo_cap >= cap - 1); REQUIRE(sources[1]->cargo_cap >= cap - 1);
	std::array<CargoPacketID, 2> identities;
	// Different ages and actual source tiles keep the two originals distinct;
	// each declared credit is ten per unit so proportional split rounding is exact.
	for (size_t i = 0; i < sources.size(); ++i) {
	 REQUIRE(CargoPacket::CanAllocateItem()); const uint16_t quantity = cap - 1;
	 auto *packet = CargoPacket::Create(quantity, static_cast<uint16_t>(7 + i), rig.Project().origin, sources[i]->tile, Money{quantity * 10});
	 packet->UpdateLoadingTile(sources[i]->tile); identities[i] = packet->index; sources[i]->cargo.Append(packet);
	}
	rig.front->CargoChanged(); const auto totals = CountTBMPackets({rig.front, rig.hauler}); const auto allocated = CargoPacket::GetNumItems();
	const auto source_chain = GetTBMChainIDs(rig.front); const auto target_chain = GetTBMChainIDs(rig.hauler); TBMPacketCustody custody;
	REQUIRE(ExecuteTBMFaceTransfer(rig.Request()).error == TBMExchangeError::None);
	CHECK(TBMExchangeRig::Aboard(rig.front) == 0); CHECK(targets[0]->cargo.TotalCount() == cap); CHECK(targets[1]->cargo.TotalCount() == cap - 2);
	CHECK(CargoPacket::GetNumItems() == allocated + 1); CHECK(CountTBMPackets({rig.front, rig.hauler}) == totals); CHECK(custody.IsSole());
	CHECK(GetTBMChainIDs(rig.front) == source_chain); CHECK(GetTBMChainIDs(rig.hauler) == target_chain);
	REQUIRE(targets[0]->cargo.Packets()->size() == 2); REQUIRE(targets[1]->cargo.Packets()->size() == 1);
	const auto first = targets[0]->cargo.Packets()->front(); const auto split = targets[0]->cargo.Packets()->back(); const auto remainder = targets[1]->cargo.Packets()->front();
	CHECK(first->index == identities[0]); CHECK(first->Count() == cap - 1); CHECK(first->GetFeederShare() == Money{(cap - 1) * 10});
	CHECK(split->index != identities[1]); CHECK(split->Count() == 1); CHECK(split->GetFeederShare() == 10);
	CHECK(remainder->index == identities[1]); CHECK(remainder->Count() == cap - 2); CHECK(remainder->GetFeederShare() == Money{(cap - 2) * 10});
	CheckRefused(rig, TBMExchangeError::NotAtBay);
}

/** Execution must re-run the whole preflight before touching packets or native train/set state. */
TEST_CASE("Face execution refuses allocation limits and stale dwell before any physical prefix moves", "[tbm-face-transfer]")
{
	TBMFaceTransferRig rig; const auto before = TBMFaceSnapshot::Capture(rig); const auto allocated = CargoPacket::GetNumItems();
	// Fault-inject only refusing allocation admission. No successful execution
	// runs under a fake pool count, and all snapshots follow its restoration.
	for (const auto room : {size_t{0}, size_t{7}}) {
	 {
	  AutoRestoreBackup capacity(_cargopacket_pool.items, CargoPacketPool::MAX_SIZE - room);
	  CHECK(ExecuteTBMFaceTransfer(rig.Request()).error == TBMExchangeError::Limit);
	 }
	 CHECK(CargoPacket::GetNumItems() == allocated); CHECK(TBMFaceSnapshot::Capture(rig) == before);
	}
	REQUIRE(ExecuteTBMFaceTransfer(rig.Request()).error == TBMExchangeError::None); CHECK(TBMExchangeRig::Aboard(rig.front) == 0);
	CheckRefused(rig, TBMExchangeError::NotAtBay);
}

/** The execution entry preserves every public packet/train fact when the required saved dwell has not elapsed. */
TEST_CASE("Face execution leaves an unelapsed loaded-return encounter unchanged", "[tbm-face-transfer]")
{
	TBMFaceTransferRig rig(TBMPhase::ReturnToPark, TBM_FACE_TRANSFER_TICKS - 1); CheckRefused(rig, TBMExchangeError::NotAtBay);
}
