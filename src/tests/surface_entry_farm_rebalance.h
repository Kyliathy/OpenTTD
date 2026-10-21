/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_farm_rebalance.h Paid farm balance retaining every original owned wagon.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGH.
 * Native commands park only empty Grain18..20 and buy real Livestock bodies.
 * Parked capacity provides neither active service nor refund credit.
 */
#ifndef TEST_SURFACE_ENTRY_FARM_REBALANCE_H
#define TEST_SURFACE_ENTRY_FARM_REBALANCE_H

/** Verify the exact active prefix and coupled original Grain reserve.
 * @param purchased Whether exactly three real new Livestock tails must exist.
 * @return Actual original powered head, with all ownership remaining native.
 */
static const Train *VerifySurfaceFarmRebalance(bool purchased)
{
	const auto *train = Train::Get(VehicleID{52}); REQUIRE(train->IsFrontEngine());
	const auto livestock = GetCargoTypeByLabel(CargoLabel{"LVST"}), grain = GetCargoTypeByLabel(CargoLabel{"GRAI"});
	const std::array<uint16_t, 11> prefix{52, 12, 13, 14, 15, 16, 17, 21, 22, 23, 50}; const Train *part = train;
	// Preserve the exact active original models/order. Only the three named
	// surplus Grain originals may leave this prefix for the owned reserve.
	for (const auto id : prefix) {
		REQUIRE(part != nullptr); REQUIRE(part->index == VehicleID{id}); REQUIRE(part->owner == _current_company);
		if (id == 52) { REQUIRE(part->engine_type == EngineID{24}); REQUIRE(part->cargo_cap == 0); }
		else if (id >= 21 && id <= 23) {
			// These three remaining Grain bodies keep the original farm Grain
			// service active while its other Grain capacity stays owned in reserve.
			REQUIRE(part->engine_type == EngineID{33}); REQUIRE(part->cargo_type == grain); REQUIRE(part->cargo_cap == 30);
		} else { REQUIRE(part->engine_type == EngineID{31}); REQUIRE(part->cargo_type == livestock); REQUIRE(part->cargo_cap == 25); }
		REQUIRE(part->gcache.cached_veh_length == 8); part = part->Next();
	}
	// Native links supply the actual new IDs. Exactly three paid plain bodies
	// may extend the known prefix; no original body is replaced or sold.
	for (uint index = 0; index < (purchased ? 3U : 0U); ++index) {
		REQUIRE(part != nullptr); REQUIRE(part->owner == _current_company); REQUIRE(part->engine_type == EngineID{31});
		REQUIRE(part->cargo_type == livestock); REQUIRE(part->cargo_cap == 25); REQUIRE(part->gcache.cached_veh_length == 8); part = part->Next();
	}
	REQUIRE(part == nullptr); REQUIRE(train->gcache.cached_total_length == (purchased ? 112 : 88)); REQUIRE(_settings_game.vehicle.max_train_length == 7);
	const auto *reserve = Train::Get(VehicleID{18}); REQUIRE(reserve->IsFreeWagon()); REQUIRE(reserve->IsStoppedInDepot()); part = reserve;
	// Free wagons have no stopped-status bit. Exact old identities, empty
	// cargo and native whole-chain depot authority define the reserve.
	for (const uint16_t id : {18, 19, 20}) {
		REQUIRE(part != nullptr); REQUIRE(part->index == VehicleID{id}); REQUIRE(part->owner == _current_company);
		REQUIRE(part->tile == TileIndex{1638}); REQUIRE(part->engine_type == EngineID{33}); REQUIRE(part->cargo_type == grain);
		REQUIRE(part->cargo_cap == 30); REQUIRE(part->cargo.TotalCount() == 0); part = part->Next();
	}
	REQUIRE(part == nullptr); REQUIRE(train->GetNumOrders() == 2);
	for (uint8_t index = 0; index < 2; ++index) {
		const auto *order = train->GetOrder(index); REQUIRE(order != nullptr); REQUIRE(order->IsType(OT_GOTO_STATION));
		REQUIRE(order->GetDestination() == DestinationID{static_cast<uint16_t>(index == 0 ? 15 : 13)});
		REQUIRE(order->GetLoadType() == (index == 0 ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad));
		REQUIRE(order->GetUnloadType() == (index == 0 ? OrderUnloadType::NoUnload : OrderUnloadType::UnloadIfPossible));
	}
	return train;
}

