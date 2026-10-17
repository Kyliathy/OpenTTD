/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file layer_industry_recipe.cpp Complete bounded recipe transactions over the actual facility owner's inventory.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Groups 6-7/10/13
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md, all-input recipe contract
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 8 atomic electrical participant
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10B pure preallocation admission uses this same complete-batch validator.
 */

#include "stdafx.h"
#include "multilayer_industry.h"
#include "layer_recipe_work.h"
#include "utility_power.h"

#include "safeguards.h"

/**
 * Compare every field of two stock views without treating either copy as physical inventory.
 * @param left Previously captured actual stocks.
 * @param right Current actual stocks, in the owner's stable slot order.
 * @return Whether meanings, quantities and capacities are unchanged.
 */
static bool EqualLayerRecipeStocks(std::span<const MultiLayerRecipeStock> left, std::span<const MultiLayerRecipeStock> right)
{
	if (left.size() != right.size()) return false;
	// Changing a cap or label invalidates a quote as surely as changing delivered quantity; check the complete view.
	for (size_t index = 0; index < left.size(); ++index) {
		if (left[index].label != right[index].label || left[index].amount != right[index].amount || left[index].capacity != right[index].capacity) return false;
	}
	return true;
}

/**
 * Check all state that could change the feasibility or identity of a saved-choice batch.
 * @param left Prepared snapshot.
 * @param right Fresh owner snapshot.
 * @return Whether recipe, layer, availability, stock, work and supplied grant quantities still agree.
 */
bool EqualLayerRecipeInventory(const LayerRecipeInventorySnapshot &left, const LayerRecipeInventorySnapshot &right)
{
	return left.recipe == right.recipe && left.layer == right.layer && left.revision == right.revision &&
		left.active == right.active && left.work_ticks == right.work_ticks && left.heat_j == right.heat_j &&
		left.power_j == right.power_j && EqualLayerRecipeStocks(left.inputs, right.inputs) && EqualLayerRecipeStocks(left.outputs, right.outputs);
}

/**
 * Prepare checked cargo deltas for one role after the common planner admitted every role.
 * @param roles Immutable recipe inputs or outputs.
 * @param batches Complete feasible batch count.
 * @return Exact debits/credits; validated recipe/stock bounds keep every product within uint32_t.
 */
static std::vector<LayerRecipeCargoDelta> MakeLayerRecipeDeltas(std::span<const MultiLayerRecipeCargo> roles, uint16_t batches)
{
	std::vector<LayerRecipeCargoDelta> deltas;
	deltas.reserve(roles.size());
	// Every role belongs to the same complete batch count; no independently rounded input can be consumed early.
	for (const auto &role : roles) {
		const uint64_t quantity = static_cast<uint64_t>(role.quantity) * batches;
		assert(quantity <= UINT32_MAX);
		deltas.push_back({role.label, static_cast<uint32_t>(quantity)});
	}
	return deltas;
}

/**
 * Build a proposal from one already captured view, so commit revalidation does not recapture or change the owner.
 * @param recipe Immutable chosen recipe.
 * @param inventory Actual borrowed inventory identity.
 * @param before Pure actual snapshot; ownership of its vectors moves into the transient proposal.
 * @return Complete checked proposal or zero-batch refusal, without state mutation.
 */
