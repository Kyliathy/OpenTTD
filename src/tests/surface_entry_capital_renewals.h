/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_capital_renewals.h Genuine empty return and paid renewal of four original services.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CQ.
 * Reuses delivered native NoLoad/append-home/buy-before-sale order flow. No
 * passenger, freight, age, position, cash or native work is supplied by a test.
 */
#ifndef TEST_SURFACE_ENTRY_CAPITAL_RENEWALS_H
#define TEST_SURFACE_ENTRY_CAPITAL_RENEWALS_H

/** @return The four still-owned original near-due useful service bodies, in stable name order. */
static std::vector<const RoadVehicle *> SurfaceCapitalRenewalVehicles()
{
	std::vector<const RoadVehicle *> result;
	// Explicit role names preserve original services through pool-slot reuse;
	// the additional local bus and every material reserve remain independent.
	for (const auto &name : {"Surface Regional passenger 1", "Surface Regional passenger 2", "Surface Regional passenger 3", "Surface Steel delivery"}) {
		const RoadVehicle *found = nullptr;
		for (const auto *vehicle : RoadVehicle::Iterate()) {
			if (vehicle->name != name) continue;
			REQUIRE(found == nullptr); found = vehicle;
		}
		REQUIRE(found != nullptr); REQUIRE(found->owner == _current_company); REQUIRE(found->age < found->max_age);
		result.push_back(found);
	}
	return result;
}

/** @param vehicle Actual regional passenger or Steel body. @return Its real originally paid depot. */
static TileIndex SurfaceCapitalRenewalDepot(const RoadVehicle &vehicle)
{
	return vehicle.cargo_type == GetCargoTypeByLabel(CargoLabel{"PASS"}) ? TileIndex{2984} : TileIndex{2443};
}

/** Verify original receivers and canonical loading without changing orders.
 * @param vehicle Actual original body or a paid replacement with its copied real route.
 * @param recalling Whether native new loading is disabled for empty return.
 */
static void VerifySurfaceCapitalRenewalOrders(const RoadVehicle &vehicle, bool recalling)
{
	const bool passenger = vehicle.cargo_type == GetCargoTypeByLabel(CargoLabel{"PASS"});
	// Both original passenger receivers retain ordinary unload; Steel keeps its
	// no-unload factory pickup and actual native receiving station2.
	for (uint8_t index = 0; index < 2; ++index) {
		const auto *order = vehicle.GetOrder(index); REQUIRE(order != nullptr); REQUIRE(order->IsType(OT_GOTO_STATION));
		const uint16_t destination = passenger ? (index == 0 ? 17 : 20) : (index == 0 ? 1 : 2);
		const auto normal = passenger ? OrderLoadType::LoadIfPossible : (index == 0 ? OrderLoadType::FullLoad : OrderLoadType::NoLoad);
		REQUIRE(order->GetDestination() == DestinationID{destination});
		REQUIRE(order->GetLoadType() == (recalling ? OrderLoadType::NoLoad : normal));
		REQUIRE(order->GetUnloadType() == (!passenger && index == 0 ? OrderUnloadType::NoUnload : OrderUnloadType::UnloadIfPossible));
		REQUIRE(order->GetStopLocation() == OrderStopLocation::FarEnd);
	}
}

/** @param work Whether actual return movement remains unpaid. @return Entire cash-backed gross renewal and native return bill. */
static Money QuoteSurfaceCapitalRenewalProtection(bool work)
{
	const auto gross = QuoteSurfaceCapitalRenewals(); Money operating{0};
	if (work) {
		const auto forecast = QuoteSurfaceModuleCapitalWork(0, 3000); REQUIRE(!forecast.year_boundary);
		operating = forecast.upkeep + forecast.interest + forecast.station;
	}
	const auto *company = Company::Get(_current_company); const auto target = Money{40000} + gross + operating;
	const bool funded = company->money >= target;
	fmt::print("SURFACE_RESUPPLY_RENEWAL_ADMISSION gross={} operating={} reserve=40000 target={} cash={} funded={} ticks={} refunds_counted=0 projected_fares=0 credit=0\n",
		gross.base(), operating.base(), target.base(), company->money.base(), funded, work ? 3000 : 0);
	REQUIRE(funded); REQUIRE(company->current_loan == Money{5000000}); return target;
}

/** Quote all real empty-return orders and the complete second bounded startup interval. */
static void QuerySurfaceCapitalRenewalReturn()
{
	QuoteSurfaceCapitalRenewalProtection(true);
	for (const auto *vehicle : SurfaceCapitalRenewalVehicles()) {
		REQUIRE(vehicle->GetNumOrders() == 2); VerifySurfaceCapitalRenewalOrders(*vehicle, false);
		for (uint8_t index = 0; index < 2; ++index) {
			// The Steel receiver already forbids loading. Native ModifyOrder
			// correctly refuses an unchanged value, so query only real changes.
			if (vehicle->GetOrder(index)->GetLoadType() == OrderLoadType::NoLoad) continue;
			const auto quote = QuoteSurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, vehicle->index, VehicleOrderID{index}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::NoLoad));
			REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == 0);
		}
		const auto tile = SurfaceCapitalRenewalDepot(*vehicle); const auto *depot = Depot::GetByTile(tile); REQUIRE(depot != nullptr);
		Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		const auto quote = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, vehicle->index, VehicleOrderID{2}, home);
		REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == 0);
		fmt::print("SURFACE_RESUPPLY_RENEWAL_RECALL vehicle={} depot={} held={} capacity={} orders=2 empty_required=true\n",
			vehicle->index.base(), tile.base(), vehicle->cargo.TotalCount(), vehicle->cargo_cap);
	}
}

