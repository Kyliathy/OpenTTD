/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_station.cpp Delivered construction, Core activation and native inventory aging.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1-6.
 */
#include "stdafx.h"
#include "orbital_station.h"
#include "orbital_service.h"
#include "space_structure.h"
#include "space_structure_cmd.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "hub_cargo.h"
#include "company_base.h"
#include "company_func.h"
#include "command_func.h"
#include "gfx_func.h"
#include "map_func.h"
#include "core/backup_type.hpp"
#include "openttd.h"
#include "transport_tube.h"
#include "timer/timer_game_tick.h"
#include "safeguards.h"

static uint64_t construction_prepared_tick = UINT64_MAX; ///< Transient native-loop admission; never persisted as progress or energy.

/** @copydoc PrepareOrbitalConstructionTick */
void PrepareOrbitalConstructionTick()
{
	// Paused/editor callbacks cannot authorize portable or externally powered assembly work.
	if (_game_mode != GameMode::Normal || _pause_mode.Any()) return;
	construction_prepared_tick = TimerGameTick::counter;
}
/** @copydoc ResetOrbitalConstructionTick */
void ResetOrbitalConstructionTick() { construction_prepared_tick = UINT64_MAX; }

/** @copydoc GetOrbitalConstructionRemainingEnergy */
std::optional<uint64_t> GetOrbitalConstructionRemainingEnergy(const OrbitalConstructionProgress &progress, uint32_t required_ticks)
{
	const uint64_t unit = GetMultiLayerSpaceBudget().work_j_per_tick;
	// A whole-joule debit must have one exact physical representation; invalid fractions cannot manufacture a partial milestone.
	if (unit == 0 || required_ticks == 0 || progress.work_ticks > required_ticks || progress.work_remainder >= unit) return std::nullopt;
	const uint64_t current = (uint64_t{progress.work_ticks} * SPACE_Q16 + progress.work_fraction) * unit + progress.work_remainder;
	const uint64_t complete = uint64_t{required_ticks} * SPACE_Q16 * unit;
	// Refuse over-completion and sub-joule work histories; the saved division remainder belongs to progress, not another energy account.
	if (current > complete || current % SPACE_Q16 != 0) return std::nullopt;
	return (complete - current) / SPACE_Q16;
}

/** @copydoc PlanOrbitalConstructionProgress */
std::optional<OrbitalConstructionProgress> PlanOrbitalConstructionProgress(const OrbitalConstructionProgress &progress, uint32_t required_ticks, uint16_t energy_j)
{
	const auto remaining = GetOrbitalConstructionRemainingEnergy(progress, required_ticks);
	const uint64_t unit = GetMultiLayerSpaceBudget().work_j_per_tick;
	// Admit only this tick's useful positive grant; neither a partial source nor the final remainder may overpay the completed recipe.
	if (!remaining || energy_j == 0 || energy_j > unit || energy_j > *remaining) return std::nullopt;
	const uint64_t numerator = uint64_t{energy_j} * SPACE_Q16 + progress.work_remainder;
	const uint64_t fraction = progress.work_fraction + numerator / unit;
	OrbitalConstructionProgress next = progress;
	next.work_ticks += static_cast<uint32_t>(fraction / SPACE_Q16);
	next.work_fraction = static_cast<uint16_t>(fraction % SPACE_Q16);
	next.work_remainder = static_cast<uint16_t>(numerator % unit);
	return next;
}

/** @copydoc GetOrbitalConstructionCore */
SpaceStructure *GetOrbitalConstructionCore(const SpaceStructureState &state)
{
	const auto *definition = GetSpaceConstructionDefinition(state.kind);
	// Carried equipment has no static footprint and cannot acquire a station dependency through coincident coordinates.
	if (definition == nullptr || definition->width == 0 || definition->height == 0) return nullptr;
	// Native pool order makes ambiguous shared edges deterministic; inert frames and foreign facilities grant no dependency.
	for (auto *core : SpaceStructure::Iterate()) {
		if (core->state.kind != SpaceBuildKind::Core) continue;
		if (core->state.owner != state.owner) continue;
		if (!IsOrbitalModuleOperational(*core)) continue;
		const auto &footprint = *GetSpaceConstructionDefinition(core->state.kind);
		const bool horizontal = (uint{state.x} + definition->width == core->state.x || uint{core->state.x} + footprint.width == state.x) &&
			state.y < uint{core->state.y} + footprint.height && core->state.y < uint{state.y} + definition->height;
		const bool vertical = (uint{state.y} + definition->height == core->state.y || uint{core->state.y} + footprint.height == state.y) &&
			state.x < uint{core->state.x} + footprint.width && core->state.x < uint{state.x} + definition->width;
		if (horizontal || vertical) return core; // Only a shared edge joins construction functions; diagonal proximity does not.
	}
	return nullptr;
}

