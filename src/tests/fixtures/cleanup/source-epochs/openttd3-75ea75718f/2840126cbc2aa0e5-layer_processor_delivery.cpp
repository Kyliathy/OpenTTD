/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_processor_delivery.cpp Real carrier packets retain surplus and self-source refusals after bounded final delivery. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 10 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "processor_fixture.h"
#include "../train.h"
#include "../economy_base.h"

#include "../safeguards.h"

/** Actual train/payment/packet owners exercise final delivery without inventing a second accepted inventory. */
class ProcessorDeliveryRig : public ProcessorRig {
public:
	Station *station = nullptr; ///< Real catchment receiver.
	Station *origin = nullptr; ///< Actual packet origin and horizontal distance provenance.
	Train *carrier = nullptr; ///< Real packet-owning train.
	CargoPayment *payment = nullptr; ///< Actual accepted-count payment participant.
	CargoType cargo = INVALID_CARGO; ///< Actual resolved Copper Ore meaning.
	/** Fund a native refinery and place a real adjacent receiving station with an actual carrier payment. */
	ProcessorDeliveryRig()
	{
		REQUIRE(Vehicle::GetNumItems() == 0);
		this->station = this->StationAt(TileXYL(29, 30, MapLayer::Surface));
		this->origin = this->StationAt(TileXYL(20, 20, MapLayer::Surface));
		this->cargo = GetCargoTypeByLabel(CargoLabel{"CORE"});
		REQUIRE(Train::CanAllocateItem());
		this->carrier = Train::Create();
		this->carrier->owner = _current_company;
		this->carrier->cargo_type = this->cargo;
		this->carrier->last_station_visited = this->station->index;
		REQUIRE(CargoPayment::CanAllocateItem());
		this->payment = CargoPayment::Create(this->carrier);
		this->carrier->cargo_payment = this->payment;
	}
	/** Settle the zero-priced fixture payment and remove the isolated packet fixture before its cargo registry teardown. */
	~ProcessorDeliveryRig()
	{
		AutoRestoreBackup mode(_game_mode, GameMode::Menu); // Settle real feeder accounting while suppressing only standalone-fixture text effects.
		delete this->payment;
		// This packet-boundary fixture has no constructed engine or construction statistics; pool cleanup releases its real packets without invoking that unrelated bookkeeping.
		_vehicle_pool.CleanPool();
	}
	/** @param source Actual packet source, including a self-source refusal. @param count Whole units explicitly offered. @param feeder Previously booked transfer share carried by this real packet. */
	void Offer(Source source, uint16_t count, Money feeder = 0)
	{
		REQUIRE(CargoPacket::CanAllocateItem());
		CargoPacket *packet = CargoPacket::Create(this->origin->index, count, source);
		packet->AddFeederShare(feeder);
		packet->UpdateLoadingTile(this->origin->xy);
		this->carrier->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
		this->carrier->cargo.Stage(true, this->station->index, {}, OrderUnloadType::UnloadIfPossible,
			&this->station->goods[this->cargo], this->cargo, this->payment, this->station->xy);
		REQUIRE(this->carrier->cargo.ActionCount(VehicleCargoList::MoveToAction::Deliver) == count);
	}
	/** @return Exactly removed physical units; rejected freight stays with the train. */
	uint Unload()
	{
		return this->carrier->cargo.Unload(100, &this->station->goods[this->cargo].GetOrCreateData().cargo, this->cargo, this->payment, this->station->xy);
	}
};

/** Even when staging offers more than a receiver can take, only real admitted units leave the actual vehicle packet. */
TEST_CASE("Native processor final delivery retains a partially full receiver's surplus", "[layer-processor-delivery]")
{
	ProcessorDeliveryRig rig;
	REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, rig.cargo, 1023) == 1023);
	rig.Offer(Source{IndustryID::Invalid(), SourceType::Industry}, 8, 80);
	const CargoPacketID original_packet = (*rig.carrier->cargo.Packets()->begin())->index;
	const auto delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo];
	CHECK(rig.Unload() == 1);
	CHECK(rig.Input(CargoLabel{"CORE"}).waiting == 1024);
	CHECK(rig.carrier->cargo.TotalCount() == 7);
	CHECK(rig.carrier->cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) == 7);
	CHECK(rig.carrier->cargo.UnloadCount() == 0);
	CHECK(rig.station->goods[rig.cargo].TotalCount() == 0);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == delivered + 1);
	CHECK(rig.payment->route_profit == 0);
	CHECK(rig.payment->visual_profit == -10);
	const CargoPacket *remaining = *rig.carrier->cargo.Packets()->begin();
	CHECK(remaining->index == original_packet);
	CHECK(remaining->GetFirstStation() == rig.origin->index);
	CHECK(remaining->GetFeederShare() == 70);
	CHECK(rig.carrier->cargo.GetFeederShare() == 70);
}

/** A producing industry cannot pay itself by returning its own packets, even if catchment advertises matching free space. */
TEST_CASE("Native processor source refusal retains the real packet and payment", "[layer-processor-delivery]")
{
	ProcessorDeliveryRig rig;
	rig.Offer(Source{rig.industry->index, SourceType::Industry}, 8);
	CHECK(rig.Unload() == 0);
	CHECK(rig.Input(CargoLabel{"CORE"}).waiting == 0);
	CHECK(rig.carrier->cargo.TotalCount() == 8);
	CHECK(rig.carrier->cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) == 8);
	CHECK(rig.payment->route_profit == 0);
	CHECK(rig.payment->visual_profit == 0);
}
