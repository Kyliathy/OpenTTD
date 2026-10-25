/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_runtime.cpp Common synchronized heat/electrical interval driver and one paid actual capsule front-tick participant.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups6-7
 * @see docs/zz-rhr/upgrades/2026-10/r2mlvs1-f-utility-and-transfer-contracts.md canonical mandatory service units
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group2 actual bounded collector offers
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1B/7 common native source/service/manufacturing requests
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C materially useful processor demand shares this original single allocation.
 */

#include "stdafx.h"
#include "transport_tube.h"
#include "transport_tube_geothermal.h"
#include "utility_power_transfer.h"
#include "geothermal_collector.h"
#include "geothermal_generator.h"
#include "geothermal_heat.h"
#include "openttd.h"
#include "multilayer_bootstrap.h"
#include "train.h"
#include "engine_base.h"
#include "company_base.h"
#include "company_func.h"
#include "core/backup_type.hpp"
#include "timer/timer_game_tick.h"
#include "orbital_service.h"
#include "orbital_recipe.h"
#include "orbital_station.h"
#include "orbital_manufacturing.h"
#include <set>
#include <map>

#include "safeguards.h"

static std::vector<TubePowerVehicle> _tube_power_vehicles; ///< Physical consumer/vehicle bindings, never another electrical or packet owner.
static std::optional<std::vector<TubePowerVehicle>> _pending_tube_power_vehicles; ///< Unaccepted save transport, inaccessible to native ticks.
static uint64_t _tube_power_prepared_tick = UINT64_MAX; ///< Saved once-only global request tick, separate from the common ledger's interval identity.
static uint64_t _pending_tube_power_prepared_tick = UINT64_MAX; ///< Unpublished saved request tick.
static VehicleID _tube_power_executing = VehicleID::Invalid(); ///< Transient whole-native-work authorization, never saved or inferred from a sprite cache.

/** @param vehicle Actual front engine. @return Mutable active physical binding, or nullptr without creating a consumer. */
static TubePowerVehicle *FindTubePowerVehicle(VehicleID vehicle)
{
	// Only retained registered bindings are scanned, with the shared4096 cap;
	// a reused vehicle pool ID cannot revive a retired consumer association.
	for (auto &binding : _tube_power_vehicles) {
		if (binding.active && binding.vehicle == vehicle) return &binding;
	}
	return nullptr;
}

/** @copydoc GetTubePowerVehicle */
const TubePowerVehicle *GetTubePowerVehicle(VehicleID vehicle) { return FindTubePowerVehicle(vehicle); }
/** @copydoc CaptureTubePowerVehicles */
std::vector<TubePowerVehicle> CaptureTubePowerVehicles() { return _tube_power_vehicles; }
/** @copydoc GetTubePowerPreparedTick */
uint64_t GetTubePowerPreparedTick() { return _tube_power_prepared_tick; }

/** @copydoc GetTubePowerPhase */
TubePowerPhase GetTubePowerPhase(const Train &train)
{
	if (train.vehstatus.Test(VehState::Crashed)) return TubePowerPhase::Idle;
	// A manually stopped capsule requests no motion. Actual damaged depot
	// service is a separate mandatory20-kW opportunity, independent of its stop.
	if (train.IsInDepot() && train.reliability < train.GetEngine()->reliability) return TubePowerPhase::DepotService;
	if (train.current_order.IsType(OT_LOADING)) return TubePowerPhase::Idle;
	// An on-track blocked capsule needs no motoring energy while it retries
	// reservations. Depot departure setup remains powered native work: creating
	// an ownerless hidden-depot path outside that whole work action would strand it.
	if (train.flags.Test(VehicleRailFlag::Stuck) && train.force_proceed == TFP_NONE && !train.IsInDepot()) return TubePowerPhase::Idle;
	if (train.vehstatus.Test(VehState::Stopped) && train.cur_speed == 0) return TubePowerPhase::Idle;
	return TubePowerPhase::Motion;
}

