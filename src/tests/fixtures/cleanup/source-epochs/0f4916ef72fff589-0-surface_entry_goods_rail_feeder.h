/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_goods_rail_feeder.h Pure short road transfer into actual existing Goods rail capacity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15CSGZB/15CSGZC.
 * Actual station packets, native road geometry and command estimates own
 * admission. No station, cargo, road, transport policy or payment is supplied.
 */
#ifndef TEST_SURFACE_ENTRY_GOODS_RAIL_FEEDER_H
#define TEST_SURFACE_ENTRY_GOODS_RAIL_FEEDER_H

#include "../vehicle_func.h"
#include "table/strings.h"

/** Complete temporary one-bay proposal, with no ownership of map tiles or money. */
struct SurfaceGoodsRailFeederQuote {
	bool found = false; ///< Whole physical bay/branch and command estimates admitted.
	TileIndex bay = INVALID_TILE, anchor = INVALID_TILE; ///< New joined12bay and actual existing reciprocal road mouth.
	DiagDirection entrance = DiagDirection::NE; ///< Native direction from bay to road mouth.
	RoadBits bits{}; ///< Complete native mouth union, including the bay-facing branch.
	Money bay_cost{0}, road_cost{0}; ///< Whole unpaid public command invoices.
	uint distance = 0; ///< Weighted existing source13road trip plus one new bay edge.
};

/** Quote a short joined12bay on the existing reciprocal road component.
 * @param graph Actual unchanged depot-connected weighted native road graph.
 * @return Shortest fully quoted13-to12proposal, then cheapest whole invoice; no paid map mutation.
 */
static SurfaceGoodsRailFeederQuote SelectSurfaceGoodsRailFeeder(const SurfaceShortcutGraph &graph)
{
	SurfaceGoodsRailFeederQuote best; const auto distances = SurfaceShortcutDistances(graph, TileIndex{2572});
	uint candidates = 0, refused = 0, error = 0;
	const auto quota = Company::Get(_current_company)->clear_limit >> 16;
	// Reuse only a real reciprocal normal-road mouth within twelve weighted
	// edges of the paid13pickup. A coordinate-near bay on a disconnected road
	// cannot become a feeder, and existing rail/platforms remain untouched.
	for (const auto &[anchor, distance] : distances) {
		if (!IsNormalRoadTile(anchor)) continue;
		if (distance + 1 > 12) continue;
		for (const auto outward : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			const auto bay = SurfaceChemicalLayout::Step(anchor, outward);
			if (!SurfaceFarmFeederBare(bay)) continue;
			if (quota < 1) continue;
			++candidates; const auto entrance = ReverseDiagDir(outward);
			const auto bay_quote = QuoteSurfaceResupply<Commands::BuildRoadStop>(bay, bay, uint8_t{1}, uint8_t{1}, RoadStopType::Truck,
				false, entrance, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, StationID{12}, true);
			if (bay_quote.Failed()) { ++refused; error = bay_quote.GetErrorMessage().base(); continue; }
			const auto current = GetRoadBits(anchor, RoadTramType::Road), bits = current | DiagDirToRoadBits(outward);
			CommandCost branch(ExpensesType::Construction);
			// The bay's road entrance must have an actual reciprocal branch.
			// Price the complete mouth union once; an already present branch
			// costs zero and cannot be billed as an imaginary new road.
			if (bits != current) branch = QuoteSurfaceResupply<Commands::BuildRoad>(anchor, anchor, bits, ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid());
			if (branch.Failed()) { ++refused; error = branch.GetErrorMessage().base(); continue; }
			const SurfaceGoodsRailFeederQuote proposal{true, bay, anchor, entrance, bits, bay_quote.GetCost(), branch.GetCost(), distance + 1};
			// Short trips allow the existing eighteen-unit truck to feed spare
			// rail capacity. Equal trip lengths prefer lower complete native
			// outlay and deterministic tile order, without forecasting any fare.
			if (!best.found || std::tuple{proposal.distance, proposal.bay_cost + proposal.road_cost, proposal.bay} <
				std::tuple{best.distance, best.bay_cost + best.road_cost, best.bay}) best = proposal;
		}
	}
	fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_FEEDER_SEARCH nodes={} candidates={} refused={} error={} quota={} found={} paid=0 ticks=0\n",
		graph.size(), candidates, refused, error, quota, best.found);
	return best;
}

