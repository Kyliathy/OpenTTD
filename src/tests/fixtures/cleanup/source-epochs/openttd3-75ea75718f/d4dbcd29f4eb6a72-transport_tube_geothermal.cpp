/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_geothermal.cpp Native geothermal tube inlet ownership and scoped shared-supply lifecycle; no energy or invoice is stored here.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BBB.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group9C.
 */
#include "stdafx.h"
#include "transport_tube_geothermal.h"
#include "geothermal_generator.h"
#include "utility_power_transfer.h"
#include "company_base.h"
#include "company_func.h"
#include "depot_base.h"
#include "station_base.h"
#include "station_map.h"
#include "rail_map.h"
#include "core/backup_type.hpp"
#include "safeguards.h"

static TubeGeothermalPowerState _tube_geothermal; ///< Reference-only original inlet owner; all quantities remain in the shared utility root.
static std::optional<TubeGeothermalPowerState> _pending_tube_geothermal; ///< Complete unpublished native references, never an electrical owner.
static bool _tube_geothermal_committing = false; ///< Reference commits cannot recursively register/close another source.

/** @param id Original issued inlet incarnation. @return Mutable retained metadata, or nullptr without acquiring any identity. */
static TubeGeothermalSource *FindTubeGeothermalSource(TubeGeothermalSourceID id)
{
	if (id.base() >= _tube_geothermal.sources.size()) return nullptr;
	return &_tube_geothermal.sources[id.base()];
}

/** @copydoc GetTubeGeothermalSource */
const TubeGeothermalSource *GetTubeGeothermalSource(TubeGeothermalSourceID id) { return FindTubeGeothermalSource(id); }

/**
 * Resolve original native physical references before reserving or using shared output.
 * @param source Exact original plant/output, controller, port and recipient incarnation.
 * @return Physical/date admission or refusal; service funds and electrical grants are separate.
 */
static TubePowerError CheckTubeGeothermalPhysicalSource(const TubeGeothermalSource &source)
{
	const auto *plant = GetGeothermalGeneratorState(source.generator);
	const auto *producer = GetUtilityPowerProducer(source.producer);
	// Original native machinery and its unique conversion pair must exist.
	// A numerical output, reused industry or foreign company cannot power an inlet.
	if (plant == nullptr || producer == nullptr || !Company::IsValidID(source.owner) || plant->owner != source.owner ||
		plant->electrical_producer != source.producer || producer->kind != UtilityGenerationKind::HeatConversion ||
		producer->conversion_input != plant->heat_consumer || !producer->active || producer->owner != source.owner ||
		producer->meter.layer != MapLayer::DeepUnderground || producer->meter.domain != UtilityEnergyDomain::Electricity) return TubePowerError::InvalidOwner;
	if (!IsGeothermalGeneratorConstructionComplete(source.generator) || !HasGeothermalGeneratorInstalledPorts(source.generator)) return TubePowerError::Unavailable; // Actual model/date/profile and both installed native ports precede physical source readiness; this never runs generation or purchases service.
	const auto inlet = GetTubeTerminal(source.inlet, source.owner);
	// Original facility identity, not proximity or a coincident replacement,
	// supplies the actual Deep tube boundary at the cable's receiving end.
	if (!inlet.has_value() || inlet->layer != MapLayer::DeepUnderground || inlet->depot != source.depot ||
		inlet->station != source.station || GetGeothermalGeneratorPort(source.generator) != source.port) return TubePowerError::InvalidTopology;
	if (!FindGeothermalUtilityPath(source.owner, UtilityEnergyDomain::Electricity, source.port, source.inlet).has_value()) return TubePowerError::InvalidTopology;
	return TubePowerError::None;
}

/** @param source Actual retained source. @return Whether its dedicated live forwarding reservation still names only the original shared output. */
static bool HasTubeGeothermalSupply(const TubeGeothermalSource &source)
{
	const auto *supply = GetUtilityPowerConsumer(source.supply);
	// Physical metadata cannot adopt another consumer, company, domain or
	// source's original promise by substituting a same-valued numerical ID.
	return supply != nullptr && supply->active && supply->owner == source.owner && supply->effect == UtilityConsumerEffect::TubeWork &&
		supply->meter.domain == UtilityEnergyDomain::Electricity && supply->meter.layer == MapLayer::DeepUnderground &&
		supply->linked_producers == std::vector{source.producer};
}

