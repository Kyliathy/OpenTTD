/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file ground_network_power_fixture.h Actual agreed paid tube-power closure and original loaded holds.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Group21B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Observers retain original native references;only the ordinary synchronized closure command mutates gameplay.
 */
#ifndef TESTS_GROUND_NETWORK_POWER_FIXTURE_H
#define TESTS_GROUND_NETWORK_POWER_FIXTURE_H

#include "ground_network_active_fixture.h"
#include "../transport_tube.h"
#include "../transport_tube_cmd.h"
#include "../utility_power.h"

/** Complete original physical/packet/carry custody while a retired source cannot authorize motion. */
struct GroundNetworkPowerHeld {
	std::vector<std::tuple<VehicleID, TileIndex, int, int, int, TrackBits, VehicleOrderID, uint>> parts; ///< Every actual original consist part and order position.
	std::vector<std::pair<CargoPacketID, uint>> packets; ///< Real packet IDs/counts;normal cargo aging remains clock-owned.
	std::array<uint16_t, 2> carry{}; ///< Existing front-owned milli-joule fractions,not a second energy account.
	/** @param other Another read-only custody observation. @return Whether all original physical owners remain exact. */
	bool operator==(const GroundNetworkPowerHeld &other) const = default;
};

/** Post one real company-authorized closure,then observe original native physical custody and settled history. */
class GroundNetworkPowerLoss {
	UtilityProducerID source{static_cast<uint32_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_POWER"))}; ///< Exact original paid ledger identity.
	std::array<VehicleID, 2> capsules{VehicleID{static_cast<uint32_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_CAPSULE"))},
		VehicleID{static_cast<uint32_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_PEER"))}}; ///< Original two paid loaded hulls.
	uint64_t original_tick = GroundNetworkUnsigned("OTTD_GROUND_NETWORK_TICK"); ///< Sealed original clock,never changed here.
	bool posted = false; ///< Observer-only command scheduling latch.
	std::optional<uint64_t> closed_tick; ///< Actual first callback after agreed source retirement.
	std::optional<GroundNetworkPowerHeld> held; ///< Actual custody after its first native closed-source step.
	uint64_t consumed = 0; ///< Read-only historical native joules at retirement.
	uint64_t invoice = 0; ///< Read-only settled original lease/usage/rounding invoice.

	/** @return Every actual original part,packet and carry without changing cargo aging or physical state. */
	GroundNetworkPowerHeld Capture() const
	{
		GroundNetworkPowerHeld result;
		// Follow both original complete chains so an articulated tail or actual
		// fragment cannot disappear behind an unchanged front/aggregate count.
		for (size_t index = 0; index < this->capsules.size(); ++index) {
			const auto *power = GetTubePowerVehicle(this->capsules[index]);
			REQUIRE(power != nullptr);
			REQUIRE(power->source == this->source);
			result.carry[index] = power->remainder_mj;
			for (const Train *part = Train::Get(this->capsules[index]); part != nullptr; part = part->Next()) {
				result.parts.emplace_back(part->index, part->tile, part->x_pos, part->y_pos, part->z_pos,
					part->track, part->cur_real_order_index, part->cargo.StoredCount());
				// Actual packets stay with their original part throughout loss;
				// neither an aggregate reconstruction nor a copied hold is accepted.
				for (const auto *packet : *part->cargo.Packets()) result.packets.emplace_back(packet->index, packet->Count());
			}
		}
		return result;
	}

public:
	/**
	 * Observe the real application opportunity and post closure only as the original existing company.
	 * @param frame Actual agreed network frame,retained alongside gameplay ticks.
	 * @param server True for the invocation-owned dedicated host;clients never fabricate company authority.
	 */
	void Observe(uint32_t frame, bool server)
	{
		const auto tick = TimerGameTick::counter;
		const auto *connection = GetTubePowerConnection(this->source);
		const auto *producer = GetUtilityPowerProducer(this->source);
		REQUIRE(connection != nullptr);
		REQUIRE(producer != nullptr);
		REQUIRE(connection->owner == CompanyID{0});
		// Allow the original real queue to enter the shared loop,then send the
		// ordinary command through server authority and both agreed queues once.
		// Only the authoritative host posts this closure once after the initial loaded custody samples.
		if (server && !this->posted && tick >= this->original_tick + 32) {
			AutoRestoreBackup actor{_current_company, connection->owner};
			REQUIRE(Command<Commands::CloseTubePower>::Post(this->source));
			this->posted = true;
		}
		if (connection->active) return;
		if (!this->closed_tick.has_value()) {
			this->closed_tick = tick;
			this->consumed = producer->consumed_lifetime_j;
			this->invoice = connection->invoiced_money;
			fmt::print("GROUND_NETWORK_POWER_CLOSED role={} frame={} tick={} source={} consumed={} invoice={}\n",
				server ? "server" : "client", frame, tick, this->source.base(), this->consumed, this->invoice);
			std::fflush(stdout);
		}
		if (tick == *this->closed_tick) return;
		if (!this->held.has_value()) this->held = this->Capture();
		CHECK(this->Capture() == *this->held);
		CHECK(producer->consumed_lifetime_j == this->consumed);
		CHECK(connection->invoiced_money == this->invoice);
		CHECK(connection->usage_remainder_j == 0);
		// Once the ordinary invalidation step has run,both original hulls
		// explicitly wait for power while retaining all physical custody.
		for (const auto id : this->capsules) CHECK(GetTubePowerVehicle(id)->wait != TubePowerWait::None);
		constexpr std::array<uint64_t, 4> offsets = {129, 257, 385, 513};
		if (std::ranges::find(offsets, tick - this->original_tick) == offsets.end()) return;
		uint cargo = 0;
		for (const auto &packet : this->held->packets) cargo += packet.second;
		fmt::print("GROUND_NETWORK_POWER_HELD role={} frame={} tick={} source={} consumed={} invoice={} parts={} packets={} cargo={} carry0={} carry1={}\n",
			server ? "server" : "client", frame, tick, this->source.base(), this->consumed, this->invoice,
			this->held->parts.size(), this->held->packets.size(), cargo, this->held->carry[0], this->held->carry[1]);
		std::fflush(stdout);
	}
};

/** Run a real native peer with original paid tube-source loss and shared complete opportunity observations. */
inline void RunGroundNetworkPowerLossPeer()
{
	GroundNetworkPowerLoss loss;
	/** @param frame Actual agreed native frame. @param server Real host role;only its ordinary command changes state. */
	RunGroundNetworkPeer([&](uint32_t frame, bool server) { loss.Observe(frame, server); }, true, true);
}

#endif /* TESTS_GROUND_NETWORK_POWER_FIXTURE_H */
