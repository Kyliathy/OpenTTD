/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_power.cpp Shared native owner of bounded Heat/Electricity publication and separate deterministic interval reservations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group7
 * @see docs/zz-rhr/upgrades/2026-10/r2mlvs1-g-content-freeze.md utility units and single authority
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group2B shared typed heat accounting
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5 paid electrical link admission and lifecycle
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups4C/5 original orbital binding retirement
 */

#include "stdafx.h"
#include "geothermal_conduit_sprite.h"
#include "utility_power.h"
#include "geothermal_utility.h"
#include "geothermal_heat.h"
#include "geothermal_riser.h"
#include "geothermal_generator.h"
#include "station_func.h"
#include "station_map.h"
#include "station_base.h"
#include "geothermal_electrical.h"
#include "utility_power_lineage.h"
#include "utility_power_transfer.h"
#include "excavation.h"
#include "transport_tube.h"
#include "transport_tube_geothermal.h"
#include "company_base.h"
#include "company_func.h"
#include "openttd.h"
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

/** @param owner Actual company. @param layer Explicit gameplay plane. @param check_actor Require current company authority. @return Existing shared owner/date/profile refusal. */
static UtilityPowerError CheckUtilityPowerOwner(CompanyID owner, MapLayer layer, bool check_actor);
/** @param id Existing shared consumer. @return Its sole mutable owner, without allocating an identity. */
static UtilityPowerConsumer *FindUtilityConsumer(UtilityConsumerID id);
/** @return The current immutable native town authority; no script adapter is inferred. */
static MultiLayerTownPowerAuthority GetNativeUtilityTownAuthority();
/** @param state Sole owner's live state or its unpublished complete candidate. @param lost Original retired edge. Expire only its unused descendants, preserving spent flow and settled lineage. */
static void ExpireUnusedUtilityDescendants(UtilityPowerState &state, UtilityLinkID lost);
/** @param link Original retained physical identity to retire. @param publish_local_revision Publish this cell's topology receipt when requested. Expire only unused descendants, retaining all spent history. */
static void RetireUtilityPowerLink(UtilityPowerLink &link, bool publish_local_revision);

/**
 * Validate an explicit terminal and its real native town before any service request.
 * @param consumer Existing town account, including its reference-only terminal/source binding.
 * @return Whether ordinary station ownership, whole occupied footprint and town authority permit service now.
 */
static bool HasUtilityTownServiceTerminal(const UtilityPowerConsumer &consumer)
{
	const Station *station = Station::GetIfValid(consumer.town_terminal);
	const Town *town = Town::GetIfValid(consumer.town);
	// A deleted, foreign, empty or script-controlled destination cannot ask
	// for power. Existing effect-only legacy accounts never acquire a terminal.
	if (!consumer.active || station == nullptr || town == nullptr || station->owner != consumer.owner ||
		consumer.effect != UtilityConsumerEffect::TownGrowth || town->flags.Test(TownFlag::CustomGrowth) ||
		GetNativeUtilityTownAuthority() != MultiLayerTownPowerAuthority::Native) return false;
	bool facility = false;
	// Check actual occupied rail/truck facility tiles, rather than a movable
	// sign, bounding-box hole or an unrelated platform on another layer.
	for (const auto &area : {station->train_station, station->truck_station}) {
		if (area.tile == INVALID_TILE) continue;
		for (const TileIndex tile : area) {
			if (!IsTileType(tile, TileType::Station) || GetStationIndex(tile) != station->index) continue; // Sparse joined stations authorize only their actual tiles.
			if (GetTileLayer(tile) != MapLayer::Surface || ClosestTownFromTile(tile, _settings_game.economy.dist_local_authority) != town) return false; // Every serving facility must belong to this town's ordinary authority area on Surface.
			facility = true;
		}
	}
	return facility;
}

/** @param consumer Explicit Surface service. @return Whether its retained original native powerhouse and paid path still admit this service, without granting energy. */
static bool HasUtilityTownServiceSupply(const UtilityPowerConsumer &consumer)
{
	if (!HasUtilityTownServiceTerminal(consumer)) return false;
	const auto *source = GetUtilityPowerConsumer(consumer.town_supply);
	// The forwarding reservation names exactly one real conversion output;
	// ordinary rate sources, imports and a retired company cannot imitate it.
	if (source == nullptr || !source->active || source->owner != consumer.owner || source->meter.layer != MapLayer::DeepUnderground ||
		source->meter.domain != UtilityEnergyDomain::Electricity || source->effect != UtilityConsumerEffect::AssemblyWork ||
		source->linked_producers.size() != 1 || source->capacity_j < 135 || consumer.capacity_j < 135) return false;
	const auto *producer = GetUtilityPowerProducer(source->linked_producers.front());
	if (producer == nullptr || !producer->active || producer->kind != UtilityGenerationKind::HeatConversion) return false; // Only an actual native coupled output may supply this geothermal adapter.
	bool machinery = false;
	// Stable native machinery identities prove the selected source/port; a
	// coincident tile or newly rebuilt industry cannot revive an old producer.
	for (const auto &generator : CaptureGeothermalGenerators()) {
		if (generator.electrical_producer != producer->meter.producer_id) continue;
		machinery = IsGeothermalGeneratorConstructionComplete(generator.industry) &&
			GetGeothermalGeneratorPort(generator.industry) == consumer.town_source_port && HasGeothermalGeneratorInstalledPorts(generator.industry);
		break;
	}
	return machinery && FindGeothermalElectricalPath(consumer.town_supply, consumer.meter.consumer_id, consumer.town_source_port, 1).has_value();
}

/** @copydoc BindUtilityTownService */
UtilityPowerError BindUtilityTownService(UtilityConsumerID destination, UtilityConsumerID source, StationID terminal, TileIndex source_port, bool execute)
{
	if (_utility_mutation_committing) return UtilityPowerError::Refused;
	if (_utility_power.operation_sequence >= UINT64_MAX - 1) return UtilityPowerError::Limit;
	auto *consumer = FindUtilityConsumer(destination);
	if (consumer == nullptr) return UtilityPowerError::InvalidReference;
	if (CheckUtilityPowerOwner(consumer->owner, MapLayer::Surface, true) != UtilityPowerError::None) return UtilityPowerError::InvalidOwner;
	if (consumer->town_terminal != StationID::Invalid()) return UtilityPowerError::Refused;
	UtilityPowerConsumer candidate = *consumer; candidate.town_terminal = terminal; candidate.town_supply = source; candidate.town_source_port = source_port;
	if (!HasUtilityTownServiceSupply(candidate)) return UtilityPowerError::InvalidTopology;
	size_t towns = 0;
	// One terminal's bounded explicit distribution list cannot multiply town
	// effects. Existing BindUtilityTownConsumer already enforces one per town.
	for (const auto &existing : _utility_power.consumers) {
		if (existing.town_terminal == terminal) ++towns;
		if (existing.town_supply == source) return UtilityPowerError::InvalidTopology; // A forwarding account is exclusive to one final demand; sharing it would duplicate request IDs in the common pass.
	}
	if (towns >= 16) return UtilityPowerError::Limit;
	if (execute) {
		consumer->town_terminal = terminal; consumer->town_supply = source; consumer->town_source_port = source_port; ++_utility_power.operation_sequence;
	}
	return UtilityPowerError::None;
}

/** @copydoc AppendUtilityTownServiceRequests */
void AppendUtilityTownServiceRequests(std::vector<UtilityPowerRequest> &requests)
{
	const auto effects = GetUtilityConsumerEffects();
	const auto terms = std::ranges::find(effects, UtilityConsumerEffect::TownGrowth, &UtilityConsumerEffectContract::effect);
	if (terms == effects.end()) return;
	const auto demand = IntegrateUtilityGeneration(terms->demand_rate_w, 1, 0);
	if (!demand.has_value()) return;
	std::vector<StationID> removed;
	// An empty/foreign terminal loses its original authorization before later
	// reconstruction or sale can reuse this reference. Temporary GS/source/path
	// interruptions retain their physical binding and simply request nothing.
	for (const auto &consumer : _utility_power.consumers) {
		if (consumer.town_terminal == StationID::Invalid()) continue;
		const auto *terminal = Station::GetIfValid(consumer.town_terminal);
		if (terminal == nullptr || terminal->owner != consumer.owner ||
			(!terminal->facilities.Test(StationFacility::Train) && !terminal->facilities.Test(StationFacility::TruckStop))) removed.push_back(consumer.town_terminal); // Removing all usable facilities ends service even while the native empty station sign awaits deletion.
	}
	for (const auto terminal : removed) RemoveUtilityTownServiceTerminal(terminal); // Expire only bounded explicit nodes, before collecting any demand.
	// Active explicit connections alone participate, in shared ConsumerID
	// order. The relay has zero demand; only original source and final town do.
	for (const auto &consumer : _utility_power.consumers) {
		if (consumer.town_terminal == StationID::Invalid()) continue;
		if (!HasUtilityTownServiceSupply(consumer)) continue;
		requests.push_back({consumer.town_supply, demand->energy_j}); requests.push_back({consumer.meter.consumer_id, demand->energy_j});
	}
}

/** @copydoc RunUtilityTownServices */
void RunUtilityTownServices()
{
	// The common allocation has already reserved original conversion output.
	// Each complete physical export precedes the existing once-only town effect;
	// no intermediate source, quantity account or duplicate generation is made.
	for (size_t index = 0; index < _utility_power.consumers.size(); ++index) {
		const auto &consumer = _utility_power.consumers[index];
		if (consumer.town_terminal == StationID::Invalid()) continue;
		if (!HasUtilityTownServiceSupply(consumer)) continue;
		// Shared atomic commits may replace their transport vectors. Retain
		// only stable references/geometry across that commit, never a pointer.
		const auto destination = consumer.meter.consumer_id, source = consumer.town_supply;
		const auto port = consumer.town_source_port; const auto owner = consumer.owner;
		AutoRestoreBackup actor{_current_company, owner};
		const auto allocation = GetUtilityAllocation(source);
		if (!allocation.has_value() || allocation->granted_energy_j == 0) continue; // No original current supply means no import or phantom growth benefit.
		const auto proposal = PrepareGeothermalPowerExport({{source, GetUtilityPowerInterval()}, destination, port, allocation->granted_energy_j});
		if (proposal.error != UtilityPowerError::None) continue;
		if (ExportGeothermalPower(proposal).error != UtilityPowerError::None) continue;
		ApplyUtilityTownEffect(destination);
	}
}

/** @copydoc RemoveUtilityTownServiceTerminal */
void RemoveUtilityTownServiceTerminal(StationID terminal)
{
	if (_utility_mutation_committing) return;
	// Destruction invalidates references before native StationID reuse. Existing
	// settled joules remain history; any current unconsumed offer expires.
	for (auto &consumer : _utility_power.consumers) {
		if (consumer.town_terminal != terminal) continue;
		const TownID town = consumer.town;
		consumer.town_terminal = StationID::Invalid(); consumer.town_supply = UtilityConsumerID{UINT32_MAX}; consumer.town_source_port = INVALID_TILE;
		SuspendUtilityTownEffect(town); RefreshUtilityTownGrowthRate(town);
	}
}

/** @copydoc HasUtilityTownServices */
bool HasUtilityTownServices()
{
	// A retained physical subscriber still needs the shared interval to expire
	// its last paid effect. Source loss cannot freeze a former bonus forever.
	/** @param consumer Original shared consumer. @return Whether its retained physical subscription needs normal effect expiry despite temporary source loss. */
	const auto subscribed = [](const auto &consumer) { return consumer.town_terminal != StationID::Invalid(); };
	return std::ranges::any_of(_utility_power.consumers, subscribed);
}

/** @param effect Immutable registered consumer response. @return Whether this is a direct-recipe/generator Heat role rather than an electrical work/effect consumer. */
static bool IsUtilityHeatEffect(UtilityConsumerEffect effect)
{
	return effect == UtilityConsumerEffect::HeatDirectRecipe || effect == UtilityConsumerEffect::HeatGenerator;
}

/** @param state Actual/candidate native ledger. @param domain Explicit requested quantity domain. @return Whether this domain's current once-only allocation has closed; unknown domains grant no rights. */
static bool IsUtilityDomainAllocated(const UtilityPowerState &state, UtilityEnergyDomain domain)
{
	return domain == UtilityEnergyDomain::Electricity ? state.allocated : domain == UtilityEnergyDomain::Heat ? state.heat_allocated : false;
}

/** @param domain Already validated electrical or thermal domain. @return The sole live domain allocation flag; generation/grants never share a cross-domain phase. */
static bool &UtilityDomainPhase(UtilityEnergyDomain domain)
{
	assert(domain == UtilityEnergyDomain::Electricity || domain == UtilityEnergyDomain::Heat);
	return domain == UtilityEnergyDomain::Electricity ? _utility_power.allocated : _utility_power.heat_allocated;
}

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

/** @copydoc GetUtilityPowerProducer */
const UtilityPowerProducer *GetUtilityPowerProducer(UtilityProducerID id) { return FindUtilityProducer(id); }

/** @copydoc GetUtilityPowerConsumer */
const UtilityPowerConsumer *GetUtilityPowerConsumer(UtilityConsumerID id) { return FindUtilityConsumer(id); }

/** @copydoc IsUtilityHeatAllocated */
bool IsUtilityHeatAllocated() { return _utility_power.heat_allocated; }
/** @copydoc HasUtilityHeatState */
bool HasUtilityHeatState()
{
	/** @param source Actual retained native producer. @return Whether its immutable meter domain is thermal, including retirement history. */
	const auto heat_source = [](const UtilityPowerProducer &source) { return source.meter.domain == UtilityEnergyDomain::Heat; };
	/** @param consumer Actual retained native consumer. @return Whether its immutable request/grant domain is thermal. */
	const auto heat_consumer = [](const UtilityPowerConsumer &consumer) { return consumer.meter.domain == UtilityEnergyDomain::Heat; };
	return _utility_power.heat_allocated || std::ranges::any_of(_utility_power.producers, heat_source) || std::ranges::any_of(_utility_power.consumers, heat_consumer);
}

/** @copydoc HasUtilityHeatConversionState */
bool HasUtilityHeatConversionState()
{
	/** @param source Original retained producer, including retired history. @return Whether its exact conversion extension must be serialized with the sole utility owner. */
	const auto converted = [](const UtilityPowerProducer &source) { return source.kind == UtilityGenerationKind::HeatConversion; };
	return std::ranges::any_of(_utility_power.producers, converted);
}

/** @copydoc GetUtilityPowerInterval */
uint64_t GetUtilityPowerInterval() { return _utility_power.interval; }
/** @copydoc IsUtilityPowerAllocated */
bool IsUtilityPowerAllocated() { return _utility_power.allocated; }

/** @param id Ledger-issued directed edge identity. @return Its retained owner record, or absence for an unallocated ID. */
static UtilityPowerLink *FindUtilityLink(UtilityLinkID id)
{
	if (id.base() >= _utility_power.links.size()) return nullptr;
	return &_utility_power.links[id.base()];
}

/** @copydoc GetUtilityPowerLink */
const UtilityPowerLink *GetUtilityPowerLink(UtilityLinkID id) { return FindUtilityLink(id); }
/** @copydoc GetUtilityPowerLinks */
std::span<const UtilityPowerLink> GetUtilityPowerLinks() { return _utility_power.links; }
/** @copydoc GetUtilityPowerConsumers */
std::span<const UtilityPowerConsumer> GetUtilityPowerConsumers() { return _utility_power.consumers; }
/** @copydoc GetUtilityLocalLinkRevision */
uint64_t GetUtilityLocalLinkRevision() { return _utility_power.local_link_revision; }
/** @copydoc HasUtilityLocalLinks */
bool HasUtilityLocalLinks() { return _utility_power.local_link_revision != 0; }

/** @copydoc RegisterUtilityLocalLink */
std::optional<UtilityLinkID> RegisterUtilityLocalLink(CompanyID owner, UtilityEnergyDomain domain, TileIndex tile, uint64_t capacity_w, bool execute)
{
	// An installation has physical identity and capacity, never a producer or
	// reservation. Whole benefit commits cannot reenter the shared ID issuer.
	if (_utility_mutation_committing || _utility_power.operation_sequence >= UINT64_MAX - 1 || _utility_power.local_link_revision >= UINT64_MAX - 1) return std::nullopt;
	if (!IsValidTile(tile)) return std::nullopt;
	if (CheckUtilityPowerOwner(owner, GetTileLayer(tile)) != UtilityPowerError::None) return std::nullopt;
	const auto *terms = GetMultiLayerLocalUtilityLinkContract(domain);
	// Unknown domains, missing floor and unbounded capacities cannot acquire
	// a reference that a future path solver might treat as installed supply.
	if (terms == nullptr || capacity_w != terms->capacity_w || !IsMultiLayerLocalUtilityLayer(domain, GetTileLayer(tile))) return std::nullopt;
	if (!CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return std::nullopt;
	if (CheckExcavatedFootprint(std::array{tile}).error != ExcavationError::None) return std::nullopt;
	if (_utility_power.links.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS) return std::nullopt;
	// Each actual tile/domain has one installed cell. Retired rows remain in
	// shared history, and replacement receives a new ID rather than reviving it.
	for (const auto &link : _utility_power.links) {
		if (link.kind == UtilityLinkKind::LocalCell && link.active && link.local_domain == domain && link.source_endpoint == tile) return std::nullopt;
	}
	UtilityPowerLink link; link.id = UtilityLinkID{_utility_power.next_link}; link.kind = UtilityLinkKind::LocalCell;
	link.local_owner = owner; link.local_domain = domain; link.local_capacity_w = capacity_w;
	link.local_layer = GetTileLayer(tile);
	link.source_endpoint = link.destination_endpoint = tile;
	if (execute) {
		_utility_power.links.push_back(link); ++_utility_power.next_link; ++_utility_power.operation_sequence; ++_utility_power.local_link_revision;
		MarkGeothermalConduitArtDirty(tile); // Committed installation repaints only this cell and adjacent paid branches.
	}
	return link.id;
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
	if (link.kind == UtilityLinkKind::ElectricalFeed) {
		// A local inlet moves only original Deep output into an unlinked tube
		// reservation. It has no portal, independent watt budget or re-export role.
		if (source.owner != destination.owner || source.meter.domain != UtilityEnergyDomain::Electricity ||
			destination.meter.domain != UtilityEnergyDomain::Electricity || source.meter.layer != MapLayer::DeepUnderground ||
			destination.meter.layer != MapLayer::DeepUnderground || link.source == link.destination ||
			destination.effect != UtilityConsumerEffect::TubeWork || !destination.linked_producers.empty() ||
			link.generator.base() >= 64000 || link.portal_revision != 0 || link.capacity_j != 0 ||
			link.riser != IndustryID::Invalid() || link.riser_capacity_w != 0 || link.flow_j > MAX_UTILITY_ACCOUNT_ENERGY_J ||
			link.source_endpoint == INVALID_TILE || link.destination_endpoint == INVALID_TILE || link.source_endpoint == link.destination_endpoint ||
			(link.active && (!source.active || !destination.active || source.linked_producers.size() != 1))) return UtilityPowerError::InvalidTopology;
		if (!world_refs) return UtilityPowerError::None;
		const auto *plant = GetGeothermalGeneratorState(link.generator);
		const auto inlet = GetTubeTerminal(link.destination_endpoint, source.owner);
		const auto *terms = plant == nullptr ? nullptr : GetMultiLayerGeothermalGeneratorTerms(plant->model);
		// Re-query original native machinery and the real recipient; coincident
		// coordinates and unfinished work confer no attachment. Temporary cable
		// loss/calendar locks retain history; operation separately rechecks readiness.
		if (plant == nullptr || terms == nullptr || plant->owner != source.owner || !plant->materials_committed ||
			plant->work_ticks != GetMultiLayerGeothermalConstruction(terms->construction)->work_ticks ||
			GetGeothermalGeneratorPort(link.generator) != link.source_endpoint || source.linked_producers.front() != plant->electrical_producer ||
			!inlet.has_value() || inlet->layer != MapLayer::DeepUnderground) return UtilityPowerError::InvalidTopology;
		return UtilityPowerError::None;
	}
	if (link.generator != IndustryID::Invalid()) return UtilityPowerError::InvalidTopology; // Other physical roles cannot conceal a local generator attachment.

	// Only declared electrical roles connect reservations; a local physical
	// cell cannot acquire an output account or implicit portal capability.
	if (link.kind != UtilityLinkKind::ElectricalPortal && link.kind != UtilityLinkKind::ElectricalRiser) return UtilityPowerError::InvalidTopology;
	// A direction connects two distinct same-company layer accounts; local
	// loops and foreign budgets cannot masquerade as a commissioned export.
	if (source.meter.domain != UtilityEnergyDomain::Electricity || destination.meter.domain != UtilityEnergyDomain::Electricity || source.owner != destination.owner || source.meter.layer == destination.meter.layer || link.source == link.destination) return UtilityPowerError::InvalidTopology;
	// Retained route history may name retired consumers; an active route can
	// never make a removed company or a reused pool identity operational again.
	if (link.active && (!source.active || !destination.active)) return UtilityPowerError::InvalidTopology;
	// Invalid endpoints/revision/capacity authorize no physical route even in
	// preview, where loaded map and company pools are deliberately unavailable.
	if (link.source_endpoint == INVALID_TILE || link.destination_endpoint == INVALID_TILE || link.source_endpoint == link.destination_endpoint ||
		link.portal_revision == 0 || link.portal_revision == UINT64_MAX || link.flow_j > MAX_UTILITY_ACCOUNT_ENERGY_J) return UtilityPowerError::InvalidTopology;
	if (link.kind == UtilityLinkKind::ElectricalPortal) {
		// Genuine older numerical portal contracts retain their exact interval
		// cap and never acquire a paid native facility by default.
		if (link.capacity_j == 0 || link.capacity_j > MAX_UTILITY_ACCOUNT_ENERGY_J || link.flow_j > link.capacity_j ||
			link.riser != IndustryID::Invalid() || link.riser_capacity_w != 0) return UtilityPowerError::InvalidTopology;
		if (world_refs && FindGeothermalRiserForPortal(link.source_endpoint) != IndustryID::Invalid()) return UtilityPowerError::InvalidTopology; // Actual native machinery requires its retained paid role; legacy registration/load cannot bypass unfinished work or later machinery loss by discarding that binding.
	} else {
		// Paid machinery stores its immutable physical rate, not a builder's
		// invented interval grant. Routing derives its bounded opportunity later.
		const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser);
		if (link.riser.base() >= 64000 || link.capacity_j != 0 || link.riser_capacity_w != terms->transfer_capacity_w) return UtilityPowerError::InvalidTopology;
		if (world_refs) {
			const auto *riser = GetGeothermalRiserState(link.riser);
			if (riser == nullptr || !IsGeothermalRiserOperational(link.riser, false) || riser->site.owner != source.owner ||
				riser->site.portal != link.source_endpoint || riser->destination != link.destination_endpoint || riser->portal_revision != link.portal_revision) return UtilityPowerError::InvalidTopology; // Original complete machinery, actual stock/floor and its accepted pair must agree; neither pool reuse nor a new coincident housing supplies readiness.
		}
	}
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
		if (existing.active && existing.kind == UtilityLinkKind::ElectricalRiser && existing.source_endpoint == endpoint) return std::nullopt; // The internal older primitive cannot multiply a sender already bound to paid native machinery.
	}
	if (execute) { _utility_power.links.push_back(link); ++_utility_power.next_link; ++_utility_power.operation_sequence; }
	return link.id;
}