/** @copydoc ComputeTubePowerDemand */
std::optional<TubePowerDemand> ComputeTubePowerDemand(const Train &train, TubePowerPhase phase, uint32_t ticks, uint16_t remainder)
{
	// Checked opportunity/fraction bounds precede all integration. An invalid
	// phase or non-front input cannot impersonate a separate paid work owner.
	if (to_underlying(phase) > to_underlying(TubePowerPhase::DepotService) || ticks == 0 || ticks > MAX_UTILITY_INTERVAL_TICKS ||
		remainder >= 1000 || !train.IsFrontEngine()) return std::nullopt;
	if (!CanTubeTrainEnterTile(train, train.tile)) return std::nullopt;
	uint64_t rate = 0;
	const auto models = GetMultiLayerTubeEnergyModels();
	const auto effects = GetUtilityConsumerEffects();
	const auto service = std::ranges::find(effects, UtilityConsumerEffect::TubeWork, &UtilityConsumerEffectContract::effect);
	if (service == effects.end()) return std::nullopt;
	// Every actual capsule contributes its own physical load and motoring or
	// service demand; broad cached masks cannot hide a mixed/foreign trailing part.
	for (const Train *part = &train; part != nullptr; part = part->Next()) {
		if (part->owner != train.owner) return std::nullopt; // Another company's trailing machinery cannot consume this front's explicit paid reservation.
		const auto capability = GetTubeVehicleCapability(part->engine_type);
		if (!capability.has_value()) return std::nullopt;
		const auto model = std::ranges::find(models, capability->catalogue->model_id, &MultiLayerTubeEnergyModel::model_id);
		if (model == models.end()) return std::nullopt;
		const uint64_t load = part->cargo.StoredCount(), capacity = part->cargo_cap;
		// A damaged/incompatible cargo envelope cannot reduce the charge by
		// wrapping arithmetic or assigning an impossible full-load denominator.
		if (load > capacity || capacity == 0 || model->empty_motion_w > model->full_motion_w) return std::nullopt;
		uint64_t part_rate = 0;
		if (phase == TubePowerPhase::Motion) part_rate = model->empty_motion_w + (model->full_motion_w - model->empty_motion_w) * load / capacity;
		if (phase == TubePowerPhase::DepotService) part_rate = service->demand_rate_w;
		if (part_rate > MAX_UTILITY_RATE_W - rate) return std::nullopt;
		rate += part_rate;
	}
	if (phase == TubePowerPhase::Idle) return TubePowerDemand{0, remainder, 0};
	const auto integrated = IntegrateUtilityGeneration(rate, ticks, remainder);
	if (!integrated.has_value()) return std::nullopt;
	return TubePowerDemand{integrated->energy_j, integrated->remainder_mj, rate};
}

/** @copydoc BindTubePowerVehicle */
std::optional<UtilityConsumerID> BindTubePowerVehicle(VehicleID vehicle, UtilityProducerID source, bool execute)
{
	if (_tube_power_executing != VehicleID::Invalid()) return std::nullopt;
	Train *train = Train::GetIfValid(vehicle);
	const auto *connection = GetTubePowerConnection(source);
	// Actual stopped interchange custody provides the explicit binding. A
	// source ID or nearby depot cannot power a foreign, moving or absent capsule.
	if (train == nullptr || connection == nullptr || !connection->active || train->owner != _current_company ||
		connection->owner != train->owner || !train->IsFrontEngine() || !train->IsStoppedInDepot() ||
		train->tile != connection->interchange || !CanTubeTrainEnterTile(*train, train->tile) || FindTubePowerVehicle(vehicle) != nullptr) return std::nullopt;
	if (_tube_power_vehicles.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS) return std::nullopt;
	if (ValidateTubePowerConnections(std::array{*connection}, GetCurrentMultiLayerProfileSelection()) != TubePowerError::None) return std::nullopt;
	if (!ComputeTubePowerDemand(*train, TubePowerPhase::Motion).has_value()) return std::nullopt;
	const std::array sources{source};
	static constexpr uint64_t capacity = MAX_UTILITY_RATE_W * 27 / 1000; // Checked whole-consist maximum for one canonical simulation tick.
	const auto consumer = RegisterUtilityPowerConsumer(train->owner, GetTileLayer(train->tile), UtilityConsumerEffect::TubeWork, capacity, sources, false);
	if (!consumer.has_value()) return std::nullopt;
	if (execute) {
		_tube_power_vehicles.reserve(_tube_power_vehicles.size() + 1); // Allocate physical transport before publishing the ledger ID.
		if (RegisterUtilityPowerConsumer(train->owner, GetTileLayer(train->tile), UtilityConsumerEffect::TubeWork, capacity, sources) != consumer) return std::nullopt;
		TubePowerVehicle binding; binding.consumer = *consumer; binding.source = source; binding.vehicle = vehicle;
		_tube_power_vehicles.push_back(binding);
	}
	return consumer;
}

