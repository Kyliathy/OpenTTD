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
#include "transport_tube_freight_fixture.h"
#include "../safeguards.h"

/** Native transfer moves one actual packet to the shared stock; a conventional truck loads it without booking final income. */
TEST_CASE("Tube interchange transfers actual packets to an ordinary freight carrier", "[transport-tube-interchange]")
{
	TubeFreightRig rig;
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
	TubeFreightRig rig;
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
	TubeFreightRig rig;
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
	CHECK(truck->cargo.Return(10, &stock, StationID::Invalid(), rig.road_tile) == 10);
	CHECK(stock.ReservedCount() == 0);
}
