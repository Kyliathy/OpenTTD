/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_farm_feeder_work.h Actual paid short feeder with original native packet owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CSGQ/15CSGR.
 * Separate saved road, station, order and work boundaries preserve every
 * successful native payment; virtual transfer income supplies no cash.
 */
#ifndef TEST_SURFACE_ENTRY_FARM_FEEDER_WORK_H
#define TEST_SURFACE_ENTRY_FARM_FEEDER_WORK_H

/** Read the complete admitted straight corridor without changing its native tiles.
 * @return Same-world1061..1381road cells, including the original component anchor.
 */
static std::vector<TileIndex> SurfaceFarmFeederCorridor()
{
	const auto tiles = SurfaceShortcutCorridor(TileIndex{1061}, TileIndex{1381}); REQUIRE(tiles.size() == 6); return tiles;
}

/** Read a complete native road union, including the actual bay entrance branch.
 * @param tile Actual admitted corridor tile.
 * @return Required native road bits; existing bits are retained by the command.
 */
static RoadBits SurfaceFarmFeederBits(TileIndex tile)
{
	auto bits = AxisToRoadBits(Axis::Y);
	// Both real bay entrances must join the straight road; a missing branch
	// would leave the paid pickup or transfer physically inaccessible.
	if (tile == TileIndex{1317} || tile == TileIndex{1381}) bits |= DiagDirToRoadBits(DiagDirection::NE);
	return bits;
}

/** Verify native reciprocal access from the original depot through both paid bay mouths. */
static void VerifySurfaceFarmFeederRoad()
{
	const auto graph = ReadSurfaceShortcutGraph();
	for (const auto tile : SurfaceFarmFeederCorridor()) {
		REQUIRE(graph.contains(tile)); REQUIRE(IsNormalRoadTile(tile));
		REQUIRE((GetRoadBits(tile, RoadTramType::Road) & SurfaceFarmFeederBits(tile)) == SurfaceFarmFeederBits(tile));
	}
}

/** Pay exactly the admitted native complete road unions with the two bays still protected. */
static void PaySurfaceFarmFeederRoad()
{
	QuerySurfaceFarmFeeder(); QuoteSurfaceFarmRoadWork("feeder-road", Money{3843}, false);
	const auto *company = Company::Get(_current_company); REQUIRE((company->clear_limit >> 16) >= 7); const auto before = company->money;
	// Each final union is quoted and paid once; existing complete bits consume
	// no invoice. Original anchor ownership and unrelated roads remain native.
	for (const auto tile : SurfaceFarmFeederCorridor()) {
		const auto bits = SurfaceFarmFeederBits(tile);
		if (IsNormalRoadTile(tile)) {
			const auto current = GetRoadBits(tile, RoadTramType::Road); if ((current | bits) == current) continue;
		}
		PaySurfaceResupply<Commands::BuildRoad>(tile, tile, bits, ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid());
	}
	VerifySurfaceFarmFeederRoad(); REQUIRE(company->money == before - Money{2509});
	fmt::print("SURFACE_RESUPPLY_FARM_FEEDER_ROAD_PAID invoice=2509 remaining_stops=1334 cash={} elapsed=0 changed_vehicles=0\n", company->money.base());
}

/** Verify both actual paid bays and derive the true pickup identity from its native tile.
 * @return Native new pickup StationID, never an assumed recyclable pool number.
 */
static StationID VerifySurfaceFarmFeederStops()
{
	VerifySurfaceFarmFeederRoad(); const auto graph = ReadSurfaceShortcutGraph();
	for (const auto tile : {TileIndex{1316}, TileIndex{1380}}) {
		REQUIRE(IsStationRoadStopTile(tile)); REQUIRE(GetRoadStopType(tile) == RoadStopType::Truck);
		REQUIRE(!IsDriveThroughStopTile(tile)); REQUIRE(GetBayRoadStopDir(tile) == DiagDirection::SW); REQUIRE(graph.contains(tile));
	}
	const auto pickup = GetStationIndex(TileIndex{1316}); REQUIRE(pickup != StationID{15}); REQUIRE(GetStationIndex(TileIndex{1380}) == StationID{15});
	REQUIRE(Station::Get(pickup)->owner == _current_company); REQUIRE(SurfaceFarmFeederCatchment(TileIndex{1316}, GetCargoTypeByLabel(CargoLabel{"LVST"})));
	return pickup;
}

/** Pay only the two newly quoted actual bays, deriving new station identity after purchase. */
static void PaySurfaceFarmFeederStops()
{
	VerifySurfaceFarmFeederRoad(); QuoteSurfaceFarmRoadWork("feeder-stops", Money{1334}, false);
	const auto *company = Company::Get(_current_company); REQUIRE((company->clear_limit >> 16) >= 2); const auto before = company->money;
	// Quote both untouched stop sites before spending either invoice. Native
	// tile authority decides permission after the actual road construction.
	for (const auto &[tile, station] : std::array<std::pair<TileIndex, StationID>, 2>{{{TileIndex{1316}, NEW_STATION}, {TileIndex{1380}, StationID{15}}}}) {
		const auto quote = QuoteSurfaceFarmFeederBay(tile, DiagDirection::SW, station); REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == Money{667});
	}
	for (const auto &[tile, station] : std::array<std::pair<TileIndex, StationID>, 2>{{{TileIndex{1316}, NEW_STATION}, {TileIndex{1380}, StationID{15}}}}) {
		PaySurfaceResupply<Commands::BuildRoadStop>(tile, tile, uint8_t{1}, uint8_t{1}, RoadStopType::Truck,
			false, DiagDirection::SW, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, station, true);
	}
	const auto pickup = VerifySurfaceFarmFeederStops(); REQUIRE(company->money == before - Money{1334});
	fmt::print("SURFACE_RESUPPLY_FARM_FEEDER_STOPS_PAID pickup={} pickup_tile=1316 transfer=15 transfer_tile=1380 invoice=1334 cash={} elapsed=0\n", pickup.base(), company->money.base());
}

