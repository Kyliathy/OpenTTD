/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_interchange.cpp Actual packet custody, finite transfer room and native feeder settlement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9.
 * Arrival placement and input packets are disclosed fixture boundaries. Actual
 * driven excavation/cross-layer useful routes remain later plan acceptance.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "transport_tube_freight_fixture.h"
#include "../safeguards.h"

/** Native transfer moves one actual packet to the shared stock; a conventional truck loads it without booking final income. */
TEST_CASE("Tube interchange transfers actual packets to an ordinary freight carrier", "[transport-tube-interchange]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubeFreightRig rig;
	const auto source = rig.BuildSource();
	const auto cash = Company::Get(_current_company)->money;
	const auto packet = rig.LoadPacket(*rig.capsule, 17, source->index);
	const auto identity = packet->index;
	rig.Arrive(*rig.capsule, rig.tube_tile);
	CHECK(CanTubeTrainEnterTile(*rig.capsule, rig.tube_tile));
	CHECK_FALSE(CanTubeTrainEnterTile(*rig.capsule, rig.rail_tile));
	CHECK_FALSE(CanTubeTrainEnterTile(*rig.capsule, rig.road_tile));
	CHECK_FALSE(CanTubeTrainEnterTile(*rig.capsule, MultiLayerMap::ProjectToSurface(rig.tube_tile)));
	rig.Stop(*rig.capsule, OrderUnloadType::Transfer, OrderLoadType::NoLoad);
	CHECK(rig.capsule->cargo.TotalCount() == 0);
	CHECK(rig.station->goods[rig.cargo].TotalCount() == 17);
	CHECK(CargoPacket::GetIfValid(identity) != nullptr);
	CHECK(Company::Get(_current_company)->money == cash);
	const auto share = CargoPacket::Get(identity)->GetFeederShare();
	CHECK(share > 0);
	Vehicle *truck = rig.Buy(rig.BuildDepot(MapLayer::Underground, true), 7, rig.cargo);
	const auto after_purchase = Company::Get(_current_company)->money;
	rig.Arrive(*truck, rig.road_tile);
	rig.Stop(*truck, OrderUnloadType::NoUnload, OrderLoadType::LoadIfPossible);
	CHECK(truck->cargo.TotalCount() == 17);
	CHECK(rig.station->goods[rig.cargo].TotalCount() == 0);
	CHECK(CargoPacket::Get(identity)->GetFeederShare() == share);
	CHECK(Company::Get(_current_company)->money == after_purchase);
	CHECK_FALSE(rig.station->TileIsInCatchment(MultiLayerMap::ProjectToSurface(source->xy)));
}

/** Partial packet admission uses the same stock/packet owner, while full and already overfull buffers keep incoming cargo aboard. */
TEST_CASE("Tube interchange bounded transfer retains refused packets and feeder shares", "[transport-tube-interchange]")
{
	const uint room = GENERATE(0u, 3u, 17u);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubeFreightRig rig;
	const auto source = rig.BuildSource();
	auto &stock = rig.station->goods[rig.cargo].GetOrCreateData().cargo;
	rig.AppendStock(TUBE_TERMINAL_TRANSFER_CAPACITY - room, source->index);
	rig.LoadPacket(*rig.capsule, 17, source->index);
	const auto cash = Company::Get(_current_company)->money;
	rig.Arrive(*rig.capsule, rig.tube_tile);
	CHECK(GetTubeStationTransferRoom(rig.station->index, rig.cargo) == room);
	rig.Stop(*rig.capsule, OrderUnloadType::Transfer, OrderLoadType::NoLoad);
	CHECK(stock.TotalCount() == TUBE_TERMINAL_TRANSFER_CAPACITY);
	CHECK(rig.capsule->cargo.TotalCount() == 17 - room);
	CHECK(rig.capsule->cargo.GetFeederShare() == 0);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(GetTubeStationTransferRoom(rig.station->index, rig.cargo) == 0);
	// A second native visit to the same full buffer cannot manufacture another
	// feeder credit, lose the retained remainder or count a delivery payment.
	if (room < 17) {
		const auto profit = rig.capsule->profit_this_year;
		rig.Stop(*rig.capsule, OrderUnloadType::Transfer, OrderLoadType::NoLoad);
		CHECK(rig.capsule->cargo.TotalCount() == 17 - room);
		CHECK(rig.capsule->profit_this_year == profit);
	}
	rig.AppendStock(5, source->index);
	CHECK(GetTubeStationTransferRoom(rig.station->index, rig.cargo) == 0);
	CHECK(stock.TotalCount() == TUBE_TERMINAL_TRANSFER_CAPACITY + 5);
}