/** @copydoc GetTubeGeothermalSourcePath */
std::optional<GeothermalUtilityPath> GetTubeGeothermalSourcePath(TubeGeothermalSourceID id)
{
	const auto *source = GetTubeGeothermalSource(id);
	// A retired incarnation or closed/rebound root cannot acquire a fresh
	// physical offer through an old source handle, even after native pool reuse.
	if (source == nullptr || !source->active || !HasTubeGeothermalSupply(*source) ||
		CheckTubeGeothermalPhysicalSource(*source) != TubePowerError::None) return std::nullopt;
	return FindGeothermalUtilityPath(source->owner, UtilityEnergyDomain::Electricity, source->port, source->inlet);
}

/** @copydoc ConnectTubeGeothermalPower */
TubeGeothermalPowerResult ConnectTubeGeothermalPower(IndustryID generator, TileIndex inlet, bool execute)
{
	TubeGeothermalPowerResult result;
	if (_tube_geothermal_committing) return result;
	const auto *plant = GetGeothermalGeneratorState(generator);
	// Only the actual controller can associate an existing native plant and
	// tube inlet; estimates cannot acquire output identities or finish machinery.
	if (plant == nullptr || plant->owner != _current_company || !Company::IsValidID(plant->owner)) { result.error = TubePowerError::InvalidOwner; return result; }
	const auto terminal = GetTubeTerminal(inlet, plant->owner);
	if (!terminal.has_value()) { result.error = TubePowerError::InvalidTopology; return result; }
	TubeGeothermalSource source;
	source.id = TubeGeothermalSourceID{_tube_geothermal.next_source}; source.generator = generator; source.producer = plant->electrical_producer;
	source.owner = plant->owner; source.port = GetGeothermalGeneratorPort(generator); source.inlet = inlet;
	source.depot = terminal->depot; source.station = terminal->station;
	result.error = CheckTubeGeothermalPhysicalSource(source);
	if (result.error != TubePowerError::None) return result;
	// Retained source history and reference receipts are bounded separately
	// from shared utility accounts; retirement never frees a source ID for reuse.
	if (_tube_geothermal.sources.size() >= MAX_TUBE_GEOTHERMAL_SOURCES || _tube_geothermal.next_source >= UINT32_MAX - 1 ||
		_tube_geothermal.sequence >= UINT64_MAX - 1) { result.error = TubePowerError::Limit; return result; }
	// The same live original plant/facility has one explicit source handle.
	// Another plant or another inlet retains an independent physical association.
	for (const auto &existing : _tube_geothermal.sources) {
		if (existing.active && existing.generator == generator && existing.inlet == inlet) { result.error = TubePowerError::Refused; return result; }
	}
	static constexpr uint64_t capacity = MAX_UTILITY_RATE_W * 27 / 1000; // Shared canonical one-tick request ceiling, not a generated amount or feature-local budget.
	const auto supply = RegisterUtilityPowerConsumer(source.owner, MapLayer::DeepUnderground, UtilityConsumerEffect::TubeWork, capacity,
		std::array{source.producer}, false);
	if (!supply.has_value()) { result.error = TubePowerError::Ledger; return result; }
	source.supply = *supply;
	if (execute) {
		_tube_geothermal.sources.reserve(_tube_geothermal.sources.size() + 1); // Acquire fallible metadata storage before the sole shared allocator issues its forwarding reservation.
		AutoRestoreBackup committing(_tube_geothermal_committing, true);
		const auto issued = RegisterUtilityPowerConsumer(source.owner, MapLayer::DeepUnderground, UtilityConsumerEffect::TubeWork, capacity, std::array{source.producer});
		if (!issued.has_value()) { result.error = TubePowerError::Ledger; return result; }
		assert(issued == supply); source.supply = *issued; // The synchronized allocator has no intervening callback; estimates and execution resolve the same actual owner identity.
		_tube_geothermal.sources.push_back(source); ++_tube_geothermal.next_source; ++_tube_geothermal.sequence;
	}
	result.error = TubePowerError::None; result.source = source.id; result.supply = source.supply; return result;
}

