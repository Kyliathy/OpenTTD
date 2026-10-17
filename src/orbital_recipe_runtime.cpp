/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_recipe_runtime.cpp Native processor controls and one common utility phase.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C
 * Shared participants alone authorize consumption/output; the actual module owns all metadata and packets.
 */
#include "stdafx.h"
#include "orbital_recipe.h"
#include "space_structure.h"
#include "space_structure_cmd.h"
#include "command_func.h"
#include "transport_tube.h"
#include "company_func.h"
#include "company_base.h"
#include "core/backup_type.hpp"
#include "timer/timer_game_tick.h"
#include "openttd.h"
#include "gfx_func.h"
#include "safeguards.h"

/** Probe all-input material feasibility through the original shared planner; scalar budgets grant no work or energy.
 * @param recipe Exact published definition. @param inventory Actual native packet view.
 * @return Whether one complete batch's original materials and outputs fit.
 */
static MultiLayerRecipeBatchPlan OrbitalMaterialOpportunity(const MultiLayerRecipe &recipe, const OrbitalRecipeInventory &inventory)
{
	const auto view = inventory.Capture();
	if (!view.active) return {0, MultiLayerRecipeLimit::WorkTime};
	return PlanMultiLayerRecipeBatches(recipe, view.inputs, view.outputs, recipe.work_economy_ticks, 0, 0, recipe.power_j_per_batch, 0);
}

/** @copydoc ConnectOrbitalRecipePower */
CommandCost ConnectOrbitalRecipePower(DoCommandFlags flags, SpaceStructure &site, const SpaceStructure &source)
{
	// Only the actual owned operational edge can acquire a dedicated original consumer; proximity or copied utility IDs cannot commission work.
	if (!IsOrbitalModuleOperational(site) || !IsOrbitalModuleOperational(source) || site.state.owner != _current_company || source.state.owner != _current_company ||
		!site.module->processing || !AreOrbitalModuleFootprintsAdjacent(site.state, source.state)) return CMD_ERROR;
	auto &state = *site.module->processing; const auto *definition = GetOrbitalRecipeDefinition(site.state.kind, state.choice);
	const auto *producer = GetUtilityPowerProducer(source.module->producer);
	// Unpowered choices cannot obtain a hidden account; reconnection never duplicates an original consumer or renews a pending physical receipt.
	if (definition == nullptr || definition->recipe.power_j_per_batch == 0 || state.consumer != UtilityConsumerID{UINT32_MAX} || state.electrical || state.source != SpaceStructureRef{} ||
		state.revision >= UINT64_MAX - 2 || producer == nullptr || !producer->active || producer->owner != _current_company || producer->kind != UtilityGenerationKind::OrbitalPanel || producer->meter.layer != MapLayer::Space) return CMD_ERROR;
	const auto rate = definition->recipe.power_j_per_batch / definition->recipe.work_economy_ticks; const std::array sources{source.module->producer};
	const auto predicted = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, rate, sources, false);
	if (!predicted) return CMD_ERROR;
	// Allocate empty private framing before registration. Only the following native synchronous registration issues the actual identity, not this pure predicted quote.
	if (flags.Test(DoCommandFlag::Execute) && !flags.Test(DoCommandFlag::QueryCost)) {
		auto work = std::make_unique<LayerRecipeElectricalWork>(definition->recipe, *predicted);
		const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, rate, sources);
		if (!consumer) return CMD_ERROR;
		assert(*consumer == *predicted); state.consumer = *consumer; state.source = GetSpaceStructureReference(source); state.electrical = std::move(work); ++state.revision; MarkWholeScreenDirty();
	}
	return CommandCost{};
}

/** @copydoc DisconnectOrbitalRecipePower */
CommandCost DisconnectOrbitalRecipePower(DoCommandFlags flags, SpaceStructure &site)
{
	// Explicit cleanup remains possible during an outage but cannot dispose of an enabled or unfinished paid-work owner.
	if (site.state.owner != _current_company || !Company::IsValidID(_current_company) || !site.module || !site.module->processing) return CMD_ERROR;
	auto &state = *site.module->processing;
	if (state.enabled || !state.electrical || state.electrical->Capture().paid_j != 0 || state.consumer == UtilityConsumerID{UINT32_MAX} || state.revision == 0 || state.revision == UINT64_MAX) return CMD_ERROR;
	const bool execute = flags.Test(DoCommandFlag::Execute) && !flags.Test(DoCommandFlag::QueryCost);
	if (RetireOrbitalUtilityAccounts({}, std::array{state.consumer}, execute) != UtilityPowerError::None) return CMD_ERROR;
	if (execute) {
		state.electrical.reset(); state.consumer = UtilityConsumerID{UINT32_MAX}; state.source = {};
		// Terminal revisions already refuse every material/work proposal. Explicit cleanup may still retire an empty connection without wrapping or renewing processing authority.
		if (state.revision < UINT64_MAX - 1) ++state.revision;
		MarkWholeScreenDirty();
	}
	return CommandCost{};
}