/** Recall only the empty original train after fresh whole-budget admission. */
static void RecallSurfaceFarmRebalance()
{
	QuerySurfaceFarmBalance(); const auto *train = VerifySurfaceFarmUpgrade(true);
	QuoteSurfaceFarmUpgrade("balance-recall", SurfaceChemicalWorkLimit(), 3000, Money{3375}, false);
	PaySurfaceResupply<Commands::SendVehicleToDepot>(TileIndex{0}, train->index, DepotCommandFlags{}, VehicleListIdentifier{});
	const auto *depot = Depot::GetByTile(TileIndex{1638}); REQUIRE(depot != nullptr);
	REQUIRE(train->current_order.IsType(OT_GOTO_DEPOT)); REQUIRE(train->current_order.GetDestination().base() == depot->index.base()); VerifySurfaceFarmUpgrade(true);
	fmt::print("SURFACE_RESUPPLY_BALANCE_RECALL head=52 depot=1638 invoice=0 changed_orders=0\n");
}

/** Earn a bounded empty return protecting every unpaid wagon and later service. */
static void WorkSurfaceFarmRebalanceReturn()
{
	const auto *train = VerifySurfaceFarmUpgrade(true); const auto limit = SurfaceChemicalWorkLimit();
	QuoteSurfaceFarmUpgrade("balance-return", limit, 3000, Money{3375}, false);
	/** @return Whether the complete empty consist has halted at its owned depot. */
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, [&] {
		REQUIRE(Company::Get(_current_company)->money >= Money{43375});
		if (!train->IsStoppedInDepot()) return false;
		REQUIRE(train->tile == TileIndex{1638});
		for (const Train *part = train; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
		return true;
	});
	fmt::print("SURFACE_RESUPPLY_BALANCE_RETURN ticks={} returned={} cash={} unpaid=3375 reserve=40000\n", receipt.ticks, receipt.reached, Company::Get(_current_company)->money.base());
}

/** Park only the three empty specified original Grain bodies by native moves. */
static void ParkSurfaceFarmRebalance()
{
	const auto *train = VerifySurfaceFarmUpgrade(true); REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->tile == TileIndex{1638});
	for (const Train *part = train; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
	QuoteSurfaceFarmUpgrade("balance-park", 3000, 0, Money{3375}, true);
	PaySurfaceResupply<Commands::MoveRailVehicle>(TileIndex{1638}, VehicleID{18}, VehicleID::Invalid(), false);
	PaySurfaceResupply<Commands::MoveRailVehicle>(TileIndex{1638}, VehicleID{19}, VehicleID{18}, false);
	PaySurfaceResupply<Commands::MoveRailVehicle>(TileIndex{1638}, VehicleID{20}, VehicleID{19}, false);
	VerifySurfaceFarmRebalance(false);
	fmt::print("SURFACE_RESUPPLY_BALANCE_PARK head=52 reserve_head=18 parked_original=3 original_wagons_owned=32 active_grain=90 parked_grain=90 livestock=175 length=88 invoice=0\n");
}