/** @param link Original retained physical edge. @param publish_local_revision Whether topology changes need a revision receipt. Retire unused promises without discarding consumed history. */
static void RetireUtilityPowerLink(UtilityPowerLink &link, bool publish_local_revision);

/**
 * Validate a complete original local feed, including a not-yet-published real work consumer.
 * @param generator Original completed dated native plant.
 * @param source_id Existing owned original forwarding reservation.
 * @param destination Actual registered or fully admitted prospective work consumer; no quantity is copied from another account.
 * @param inlet Original actual native receiving facility.
 * @return Complete unpublished existing-format link, or unchanged refusal before either issuer advances.
 */
static std::optional<UtilityPowerLink> PrepareGeothermalLocalFeedRegistration(IndustryID generator, UtilityConsumerID source_id,
	const UtilityPowerConsumer &destination, TileIndex inlet)
{
	const auto *source = FindUtilityConsumer(source_id);
	// Native participants, company/date policy and bounded nonreused identities
	// must all precede even an estimated registration.
	if (_utility_mutation_committing || source == nullptr ||
		CheckUtilityPowerOwner(source->owner, source->meter.layer) != UtilityPowerError::None ||
		_utility_power.links.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS || _utility_power.next_link >= UINT32_MAX - 1 ||
		_utility_power.operation_sequence >= UINT64_MAX - 1) return std::nullopt;
	UtilityPowerLink link;
	link.id = UtilityLinkID{_utility_power.next_link}; link.kind = UtilityLinkKind::ElectricalFeed; link.generator = generator;
	link.source = source_id; link.destination = destination.meter.consumer_id;
	link.source_endpoint = GetGeothermalGeneratorPort(generator); link.destination_endpoint = inlet;
	if (!IsGeothermalGeneratorConstructionComplete(generator) || !HasGeothermalGeneratorInstalledPorts(generator)) return std::nullopt; // New attachment requires current dated native readiness; a retained saved attachment alone confers no operating permission.
	if (CheckUtilityPowerLink(link, *source, destination) != UtilityPowerError::None) return std::nullopt;
	if (!FindGeothermalUtilityPath(source->owner, UtilityEnergyDomain::Electricity, link.source_endpoint, inlet).has_value()) return std::nullopt;
	// One recipient has one explicit physical source identity. A replacement
	// inlet must retire its old attachment before registration can succeed.
	for (const auto &existing : _utility_power.links) {
		if (existing.active && existing.kind == UtilityLinkKind::ElectricalFeed && existing.destination == link.destination) return std::nullopt;
	}
	return link;
}

/** @copydoc RegisterGeothermalLocalFeed */
std::optional<UtilityLinkID> RegisterGeothermalLocalFeed(IndustryID generator, UtilityConsumerID source_id,
	UtilityConsumerID destination_id, TileIndex inlet, bool execute)
{
	const auto *destination = FindUtilityConsumer(destination_id);
	if (destination == nullptr) return std::nullopt;
	const auto link = PrepareGeothermalLocalFeedRegistration(generator, source_id, *destination, inlet);
	if (!link.has_value()) return std::nullopt;
	if (execute) { _utility_power.links.push_back(*link); ++_utility_power.next_link; ++_utility_power.operation_sequence; }
	return link->id;
}

/** @copydoc RegisterGeothermalLocalFeedRecipient */
std::optional<UtilityLocalFeedRecipient> RegisterGeothermalLocalFeedRecipient(IndustryID generator, UtilityConsumerID supply_id,
	TileIndex inlet, uint64_t capacity_j, bool execute)
{
	const auto *supply = FindUtilityConsumer(supply_id);
	if (supply == nullptr || capacity_j == 0) return std::nullopt; // A missing original root or a zero-work placeholder cannot acquire recipient/feed identities.
	const auto consumer = RegisterUtilityPowerConsumer(supply->owner, MapLayer::DeepUnderground, UtilityConsumerEffect::TubeWork,
		capacity_j, {}, false);
	if (!consumer.has_value()) return std::nullopt;
	// This is the actual proposed destination, not a temporary registered dummy
	// or a copy of the quantity owner. Shared consumer admission has already
	// checked the complete role, request bound, actor and next original identity.
	UtilityPowerConsumer destination;
	destination.owner = supply->owner; destination.effect = UtilityConsumerEffect::TubeWork; destination.capacity_j = capacity_j;
	destination.meter.consumer_id = *consumer; destination.meter.layer = MapLayer::DeepUnderground;
	destination.meter.domain = UtilityEnergyDomain::Electricity;
	const auto link = PrepareGeothermalLocalFeedRegistration(generator, supply_id, destination, inlet);
	if (!link.has_value()) return std::nullopt;
	if (execute) {
		// Acquire both fallible storage expansions before publishing either ID.
		// Subsequent record moves and scalar publication have no callbacks or
		// refusable step, so a failed inlet never strands a work consumer.
		_utility_power.consumers.reserve(_utility_power.consumers.size() + 1);
		_utility_power.links.reserve(_utility_power.links.size() + 1);
		_utility_power.consumers.push_back(std::move(destination)); _utility_power.links.push_back(*link);
		++_utility_power.next_consumer; ++_utility_power.next_link; ++_utility_power.operation_sequence;
	}
	return UtilityLocalFeedRecipient{*consumer, link->id};
}

/** @copydoc InvalidateUtilityLocalFeedInlets */
void InvalidateUtilityLocalFeedInlets(TileIndex tile)
{
	if (_utility_mutation_committing) return;
	bool changed = false;
	// Actual transport notifications resolve only bounded retained inlets. A
	// removed recipient loses its original attachment before native pool reuse.
	for (auto &link : _utility_power.links) {
		if (!link.active || link.kind != UtilityLinkKind::ElectricalFeed || link.destination_endpoint != tile) continue;
		if (GetTubeTerminal(tile, _utility_power.consumers[link.destination.base()].owner).has_value()) continue;
		RetireUtilityPowerLink(link, true); changed = true;
	}
	if (changed && _utility_power.operation_sequence < UINT64_MAX - 1) ++_utility_power.operation_sequence; // Lifecycle loss expires unused rights even at the terminal sequence boundary, without renewing retry identity.
}

/** @copydoc RegisterGeothermalRiserLink */
std::optional<UtilityLinkID> RegisterGeothermalRiserLink(IndustryID industry, UtilityConsumerID source_id, UtilityConsumerID destination_id, bool execute)
{
	if (_utility_mutation_committing) return std::nullopt;
	const auto *riser = GetGeothermalRiserState(industry);
	const auto *source = FindUtilityConsumer(source_id), *destination = FindUtilityConsumer(destination_id);
	// Actual paid machinery and both registered reservations precede every
	// identity/sequence change. A source grant or portal alone supplies no riser.
	if (riser == nullptr || source == nullptr || destination == nullptr || !IsGeothermalRiserOperational(industry) ||
		CheckUtilityPowerOwner(source->owner, source->meter.layer) != UtilityPowerError::None ||
		CheckUtilityPowerOwner(destination->owner, destination->meter.layer) != UtilityPowerError::None) return std::nullopt;
	if (_utility_power.links.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS || _utility_power.next_link >= UINT32_MAX - 1 || _utility_power.operation_sequence >= UINT64_MAX - 1) return std::nullopt; // Bounded nonreused identity and retry sequence must fit before publication.
	UtilityPowerLink link;
	link.id = UtilityLinkID{_utility_power.next_link}; link.kind = UtilityLinkKind::ElectricalRiser;
	link.source = source_id; link.destination = destination_id; link.source_endpoint = riser->site.portal; link.destination_endpoint = riser->destination;
	link.portal_revision = riser->portal_revision; link.riser = industry;
	link.riser_capacity_w = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->transfer_capacity_w;
	if (CheckUtilityPowerLink(link, *source, *destination) != UtilityPowerError::None) return std::nullopt;
	// Different numerical consumer pairs cannot multiply one paid sender's
	// physical rate; any existing portal contract here must first be retired.
	for (const auto &existing : _utility_power.links) {
		if (!existing.active || existing.kind == UtilityLinkKind::LocalCell) continue;
		if (existing.source_endpoint == link.source_endpoint || (existing.source == source_id && existing.destination == destination_id)) return std::nullopt;
	}
	if (execute) { _utility_power.links.push_back(link); ++_utility_power.next_link; ++_utility_power.operation_sequence; }
	return link.id;
}

/** Original owned service accounts and ordered retained edges; this read-only view owns no supply or recoverable credit. */
struct RetainedGeothermalTownRoute {
	std::array<UtilityConsumerID, 3> accounts{UtilityConsumerID{UINT32_MAX}, UtilityConsumerID{UINT32_MAX}, UtilityConsumerID{UINT32_MAX}}; ///< Original Deep, optional Underground and Surface identities.
	std::array<UtilityLinkID, 2> links{UtilityLinkID{UINT32_MAX}, UtilityLinkID{UINT32_MAX}}; ///< Original directed edges, including retired history after physical loss.
	size_t legs = 0; ///< One direct or two ascending native paid transitions.
};

/**
 * Resolve only the existing town's exclusive original native route.
 * @param town Actual retained town account, including after terminal references were cleared.
 * @return Original owned accounts/edges, or refusal for effect-only, ambiguous, branched or foreign graphs.
 */
static std::optional<RetainedGeothermalTownRoute> FindRetainedGeothermalTownRoute(const UtilityPowerConsumer &town)
{
	RetainedGeothermalTownRoute route;
	std::array<UtilityConsumerID, 3> reverse{town.meter.consumer_id, UtilityConsumerID{UINT32_MAX}, UtilityConsumerID{UINT32_MAX}};
	std::array<UtilityLinkID, 2> edges{UtilityLinkID{UINT32_MAX}, UtilityLinkID{UINT32_MAX}};
	// Walk at most the two actual ascending planes backwards. Prefer the sole
	// active attachment; after physical loss, the latest retained attachment
	// identifies history only and can never restore its expired rights.
	for (size_t depth = 0; depth < 2; ++depth) {
		const UtilityPowerLink *selected = nullptr;
		for (const auto &link : _utility_power.links) {
			if (link.kind != UtilityLinkKind::ElectricalRiser || link.destination != reverse[depth]) continue; // Other roles and destinations do not belong to this town's paid route.
			if (selected != nullptr && selected->active && link.active) return std::nullopt; // Two live parents are not the exclusive service this command is authorized to replace.
			if (selected == nullptr || link.active || !selected->active) selected = &link; // Stable issued order chooses the last historical edge only when no live edge exists.
		}
		if (selected == nullptr) return std::nullopt;
		const auto *source = FindUtilityConsumer(selected->source);
		// Retired company accounts and unrelated consumer roles cannot regain
		// authority through a surviving numeric link or native town association.
		if (source == nullptr || !source->active || source->owner != town.owner || source->effect != UtilityConsumerEffect::AssemblyWork ||
			source->meter.domain != UtilityEnergyDomain::Electricity) return std::nullopt;
		reverse[depth + 1] = source->meter.consumer_id; edges[depth] = selected->id;
		if (source->meter.layer == MapLayer::DeepUnderground) { route.legs = depth + 1; break; }
		if (source->meter.layer != MapLayer::Underground) return std::nullopt;
	}
	if (route.legs == 0) return std::nullopt;
	// Reconstruct physical order without creating a second graph. Every old
	// nonterminal node must remain exclusive to these exact original edges.
	for (size_t node = 0; node <= route.legs; ++node) route.accounts[node] = reverse[route.legs - node];
	for (size_t leg = 0; leg < route.legs; ++leg) route.links[leg] = edges[route.legs - leg - 1];
	if (town.town_supply.base() != UINT32_MAX && town.town_supply != route.accounts[0]) return std::nullopt; // A surviving binding must agree with its actual original route, rather than seizing another assembly reservation.
	for (const auto &link : _utility_power.links) {
		if (!link.active || link.kind == UtilityLinkKind::LocalCell) continue; // Installed floor cells own no service account and are never retired by this command.
		const auto nodes = std::span(route.accounts).first(route.legs);
		const bool touches = std::ranges::find(nodes, link.source) != nodes.end() || std::ranges::find(nodes, link.destination) != nodes.end(); // Either end can expose an unrelated live branch sharing this old reservation.
		if (touches && std::ranges::find(route.links, link.id) == route.links.end()) return std::nullopt; // Replacing this service cannot revoke another live destination's route.
	}
	for (const auto &consumer : _utility_power.consumers) {
		if (consumer.meter.consumer_id != town.meter.consumer_id && consumer.town_supply == route.accounts[0]) return std::nullopt; // One forwarding account cannot be appropriated from another retained town.
	}
	const auto *source = FindUtilityConsumer(route.accounts[0]);
	if (source->linked_producers.size() > 1) return std::nullopt;
	if (!source->linked_producers.empty()) {
		const auto *producer = FindUtilityProducer(source->linked_producers.front());
		if (producer == nullptr || producer->kind != UtilityGenerationKind::HeatConversion || producer->owner != town.owner) return std::nullopt; // Only an actual original native converter establishes recoverable service ancestry.
	}
	return route;
}

/**
 * Replace references/unused rights inside the original owner's unpublished candidate.
 * @param request Explicit current paid machinery and recipient selection.
 * @param old Exclusive original account/edge identities, already resolved without mutation.
 * @param generator Actual selected complete native generator and original coupled output.
 * @param port Actual selected generator's installed electrical boundary.
 * @param demand_j Canonical whole town opportunity; never caller-provided energy.
 * @param risers Actual selected current paid directions, already physically preflighted.
 * @param execute False validates/predicts without publishing any settlement or identity.
 * @return Complete atomic recovery or unchanged refusal, retaining the original town and all settled history.
 */
static GeothermalTownServiceConnection RecoverGeothermalTownService(const GeothermalTownServiceRequest &request,
	const RetainedGeothermalTownRoute &old, const GeothermalGeneratorState &generator, TileIndex port, uint64_t demand_j,
	const std::array<const GeothermalRiserState *, 2> &risers, bool execute)
{
	GeothermalTownServiceConnection result;
	// Retained service roles keep their immutable capacities. Recovery may not
	// turn an undersized generic assembly account into implicit town supply.
	for (size_t node = 0; node <= old.legs; ++node) {
		if (_utility_power.consumers[old.accounts[node].base()].capacity_j < demand_j) return result;
	}
	UtilityPowerState candidate = _utility_power;
	const auto &original_source = candidate.consumers[old.accounts[0].base()];
	const bool replacement = original_source.linked_producers != std::vector{generator.electrical_producer};
	auto accounts = old.accounts;
	// A replacement converter gets a fresh forwarding identity. Its retired
	// predecessor retains all original root/descendant history and can never
	// spend an old grant against this newly selected physical output.
	if (replacement) {
		result.error = UtilityPowerError::Limit;
		if (candidate.consumers.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS || candidate.next_consumer >= UINT32_MAX - 1) return result;
		UtilityPowerConsumer source;
		source.owner = _current_company; source.effect = UtilityConsumerEffect::AssemblyWork; source.capacity_j = demand_j;
		source.meter.consumer_id = UtilityConsumerID{candidate.next_consumer++}; source.meter.layer = MapLayer::DeepUnderground;
		source.meter.domain = UtilityEnergyDomain::Electricity;
		source.linked_producers = {generator.electrical_producer}; accounts[0] = source.meter.consumer_id;
		candidate.consumers.push_back(std::move(source));
	}
	auto &town = candidate.consumers[old.accounts[old.legs].base()];
	bool changed = replacement || town.town_terminal != request.terminal || town.town_source_port != port || town.town_supply != accounts[0]; // Explicit recipient/source changes are one requested repair; an identical click must not release or renew any current right.
	town.town_terminal = request.terminal; town.town_source_port = port; town.town_supply = accounts[0];
	result.error = UtilityPowerError::InvalidReference;
	if (!HasUtilityTownServiceTerminal(town)) return result;
	// Keep an unchanged paid edge and its spent interval flow. Only a changed
	// original source, machinery or commissioning stamp requires a fresh edge;
	// its historical predecessor remains retained with its actual spent flow.
	for (size_t leg = 0; leg < old.legs; ++leg) {
		const auto *riser = risers[leg]; const auto &previous = candidate.links[old.links[leg].base()];
		const bool reusable = previous.active && previous.source == accounts[leg] && previous.destination == accounts[leg + 1] &&
			previous.riser == riser->industry && previous.portal_revision == riser->portal_revision &&
			previous.source_endpoint == riser->site.portal && previous.destination_endpoint == riser->destination; // Reuse requires the exact same paid physical owner, direction and account endpoints; coincident rebuilt geometry cannot revive an old edge.
		if (reusable) { result.links[leg] = previous.id; continue; }
		result.error = UtilityPowerError::Limit;
		if (candidate.links.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS || candidate.next_link >= UINT32_MAX - 1) return result;
		ExpireUnusedUtilityDescendants(candidate, previous.id); candidate.links[old.links[leg].base()].active = false;
		UtilityPowerLink link;
		link.id = UtilityLinkID{candidate.next_link++}; link.kind = UtilityLinkKind::ElectricalRiser;
		link.source = accounts[leg]; link.destination = accounts[leg + 1];
		link.source_endpoint = riser->site.portal; link.destination_endpoint = riser->destination;
		link.portal_revision = riser->portal_revision; link.riser = riser->industry;
		link.riser_capacity_w = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->transfer_capacity_w;
		result.error = CheckUtilityPowerLink(link, candidate.consumers[link.source.base()], candidate.consumers[link.destination.base()]);
		if (result.error != UtilityPowerError::None) return result;
		result.links[leg] = link.id; candidate.links.push_back(std::move(link)); changed = true;
	}
	result.error = UtilityPowerError::Refused;
	if (!changed) return result;
	// Settle only the old exclusive service's still-unused reservations. A
	// posted town effect, consumed electricity, transferred lineage and every
	// physical link's already-spent flow remain unchanged until normal expiry.
	for (size_t node = 0; node <= old.legs; ++node) {
		auto &consumer = candidate.consumers[old.accounts[node].base()];
		for (auto &share : consumer.shares) {
			const auto unused = FreeUtilityShare(share); share.released_j += unused;
			candidate.producers[share.producer.base()].reserved_j -= unused;
		}
	}
	if (replacement) {
		auto &retired = candidate.consumers[old.accounts[0].base()]; retired.linked_producers.clear(); retired.active = false;
	}
	++candidate.operation_sequence;
	result.error = ValidateUtilityPowerState(candidate, GetCurrentMultiLayerProfileSelection());
	if (result.error != UtilityPowerError::None) return result;
	result.source = accounts[0]; result.destination = accounts[old.legs];
	if (old.legs == 2) result.relay = accounts[1];
	if (execute) _utility_power = std::move(candidate);
	return result;
}