/** @copydoc CmdControlOrbitalRecipe */
CommandCost CmdControlOrbitalRecipe(DoCommandFlags flags, SpaceStructureID id, uint64_t generation, uint32_t recipe_value, uint8_t control_value)
{
	auto *site = ResolveSpaceStructure({id, generation}); const auto control = static_cast<OrbitalRecipeControl>(control_value);
	// Exact owned incarnation and representable revision precede all controls, including pause/cancel during an outage; stale queues cannot target replacement modules.
	if (site == nullptr || site->state.owner != _current_company || !Company::IsValidID(_current_company) || !site->module || !site->module->processing ||
		control >= OrbitalRecipeControl::End || site->module->processing->revision == 0 || site->module->processing->revision == UINT64_MAX) return CMD_ERROR;
	auto &state = *site->module->processing; auto choice = state.choice;
	if (control == OrbitalRecipeControl::Start) {
		MultiLayerRecipeID requested;
		// Decode the same stable big-endian four-byte choice used by native industry commands; no local ordinal becomes a second recipe registry.
		for (size_t i = 0; i < requested.size(); ++i) requested[i] = static_cast<uint8_t>(recipe_value >> (24 - 8 * i));
		if (recipe_value == 0) requested = GetDefaultOrbitalRecipe(site->state.kind);
		const auto *definition = GetOrbitalRecipeDefinition(site->state.kind, requested);
		// Changing an alternative cannot silently abandon paid/baseline work. Explicit cancellation owns that irreversible physical-progress discard.
		if (definition == nullptr || state.revision >= UINT64_MAX - 2 || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || TimerGameCalendar::year < definition->introduction_year || !IsOrbitalModuleOperational(*site) || state.enabled || state.unpowered_work_ticks != 0 ||
			(state.electrical && (state.electrical->Capture().paid_j != 0 || requested != state.choice))) return CMD_ERROR;
		choice = requested;
	} else if (control == OrbitalRecipeControl::Resume) {
		const auto *definition = GetOrbitalRecipeDefinition(site->state.kind, state.choice);
		// Resume retains its original paid work but still requires the currently available profile/date and actual operational module.
		if (definition == nullptr || state.revision >= UINT64_MAX - 2 || state.enabled || !IsOrbitalModuleOperational(*site) || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || TimerGameCalendar::year < definition->introduction_year) return CMD_ERROR;
	} else if (control == OrbitalRecipeControl::Pause) {
		if (!state.enabled) return CMD_ERROR;
	} else if (state.electrical) {
		const auto &paid = state.electrical->Capture();
		// Both native history and the original private sequence must represent the complete discard before cancellation changes any owner.
		if (paid.paid_j > UINT64_MAX - state.discarded_paid_j || (paid.paid_j != 0 && paid.sequence >= UINT64_MAX - 1)) return CMD_ERROR;
	}
	// Mixed Execute/QueryCost remains a pure quote, preserving permission, choice, progress, opportunity and history.
	if (flags.Test(DoCommandFlag::Execute) && !flags.Test(DoCommandFlag::QueryCost)) {
		state.choice = choice; state.enabled = control == OrbitalRecipeControl::Start || control == OrbitalRecipeControl::Resume;
		if (control == OrbitalRecipeControl::Cancel) {
			if (state.electrical) { const auto discarded = state.electrical->Discard(); assert(discarded); state.discarded_paid_j += *discarded; }
			state.unpowered_work_ticks = 0;
		}
		// A terminal owner can still pause/cancel safely: all old production quotes already refuse that revision, so cleanup need not wrap its exhausted stamp.
		if (state.revision < UINT64_MAX - 1) ++state.revision;
		MarkWholeScreenDirty();
	}
	return CommandCost{};
}

/** @copydoc AppendOrbitalRecipeRequests */
void AppendOrbitalRecipeRequests(std::vector<UtilityPowerRequest> &requests)
{
	// All actual modules compete once in the original allocation; idle, congested, unpowered and fully paid recipes request no phantom electricity.
	for (auto *site : SpaceStructure::Iterate()) {
		if (!site->module || !site->module->processing || !Company::IsValidID(site->state.owner)) continue;
		auto &state = *site->module->processing;
		// Missing original geometry/source must not reserve another facility's useful output even before the common allocator runs.
		if (!state.enabled || !state.electrical || !IsOrbitalRecipePowerConnectionValid(*site, state)) continue;
		AutoRestoreBackup actor(_current_company, site->state.owner); OrbitalRecipeInventory inventory(state);
		const auto *definition = GetOrbitalRecipeDefinition(site->state.kind, state.choice); if (definition == nullptr) continue;
		const auto demand = PrepareLayerRecipeWorkDemand(definition->recipe, inventory, inventory, GetMultiLayerSpaceBudget().work_j_per_tick);
		if (demand.error != LayerRecipeCommitError::None || !inventory.ValidateWork(demand)) continue;
		requests.push_back({state.consumer, demand.energy_j});
	}
}