/** Pay native gross invoices, append actual returned IDs and release service. */
static void PaySurfaceFarmRebalance()
{
	const auto *train = VerifySurfaceFarmRebalance(false); REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->tile == TileIndex{1638});
	for (const Train *part = train; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
	QuoteSurfaceFarmUpgrade("balance-purchase", 3000, 0, Money{3375}, true); REQUIRE(Train::CanAllocateItem(3));
	const auto *company = Company::Get(_current_company); const auto before = company->money;
	const auto livestock = GetCargoTypeByLabel(CargoLabel{"LVST"}); VehicleID tail{50};
	// Immediate native attachment keeps new Livestock separate from the old
	// free Grain chain. Returned IDs, never guessed pool slots, define the tails.
	for (uint index = 0; index < 3; ++index) {
		const auto bought = PaySurfaceResupply<Commands::BuildVehicle>(TileIndex{1638}, TileIndex{1638}, EngineID{31}, false, livestock, ClientID::Server);
		REQUIRE(std::get<0>(bought).GetCost() == Money{1125}); auto *wagon = Train::Get(std::get<1>(bought));
		REQUIRE(wagon->cargo_cap == 25); REQUIRE(wagon->cargo.TotalCount() == 0);
		if (wagon->First() != train) PaySurfaceResupply<Commands::MoveRailVehicle>(TileIndex{1638}, wagon->index, tail, false);
		REQUIRE(Train::Get(tail)->Next() == wagon); tail = wagon->index;
		fmt::print("SURFACE_RESUPPLY_BALANCE_WAGON position={} wagon={} invoice=1125 capacity=25\n", index, tail.base());
	}
	VerifySurfaceFarmRebalance(true); PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, train->index, true);
	REQUIRE(!train->vehstatus.Test(VehState::Stopped)); REQUIRE(company->money == before - Money{3375});
	fmt::print("SURFACE_RESUPPLY_BALANCE_PAID invoice=3375 livestock=250 active_grain=90 parked_grain=90 length=112 original_wagons_owned=32 cash={} changed_orders=0\n", company->money.base());
}

/** Measure each new body's real pickup/delivery and actual cash within the cap. */
static void WorkSurfaceFarmRebalance()
{
	const auto *train = VerifySurfaceFarmRebalance(true); const auto limit = SurfaceChemicalWorkLimit();
	REQUIRE(!train->vehstatus.Test(VehState::Stopped)); QuoteSurfaceFarmUpgrade("balance-service", limit, 0, Money{0}, false);
	const auto *company = Company::Get(_current_company); std::array<const Train *, 3> wagons{};
	std::array<bool, 3> loaded{}, delivered{}; const Train *part = Train::Get(VehicleID{50})->Next();
	for (auto &wagon : wagons) { REQUIRE(part != nullptr); wagon = part; part = part->Next(); }
	/** @return Whether all new bodies delivered and actual working cash recovered. */
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, [&] {
		REQUIRE(company->money >= Money{40000});
		// Measure every real tail separately. A loaded body must subsequently
		// empty at its persistent factory receiver to count as delivered service.
		for (uint index = 0; index < wagons.size(); ++index) {
			if (wagons[index]->cargo.TotalCount() > 0) loaded[index] = true;
			if (loaded[index] && train->last_station_visited == StationID{13} && wagons[index]->cargo.TotalCount() == 0) {
				// Pickup plus receiver-side emptying identifies useful delivered
				// capacity, rather than unused paid capacity or depot emptying.
				delivered[index] = true;
			}
		}
		// Actual cash after paid costs must accompany every delivered new body.
		return std::ranges::find(delivered, false) == delivered.end() && company->money >= Money{100000};
	});
	VerifySurfaceFarmRebalance(true);
	for (uint index = 0; index < wagons.size(); ++index) {
		fmt::print("SURFACE_RESUPPLY_BALANCE_USE wagon={} picked_up={} delivered={} held={}\n", wagons[index]->index.base(), loaded[index], delivered[index], wagons[index]->cargo.TotalCount());
	}
	fmt::print("SURFACE_RESUPPLY_BALANCE_SERVICE ticks={} cash={} restored={} partial={} module_capital_complete=false\n", receipt.ticks, company->money.base(), company->money >= Money{100000}, !receipt.reached);
}
#endif /* TEST_SURFACE_ENTRY_FARM_REBALANCE_H */
