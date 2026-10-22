/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_industry.h Layer industry admission and finite mine ownership. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md */

#ifndef MULTILAYER_INDUSTRY_H
#define MULTILAYER_INDUSTRY_H

#include "excavation.h"
#include "multilayer_recipe.h"
#include "industry_type.h"
#include "command_type.h"
#include "station_supply.h"

struct Industry;

/** Stable package-local mine definitions; runtime IndustryTypes are resolved through the existing IIDS owner. */
static constexpr uint16_t LAYER_MINE_LOCAL_BASE = 0x8100;
/** Stable package-local Surface processor definitions, ordered by their published primary recipe identity. */
static constexpr uint16_t LAYER_PROCESSOR_LOCAL_BASE = 0x8200;

/** Explicit behavior authority; neither a translated name nor substitute artwork establishes capability. */
struct LayerIndustryCapability {
	MultiLayerCapabilities layers; ///< Only these world layers may host this definition.
	CargoLabel resource = CT_INVALID; ///< Finite geological/output meaning for a mine.
	MultiLayerRecipeID recipe{}; ///< Content-owned recipe identity, distinct from a runtime industry slot.
	TimerGameCalendar::Year introduction_year{0}; ///< Calendar introduction, in addition to the shared Deep gate.
	uint16_t output_capacity = 0; ///< Accountable output bound in the cargo's packet units.
	uint8_t equipment_cost_factor = 1; ///< Deep machinery construction multiplier; excavation is charged separately.
	std::string_view asset_id{}; ///< Exact r2mlvs13 logical art family; planned art grants no admission or reserve authority.
};

/** Stable admission failure; no failed estimate allocates an industry, claims geology or clears a tile. */
enum class LayerIndustryError : uint8_t {
	None, InvalidDefinition, WrongLayer, Unavailable, InvalidCompany, InvalidDeposit,
	Claimed, Depleted, NoSeamContact, Footprint, Access, Occupied, Capacity, Funds,
};

/** Explicit player-selected seam and layout; the owner derives every machinery and loading cell. */
struct LayerMineRequest {
	TileIndex tile = INVALID_TILE; ///< World-tile machinery anchor; never relative to the camera.
	IndustryType type = IT_INVALID; ///< Enabled first-party runtime definition resolved by IIDS.
	size_t layout = 0; ///< Selected installed layout, checked before any index access.
	GeologyDepositID deposit{UINT32_MAX}; ///< Explicit geological selection; no random prospecting replacement.
	CompanyID company = CompanyID::Invalid(); ///< Actual claiming company; deity/editor does not fabricate ownership.
};

/** Pure whole-site admission result, prepared before clearing or allocating any identity. */
struct LayerMineAdmission {
	LayerIndustryError error = LayerIndustryError::InvalidDefinition; ///< First deterministic refusal.
	TileIndex failure_tile = INVALID_TILE; ///< Exact refused cell, when geometry/occupancy fails.
	std::vector<TileIndex> machinery; ///< Actual constructed layout cells; never special water-check cells.
	std::vector<TileIndex> loading; ///< Two-wide, two-long loading approach below the machinery footprint.
	uint64_t expected_reserve = 0; ///< Geological stamp; a stale quote cannot authorize construction.
	Money cost = 0; ///< Complete machinery and clearing quote; excavation is already paid by its own owner.
};

/**
 * Resolve an explicitly supported enabled native definition under the accepted first-party profile.
 * @param type Runtime industry identity; invalid, original and ordinary NewGRF definitions have no mine authority.
 * @return Immutable capability, or nullptr; substitute graphics do not participate in this decision.
 */
const LayerIndustryCapability *GetLayerIndustryCapability(IndustryType type);

/**
 * Install the ten native definitions and their 3x3 machinery layouts after ordinary GRF polishing.
 * @param activated Exact accepted first-party package descriptors; no external or mismatched package is adapted.
 * @return Whether all definitions resolve through existing IIDS/TIDS owners; installation creates no mines or reserves.
 */
bool InitialiseLayerMineDefinitions(const GRFConfigList &activated);

