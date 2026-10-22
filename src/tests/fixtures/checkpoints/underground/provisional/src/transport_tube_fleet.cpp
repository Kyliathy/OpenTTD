/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_fleet.cpp Original native capsule physical/economy tuning.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group10.
 * @see docs/zz-rhr/upgrades/2026-10/r2mlvs1-e-ground-and-space-budgets.md capsule tuning.
 */

#include "stdafx.h"
#include "transport_tube_fleet.h"
#include "safeguards.h"

/** @copydoc GetTubeVehicleTuning */
const TubeVehicleTuning *GetTubeVehicleTuning(uint16_t model_id)
{
	static constexpr TubeVehicleTuning tuning[] = {
		{9, 60, 100, 1000, 20, 120, 100, 8, 10, 128},
		{10, 40, 140, 1200, 18, 150, 110, 12, 10, 128},
		{11, 40, 100, 1000, 25, 180, 130, 6, 15, 128},
		{12, 24, 110, 1000, 35, 220, 180, 4, 20, 128},
		{13, 96, 80, 900, 60, 250, 220, 4, 20, 96},
	};
	const auto found = std::ranges::find(tuning, model_id, &TubeVehicleTuning::model_id);
	return found == std::end(tuning) ? nullptr : found;
}