/** @copydoc CloseTubeGeothermalPower */
TubePowerError CloseTubeGeothermalPower(TubeGeothermalSourceID id, bool execute)
{
	if (_tube_geothermal_committing) return TubePowerError::Refused;
	auto *source = FindTubeGeothermalSource(id);
	if (source == nullptr || !source->active) return TubePowerError::Refused; // Retired identities cannot close or reopen a replacement inlet.
	if (_current_company != source->owner) return TubePowerError::InvalidOwner;
	if (_tube_geothermal.sequence >= UINT64_MAX - 1) return TubePowerError::Limit;
	if (RetireUtilityLocalFeedSupply(source->supply, false) != UtilityPowerError::None) return TubePowerError::Ledger;
	if (execute) {
		AutoRestoreBackup committing(_tube_geothermal_committing, true);
		if (RetireUtilityLocalFeedSupply(source->supply) != UtilityPowerError::None) return TubePowerError::Ledger;
		source->active = false; ++_tube_geothermal.sequence;
	}
	return TubePowerError::None;
}

/** @param source Original physical association being lost. Retire only its unused shared forwarding rights without touching native generation or sibling sources. */
static void RetireTubeGeothermalSource(TubeGeothermalSource &source)
{
	InvalidateUtilityLocalFeedSupply(source.supply); source.active = false;
}

/** @copydoc InvalidateTubeGeothermalSourcesForTile */
void InvalidateTubeGeothermalSourcesForTile(TileIndex tile)
{
	if (_tube_geothermal_committing) return;
	bool changed = false;
	// Only original receiving facilities can invalidate their metadata here.
	// Cable repair and unrelated guideway notifications retain source identity.
	for (auto &source : _tube_geothermal.sources) {
		if (!source.active || source.inlet != tile) continue;
		const auto terminal = GetTubeTerminal(tile, source.owner);
		if (terminal.has_value() && terminal->depot == source.depot && terminal->station == source.station) continue;
		RetireTubeGeothermalSource(source); changed = true;
	}
	if (changed && _tube_geothermal.sequence < UINT64_MAX - 1) ++_tube_geothermal.sequence; // Actual loss closes a whole notification batch without overflowing terminal reference history.
}

/** @copydoc InvalidateTubeGeothermalSourcesForProducer */
void InvalidateTubeGeothermalSourcesForProducer(UtilityProducerID producer)
{
	if (_tube_geothermal_committing) return;
	bool changed = false;
	// One actual output may serve several original inlets. Only those physical
	// associations retire before that output's eligibility disappears.
	for (auto &source : _tube_geothermal.sources) {
		if (!source.active || source.producer != producer) continue;
		RetireTubeGeothermalSource(source); changed = true;
	}
	if (changed && _tube_geothermal.sequence < UINT64_MAX - 1) ++_tube_geothermal.sequence; // Source loss must preserve a terminal receipt without wrapping or minting an old incarnation.
}

/** @copydoc ChangeTubeGeothermalPowerCompanyOwnership */
void ChangeTubeGeothermalPowerCompanyOwnership(Owner old_owner, [[maybe_unused]] Owner new_owner)
{
	bool changed = false;
	// Shared company lifecycle has already moved/expired original quantities.
	// Mirror each original root's actual retained owner, then require an explicit
	// fresh physical source instead of transferring an old inlet's permission.
	for (auto &source : _tube_geothermal.sources) {
		if (source.owner != old_owner) continue;
		if (source.active) { RetireTubeGeothermalSource(source); changed = true; }
		const auto *supply = GetUtilityPowerConsumer(source.supply);
		if (supply != nullptr && source.owner != supply->owner) { source.owner = supply->owner; changed = true; } // Only actual shared ownership changes update retired metadata; a reused numeric company ID cannot rewrite an unrelated tombstone or its receipt.
	}
	if (changed && _tube_geothermal.sequence < UINT64_MAX - 1) ++_tube_geothermal.sequence; // Company cleanup remains nonrefusible while reference receipts never overflow.
}

