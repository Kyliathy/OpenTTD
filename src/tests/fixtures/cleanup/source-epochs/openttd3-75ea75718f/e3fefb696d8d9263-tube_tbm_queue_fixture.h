/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_tbm_queue_fixture.h Actual competing capsules and paid-grid loss/recovery on the original earned tube floor.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16D.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_TUBE_TBM_QUEUE_FIXTURE_H
#define TESTS_TUBE_TBM_QUEUE_FIXTURE_H

#include "tube_tbm_useful_fixture.h"
#include "../pbs.h"
#include "../signal_type.h"

/** Borrowed original paid useful world plus one real paid competing capsule/berth;all packets,paths and grants remain native-owned. */
class TubeTBMQueueWorld : public TubeTBMUsefulWorld {
public:
	Train *peer = nullptr; ///< Real additional same-company Mole bought on the original paid chamber floor.
	TileIndex branch = INVALID_TILE; ///< Native side service depot,not a new geological opening.
	UtilityProducerID previous_source{UINT32_MAX}; ///< Original paid producer retained after ordinary closure.
	TubePowerConnection original_connection; ///< Read-only original installed wire/invoice observations for paid reconstruction.
	uint queued_units = 0; ///< Observed actual packets that have already left both loading berths.
	Train *winner = nullptr; ///< Actual reservation owner observed at the merge,never assigned to native PBS.
	Train *waiting = nullptr; ///< Actual powered contender held by that reservation.

