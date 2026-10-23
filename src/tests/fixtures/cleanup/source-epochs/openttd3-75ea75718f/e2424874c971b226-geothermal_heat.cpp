/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_heat.cpp Pure typed generator/direct-processing admission; shared utility accounting retains every heat quantity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups2-4
 */

#include "stdafx.h"
#include "geothermal_heat.h"
#include "geothermal_collector.h"
#include "geothermal_utility.h"
#include "excavation.h"
#include "tile_map.h"
#include "company_base.h"
#include "company_func.h"
#include "openttd.h"
#include "core/backup_type.hpp"
#include <set>
#include "safeguards.h"

static std::map<UtilityConsumerID, TileIndex> _geothermal_heat_consumers; ///< Physical ports only; shared utility IDs/control/role/quantities retain their existing owner.
static std::vector<GeothermalHeatRoute> _geothermal_heat_routes; ///< Current exact source-slice path references, with no private energy or use balance.
static uint64_t _geothermal_route_interval = 0; ///< Native grant epoch of retained paths; zero before a physical allocation.
static std::optional<std::tuple<std::vector<GeothermalHeatConsumerBinding>, std::vector<GeothermalHeatRoute>, uint64_t>> _pending_geothermal_heat; ///< Unpublished saved references, never operational callbacks or energy.

/** @param row Existing copied native consumer. @return Its original issued identity for independent reference lookup. */
static UtilityConsumerID HeatConsumerIdentity(const UtilityPowerConsumer &row) { return row.meter.consumer_id; }
/** @param row Existing copied native source. @return Its original issued identity for independent reference lookup. */
static UtilityProducerID HeatProducerIdentity(const UtilityPowerProducer &row) { return row.meter.producer_id; }

/** @copydoc PrepareGeothermalHeatRequest */
std::optional<UtilityPowerRequest> PrepareGeothermalHeatRequest(const GeothermalHeatRequest &request)
{
	const auto *consumer = GetUtilityPowerConsumer(request.consumer);
	const auto &profile = GetCurrentMultiLayerProfileSelection();
	// Requests cannot authorize a foreign company, unavailable Deep technology
	// or electrical account to consume heat. No owner is inferred from a port.
	if (_game_mode != GameMode::Normal || profile.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile.legacy ||
		!CheckDeepLayerAvailability(profile, TimerGameCalendar::date) || !Company::IsValidID(request.owner) || request.owner != _current_company ||
		consumer == nullptr || !consumer->active || consumer->owner != request.owner || consumer->meter.domain != UtilityEnergyDomain::Heat ||
		consumer->meter.layer != MapLayer::DeepUnderground) return std::nullopt;
	// A consumer's fixed whole benefit and bounded request must agree before
	// the later network reserves scarce heat. Neither role has implicit priority.
	if (to_underlying(request.use) > to_underlying(UtilityHeatUse::ElectricalGenerator) || request.needed_j > consumer->capacity_j ||
		consumer->effect != (request.use == UtilityHeatUse::DirectRecipe ? UtilityConsumerEffect::HeatDirectRecipe : UtilityConsumerEffect::HeatGenerator)) return std::nullopt;
	// A real excavated Deep port is necessary but supplies no connectivity.
	// Surface coordinates, solid rock and cold blueprint IDs cannot be ports.
	if (!IsValidTile(request.endpoint) || GetTileLayer(request.endpoint) != MapLayer::DeepUnderground) return std::nullopt;
	if (CheckExcavatedFootprint(std::array{request.endpoint}).error != ExcavationError::None) return std::nullopt;
	return UtilityPowerRequest{request.consumer, request.needed_j};
}

/** @copydoc RegisterGeothermalHeatConsumer */
std::optional<UtilityConsumerID> RegisterGeothermalHeatConsumer(CompanyID owner, TileIndex endpoint, UtilityHeatUse use, uint64_t capacity_j, bool execute)
{
	const auto cell = GetGeothermalUtilityLink(endpoint, UtilityEnergyDomain::Heat);
	// A physical consumer requires an actual owned installed port before a
	// shared identity is acquired; cold plans/proximity supply no connection.
	if (owner != _current_company || !cell.has_value() || cell->owner != owner || _geothermal_heat_consumers.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS) return std::nullopt;
	const auto candidate = RegisterUtilityHeatConsumer(owner, use, capacity_j, {}, false);
	if (!candidate.has_value()) return std::nullopt;
	if (execute) {
		const auto [position, inserted] = _geothermal_heat_consumers.emplace(*candidate, endpoint); // Acquire physical reference storage before native identity publication.
		if (!inserted) return std::nullopt;
		if (RegisterUtilityHeatConsumer(owner, use, capacity_j, {}) != candidate) { _geothermal_heat_consumers.erase(position); return std::nullopt; }
	}
	return candidate;
}

