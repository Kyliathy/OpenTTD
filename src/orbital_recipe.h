/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_recipe.h Canonical orbital recipe buffers over the native module's sole packet stock.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C native metadata, scheduling and independently admitted persistence.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K original paid-work presentation.
 * The shared executor plans every material delta. This participant owns no replacement cargo registry or electrical account.
 */
#ifndef ORBITAL_RECIPE_H
#define ORBITAL_RECIPE_H

#include "layer_recipe_work.h"
#include "space_structure_type.h"
#include "multilayer_space_content.h"
#include "cargopacket.h"
#include "command_type.h"
#include <map>

/** Existing native structure lifetime owner; this participant borrows its generation-qualified module stock. */
struct SpaceStructure;
/** Detached native site metadata used for same-file processing joins, without granting physical ownership. */
struct SpaceStructureRecord;
/** Same native structure authority retains bounded spent history after empty removal. */
struct SpaceStructureRetirementHistory;

/** Existing SPST per-label stock-reference bound; processing cannot enlarge its native saved container. */
inline constexpr size_t MAX_ORBITAL_RECIPE_PACKET_REFERENCES = 64;

/** Optional pure whole-output admission veto, in addition to the actual native packet-pool bound.
 * @param packets Complete positive output-reference count proposed by the shared planner.
 * @return Whether this additional physical participant admits the allocation; true never bypasses the native pool's own check.
 */
using OrbitalRecipeAllocationAdmission = bool (*)(size_t packets);

/** @param kind Canonical static module kind. @return Whether this kind has a published orbital processing family. */
bool IsOrbitalRecipeKind(SpaceBuildKind kind);
/** @param kind Actual canonical processor kind. @return Its published default recipe identity, or the explicit empty identity. */
MultiLayerRecipeID GetDefaultOrbitalRecipe(SpaceBuildKind kind);
/** Resolve one existing content alternative without copying its numerical definition.
 * @param kind Actual canonical processor kind.
 * @param choice Explicit saved recipe identity; empty means the kind's published default.
 * @return Matching immutable Space recipe in that same published family, or nullptr.
 */
const MultiLayerManufacturingRecipe *GetOrbitalRecipeDefinition(SpaceBuildKind kind, MultiLayerRecipeID choice = {});
/** Resolve the retained union of one family's physical input/output buffer bounds.
 * @param kind Actual canonical processor kind.
 * @param label Content-owned cargo meaning, including labels from a checked file's own metadata.
 * @return Per-label whole-unit buffer cap, or zero for cargo outside this processor family.
 */
uint16_t GetOrbitalRecipeCargoCapacity(SpaceBuildKind kind, CargoLabel label);

/** Recipe choice, paid progress and historical material counters belong to one existing structure incarnation.
 * Physical inputs/products stay exclusively in that structure's OrbitalModule::stock.
 * Group10C contains the live private owner in the native module and joins detached transport to actual utility/structure identities before restore.
 */
