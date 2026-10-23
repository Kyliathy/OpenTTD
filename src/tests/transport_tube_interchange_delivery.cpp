/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_interchange_delivery.cpp Native freight return, conventional final payment and last-moment buffer refusal.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9.
 * Source packets, excavated floor and commissioned inlet are fixture inputs.
 * The payment test explicitly places the same conventional carrier at Surface;
 * it proves final settlement, not the later driven cross-layer useful route.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "transport_tube_freight_fixture.h"
#include "movement_fixture.h"
#include "../order_func.h"
#include "../signal_func.h"
#include "../train_cmd.h"
#include "../safeguards.h"

/** Shared native feeder accounting books no intermediate cash and exactly one final income for actual rail and road recipients. */
TEST_CASE("Tube transfer settles final freight once through conventional rail or road", "[transport-tube-interchange-delivery]")
{
	const bool road = GENERATE(false, true);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubeFreightRig rig;
	const Station *source = rig.BuildSource();
	rig.LoadPacket(*rig.capsule, 17, source->index);
	rig.Arrive(*rig.capsule, rig.tube_tile);
	rig.Stop(*rig.capsule, OrderUnloadType::Transfer, OrderLoadType::NoLoad);
	const auto depot = rig.BuildDepot(MapLayer::Underground, road);
	Vehicle *front = rig.Buy(depot, road ? 7 : 4, road ? rig.cargo : INVALID_CARGO);
	Vehicle *hold = front;
	if (!road) {
		hold = rig.Buy(depot, 5, rig.cargo);
		REQUIRE(CmdMoveRailVehicle(DoCommandFlag::Execute, hold->index, front->index, false).Succeeded());
	}
	rig.Arrive(*front, road ? rig.road_tile : rig.rail_tile);
	if (hold != front) rig.Arrive(*hold, TileAddWrap(rig.rail_tile, 0, 1));
	rig.Stop(*front, OrderUnloadType::NoUnload, OrderLoadType::LoadIfPossible);
	REQUIRE(hold->cargo.TotalCount() == 17);
	CHECK(rig.station->goods[rig.cargo].TotalCount() == 0);
	const Money feeder_share = hold->cargo.GetFeederShare();
	REQUIRE(feeder_share > 0);
	// This reduced registry omits GRF finalization. Resolve the original
	// power plant's own accepted labels, then use its real construction command;
	// no fake accepting station or direct delivery counter substitutes for it.
	auto &spec = *const_cast<IndustrySpec *>(GetIndustrySpec(1));
	for (size_t slot = 0; slot < spec.accepts_cargo_label.size(); ++slot) {
		if (const auto *label = std::get_if<CargoLabel>(&spec.accepts_cargo_label[slot])) spec.accepts_cargo[slot] = GetCargoTypeByLabel(*label);
	}
	{
		AutoRestoreBackup editor(_game_mode, GameMode::Editor);
		REQUIRE(CmdBuildIndustry(DoCommandFlag::Execute, TileXYL(30, 30, MapLayer::Surface), 1, 0, true, 123).Succeeded());
	}
	Industry *receiver = nullptr;
	// Resolve the actual command-created original facility, not an assumed
	// pool position alongside this fixture's already existing native mine.
	for (Industry *industry : Industry::Iterate()) {
		if (industry->type == 1) receiver = industry;
	}
	REQUIRE(receiver != nullptr);
	// Completion is declared fixture staging, separate from the freight
	// transaction whose industry acceptance and packet debit remain native.
	for (const TileIndex tile : receiver->location) {
		if (!receiver->TileBelongsToIndustry(tile)) continue;
		SetIndustryCompleted(tile);
		// Ordinary GRF finalization also resolves each original tile's labels.
		// Keep its declared acceptance amounts and facility meaning unchanged;
		// a station flag written by the fixture would conceal the real boundary.
		auto &tile_spec = *const_cast<IndustryTileSpec *>(GetIndustryTileSpec(GetIndustryGfx(tile)));
		for (size_t slot = 0; slot < tile_spec.accepts_cargo_label.size(); ++slot) {
			if (const auto *label = std::get_if<CargoLabel>(&tile_spec.accepts_cargo_label[slot])) tile_spec.accepts_cargo[slot] = GetCargoTypeByLabel(*label);
		}
	}
	const auto destination_tile = TileXYL(28, 30, MapLayer::Surface);
	if (road) {
		REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, destination_tile, 1, 1, RoadStopType::Truck, false,
			DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	} else {
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, destination_tile, RAILTYPE_RAIL, Axis::Y,
			1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	}
	Station *destination = Station::GetByTile(destination_tile);
	destination->RecomputeCatchment();
	UpdateStationAcceptance(destination, false);
	REQUIRE(destination->goods[rig.cargo].status.Test(GoodsEntry::State::Acceptance));
	CHECK_FALSE(GetTubeTerminal(destination_tile, _current_company).has_value());
	rig.Arrive(*front, destination_tile);
	if (hold != front) rig.Arrive(*hold, TileAddWrap(destination_tile, 0, 1));
	const auto *packet = *hold->cargo.Packets()->begin();
	const Money expected = GetTransportedGoodsIncome(17, packet->GetDistance(hold->GetCargoTile()), packet->GetPeriodsInTransit(), rig.cargo);
	REQUIRE(expected > 0);
	const auto cash = Company::Get(_current_company)->money;
	const auto delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo];
	rig.Stop(*front, OrderUnloadType::UnloadIfPossible, OrderLoadType::NoLoad, destination);
	CHECK(hold->cargo.TotalCount() == 0);
	CHECK(Company::Get(_current_company)->money == cash + expected);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == delivered + 17);
	CHECK(rig.capsule->profit_this_year + front->profit_this_year == expected << 8);
	// An empty repeat visit cannot settle the transferred packet a second time.
	rig.Stop(*front, OrderUnloadType::UnloadIfPossible, OrderLoadType::NoLoad, destination);
	CHECK(Company::Get(_current_company)->money == cash + expected);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == delivered + 17);
}