/** @param producer Actual registered source. @return Completed actual machinery bound to this exact meter, or invalid; no source or budget is inferred from coordinates. */
static IndustryID HeatSourceCollector(UtilityProducerID producer)
{
	// Only bounded native machinery bindings are inspected; the original
	// natural field and native shared meter remain their respective owners.
	for (const auto &collector : CaptureGeothermalCollectors()) {
		if (collector.heat_producer == producer && IsGeothermalCollectorOperational(collector.industry)) return collector.industry;
	}
	return IndustryID::Invalid();
}

/** @copydoc PrepareVentHeatAllocation */
GeothermalHeatAllocation PrepareVentHeatAllocation(uint64_t interval, std::span<const GeothermalHeatDemand> demands)
{
	GeothermalHeatAllocation plan; plan.interval = interval; plan.sequence = CaptureUtilityPowerState().operation_sequence; plan.topology_revision = GetUtilityLocalLinkRevision();
	if (_current_company != OWNER_DEITY) { plan.error = UtilityPowerError::InvalidOwner; return plan; }
	// A repeated or guessed epoch cannot reserve another copy of the source
	// offer; bounded whole requests precede any routing or native mutation.
	if (interval != GetUtilityPowerInterval() || interval == UINT64_MAX || IsUtilityHeatAllocated()) { plan.error = UtilityPowerError::InvalidInterval; return plan; }
	if (demands.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) { plan.error = UtilityPowerError::Limit; return plan; }
	std::vector<GeothermalHeatDemand> ordered(demands.begin(), demands.end());
	/** @param demand Actual transient whole consumer. @return Its shared identity for the frozen deterministic priority. */
	std::ranges::sort(ordered, {}, [](const auto &demand) { return demand.request.consumer; });
	std::map<UtilityProducerID, uint64_t> remaining;
	std::map<UtilityLinkID, uint64_t> path_used;
	std::set<UtilityConsumerID> seen;
	size_t references = 0;
	const auto native = CaptureUtilityPowerState();
	std::map<UtilityProducerID, TileIndex> source_ports;
	uint32_t ticks = 0;
	// Capture ready native source bindings once per proposal. A common interval
	// gives every shared pipe one duration, even where two sources compete;
	// unpublished sources have zero supply and cannot invent extra pipe time.
	for (const auto &collector : CaptureGeothermalCollectors()) {
		if (!IsGeothermalCollectorOperational(collector.industry)) continue;
		const auto *source = GetUtilityPowerProducer(collector.heat_producer);
		if (source == nullptr || !source->active) continue;
		source_ports.emplace(collector.heat_producer, GetGeothermalSiteClaim(collector.site)->site.endpoint);
		if (source->meter.accounting_interval != interval) continue;
		if (ticks != 0 && ticks != source->meter.economy_ticks) { plan.error = UtilityPowerError::InvalidInterval; return plan; } // Mixed durations cannot multiply capacity in the same shared physical interval.
		ticks = source->meter.economy_ticks;
	}
	// Consumers sort before sources. Generator and direct recipes compete for
	// one budget; feasible input/output/auxiliary work is checked before demand.
	for (const auto &demand : ordered) {
		const auto &request = demand.request;
		const auto binding = _geothermal_heat_consumers.find(request.consumer);
		if (binding == _geothermal_heat_consumers.end() || binding->second != request.endpoint || !seen.insert(request.consumer).second) { plan.error = UtilityPowerError::InvalidReference; return plan; }
		AutoRestoreBackup actor(_current_company, request.owner);
		const auto admitted = PrepareGeothermalHeatRequest(request);
		if (!admitted.has_value()) { plan.error = UtilityPowerError::InvalidReference; return plan; }
		uint64_t needed = request.needed_j;
		// Actual whole consumers can become starved/full between ticks. That
		// owner requests no heat, leaving inventory and the source available to
		// other peers; no animation or copied stock declares work feasible.
		if (needed != 0 && (demand.participant == nullptr || demand.participant->ThermalUse() != request.use || !demand.participant->ValidateDemand(request))) needed = 0;
		plan.requests.push_back({request.consumer, needed});
		uint64_t left = needed;
		// Stable native producer order preserves one original-source lineage.
		// Every traversed installed cell debits its transient residual capacity
		// in this proposal, shared across all earlier consumers/sources.
		for (const auto &source : native.producers) {
			if (!source.active || source.owner != request.owner || source.meter.domain != UtilityEnergyDomain::Heat) continue;
			const auto port = source_ports.find(source.meter.producer_id);
			if (port == source_ports.end()) continue;
			std::vector<UtilityLinkID> excluded;
			// Only saturated previously traversed cells need exclusion. Bounded
			// BFS can select an installed alternative without a rock-plane scan
			// or an optimal-flow solver; ports remain part of every chosen route.
			for (const auto &[cell, used] : path_used) {
				const auto capacity = IntegrateUtilityGeneration(GetUtilityPowerLink(cell)->local_capacity_w, ticks, 0);
				if (capacity.has_value() && used >= capacity->energy_j) excluded.push_back(cell);
			}
			const auto path = FindGeothermalUtilityPath(request.owner, UtilityEnergyDomain::Heat, port->second, request.endpoint, excluded);
			if (!path.has_value()) continue;
			const auto id = source.meter.producer_id;
			remaining.try_emplace(id, source.meter.accounting_interval == interval ? source.meter.generation_energy_j : 0);
			uint64_t room = MAX_UTILITY_ACCOUNT_ENERGY_J;
			for (const auto cell : path->cells) {
				const auto *link = GetUtilityPowerLink(cell);
				const auto capacity = ticks == 0 ? std::optional<UtilityEnergyIntegration>{UtilityEnergyIntegration{}} : IntegrateUtilityGeneration(link->local_capacity_w, ticks, 0);
				if (!capacity.has_value() || path_used[cell] > capacity->energy_j) { plan.error = UtilityPowerError::InvalidQuantity; return plan; } // Installed synchronized rate and prior reservations bound the shared path, with no fractional pipe store.
				room = std::min(room, capacity->energy_j - path_used[cell]);
			}
			const uint64_t quantity = std::min({left, remaining[id], room});
			if (plan.shares.size() >= MAX_UTILITY_POWER_CONNECTIONS) { plan.error = UtilityPowerError::Limit; return plan; } // Bound the complete eligibility proposal before storing another source pair, including zero-quantity connectivity.
			plan.shares.push_back({request.consumer, id, quantity});
			if (quantity == 0) continue;
			references += path->cells.size();
			if (references > MAX_UTILITY_POWER_SHARES) { plan.error = UtilityPowerError::Limit; return plan; }
			plan.routes.push_back({request.consumer, id, path->cells});
			for (const auto cell : path->cells) path_used[cell] += quantity; // Transient path reservations constrain peers; live quantities remain unmodified until native commit.
			remaining[id] -= quantity; left -= quantity;
		}
	}
	plan.error = AllocateUtilityHeatRoutes(interval, plan.requests, plan.shares, false);
	return plan;
}