/** Transient transfer bounds; these counts own no material and never replace the actual packet lists. */
struct OrbitalMaterialMove {
	size_t packets = 0; ///< Conservative additional destination references; merging can only reduce this count.
	uint splits = 0; ///< At most one allocation at the requested quantity boundary.
};

/** Count a complete material move before any label leaves the Core's actual stock.
 * @param stock Sole native source container. @param quantity Required still-missing whole units, including zero.
 * @return Exact split-allocation count and conservative reference count, or absence for insufficient physical stock.
 */
static std::optional<OrbitalMaterialMove> PlanOrbitalMaterialMove(const StationCargoList &stock, uint quantity)
{
	OrbitalMaterialMove plan;
	if (quantity == 0) return plan;
	if (quantity > stock.AvailableCount()) return std::nullopt;
	// Follow the same native bucket/packet order as HubCargoTransfer so both inputs' possible splits are admitted together.
	for (const auto &[hop, packets] : *stock.Packets()) {
		(void)hop;
		for (const auto *packet : packets) {
			++plan.packets;
			if (quantity < packet->Count()) { plan.splits = 1; return plan; }
			quantity -= packet->Count();
			if (quantity == 0) return plan;
		}
	}
	return std::nullopt;
}

/** @copydoc CmdReserveOrbitalModuleInputs */
CommandCost CmdReserveOrbitalModuleInputs(DoCommandFlags flags, SpaceStructureID id, uint64_t generation, SpaceStructureID core_id, uint64_t core_generation)
{
	auto *site = ResolveSpaceStructure({id, generation}); auto *core = ResolveSpaceStructure({core_id, core_generation});
	// The paid blueprint already reserves its eventual native output identity/footprint; only its exact owner may fill it from its physical Core.
	if (site == nullptr || core == nullptr || !Company::IsValidID(_current_company) || site->state.owner != _current_company ||
		core->state.owner != _current_company || site->state.phase != SpaceStructurePhase::Blueprint || site->module != nullptr ||
		site->job.work_ticks != 0 || site->job.work_fraction != 0 || GetOrbitalConstructionCore(site->state) != core) return CMD_ERROR;
	const auto *definition = GetSpaceConstructionDefinition(site->state.kind);
	// A complete canonical static recipe and assembly duration are required before reserving any input or granting future work.
	if (definition == nullptr || definition->inputs.size() != site->job.inputs.size() || definition->work_ticks == 0 ||
		site->state.capital_paid != definition->capital || !IsSpaceStructureSiteValid(site->state, Map::SizeX(), Map::SizeY())) return CMD_ERROR;
	std::array<CargoType, 2> cargo{}; std::array<uint16_t, 2> missing{}; uint splits = 0; bool any_missing = false;
	// Preflight both labels against their real separate owners; missing either preserves every already delivered/reserved quantity.
	for (uint i = 0; i < 2; ++i) {
		cargo[i] = GetCargoTypeByLabel(definition->inputs[i].label); const auto held = site->job.inputs[i].TotalCount();
		// Resolved distinct material types and bounded saved quantities prevent substitution, double counting or subtraction overflow.
		if (!IsValidCargoType(cargo[i]) || definition->inputs[i].quantity > UINT16_MAX || held > definition->inputs[i].quantity ||
			(i != 0 && cargo[i] == cargo[0])) return CMD_ERROR;
		missing[i] = static_cast<uint16_t>(definition->inputs[i].quantity - held); any_missing |= missing[i] != 0;
		const auto plan = PlanOrbitalMaterialMove(core->module->stock[cargo[i]], missing[i]);
		if (!plan) return CMD_ERROR;
		splits += plan->splits; size_t retained = 0;
		// The native save transport has64 packet references per recipe slot; merged packets may reduce, never enlarge, the admitted move.
		for (const auto &[hop, packets] : *site->job.inputs[i].Packets()) { (void)hop; retained += packets.size(); }
		if (retained + plan->packets > 64) return CMD_ERROR;
	}
	if (!any_missing) return CMD_ERROR;
	// Check the whole batch's split capacity once; two individually possible splits cannot overbook the last free packet identity.
	if (splits != 0 && !CargoPacket::CanAllocateItem(splits)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		// No intervening owner mutation occurs after the all-input preflight; each moved packet becomes exclusive to this same typed job.
		for (uint i = 0; i < 2; ++i) {
			if (missing[i] == 0) continue;
			const bool moved = HubCargoTransfer::Move(core->module->stock[cargo[i]], site->job.inputs[i], missing[i]); assert(moved); (void)moved;
		}
		MarkWholeScreenDirty();
	}
	return CommandCost{}; // Capital was paid at placement; internal material reservation earns no freight payment.
}