/** @copydoc ConnectGeothermalTownService */
GeothermalTownServiceConnection ConnectGeothermalTownService(const GeothermalTownServiceRequest &request, bool execute)
{
	GeothermalTownServiceConnection result;
	const auto &profile = GetCurrentMultiLayerProfileSelection();
	// A player connection needs the actual native gameplay policy. Editor or
	// legacy metadata cannot create a fresh physical supply authorization.
	if (_game_mode != GameMode::Normal || profile.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile.legacy) {
		result.error = UtilityPowerError::UnsupportedProfile; return result;
	}
	result.error = CheckUtilityPowerOwner(_current_company, MapLayer::DeepUnderground);
	if (result.error != UtilityPowerError::None) return result;
	result.error = UtilityPowerError::InvalidReference;
	const auto *generator = GetGeothermalGeneratorState(request.generator);
	// Only this company's completed real machinery and installed original port
	// can choose its coupled output; caller-supplied rate sources are excluded.
	if (generator == nullptr || generator->owner != _current_company || !IsGeothermalGeneratorConstructionComplete(request.generator) ||
		!HasGeothermalGeneratorInstalledPorts(request.generator)) return result;
	const auto *producer = FindUtilityProducer(generator->electrical_producer);
	// The registered converter must remain this exact machine's typed pair,
	// rather than a retired, foreign or unrelated source with a similar rate.
	if (producer == nullptr || !producer->active || producer->owner != _current_company ||
		producer->kind != UtilityGenerationKind::HeatConversion || producer->conversion_input != generator->heat_consumer) return result;
	const auto port = GetGeothermalGeneratorPort(request.generator);
	const size_t legs = request.second_riser == IndustryID::Invalid() ? 1 : 2;
	const std::array<const GeothermalRiserState *, 2> risers{
		GetGeothermalRiserState(request.first_riser), GetGeothermalRiserState(request.second_riser)};
	const auto effects = GetUtilityConsumerEffects();
	const auto effect = std::ranges::find(effects, UtilityConsumerEffect::TownGrowth, &UtilityConsumerEffectContract::effect);
	if (effect == effects.end()) return result;
	const auto demand = IntegrateUtilityGeneration(effect->demand_rate_w, 1, 0);
	if (!demand.has_value()) return result;
	std::optional<RetainedGeothermalTownRoute> retained;
	// Existing native service recovery preserves the unique owned town
	// account. Effect-only/foreign/ambiguous associations confer no repair
	// authority and cannot be silently replaced by a second town account.
	for (const auto &existing : _utility_power.consumers) {
		if (existing.town != request.town) continue;
		if (retained.has_value() || !existing.active || existing.owner != _current_company || existing.effect != UtilityConsumerEffect::TownGrowth ||
			existing.meter.layer != MapLayer::Surface || existing.meter.domain != UtilityEnergyDomain::Electricity) return result;
		retained = FindRetainedGeothermalTownRoute(existing);
		if (!retained.has_value() || retained->legs != legs) return result; // This bounded repair preserves the original one/two-plane service topology; changing its plane count requires a separate explicit route operation.
	}
	result.error = UtilityPowerError::InvalidTopology;
	TileIndex inlet = port;
	// Validate the explicit one/two paid directions and every actual intervening
	// cable before issuing any account. The next inlet is the original arriving
	// housing, so equal XY coordinates never substitute for an Underground path.
	for (size_t leg = 0; leg < legs; ++leg) {
		const auto *riser = risers[leg];
		const auto source_layer = leg == 0 ? MapLayer::DeepUnderground : MapLayer::Underground;
		const auto destination_layer = leg + 1 == legs ? MapLayer::Surface : MapLayer::Underground;
		// Every selected original must be paid, complete, owned and connected in
		// the requested ascending direction; a reused native ID grants nothing.
		if (riser == nullptr || riser->site.owner != _current_company || !IsGeothermalRiserOperational(riser->industry) ||
			GetTileLayer(riser->site.portal) != source_layer || GetTileLayer(riser->destination) != destination_layer) return result;
		const auto path = FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Electricity, inlet, riser->site.port);
		if (!path.has_value()) return result;
		const auto capacity = IntegrateUtilityGeneration(std::min<uint64_t>(path->capacity_w,
			GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->transfer_capacity_w), 1, 0);
		// A complete town request needs a real interval opportunity along every
		// leg; a zero or undersized cable cannot become implicit supply.
		if (!capacity.has_value() || capacity->energy_j < demand->energy_j) return result;
		inlet = riser->destination;
		// Preserve the sole physical sender's rate and unrelated participants.
		// Only this town's exact retained edge may be superseded atomically;
		// another customer cannot seize an occupied physical direction.
		for (const auto &existing : _utility_power.links) {
			if (!existing.active || existing.kind == UtilityLinkKind::LocalCell) continue; // Installed cells are separate infrastructure, not another portal direction.
			if (retained.has_value() && std::ranges::find(retained->links, existing.id) != retained->links.end()) continue; // This original service's two retained directions are the only authorized replacements.
			if (existing.source_endpoint == riser->site.portal) return result;
		}
	}
	if (retained.has_value()) return RecoverGeothermalTownService(request, *retained, *generator, port, demand->energy_j, risers, execute);
	const size_t nodes = legs + 1;
	result.error = UtilityPowerError::Limit;
	// Whole retained storage/cursor bounds precede candidate allocation. A
	// failure on the last leg must not leave earlier consumers or identity gaps.
	if (_utility_power.consumers.size() + nodes > MAX_UTILITY_INTERVAL_ACCOUNTS ||
		_utility_power.links.size() + legs > MAX_UTILITY_INTERVAL_ACCOUNTS ||
		_utility_power.next_consumer > UINT32_MAX - nodes - 1 || _utility_power.next_link > UINT32_MAX - legs - 1) return result;
	UtilityPowerState candidate = _utility_power;
	std::array<UtilityPowerConsumer, 3> accounts;
	// These are prospective records inside the sole ledger's transaction, not
	// a GUI registry: explicit Deep forwarding, optional zero-demand UG relay,
	// then one real Surface town. No record contains generation or a grant.
	for (size_t node = 0; node < nodes; ++node) {
		auto &account = accounts[node]; account.owner = _current_company; account.capacity_j = demand->energy_j;
		account.effect = node + 1 == nodes ? UtilityConsumerEffect::TownGrowth : UtilityConsumerEffect::AssemblyWork;
		account.meter.consumer_id = UtilityConsumerID{candidate.next_consumer++}; account.meter.domain = UtilityEnergyDomain::Electricity;
		account.meter.layer = node == 0 ? MapLayer::DeepUnderground : node + 1 == nodes ? MapLayer::Surface : MapLayer::Underground;
	}
	accounts.front().linked_producers.push_back(generator->electrical_producer);
	auto &town = accounts[nodes - 1]; town.town = request.town; town.town_terminal = request.terminal;
	town.town_supply = accounts.front().meter.consumer_id; town.town_source_port = port;
	result.error = UtilityPowerError::InvalidReference;
	if (!HasUtilityTownServiceTerminal(town)) return result;
	std::array<UtilityLinkID, 2> links{UtilityLinkID{UINT32_MAX}, UtilityLinkID{UINT32_MAX}};
	// Reuse the canonical paid-link validator with the complete prospective
	// accounts and actual native machinery. Only after all legs succeed can the
	// common owner publish their dense identities together.
	for (size_t leg = 0; leg < legs; ++leg) {
		const auto *riser = risers[leg]; UtilityPowerLink link;
		link.id = UtilityLinkID{candidate.next_link++}; link.kind = UtilityLinkKind::ElectricalRiser;
		link.source = accounts[leg].meter.consumer_id; link.destination = accounts[leg + 1].meter.consumer_id;
		link.source_endpoint = riser->site.portal; link.destination_endpoint = riser->destination;
		link.portal_revision = riser->portal_revision; link.riser = riser->industry;
		link.riser_capacity_w = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->transfer_capacity_w;
		result.error = CheckUtilityPowerLink(link, accounts[leg], accounts[leg + 1]);
		if (result.error != UtilityPowerError::None) return result;
		links[leg] = link.id; candidate.links.push_back(std::move(link));
	}
	for (size_t node = 0; node < nodes; ++node) candidate.consumers.push_back(accounts[node]); // Preserve every prospective account's fixed role and issued order within this one publication.
	++candidate.operation_sequence;
	result.error = ValidateUtilityPowerState(candidate, profile);
	if (result.error != UtilityPowerError::None) return result;
	result.source = accounts.front().meter.consumer_id;
	if (legs == 2) result.relay = accounts[1].meter.consumer_id;
	result.destination = town.meter.consumer_id; result.links = links;
	if (execute) _utility_power = std::move(candidate);
	return result;
}

/** @param route Exact accepted original physical source path. @return Whether this native owner expired a positive unused current slice; original consumed quantities and source identity remain intact. */
static bool ExpireUnusedUtilityHeatRoute(const GeothermalHeatRoute &route)
{
	auto *consumer = FindUtilityConsumer(route.consumer);
	// Physical references may retain historical paths after expiry. They can
	// release only this same native thermal consumer's current source slice.
	if (consumer == nullptr || consumer->meter.domain != UtilityEnergyDomain::Heat || consumer->meter.accounting_interval != _utility_power.interval) return false;
	const auto share = std::ranges::find(consumer->shares, route.producer, &UtilityPowerShare::producer);
	if (share == consumer->shares.end()) return false;
	const uint64_t unused = FreeUtilityShare(*share);
	if (unused == 0) return false;
	share->released_j += unused; FindUtilityProducer(share->producer)->reserved_j -= unused;
	return true;
}

/** @copydoc RevalidateUtilityHeatRoutes */
void RevalidateUtilityHeatRoutes()
{
	assert(!_utility_mutation_committing);
	if (GetGeothermalHeatRouteInterval() != _utility_power.interval) return;
	bool changed = false;
	// A genuine older393 file could retain unused rights after losing a cell.
	// Resolve accepted original references once during actual load; normalize
	// only that lost quantity through its native owner, without rebuilding or
	// granting a replacement path. Current healthy files remain byte-exact.
	for (const auto &route : GetGeothermalHeatRoutes()) {
		for (const auto id : route.cells) {
			const auto *cell = FindUtilityLink(id);
			if (cell != nullptr && cell->active) continue; // Retained original active cells preserve the exact allocated route; a fresh replacement ID cannot revive a retired node.
			changed |= ExpireUnusedUtilityHeatRoute(route); break;
		}
	}
	if (changed && _utility_power.operation_sequence < UINT64_MAX - 1) ++_utility_power.operation_sequence; // Nonrefusible load/lifecycle expiry stays monotonic without overflowing a terminal sequence.
}

/**
 * Revoke unused descendant rights at every depth of one original lost electrical reference.
 * @param state Sole live owner or unpublished complete recovery candidate.
 * @param lost Original installed cell or paid/genuine older edge, never a replacement ID.
 */
static void ExpireUnusedUtilityDescendants(UtilityPowerState &state, UtilityLinkID lost)
{
	assert(state.next_share <= MAX_UTILITY_POWER_SHARES);
	std::array<UtilityPowerShare *, MAX_UTILITY_POWER_SHARES> nodes{};
	std::array<bool, MAX_UTILITY_POWER_SHARES> affected{};
	// Retained IDs provide a dense topological order independent of each
	// consumer's canonical source/path sorting. Fixed bounded storage keeps
	// actual nonrefusible company/housing cleanup free of heap allocation.
	for (auto &consumer : state.consumers) {
		for (auto &share : consumer.shares) {
			assert(share.id.base() < state.next_share);
			nodes[share.id.base()] = &share;
		}
	}
	// Earlier-parent issuance propagates loss once through every branch. A
	// whole-path arrival names the lost reference directly; staged arrivals
	// inherit it from their exact parent even when their final edge survives.
	for (uint32_t id = 0; id < state.next_share; ++id) {
		assert(nodes[id] != nullptr);
		auto &share = *nodes[id];
		const bool inherited = share.parent.base() != UINT32_MAX && affected[share.parent.base()]; // Only a retained earlier parent can pass on original physical loss.
		affected[id] = inherited || std::ranges::find(share.segment, lost) != share.segment.end(); // Either original segment membership or ancestry revokes this descendant's remaining rights.
		if (!affected[id]) continue;
		const auto unused = FreeUtilityShare(share);
		share.released_j += unused; state.producers[share.producer.base()].reserved_j -= unused;
		// Released/consumed/transferred histories and spent physical allowance
		// remain exact. No donor credit or repair can renew these original rights.
	}
}

/**
 * Retire original physical identity and expire its still-unused complete descendant rights.
 * @param link Actual retained route owner, already admitted for retirement.
 * @param publish_local_revision False lets a whole-company operation publish one topology receipt for its complete batch.
 */
static void RetireUtilityPowerLink(UtilityPowerLink &link, bool publish_local_revision = true)
{
	ExpireUnusedUtilityDescendants(_utility_power, link.id);
	if (link.kind == UtilityLinkKind::LocalCell) {
		// The physical family supplies original issued cell references only.
		// This native owner expires each affected slice's free quantity once;
		// consumed history and every unaffected source/consumer stay intact.
		// Borrowed routes avoid allocation during nonrefusible company cleanup.
		if (link.local_domain == UtilityEnergyDomain::Heat && GetGeothermalHeatRouteInterval() == _utility_power.interval) {
			for (const auto &route : GetGeothermalHeatRoutes()) {
				if (std::ranges::find(route.cells, link.id) == route.cells.end()) continue;
				ExpireUnusedUtilityHeatRoute(route);
			}
		}
		link.active = false;
		InvalidateGeothermalUtilityPathCache();
		MarkGeothermalConduitArtDirty(link.source_endpoint); // Physical retirement caps surviving neighbours without changing settled energy.
		if (publish_local_revision && _utility_power.local_link_revision < UINT64_MAX - 1) ++_utility_power.local_link_revision; // Independent cell retirement publishes its own receipt; a whole-company batch publishes once after all cells change.
		return; // Physical loss revokes only unused rights and derived connectivity; repairing a fresh cell cannot revive an original grant.
	}
	link.active = false;
}

/**
 * Retire local physical attachments before their original output loses eligibility.
 * @param output Exact original producer being removed, including a native conversion source.
 * Spent lineage survives; source removal cannot leave an active inlet naming an empty forwarding reservation.
 */
static void RetireUtilityLocalFeeds(UtilityProducerID output)
{
	// Inspect only retained local attachments. Multiple recipients may share
	// this original output, but no unrelated source or physical inlet is retired.
	for (auto &link : _utility_power.links) {
		if (!link.active || link.kind != UtilityLinkKind::ElectricalFeed) continue;
		const auto &sources = _utility_power.consumers[link.source.base()].linked_producers;
		if (std::ranges::find(sources, output) != sources.end()) RetireUtilityPowerLink(link);
	}
}

/** @param supply Existing forwarding owner. @return Whether retiring it affects only original local-feed promises, with no foreign physical role or imported root. */
static UtilityPowerError CheckUtilityLocalFeedSupply(const UtilityPowerConsumer &supply)
{
	// A tube source owns an original Deep electrical forwarding reservation,
	// never an arbitrary consumer, imported relay or another facility's output.
	if (supply.effect != UtilityConsumerEffect::TubeWork || supply.meter.domain != UtilityEnergyDomain::Electricity ||
		supply.meter.layer != MapLayer::DeepUnderground || supply.linked_producers.size() > 1) return UtilityPowerError::InvalidTopology;
	for (const auto id : supply.linked_producers) {
		const auto *producer = FindUtilityProducer(id);
		if (producer == nullptr || producer->kind != UtilityGenerationKind::HeatConversion || producer->owner != supply.owner) return UtilityPowerError::InvalidReference; // Only the original native conversion output may fund this source's root promise.
	}
	// Dedicated source reservations cannot retire another feature's incoming
	// crossing or outgoing paid riser while closing their own local recipients.
	for (const auto &link : _utility_power.links) {
		if (!link.active) continue;
		if (link.destination == supply.meter.consumer_id ||
			(link.source == supply.meter.consumer_id && link.kind != UtilityLinkKind::ElectricalFeed)) return UtilityPowerError::Refused;
	}
	// Forwarding history remains rooted in original publication. An imported
	// account cannot be disguised as a disposable source to release foreign rights.
	for (const auto &share : supply.shares) {
		if (share.parent.base() != UINT32_MAX) return UtilityPowerError::InvalidTopology;
	}
	return UtilityPowerError::None;
}

/** @copydoc InvalidateUtilityLocalFeedRecipient */
void InvalidateUtilityLocalFeedRecipient(UtilityConsumerID consumer_id, UtilityLinkID feed_id)
{
	if (_utility_mutation_committing) return;
	auto *consumer = FindUtilityConsumer(consumer_id); auto *feed = FindUtilityLink(feed_id);
	// Native capsule loss can retire only its original terminal request/feed.
	// A mistyped root, physical cell or foreign role cannot seize a whole supply.
	if (consumer == nullptr || feed == nullptr || feed->kind != UtilityLinkKind::ElectricalFeed || feed->destination != consumer_id ||
		consumer->effect != UtilityConsumerEffect::TubeWork || consumer->meter.domain != UtilityEnergyDomain::Electricity ||
		consumer->meter.layer != MapLayer::DeepUnderground || !consumer->linked_producers.empty()) return;
	if (!consumer->active && !feed->active) return; // Repeated native teardown cannot repeat the original retirement receipt.
	// An actual work recipient has no forwarding role. Refuse a different
	// account's active outgoing route rather than retiring another user's rights.
	for (const auto &link : _utility_power.links) {
		if (link.active && link.kind != UtilityLinkKind::LocalCell && link.source == consumer_id) return;
	}
	if (feed->active) RetireUtilityPowerLink(*feed);
	// Feed retirement expires original imported children. Release any remaining
	// unused original shares once; consumed history and sibling roots stay intact.
	for (auto &share : consumer->shares) {
		const auto unused = FreeUtilityShare(share); share.released_j += unused;
		FindUtilityProducer(share.producer)->reserved_j -= unused;
	}
	consumer->active = false;
	if (_utility_power.operation_sequence < UINT64_MAX - 1) ++_utility_power.operation_sequence;
}

/** @param supply Admitted actual forwarding reservation. Close only unused original rights and publish one bounded shared lifecycle receipt. */
static void RetireUtilityLocalFeedSupplyImpl(UtilityPowerConsumer &supply)
{
	if (!supply.active) return;
	// Each original outgoing feed expires its unused children once. Spent flow
	// and transferred parent debits survive, and no sibling source is touched.
	for (auto &link : _utility_power.links) {
		if (link.active && link.kind == UtilityLinkKind::ElectricalFeed && link.source == supply.meter.consumer_id) RetireUtilityPowerLink(link);
	}
	// Return only this forwarding root's remaining promises to their original
	// producer; native generator history/service and other users are unchanged.
	for (auto &share : supply.shares) {
		const uint64_t unused = FreeUtilityShare(share); share.released_j += unused;
		FindUtilityProducer(share.producer)->reserved_j -= unused;
	}
	supply.linked_producers.clear(); supply.active = false;
	if (_utility_power.operation_sequence < UINT64_MAX - 1) ++_utility_power.operation_sequence; // Terminal sequences already refuse new work, but actual physical loss must still expire unused rights without overflow.
}

