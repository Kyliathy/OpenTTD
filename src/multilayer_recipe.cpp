/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_recipe.cpp Checked all-input planning without premature stock consumption. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#include "stdafx.h"
#include "multilayer_recipe.h"
#include "industry_type.h"

#include "safeguards.h"

static constexpr MultiLayerRecipeCargo _copper_inputs[] = {{CargoLabel{"CORE"}, 4}, {CargoLabel{"COAL"}, 1}};
static constexpr MultiLayerRecipeCargo _bauxite_inputs[] = {{CargoLabel{"BAUX"}, 4}, {CargoLabel{"COAL"}, 1}};
static constexpr MultiLayerRecipeCargo _nickel_inputs[] = {{CargoLabel{"NORE"}, 4}, {CargoLabel{"COAL"}, 1}};
static constexpr MultiLayerRecipeCargo _metal_outputs[] = {{CargoLabel{"RFMT"}, 2}};
static constexpr MultiLayerRecipeCargo _chemical_inputs[] = {{CargoLabel{"SALT"}, 2}, {CargoLabel{"OIL_"}, 2}};
static constexpr MultiLayerRecipeCargo _chemical_outputs[] = {{CargoLabel{"CHEM"}, 2}};
static constexpr MultiLayerRecipeCargo _battery_inputs[] = {{CargoLabel{"LITH"}, 2}, {CargoLabel{"COBT"}, 1}, {CargoLabel{"CHEM"}, 1}};
static constexpr MultiLayerRecipeCargo _battery_outputs[] = {{CargoLabel{"BATY"}, 2}};
static constexpr MultiLayerRecipeCargo _electronics_inputs[] = {{CargoLabel{"RFMT"}, 2}, {CargoLabel{"RARE"}, 1}};
static constexpr MultiLayerRecipeCargo _electronics_outputs[] = {{CargoLabel{"ELEC"}, 2}};
static constexpr MultiLayerRecipeCargo _component_inputs[] = {{CargoLabel{"STEL"}, 2}, {CargoLabel{"RFMT"}, 2}, {CargoLabel{"CHEM"}, 1}};
static constexpr MultiLayerRecipeCargo _component_outputs[] = {{CargoLabel{"SCMP"}, 2}};
static constexpr MultiLayerRecipeCargo _robotics_inputs[] = {{CargoLabel{"ELEC"}, 2}, {CargoLabel{"BATY"}, 1}, {CargoLabel{"RFMT"}, 1}};
static constexpr MultiLayerRecipeCargo _robotics_outputs[] = {{CargoLabel{"RBPT"}, 2}};
static constexpr MultiLayerRecipeCargo _propellant_inputs[] = {{CargoLabel{"CHEM"}, 2}};
static constexpr MultiLayerRecipeCargo _propellant_outputs[] = {{CargoLabel{"PROP"}, 2}};
static constexpr MultiLayerRecipeCargo _ice_inputs[] = {{CargoLabel{"WICE"}, 2}};
static constexpr MultiLayerRecipeCargo _ice_outputs[] = {{CargoLabel{"PROP"}, 1}};
static constexpr MultiLayerRecipeCargo _orbital_nickel_inputs[] = {{CargoLabel{"NORE"}, 4}};
static constexpr MultiLayerRecipeCargo _mirror_inputs[] = {{CargoLabel{"SCMP"}, 2}, {CargoLabel{"ELEC"}, 1}};
static constexpr MultiLayerRecipeCargo _mirror_outputs[] = {{CargoLabel{"SMIK"}, 1}};

