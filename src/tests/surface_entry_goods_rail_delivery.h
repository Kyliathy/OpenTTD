/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_goods_rail_delivery.h Actual first-source13 Goods rail acceptance and posted payment.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CSGZH/15CSGZI/15CSGZM/15CSGZN.
 * Original train49 and native packet/payment owners alone perform transport.
 * Whole unloading fares may include other Goods; no cohort-only fare is invented.
 */
#ifndef TEST_SURFACE_ENTRY_GOODS_RAIL_DELIVERY_H
#define TEST_SURFACE_ENTRY_GOODS_RAIL_DELIVERY_H
#include "../economy_base.h"
#include <limits>

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
 * @param reserve Historical40000startup floor;30000onlyfor the separately bound first-payment joint-feeder consumer.
 */
static void WorkSurfaceGoodsRailDelivery(uint ceiling, bool settlement = false, bool stop_on_payment = true, bool both_feeders = false, Money reserve = Money{40000})
{
	// The priced lower planning buffer belongs only to the real joint
	// feeder's bounded first-payment scope. Every historical consumer and
	// general cash-goal measurement retains its40000default requirement.
	REQUIRE((reserve == Money{40000} || (reserve == Money{30000} && both_feeders && stop_on_payment)));
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
	const uint limit = QuoteSurfaceGoodsRailFeederWork(Money{0}, ceiling, reserve);
	fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_DELIVERY_SELECTION ceiling={} found={} ticks={} settlement={} initial={}\n", ceiling, limit > 0, limit, settlement, initial.first);
	if (limit == 0) return;
	uint previous = initial.first, boarded = 0, accepted = 0; bool posted = false;
	/** @return Whether the explicitly selected native payment/cash completion predicate passed. */
	const auto done = [&] {
		REQUIRE(company->money >= reserve); const auto current = SurfaceGoodsRailDeliveryStock(train).first;
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
/** Purely compare complete short transport bills under existing and candidate startup reserves. */
static void QuerySurfaceJointGoodsSettlementBudget()
{
	const auto *train = VerifySurfaceGoodsRailDelivery(true); const auto *company = Company::Get(_current_company);
	REQUIRE(company->money == Money{44172}); REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	const auto stock = SurfaceGoodsRailDeliveryStock(train); REQUIRE(stock.first == 179); REQUIRE(stock.second == Money{294});
	REQUIRE(train->last_station_visited == StationID{12}); REQUIRE(train->cargo_payment == nullptr);
	int minimum = std::numeric_limits<int>::max(); uint active = 0;
	const int lead = company->settings.engine_renew ? std::max(0, -int{company->settings.engine_renew_months} * 30) : 0;
	// Only actual active primary ground services can create near-term renewal
	// obligations. Stopped reserves and individual wagons grant no extra life
	// or capacity; the full operating quote still includes orbital upkeep.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->owner != _current_company) continue;
		if (!vehicle->IsPrimaryVehicle()) continue;
		if (vehicle->type != VehicleType::Train && vehicle->type != VehicleType::Road) continue;
		if (vehicle->vehstatus.Test(VehState::Stopped)) continue;
		++active; minimum = std::min(minimum, vehicle->max_age.base() - vehicle->age.base());
	}
	REQUIRE(active == 15); REQUIRE(minimum > 365 + lead);
	fmt::print("SURFACE_RESUPPLY_JOINT_SETTLEMENT_BUDGET_SOURCE rail=49 first13=179 virtual_share=294 last_station=12 receiver=11 payment_pending=false active_ground={} minimum_remaining={} renewal_lead={} near_due_next_year=0 cash={} working_goal=100000 module_reserve=150000 incomplete_module_base=510000 paid=0 elapsed=0 work_permission=false\n",
		active, minimum, lead, company->money.base());
	// This comparison changes no company setting or balance. A candidate
	// reserve may be adopted only by a separately bounded first-payment
	// consumer after this entire current bill is proved affordable.
	for (const uint limit : {2000, 1000, 500, 250, 64}) {
		const auto bill = QuoteSurfaceFixedPriceIncome(limit); const auto operating = bill.upkeep + bill.interest + bill.station;
		const auto historical = operating + Money{40000}, candidate = operating + Money{30000};
		fmt::print("SURFACE_RESUPPLY_JOINT_SETTLEMENT_BUDGET_ADMISSION ticks={} days={} months={} operating={} historical_reserve=40000 historical_target={} historical_funded={} candidate_reserve=30000 candidate_target={} candidate_funded={} cash={} future_income_counted=0 resale_counted=0 credit=0 work_permission=false\n",
			limit, bill.days, bill.months, operating.base(), historical.base(), company->money >= historical, candidate.base(), company->money >= candidate, company->money.base());
	}
}
#endif /* TEST_SURFACE_ENTRY_GOODS_RAIL_DELIVERY_H */