/** Quote real Factory13Goods transfer into original49's existing12pickup, with no transport changes. */
static void QuerySurfaceGoodsRailFeeder()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{47833}); REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	VerifySurfaceFactoryGoodsOrders(); VerifySurfaceFarmFeederOrders(); VerifySurfaceFarmRebalance(true); VerifySurfaceIntertown(true);
	const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	const auto *pickup = Station::Get(StationID{13}), *transfer = Station::Get(StationID{12}), *receiver = Station::Get(StationID{11});
	REQUIRE(pickup->owner == _current_company); REQUIRE(transfer->owner == _current_company); REQUIRE(receiver->owner == _current_company);
	REQUIRE(transfer->train_station.tile == TileIndex{3217}); REQUIRE(receiver->goods[goods].status.Test(GoodsEntry::State::Acceptance));
	REQUIRE(IsStationRoadStopTile(TileIndex{2572})); REQUIRE(GetStationIndex(TileIndex{2572}) == StationID{13}); REQUIRE(GetRoadStopType(TileIndex{2572}) == RoadStopType::Truck);
	const auto *train = Train::Get(VehicleID{49}); REQUIRE(train->engine_type == EngineID{24}); REQUIRE(train->owner == _current_company); REQUIRE(train->GetNumOrders() == 2);
	REQUIRE(train->GetOrder(0)->GetDestination() == DestinationID{12}); REQUIRE(train->GetOrder(0)->GetLoadType() == OrderLoadType::LoadIfPossible);
	REQUIRE(train->GetOrder(1)->GetDestination() == DestinationID{11}); REQUIRE(train->GetOrder(1)->GetLoadType() == OrderLoadType::NoLoad);
	uint parts = 0, capacity = 0, held = 0;
	// The original whole consist supplies spare real capacity; no parked
	// wagon, forecast production or new train is added to this proposal.
	for (const auto *part = train; part != nullptr; part = part->Next()) {
		++parts; capacity += part->cargo_cap; held += part->cargo.TotalCount();
		if (part->cargo_cap > 0) REQUIRE(part->cargo_type == goods);
	}
	REQUIRE(parts == 13); REQUIRE(capacity == 300); REQUIRE(held == 78);
	uint source5 = 0, source11 = 0;
	// Only actual current first13industry Goods may enter this feeder. The
	// original fifty launch Goods at21 and held commercial shipments remain
	// separate native owners and cannot fund or supply a quoted transfer.
	for (const auto &[hop, packets] : *pickup->goods[goods].GetData().cargo.Packets()) for (const auto *packet : packets) {
		REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetFirstStation() == StationID{13});
		REQUIRE((packet->GetSource().id == 5 || packet->GetSource().id == 11));
		if (packet->GetSource().id == 5) source5 += packet->Count(); else source11 += packet->Count();
	}
	REQUIRE(source5 == 493); REQUIRE(source11 == 15); REQUIRE(transfer->goods[goods].AvailableCount() == 0);
	const auto *truck = RoadVehicle::Get(VehicleID{3}); REQUIRE(truck->engine_type == EngineID{140}); REQUIRE(truck->cargo_cap == 18);
	const auto proposal = SelectSurfaceGoodsRailFeeder(ReadSurfaceShortcutGraph());
	Order order; order.MakeGoToStation(StationID{12}); order.SetLoadType(OrderLoadType::NoLoad); order.SetUnloadType(OrderUnloadType::Transfer);
	order.SetNonStopType(OrderNonStopFlag::NonStop); order.SetStopLocation(OrderStopLocation::FarEnd);
	const auto remove = QuoteSurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{1});
	const auto insert = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, order);
	REQUIRE(remove.Succeeded()); REQUIRE(remove.GetCost() == Money{0});
	// The new bay is a real prerequisite:12currently has only railway
	// platforms. Keep its literal native order refusal and never supply a
	// fake truck facility for this query. The identical transfer policy at
	// actual13proves its native zero price; after paid12bay construction a
	// later consumer must freshly pass the real12order/facility gate.
	REQUIRE(!transfer->facilities.Test(StationFacility::TruckStop)); REQUIRE(insert.Failed());
	REQUIRE(insert.GetErrorMessage() == STR_ERROR_CAN_T_ADD_ORDER); REQUIRE(GetVehicleCannotUseStationReason(truck, transfer) == STR_ERROR_NO_TRUCK_STATION);
	Order control = order; control.SetDestination(DestinationID{13});
	const auto control_quote = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, control);
	REQUIRE(control_quote.Succeeded()); REQUIRE(control_quote.GetCost() == Money{0});
	fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_FEEDER_ORDER destination=12 available=false error={} reason={} prerequisite=paid_truck_bay control_destination=13 control_available=true control_invoice=0 paid=0\n",
		insert.GetErrorMessage().base(), GetVehicleCannotUseStationReason(truck, transfer).base());
	fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_FEEDER_SOURCE truck=3 engine=140 capacity=18 truck_held={} pickup=13 source5={} source11={} physical={} transfer=12 waiting=0 rail=49 rail_parts={} rail_capacity={} rail_held={} final_receiver=11 receiver_accepts=true order_invoice=0 new_vehicle=0 paid=0 ticks=0\n",
		truck->cargo.TotalCount(), source5, source11, source5 + source11, parts, capacity, held);
	fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_FEEDER_PROPOSAL found={} bay={} anchor={} entrance={} bits={} bay_invoice={} road_invoice={} distance={} new_station=false rail_changes=0 paid=0 ticks=0\n",
		proposal.found, proposal.bay.base(), proposal.anchor.base(), to_underlying(proposal.entrance), proposal.bits.base(), proposal.bay_cost.base(), proposal.road_cost.base(), proposal.distance);
	// Every complete proposed interval pays all current ground/Space/interest
	// owners up front in admission. A future final rail fare or virtual road
	// transfer share contributes no cash or permission to this investment.
	for (const uint limit : {2000, 1000, 500, 250, 64}) {
		const auto bill = QuoteSurfaceFixedPriceIncome(limit); const auto operating = bill.upkeep + bill.interest + bill.station;
		const auto invoice = proposal.bay_cost + proposal.road_cost, target = Money{40000} + invoice + operating;
		fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_FEEDER_ADMISSION ticks={} operating={} invoice={} reserve=40000 target={} cash={} funded={} complete={} future_income_counted=0 refunds_counted=0 credit=0\n",
			limit, operating.base(), invoice.base(), target.base(), company->money.base(), company->money >= target, proposal.found);
	}
}
/** Admit one whole funded native interval without counting a future final rail fare.
 * @param unpaid Complete remaining bay/branch invoice, zero after actual construction.
 * @param ceiling Explicit group-owned work maximum; first GZCscope permits500only.
 * @return Longest complete funded cap or zero for unchanged refusal; supplies no earned ticks.
 */
