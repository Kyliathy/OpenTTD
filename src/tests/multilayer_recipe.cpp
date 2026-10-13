/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_recipe.cpp All-input refusal, stock conservation and bounded planning fixtures. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_recipe.h"

#include "../safeguards.h"

/** Verify recipe planning retains partial inputs and refuses full output. */
TEST_CASE("Recipe planning retains partial inputs and refuses full output", "[multilayer-recipe]")
{
	const MultiLayerRecipeCargo inputs[] = {{CargoLabel{"STEL"}, 2}, {CargoLabel{"RFMT"}, 2}, {CargoLabel{"CHEM"}, 1}};
	const MultiLayerRecipeCargo outputs[] = {{CargoLabel{"SCMP"}, 2}};
	const MultiLayerRecipe recipe{MultiLayerRecipeID{"COMP"}, inputs, outputs, 74, 4};
	MultiLayerRecipeStock stock[] = {{CargoLabel{"STEL"}, 4, 1024}, {CargoLabel{"RFMT"}, 1, 1024}, {CargoLabel{"CHEM"}, 3, 1024}};
	MultiLayerRecipeStock product[] = {{CargoLabel{"SCMP"}, 0, 1024}};
	CHECK(ValidateMultiLayerRecipe(recipe) == MultiLayerRecipeLimit::None);
	auto plan = PlanMultiLayerRecipeBatches(recipe, stock, product, 296, 0, 0, 0, 0);
	CHECK(plan.batches == 0);
	CHECK(plan.limiting == MultiLayerRecipeLimit::Input);
	CHECK(stock[0].amount == 4);
	CHECK(stock[1].amount == 1);
	CHECK(stock[2].amount == 3);
	CHECK(product[0].amount == 0);
	stock[1].amount = 6;
	plan = PlanMultiLayerRecipeBatches(recipe, stock, product, 296, 0, 0, 0, 0);
	CHECK(plan.batches == 2);
	CHECK(plan.limiting == MultiLayerRecipeLimit::None);
	product[0].amount = 1023;
	plan = PlanMultiLayerRecipeBatches(recipe, stock, product, 296, 0, 0, 0, 0);
	CHECK(plan.batches == 0);
	CHECK(plan.limiting == MultiLayerRecipeLimit::Output);
	CHECK(stock[1].amount == 6);
	CHECK(product[0].amount == 1023);
	product[0].amount = 1022;
	CHECK(PlanMultiLayerRecipeBatches(recipe, stock, product, 296, 0, 0, 0, 0).batches == 1);
	CHECK(PlanMultiLayerRecipeBatches(recipe, stock, product, 73, 0, 0, 0, 0).limiting == MultiLayerRecipeLimit::WorkTime);
	stock[0].amount = 1025;
	CHECK(PlanMultiLayerRecipeBatches(recipe, stock, product, 296, 0, 0, 0, 0).limiting == MultiLayerRecipeLimit::InvalidStock);
	const MultiLayerRecipeStock duplicate[] = {stock[1], stock[1], stock[2]};
	CHECK(PlanMultiLayerRecipeBatches(recipe, duplicate, product, 296, 0, 0, 0, 0).limiting == MultiLayerRecipeLimit::InvalidStock);
}

/** Verify recipe limits conserve finite deposits and granted utility shares. */
TEST_CASE("Recipe limits conserve finite deposits and granted utility shares", "[multilayer-recipe]")
{
	const MultiLayerRecipeCargo output[] = {{CargoLabel{"CORE"}, 4}};
	const MultiLayerRecipeStock stock[] = {{CargoLabel{"CORE"}, 0, 1024}};
	MultiLayerRecipe mine{MultiLayerRecipeID{"MCOP"}, {}, output, 74, 4, CargoLabel{"CORE"}, 4};
	CHECK(PlanMultiLayerRecipeBatches(mine, {}, stock, 296, 7, 0, 0, 0).batches == 1);
	CHECK(PlanMultiLayerRecipeBatches(mine, {}, stock, 296, 3, 0, 0, 0).limiting == MultiLayerRecipeLimit::Reserve);
	mine.heat_j_per_batch = 10;
	mine.power_j_per_batch = 5;
	CHECK(PlanMultiLayerRecipeBatches(mine, {}, stock, 296, 16, 9, 20, 0).limiting == MultiLayerRecipeLimit::Heat);
	CHECK(PlanMultiLayerRecipeBatches(mine, {}, stock, 296, 16, 40, 4, 0).limiting == MultiLayerRecipeLimit::Power);
	CHECK(PlanMultiLayerRecipeBatches(mine, {}, stock, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX, 0).batches == 4);
	mine.aggregate_per_batch = 6;
	CHECK(PlanMultiLayerRecipeBatches(mine, {}, stock, 296, 16, 40, 20, 5).limiting == MultiLayerRecipeLimit::Aggregate);
	CHECK(PlanMultiLayerRecipeBatches(mine, {}, stock, 296, 16, 40, 20, 12).batches == 2);
	CHECK(stock[0].amount == 0);
}