/**
 * Resolve one geological resource's installed native definition through the existing industry mapping owner.
 * @param resource Content-owned mineral cargo label, never a runtime cargo slot or translated name.
 * @return Enabled exact native industry type, or IT_INVALID when unknown/uninstalled/incompatible.
 */
IndustryType GetLayerMineIndustryType(CargoLabel resource);

/**
 * Distinguish finite geological mines from supported Surface processors at command/UI boundaries.
 * @param type Actual runtime industry identity resolved through IIDS.
 * @return Whether the whitelisted capability has a finite mineral resource, requiring explicit seam funding.
 */
bool IsLayerMineDefinition(IndustryType type);

/**
 * Check the shared operational technology rule for a whitelisted capability.
 * @param capability Immutable capability returned by the definition owner.
 * @param layer Actual requested operating layer.
 * @param policy Explicit accepted gameplay policy; scenario authoring does not grant earlier operation.
 * @param date Synchronized calendar date, never economy time or a render counter.
 * @return Whether funding/activation/production may operate on this layer/date.
 */
bool IsLayerIndustryTechnologyAvailable(const LayerIndustryCapability &capability, MapLayer layer, const MultiLayerProfileSelection &policy, TimerGameCalendar::Date date);

/**
 * Check the shared placement boundary before commands or automatic generation enter callbacks/clearing.
 * @param tile Explicit world anchor.
 * @param type Runtime definition to place.
 * @return Permission or a command refusal; Surface legacy policy is preserved.
 */
CommandCost CheckLayerIndustryConstruction(TileIndex tile, IndustryType type);

/** @param industry Existing placed facility. @return Whether it owns at least one machinery tile and all of its machinery is physically complete. */
bool IsLayerIndustryComplete(const Industry &industry);

/**
 * Validate the entire excavated mine site against the sole accepted geological authority.
 * @param request Explicit selected seam, company, definition and layout; footprint is derived by this owner.
 * @return Pure prepared admission, or unchanged refusal; no pool, tile, reserve, cash or RNG mutation.
 */
LayerMineAdmission ValidateMineDepositClaim(const LayerMineRequest &request);

/** Mine lifecycle; exhausted mines retain their binding/output until the player removes the empty industry. */
enum class LayerMinePhase : uint8_t { Active, Depleted, Abandoned };

/** Saved industry-to-geology binding and historical counters; physical output remains exclusively in Industry::produced. */
struct LayerMineState {
	IndustryID industry = IndustryID::Invalid(); ///< Existing pool identity, never a local substitute ID.
	GeologyDepositID deposit{UINT32_MAX}; ///< The sole geological reserve owner.
	CompanyID company = CompanyID::Invalid(); ///< Exclusive mining company, independent of original founder history.
	LayerMinePhase phase = LayerMinePhase::Active; ///< Depletion pauses extraction while preserving stored output.
	uint64_t work_ticks = 0; ///< Saved synchronized available work, bounded by the active recipe.
	uint64_t extracted = 0; ///< Historical reserve-to-output units; never a second cargo inventory.
	uint64_t dispatched = 0; ///< Historical actual station supply, counted once.
	uint64_t revision = 1; ///< Changes invalidate stale production/dispatch proposals.
	uint32_t work_fraction = 0; ///< Remaining fraction of one work tick, with the recipe's output-per-batch as denominator.
	uint16_t scale_fraction = 0; ///< Saved work-rate numerator modulo 200; cargo scale/recession act before reserve extraction.
	/** @param other Candidate restored record. @return Exact equality of all authoritative saved fields. */
	bool operator==(const LayerMineState &other) const = default;
};

/** @return Mine bindings in increasing real industry order; geology/output are saved by their own owners. */
std::vector<LayerMineState> CaptureLayerMines();
/** @param rows Parsed unpublished bindings; no claims or output are created by staging. */
void StageLoadedLayerMines(std::vector<LayerMineState> rows);
/** @return Whether every binding and native mine agrees with actual rebuilt industries/geology; publish only a complete valid family. */
bool AcceptLoadedLayerMines();
/** Clear unpublished mine wire tables during whole-world initialization. */
void ResetLayerMineLoadStaging();
/** Validate independent preview header/table/count/feature consistency without activating bindings. */
void ValidateLayerMineLoadPreview();
/** @return Canonical binding/counter digest; physical reserve and cargo remain in their separate owner diagnostics. */
uint64_t ComputeLayerMineDigest();