/** Verify only actual62uses new pickup then original15NoLoad/Transfer.
 * @return Existing paid carrier with the exact new native two-stop policy.
 */
static const RoadVehicle *VerifySurfaceFarmFeederOrders()
{
	const auto pickup = VerifySurfaceFarmFeederStops(); const auto *truck = FindSurfaceFarmRoadIncome(); REQUIRE(truck != nullptr); REQUIRE(truck->index == VehicleID{62});
	REQUIRE(truck->GetNumOrders() == 2); REQUIRE(!truck->vehstatus.Test(VehState::Stopped));
	for (uint8_t index = 0; index < 2; ++index) {
		const auto *order = truck->GetOrder(index); REQUIRE(order->IsType(OT_GOTO_STATION));
		REQUIRE(order->GetDestination() == DestinationID{index == 0 ? pickup.base() : uint16_t{15}});
		REQUIRE(order->GetLoadType() == (index == 0 ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad));
		REQUIRE(order->GetUnloadType() == (index == 0 ? OrderUnloadType::NoUnload : OrderUnloadType::Transfer));
	}
	return truck;
}

/** Reassign only empty actual62, with no new vehicle, cargo, position or simulation time. */
static void OrderSurfaceFarmFeeder()
{
	const auto pickup = VerifySurfaceFarmFeederStops(); const auto *truck = VerifySurfaceFarmRoadIncome(); REQUIRE(truck->cargo.TotalCount() == 0);
	QuoteSurfaceFarmRoadWork("feeder-orders", Money{0}, false); const auto cash = Company::Get(_current_company)->money;
	// Remove the old longhaul only from this empty paid body. All original
	// useful trains/buses/Goods services retain their own orders and stock.
	while (truck->GetNumOrders() > 0) PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
	for (uint8_t index = 0; index < 2; ++index) {
		Order order; order.MakeGoToStation(index == 0 ? pickup : StationID{15}); order.SetStopLocation(OrderStopLocation::FarEnd);
		order.SetLoadType(index == 0 ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad);
		order.SetUnloadType(index == 0 ? OrderUnloadType::NoUnload : OrderUnloadType::Transfer);
		PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{index}, order);
	}
	// Native SkipToOrder rejects the already-selected index. Select the other
	// real order first when necessary, then reset travel to the actual pickup.
	if (truck->cur_implicit_order_index == 0) PaySurfaceResupply<Commands::SkipToOrder>(TileIndex{0}, truck->index, VehicleOrderID{1});
	PaySurfaceResupply<Commands::SkipToOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
	VerifySurfaceFarmFeederOrders(); REQUIRE(Company::Get(_current_company)->money == cash);
	fmt::print("SURFACE_RESUPPLY_FARM_FEEDER_ORDERED vehicle=62 pickup={} transfer=15 invoice=0 held=0 cash={} elapsed=0 new_vehicle=0\n", pickup.base(), cash.base());
}

/** Earn only a newly funded2000tick feeder approach or exact loaded-five followthrough.
 * @param follow Whether this separate phase consumes the actual GQfive-unit loaded checkpoint; the original work phase still requires empty cargo.
 */
static void WorkSurfaceFarmFeeder(bool follow = false)
{
	const auto *truck = VerifySurfaceFarmFeederOrders(); const auto pickup = GetStationIndex(TileIndex{1316});
	REQUIRE(truck->cargo.TotalCount() == (follow ? 5 : 0));
	QuoteSurfaceFarmRoadWork("feeder-service", Money{0}, false); const auto *company = Company::Get(_current_company);
	const auto profit = truck->profit_this_year; const auto year = TimerGameEconomy::year; uint maximum = 0; bool transferred = false;
	/** @return Whether the actual first native transfer or genuine working cash has been reached. */
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", 2000, [&] {
		REQUIRE(company->money >= Money{40000}); maximum = std::max(maximum, truck->cargo.TotalCount());
		// Native pickup must retain the actual production identity and first
		// station. No packet creation, rewrite or inventory grant is permitted.
		for (const auto *packet : *truck->cargo.Packets()) {
			REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == 7); REQUIRE(packet->GetFirstStation() == pickup);
		}
		// Only an actually loaded shipment emptying at the real15transfer bay
		// demonstrates the short feeder. Its virtual profit is not final cash.
		if (maximum > 0 && truck->last_station_visited == StationID{15} && truck->cargo.TotalCount() == 0) transferred = true;
		return transferred || company->money >= Money{100000};
	});
	auto delta = truck->profit_this_year - profit;
	if (TimerGameEconomy::year != year) delta = truck->profit_last_year - profit + truck->profit_this_year;
	VerifySurfaceFarmFeederOrders(); VerifySurfaceFarmRebalance(true); VerifySurfaceIntertown();
	fmt::print("SURFACE_RESUPPLY_FARM_FEEDER_SERVICE ticks={} vehicle=62 pickup={} transfer=15 picked_up={} transferred={} maximum_loaded={} held={} profit_delta_256={} cash={} restored={} partial={} feeder_credit_is_cash=false final_payment_proven=false\n",
		receipt.ticks, pickup.base(), maximum > 0, transferred, maximum, truck->cargo.TotalCount(), delta.base(), company->money.base(), company->money >= Money{100000}, !transferred);
}
#endif /* TEST_SURFACE_ENTRY_FARM_FEEDER_WORK_H */
