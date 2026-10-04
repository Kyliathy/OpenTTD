/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_utility.cpp Deterministic pure energy integration, exports and shared heat allocation. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#include "stdafx.h"
#include "multilayer_utility.h"
#include "gfx_type.h"

#include "safeguards.h"

static_assert(MILLISECONDS_PER_TICK == 27, "Utility bounds/schema must change explicitly with simulation duration");
static_assert(MAX_UTILITY_ACCOUNT_ENERGY_J <= UINT64_MAX / MAX_UTILITY_INTERVAL_ACCOUNTS);

/**
 * Integrate bounded rates while carrying fractional joules across synchronized intervals.
 * @param rate_w Watts from the producer after its one source-specific availability modifier.
 * @param economy_ticks Fixed simulation tick count, never elapsed render frames.
 * @param remainder_mj Saved previous millijoule numerator remainder.
 * @return Whole joules/remainder or refusal, without changing the live meter.
 */
std::optional<UtilityEnergyIntegration> IntegrateUtilityGeneration(uint64_t rate_w, uint32_t economy_ticks, uint16_t remainder_mj)
{
	if (rate_w > MAX_UTILITY_RATE_W) return std::nullopt;
	if (economy_ticks > MAX_UTILITY_INTERVAL_TICKS) return std::nullopt;
	if (remainder_mj >= 1000) return std::nullopt;
	const uint64_t numerator = rate_w * economy_ticks * MILLISECONDS_PER_TICK + remainder_mj;
	return UtilityEnergyIntegration{numerator / 1000, static_cast<uint16_t>(numerator % 1000)};
}

/**
 * Prepare one conserved electricity ledger in deterministic reference order.
 * @param interval New owner-issued nonzero allocation interval.
 * @param producers Existing generation/output accounts; no implicit cross-layer source.
 * @param consumers Existing demands with validated local network eligibility.
 * @param exports Existing commissioned electrical links; requests already respect link capacity.
 * @return Pure atomic proposal, or an invalid empty plan on bad input.
 */
