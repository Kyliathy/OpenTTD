/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_tuning.cpp Publish one immutable operating contract for each distinct mining role.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9
 * @see docs/zz-rhr/upgrades/2026-10/r2mlvs1-e-ground-and-space-budgets.md Heavy mining contract
 */
#include "stdafx.h"
#include "space_mining_tuning.h"
#include "multilayer_space_content.h"
#include "safeguards.h"

/** @copydoc GetSpaceMiningTuning */
const SpaceMiningTuning *GetSpaceMiningTuning(uint16_t model)
{
	const auto &legacy = GetMultiLayerSpaceBudget();
	static const SpaceMiningTuning probe{15, legacy.probe_hold_sixteenths, legacy.mining_units, legacy.mining_interval,
		legacy.active_per_day, legacy.idle_per_day, legacy.service_ticks, 1, 1, legacy.returns_per_service, legacy.starting_pods};
	static constexpr SpaceMiningTuning heavy{SPACE_HEAVY_MINER_MODEL, 40 * 16, 4, 74, 1000, 200, 296, 2, 2, 2, 2};
	switch (model) {
		case 15: return &probe;
		case SPACE_HEAVY_MINER_MODEL: return &heavy;
		default: return nullptr;
	}
}