/** @copydoc RetireUtilityLocalFeedSupply */
UtilityPowerError RetireUtilityLocalFeedSupply(UtilityConsumerID id, bool execute)
{
	if (_utility_mutation_committing) return UtilityPowerError::Refused;
	auto *supply = FindUtilityConsumer(id); if (supply == nullptr) return UtilityPowerError::InvalidReference;
	if (_current_company != OWNER_DEITY && _current_company != supply->owner) return UtilityPowerError::InvalidOwner; // Only the original controller or native system cleanup may close this source's forwarding rights.
	const auto admission = CheckUtilityLocalFeedSupply(*supply); if (admission != UtilityPowerError::None) return admission;
	if (supply->active && _utility_power.operation_sequence >= UINT64_MAX - 1) return UtilityPowerError::Limit; // An ordinary estimated/explicit closure must fit its single shared receipt before changing any participant.
	if (execute) RetireUtilityLocalFeedSupplyImpl(*supply);
	return UtilityPowerError::None;
}

/** @copydoc InvalidateUtilityLocalFeedSupply */
void InvalidateUtilityLocalFeedSupply(UtilityConsumerID id)
{
	if (_utility_mutation_committing) return;
	auto *supply = FindUtilityConsumer(id);
	if (supply == nullptr || CheckUtilityLocalFeedSupply(*supply) != UtilityPowerError::None) return; // Lifecycle cleanup cannot appropriate an absent or foreign physical reservation.
	RetireUtilityLocalFeedSupplyImpl(*supply);
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
	if (source == nullptr && link->kind != UtilityLinkKind::LocalCell) return UtilityPowerError::InvalidReference; // Local cells own no source-side energy reservation.
	// System cleanup can retire lost owners; ordinary companies can alter
	// only their own explicit route, without touching another imported budget.
	const auto owner = link->kind == UtilityLinkKind::LocalCell ? link->local_owner : source->owner;
	if (_current_company != OWNER_DEITY && _current_company != owner) return UtilityPowerError::InvalidOwner;
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
		if (link.kind == UtilityLinkKind::LocalCell) continue; // Local Deep cells cannot inherit a portal's commissioning or loss policy; both electrical portal roles retain it.
		if (link.source_endpoint != endpoint && link.destination_endpoint != endpoint) continue;
		RetireUtilityPowerLink(link); changed = true;
	}
	// Terminal sequences already refuse new work; lifecycle loss still
	// expires unused credit without overflowing its saved retry identity.
	if (changed && _utility_power.operation_sequence < UINT64_MAX - 1) ++_utility_power.operation_sequence;
}

/** @copydoc InvalidateUtilityPowerLinksForRiser */
void InvalidateUtilityPowerLinksForRiser(IndustryID industry)
{
	if (_utility_mutation_committing) return;
	bool changed = false;
	// Retire only actual paid attachments before native pool reuse. Generic
	// legacy portal edges and unrelated machinery keep their original owners.
	for (auto &link : _utility_power.links) {
		if (!link.active || link.kind != UtilityLinkKind::ElectricalRiser || link.riser != industry) continue;
		RetireUtilityPowerLink(link); changed = true;
	}
	if (changed && _utility_power.operation_sequence < UINT64_MAX - 1) ++_utility_power.operation_sequence;
}

/** @copydoc RegisterUtilityPowerProducer
 * @param domain Explicit immutable source quantity domain; Heat is Deep geothermal only. */
static std::optional<UtilityProducerID> RegisterUtilityDomainProducer(CompanyID owner, MapLayer layer, UtilityGenerationKind kind, bool execute, UtilityEnergyDomain domain)
{
	// Source acquisition cannot reenter a whole benefit commit or exhaust its
	// retry sequence before publication. Refusal must leave the issuer unchanged.
	if (_utility_mutation_committing || _utility_power.operation_sequence >= UINT64_MAX - 1) return std::nullopt;
	// Domain and source family are fixed at registration; a Surface solar or
	// external electrical source cannot later masquerade as Deep vent heat.
	if (domain != UtilityEnergyDomain::Electricity && domain != UtilityEnergyDomain::Heat) return std::nullopt;
	if (domain == UtilityEnergyDomain::Heat && (layer != MapLayer::DeepUnderground || kind != UtilityGenerationKind::Geothermal)) return std::nullopt;
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
		producer.meter.domain = domain;
		producer.meter.producer_id = id;
		producer.meter.layer = layer;
		_utility_power.producers.push_back(std::move(producer));
		++_utility_power.next_producer;
		++_utility_power.operation_sequence;
	}
	return id;
}

/** @copydoc RegisterUtilityPowerProducer */
std::optional<UtilityProducerID> RegisterUtilityPowerProducer(CompanyID owner, MapLayer layer, UtilityGenerationKind kind, bool execute)
{
	return RegisterUtilityDomainProducer(owner, layer, kind, execute, UtilityEnergyDomain::Electricity);
}
/** @copydoc RegisterUtilityHeatProducer */
std::optional<UtilityProducerID> RegisterUtilityHeatProducer(CompanyID owner, bool execute)
{
	return RegisterUtilityDomainProducer(owner, MapLayer::DeepUnderground, UtilityGenerationKind::Geothermal, execute, UtilityEnergyDomain::Heat);
}

/** @copydoc RegisterUtilityPowerConsumer
 * @param domain Immutable native request/grant domain; roles and eligible sources must match it. */
static std::optional<UtilityConsumerID> RegisterUtilityDomainConsumer(CompanyID owner, MapLayer layer, UtilityConsumerEffect effect,
	uint64_t capacity_j, std::span<const UtilityProducerID> sources, bool execute, UtilityEnergyDomain domain)
{
	if (CheckUtilityPowerOwner(owner, layer) != UtilityPowerError::None) return std::nullopt;
	// Registration binds role and energy together. A thermal generator/direct
	// recipe never becomes an electrical bonus/tube/town consumer by relabelling.
	if (domain != UtilityEnergyDomain::Electricity && domain != UtilityEnergyDomain::Heat) return std::nullopt;
	if (to_underlying(effect) > to_underlying(UtilityConsumerEffect::HeatGenerator) || IsUtilityHeatEffect(effect) != (domain == UtilityEnergyDomain::Heat)) return std::nullopt;
	if (domain == UtilityEnergyDomain::Heat && layer != MapLayer::DeepUnderground) return std::nullopt;
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
		if (source->owner != owner || source->meter.layer != layer || source->meter.domain != domain || !unique.insert(id).second) return std::nullopt;
	}
	const UtilityConsumerID id{_utility_power.next_consumer};
	if (execute) {
		UtilityPowerConsumer consumer;
		consumer.owner = owner;
		consumer.effect = effect;
		consumer.capacity_j = capacity_j;
		consumer.meter.domain = domain;
		consumer.meter.consumer_id = id;
		consumer.meter.layer = layer;
		consumer.linked_producers.assign(unique.begin(), unique.end());
		_utility_power.consumers.push_back(std::move(consumer));
		++_utility_power.next_consumer;
		++_utility_power.operation_sequence;
	}
	return id;
}

/** @copydoc RegisterUtilityPowerConsumer */
std::optional<UtilityConsumerID> RegisterUtilityPowerConsumer(CompanyID owner, MapLayer layer, UtilityConsumerEffect effect,
	uint64_t capacity_j, std::span<const UtilityProducerID> sources, bool execute)
{
	return RegisterUtilityDomainConsumer(owner, layer, effect, capacity_j, sources, execute, UtilityEnergyDomain::Electricity);
}
/** @copydoc RegisterUtilityHeatConsumer */
std::optional<UtilityConsumerID> RegisterUtilityHeatConsumer(CompanyID owner, UtilityHeatUse use, uint64_t capacity_j,
	std::span<const UtilityProducerID> sources, bool execute)
{
	if (to_underlying(use) > to_underlying(UtilityHeatUse::ElectricalGenerator)) return std::nullopt;
	const auto effect = use == UtilityHeatUse::ElectricalGenerator ? UtilityConsumerEffect::HeatGenerator : UtilityConsumerEffect::HeatDirectRecipe;
	return RegisterUtilityDomainConsumer(owner, MapLayer::DeepUnderground, effect, capacity_j, sources, execute, UtilityEnergyDomain::Heat);
}

/** @copydoc PublishUtilityGeneration
 * @param domain Typed actual publication; each native source and its allocation phase must match it. */
static UtilityPowerError PublishUtilityDomainGeneration(UtilityProducerID id, uint64_t interval, uint64_t rate_w, uint32_t economy_ticks, bool execute, UtilityEnergyDomain domain)
{
	auto *source = FindUtilityProducer(id);
	if (source == nullptr) return UtilityPowerError::InvalidReference;
	if (source->kind == UtilityGenerationKind::HeatConversion) return UtilityPowerError::Refused; // Only the complete Heat-debit transaction may publish these exact electrical joules.
	if (source->meter.domain != domain) return UtilityPowerError::InvalidReference; // Typed publication cannot generate another domain under an existing source identity.
	if (!source->active) return UtilityPowerError::InvalidReference;
	const auto permission = CheckUtilityPowerOwner(source->owner, source->meter.layer);
	if (permission != UtilityPowerError::None) return permission;
	// Only the owner's live nonterminal epoch accepts publication; old receipts
	// and guessed future intervals cannot generate another copy of the energy.
	if (interval != _utility_power.interval || interval == UINT64_MAX) return UtilityPowerError::InvalidInterval;
	if (IsUtilityDomainAllocated(_utility_power, domain)) return UtilityPowerError::AlreadyAllocated;
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

/** @copydoc PublishUtilityGeneration */
UtilityPowerError PublishUtilityGeneration(UtilityProducerID id, uint64_t interval, uint64_t rate_w, uint32_t economy_ticks, bool execute)
{
	return PublishUtilityDomainGeneration(id, interval, rate_w, economy_ticks, execute, UtilityEnergyDomain::Electricity);
}
/** @copydoc PublishUtilityHeatGeneration */
UtilityPowerError PublishUtilityHeatGeneration(UtilityProducerID id, uint64_t interval, uint64_t rate_w, uint32_t economy_ticks, bool execute)
{
	return PublishUtilityDomainGeneration(id, interval, rate_w, economy_ticks, execute, UtilityEnergyDomain::Heat);
}

/** @copydoc AllocateUtilityPower
 * @param domain Explicit native quantity domain; source/request/slice eligibility and phase remain separate.
 * @param routed Optional physical Heat source proposal; absence retains the original shared allocation policy. */
static UtilityPowerError AllocateUtilityDomain(uint64_t interval, std::span<const UtilityPowerRequest> requests, bool execute, UtilityEnergyDomain domain,
	std::optional<std::span<const UtilityHeatRoutingShare>> routed = std::nullopt)
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
	if (IsUtilityDomainAllocated(_utility_power, domain)) return UtilityPowerError::AlreadyAllocated;
	if (requests.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return UtilityPowerError::Limit;
	std::map<UtilityConsumerID, std::vector<UtilityProducerID>> eligibility;
	if (routed.has_value()) {
		if (domain != UtilityEnergyDomain::Heat) return UtilityPowerError::InvalidTopology;
		if (routed->size() > MAX_UTILITY_POWER_CONNECTIONS) return UtilityPowerError::Limit;
		std::set<std::pair<UtilityConsumerID, UtilityProducerID>> pairs;
		// A physical proposal may change eligible sources, but cannot introduce
		// another company's quantity, duplicate pair or nonthermal identity.
		for (const auto &route : *routed) {
			const auto *consumer = FindUtilityConsumer(route.consumer); const auto *source = FindUtilityProducer(route.producer);
			if (consumer == nullptr || source == nullptr || !consumer->active || !source->active || consumer->meter.domain != domain || source->meter.domain != domain ||
				consumer->owner != source->owner || consumer->meter.layer != source->meter.layer || route.quantity_j > MAX_UTILITY_ACCOUNT_ENERGY_J ||
				!pairs.emplace(route.consumer, route.producer).second) return UtilityPowerError::InvalidTopology;
			eligibility[route.consumer].push_back(route.producer);
		}
	}
	std::set<UtilityConsumerID> unique;
	std::vector<UtilityConsumerDemand> demands;
	// Validate every request before reserving anything. Foreign, duplicate or
	// over-capacity demand must leave the whole interval's supply unallocated.
	for (const auto &request : requests) {
		const auto *consumer = FindUtilityConsumer(request.consumer);
		if (consumer == nullptr) return UtilityPowerError::InvalidReference;
		if (consumer->meter.domain != domain) return UtilityPowerError::InvalidReference; // An electrical request cannot reserve heat, or vice versa, even with the same native owner/layer.
		if (!consumer->active) return UtilityPowerError::InvalidReference;
		const auto permission = CheckUtilityPowerOwner(consumer->owner, consumer->meter.layer, false);
		if (permission != UtilityPowerError::None) return permission;
		if (!unique.insert(request.consumer).second) return UtilityPowerError::InvalidReference;
		if (request.demand_j > consumer->capacity_j) return UtilityPowerError::InvalidQuantity;
		const std::span<const UtilityProducerID> sources = routed.has_value() ? std::span<const UtilityProducerID>{eligibility[request.consumer]} : std::span<const UtilityProducerID>{consumer->linked_producers};
		demands.push_back({request.consumer, consumer->meter.layer, sources, request.demand_j});
	}
	std::vector<UtilityProducerSnapshot> supply;
	// A source which did not publish has genuinely zero supply. All companies
	// share this system phase, but each consumer's saved links remain owned/local.
	for (const auto &source : _utility_power.producers) {
		if (!source.active || source.meter.domain != domain) continue; // Only active same-domain sources participate; every natural thermal budget remains distinct from electrical generation.
		const auto permission = CheckUtilityPowerOwner(source.owner, source.meter.layer, false);
		if (permission != UtilityPowerError::None) return permission;
		supply.push_back({source.meter.producer_id, source.meter.layer,
			source.meter.accounting_interval == interval ? source.meter.generation_energy_j : 0});
	}
	auto plan = PlanUtilityAllocation(interval, supply, demands, {});
	if (!plan.valid) return UtilityPowerError::InvalidTopology;
	if (routed.has_value()) {
		// The feature proposes physical shares; only this owner publishes them.
		// Reset the transient numerical default, then revalidate complete source
		// and requested totals before any live eligibility/reservation changes.
		for (auto &source : plan.producers) { source.consumed_j = 0; source.unused_j = source.generated_j; }
		for (auto &grant : plan.consumers) { grant.granted_j = 0; grant.sources.clear(); }
		std::vector<UtilityHeatRoutingShare> ordered(routed->begin(), routed->end());
		/** @param left First physical share. @param right Second share. @return Frozen consumer/source priority, independent of container order. */
		std::ranges::sort(ordered, [](const auto &left, const auto &right) { return std::tie(left.consumer, left.producer) < std::tie(right.consumer, right.producer); });
		for (const auto &route : ordered) {
			const auto source = std::ranges::find(plan.producers, route.producer, &UtilityProducerBalance::id);
			const auto consumer = std::ranges::find(plan.consumers, route.consumer, &UtilityConsumerGrant::id);
			if (source == plan.producers.end() || consumer == plan.consumers.end()) return UtilityPowerError::InvalidReference; // Omitted or inactive participants cannot receive a hidden route reservation.
			if (route.quantity_j > source->unused_j || route.quantity_j > consumer->demand_j - consumer->granted_j) return UtilityPowerError::InvalidQuantity; // Complete native balances bound every physical share before any account changes.
			source->unused_j -= route.quantity_j; source->consumed_j += route.quantity_j; consumer->granted_j += route.quantity_j;
			if (route.quantity_j != 0) consumer->sources.push_back({route.producer, route.quantity_j});
		}
	}
	std::vector<std::pair<size_t, UtilityPowerConsumer>> replacements;
	size_t connections = 0, shares = 0;
	uint32_t next_share = _utility_power.next_share;
	// Acquire complete bounded consumer storage before changing any source.
	// Only a transient transaction candidate is copied; the native owner alone
	// publishes authoritative quantities, fractions and source lineage.
	for (size_t index = 0; index < _utility_power.consumers.size(); ++index) {
		const auto &original = _utility_power.consumers[index];
		if (original.meter.domain != domain) {
			connections += original.linked_producers.size(); shares += original.shares.size(); continue;
		}
		auto consumer = original;
		if (routed.has_value()) consumer.linked_producers = eligibility[consumer.meter.consumer_id];
		consumer.meter.accounting_interval = interval; consumer.meter.demand_energy_j = 0;
		consumer.meter.granted_energy_j = 0; consumer.meter.unmet_energy_j = 0; consumer.shares.clear();
		consumer.town_bonus_percent = 0; consumer.town_effect_applied = false; consumer.town_effect_suspended = false;
		const auto grant = std::ranges::find(plan.consumers, consumer.meter.consumer_id, &UtilityConsumerGrant::id);
		if (grant != plan.consumers.end()) {
			consumer.meter.demand_energy_j = grant->demand_j; consumer.meter.granted_energy_j = grant->granted_j;
			consumer.meter.unmet_energy_j = grant->demand_j - grant->granted_j;
			// Issue every positive root in the shared owner before publication.
			// The other typed domain retains its existing IDs in this same epoch.
			for (const auto &share : grant->sources) {
				if (next_share >= MAX_UTILITY_POWER_SHARES) return UtilityPowerError::Limit;
				UtilityPowerShare root{share.producer, share.energy_j, 0, 0}; root.id = UtilityShareID{next_share++};
				consumer.shares.push_back(std::move(root));
			}
		}
		connections += consumer.linked_producers.size(); shares += consumer.shares.size();
		replacements.emplace_back(index, std::move(consumer));
	}
	// The complete retained cross-domain graph and current slice family must
	// fit their save/runtime bounds, even after source loss retains old lineage.
	if (connections > MAX_UTILITY_POWER_CONNECTIONS || shares > MAX_UTILITY_POWER_SHARES) return UtilityPowerError::Limit;
	if (execute) {
		for (const auto &source : plan.producers) FindUtilityProducer(source.id)->reserved_j = source.consumed_j; // Promised quantities remain reservations until whole actual use settles them.
		for (auto &[index, consumer] : replacements) _utility_power.consumers[index] = std::move(consumer); // All fallible storage was acquired; other-domain accounts remain untouched.
		_utility_power.next_share = next_share;
		UtilityDomainPhase(domain) = true; ++_utility_power.operation_sequence;
	}
	return UtilityPowerError::None;
}

/** @copydoc AllocateUtilityPower */
UtilityPowerError AllocateUtilityPower(uint64_t interval, std::span<const UtilityPowerRequest> requests, bool execute)
{
	return AllocateUtilityDomain(interval, requests, execute, UtilityEnergyDomain::Electricity);
}
/** @copydoc AllocateUtilityHeat */
UtilityPowerError AllocateUtilityHeat(uint64_t interval, std::span<const UtilityPowerRequest> requests, bool execute)
{
	return AllocateUtilityDomain(interval, requests, execute, UtilityEnergyDomain::Heat);
}

/** @copydoc AllocateUtilityHeatRoutes */
UtilityPowerError AllocateUtilityHeatRoutes(uint64_t interval, std::span<const UtilityPowerRequest> requests,
	std::span<const UtilityHeatRoutingShare> routes, bool execute)
{
	return AllocateUtilityDomain(interval, requests, execute, UtilityEnergyDomain::Heat, routes);
}

/** @copydoc GetUtilityAllocation
 * @param domain Exact requested balance domain; read-only views never allocate or convert quantities. */
static std::optional<UtilityPowerAccount> GetUtilityDomainAllocation(UtilityConsumerID id, UtilityEnergyDomain domain)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return std::nullopt;
	const auto *consumer = FindUtilityConsumer(id);
	// A missing, foreign or not-yet-allocated consumer has no current grant.
	// Reading a window cannot carry a previous epoch forward or renew a lease.
	if (consumer == nullptr || consumer->meter.domain != domain || !consumer->active || consumer->owner != _current_company || !IsUtilityDomainAllocated(_utility_power, domain) || consumer->meter.accounting_interval != _utility_power.interval) return std::nullopt;
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

/** @copydoc GetUtilityAllocation */
std::optional<UtilityPowerAccount> GetUtilityAllocation(UtilityConsumerID id) { return GetUtilityDomainAllocation(id, UtilityEnergyDomain::Electricity); }
/** @copydoc GetUtilityHeatAllocation */
std::optional<UtilityPowerAccount> GetUtilityHeatAllocation(UtilityConsumerID id) { return GetUtilityDomainAllocation(id, UtilityEnergyDomain::Heat); }

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
		_utility_power.next_share = 0; // Both domains' slices have cleared; only this whole-epoch transition may reuse a slice number.
		++_utility_power.interval;
		_utility_power.allocated = false; _utility_power.heat_allocated = false;
		ClearGeothermalHeatRoutes(); // Native expiry owns all quantities; only now can their prior physical path references expire.
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
		InvalidateTubeGeothermalSourcesForProducer(id); // Close exact original inlet roots before erasing output eligibility.
		RetireUtilityLocalFeeds(id); // Notify only this source's original physical attachments before linked eligibility is cleared.
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
		if (_utility_power.operation_sequence < UINT64_MAX - 1) ++_utility_power.operation_sequence; // Forwarding cleanup may already publish the last valid receipt; native source loss cannot overflow it.
	}
	return UtilityPowerError::None;
}