/** @copydoc CaptureTubeGeothermalPowerState */
TubeGeothermalPowerState CaptureTubeGeothermalPowerState() { return _tube_geothermal; }
/** @copydoc GetTubeGeothermalSources */
std::span<const TubeGeothermalSource> GetTubeGeothermalSources() { return _tube_geothermal.sources; }

/** @copydoc ComputeTubeGeothermalPowerDigest */
uint64_t ComputeTubeGeothermalPowerDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Explicit retained reference/history scalar. */
	const auto field = [&hash](uint64_t value) {
		for (uint8_t byte = 0; byte < 8; ++byte) { hash ^= value & 0xFF; hash *= 1099511628211ULL; value >>= 8; } // Fixed byte order excludes padding and host layout from synchronized source identity.
	};
	field(_tube_geothermal.next_source); field(_tube_geothermal.sequence); field(_tube_geothermal.sources.size());
	// Every original incarnation and tombstone participates; a coincident
	// rendered inlet or equivalent route cannot replace retained identity.
	for (const auto &source : _tube_geothermal.sources) {
		field(source.id.base()); field(source.generator.base()); field(source.producer.base()); field(source.supply.base());
		field(source.owner.base()); field(source.port.base()); field(source.inlet.base()); field(source.depot.base()); field(source.station.base()); field(source.active);
	}
	return hash;
}

/** @copydoc ResetTubeGeothermalPowerState */
void ResetTubeGeothermalPowerState()
{
	_tube_geothermal = {}; _pending_tube_geothermal.reset(); _tube_geothermal_committing = false;
	ResetTubeGeothermalPowerLoadStaging();
}

