/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file ground_network_recovery_fixture.h Real paid replacement/reconnection and original finite-cargo recovery.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Group21C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Exact broken originals continue through normal commands and the actual native peer game loop.
 */
#ifndef TESTS_GROUND_NETWORK_RECOVERY_FIXTURE_H
#define TESTS_GROUND_NETWORK_RECOVERY_FIXTURE_H

#include "ground_network_power_fixture.h"
#include "../order_cmd.h"
#include "../depot_base.h"
#include "../multilayer_industry.h"

/** Observe original broken paid owners,post only native recovery commands and retain finite cargo accounting. */
class GroundNetworkPowerRecovery {
	UtilityProducerID old_source{static_cast<uint32_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_POWER"))}; ///< Retired historical service from21B.
	std::array<VehicleID, 2> capsules{VehicleID{static_cast<uint32_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_CAPSULE"))},
		VehicleID{static_cast<uint32_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_PEER"))}}; ///< Same original two loaded hulls.
	VehicleID freight{static_cast<uint32_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_FREIGHT"))}; ///< Original conventional train,never replaced.
	IndustryID mine{static_cast<uint16_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_MINE"))}; ///< Original finite Coal mine.
	IndustryID factory{static_cast<uint16_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_FACTORY"))}; ///< Original useful Surface receiver.
	uint64_t original_tick = GroundNetworkUnsigned("OTTD_GROUND_NETWORK_TICK"); ///< Exact broken-world input clock.
	bool connect_posted = false; ///< Observer-only native command latch.
	bool reconnect_posted = false; ///< Observer-only explicit binding/freight command latch.
	std::optional<UtilityProducerID> fresh; ///< Actual ledger-issued replacement observed after public command execution.
	std::optional<uint64_t> old_consumed; ///< Original settled lifetime electricity,never reset or refunded.
	uint64_t old_invoice = 0; ///< Original settled paid service history.
	uint64_t initial_intake = 0; ///< Original useful receiver's already accepted Coal.
	uint64_t opening_reserve = 0; ///< Original finite balance after paid excavation losses.

public:
	/**
	 * Continue actual native paid service and original physical cargo;no simulation owner is called here.
	 * @param frame Actual agreed network frame after command execution.
	 * @param server Real host role,whose existing-company commands travel through both native agreed queues.
	 */
	void Observe(uint32_t frame, bool server)
	{
		const auto tick = TimerGameTick::counter;
		const auto cargo = GetCargoTypeByLabel(CargoLabel{"COAL"});
		const auto *connection = GetTubePowerConnection(this->old_source);
		const auto *old_meter = GetUtilityPowerProducer(this->old_source);
		const auto *mine_state = GetLayerMineState(this->mine);
		REQUIRE(connection != nullptr);
		REQUIRE(old_meter != nullptr);
		REQUIRE(mine_state != nullptr);
		REQUIRE_FALSE(connection->active);
		REQUIRE_FALSE(old_meter->active);
		REQUIRE(connection->owner == CompanyID{0});
		const auto *deposit = GetGeologyWorldState().deposits.Get(mine_state->deposit);
		REQUIRE(deposit != nullptr);
		const auto receiver = Industry::Get(this->factory)->GetCargoAccepted(cargo);
		REQUIRE(receiver != Industry::Get(this->factory)->accepted.end());
		REQUIRE(GetTileLayer(Industry::Get(this->factory)->location.tile) == MapLayer::Surface);
		if (!this->old_consumed.has_value()) {
			REQUIRE(tick == this->original_tick + 1);
			this->old_consumed = old_meter->consumed_lifetime_j;
			this->old_invoice = connection->invoiced_money;
			this->initial_intake = receiver->waiting;
			this->opening_reserve = deposit->remaining_reserve + mine_state->extracted;
			REQUIRE(Train::Get(this->freight)->vehstatus.Test(VehState::Stopped));
		}
		CHECK(old_meter->consumed_lifetime_j == *this->old_consumed);
		CHECK(connection->invoiced_money == this->old_invoice);
		// Purchase one fresh service over the original paid physical corridor;
		// the native command alone checks topology,allocates identity and bills.
		if (server && !this->connect_posted && tick >= this->original_tick + 32) {
			AutoRestoreBackup actor{_current_company, connection->owner};
			REQUIRE(Command<Commands::ConnectTubePower>::Post(connection->portal, connection->interchange,
				TubePowerCommandPath{connection->utility_path}));
			this->connect_posted = true;
		}
		// Observe actual native publication rather than predicting/reusing a
		// pool ID. No new source can inherit the retired historical account.
		for (const auto &candidate : GetTubePowerConnections()) {
			if (!candidate.active) continue;
			REQUIRE(candidate.producer != this->old_source);
			REQUIRE(candidate.portal == connection->portal);
			REQUIRE(candidate.interchange == connection->interchange);
			if (!this->fresh.has_value()) this->fresh = candidate.producer;
			REQUIRE(candidate.producer == *this->fresh);
		}
		// Both original loaded hulls need an explicit ordinary reconnect;the
		// same existing freight resumes its retained physical Surface route.
		if (server && this->fresh.has_value() && !this->reconnect_posted) {
			AutoRestoreBackup actor{_current_company, connection->owner};
			for (const auto id : this->capsules) REQUIRE(Command<Commands::SetTubeVehiclePower>::Post(id, *this->fresh, true));
			const auto tile = Train::Get(this->freight)->tile;
			REQUIRE(Command<Commands::ModifyOrder>::Post(tile, this->freight, VehicleOrderID{3}, MOF_DEPOT_ACTION, static_cast<uint16_t>(OrderDepotAction::AlwaysGo)));
			REQUIRE(Command<Commands::StartStopVehicle>::Post(tile, this->freight, true));
			this->reconnect_posted = true;
		}
		const auto *first = Train::Get(this->capsules[0]);
		const auto *second = Train::Get(this->capsules[1]);
		const auto *wagon = Train::Get(this->freight)->Next();
		REQUIRE(wagon != nullptr);
		const auto *origin = Station::Get(first->GetOrder(0)->GetDestination().ToStationID());
		const auto *interchange = Station::Get(first->GetOrder(1)->GetDestination().ToStationID());
		const auto physical = Industry::Get(this->mine)->produced.front().waiting + origin->goods[cargo].TotalCount() +
			first->cargo.StoredCount() + second->cargo.StoredCount() + interchange->goods[cargo].TotalCount() +
			wagon->cargo.StoredCount() + receiver->waiting;
		CHECK(mine_state->extracted == physical);
		CHECK(this->opening_reserve == deposit->remaining_reserve + mine_state->extracted);
		constexpr std::array<uint64_t, 6> offsets = {1, 2, 129, 1025, 2049, 3073};
		if (std::ranges::find(offsets, tick - this->original_tick) == offsets.end()) return;
		bool home = true;
		// Whole original return to each retained order's home depot proves
		// physical recovery beyond merely clearing a power/PBS wait reason.
		for (const auto *front : {first, second}) {
			const auto *depot = Depot::Get(front->GetOrder(3)->GetDestination().ToDepotID());
			home &= front->IsChainInDepot() && front->tile == depot->xy && front->cargo.StoredCount() == 0;
		}
		const auto energy = this->fresh.has_value() ? GetUtilityPowerProducer(*this->fresh)->consumed_lifetime_j : 0;
		fmt::print("GROUND_NETWORK_POWER_RECOVERY role={} frame={} tick={} old_source={} fresh_source={} old_energy={} fresh_energy={} intake={} initial_intake={} extracted={} remaining={} physical={} home={} cash={}\n",
			server ? "server" : "client", frame, tick, this->old_source.base(), this->fresh.value_or(UtilityProducerID{UINT32_MAX}).base(),
			*this->old_consumed, energy, receiver->waiting, this->initial_intake, mine_state->extracted,
			deposit->remaining_reserve, physical, home ? 1 : 0, Company::Get(connection->owner)->money.base());
		std::fflush(stdout);
		if (tick == this->original_tick + 3073) {
			REQUIRE(this->fresh.has_value());
			CHECK(energy > 0);
			CHECK(receiver->waiting >= this->initial_intake + 120);
			CHECK(home);
		}
	}
};

/** Run one bounded real native paid-recovery peer,with complete snapshots through3072actual opportunities. */
inline void RunGroundNetworkPowerRecoveryPeer()
{
	GroundNetworkPowerRecovery recovery;
	/** @param frame Actual agreed native frame. @param server Real host role;commands keep original company ownership. */
	RunGroundNetworkPeer([&](uint32_t frame, bool server) { recovery.Observe(frame, server); }, true, true, {128, 1024, 2048, 3072});
}

#endif /* TESTS_GROUND_NETWORK_RECOVERY_FIXTURE_H */
