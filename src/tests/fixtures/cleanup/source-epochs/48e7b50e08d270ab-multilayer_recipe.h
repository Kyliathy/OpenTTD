/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_recipe.h All-input recipe and bounded stock planning contract. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#ifndef MULTILAYER_RECIPE_H
#define MULTILAYER_RECIPE_H

#include "multilayer_content.h"

/** Stable saved alternative recipe identity, distinct from cargo and industry IDs. */
using MultiLayerRecipeID = Label<struct MultiLayerRecipeIDTag>;

/** One required or produced cargo quantity, in the label's declared packet unit. */
struct MultiLayerRecipeCargo {
	CargoLabel label; ///< Resolve by meaning, never runtime slot.
	uint32_t quantity; ///< Positive quantity per complete batch.
};

/** Immutable all-input batch definition; native code owns consumption for bundled definitions too. */
struct MultiLayerRecipe {
	MultiLayerRecipeID id; ///< Saved explicit recipe choice; alternatives never run simultaneously.
	std::span<const MultiLayerRecipeCargo> inputs; ///< Every listed input is required for a batch.
	std::span<const MultiLayerRecipeCargo> outputs; ///< Every listed output must fit before any input is consumed.
	uint32_t work_economy_ticks = 0; ///< Synchronized work duration per complete batch.
	uint16_t max_batches = 0; ///< Bounded work per evaluation interval.
	CargoLabel reserve_label = CT_INVALID; ///< Mine resource identity; invalid when no deposit is consumed.
	uint32_t reserve_per_batch = 0; ///< Exact finite reserve debit per output batch.
	uint64_t heat_j_per_batch = 0; ///< Direct-heat share, supplied once by the vent allocation owner.
	uint64_t power_j_per_batch = 0; ///< Electricity already granted by the utility owner.
	uint32_t aggregate_per_batch = 0; ///< Internal construction stock, not paid freight.
};

/** Read-only snapshot of one bounded facility stock. */
struct MultiLayerRecipeStock {
	CargoLabel label; ///< Meaning owned by the existing cargo registry.
	uint32_t amount = 0; ///< Current quantity; must not exceed capacity.
	uint32_t capacity = 0; ///< Positive per-label cap, at most the existing industry stock limit.
};

/** Deterministic reason a batch or definition refuses progress. */
enum class MultiLayerRecipeLimit : uint8_t {
	None, ///< At least one complete batch fits all prerequisites.
	InvalidDefinition, ///< Missing identity/form, repeated roles, invalid quantities or unchecked bounds.
	InvalidStock, ///< Missing/duplicate stock or amount beyond its capacity.
	WorkTime, ///< Not enough synchronized work for one batch.
	Input, ///< At least one required cargo is missing.
	Output, ///< At least one product cannot fit.
	Reserve, ///< Finite mine reserve cannot fund the batch.
	Heat, ///< Direct-heat allocation is insufficient.
	Power, ///< Granted electric energy is insufficient.
	Aggregate, ///< Internal construction output stock is full.
};

/** Pure pre-commit result; the feature owner revalidates and applies all deltas atomically. */
struct MultiLayerRecipeBatchPlan {
	uint16_t batches = 0; ///< Complete batches; zero authorizes no consumption of any resource.
	MultiLayerRecipeLimit limiting = MultiLayerRecipeLimit::InvalidDefinition; ///< Visible pause/refusal reason.
};

/** Published manufacturing definition, separate from mapped NewGRF industry runtime IDs. */
struct MultiLayerManufacturingRecipe {
	std::string_view facility; ///< Content owner/facility family, not an independently allocated industry ID.
	MapLayer layer; ///< Required operating layer; inputs still need an actual route.
	TimerGameCalendar::Year introduction_year; ///< Calendar availability, distinct from batch duration.
	MultiLayerRecipe recipe; ///< Immutable saved-choice batch.
	uint16_t input_capacity; ///< Maximum incoming stock per label.
	uint16_t output_capacity; ///< Maximum product stock per label.
};

/**
 * Read the first-party Surface/orbital manufacturing recipes without allocating industries.
 * @return Immutable definitions; feature owners install behavior and preserve saved choices.
 */
std::span<const MultiLayerManufacturingRecipe> GetMultiLayerManufacturingRecipes();

/** Mine/receiver calendar and finite-output definition, independent of runtime industry mappings. */
struct MultiLayerMineRecipe {
	CargoLabel resource; ///< One main output meaning and finite deposit reserve identity.
	MapLayer layer; ///< Underground or Deep; excavation/access validation belongs to the feature owner.
	TimerGameCalendar::Year introduction_year; ///< First extraction date, including Deep's calendar gate.
	std::string_view receiver; ///< Useful Surface receiver family, not a private industry pool reference.
	TimerGameCalendar::Year receiver_year; ///< Receiver must be available no later than this mine.
	MultiLayerRecipe recipe; ///< Single-output finite reserve batch.
	uint16_t output_capacity; ///< Bounded product stock, in this resource's tonnes.
};

/**
 * Read all ten mine output and conventional receiver contracts.
 * @return Immutable finite-resource definitions; no deposits or industries are created here.
 */
std::span<const MultiLayerMineRecipe> GetMultiLayerMineRecipes();

/**
 * Read Deep direct-heat recipes, using the same all-input/energy/output contract as Surface plants.
 * @return Immutable 2010/2030/2045 definitions; feature owners install heat-network behavior.
 */
std::span<const MultiLayerManufacturingRecipe> GetMultiLayerGeothermalRecipes();

/**
 * Validate one recipe's identities, forms, slot/stock limits and arithmetic bounds.
 * @param recipe Immutable definition owned by the selected content contract.
 * @return None for a valid definition, otherwise InvalidDefinition.
 */
MultiLayerRecipeLimit ValidateMultiLayerRecipe(const MultiLayerRecipe &recipe);

/**
 * Plan complete batches without consuming stock, reserve, energy, time, RNG or money.
 * @param recipe Validated selected recipe; one native owner later performs the commit.
 * @param input_stock Read-only incoming stocks for the listed inputs.
 * @param output_stock Read-only product stocks for every listed output.
 * @param work_ticks Saved available synchronized work; paused facilities do not accrue new work.
 * @param reserve Finite reserve available to this mine's resource.
 * @param heat_j Direct heat already reserved by the shared vent allocation.
 * @param power_j Electricity already granted by the shared utility allocation.
 * @param aggregate_free_space Free capacity in internal construction stock.
 * @return Bounded atomic plan and a deterministic limiting reason; no stock changes here.
 */
MultiLayerRecipeBatchPlan PlanMultiLayerRecipeBatches(const MultiLayerRecipe &recipe, std::span<const MultiLayerRecipeStock> input_stock, std::span<const MultiLayerRecipeStock> output_stock, uint64_t work_ticks, uint64_t reserve, uint64_t heat_j, uint64_t power_j, uint32_t aggregate_free_space);

#endif /* MULTILAYER_RECIPE_H */
