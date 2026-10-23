/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file layer_processor_presentation.h Pure Surface processor availability and actual inventory views.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group39B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Groups10/15.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef LAYER_PROCESSOR_PRESENTATION_H
#define LAYER_PROCESSOR_PRESENTATION_H

#include "layer_industry_processor.h"

/** Derived presentation only; no value stores cargo, advances work or grants recipe authority. */
enum class LayerProcessorOperationalStatus : uint8_t {
	MissingState, ///< Foreign definition, missing native state or unsupported saved recipe; no authored pose authority.
	Construction, ///< At least one actual owned machinery tile remains incomplete.
	Unavailable, ///< Actual operating layer, game mode or selected recipe calendar prevents work.
	Paused, ///< Existing synchronized pause mode holds physical stocks/work.
	Stopped, ///< The existing native industry closure state prevents work.
	Idle, ///< At least one actual required input cannot supply a whole batch.
	Working, ///< Actual all-input/output prerequisites permit earning the next complete batch's work.
	OutputFull, ///< Actual output or production-history room cannot hold a complete batch.
	InvalidState, ///< Malformed native stock/work/revision cannot authorize an operating pose.
};

/**
 * Read the existing processor's operational availability for production and presentation alike.
 * @param industry Actual native facility whose physical layer and construction are checked.
 * @param recipe Existing immutable selected Surface recipe; no copied registry is introduced.
 * @return Whether existing mode, pause, closure, layer, construction and calendar gates admit work.
 */
bool IsLayerProcessorActive(const Industry &industry, const MultiLayerManufacturingRecipe &recipe);

/**
 * Capture the sole native input/output inventory through the same adapter used by production.
 * @param industry Actual Industry owning every physical accepted/produced cargo slot.
 * @param state Its existing saved recipe/work/revision record, never another cargo store.
 * @param recipe Existing immutable selected recipe and input/output capacities.
 * @return Transient read-only snapshot; invalid cargo mappings mark it inactive before dereference.
 */
LayerRecipeInventorySnapshot CaptureLayerProcessorInventory(const Industry &industry, const LayerProcessorState &state,
	const MultiLayerManufacturingRecipe &recipe);

/**
 * Derive a processor's visible operating state without earning work or committing a batch.
 * @param industry Actual native facility; foreign or unbound definitions receive no pose authority.
 * @return Current owner-derived state; the common planner tests one batch's physical feasibility,
 *         independently of banked work, without changing any saved balance, history or RNG.
 */
LayerProcessorOperationalStatus GetLayerProcessorOperationalStatus(const Industry &industry);

#endif /* LAYER_PROCESSOR_PRESENTATION_H */
