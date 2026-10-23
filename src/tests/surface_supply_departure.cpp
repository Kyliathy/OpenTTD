/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_departure.cpp Finite first-departure transitions and native refusal before splitting.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md actual cargo custody.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-c-supply-receipts.md irreversible legacy dispatch.
 * Detached receipts and native component packets are declared inputs, never a legacy save-admission bypass.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "surface_supply_record_fixture.h"
#include "surface_supply_fixture.h"
#include "../surface_supply_order_runtime.h"
#include "../surface_supply_order_wire.h"
#include "../hub_cargo.h"
#include "../core/random_func.hpp"
#include "../timer/timer_game_calendar.h"
#include "../safeguards.h"

/** Declare an admitted original legacy receipt without installing it or materializing stock.
 * @param rig Explicit same-file map/company/station facts for pure conversion.
 * @return Sole detached history with100 purchased,20 refunded and40 previously dispatched units.
 */
static SurfaceSupplyOrderSnapshot DepartureHistory(const SurfaceSupplyRecordRig &rig)
{
	LegacyBSupplyOrderWire row{1, 0, 3, CargoLabel{"RFMT"}, 100,
		TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1).base(), 100, 20, 40, 25, 1};
	const auto converted = ConvertLegacyBSupply({1, 1, 2, {row}}, rig.context);
	REQUIRE(converted.has_value()); return *converted;
}

/** Append a declared real packet to one source list, without pretending its optional link is admitted.
 * @param stock Actual component source owner.
 * @param quantity Whole physical units.
 * @param link Optional declared original ID, intentionally unresolved in refusal cases.
 * @param shipped Original declared shipment flag, never a dispatch accounting call.
 * @return Actual fragment pointer; these fixtures deliberately avoid compatible subsequent appends.
 */
static CargoPacket *DeparturePacket(StationCargoList &stock, uint16_t quantity, uint64_t link = 0, bool shipped = false)
{
	REQUIRE(CargoPacket::CanAllocateItem());
	auto *packet = CargoPacket::Create(StationID{3}, quantity, Source{CompanyID{0}, SourceType::CompanySupply});
	if (link != 0) REQUIRE(packet->RestoreCompanySupplyOrder(link, shipped));
	stock.Append(packet, StationID::Invalid()); return packet;
}

/** Checked subtraction conserves the original finite bill, including a legitimate final departure after closure/loss. */
TEST_CASE("Canonical legacy dispatch consumes only the original unrefunded unshipped remainder", "[surface-supply-departure]")
{
	SurfaceSupplyRecordRig rig; auto book = DepartureHistory(rig);
	const bool owner_lost = GENERATE(false, true);
	if (owner_lost) { book.records[0].owner = CompanyID::Invalid(); book.records[0].origin = StationID::Invalid(); }
	REQUIRE(ValidateSurfaceSupplyRecords(book, rig.context) == SurfaceSupplyRecordError::None);
	const auto original = book; const auto installed = CaptureSurfaceSupplyOrders(); const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
	CHECK(CanRecordSurfaceSupplyDispatch(book, 1, 40)); CHECK_FALSE(CanRecordSurfaceSupplyDispatch(book, 1, 41)); CHECK(book == original);
	REQUIRE(RecordSurfaceSupplyDispatch(book, 1, 15)); CHECK(book.records[0].Legacy()->dispatched == 55);
	REQUIRE(RecordSurfaceSupplyDispatch(book, 1, 25)); CHECK(book.records[0].Legacy()->dispatched == 80);
	const auto exhausted = book; CHECK_FALSE(RecordSurfaceSupplyDispatch(book, 1, 1)); CHECK(book == exhausted);
	book.records[0].Legacy()->dispatched = original.records[0].Legacy()->dispatched;
	CHECK(book == original); // Every other original scalar, including tariff, cap day, refunded count and closure, stayed intact.
	CHECK(CaptureSurfaceSupplyOrders() == installed); CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Unknown/native/zero or contradictory histories refuse before modifying even a detached counter. */
TEST_CASE("Canonical legacy dispatch refuses foreign native and overdrawn history unchanged", "[surface-supply-departure]")
{
	SurfaceSupplyRecordRig rig; auto book = DepartureHistory(rig);
	SurfaceSupplyOrderID id = 1; uint16_t quantity = 1;
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6, 7);
	switch (fault) {
		case 0: id = 0; break;
		case 1: id = uint64_t{1} << 40; break;
		case 2: quantity = 0; break;
		case 3: quantity = 41; break;
		case 4: book.records[0].Legacy()->refunded = 101; break;
		case 5: book.records[0].Legacy()->dispatched = 81; break;
		case 6: book.records[0] = rig.Record(1); break;
		case 7: book.records[0].receipt = std::monostate{}; break;
	}
	const auto before = book;
	CHECK_FALSE(CanRecordSurfaceSupplyDispatch(book, id, quantity)); CHECK_FALSE(RecordSurfaceSupplyDispatch(book, id, quantity)); CHECK(book == before);
}