/** @copydoc ValidateTubeGeothermalPowerState */
bool ValidateTubeGeothermalPowerState(const TubeGeothermalPowerState &state, const UtilityPowerState &utility, bool world)
{
	// Retained dense history is the issuer's authority. Omitted tombstones or
	// a wrapped receipt could otherwise let a replacement inherit an old inlet.
	if (state.sources.size() > MAX_TUBE_GEOTHERMAL_SOURCES || state.next_source != state.sources.size() ||
		state.sequence < state.sources.size() || state.sequence == UINT64_MAX || (state.sources.empty() && state.sequence != 0)) return false;
	std::set<UtilityConsumerID> supplies;
	std::set<std::pair<IndustryID, TileIndex>> active_inlets;
	// Validate every original incarnation, including retired roots. Retirement
	// preserves attribution but never requires vanished/reused native machinery.
	for (size_t index = 0; index < state.sources.size(); ++index) {
		const auto &source = state.sources[index];
		// Retired addresses remain original Deep history even though their
		// native facilities can be gone; map resize cannot silently relocate them.
		if (world && (!IsValidTile(source.port) || !IsValidTile(source.inlet) || GetTileLayer(source.port) != MapLayer::DeepUnderground ||
			GetTileLayer(source.inlet) != MapLayer::DeepUnderground)) return false;
		if (source.id.base() != index || source.generator.base() >= 64000 || source.producer.base() >= utility.producers.size() ||
			source.supply.base() >= utility.consumers.size() || !supplies.insert(source.supply).second ||
			(source.owner.base() >= MAX_COMPANIES && source.owner != CompanyID::Invalid()) ||
			(source.depot == DepotID::Invalid()) == (source.station == StationID::Invalid())) return false; // Each inlet owns one distinct original root and exactly one native facility kind; a duplicate root cannot create independent physical permissions.
		const auto &producer = utility.producers[source.producer.base()];
		const auto &supply = utility.consumers[source.supply.base()];
		// The dedicated root is a shared TubeWork reservation, not imported
		// generation, another effect or a feature-local rate/energy account.
		if (producer.kind != UtilityGenerationKind::HeatConversion || producer.meter.domain != UtilityEnergyDomain::Electricity ||
			producer.meter.layer != MapLayer::DeepUnderground || supply.owner != source.owner || supply.effect != UtilityConsumerEffect::TubeWork ||
			supply.meter.domain != UtilityEnergyDomain::Electricity || supply.meter.layer != MapLayer::DeepUnderground ||
			supply.capacity_j != MAX_UTILITY_RATE_W * 27 / 1000) return false;
		// An original forwarding root cannot adopt imported energy, an unrelated
		// riser/portal role or another inlet's path, including retained old links.
		for (const auto &link : utility.links) {
			if (link.destination == source.supply) return false;
			if (link.source != source.supply) continue;
			if (link.kind != UtilityLinkKind::ElectricalFeed || link.generator != source.generator || link.source_endpoint != source.port ||
				link.destination_endpoint != source.inlet || (!source.active && link.active)) return false;
		}
		if (!source.active) {
			if (supply.active || !supply.linked_producers.empty()) return false; // Historical metadata cannot leave a usable forwarding permission; original producer ownership may legitimately have moved later.
			continue;
		}
		// Only a live same-company original conversion output can authorize a
		// live inlet. Current grants are deliberately neither required nor repaired.
		if (!producer.active || !supply.active || producer.owner != source.owner || source.owner == CompanyID::Invalid() ||
			supply.linked_producers != std::vector{source.producer} || !active_inlets.emplace(source.generator, source.inlet).second) return false;
		if (!world) continue;
		const auto *plant = GetGeothermalGeneratorState(source.generator);
		const auto *terms = plant == nullptr ? nullptr : GetMultiLayerGeothermalGeneratorTerms(plant->model);
		// Native identity/work persists through temporary cable, funds or date
		// unavailability. Operational readiness is a separate runtime query.
		if (plant == nullptr || terms == nullptr || !Company::IsValidID(source.owner) || plant->owner != source.owner ||
			plant->electrical_producer != source.producer || plant->heat_consumer != producer.conversion_input || !plant->materials_committed ||
			plant->work_ticks != GetMultiLayerGeothermalConstruction(terms->construction)->work_ticks || GetGeothermalGeneratorPort(source.generator) != source.port ||
			!IsValidTile(source.inlet) || GetTileLayer(source.inlet) != MapLayer::DeepUnderground || GetTileOwner(source.inlet) != source.owner.base() ||
			!IsTubeRailType(GetTileRailType(source.inlet))) return false;
		const auto &geology = GetGeologyWorldState();
		// Physical identity retains its excavated native floor during date locks;
		// a corrupted solid/work cell cannot acquire an inlet merely from tile art.
		if (geology.generator_version != 1 || geology.job_claims.contains(source.inlet) ||
			geology.terrain.Get(source.inlet) != ExcavationState::Excavated) return false;
		if (source.depot != DepotID::Invalid()) {
			const auto *depot = Depot::GetIfValid(source.depot);
			if (depot == nullptr || depot->xy != source.inlet || !IsRailDepotTile(source.inlet) || GetDepotIndex(source.inlet) != source.depot) return false; // Saved original native pool and physical depot must agree; a coincident replacement never repairs a source reference.
		} else {
			const auto *station = Station::GetIfValid(source.station);
			if (station == nullptr || station->owner != source.owner || !IsRailStationTile(source.inlet) ||
				GetStationIndex(source.inlet) != source.station || IsStationTileBlocked(source.inlet)) return false; // Only the original owned usable platform is the saved receiving boundary.
		}
	}
	return true;
}

/** @copydoc StageLoadedTubeGeothermalPowerState */
void StageLoadedTubeGeothermalPowerState(TubeGeothermalPowerState state) { _pending_tube_geothermal = std::move(state); }

/** @copydoc AcceptLoadedTubeGeothermalPowerState */
bool AcceptLoadedTubeGeothermalPowerState()
{
	if (!_pending_tube_geothermal.has_value()) return true;
	if (!ValidateTubeGeothermalPowerState(*_pending_tube_geothermal, CaptureUtilityPowerState())) return false;
	_tube_geothermal = std::move(*_pending_tube_geothermal); _pending_tube_geothermal.reset(); return true;
}