static LayerRecipeBatchProposal BuildLayerRecipeProposal(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory, LayerRecipeInventorySnapshot before)
{
	LayerRecipeBatchProposal proposal;
	proposal.owner = &inventory;
	proposal.definition = &recipe;
	proposal.before = std::move(before);
	const auto &state = proposal.before;
	// This executor owns delivered-stock processing; geological extraction and internal aggregate have separate owners.
	if (recipe.reserve_per_batch != 0 || recipe.aggregate_per_batch != 0 || ValidateMultiLayerRecipe(recipe) != MultiLayerRecipeLimit::None) return proposal;
	// A saved alternative and valid physical layer must match; invalid/revision-exhausted identities grant no commit.
	if (state.recipe != recipe.id || !MultiLayerMap::IsValidLayer(state.layer) || state.revision == 0 || state.revision == UINT64_MAX) {
		proposal.plan.limiting = MultiLayerRecipeLimit::InvalidStock;
		return proposal;
	}
	if (!state.active) {
		proposal.plan.limiting = MultiLayerRecipeLimit::WorkTime;
		return proposal;
	}
	const auto *work = inventory.ElectricalWork();
	if (work != nullptr) {
		// Only this noncopyable shared receipt supplies already-paid progress. An arbitrary snapshot work/power scalar cannot reinterpret another recipe's bill or duplicate earned work.
		if (!ValidateLayerRecipeElectricalWork(recipe, work->Capture()) || work->WorkTicks() != state.work_ticks) {
			proposal.plan.limiting = MultiLayerRecipeLimit::InvalidStock; return proposal;
		}
		proposal.electrical_work = work->Capture();
	}
	// Staged work has already consumed its original electrical source. Ordinary callers retain the actual current-grant path; both use this same all-input/output planner.
	const uint64_t available_power = work == nullptr ? state.power_j : work->Capture().paid_j;
	proposal.plan = PlanMultiLayerRecipeBatches(recipe, state.inputs, state.outputs, state.work_ticks, 0, state.heat_j, available_power, 0);
	if (proposal.plan.batches == 0) return proposal;
	proposal.inputs = MakeLayerRecipeDeltas(recipe.inputs, proposal.plan.batches);
	proposal.outputs = MakeLayerRecipeDeltas(recipe.outputs, proposal.plan.batches);
	// Validation bounds each per-batch energy product and the planner bounds all stock products before multiplication.
	proposal.work_ticks = static_cast<uint64_t>(recipe.work_economy_ticks) * proposal.plan.batches;
	proposal.heat_j = recipe.heat_j_per_batch * proposal.plan.batches;
	proposal.power_j = recipe.power_j_per_batch * proposal.plan.batches;
	if (work != nullptr) { proposal.paid_power_j = proposal.power_j; proposal.power_j = 0; }
	return proposal;
}

/** @copydoc PrepareLayerRecipeBatch */
LayerRecipeBatchProposal PrepareLayerRecipeBatch(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory)
{
	return BuildLayerRecipeProposal(recipe, inventory, inventory.Capture());
}

/** Shared utility participant commits the same actual recipe owner; it owns no copied stock or electrical counter. */
class RecipeUtilityTransaction final : public UtilityMutationParticipant {
	LayerRecipeCommitParticipant &participant; ///< Existing whole physical inventory/work transaction owner.
	const LayerRecipeInventoryView &inventory; ///< Exact actual owner view used for original preparation.
	const LayerRecipeBatchProposal &recipe; ///< Original checked material/work proposal, transient within this action.
public:
	/** @param participant Actual recipe commit owner. @param inventory Its exact original view. @param recipe Complete checked proposal. */
	RecipeUtilityTransaction(LayerRecipeCommitParticipant &participant, const LayerRecipeInventoryView &inventory,
		const LayerRecipeBatchProposal &recipe) : participant(participant), inventory(inventory), recipe(recipe) {}
	/** @param proposal Exact electrical reservation. @return Whether the same actual inventory and complete material/work action remain valid. */
	bool Validate([[maybe_unused]] const UtilityMutationProposal &proposal) const override
	{
		if (!EqualLayerRecipeInventory(this->recipe.before, this->inventory.Capture())) return false;
		return this->participant.Validate(this->recipe);
	}
	/** @param proposal Exact already checked electrical debit. @return Complete inventory/work commit or unchanged refusal. */
	bool Commit([[maybe_unused]] const UtilityMutationProposal &proposal) noexcept override { return CommitLayerRecipeInventoryAndPaidWork(this->participant, this->recipe); }
};

