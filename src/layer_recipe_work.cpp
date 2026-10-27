/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file layer_recipe_work.cpp Shared paid recipe progress and once-only complete material settlement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md shared recipe participant
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 2 (include what is used on Linux)
 * Only actual utility transactions earn work. Pending receipts are consumed physical progress, never a second grid or refundable power store.
 */
#include "stdafx.h"
#include "layer_recipe_work.h"
#include <utility> // std::exchange in the move operations below.

#include "safeguards.h"

/** Resolve an exact integral rate without reinterpreting a published recipe or permitting geological/thermal stock to fund electrical work.
 * @param recipe Actual immutable complete material recipe.
 * @return Positive joules per work tick, or absence for unsupported staged-work arithmetic.
 */
static std::optional<uint64_t> ElectricalRecipeWorkRate(const MultiLayerRecipe &recipe)
{
	// Staged electrical progress supports only delivered-stock recipes with a positive exact per-tick rate; heat, geological extraction and nonintegral rates retain their separate owners.
	if (ValidateMultiLayerRecipe(recipe) != MultiLayerRecipeLimit::None || recipe.reserve_per_batch != 0 || recipe.aggregate_per_batch != 0 ||
		recipe.heat_j_per_batch != 0 || recipe.work_economy_ticks == 0 || recipe.power_j_per_batch == 0 || recipe.power_j_per_batch % recipe.work_economy_ticks != 0) return std::nullopt;
	return recipe.power_j_per_batch / recipe.work_economy_ticks;
}

/** @copydoc ValidateLayerRecipeElectricalWork */
bool ValidateLayerRecipeElectricalWork(const MultiLayerRecipe &recipe, const LayerRecipeElectricalWorkRecord &record)
{
	const auto rate = ElectricalRecipeWorkRate(recipe);
	// The same immutable alternative/rate and original consumer must own every paid joule; another choice or scalar work field cannot reuse this receipt.
	if (!rate || record.recipe != recipe.id || record.consumer == UINT32_MAX || record.work_per_batch != recipe.work_economy_ticks ||
		record.power_per_batch != recipe.power_j_per_batch || record.paid_j > recipe.power_j_per_batch || record.sequence == 0 || record.sequence == UINT64_MAX) return false;
	// A fresh receipt has no opportunity history; an earned/settled receipt retains this interval's actual positive work, capped to one canonical tick even across split grants.
	if (record.last_interval == 0) return record.interval_paid_j == 0 && record.paid_j == 0 && record.sequence == 1;
	return record.interval_paid_j != 0 && record.interval_paid_j <= *rate && record.sequence > 1;
}

/** @copydoc LayerRecipeElectricalWork::LayerRecipeElectricalWork(const MultiLayerRecipe &, UtilityConsumerID) */
LayerRecipeElectricalWork::LayerRecipeElectricalWork(const MultiLayerRecipe &recipe, UtilityConsumerID consumer)
{
	this->state.recipe = recipe.id; this->state.consumer = consumer.base(); this->state.work_per_batch = recipe.work_economy_ticks; this->state.power_per_batch = recipe.power_j_per_batch;
}

/** @copydoc LayerRecipeElectricalWork::LayerRecipeElectricalWork(LayerRecipeElectricalWork &&) */
LayerRecipeElectricalWork::LayerRecipeElectricalWork(LayerRecipeElectricalWork &&other) noexcept : state(std::exchange(other.state, {})) {}

/** @copydoc LayerRecipeElectricalWork::operator=(LayerRecipeElectricalWork &&) */
LayerRecipeElectricalWork &LayerRecipeElectricalWork::operator=(LayerRecipeElectricalWork &&other) noexcept
{
	if (this != &other) this->state = std::exchange(other.state, {});
	return *this;
}

/** @copydoc LayerRecipeElectricalWork::WorkTicks */
uint64_t LayerRecipeElectricalWork::WorkTicks() const
{
	// Invalid/moved receipts cannot expose completed work; the full immutable binding is checked against the published definition before any mutation.
	if (this->state.work_per_batch == 0 || this->state.power_per_batch == 0 || this->state.power_per_batch % this->state.work_per_batch != 0 || this->state.paid_j > this->state.power_per_batch) return 0;
	return this->state.paid_j / (this->state.power_per_batch / this->state.work_per_batch);
}

