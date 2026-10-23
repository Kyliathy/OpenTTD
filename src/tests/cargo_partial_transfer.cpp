/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file cargo_partial_transfer.cpp Exact partial delivery refusal preserves native cargo and accounting.
	* @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical packet ownership.
	* @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group17.
	* Declared packet components exercise native reassignment, not procurement or an operational route.
	*/
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../cargopacket.h"
#include "../station_base.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

/** Existing native designations; no parallel cargo state is introduced. */
using CargoAction = VehicleCargoList::MoveToAction;

/** Declare a physical packet with independent original purpose and feeder facts.
	* @param origin Distinct original source prevents unintended packet consolidation.
	* @param count Whole units carried by this fragment.
	* @param purpose Zero is ordinary paying freight, one unlinked native supply, two linked shipped legacy stock.
	* @return Native packet for immediate ownership by a cargo list.
	*/
static CargoPacket *PartialTransferPacket(uint16_t origin, uint16_t count, int purpose)
{
	REQUIRE(CargoPacket::CanAllocateItem());
	if (purpose == 0) return CargoPacket::Create(count, uint16_t{0}, StationID{origin}, TileIndex{0}, Money{count * 10});
	auto *packet = CargoPacket::Create(StationID{origin}, count, Source{Source::Invalid, SourceType::CompanySupply});
	if (purpose == 2) REQUIRE(packet->RestoreCompanySupplyOrder(uint64_t{1} << 40, true));
	return packet;
}

/** Stage explicit packet components through the native action owner.
	* @param list Native destination; the caller retains its automatic teardown.
	* @param prefix Already staged whole transfer units before the eight-unit delivery packet.
	* @param purpose Original paying/native/legacy purpose.
	*/
static void PreparePartialTransfer(VehicleCargoList &list, uint16_t prefix, int purpose)
{
	if (prefix != 0) list.Append(PartialTransferPacket(10, prefix, purpose));
	list.Append(PartialTransferPacket(11, 8, purpose));
	const GoodsEntry goods{};
	REQUIRE(list.Stage(true, StationID::Invalid(), {}, OrderUnloadType::Unload, &goods, CargoType{0}, nullptr, INVALID_TILE));
	if (prefix != 0) REQUIRE(list.Reassign<CargoAction::Deliver, CargoAction::Transfer>(prefix) == prefix);
	// Distinct original routes expose premature mutation when a later split
	// refuses; the existing Transfer prefix must retain its original next hop.
	for (CargoPacket *packet : *list.Packets()) packet->SetNextHop(StationID{12});
}

/** Three transferred units from eight leave exactly five Deliver units for every original purpose and an optional Transfer prefix. */
TEST_CASE("Partial native reassignment preserves exact counts purposes routes and feeder shares", "[cargo-partial-transfer]")
{
	const int purpose = GENERATE(0, 1, 2); const uint16_t prefix = GENERATE(uint16_t{0}, uint16_t{2});
	VehicleCargoList list; PreparePartialTransfer(list, prefix, purpose);
	const auto pool = CargoPacket::GetNumItems();
	REQUIRE(list.Reassign<CargoAction::Deliver, CargoAction::Transfer>(3) == 3);
	CHECK(list.TotalCount() == prefix + 8); CHECK(list.ActionCount(CargoAction::Transfer) == prefix + 3);
	CHECK(list.ActionCount(CargoAction::Deliver) == 5); CHECK(list.ActionCount(CargoAction::Keep) == 0); CHECK(list.ActionCount(CargoAction::Load) == 0);
	CHECK(CargoPacket::GetNumItems() == pool + 1);
	auto it = list.Packets()->begin();
	if (prefix != 0) { CHECK((*it)->Count() == prefix); CHECK((*it)->GetNextHop() == StationID{12}); ++it; }
	const auto *moved = *it++; const auto *remaining = *it++;
	CHECK(it == list.Packets()->end()); CHECK(moved->Count() == 3); CHECK(remaining->Count() == 5);
	CHECK(moved->GetNextHop() == StationID::Invalid()); CHECK(remaining->GetNextHop() == StationID{12});
	CHECK(moved->GetFeederShare() == (purpose == 0 ? Money{30} : Money{0})); CHECK(remaining->GetFeederShare() == (purpose == 0 ? Money{50} : Money{0}));
	// Both fragments preserve the same native purpose and exact optional uint64
	// link; an unrelated action change cannot create a fresh departure or payer.
	for (const auto *packet : {moved, remaining}) {
	 CHECK(packet->GetSource().IsCompanySupply() == (purpose != 0)); CHECK(packet->GetCompanySupplyOrder() == (purpose == 2 ? uint64_t{1} << 40 : 0));
	 CHECK(packet->HasCompanySupplyShipped() == (purpose == 2));
	}
}

