/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply.cpp Actual paid-order/packet/payment boundaries; explicit fixture stations and arrival do not prove the later driven bootstrap.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "surface_supply_fixture.h"
#include "../surface_supply.h"
#include "../surface_supply_cmd.h"
#include "../station_supply.h"
#include "../train.h"
#include "../economy_base.h"
#include "../timer/timer_game_economy.h"
#include "../safeguards.h"

/** Quotes and estimates retain actual money, packet pool, station stock and the unallocated order cursor. */
TEST_CASE("Paid Surface seed quotes are pure and use canonical dates and tariffs", "[surface-supply-quote]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	const auto cash = Company::Get(_current_company)->money;
	const auto packets = CargoPacket::GetNumItems();
	const auto next = GetNextSurfaceSupplyOrderID();
	// Each label has its own canonical introduction and tariff; no early Deep
	// manufacturing or hidden-layer stock is needed for these paid Surface orders.
	for (const auto &[label, introduced, price] : std::array{
		std::tuple{CargoLabel{"STEL"}, 1950, 100}, std::tuple{CargoLabel{"RFMT"}, 1950, 180},
		std::tuple{CargoLabel{"ELEC"}, 1965, 500}, std::tuple{CargoLabel{"BATY"}, 1965, 350}}) {
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{introduced - 1}, 0, 1);
		CHECK(PrepareSurfaceSupplyOrder(rig.origin->index, label, 1).error == SurfaceSupplyError::Date);
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{introduced}, 0, 1);
		const auto quote = PrepareSurfaceSupplyOrder(rig.origin->index, label, 2);
		REQUIRE(quote.error == SurfaceSupplyError::None);
		CHECK(quote.cost == Money{price * 2});
		CHECK(PlaceSurfaceSupplyOrder(quote, false) == SurfaceSupplyError::None);
		CHECK(rig.origin->goods[quote.cargo].TotalCount() == 0);
	}
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(GetNextSurfaceSupplyOrderID() == next);
	CHECK(PrepareSurfaceSupplyOrder(rig.origin->index, CargoLabel{"CHEM"}, 1).error == SurfaceSupplyError::Supply);
	CHECK(PrepareSurfaceSupplyOrder(rig.origin->index, CargoLabel{"STEL"}, 0).error == SurfaceSupplyError::Quantity);
	_economy.inflation_prices = uint64_t{2} << 16;
	CHECK(PrepareSurfaceSupplyOrder(rig.origin->index, CargoLabel{"STEL"}, 2).cost == Money{400});
}

/** Actual paid history shares one cap across stations, and an identical quote cannot settle twice. */
TEST_CASE("Paid Surface orders settle once and retain the company economy-day cap", "[surface-supply-order]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	const auto cash = Company::Get(_current_company)->money;
	const auto quote = PrepareSurfaceSupplyOrder(rig.origin->index, CargoLabel{"STEL"}, 256);
	REQUIRE(quote.error == SurfaceSupplyError::None);
	REQUIRE(PlaceSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::None);
	CHECK(Company::Get(_current_company)->money == cash - Money{25600});
	CHECK(rig.Queue(CargoLabel{"STEL"}).TotalCount() == 256);
	CHECK(rig.origin->goods[quote.cargo].HasRating());
	CHECK(rig.origin->goods[quote.cargo].link_graph != LinkGraphID::Invalid());
	const auto *order = GetSurfaceSupplyOrder(quote.next_order);
	REQUIRE(order != nullptr);
	CHECK(order->quantity == 256);
	CHECK(PrepareSurfaceSupplyOrder(rig.second->index, CargoLabel{"STEL"}, 1).error == SurfaceSupplyError::DailyCap);
	CHECK(PlaceSurfaceSupplyOrder(quote, true) != SurfaceSupplyError::None);
	CHECK(rig.Queue(CargoLabel{"STEL"}).TotalCount() == 256);
	CHECK(Company::Get(_current_company)->money == cash - Money{25600});
	TimerGameEconomy::date = TimerGameEconomy::Date{TimerGameEconomy::date.base() + 1};
	const auto tomorrow = PrepareSurfaceSupplyOrder(rig.second->index, CargoLabel{"STEL"}, 1);
	REQUIRE(tomorrow.error == SurfaceSupplyError::None);
	CHECK(tomorrow.day_used == 0);
	CHECK(PlaceSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::Stale);
	CHECK(GetNextSurfaceSupplyOrderID().base() == quote.next_order.base() + 1);
}