/** @copydoc BindTubeGeothermalPowerVehicle */
std::optional<UtilityConsumerID> BindTubeGeothermalPowerVehicle(VehicleID vehicle, TubeGeothermalSourceID source_id, bool execute)
{
	if (_tube_power_executing != VehicleID::Invalid()) return std::nullopt;
	Train *train = Train::GetIfValid(vehicle); const auto *source = GetTubeGeothermalSource(source_id);
	// Explicit stopped native custody, original source ownership and complete
	// physical component admission precede both shared recipient identities.
	if (train == nullptr || source == nullptr || !source->active || train->owner != _current_company || source->owner != train->owner ||
		!train->IsFrontEngine() || !train->IsStoppedInDepot() || train->vehstatus.Test(VehState::Crashed) ||
		FindTubePowerVehicle(vehicle) != nullptr || _tube_power_vehicles.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS ||
		!IsTubeGeothermalPowerConnected(source_id, *train)) return std::nullopt;
	const auto terminal = GetTubeTerminal(train->tile, train->owner);
	if (!terminal.has_value() || terminal->depot == DepotID::Invalid()) return std::nullopt; // Hidden/native flags alone cannot impersonate an actual owned capsule purchase depot.
	if (!ComputeTubePowerDemand(*train, TubePowerPhase::Motion).has_value()) return std::nullopt;
	static constexpr uint64_t capacity = MAX_UTILITY_RATE_W * 27 / 1000;
	const auto quote = RegisterGeothermalLocalFeedRecipient(source->generator, source->supply, source->inlet, capacity, false);
	if (!quote.has_value()) return std::nullopt;
	if (execute) {
		_tube_power_vehicles.reserve(_tube_power_vehicles.size() + 1); // Acquire physical row storage before the sole shared owner publishes the complete consumer/feed pair.
		const auto actual = RegisterGeothermalLocalFeedRecipient(source->generator, source->supply, source->inlet, capacity);
		if (!actual.has_value()) return std::nullopt;
		assert(actual->consumer == quote->consumer && actual->feed == quote->feed);
		TubePowerVehicle binding; binding.consumer = actual->consumer; binding.source = source->producer; binding.vehicle = vehicle;
		binding.geothermal_source = source_id; binding.local_feed = actual->feed; binding.wait = TubePowerWait::Unavailable;
		_tube_power_vehicles.push_back(binding);
	}
	return quote->consumer;
}

/**
 * Ensure switching can finish the original external grant release after acquiring one replacement recipient.
 * @param binding Original native association being replaced, with its current provider and unused grant.
 * @return Whether whole retirement is available without exhausting the shared receipt sequence; no owner changes here.
 */
static bool CanRetireTubePowerVehicleForSwitch(const TubePowerVehicle &binding)
{
	if (binding.geothermal_source.base() != UINT32_MAX) return true; // Original local-feed retirement is nonrefusible, including at the terminal receipt boundary.
	const auto grant = GetUtilityAllocation(binding.consumer);
	if (!grant.has_value() || grant->granted_energy_j == 0) return true; // An absent or exhausted original offer has no remaining rights to release.
	const auto release = PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {binding.consumer, grant->accounting_interval}, grant->granted_energy_j);
	// Fresh registration and the subsequent exact release each need one
	// shared receipt. Refuse before acquisition if either step cannot finish.
	return release.error == UtilityPowerError::None && release.sequence_before < UINT64_MAX - 2;
}

/** @copydoc ReconnectTubeGeothermalPowerVehicle */
std::optional<UtilityConsumerID> ReconnectTubeGeothermalPowerVehicle(VehicleID vehicle, TubeGeothermalSourceID source_id, bool execute)
{
	if (_tube_power_executing != VehicleID::Invalid()) return std::nullopt;
	Train *train = Train::GetIfValid(vehicle); const auto *old = FindTubePowerVehicle(vehicle); const auto *source = GetTubeGeothermalSource(source_id);
	// Switching must preserve actual stopped custody and reach the newly
	// selected original inlet. A retired source, foreign vehicle or duplicate
	// choice cannot acquire another allowance or revive an earlier grant.
	if (train == nullptr || old == nullptr || source == nullptr || !source->active || train->owner != _current_company ||
		source->owner != train->owner || !train->IsFrontEngine() || train->cur_speed != 0 || train->vehstatus.Test(VehState::Crashed) ||
		old->geothermal_source == source_id || _tube_power_vehicles.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS ||
		!IsTubeGeothermalPowerConnected(source_id, *train) || !CanRetireTubePowerVehicleForSwitch(*old)) return std::nullopt;
	if (!ComputeTubePowerDemand(*train, TubePowerPhase::Motion, 1, old->remainder_mj).has_value()) return std::nullopt;
	static constexpr uint64_t capacity = MAX_UTILITY_RATE_W * 27 / 1000;
	const auto quote = RegisterGeothermalLocalFeedRecipient(source->generator, source->supply, source->inlet, capacity, false);
	if (!quote.has_value()) return std::nullopt;
	if (execute) {
		const uint16_t carry = old->remainder_mj; const UtilityConsumerID original = old->consumer;
		_tube_power_vehicles.reserve(_tube_power_vehicles.size() + 1); // Acquire fallible binding storage before changing either shared recipient or retiring the original.
		const auto actual = RegisterGeothermalLocalFeedRecipient(source->generator, source->supply, source->inlet, capacity);
		if (!actual.has_value()) return std::nullopt;
		assert(actual->consumer == quote->consumer && actual->feed == quote->feed);
		RetireTubePowerVehicle(vehicle);
		const auto retired = std::ranges::find(_tube_power_vehicles, original, &TubePowerVehicle::consumer);
		retired->remainder_mj = 0; // The historical binding relinquishes its numerator; only the new original recipient carries it forward.
		TubePowerVehicle binding; binding.consumer = actual->consumer; binding.source = source->producer; binding.vehicle = vehicle;
		binding.remainder_mj = carry; binding.geothermal_source = source_id; binding.local_feed = actual->feed;
		_tube_power_vehicles.push_back(binding);
	}
	return quote->consumer;
}