/** A later split refusal must leave earlier packet routes unchanged, while a whole boundary needs no allocation even under exhaustion. */
TEST_CASE("Partial reassignment refuses pool exhaustion without earlier mutation and whole boundaries still work", "[cargo-partial-transfer]")
{
	VehicleCargoList list; PreparePartialTransfer(list, 2, 0);
	// Append a second Deliver fragment before staging anew, so the selected
	// request spans a whole packet before its final partial split fails.
	list.Append(PartialTransferPacket(13, 8, 0)); const GoodsEntry goods{};
	REQUIRE(list.Stage(true, StationID::Invalid(), {}, OrderUnloadType::Unload, &goods, CargoType{0}, nullptr, INVALID_TILE));
	REQUIRE(list.Reassign<CargoAction::Deliver, CargoAction::Transfer>(2) == 2);
	for (CargoPacket *packet : *list.Packets()) packet->SetNextHop(StationID{12});
	const auto packets = *list.Packets(); const auto allocated = CargoPacket::GetNumItems();
	{
	 // Scoped allocation-admission fault only: do not allocate/delete/iterate
	 // the pool while its count is overridden. This invokes the real Split
	 // refusal without allocating sixteen million irrelevant dummy packets.
	 AutoRestoreBackup exhausted(_cargopacket_pool.items, CargoPacketPool::MAX_SIZE);
	 REQUIRE_FALSE(CargoPacket::CanAllocateItem()); CHECK(list.Reassign<CargoAction::Deliver, CargoAction::Transfer>(11) == 0);
	}
	CHECK(CargoPacket::GetNumItems() == allocated); CHECK(*list.Packets() == packets); CHECK(list.TotalCount() == 18);
	CHECK(list.ActionCount(CargoAction::Transfer) == 2); CHECK(list.ActionCount(CargoAction::Deliver) == 16);
	// Every selected and unselected fragment must retain its original route and
	// proportional credit when no physical boundary could be admitted.
	for (const auto *packet : *list.Packets()) { CHECK(packet->GetNextHop() == StationID{12}); CHECK(packet->GetFeederShare() == Money{packet->Count() * 10}); }
	{
	 AutoRestoreBackup exhausted(_cargopacket_pool.items, CargoPacketPool::MAX_SIZE);
	 CHECK(list.Reassign<CargoAction::Deliver, CargoAction::Transfer>(8) == 8);
	}
	CHECK(list.ActionCount(CargoAction::Transfer) == 10); CHECK(list.ActionCount(CargoAction::Deliver) == 8); CHECK(CargoPacket::GetNumItems() == allocated);
	CHECK(list.Reassign<CargoAction::Deliver, CargoAction::Transfer>(100) == 8); CHECK(list.ActionCount(CargoAction::Transfer) == 18);
	CHECK(list.ActionCount(CargoAction::Deliver) == 0); CHECK(list.Reassign<CargoAction::Deliver, CargoAction::Transfer>(3) == 0);
}