/**
 * Read a mine binding without obtaining mutable geology or another output inventory.
 * @param industry Stable pool identity, including deleted/invalid references.
 * @return Read-only binding, or nullptr; valid only until the next mine-state mutation.
 */
const LayerMineState *GetLayerMineState(IndustryID industry);

/** Derived player-facing mine availability; never a saved phase, new identity or production authority. */
enum class LayerMineOperationalStatus : uint8_t { MissingState, Construction, Unavailable, Abandoned, Depleted, Paused, OutputFull, AwaitPickup, Operating, Stopped };

/**
 * Read actual mine construction, date, reserve, output and service without advancing work or committing supply.
 * @param industry Existing native industry whose own physical layer defines availability.
 * @return Derived current status; missing/restoring records never present guessed reserve as an operating mine.
 */
LayerMineOperationalStatus GetLayerMineOperationalStatus(const Industry &industry);

/**
 * Resolve the exclusive physical mine binding for a seam, independently of company claim sentinels.
 * @param deposit Actual finite geological identity.
 * @return Existing industry's identity, or invalid; abandoned bindings still exclude a second mine.
 */
IndustryID FindLayerMineIndustry(GeologyDepositID deposit);

/**
 * Move native mine ownership at the common company takeover/removal boundary.
 * @param old_company Actual company being replaced/removed.
 * @param new_company Existing new owner, or invalid for abandonment.
 */
void ChangeLayerIndustryOwnership(CompanyID old_company, CompanyID new_company);

/**
 * Reserve a binding and acquire its geological claim before already admitted non-failing tile creation.
 * @param industry Newly allocated zero-footprint industry; no tiles have been written.
 * @param request Exact fully admitted player selection.
 * @param admission Complete prepared geometry/reserve/cost result.
 * @return Complete binding/claim or unchanged refusal; caller deletes an unbound zero-footprint industry on refusal.
 */
LayerIndustryError BindLayerMineIndustry(Industry &industry, const LayerMineRequest &request, const LayerMineAdmission &admission);

/**
 * Release the binding/claim of an empty industry being demolished, preserving geology.
 * @param industry Existing mine pool identity; non-mines are ignored.
 */
void ReleaseLayerMineIndustry(IndustryID industry);

/**
 * Refuse demolition while finite output still needs an accountable station handoff.
 * @param industry Existing industry whose footprint is being cleared.
 * @return Whether demolition would destroy stored finite cargo.
 */
bool LayerMineHasStoredOutput(const Industry &industry);

/** Clear side state during whole-world initialization, without releasing claims in the old world. */
void ResetLayerIndustryState();

/** Exact reserve-to-output proposal/receipt; no fractional tonnes or output inventory is hidden here. */
struct LayerMineProduction {
	LayerIndustryError error = LayerIndustryError::None; ///< Refusal or successful bounded work; a zero amount never debits reserve.
	uint32_t amount = 0; ///< Whole units admitted to the industry's existing output slot.
	uint64_t work_ticks = 0; ///< Whole part of synchronized work charged, separate from geological excavation work.
	uint32_t work_fraction = 0; ///< Exact fractional work debit, with the mine recipe's output-per-batch as denominator.
};

/**
 * Produce finite mineral output through the sole industry/geology transaction owners.
 * @param industry Existing fully bound mine, never a stand-in inventory or unplaced candidate.
 * @param economy_ticks Synchronized work allowance supplied once by OnTick_Industry; paused mines accrue none.
 * @param execute False prepares a pure estimate, true revalidates and commits reserve/output/history/work together.
 * @return Exact bounded whole output and rational work debit, or unchanged refusal.
 */
LayerMineProduction ProduceLayerMineCargo(Industry &industry, uint32_t economy_ticks, bool execute);