/** @copydoc LayerRecipeElectricalWork::Restore */
std::unique_ptr<LayerRecipeElectricalWork> LayerRecipeElectricalWork::Restore(const MultiLayerRecipe &recipe, const LayerRecipeElectricalWorkRecord &record, MapLayer layer)
{
	const auto *consumer = GetUtilityPowerConsumer(UtilityConsumerID{record.consumer});
	// Saved arithmetic cannot invent a consumer, another physical layer or a future opportunity. Native feature joins must already establish the exact incarnation and exclusive historical owner.
	if (!ValidateLayerRecipeElectricalWork(recipe, record) || consumer == nullptr || consumer->meter.layer != layer || record.last_interval > GetUtilityPowerInterval()) return nullptr;
	if (record.last_interval == GetUtilityPowerInterval()) {
		uint64_t consumed = 0;
		// Current-interval paid work must be backed by this exact actual consumer's settled source slices; a copied paid field cannot claim unused allocation as completed work.
		for (const auto &share : consumer->shares) {
			if (share.consumed_j > UINT64_MAX - consumed) return nullptr;
			consumed += share.consumed_j;
		}
		if (record.interval_paid_j > consumed) return nullptr;
	}
	auto work = std::make_unique<LayerRecipeElectricalWork>(recipe, UtilityConsumerID{record.consumer}); work->state = record; return work;
}

/** @copydoc LayerRecipeElectricalWork::Discard */
std::optional<uint64_t> LayerRecipeElectricalWork::Discard()
{
	if (state.paid_j == 0) return 0;
	if (state.sequence >= UINT64_MAX - 1) return std::nullopt;
	const auto discarded = state.paid_j; state.paid_j = 0; ++state.sequence; return discarded;
}

/** Sole shared access to private pending progress after the corresponding complete physical transaction succeeds. */
struct LayerRecipeWorkAccess {
	/** @param work Same original physical receipt. @param proposal Exact already revalidated actual source debit. */
	static void Earn(LayerRecipeElectricalWork &work, const LayerRecipeWorkProposal &proposal) noexcept
	{
		auto &state = work.state;
		if (state.last_interval != proposal.grant.interval) state.interval_paid_j = 0;
		state.last_interval = proposal.grant.interval; state.interval_paid_j += proposal.energy_j; state.paid_j += proposal.energy_j; ++state.sequence;
	}
	/** @param work Same original physical receipt. @param amount Already revalidated paid work embodied in the completed material batch. */
	static void Settle(LayerRecipeElectricalWork &work, uint64_t amount) noexcept
	{
		work.state.paid_j -= amount; ++work.state.sequence;
	}
};