/** Earlier actual arrivals reserve transfer space in their own staged packets; a second arrival cannot allocate it again. */
TEST_CASE("Tube interchange pending unloads and loading reservations share the bound", "[transport-tube-interchange]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubeFreightRig rig;
	const auto source = rig.BuildSource();
	auto &stock = rig.station->goods[rig.cargo].GetOrCreateData().cargo;
	rig.AppendStock(TUBE_TERMINAL_TRANSFER_CAPACITY - 17, source->index);
	rig.LoadPacket(*rig.capsule, 17, source->index);
	rig.Arrive(*rig.capsule, rig.tube_tile);
	rig.capsule->current_order.MakeLoading(false);
	rig.capsule->current_order.SetUnloadType(OrderUnloadType::Transfer);
	rig.capsule->current_order.SetLoadType(OrderLoadType::NoLoad);
	PrepareUnload(rig.capsule);
	CHECK(rig.capsule->cargo.ActionCount(VehicleCargoList::MoveToAction::Transfer) == 17);
	CHECK(GetTubeStationTransferRoom(rig.station->index, rig.cargo) == 0);
	CHECK(GetTubeInterchangeTransferRoom(rig.station->index, rig.cargo, &stock, true) == 0);
	CHECK(GetTubeInterchangeTransferRoom(rig.station->index, rig.cargo, &stock) == 17);
	Train *second = rig.BuyCapsule();
	rig.LoadPacket(*second, 7, source->index);
	rig.Arrive(*second, TileAddWrap(rig.tube_tile, 0, 1));
	rig.Stop(*second, OrderUnloadType::Transfer, OrderLoadType::NoLoad);
	CHECK(second->cargo.TotalCount() == 7);
	CHECK(stock.TotalCount() == TUBE_TERMINAL_TRANSFER_CAPACITY);
	CHECK(rig.capsule->cargo.TotalCount() == 0);
	delete rig.capsule->cargo_payment;
	rig.station->loading_vehicles.remove(rig.capsule);
	Vehicle *truck = rig.Buy(rig.BuildDepot(MapLayer::Underground, true), 7, rig.cargo);
	const std::array<StationID, 0> any{};
	CHECK(stock.Reserve(10, &truck->cargo, any, rig.road_tile) == 10);
	CHECK(stock.ReservedCount() == 10);
	CHECK(GetTubeStationTransferRoom(rig.station->index, rig.cargo) == 0);
	CHECK(GetTubeInterchangeTransferRoom(rig.station->index, rig.cargo, &stock, true) == 0);
	CHECK(GetTubeInterchangeTransferRoom(rig.station->index, rig.cargo, &stock) == 0);
	CHECK(truck->cargo.Return(10, &stock, StationID::Invalid(), rig.road_tile) == 10);
	CHECK(stock.ReservedCount() == 0);
}

/** Exact queue identity and installed berth membership distinguish zero tube room from no applicable tube policy. */
TEST_CASE("Tube room adapters share one bound only for the actual joined station queue", "[transport-tube-room]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); TubeFreightRig rig; auto *ordinary = rig.BuildSource(); const auto id = rig.station->index;
	CHECK_FALSE(rig.station->goods[rig.cargo].HasData()); CHECK(GetTubeStationTransferRoom(id, rig.cargo) == 1024);
	CHECK_FALSE(rig.station->goods[rig.cargo].HasData());
	StationCargoList detached{};
	CHECK_FALSE(GetTubeInterchangeTransferRoom(id, rig.cargo, &detached).has_value());
	CHECK_FALSE(rig.station->goods[rig.cargo].HasData());
	auto &queue = rig.station->goods[rig.cargo].GetOrCreateData().cargo;
	auto &other = ordinary->goods[rig.cargo].GetOrCreateData().cargo;
	CHECK(GetTubeInterchangeTransferRoom(id, rig.cargo, &queue) == 1024);
	CHECK_FALSE(GetTubeInterchangeTransferRoom(id, rig.cargo, &other).has_value());
	CHECK_FALSE(GetTubeInterchangeTransferRoom(id, rig.cargo, nullptr).has_value());
	CHECK_FALSE(GetTubeInterchangeTransferRoom(id, INVALID_CARGO, &queue).has_value());
	CHECK_FALSE(GetTubeInterchangeTransferRoom(StationID::Invalid(), rig.cargo, &queue).has_value());
	CHECK_FALSE(GetTubeInterchangeTransferRoom(ordinary->index, rig.cargo, &other).has_value());
	CHECK(GetTubeStationTransferRoom(id, INVALID_CARGO) == 0); CHECK(GetTubeStationTransferRoom(ordinary->index, rig.cargo) == UINT_MAX);
	{
		// A foreign tube berth inside a claimed rectangle cannot confer native
		// station membership or cap this ordinary station's actual queue.
		AutoRestoreBackup rectangle(ordinary->train_station, rig.station->train_station);
		CHECK(GetTubeStationTransferRoom(ordinary->index, rig.cargo) == UINT_MAX);
		CHECK_FALSE(GetTubeInterchangeTransferRoom(ordinary->index, rig.cargo, &other).has_value());
	}
	rig.AppendStock(1021, ordinary->index); CHECK(GetTubeInterchangeTransferRoom(id, rig.cargo, &queue) == 3);
	CHECK(GetTubeInterchangeTransferRoom(Station::GetByTile(rig.rail_tile)->index, rig.cargo, &queue) == 3);
	CHECK(GetTubeInterchangeTransferRoom(Station::GetByTile(rig.road_tile)->index, rig.cargo, &queue) == 3);
	rig.AppendStock(3, ordinary->index); CHECK(GetTubeInterchangeTransferRoom(id, rig.cargo, &queue) == 0);
	rig.AppendStock(UINT16_MAX, ordinary->index); CHECK(GetTubeInterchangeTransferRoom(id, rig.cargo, &queue) == 0);
	CHECK(queue.TotalCount() == 1024 + UINT16_MAX); CHECK(GetTubeStationTransferRoom(id, rig.cargo) == 0);
}