/** One transient exact supply proposal; source work/reserve are unaffected by station dispatch. */
struct LayerMineDispatchTicket {
	IndustryID industry = IndustryID::Invalid(); ///< Actual source identity.
	uint64_t revision = 0; ///< Expected source's saved change stamp; a retry cannot debit the same output twice.
	uint16_t waiting = 0; ///< Expected physical output balance.
	uint16_t transported = 0; ///< Expected monthly transported history, checked before narrow addition.
	ExactStationSupplyTicket station; ///< Pure bounded station/packet/service admission.
};

/**
 * Prepare finite output supply for the best eligible actual same-layer station.
 * @param industry Existing bound mine; depleted mines may still dispatch their stored output.
 * @return Pure bounded ticket; no eligible service, storage or packet room produces a zero-amount ticket.
 */
LayerMineDispatchTicket PrepareLayerMineDispatch(const Industry &industry);

/**
 * Revalidate and commit exactly the station owner's receipt and matching physical mine debit.
 * @param industry Same existing mine that prepared the quote.
 * @param ticket Whole transient quote; changed source/station/service and repeated quotes are refused unchanged.
 * @return Exact whole output units moved, or zero; reserve and production history never change during dispatch.
 */
uint32_t CommitLayerMineDispatch(Industry &industry, const LayerMineDispatchTicket &ticket);

/** @param industry Existing bound mine whose tile loop requests supply. @return Whole committed station units, or zero with stock retained. */
uint32_t DispatchLayerMineCargo(Industry &industry);

/** Read-only copy of an existing facility's actual stock/work; it is not a physical cargo or utility inventory. */
struct LayerRecipeInventorySnapshot {
	MultiLayerRecipeID recipe{}; ///< Saved owner-selected alternative; unknown/invalid choice cannot run a batch.
	MapLayer layer = MapLayer::Invalid; ///< Actual operating layer; no camera or coincident catchment permission.
	uint64_t revision = 0; ///< Owner's saved change stamp, including stock, work and recipe changes.
	bool active = false; ///< Actual construction/date/pause admission; inactive buffers are preserved.
	std::vector<MultiLayerRecipeStock> inputs; ///< Delivered packet inventory, distinct from produced stock/reserve.
	std::vector<MultiLayerRecipeStock> outputs; ///< Existing bounded product slots; every output must fit.
	uint64_t work_ticks = 0; ///< Existing synchronized saved work balance.
	uint64_t heat_j = 0; ///< Proposed authoritative grant quantity, never extracted mineral or freight; token integration belongs to Group 13.
	uint64_t power_j = 0; ///< Proposed authoritative electric grant quantity; this view cannot allocate or consume a grid grant.
};

/**
 * Read-only inventory boundary implemented by the actual Industry or later SpaceStructure owner.
 * A proposal borrows this object's identity within one simulation action. It is not saved,
 * and this interface allocates no industry/structure/cargo/utility identities.
 */
class LayerRecipeInventoryView {
public:
	/** Destroy through the public read-only owner boundary. */
	virtual ~LayerRecipeInventoryView() = default;
	/** @return A pure snapshot of the owner's actual inventory/recipe/work/grant view; no stock, RNG or counters change. */
	virtual LayerRecipeInventorySnapshot Capture() const = 0;
};

/** One checked input debit or output credit, in the selected content label's actual packet unit. */
struct LayerRecipeCargoDelta {
	CargoLabel label = CT_INVALID; ///< Content meaning resolved by the owner against its actual slots.
	uint32_t amount = 0; ///< Exact positive whole quantity for all complete batches.
	/** @param other Proposed delta to compare. @return Whether both meaning and quantity match. */
	bool operator==(const LayerRecipeCargoDelta &other) const = default;
};

/** Bounded transient batch proposal, with enough owner state to reject retries, stale stock and foreign participants. */
struct LayerRecipeBatchProposal {
	const LayerRecipeInventoryView *owner = nullptr; ///< Borrowed actual owner; null/default cannot authorize mutation and is never serialized.
	const MultiLayerRecipe *definition = nullptr; ///< Borrowed immutable published recipe; it is neither copied into a registry nor serialized in a proposal.
	LayerRecipeInventorySnapshot before; ///< Pure expected state; this copied view grants no physical custody.
	MultiLayerRecipeBatchPlan plan; ///< Existing all-input planner's bounded count and deterministic limit.
	std::vector<LayerRecipeCargoDelta> inputs; ///< Exact complete input debits, never independent-input multipliers.
	std::vector<LayerRecipeCargoDelta> outputs; ///< Exact complete output credits, all of which must fit.
	uint64_t work_ticks = 0; ///< Whole work debit for the complete batch count.
	uint64_t heat_j = 0; ///< Proposed grant use; actual token scheduling/allocation remains with its utility owner.
	uint64_t power_j = 0; ///< Separate electrical grant use, never cargo payment or mine reserve.
};