/* Alternative rows share facility stock/work. Selecting one stable recipe never runs every alternative at once. */
static const MultiLayerManufacturingRecipe _manufacturing_recipes[] = {
	{"Metal refinery", MapLayer::Surface, TimerGameCalendar::Year{1950}, {MultiLayerRecipeID{"CFMT"}, _copper_inputs, _metal_outputs, 74, 4}, 1024, 1024},
	{"Metal refinery", MapLayer::Surface, TimerGameCalendar::Year{1950}, {MultiLayerRecipeID{"BFMT"}, _bauxite_inputs, _metal_outputs, 74, 4}, 1024, 1024},
	{"Metal refinery", MapLayer::Surface, TimerGameCalendar::Year{1980}, {MultiLayerRecipeID{"NFMT"}, _nickel_inputs, _metal_outputs, 74, 4}, 1024, 1024},
	{"Chemical works", MapLayer::Surface, TimerGameCalendar::Year{1950}, {MultiLayerRecipeID{"CHEM"}, _chemical_inputs, _chemical_outputs, 74, 4}, 1024, 1024},
	{"Battery works", MapLayer::Surface, TimerGameCalendar::Year{1980}, {MultiLayerRecipeID{"BATT"}, _battery_inputs, _battery_outputs, 74, 4}, 1024, 1024},
	{"Electronics works", MapLayer::Surface, TimerGameCalendar::Year{1980}, {MultiLayerRecipeID{"ELEC"}, _electronics_inputs, _electronics_outputs, 74, 4}, 1024, 1024},
	{"Component works", MapLayer::Surface, TimerGameCalendar::Year{1980}, {MultiLayerRecipeID{"COMP"}, _component_inputs, _component_outputs, 74, 4}, 1024, 1024},
	{"Robotics works", MapLayer::Surface, TimerGameCalendar::Year{1980}, {MultiLayerRecipeID{"RBOT"}, _robotics_inputs, _robotics_outputs, 74, 4}, 1024, 1024},
	{"Chemical works", MapLayer::Surface, TimerGameCalendar::Year{1980}, {MultiLayerRecipeID{"PCHM"}, _propellant_inputs, _propellant_outputs, 74, 4}, 1024, 1024},
	{"Orbital processor", MapLayer::Space, TimerGameCalendar::Year{1995}, {MultiLayerRecipeID{"PICE"}, _ice_inputs, _ice_outputs, 74, 4}, 1024, 1024},
	{"Orbital processor", MapLayer::Space, TimerGameCalendar::Year{1995}, {MultiLayerRecipeID{"OREF"}, _orbital_nickel_inputs, _metal_outputs, 74, 4}, 1024, 1024},
	{"Mirror-kit works", MapLayer::Surface, TimerGameCalendar::Year{1980}, {MultiLayerRecipeID{"MKIT"}, _mirror_inputs, _mirror_outputs, 148, 2}, 1024, 512},
};

/**
 * Publish immutable batch/capacity definitions to feature owners.
 * @return Surface and orbital recipes, each with one stable explicit choice ID.
 */
std::span<const MultiLayerManufacturingRecipe> GetMultiLayerManufacturingRecipes()
{
	return _manufacturing_recipes;
}

static constexpr MultiLayerRecipeCargo _mine_coal[] = {{CargoLabel{"COAL"}, 4}};
static constexpr MultiLayerRecipeCargo _mine_iron[] = {{CargoLabel{"IORE"}, 4}};
static constexpr MultiLayerRecipeCargo _mine_copper[] = {{CargoLabel{"CORE"}, 4}};
static constexpr MultiLayerRecipeCargo _mine_bauxite[] = {{CargoLabel{"BAUX"}, 4}};
static constexpr MultiLayerRecipeCargo _mine_salt[] = {{CargoLabel{"SALT"}, 4}};
static constexpr MultiLayerRecipeCargo _mine_lithium[] = {{CargoLabel{"LITH"}, 4}};
static constexpr MultiLayerRecipeCargo _mine_nickel[] = {{CargoLabel{"NORE"}, 4}};
static constexpr MultiLayerRecipeCargo _mine_cobalt[] = {{CargoLabel{"COBT"}, 4}};
static constexpr MultiLayerRecipeCargo _mine_rare[] = {{CargoLabel{"RARE"}, 2}};
static constexpr MultiLayerRecipeCargo _mine_uranium[] = {{CargoLabel{"URAN"}, 1}};