/** @copydoc TickOrbitalRecipes */
void TickOrbitalRecipes()
{
	// Only the normal synchronized tick after the common allocation can earn work. Pauses, editor calls and repeated invocations cannot accelerate a physical recipe.
	if (_game_mode != GameMode::Normal || _pause_mode.Any() || GetTubePowerPreparedTick() != TimerGameTick::counter || TimerGameTick::counter >= UINT64_MAX - 1) return;
	const uint64_t stamp = TimerGameTick::counter + 1;
	// Native pool order is deterministic; each module owns its actual stock, private progress and one opportunity independently of rendering or polling.
	for (auto *site : SpaceStructure::Iterate()) {
		if (!site->module || !site->module->processing || !Company::IsValidID(site->state.owner)) continue;
		auto &state = *site->module->processing;
		if (!state.enabled || state.last_native_tick == stamp || !IsOrbitalModuleOperational(*site)) continue;
		AutoRestoreBackup actor(_current_company, site->state.owner); OrbitalRecipeInventory inventory(state);
		const auto *definition = GetOrbitalRecipeDefinition(site->state.kind, state.choice); if (definition == nullptr) continue;
		const auto &recipe = definition->recipe;
		if (state.electrical) {
			const auto quote = PrepareLayerRecipeWork(recipe, inventory, inventory, GetMultiLayerSpaceBudget().work_j_per_tick);
			// One real grant must settle with the complete physical owner; a failed native work transaction cannot advance the synchronized opportunity.
			if (quote.error == LayerRecipeCommitError::None && CommitLayerRecipeWork(recipe, inventory, inventory, quote) == LayerRecipeCommitError::None) state.last_native_tick = stamp;
		} else if (recipe.power_j_per_batch == 0) {
			const auto material = OrbitalMaterialOpportunity(recipe, inventory);
			// Unpowered choices still earn only one baseline tick with every input/output present; full progress survives packet allocation refusal without multiplying throughput.
			if (material.batches != 0 && state.unpowered_work_ticks < recipe.work_economy_ticks && inventory.CanRecordWork(0)) {
				++state.unpowered_work_ticks; ++state.revision; state.last_native_tick = stamp;
			}
		}
		const auto proposal = PrepareLayerRecipeBatch(recipe, inventory);
		// Every original output must exist before any input disappears. Failure retains complete paid/baseline progress and actual packets for another native tick.
		if (proposal.plan.batches != 0 && inventory.PrepareOutputs(proposal, true) && CommitLayerRecipeBatch(recipe, inventory, inventory, proposal).error == LayerRecipeCommitError::None) state.last_native_tick = stamp;
	}
}

/** @copydoc GetOrbitalRecipeWait */
OrbitalRecipeWait GetOrbitalRecipeWait(const SpaceStructure &site)
{
	if (!site.module || !site.module->processing) return OrbitalRecipeWait::Inactive;
	auto &state = *site.module->processing;
	if (!state.enabled) return OrbitalRecipeWait::Disabled;
	OrbitalRecipeInventory inventory(state); const auto *definition = GetOrbitalRecipeDefinition(site.state.kind, state.choice);
	// Missing native forms/owner/profile invalidate the whole view; a UI query cannot expose partial permission from retained stock.
	if (definition == nullptr || !inventory.Capture().active) return OrbitalRecipeWait::Inactive;
	const auto material = OrbitalMaterialOpportunity(definition->recipe, inventory);
	if (material.limiting == MultiLayerRecipeLimit::Input) return OrbitalRecipeWait::Input;
	if (material.limiting == MultiLayerRecipeLimit::Output) return OrbitalRecipeWait::Output;
	if (material.batches == 0) return OrbitalRecipeWait::Limit;
	const auto batch = PrepareLayerRecipeBatch(definition->recipe, inventory);
	if (batch.plan.batches != 0) {
		if (!CargoPacket::CanAllocateItem(batch.outputs.size())) return OrbitalRecipeWait::Allocation;
		return inventory.PrepareOutputs(batch, false) ? OrbitalRecipeWait::None : OrbitalRecipeWait::Limit;
	}
	if (!inventory.CanRecordWork(0)) return OrbitalRecipeWait::Limit;
	if (definition->recipe.power_j_per_batch == 0) return OrbitalRecipeWait::Work;
	if (!state.electrical) return OrbitalRecipeWait::Power;
	const auto work = PrepareLayerRecipeWork(definition->recipe, inventory, inventory, GetMultiLayerSpaceBudget().work_j_per_tick);
	// A valid electrical quote can still be unrecordable in the actual native material/history owner; report that bound before advertising useful work.
	if (work.error == LayerRecipeCommitError::None && !inventory.ValidateWork(work)) return OrbitalRecipeWait::Limit;
	return work.limiting == MultiLayerRecipeLimit::Power ? OrbitalRecipeWait::Power : OrbitalRecipeWait::Work;
}