/** @copydoc ReconnectTubePowerVehicle */
std::optional<UtilityConsumerID> ReconnectTubePowerVehicle(VehicleID vehicle, UtilityProducerID source, bool execute)
{
	if (_tube_power_executing != VehicleID::Invalid()) return std::nullopt;
	Train *train = Train::GetIfValid(vehicle);
	auto *old = FindTubePowerVehicle(vehicle);
	const auto *connection = GetTubePowerConnection(source);
	// Actual stopped custody, current owner and explicit connected replacement
	// precede identity acquisition; a query or a reused source cannot resume
	// another company's train or provide a second copy of its old reservation.
	if (train == nullptr || old == nullptr || connection == nullptr || train->owner != _current_company || train->cur_speed != 0 ||
		train->vehstatus.Test(VehState::Crashed) || old->source == source || !IsTubePowerConnected(source, *train) ||
		_tube_power_vehicles.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS || !CanRetireTubePowerVehicleForSwitch(*old)) return std::nullopt;
	if (!ComputeTubePowerDemand(*train, TubePowerPhase::Motion, 1, old->remainder_mj).has_value()) return std::nullopt;
	if (ValidateTubePowerConnections(std::array{*connection}, GetCurrentMultiLayerProfileSelection()) != TubePowerError::None) return std::nullopt;
	const std::array sources{source};
	static constexpr uint64_t capacity = MAX_UTILITY_RATE_W * 27 / 1000;
	const auto consumer = RegisterUtilityPowerConsumer(train->owner, GetTileLayer(train->tile), UtilityConsumerEffect::TubeWork, capacity, sources, false);
	if (!consumer.has_value()) return std::nullopt;
	if (execute) {
		const uint16_t carry = old->remainder_mj;
		const UtilityConsumerID original = old->consumer;
		_tube_power_vehicles.reserve(_tube_power_vehicles.size() + 1); // Finish fallible storage acquisition before either physical binding changes.
		if (RegisterUtilityPowerConsumer(train->owner, GetTileLayer(train->tile), UtilityConsumerEffect::TubeWork, capacity, sources) != consumer) return std::nullopt;
		RetireTubePowerVehicle(vehicle);
		// The historical row relinquishes its active demand numerator. Only
		// the new physical association carries it into future admitted work.
		const auto retired = std::ranges::find(_tube_power_vehicles, original, &TubePowerVehicle::consumer);
		retired->remainder_mj = 0;
		TubePowerVehicle binding; binding.consumer = *consumer; binding.source = source; binding.vehicle = vehicle; binding.remainder_mj = carry;
		_tube_power_vehicles.push_back(binding);
	}
	return consumer;
}

/**
 * Resolve the original provider's actual physical admission without creating a substitute source.
 * @param binding Saved original capsule/provider association.
 * @param train Actual owned capsule front and its complete consist.
 * @return Whether the original source, purchased electrical path and owned guideway still connect this capsule.
 */
static bool IsTubePowerBindingConnected(const TubePowerVehicle &binding, const Train &train)
{
	return binding.geothermal_source.base() == UINT32_MAX ? IsTubePowerConnected(binding.source, train) :
		IsTubeGeothermalPowerConnected(binding.geothermal_source, train);
}

