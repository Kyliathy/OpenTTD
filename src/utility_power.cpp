/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_power.cpp Native owner of bounded electrical publication and deterministic interval reservations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group7
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-g-content-freeze.md utility units and single authority
 */

#include "stdafx.h"
#include "utility_power.h"
#include "company_base.h"
#include "company_func.h"
#include "map_layer.h"
#include "timer/timer_game_calendar.h"
#include "core/backup_type.hpp"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "town.h"
#include "town_cmd.h"
#include <set>

#include "safeguards.h"

static UtilityPowerState _utility_power; ///< Sole runtime source/consumer/epoch authority; never a render-owned ledger.
static std::optional<UtilityPowerState> _pending_utility_power; ///< Unaccepted file transport, never another live account owner.
static bool _utility_mutation_committing = false; ///< Whole participant commits cannot recursively change their own prepared ledger.

/**
 * Validate the native owner and explicit layer before account creation/publication.
 * @param owner Actual asking/owning company.
 * @param layer Actual allocated output/consumption layer.
 * @param check_actor Whether this company-scoped operation must match the caller; global allocation uses its explicit system authority.
 * @return Specific shared policy refusal, without changing accepted content/calendar state.
 */
static UtilityPowerError CheckUtilityPowerOwner(CompanyID owner, MapLayer layer, bool check_actor = true)
{
	if (_utility_mutation_committing) return UtilityPowerError::Refused;
	if (_utility_power.operation_sequence >= UINT64_MAX - 1) return UtilityPowerError::Limit;
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return UtilityPowerError::UnsupportedProfile;
	// A caller cannot register/publish another company's account, even when
	// producers occupy the same coordinates or are observed through an extra view.
	if (!Company::IsValidID(owner) || (check_actor && owner != _current_company)) return UtilityPowerError::InvalidOwner;
	if (!MultiLayerMap::IsValidLayer(layer)) return UtilityPowerError::InvalidLayer;
	// Deep electricity must obey the same actual saved calendar policy as its
	// physical producer; viewing a locked layer cannot authorize a meter there.
	if (layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return UtilityPowerError::InvalidLayer;
	return UtilityPowerError::None;
}

/**
 * Find a source in stable owner order, without creating a missing account.
 * @param id Exact ledger-allocated source ID.
 * @return Mutable owner pointer for a revalidated ledger operation, or nullptr.
 */
static UtilityPowerProducer *FindUtilityProducer(UtilityProducerID id)
{
	// This owner retains dense allocated identities, including tombstones;
	// direct indexing bounds per-action work without scanning all generators.
	if (id.base() >= _utility_power.producers.size()) return nullptr;
	return &_utility_power.producers[id.base()];
}

/**
 * Find a consumer in stable owner order, without manufacturing a grant.
 * @param id Exact ledger-allocated consumer ID.
 * @return Mutable owner pointer for a revalidated ledger operation, or nullptr.
 */
static UtilityPowerConsumer *FindUtilityConsumer(UtilityConsumerID id)
{
	if (id.base() >= _utility_power.consumers.size()) return nullptr;
	return &_utility_power.consumers[id.base()];
}

/** @param id Ledger-issued directed edge identity. @return Its retained owner record, or absence for an unallocated ID. */
static UtilityPowerLink *FindUtilityLink(UtilityLinkID id)
{
	if (id.base() >= _utility_power.links.size()) return nullptr;
	return &_utility_power.links[id.base()];
}

/** @param share Exact checked original reservation slice. @return Remaining usable quantity after settlement, expiry and outgoing transfer. */
static uint64_t FreeUtilityShare(const UtilityPowerShare &share)
{
	return share.allocated_j - share.consumed_j - share.released_j - share.transferred_j;
}

/**
 * Check an explicit directed route against its real paired commissioning owner.
 * @param link Candidate saved route and exact commissioning revision.
 * @param source Actual source-side consumer owner/layer.
 * @param destination Actual destination-side consumer owner/layer.
 * @param world_refs True resolves actual portal/company/calendar references; false checks copied structural transport only.
 * @return Specific topology/layer refusal or complete admission; no flow or physical state changes.
 */
static UtilityPowerError CheckUtilityPowerLink(const UtilityPowerLink &link, const UtilityPowerConsumer &source,
	const UtilityPowerConsumer &destination, bool world_refs = true)
{
	// A direction connects two distinct same-company layer accounts; local
	// loops and foreign budgets cannot masquerade as a commissioned export.
	if (source.owner != destination.owner || source.meter.layer == destination.meter.layer || link.source == link.destination) return UtilityPowerError::InvalidTopology;
	// Retained route history may name retired consumers; an active route can
	// never make a removed company or a reused pool identity operational again.
	if (link.active && (!source.active || !destination.active)) return UtilityPowerError::InvalidTopology;
	// Invalid endpoints/revision/capacity authorize no physical route even in
	// preview, where loaded map and company pools are deliberately unavailable.
	if (link.source_endpoint == INVALID_TILE || link.destination_endpoint == INVALID_TILE || link.source_endpoint == link.destination_endpoint ||
		link.portal_revision == 0 || link.capacity_j == 0 || link.capacity_j > MAX_UTILITY_ACCOUNT_ENERGY_J || link.flow_j > link.capacity_j) return UtilityPowerError::InvalidTopology;
	if (!world_refs) return UtilityPowerError::None;
	const auto pair = GetLayerPortalPairKey(link.source_endpoint);
	if (!pair.has_value()) return UtilityPowerError::InvalidTopology;
	// Resolve reciprocal geometry before asserting pair access, then require
	// the exact saved world layers and actual shared company ownership.
	if (GetLayerPortalPairTile(link.source_endpoint) != link.destination_endpoint || GetTileLayer(link.source_endpoint) != source.meter.layer ||
		GetTileLayer(link.destination_endpoint) != destination.meter.layer || GetTileOwner(link.source_endpoint) != source.owner.base()) return UtilityPowerError::InvalidTopology;
	const auto *record = GetLayerPortalWorkRecord(link.source_endpoint);
	// Only the actual Connected revision admits flow; a sealed, locked or
	// reconstructed housing cannot inherit the old electrical edge's credit.
	if (record == nullptr || record->state != LayerPortalWorkState::Connected || record->company != source.owner || record->revision != link.portal_revision ||
		CheckLayerPortalAvailability(link.source_endpoint) != LayerPortalWorkError::None) return UtilityPowerError::InvalidTopology;
	return UtilityPowerError::None;
}

/** @copydoc RegisterUtilityPowerLink */
std::optional<UtilityLinkID> RegisterUtilityPowerLink(UtilityConsumerID source_id, UtilityConsumerID destination_id,
	TileIndex endpoint, uint64_t capacity_j, bool execute)
{
	const auto *source = FindUtilityConsumer(source_id);
	const auto *destination = FindUtilityConsumer(destination_id);
	// Both actual registered participants must exist before any edge identity
	// is allocated; caller-supplied numbers cannot create destination energy.
	if (source == nullptr || destination == nullptr) return std::nullopt;
	if (CheckUtilityPowerOwner(source->owner, source->meter.layer) != UtilityPowerError::None) return std::nullopt;
	if (CheckUtilityPowerOwner(destination->owner, destination->meter.layer) != UtilityPowerError::None) return std::nullopt;
	if (!GetLayerPortalPairKey(endpoint).has_value()) return std::nullopt;
	const auto *record = GetLayerPortalWorkRecord(endpoint);
	if (record == nullptr) return std::nullopt;
	// Finite retained edge identities follow the same no-reuse policy as
	// generation/consumer records and cannot grow without a bound.
	if (_utility_power.links.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS || _utility_power.next_link >= UINT32_MAX - 1) return std::nullopt;
	UtilityPowerLink link;
	link.id = UtilityLinkID{_utility_power.next_link}; link.source = source_id; link.destination = destination_id;
	link.source_endpoint = endpoint; link.destination_endpoint = GetLayerPortalPairTile(endpoint);
	link.portal_revision = record->revision; link.capacity_j = capacity_j;
	if (CheckUtilityPowerLink(link, *source, *destination) != UtilityPowerError::None) return std::nullopt;
	// An existing active ordered participant pair has one explicit metered
	// route; parallel duplicate edges cannot multiply its interval capacity.
	for (const auto &existing : _utility_power.links) {
		if (existing.active && existing.source == source_id && existing.destination == destination_id) return std::nullopt;
	}
	if (execute) { _utility_power.links.push_back(link); ++_utility_power.next_link; ++_utility_power.operation_sequence; }
	return link.id;
}

/**
 * Expire only a route's still-unused imported slices during actual topology loss.
 * @param link Actual retained route owner, already admitted for retirement.
 */
static void RetireUtilityPowerLink(UtilityPowerLink &link)
{
	auto *destination = FindUtilityConsumer(link.destination);
	if (destination != nullptr) {
		// Imported allocated/settled lineage stays saved; only free destination
		// quantities expire, with no reverse credit to the source reservation.
		for (auto &share : destination->shares) {
			if (share.imported_link != link.id) continue;
			const uint64_t unused = FreeUtilityShare(share);
			share.released_j += unused;
			FindUtilityProducer(share.producer)->reserved_j -= unused;
		}
	}
	link.active = false;
}

/** @copydoc RemoveUtilityPowerLink */
UtilityPowerError RemoveUtilityPowerLink(UtilityLinkID id, bool execute)
{
	if (_utility_mutation_committing) return UtilityPowerError::Refused;
	const auto *link = FindUtilityLink(id);
	// Missing/retired routes cannot repeat a loss release or be rebound to a
	// replacement physical housing under the same identity.
	if (link == nullptr || !link->active) return UtilityPowerError::InvalidReference;
	const auto *source = FindUtilityConsumer(link->source);
	if (source == nullptr) return UtilityPowerError::InvalidReference;
	// System cleanup can retire lost owners; ordinary companies can alter
	// only their own explicit route, without touching another imported budget.
	if (_current_company != OWNER_DEITY && _current_company != source->owner) return UtilityPowerError::InvalidOwner;
	if (_utility_power.operation_sequence >= UINT64_MAX - 1) return UtilityPowerError::Limit;
	if (execute) { RetireUtilityPowerLink(*FindUtilityLink(id)); ++_utility_power.operation_sequence; }
	return UtilityPowerError::None;
}

/** @copydoc InvalidateUtilityPowerLinksForPortal */
void InvalidateUtilityPowerLinksForPortal(TileIndex endpoint)
{
	if (_utility_mutation_committing) return;
	bool changed = false;
	// Physical removal/sealing scans only bounded registered edges, never
	// all map planes, and retires both directed uses of either actual endpoint.
	for (auto &link : _utility_power.links) {
		if (!link.active) continue;
		if (link.source_endpoint != endpoint && link.destination_endpoint != endpoint) continue;
		RetireUtilityPowerLink(link); changed = true;
	}
	// Terminal sequences already refuse new work; lifecycle loss still
	// expires unused credit without overflowing its saved retry identity.
	if (changed && _utility_power.operation_sequence < UINT64_MAX - 1) ++_utility_power.operation_sequence;
}

/** @copydoc RegisterUtilityPowerProducer */
std::optional<UtilityProducerID> RegisterUtilityPowerProducer(CompanyID owner, MapLayer layer, UtilityGenerationKind kind, bool execute)
{
	if (CheckUtilityPowerOwner(owner, layer) != UtilityPowerError::None) return std::nullopt;
	if (to_underlying(kind) > to_underlying(UtilityGenerationKind::OrbitalPanel)) return std::nullopt;
	// Bounded storage and monotonic identities keep malformed saves and retired
	// references from creating unbounded accounts or reusing the invalid sentinel.
	if (_utility_power.producers.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS || _utility_power.next_producer >= UINT32_MAX - 1) return std::nullopt;
	const UtilityProducerID id{_utility_power.next_producer};
	if (execute) {
		UtilityPowerProducer producer;
		producer.owner = owner;
		producer.kind = kind;
		producer.meter.producer_id = id;
		producer.meter.layer = layer;
		_utility_power.producers.push_back(std::move(producer));
		++_utility_power.next_producer;
		++_utility_power.operation_sequence;
	}
	return id;
}

/** @copydoc RegisterUtilityPowerConsumer */
std::optional<UtilityConsumerID> RegisterUtilityPowerConsumer(CompanyID owner, MapLayer layer, UtilityConsumerEffect effect,
	uint64_t capacity_j, std::span<const UtilityProducerID> sources, bool execute)
{
	if (CheckUtilityPowerOwner(owner, layer) != UtilityPowerError::None) return std::nullopt;
	if (to_underlying(effect) > to_underlying(UtilityConsumerEffect::TownGrowth)) return std::nullopt;
	// Towns and their native growth offers are Surface entities. A numeric
	// Deep/Space account cannot create an unsaveable or hidden city effect.
	if (effect == UtilityConsumerEffect::TownGrowth && layer != MapLayer::Surface) return std::nullopt;
	if (capacity_j > MAX_UTILITY_ACCOUNT_ENERGY_J) return std::nullopt;
	if (sources.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return std::nullopt;
	size_t connections = sources.size();
	// Bound the whole saved connectivity graph, not only each per-consumer
	// list, so a dense malformed network cannot allocate millions of rows.
	for (const auto &consumer : _utility_power.consumers) connections += consumer.linked_producers.size();
	if (connections > MAX_UTILITY_POWER_CONNECTIONS) return std::nullopt;
	// Consumer limits and invalid sentinels follow the same bounded one-owner
	// policy as producers; estimation cannot consume the next free identity.
	if (_utility_power.consumers.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS || _utility_power.next_consumer >= UINT32_MAX - 1) return std::nullopt;
	std::set<UtilityProducerID> unique;
	// Physical connectivity is supplied explicitly by the feature/network owner.
	// Duplicate, foreign and cross-layer links cannot borrow another local budget.
	for (const UtilityProducerID id : sources) {
		const auto *source = FindUtilityProducer(id);
		if (source == nullptr) return std::nullopt;
		if (!source->active) return std::nullopt;
		if (source->owner != owner || source->meter.layer != layer || !unique.insert(id).second) return std::nullopt;
	}
	const UtilityConsumerID id{_utility_power.next_consumer};
	if (execute) {
		UtilityPowerConsumer consumer;
		consumer.owner = owner;
		consumer.effect = effect;
		consumer.capacity_j = capacity_j;
		consumer.meter.consumer_id = id;
		consumer.meter.layer = layer;
		consumer.linked_producers.assign(unique.begin(), unique.end());
		_utility_power.consumers.push_back(std::move(consumer));
		++_utility_power.next_consumer;
		++_utility_power.operation_sequence;
	}
	return id;
}

/** @copydoc PublishUtilityGeneration */
UtilityPowerError PublishUtilityGeneration(UtilityProducerID id, uint64_t interval, uint64_t rate_w, uint32_t economy_ticks, bool execute)
{
	auto *source = FindUtilityProducer(id);
	if (source == nullptr) return UtilityPowerError::InvalidReference;
	if (!source->active) return UtilityPowerError::InvalidReference;
	const auto permission = CheckUtilityPowerOwner(source->owner, source->meter.layer);
	if (permission != UtilityPowerError::None) return permission;
	// Only the owner's live nonterminal epoch accepts publication; old receipts
	// and guessed future intervals cannot generate another copy of the energy.
	if (interval != _utility_power.interval || interval == UINT64_MAX) return UtilityPowerError::InvalidInterval;
	if (_utility_power.allocated) return UtilityPowerError::AlreadyAllocated;
	if (source->meter.accounting_interval == interval) return UtilityPowerError::DuplicatePublication;
	const auto integrated = IntegrateUtilityGeneration(rate_w, economy_ticks, source->meter.remainder_mj);
	if (!integrated.has_value()) return UtilityPowerError::InvalidQuantity;
	if (integrated->energy_j > UINT64_MAX - source->generated_lifetime_j) return UtilityPowerError::Limit;
	if (execute) {
		source->meter.accounting_interval = interval;
		source->meter.generation_rate_w = rate_w;
		source->meter.economy_ticks = economy_ticks;
		source->meter.generation_energy_j = integrated->energy_j;
		source->meter.remainder_mj = integrated->remainder_mj;
		source->generated_lifetime_j += integrated->energy_j;
		++_utility_power.operation_sequence;
	}
	return UtilityPowerError::None;
}

/** @copydoc AllocateUtilityPower */
UtilityPowerError AllocateUtilityPower(uint64_t interval, std::span<const UtilityPowerRequest> requests, bool execute)
{
	if (_utility_mutation_committing) return UtilityPowerError::Refused;
	if (_utility_power.operation_sequence >= UINT64_MAX - 1) return UtilityPowerError::Limit;
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return UtilityPowerError::UnsupportedProfile;
	// Allocation is one global synchronized system phase across all companies.
	// A company command cannot close the world's epoch before its peers request.
	if (_current_company != OWNER_DEITY) return UtilityPowerError::InvalidOwner;
	// Allocation must close this exact live nonterminal epoch; a delayed or
	// guessed future request cannot reserve another interval's supply.
	if (interval != _utility_power.interval || interval == UINT64_MAX) return UtilityPowerError::InvalidInterval;
	if (_utility_power.allocated) return UtilityPowerError::AlreadyAllocated;
	if (requests.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return UtilityPowerError::Limit;
	std::set<UtilityConsumerID> unique;
	std::vector<UtilityConsumerDemand> demands;
	// Validate every request before reserving anything. Foreign, duplicate or
	// over-capacity demand must leave the whole interval's supply unallocated.
	for (const auto &request : requests) {
		const auto *consumer = FindUtilityConsumer(request.consumer);
		if (consumer == nullptr) return UtilityPowerError::InvalidReference;
		if (!consumer->active) return UtilityPowerError::InvalidReference;
		const auto permission = CheckUtilityPowerOwner(consumer->owner, consumer->meter.layer, false);
		if (permission != UtilityPowerError::None) return permission;
		if (!unique.insert(request.consumer).second) return UtilityPowerError::InvalidReference;
		if (request.demand_j > consumer->capacity_j) return UtilityPowerError::InvalidQuantity;
		demands.push_back({request.consumer, consumer->meter.layer, consumer->linked_producers, request.demand_j});
	}
	std::vector<UtilityProducerSnapshot> supply;
	// A source which did not publish has genuinely zero supply. All companies
	// share this system phase, but each consumer's saved links remain owned/local.
	for (const auto &source : _utility_power.producers) {
		if (!source.active) continue;
		const auto permission = CheckUtilityPowerOwner(source.owner, source.meter.layer, false);
		if (permission != UtilityPowerError::None) return permission;
		supply.push_back({source.meter.producer_id, source.meter.layer,
			source.meter.accounting_interval == interval ? source.meter.generation_energy_j : 0});
	}
	const auto plan = PlanUtilityAllocation(interval, supply, demands, {});
	if (!plan.valid) return UtilityPowerError::InvalidTopology;
	if (execute) {
		// Frozen allocation's consumed_j denotes the proposal's promised supply.
		// Store it as reserved_j: only later atomic settlement may call it actual use.
		for (const auto &source : plan.producers) FindUtilityProducer(source.id)->reserved_j = source.consumed_j;
		// Every registered consumer receives a current zero baseline, including
		// omitted requests. Its former epoch cannot hide or renew an old grant.
		for (auto &consumer : _utility_power.consumers) {
			consumer.meter.accounting_interval = interval;
			consumer.meter.demand_energy_j = 0;
			consumer.meter.granted_energy_j = 0;
			consumer.meter.unmet_energy_j = 0;
			consumer.shares.clear();
			consumer.town_bonus_percent = 0;
			consumer.town_effect_applied = false;
			consumer.town_effect_suspended = false;
		}
		for (const auto &grant : plan.consumers) {
			auto &consumer = *FindUtilityConsumer(grant.id);
			auto &meter = consumer.meter;
			meter.accounting_interval = interval;
			meter.demand_energy_j = grant.demand_j;
			meter.granted_energy_j = grant.granted_j;
			meter.unmet_energy_j = grant.demand_j - grant.granted_j;
			// Copy the allocator's exact positive slices once. It alone chose
			// source ordering; the runtime must not recompute or guess lineage.
			for (const auto &share : grant.sources) consumer.shares.push_back({share.producer, share.energy_j, 0, 0});
		}
		_utility_power.allocated = true;
		++_utility_power.operation_sequence;
	}
	return UtilityPowerError::None;
}

/** @copydoc GetUtilityAllocation */
std::optional<UtilityPowerAccount> GetUtilityAllocation(UtilityConsumerID id)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	const auto *consumer = FindUtilityConsumer(id);
	// A missing, foreign or not-yet-allocated consumer has no current grant.
	// Reading a window cannot carry a previous epoch forward or renew a lease.
	if (consumer == nullptr || !consumer->active || consumer->owner != _current_company || !_utility_power.allocated || consumer->meter.accounting_interval != _utility_power.interval) return std::nullopt;
	UtilityPowerAccount view = consumer->meter;
	uint64_t available_j = 0, consumed_j = 0;
	// Project the remaining reservation from its one saved slice owner; a lost
	// source or prior settlement cannot appear as usable electricity again.
	for (const auto &share : consumer->shares) {
		available_j += FreeUtilityShare(share);
		consumed_j += share.consumed_j;
	}
	view.granted_energy_j = available_j;
	view.unmet_energy_j = view.demand_energy_j - consumed_j - available_j;
	return view;
}

/** @copydoc BeginUtilityPowerInterval */
UtilityPowerError BeginUtilityPowerInterval(bool execute)
{
	if (_utility_mutation_committing) return UtilityPowerError::Refused;
	if (_utility_power.operation_sequence >= UINT64_MAX - 1) return UtilityPowerError::Limit;
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return UtilityPowerError::UnsupportedProfile;
	if (_current_company != OWNER_DEITY) return UtilityPowerError::InvalidOwner;
	if (!_utility_power.allocated) return UtilityPowerError::AllocationRequired;
	if (_utility_power.interval >= UINT64_MAX - 1) return UtilityPowerError::InvalidInterval;
	if (execute) {
		// A new accounting interval curtails unspent promises, never refunds
		// settled work or carries a prior consumer reservation into a new epoch.
		for (auto &consumer : _utility_power.consumers) {
			consumer.shares.clear();
			consumer.meter.demand_energy_j = 0;
			consumer.meter.granted_energy_j = 0;
			consumer.meter.unmet_energy_j = 0;
			consumer.town_bonus_percent = 0;
			consumer.town_effect_applied = false;
			consumer.town_effect_suspended = false;
			if (consumer.town != TownID::Invalid()) RefreshUtilityTownGrowthRate(consumer.town);
		}
		// Only the fixed millijoule generation carry survives rotation. A source
		// must publish again; its old whole joules cannot become new supply.
		for (auto &source : _utility_power.producers) {
			source.reserved_j = 0;
			source.meter.generation_energy_j = 0;
			source.meter.generation_rate_w = 0;
			source.meter.economy_ticks = 0;
		}
		// Prior imports expire with reservations; an active physical edge starts
		// the new interval with zero metered flow rather than renewed old credit.
		for (auto &link : _utility_power.links) link.flow_j = 0;
		++_utility_power.interval;
		_utility_power.allocated = false;
		++_utility_power.operation_sequence;
	}
	return UtilityPowerError::None;
}

/** @copydoc RemoveUtilityPowerProducer */
UtilityPowerError RemoveUtilityPowerProducer(UtilityProducerID id, bool execute)
{
	if (_utility_mutation_committing) return UtilityPowerError::Refused;
	if (_utility_power.operation_sequence >= UINT64_MAX - 1) return UtilityPowerError::Limit;
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return UtilityPowerError::UnsupportedProfile;
	auto *source = FindUtilityProducer(id);
	if (source == nullptr) return UtilityPowerError::InvalidReference;
	if (!source->active) return UtilityPowerError::InvalidReference;
	// System lifecycle cleanup may release a lost company/source; ordinary
	// company callers may only retire their own actual source identity.
	if (_current_company != OWNER_DEITY && _current_company != source->owner) return UtilityPowerError::InvalidOwner;
	if (execute) {
		// Retain the retired original identity for current settled lineage. Clear
		// its eligibility everywhere, but release only that source's unspent
		// slices, leaving other sources and already consumed joules untouched.
		for (auto &consumer : _utility_power.consumers) {
			std::erase(consumer.linked_producers, id);
			for (auto &share : consumer.shares) {
				if (share.producer != id) continue;
				const uint64_t unused = FreeUtilityShare(share);
				share.released_j += unused;
				source->reserved_j -= unused;
			}
		}
		source->active = false;
		++_utility_power.operation_sequence;
	}
	return UtilityPowerError::None;
}

/** @copydoc ChangeUtilityPowerCompanyOwnership */
void ChangeUtilityPowerCompanyOwnership(Owner old_owner, Owner new_owner)
{
	assert(!_utility_mutation_committing);
	assert(old_owner != new_owner);
	bool changed = false;
	std::set<UtilityProducerID> live_lineage;
	// Inactive consumers belong to former company incarnations. Collect only
	// the current company's live original-source lineage, including a source
	// retired while its current consumer still retains settlement history.
	for (const auto &consumer : _utility_power.consumers) {
		if (!consumer.active) continue;
		if (consumer.owner != old_owner) continue;
		for (const auto &share : consumer.shares) live_lineage.insert(share.producer);
	}
	// Physical eligibility is invalidated before tiles/company identities
	// change. Takeover must commission a new route rather than inherit stale
	// portal revisions; settled imported use is never refunded.
	for (auto &link : _utility_power.links) {
		const auto *source = FindUtilityConsumer(link.source);
		if (!link.active) continue;
		if (source->owner != old_owner) continue;
		RetireUtilityPowerLink(link);
		changed = true;
	}
	// Current source/consumer ownership moves together, including its retired lineage.
	// Removal expires only remaining source slices, keeping lifetime use and
	// original numeric identity so CompanyID reuse cannot renew old energy.
	for (auto &source : _utility_power.producers) {
		if (source.owner != old_owner) continue;
		// A reused CompanyID cannot acquire an earlier incarnation's retired
		// records; only live sources or the current live consumer's lineage move.
		if (!source.active && !live_lineage.contains(source.meter.producer_id)) continue;
		changed = true;
		if (new_owner != INVALID_OWNER) {
			source.owner = CompanyID{new_owner};
			continue;
		}
		source.active = false;
	}
	// A retired consumer remains a saved accounting tombstone, with no town
	// eligibility or local connections. Already posted/consumed history remains
	// suspended; the actual town recomputes its ordinary rate without the offer.
	for (auto &consumer : _utility_power.consumers) {
		if (consumer.owner != old_owner) continue;
		if (!consumer.active) continue;
		changed = true;
		if (new_owner != INVALID_OWNER) {
			consumer.owner = CompanyID{new_owner};
			continue;
		}
		consumer.active = false;
		consumer.linked_producers.clear();
		// Same-company allocation guarantees these exact original sources.
		// Visit each retained share once, independent of the producer count;
		// company removal must not multiply work across the whole ledger.
		for (auto &share : consumer.shares) {
			const auto unused = FreeUtilityShare(share);
			share.released_j += unused;
			FindUtilityProducer(share.producer)->reserved_j -= unused;
		}
		const auto town = consumer.town;
		consumer.town = TownID::Invalid();
		if (consumer.effect == UtilityConsumerEffect::TownGrowth) consumer.town_effect_suspended = true;
		if (town != TownID::Invalid()) RefreshUtilityTownGrowthRate(town);
	}
	// Any still-admissible prepared operation becomes stale. A terminal
	// sequence already refuses preparation, so lifecycle can still finish
	// without overflow or delaying actual company removal.
	if (changed && _utility_power.operation_sequence < UINT64_MAX - 1) ++_utility_power.operation_sequence;
}

/** @copydoc CaptureUtilityPowerState */
UtilityPowerState CaptureUtilityPowerState() { return _utility_power; }

/** @copydoc ComputeUtilityPowerDigest */
uint64_t ComputeUtilityPowerDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Explicit scalar encoded in fixed little-endian bytes for host-independent diagnostics. */
	const auto field = [&hash](uint64_t value) {
		// Fixed-width encoding keeps host byte order and compiler layout out of
		// saved-account comparisons, including on different multiplayer machines.
		for (uint8_t byte = 0; byte < 8; ++byte) { hash ^= value & 0xFF; hash *= 1099511628211ULL; value >>= 8; }
	};
	/** @param meter Canonical saved unit/identity/quantity fields, each included once in declaration order. */
	const auto account = [&field](const UtilityPowerAccount &meter) {
		field(to_underlying(meter.domain)); field(meter.producer_id.base()); field(meter.consumer_id.base()); field(to_underlying(meter.layer));
		field(meter.accounting_interval); field(meter.economy_ticks); field(meter.generation_rate_w); field(meter.generation_energy_j);
		field(meter.demand_energy_j); field(meter.granted_energy_j); field(meter.unmet_energy_j); field(meter.remainder_mj);
	};
	field(_utility_power.interval); field(_utility_power.next_producer); field(_utility_power.next_consumer); field(_utility_power.allocated);
	field(_utility_power.operation_sequence);
	field(_utility_power.next_link);
	field(_utility_power.producers.size());
	// Retired sources remain in identity order; their fractions and settled
	// lineage are still authoritative even when they have no usable output.
	for (const auto &source : _utility_power.producers) {
		account(source.meter); field(source.owner.base()); field(to_underlying(source.kind)); field(source.reserved_j); field(source.active);
		field(source.generated_lifetime_j); field(source.consumed_lifetime_j);
	}
	field(_utility_power.consumers.size());
	// Exact collection lengths and source IDs distinguish missing links/slices
	// from unchanged totals, without deriving a replacement electrical credit.
	for (const auto &consumer : _utility_power.consumers) {
		account(consumer.meter); field(consumer.owner.base()); field(consumer.active); field(to_underlying(consumer.effect)); field(consumer.capacity_j);
		field(consumer.town.base()); field(consumer.town_bonus_percent); field(consumer.town_effect_applied); field(consumer.town_effect_suspended);
		field(consumer.linked_producers.size());
		for (const auto source : consumer.linked_producers) field(source.base());
		field(consumer.shares.size());
		for (const auto &share : consumer.shares) { field(share.producer.base()); field(share.allocated_j); field(share.consumed_j); field(share.released_j); field(share.transferred_j); field(share.imported_link.base()); }
	}
	field(_utility_power.links.size());
	// Exact directed endpoints/revision, capacity, flow and retirement remain
	// saved authority; an equivalent display network cannot replace them.
	for (const auto &link : _utility_power.links) {
		field(link.id.base()); field(link.source.base()); field(link.destination.base()); field(link.source_endpoint.base()); field(link.destination_endpoint.base());
		field(link.portal_revision); field(link.capacity_j); field(link.flow_j); field(link.active);
	}
	return hash;
}

/**
 * Fingerprint exact transient mutation fields without granting any authority.
 * @param proposal Fully populated owned request and saved sequence.
 * @return Host-independent field fingerprint used only for commit revalidation.
 */
static uint64_t SealUtilityMutation(const UtilityMutationProposal &proposal)
{
	uint64_t hash = 14695981039346656037ULL;
	const std::array<uint64_t, 8> fields{to_underlying(proposal.kind), proposal.grant.consumer.base(), proposal.grant.interval,
		proposal.owner.base(), proposal.quantity_j, proposal.destination.base(), proposal.link.base(), proposal.sequence_before};
	// Explicit scalar bytes reject changed quantity/kind/target independently
	// of pointer layout, without scanning unrelated networks on each work tick.
	for (uint64_t value : fields) {
		for (uint8_t byte = 0; byte < 8; ++byte) { hash ^= value & 0xFF; hash *= 1099511628211ULL; value >>= 8; }
	}
	return hash;
}

/** @copydoc PrepareUtilityMutation */
UtilityMutationProposal PrepareUtilityMutation(UtilityMutationKind kind, UtilityGrantID grant, uint64_t quantity_j,
	UtilityConsumerID destination, UtilityLinkID link)
{
	UtilityMutationProposal proposal;
	proposal.kind = kind; proposal.grant = grant; proposal.quantity_j = quantity_j;
	proposal.owner = _current_company; proposal.destination = destination; proposal.link = link;
	proposal.sequence_before = _utility_power.operation_sequence;
	auto *consumer = FindUtilityConsumer(grant.consumer);
	if (consumer == nullptr) return proposal;
	if (!consumer->active) return proposal;
	proposal.error = CheckUtilityPowerOwner(consumer->owner, consumer->meter.layer);
	if (proposal.error != UtilityPowerError::None) return proposal;
	// An exact current reservation is required; previous/future grants and
	// an open generation phase cannot supply an action's electrical budget.
	if (!_utility_power.allocated || grant.interval != _utility_power.interval || consumer->meter.accounting_interval != grant.interval) {
		proposal.error = UtilityPowerError::InvalidInterval; return proposal;
	}
	// Zero or out-of-contract quantities are refused before any participant
	// sees a proposal, preventing meaningless sequence/settlement advancement.
	if (quantity_j == 0 || quantity_j > MAX_UTILITY_ACCOUNT_ENERGY_J) {
		proposal.error = UtilityPowerError::InvalidQuantity; return proposal;
	}
	if (to_underlying(kind) > to_underlying(UtilityMutationKind::TransferLayer)) {
		proposal.error = UtilityPowerError::InvalidReference; return proposal;
	}
	if (kind == UtilityMutationKind::TransferLayer) {
		const auto *route = FindUtilityLink(link);
		const auto *target = FindUtilityConsumer(destination);
		// Only this exact directed active route may move the source grant to
		// an already requested destination in the same closed accounting epoch.
		if (route == nullptr || target == nullptr || !route->active || route->source != grant.consumer || route->destination != destination ||
			target->meter.accounting_interval != grant.interval) { proposal.error = UtilityPowerError::InvalidTopology; return proposal; }
		proposal.error = CheckUtilityPowerLink(*route, *consumer, *target);
		if (proposal.error != UtilityPowerError::None) return proposal;
		// Both actual route capacity and original requested destination room
		// bound imports; an omitted request cannot receive invented demand.
		if (quantity_j > route->capacity_j - route->flow_j || quantity_j > target->meter.demand_energy_j - target->meter.granted_energy_j) {
			proposal.error = UtilityPowerError::InsufficientGrant; return proposal;
		}
		uint64_t local = 0;
		// Imports never re-export this interval. Only original generation slices
		// of this source-side reservation can authorize the directed flow.
		for (const auto &share : consumer->shares) {
			if (share.imported_link.base() == UINT32_MAX) local += FreeUtilityShare(share);
		}
		if (quantity_j > local) { proposal.error = UtilityPowerError::InsufficientGrant; return proposal; }
		size_t additional = 0;
		uint64_t unassigned = quantity_j;
		// Only genuine new original-source/route contributions consume a row.
		// Merging another flow into an existing import cannot falsely exhaust
		// the whole-ledger cap merely because the donor has many old slices.
		for (const auto &share : consumer->shares) {
			if (share.imported_link.base() != UINT32_MAX) continue;
			const uint64_t take = std::min(unassigned, FreeUtilityShare(share));
			if (take == 0) continue;
			/** @param existing Actual destination contribution. @return Whether the same source/route row already exists. */
			const auto matches = [&](const UtilityPowerShare &existing) { return existing.producer == share.producer && existing.imported_link == link; };
			if (std::ranges::find_if(target->shares, matches) == target->shares.end()) ++additional;
			unassigned -= take;
			if (unassigned == 0) break;
		}
		size_t count = 0;
		// Bound actual new rows across registered participants; imported
		// capacity does not depend on a camera or derived network cache.
		for (const auto &existing : _utility_power.consumers) count += existing.shares.size();
		if (count + additional > MAX_UTILITY_POWER_SHARES || target->shares.size() + additional > MAX_UTILITY_INTERVAL_ACCOUNTS) {
			proposal.error = UtilityPowerError::Limit; return proposal;
		}
		proposal.seal = SealUtilityMutation(proposal);
		return proposal;
	}
	// Local settlement/release cannot carry a hidden destination or route;
	// those fields belong exclusively to an explicit metered transfer.
	if (destination.base() != UINT32_MAX || link.base() != UINT32_MAX) {
		proposal.error = UtilityPowerError::InvalidTopology; return proposal;
	}
	uint64_t available = 0;
	// Exact free source slices authorize the quantity; original grants and
	// repeated UI queries cannot restore previously consumed/released joules.
	for (const auto &share : consumer->shares) available += FreeUtilityShare(share);
	if (quantity_j > available) { proposal.error = UtilityPowerError::InsufficientGrant; return proposal; }
	proposal.seal = SealUtilityMutation(proposal);
	return proposal;
}

/** @copydoc CommitUtilityMutation */
UtilityMutationReceipt CommitUtilityMutation(const UtilityMutationProposal &proposal, UtilityMutationParticipant *participant)
{
	UtilityMutationReceipt receipt{UtilityPowerError::InvalidReference, _utility_power.operation_sequence, 0};
	if (proposal.error != UtilityPowerError::None) { receipt.error = proposal.error; return receipt; }
	// Every accepted ledger change invalidates prior tickets, including save-
	// restored retries; estimating or reading never advances this sequence.
	if (proposal.sequence_before != _utility_power.operation_sequence || proposal.owner != _current_company) {
		receipt.error = UtilityPowerError::StaleMutation; return receipt;
	}
	const auto current = PrepareUtilityMutation(proposal.kind, proposal.grant, proposal.quantity_j, proposal.destination, proposal.link);
	if (current.error != UtilityPowerError::None) { receipt.error = current.error; return receipt; }
	if (current.seal != proposal.seal) { receipt.error = UtilityPowerError::StaleMutation; return receipt; }
	// Actual use requires a complete physical/inventory/effect participant;
	// a direct call cannot debit energy for a nonexistent gameplay benefit.
	if (proposal.kind == UtilityMutationKind::Consume && participant == nullptr) { receipt.error = UtilityPowerError::Refused; return receipt; }
	auto &consumer = *FindUtilityConsumer(proposal.grant.consumer);
	// Town offers require the actual native town participant and settle once
	// per interval. A generic expense/inventory callback cannot create a town
	// bonus or consume its offer after GS takeover or previous application.
	if (proposal.kind == UtilityMutationKind::Consume && consumer.effect == UtilityConsumerEffect::TownGrowth &&
		(consumer.town == TownID::Invalid() || consumer.town_effect_applied || consumer.town_effect_suspended ||
			participant->EffectKind() != UtilityConsumerEffect::TownGrowth)) { receipt.error = UtilityPowerError::Refused; return receipt; }
	std::vector<std::pair<size_t, uint64_t>> debits;
	debits.reserve(consumer.shares.size());
	uint64_t remaining = proposal.quantity_j;
	// Precompute all exact source debits before committing any participant,
	// so the final ledger writes allocate nothing and cannot partially fail.
	for (size_t index = 0; index < consumer.shares.size(); ++index) {
		const auto &share = consumer.shares[index];
		// A transfer uses only original local slices; ordinary use/release may
		// settle imported quantities against their retained generation origin.
		if (proposal.kind == UtilityMutationKind::TransferLayer && share.imported_link.base() != UINT32_MAX) continue;
		const uint64_t take = std::min(remaining, FreeUtilityShare(share));
		if (take == 0) continue;
		const auto &source = *FindUtilityProducer(share.producer);
		if (take > source.reserved_j) { receipt.error = UtilityPowerError::InvalidQuantity; return receipt; }
		// Source lifetime totals cannot overflow or exceed once-only actual
		// publication, regardless of how many work actions settle this epoch.
		if (proposal.kind == UtilityMutationKind::Consume && take > source.generated_lifetime_j - source.consumed_lifetime_j) {
			receipt.error = UtilityPowerError::InvalidQuantity; return receipt;
		}
		debits.emplace_back(index, take);
		remaining -= take;
		if (remaining == 0) break;
	}
	if (remaining != 0) { receipt.error = UtilityPowerError::InsufficientGrant; return receipt; }
	auto *target = proposal.kind == UtilityMutationKind::TransferLayer ? FindUtilityConsumer(proposal.destination) : nullptr;
	if (target != nullptr) target->shares.reserve(target->shares.size() + debits.size()); // Allocate before any complete participant commit; final metered writes cannot fail for capacity growth.
	AutoRestoreBackup transaction(_utility_mutation_committing, true);
	// Validation and owner commit both run with recursive ledger writes
	// refused; the participant may read its grant but cannot renew/debit it.
	if (participant != nullptr) {
		if (!participant->Validate(proposal)) { receipt.error = UtilityPowerError::Refused; return receipt; }
		if (!participant->Commit(proposal)) { receipt.error = UtilityPowerError::Refused; return receipt; }
	}
	// Every owner has accepted the complete transaction. Only now settle the
	// exact original slices and source reserve; released units remain unused.
	for (const auto &[index, take] : debits) {
		auto &share = consumer.shares[index];
		auto &source = *FindUtilityProducer(share.producer);
		if (proposal.kind == UtilityMutationKind::TransferLayer) {
			share.transferred_j += take;
			/** @param candidate Existing destination slice. @return Whether it retains this exact original source and directed route. */
			const auto matches = [&](const UtilityPowerShare &candidate) { return candidate.producer == share.producer && candidate.imported_link == proposal.link; };
			const auto existing = std::ranges::find_if(target->shares, matches);
			if (existing == target->shares.end()) target->shares.push_back({share.producer, take, 0, 0, 0, proposal.link});
			else existing->allocated_j += take;
			continue; // Original source reserve is conserved across the two consumer accounts.
		} else if (proposal.kind == UtilityMutationKind::Consume) {
			share.consumed_j += take;
			source.consumed_lifetime_j += take;
		} else {
			share.released_j += take;
		}
		source.reserved_j -= take;
	}
	if (target != nullptr) {
		// Destination credit presents the same once-generated joules and keeps
		// original requested demand; no output meter or publication is created.
		target->meter.granted_energy_j += proposal.quantity_j;
		target->meter.unmet_energy_j -= proposal.quantity_j;
		FindUtilityLink(proposal.link)->flow_j += proposal.quantity_j;
		/** @param left Saved contribution. @param right Saved contribution. @return Stable original-source/route order, independent of transfer submission order. */
		const auto order = [](const UtilityPowerShare &left, const UtilityPowerShare &right) {
			return std::pair(left.producer, left.imported_link) < std::pair(right.producer, right.imported_link);
		};
		std::ranges::sort(target->shares, order);
	}
	// The town participant has committed the actual rate. Record its accepted
	// native offer at the same boundary as original-source electrical use;
	// ordinary work callbacks keep their own physical progress authority.
	if (proposal.kind == UtilityMutationKind::Consume && consumer.effect == UtilityConsumerEffect::TownGrowth) {
		consumer.town_effect_applied = true;
		consumer.town_bonus_percent = *ComputeNativeTownPowerBonus(MultiLayerTownPowerAuthority::Native, proposal.quantity_j, consumer.meter.demand_energy_j);
	}
	++_utility_power.operation_sequence;
	return {UtilityPowerError::None, _utility_power.operation_sequence, proposal.quantity_j};
}

/** @return Actual accepted profile's exclusive town-effect authority; unsupported/Classic policy owns no native benefit. */
static MultiLayerTownPowerAuthority GetNativeUtilityTownAuthority()
{
	const auto *profile = GetMultiLayerContentProfile(GetCurrentMultiLayerProfileSelection().id);
	// Classic and unknown profiles cannot gain native city effects through a
	// stale binding, and no unavailable ETI adapter is silently selected.
	if (profile == nullptr || profile->id != MultiLayerGameplayProfileID::MultilayerIndustry) return MultiLayerTownPowerAuthority::Invalid;
	return profile->town_power_authority;
}

/** Whole actual work participant shares the same electrical transaction without owning another stock/work registry. */
class UtilityWorkTransaction final : public UtilityMutationParticipant {
	UtilityWorkParticipant &owner; ///< Existing physical work owner.
	UtilityWorkEffect effect; ///< Exact admitted bounded response, transient within this action.
public:
	/** @param owner Actual complete work owner. @param effect Exact admitted positive work/energy response. */
	UtilityWorkTransaction(UtilityWorkParticipant &owner, UtilityWorkEffect effect) : owner(owner), effect(effect) {}
	/** @param proposal Exact utility action. @return Whether the actual complete work owner still admits its response without mutation. */
	bool Validate([[maybe_unused]] const UtilityMutationProposal &proposal) const override { return this->owner.Validate(this->effect); }
	/** @param proposal Exact revalidated utility action. @return Complete physical work success or unchanged refusal. */
	bool Commit([[maybe_unused]] const UtilityMutationProposal &proposal) noexcept override { return this->owner.Commit(this->effect); }
	/** @return The actual bounded work effect applied by this whole owner, never a town authority marker. */
	std::optional<UtilityConsumerEffect> EffectKind() const override { return this->effect.kind; }
};

/** @copydoc ApplyUtilityWorkEffect */
UtilityMutationReceipt ApplyUtilityWorkEffect(UtilityConsumerID id, uint32_t requested_ticks, UtilityWorkParticipant &participant)
{
	UtilityMutationReceipt refusal{UtilityPowerError::InvalidReference, _utility_power.operation_sequence, 0};
	const auto *consumer = FindUtilityConsumer(id);
	if (consumer == nullptr) return refusal;
	if (!consumer->active) return refusal;
	const auto permission = CheckUtilityPowerOwner(consumer->owner, consumer->meter.layer);
	if (permission != UtilityPowerError::None) { refusal.error = permission; return refusal; }
	// Native work has only its three published service/processor contracts;
	// city offers use their own real Town owner and cannot become generic work.
	if (consumer->effect == UtilityConsumerEffect::TownGrowth || requested_ticks == 0 || requested_ticks > MAX_UTILITY_INTERVAL_TICKS) {
		refusal.error = UtilityPowerError::InvalidQuantity; return refusal;
	}
	const auto contracts = GetUtilityConsumerEffects();
	const auto contract = std::ranges::find(contracts, consumer->effect, &UtilityConsumerEffectContract::effect);
	if (contract == contracts.end()) return refusal;
	const auto allocation = GetUtilityAllocation(id);
	if (!allocation.has_value()) { refusal.error = UtilityPowerError::InvalidInterval; return refusal; }
	const uint64_t per_tick_j = static_cast<uint64_t>(contract->demand_rate_w) * 27 / 1000;
	UtilityWorkEffect effect; effect.kind = consumer->effect;
	if (contract->required_for_work) {
		effect.work_economy_ticks = static_cast<uint32_t>(std::min<uint64_t>(requested_ticks, allocation->granted_energy_j / per_tick_j));
		effect.energy_j = effect.work_economy_ticks * per_tick_j;
	} else {
		const uint64_t demand = per_tick_j * requested_ticks;
		effect.energy_j = std::min(demand, allocation->granted_energy_j);
		effect.bonus_percent = static_cast<uint8_t>(effect.energy_j * contract->maximum_bonus_percent / demand);
		effect.work_economy_ticks = static_cast<uint32_t>(static_cast<uint64_t>(requested_ticks) * effect.bonus_percent / 100);
	}
	// No positive admitted work means no benefit and no energy debit. Optional
	// baseline-zero/rounding and mandatory starvation cannot create progress.
	if (effect.energy_j == 0 || effect.work_economy_ticks == 0) { refusal.error = UtilityPowerError::InsufficientGrant; return refusal; }
	const auto proposal = PrepareUtilityMutation(UtilityMutationKind::Consume, {id, allocation->accounting_interval}, effect.energy_j);
	UtilityWorkTransaction transaction(participant, effect);
	return CommitUtilityMutation(proposal, &transaction);
}

/** @copydoc BindUtilityTownConsumer */
UtilityPowerError BindUtilityTownConsumer(UtilityConsumerID id, TownID town_id, bool execute)
{
	auto *consumer = FindUtilityConsumer(id);
	if (consumer == nullptr) return UtilityPowerError::InvalidReference;
	if (!consumer->active) return UtilityPowerError::InvalidReference;
	const auto permission = CheckUtilityPowerOwner(consumer->owner, consumer->meter.layer);
	if (permission != UtilityPowerError::None) return permission;
	Town *town = Town::GetIfValid(town_id);
	// Only one actual Surface town under native authority can admit this
	// explicit TownGrowth account; custom GS control refuses before binding.
	if (consumer->effect != UtilityConsumerEffect::TownGrowth || consumer->meter.layer != MapLayer::Surface || town == nullptr ||
		GetTileLayer(town->xy) != MapLayer::Surface || town->flags.Test(TownFlag::CustomGrowth) ||
		GetNativeUtilityTownAuthority() != MultiLayerTownPowerAuthority::Native) return UtilityPowerError::Refused;
	if (consumer->town == town_id) return UtilityPowerError::None;
	if (consumer->town_effect_applied) return UtilityPowerError::Refused;
	// One native supplier owns a town offer across all companies; independent
	// accounts cannot stack several25-percent modifiers on the same city.
	for (const auto &existing : _utility_power.consumers) {
		if (existing.town == town_id) return UtilityPowerError::InvalidTopology;
	}
	if (execute) { consumer->town = town_id; ++_utility_power.operation_sequence; }
	return UtilityPowerError::None;
}

/** Real native town participant adjusts its ordinary growth owner without claiming custom-GS authority or changing RNG. */
class UtilityTownTransaction final : public UtilityMutationParticipant {
	TownID town; ///< Actual bound town identity, checked again before physical rate change.
	uint8_t bonus; ///< Exact capped admitted current offer.
public:
	/** @param town Actual bound Surface town. @param bonus Checked native benefit within0..25. */
	UtilityTownTransaction(TownID town, uint8_t bonus) : town(town), bonus(bonus) {}
	/** @param proposal Exact owned electrical offer. @return Whether actual town/native authority still admits this whole action without mutation. */
	bool Validate([[maybe_unused]] const UtilityMutationProposal &proposal) const override
	{
		const Town *target = Town::GetIfValid(this->town);
		// A script takeover/deletion between preparation and commit refuses the
		// city effect before any original-source energy can be settled.
		return target != nullptr && !target->flags.Test(TownFlag::CustomGrowth) && GetTileLayer(target->xy) == MapLayer::Surface &&
			GetNativeUtilityTownAuthority() == MultiLayerTownPowerAuthority::Native;
	}
	/** @param proposal Exact revalidated electrical offer. @return Complete ordinary town-rate change or unchanged refusal. */
	bool Commit([[maybe_unused]] const UtilityMutationProposal &proposal) noexcept override { return ApplyUtilityTownGrowthRate(this->town, this->bonus); }
	/** @return The actual native town effect applied by this complete participant. */
	std::optional<UtilityConsumerEffect> EffectKind() const override { return UtilityConsumerEffect::TownGrowth; }
};

/** @copydoc ApplyUtilityTownEffect */
UtilityMutationReceipt ApplyUtilityTownEffect(UtilityConsumerID id)
{
	UtilityMutationReceipt refusal{UtilityPowerError::Refused, _utility_power.operation_sequence, 0};
	const auto *consumer = FindUtilityConsumer(id);
	if (consumer == nullptr) { refusal.error = UtilityPowerError::InvalidReference; return refusal; }
	// A city offer is explicit, current and once-only. No missing association,
	// repeated request or GS-suspended interval can renew a growth benefit.
	if (consumer->effect != UtilityConsumerEffect::TownGrowth || consumer->town == TownID::Invalid() || consumer->town_effect_applied || consumer->town_effect_suspended) return refusal;
	const auto allocation = GetUtilityAllocation(id);
	if (!allocation.has_value()) { refusal.error = UtilityPowerError::InvalidInterval; return refusal; }
	const auto bonus = ComputeNativeTownPowerBonus(GetNativeUtilityTownAuthority(), allocation->granted_energy_j, allocation->demand_energy_j);
	// A zero or unsupported benefit consumes nothing; absence of useful town
	// power cannot appear as a successful physical effect or hidden sink.
	if (!bonus.has_value() || *bonus == 0) return refusal;
	const auto proposal = PrepareUtilityMutation(UtilityMutationKind::Consume, {id, allocation->accounting_interval}, allocation->granted_energy_j);
	UtilityTownTransaction transaction(consumer->town, *bonus);
	return CommitUtilityMutation(proposal, &transaction);
}

/** @copydoc GetUtilityTownPowerBonus */
uint8_t GetUtilityTownPowerBonus(TownID town_id)
{
	if (GetNativeUtilityTownAuthority() != MultiLayerTownPowerAuthority::Native) return 0;
	const Town *town = Town::GetIfValid(town_id);
	// Actual custom GS authority always wins, including during commands and
	// nondestructive native queries; the viewer cannot restore a suspended offer.
	if (town == nullptr || town->flags.Test(TownFlag::CustomGrowth)) return 0;
	// Read only bounded registered offers, not every map plane or town. Binding
	// uniqueness ensures one accepted current benefit rather than stacked rates.
	for (const auto &consumer : _utility_power.consumers) {
		if (consumer.town != town_id) continue;
		if (!_utility_power.allocated || consumer.meter.accounting_interval != _utility_power.interval || !consumer.town_effect_applied || consumer.town_effect_suspended) return 0;
		return consumer.town_bonus_percent;
	}
	return 0;
}

/** @copydoc SuspendUtilityTownEffect */
void SuspendUtilityTownEffect(TownID town_id)
{
	if (_utility_mutation_committing) return;
	bool changed = false;
	// Script takeover suppresses this exact interval and expires its unused
	// electrical offer; settled city service and source lifetime use remain real.
	for (auto &consumer : _utility_power.consumers) {
		if (consumer.town != town_id) continue;
		consumer.town_effect_suspended = true;
		for (auto &share : consumer.shares) {
			const uint64_t unused = FreeUtilityShare(share);
			share.released_j += unused; FindUtilityProducer(share.producer)->reserved_j -= unused;
		}
		changed = true;
	}
	if (changed && _utility_power.operation_sequence < UINT64_MAX - 1) ++_utility_power.operation_sequence;
}

/** @copydoc RemoveUtilityTownBindings */
void RemoveUtilityTownBindings(TownID town_id)
{
	if (_utility_mutation_committing) return;
	SuspendUtilityTownEffect(town_id);
	// Delete the association before the town pool ID can be reused. Already
	// applied/consumed history is retained and cannot benefit its replacement.
	for (auto &consumer : _utility_power.consumers) {
		if (consumer.town == town_id) consumer.town = TownID::Invalid();
	}
}

/** @copydoc ResetUtilityPowerState */
void ResetUtilityPowerState()
{
	if (_utility_mutation_committing) return;
	_utility_power = {};
	_pending_utility_power.reset();
	_utility_mutation_committing = false;
	ResetUtilityPowerLoadStaging();
}

/** @copydoc HasUtilityPowerState */
bool HasUtilityPowerState()
{
	return _utility_power.interval != 1 || _utility_power.allocated || _utility_power.next_producer != 0 || _utility_power.next_consumer != 0 || _utility_power.next_link != 0 || _utility_power.operation_sequence != 0;
}

/** @copydoc ValidateUtilityPowerState */
UtilityPowerError ValidateUtilityPowerState(const UtilityPowerState &state, const MultiLayerProfileSelection &selection, bool check_world_refs, uint layer_count)
{
	// Empty older/Classic worlds retain an empty ledger; actual electrical
	// history requires the accepted native profile, never the startup selector.
	if (selection.id != MultiLayerGameplayProfileID::MultilayerIndustry &&
		(state.interval != 1 || state.allocated || state.next_producer != 0 || state.next_consumer != 0 || state.next_link != 0 || state.operation_sequence != 0 || !state.producers.empty() || !state.consumers.empty() || !state.links.empty())) return UtilityPowerError::UnsupportedProfile;
	// Zero is an unpublished sentinel and the terminal epoch cannot rotate;
	// neither may become an accepted accounting interval.
	if (state.interval == 0 || state.interval == UINT64_MAX) return UtilityPowerError::InvalidInterval;
	if (state.operation_sequence == UINT64_MAX) return UtilityPowerError::Limit;
	if (state.next_producer == UINT32_MAX || state.next_consumer == UINT32_MAX || state.next_link == UINT32_MAX) return UtilityPowerError::Limit;
	// This owner retains every allocated identity, including retired sources;
	// cursor gaps would discard saved fractions or allow identity substitution.
	if (state.producers.size() != state.next_producer || state.consumers.size() != state.next_consumer || state.links.size() != state.next_link) return UtilityPowerError::InvalidReference;
	// Both record families have independent finite storage limits; neither
	// may expand file acceptance into an unbounded simulation workload.
	if (state.producers.size() > MAX_UTILITY_INTERVAL_ACCOUNTS || state.consumers.size() > MAX_UTILITY_INTERVAL_ACCOUNTS || state.links.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return UtilityPowerError::Limit;
	// Only explicitly supported saved policies can own even an empty ledger;
	// an unknown numeric profile is never silently treated as Classic.
	if (selection.id != MultiLayerGameplayProfileID::Classic && selection.id != MultiLayerGameplayProfileID::MultilayerIndustry) return UtilityPowerError::UnsupportedProfile;
	size_t connection_count = 0, share_count = 0;
	std::map<UtilityProducerID, const UtilityPowerProducer *> sources;
	std::map<UtilityProducerID, uint64_t> open, used;
	std::map<UtilityLinkID, const UtilityPowerLink *> links;
	std::map<UtilityLinkID, uint64_t> flows;
	std::map<std::pair<UtilityConsumerID, UtilityProducerID>, uint64_t> transferred, imported;
	std::set<std::pair<UtilityConsumerID, UtilityConsumerID>> active_routes;
	std::set<TownID> bound_towns;
	// Exact retained link IDs and participant references must be complete
	// before imported source slices can resolve their original donor account.
	for (size_t index = 0; index < state.links.size(); ++index) {
		const auto &link = state.links[index];
		if (link.id.base() != index) return UtilityPowerError::InvalidReference;
		// Both endpoints name actual retained consumers; absent references can
		// never be repaired into an output account or a second generator.
		if (link.source.base() >= state.consumers.size() || link.destination.base() >= state.consumers.size()) return UtilityPowerError::InvalidReference;
		const auto &source = state.consumers[link.source.base()];
		const auto &destination = state.consumers[link.destination.base()];
		const auto admission = CheckUtilityPowerLink(link, source, destination, check_world_refs && link.active);
		if (admission != UtilityPowerError::None) return admission;
		// A single active directed participant pair has one metered capacity;
		// duplicate routes cannot multiply its otherwise bounded import budget.
		if (link.active && !active_routes.emplace(link.source, link.destination).second) return UtilityPowerError::InvalidTopology;
		// Metered flow belongs to the current closed epoch and exact requested
		// participants; registration in an open phase has zero credit.
		if (link.flow_j != 0 && (!state.allocated || source.meter.accounting_interval != state.interval || destination.meter.accounting_interval != state.interval)) return UtilityPowerError::InvalidInterval;
		links.emplace(link.id, &link);
	}
	UtilityProducerID previous{UINT32_MAX};
	// Strict source order/cursors and frozen units reject duplicate identities
	// and malformed carry before any candidate becomes live authority.
	for (const auto &source : state.producers) {
		const auto &meter = source.meter;
		if (meter.producer_id.base() >= state.next_producer || (previous.base() != UINT32_MAX && meter.producer_id <= previous)) return UtilityPowerError::InvalidReference;
		previous = meter.producer_id;
		if (source.owner.base() >= MAX_COMPANIES) return UtilityPowerError::InvalidOwner;
		if (to_underlying(meter.layer) >= (check_world_refs ? MultiLayerMap::GetLayerCount() : layer_count)) return UtilityPowerError::InvalidLayer;
		// Active loaded Deep sources require the saved unlock policy; preview
		// performs only structural checks until actual calendar/profile fixups.
		if (check_world_refs && source.active && meter.layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(selection, TimerGameCalendar::date)) return UtilityPowerError::InvalidLayer;
		// Retired identities may retain lineage after their company disappears;
		// active generation always requires a real loaded company.
		if (check_world_refs && source.active && !Company::IsValidID(source.owner)) return UtilityPowerError::InvalidOwner;
		// A never-published source has neither generation nor fractional history;
		// accepting carry here would manufacture electricity on its first call.
		if (meter.accounting_interval == 0 && (meter.generation_rate_w != 0 || meter.economy_ticks != 0 || meter.generation_energy_j != 0 || meter.remainder_mj != 0)) return UtilityPowerError::InvalidQuantity;
		if (to_underlying(source.kind) > to_underlying(UtilityGenerationKind::OrbitalPanel)) return UtilityPowerError::InvalidReference;
		// Source meters belong exclusively to electricity generation; binding
		// a consumer ID or thermal domain here would mix quantity owners.
		if (meter.domain != UtilityEnergyDomain::Electricity || meter.consumer_id.base() != UINT32_MAX) return UtilityPowerError::InvalidReference;
		// Publication uses frozen rate/duration/energy/carry limits and cannot
		// name a future interval that bypasses once-only generation.
		if (meter.accounting_interval > state.interval || meter.generation_rate_w > MAX_UTILITY_RATE_W || meter.economy_ticks > MAX_UTILITY_INTERVAL_TICKS || meter.generation_energy_j > MAX_UTILITY_ACCOUNT_ENERGY_J || meter.remainder_mj >= 1000) return UtilityPowerError::InvalidQuantity;
		// Demand and grants belong to consumer meters; unused source slots
		// cannot conceal a second reservation or generation credit.
		if (meter.demand_energy_j != 0 || meter.granted_energy_j != 0 || meter.unmet_energy_j != 0) return UtilityPowerError::InvalidQuantity;
		// Neither an open generation phase nor a retired source can promise
		// usable output; source loss must release all remaining reservations.
		if ((!state.allocated && source.reserved_j != 0) || (!source.active && source.reserved_j != 0)) return UtilityPowerError::InvalidQuantity;
		// Lifetime settlement cannot exceed real publication and must retain
		// at least current interval generation even after prior rotations.
		if (source.consumed_lifetime_j > source.generated_lifetime_j || source.generated_lifetime_j < meter.generation_energy_j) return UtilityPowerError::InvalidQuantity;
		// A saved publication must correspond to its actual declared rate/ticks
		// and a legal previous fractional carry, never arbitrary invented joules.
		const uint64_t emitted = meter.generation_energy_j * 1000 + meter.remainder_mj;
		const uint64_t produced = meter.generation_rate_w * meter.economy_ticks * 27;
		if (emitted < produced || emitted - produced >= 1000) return UtilityPowerError::InvalidQuantity;
		sources.emplace(meter.producer_id, &source);
	}
	UtilityConsumerID previous_consumer{UINT32_MAX};
	// Original grant totals equal the exact saved slices; free/use/release
	// balances are checked from those records instead of reconstructed energy.
	for (const auto &consumer : state.consumers) {
		const auto &meter = consumer.meter;
		if (meter.consumer_id.base() >= state.next_consumer || (previous_consumer.base() != UINT32_MAX && meter.consumer_id <= previous_consumer)) return UtilityPowerError::InvalidReference;
		previous_consumer = meter.consumer_id;
		if (consumer.owner.base() >= MAX_COMPANIES) return UtilityPowerError::InvalidOwner;
		// Preview cannot dereference live pools; full acceptance must bind this
		// consumer to its actual saved company before granting usable energy.
		if (check_world_refs && consumer.active && !Company::IsValidID(consumer.owner)) return UtilityPowerError::InvalidOwner;
		// Company tombstones have no usable topology or town identity; retained
		// shares are checked below for exact settlement and zero free balance.
		if (!consumer.active && (!consumer.linked_producers.empty() || consumer.town != TownID::Invalid())) return UtilityPowerError::InvalidReference;
		if (to_underlying(meter.layer) >= (check_world_refs ? MultiLayerMap::GetLayerCount() : layer_count)) return UtilityPowerError::InvalidLayer;
		// A valid numeric Deep layer cannot authorize operation before the
		// calendar gate, even if all linked sources are retired or absent.
		if (check_world_refs && meter.layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(selection, TimerGameCalendar::date)) return UtilityPowerError::InvalidLayer;
		// Consumers use only the electrical ledger and a supported bounded
		// effect, with no producer identity hidden in the same meter.
		if (meter.domain != UtilityEnergyDomain::Electricity || meter.producer_id.base() != UINT32_MAX || to_underlying(consumer.effect) > to_underlying(UtilityConsumerEffect::TownGrowth)) return UtilityPowerError::InvalidReference;
		// Stored grants must fit actual capacity/demand and match original
		// unmet demand; future epochs cannot supply renewed reservations.
		if (consumer.capacity_j > MAX_UTILITY_ACCOUNT_ENERGY_J || meter.demand_energy_j > consumer.capacity_j || meter.granted_energy_j > meter.demand_energy_j || meter.unmet_energy_j != meter.demand_energy_j - meter.granted_energy_j || meter.accounting_interval > state.interval) return UtilityPowerError::InvalidQuantity;
		// Generation duration/fractions remain at the source; a consumer
		// cannot introduce its own output or fractional electrical carry.
		if (meter.generation_rate_w != 0 || meter.generation_energy_j != 0 || meter.economy_ticks != 0 || meter.remainder_mj != 0) return UtilityPowerError::InvalidQuantity;
		if (consumer.effect != UtilityConsumerEffect::TownGrowth) {
			// Ordinary work/inventory consumers cannot smuggle a city association
			// or independent growth multiplier into otherwise valid energy rows.
			if (consumer.town != TownID::Invalid() || consumer.town_bonus_percent != 0 || consumer.town_effect_applied || consumer.town_effect_suspended) return UtilityPowerError::InvalidReference;
		} else {
			// Only Surface city offers have bounded native state, and a posted
			// benefit must be tied to this closed current accounting interval.
			if (meter.layer != MapLayer::Surface || consumer.town_bonus_percent > 25 || (!consumer.town_effect_applied && consumer.town_bonus_percent != 0) ||
				(consumer.town_effect_applied && (!state.allocated || meter.accounting_interval != state.interval))) return UtilityPowerError::InvalidQuantity;
			if (consumer.town != TownID::Invalid()) {
				if (!bound_towns.insert(consumer.town).second) return UtilityPowerError::InvalidTopology;
				if (check_world_refs) {
					const Town *town = Town::GetIfValid(consumer.town);
					// Actual loaded Surface identity and custom-script authority are
					// verified after real pools; preview owns no town references.
					if (town == nullptr || GetTileLayer(town->xy) != MapLayer::Surface ||
						(town->flags.Test(TownFlag::CustomGrowth) && consumer.town_effect_applied && !consumer.town_effect_suspended)) return UtilityPowerError::InvalidReference;
				}
			} else if (consumer.town_effect_applied && !consumer.town_effect_suspended) {
				return UtilityPowerError::InvalidReference; // Deleted towns retain only suspended settled history, never a reusable growth offer.
			}
		}
		// Per-account graph limits precede whole-store accumulation, bounding
		// both physical eligibility and saved reservation traversal.
		if (consumer.linked_producers.size() > MAX_UTILITY_INTERVAL_ACCOUNTS || consumer.shares.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return UtilityPowerError::Limit;
		connection_count += consumer.linked_producers.size();
		share_count += consumer.shares.size();
		// Whole-ledger limits bound memory and work independently of each
		// individual consumer, including crafted many-to-many connectivity.
		if (connection_count > MAX_UTILITY_POWER_CONNECTIONS || share_count > MAX_UTILITY_POWER_SHARES) return UtilityPowerError::Limit;
		UtilityProducerID previous_link{UINT32_MAX};
		// Physical eligibility cannot contain duplicates, dead/foreign producers
		// or implicit cross-layer shortcuts.
		for (const auto id : consumer.linked_producers) {
			const auto found = sources.find(id);
			if (found == sources.end()) return UtilityPowerError::InvalidReference;
			if ((previous_link.base() != UINT32_MAX && id <= previous_link) || !found->second->active || found->second->owner != consumer.owner || found->second->meter.layer != meter.layer) return UtilityPowerError::InvalidTopology;
			previous_link = id;
		}
		uint64_t allocated = 0;
		std::optional<std::pair<UtilityProducerID, UtilityLinkID>> previous_share;
		// Retired origins may retain settled lineage but no usable balance;
		// validate each addition before arithmetic to prevent crafted overflow.
		for (const auto &share : consumer.shares) {
			const auto found = sources.find(share.producer);
			if (found == sources.end()) return UtilityPowerError::InvalidReference;
			const std::pair identity(share.producer, share.imported_link);
			// Original-source/route keys are unique and stable, including multiple
			// independently metered imports from the same generation account.
			if (previous_share.has_value() && identity <= *previous_share) return UtilityPowerError::InvalidReference;
			previous_share = identity;
			// Each independent use/release/transfer fits its original allocation
			// before subtraction; crafted rows cannot wrap into free availability.
			if (share.allocated_j == 0 || share.allocated_j > meter.granted_energy_j - allocated || share.consumed_j > share.allocated_j ||
				share.released_j > share.allocated_j - share.consumed_j || share.transferred_j > share.allocated_j - share.consumed_j - share.released_j) return UtilityPowerError::InvalidQuantity;
			const auto &source = *found->second;
			if (source.owner != consumer.owner) return UtilityPowerError::InvalidTopology;
			const uint64_t remaining = FreeUtilityShare(share);
			// A retired consumer cannot retain usable energy even if its old
			// numeric company ID now belongs to a new live company.
			if (!consumer.active && remaining != 0) return UtilityPowerError::InvalidQuantity;
			if (share.imported_link.base() == UINT32_MAX) {
				if (source.meter.layer != meter.layer) return UtilityPowerError::InvalidTopology;
				if (share.transferred_j != 0) transferred[{meter.consumer_id, share.producer}] += share.transferred_j;
			} else {
				const auto route = links.find(share.imported_link);
				if (route == links.end()) return UtilityPowerError::InvalidReference;
				const auto &link = *route->second;
				// Imports belong to this exact destination and original donor layer;
				// they may settle locally but never re-export in the same interval.
				if (link.destination != meter.consumer_id || state.consumers[link.source.base()].meter.layer != source.meter.layer || share.transferred_j != 0) return UtilityPowerError::InvalidTopology;
				if (!link.active && remaining != 0) return UtilityPowerError::InvalidQuantity;
				// Total retained imports must fit their one actual directed flow;
				// including settled/released slices preserves exact lineage history.
				if (share.allocated_j > link.flow_j - flows[link.id]) return UtilityPowerError::InvalidQuantity;
				flows[link.id] += share.allocated_j;
				imported[{link.source, share.producer}] += share.allocated_j;
			}
			// Retired origins preserve settled/released history but never an
			// available balance that could revive after a retry or reload.
			if (!source.active && remaining != 0) return UtilityPowerError::InvalidQuantity;
			const uint64_t total = open[share.producer] + used[share.producer];
			if (remaining > source.meter.generation_energy_j - total || share.consumed_j > source.meter.generation_energy_j - total - remaining) return UtilityPowerError::InvalidQuantity;
			open[share.producer] += remaining;
			used[share.producer] += share.consumed_j;
			allocated += share.allocated_j;
		}
		if (allocated != meter.granted_energy_j) return UtilityPowerError::InvalidQuantity;
		if (consumer.town_effect_applied) {
			uint64_t consumed = 0;
			// The posted city benefit derives only from its exact settled slices,
			// rather than an independently editable saved bonus or copied grant.
			for (const auto &share : consumer.shares) consumed += share.consumed_j;
			const auto bonus = ComputeNativeTownPowerBonus(MultiLayerTownPowerAuthority::Native, consumed, meter.demand_energy_j);
			if (!bonus.has_value() || *bonus != consumer.town_bonus_percent) return UtilityPowerError::InvalidQuantity;
		}
		// A consumer outside this closed allocation has no active request or
		// saved slices; prior epochs cannot be carried as fresh usable energy.
		if ((!state.allocated || meter.accounting_interval != state.interval) && (meter.demand_energy_j != 0 || allocated != 0 || !consumer.shares.empty())) return UtilityPowerError::InvalidInterval;
	}
	// Exact reserve sums conserve source promises; no missing/corrupt row is
	// repaired into a new source or a duplicate electrical credit.
	for (const auto &source : state.producers) {
		if (source.reserved_j != open[source.meter.producer_id]) return UtilityPowerError::InvalidQuantity;
		if (used[source.meter.producer_id] > source.consumed_lifetime_j) return UtilityPowerError::InvalidQuantity;
		// Historic settled use must fit historic actual generation separately
		// from the current interval; its current free budget cannot fund an
		// invented prior consumption counter as well as today's reservations.
		if (source.consumed_lifetime_j - used[source.meter.producer_id] > source.generated_lifetime_j - source.meter.generation_energy_j) return UtilityPowerError::InvalidQuantity;
	}
	// Every donor debit has exactly one conserved imported allocation and
	// every directed route agrees with its saved actual flow, never generation.
	if (transferred != imported) return UtilityPowerError::InvalidQuantity;
	for (const auto &link : state.links) {
		if (link.flow_j != flows[link.id]) return UtilityPowerError::InvalidQuantity;
	}
	return UtilityPowerError::None;
}

/** @copydoc StageLoadedUtilityPowerState */
void StageLoadedUtilityPowerState(UtilityPowerState state)
{
	if (_utility_mutation_committing) return;
	_pending_utility_power = std::move(state);
}

/** @copydoc AcceptLoadedUtilityPowerState */
bool AcceptLoadedUtilityPowerState()
{
	if (_utility_mutation_committing) return false;
	if (!_pending_utility_power.has_value()) return true;
	if (ValidateUtilityPowerState(*_pending_utility_power, GetCurrentMultiLayerProfileSelection()) != UtilityPowerError::None) return false;
	_utility_power = std::move(*_pending_utility_power);
	_pending_utility_power.reset();
	return true;
}