/** @copydoc AllocateVentHeat */
UtilityPowerError AllocateVentHeat(uint64_t interval, std::span<const GeothermalHeatDemand> demands, bool execute)
{
	auto plan = PrepareVentHeatAllocation(interval, demands);
	if (plan.error != UtilityPowerError::None) return plan.error;
	if (!execute) return UtilityPowerError::None;
	// Preparation owns no resource. Recheck both actual authority revisions
	// immediately before the one native commit; acquire route storage first.
	if (plan.sequence != CaptureUtilityPowerState().operation_sequence || plan.topology_revision != GetUtilityLocalLinkRevision()) return UtilityPowerError::StaleMutation;
	const auto result = AllocateUtilityHeatRoutes(interval, plan.requests, plan.shares);
	if (result != UtilityPowerError::None) return result;
	_geothermal_heat_routes = std::move(plan.routes); _geothermal_route_interval = interval;
	return UtilityPowerError::None;
}

/** @param grant Exact owned native reservation. @return Whether every remaining source slice retains an operational original source and complete active owned installed path. */
static bool IsGeothermalHeatRouteUsable(UtilityGrantID grant)
{
	if (grant.interval != _geothermal_route_interval) return false;
	const auto *consumer = GetUtilityPowerConsumer(grant.consumer);
	if (consumer == nullptr) return false;
	// Native source slices alone own remaining heat. Each usable positive
	// slice must resolve its saved physical path before a whole benefit runs.
	for (const auto &share : consumer->shares) {
		if (share.allocated_j == share.consumed_j + share.released_j + share.transferred_j) continue;
		/** @param candidate Retained physical slice. @return Whether it belongs to this exact shared consumer/source pair. */
		const auto route = std::ranges::find_if(_geothermal_heat_routes, [&](const auto &candidate) {
			return candidate.consumer == grant.consumer && candidate.producer == share.producer; // Exact saved source/consumer pair, never a nearby replacement source.
		});
		if (route == _geothermal_heat_routes.end()) return false;
		if (HeatSourceCollector(share.producer) == IndustryID::Invalid()) return false;
		for (const auto id : route->cells) {
			const auto *cell = GetUtilityPowerLink(id);
			if (cell == nullptr || !cell->active || cell->kind != UtilityLinkKind::LocalCell || cell->local_domain != UtilityEnergyDomain::Heat || cell->local_owner != consumer->owner) return false; // Removed, foreign or replaced cells cannot revive unused rights under an old route identity.
		}
	}
	return true;
}