/** Verify recipe validation rejects ambiguous roles and unsafe arithmetic. */
TEST_CASE("Recipe validation rejects ambiguous roles and unsafe arithmetic", "[multilayer-recipe]")
{
	const MultiLayerRecipeCargo input[] = {{CargoLabel{"CHEM"}, 2}};
	const MultiLayerRecipeCargo output[] = {{CargoLabel{"PROP"}, 2}};
	MultiLayerRecipe recipe{MultiLayerRecipeID{"FUEL"}, input, output, 74, 4};
	CHECK(ValidateMultiLayerRecipe(recipe) == MultiLayerRecipeLimit::None);
	REQUIRE(GetMultiLayerCargoContract(CargoLabel{"CHEM"}) != nullptr);
	CHECK(GetMultiLayerCargoContract(CargoLabel{"CHEM"})->form == MultiLayerCargoForm::Liquid);
	CHECK(GetMultiLayerCargoContract(CargoLabel{"PROP"})->unit == "kilolitres");
	CHECK(GetMultiLayerCargoContract(CargoLabel{"FAKE"}) == nullptr);
	const MultiLayerRecipeCargo duplicate[] = {input[0], input[0]};
	recipe.inputs = duplicate;
	CHECK(ValidateMultiLayerRecipe(recipe) == MultiLayerRecipeLimit::InvalidDefinition);
	recipe.inputs = input;
	recipe.outputs = input;
	CHECK(ValidateMultiLayerRecipe(recipe) == MultiLayerRecipeLimit::InvalidDefinition);
	recipe.outputs = output;
	recipe.power_j_per_batch = UINT64_MAX;
	CHECK(ValidateMultiLayerRecipe(recipe) == MultiLayerRecipeLimit::InvalidDefinition);
	recipe.power_j_per_batch = 0;
	recipe.reserve_label = CargoLabel{"PROP"};
	CHECK(ValidateMultiLayerRecipe(recipe) == MultiLayerRecipeLimit::InvalidDefinition);
	recipe.reserve_per_batch = 1;
	CHECK(ValidateMultiLayerRecipe(recipe) == MultiLayerRecipeLimit::InvalidDefinition);
	recipe.reserve_label = CT_INVALID;
	recipe.reserve_per_batch = 0;
	const MultiLayerRecipeCargo oversized[] = {{CargoLabel{"CHEM"}, 65536}};
	recipe.inputs = oversized;
	CHECK(ValidateMultiLayerRecipe(recipe) == MultiLayerRecipeLimit::InvalidDefinition);
	const MultiLayerRecipeCargo zero[] = {{CargoLabel{"CHEM"}, 0}};
	recipe.inputs = zero;
	CHECK(ValidateMultiLayerRecipe(recipe) == MultiLayerRecipeLimit::InvalidDefinition);
	const MultiLayerRecipeCargo missing[] = {{CargoLabel{"FAKE"}, 1}};
	recipe.inputs = missing;
	CHECK(ValidateMultiLayerRecipe(recipe) == MultiLayerRecipeLimit::InvalidDefinition);
	recipe.inputs = {};
	recipe.outputs = {};
	CHECK(ValidateMultiLayerRecipe(recipe) == MultiLayerRecipeLimit::InvalidDefinition);
}

/** Verify published manufacturing recipes need every input and fit bounded stock. */
TEST_CASE("Published manufacturing recipes need every input and fit bounded stock", "[multilayer-manufacturing]")
{
	const auto definitions = GetMultiLayerManufacturingRecipes();
	REQUIRE(definitions.size() == 14);
	std::set<MultiLayerRecipeID> identities;
	// Exercise the shared planner for every published choice; no plant may quietly convert a subset of inputs.
	for (const auto &definition : definitions) {
		const auto &recipe = definition.recipe;
		CHECK(identities.insert(recipe.id).second);
		CHECK(ValidateMultiLayerRecipe(recipe) == MultiLayerRecipeLimit::None);
		std::vector<MultiLayerRecipeStock> inputs;
		std::vector<MultiLayerRecipeStock> outputs;
		for (const auto &input : recipe.inputs) inputs.push_back({input.label, input.quantity * 2, definition.input_capacity});
		for (const auto &output : recipe.outputs) outputs.push_back({output.label, 0, definition.output_capacity});
		CHECK(PlanMultiLayerRecipeBatches(recipe, inputs, outputs, recipe.work_economy_ticks * 2, 0, 0, recipe.power_j_per_batch * 2, 0).batches == 2);
		// Removing each required label independently must pause the whole batch and retain the other stocks.
		for (auto &stock : inputs) {
			const auto retained = stock.amount;
			stock.amount = 0;
			CHECK(PlanMultiLayerRecipeBatches(recipe, inputs, outputs, recipe.work_economy_ticks * 2, 0, 0, recipe.power_j_per_batch * 2, 0).batches == 0);
			stock.amount = retained;
		}
		outputs[0].amount = outputs[0].capacity;
		CHECK(PlanMultiLayerRecipeBatches(recipe, inputs, outputs, recipe.work_economy_ticks * 2, 0, 0, recipe.power_j_per_batch * 2, 0).limiting == MultiLayerRecipeLimit::Output);
	}
}