static uint QuoteSurfaceGoodsRailFeederWork(Money unpaid, uint ceiling = 500)
{
	REQUIRE(ceiling > 0); REQUIRE(ceiling <= 2000); const auto *company = Company::Get(_current_company);
	// Every current owner, calendar charge and remaining physical invoice is
	// priced before admission. A shorter cap is a whole independent proposal,
	// never a loan against transferred cargo or an expected final train fare.
	for (const uint limit : {2000, 1000, 500, 250, 64}) {
		if (limit > ceiling) continue;
		const auto bill = QuoteSurfaceFixedPriceIncome(limit); const auto operating = bill.upkeep + bill.interest + bill.station;
		const auto target = Money{40000} + unpaid + operating; const bool funded = company->money >= target;
		fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_FEEDER_WORK_ADMISSION ceiling={} ticks={} unpaid={} operating={} reserve=40000 target={} cash={} funded={} future_income_counted=0\n",
			ceiling, limit, unpaid.base(), operating.base(), target.base(), company->money.base(), funded);
		if (funded) return limit;
	}
	return 0;
}

/** Verify the actual paid joined12bay and its complete reciprocal connection. */
static void VerifySurfaceGoodsRailFeederStop()
{
	REQUIRE(IsStationRoadStopTile(TileIndex{2575})); REQUIRE(GetStationIndex(TileIndex{2575}) == StationID{12});
	REQUIRE(GetRoadStopType(TileIndex{2575}) == RoadStopType::Truck); REQUIRE(!IsDriveThroughStopTile(TileIndex{2575}));
	REQUIRE(GetBayRoadStopDir(TileIndex{2575}) == DiagDirection::SE); REQUIRE(Station::Get(StationID{12})->owner == _current_company);
	REQUIRE((GetRoadBits(TileIndex{2639}, RoadTramType::Road) & RoadBits{11}) == RoadBits{11});
	const auto graph = ReadSurfaceShortcutGraph(); const auto distance = SurfaceShortcutDistances(graph, TileIndex{2572});
	REQUIRE(distance.contains(TileIndex{2575})); REQUIRE(distance.at(TileIndex{2575}) == 5);
	REQUIRE(CanVehicleUseStation(RoadVehicle::Get(VehicleID{3}), Station::Get(StationID{12})));
}