/** @copydoc ValidateLayerRecipeBatchProposal */
LayerRecipeCommitError ValidateLayerRecipeBatchProposal(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory, LayerRecipeCommitParticipant &participant, const LayerRecipeBatchProposal &proposal)
{
	// Preparation and execution must use one real owner; a copied view cannot authorize another facility's stocks.
	if (proposal.owner != &inventory || &participant.Inventory() != &inventory) return LayerRecipeCommitError::WrongOwner;
	if (participant.ElectricalWork() != inventory.ElectricalWork()) return LayerRecipeCommitError::WrongOwner;
	if (proposal.definition != &recipe) return LayerRecipeCommitError::InvalidProposal;
	const auto current = inventory.Capture();
	if (!EqualLayerRecipeInventory(proposal.before, current)) return LayerRecipeCommitError::Stale;
	const auto expected = BuildLayerRecipeProposal(recipe, inventory, current);
	// Recompute every delta and bound: a modified count, secondary output, work debit or grant cannot reuse a quote.
	if (proposal.plan.batches != expected.plan.batches || proposal.plan.limiting != expected.plan.limiting ||
			proposal.inputs != expected.inputs || proposal.outputs != expected.outputs || proposal.work_ticks != expected.work_ticks ||
			proposal.heat_j != expected.heat_j || proposal.power_j != expected.power_j || proposal.electrical_work != expected.electrical_work ||
			proposal.paid_power_j != expected.paid_power_j) return LayerRecipeCommitError::InvalidProposal;
	// Malformed definitions/bindings are refusals; ordinary lack of stock or work remains a harmless zero-batch pause.
	if (proposal.plan.limiting == MultiLayerRecipeLimit::InvalidDefinition || proposal.plan.limiting == MultiLayerRecipeLimit::InvalidStock) return LayerRecipeCommitError::InvalidProposal;
	return LayerRecipeCommitError::None;
}

/** @copydoc CommitLayerRecipeBatch */
LayerRecipeBatchReceipt CommitLayerRecipeBatch(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory, LayerRecipeCommitParticipant &participant, const LayerRecipeBatchProposal &proposal)
{
	const auto checked = ValidateLayerRecipeBatchProposal(recipe, inventory, participant, proposal);
	if (checked != LayerRecipeCommitError::None) return {checked};
	if (proposal.plan.batches == 0) return {LayerRecipeCommitError::None, 0};
	// Heat still requires its distinct vent owner; electrical snapshots alone
	// also grant no custody without this exact participant's real ledger token.
	if (proposal.heat_j != 0) return {LayerRecipeCommitError::Refused};
	if (proposal.power_j != 0) {
		UtilityGrantID grant;
		if (!participant.GetElectricalGrant(grant)) return {LayerRecipeCommitError::Refused};
		const auto allocation = GetUtilityAllocation(grant.consumer);
		// A physical recipe may use only its own explicit layer's actual grant;
		// overlapping Surface/underground inventory cannot borrow another meter.
		if (!allocation || allocation->layer != proposal.before.layer) return {LayerRecipeCommitError::Refused};
		const auto energy = PrepareUtilityMutation(UtilityMutationKind::Consume, grant, proposal.power_j);
		if (energy.error != UtilityPowerError::None) return {LayerRecipeCommitError::Refused};
		RecipeUtilityTransaction transaction(participant, inventory, proposal);
		const auto receipt = CommitUtilityMutation(energy, &transaction);
		if (receipt.error != UtilityPowerError::None) return {LayerRecipeCommitError::Refused};
		return {LayerRecipeCommitError::None, proposal.plan.batches};
	}
	if (!participant.Validate(proposal)) return {LayerRecipeCommitError::Refused};
	if (!CommitLayerRecipeInventoryAndPaidWork(participant, proposal)) return {LayerRecipeCommitError::Refused};
	return {LayerRecipeCommitError::None, proposal.plan.batches};
}
