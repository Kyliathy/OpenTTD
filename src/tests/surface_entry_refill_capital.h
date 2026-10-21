/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_refill_capital.h Pure native redundant empty fuel-carrier capital admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CSGZE/15CSGZF.
 * Actual paid39has completed its supply duty; identical2retains fuel capacity.
 * Only a legal public sale estimate is proposed. Refunds remain unbanked here.
 */
#ifndef TEST_SURFACE_ENTRY_REFILL_CAPITAL_H
#define TEST_SURFACE_ENTRY_REFILL_CAPITAL_H

/** Verify only the redundant empty refill and its retained same-capacity fuel alternative.
 * @return Actual paid39, still present, empty and stopped in its original owned depot.
 */
static const RoadVehicle *VerifySurfaceRedundantRefill()
{
	const auto fuel = GetCargoTypeByLabel(CargoLabel{"PROP"});
	REQUIRE(IsRoadDepotTile(TileIndex{2443})); REQUIRE(GetTileOwner(TileIndex{2443}) == _current_company);
	// Both actual existing bodies share the same real fuel capability.
	// Retiring39would leave2available for later procurement; no extra body,
	// refit, free cargo or replacement order is assumed by this comparison.
	for (const auto id : {VehicleID{2}, VehicleID{39}}) {
		const auto *truck = RoadVehicle::Get(id); REQUIRE(truck->owner == _current_company); REQUIRE(truck->engine_type == EngineID{134});
		REQUIRE(truck->cargo_type == fuel); REQUIRE(truck->cargo_cap == 27); REQUIRE(truck->cargo.TotalCount() == 0);
		REQUIRE(truck->tile == TileIndex{2443}); REQUIRE(truck->IsStoppedInDepot()); REQUIRE(truck->cargo_payment == nullptr);
		REQUIRE(truck->name == (id == VehicleID{2} ? "Surface Propellant reserve" : "Surface Propellant refill"));
	}
	const auto *chemical = RoadVehicle::Get(VehicleID{55}); REQUIRE(chemical->owner == _current_company); REQUIRE(chemical->cargo_type == GetCargoTypeByLabel(CargoLabel{"CHEM"}));
	REQUIRE(chemical->cargo_cap == 27); REQUIRE(chemical->cargo.TotalCount() == 0); REQUIRE(chemical->IsStoppedInDepot());
	VerifySurfaceGoodsRailFeederStop(); VerifySurfaceFactoryGoodsOrders(true); VerifySurfaceFarmFeederOrders(); VerifySurfaceFarmRebalance(true); VerifySurfaceIntertown(true);
	return RoadVehicle::Get(VehicleID{39});
}

/** Quote legal recovery of only redundant39, granting neither money nor operating permission. */
static void QuerySurfaceRefillCapital()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{47627}); REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	const auto *truck = VerifySurfaceRedundantRefill();
	const auto sale = QuoteSurfaceResupply<Commands::SellVehicle>(TileIndex{2443}, truck->index, false, false, ClientID::Server);
	if (sale.Succeeded()) REQUIRE(sale.GetCost() == -truck->value);
	const auto refund = sale.Succeeded() ? -sale.GetCost() : Money{0};
	fmt::print("SURFACE_RESUPPLY_REFILL_CAPITAL_SOURCE vehicle=39 retained=2 engine=134 capacity=27 held=0 depot=2443 same_role_retained=true chemical_retained=55 value={} sale_available={} sale_invoice={} refund={} error={} paid=0 ticks=0 work_permission=false\n",
		truck->value.base(), sale.Succeeded(), sale.GetCost().base(), refund.base(), sale.GetErrorMessage().base());
	// This conditional balance is deliberately separate from actual money.
	// A later native sale must finish and bank its real refund before another
	// complete operating interval can pass admission; no estimate pays a bill.
	for (const uint limit : {2000, 1000, 500, 250, 64}) {
		const auto bill = QuoteSurfaceFixedPriceIncome(limit); const auto operating = bill.upkeep + bill.interest + bill.station;
		const auto target = Money{40000} + operating; const auto conditional = company->money + refund;
		fmt::print("SURFACE_RESUPPLY_REFILL_CAPITAL_ADMISSION ticks={} operating={} reserve=40000 target={} cash={} current_funded={} refund={} conditional_after_actual_sale={} funded_only_if_banked={} future_refund_counted_as_cash=0 work_permission=false credit=0\n",
			limit, operating.base(), target.base(), company->money.base(), company->money >= target, refund.base(), conditional.base(), conditional >= target);
	}
}
/** Retire only the admitted empty duplicate and price work only after its actual refund is banked. */
static void SellSurfaceRefillCapital()
{
	QuerySurfaceRefillCapital(); const auto *company = Company::Get(_current_company); const auto cash = company->money;
	const auto *retained = RoadVehicle::Get(VehicleID{2}), *chemical = RoadVehicle::Get(VehicleID{55});
	REQUIRE(retained->max_age.base() - retained->age.base() > 500);
	PaySurfaceResupply<Commands::SellVehicle>(TileIndex{2443}, VehicleID{39}, false, false, ClientID::Server);
	REQUIRE(!Vehicle::IsValidID(VehicleID{39})); REQUIRE(RoadVehicle::Get(VehicleID{2}) == retained); REQUIRE(RoadVehicle::Get(VehicleID{55}) == chemical);
	REQUIRE(retained->cargo.TotalCount() == 0); REQUIRE(chemical->cargo.TotalCount() == 0); REQUIRE(company->money == cash + Money{2120});
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	// Only the completed command's actual company balance can fund this
	// admission. No simulation tick, virtual transfer credit or anticipated
	// refund contributes to the whole current operating bill and cash floor.
	const auto cap = QuoteSurfaceGoodsRailFeederWork(Money{0}, 500); REQUIRE(cap == 500);
	fmt::print("SURFACE_RESUPPLY_REFILL_CAPITAL_SOLD vehicle=39 invoice=-2120 refund=2120 cash={} retained=2 retained_capacity=27 chemical_retained=55 funded_cap={} elapsed=0 earned=0 future_refund_counted_as_cash=0 credit=0\n",
		company->money.base(), cap);
}
#endif /* TEST_SURFACE_ENTRY_REFILL_CAPITAL_H */
