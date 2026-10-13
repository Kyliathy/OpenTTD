/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_runtime.cpp Common synchronized heat/electrical interval driver and one paid actual capsule front-tick participant.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups6-7
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-f-utility-and-transfer-contracts.md canonical mandatory service units
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group2 actual bounded collector offers
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1B common Core source/service requests
 */

#include "stdafx.h"
#include "transport_tube.h"
#include "geothermal_collector.h"
#include "openttd.h"
#include "multilayer_bootstrap.h"
#include "train.h"
#include "engine_base.h"
#include "company_base.h"
#include "company_func.h"
#include "core/backup_type.hpp"
#include "timer/timer_game_tick.h"
#include "orbital_service.h"
#include "orbital_station.h"
#include <set>

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
		old->source == source || !IsTubePowerConnected(source, *train) || _tube_power_vehicles.size() >= MAX_UTILITY_INTERVAL_ACCOUNTS) return std::nullopt;
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

/** @copydoc PrepareTubePowerTick */
void PrepareTubePowerTick()
{
	if (_tube_power_executing != VehicleID::Invalid()) return;
	if (_tube_power_prepared_tick == TimerGameTick::counter) return;
	// Native simulation owns both domains. Pausing or an editor view cannot
	// earn heat; a completed collector also runs when no capsules exist.
	if (_game_mode != GameMode::Normal || _pause_mode.Any()) return;
	PrepareOrbitalConstructionTick(); // Paid portable Core tooling also receives one native opportunity when no utility source exists yet.
	// Completed Core panels also need this single synchronized publication/allocation phase when no capsule or collector exists.
	if (std::ranges::none_of(_tube_power_vehicles, &TubePowerVehicle::active) && !HasGeothermalHeatSources() && !HasOrbitalUtilitySources()) return;
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		if (IsUtilityPowerAllocated() && BeginUtilityPowerInterval() != UtilityPowerError::None) return;
	}
	// A refused epoch rotation must not label an older grant as a newly prepared simulation opportunity for any consumer.
	_tube_power_prepared_tick = TimerGameTick::counter;
	const uint64_t interval = GetUtilityPowerInterval();
	PublishGeothermalHeatSources(interval, 1);
	PublishOrbitalUtilitySources(interval, 1);
	// Publish each installed service once before any company's request is
	// allocated. Availability/funds failures provide no source for this tick.
	for (const auto &connection : CaptureTubePowerConnections()) {
		if (!connection.active) continue;
		if (!Company::IsValidID(connection.owner)) continue;
		AutoRestoreBackup actor(_current_company, connection.owner);
		PublishTubeExternalPower(connection.producer, interval, 1);
	}
	std::vector<UtilityPowerRequest> requests;
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
		if (!IsTubePowerConnected(binding.source, *train)) { binding.wait = TubePowerWait::Topology; continue; }
		binding.phase = GetTubePowerPhase(*train);
		const auto demand = ComputeTubePowerDemand(*train, binding.phase, 1, binding.remainder_mj);
		if (!demand.has_value()) continue;
		binding.wait = TubePowerWait::None;
		if (demand->energy_j != 0) requests.push_back({binding.consumer, demand->energy_j});
	}
	AppendOrbitalUtilityRequests(requests);
	AppendOrbitalConstructionRequests(requests);
	AutoRestoreBackup system(_current_company, OWNER_DEITY);
	// Physical thermal requests arrive through the upcoming connected-network
	// owner. Until then close an empty thermal pass, curtailing unused offers
	// through the shared ledger without inventing a proximity consumer or grant.
	if (HasUtilityHeatState()) AllocateUtilityHeat(interval, {});
	AllocateUtilityPower(interval, requests);
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
	if (!IsTubePowerConnected(binding->source, train)) { binding->wait = TubePowerWait::Topology; return false; }
	const auto demand = ComputeTubePowerDemand(train, binding->phase, 1, binding->remainder_mj);
	if (!demand.has_value()) { binding->wait = TubePowerWait::Unavailable; return false; }
	const auto grant = GetUtilityAllocation(binding->consumer);
	// Reading mechanical compatibility or a previous interval's grant never
	// enables propulsion; only this exact prepared opportunity can settle.
	if (!grant.has_value() || grant->granted_energy_j < demand->energy_j) { binding->wait = TubePowerWait::Grant; return false; }
	AutoRestoreBackup execution(_tube_power_executing, train.index);
	const auto result = ConsumeTubeExternalPower(binding->source, binding->consumer, demand->energy_j, participant);
	if (result.error != UtilityPowerError::None) {
		binding->wait = result.error == UtilityPowerError::InvalidTopology ? TubePowerWait::Topology : TubePowerWait::Funds;
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
	const UtilityPowerState *utility, std::span<const TubePowerConnection> connections)
{
	if (rows.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return TubePowerError::Limit;
	std::set<UtilityConsumerID> consumers;
	std::set<VehicleID> vehicles;
	// Retained binding/consumer identities, fractional carry and phase epochs
	// have one physical owner; inactive rows never authorize pool-ID reuse.
	for (const auto &row : rows) {
		if (!consumers.insert(row.consumer).second) return TubePowerError::InvalidTopology;
		if (row.active && !vehicles.insert(row.vehicle).second) return TubePowerError::InvalidTopology;
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
	if (_pending_tube_power_prepared_tick != UINT64_MAX && _pending_tube_power_prepared_tick > TimerGameTick::counter) return false;
	if (ValidateTubePowerVehicles(*_pending_tube_power_vehicles) != TubePowerError::None) return false;
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
		field(row.prepared_tick); field(row.consumed_tick); field(to_underlying(row.phase)); field(to_underlying(row.wait)); field(row.active);
	}
	return hash;
}
