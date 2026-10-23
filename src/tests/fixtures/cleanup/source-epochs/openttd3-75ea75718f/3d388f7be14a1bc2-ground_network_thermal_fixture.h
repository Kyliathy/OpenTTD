/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file ground_network_thermal_fixture.h Actual agreed failure/recovery of original paid geothermal risers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Group21D.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Only ordinary network commands mutate the original paid plant; observations never issue utility grants.
 */
#ifndef TESTS_GROUND_NETWORK_THERMAL_FIXTURE_H
#define TESTS_GROUND_NETWORK_THERMAL_FIXTURE_H

#include "ground_network_active_fixture.h"
#include "../geothermal_generator.h"
#include "../geothermal_riser.h"
#include "../geothermal_service_cmd.h"
#include "../layer_portal_cmd.h"
#include "../utility_power.h"
#include "../safeguards.h"

/** Follow the exact original native plant/riser/town owners through agreed portal and service commands. */
class GroundNetworkThermalRiser {
	uint64_t original_tick = GroundNetworkUnsigned("OTTD_GROUND_NETWORK_TICK"); ///< Original paid input clock,never overwritten.
	IndustryID generator{static_cast<uint16_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_GENERATOR"))}; ///< Original operating Magma machinery.
	IndustryID vent{static_cast<uint16_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_VENT_GENERATOR"))}; ///< Original retained VentTap history.
	IndustryID first{static_cast<uint16_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_FIRST_RISER"))}; ///< Original Deep sending machinery.
	IndustryID second{static_cast<uint16_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_SECOND_RISER"))}; ///< Original Underground sending machinery.
	UtilityConsumerID destination{static_cast<uint32_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_TOWN_CONSUMER"))}; ///< Original native Surface service.
	std::optional<UtilityLinkID> old_edge, first_edge, fresh_edge; ///< Actual shared issuer identities,not predicted replacements.
	std::optional<uint64_t> closed_energy; ///< Settled original output when native closure actually executes.
	std::optional<uint64_t> closed_tick; ///< Actual retired-edge observation before the next native interval rotation.
	uint64_t retired_flow = 0, vent_service = 0, vent_energy = 0; ///< Historical owners retained through the failure.
	bool close_posted = false, open_posted = false, service_posted = false; ///< Observer-only command latches.

public:
	/** @param frame Actual native agreed frame. @param server Host posts normal existing-company commands;both peers independently observe the resulting owners. */
	void Observe(uint32_t frame, bool server)
	{
		const auto tick = TimerGameTick::counter;
		const auto *plant = GetGeothermalGeneratorState(this->generator);
		const auto *original_vent = GetGeothermalGeneratorState(this->vent);
		const auto *riser = GetGeothermalRiserState(this->second);
		const auto *town = GetUtilityPowerConsumer(this->destination);
		REQUIRE(plant != nullptr);
		REQUIRE(original_vent != nullptr);
		REQUIRE(riser != nullptr);
		REQUIRE(town != nullptr);
		const auto *output = GetUtilityPowerProducer(plant->electrical_producer);
		const auto *vent_output = GetUtilityPowerProducer(original_vent->electrical_producer);
		REQUIRE(output != nullptr);
		REQUIRE(vent_output != nullptr);
		REQUIRE(town->town_terminal != StationID::Invalid());
		REQUIRE(riser->materials_committed);
		REQUIRE_FALSE(riser->portal_retired);
		const auto offset = tick - this->original_tick;
		if (!this->old_edge.has_value()) {
			REQUIRE(offset == 1);
			for (const auto &link : CaptureUtilityPowerState().links) {
				if (!link.active || link.kind != UtilityLinkKind::ElectricalRiser) continue;
				if (link.riser == this->second) this->old_edge = link.id;
				if (link.riser == this->first) this->first_edge = link.id;
			}
			REQUIRE(this->old_edge.has_value());
			REQUIRE(this->first_edge.has_value());
			this->vent_service = original_vent->active_service_ticks;
			this->vent_energy = vent_output->consumed_lifetime_j;
		}
		CHECK(original_vent->active_service_ticks == this->vent_service);
		CHECK(vent_output->consumed_lifetime_j == this->vent_energy);
		const auto *old = GetUtilityPowerLink(*this->old_edge);
		REQUIRE(old != nullptr);
		if (!old->active && !this->closed_energy.has_value()) {
			this->closed_energy = output->consumed_lifetime_j;
			this->closed_tick = tick;
			this->retired_flow = old->flow_j;
		}
		if (this->closed_energy.has_value()) {
			CHECK_FALSE(old->active);
			// Flow belongs to the current accounting interval,including retired
			// edges. The next native BeginUtilityPowerInterval clears it;
			// lifetime spent electricity remains with the original producer.
			CHECK(old->flow_j == (tick == *this->closed_tick ? this->retired_flow : 0));
			CHECK(output->consumed_lifetime_j >= *this->closed_energy);
		}
		if (server && !this->close_posted && offset >= 32) {
			AutoRestoreBackup actor{_current_company, plant->owner};
			REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Post(riser->site.portal, false));
			this->close_posted = true;
		}
		if (server && !this->open_posted && offset >= 272) {
			REQUIRE(this->closed_energy.has_value());
			AutoRestoreBackup actor{_current_company, plant->owner};
			REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Post(riser->site.portal, true));
			this->open_posted = true;
		}
		if (server && this->open_posted && !this->service_posted && IsGeothermalRiserOperational(this->second)) {
			AutoRestoreBackup actor{_current_company, plant->owner};
			REQUIRE(Command<Commands::ConnectGeothermalTownService>::Post(this->generator, this->first, this->second,
				town->town_terminal, town->town));
			this->service_posted = true;
		}
		if (this->closed_energy.has_value() && !this->fresh_edge.has_value()) {
			for (const auto &link : CaptureUtilityPowerState().links) {
				if (link.active && link.kind == UtilityLinkKind::ElectricalRiser && link.riser == this->second) {
					REQUIRE(link.id != *this->old_edge);
					this->fresh_edge = link.id;
				}
			}
			if (!this->fresh_edge.has_value()) CHECK(output->consumed_lifetime_j == *this->closed_energy);
		}
		constexpr std::array<uint64_t, 6> samples = {1, 2, 129, 257, 385, 513};
		if (std::ranges::find(samples, offset) == samples.end()) return;
		uint64_t heat = 0;
		for (const auto &origin : output->conversion_origins) heat += origin.heat_j;
		CHECK(output->generated_lifetime_j * 100 + output->conversion_remainder == heat * output->conversion_efficiency);
		const auto *fresh = this->fresh_edge.has_value() ? GetUtilityPowerLink(*this->fresh_edge) : nullptr;
		fmt::print("GROUND_NETWORK_THERMAL role={} frame={} tick={} operational={} old_edge={} old_active={} old_flow={} first_edge={} first_active={} fresh_edge={} fresh_flow={} heat={} generated={} useful={} service={} vent_service={} vent_use={} cash={}\n",
			server ? "server" : "client", frame, tick, IsGeothermalRiserOperational(this->second) ? 1 : 0,
			this->old_edge->base(), old->active ? 1 : 0, old->flow_j, this->first_edge->base(),
			GetUtilityPowerLink(*this->first_edge)->active ? 1 : 0,
			this->fresh_edge.value_or(UtilityLinkID{UINT32_MAX}).base(), fresh != nullptr ? fresh->flow_j : 0,
			heat, output->generated_lifetime_j, output->consumed_lifetime_j, plant->active_service_ticks,
			this->vent_service, this->vent_energy, Company::Get(plant->owner)->money.base());
		std::fflush(stdout);
		if (offset == 129 || offset == 257) {
			CHECK_FALSE(IsGeothermalRiserOperational(this->second));
			REQUIRE(this->closed_energy.has_value());
			CHECK(output->consumed_lifetime_j == *this->closed_energy);
		}
		if (offset == 513) {
			REQUIRE(this->fresh_edge.has_value());
			CHECK(IsGeothermalRiserOperational(this->second));
			CHECK(output->consumed_lifetime_j > *this->closed_energy);
			CHECK(GetUtilityPowerLink(*this->first_edge)->active);
		}
	}
};

/** Observe original paid riser failure and actual recovered export over512native opportunities. */
inline void RunGroundNetworkThermalRiserPeer()
{
	GroundNetworkThermalRiser thermal;
	RunGroundNetworkPeer([&](uint32_t frame, bool server) { thermal.Observe(frame, server); }, true, true);
}

#endif /* TESTS_GROUND_NETWORK_THERMAL_FIXTURE_H */