/** @copydoc RetireOrbitalUtilityAccounts */
UtilityPowerError RetireOrbitalUtilityAccounts(std::span<const UtilityProducerID> producer_ids, std::span<const UtilityConsumerID> consumer_ids, bool execute)
{
	if (_utility_mutation_committing) return UtilityPowerError::Refused;
	// Current native modules own at most one panel and separate service/construction consumers; bound the whole lifecycle before resolving references.
	if (producer_ids.size() > 1 || consumer_ids.size() > 2) return UtilityPowerError::Limit;
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return UtilityPowerError::UnsupportedProfile;
	std::array<UtilityPowerProducer *, 1> producers{};
	std::array<UtilityPowerConsumer *, 2> consumers{};
	bool changed = false;
	// Retain pointers only after full typed-owner admission; no caller span can be invalidated when eligibility vectors are later cleared.
	for (size_t i = 0; i < producer_ids.size(); ++i) {
		auto *p = FindUtilityProducer(producer_ids[i]);
		if (p == nullptr || p->kind != UtilityGenerationKind::OrbitalPanel || p->meter.layer != MapLayer::Space || p->meter.domain != UtilityEnergyDomain::Electricity) return UtilityPowerError::InvalidReference;
		if (_current_company != OWNER_DEITY && _current_company != p->owner) return UtilityPowerError::InvalidOwner;
		producers[i] = p; changed |= p->active;
	}
	// Each original consumer is retired once, preserving its settled shares even if a previous company lifecycle already made it inactive.
	for (size_t i = 0; i < consumer_ids.size(); ++i) {
		auto *c = FindUtilityConsumer(consumer_ids[i]);
		if (c == nullptr || c->effect != UtilityConsumerEffect::AssemblyWork || c->meter.layer != MapLayer::Space || c->meter.domain != UtilityEnergyDomain::Electricity ||
			(i != 0 && consumer_ids[i] == consumer_ids[0])) return UtilityPowerError::InvalidReference;
		if (_current_company != OWNER_DEITY && _current_company != c->owner) return UtilityPowerError::InvalidOwner;
		consumers[i] = c; changed |= c->active;
	}
	// A physical transfer route has another owner; module retirement cannot silently decommission that external benefit or rewrite its lineage.
	for (const auto &link : _utility_power.links) {
		if (!link.active) continue;
		if (std::ranges::find(consumer_ids, link.source) != consumer_ids.end() || std::ranges::find(consumer_ids, link.destination) != consumer_ids.end()) return UtilityPowerError::Refused;
	}
	// One whole operation advances the shared sequence once, so an overflow cannot leave the first of several bindings partially retired.
	if (changed && _utility_power.operation_sequence >= UINT64_MAX - 1) return UtilityPowerError::Limit;
	if (!execute || !changed) return UtilityPowerError::None;
	// Expire each retiring consumer's unspent allocation once. Original consumed/transferred lineage and other consumers remain untouched.
	for (auto *c : consumers) {
		if (c == nullptr || !c->active) continue;
		for (auto &share : c->shares) {
			const auto unused = FreeUtilityShare(share);
			share.released_j += unused;
			auto *source = FindUtilityProducer(share.producer); assert(source != nullptr);
			source->reserved_j -= unused;
		}
		c->linked_producers.clear(); c->active = false;
	}
	// Source retirement applies the existing exact unused-share rule across its whole eligible network, retaining all paid history and original IDs.
	for (auto *p : producers) {
		if (p == nullptr || !p->active) continue;
		for (auto &c : _utility_power.consumers) {
			std::erase(c.linked_producers, p->meter.producer_id);
			for (auto &share : c.shares) {
				if (share.producer != p->meter.producer_id) continue;
				const auto unused = FreeUtilityShare(share);
				share.released_j += unused; p->reserved_j -= unused;
			}
		}
		p->active = false;
	}
	++_utility_power.operation_sequence;
	return UtilityPowerError::None;
}

/**
 * Select this company incarnation's complete retained resource-reference components.
 * @param owner Actual company whose live rights seed the ownership event.
 * @return Producer/consumer membership, including connected retired history but excluding unrelated old CompanyID tombstones.
 */
static std::pair<std::vector<bool>, std::vector<bool>> CollectUtilityCompanyLineage(Owner owner)
{
	const size_t producers = _utility_power.producers.size(), consumers = _utility_power.consumers.size();
	std::vector<size_t> parents(producers + consumers);
	std::vector<uint8_t> ranks(parents.size());
	for (size_t index = 0; index < parents.size(); ++index) parents[index] = index;
	/** @param node Original source or reservation reference. @return Its bounded ownership component, with path compression and no resource mutation. */
	const auto root = [&parents](size_t node) {
		// Shared history can contain long parent/source chains. Compress only
		// transient references so lifecycle work remains bounded by actual records.
		while (parents[node] != node) { parents[node] = parents[parents[node]]; node = parents[node]; }
		return node;
	};
	/** @param first First original resource reference. @param second Coupled original resource reference. Join only transient ownership components, never quantities or identities. */
	const auto join = [&parents, &ranks, &root](size_t first, size_t second) {
		first = root(first); second = root(second); if (first == second) return;
		if (ranks[first] < ranks[second]) std::swap(first, second);
		parents[second] = first; if (ranks[first] == ranks[second]) ++ranks[first];
	};
	// Current and retired reservations retain original source shares. Their
	// owners must move together when any live right still anchors that history.
	for (size_t index = 0; index < consumers; ++index) {
		const auto &consumer = _utility_power.consumers[index]; if (consumer.owner != owner) continue;
		for (const auto &share : consumer.shares) {
			if (share.producer.base() < producers && _utility_power.producers[share.producer.base()].owner == owner) join(producers + index, share.producer.base()); // Foreign or absent references cannot attach another company's records to this ownership event.
		}
	}
	// A surviving external capsule retains its original source reference even
	// after closure clears eligibility and later intervals expire all shares.
	// That live physical association couples billing history to its recipient
	// during takeover; it grants no electricity and revives no source permission.
	for (const auto &binding : CaptureTubePowerVehicles()) {
		// Native feeds already retain their endpoint lineage below. Inactive
		// associations cannot let a reused company/vehicle ID adopt old history.
		if (!binding.active) continue; // The exact External source-kind check below excludes geothermal bindings without borrowing a provider discriminator.
		const auto producer = binding.source.base(), consumer = binding.consumer.base();
		// Missing account references cannot extend the transient ownership graph
		// beyond the sole ledger or attach an unrelated historical incarnation.
		if (producer >= producers || consumer >= consumers) continue;
		const auto &source = _utility_power.producers[producer];
		const auto &recipient = _utility_power.consumers[consumer];
		// Only the same company's live typed tube request and its exact external
		// source can couple history. Foreign, inactive or differently typed
		// accounts cannot gain ownership merely through a numeric reference.
		if (!recipient.active || recipient.owner != owner || recipient.effect != UtilityConsumerEffect::TubeWork ||
				source.owner != owner || source.kind != UtilityGenerationKind::External) continue;
		join(producers + consumer, producer);
	}
	// An original physical route also couples its retained endpoint owners,
	// even before any energy exists. A live recipient must bring its closed
	// source history with it; otherwise an unused feed becomes an invalid
	// cross-company route at takeover. Local cells have no account endpoints.
	for (const auto &link : _utility_power.links) {
		if (link.kind == UtilityLinkKind::LocalCell) continue;
		if (link.source.base() >= consumers || link.destination.base() >= consumers) continue; // Absent endpoint references cannot seed or appropriate another ownership component.
		if (_utility_power.consumers[link.source.base()].owner == owner && _utility_power.consumers[link.destination.base()].owner == owner) {
			join(producers + link.source.base(), producers + link.destination.base()); // Only original same-company endpoints join; the existing live-anchor rule still excludes disconnected old CompanyID tombstones.
		}
	}
	// Exact conversion also retains its original input and lifetime thermal
	// origins. Ownership cannot split this coupled history across companies.
	for (size_t index = 0; index < producers; ++index) {
		const auto &producer = _utility_power.producers[index];
		if (producer.owner != owner || producer.kind != UtilityGenerationKind::HeatConversion) continue;
		if (producer.conversion_input.base() < consumers && _utility_power.consumers[producer.conversion_input.base()].owner == owner) join(index, producers + producer.conversion_input.base()); // Only this company's actual retained conversion input joins the original output.
		for (const auto &origin : producer.conversion_origins) {
			if (origin.producer.base() < producers && _utility_power.producers[origin.producer.base()].owner == owner) join(index, origin.producer.base()); // Retained original thermal attribution is a reference, never permission to seize a foreign source.
		}
	}
	std::vector<bool> live(parents.size());
	// Only actual live company records seed a component. A later company
	// reusing this numeric ID cannot acquire disconnected older tombstones.
	for (size_t index = 0; index < producers; ++index) {
		const auto &producer = _utility_power.producers[index];
		if (producer.owner == owner && producer.active) live[root(index)] = true;
	}
	for (size_t index = 0; index < consumers; ++index) {
		const auto &consumer = _utility_power.consumers[index];
		if (consumer.owner == owner && consumer.active) live[root(producers + index)] = true;
	}
	std::pair<std::vector<bool>, std::vector<bool>> selected{std::vector<bool>(producers), std::vector<bool>(consumers)};
	for (size_t index = 0; index < producers; ++index) selected.first[index] = live[root(index)];
	for (size_t index = 0; index < consumers; ++index) selected.second[index] = live[root(producers + index)];
	return selected;
}

/** @copydoc ChangeUtilityPowerCompanyOwnership */
void ChangeUtilityPowerCompanyOwnership(Owner old_owner, Owner new_owner)
{
	assert(!_utility_mutation_committing);
	assert(old_owner != new_owner);
	bool changed = false, local_changed = false;
	const auto lineage = CollectUtilityCompanyLineage(old_owner);
	// Physical eligibility is invalidated before tiles/company identities
	// change. Takeover must commission a new route rather than inherit stale
	// portal revisions; settled imported use is never refunded.
	for (auto &link : _utility_power.links) {
		if (link.kind == UtilityLinkKind::LocalCell) {
			// Actual installed infrastructure follows takeover; removal retires
			// only current active cells. Historical CompanyID reuse owns no pipe.
			if (!link.active || link.local_owner != old_owner) continue;
			if (new_owner == INVALID_OWNER) RetireUtilityPowerLink(link, false);
			else {
				link.local_owner = CompanyID{new_owner};
				InvalidateGeothermalUtilityPathCache();
				MarkGeothermalConduitArtDirty(link.source_endpoint); // Takeover changes visible connectivity only after canonical ownership changes.
			}
			changed = local_changed = true; continue;
		}
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
		// Connected retired history follows current live rights. Disconnected
		// earlier CompanyID incarnations have no live anchor and remain untouched.
		if (!lineage.first[source.meter.producer_id.base()]) continue;
		changed = true;
		if (new_owner != INVALID_OWNER) {
			source.owner = CompanyID{new_owner};
			continue;
		}
		source.active = false;
	}
	// Connected retired consumers follow their live resource history without
	// regaining town eligibility, physical permission or an unused energy offer.
	// Disconnected older company incarnations keep their original tombstones.
	for (auto &consumer : _utility_power.consumers) {
		if (consumer.owner != old_owner) continue;
		if (!lineage.second[consumer.meter.consumer_id.base()]) continue;
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
		consumer.town = TownID::Invalid(); consumer.town_terminal = StationID::Invalid(); consumer.town_supply = UtilityConsumerID{UINT32_MAX}; consumer.town_source_port = INVALID_TILE;
		if (consumer.effect == UtilityConsumerEffect::TownGrowth) consumer.town_effect_suspended = true;
		if (town != TownID::Invalid()) RefreshUtilityTownGrowthRate(town);
	}
	// All installed cells move/retire in one actual company operation. One
	// topology receipt invalidates derived routes without outrunning the
	// single saved operation receipt merely because the company owns many cells.
	if (local_changed && _utility_power.local_link_revision < UINT64_MAX - 1) ++_utility_power.local_link_revision;
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
	field(_utility_power.heat_allocated); // The independent thermal allocation phase is authoritative, not reconstructed from a graph or animation.
	field(_utility_power.operation_sequence);
	field(_utility_power.next_link);
	field(_utility_power.next_share); // Exact per-epoch lineage identity survives sorting, retry and native reload across both domains.
	field(_utility_power.producers.size());
	// Retired sources remain in identity order; their fractions and settled
	// lineage are still authoritative even when they have no usable output.
	for (const auto &source : _utility_power.producers) {
		account(source.meter); field(source.owner.base()); field(to_underlying(source.kind)); field(source.reserved_j); field(source.active);
		field(source.generated_lifetime_j); field(source.consumed_lifetime_j);
		// Earlier source kinds retain their exact digest. Conversion-only
		// identity adds every saved ratio and original thermal contribution.
		if (source.kind == UtilityGenerationKind::HeatConversion) {
			field(source.conversion_input.base()); field(source.conversion_efficiency); field(source.conversion_capacity_w);
			field(source.conversion_remainder); field(source.conversion_origins.size());
			for (const auto &origin : source.conversion_origins) { field(origin.producer.base()); field(origin.heat_j); }
		}
	}
	field(_utility_power.consumers.size());
	// Exact collection lengths and source IDs distinguish missing links/slices
	// from unchanged totals, without deriving a replacement electrical credit.
	for (const auto &consumer : _utility_power.consumers) {
		account(consumer.meter); field(consumer.owner.base()); field(consumer.active); field(to_underlying(consumer.effect)); field(consumer.capacity_j);
		field(consumer.town.base()); field(consumer.town_bonus_percent); field(consumer.town_effect_applied); field(consumer.town_effect_suspended);
		// Only a physical service adds terminal/root/port identity; absent fields
		// cannot alter older native consumer digests or imply a subscription.
		if (consumer.town_terminal != StationID::Invalid() || consumer.town_supply.base() != UINT32_MAX || consumer.town_source_port != INVALID_TILE) {
			field(consumer.town_terminal.base()); field(consumer.town_supply.base()); field(consumer.town_source_port.base());
		}
		field(consumer.linked_producers.size());
		for (const auto source : consumer.linked_producers) field(source.base());
		field(consumer.shares.size());
		for (const auto &share : consumer.shares) {
			field(share.producer.base()); field(share.allocated_j); field(share.consumed_j); field(share.released_j); field(share.transferred_j); field(share.imported_link.base());
			field(share.id.base()); field(share.parent.base()); field(share.segment.size());
			for (const auto link : share.segment) field(link.base());
		}
	}
	field(_utility_power.links.size());
	// Exact directed endpoints/revision, capacity, flow and retirement remain
	// saved authority; an equivalent display network cannot replace them.
	for (const auto &link : _utility_power.links) {
		field(link.id.base()); field(link.source.base()); field(link.destination.base()); field(link.source_endpoint.base()); field(link.destination_endpoint.base());
		field(link.portal_revision); field(link.capacity_j); field(link.flow_j); field(link.active);
		field(to_underlying(link.kind)); field(link.local_owner.base()); field(to_underlying(link.local_domain)); field(link.local_capacity_w);
		if (link.kind == UtilityLinkKind::ElectricalRiser) { field(link.riser.base()); field(link.riser_capacity_w); } // Preserve genuine older role digests while including every paid machinery/rate scalar in the new declared role.
		if (link.kind == UtilityLinkKind::ElectricalFeed) field(link.generator.base()); // Earlier physical-role digests retain their original fields.
		if (link.kind == UtilityLinkKind::LocalCell && link.local_layer != MapLayer::DeepUnderground) field(to_underlying(link.local_layer)); // Preserve original Deep digests while covering the appended Underground electrical role.
	}
	field(_utility_power.local_link_revision);
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
	const std::array<uint64_t, 9> fields{to_underlying(proposal.domain), to_underlying(proposal.kind), proposal.grant.consumer.base(), proposal.grant.interval,
		proposal.owner.base(), proposal.quantity_j, proposal.destination.base(), proposal.link.base(), proposal.sequence_before};
	// Explicit scalar bytes reject changed quantity/kind/target independently
	// of pointer layout, without scanning unrelated networks on each work tick.
	for (uint64_t value : fields) {
		for (uint8_t byte = 0; byte < 8; ++byte) { hash ^= value & 0xFF; hash *= 1099511628211ULL; value >>= 8; }
	}
	return hash;
}

/** @copydoc PrepareUtilityMutation
 * @param domain Exact typed reservation domain; thermal preparation admits local use/release only. */