/** @copydoc HasOrbitalConstructionInputs */
bool HasOrbitalConstructionInputs(const SpaceStructure &site)
{
	const auto *d = GetSpaceConstructionDefinition(site.state.kind);
	// Only the matching inert recipe may authorize builder work; a completed facility cannot consume it again.
	if (site.state.phase != SpaceStructurePhase::Blueprint || d == nullptr || d->inputs.size() != site.job.inputs.size()) return false;
	// Every material must be present in the job's exclusive physical owner before work can begin.
	for (uint i = 0; i < 2; ++i) if (site.job.inputs[i].AvailableCount() != d->inputs[i].quantity) return false;
	return true;
}
/** Check the full physical builder admission, independent of blueprint material availability.
 * @param v Native builder. @param site Native target site. @return Whether owned, unstopped and precisely docked.
 */
static bool ReadyProbe(const SpaceVehicle &v, const SpaceStructure &site)
{
	return Company::IsValidID(v.owner) && v.owner == site.state.owner && GetSpaceVehicleModel(v) == 20 &&
		v.construction.content_version == MULTILAYER_SPACE_CONTENT_VERSION && v.manifest != nullptr &&
		v.construction.placement == SpacePlacement::Orbital && !v.vehstatus.Test(VehState::Stopped) && !v.vehstatus.Test(VehState::Crashed) &&
		v.manifest->service_ticks == 0 && !HasOrbitalServiceAssignment(v) && v.position == GetSpaceStructureApproach(site.state);
}
/** @param v Native builder. @return Its current, generation-qualified construction target, or nullptr. */
static SpaceStructure *ConstructionTarget(const SpaceVehicle &v)
{
	const auto &q = v.space_orders;
	// Finished or timed-out orders cannot acquire the construction slot implicitly.
	if (q.cursor >= q.entries.size() || q.blocked == SpaceOrderBlock::Timeout) return nullptr;
	const auto &o = q.entries[q.cursor];
	// Only an explicit blueprint order may work; visiting the same coordinates grants no construction right.
	if (o.action != SpaceOrderAction::Construct || o.target.kind != SpaceOrderTargetKind::Blueprint || o.target.id >= SpaceStructureID::End().base()) return nullptr;
	return ResolveSpaceStructure({SpaceStructureID{static_cast<uint16_t>(o.target.id)}, o.target.generation});
}

/** Resolve the one physical builder sharing this Core's bounded construction slot.
 * @param core Actual operational Core. @return Lowest native eligible builder ID, or nullptr when no complete job is ready.
 */
static const SpaceVehicle *CoreConstructionProbe(const SpaceStructure &core)
{
	// Stable vehicle order prevents several ready sites/builders from multiplying the same physical construction slot.
	for (const auto *probe : SpaceVehicle::Iterate()) {
		const auto *site = ConstructionTarget(*probe);
		// Core bootstrapping uses paid portable tooling; every extension needs actual materials, precise approach and this owned anchor.
		if (site == nullptr || site->state.kind == SpaceBuildKind::Core || !HasOrbitalConstructionInputs(*site) || !ReadyProbe(*probe, *site)) continue;
		if (GetOrbitalConstructionCore(site->state) == &core) return probe;
	}
	return nullptr;
}