static const MultiLayerMineRecipe _mine_recipes[] = {
	{CargoLabel{"COAL"}, MapLayer::Underground, TimerGameCalendar::Year{1950}, "Power station", TimerGameCalendar::Year{1950}, {MultiLayerRecipeID{"MCOA"}, {}, _mine_coal, 74, 1, CargoLabel{"COAL"}, 4}, 1024},
	{CargoLabel{"IORE"}, MapLayer::Underground, TimerGameCalendar::Year{1950}, "Steel mill", TimerGameCalendar::Year{1950}, {MultiLayerRecipeID{"MIRN"}, {}, _mine_iron, 74, 1, CargoLabel{"IORE"}, 4}, 1024},
	{CargoLabel{"CORE"}, MapLayer::Underground, TimerGameCalendar::Year{1950}, "Metal refinery", TimerGameCalendar::Year{1950}, {MultiLayerRecipeID{"MCOP"}, {}, _mine_copper, 74, 1, CargoLabel{"CORE"}, 4}, 1024},
	{CargoLabel{"BAUX"}, MapLayer::Underground, TimerGameCalendar::Year{1950}, "Metal refinery", TimerGameCalendar::Year{1950}, {MultiLayerRecipeID{"MBAU"}, {}, _mine_bauxite, 74, 1, CargoLabel{"BAUX"}, 4}, 1024},
	{CargoLabel{"SALT"}, MapLayer::Underground, TimerGameCalendar::Year{1950}, "Chemical works", TimerGameCalendar::Year{1950}, {MultiLayerRecipeID{"MSAL"}, {}, _mine_salt, 74, 1, CargoLabel{"SALT"}, 4}, 1024},
	{CargoLabel{"LITH"}, MapLayer::DeepUnderground, TimerGameCalendar::Year{1980}, "Battery works", TimerGameCalendar::Year{1980}, {MultiLayerRecipeID{"MLIT"}, {}, _mine_lithium, 74, 1, CargoLabel{"LITH"}, 4}, 1024},
	{CargoLabel{"NORE"}, MapLayer::DeepUnderground, TimerGameCalendar::Year{1980}, "Metal refinery", TimerGameCalendar::Year{1950}, {MultiLayerRecipeID{"MNIC"}, {}, _mine_nickel, 74, 1, CargoLabel{"NORE"}, 4}, 1024},
	{CargoLabel{"COBT"}, MapLayer::DeepUnderground, TimerGameCalendar::Year{1980}, "Battery works", TimerGameCalendar::Year{1980}, {MultiLayerRecipeID{"MCOB"}, {}, _mine_cobalt, 74, 1, CargoLabel{"COBT"}, 4}, 1024},
	{CargoLabel{"RARE"}, MapLayer::DeepUnderground, TimerGameCalendar::Year{1980}, "Electronics works", TimerGameCalendar::Year{1980}, {MultiLayerRecipeID{"MRAR"}, {}, _mine_rare, 74, 1, CargoLabel{"RARE"}, 2}, 512},
	{CargoLabel{"URAN"}, MapLayer::DeepUnderground, TimerGameCalendar::Year{1980}, "Nuclear plant", TimerGameCalendar::Year{1980}, {MultiLayerRecipeID{"MURA"}, {}, _mine_uranium, 74, 1, CargoLabel{"URAN"}, 1}, 256},
};

/**
 * Publish finite extraction records and their dated useful receivers.
 * @return Ten immutable mine definitions from the first-party content contract.
 */
std::span<const MultiLayerMineRecipe> GetMultiLayerMineRecipes()
{
	return _mine_recipes;
}

static constexpr MultiLayerRecipeCargo _heat_iron_inputs[] = {{CargoLabel{"IORE"}, 4}};
static constexpr MultiLayerRecipeCargo _heat_copper_inputs[] = {{CargoLabel{"CORE"}, 4}};
static constexpr MultiLayerRecipeCargo _heat_nickel_inputs[] = {{CargoLabel{"NORE"}, 4}};
static constexpr MultiLayerRecipeCargo _heat_steel_outputs[] = {{CargoLabel{"STEL"}, 2}};
static constexpr MultiLayerRecipeCargo _forge_component_inputs[] = {{CargoLabel{"RFMT"}, 2}, {CargoLabel{"ELEC"}, 1}};
static constexpr MultiLayerRecipeCargo _forge_robotics_inputs[] = {{CargoLabel{"RFMT"}, 1}, {CargoLabel{"ELEC"}, 1}, {CargoLabel{"BATY"}, 1}};

/* Direct heat is a granted share, not fuel freight; alternatives retain the one saved recipe choice. */
static const MultiLayerManufacturingRecipe _geothermal_recipes[] = {
	{"Geothermal Smelter", MapLayer::DeepUnderground, TimerGameCalendar::Year{2010}, {MultiLayerRecipeID{"HSTL"}, _heat_iron_inputs, _heat_steel_outputs, 74, 4, CT_INVALID, 0, 500000, 5000}, 1024, 1024},
	{"Geothermal Smelter", MapLayer::DeepUnderground, TimerGameCalendar::Year{2010}, {MultiLayerRecipeID{"HCOP"}, _heat_copper_inputs, _metal_outputs, 74, 4, CT_INVALID, 0, 500000, 5000}, 1024, 1024},
	{"Geothermal Smelter", MapLayer::DeepUnderground, TimerGameCalendar::Year{2010}, {MultiLayerRecipeID{"HNIC"}, _heat_nickel_inputs, _metal_outputs, 74, 4, CT_INVALID, 0, 500000, 5000}, 1024, 1024},
	{"Lithothermal Battery Works", MapLayer::DeepUnderground, TimerGameCalendar::Year{2030}, {MultiLayerRecipeID{"HBAT"}, _battery_inputs, _battery_outputs, 74, 4, CT_INVALID, 0, 600000, 10000}, 1024, 1024},
	{"Mantle Forge", MapLayer::DeepUnderground, TimerGameCalendar::Year{2045}, {MultiLayerRecipeID{"HCMP"}, _forge_component_inputs, _component_outputs, 74, 4, CT_INVALID, 0, 800000, 15000}, 1024, 1024},
	{"Mantle Forge", MapLayer::DeepUnderground, TimerGameCalendar::Year{2045}, {MultiLayerRecipeID{"HRBT"}, _forge_robotics_inputs, _robotics_outputs, 74, 4, CT_INVALID, 0, 800000, 15000}, 1024, 1024},
};

