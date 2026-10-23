/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_industry_processor.h Native delivered-stock processing and saved recipe choice. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Groups 10/16 */

#ifndef LAYER_INDUSTRY_PROCESSOR_H
#define LAYER_INDUSTRY_PROCESSOR_H

#include "multilayer_industry.h"

/** Saved processing counters and choice; all physical input/output stock remains in the existing Industry object. */
struct LayerProcessorState {
	IndustryID industry = IndustryID::Invalid(); ///< Actual inventory owner.
	CompanyID controller = CompanyID::Invalid(); ///< Funding company controls the recipe; this is not another cargo owner.
	MultiLayerRecipeID recipe{}; ///< One frozen alternative shared by the whole facility.
	uint64_t work_ticks = 0; ///< Bounded synchronized work shared across alternative choices.
	uint16_t scale_fraction = 0; ///< Rate/recession work remainder modulo 200.
	uint64_t revision = 1; ///< Invalidates quotes after any stock, work or choice change.
	uint64_t dispatched = 0; ///< Historical actual packet supply, not a second product inventory.
	std::map<MultiLayerRecipeID, uint64_t> batches; ///< Preallocated alternative counters; commits allocate no new keys.
	std::map<CargoLabel, uint64_t> consumed; ///< Historical exact input debits by stable meaning.
	std::map<CargoLabel, uint64_t> produced; ///< Historical exact product credits by stable meaning.
	/** @param other Candidate restored record. @return Exact equality of every saved choice/work/counter field. */
	bool operator==(const LayerProcessorState &other) const = default;
};

/** @return Processor side records in increasing actual industry order; no physical cargo is copied. */
std::vector<LayerProcessorState> CaptureLayerProcessors();
/** @param rows Unpublished parsed choice/work/history; existing Industry slots remain the only cargo stores. */
void StageLoadedLayerProcessors(std::vector<LayerProcessorState> rows);
/** @return Whether every native processor, slot union, choice and batch history agrees with actual rebuilt industries; publish a complete family only. */
bool AcceptLoadedLayerProcessors();
/** Clear unpublished processor wire transport during world initialization. */
void ResetLayerProcessorLoadStaging();
/** Validate independent preview tables/header/counts/feature flags without activating recipe choices. */
void ValidateLayerProcessorLoadPreview();
/** @return Canonical choice/work/history digest with actual input/output stocks, excluding presentation and transient proposals. */
uint64_t ComputeLayerProcessorDigest();

/** @param type Runtime definition. @return Whether its exact native capability is a Surface material processor. */
bool IsLayerProcessorDefinition(IndustryType type);
/** @param activated Exact pinned package descriptors. @return Whether all seven native facility definitions and layouts install without creating stock. */
bool InitialiseLayerProcessorDefinitions(const GRFConfigList &activated);
/** @param primary Stable primary facility recipe ID. @return Actual mapped native facility type, or IT_INVALID. */
IndustryType GetLayerProcessorIndustryType(MultiLayerRecipeID primary);
/** @param industry Actual industry ID. @return Read-only saved choice/counters, or nullptr; valid until the next state mutation. */
const LayerProcessorState *GetLayerProcessorState(IndustryID industry);
/** @param industry Fully created actual native facility; ordinary industries are ignored. */
void RegisterLayerProcessorIndustry(const Industry &industry);
/** @param industry Actual deleted industry ID; non-processors are ignored. */
void ReleaseLayerProcessorIndustry(IndustryID industry);
/** Clear processor side state during whole-world initialization, without touching old physical stock. */
void ResetLayerProcessorState();
/** @param old_company Removed/taken-over controller. @param new_company New controller, or invalid after abandonment; saved stocks/choice persist. */
void ChangeLayerProcessorControl(CompanyID old_company, CompanyID new_company);
/** @param industry Existing facility. @return Whether demolition would discard delivered or produced physical cargo. */
bool LayerProcessorHasStoredCargo(const Industry &industry);
/** @param industry Actual native processor. @param cargo Runtime input label slot. @return Exact free incoming/historical room, or zero for invalid/unavailable input. */
uint32_t GetLayerProcessorInputRoom(const Industry &industry, CargoType cargo);
/** @param industry Actual receiving processor. @param cargo Runtime input slot. @param offered Whole packet units. @return Exactly committed input units; every refusal leaves stocks/history unchanged. */
uint32_t ReceiveLayerProcessorCargo(Industry &industry, CargoType cargo, uint32_t offered);
/** @param station Actual receiving station. @param cargo Runtime cargo slot. @return Bounded room across associated same-layer native/legacy industries; whole units outside this room remain aboard. */
uint32_t GetLayerIndustryReceiverRoomAt(const Station &station, CargoType cargo);
/** @param industry Existing processor. @param economy_ticks Work earned once by the real tick dispatcher. */
void AdvanceLayerProcessor(Industry &industry, uint32_t economy_ticks);
/** @param industry Existing native processor. @return Exact station units committed across all product slots, or zero with output retained. */
uint32_t DispatchLayerProcessorCargo(Industry &industry);
/** @param industry Actual processor. @param recipe Frozen choice. @param company Current controller, first valid company recovering abandoned control, or explicit deity. @param execute False estimates; true commits only choice/control/revision, preserving stock. @return Whether the complete supported choice is admitted. */
bool SelectLayerProcessorRecipe(Industry &industry, MultiLayerRecipeID recipe, CompanyID company, bool execute);
/** @param industry Actual processor. @return The selected immutable Surface manufacturing record, or nullptr. */
const MultiLayerManufacturingRecipe *GetLayerProcessorRecipe(const Industry &industry);

#endif /* LAYER_INDUSTRY_PROCESSOR_H */