/** Pay only the original378bay/branch after complete native whole-bill admission. */
static void PaySurfaceGoodsRailFeederInfrastructure()
{
	QuerySurfaceGoodsRailFeeder(); const auto proposal = SelectSurfaceGoodsRailFeeder(ReadSurfaceShortcutGraph());
	REQUIRE(proposal.found); REQUIRE(proposal.bay == TileIndex{2575}); REQUIRE(proposal.anchor == TileIndex{2639});
	REQUIRE(proposal.entrance == DiagDirection::SE); REQUIRE(proposal.bits == RoadBits{11});
	REQUIRE(proposal.bay_cost == Money{307}); REQUIRE(proposal.road_cost == Money{71});
	REQUIRE(QuoteSurfaceGoodsRailFeederWork(Money{378}) > 0); const auto *company = Company::Get(_current_company); const auto cash = company->money;
	PaySurfaceResupply<Commands::BuildRoad>(proposal.anchor, proposal.anchor, proposal.bits, ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid());
	PaySurfaceResupply<Commands::BuildRoadStop>(proposal.bay, proposal.bay, uint8_t{1}, uint8_t{1}, RoadStopType::Truck,
		false, proposal.entrance, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, StationID{12}, true);
	VerifySurfaceGoodsRailFeederStop(); VerifySurfaceFactoryGoodsOrders(); REQUIRE(company->money == cash - Money{378});
	fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_FEEDER_INFRASTRUCTURE bay=2575 anchor=2639 station=12 branch_invoice=71 bay_invoice=307 invoice=378 elapsed=0 cash={} new_station=false\n", company->money.base());
}

/** Replace only the empty3receiver after real12truck compatibility and native order admission pass. */
static void OrderSurfaceGoodsRailFeeder()
{
	VerifySurfaceGoodsRailFeederStop(); VerifySurfaceFactoryGoodsOrders(); REQUIRE(QuoteSurfaceGoodsRailFeederWork(Money{0}) > 0);
	const auto *truck = RoadVehicle::Get(VehicleID{3}); REQUIRE(truck->cargo.TotalCount() == 0); REQUIRE(truck->cargo_payment == nullptr);
	const auto cash = Company::Get(_current_company)->money; const auto tile = truck->tile;
	auto order = SurfaceFactoryGoodsOrder(false); order.SetDestination(DestinationID{12}); order.SetUnloadType(OrderUnloadType::Transfer);
	const auto quote = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, order);
	REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == Money{0});
	PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{1});
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, order);
	VerifySurfaceFactoryGoodsOrders(true); REQUIRE(truck->tile == tile); REQUIRE(truck->cargo.TotalCount() == 0); REQUIRE(Company::Get(_current_company)->money == cash);
	fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_FEEDER_ORDERED vehicle=3 pickup=13 transfer=12 load=4 unload=2 invoice=0 elapsed=0 cash={} original25_receiver=20 actual_facility=true\n", cash.base());
}