/**
 * Publish bounded direct-heat manufacturing while preserving Surface bootstrap alternatives.
 * @return Six stable choices across three dated Deep factory families.
 */
std::span<const MultiLayerManufacturingRecipe> GetMultiLayerGeothermalRecipes()
{
	return _geothermal_recipes;
}

/**
 * Check one cargo role list before native production can bind it to facility stock.
 * @param cargoes Distinct positive per-batch input or output quantities.
 * @return Whether every role has a declared form and fits existing stock width.
 */
static bool ValidateRecipeCargoes(std::span<const MultiLayerRecipeCargo> cargoes)
{
	// Repeated labels would debit/credit the same stock twice; validate the entire role list before any commit.
	for (size_t i = 0; i < cargoes.size(); ++i) {
		if (cargoes[i].quantity == 0) return false;
		if (cargoes[i].quantity > UINT16_MAX) return false;
		if (GetMultiLayerCargoContract(cargoes[i].label) == nullptr) return false;
		for (size_t j = 0; j < i; ++j) {
			if (cargoes[i].label == cargoes[j].label) return false;
		}
	}
	return true;
}

/**
 * Check immutable recipe quantities and role ownership before evaluating stocks.
 * @param recipe Selected definition, with stable saved alternative identity.
 * @return None or InvalidDefinition; no state or selected choice is changed.
 */
MultiLayerRecipeLimit ValidateMultiLayerRecipe(const MultiLayerRecipe &recipe)
{
	if (recipe.id.Empty()) return MultiLayerRecipeLimit::InvalidDefinition;
	// A useful batch must consume or produce something; an empty recipe would fabricate work progress.
	if (recipe.inputs.empty() && recipe.outputs.empty()) return MultiLayerRecipeLimit::InvalidDefinition;
	if (recipe.inputs.size() > INDUSTRY_NUM_INPUTS) return MultiLayerRecipeLimit::InvalidDefinition;
	if (recipe.outputs.size() > INDUSTRY_NUM_OUTPUTS) return MultiLayerRecipeLimit::InvalidDefinition;
	if (recipe.work_economy_ticks == 0) return MultiLayerRecipeLimit::InvalidDefinition;
	if (recipe.max_batches == 0) return MultiLayerRecipeLimit::InvalidDefinition;
	if (!ValidateRecipeCargoes(recipe.inputs)) return MultiLayerRecipeLimit::InvalidDefinition;
	if (!ValidateRecipeCargoes(recipe.outputs)) return MultiLayerRecipeLimit::InvalidDefinition;
	if (recipe.heat_j_per_batch > UINT64_MAX / recipe.max_batches) return MultiLayerRecipeLimit::InvalidDefinition;
	if (recipe.power_j_per_batch > UINT64_MAX / recipe.max_batches) return MultiLayerRecipeLimit::InvalidDefinition;
	if (recipe.aggregate_per_batch > UINT16_MAX) return MultiLayerRecipeLimit::InvalidDefinition;
	// Reserve identity is meaningful only with a positive debit; reject half-configured mine records.
	if ((recipe.reserve_label != CT_INVALID) != (recipe.reserve_per_batch != 0)) return MultiLayerRecipeLimit::InvalidDefinition;
	// A single batch may not disguise a same-label stock cycle as useful production.
	for (const auto &input : recipe.inputs) {
		for (const auto &output : recipe.outputs) {
			if (input.label == output.label) return MultiLayerRecipeLimit::InvalidDefinition;
		}
	}
	if (recipe.reserve_per_batch != 0) {
		// Mine extraction has exactly one output matching its finite reserve, so depleted reserve cannot create another ore.
		if (recipe.outputs.size() != 1) return MultiLayerRecipeLimit::InvalidDefinition;
		if (recipe.outputs[0].label != recipe.reserve_label) return MultiLayerRecipeLimit::InvalidDefinition;
		if (recipe.outputs[0].quantity != recipe.reserve_per_batch) return MultiLayerRecipeLimit::InvalidDefinition;
	}
	return MultiLayerRecipeLimit::None;
}