/** A stock/funds/ownership/date change after quoting refuses before allocation or expense. */
TEST_CASE("Paid Surface orders refuse wrong facilities and stale complete admission unchanged", "[surface-supply-refusal]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	const CargoLabel label{"RFMT"};
	const auto quote = PrepareSurfaceSupplyOrder(rig.origin->index, label, 2);
	REQUIRE(quote.error == SurfaceSupplyError::None);
	const auto cursor = GetNextSurfaceSupplyOrderID();
	SECTION("actual funds") {
		Company::Get(_current_company)->money = 359;
		CHECK(PlaceSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::Funds);
		CHECK(Company::Get(_current_company)->money == Money{359});
	}
	SECTION("bounded stock includes native pickup reservations") {
		rig.Queue(label).Append(CargoPacket::Create(rig.origin->index, UINT16_MAX, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
		REQUIRE(Train::CanAllocateItem()); rig.carrier = Train::Create();
		const std::array<StationID, 0> next{};
		REQUIRE(rig.Queue(label).Reserve(1, &rig.carrier->cargo, next, rig.origin->xy) == 1);
		CHECK(rig.carrier->cargo.ReservedCount() == 1);
		CHECK(PlaceSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::Stock);
	}
	SECTION("wrong company") {
		AutoRestoreBackup actor{_current_company, CompanyID::Invalid()};
		CHECK(PlaceSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::Company);
	}
	SECTION("bus stop") {
		rig.origin->facilities = {StationFacility::BusStop};
		CHECK(PlaceSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::Station);
	}
	SECTION("underground same XY") {
		Station *hidden = rig.StationAt(TileXYL(20, 20, MapLayer::Underground));
		CHECK(PrepareSurfaceSupplyOrder(hidden->index, label, 2).error == SurfaceSupplyError::Station);
		CHECK(hidden->goods[quote.cargo].TotalCount() == 0);
	}
	SECTION("edited quote") {
		auto changed = quote; changed.cost = 1;
		CHECK(PlaceSurfaceSupplyOrder(changed, true) == SurfaceSupplyError::Stale);
	}
	CHECK(GetNextSurfaceSupplyOrderID() == cursor);
}

/** Rating culling preserves actual paid assets while ordinary freight and explicit removal keep their native policies. */
TEST_CASE("Station rating culling retains purchased construction packets", "[surface-supply-rating]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyRig rig;
	const CargoLabel label{"STEL"}; const auto quote = PrepareSurfaceSupplyOrder(rig.origin->index, label, 5);
	REQUIRE(PlaceSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::None);
	const auto cash = Company::Get(_current_company)->money;
	const std::vector<SurfaceSupplyOrder> history(GetSurfaceSupplyOrderHistory().begin(), GetSurfaceSupplyOrderHistory().end());
	StationCargoAmountMap remaining;
	// An all-protected queue must terminate with every actual native unit and
	// its original source total retained; history is no substitute for packets.
	CHECK(rig.Queue(label).Truncate(UINT_MAX, &remaining, true) == 0);
	CHECK(rig.Queue(label).TotalCount() == 5); CHECK(remaining[rig.origin->index] == 5);
	SECTION("mixed queues cull ordinary freight and preserve exact cancellation") {
		REQUIRE(CargoPacket::CanAllocateItem());
		CargoPacket *ordinary = CargoPacket::Create(rig.origin->index, 3, Source{Source::Invalid, SourceType::Industry});
		rig.Queue(label).Append(ordinary, StationID::Invalid()); remaining.clear();
		CHECK(rig.Queue(label).Truncate(UINT_MAX, &remaining, true) == 3);
		CHECK(rig.Queue(label).TotalCount() == 5); CHECK(remaining[rig.origin->index] == 5);
		CHECK(rig.Queue(label).CountUnshippedCompanySupply(quote.next_order.base()) == 5);
		const auto cancellation = PrepareSurfaceSupplyCancellation(quote.next_order); REQUIRE(cancellation.error == SurfaceSupplyError::None);
		REQUIRE(CancelSurfaceSupplyOrder(cancellation, true) == SurfaceSupplyError::None);
		CHECK(rig.Queue(label).TotalCount() == 0); CHECK(Company::Get(_current_company)->money == cash + quote.cost);
	}
	SECTION("explicit destruction retains its existing no-refund removal policy") {
		CHECK(rig.Queue(label).Truncate() == 5); CHECK(rig.Queue(label).TotalCount() == 0);
		CHECK(Company::Get(_current_company)->money == cash);
		CHECK(std::ranges::equal(GetSurfaceSupplyOrderHistory(), history));
	}
}

/** Native packet loading, split/merge and delivery preserve paid purpose and never book feeder/final/subsidy income. */
TEST_CASE("Purchased material reaches the native receiver without freight payment", "[surface-supply-purpose]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	const CargoLabel label{"RFMT"};
	AutoRestoreBackup<Money> payment{CargoSpec::Get(GetCargoTypeByLabel(label))->current_payment, Money{1000000}};
	const auto quote = PrepareSurfaceSupplyOrder(rig.origin->index, label, 2);
	REQUIRE(PlaceSurfaceSupplyOrder(quote, true) == SurfaceSupplyError::None);
	const auto cash = Company::Get(_current_company)->money;
	rig.Load(label, 2);
	CargoPacket *packet = rig.carrier->cargo.Packets()->front();
	CHECK(packet->GetCompanySupplyOrder() == quote.next_order.base());
	CHECK(packet->HasCompanySupplyShipped());
	REQUIRE(Train::CanAllocateItem());
	Train *relay = Train::Create();
	relay->owner = _current_company; relay->cargo_type = quote.cargo; relay->cargo_cap = 2;
	// The real partial vehicle move owns both count/action caches and packet
	// splitting. Directly splitting a packet inside a live list would bypass that custody boundary.
	REQUIRE(rig.carrier->cargo.Shift(1, &relay->cargo) == 1);
	const CargoPacket *split = relay->cargo.Packets()->front();
	CHECK(split->GetCompanySupplyOrder() == quote.next_order.base());
	CHECK(split->HasCompanySupplyShipped());
	REQUIRE(relay->cargo.Shift(1, &rig.carrier->cargo) == 1);
	CHECK(rig.carrier->cargo.TotalCount() == 2);
	CHECK(rig.carrier->cargo.Packets()->size() == 1);
	packet = rig.carrier->cargo.Packets()->front();
	CHECK(rig.carrier->cargo_payment->PayTransfer(quote.cargo, packet, 2, rig.destination->xy) == Money{0});
	const uint waiting = rig.Input(label).waiting;
	rig.carrier->cargo.Stage(true, rig.destination->index, {}, OrderUnloadType::UnloadIfPossible,
		&rig.destination->goods[quote.cargo], quote.cargo, rig.carrier->cargo_payment, rig.destination->xy);
	REQUIRE(rig.carrier->cargo.Unload(2, &rig.destination->goods[quote.cargo].GetOrCreateData().cargo,
		quote.cargo, rig.carrier->cargo_payment, rig.destination->xy) == 2);
	CHECK(rig.Input(label).waiting == waiting + 2);
	CHECK(rig.carrier->cargo.TotalCount() == 0);
	rig.SettlePayment();
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(GetSurfaceSupplyOrder(quote.next_order)->quantity == 2);
}

/** A genuinely priced ordinary packet on the same native route still earns final freight income. */
TEST_CASE("Ordinary native freight payment remains positive beside company supplies", "[surface-supply-payment-control]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	const CargoLabel label{"RFMT"};
	const CargoType cargo = GetCargoTypeByLabel(label);
	AutoRestoreBackup<Money> payment{CargoSpec::Get(cargo)->current_payment, Money{1000000}};
	const auto cash = Company::Get(_current_company)->money;
	REQUIRE(CargoPacket::CanAllocateItem());
	rig.Queue(label).Append(CargoPacket::Create(rig.origin->index, 2, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid()); // Explicit ordinary control stock, not purchased seed.
	rig.Load(label, 2);
	CHECK(rig.carrier->cargo.Packets()->front()->GetCompanySupplyOrder() == 0);
	rig.carrier->cargo.Stage(true, rig.destination->index, {}, OrderUnloadType::UnloadIfPossible,
		&rig.destination->goods[cargo], cargo, rig.carrier->cargo_payment, rig.destination->xy);
	REQUIRE(rig.carrier->cargo.Unload(2, &rig.destination->goods[cargo].GetOrCreateData().cargo,
		cargo, rig.carrier->cargo_payment, rig.destination->xy) == 2);
	rig.SettlePayment();
	CHECK(Company::Get(_current_company)->money > cash);
}

/** Native command estimates allocate nothing; synchronized execution pays once and identical retried cursors refuse. */
TEST_CASE("Native Surface purchase commands retain one invoice and explicit retry identity", "[surface-supply-command]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	const CargoLabel label{"BATY"};
	const auto cursor = GetNextSurfaceSupplyOrderID();
	const auto cash = Company::Get(_current_company)->money;
	const auto packets = CargoPacket::GetNumItems();
	const auto estimate = Command<Commands::OrderSurfaceSupply>::Do({}, rig.origin->index, label, 2, cursor);
	REQUIRE(estimate.Succeeded());
	CHECK(estimate.GetCost() == Money{0});
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(GetNextSurfaceSupplyOrderID() == cursor);
	CHECK(CargoPacket::GetNumItems() == packets);
	const auto settled = Command<Commands::OrderSurfaceSupply>::Do(DoCommandFlag::Execute, rig.origin->index, label, 2, cursor);
	REQUIRE(settled.Succeeded());
	CHECK(settled.GetCost() == Money{0});
	CHECK(Company::Get(_current_company)->money == cash - Money{700});
	CHECK(rig.Queue(label).TotalCount() == 2);
	CHECK(Command<Commands::OrderSurfaceSupply>::Do(DoCommandFlag::Execute, rig.origin->index, label, 2, cursor).Failed());
	CHECK(Company::Get(_current_company)->money == cash - Money{700});
	CHECK(rig.Queue(label).TotalCount() == 2);
}

/** The actual native command transports canonical label bytes and typed station/cursor values without slot inference. */
TEST_CASE("Surface purchase command arguments round trip through native network buffers", "[surface-supply-command-serialization]")
{
	using Args = CommandTraits<Commands::OrderSurfaceSupply>::Args;
	const Args request{StationID{123}, CargoLabel{"RFMT"}, uint16_t{17}, SurfaceSupplyOrderID{42}};
	const auto bytes = EndianBufferWriter<CommandDataBuffer>::FromValue(request);
	CHECK(bytes.size() == sizeof(uint16_t) + 4 + sizeof(uint16_t) + sizeof(uint32_t));
	CHECK(EndianBufferReader::ToValue<Args>(bytes) == request);
	const Args unknown{StationID{123}, CargoLabel{}, uint16_t{17}, SurfaceSupplyOrderID{42}};
	CHECK(EndianBufferReader::ToValue<Args>(EndianBufferWriter<CommandDataBuffer>::FromValue(unknown)) == unknown);
}

/** Native queue/hold consolidation cannot erase distinct purchase histories or turn ordinary stock into bought supplies. */
TEST_CASE("Native station and vehicle loading retain distinct purchased and ordinary packet purposes", "[surface-supply-purpose-separation]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	SurfaceSupplyRig rig;
	const CargoLabel label{"RFMT"};
	const auto first = PrepareSurfaceSupplyOrder(rig.origin->index, label, 1);
	REQUIRE(PlaceSurfaceSupplyOrder(first, true) == SurfaceSupplyError::None);
	const auto second = PrepareSurfaceSupplyOrder(rig.origin->index, label, 1);
	REQUIRE(PlaceSurfaceSupplyOrder(second, true) == SurfaceSupplyError::None);
	REQUIRE(CargoPacket::CanAllocateItem());
	rig.Queue(label).Append(CargoPacket::Create(rig.origin->index, 1, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid()); // Same ordinary provenance/age/location deliberately makes purchase purpose the meaningful consolidation boundary.
	rig.Load(label, 3);
	std::vector<uint32_t> purposes;
	// Count actual native packet identities after queue consolidation/loading;
	// equal aggregate stock alone cannot prove that refund/payment history survived.
	for (const CargoPacket *packet : *rig.carrier->cargo.Packets()) purposes.push_back(packet->GetCompanySupplyOrder());
	std::ranges::sort(purposes);
	CHECK(purposes == std::vector<uint32_t>{0, first.next_order.base(), second.next_order.base()});
	CHECK(rig.carrier->cargo.TotalCount() == 3);
	CHECK(rig.Queue(label).TotalCount() == 0);
}