struct OrbitalRecipeMetadata {
	SpaceStructureRef site; ///< Existing native structure identity and incarnation; this state issues no site or cargo identity.
	MultiLayerRecipeID choice{}; ///< Explicit canonical alternative; another family cannot reinterpret retained inputs/work.
	SpaceStructureRef source; ///< Explicit original adjacent panel, absent for unpowered or unconnected work.
	UtilityConsumerID consumer{UINT32_MAX}; ///< Existing ledger-owned processing consumer, distinct from Core/Dock service identities.
	uint64_t revision = 1; ///< Physical owner change stamp; stock, work and recipe mutations must advance it before another quote.
	uint64_t unpowered_work_ticks = 0; ///< Baseline work for genuinely unpowered profile recipes only.
	std::map<MultiLayerRecipeID, uint64_t> batches; ///< Preallocated family history; commit cannot allocate keys or invent another alternative.
	std::map<CargoLabel, uint64_t> consumed; ///< Exact whole-unit physical debits by original content meaning.
	std::map<CargoLabel, uint64_t> produced; ///< Exact whole-unit physical credits by original content meaning.
	std::map<MultiLayerRecipeID, uint64_t> residue_mass_sixteenths; ///< Exact canonical per-recipe process residue; this spent material is never freight or mine replenishment.
	bool enabled = false; ///< Explicit player start/resume permission; activation and load never silently start production.
	uint64_t last_native_tick = 0; ///< Last actual synchronized work tick plus one; zero is the explicit never-worked state.
	uint64_t paid_lifetime_j = 0; ///< Exact actual utility debits embodied in complete batches, pending work or explicitly discarded progress; never reusable electricity.
	uint64_t discarded_paid_j = 0; ///< Already spent electrical work abandoned by an explicit cancellation, never refunded or transferred.
	/** @param other Original metadata to compare. @return Whether every bounded ownership, progress and history field agrees. */
	bool operator==(const OrbitalRecipeMetadata &other) const = default;
};

/** Sole live private paid-work owner contained by the native processing module; it owns no second physical stock. */
struct OrbitalRecipeState : OrbitalRecipeMetadata {
	std::unique_ptr<LayerRecipeElectricalWork> electrical; ///< Sole private already-paid progress owner for an electrical recipe.
};

/** Detached processing transport; arithmetic and same-file module/utility joins must pass before a private work owner can be restored. */
struct OrbitalRecipeRecord : OrbitalRecipeMetadata {
	std::optional<LayerRecipeElectricalWorkRecord> electrical; ///< Read-only saved physical progress, never a usable grid balance.
	/** @param other Original transport to compare. @return Whether every metadata and paid-progress field agrees. */
	bool operator==(const OrbitalRecipeRecord &other) const = default;
};

/** Exact typed player controls; a pause retains pending work and explicit cancellation discards only that unfinished progress. */
enum class OrbitalRecipeControl : uint8_t { Start, Pause, Resume, Cancel, End };

/** Native processing wait reason; it grants no reservation or scheduling authority. */
enum class OrbitalRecipeWait : uint8_t { None, Disabled, Inactive, Input, Output, Work, Power, Allocation, Limit };

/** Bind explicit empty processing metadata to an existing completed frame, without granting activation, work, cargo or a consumer.
 * @param site Actual existing generation-qualified completed processor frame, including one still awaiting activation.
 * @return Disabled default metadata with preallocated bounded family history, or nullptr for a foreign/nonprocessor/incomplete identity.
 */
std::unique_ptr<OrbitalRecipeState> CreateEmptyOrbitalRecipeState(const SpaceStructure &site);

/** @param requests Existing common allocator's request set. Add only materially useful actual module demand; issue no accounts or energy. */
void AppendOrbitalRecipeRequests(std::vector<UtilityPowerRequest> &requests);
/** Advance enabled native processors once per actual synchronized opportunity through the original shared recipe/utility owners. */
void TickOrbitalRecipes();
/** @param site Actual processor incarnation. @return Pure current limiting reason, without issuing work, stock or reservations. */
OrbitalRecipeWait GetOrbitalRecipeWait(const SpaceStructure &site);
/** Inspect this original processor's already committed current native opportunity.
 * @param site Actual processor; the current actor must already own its stock query.
 * @return Existing paid/baseline work parity, or absence while idle, paused, stale or unavailable.
 */
std::optional<uint8_t> GetOrbitalRecipeVisualFrame(const SpaceStructure &site);
/** Connect the original typed native processing consumer, with no work or energy grant.
 * @param flags Estimate/execute; QueryCost is pure. @param site Actual owned operational processor. @param source Actual owned adjacent native panel.
 * @return Whole connection admission or unchanged refusal.
 */