/** Ledger-owned reservation identity; defined by utility_power.h, never derived from an industry ID. */
struct UtilityGrantID;

/** Actual inventory commit participant; a failed commit must preserve every input, output, work balance and supplied grant. */
class LayerRecipeCommitParticipant {
public:
	/** Destroy through the shared complete-transaction boundary. */
	virtual ~LayerRecipeCommitParticipant() = default;
	/** @return The same actual inventory view used to prepare the proposal, never a proxy for another physical owner. */
	virtual const LayerRecipeInventoryView &Inventory() const = 0;
	/**
	 * Expose an actual electrical reservation for this exact inventory owner.
	 * @param grant Receives the ledger-issued current consumer/epoch identity only on success.
	 * @return Whether this owner has an actual grant; default refuses scalar-only power and allocates no utility identity.
	 */
	virtual bool GetElectricalGrant([[maybe_unused]] UtilityGrantID &grant) const { return false; }
	/**
	 * Revalidate the complete actual owner and its optional authoritative grant participant.
	 * @param proposal Exact bounded all-input/output/work proposal.
	 * @return Whether all participants still accept the whole transaction; validation must change nothing.
	 */
	virtual bool Validate(const LayerRecipeBatchProposal &proposal) const = 0;
	/**
	 * Commit the whole already revalidated inventory/work transaction without partial-failure writes.
	 * @param proposal Exact checked debits/credits; optional supplied grants require their owner's atomic participant.
	 * @return Complete success, or unchanged refusal; true advances the owner's saved revision so retries cannot debit twice.
	 */
	virtual bool Commit(const LayerRecipeBatchProposal &proposal) noexcept = 0;
};

/** Commit outcome distinct from a recipe feasibility pause; neither stale nor refused proposals may consume anything. */
enum class LayerRecipeCommitError : uint8_t { None, InvalidProposal, WrongOwner, Stale, Refused };

/** Exact complete commit receipt, without another stock or payment authority. */
struct LayerRecipeBatchReceipt {
	LayerRecipeCommitError error = LayerRecipeCommitError::InvalidProposal; ///< Mutation refusal or complete success.
	uint16_t batches = 0; ///< Actually committed complete batches; zero on every refusal.
};

/**
 * Prepare complete bounded batches from the owner's selected published all-input recipe.
 * @param recipe Immutable content-owned selected record; its identity must match the owner's saved alternative.
 * @param inventory Actual read-only inventory owner, which may be an Industry or later SpaceStructure.
 * @return Pure bounded proposal; reserve extraction is separate and zero feasible batches consume nothing.
 * Declared energy quantities alone authorize no runtime grid mutation.
 */
LayerRecipeBatchProposal PrepareLayerRecipeBatch(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory);

/**
 * Revalidate and commit all selected inputs, every output and work through one actual participant.
 * @param recipe Same immutable published record used for preparation; no independent-input production path is permitted.
 * @param inventory Same real inventory owner that prepared the proposal.
 * @param participant Actual non-failing complete commit boundary, including any supplied authoritative utility participant.
 * @param proposal Whole transient proposal; foreign, stale, changed or repeated proposals are refused.
 * @return Exact complete receipt or unchanged refusal; scalar-only energy refuses, and electricity requires this actual owner's ledger reservation.
 */
LayerRecipeBatchReceipt CommitLayerRecipeBatch(const MultiLayerRecipe &recipe, const LayerRecipeInventoryView &inventory, LayerRecipeCommitParticipant &participant, const LayerRecipeBatchProposal &proposal);

#endif /* MULTILAYER_INDUSTRY_H */