UtilityAllocationPlan PlanUtilityAllocation(uint64_t interval, std::span<const UtilityProducerSnapshot> producers, std::span<const UtilityConsumerDemand> consumers, std::span<const UtilityElectricalExport> exports)
{
	if (interval == 0) return {};
	if (interval == UINT64_MAX) return {};
	if (producers.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return {};
	if (consumers.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return {};
	if (exports.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return {};
	UtilityAllocationPlan plan;
	plan.interval = interval;
	// Snapshot each real output account once; allocation maps below are temporary indexes, never another registry.
	for (const auto &producer : producers) {
		if (producer.id.base() == UINT32_MAX) return {};
		if (to_underlying(producer.layer) >= MAX_MAP_LAYERS) return {};
		if (producer.generated_j > MAX_UTILITY_ACCOUNT_ENERGY_J) return {};
		plan.producers.push_back({producer.id, producer.layer, producer.generated_j, 0, 0, 0, producer.generated_j});
	}
	std::ranges::sort(plan.producers, {}, &UtilityProducerBalance::id);
	std::map<UtilityProducerID, size_t> source_index;
	for (size_t i = 0; i < plan.producers.size(); ++i) {
		if (!source_index.emplace(plan.producers[i].id, i).second) return {};
	}
	std::vector<UtilityConsumerDemand> ordered_consumers(consumers.begin(), consumers.end());
	std::ranges::sort(ordered_consumers, {}, &UtilityConsumerDemand::id);
	std::set<UtilityConsumerID> consumer_ids;
	// Validate the whole eligibility snapshot before planning any debit, including zero-demand consumers.
	for (const auto &consumer : ordered_consumers) {
		if (consumer.id.base() == UINT32_MAX) return {};
		if (!consumer_ids.insert(consumer.id).second) return {};
		if (to_underlying(consumer.layer) >= MAX_MAP_LAYERS) return {};
		if (consumer.demand_j > MAX_UTILITY_ACCOUNT_ENERGY_J) return {};
		if (consumer.linked_producers.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return {};
		std::set<UtilityProducerID> links;
		for (const auto id : consumer.linked_producers) {
			if (!links.insert(id).second) return {};
			const auto found = source_index.find(id);
			if (found == source_index.end()) return {};
			if (plan.producers[found->second].layer != consumer.layer) return {};
		}
	}
	std::vector<UtilityElectricalExport> ordered_exports(exports.begin(), exports.end());
	std::ranges::sort(ordered_exports, {}, &UtilityElectricalExport::id);
	std::set<UtilityLinkID> export_ids;
	// Commissioned links are explicit source/destination references; coordinate overlap cannot grant this eligibility.
	for (const auto &link : ordered_exports) {
		if (link.id.base() == UINT32_MAX) return {};
		if (!export_ids.insert(link.id).second) return {};
		if (link.requested_j > MAX_UTILITY_ACCOUNT_ENERGY_J) return {};
		const auto source = source_index.find(link.source);
		const auto destination = source_index.find(link.destination);
		if (source == source_index.end()) return {};
		if (destination == source_index.end()) return {};
		if (plan.producers[source->second].layer == plan.producers[destination->second].layer) return {};
	}
	// Reserve exports before local demands. Only this interval's original generation may be exported; imports cannot circulate.
	for (const auto &link : ordered_exports) {
		auto &source = plan.producers[source_index.at(link.source)];
		auto &destination = plan.producers[source_index.at(link.destination)];
		const uint64_t flow = std::min(link.requested_j, source.generated_j - source.exported_j);
		source.exported_j += flow;
		source.unused_j -= flow;
		destination.imported_j += flow;
		destination.unused_j += flow;
		plan.links.push_back({link.id, flow});
	}
	// Allocate once by consumer ID, then source ID. Every grant debits its eligible layer-local output balance immediately.
	for (const auto &consumer : ordered_consumers) {
		uint64_t remaining = consumer.demand_j;
		std::vector<UtilityProducerID> links(consumer.linked_producers.begin(), consumer.linked_producers.end());
		std::ranges::sort(links);
		for (const auto id : links) {
			auto &source = plan.producers[source_index.at(id)];
			const uint64_t grant = std::min(remaining, source.unused_j);
			source.unused_j -= grant;
			source.consumed_j += grant;
			remaining -= grant;
		}
		plan.consumers.push_back({consumer.id, consumer.demand_j, consumer.demand_j - remaining});
	}
	plan.valid = true;
	return plan;
}

/**
 * Divide a single vent/network's validated Deep-local heat before any conversion or direct recipe.
 * @param available_j Thermal joules actually supplied in this interval.
 * @param consumers Eligible requests sharing this source, with stable distinct IDs.
 * @return Ordered shares bounded by the one thermal source, or invalid-input refusal.
 */
std::optional<std::vector<UtilityHeatGrant>> PlanVentHeatShares(uint64_t available_j, std::span<const UtilityHeatDemand> consumers)
{
	if (available_j > MAX_UTILITY_ACCOUNT_ENERGY_J) return std::nullopt;
	if (consumers.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return std::nullopt;
	std::vector<UtilityHeatDemand> ordered(consumers.begin(), consumers.end());
	std::ranges::sort(ordered, {}, &UtilityHeatDemand::id);
	std::set<UtilityConsumerID> identities;
	std::vector<UtilityHeatGrant> result;
	// Direct heat and generator shares compete in the same stable order; a generator never converts the full vent again.
	for (const auto &consumer : ordered) {
		if (consumer.id.base() == UINT32_MAX) return std::nullopt;
		if (!identities.insert(consumer.id).second) return std::nullopt;
		if (consumer.requested_j > MAX_UTILITY_ACCOUNT_ENERGY_J) return std::nullopt;
		if (to_underlying(consumer.use) > to_underlying(UtilityHeatUse::ElectricalGenerator)) return std::nullopt;
		const uint64_t grant = std::min(available_j, consumer.requested_j);
		available_j -= grant;
		result.push_back({consumer.id, consumer.use, grant});
	}
	return result;
}

/**
 * Convert a granted generator heat share with a saved fractional efficiency remainder.
 * @param heat_j Granted thermal joules, never another consumer's heat.
 * @param efficiency_percent Model's declared 0..100 percent conversion.
 * @param percent_remainder Prior numerator remainder, 0..99.
 * @return Electrical output and new remainder, or invalid-input refusal.
 */
std::optional<UtilityHeatConversion> ConvertUtilityHeatToElectricity(uint64_t heat_j, uint8_t efficiency_percent, uint8_t percent_remainder)
{
	if (heat_j > MAX_UTILITY_ACCOUNT_ENERGY_J) return std::nullopt;
	if (efficiency_percent > 100) return std::nullopt;
	if (percent_remainder >= 100) return std::nullopt;
	const uint64_t numerator = heat_j * efficiency_percent + percent_remainder;
	return UtilityHeatConversion{numerator / 100, static_cast<uint8_t>(numerator % 100)};
}

/**
 * Restrict the one solar modifier to actual Surface solar generation.
 * @param kind Producer's generation family.
 * @param layer Producer's authoritative output layer.
 * @return Whether the solar owner may consider a validated mirror assignment.
 */
bool IsUtilityMirrorEligible(UtilityGenerationKind kind, MapLayer layer)
{
	// Only a Surface farm can receive the assigned mirror; heat conversion and orbital panels keep their own output.
	return kind == UtilityGenerationKind::SolarFarm && layer == MapLayer::Surface;
}

static constexpr UtilityConsumerEffectContract _consumer_effects[] = {
	{UtilityConsumerEffect::ProcessorBoost, 50000, false, 25},
	{UtilityConsumerEffect::TubeWork, 20000, true, 0},
	{UtilityConsumerEffect::AssemblyWork, 30000, true, 0},
	{UtilityConsumerEffect::TownGrowth, 5000, false, 25},
};

/**
 * Publish useful-power response caps while leaving commands and facility timers with their feature owners.
 * @return Immutable native demand/effect definitions.
 */
std::span<const UtilityConsumerEffectContract> GetUtilityConsumerEffects()
{
	return _consumer_effects;
}

/**
 * Compute a native-only bounded growth benefit from actual electricity use.
 * @param authority Selected exclusive town-effect owner.
 * @param granted_j Actual bounded grant for this eligible town interval.
 * @param demand_j Actual bounded request; absent demand gives no effect.
 * @return Capped benefit, or refusal if a Game Script/invalid authority owns it.
 */
std::optional<uint8_t> ComputeNativeTownPowerBonus(MultiLayerTownPowerAuthority authority, uint64_t granted_j, uint64_t demand_j)
{
	if (authority != MultiLayerTownPowerAuthority::Native) return std::nullopt;
	if (demand_j > MAX_UTILITY_ACCOUNT_ENERGY_J) return std::nullopt;
	if (granted_j > demand_j) return std::nullopt;
	if (demand_j == 0) return uint8_t{0};
	return static_cast<uint8_t>(granted_j * 25 / demand_j);
}