	/** Fund a real side berth/merge and competing capsule entirely on the original paid chamber,then use native orders to start both actual fronts. */
	void PrepareQueue()
	{
		this->ResolveUseful();
		this->branch = this->At(0, 5);
		const auto floor = std::vector<uint8_t>(GetGeologyWorldState().terrain.Data().begin(), GetGeologyWorldState().terrain.Data().end());
		this->Do<Commands::BuildRailDepot>(this->branch, this->tube, DiagDirection::SW);
		this->Do<Commands::BuildRailStation>(this->At(1, 5), this->tube, Axis::X, uint8_t{1}, uint8_t{2}, STAT_CLASS_DFLT, uint16_t{0}, this->origin->index, false);
		this->Do<Commands::BuildRail>(this->At(3, 5), this->tube, Track::X, false);
		this->Do<Commands::BuildRail>(this->At(4, 5), this->tube, Track::Right, false);
		// Actual path signals on both paid approaches give the same native PBS
		// owner exclusive custody of the merge. No diagnostic writes track
		// reservations or manufactures a hidden-depot claim.
		for (const auto [tile, track] : {std::pair{this->At(4, 4), Track::Y}, std::pair{this->At(3, 5), Track::X}}) {
			const auto direction = track == Track::Y ? Trackdir::Y_SE : Trackdir::X_SW;
			this->Do<Commands::BuildSignal>(tile, track, SignalType::Path, SignalVariant::Electric, false, false, false, SignalType::Block, SignalType::Block, uint8_t{0}, SignalAlongTrackdir(direction));
		}
		// First binding belongs at the actual installed inlet depot. Purchase
		// there,then physically visit the far reversal depot and side branch
		// through ordinary empty orders;no source ID can bind a remote purchase.
		this->peer = this->Buy(this->tube_depots[0], GetTubeEngine(9));
		this->Do<Commands::SetTubeVehiclePower>(this->peer->index, this->power, false);
		Order far; far.MakeGoToDepot(Depot::GetByTile(this->tube_depots[1])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		Order side; side.MakeGoToDepot(Depot::GetByTile(this->branch)->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {OrderDepotActionFlag::Halt});
		this->Do<Commands::InsertOrder>(this->peer->index, VehicleOrderID{0}, far);
		this->Do<Commands::InsertOrder>(this->peer->index, VehicleOrderID{1}, side);
		ProcessOrders(this->peer);
		this->Do<Commands::StartStopVehicle>(this->peer->index, true);
		uint approach_ticks = 0;
		// A real service reversal and complete empty side-depot arrival earn
		// this competing approach. Private movement,force-proceed or order skips
		// cannot establish a peer on the opposite actual paid branch.
		while (approach_ticks < 12000) {
			this->Tick();
			++approach_ticks;
			this->CheckConservation(this->peer);
			if (this->peer->IsChainInDepot() && this->peer->tile == this->branch && this->peer->vehstatus.Test(VehState::Stopped)) break;
		}
		INFO("paid peer approach ticks=" << approach_ticks << " peer=" << DescribeTrain(this->peer));
		REQUIRE(approach_ticks < 12000);
		REQUIRE(this->peer->cargo.StoredCount() == 0);
		this->Do<Commands::DeleteOrder>(this->peer->index, VehicleOrderID{1});
		this->Do<Commands::DeleteOrder>(this->peer->index, VehicleOrderID{0});
		this->Orders(*this->peer, this->origin->index, this->interchange->index, {this->branch, this->tube_depots[1]}, true);
		this->Do<Commands::ModifyOrder>(this->peer->index, VehicleOrderID{3}, MOF_DEPOT_ACTION, static_cast<uint16_t>(OrderDepotAction::Stop));
		this->Do<Commands::StartStopVehicle>(this->capsule->index, true);
		this->Do<Commands::StartStopVehicle>(this->peer->index, true);
		CHECK(std::vector<uint8_t>(GetGeologyWorldState().terrain.Data().begin(), GetGeologyWorldState().terrain.Data().end()) == floor);
	}

	/** @return Whether two actual loaded,powered,departed capsules now compete for the same native reservation;the query creates no path or grant. */
	bool ObserveQueue()
	{
		if (this->capsule->cargo.StoredCount() == 0) return false;
		if (this->peer->cargo.StoredCount() == 0) return false;
		const auto *first = GetTubePowerVehicle(this->capsule->index), *second = GetTubePowerVehicle(this->peer->index);
		if (first->wait != TubePowerWait::None) return false;
		if (second->wait != TubePowerWait::None) return false;
		const auto reserved = GetReservedTrackbits(this->At(4, 5));
		// A real reservation owner at the shared exit must hold the other
		// loaded contender on its plain approach. Loading dwell,depot placement
		// or player-stopped trains cannot establish this queue observation.
		for (const auto track : {Track::Y, Track::Right}) {
			if (!reserved.Test(track)) continue;
			Train *front = GetTrainForReservation(this->At(4, 5), track);
			if (front != this->capsule && front != this->peer) continue;
			Train *other = front == this->capsule ? this->peer : this->capsule;
			// Native PBS may hold the actual front at the final platform cell
			// before its signal. Require its real Stuck reservation wait and
			// departed transfer order instead of assuming the front already
			// entered the signal tile or mistaking loading dwell for a queue.
			if (other->IsInDepot() || other->cur_speed != 0 || other->vehstatus.Test(VehState::Stopped) ||
					!other->flags.Test(VehicleRailFlag::Stuck) || !other->current_order.IsType(OT_GOTO_STATION) ||
					other->current_order.GetDestination() != this->interchange->index) continue;
			this->winner = front;
			this->waiting = other;
			this->queued_units = this->capsule->cargo.StoredCount() + this->peer->cargo.StoredCount();
			return true;
		}
		return false;
	}

	/**
	 * Capture the actual complete physical/order/cargo fragment tuple during a paid-grid hold.
	 * @return Original participant identities,positions,orders,packet IDs/quantities and native carry;normal cargo aging remains clock-owned.
	 */
	auto HeldOwners() const
	{
		std::vector<std::tuple<VehicleID, TileIndex, int, int, int, TrackBits, VehicleOrderID, uint, uint16_t>> fronts;
		std::vector<std::pair<CargoPacketID, uint>> packets;
		// Every actual competing front and native fragment remains in custody
		// while a closed paid source cannot authorize new motion or spoil work.
		for (const Train *front : {this->capsule, this->peer}) {
			fronts.emplace_back(front->index, front->tile, front->x_pos, front->y_pos, front->z_pos, front->track,
				front->cur_real_order_index, front->cargo.StoredCount(), GetTubePowerVehicle(front->index)->remainder_mj);
			for (const auto *packet : *front->cargo.Packets()) packets.emplace_back(packet->index, packet->Count());
		}
		return std::pair{fronts, packets};
	}

	/** Close the actual original paid service,retain its historical account and demonstrate intact loaded holds without private grant or energy writes. */
	void LosePower()
	{
		this->previous_source = this->power;
		this->original_connection = *GetTubePowerConnection(this->power);
		this->Do<Commands::CloseTubePower>(this->power);
		this->Tick();
		const auto held = this->HeldOwners();
		const auto consumed = GetUtilityPowerProducer(this->power)->consumed_lifetime_j;
		// A real closed-source interval advances all normal owners while the
		// two loaded trains stay put. No elapsed tick can mint a replacement
		// grant,move the packets or silently refund settled original electricity.
		for (uint tick = 0; tick < 20; ++tick) {
			this->Tick();
			CHECK(this->HeldOwners() == held);
			CHECK(GetUtilityPowerProducer(this->power)->consumed_lifetime_j == consumed);
			this->CheckConservation(this->peer);
		}
		CHECK(GetTubePowerVehicle(this->capsule->index)->wait != TubePowerWait::None);
		CHECK(GetTubePowerVehicle(this->peer->index)->wait != TubePowerWait::None);
	}

	/** Purchase a fresh actual paid service over the retained physical corridor and explicitly reconnect both original loaded capsules without refunding the old producer. */
	void RestorePower()
	{
		const auto original_consumed = GetUtilityPowerProducer(this->previous_source)->consumed_lifetime_j;
		const auto first_carry = GetTubePowerVehicle(this->capsule->index)->remainder_mj;
		const auto second_carry = GetTubePowerVehicle(this->peer->index)->remainder_mj;
		this->Do<Commands::ConnectTubePower>(this->original_connection.portal, this->original_connection.interchange,
			TubePowerCommandPath{this->original_connection.utility_path});
		const auto connections = GetTubePowerConnections();
		/** @param connection Actual retained native physical service. @return Whether this is the freshly paid live replacement rather than retired history. */
		const auto active = [](const auto &connection) { return connection.active; };
		const auto replacement = std::ranges::find_if(connections, active);
		REQUIRE(replacement != connections.end());
		this->power = replacement->producer;
		REQUIRE(this->power != this->previous_source);
		this->Do<Commands::SetTubeVehiclePower>(this->capsule->index, this->power, true);
		this->Do<Commands::SetTubeVehiclePower>(this->peer->index, this->power, true);
		CHECK(GetTubePowerVehicle(this->capsule->index)->remainder_mj == first_carry);
		CHECK(GetTubePowerVehicle(this->peer->index)->remainder_mj == second_carry);
		CHECK(GetUtilityPowerProducer(this->previous_source)->consumed_lifetime_j == original_consumed);
		// The original conventional carrier continues its own ordinary useful
		// loop so both recovered capsule shipments can reach Surface. It never
		// changes modes or borrows any of the retired spoil machinery.
		this->Do<Commands::ModifyOrder>(this->freight->index, VehicleOrderID{3}, MOF_DEPOT_ACTION, static_cast<uint16_t>(OrderDepotAction::AlwaysGo));
		this->Do<Commands::StartStopVehicle>(this->freight->index, true);
	}
};

#endif /* TESTS_TUBE_TBM_QUEUE_FIXTURE_H */