/** @copydoc PrepareTubePowerTick */
void PrepareTubePowerTick(std::span<const GeothermalHeatDemand> heat_demands)
{
	if (_tube_power_executing != VehicleID::Invalid()) return;
	if (_tube_power_prepared_tick == TimerGameTick::counter) return;
	// Native simulation owns both domains. Pausing or an editor view cannot
	// earn heat; a completed collector also runs when no capsules exist.
	if (_game_mode != GameMode::Normal || _pause_mode.Any()) return;
	PrepareOrbitalConstructionTick(); // Paid portable Core tooling also receives one native opportunity when no utility source exists yet.
	// Completed Core panels also need this single synchronized publication/allocation phase when no capsule or collector exists.
	// Retained plants and physical town subscriptions still require ordinary
	// interval expiry/recovery after source loss, independently of capsule presence.
	if (std::ranges::none_of(_tube_power_vehicles, &TubePowerVehicle::active) && !HasGeothermalHeatSources() && !HasGeothermalGenerators() && !HasOrbitalUtilitySources() && !HasUtilityTownServices()) return;
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		if (IsUtilityPowerAllocated() && BeginUtilityPowerInterval() != UtilityPowerError::None) return;
	}
	// A refused epoch rotation must not label an older grant as a newly prepared simulation opportunity for any consumer.
	_tube_power_prepared_tick = TimerGameTick::counter;
	const uint64_t interval = GetUtilityPowerInterval();
	PublishGeothermalHeatSources(interval, 1);
	PublishOrbitalUtilitySources(interval, 1);
	// The original generator owner closes Heat once and commits native service
	// and conversion before any electrical reservation can use its output.
	RunGeothermalGenerators(interval, 1, heat_demands);
	// Publish each installed service once before any company's request is
	// allocated. Availability/funds failures provide no source for this tick.
	for (const auto &connection : CaptureTubePowerConnections()) {
		if (!connection.active) continue;
		if (!Company::IsValidID(connection.owner)) continue;
		AutoRestoreBackup actor(_current_company, connection.owner);
		PublishTubeExternalPower(connection.producer, interval, 1);
	}
	std::vector<UtilityPowerRequest> requests;
	std::map<UtilityConsumerID, uint64_t> geothermal_requests;
	std::vector<UtilityPathTransferRequest> geothermal_imports;
	// All registered capsule companies share one stable ledger allocation.
	// Request gathering precedes every native front tick and changes no physical
	// load, progress, demand carry or player stop flag.
	for (auto &binding : _tube_power_vehicles) {
		if (!binding.active) continue;
		Train *train = Train::GetIfValid(binding.vehicle);
		binding.prepared_tick = TimerGameTick::counter;
		binding.phase = TubePowerPhase::Idle; binding.wait = TubePowerWait::Unavailable;
		if (train == nullptr) continue;
		if (!CanTubeTrainEnterTile(*train, train->tile)) continue;
		if (!IsTubePowerBindingConnected(binding, *train)) { binding.wait = TubePowerWait::Topology; continue; }
		binding.phase = GetTubePowerPhase(*train);
		const auto demand = ComputeTubePowerDemand(*train, binding.phase, 1, binding.remainder_mj);
		if (!demand.has_value()) continue;
		binding.wait = TubePowerWait::None;
		if (demand->energy_j == 0) continue;
		if (binding.geothermal_source.base() != UINT32_MAX) {
			const auto *source = GetTubeGeothermalSource(binding.geothermal_source);
			const auto path = GetTubeGeothermalSourcePath(binding.geothermal_source);
			const auto *supply = source == nullptr ? nullptr : GetUtilityPowerConsumer(source->supply);
			// Only the original live forwarding reservation may request native
			// output. Every capsule also registers its real unmet demand so the
			// common owner can import original slices through its exact feed.
			if (supply == nullptr || !supply->active || !path.has_value()) { binding.wait = TubePowerWait::Topology; continue; }
			auto &total = geothermal_requests[source->supply];
			total += std::min(demand->energy_j, supply->capacity_j - total);
			geothermal_imports.push_back({{source->supply, interval}, binding.consumer, source->port, demand->energy_j,
				{{binding.local_feed, path->cells}}});
		}
		requests.push_back({binding.consumer, demand->energy_j});
	}
	// Each original root competes once for the sum of its actual capsule
	// requests, capped at its existing shared admission ceiling. Neither a
	// second inlet nor a recipient creates another generation allowance.
	for (const auto &[supply, demand] : geothermal_requests) requests.push_back({supply, demand});
	AppendUtilityTownServiceRequests(requests);
	AppendOrbitalUtilityRequests(requests);
	AppendOrbitalConstructionRequests(requests);
	AppendOrbitalManufacturingRequests(requests); // A real queued Assembly competes in the same one allocation; no private grant or idle demand.
	AppendOrbitalRecipeRequests(requests); // Material-feasible processors compete in this same single pass; paid/full/idle buffers demand no fictitious grant.
	AutoRestoreBackup system(_current_company, OWNER_DEITY);
	// The physical geothermal owner has already closed Heat, including an
	// empty feasible set. Other electrical sources still share this one pass.
	if (AllocateUtilityPower(interval, requests) != UtilityPowerError::None) return;
	// Original consumer identity fixes import priority even after a valid
	// native load presents binding rows in a different transport order.
	std::ranges::sort(geothermal_imports, {}, &UtilityPathTransferRequest::destination);
	// Whole transfers use shared
	// parent slices and aggregate purchased-cell residuals; a short source or
	// cable cannot partially fund a native tick or renew a spent path allowance.
	for (const auto &request : geothermal_imports) {
		const auto *recipient = GetUtilityPowerConsumer(request.destination);
		AutoRestoreBackup actor(_current_company, recipient->owner);
		const auto proposal = PrepareUtilityPathTransfer(request);
		if (proposal.error != UtilityPowerError::None) continue;
		CommitUtilityPathTransfer(proposal);
	}
	RunUtilityTownServices(); // Export and the real town effect settle once after all companies share the same allocation.
}

