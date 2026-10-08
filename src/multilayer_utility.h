/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_utility.h Shared utility snapshot/allocation contract with checked integer units. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#ifndef MULTILAYER_UTILITY_H
#define MULTILAYER_UTILITY_H

#include "multilayer_content.h"

/** Stable electrical link identity allocated by the shared utility owner, not feature-local registries. */
using UtilityLinkID = StrongType::Typedef<uint32_t, struct UtilityLinkIDTag, StrongType::Compare>;

static constexpr uint64_t MAX_UTILITY_RATE_W = 1000000000; ///< Checked one-account generation-rate bound.
static constexpr uint32_t MAX_UTILITY_INTERVAL_TICKS = 1000000; ///< Checked synchronized duration bound.
static constexpr uint64_t MAX_UTILITY_ACCOUNT_ENERGY_J = 27000000000000; ///< Rate/duration ceiling at the fixed 27-ms tick.
static constexpr size_t MAX_UTILITY_INTERVAL_ACCOUNTS = 4096; ///< Producer/consumer/link list ceiling per planning pass.

/** Checked rate integration; millijoules are retained rather than discarded on small intervals. */
struct UtilityEnergyIntegration {
	uint64_t energy_j = 0; ///< Whole generated joules.
	uint16_t remainder_mj = 0; ///< Saved numerator remainder, 0..999.
};

/** One authoritative producer's available electrical energy snapshot, without a new runtime registry. */
struct UtilityProducerSnapshot {
	UtilityProducerID id; ///< Stable reference supplied by the shared utility owner.
	MapLayer layer; ///< Explicit output layer; no coordinate/catchment inference.
	uint64_t generated_j; ///< This interval's generation, before local consumption and exports.
};

/** Eligible local electricity demand; connectivity is validated by the owning utility network. */
struct UtilityConsumerDemand {
	UtilityConsumerID id; ///< Stable consumer reference, distinct from producer IDs.
	MapLayer layer; ///< Every linked producer must output to this layer.
	std::span<const UtilityProducerID> linked_producers; ///< Explicit eligible sources; duplicates/absent/wrong-layer references refuse.
	uint64_t demand_j; ///< Bounded interval demand, never paid freight.
};

/** Commissioned directed inter-layer electrical link; source generation is debited exactly once. */
struct UtilityElectricalExport {
	UtilityLinkID id; ///< Stable link order; link construction/ownership validation belongs to its feature command.
	UtilityProducerID source; ///< Existing source account supplying its own generation.
	UtilityProducerID destination; ///< Existing destination-layer output account credited with imported energy.
	uint64_t requested_j; ///< Desired interval flow, bounded by source generation and commissioned link capacity.
};

/** Conserved per-producer ledger, suitable for one revalidated atomic commit. */
struct UtilityProducerBalance {
	UtilityProducerID id; ///< Stable sorted source/output reference.
	MapLayer layer; ///< Output layer.
	uint64_t generated_j; ///< Snapshot generation.
	uint64_t imported_j = 0; ///< Credits from explicit links, once per flow.
	uint64_t exported_j = 0; ///< Debits to explicit links, once per flow.
	uint64_t consumed_j = 0; ///< Granted local use.
	uint64_t unused_j = 0; ///< Unconsumed energy; curtail at interval end unless a separately bounded store owns it.
};

/** Exact source contribution to one proposed grant; runtime settlement retains this lineage rather than guessing it from totals. */
struct UtilitySourceShare {
	UtilityProducerID producer; ///< Local output account whose interval availability is reserved; explicit plan links retain any import origin.
	uint64_t energy_j; ///< Whole allocated quantity from this source, never a second generation event.
};

/** Granted demand, distinct from requested or generated power. */
struct UtilityConsumerGrant {
	UtilityConsumerID id; ///< Stable sorted consumer reference.
	uint64_t demand_j; ///< Original requested energy.
	uint64_t granted_j; ///< Actual grant; unmet demand is demand minus grant.
	std::vector<UtilitySourceShare> sources; ///< Exact deterministic source slices; their sum equals granted_j.
};

/** Exactly one conserved debit/credit for a commissioned link. */
struct UtilityLinkFlow {
	UtilityLinkID id; ///< Stable link reference.
	uint64_t energy_j; ///< Granted flow, not a second generation event.
};

/** Pure allocation proposal; no timer, money, stock or live account is changed by planning. */
struct UtilityAllocationPlan {
	bool valid = false; ///< False authorizes no commit; invalid input returns an empty proposal.
	uint64_t interval = 0; ///< Owning ledger must refuse stale/repeated/terminal intervals before commit.
	std::vector<UtilityProducerBalance> producers; ///< Stable producer order and conserved balances.
	std::vector<UtilityConsumerGrant> consumers; ///< Stable consumer order and actual grants.
	std::vector<UtilityLinkFlow> links; ///< Stable link order and debit/credit quantities.
};

/** Direct heat and generation compete for one renewable but rate-limited vent budget. */
enum class UtilityHeatUse : uint8_t { DirectRecipe, ElectricalGenerator };

/** One heat request; connectivity is Deep-local and supplied by the vent/network owner. */
struct UtilityHeatDemand {
	UtilityConsumerID id; ///< Stable linked consumer reference.
	UtilityHeatUse use; ///< Direct recipe versus conversion, not a second copy of the source heat.
	uint64_t requested_j; ///< Interval thermal joules.
};

