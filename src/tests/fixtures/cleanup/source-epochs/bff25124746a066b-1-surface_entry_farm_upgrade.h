/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_farm_upgrade.h Actual empty recall and single paid farm wagon within native limits.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGF.
 * Ordinary commands own recall, the new ID, coupling and service. The original
 * twelve wagons, two orders, Grain role and all launch materials remain useful.
 */
#ifndef TEST_SURFACE_ENTRY_FARM_UPGRADE_H
#define TEST_SURFACE_ENTRY_FARM_UPGRADE_H

/** Verify the immutable original Farm bodies/orders and the optional actual paid tail.
 * @param upgraded Whether exactly one real31Livestock25tail must already exist.
 * @return Original front engine; native commands alone may modify its consist.
 */
static const Train *VerifySurfaceFarmUpgrade(bool upgraded)
{
	const auto *train = Train::Get(VehicleID{52}); const auto livestock = GetCargoTypeByLabel(CargoLabel{"LVST"});
	const auto grain = GetCargoTypeByLabel(CargoLabel{"GRAI"}); REQUIRE(train->IsFrontEngine()); REQUIRE(train->owner == _current_company);
	const std::array<VehicleID, 13> original{VehicleID{52}, VehicleID{12}, VehicleID{13}, VehicleID{14}, VehicleID{15}, VehicleID{16}, VehicleID{17},
		VehicleID{18}, VehicleID{19}, VehicleID{20}, VehicleID{21}, VehicleID{22}, VehicleID{23}};
	const Train *part = train;
	// Every original paid body retains its position, model and cargo capacity.
	// The sole new native body may appear only after the entire original prefix.
	for (uint index = 0; index < original.size(); ++index) {
		REQUIRE(part != nullptr); REQUIRE(part->index == original[index]); REQUIRE(part->owner == _current_company);
		if (index == 0) { REQUIRE(part->engine_type == EngineID{24}); REQUIRE(part->cargo_cap == 0); }
		else if (index <= 6) { REQUIRE(part->engine_type == EngineID{31}); REQUIRE(part->cargo_type == livestock); REQUIRE(part->cargo_cap == 25); }
		else { REQUIRE(part->engine_type == EngineID{33}); REQUIRE(part->cargo_type == grain); REQUIRE(part->cargo_cap == 30); }
		part = part->Next();
	}
	if (upgraded) {
		REQUIRE(part != nullptr); REQUIRE(part->Next() == nullptr); REQUIRE(part->engine_type == EngineID{31});
		REQUIRE(part->owner == _current_company); REQUIRE(part->cargo_type == livestock); REQUIRE(part->cargo_cap == 25);
		REQUIRE(part->gcache.cached_veh_length == 8);
	} else { REQUIRE(part == nullptr); }
	REQUIRE(train->gcache.cached_total_length == (upgraded ? 112 : 104)); REQUIRE(_settings_game.vehicle.max_train_length == 7);
	REQUIRE(train->GetNumOrders() == 2);
	for (uint8_t index = 0; index < 2; ++index) {
		const auto *order = train->GetOrder(index); REQUIRE(order != nullptr); REQUIRE(order->IsType(OT_GOTO_STATION));
		REQUIRE(order->GetDestination() == DestinationID{static_cast<uint16_t>(index == 0 ? 15 : 13)});
		REQUIRE(order->GetLoadType() == (index == 0 ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad));
		REQUIRE(order->GetUnloadType() == OrderUnloadType::UnloadIfPossible);
	}
	return train;
}

/** Quote all actual current costs and any still unpaid wagon/future service.
 * @param stage Explicit return, purchase or service diagnostic boundary.
 * @param limit First actual proposed interval in1..3000ticks.
 * @param future Further fully protected service, at most3000ticks.
 * @param unpaid Gross still-unpaid native wagon invoice, never a refund credit.
 * @param restarting Include the stopped original head's future/pending running cost before native release.
 */
static void QuoteSurfaceFarmUpgrade(std::string_view stage, uint limit, uint future, Money unpaid, bool restarting)
{
	REQUIRE(limit > 0); REQUIRE(limit <= 3000); REQUIRE(future <= 3000);
	QuoteSurfaceFixedPriceIncome(limit); auto work = QuoteSurfaceModuleCapitalWork(0, limit + future);
	const auto *company = Company::Get(_current_company); Money restart{0};
	if (restarting) {
		const auto *train = Train::Get(VehicleID{52}); REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->GetEngine()->GetGRF() == nullptr);
		const int lead = company->settings.engine_renew ? std::max(0, -int{company->settings.engine_renew_months} * 30) : 0;
		REQUIRE(train->max_age.base() - train->age.base() > static_cast<int>(work.days) + lead);
		const auto annual = train->GetRunningCost().base(); REQUIRE(annual >= 0);
		constexpr uint64_t denominator = uint64_t{CalendarTime::DAYS_IN_YEAR} * Ticks::DAY_TICKS * 256;
		// The generic quote excludes stopped ground vehicles. This head will
		// actually restart, so its entire future plus pending debit remains due.
		restart = Money{static_cast<int64_t>((static_cast<uint64_t>(annual) * (limit + future + train->running_ticks) + denominator - 1) / denominator) + 1};
	}
	const auto operating = work.upkeep + work.interest + work.station + restart; const auto target = Money{40000} + unpaid + operating;
	fmt::print("SURFACE_RESUPPLY_FARM_UPGRADE_ADMISSION stage={} ticks={} future_ticks={} total_ticks={} operating={} restart={} unpaid={} reserve=40000 target={} cash={} funded={} future_income_counted=0 refunds_counted=0 credit=0\n",
		stage, limit, future, limit + future, operating.base(), restart.base(), unpaid.base(), target.base(), company->money.base(), company->money >= target);
	REQUIRE(company->money >= target); REQUIRE(company->current_loan == Money{5000000});
}