/** A paid actual capsule follows native station/depot orders, transfers its supplied load, then physically returns empty. */
TEST_CASE("Tube freight follows paid native orders and returns empty to its service depot", "[transport-tube-interchange-delivery]")
{
	PathfinderDefaults pathfinder;
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubeFreightRig rig;
	AutoRestoreBackup<uint64_t> tick(TimerGameTick::counter, uint64_t{0});
	const auto source = rig.BuildSource();
	rig.LoadPacket(*rig.capsule, 17, source->index);
	rig.capsule->CargoChanged();
	// Only these three native route cells connect the existing depot to its
	// genuine station berth. The fixture floor was supplied, not bored for free.
	for (uint y = 41; y < 44; ++y) rig.BuildTrack(TileXYL(38, y, MapLayer::Underground), rig.tube, Track::Y);
	Order transfer;
	transfer.MakeGoToStation(rig.station->index);
	transfer.SetUnloadType(OrderUnloadType::Transfer);
	transfer.SetLoadType(OrderLoadType::NoLoad);
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, rig.capsule->index, 0, transfer).Succeeded());
	Order home;
	home.MakeGoToDepot(Depot::GetByTile(rig.depot)->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {OrderDepotActionFlag::Halt});
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, rig.capsule->index, 1, home).Succeeded());
	ProcessOrders(rig.capsule);
	const auto service = ConnectTubeExternalPower(rig.portal, rig.depot, rig.path);
	REQUIRE(service.error == TubePowerError::None);
	REQUIRE(BindTubePowerVehicle(rig.capsule->index, service.producer).has_value());
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, rig.capsule->index, true).Succeeded());
	UpdateSignalsInBuffer();
	uint steps = 0;
	bool visited = false;
	// Advance only synchronized real power, native movement and station loading.
	// The bounded loop cannot teleport an arrival, clear a vehicle wait or
	// fabricate an empty return by editing cargo, position or current orders.
	for (; steps < 6000; ++steps) {
		++TimerGameTick::counter;
		PrepareTubePowerTick();
		REQUIRE(rig.capsule->Tick());
		LoadUnloadStation(rig.station);
		if (rig.station->goods[rig.cargo].TotalCount() == 17) visited = true;
		if (visited && rig.capsule->IsStoppedInDepot()) break; // Complete the return only after real stock arrived and the native halt order parked the empty capsule.
	}
	REQUIRE(steps < 6000);
	CHECK(visited);
	CHECK(rig.capsule->tile == rig.depot);
	CHECK(rig.capsule->cargo.TotalCount() == 0);
	CHECK(rig.station->goods[rig.cargo].TotalCount() == 17);
	CHECK(GetUtilityPowerProducer(service.producer)->consumed_lifetime_j > 0);
	CHECK(GetTubePowerConnection(service.producer)->invoiced_money > 20);
}