/** @copydoc PrepareLayerRecipeWorkDemand */
LayerRecipeWorkProposal PrepareLayerRecipeWorkDemand(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory,
	LayerRecipeElectricalWorkParticipant &participant, uint64_t requested_j)
{
	LayerRecipeWorkProposal result; result.owner = &inventory; result.definition = &recipe; result.before = inventory.Capture();
	const auto *work = inventory.ElectricalWork(); const auto rate = ElectricalRecipeWorkRate(recipe);
	// A copied view or another participant's receipt cannot earn work for the physical stock owner that supplied this proposal.
	if (&participant.Inventory() != &inventory || work == nullptr || participant.ElectricalWork() != work) { result.error = LayerRecipeCommitError::WrongOwner; result.limiting = MultiLayerRecipeLimit::InvalidStock; return result; }
	result.receipt = work->Capture(); const auto &state = result.before;
	if (!rate) return result;
	result.limiting = MultiLayerRecipeLimit::InvalidStock;
	// Receipt arithmetic, owner revision and completed whole work must agree before even pricing a real electrical opportunity.
	if (!ValidateLayerRecipeElectricalWork(recipe, result.receipt) || state.recipe != recipe.id ||
		state.revision == 0 || state.revision == UINT64_MAX || state.work_ticks != work->WorkTicks() || requested_j == 0 || result.receipt.sequence >= UINT64_MAX - 2) return result;
	if (!state.active) { result.error = LayerRecipeCommitError::Refused; result.limiting = MultiLayerRecipeLimit::WorkTime; return result; }
	// This pure probe asks whether one complete material batch could fit. Its scalar budgets authorize no energy; the subsequent allocated-work executor alone may pay actual progress.
	const auto material = PlanMultiLayerRecipeBatches(recipe, state.inputs, state.outputs, recipe.work_economy_ticks, 0, 0, recipe.power_j_per_batch, 0);
	if (material.batches == 0) { result.error = LayerRecipeCommitError::Refused; result.limiting = material.limiting; return result; }
	result.grant = {UtilityConsumerID{result.receipt.consumer}, GetUtilityPowerInterval()};
	// Future or exhausted opportunity history cannot be reset by demand gathering, cancellation or another allocation pass.
	if (result.receipt.last_interval > result.grant.interval) { result.error = LayerRecipeCommitError::Refused; result.limiting = MultiLayerRecipeLimit::Power; return result; }
	const uint64_t used = result.receipt.last_interval == result.grant.interval ? result.receipt.interval_paid_j : 0;
	result.energy_j = std::min({requested_j, *rate - used, recipe.power_j_per_batch - result.receipt.paid_j});
	result.error = result.energy_j == 0 ? LayerRecipeCommitError::Refused : LayerRecipeCommitError::None;
	result.limiting = result.energy_j == 0 ? MultiLayerRecipeLimit::WorkTime : MultiLayerRecipeLimit::None;
	return result;
}

/** @copydoc PrepareLayerRecipeWork */
LayerRecipeWorkProposal PrepareLayerRecipeWork(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory,
	LayerRecipeElectricalWorkParticipant &participant, uint64_t requested_j)
{
	auto result = PrepareLayerRecipeWorkDemand(recipe, inventory, participant, requested_j);
	if (result.error != LayerRecipeCommitError::None) return result;
	result.limiting = MultiLayerRecipeLimit::Power;
	// Only this dedicated consumer's current actual reservation on the same physical layer can finance the receipt; snapshot power and stale intervals grant no permission.
	if (!participant.GetElectricalGrant(result.grant) || result.grant.consumer.base() != result.receipt.consumer || result.grant.interval != GetUtilityPowerInterval() || result.receipt.last_interval > result.grant.interval) { result.error = LayerRecipeCommitError::Refused; return result; }
	const auto allocation = GetUtilityAllocation(result.grant.consumer);
	if (!allocation || allocation->layer != result.before.layer) { result.error = LayerRecipeCommitError::Refused; return result; }
	result.energy_j = std::min(result.energy_j, allocation->granted_energy_j);
	result.error = result.energy_j == 0 ? LayerRecipeCommitError::Refused : LayerRecipeCommitError::None;
	result.limiting = result.energy_j != 0 ? MultiLayerRecipeLimit::None : MultiLayerRecipeLimit::Power;
	return result;
}

/** Revalidate all actual material/work/receipt fields through the same pure proposal owner.
 * @param recipe Same immutable selected definition.
 * @param inventory Same actual physical view.
 * @param participant Same complete work participant.
 * @param proposal Original checked work quote.
 * @return Whether this exact actual opportunity and complete owner still agree.
 */
static bool RevalidateRecipeWork(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory,
	LayerRecipeElectricalWorkParticipant &participant, const LayerRecipeWorkProposal &proposal)
{
	const auto expected = PrepareLayerRecipeWork(recipe, inventory, participant, proposal.energy_j);
	return expected.error == LayerRecipeCommitError::None && proposal.error == LayerRecipeCommitError::None && proposal.limiting == expected.limiting && proposal.owner == &inventory && proposal.definition == &recipe &&
		EqualLayerRecipeInventory(proposal.before, expected.before) && proposal.receipt == expected.receipt && proposal.grant.consumer == expected.grant.consumer &&
		proposal.grant.interval == expected.grant.interval && proposal.energy_j == expected.energy_j;
}