/** Read actual first13Goods already transferred into original12or its original49train.
 * @return Whole cohort quantity and native virtual feeder share; neither is posted cash.
 */
static std::pair<uint, Money> SurfaceGoodsRailFeederStock()
{
	uint quantity = 0; Money share{0}; const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	/** Add one real packet only when its first station identifies this physical feeder cohort.
	 * @param packet Actual sole-owned station/train packet, never a copied inventory.
	 */
	const auto add = [&](const CargoPacket *packet) {
		if (packet->GetFirstStation() != StationID{13}) return;
		REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE((packet->GetSource().id == 5 || packet->GetSource().id == 11));
		quantity += packet->Count(); share += packet->GetFeederShare();
	};
	// Native transfer and immediate rail loading may split/merge the same
	// physical packets. First13distinguishes them from newly produced first12
	// offers, while summing actual owners avoids a recyclable-ID receipt.
	for (const auto &[hop, packets] : *Station::Get(StationID{12})->goods[goods].GetData().cargo.Packets()) for (const auto *packet : packets) add(packet);
	for (const auto *part = Train::Get(VehicleID{49}); part != nullptr; part = part->Next()) {
		if (part->cargo_cap > 0) REQUIRE(part->cargo_type == goods);
		for (const auto *packet : *part->cargo.Packets()) add(packet);
	}
	return {quantity, share};
}

/** Earn only a freshly funded first physical transfer interval.
 * @param ceiling Explicit work maximum,500in first GZCscope; any later followthrough owns its own cap.
 */
static void WorkSurfaceGoodsRailFeeder(uint ceiling)
{
	VerifySurfaceGoodsRailFeederStop(); VerifySurfaceFactoryGoodsOrders(true); VerifySurfaceFarmFeederOrders(); VerifySurfaceFarmRebalance(true); VerifySurfaceIntertown(true);
	const uint limit = QuoteSurfaceGoodsRailFeederWork(Money{0}, ceiling);
	fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_FEEDER_WORK_SELECTION ceiling={} found={} ticks={}\n", ceiling, limit > 0, limit);
	if (limit == 0) return;
	const auto *company = Company::Get(_current_company); const auto *truck = RoadVehicle::Get(VehicleID{3});
	const auto initial = SurfaceGoodsRailFeederStock(); REQUIRE(initial.first == 0);
	/** @return Whether the real first13cohort fully left3at12or actual cash restored the working goal. */
	const auto done = [&] {
		REQUIRE(company->money >= Money{40000});
		// Final rail income is still a later boundary. Actual sole-owned
		// first13stock at12/on49and an empty3at12prove physical transfer,
		// with no credit for its virtual feeder profit or future delivery.
		const bool transferred = SurfaceGoodsRailFeederStock().first > 0 && truck->cargo.TotalCount() == 0 && truck->last_station_visited == StationID{12};
		return transferred || company->money >= Money{100000};
	};
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, done); const auto final = SurfaceGoodsRailFeederStock();
	VerifySurfaceGoodsRailFeederStop(); VerifySurfaceFactoryGoodsOrders(true); VerifySurfaceFarmFeederOrders(); VerifySurfaceFarmRebalance(true); VerifySurfaceIntertown(true);
	fmt::print("SURFACE_RESUPPLY_GOODS_RAIL_FEEDER_SERVICE ticks={} cohort={} feeder_share={} truck_held={} last_station={} cash={} restored={} partial={} feeder_credit_is_cash=false rail_final_payment_claimed=false\n",
		work.ticks, final.first, final.second.base(), truck->cargo.TotalCount(), truck->last_station_visited.base(), company->money.base(), company->money >= Money{100000}, !work.reached);
}
#endif /* TEST_SURFACE_ENTRY_GOODS_RAIL_FEEDER_H */
