/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file layer_recipe_work.h Shared recipe-bound progress earned through the sole actual electrical ledger.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md shared complete-batch contract
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10 staged electrical work
 * Receipts record work already paid. They cannot supply, transfer, allocate or refund electricity.
 */
#ifndef LAYER_RECIPE_WORK_H
#define LAYER_RECIPE_WORK_H

#include "multilayer_industry.h"
#include "utility_power.h"
#include <memory>

/** Internal shared transaction access; features cannot directly increase or debit private paid-work fields. */
struct LayerRecipeWorkAccess;

/** Noncopyable physical paid-work owner; only shared transactions can increase or settle its pending progress.
 * A feature binds one receipt to its existing utility consumer. Adapters borrow that same object rather than copying it.
 */
class LayerRecipeElectricalWork {
	LayerRecipeElectricalWorkRecord state; ///< One recipe's already-paid pending work and exact opportunity/retry provenance.
	friend struct LayerRecipeWorkAccess;
public:
	/** @param recipe Exact published electrical alternative. @param consumer Actual dedicated existing native utility consumer. */
	LayerRecipeElectricalWork(const MultiLayerRecipe &recipe, UtilityConsumerID consumer);
	/** Prevent two physical owners from independently settling a copied receipt. */
	LayerRecipeElectricalWork(const LayerRecipeElectricalWork &) = delete;
	/** @param other Receipt which cannot be copied into another physical owner. @return No copy assignment is available. */
	LayerRecipeElectricalWork &operator=(const LayerRecipeElectricalWork &other) = delete;
	/** @param other Receipt whose sole physical owner is transferred, retaining every paid-work field. */
	LayerRecipeElectricalWork(LayerRecipeElectricalWork &&other) noexcept;
	/** @param other Receipt whose sole physical owner is transferred. @return This transferred physical owner. */
	LayerRecipeElectricalWork &operator=(LayerRecipeElectricalWork &&other) noexcept;
	/** @return Read-only native serialization/revalidation fields; this copy grants no work or electricity. */
	const LayerRecipeElectricalWorkRecord &Capture() const { return this->state; }
	/** @return Whole physical work ticks derived from the immutable paid rate; invalid bindings expose no progress. */
	uint64_t WorkTicks() const;
	/** Restore already paid physical progress after the native feature has admitted its complete same-file ownership joins.
	 * @param recipe Original immutable selected definition, never a definition supplied by copied metadata.
	 * @param record Source-independent validated saved progress; this alone cannot establish module or consumer uniqueness.
	 * @param layer Exact native owner's physical layer, also required by the actual restored utility consumer.
	 * @return Private restored owner after actual ledger epoch/current-use admission, or nullptr without mutation. The caller must first validate native incarnation, unique consumer and historical material/energy joins.
	 */
	static std::unique_ptr<LayerRecipeElectricalWork> Restore(const MultiLayerRecipe &recipe, const LayerRecipeElectricalWorkRecord &record, MapLayer layer);
	/** Explicitly abandon unfinished physical work without refunding electricity or renewing this opportunity.
	 * @return Exact discarded paid joules for the native historical ledger, or absence when the receipt sequence cannot represent cancellation. Original interval/use metadata always survives.
	 */
	std::optional<uint64_t> Discard();
};

/** Pure one-opportunity work demand/quote; actual commit additionally requires the original current native allocation and complete revalidation. */
struct LayerRecipeWorkProposal {
	LayerRecipeCommitError error = LayerRecipeCommitError::InvalidProposal; ///< Invalid/changed owner or unavailable actual work admission.
	MultiLayerRecipeLimit limiting = MultiLayerRecipeLimit::InvalidDefinition; ///< Shared material, work or actual-power reason; callers need no private substitute planner.
	const LayerRecipeInventoryView *owner = nullptr; ///< Exact borrowed physical inventory view for this one action.
	const MultiLayerRecipe *definition = nullptr; ///< Exact borrowed immutable selected recipe.
	LayerRecipeInventorySnapshot before; ///< Complete expected physical stock/work/revision view.
	LayerRecipeElectricalWorkRecord receipt; ///< Exact original private receipt captured without mutation.
	UtilityGrantID grant; ///< Original existing consumer/current epoch; pre-allocation demand carries only this target identity and confers no reservation.
	uint64_t energy_j = 0; ///< Proposed useful work, bounded by remaining batch/opportunity; an allocated work quote is additionally capped by the actual native grant.
};

