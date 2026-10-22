/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_goods_rail_delivery.h Actual first-source13 Goods rail acceptance and posted payment.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CSGZH/15CSGZI.
 * Original train49 and native packet/payment owners alone perform transport.
 * Whole unloading fares may include other Goods; no cohort-only fare is invented.
 */
#ifndef TEST_SURFACE_ENTRY_GOODS_RAIL_DELIVERY_H
#define TEST_SURFACE_ENTRY_GOODS_RAIL_DELIVERY_H
#include "../economy_base.h"

/** Verify the original whole Goods train and its unchanged pickup/final-receiver policy.
 * @param both_feeders Explicit later second paid road feeder; false retains25direct20service.
 * @return Actual original head49, retaining all twelve original Goods wagons.
 */
static const Train *VerifySurfaceGoodsRailDelivery(bool both_feeders = false)
{
	VerifySurfaceGoodsRailFeederStop(); VerifySurfaceFactoryGoodsOrders(true, both_feeders); VerifySurfaceFarmFeederOrders(); VerifySurfaceFarmRebalance(true); VerifySurfaceIntertown(true);
	const auto *train = Train::Get(VehicleID{49}); REQUIRE(train->IsFrontEngine()); REQUIRE(train->owner == _current_company); REQUIRE(train->engine_type == EngineID{24});
	REQUIRE(!train->vehstatus.Test(VehState::Stopped)); REQUIRE(train->GetNumOrders() == 2);
	// Rail pickup and final delivery retain their actual old destinations.
	// Only the earlier paid road feeder supplied the new first13cohort.
	for (uint8_t index = 0; index < 2; ++index) {
		const auto *order = train->GetOrder(index); REQUIRE(order->IsType(OT_GOTO_STATION));
		REQUIRE(order->GetDestination() == DestinationID{static_cast<uint16_t>(index == 0 ? 12 : 11)});
		REQUIRE(order->GetLoadType() == (index == 0 ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad));
		REQUIRE(order->GetUnloadType() == (index == 0 ? OrderUnloadType::NoUnload : OrderUnloadType::UnloadIfPossible));
	}
	uint parts = 0, capacity = 0;
	// Preserve real coupled capacity instead of crediting a parked wagon,
	// prospective vehicle or shortened train with this delivery opportunity.
	for (const auto *part = train; part != nullptr; part = part->Next()) { ++parts; capacity += part->cargo_cap; }
	REQUIRE(parts == 13); REQUIRE(capacity == 300);
	const auto *receiver = Station::Get(StationID{11}); REQUIRE(receiver->owner == _current_company);
	REQUIRE(receiver->goods[GetCargoTypeByLabel(CargoLabel{"GOOD"})].status.Test(GoodsEntry::State::Acceptance));
	return train;
}

/** Read only actual first13Goods on the original train across native packet splits/merges.
 * @param train Actual verified original head49 and its unchanged consist.
 * @return Whole first13cohort quantity and native virtual feeder share, never cash.
 */
static std::pair<uint, Money> SurfaceGoodsRailDeliveryStock(const Train *train)
{
	uint quantity = 0; Money share{0}; const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	// First station and production identity survive native packet allocation
	// and wagon distribution. Recyclable packet IDs cannot identify a shipment
	// or turn waiting/boarded quantities into another inventory or fare owner.
	for (const auto *part = train; part != nullptr; part = part->Next()) {
		if (part->cargo_cap > 0) REQUIRE(part->cargo_type == goods);
		for (const auto *packet : *part->cargo.Packets()) {
			if (packet->GetFirstStation() != StationID{13}) continue;
			REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE((packet->GetSource().id == 5 || packet->GetSource().id == 11));
			quantity += packet->Count(); share += packet->GetFeederShare();
		}
	}
	return {quantity, share};
}

/** Earn one freshly funded original rail delivery or explicitly qualified pending-payment interval.
 * @param ceiling Separately admitted maximum, at most2000ticks and never a time grant.
 * @param settlement Explicit later continuation of a real preceding accepted-cohort/pending-payment receipt; historical/default transport mode remains false.
 * @param stop_on_payment Historical/default first-payment completion; false only in the separately bounded actual cash-goal reserve consumer.
 * @param both_feeders Explicit later25transfer12consumer; false preserves every preceding route contract.
 */