/** One consumer's granted share of the vent, allocated before electrical conversion. */
struct UtilityHeatGrant {
	UtilityConsumerID id; ///< Stable sorted consumer reference.
	UtilityHeatUse use; ///< Consumer's declared thermal role.
	uint64_t granted_j; ///< At most its request and the vent's remaining share.
};

/** Checked heat-to-electric conversion with a distinct fractional-percent remainder. */
struct UtilityHeatConversion {
	uint64_t electric_j; ///< Whole electrical joules, at most granted heat.
	uint8_t percent_remainder; ///< Saved numerator remainder 0..99.
};

/** Source family before its one source-specific availability/mirror calculation. */
enum class UtilityGenerationKind : uint8_t { SolarFarm, Geothermal, Nuclear, Coal, External, OrbitalPanel };

/** Native consumer response; optional Surface processing does not block the pre-grid manufacturing bootstrap. */
enum class UtilityConsumerEffect : uint8_t {
	ProcessorBoost, TubeWork, AssemblyWork, TownGrowth,
	HeatDirectRecipe, ///< Appended immutable thermal role; electrical work/effect APIs refuse this consumer.
	HeatGenerator, ///< Appended heat-to-electric conversion role; only actually granted/settled thermal shares may fund conversion.
};

/** Bounded effect definition, with actual grants from the one utility ledger. */
struct UtilityConsumerEffectContract {
	UtilityConsumerEffect effect; ///< Feature owner's declared response to electrical grants.
	uint32_t demand_rate_w; ///< Maximum request rate before per-interval checked integration.
	bool required_for_work; ///< Missing energy pauses work only for mandatory consumers.
	uint8_t maximum_bonus_percent; ///< Optional benefit cap; no cash/freight reward.
};

/**
 * Check source eligibility before the solar owner applies its single modifier.
 * @param kind Actual generation family; geothermal heat conversion cannot masquerade as solar.
 * @param layer Actual producer output layer, independent of camera selection.
 * @return True only for a Surface solar farm; assignment/availability validation remains with the solar owner.
 */
bool IsUtilityMirrorEligible(UtilityGenerationKind kind, MapLayer layer);

/**
 * Read bounded native consumer effects without changing facilities or town growth.
 * @return Immutable demand/pause/bonus contracts for feature owners.
 */
std::span<const UtilityConsumerEffectContract> GetUtilityConsumerEffects();

/**
 * Compute one capped native town bonus, refusing an adapter-owned effect.
 * @param authority Selected profile's exclusive Surface town-effect authority.
 * @param granted_j Actual granted interval electricity, at most requested demand.
 * @param demand_j Positive eligible demand; zero demand has zero benefit.
 * @return Bonus in 0..25 percent, or no result for non-native authority/invalid quantities.
 */
std::optional<uint8_t> ComputeNativeTownPowerBonus(MultiLayerTownPowerAuthority authority, uint64_t granted_j, uint64_t demand_j);

/**
 * Integrate one generation rate using fixed simulation ticks, independent of rendering/wall time.
 * @param rate_w Bounded nonnegative generation rate, in watts.
 * @param economy_ticks Synchronized elapsed ticks, at most MAX_UTILITY_INTERVAL_TICKS.
 * @param remainder_mj Prior saved fractional generation, 0..999 millijoules.
 * @return Whole joules/new remainder, or no result for invalid/overflowing input.
 */
std::optional<UtilityEnergyIntegration> IntegrateUtilityGeneration(uint64_t rate_w, uint32_t economy_ticks, uint16_t remainder_mj);

/**
 * Plan explicit exports then linked local grants in stable ID order, without live mutation.
 * @param interval New nonzero accounting interval; commit owner checks it has not already run.
 * @param producers Actual bounded generation/output snapshots.
 * @param consumers Actual bounded demands and explicit local source eligibility.
 * @param exports Validated commissioned inter-layer links; imported energy is not re-exported this interval.
 * @return Conserved sorted proposal, or an invalid empty plan on any bad identity/layer/bound.
 */
UtilityAllocationPlan PlanUtilityAllocation(uint64_t interval, std::span<const UtilityProducerSnapshot> producers, std::span<const UtilityConsumerDemand> consumers, std::span<const UtilityElectricalExport> exports);

/**
 * Divide one Deep vent's finite interval heat before recipes or generators consume it.
 * @param available_j Validated vent/network owner's thermal interval budget.
 * @param consumers Explicitly linked direct-heat/generation requests; sorted by stable consumer ID.
 * @return Granted shares whose sum never exceeds heat, or no result for invalid input.
 */
std::optional<std::vector<UtilityHeatGrant>> PlanVentHeatShares(uint64_t available_j, std::span<const UtilityHeatDemand> consumers);

/**
 * Convert a previously granted thermal share once, never the entire vent budget again.
 * @param heat_j Granted generator share, bounded by the shared heat plan.
 * @param efficiency_percent Integer efficiency in 0..100; model availability is a calendar policy.
 * @param percent_remainder Prior saved conversion numerator remainder, 0..99.
 * @return Electrical joules/new remainder, or no result for invalid bounds.
 */
std::optional<UtilityHeatConversion> ConvertUtilityHeatToElectricity(uint64_t heat_j, uint8_t efficiency_percent, uint8_t percent_remainder);

#endif /* MULTILAYER_UTILITY_H */
