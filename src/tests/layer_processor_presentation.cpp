/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file layer_processor_presentation.cpp Pure native status with real command-created processor stocks/work.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group39B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Full/history/malformed probes explicitly stage native boundary values; they are not game-loop acceptance.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "processor_fixture.h"
#include "../layer_processor_presentation.h"

#include "../safeguards.h"

/**
 * Prove repeated presentation reads preserve all saved stock/work/history, cash and RNG.
 * @param industry Actual command-created native processor queried without simulation advancement.
 * @param expected State derived from the fixture's current actual owner values.
 */
static void CheckProcessorStatusPure(const Industry &industry, LayerProcessorOperationalStatus expected)
{
	const auto state = *GetLayerProcessorState(industry.index);
	const auto digest = ComputeLayerProcessorDigest();
	const auto random = _random, interactive = _interactive_random;
	const auto cash = Company::Get(_current_company)->money;
	const auto count = Industry::GetNumItems();
	// Repeated tile/view consumers must observe the same state without earning
	// work, consuming material, changing history or advancing either RNG.
	for (int read = 0; read < 4; ++read) CHECK(GetLayerProcessorOperationalStatus(industry) == expected);
	CHECK(*GetLayerProcessorState(industry.index) == state);
	CHECK(ComputeLayerProcessorDigest() == digest);
	CHECK(_random.state[0] == random.state[0]);
	CHECK(_random.state[1] == random.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]);
	CHECK(_interactive_random.state[1] == interactive.state[1]);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(Industry::GetNumItems() == count);
}

/** Actual inputs, rather than banked work or waiting final output, determine idle versus earning a complete batch. */
TEST_CASE("Processor presentation reads all actual inputs independently of its work bank", "[layer-processor-presentation]")
{
	const auto primary = GENERATE(MultiLayerRecipeID{"CFMT"}, MultiLayerRecipeID{"CHEM"}, MultiLayerRecipeID{"BATT"}, MultiLayerRecipeID{"ELEC"});
	ProcessorRig rig(primary);
	const auto &record = *GetLayerProcessorRecipe(*rig.industry);
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::Idle);
	AdvanceLayerProcessor(*rig.industry, record.recipe.work_economy_ticks);
	REQUIRE(GetLayerProcessorState(rig.industry->index)->work_ticks > 0);
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::Idle);
	// Deliver all declared inputs through their native incoming-stock owner;
	// a partial role set may never produce a working equipment presentation.
	for (const auto &role : record.recipe.inputs) REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(role.label), role.quantity) == role.quantity);
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::Working);
	AdvanceLayerProcessor(*rig.industry, 0);
	CHECK(GetLayerProcessorState(rig.industry->index)->batches.at(primary) == 1);
	CHECK(GetLayerProcessorState(rig.industry->index)->work_ticks == 0);
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::Idle);
	CHECK(rig.Output(record.recipe.outputs[0].label).waiting == record.recipe.outputs[0].quantity);
	// A real next all-input set can earn work from zero; presentation does not
	// fabricate earned ticks or consume the final output of the previous batch.
	for (const auto &role : record.recipe.inputs) REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(role.label), role.quantity) == role.quantity);
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::Working);
}

/** Both physical output capacity and native production-history capacity hold the same complete input set unchanged. */
TEST_CASE("Processor presentation respects actual output and production history room", "[layer-processor-presentation]")
{
	ProcessorRig rig;
	const auto &record = *GetLayerProcessorRecipe(*rig.industry);
	// Stage only actual native boundary slots; the probe deliberately does not
	// claim those full-output values were earned in a driven transport loop.
	for (const auto &role : record.recipe.inputs) REQUIRE(ReceiveLayerProcessorCargo(*rig.industry, GetCargoTypeByLabel(role.label), role.quantity) == role.quantity);
	auto &output = rig.Output(record.recipe.outputs[0].label);
	output.waiting = record.output_capacity;
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::OutputFull);
	output.waiting = record.output_capacity - record.recipe.outputs[0].quantity + 1;
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::OutputFull);
	output.waiting = 0;
	output.history[THIS_MONTH].production = UINT16_MAX - record.recipe.outputs[0].quantity + 1;
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::OutputFull);
	output.history[THIS_MONTH].production = UINT16_MAX;
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::OutputFull);
	output.history[THIS_MONTH].production = 0;
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::Working);
}

/** Calendar, pause, incomplete owned cells and invalid native references never gain authority from an authored sprite. */
TEST_CASE("Processor presentation preserves native availability and invalid owner guards", "[layer-processor-presentation]")
{
	ProcessorRig rig(MultiLayerRecipeID{"BATT"});
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::Idle);
	_pause_mode.Set(PauseMode::Normal);
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::Paused);
	_pause_mode = {};
	const auto date = TimerGameCalendar::date;
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::Unavailable);
	TimerGameCalendar::date = date;
	rig.industry->prod_level = PRODLEVEL_CLOSURE;
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::Stopped);
	rig.industry->prod_level = PRODLEVEL_DEFAULT;
	const TileIndex owned = rig.industry->location.tile;
	ClrBit(Tile{owned}.m1(), 7); // Explicit native fixture: only this real funded tile becomes incomplete.
	// Native stages are per actual tile; one incomplete cell holds the whole
	// processor while the other fifteen completed cells retain their final pose.
	for (uint8_t stage = 0; stage < 4; ++stage) {
		SetIndustryConstructionStage(owned, stage);
		CHECK(GetIndustryConstructionStage(owned) == stage);
		CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::Construction);
	}
	SetIndustryCompleted(owned);
	auto rows = CaptureLayerProcessors();
	ReleaseLayerProcessorIndustry(rig.industry->index); // Explicit native missing-state fixture, never an ordinary-game fallback.
	CHECK(GetLayerProcessorOperationalStatus(*rig.industry) == LayerProcessorOperationalStatus::MissingState);
	StageLoadedLayerProcessors(rows);
	REQUIRE(AcceptLoadedLayerProcessors());
	const IndustryType type = rig.industry->type;
	rig.industry->type = 1; // Genuine foreign original industry definition cannot inherit the native pose.
	CHECK(GetLayerProcessorOperationalStatus(*rig.industry) == LayerProcessorOperationalStatus::MissingState);
	rig.industry->type = type;
	const CargoType cargo = rig.industry->accepted[0].cargo;
	const auto state = *GetLayerProcessorState(rig.industry->index);
	rig.industry->accepted[0].cargo = INVALID_CARGO; // Explicit malformed native-slot fixture checks safe refusal before CargoSpec lookup.
	CHECK(GetLayerProcessorOperationalStatus(*rig.industry) == LayerProcessorOperationalStatus::InvalidState);
	CHECK(*GetLayerProcessorState(rig.industry->index) == state);
	rig.industry->accepted[0].cargo = cargo;
	CheckProcessorStatusPure(*rig.industry, LayerProcessorOperationalStatus::Idle);
}