/** Space is revalidated when packets actually move; ordinary stations keep their unrestricted native transfer behavior. */
TEST_CASE("Tube unload rechecks physical room without capping ordinary station stock", "[transport-tube-interchange-delivery]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubeFreightRig rig;
	const auto source = rig.BuildSource();
	CHECK(GetTubeStationTransferRoom(source->index, rig.cargo) == UINT_MAX);
	rig.LoadPacket(*rig.capsule, 17, source->index);
	rig.Arrive(*rig.capsule, rig.tube_tile);
	rig.capsule->current_order.MakeLoading(false);
	rig.capsule->current_order.SetUnloadType(OrderUnloadType::Transfer);
	PrepareUnload(rig.capsule);
	REQUIRE(rig.capsule->cargo.ActionCount(VehicleCargoList::MoveToAction::Transfer) == 17);
	const Money share = rig.capsule->cargo.GetFeederShare();
	rig.AppendStock(TUBE_TERMINAL_TRANSFER_CAPACITY, source->index);
	auto &stock = rig.station->goods[rig.cargo].GetOrCreateData().cargo;
	CHECK(rig.capsule->cargo.Unload(17, &stock, rig.cargo, rig.capsule->cargo_payment, rig.tube_tile) == 0);
	CHECK(rig.capsule->cargo.TotalCount() == 17);
	CHECK(rig.capsule->cargo.GetFeederShare() == share);
	CHECK(stock.TotalCount() == TUBE_TERMINAL_TRANSFER_CAPACITY);
	delete rig.capsule->cargo_payment;
	rig.station->loading_vehicles.remove(rig.capsule);
}

/** An unrelated real queue must not inherit the payment station's finite tube limit. */
TEST_CASE("Tube unload applicability follows the exact receiving list without capping another native queue", "[transport-tube-room]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubeFreightRig rig; auto *ordinary = rig.BuildSource();
	rig.LoadPacket(*rig.capsule, 17, ordinary->index); rig.Arrive(*rig.capsule, rig.tube_tile);
	rig.capsule->current_order.MakeLoading(false); rig.capsule->current_order.SetUnloadType(OrderUnloadType::Transfer);
	PrepareUnload(rig.capsule); REQUIRE(rig.capsule->cargo.ActionCount(VehicleCargoList::MoveToAction::Transfer) == 17);
	rig.AppendStock(TUBE_TERMINAL_TRANSFER_CAPACITY, ordinary->index);
	auto &other = ordinary->goods[rig.cargo].GetOrCreateData().cargo;
	const auto cash = Company::Get(_current_company)->money; const auto share = rig.capsule->cargo.GetFeederShare();
	CHECK_FALSE(GetTubeInterchangeTransferRoom(rig.station->index, rig.cargo, &other).has_value());
	// Explicit mismatched payment/destination component: native transfer still
	// owns the supplied list. The tube adapter declines applicability instead
	// of borrowing a full unrelated queue or substituting another inventory.
	CHECK(rig.capsule->cargo.Unload(17, &other, rig.cargo, rig.capsule->cargo_payment, rig.tube_tile) == 17);
	CHECK(rig.capsule->cargo.TotalCount() == 0); CHECK(other.TotalCount() == 17);
	CHECK(rig.station->goods[rig.cargo].TotalCount() == TUBE_TERMINAL_TRANSFER_CAPACITY);
	CHECK(other.Packets()->begin()->second.front()->GetFeederShare() == share);
	delete rig.capsule->cargo_payment; rig.station->loading_vehicles.remove(rig.capsule);
	CHECK(Company::Get(_current_company)->money == cash);
}