static void WorkSurfaceGoodsRailDelivery(uint ceiling, bool settlement = false, bool stop_on_payment = true, bool both_feeders = false)
{
	const auto *train = VerifySurfaceGoodsRailDelivery(both_feeders); const auto *company = Company::Get(_current_company);
	const auto initial = SurfaceGoodsRailDeliveryStock(train); const auto year = TimerGameEconomy::year; const auto profit = train->profit_this_year;
	Money observed_fare{0};
	// Only the explicit settlement consumer may start with a positive real
	// payment after earlier acceptance. Ordinary transport cannot treat an
	// unrelated old unloading payment as evidence for the new feeder cohort.
	if (settlement) {
		REQUIRE(initial.first == 0); REQUIRE(train->last_station_visited == StationID{11}); REQUIRE(train->cargo_payment != nullptr);
		REQUIRE(train->cargo_payment->route_profit > Money{0}); observed_fare = train->cargo_payment->route_profit;
	}
	const uint limit = QuoteSurfaceGoodsRailFeederWork(Money{0}, ceiling);
	fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_DELIVERY_SELECTION ceiling={} found={} ticks={} settlement={} initial={}\n", ceiling, limit > 0, limit, settlement, initial.first);
	if (limit == 0) return;
	uint previous = initial.first, boarded = 0, accepted = 0; bool posted = false;
	/** @return Whether the explicitly selected native payment/cash completion predicate passed. */
	const auto done = [&] {
		REQUIRE(company->money >= Money{40000}); const auto current = SurfaceGoodsRailDeliveryStock(train).first;
		if (current > previous) boarded += current - previous;
		// The unchanged NoLoad/UnloadIfPossible receiver can lose this
		// cohort only through actual accepting11delivery. Old first12Goods
		// remain outside its quantity receipt even when unloaded alongside it.
		if (current < previous) {
			REQUIRE(train->last_station_visited == StationID{11});
			REQUIRE(Station::Get(StationID{11})->goods[GetCargoTypeByLabel(CargoLabel{"GOOD"})].status.Test(GoodsEntry::State::Acceptance));
			accepted += previous - current;
		}
		previous = current;
		// CargoPayment alone posts cash after unloading. Its observed whole
		// route fare can include first12Goods, so this proves settlement of
		// the actual unload while claiming no feeder-only monetary amount.
		if ((accepted > 0 || settlement) && train->cargo_payment != nullptr) {
			REQUIRE(train->last_station_visited == StationID{11});
			observed_fare = std::max(observed_fare, train->cargo_payment->route_profit);
		}
		posted = observed_fare > Money{0} && current == 0 && train->cargo_payment == nullptr;
		// The separate reserve consumer continues normal useful services
		// until actual company cash reaches its goal. A single posted fare
		// ends only the historical first-payment delivery mode.
		return company->money >= Money{100000} || (stop_on_payment && posted);
	};
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, done); const auto final = SurfaceGoodsRailDeliveryStock(train);
	REQUIRE(initial.first + boarded == accepted + final.first); VerifySurfaceGoodsRailDelivery(both_feeders);
	auto delta = train->profit_this_year - profit;
	if (TimerGameEconomy::year != year) delta = train->profit_last_year - profit + train->profit_this_year;
	fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_DELIVERY_SERVICE ticks={} initial={} boarded={} accepted={} aboard={} share={} profit_delta_256={} observed_whole_unload_fare={} payment_pending={} payment_posted={} settlement={} cash={} restored={} cohort_only_fare_claimed=false feeder_credit_is_cash=false annual_projection=false stop_on_payment={}\n",
		work.ticks, initial.first, boarded, accepted, final.first, final.second.base(), delta.base(), observed_fare.base(), train->cargo_payment != nullptr, posted, settlement,
		company->money.base(), company->money >= Money{100000}, stop_on_payment);
}
#endif /* TEST_SURFACE_ENTRY_GOODS_RAIL_DELIVERY_H */
