/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_sound.cpp Presentation-only native TBM phase cues using existing licensed base sounds.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md task83.4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
 */

#include "stdafx.h"
#include "multilayer_sound.h"
#include "train.h"
#include "tbm.h"
#include "rail_map.h"
#include "sound_func.h"
#include "settings_type.h"

#include "safeguards.h"

/** @copydoc GetMultiLayerTBMSound */
std::optional<MultiLayerTBMSound> GetMultiLayerTBMSound(const Train &train)
{
	// Only the actual accepted native cutter front emits machinery cues. Its
	// support/hoppers cannot duplicate sound, and Classic/crashed entities keep
	// their existing ordinary sound/lifecycle behavior.
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		!train.IsFrontEngine() || train.vehstatus.Test(VehState::Crashed) || GetTBMEngineModel(train.engine_type) >= 2) return std::nullopt;
	MultiLayerTBMSoundEvent event = MultiLayerTBMSoundEvent::Idle;
	const auto *project = GetTBMProjectForCutter(train.index);
	// Portal assembly magazines also use Track::Depot. Only an actual rail
	// depot supplies the maintenance cue; a shaft is not a service facility.
	if (train.IsInDepot() && IsRailDepotTile(train.tile)) {
		event = MultiLayerTBMSoundEvent::Service;
	} else if (train.IsDrivingBackwards() && train.cur_speed != 0 && !train.vehstatus.Test(VehState::Stopped)) {
		// Backing is actual reversed motion, not rotating a cutter or cutting
		// backwards. The existing movement owner supplies its warning cue.
		event = MultiLayerTBMSoundEvent::Backing;
	} else if (project != nullptr && project->phase == TBMPhase::Bore && project->blocked == TBMBlockedReason::None && !train.vehstatus.Test(VehState::Stopped)) {
		// A stopped/blocked project cannot sound as though it is cutting. The
		// one saved controller remains the sole authority for active boring.
		event = MultiLayerTBMSoundEvent::Boring;
	}
	return _multilayer_tbm_sounds[static_cast<size_t>(event)];
}

/** @copydoc PlayMultiLayerTBMSound */
bool PlayMultiLayerTBMSound(const Train &train, uint64_t visual_ticks)
{
	// Dedicated clients and the player's vehicle mute setting suppress only
	// presentation. Neither silence nor a sampled cue changes simulation state.
	if (_network_dedicated || !_settings_client.sound.vehicle) return false;
	const auto cue = GetMultiLayerTBMSound(train);
	if (!cue) return false;
	if (visual_ticks % cue->interval != 0) return false;
	SndPlayVehicleFx(cue->sample, &train);
	return true;
}