CommandCost ConnectOrbitalRecipePower(DoCommandFlags flags, SpaceStructure &site, const SpaceStructure &source);
/** Retire an explicitly stopped empty-progress connection without refunding energy.
 * @param flags Estimate/execute; QueryCost is pure. @param site Actual owned processing module, including during a dependency outage.
 * @return Original-ledger retirement or unchanged refusal while work remains enabled/pending.
 */
CommandCost DisconnectOrbitalRecipePower(DoCommandFlags flags, SpaceStructure &site);
/** @param site Actual operational processor. @param state Its exact original recipe/source/consumer metadata. @return Whether the original owned adjacent native panel and dedicated consumer remain available. */
bool IsOrbitalRecipePowerConnectionValid(const SpaceStructure &site, const OrbitalRecipeState &state);

/** @return Exact detached metadata/private receipt copies from each actual native processing owner; no stock or utility balances are copied into another authority. */
std::vector<OrbitalRecipeRecord> CaptureOrbitalRecipes();
/** Admit complete same-file processing ownership, arithmetic and historical energy before native restore or copied load acceptance.
 * @param records Canonically ordered native processing metadata, one per processor module.
 * @param sites Same-file actual structure/module/stock ownership records.
 * @param utility Same-file original utility owner, including retained retired panels/consumers.
 * @param native_tick Same-file synchronized clock, never the active world's clock in copied validation.
 * @param history Same-file bounded spent structure authority, including removed processor receipts.
 * @return Whether every required owner, private progress and material/energy history join admits the complete family.
 */
bool ValidateOrbitalRecipeRecords(std::span<const OrbitalRecipeRecord> records, std::span<const SpaceStructureRecord> sites,
	const UtilityPowerState &utility, uint64_t native_tick, const SpaceStructureRetirementHistory &history);
/** @return Full actual native processing owner/history admission; no private work or stock is created by this check. */
bool ValidateOrbitalRecipes();
/** Clear only unpublished processing transport during world teardown/load failure; it supplies no runtime work. */
void ResetOrbitalRecipeLoadStaging();
/** @return Complete native private-owner restoration after actual structures/utility admission, or explicit disabled empty legacy migration. */
bool AcceptLoadedOrbitalRecipes();
/** Validate required presence and independent same-file processing metadata after all copied native families have decoded. */
void ValidateOrbitalRecipeLoadPreview();
/** @return Stable complete native processing/history digest; it owns no cargo, source or scheduling authority. */
uint64_t ComputeOrbitalRecipeDigest();
/** Price bounded spent retirement history through the original content definitions.
 * @param history Same native structure authority's already consumed recipe/discarded-work aggregates.
 * @return Exact historical electrical cost after material/residue overflow admission, or absence for malformed history; never a usable energy grant.
 */
std::optional<uint64_t> GetOrbitalRecipeRetirementEnergy(const SpaceStructureRetirementHistory &history);

/** Bind explicit processing metadata to actual native module/source identities, without creating stock, work or a utility identity.
 * @param site Actual operational existing processor incarnation.
 * @param choice Explicit canonical alternative, or empty for its default.
 * @param source Existing adjacent owned original panel; absent until an electrical connection is supplied.
 * @param consumer Existing native reservation owner; invalid until connected, and always invalid for an unpowered recipe.
 * @return Bounded initialized metadata, or nullptr for any unsupported binding/profile/date.
 */
std::unique_ptr<OrbitalRecipeState> CreateOrbitalRecipeState(SpaceStructureRef site, MultiLayerRecipeID choice = {},
	SpaceStructureRef source = {}, UtilityConsumerID consumer = UtilityConsumerID{UINT32_MAX});

