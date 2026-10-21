/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_fleet.h Native five-capsule tuning and explicit complete-fleet installation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group10.
 * @see docs/zz-rhr/upgrades/2026-10/r2mlvs1-e-ground-and-space-budgets.md capsule tuning.
 * Cargo/date/layer policy and metered energy retain their existing canonical
 * catalogues; physical/economy coefficients allocate no identity or funds.
 */

#ifndef TRANSPORT_TUBE_FLEET_H
#define TRANSPORT_TUBE_FLEET_H

#include "newgrf_config.h"
#include <cstdint>

/** Original per-model native coefficients, applied through ordinary Engine/Train economy and physics owners. */
struct TubeVehicleTuning {
	uint16_t model_id; ///< Existing immutable catalogue key, never an EngineID.
	uint16_t capacity; ///< Whole declared cargo units; the canonical label supplies tonnes/crates/kilolitres.
	uint16_t max_speed; ///< Native rail speed units, preserving the existing model9 value100.
	uint16_t power_hp; ///< Native acceleration power; electrical permission is separately metered in joules.
	uint16_t weight_t; ///< Empty native hull mass in tonnes.
	uint8_t price_factor; ///< Ordinary train purchase coefficient against its current economy base.
	uint8_t running_factor; ///< Ordinary electric-train running coefficient, independent of actual paid electricity.
	uint8_t load_amount; ///< Whole units handled per native loading opportunity.
	uint8_t refit_cost; ///< Ordinary refit coefficient for the model's exact permitted cargo forms.
	uint8_t tractive_effort; ///< Native acceleration coefficient; the heavy model retains slower acceleration.
};

/**
 * Inspect original model tuning without installing a definition or changing availability.
 * @param model_id Existing logical model9..13; all other domains/unknown models refuse.
 * @return Borrowed immutable tuning, or nullptr; dates/refits/energy are not copied here.
 */
const TubeVehicleTuning *GetTubeVehicleTuning(uint16_t model_id);

/**
 * Install all five native definitions through complete preflight and the existing EIDS owner.
 * @param activated Exact active native package/cargos; the canonical tube railtype must already exist.
 * @return Complete installation, or unchanged refusal before allocating the first missing model.
 * Model9 retains its prototype identity/stats. Registration grants no purchase
 * date/layer, powered work, toolbar action or cross-layer permission.
 */
bool InitialiseTubeFleetDefinitions(const GRFConfigList &activated);

#endif /* TRANSPORT_TUBE_FLEET_H */