/** Suppress future pickups while retaining actual delivery receivers and a real depot halt. */
static void RecallSurfaceCapitalRenewals()
{
	QuerySurfaceCapitalRenewalReturn();
	for (const auto *vehicle : SurfaceCapitalRenewalVehicles()) {
		for (uint8_t index = 0; index < 2; ++index) {
			if (vehicle->GetOrder(index)->GetLoadType() == OrderLoadType::NoLoad) continue;
			PaySurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, vehicle->index, VehicleOrderID{index}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::NoLoad));
		}
		const auto *depot = Depot::GetByTile(SurfaceCapitalRenewalDepot(*vehicle)); REQUIRE(depot != nullptr);
		Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, vehicle->index, VehicleOrderID{2}, home);
		VerifySurfaceCapitalRenewalOrders(*vehicle, true); REQUIRE(vehicle->GetNumOrders() == 3);
	}
}

/** @return Whether all four real bodies returned empty to their owned depots, after useful delivery. */
static bool SurfaceCapitalRenewalsEmpty()
{
	for (const auto *vehicle : SurfaceCapitalRenewalVehicles()) {
		// Physical depot halt and empty custody jointly authorize sale; neither
		// an order nor an exhausted timeout can stand in for delivered passengers.
		if (!vehicle->IsStoppedInDepot() || vehicle->tile != SurfaceCapitalRenewalDepot(*vehicle) || vehicle->cargo.TotalCount() != 0) return false;
	}
	return true;
}

/** Earn at most the second already priced3000tick interval, preserving any real partial return. */
static void WorkSurfaceCapitalRenewals()
{
	QuoteSurfaceCapitalRenewalProtection(true); REQUIRE(SurfaceChemicalWorkLimit() == 3000);
	for (const auto *vehicle : SurfaceCapitalRenewalVehicles()) {
		REQUIRE(vehicle->GetNumOrders() == 3); VerifySurfaceCapitalRenewalOrders(*vehicle, true);
	}
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", 3000, SurfaceCapitalRenewalsEmpty);
	REQUIRE(Company::Get(_current_company)->money >= Money{40000} + QuoteSurfaceCapitalRenewals());
	fmt::print("SURFACE_RESUPPLY_RENEWAL_RETURN ticks={} empty_home={} partial={} cash={} pending_gross=25633 reserve=40000\n",
		receipt.ticks, SurfaceCapitalRenewalsEmpty(), !SurfaceCapitalRenewalsEmpty(), Company::Get(_current_company)->money.base());
}

/** Pay matching replacements before selling each genuinely empty original and restore its useful role. */
static void PaySurfaceCapitalRenewals()
{
	REQUIRE(SurfaceCapitalRenewalsEmpty()); QuoteSurfaceCapitalRenewalProtection(false);
	const auto original = SurfaceCapitalRenewalVehicles(); const auto *company = Company::Get(_current_company);
	const auto initial = company->money; Money gross{0}, refunds{0};
	for (const auto *old : original) {
		REQUIRE(old->GetNumOrders() == 3); VerifySurfaceCapitalRenewalOrders(*old, true);
		const auto name = old->name; const auto id = old->index; const auto tile = SurfaceCapitalRenewalDepot(*old);
		const auto bought = PaySurfaceResupply<Commands::BuildVehicle>(tile, tile, old->engine_type, false, old->cargo_type, ClientID::Server);
		gross += std::get<0>(bought).GetCost(); const auto *vehicle = RoadVehicle::Get(std::get<1>(bought));
		REQUIRE(vehicle->IsStoppedInDepot()); REQUIRE(vehicle->cargo.TotalCount() == 0); REQUIRE(vehicle->cargo_cap == old->cargo_cap);
		PaySurfaceResupply<Commands::CloneOrder>(TileIndex{0}, CO_COPY, vehicle->index, id);
		PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, vehicle->index, VehicleOrderID{2});
		const bool passenger = vehicle->cargo_type == GetCargoTypeByLabel(CargoLabel{"PASS"});
		for (uint8_t index = 0; index < 2; ++index) {
			const auto load = passenger ? OrderLoadType::LoadIfPossible : (index == 0 ? OrderLoadType::FullLoad : OrderLoadType::NoLoad);
			if (vehicle->GetOrder(index)->GetLoadType() == load) continue;
			PaySurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, vehicle->index, VehicleOrderID{index}, MOF_LOAD, static_cast<uint16_t>(load));
		}
		VerifySurfaceCapitalRenewalOrders(*vehicle, false); REQUIRE(vehicle->GetNumOrders() == 2);
		const auto sold = PaySurfaceResupply<Commands::SellVehicle>(TileIndex{0}, id, false, false, ClientID::Server);
		REQUIRE(sold.GetCost() <= 0); refunds -= sold.GetCost();
		PaySurfaceResupply<Commands::RenameVehicle>(TileIndex{0}, vehicle->index, name);
		PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, vehicle->index, true);
		fmt::print("SURFACE_RESUPPLY_RENEWAL_PAID old={} vehicle={} engine={} capacity={} gross={} refund={} age={} empty=true orders=2 depot={}\n",
			id.base(), vehicle->index.base(), vehicle->engine_type.base(), vehicle->cargo_cap, std::get<0>(bought).GetCost().base(), -sold.GetCost().base(), vehicle->age.base(), tile.base());
	}
	REQUIRE(gross == Money{25633}); REQUIRE(company->money == initial - gross + refunds);
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->money >= Money{40000});
	fmt::print("SURFACE_RESUPPLY_RENEWAL_SETTLED gross={} refunds={} cash_before={} cash={} debt=5000000 bodies=4 reserve=40000 future_fares=0\n",
		gross.base(), refunds.base(), initial.base(), company->money.base());
}
#endif /* TEST_SURFACE_ENTRY_CAPITAL_RENEWALS_H */