static UtilityMutationProposal PrepareUtilityDomainMutation(UtilityMutationKind kind, UtilityGrantID grant, uint64_t quantity_j,
	UtilityConsumerID destination, UtilityLinkID link, UtilityEnergyDomain domain)
{
	UtilityMutationProposal proposal;
	proposal.domain = domain; proposal.kind = kind; proposal.grant = grant; proposal.quantity_j = quantity_j;
	proposal.owner = _current_company; proposal.destination = destination; proposal.link = link;
	proposal.sequence_before = _utility_power.operation_sequence;
	auto *consumer = FindUtilityConsumer(grant.consumer);
	if (consumer == nullptr) return proposal;
	if (consumer->meter.domain != domain) return proposal; // Typed preparation cannot borrow a grant from another energy domain.
	if (domain == UtilityEnergyDomain::Heat && kind == UtilityMutationKind::TransferLayer) return proposal; // Heat stays Deep-local; only electricity may travel through a commissioned riser.
	if (!consumer->active) return proposal;
	proposal.error = CheckUtilityPowerOwner(consumer->owner, consumer->meter.layer);
	if (proposal.error != UtilityPowerError::None) return proposal;
	// An exact current reservation is required; previous/future grants and
	// an open generation phase cannot supply an action's electrical budget.
	if (!IsUtilityDomainAllocated(_utility_power, domain) || grant.interval != _utility_power.interval || consumer->meter.accounting_interval != grant.interval) {
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
		// This older one-hop operation spends a genuine fixed-joule portal
		// allowance. Paid risers store watts and require complete installed
		// paths through PrepareUtilityPathTransfer; treating their zero joule
		// cap as a legacy residual after nonzero flow would underflow and admit
		// a cable-free child that violates the original physical lineage.
		if (route->kind != UtilityLinkKind::ElectricalPortal) { proposal.error = UtilityPowerError::InvalidTopology; return proposal; }
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
		if (count + additional > MAX_UTILITY_POWER_SHARES || _utility_power.next_share + additional > MAX_UTILITY_POWER_SHARES || target->shares.size() + additional > MAX_UTILITY_INTERVAL_ACCOUNTS) {
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

/** @copydoc PrepareUtilityMutation */
UtilityMutationProposal PrepareUtilityMutation(UtilityMutationKind kind, UtilityGrantID grant, uint64_t quantity_j,
	UtilityConsumerID destination, UtilityLinkID link)
{
	return PrepareUtilityDomainMutation(kind, grant, quantity_j, destination, link, UtilityEnergyDomain::Electricity);
}
/** @copydoc PrepareUtilityHeatMutation */
UtilityMutationProposal PrepareUtilityHeatMutation(UtilityMutationKind kind, UtilityGrantID grant, uint64_t quantity_j)
{
	return PrepareUtilityDomainMutation(kind, grant, quantity_j, UtilityConsumerID{UINT32_MAX}, UtilityLinkID{UINT32_MAX}, UtilityEnergyDomain::Heat);
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
	const auto current = PrepareUtilityDomainMutation(proposal.kind, proposal.grant, proposal.quantity_j, proposal.destination, proposal.link, proposal.domain);
	if (current.error != UtilityPowerError::None) { receipt.error = current.error; return receipt; }
	if (current.seal != proposal.seal) { receipt.error = UtilityPowerError::StaleMutation; return receipt; }
	// Actual use requires a complete physical/inventory/effect participant;
	// a direct call cannot debit energy for a nonexistent gameplay benefit.
	if (proposal.kind == UtilityMutationKind::Consume && participant == nullptr) { receipt.error = UtilityPowerError::Refused; return receipt; }
	auto &consumer = *FindUtilityConsumer(proposal.grant.consumer);
	// Once bound to an exact converter, this original generator input can
	// settle only with its electrical publication/history. Tombstones preserve
	// that obligation; unused grant release remains independently available.
	if (proposal.kind == UtilityMutationKind::Consume && proposal.domain == UtilityEnergyDomain::Heat) {
		for (const auto &source : _utility_power.producers) {
			if (source.kind == UtilityGenerationKind::HeatConversion && source.conversion_input == consumer.meter.consumer_id) {
				receipt.error = UtilityPowerError::Refused; return receipt;
			}
		}
	}

	// A complete benefit must consume its own domain and immutable thermal
	// role. Generic electricity, direct-recipe and generator participants cannot
	// silently substitute one another's grants or invent conversion eligibility.
	if (proposal.kind == UtilityMutationKind::Consume && participant->EnergyDomain() != proposal.domain) { receipt.error = UtilityPowerError::Refused; return receipt; }
	if (proposal.kind == UtilityMutationKind::Consume && proposal.domain == UtilityEnergyDomain::Heat) {
		const auto use = consumer.effect == UtilityConsumerEffect::HeatGenerator ? UtilityHeatUse::ElectricalGenerator : UtilityHeatUse::DirectRecipe;
		if (participant->ThermalUse() != use) { receipt.error = UtilityPowerError::Refused; return receipt; }
	}
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
		// Installed paid tube sources can settle use only through their complete
		// physical/tariff participant. A generic ledger consumer cannot bypass
		// that invoice, while pure release and existing other source families
		// retain their independent native transaction boundary.
		if (proposal.kind == UtilityMutationKind::Consume && GetTubePowerConnection(share.producer) != nullptr &&
			!IsTubeExternalPowerCommitting()) { receipt.error = UtilityPowerError::Refused; return receipt; }
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
	std::vector<UtilityPowerShare> target_shares;
	uint32_t next_share = _utility_power.next_share;
	if (target != nullptr) {
		// Acquire every child and its physical segment before any complete
		// participant commits. Final numerical publication then only swaps
		// storage; even a one-hop path never allocates after physical success.
		target_shares = target->shares; target_shares.reserve(target_shares.size() + debits.size());
		for (const auto &[index, take] : debits) {
			const auto &parent = consumer.shares[index];
			/** @param candidate Existing retained destination slice. @return Whether this is the exact same parent and original one-hop arrival. */
			const auto matches = [&](const UtilityPowerShare &candidate) { return candidate.parent == parent.id && candidate.segment.size() == 1 && candidate.segment.front() == proposal.link; };
			const auto existing = std::ranges::find_if(target_shares, matches);
			if (existing == target_shares.end()) {
				UtilityPowerShare child{parent.producer, take, 0, 0, 0, proposal.link};
				child.id = UtilityShareID{next_share++}; child.parent = parent.id; child.segment = {proposal.link};
				target_shares.push_back(std::move(child));
			} else existing->allocated_j += take;
		}
		/** @param left First retained contribution. @param right Second retained contribution. @return Canonical original-source/parent/segment order; identities remain stable across sorting. */
		std::ranges::sort(target_shares, [](const UtilityPowerShare &left, const UtilityPowerShare &right) {
			return std::tie(left.producer, left.parent, left.segment) < std::tie(right.producer, right.parent, right.segment);
		});
	}
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
		target->shares.swap(target_shares); _utility_power.next_share = next_share;
		// Destination credit presents the same once-generated joules and keeps
		// original requested demand; no output meter or publication is created.
		target->meter.granted_energy_j += proposal.quantity_j;
		target->meter.unmet_energy_j -= proposal.quantity_j;
		FindUtilityLink(proposal.link)->flow_j += proposal.quantity_j;
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

/** Unpublished whole-action candidate; only the existing shared owner may publish its preallocated final state. */
struct UtilityPathTransferDraft {
	UtilityPowerError error = UtilityPowerError::InvalidReference; ///< Exact unchanged refusal or complete candidate admission.
	uint64_t quantity_j = 0; ///< Positive whole quantity admitted by every original physical participant.
	std::optional<UtilityPowerState> after; ///< Transient validation workspace, never a second live energy owner.
};

/** @param first Installed native cell. @param second Following installed native cell. @return Same-plane cardinal continuity using the map controller's world geometry. */
static bool AreUtilityTransferNeighbors(TileIndex first, TileIndex second)
{
	if (GetTileLayer(first) != GetTileLayer(second)) return false;
	return std::abs(static_cast<int>(TileX(first)) - static_cast<int>(TileX(second))) +
		std::abs(static_cast<int>(TilePlaneY(first)) - static_cast<int>(TilePlaneY(second))) == 1;
}

/**
 * Resolve every original paid crossing and installed inlet without granting an intermediate account.
 * @param request Exact source, final destination and complete physical route.
 * @param owner Actual controller of both reservations and every physical participant.
 * @param segment Receives bounded ordered new references only after their native checks succeed.
 * @return Complete admission or a refusal without any owner mutation.
 */
static UtilityPowerError CheckUtilityTransferPath(const UtilityPathTransferRequest &request, CompanyID owner,
	std::vector<UtilityLinkID> &segment)
{
	// Empty or oversized requests cannot bypass a paid crossing or create an
	// unbounded transient graph before the shared epoch limits are checked.
	if (request.legs.empty() || request.legs.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return UtilityPowerError::InvalidTopology;
	std::set<UtilityConsumerID> accounts{request.grant.consumer};
	std::set<UtilityLinkID> visited;
	UtilityConsumerID current = request.grant.consumer;
	TileIndex expected = request.source_port;
	// Every leg starts at the actual source/previous arriving housing and
	// finishes at its own original machinery port before crossing that edge.
	for (const auto &leg : request.legs) {
		const auto *edge = FindUtilityLink(leg.edge);
		if (edge == nullptr) return UtilityPowerError::InvalidReference;
		// Generic portal capacity and repeated account circulation cannot stand
		// in for an original operational paid riser on this complete route.
		const bool local = edge->kind == UtilityLinkKind::ElectricalFeed;
		if (!edge->active || (!local && edge->kind != UtilityLinkKind::ElectricalRiser) || edge->source != current ||
			leg.cells.empty() || !accounts.insert(edge->destination).second) return UtilityPowerError::InvalidTopology;
		// A direct local feed is terminal and starts at its own original plant;
		// it cannot become a riser bypass or relay an imported source slice.
		if (local && (request.legs.size() != 1 || expected != edge->source_endpoint)) return UtilityPowerError::InvalidTopology;
		if (local && (!IsGeothermalGeneratorConstructionComplete(edge->generator) || !HasGeothermalGeneratorInstalledPorts(edge->generator))) return UtilityPowerError::InvalidTopology; // Calendar/service-path interruptions forbid new work while retaining the original attachment and settled history.
		if (leg.cells.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS - segment.size()) return UtilityPowerError::Limit;
		const auto *from = FindUtilityConsumer(edge->source), *to = FindUtilityConsumer(edge->destination);
		if (from == nullptr || to == nullptr) return UtilityPowerError::InvalidReference; // Both declared participants must already exist; references never create output accounts.
		if (from->owner != owner || to->owner != owner) return UtilityPowerError::InvalidOwner; // Every crossing remains under the original controller.
		const auto readiness = CheckUtilityPowerLink(*edge, *from, *to);
		if (readiness != UtilityPowerError::None) return readiness;
		TileIndex previous = INVALID_TILE;
		// Exact ordered cells, rather than reachability through an arbitrary
		// installed network, determine the physical rights this action spends.
		for (const auto id : leg.cells) {
			const auto *cell = FindUtilityLink(id);
			if (cell == nullptr) return UtilityPowerError::InvalidReference;
			if (!cell->active || cell->kind != UtilityLinkKind::LocalCell || cell->local_domain != UtilityEnergyDomain::Electricity ||
				cell->local_owner != owner || cell->local_layer != from->meter.layer || !visited.insert(id).second) return UtilityPowerError::InvalidTopology; // Missing, foreign, thermal or repeated cells cannot acquire this electrical route's capacity.
			if (previous == INVALID_TILE) {
				if (cell->source_endpoint != expected) return UtilityPowerError::InvalidTopology;
			} else if (!AreUtilityTransferNeighbors(previous, cell->source_endpoint)) return UtilityPowerError::InvalidTopology;
			previous = cell->source_endpoint; segment.push_back(id);
		}
		const auto *riser = local ? nullptr : GetGeothermalRiserState(edge->riser);
		// The complete local cable reaches the actual recipient; a crossing
		// instead reaches its original machine's inlet before leaving the plane.
		if ((local ? previous != edge->destination_endpoint : riser == nullptr || previous != riser->site.port) ||
			!visited.insert(leg.edge).second) return UtilityPowerError::InvalidTopology;
		segment.push_back(leg.edge); current = edge->destination; expected = edge->destination_endpoint;
	}
	return current == request.destination ? UtilityPowerError::None : UtilityPowerError::InvalidTopology;
}

/**
 * Keep an imported slice's original physical rights when proposing its continuation.
 * @param share Exact current source slice in the validated shared graph.
 * @param nodes Dense retained slice references, including ancestors.
 * @param request Complete new path and explicitly installed sending port.
 * @param references New physical references that cannot repeat an ancestor.
 * @param destinations New account arrivals that cannot revisit an ancestor.
 * @return Whether this free slice may enter the proposed complete paid route.
 */
static bool IsUtilityTransferParent(const UtilityPowerShare &share, std::span<const UtilityPowerShare *const> nodes,
	const UtilityPathTransferRequest &request, const std::set<UtilityLinkID> &references,
	const std::set<UtilityConsumerID> &destinations)
{
	if (FreeUtilityShare(share) == 0) return false;
	if (share.parent.base() == UINT32_MAX) return true;
	const auto *incoming = FindUtilityLink(share.imported_link);
	// An imported quantity can continue only at its actual arriving housing;
	// genuine generic one-hop imports retain their original re-export refusal.
	if (incoming == nullptr || incoming->kind != UtilityLinkKind::ElectricalRiser || incoming->destination_endpoint != request.source_port) return false;
	const UtilityPowerShare *ancestor = &share;
	// Monotonic validated parent identities bound traversal. No ancestor edge,
	// cell or account may be reused to circulate the same original quantity.
	while (ancestor->parent.base() != UINT32_MAX) {
		for (const auto id : ancestor->segment) {
			const auto &link = *FindUtilityLink(id);
			if (!link.active || link.kind == UtilityLinkKind::ElectricalPortal || references.contains(id)) return false; // Lost or repeated original rights cannot be revived by a later route.
			if (link.kind != UtilityLinkKind::LocalCell &&
				(destinations.contains(link.source) || destinations.contains(link.destination))) return false; // New arrivals cannot revisit any retained account in the original path.
		}
		ancestor = nodes[ancestor->parent.base()];
	}
	return true;
}

/**
 * Preallocate and validate one complete shared-owner action using actual original source durations.
 * @param request Exact whole route and positive quantity/offer upper bound.
 * @param exact True requires the requested quantity; false computes the bounded current maximum.
 * @return A complete unpublished candidate or unchanged refusal; no physical or ledger mutation occurs here.
 */
static UtilityPathTransferDraft BuildUtilityPathTransferDraft(const UtilityPathTransferRequest &request, bool exact)
{
	UtilityPathTransferDraft draft;
	const auto profile = GetCurrentMultiLayerProfileSelection();
	if (_game_mode != GameMode::Normal || profile.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile.legacy) { draft.error = UtilityPowerError::UnsupportedProfile; return draft; } // Paid operations require actual current gameplay policy, independent of editor/camera state.
	const auto *source = FindUtilityConsumer(request.grant.consumer), *target = FindUtilityConsumer(request.destination);
	if (source == nullptr || target == nullptr) return draft; // Both reservations must be existing native participants.
	draft.error = CheckUtilityPowerOwner(source->owner, source->meter.layer);
	if (draft.error != UtilityPowerError::None) return draft;
	if (!source->active || !target->active || source->owner != target->owner) { draft.error = UtilityPowerError::InvalidOwner; return draft; } // Removed/foreign participants cannot receive original-company rights.
	if (source->meter.domain != UtilityEnergyDomain::Electricity || target->meter.domain != UtilityEnergyDomain::Electricity ||
		request.grant.consumer == request.destination || !IsValidTile(request.source_port) || GetTileLayer(request.source_port) != source->meter.layer) { draft.error = UtilityPowerError::InvalidTopology; return draft; } // Heat, self-circulation and a port on another plane cannot become electrical export.
	if (request.quantity_j == 0 || request.quantity_j > MAX_UTILITY_ACCOUNT_ENERGY_J) { draft.error = UtilityPowerError::InvalidQuantity; return draft; } // An action must be positive and fit the shared unit bound.
	if (!_utility_power.allocated) { draft.error = UtilityPowerError::AllocationRequired; return draft; }
	if (request.grant.interval != _utility_power.interval || source->meter.accounting_interval != _utility_power.interval ||
		target->meter.accounting_interval != _utility_power.interval) { draft.error = UtilityPowerError::InvalidInterval; return draft; } // Both actual requests and the grant must belong to this single closed accounting epoch.
	draft.error = ValidateUtilityPowerState(_utility_power, profile);
	if (draft.error != UtilityPowerError::None) return draft;
	if (!ValidateGeothermalUtilityLinks()) { draft.error = UtilityPowerError::InvalidTopology; return draft; }
	std::vector<UtilityLinkID> segment;
	draft.error = CheckUtilityTransferPath(request, source->owner, segment);
	if (draft.error != UtilityPowerError::None) return draft;
	std::set<UtilityLinkID> references(segment.begin(), segment.end());
	std::set<UtilityConsumerID> destinations;
	for (const auto &leg : request.legs) destinations.insert(FindUtilityLink(leg.edge)->destination); // All new arrivals participate in ancestor circulation checks.
	std::vector<const UtilityPowerShare *> nodes(_utility_power.next_share);
	std::map<UtilityLinkID, std::pair<uint64_t, uint64_t>> usage;
	// Derive spent cell/edge opportunity from newly traversed segments only;
	// descendants must never debit an ancestor's already settled edge again.
	for (const auto &consumer : _utility_power.consumers) {
		for (const auto &share : consumer.shares) {
			nodes[share.id.base()] = &share;
			const auto ticks = FindUtilityProducer(share.producer)->meter.economy_ticks;
			for (const auto id : share.segment) {
				if (!references.contains(id)) continue;
				auto &[spent, duration] = usage[id]; spent += share.allocated_j; duration = ticks;
			}
		}
	}
	std::optional<uint64_t> duration;
	// All already-spent resources on the proposed path must describe the
	// same synchronized window; a quote cannot multiply one physical rate.
	for (const auto &[id, spent] : usage) {
		if (duration.has_value() && *duration != spent.second) { draft.error = UtilityPowerError::InvalidInterval; return draft; }
		duration = spent.second;
	}
	std::vector<size_t> eligible;
	uint64_t available = 0;
	// Stable original-source/parent order selects one compatible publication
	// window. Different source windows cannot share these physical resources.
	for (size_t index = 0; index < source->shares.size(); ++index) {
		const auto &share = source->shares[index];
		// Terminal local feeds spend only an original local root, never re-exporting imported power.
		if (FindUtilityLink(request.legs.front().edge)->kind == UtilityLinkKind::ElectricalFeed && share.parent.base() != UINT32_MAX) continue;
		if (!IsUtilityTransferParent(share, nodes, request, references, destinations)) continue;
		const auto &producer = *FindUtilityProducer(share.producer);
		if (!producer.active || producer.meter.accounting_interval != _utility_power.interval || producer.meter.economy_ticks == 0) continue; // Only actual current original publications can provide physical opportunity.
		if (duration.has_value() && *duration != producer.meter.economy_ticks) continue; // One resource cannot accept a second synchronized window in the same epoch.
		duration = producer.meter.economy_ticks; eligible.push_back(index); available += FreeUtilityShare(share);
	}
	if (!duration.has_value()) { draft.error = UtilityPowerError::InsufficientGrant; return draft; }
	uint64_t capacity = std::min({request.quantity_j, available, target->meter.demand_energy_j - target->meter.granted_energy_j});
	// Every installed cell and original paid edge shares one aggregate rate
	// allowance, including spent/released rights from earlier whole actions.
	for (const auto id : segment) {
		const auto &link = *FindUtilityLink(id);
		if (link.kind == UtilityLinkKind::ElectricalFeed) continue; // The terminal adds no independent rate budget; purchased cable cells retain the aggregate physical limit.
		const auto rate = link.kind == UtilityLinkKind::LocalCell ? link.local_capacity_w : link.riser_capacity_w;
		const auto opportunity = IntegrateUtilityGeneration(rate, static_cast<uint32_t>(*duration), 0);
		if (!opportunity.has_value()) { draft.error = UtilityPowerError::InvalidQuantity; return draft; }
		const auto limit = opportunity->energy_j;
		const auto spent = usage.contains(id) ? usage.at(id).first : 0;
		if (spent > limit) { draft.error = UtilityPowerError::InvalidQuantity; return draft; }
		capacity = std::min(capacity, limit - spent);
	}
	if (capacity == 0 || (exact && capacity != request.quantity_j)) { draft.error = UtilityPowerError::InsufficientGrant; return draft; } // Exact preparation never silently commits a smaller physical export.
	auto after = _utility_power;
	auto &from = after.consumers[request.grant.consumer.base()], &to = after.consumers[request.destination.base()];
	uint64_t remaining = capacity;
	// Acquire complete child storage in a transient copy. Only exact matching
	// parents/segments merge, preserving distinct original physical histories.
	for (const auto index : eligible) {
		auto &parent = from.shares[index]; const auto take = std::min(remaining, FreeUtilityShare(parent));
		if (take == 0) continue;
		/** @param child Retained destination contribution. @return Whether it owns this exact original parent and newly traversed path. */
		const auto matches = [&](const UtilityPowerShare &child) { return child.parent == parent.id && child.segment == segment; };
		const auto existing = std::ranges::find_if(to.shares, matches);
		if (existing == to.shares.end()) {
			if (after.next_share >= MAX_UTILITY_POWER_SHARES || to.shares.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS) { draft.error = UtilityPowerError::Limit; return draft; } // Bounded identities and per-consumer rows must fit before final publication.
			UtilityPowerShare child{parent.producer, take, 0, 0, 0, request.legs.back().edge};
			child.id = UtilityShareID{after.next_share++}; child.parent = parent.id; child.segment = segment; to.shares.push_back(std::move(child));
		} else existing->allocated_j += take;
		parent.transferred_j += take; remaining -= take;
		if (remaining == 0) break;
	}
	/** @param left First retained slice. @param right Second retained slice. @return Canonical original-source/parent/path order without changing either slice identity. */
	std::ranges::sort(to.shares, [](const UtilityPowerShare &left, const UtilityPowerShare &right) {
		return std::tie(left.producer, left.parent, left.segment) < std::tie(right.producer, right.parent, right.segment);
	});
	to.meter.granted_energy_j += capacity; to.meter.unmet_energy_j -= capacity;
	for (const auto &leg : request.legs) after.links[leg.edge.base()].flow_j += capacity; // Each newly traversed paid crossing settles once; intermediate accounts acquire no grant.
	++after.operation_sequence;
	draft.error = ValidateUtilityPowerState(after, profile);
	if (draft.error != UtilityPowerError::None) return draft;
	draft.quantity_j = capacity; draft.after = std::move(after); return draft;
}

/** @param proposal Exact transient whole request. @return Deterministic field-wise fingerprint, independent of padding, pointers and host container layout. */
static uint64_t SealUtilityPathTransfer(const UtilityPathTransferProposal &proposal)
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Explicit integer field of the immutable proposal. */
	const auto field = [&hash](uint64_t value) {
		for (uint8_t byte = 0; byte < 8; ++byte) { hash ^= value & 0xFF; hash *= 1099511628211ULL; value >>= 8; } // Explicit byte order makes retry identity deterministic across native hosts.
	};
	field(proposal.owner.base()); field(proposal.sequence_before); field(proposal.request.grant.consumer.base()); field(proposal.request.grant.interval);
	field(proposal.request.destination.base()); field(proposal.request.source_port.base()); field(proposal.request.quantity_j); field(proposal.request.legs.size());
	// Include lengths and every ordered reference so insertion, deletion or
	// rearrangement cannot reuse an earlier admitted complete request.
	for (const auto &leg : proposal.request.legs) {
		field(leg.edge.base()); field(leg.cells.size());
		for (const auto id : leg.cells) field(id.base());
	}
	return hash;
}

/** @copydoc GetUtilityPathTransferOffer */
UtilityPathTransferOffer GetUtilityPathTransferOffer(const UtilityPathTransferRequest &request)
{
	const auto draft = BuildUtilityPathTransferDraft(request, false); return {draft.error, draft.quantity_j};
}

/** @copydoc PrepareUtilityPathTransfer */
UtilityPathTransferProposal PrepareUtilityPathTransfer(const UtilityPathTransferRequest &request)
{
	UtilityPathTransferProposal proposal; proposal.request = request; proposal.sequence_before = _utility_power.operation_sequence;
	proposal.error = BuildUtilityPathTransferDraft(request, true).error;
	if (proposal.error == UtilityPowerError::None) { proposal.owner = FindUtilityConsumer(request.grant.consumer)->owner; proposal.seal = SealUtilityPathTransfer(proposal); }
	return proposal;
}

/** @copydoc CommitUtilityPathTransfer */
UtilityMutationReceipt CommitUtilityPathTransfer(const UtilityPathTransferProposal &proposal)
{
	UtilityMutationReceipt receipt{UtilityPowerError::StaleMutation, _utility_power.operation_sequence, 0};
	// Modified, replayed or foreign proposals cannot publish a previously
	// admitted path after any accepted shared-owner operation has intervened.
	if (proposal.error != UtilityPowerError::None || proposal.owner != _current_company ||
		proposal.sequence_before != _utility_power.operation_sequence || proposal.seal != SealUtilityPathTransfer(proposal)) return receipt;
	if (_utility_mutation_committing) { receipt.error = UtilityPowerError::Refused; return receipt; }
	auto draft = BuildUtilityPathTransferDraft(proposal.request, true);
	if (draft.error != UtilityPowerError::None) { receipt.error = draft.error; return receipt; }
	AutoRestoreBackup transaction(_utility_mutation_committing, true);
	// All actual owners, original quantities, capacities and allocated storage
	// are ready. One non-failing move publishes every crossing and one sequence;
	// borrowed utility record pointers obey their existing mutation lifetime.
	_utility_power = std::move(*draft.after);
	return {UtilityPowerError::None, _utility_power.operation_sequence, draft.quantity_j};
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
		if (consumer.town == town_id) {
			consumer.town = TownID::Invalid(); consumer.town_terminal = StationID::Invalid();
			consumer.town_supply = UtilityConsumerID{UINT32_MAX}; consumer.town_source_port = INVALID_TILE;
		}
	}
}

/** @copydoc ResetUtilityPowerState */
void ResetUtilityPowerState()
{
	if (_utility_mutation_committing) return;
	_utility_power = {};
	ResetGeothermalHeatState();
	ResetGeothermalUtilityPathCache();
	_pending_utility_power.reset();
	_utility_mutation_committing = false;
	ResetUtilityPowerLoadStaging();
}

/** @copydoc HasUtilityPowerState */
bool HasUtilityPowerState()
{
	return _utility_power.interval != 1 || _utility_power.allocated || _utility_power.next_producer != 0 || _utility_power.next_consumer != 0 || _utility_power.next_link != 0 || _utility_power.next_share != 0 || _utility_power.operation_sequence != 0;
}

/** @copydoc RegisterUtilityHeatConversionProducer */
std::optional<UtilityProducerID> RegisterUtilityHeatConversionProducer(CompanyID owner, UtilityConsumerID input,
	uint8_t efficiency_percent, uint64_t capacity_w, bool execute)
{
	if (CheckUtilityPowerOwner(owner, MapLayer::DeepUnderground) != UtilityPowerError::None) return std::nullopt;
	const auto *consumer = FindUtilityConsumer(input);
	// Only the original active owned generator role may acquire one output;
	// a direct recipe, retired consumer or foreign account cannot become a plant.
	if (consumer == nullptr || !consumer->active || consumer->owner != owner || consumer->meter.domain != UtilityEnergyDomain::Heat ||
		consumer->effect != UtilityConsumerEffect::HeatGenerator || consumer->meter.layer != MapLayer::DeepUnderground ||
		efficiency_percent == 0 || efficiency_percent > 100 || capacity_w == 0 || capacity_w > MAX_UTILITY_RATE_W) return std::nullopt;
	// Retained bindings include tombstones. Reissuing an output for the same
	// original consumer would open another carry/efficiency and retry identity.
	for (const auto &source : _utility_power.producers) {
		if (source.kind == UtilityGenerationKind::HeatConversion && source.conversion_input == input) return std::nullopt;
	}
	// Bound both storage and the monotonic issuer before any registration; a
	// pure prediction cannot reserve a source slot or change its retry sequence.
	if (_utility_power.producers.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS || _utility_power.next_producer >= UINT32_MAX - 1) return std::nullopt;
	const UtilityProducerID id{_utility_power.next_producer};
	if (execute) {
		UtilityPowerProducer source; source.owner = owner; source.kind = UtilityGenerationKind::HeatConversion;
		source.meter.producer_id = id; source.meter.domain = UtilityEnergyDomain::Electricity; source.meter.layer = MapLayer::DeepUnderground;
		source.conversion_input = input; source.conversion_efficiency = efficiency_percent; source.conversion_capacity_w = capacity_w;
		_utility_power.producers.push_back(std::move(source)); ++_utility_power.next_producer; ++_utility_power.operation_sequence;
	}
	return id;
}

/** Unpublished complete original-Heat debit and exact electrical publication; no second live quantity authority exists. */
struct UtilityHeatConversionDraft {
	UtilityPowerError error = UtilityPowerError::InvalidReference; ///< Exact unchanged refusal until the entire candidate validates.
	UtilityMutationProposal heat; ///< Original native consume ticket presented to the actual whole participant.
	UtilityHeatConversion converted{}; ///< Existing canonical checked integer conversion, including its separate numerator.
	std::optional<UtilityPowerState> after; ///< All storage is acquired before any physical invoice/benefit can commit.
};

/** @param energy_j Bounded exact electrical output. @param ticks Validated synchronized duration; zero means no current publication. @return Rounded-up whole-watt equivalent for the actual output, independent of its immutable rated ceiling. */
static uint64_t GetExactUtilityConversionRate(uint64_t energy_j, uint32_t ticks)
{
	static_assert(MAX_UTILITY_ACCOUNT_ENERGY_J <= UINT64_MAX / 1000);
	if (ticks == 0) return 0;
	const uint64_t numerator = energy_j * 1000, duration = uint64_t{ticks} * 27;
	return numerator / duration + uint64_t{numerator % duration != 0};
}

/** @param request Exact original input/output/service opportunity. @return Fully validated unpublished candidate; no actual quantity, carry, ID or participant changes. */
static UtilityHeatConversionDraft BuildUtilityHeatConversionDraft(const UtilityHeatConversionRequest &request)
{
	UtilityHeatConversionDraft draft;
	draft.heat = PrepareUtilityHeatMutation(UtilityMutationKind::Consume, request.grant, request.heat_j);
	if (draft.heat.error != UtilityPowerError::None) { draft.error = draft.heat.error; return draft; }
	const auto *input = FindUtilityConsumer(request.grant.consumer); const auto *output = FindUtilityProducer(request.output);
	// Both immutable roles and original control must match before interpreting
	// the output ratio. Another electrical source cannot borrow this heat grant.
	if (output == nullptr || !output->active || output->kind != UtilityGenerationKind::HeatConversion || output->owner != input->owner ||
		output->conversion_input != input->meter.consumer_id || input->effect != UtilityConsumerEffect::HeatGenerator ||
		output->meter.domain != UtilityEnergyDomain::Electricity || output->meter.layer != MapLayer::DeepUnderground) return draft;
	if (_utility_power.allocated) { draft.error = UtilityPowerError::AlreadyAllocated; return draft; }
	if (output->meter.accounting_interval == request.grant.interval) { draft.error = UtilityPowerError::DuplicatePublication; return draft; }
	// Every committed run needs a bounded actual duration and fits the original
	// source rating. The output uses exact input-derived joules, never rate rounding.
	if (request.economy_ticks == 0 || request.economy_ticks > MAX_UTILITY_INTERVAL_TICKS) { draft.error = UtilityPowerError::InvalidQuantity; return draft; }
	const auto converted = ConvertUtilityHeatToElectricity(request.heat_j, output->conversion_efficiency, output->conversion_remainder);
	const auto ceiling = IntegrateUtilityGeneration(output->conversion_capacity_w, request.economy_ticks, 0);
	if (!converted.has_value() || !ceiling.has_value() || converted->electric_j > request.heat_j || converted->electric_j > ceiling->energy_j ||
		converted->electric_j > UINT64_MAX - output->generated_lifetime_j) { draft.error = UtilityPowerError::InvalidQuantity; return draft; }
	draft.converted = *converted;
	draft.after = _utility_power;
	auto &next = *draft.after; auto &consumer = next.consumers[request.grant.consumer.base()]; auto &source = next.producers[request.output.base()];
	size_t origins = 0;
	// Historical source pairs have a whole-ledger bound, including retired
	// outputs. Exhaustion refuses new use instead of discarding paid provenance.
	for (const auto &producer : next.producers) origins += producer.conversion_origins.size();
	uint64_t remaining = request.heat_j;
	// Settle the same canonical original slices as ordinary native consumption.
	// The copied candidate acquires any new origin row before the actual benefit.
	for (auto &share : consumer.shares) {
		const uint64_t take = std::min(remaining, FreeUtilityShare(share)); if (take == 0) continue;
		auto &thermal = next.producers[share.producer.base()];
		// A converter attributes only genuine same-domain original Heat sources;
		// underflow or lifetime overflow refuses the entire candidate unchanged.
		if (thermal.meter.domain != UtilityEnergyDomain::Heat || thermal.kind != UtilityGenerationKind::Geothermal || take > thermal.reserved_j ||
			take > UINT64_MAX - thermal.consumed_lifetime_j) { draft.error = UtilityPowerError::InvalidQuantity; return draft; }
		auto origin = std::ranges::lower_bound(source.conversion_origins, share.producer, {}, &UtilityHeatConversionOrigin::producer);
		if (origin == source.conversion_origins.end() || origin->producer != share.producer) {
			if (origins >= MAX_UTILITY_CONVERSION_ORIGINS) { draft.error = UtilityPowerError::Limit; return draft; }
			origin = source.conversion_origins.insert(origin, {share.producer, 0}); ++origins;
		}
		if (take > UINT64_MAX - origin->heat_j) { draft.error = UtilityPowerError::InvalidQuantity; return draft; }
		origin->heat_j += take; share.consumed_j += take; thermal.consumed_lifetime_j += take; thermal.reserved_j -= take;
		remaining -= take; if (remaining == 0) break;
	}
	if (remaining != 0) { draft.error = UtilityPowerError::InsufficientGrant; return draft; }
	source.meter.accounting_interval = request.grant.interval; source.meter.generation_rate_w = GetExactUtilityConversionRate(converted->electric_j, request.economy_ticks);
	source.meter.economy_ticks = request.economy_ticks; source.meter.generation_energy_j = converted->electric_j; source.meter.remainder_mj = 0;
	source.conversion_remainder = converted->percent_remainder; source.generated_lifetime_j += converted->electric_j; ++next.operation_sequence;
	draft.error = ValidateUtilityPowerState(next, GetCurrentMultiLayerProfileSelection()); return draft;
}

/** @param proposal Exact transient request/result. @return Stable field-wise fingerprint; no pointer, padding or host byte order is used. */
static uint64_t SealUtilityHeatConversion(const UtilityHeatConversionProposal &proposal)
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value One explicit request/result scalar; fixed bytes keep replay checks deterministic. */
	const auto field = [&hash](uint64_t value) {
		for (uint byte = 0; byte < 8; ++byte) { hash ^= value & 0xFF; hash *= 1099511628211ULL; value >>= 8; }
	};
	field(proposal.owner.base()); field(proposal.sequence_before); field(proposal.request.grant.consumer.base()); field(proposal.request.grant.interval);
	field(proposal.request.output.base()); field(proposal.request.heat_j); field(proposal.request.economy_ticks);
	field(proposal.electric_j); field(proposal.loss_j); field(proposal.remainder_after); return hash;
}

/** @copydoc PrepareUtilityHeatConversion */
UtilityHeatConversionProposal PrepareUtilityHeatConversion(const UtilityHeatConversionRequest &request)
{
	UtilityHeatConversionProposal proposal; proposal.request = request; proposal.sequence_before = _utility_power.operation_sequence;
	const auto draft = BuildUtilityHeatConversionDraft(request); proposal.error = draft.error;
	if (draft.error == UtilityPowerError::None) {
		proposal.owner = draft.heat.owner; proposal.electric_j = draft.converted.electric_j;
		proposal.loss_j = request.heat_j - draft.converted.electric_j; proposal.remainder_after = draft.converted.percent_remainder;
		proposal.seal = SealUtilityHeatConversion(proposal);
	}
	return proposal;
}

/** @copydoc RetireUtilityHeatConversion */
UtilityPowerError RetireUtilityHeatConversion(UtilityConsumerID input, UtilityProducerID output, bool execute)
{
	if (_utility_mutation_committing) return UtilityPowerError::Refused;
	auto *consumer = FindUtilityConsumer(input);
	auto *source = output.base() == UINT32_MAX ? nullptr : FindUtilityProducer(output);
	// Physical demolition retires only its original typed pair, including a
	// consumer waiting for output capacity. No unrelated account can be seized.
	if (consumer == nullptr || consumer->effect != UtilityConsumerEffect::HeatGenerator || consumer->meter.domain != UtilityEnergyDomain::Heat ||
		consumer->meter.layer != MapLayer::DeepUnderground || (output.base() != UINT32_MAX &&
		(source == nullptr || source->kind != UtilityGenerationKind::HeatConversion || source->conversion_input != input))) return UtilityPowerError::InvalidReference;
	if (_current_company != OWNER_DEITY && _current_company != consumer->owner) return UtilityPowerError::InvalidOwner;
	// A pending registration cannot omit an already issued original output.
	// Historical pair identity remains immutable even after either side retires.
	for (const auto &candidate : _utility_power.producers) {
		if (candidate.kind == UtilityGenerationKind::HeatConversion && candidate.conversion_input == input && candidate.meter.producer_id != output) return UtilityPowerError::InvalidReference;
	}
	const bool changed = consumer->active || (source != nullptr && source->active);
	if (!execute || !changed) return UtilityPowerError::None;
	// Return only the input's unused promises. Consumed Heat and original
	// output provenance survive as tombstones and cannot fund reconstruction.
	for (auto &share : consumer->shares) {
		const uint64_t unused = FreeUtilityShare(share); share.released_j += unused;
		FindUtilityProducer(share.producer)->reserved_j -= unused;
	}
	consumer->linked_producers.clear(); consumer->active = false;
	if (source != nullptr) {
		InvalidateTubeGeothermalSourcesForProducer(output); // Close exact original inlet roots before erasing output eligibility.
		RetireUtilityLocalFeeds(output); // Expire original physical attachments before this coupled output loses eligibility.
		// Release this output's unused downstream slices without disturbing
		// other electrical sources or already consumed/transferred descendants.
		for (auto &downstream : _utility_power.consumers) {
			std::erase(downstream.linked_producers, output);
			for (auto &share : downstream.shares) {
				if (share.producer != output) continue;
				const uint64_t unused = FreeUtilityShare(share); share.released_j += unused; source->reserved_j -= unused;
			}
		}
		source->active = false;
	}
	// Native demolition cannot leave live orphaned identities at terminal
	// sequence saturation. At that boundary every new benefit already refuses.
	if (_utility_power.operation_sequence < UINT64_MAX - 1) ++_utility_power.operation_sequence;
	return UtilityPowerError::None;
}

/** @copydoc CommitUtilityHeatConversion */
UtilityHeatConversionReceipt CommitUtilityHeatConversion(const UtilityHeatConversionProposal &proposal, UtilityMutationParticipant &participant)
{
	UtilityHeatConversionReceipt receipt{UtilityPowerError::StaleMutation, _utility_power.operation_sequence};
	if (_utility_mutation_committing) { receipt.error = UtilityPowerError::Refused; return receipt; }
	if (proposal.error != UtilityPowerError::None) { receipt.error = proposal.error; return receipt; }
	// Replayed, modified or foreign tickets cannot reuse an earlier admitted
	// ratio, whole result or input quantity after any accepted owner mutation.
	if (proposal.owner != _current_company || proposal.sequence_before != _utility_power.operation_sequence || proposal.seal != SealUtilityHeatConversion(proposal)) return receipt;
	auto draft = BuildUtilityHeatConversionDraft(proposal.request);
	if (draft.error != UtilityPowerError::None) { receipt.error = draft.error; return receipt; }
	// Derived results are rechecked as well as their seal; callers cannot
	// substitute a new output/carry while retaining a valid input reservation.
	if (draft.converted.electric_j != proposal.electric_j || draft.converted.percent_remainder != proposal.remainder_after ||
		proposal.loss_j != proposal.request.heat_j - draft.converted.electric_j) return receipt;
	AutoRestoreBackup transaction(_utility_mutation_committing, true);
	// All storage and both quantity domains already validate. Only the actual
	// generator benefit/invoice may now commit, with recursive writes refused.
	if (participant.EnergyDomain() != UtilityEnergyDomain::Heat || participant.ThermalUse() != UtilityHeatUse::ElectricalGenerator ||
		!participant.Validate(draft.heat) || !participant.Commit(draft.heat)) { receipt.error = UtilityPowerError::Refused; return receipt; }
	static_assert(std::is_nothrow_move_assignable_v<UtilityPowerState>); // Publication after the physical invoice must have no remaining fallible storage operation.
	_utility_power = std::move(*draft.after);
	return {UtilityPowerError::None, _utility_power.operation_sequence, proposal.request.heat_j, proposal.electric_j, proposal.loss_j};
}

/** @param state Complete structurally checked native ledger. @return Exact conversion reference/history/ratio admission without repairing or publishing a quantity. */
static UtilityPowerError ValidateUtilityHeatConversionState(const UtilityPowerState &state)
{
	std::set<UtilityConsumerID> inputs;
	std::vector<uint64_t> attributed(state.producers.size(), 0);
	size_t origins = 0;
	// Conversion provenance remains with the shared output owner for its full
	// lifetime. Ordinary sources cannot hide conversion carry or thermal history.
	for (const auto &source : state.producers) {
		if (source.kind != UtilityGenerationKind::HeatConversion) {
			if (source.conversion_input.base() != UINT32_MAX || source.conversion_efficiency != 0 || source.conversion_capacity_w != 0 ||
				source.conversion_remainder != 0 || !source.conversion_origins.empty()) return UtilityPowerError::InvalidReference;
			continue;
		}
		// The original generator consumer is immutable and unique, including
		// retired source history. A live source still needs its same live controller.
		if (source.meter.domain != UtilityEnergyDomain::Electricity || source.meter.layer != MapLayer::DeepUnderground ||
			source.conversion_input.base() >= state.consumers.size() || !inputs.insert(source.conversion_input).second ||
			source.conversion_efficiency == 0 || source.conversion_efficiency > 100 || source.conversion_remainder >= 100 ||
			source.conversion_capacity_w == 0 || source.conversion_capacity_w > MAX_UTILITY_RATE_W) return UtilityPowerError::InvalidReference;
		const auto &input = state.consumers[source.conversion_input.base()];
		if (input.meter.consumer_id != source.conversion_input || input.meter.domain != UtilityEnergyDomain::Heat ||
			input.meter.layer != MapLayer::DeepUnderground || input.effect != UtilityConsumerEffect::HeatGenerator ||
			(source.active && (!input.active || source.owner != input.owner))) return UtilityPowerError::InvalidReference;
		const auto ceiling = IntegrateUtilityGeneration(source.conversion_capacity_w, source.meter.economy_ticks, 0);
		// Only input-derived whole joules bypass the earlier rate integrator.
		// Their own source ceiling/duration and unused millijoule slot remain exact.
		if (!ceiling.has_value() || source.meter.remainder_mj != 0 || source.meter.generation_energy_j > ceiling->energy_j ||
			source.meter.generation_rate_w != GetExactUtilityConversionRate(source.meter.generation_energy_j, source.meter.economy_ticks)) return UtilityPowerError::InvalidQuantity;
		uint64_t heat = 0; UtilityProducerID previous{UINT32_MAX};
		// Each positive canonical original source is counted once. Global
		// attribution cannot exceed that source's already settled actual Heat use.
		for (const auto &origin : source.conversion_origins) {
			if (++origins > MAX_UTILITY_CONVERSION_ORIGINS) return UtilityPowerError::Limit;
			if (origin.producer.base() >= state.producers.size() || origin.heat_j == 0 ||
				(previous.base() != UINT32_MAX && origin.producer <= previous)) return UtilityPowerError::InvalidReference;
			const auto &thermal = state.producers[origin.producer.base()];
			if (thermal.meter.producer_id != origin.producer || thermal.meter.domain != UtilityEnergyDomain::Heat ||
				thermal.kind != UtilityGenerationKind::Geothermal || origin.heat_j > UINT64_MAX - heat ||
				origin.heat_j > UINT64_MAX - attributed[origin.producer.base()]) return UtilityPowerError::InvalidQuantity;
			previous = origin.producer; heat += origin.heat_j; attributed[origin.producer.base()] += origin.heat_j;
		}
		const uint64_t whole = (heat / 100) * source.conversion_efficiency;
		const uint64_t numerator = (heat % 100) * source.conversion_efficiency;
		// Quotient/remainder arithmetic validates lifetime history across uint64
		// without multiplying an unbounded lifetime Heat total by the efficiency.
		if (source.generated_lifetime_j != whole + numerator / 100 || source.conversion_remainder != numerator % 100 ||
			(!source.conversion_origins.empty() && source.meter.accounting_interval == 0)) return UtilityPowerError::InvalidQuantity;
	}
	// Direct recipes may own the remaining historical Heat use, but no original
	// thermal joule may be attributed to more electrical outputs than it supplied.
	for (size_t index = 0; index < attributed.size(); ++index) {
		if (attributed[index] > state.producers[index].consumed_lifetime_j) return UtilityPowerError::InvalidQuantity;
	}
	return UtilityPowerError::None;
}

/** Validate the sole candidate while controlling the final circular plant/feed join.
 * @param state Complete native utility candidate; no quantities or identities are repaired.
 * @param selection This world's accepted content/profile policy.
 * @param check_world_refs Resolve actual company, date and physical owners.
 * @param layer_count Candidate plane count when checking copied transport.
 * @param check_local_feed_refs Resolve original generator bindings now; only native staged adoption defers this until the mandatory final afterload audit.
 * @return Complete numerical/reference admission or its specific refusal.
 */
static UtilityPowerError ValidateUtilityPowerStateReferences(const UtilityPowerState &state, const MultiLayerProfileSelection &selection,
	bool check_world_refs, uint layer_count, bool check_local_feed_refs)
{
	// Empty older/Classic worlds retain an empty ledger; actual electrical
	// history requires the accepted native profile, never the startup selector.
	if (selection.id != MultiLayerGameplayProfileID::MultilayerIndustry &&
		(state.interval != 1 || state.allocated || state.heat_allocated || state.next_producer != 0 || state.next_consumer != 0 || state.next_link != 0 || state.next_share != 0 || state.operation_sequence != 0 || !state.producers.empty() || !state.consumers.empty() || !state.links.empty())) return UtilityPowerError::UnsupportedProfile;
	// Zero is an unpublished sentinel and the terminal epoch cannot rotate;
	// neither may become an accepted accounting interval.
	if (state.interval == 0 || state.interval == UINT64_MAX) return UtilityPowerError::InvalidInterval;
	if (state.operation_sequence == UINT64_MAX) return UtilityPowerError::Limit;
	if (state.next_producer == UINT32_MAX || state.next_consumer == UINT32_MAX || state.next_link == UINT32_MAX) return UtilityPowerError::Limit;
	if (state.next_share > MAX_UTILITY_POWER_SHARES) return UtilityPowerError::Limit;
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
	std::set<std::pair<UtilityConsumerID, UtilityConsumerID>> active_routes;
	std::set<TownID> bound_towns;
	std::set<std::pair<TileIndex, UtilityEnergyDomain>> local_cells;
	size_t local_count = 0;
	// Exact retained link IDs and participant references must be complete
	// before imported source slices can resolve their original donor account.
	for (size_t index = 0; index < state.links.size(); ++index) {
		const auto &link = state.links[index];
		if (link.id.base() != index) return UtilityPowerError::InvalidReference;
		if (link.kind == UtilityLinkKind::LocalCell) {
			++local_count;
			const auto *terms = GetMultiLayerLocalUtilityLinkContract(link.local_domain);
			// A local cell has no reservation/export/flow fields. Its paid native
			// definition alone supplies bounded installed rate and domain.
			if (terms == nullptr || !IsMultiLayerLocalUtilityLayer(link.local_domain, link.local_layer) || to_underlying(link.local_layer) >= layer_count || link.local_owner.base() >= MAX_COMPANIES || link.local_capacity_w != terms->capacity_w ||
				link.source != UtilityConsumerID{UINT32_MAX} || link.destination != UtilityConsumerID{UINT32_MAX} || link.portal_revision != 0 ||
				link.capacity_j != 0 || link.flow_j != 0 || link.source_endpoint == INVALID_TILE || link.destination_endpoint != link.source_endpoint ||
				link.riser != IndustryID::Invalid() || link.riser_capacity_w != 0 || link.generator != IndustryID::Invalid()) return UtilityPowerError::InvalidTopology;
			if (link.active && !local_cells.emplace(link.source_endpoint, link.local_domain).second) return UtilityPowerError::InvalidTopology; // Duplicate identities cannot multiply the same installed tile/domain capacity.
			if (check_world_refs && (!IsValidTile(link.source_endpoint) || GetTileLayer(link.source_endpoint) != link.local_layer)) return UtilityPowerError::InvalidTopology; // Retired installations retain their original address/plane as well; only live floor and company availability may disappear.
			if (check_world_refs && link.active) {
				// Actual saved company, technology and excavated declared rock floor
				// must exist; a Surface coordinate cannot become a physical pipe.
				if (!Company::IsValidID(link.local_owner) || !IsValidTile(link.source_endpoint) || GetTileLayer(link.source_endpoint) != link.local_layer ||
					!CheckDeepLayerAvailability(selection, TimerGameCalendar::date) || CheckExcavatedFootprint(std::array{link.source_endpoint}).error != ExcavationError::None) return UtilityPowerError::InvalidTopology;
			}
			continue;
		}
		// Portal routes preserve their genuine older role and cannot smuggle a
		// local owner, thermal domain or rate into electrical transfer history.
		if ((link.kind != UtilityLinkKind::ElectricalPortal && link.kind != UtilityLinkKind::ElectricalRiser && link.kind != UtilityLinkKind::ElectricalFeed) || link.local_owner != CompanyID::Invalid() ||
			link.local_domain != UtilityEnergyDomain::Electricity || link.local_capacity_w != 0 || link.local_layer != MapLayer::DeepUnderground) return UtilityPowerError::InvalidTopology;
		// Both endpoints name actual retained consumers; absent references can
		// never be repaired into an output account or a second generator.
		if (link.source.base() >= state.consumers.size() || link.destination.base() >= state.consumers.size()) return UtilityPowerError::InvalidReference;
		const auto &source = state.consumers[link.source.base()];
		const auto &destination = state.consumers[link.destination.base()];
		// Plants bind their conversion pair from this shared candidate. Defer
		// only the reverse feed-to-plant check until both owners have accepted;
		// ordinary company, portal and riser checks retain their native stage.
		const auto admission = CheckUtilityPowerLink(link, source, destination,
			check_world_refs && link.active && (check_local_feed_refs || link.kind != UtilityLinkKind::ElectricalFeed));
		if (admission != UtilityPowerError::None) return admission;
		// A single active directed participant pair has one metered capacity;
		// duplicate routes cannot multiply its otherwise bounded import budget.
		if (link.active && !active_routes.emplace(link.source, link.destination).second) return UtilityPowerError::InvalidTopology;
		if (link.active && link.kind == UtilityLinkKind::ElectricalRiser) {
			// Different consumer IDs or a concealed older portal role cannot
			// multiply one installed native sender's frozen physical rate.
			for (const auto &other : state.links) {
				if (other.active && other.kind != UtilityLinkKind::LocalCell && other.id != link.id && other.source_endpoint == link.source_endpoint) return UtilityPowerError::InvalidTopology;
			}
		}
		// Metered flow belongs to the current closed epoch and exact requested
		// participants; registration in an open phase has zero credit.
		if (link.flow_j != 0 && (!state.allocated || source.meter.accounting_interval != state.interval || destination.meter.accounting_interval != state.interval)) return UtilityPowerError::InvalidInterval;
		links.emplace(link.id, &link);
	}
	// The topology revision is a saved change receipt. Resource-only operations
	// never change it, and omitted installed history cannot default to zero.
	if ((local_count == 0) != (state.local_link_revision == 0) || state.local_link_revision < local_count ||
		state.local_link_revision > state.operation_sequence || state.local_link_revision == UINT64_MAX) return UtilityPowerError::InvalidTopology;
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
		if (to_underlying(source.kind) > to_underlying(UtilityGenerationKind::HeatConversion)) return UtilityPowerError::InvalidReference;
		// Each typed source owns one quantity domain and no consumer identity.
		// Deep geothermal Heat cannot masquerade as solar/external electricity.
		if (to_underlying(meter.domain) > to_underlying(UtilityEnergyDomain::Heat) || meter.consumer_id.base() != UINT32_MAX ||
			(meter.domain == UtilityEnergyDomain::Heat && (source.kind != UtilityGenerationKind::Geothermal || meter.layer != MapLayer::DeepUnderground))) return UtilityPowerError::InvalidReference;
		// Publication uses frozen rate/duration/energy/carry limits and cannot
		// name a future interval that bypasses once-only generation.
		if (meter.accounting_interval > state.interval || meter.generation_rate_w > MAX_UTILITY_RATE_W || meter.economy_ticks > MAX_UTILITY_INTERVAL_TICKS || meter.generation_energy_j > MAX_UTILITY_ACCOUNT_ENERGY_J || meter.remainder_mj >= 1000) return UtilityPowerError::InvalidQuantity;
		// Demand and grants belong to consumer meters; unused source slots
		// cannot conceal a second reservation or generation credit.
		if (meter.demand_energy_j != 0 || meter.granted_energy_j != 0 || meter.unmet_energy_j != 0) return UtilityPowerError::InvalidQuantity;
		// Neither an open generation phase nor a retired source can promise
		// usable output; source loss must release all remaining reservations.
		if ((!IsUtilityDomainAllocated(state, meter.domain) && source.reserved_j != 0) || (!source.active && source.reserved_j != 0)) return UtilityPowerError::InvalidQuantity;
		// Lifetime settlement cannot exceed real publication and must retain
		// at least current interval generation even after prior rotations.
		if (source.consumed_lifetime_j > source.generated_lifetime_j || source.generated_lifetime_j < meter.generation_energy_j) return UtilityPowerError::InvalidQuantity;
		// A saved publication must correspond to its actual declared rate/ticks
		// and a legal previous fractional carry, never arbitrary invented joules.
		// Conversion derives exact joules from original Heat, with its own rate
		// ceiling and percent carry validated below; other sources retain the
		// original rate-integrator identity and millijoule accounting unchanged.
		if (source.kind != UtilityGenerationKind::HeatConversion) {
			const uint64_t emitted = meter.generation_energy_j * 1000 + meter.remainder_mj;
			const uint64_t produced = meter.generation_rate_w * meter.economy_ticks * 27;
			if (emitted < produced || emitted - produced >= 1000) return UtilityPowerError::InvalidQuantity;
		}
		sources.emplace(meter.producer_id, &source);
	}
	std::map<StationID, size_t> terminal_towns;
	std::set<UtilityConsumerID> forwarding_sources;
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
		// A consumer has one explicit domain and matching immutable role, with
		// no hidden producer identity. Thermal participants remain Deep-local;
		// electrical effects cannot reinterpret their heat grants as electricity.
		if (to_underlying(meter.domain) > to_underlying(UtilityEnergyDomain::Heat) || meter.producer_id.base() != UINT32_MAX ||
			to_underlying(consumer.effect) > to_underlying(UtilityConsumerEffect::HeatGenerator) || IsUtilityHeatEffect(consumer.effect) != (meter.domain == UtilityEnergyDomain::Heat) ||
			(meter.domain == UtilityEnergyDomain::Heat && meter.layer != MapLayer::DeepUnderground)) return UtilityPowerError::InvalidReference;
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
				(consumer.town_effect_applied && (!IsUtilityDomainAllocated(state, meter.domain) || meter.accounting_interval != state.interval))) return UtilityPowerError::InvalidQuantity;
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
		// New physical services carry three references together; genuine older
		// effect-only rows remain absent. Preview validates the detached graph,
		// while full acceptance additionally resolves the real terminal identity.
		if (consumer.town_terminal == StationID::Invalid()) {
			if (consumer.town_supply.base() != UINT32_MAX || consumer.town_source_port != INVALID_TILE) return UtilityPowerError::InvalidReference; // Partial bindings cannot silently become a new source or port after load.
		} else {
			if (!consumer.active || consumer.effect != UtilityConsumerEffect::TownGrowth || consumer.town == TownID::Invalid() ||
				consumer.town_supply == meter.consumer_id || consumer.town_source_port == INVALID_TILE ||
				(check_world_refs && consumer.town_source_port.base() >= Map::Size())) return UtilityPowerError::InvalidReference;
			// Identity retention is contiguous and bounded above. Check the
			// indexed identity explicitly before using its detached transport;
			// validation need not rescan every consumer for each service.
			const auto *source = consumer.town_supply.base() < state.consumers.size() ? &state.consumers[consumer.town_supply.base()] : nullptr;
			if (source == nullptr || source->meter.consumer_id != consumer.town_supply || source->owner != consumer.owner ||
				source->meter.layer != MapLayer::DeepUnderground || source->meter.domain != UtilityEnergyDomain::Electricity ||
				source->effect != UtilityConsumerEffect::AssemblyWork || source->linked_producers.size() > 1) return UtilityPowerError::InvalidReference;
			// Real source retirement removes eligibility from its forwarding
			// consumer. Preserve that inert original reference through load;
			// live admission still requires exactly one real active converter.
			if (!source->linked_producers.empty()) {
				const auto producer = sources.find(source->linked_producers.front());
				if (producer == sources.end() || producer->second->kind != UtilityGenerationKind::HeatConversion || producer->second->owner != consumer.owner) return UtilityPowerError::InvalidReference; // A retained eligible source must still be the original same-company conversion kind, never a substituted rate source.
			}
			if (++terminal_towns[consumer.town_terminal] > 16) return UtilityPowerError::Limit;
			if (!forwarding_sources.insert(consumer.town_supply).second) return UtilityPowerError::InvalidTopology;
			if (check_world_refs) {
				const auto *terminal = Station::GetIfValid(consumer.town_terminal);
				if (terminal == nullptr || terminal->owner != consumer.owner) return UtilityPowerError::InvalidReference; // Destruction/company loss must revoke the saved reference before StationID reuse.
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
			if ((previous_link.base() != UINT32_MAX && id <= previous_link) || !found->second->active || found->second->owner != consumer.owner || found->second->meter.layer != meter.layer || found->second->meter.domain != meter.domain) return UtilityPowerError::InvalidTopology;
			previous_link = id;
		}
		uint64_t allocated = 0;
		std::optional<std::tuple<UtilityProducerID, UtilityShareID, std::vector<UtilityLinkID>>> previous_share;
		// Retired origins may retain settled lineage but no usable balance;
		// validate each addition before arithmetic to prevent crafted overflow.
		for (const auto &share : consumer.shares) {
			const auto found = sources.find(share.producer);
			if (found == sources.end()) return UtilityPowerError::InvalidReference;
			const std::tuple identity(share.producer, share.parent, share.segment);
			// Equal producers and final edges can arrive through distinct exact
			// parents/paths; only identical complete incoming rights may merge.
			if (previous_share.has_value() && identity <= *previous_share) return UtilityPowerError::InvalidReference;
			previous_share = identity;
			// Each independent use/release/transfer fits its original allocation
			// before subtraction; crafted rows cannot wrap into free availability.
			if (share.allocated_j == 0 || share.allocated_j > meter.granted_energy_j - allocated || share.consumed_j > share.allocated_j ||
				share.released_j > share.allocated_j - share.consumed_j || share.transferred_j > share.allocated_j - share.consumed_j - share.released_j) return UtilityPowerError::InvalidQuantity;
			const auto &source = *found->second;
			if (meter.domain == UtilityEnergyDomain::Heat && (share.imported_link.base() != UINT32_MAX || share.transferred_j != 0)) return UtilityPowerError::InvalidTopology; // Thermal shares are Deep-local and cannot carry electrical riser/import lineage.
			if (source.owner != consumer.owner || source.meter.domain != meter.domain) return UtilityPowerError::InvalidTopology; // Saved source slices preserve both ownership and domain; consumed heat cannot appear as electrical lineage.
			const uint64_t remaining = FreeUtilityShare(share);
			// A retired consumer cannot retain usable energy even if its old
			// numeric company ID now belongs to a new live company.
			if (!consumer.active && remaining != 0) return UtilityPowerError::InvalidQuantity;
			if (share.imported_link.base() == UINT32_MAX) {
				if (source.meter.layer != meter.layer) return UtilityPowerError::InvalidTopology;
			} else {
				const auto route = links.find(share.imported_link);
				if (route == links.end()) return UtilityPowerError::InvalidReference;
				const auto &link = *route->second;
				// The final edge names this actual reservation. Complete parent
				// lineage below resolves original layers and each prior transition.
				if (link.destination != meter.consumer_id) return UtilityPowerError::InvalidTopology;
				if (!link.active && remaining != 0) return UtilityPowerError::InvalidQuantity;
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
		if ((!IsUtilityDomainAllocated(state, meter.domain) || meter.accounting_interval != state.interval) && (meter.demand_energy_j != 0 || allocated != 0 || !consumer.shares.empty())) return UtilityPowerError::InvalidInterval;
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
	// Exact direct parent/child amounts and newly traversed physical segments
	// conserve every transfer, including branches and later descendants.
	const auto lineage = ValidateUtilityPowerLineage(state, check_world_refs ? UtilityLineageGeometry{Map::SizeX(), Map::SizeY(), MultiLayerMap::GetLayerCount()} : UtilityLineageGeometry{0, 0, layer_count});
	if (lineage != UtilityPowerError::None) return lineage;
	if (check_world_refs) {
		// Cardinal geometry alone does not declare a machinery port. Resolve
		// each original active paid transition against its actual loaded native
		// association; retired history never adopts a reused Industry identity.
		for (const auto &consumer : state.consumers) {
			for (const auto &share : consumer.shares) {
				TileIndex inlet = INVALID_TILE;
				for (const auto id : share.segment) {
					const auto &link = state.links[id.base()];
					if (link.kind == UtilityLinkKind::LocalCell) { inlet = link.source_endpoint; continue; }
					if (link.active && link.kind == UtilityLinkKind::ElectricalRiser) {
						const auto *riser = GetGeothermalRiserState(link.riser);
						if (riser == nullptr || inlet != riser->site.port) return UtilityPowerError::InvalidTopology;
					}
					inlet = INVALID_TILE;
				}
			}
		}
	}
	return ValidateUtilityHeatConversionState(state);
}

/** @copydoc ValidateUtilityPowerState */
UtilityPowerError ValidateUtilityPowerState(const UtilityPowerState &state, const MultiLayerProfileSelection &selection, bool check_world_refs, uint layer_count)
{
	return ValidateUtilityPowerStateReferences(state, selection, check_world_refs, layer_count, true);
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
	// Native generators need the restored utility identities before they can
	// accept. Afterload must resolve every deferred original feed before ticks.
	if (ValidateUtilityPowerStateReferences(*_pending_utility_power, GetCurrentMultiLayerProfileSelection(), true,
		MultiLayerMap::GetLayerCount(), false) != UtilityPowerError::None) return false;
	_utility_power = std::move(*_pending_utility_power);
	ResetGeothermalUtilityPathCache();
	_pending_utility_power.reset();
	return true;
}