/** Actual consumer wrapper joins complete work validation and live physical path admission to the same native energy settlement. */
class GeothermalRoutedParticipant final : public UtilityMutationParticipant {
	GeothermalHeatParticipant &physical; ///< Actual complete work/input/output/auxiliary owner, never copied stock.
public:
	/** @param physical Actual consumer receiving the bounded whole benefit. */
	explicit GeothermalRoutedParticipant(GeothermalHeatParticipant &physical) : physical(physical) {}
	/** @return Immutable thermal benefit domain. */
	UtilityEnergyDomain EnergyDomain() const override { return UtilityEnergyDomain::Heat; }
	/** @return Actual complete consumer's frozen direct/generator role. */
	std::optional<UtilityHeatUse> ThermalUse() const override { return this->physical.ThermalUse(); }
	/** @param proposal Exact native source reservation. @return Whole physical/path/work feasibility without changing any owner. */
	bool Validate(const UtilityMutationProposal &proposal) const override { return IsGeothermalHeatRouteUsable(proposal.grant) && this->physical.Validate(proposal); }
	/** @param proposal Exact checked native quantity. @return Whole actual work success or unchanged refusal, with no utility reentry. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override { return this->physical.Commit(proposal); }
};

/** @copydoc ConsumeGeothermalHeat */
UtilityMutationReceipt ConsumeGeothermalHeat(UtilityGrantID grant, uint64_t quantity_j, GeothermalHeatParticipant &participant)
{
	const auto proposal = PrepareUtilityHeatMutation(UtilityMutationKind::Consume, grant, quantity_j);
	GeothermalRoutedParticipant routed(participant); return CommitUtilityMutation(proposal, &routed);
}

/** @copydoc ConvertGeothermalHeat */
UtilityHeatConversionReceipt ConvertGeothermalHeat(const UtilityHeatConversionProposal &proposal, GeothermalHeatParticipant &participant)
{
	GeothermalRoutedParticipant routed(participant);
	return CommitUtilityHeatConversion(proposal, routed);
}

/** @copydoc CaptureGeothermalHeatConsumers */
std::vector<GeothermalHeatConsumerBinding> CaptureGeothermalHeatConsumers()
{
	std::vector<GeothermalHeatConsumerBinding> rows; rows.reserve(_geothermal_heat_consumers.size());
	for (const auto &[consumer, endpoint] : _geothermal_heat_consumers) rows.push_back({consumer, endpoint}); // Stable actual shared identity order transports physical references only.
	return rows;
}
/** @copydoc CaptureGeothermalHeatRoutes */
std::vector<GeothermalHeatRoute> CaptureGeothermalHeatRoutes() { return _geothermal_heat_routes; }
/** @copydoc GetGeothermalHeatRoutes */
std::span<const GeothermalHeatRoute> GetGeothermalHeatRoutes() { return _geothermal_heat_routes; }
/** @copydoc GetGeothermalHeatRouteInterval */
uint64_t GetGeothermalHeatRouteInterval() { return _geothermal_route_interval; }
/** @copydoc ResetGeothermalHeatState */
void ResetGeothermalHeatState() { _geothermal_heat_consumers.clear(); _geothermal_heat_routes.clear(); _geothermal_route_interval = 0; _pending_geothermal_heat.reset(); ResetGeothermalHeatRouteLoadStaging(); }

/** @copydoc ClearGeothermalHeatRoutes */
void ClearGeothermalHeatRoutes() { _geothermal_heat_routes.clear(); _geothermal_route_interval = 0; }

/** @copydoc StageLoadedGeothermalHeatState */
void StageLoadedGeothermalHeatState(std::vector<GeothermalHeatConsumerBinding> bindings, std::vector<GeothermalHeatRoute> routes, uint64_t interval)
{
	_pending_geothermal_heat.emplace(std::move(bindings), std::move(routes), interval);
}

/** @copydoc ValidateGeothermalHeatRouteState */
bool ValidateGeothermalHeatRouteState(std::span<const GeothermalHeatConsumerBinding> bindings, std::span<const GeothermalHeatRoute> routes, uint64_t interval,
	const UtilityPowerState &native, uint32_t width, uint32_t height, uint8_t layers, const std::map<UtilityProducerID, TileIndex> &source_ports)
{
	// Saved ports and route rights contain references only. Resolve the same
	// copied native quantities and immutable physical rates before publishing.
	// Explicit single-plane geometry never borrows the running world's map.
	if (bindings.size() > MAX_UTILITY_INTERVAL_ACCOUNTS || routes.size() > MAX_UTILITY_POWER_SHARES ||
		(interval != 0 && (interval != native.interval || !native.heat_allocated)) || (interval == 0 && !routes.empty()) ||
		(!bindings.empty() && (layers != 4 || width == 0 || height == 0))) return false;
	const uint64_t plane = static_cast<uint64_t>(width) * height;
	std::map<UtilityConsumerID, TileIndex> accepted;
	for (const auto &binding : bindings) {
		const auto consumer = std::ranges::find(native.consumers, binding.consumer, HeatConsumerIdentity); // Existing shared identity, never feature-local registration during preview.
		if (consumer == native.consumers.end() || consumer->meter.domain != UtilityEnergyDomain::Heat || consumer->meter.layer != MapLayer::DeepUnderground || plane == 0 || binding.endpoint.base() / plane != to_underlying(MapLayer::DeepUnderground) ||
			!accepted.emplace(binding.consumer, binding.endpoint).second) return false; // Invalid/shared/wrong-layer identity cannot provide an implicit consumer port.
	}
	std::set<std::pair<UtilityConsumerID, UtilityProducerID>> pairs;
	std::map<UtilityLinkID, uint64_t> reserved;
	size_t references = 0;
	uint32_t ticks = 0;
	// Each retained source slice maps to exactly one original cardinal path.
	// Inactive nodes may retain consumed history after loss; live use checks
	// actual activity/source availability again before any complete benefit.
	for (const auto &route : routes) {
		const auto endpoint = accepted.find(route.consumer);
		const auto consumer = std::ranges::find(native.consumers, route.consumer, HeatConsumerIdentity);
		const auto source = std::ranges::find(native.producers, route.producer, HeatProducerIdentity);
		if (endpoint == accepted.end() || consumer == native.consumers.end() || source == native.producers.end() || source->meter.domain != UtilityEnergyDomain::Heat || source->meter.accounting_interval != interval || route.cells.empty() ||
			!pairs.emplace(route.consumer, route.producer).second) return false;
		if (ticks != 0 && ticks != source->meter.economy_ticks) return false; // One epoch supplies one physical capacity duration across all original source slices.
		ticks = source->meter.economy_ticks;
		const auto share = std::ranges::find(consumer->shares, route.producer, &UtilityPowerShare::producer);
		if (share == consumer->shares.end()) return false;
		const uint64_t unused = share->allocated_j - share->consumed_j - share->released_j - share->transferred_j;
		TileIndex previous = INVALID_TILE; std::set<UtilityLinkID> unique;
		for (const auto id : route.cells) {
			const auto cell = std::ranges::find(native.links, id, &UtilityPowerLink::id);
			if (++references > MAX_UTILITY_POWER_SHARES || cell == native.links.end() || cell->kind != UtilityLinkKind::LocalCell || cell->local_domain != UtilityEnergyDomain::Heat ||
				cell->source_endpoint.base() / plane != to_underlying(MapLayer::DeepUnderground) || !unique.insert(id).second) return false; // Missing/repeated/nonthermal cells cannot multiply a source path's capacity.
			const auto tile = cell->source_endpoint;
			if (unused != 0 && cell->active && cell->local_owner != consumer->owner) return false; // Foreign active infrastructure cannot carry a forged usable reservation; released/consumed history retains its original route after control loss.
			if (previous != INVALID_TILE) {
				const auto dx = std::abs(static_cast<int>(tile.base() % width) - static_cast<int>(previous.base() % width));
				const auto dy = std::abs(static_cast<int>((tile.base() / width) % height) - static_cast<int>((previous.base() / width) % height));
				if (dx + dy != 1) return false;
			}
			const auto limit = IntegrateUtilityGeneration(cell->local_capacity_w, source->meter.economy_ticks, 0);
			if (!limit.has_value()) return false;
			reserved[id] += share->allocated_j - share->released_j;
			if (reserved[id] > limit->energy_j) return false; // Shared source use plus unreleased rights remain bounded by each actual traversed cell's interval capacity.
			previous = tile;
		}
		if (previous != endpoint->second) return false;
		const auto port = source_ports.find(route.producer);
		const auto first = std::ranges::find(native.links, route.cells.front(), &UtilityPowerLink::id);
		if ((source->active && port == source_ports.end()) || (port != source_ports.end() && first->source_endpoint != port->second)) return false; // A live source path begins at its actual constructed port; retired source history needs no replacement machinery.
	}
	for (const auto &[id, endpoint] : accepted) {
		const auto consumer = std::ranges::find(native.consumers, id, HeatConsumerIdentity);
		for (const auto &share : consumer->shares) {
			if (consumer->meter.accounting_interval == interval && !pairs.contains({id, share.producer})) return false; // Omitting a current positive slice's route cannot turn it into geometry-free usable heat.
		}
	}
	return true;
}

/** @copydoc AcceptLoadedGeothermalHeatState */
bool AcceptLoadedGeothermalHeatState()
{
	if (!_pending_geothermal_heat.has_value()) return true;
	const auto &[bindings, routes, interval] = *_pending_geothermal_heat;
	std::map<UtilityProducerID, TileIndex> ports;
	// Source associations come from accepted actual construction, including
	// retained incomplete/abandoned history; load never manufactures energy.
	for (const auto &collector : CaptureGeothermalCollectors()) {
		if (collector.heat_producer.base() == UINT32_MAX) continue;
		ports.emplace(collector.heat_producer, GetGeothermalSiteClaim(collector.site)->site.endpoint);
	}
	if (!ValidateGeothermalHeatRouteState(bindings, routes, interval, CaptureUtilityPowerState(), Map::SizeX(), Map::SizeY(), static_cast<uint8_t>(Map::LayerCount()), ports)) return false;
	std::map<UtilityConsumerID, TileIndex> accepted;
	for (const auto &binding : bindings) accepted.emplace(binding.consumer, binding.endpoint); // Preacquire bounded physical ports before publishing any accepted state.
	auto acquired_routes = routes;
	_geothermal_heat_consumers = std::move(accepted); _geothermal_heat_routes = std::move(acquired_routes); _geothermal_route_interval = interval;
	_pending_geothermal_heat.reset(); return true;
}

/** @copydoc ComputeGeothermalHeatRouteDigest */
uint64_t ComputeGeothermalHeatRouteDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Explicit physical reference scalar; fixed bytes exclude host padding, callbacks and presentation. */
	const auto field = [&hash](uint64_t value) { for (uint byte = 0; byte < 8; ++byte) { hash ^= value & 255; hash *= 1099511628211ULL; value >>= 8; } };
	field(_geothermal_route_interval); field(_geothermal_heat_consumers.size());
	for (const auto &[consumer, endpoint] : _geothermal_heat_consumers) { field(consumer.base()); field(endpoint.base()); } // Stable retained ports contain no extra energy or stock balance.
	field(_geothermal_heat_routes.size());
	for (const auto &route : _geothermal_heat_routes) {
		field(route.consumer.base()); field(route.producer.base()); field(route.cells.size());
		for (const auto cell : route.cells) field(cell.base()); // Exact original installed IDs cannot be reconstructed from proximity after reload.
	}
	return hash;
}