/** @copydoc AppendOrbitalConstructionRequests */
void AppendOrbitalConstructionRequests(std::vector<UtilityPowerRequest> &requests)
{
	// Consumer identities are commissioned only by the current normal simulation opportunity, never by paused inspection.
	if (_game_mode != GameMode::Normal || _pause_mode.Any() || construction_prepared_tick != TimerGameTick::counter) return;
	// Each actual Core owns one assembly consumer, separate from its service berth; both compete in the shared allocator for its real panel.
	for (auto *core : SpaceStructure::Iterate()) {
		if (core->state.kind != SpaceBuildKind::Core) continue;
		if (!IsOrbitalModuleOperational(*core)) continue;
		const auto *probe = CoreConstructionProbe(*core);
		if (probe == nullptr) continue;
		const auto *source = GetUtilityPowerProducer(core->module->producer);
		// Legacy uncommissioned or retired panels cannot acquire a free consumer or revive paid construction through a reused company ID.
		if (source == nullptr || !source->active || source->owner != core->state.owner) continue;
		AutoRestoreBackup actor(_current_company, source->owner);
		auto &consumer_id = core->module->assembly_consumer;
		if (consumer_id == UtilityConsumerID{UINT32_MAX}) {
			const std::array sources{core->module->producer};
			const auto registered = RegisterUtilityPowerConsumer(source->owner, MapLayer::Space, UtilityConsumerEffect::AssemblyWork,
				GetMultiLayerSpaceBudget().work_j_per_tick, sources);
			if (!registered) continue;
			consumer_id = *registered; // Registration adds identity/capacity, never energy or physical progress.
		}
		const auto *consumer = GetUtilityPowerConsumer(consumer_id);
		// Runtime gathering retains the same original bounded typed connection accepted by native/copy validation.
		if (consumer == nullptr || !consumer->active || consumer->owner != core->state.owner || consumer->effect != UtilityConsumerEffect::AssemblyWork ||
			consumer->capacity_j != GetMultiLayerSpaceBudget().work_j_per_tick || consumer->linked_producers != std::vector{core->module->producer}) continue;
		const auto *site = ConstructionTarget(*probe);
		const auto remaining = GetOrbitalConstructionRemainingEnergy(site->job, GetSpaceConstructionDefinition(site->state.kind)->work_ticks);
		// The final request is capped at useful work; surplus solar generation stays with the shared ledger's curtailment rules.
		if (!remaining || *remaining == 0) continue;
		requests.push_back({consumer_id, std::min<uint64_t>(*remaining, GetMultiLayerSpaceBudget().work_j_per_tick)});
	}
}
/** @copydoc ReserveOrbitalConstructionInputs */
bool ReserveOrbitalConstructionInputs(SpaceStructure &site, SpaceVehicle &probe, bool execute)
{
	// Delivered packets leave the builder only for its owned, precisely approached, current blueprint order.
	if (site.state.phase != SpaceStructurePhase::Blueprint || !ReadyProbe(probe, site) || ConstructionTarget(probe) != &site || probe.cargo.TotalCount() == 0) return false;
	const auto &d = *GetSpaceConstructionDefinition(site.state.kind);
	// Find the actual recipe label; the other material's stock cannot substitute or be counted twice.
	for (uint i = 0; i < 2; ++i) if (probe.cargo_type == GetCargoTypeByLabel(d.inputs[i].label)) {
		const uint missing = d.inputs[i].quantity - site.job.inputs[i].TotalCount();
		const uint quantity = std::min(missing, probe.cargo.TotalCount());
		// Full recipe slots and failed packet-split admission retain the complete freight hold.
		if (quantity == 0 || !HubCargoTransfer::CanMove(probe.cargo, quantity)) return false;
		return !execute || HubCargoTransfer::Move(probe.cargo, site.job.inputs[i], quantity, probe.tile);
	}
	return false;
}
/** @copydoc IsSpaceConstructionWorkTick */
bool IsSpaceConstructionWorkTick(const SpaceVehicle &probe)
{
	// Only the actual normal native loop can authorize work; a query, paused reload or repeated callback cannot earn another tick.
	if (_game_mode != GameMode::Normal || _pause_mode.Any() || construction_prepared_tick != TimerGameTick::counter) return false;
	const auto *site = ConstructionTarget(probe);
	// Work needs both physical builder admission and the complete all-input material reservation.
	if (site == nullptr || !ReadyProbe(probe, *site) || !HasOrbitalConstructionInputs(*site) || site->job.last_work_tick == TimerGameTick::counter) return false;
	if (site->state.kind == SpaceBuildKind::Core) {
		// Paid portable builders bootstrap a Core independently; only one precise native builder may advance its job in this opportunity.
		for (const auto *v : SpaceVehicle::Iterate()) if (v->index < probe.index && ConstructionTarget(*v) == site && ReadyProbe(*v, *site)) return false;
		return true;
	}
	const auto *core = GetOrbitalConstructionCore(site->state);
	// Every extension competes for its Core's single physical slot and current actual grant; a disabled panel holds all material.
	if (core == nullptr || !core->module->power_enabled || CoreConstructionProbe(*core) != &probe || GetTubePowerPreparedTick() != TimerGameTick::counter) return false;
	AutoRestoreBackup actor(_current_company, probe.owner);
	const auto grant = GetUtilityAllocation(core->module->assembly_consumer);
	return grant.has_value() && grant->granted_energy_j != 0;
}