/** Native load/reserve stops before splitting a forged fragment and reports only physically moved valid prefix units. */
TEST_CASE("Native legacy departure refusal preserves packet and cache counts and reports actual movement", "[surface-supply-departure]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig;
	REQUIRE(CaptureSurfaceSupplyOrders().records.empty());
	const bool reserve = GENERATE(false, true); const uint16_t prefix = GENERATE(uint16_t{0}, uint16_t{2});
	StationCargoList source{}; VehicleCargoList carrier{};
	if (prefix != 0) DeparturePacket(source, prefix);
	auto *bad = DeparturePacket(source, 10, uint64_t{1} << 40);
	const auto packets = CargoPacket::GetNumItems(); const auto random = _random; const auto cash = Company::Get(_current_company)->money;
	const auto book = CaptureSurfaceSupplyOrders(); const auto tile = Station::Get(rig.station)->xy;
	CHECK_FALSE(bad->CanMarkCompanySupplyShipped(3)); CHECK_FALSE(bad->MarkCompanySupplyShipped());
	const uint moved = reserve ? source.Reserve(prefix + 3, &carrier, {}, tile) : source.Load(prefix + 3, &carrier, {}, tile);
	CHECK(moved == prefix); CHECK(source.AvailableCount() == 10); CHECK(carrier.TotalCount() == prefix);
	CHECK(source.ReservedCount() == (reserve ? prefix : 0)); CHECK(bad->Count() == 10); CHECK_FALSE(bad->HasCompanySupplyShipped());
	CHECK(CargoPacket::GetNumItems() == packets); CHECK(CaptureSurfaceSupplyOrders() == book); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	if (reserve) {
		CHECK(carrier.Return(prefix, &source, StationID::Invalid(), tile) == prefix);
		CHECK(source.ReservedCount() == 0); CHECK(source.AvailableCount() == prefix + 10);
	}
}

/** A whole hub preflight visits the later invalid fragment before moving an otherwise valid native prefix. */
TEST_CASE("Hub legacy departure preflight refuses a later unknown order without moving earlier stock", "[surface-supply-departure]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig;
	REQUIRE(CaptureSurfaceSupplyOrders().records.empty());
	const bool shipped = GENERATE(false, true);
	StationCargoList source{}, target{}; VehicleCargoList carrier{};
	DeparturePacket(source, 2); auto *bad = DeparturePacket(source, 10, 99, shipped);
	const auto packets = CargoPacket::GetNumItems(); const auto book = CaptureSurfaceSupplyOrders(); const auto cash = Company::Get(_current_company)->money;
	const auto random = _random; const auto tile = Station::Get(rig.station)->xy;
	CHECK_FALSE(HubCargoTransfer::CanMove(source, 5)); CHECK_FALSE(HubCargoTransfer::Move(source, target, 5));
	CHECK_FALSE(HubCargoTransfer::Move(source, carrier, 5, tile));
	CHECK(source.TotalCount() == 12); CHECK(target.TotalCount() == 0); CHECK(carrier.TotalCount() == 0);
	CHECK(bad->Count() == 10); CHECK(bad->HasCompanySupplyShipped() == shipped); CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(CaptureSurfaceSupplyOrders() == book); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Existing ordinary and unlinked A supplies retain actual native movement without any legacy history event. */
TEST_CASE("Native and ordinary unlinked packets move through hub and reservation without legacy accounting", "[surface-supply-departure]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig;
	REQUIRE(CaptureSurfaceSupplyOrders().records.empty());
	const bool bought = GENERATE(false, true);
	StationCargoList source{}, job{}, returned{}; VehicleCargoList carrier{};
	REQUIRE(CargoPacket::CanAllocateItem());
	source.Append(CargoPacket::Create(StationID{3}, uint16_t{10}, Source{CompanyID{0}, bought ? SourceType::CompanySupply : SourceType::Industry}), StationID::Invalid());
	const auto book = CaptureSurfaceSupplyOrders(); const auto cash = Company::Get(_current_company)->money;
	const auto tile = Station::Get(rig.station)->xy;
	REQUIRE(HubCargoTransfer::Move(source, job, 4)); CHECK(source.TotalCount() == 6); CHECK(job.TotalCount() == 4);
	REQUIRE(HubCargoTransfer::Move(job, carrier, 4, tile)); CHECK(job.TotalCount() == 0); CHECK(carrier.TotalCount() == 4);
	CHECK((*carrier.Packets()->begin())->GetCompanySupplyOrder() == 0); CHECK_FALSE((*carrier.Packets()->begin())->HasCompanySupplyShipped());
	REQUIRE(HubCargoTransfer::Move(carrier, returned, 4, tile)); CHECK(carrier.TotalCount() == 0); CHECK(returned.TotalCount() == 4);
	REQUIRE(returned.Reserve(3, &carrier, {}, tile) == 3); CHECK(returned.ReservedCount() == 3);
	REQUIRE(carrier.Return(3, &returned, StationID::Invalid(), tile) == 3); CHECK(returned.TotalCount() == 4); CHECK(returned.ReservedCount() == 0);
	CHECK(CaptureSurfaceSupplyOrders() == book); CHECK(Company::Get(_current_company)->money == cash);
}
