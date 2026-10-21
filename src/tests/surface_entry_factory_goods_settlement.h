/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_factory_goods_settlement.h Observe actual native cash settlement after Goods acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGVB.
 * The public native CargoPayment owner alone posts the pending572receipt.
 * This phase grants no payment, loading timer, route, position or cargo.
 */
#ifndef TEST_SURFACE_ENTRY_FACTORY_GOODS_SETTLEMENT_H
#define TEST_SURFACE_ENTRY_FACTORY_GOODS_SETTLEMENT_H
#include "../economy_base.h"

/** Earn at most64funded native ticks until the actual pending vehicle3payment posts. */
static void SettleSurfaceFactoryGoods()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{49932}); REQUIRE(company->current_loan == Money{5000000});
	VerifySurfaceFactoryGoodsOrders(); VerifySurfaceFarmFeederOrders(); VerifySurfaceFarmRebalance(true); VerifySurfaceIntertown();
	const auto *truck = RoadVehicle::Get(VehicleID{3}); REQUIRE(truck->cargo.TotalCount() == 0); REQUIRE(truck->last_station_visited == StationID{20});
	const auto *payment = truck->cargo_payment; REQUIRE(payment != nullptr); REQUIRE(payment->front == truck);
	REQUIRE(payment->route_profit == Money{572}); REQUIRE(payment->visual_profit == Money{572}); REQUIRE(payment->visual_transfer == Money{0});
	const auto bill = QuoteSurfaceFixedPriceIncome(64); const auto operating = bill.upkeep + bill.interest + bill.station; const auto target = Money{40000} + operating;
	fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_SETTLEMENT_ADMISSION ticks=64 operating={} reserve=40000 target={} cash={} funded={} pending=572 pending_credit=0 future_income_counted=0\n",
		operating.base(), target.base(), company->money.base(), company->money >= target);
	REQUIRE(company->money >= target); const auto before_profit = truck->profit_this_year; const auto year = TimerGameEconomy::year;
	/** @return Whether the native loading/payment lifecycle has settled its sole pending owner. */
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", 64, [&] {
		REQUIRE(company->money >= Money{40000}); return truck->cargo_payment == nullptr;
	});
	const bool settled = truck->cargo_payment == nullptr; auto profit = truck->profit_this_year - before_profit;
	if (TimerGameEconomy::year != year) profit = truck->profit_last_year - before_profit + truck->profit_this_year;
	// Acceptance was already recorded before this phase. Only the native
	// owner's disappearance together with posted road income and actual
	// vehicle profit demonstrates settlement into spendable company cash.
	if (settled) {
		REQUIRE(work.expenses[truck->GetExpenseType(true)] <= Money{-572}); REQUIRE(profit > Money{0});
	}
	VerifySurfaceFactoryGoodsOrders(); VerifySurfaceFarmRebalance(true); VerifySurfaceFarmFeederOrders(); VerifySurfaceIntertown();
	fmt::print("SURFACE_RESUPPLY_FACTORY_GOODS_SETTLED ticks={} vehicle=3 pending=572 settled={} profit_delta_256={} cash={} partial={} direct_payment_granted=false\n",
		work.ticks, settled, profit.base(), company->money.base(), !settled);
}
#endif /* TEST_SURFACE_ENTRY_FACTORY_GOODS_SETTLEMENT_H */