/** Apply already admitted physical work, including the one final common-material milestone.
 * @param site Sole native job owner. @param next Exact next physical progress. @param interval Original shared epoch, or zero for portable Core tooling.
 */
static void ApplyConstructionProgress(SpaceStructure &site, const OrbitalConstructionProgress &next, uint64_t interval) noexcept
{
	const auto &d = *GetSpaceConstructionDefinition(site.state.kind);
	if (next.work_ticks == d.work_ticks) {
		// Whole-list consumption cannot allocate/fail after the complete all-input preflight.
		for (uint i = 0; i < 2; ++i) {
			const bool consumed = HubCargoTransfer::Consume(site.job.inputs[i], d.inputs[i].quantity); assert(consumed); (void)consumed;
			site.job.consumed[i] = static_cast<uint16_t>(d.inputs[i].quantity);
		}
		site.state.phase = SpaceStructurePhase::FrameComplete;
	}
	static_cast<OrbitalConstructionProgress &>(site.job) = next;
	site.job.last_work_tick = TimerGameTick::counter; site.job.last_work_interval = interval;
}

/** Complete physical assembly participant; the shared owner commits its exact source debit with the same saved work milestone. */
class OrbitalConstructionStep final : public UtilityMutationParticipant {
	SpaceStructure &site;
	SpaceStructure &core;
	SpaceVehicle &probe;
	OrbitalConstructionProgress next;
public:
	/** @param site Actual paid job. @param core Actual physical work provider. @param probe Ordered paid builder. @param next Exact preflighted progress. */
	OrbitalConstructionStep(SpaceStructure &site, SpaceStructure &core, SpaceVehicle &probe, OrbitalConstructionProgress next) : site(site), core(core), probe(probe), next(next) {}
	/** @return The actual assembly effect; generic response quanta never replace this recipe's exact grant use. */
	std::optional<UtilityConsumerEffect> EffectKind() const override { return UtilityConsumerEffect::AssemblyWork; }
	/** @param proposal Shared owner's exact current transaction. @return Whether all physical inputs/slot/progress still agree without ledger reentry. */
	bool Validate(const UtilityMutationProposal &proposal) const override
	{
		const auto planned = PlanOrbitalConstructionProgress(this->site.job, GetSpaceConstructionDefinition(this->site.state.kind)->work_ticks,
			static_cast<uint16_t>(proposal.quantity_j));
		return _game_mode == GameMode::Normal && !_pause_mode.Any() && proposal.quantity_j <= GetMultiLayerSpaceBudget().work_j_per_tick && planned.has_value() && *planned == this->next &&
			construction_prepared_tick == TimerGameTick::counter && this->site.job.last_work_tick != TimerGameTick::counter &&
			HasOrbitalConstructionInputs(this->site) && ReadyProbe(this->probe, this->site) && ConstructionTarget(this->probe) == &this->site &&
			GetOrbitalConstructionCore(this->site.state) == &this->core && this->core.module->power_enabled && CoreConstructionProbe(this->core) == &this->probe &&
			proposal.grant.consumer == this->core.module->assembly_consumer;
	}
	/** @param proposal Already revalidated whole transaction. @return Complete no-allocation physical success, with no utility API reentry. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override
	{
		ApplyConstructionProgress(this->site, this->next, proposal.grant.interval); return true;
	}
};

/** @copydoc CommitOrbitalConstructionStep */
bool CommitOrbitalConstructionStep(SpaceVehicle &probe)
{
	if (!IsSpaceConstructionWorkTick(probe)) return false;
	auto &site = *ConstructionTarget(probe);
	if (site.state.kind == SpaceBuildKind::Core) {
		auto next = static_cast<const OrbitalConstructionProgress &>(site.job); ++next.work_ticks;
		ApplyConstructionProgress(site, next, 0); return true; // The paid portable first-Core path never registers a fictitious station source.
	}
	auto &core = *GetOrbitalConstructionCore(site.state); AutoRestoreBackup actor(_current_company, probe.owner);
	const auto grant = GetUtilityAllocation(core.module->assembly_consumer);
	const auto next = PlanOrbitalConstructionProgress(site.job, GetSpaceConstructionDefinition(site.state.kind)->work_ticks,
		static_cast<uint16_t>(grant->granted_energy_j));
	if (!next) return false;
	OrbitalConstructionStep step(site, core, probe, *next);
	const auto proposal = PrepareUtilityMutation(UtilityMutationKind::Consume, {core.module->assembly_consumer, GetUtilityPowerInterval()}, grant->granted_energy_j);
	return CommitUtilityMutation(proposal, &step).error == UtilityPowerError::None;
}
/** @copydoc IsOrbitalModuleOperational */
bool IsOrbitalModuleOperational(const SpaceStructure &site)
{
	return site.state.phase == SpaceStructurePhase::FrameComplete && site.state.kind == SpaceBuildKind::Core &&
		site.module != nullptr && site.module->activation_ticks == 0 && Company::IsValidID(site.state.owner);
}

/** @copydoc GetOrbitalModuleCargoCapacity */
uint16_t GetOrbitalModuleCargoCapacity(const SpaceStructure &site, CargoType cargo)
{
	// An inert/unowned Core and an unresolved cargo label supply no usable capacity.
	if (!IsOrbitalModuleOperational(site) || !IsValidCargoType(cargo)) return 0;
	return GetMultiLayerSpaceBudget().stock_cap;
}

/** @copydoc InsertOrbitalPacketOwners */
bool InsertOrbitalPacketOwners(std::set<CargoPacketID> &owned)
{
	/** @param list Actual native packet container. @return Whether its identities are still exclusive. */
	auto insert = [&](const StationCargoList &list) {
		// Retain each packet's physical identity rather than trusting aggregate container counts.
		for (const auto &[hop, packets] : *list.Packets()) {
			(void)hop;
			for (const auto *packet : packets) if (!owned.insert(packet->index).second) return false;
		}
		return true;
	};
	// Every site has exclusive construction stock and, after activation starts, separate operational stock.
	for (const auto *site : SpaceStructure::Iterate()) {
		for (const auto &slot : site->job.inputs) if (!insert(slot)) return false;
		if (site->module) for (const auto &slot : site->module->stock) if (!insert(slot)) return false;
		if (site->module) for (const auto &slot : site->module->service.inputs) if (!insert(slot)) return false;
	}
	return true;
}

/** @copydoc TickOrbitalConstructionInventory */
void TickOrbitalConstructionInventory()
{
	// Both retained construction inputs and completed stock age in their sole native container.
	for (auto *site : SpaceStructure::Iterate()) {
		// Empty construction jobs retain their saved aging remainder without inventing a new packet history.
		if (site->job.inputs[0].TotalCount() != 0 || site->job.inputs[1].TotalCount() != 0) {
			if (++site->job.age_ticks == Ticks::CARGO_AGING_TICKS) {
				site->job.age_ticks = 0;
				for (auto &slot : site->job.inputs) HubCargoTransfer::Age(slot);
			}
		}
		if (!site->module) continue;
		auto &module = *site->module;
		// Company loss preserves paid stock and pending activation; a reused company ID gains no operation.
		if (Company::IsValidID(site->state.owner) && module.activation_ticks != 0) --module.activation_ticks;
		if (++module.age_ticks == Ticks::CARGO_AGING_TICKS) {
			module.age_ticks = 0;
			// Age every physical cargo label once; GUI polling and service queries never age or recreate stock.
			for (auto &slot : module.stock) HubCargoTransfer::Age(slot);
			for (auto &slot : module.service.inputs) HubCargoTransfer::Age(slot);
		}
	}
	TickOrbitalServices();
}