/** @copydoc IsTubePowerExecutionAuthorized */
bool IsTubePowerExecutionAuthorized(const Train &train) { return _tube_power_executing == train.First()->index; }

/** @copydoc AdvanceTubePowerTick */
bool AdvanceTubePowerTick(Train &train, UtilityMutationParticipant &participant)
{
	auto *binding = FindTubePowerVehicle(train.index);
	if (binding == nullptr) return false;
	// A repeated native invocation, stale allocation or a changed physical
	// phase cannot reuse an earlier tick's energy or clear a manual stop.
	if (binding->prepared_tick != TimerGameTick::counter || binding->consumed_tick == TimerGameTick::counter || binding->wait != TubePowerWait::None ||
		binding->phase != GetTubePowerPhase(train)) return false;
	if (binding->phase == TubePowerPhase::Idle) return false;
	if (!IsTubePowerBindingConnected(*binding, train)) { binding->wait = TubePowerWait::Topology; return false; }
	const auto demand = ComputeTubePowerDemand(train, binding->phase, 1, binding->remainder_mj);
	if (!demand.has_value()) { binding->wait = TubePowerWait::Unavailable; return false; }
	const auto grant = GetUtilityAllocation(binding->consumer);
	// Reading mechanical compatibility or a previous interval's grant never
	// enables propulsion; only this exact prepared opportunity can settle.
	if (!grant.has_value() || grant->granted_energy_j < demand->energy_j) { binding->wait = TubePowerWait::Grant; return false; }
	AutoRestoreBackup execution(_tube_power_executing, train.index);
	const bool geothermal = binding->geothermal_source.base() != UINT32_MAX;
	// Native conversion has already paid its real operating service. Its
	// original imported slice settles only the complete native work participant;
	// leased external supply retains its separate existing tariff participant.
	const auto result = geothermal ? CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::Consume,
		{binding->consumer, grant->accounting_interval}, demand->energy_j), &participant) :
		ConsumeTubeExternalPower(binding->source, binding->consumer, demand->energy_j, participant);
	if (result.error != UtilityPowerError::None) {
		binding->wait = result.error == UtilityPowerError::InvalidTopology ? TubePowerWait::Topology :
			geothermal ? TubePowerWait::Unavailable : TubePowerWait::Funds;
		return false;
	}
	binding->consumed_tick = TimerGameTick::counter; binding->remainder_mj = demand->remainder_mj; binding->wait = TubePowerWait::None;
	return true;
}

/** @copydoc RetireTubePowerVehicle */
void RetireTubePowerVehicle(VehicleID vehicle)
{
	auto *binding = FindTubePowerVehicle(vehicle);
	if (binding == nullptr) return;
	if (binding->geothermal_source.base() != UINT32_MAX) {
		InvalidateUtilityLocalFeedRecipient(binding->consumer, binding->local_feed);
		binding->active = false; binding->wait = TubePowerWait::NoConnection; return;
	}
	const auto *consumer = GetUtilityPowerConsumer(binding->consumer);
	if (consumer != nullptr) {
		AutoRestoreBackup actor(_current_company, consumer->owner);
		const auto grant = GetUtilityAllocation(binding->consumer);
		if (grant.has_value() && grant->granted_energy_j != 0) {
			const auto release = PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {binding->consumer, grant->accounting_interval}, grant->granted_energy_j);
			CommitUtilityMutation(release);
		}
	}
	binding->active = false; binding->wait = TubePowerWait::NoConnection;
}