/**
 * Locate a unique valid stock binding for one role.
 * @param stocks Actual read-only facility stock snapshot.
 * @param label Required meaning to locate.
 * @return Stock pointer, or nullptr for absent/ambiguous/invalid stock.
 */
static const MultiLayerRecipeStock *FindRecipeStock(std::span<const MultiLayerRecipeStock> stocks, CargoLabel label)
{
	const MultiLayerRecipeStock *found = nullptr;
	// Duplicate bindings would let a callback plan against one stock and commit against another.
	for (const auto &stock : stocks) {
		if (stock.label != label) continue;
		if (found != nullptr) return nullptr;
		if (stock.capacity == 0) return nullptr;
		if (stock.capacity > UINT16_MAX) return nullptr;
		if (stock.amount > stock.capacity) return nullptr;
		found = &stock;
	}
	return found;
}

/**
 * Compute a bounded all-input plan while retaining every resource on refusal.
 * @param recipe Validated selected alternative.
 * @param input_stock Actual stocks for every required input role.
 * @param output_stock Actual stocks/caps for every output role.
 * @param work_ticks Available synchronized work, after the owner's pause rules.
 * @param reserve Available finite deposit quantity.
 * @param heat_j Granted direct heat for this batch evaluation.
 * @param power_j Granted electricity for this evaluation.
 * @param aggregate_free_space Remaining internal aggregate storage.
 * @return Complete batch count and deterministic limiting reason, with no mutation.
 */
MultiLayerRecipeBatchPlan PlanMultiLayerRecipeBatches(const MultiLayerRecipe &recipe, std::span<const MultiLayerRecipeStock> input_stock, std::span<const MultiLayerRecipeStock> output_stock, uint64_t work_ticks, uint64_t reserve, uint64_t heat_j, uint64_t power_j, uint32_t aggregate_free_space)
{
	if (ValidateMultiLayerRecipe(recipe) != MultiLayerRecipeLimit::None) return {};
	uint64_t batches = std::min<uint64_t>(recipe.max_batches, work_ticks / recipe.work_economy_ticks);
	MultiLayerRecipeLimit limiting = batches == 0 ? MultiLayerRecipeLimit::WorkTime : MultiLayerRecipeLimit::None;
	// Every input constrains the same batch count; independent input conversion cannot satisfy this contract.
	for (const auto &input : recipe.inputs) {
		const auto *stock = FindRecipeStock(input_stock, input.label);
		if (stock == nullptr) return {0, MultiLayerRecipeLimit::InvalidStock};
		const uint64_t limit = stock->amount / input.quantity;
		if (limit < batches) { batches = limit; limiting = MultiLayerRecipeLimit::Input; }
	}
	// Check all product spaces before granting any consumption; clamping a later output would destroy materials.
	for (const auto &output : recipe.outputs) {
		const auto *stock = FindRecipeStock(output_stock, output.label);
		if (stock == nullptr) return {0, MultiLayerRecipeLimit::InvalidStock};
		const uint64_t limit = (stock->capacity - stock->amount) / output.quantity;
		if (limit < batches) { batches = limit; limiting = MultiLayerRecipeLimit::Output; }
	}
	if (recipe.reserve_per_batch != 0) {
		const uint64_t limit = reserve / recipe.reserve_per_batch;
		if (limit < batches) { batches = limit; limiting = MultiLayerRecipeLimit::Reserve; }
	}
	if (recipe.heat_j_per_batch != 0) {
		const uint64_t limit = heat_j / recipe.heat_j_per_batch;
		if (limit < batches) { batches = limit; limiting = MultiLayerRecipeLimit::Heat; }
	}
	if (recipe.power_j_per_batch != 0) {
		const uint64_t limit = power_j / recipe.power_j_per_batch;
		if (limit < batches) { batches = limit; limiting = MultiLayerRecipeLimit::Power; }
	}
	if (recipe.aggregate_per_batch != 0) {
		const uint64_t limit = aggregate_free_space / recipe.aggregate_per_batch;
		if (limit < batches) { batches = limit; limiting = MultiLayerRecipeLimit::Aggregate; }
	}
	return {static_cast<uint16_t>(batches), batches == 0 ? limiting : MultiLayerRecipeLimit::None};
}
