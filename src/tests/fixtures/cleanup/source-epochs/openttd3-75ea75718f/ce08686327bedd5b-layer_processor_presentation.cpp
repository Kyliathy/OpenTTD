/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file layer_processor_presentation.cpp Shared pure availability/snapshot and native processor presentation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group39B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Groups10/15.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "stdafx.h"
#include "layer_processor_presentation.h"
#include "industry.h"
#include "openttd.h"

#include "safeguards.h"

/** @copydoc IsLayerProcessorActive */
bool IsLayerProcessorActive(const Industry &industry, const MultiLayerManufacturingRecipe &recipe)
{
	// Production and presentation share all existing operating gates. A camera,
	// completed-looking sprite or banked work cannot enable an unavailable plant.
	return _game_mode == GameMode::Normal && _pause_mode.None() && industry.prod_level != PRODLEVEL_CLOSURE &&
		GetTileLayer(industry.location.tile) == recipe.layer && IsLayerIndustryComplete(industry) &&
		TimerGameCalendar::ConvertDateToYMD(TimerGameCalendar::date).year >= recipe.introduction_year;
}

/** @copydoc CaptureLayerProcessorInventory */
LayerRecipeInventorySnapshot CaptureLayerProcessorInventory(const Industry &industry, const LayerProcessorState &state,
	const MultiLayerManufacturingRecipe &recipe)
{
	LayerRecipeInventorySnapshot snapshot;
	snapshot.recipe = state.recipe;
	snapshot.layer = GetTileLayer(industry.location.tile);
	snapshot.revision = state.revision;
	snapshot.active = IsLayerProcessorActive(industry, recipe);
	snapshot.work_ticks = state.work_ticks;
	// All accepted alternatives remain physically owned by Industry. The
	// selected recipe's common planner alone chooses the roles it may consume.
	for (const auto &slot : industry.accepted) {
		if (!IsValidCargoType(slot.cargo)) { snapshot.active = false; return snapshot; }
		snapshot.inputs.push_back({CargoSpec::Get(slot.cargo)->label, slot.waiting, recipe.input_capacity});
	}
	// Include actual output and bounded native production-history room together;
	// presentation cannot advertise work that the real atomic commit would lose.
	for (const auto &slot : industry.produced) {
		if (!IsValidCargoType(slot.cargo)) { snapshot.active = false; return snapshot; }
		const uint32_t history_room = UINT16_MAX - slot.history[THIS_MONTH].production;
		snapshot.outputs.push_back({CargoSpec::Get(slot.cargo)->label, slot.waiting,
			std::min<uint32_t>(recipe.output_capacity, slot.waiting + history_room)});
	}
	return snapshot;
}

/** @copydoc GetLayerProcessorOperationalStatus */
LayerProcessorOperationalStatus GetLayerProcessorOperationalStatus(const Industry &industry)
{
	using Status = LayerProcessorOperationalStatus;
	const auto *state = GetLayerProcessorState(industry.index);
	const auto *recipe = GetLayerProcessorRecipe(industry);
	// Only the exact installed family and its current saved choice grant an
	// authored processor pose; stale IDs and foreign definitions grant none.
	if (!IsLayerProcessorDefinition(industry.type) || state == nullptr || recipe == nullptr) return Status::MissingState;
	if (!IsLayerIndustryComplete(industry)) return Status::Construction;
	if (industry.prod_level == PRODLEVEL_CLOSURE) return Status::Stopped;
	// Native operating layer and the recipe's calendar retain admission. Art
	// never enables a Surface/Deep substitution or a future receiver early.
	if (_game_mode != GameMode::Normal || GetTileLayer(industry.location.tile) != recipe->layer ||
			TimerGameCalendar::ConvertDateToYMD(TimerGameCalendar::date).year < recipe->introduction_year) return Status::Unavailable;
	if (_pause_mode.Any()) return Status::Paused;
	// Match production's bounded saved-work/revision guards. These conditions
	// report invalid state without repairing, consuming or resetting it.
	if (state->revision == 0 || state->revision == UINT64_MAX || state->scale_fraction >= 200 ||
			state->work_ticks > static_cast<uint64_t>(recipe->recipe.work_economy_ticks) * recipe->recipe.max_batches) return Status::InvalidState;
	const auto snapshot = CaptureLayerProcessorInventory(industry, *state, *recipe);
	if (!snapshot.active) return Status::InvalidState;
	// An empty physical output with exhausted native production history has
	// zero effective snapshot capacity. The common planner refuses that stock
	// shape, but the actual commit guard identifies a legitimate full-history
	// hold, not an invalid Industry. Preserve its real zero room in production.
	for (const auto &role : recipe->recipe.outputs) {
		const auto slot = std::ranges::find(industry.produced, GetCargoTypeByLabel(role.label), &Industry::ProducedCargo::cargo);
		if (slot == industry.produced.end()) return Status::InvalidState;
		if (slot->waiting == 0 && slot->history[THIS_MONTH].production == UINT16_MAX) return Status::OutputFull;
	}
	// Ask the existing common planner whether one whole batch's actual physical
	// prerequisites fit. The duration is a feasibility argument only: banked
	// work is neither copied nor increased. Missing inputs can coexist with
	// banked work, while an empty work bank can still be earning a valid batch.
	const auto plan = PlanMultiLayerRecipeBatches(recipe->recipe, snapshot.inputs, snapshot.outputs,
		recipe->recipe.work_economy_ticks, 0, snapshot.heat_j, snapshot.power_j, UINT32_MAX);
	switch (plan.limiting) {
		case MultiLayerRecipeLimit::None: return Status::Working;
		case MultiLayerRecipeLimit::Input: return Status::Idle;
		case MultiLayerRecipeLimit::Output: return Status::OutputFull;
		default: return Status::InvalidState;
	}
}