/** @copydoc ValidateTubePowerVehicles */
TubePowerError ValidateTubePowerVehicles(std::span<const TubePowerVehicle> rows, bool world_refs,
	const UtilityPowerState *utility, std::span<const TubePowerConnection> connections, const TubeGeothermalPowerState *geothermal)
{
	if (rows.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return TubePowerError::Limit;
	std::set<UtilityConsumerID> consumers;
	std::set<VehicleID> vehicles;
	std::set<UtilityLinkID> feeds;
	// Retained binding/consumer identities, fractional carry and phase epochs
	// have one physical owner; inactive rows never authorize pool-ID reuse.
	for (const auto &row : rows) {
		if (!consumers.insert(row.consumer).second) return TubePowerError::InvalidTopology;
		if (row.active && !vehicles.insert(row.vehicle).second) return TubePowerError::InvalidTopology;
		const bool native_source = row.geothermal_source.base() != UINT32_MAX;
		if (native_source != (row.local_feed.base() != UINT32_MAX)) return TubePowerError::InvalidTopology; // A provider incarnation and its original terminal feed travel together; defaults identify only genuine external service.
		if (native_source && !feeds.insert(row.local_feed).second) return TubePowerError::InvalidTopology; // One original recipient feed cannot be adopted by another retained capsule binding.
		// Invalid scalar identities or future/contradictory consumed epochs
		// cannot fabricate an accepted power opportunity during loading/preview.
		if (row.consumer.base() == UINT32_MAX || row.source.base() == UINT32_MAX || row.vehicle == VehicleID::Invalid() ||
			row.remainder_mj >= 1000 || to_underlying(row.phase) > to_underlying(TubePowerPhase::DepotService) ||
			to_underlying(row.wait) > to_underlying(TubePowerWait::Unavailable) ||
			(row.consumed_tick != UINT64_MAX && (row.prepared_tick == UINT64_MAX || row.consumed_tick > row.prepared_tick))) return TubePowerError::InvalidTopology;
		if (!world_refs && utility == nullptr) continue; // Basic staging may precede complete electrical decoding; complete preview supplies both copied owners.
		// Actual work cannot arrive from a future simulation clock; a hostile
		// save must not gain a reusable prepaid opportunity after loading.
		if (world_refs && row.prepared_tick != UINT64_MAX && row.prepared_tick > TimerGameTick::counter) return TubePowerError::InvalidTopology;
		const auto *consumer = world_refs ? GetUtilityPowerConsumer(row.consumer) :
			row.consumer.base() < utility->consumers.size() ? &utility->consumers[row.consumer.base()] : nullptr;
		if (native_source) {
			const auto source = world_refs ? GetTubeGeothermalSource(row.geothermal_source) :
				geothermal != nullptr && row.geothermal_source.base() < geothermal->sources.size() ? &geothermal->sources[row.geothermal_source.base()] : nullptr;
			const auto *feed = world_refs ? GetUtilityPowerLink(row.local_feed) :
				row.local_feed.base() < utility->links.size() ? &utility->links[row.local_feed.base()] : nullptr;
			// Original metadata, shared terminal role and actual request must agree.
			// Source/feed loss retains historical references and a powerless live
			// capsule; load never replaces those records or reactivates old rights.
			if (source == nullptr || consumer == nullptr || feed == nullptr || source->producer != row.source || feed->kind != UtilityLinkKind::ElectricalFeed ||
				feed->source != source->supply || feed->destination != row.consumer || feed->generator != source->generator ||
				feed->source_endpoint != source->port || feed->destination_endpoint != source->inlet || consumer->effect != UtilityConsumerEffect::TubeWork ||
				consumer->meter.layer != MapLayer::DeepUnderground || consumer->meter.domain != UtilityEnergyDomain::Electricity ||
				consumer->capacity_j != MAX_UTILITY_RATE_W * 27 / 1000 || !consumer->linked_producers.empty() ||
				consumer->active != row.active || (row.active && consumer->owner != source->owner) || (!row.active && feed->active)) return TubePowerError::InvalidTopology;
			const auto links = world_refs ? GetUtilityPowerLinks() : std::span<const UtilityPowerLink>{utility->links};
			// A typed work recipient owns exactly its original terminal feed and
			// no re-export role; another reference cannot inject an alternative offer.
			for (const auto &other : links) {
				if (other.kind == UtilityLinkKind::LocalCell) continue;
				if (other.source == row.consumer || (other.destination == row.consumer && other.id != row.local_feed)) return TubePowerError::InvalidTopology;
			}
			if (!world_refs || !row.active) continue;
			const auto *train = Train::GetIfValid(row.vehicle);
			if (train == nullptr || !train->IsFrontEngine() || train->owner != consumer->owner || !CanTubeTrainEnterTile(*train, train->tile) ||
				!ComputeTubePowerDemand(*train, TubePowerPhase::Motion, 1, row.remainder_mj).has_value()) return TubePowerError::InvalidTopology; // Actual saved capsule/cargo capability is structural; temporary source/path/date readiness does not rewrite original binding identity.
			continue;
		}
		const auto found = std::ranges::find(connections, row.source, &TubePowerConnection::producer);
		const auto *connection = world_refs ? GetTubePowerConnection(row.source) : found == connections.end() ? nullptr : &*found;
		// Every retained consumer originated from the exact paid service;
		// retired physical rows keep their historical lineage after deletion.
		if (consumer == nullptr || connection == nullptr || (row.active && consumer->owner != connection->owner) ||
			consumer->effect != UtilityConsumerEffect::TubeWork || (row.active && !consumer->active)) return TubePowerError::InvalidTopology;
		// The common ledger removes retired sources from eligibility. A saved
		// waiting binding keeps its original physical source, with no usable
		// link; requiring that expired link would make legitimate loss saves unloadable.
		if (connection->active) {
			if (consumer->linked_producers.size() != 1 || consumer->linked_producers.front() != row.source) return TubePowerError::InvalidTopology;
		} else {
			if (!consumer->linked_producers.empty()) return TubePowerError::InvalidTopology;
		}
		if (!world_refs || !row.active) continue;
		const Train *train = Train::GetIfValid(row.vehicle);
		if (train == nullptr) return TubePowerError::InvalidTopology;
		if (!train->IsFrontEngine() || train->owner != consumer->owner || !CanTubeTrainEnterTile(*train, train->tile)) return TubePowerError::InvalidTopology;
		if (!ComputeTubePowerDemand(*train, TubePowerPhase::Motion, 1, row.remainder_mj).has_value()) return TubePowerError::InvalidTopology;
	}
	return TubePowerError::None;
}

/** @copydoc StageLoadedTubePowerVehicles */
void StageLoadedTubePowerVehicles(std::vector<TubePowerVehicle> rows, uint64_t tick)
{
	_pending_tube_power_vehicles = std::move(rows); _pending_tube_power_prepared_tick = tick;
}
/** @copydoc AcceptLoadedTubePowerVehicles */
bool AcceptLoadedTubePowerVehicles()
{
	if (!_pending_tube_power_vehicles.has_value()) return true;
	// A saved future request cannot authorize native work before its real
	// simulation opportunity; the initial sentinel carries no prepared grant.
	if (_pending_tube_power_prepared_tick != UINT64_MAX && _pending_tube_power_prepared_tick > TimerGameTick::counter) return false;
	if (ValidateTubePowerVehicles(*_pending_tube_power_vehicles) != TubePowerError::None) return false;
	// Saved transport order conveys no priority or synchronized identity.
	// New bindings already append increasing nonreused consumer IDs; normalize
	// accepted transport once so captures, digests and subsequent ticks agree.
	std::ranges::sort(*_pending_tube_power_vehicles, {}, &TubePowerVehicle::consumer);
	_tube_power_vehicles = std::move(*_pending_tube_power_vehicles); _pending_tube_power_vehicles.reset();
	_tube_power_prepared_tick = _pending_tube_power_prepared_tick; return true;
}
/** @copydoc ResetTubePowerVehicles */
void ResetTubePowerVehicles()
{
	_tube_power_vehicles.clear(); _pending_tube_power_vehicles.reset();
	_tube_power_prepared_tick = _pending_tube_power_prepared_tick = UINT64_MAX;
	_tube_power_executing = VehicleID::Invalid();
	ResetTubePowerVehicleLoadStaging();
}
/** @copydoc ComputeTubePowerVehicleDigest */
uint64_t ComputeTubePowerVehicleDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Explicit physical scalar; hash fixed bytes without padding, camera or host order. */
	const auto field = [&hash](uint64_t value) {
		for (uint byte = 0; byte < 8; ++byte) { hash ^= value & 0xFF; hash *= 1099511628211ULL; value >>= 8; }
	};
	field(_tube_power_vehicles.size());
	field(_tube_power_prepared_tick);
	// Each real binding field is included once in stable consumer identity
	// order; cargo packets and electrical quantities retain their own roots.
	for (const auto &row : _tube_power_vehicles) {
		field(row.consumer.base()); field(row.source.base()); field(row.vehicle.base()); field(row.remainder_mj);
		// Legacy external bindings keep their original digest; explicit native provider references add only their own identity fields.
		if (row.geothermal_source.base() != UINT32_MAX || row.local_feed.base() != UINT32_MAX) { field(row.geothermal_source.base()); field(row.local_feed.base()); }
		field(row.prepared_tick); field(row.consumed_tick); field(to_underlying(row.phase)); field(to_underlying(row.wait)); field(row.active);
	}
	return hash;
}
