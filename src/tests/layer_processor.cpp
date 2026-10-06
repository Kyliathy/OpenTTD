/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_processor.cpp Command-funded Surface recipes, bounded stock, lifecycle and exact supply. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 10 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "processor_fixture.h"

#include "../safeguards.h"

/** Every frozen Surface material alternative resolves through the actual seven-facility catalogue and consumes all declared roles together. */
TEST_CASE("Native Surface processors execute every frozen material alternative", "[layer-processor]")
{
	const size_t index = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 11);
	const auto &record = GetMultiLayerManufacturingRecipes()[index];
	REQUIRE(record.layer == MapLayer::Surface);
	const auto records = GetMultiLayerManufacturingRecipes();
	const auto primary = std::ranges::find(records, record.facility, &MultiLayerManufacturingRecipe::facility);
	ProcessorRig rig(primary->recipe.id);
	REQUIRE(CmdSelectLayerIndustryRecipe({}, rig.industry->index, ProcessorRig::Encode(record.recipe.id)).Succeeded());
	CHECK(GetLayerProcessorState(rig.industry->index)->recipe == primary->recipe.id);
	REQUIRE(CmdSelectLayerIndustryRecipe(DoCommandFlag::Execute, rig.industry->index, ProcessorRig::Encode(record.recipe.id)).Succeeded());
	CHECK(rig.industry->location.w == 4);
	CHECK(rig.industry->location.h == 4);
	CHECK(GetLayerIndustryCapability(rig.industry->type)->asset_id.starts_with("facility."));
	// Deliver every required role except the final one; earned work must not consume a partial material set.
	for (size_t input = 0; input + 1 < record.recipe.inputs.size(); ++input) {
		const auto &role = record.recipe.inputs[input];
		CHECK(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(role.label), role.quantity) == role.quantity);
	}
	AdvanceLayerProcessor(*rig.industry, record.recipe.work_economy_ticks);
	CHECK(GetLayerProcessorState(rig.industry->index)->batches.at(record.recipe.id) == 0);
	for (const auto &role : record.recipe.outputs) CHECK(rig.Output(role.label).waiting == 0);
	const auto &last = record.recipe.inputs.back();
	REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(last.label), last.quantity) == last.quantity);
	AdvanceLayerProcessor(*rig.industry, 0);
	const auto *state = GetLayerProcessorState(rig.industry->index);
	CHECK(state->batches.at(record.recipe.id) == 1);
	CHECK(state->work_ticks == 0);
	// One batch is exactly its declared debits/credits; delivery history remains distinct from recipe consumption.
	for (const auto &role : record.recipe.inputs) {
		CHECK(rig.Input(role.label).waiting == 0);
		CHECK(state->consumed.at(role.label) == role.quantity);
		CHECK((*rig.Input(role.label).history)[THIS_MONTH].accepted == role.quantity);
	}
	for (const auto &role : record.recipe.outputs) {
		CHECK(rig.Output(role.label).waiting == role.quantity);
		CHECK(rig.Output(role.label).history[THIS_MONTH].production == role.quantity);
		CHECK(state->produced.at(role.label) == role.quantity);
	}
	CHECK(LayerProcessorHasStoredCargo(*rig.industry));
}

/** Full physical output and narrow history room suspend whole batches; pause and closure preserve stocks and work. */
TEST_CASE("Native processor buffers and availability never discard delivered material", "[layer-processor]")
{
	ProcessorRig rig;
	const auto &record = *GetLayerProcessorRecipe(*rig.industry);
	for (const auto &role : record.recipe.inputs) REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(role.label), role.quantity) == role.quantity);
	auto &output = rig.Output(record.recipe.outputs[0].label);
	output.waiting = record.output_capacity;
	AdvanceLayerProcessor(*rig.industry, 74);
	CHECK(GetLayerProcessorState(rig.industry->index)->batches.at(record.recipe.id) == 0);
	CHECK(rig.Input(CargoLabel{"CORE"}).waiting == 4);
	output.waiting = 0;
	output.history[THIS_MONTH].production = UINT16_MAX - 1;
	AdvanceLayerProcessor(*rig.industry, 0);
	CHECK(rig.Input(CargoLabel{"CORE"}).waiting == 4);
	output.history[THIS_MONTH].production = 0;
	const uint64_t work = GetLayerProcessorState(rig.industry->index)->work_ticks;
	_pause_mode.Set(PauseMode::Normal);
	AdvanceLayerProcessor(*rig.industry, 100);
	CHECK(GetLayerProcessorState(rig.industry->index)->work_ticks == work);
	CHECK(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(CargoLabel{"COAL"}), 1) == 0);
	_pause_mode = {};
	AdvanceLayerProcessor(*rig.industry, 0);
	CHECK(output.waiting == 2);
	CHECK(DispatchLayerProcessorCargo(*rig.industry) == 0);
	CHECK(output.waiting == 2);
	Station *station = rig.StationAt(TileXYL(29, 30, MapLayer::Surface));
	station->goods[output.cargo].rating = 255;
	station->goods[output.cargo].status.Set(GoodsEntry::State::Rating);
	station->goods[output.cargo].amount_fract = 19;
	CHECK(DispatchLayerProcessorCargo(*rig.industry) == 2);
	CHECK(output.waiting == 0);
	CHECK(station->goods[output.cargo].TotalCount() == 2);
	CHECK(station->goods[output.cargo].amount_fract == 19);
	CHECK(GetLayerProcessorState(rig.industry->index)->dispatched == 2);
}

/** Choice mutation is explicit, calendar-gated and controlled by the funding company; abandoned actors gain no authority. */
TEST_CASE("Native processor recipe command preserves stock and enforces control", "[layer-processor]")
{
	ProcessorRig rig;
	REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(CargoLabel{"BAUX"}), 4) == 4);
	const CompanyID controller = _current_company;
	_current_company = CompanyID::Invalid();
	CHECK(CmdSelectLayerIndustryRecipe(DoCommandFlag::Execute, rig.industry->index, ProcessorRig::Encode(MultiLayerRecipeID{"BFMT"})).Failed());
	_current_company = controller;
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 0, 1);
	CHECK(CmdSelectLayerIndustryRecipe(DoCommandFlag::Execute, rig.industry->index, ProcessorRig::Encode(MultiLayerRecipeID{"NFMT"})).Failed());
	REQUIRE(CmdSelectLayerIndustryRecipe(DoCommandFlag::Execute, rig.industry->index, ProcessorRig::Encode(MultiLayerRecipeID{"BFMT"})).Succeeded());
	CHECK(rig.Input(CargoLabel{"BAUX"}).waiting == 4);
	CHECK(CmdSelectLayerIndustryRecipe(DoCommandFlag::Execute, rig.industry->index, ProcessorRig::Encode(MultiLayerRecipeID{"RBOT"})).Failed());
	CHECK(GetLayerProcessorState(rig.industry->index)->recipe == MultiLayerRecipeID{"BFMT"});
}
