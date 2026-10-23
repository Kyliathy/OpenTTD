/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_riser_delivery.cpp Actual carrier-packet final delivery retains excess construction inputs and their feeder/payment provenance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5B
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_riser_fixture.h"
#include "../train.h"
#include "../economy_base.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** Native machinery, actual station catchment, packet-owning train and final-payment participants; explicit packet staging is not a driven first-build journey. */
class RiserDeliveryRig : public NativeRiserRig {
public:
	Station *station = nullptr; ///< Actual layer-local receiving station beside funded machinery.
	Station *origin = nullptr; ///< Actual packet origin and distance provenance on the same plane.
	Train *carrier = nullptr; ///< Real native cargo-packet owner.
	CargoPayment *payment = nullptr; ///< Actual final-delivery accepted-count and feeder-account participant.
	CargoType cargo = INVALID_CARGO; ///< Current frozen construction-input meaning.
	/** Fund actual unfinished machinery and supply layer-local transport/payment owners without fabricating completion or stock. */
	RiserDeliveryRig()
	{
		REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(this->Fund().Succeeded());
		this->station = this->StationAt(TileXYL(23, 20, MapLayer::DeepUnderground));
		this->origin = this->StationAt(TileXYL(18, 24, MapLayer::DeepUnderground));
		REQUIRE(Train::CanAllocateItem()); this->carrier = Train::Create();
		this->carrier->owner = _current_company; this->carrier->last_station_visited = this->station->index;
		REQUIRE(CargoPayment::CanAllocateItem()); this->payment = CargoPayment::Create(this->carrier);
		this->carrier->cargo_payment = this->payment;
	}
	/** Settle native feeder accounting and remove explicit packet-only vehicles before the parent destroys their industry/station/cargo owners. */
	~RiserDeliveryRig()
	{
		AutoRestoreBackup mode(_game_mode, GameMode::Menu);
		delete this->payment;
		_vehicle_pool.CleanPool(); // Packet-only vehicles have no built engine/statistics; release their real retained packets without unrelated engine bookkeeping.
	}
	/** @param label Frozen construction meaning. @param count Whole offered native packet units. @param feeder Previously recorded transfer share. @param source Actual cargo source, including deliberate self-source refusal. @return Original native packet identity, retained whenever acceptance leaves a remainder. */
	CargoPacketID Offer(CargoLabel label, uint16_t count, Money feeder = 0, IndustryID source = IndustryID::Invalid())
	{
		REQUIRE(this->carrier->cargo.TotalCount() == 0);
		this->cargo = GetCargoTypeByLabel(label); this->carrier->cargo_type = this->cargo;
		REQUIRE(CargoPacket::CanAllocateItem());
		auto *packet = CargoPacket::Create(this->origin->index, count, Source{source, SourceType::Industry});
		const auto id = packet->index;
		packet->AddFeederShare(feeder); packet->UpdateLoadingTile(this->origin->xy);
		this->carrier->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
		this->carrier->cargo.Stage(true, this->station->index, {}, OrderUnloadType::UnloadIfPossible,
			&this->station->goods[this->cargo], this->cargo, this->payment, this->station->xy);
		REQUIRE(this->carrier->cargo.ActionCount(VehicleCargoList::MoveToAction::Deliver) == count);
		return id;
	}
	/** @return Exact physically removed units; final-delivery refusal leaves the original real packet aboard. */
	uint Unload()
	{
		return this->carrier->cargo.Unload(100, &this->station->goods[this->cargo].GetOrCreateData().cargo, this->cargo, this->payment, this->station->xy);
	}
};

/** Partial construction acceptance removes only the remaining bill, retaining exact native packet identity, source and feeder share for the excess aboard. */
TEST_CASE("Native riser packet delivery retains excess construction supplies aboard", "[geothermal-riser-delivery]")
{
	RiserDeliveryRig rig;
	const auto packet = rig.Offer(CargoLabel{"STEL"}, 8, Money{80});
	const auto delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo];
	CHECK(rig.Unload() == 4);
	const auto input = rig.Riser().GetCargoAccepted(rig.cargo); REQUIRE(input != rig.Riser().accepted.end());
	CHECK(input->waiting == 4); REQUIRE(input->history != nullptr); CHECK((*input->history)[THIS_MONTH].accepted == 4);
	CHECK(rig.carrier->cargo.TotalCount() == 4); CHECK(rig.carrier->cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) == 4);
	CHECK(rig.station->goods[rig.cargo].TotalCount() == 0);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == delivered + 4);
	const auto *remaining = *rig.carrier->cargo.Packets()->begin();
	CHECK(remaining->index == packet); CHECK(remaining->GetFirstStation() == rig.origin->index); CHECK(remaining->GetFeederShare() == Money{40});
	CHECK(rig.payment->route_profit == Money{0}); CHECK(rig.payment->visual_profit == Money{-40});
	CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Failed());
}

/** All four real delivered packet meanings enter the single bill; completed or unavailable machinery and self-source attempts retain subsequent packets and cannot create delivery income. */
TEST_CASE("Native riser final delivery respects construction readiness and source refusal", "[geothermal-riser-delivery]")
{
	RiserDeliveryRig rig;
	bool complete = false;
	IndustryID source = IndustryID::Invalid();
	SECTION("full delivered bill then completed machinery") {
		// Exact accepted native packets supply all four independent meanings;
		// construction then consumes only those real stocks at its paid boundary.
		for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->inputs) {
			rig.Offer(input.label, static_cast<uint16_t>(input.quantity)); CHECK(rig.Unload() == input.quantity);
			CHECK(rig.carrier->cargo.TotalCount() == 0);
		}
		rig.Tick(296); REQUIRE(IsGeothermalRiserOperational(rig.Riser().index)); complete = true;
	}
	SECTION("sealed actual portal") { REQUIRE(ChangeLayerPortalConnection(rig.request.portal, _current_company, false, true) == LayerPortalWorkError::None); }
	SECTION("actual self source") { source = rig.Riser().index; }
	const auto packet = rig.Offer(CargoLabel{"BATY"}, 3, Money{30}, source);
	const auto delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo];
	CHECK(rig.Unload() == 0); CHECK(rig.carrier->cargo.TotalCount() == 3);
	CHECK((*rig.carrier->cargo.Packets()->begin())->index == packet); CHECK(rig.carrier->cargo.GetFeederShare() == Money{30});
	CHECK(rig.Riser().GetCargoAccepted(rig.cargo)->waiting == 0);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[rig.cargo] == delivered);
	CHECK(rig.payment->route_profit == Money{0}); CHECK(rig.payment->visual_profit == Money{0});
	CHECK(GetGeothermalRiserState(rig.Riser().index)->work_ticks == (complete ? 296 : 0));
}