/** Whole actual inventory revision and private physical progress settle with one native electrical debit. */
class RecipeElectricalWorkTransaction final : public UtilityMutationParticipant {
	const MultiLayerRecipe &recipe; ///< Original immutable material/work definition.
	const LayerRecipeInventoryView &inventory; ///< Same actual physical view used for preparation.
	LayerRecipeElectricalWorkParticipant &participant; ///< Actual complete owner, never a copied stock container.
	const LayerRecipeWorkProposal &work; ///< Original checked quote whose private receipt remains unchanged until commit.
public:
	/** @param recipe Exact selected definition. @param inventory Actual owner view. @param participant Its complete physical participant. @param work Original actual grant/work quote. */
	RecipeElectricalWorkTransaction(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory,
		LayerRecipeElectricalWorkParticipant &participant, const LayerRecipeWorkProposal &work) : recipe(recipe), inventory(inventory), participant(participant), work(work) {}
	/** @param proposal Exact original native electrical debit. @return Whether both complete work and the same physical owner still admit this quantity/interval. */
	bool Validate(const UtilityMutationProposal &proposal) const override
	{
		return proposal.quantity_j == this->work.energy_j && proposal.grant.consumer == this->work.grant.consumer && proposal.grant.interval == this->work.grant.interval &&
			RevalidateRecipeWork(this->recipe, this->inventory, this->participant, this->work) && this->participant.ValidateWork(this->work);
	}
	/** @param proposal Already checked electrical debit. @return Complete physical/work success or unchanged refusal; the shared utility owner settles its source in the same transaction. */
	bool Commit([[maybe_unused]] const UtilityMutationProposal &proposal) noexcept override
	{
		if (!this->participant.CommitWork(this->work)) return false;
		LayerRecipeWorkAccess::Earn(*this->participant.ElectricalWork(), this->work); return true;
	}
};

/** @copydoc CommitLayerRecipeWork */
LayerRecipeCommitError CommitLayerRecipeWork(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory,
	LayerRecipeElectricalWorkParticipant &participant, const LayerRecipeWorkProposal &proposal)
{
	if (!RevalidateRecipeWork(recipe, inventory, participant, proposal)) return LayerRecipeCommitError::Stale;
	const auto mutation = PrepareUtilityMutation(UtilityMutationKind::Consume, proposal.grant, proposal.energy_j);
	if (mutation.error != UtilityPowerError::None) return LayerRecipeCommitError::Refused;
	RecipeElectricalWorkTransaction transaction(recipe, inventory, participant, proposal);
	return CommitUtilityMutation(mutation, &transaction).error == UtilityPowerError::None ? LayerRecipeCommitError::None : LayerRecipeCommitError::Refused;
}

/** @copydoc CommitLayerRecipeInventoryAndPaidWork */
bool CommitLayerRecipeInventoryAndPaidWork(LayerRecipeCommitParticipant &participant, const LayerRecipeBatchProposal &proposal) noexcept
{
	auto *work = participant.ElectricalWork();
	if (proposal.electrical_work) {
		// Complete material settlement must debit the same original receipt once; another view, altered amount or exhausted sequence cannot consume physical stocks first.
		if (work == nullptr || participant.Inventory().ElectricalWork() != work || work->Capture() != *proposal.electrical_work ||
			proposal.paid_power_j == 0 || proposal.paid_power_j > work->Capture().paid_j || work->Capture().sequence >= UINT64_MAX - 1) return false;
	} else {
		// Ordinary whole-grant callers retain their existing path; missing receipt framing can never turn paid progress into a free material batch.
		if (work != nullptr || proposal.paid_power_j != 0) return false;
	}
	if (!participant.Commit(proposal)) return false;
	if (work != nullptr) LayerRecipeWorkAccess::Settle(*work, proposal.paid_power_j);
	return true;
}