/** Send only the actual empty original Farm train to its native owned depot for a halted return. */
static void RecallSurfaceFarmUpgrade()
{
	QuerySurfaceFarmCapacity(); const auto *train = VerifySurfaceFarmUpgrade(false);
	for (const Train *part = train; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
	PaySurfaceResupply<Commands::SendVehicleToDepot>(TileIndex{0}, train->index, DepotCommandFlags{}, VehicleListIdentifier{});
	REQUIRE(train->current_order.IsType(OT_GOTO_DEPOT)); const auto *depot = Depot::GetByTile(TileIndex{1638}); REQUIRE(depot != nullptr);
	REQUIRE(train->current_order.GetDestination().base() == depot->index.base()); VerifySurfaceFarmUpgrade(false);
	fmt::print("SURFACE_RESUPPLY_FARM_RECALL head=52 depot=1638 invoice=0 changed_persistent_orders=0 cargo=0\n");
}

/** Earn only the bounded empty return while protecting the unpaid body and full later service. */
static void WorkSurfaceFarmReturn()
{
	const auto *train = VerifySurfaceFarmUpgrade(false); const auto limit = SurfaceChemicalWorkLimit();
	QuoteSurfaceFarmUpgrade("return", limit, 3000, Money{1125}, false);
	/** @return Whether the entire original empty train is actually halted in the owned depot. */
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, [&] {
		REQUIRE(Company::Get(_current_company)->money >= Money{41125});
		if (!train->IsStoppedInDepot()) return false;
		REQUIRE(train->tile == TileIndex{1638});
		for (const Train *part = train; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
		return true;
	});
	fmt::print("SURFACE_RESUPPLY_FARM_RETURN ticks={} head=52 depot=1638 returned={} cash={} unpaid=1125 reserve=40000\n",
		receipt.ticks, receipt.reached, Company::Get(_current_company)->money.base());
}

/** Buy only the quoted native wagon, append its actual ID and release all original service orders. */
static void PaySurfaceFarmUpgrade()
{
	const auto *train = VerifySurfaceFarmUpgrade(false); REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->tile == TileIndex{1638});
	for (const Train *part = train; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
	QuoteSurfaceFarmUpgrade("purchase", 3000, 0, Money{1125}, true);
	const auto *company = Company::Get(_current_company); const auto before = company->money; const auto livestock = GetCargoTypeByLabel(CargoLabel{"LVST"});
	const auto bought = PaySurfaceResupply<Commands::BuildVehicle>(TileIndex{1638}, TileIndex{1638}, EngineID{31}, false, livestock, ClientID::Server);
	auto *wagon = Train::Get(std::get<1>(bought)); REQUIRE(wagon->cargo_cap == 25); REQUIRE(wagon->cargo.TotalCount() == 0);
	// Native building may already attach a compatible body. Otherwise append
	// only its real returned ID after original tail23, never moving old wagons.
	if (wagon->First() != train) PaySurfaceResupply<Commands::MoveRailVehicle>(TileIndex{1638}, wagon->index, VehicleID{23}, false);
	VerifySurfaceFarmUpgrade(true); REQUIRE(Train::Get(VehicleID{23})->Next() == wagon);
	PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, train->index, true);
	REQUIRE(!train->vehstatus.Test(VehState::Stopped)); REQUIRE(company->money == before - Money{1125});
	fmt::print("SURFACE_RESUPPLY_FARM_PAID head=52 wagon={} engine=31 invoice=1125 capacity=25 livestock=175 grain=180 length=112 cash_before={} cash={} removed_original=0 changed_orders=0\n",
		wagon->index.base(), before.base(), company->money.base());
}

/** Measure actual new-wagon pickup and genuine working cash within the freshly funded service cap. */
static void WorkSurfaceFarmUpgrade()
{
	const auto *train = VerifySurfaceFarmUpgrade(true); const auto limit = SurfaceChemicalWorkLimit();
	const auto *wagon = Train::Get(VehicleID{23})->Next(); REQUIRE(wagon != nullptr); REQUIRE(!train->vehstatus.Test(VehState::Stopped));
	QuoteSurfaceFarmUpgrade("service", limit, 0, Money{0}, false); const auto *company = Company::Get(_current_company); bool loaded = false;
	/** @return Whether real extra-wagon pickup and actual working cash have both occurred. */
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, [&] {
		REQUIRE(company->money >= Money{40000});
		if (wagon->cargo.TotalCount() > 0) loaded = true;
		// An empty purchased wagon cannot masquerade as delivered capacity;
		// only native packets plus actual cash can satisfy this combined stop.
		return loaded && company->money >= Money{100000};
	});
	VerifySurfaceFarmUpgrade(true);
	fmt::print("SURFACE_RESUPPLY_FARM_SERVICE ticks={} head=52 wagon={} held={} picked_up={} cash={} restored={} partial={} working_shortfall={} module_base_shortfall={} module_capital_complete=false\n",
		receipt.ticks, wagon->index.base(), wagon->cargo.TotalCount(), loaded, company->money.base(), company->money >= Money{100000}, !receipt.reached,
		std::max(Money{0}, Money{100000} - company->money).base(), std::max(Money{0}, Money{510000} - company->money).base());
}
#endif /* TEST_SURFACE_ENTRY_FARM_UPGRADE_H */