/** Actual module packet inventory and complete physical participant for the single shared batch/work executors. */
class OrbitalRecipeInventory final : public LayerRecipeInventoryView, public LayerRecipeElectricalWorkParticipant {
	OrbitalRecipeState &state; ///< Same actual metadata owner for this one synchronous prepare/commit action.
	OrbitalRecipeAllocationAdmission allocation_admission; ///< Optional additional pure refusal; actual native packet capacity remains mandatory.
	/** Temporary ownership of a fully allocated future product; only successful material commit appends it to native module stock. */
	struct Product {
		CargoType cargo = INVALID_CARGO; ///< Resolved native cargo compartment in the same live module.
		uint32_t amount = 0; ///< Original checked whole credit, independently compared before append.
		std::unique_ptr<CargoPacket> packet; ///< Existing actual native packet; failure destroys it without consuming any input.
	};
	std::vector<Product> products; ///< Complete output allocation before any input or paid work can be consumed.
	/** @return Same existing native owned operational module, or nullptr after loss, foreign control, changed incarnation or unsupported choice. */
	SpaceStructure *Site() const;
	/** @param proposal Exact shared complete material quote. @param allocated Whether every product must already have its actual native packet. @return Whether all real stocks/caps/history and optional product allocations still admit the whole batch. */
	bool ValidatePhysicalBatch(const LayerRecipeBatchProposal &proposal, bool allocated) const;
public:
	/** @param state Actual generation-qualified metadata; the native module remains sole physical stock owner. @param allocation_admission Optional additional pure whole-output veto, including isolated allocation-refusal verification. */
	explicit OrbitalRecipeInventory(OrbitalRecipeState &state, OrbitalRecipeAllocationAdmission allocation_admission = nullptr) : state(state), allocation_admission(allocation_admission) {}
	/** @return Pure actual packet totals, bounded role buffers, work and current real grant projection. */
	LayerRecipeInventorySnapshot Capture() const override;
	/** @return This exact actual module view used for preparation. */
	const LayerRecipeInventoryView &Inventory() const override { return *this; }
	/** @return Same optional original noncopyable paid-work owner, never a copy of a scalar receipt. */
	const LayerRecipeElectricalWork *ElectricalWork() const override { return state.electrical.get(); }
	/** @return Same mutable paid-work owner, debited only by the shared transaction. */
	LayerRecipeElectricalWork *ElectricalWork() override { return state.electrical.get(); }
	/** @param grant Receives this exact module's actual current native processing reservation. @return Whether original source/consumer ownership still admits it. */
	bool GetElectricalGrant(UtilityGrantID &grant) const override;
	/** Preallocate every real output before material mutation; a query only checks actual bounded room.
	 * @param proposal Original checked shared material batch quote.
	 * @param execute Whether to allocate actual native output packets; false changes no pool, stock, work, history or money.
	 * @return Whether the complete output set fits, preserving all inputs/paid work on every refusal.
	 */
	bool PrepareOutputs(const LayerRecipeBatchProposal &proposal, bool execute);
	/** Check future work/history representability without spending electricity or advancing a native opportunity.
	 * @param energy_j Actual proposed future electrical work; zero for a genuinely unpowered baseline opportunity.
	 * @return Whether this same actual module can record one future original batch and its spent work without overflow.
	 */
	bool CanRecordWork(uint64_t energy_j) const;
	/** @param proposal Original shared material quote. @return Complete physical packet/history admission, including all already allocated output packets. */
	bool Validate(const LayerRecipeBatchProposal &proposal) const override;
	/** @param proposal Already revalidated shared material quote. @return Complete physical packet/history commit; the shared executor alone debits paid work. */
	bool Commit(const LayerRecipeBatchProposal &proposal) noexcept override;
	/** @param proposal Original real work quote. @return Whether this same actual stock/work owner still admits one opportunity. */
	bool ValidateWork(const LayerRecipeWorkProposal &proposal) const override;
	/** @param proposal Checked actual work opportunity. @return Physical revision advancement only; the shared API alone earns private paid progress. */
	bool CommitWork(const LayerRecipeWorkProposal &proposal) noexcept override;
};

#endif /* ORBITAL_RECIPE_H */