/** Actual physical owner participates in shared work as well as complete material-batch settlement.
 * CommitWork and the inherited material Commit update physical stock/revision only. The shared transaction alone changes the private paid-work receipt afterward.
 */
class LayerRecipeElectricalWorkParticipant : public LayerRecipeCommitParticipant {
public:
	/** @param proposal Exact original stock/work/receipt/opportunity proposal. @return Whether the complete owner still admits work, without mutation. */
	virtual bool ValidateWork(const LayerRecipeWorkProposal &proposal) const = 0;
	/** @param proposal Already revalidated work action. @return Complete owner success or unchanged refusal; success advances its physical revision, while the shared API updates the receipt. */
	virtual bool CommitWork(const LayerRecipeWorkProposal &proposal) noexcept = 0;
};

/** Price bounded material-feasible electrical work before the one common allocator publishes grants.
 * @param recipe Original immutable selected definition.
 * @param inventory Same actual physical inventory/work view.
 * @param participant Same actual whole physical participant.
 * @param requested_j Positive desired work, capped by this original receipt's remaining batch and current opportunity.
 * @return Pure demand with the shared material limiting reason. It grants no energy; actual work commit still requires the original current native allocation and complete revalidation.
 */
LayerRecipeWorkProposal PrepareLayerRecipeWorkDemand(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory,
	LayerRecipeElectricalWorkParticipant &participant, uint64_t requested_j);

/** Validate source-independent serialized progress arithmetic; native owners separately join their real consumer/module incarnation.
 * @param recipe Exact original selected immutable definition.
 * @param record Original physical paid-work fields, including immutable rate and actual opportunity bounds.
 * @return Whether the receipt can describe at most one pending batch without overflow, fabricated fractional work or renewed opportunity.
 */
bool ValidateLayerRecipeElectricalWork(const MultiLayerRecipe &recipe, const LayerRecipeElectricalWorkRecord &record);

/** Prepare work from actual input/output headroom and the same owner's current electrical reservation.
 * @param recipe Exact selected immutable alternative; staged work currently requires a positive integral joule-per-tick rate.
 * @param inventory Actual physical stock/work view exposing its same noncopyable receipt.
 * @param participant Complete physical owner and its original ledger grant supplier.
 * @param requested_j Maximum actual joules requested in this action; split grants share one opportunity's cap.
 * @return Pure bounded proposal; refusal changes no stock, work, receipt, utility grant or RNG.
 */
LayerRecipeWorkProposal PrepareLayerRecipeWork(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory,
	LayerRecipeElectricalWorkParticipant &participant, uint64_t requested_j);

/** Revalidate and commit the real source debit with physical work through the shared utility owner.
 * @param recipe Same immutable alternative used for preparation.
 * @param inventory Same actual physical inventory view.
 * @param participant Same complete physical work owner.
 * @param proposal Original complete checked work/grant/opportunity proposal.
 * @return Complete success or unchanged stale/refused action; no material is consumed until the existing complete-batch executor commits.
 */
LayerRecipeCommitError CommitLayerRecipeWork(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory,
	LayerRecipeElectricalWorkParticipant &participant, const LayerRecipeWorkProposal &proposal);

/** Internal complete-batch physical commit hook used only after the shared executor revalidates all deltas and owners.
 * @param participant Same actual complete physical inventory participant.
 * @param proposal Already revalidated original complete-batch proposal, with its optional exact paid-work receipt.
 * @return Complete physical/receipt success or unchanged refusal; this hook never allocates or consumes electricity.
 */
bool CommitLayerRecipeInventoryAndPaidWork(LayerRecipeCommitParticipant &participant, const LayerRecipeBatchProposal &proposal) noexcept;

/** Revalidate the original shared complete-batch quote without invoking any physical callback.
 * @param recipe Same immutable selected definition.
 * @param inventory Same actual physical owner view.
 * @param participant Same complete physical participant.
 * @param proposal Original complete material/work quote.
 * @return Shared owner, staleness and all-delta admission, before output allocation or any physical mutation.
 */
LayerRecipeCommitError ValidateLayerRecipeBatchProposal(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory,
	LayerRecipeCommitParticipant &participant, const LayerRecipeBatchProposal &proposal);

#endif /* LAYER_RECIPE_WORK_H */
