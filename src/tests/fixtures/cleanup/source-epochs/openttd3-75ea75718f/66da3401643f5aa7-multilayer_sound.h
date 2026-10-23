/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_sound.h Read-only native TBM sound cues reusing the selected licensed sound baseset.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md task83.4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md task16.2
 */

#ifndef MULTILAYER_SOUND_H
#define MULTILAYER_SOUND_H

#include "sound_type.h"
#include <array>
#include <cstdint>
#include <optional>

/** Existing physical train owner; a sound cue never allocates a machine or changes its assignment. */
struct Train;

/** Transient audible states; these are not saved work/controller phases or new commands. */
enum class MultiLayerTBMSoundEvent : uint8_t { Idle, Boring, Backing, Service };

/** One native cue bound to an existing global SoundFx slot in the selected sounds baseset. */
struct MultiLayerTBMSound {
	MultiLayerTBMSoundEvent event; ///< State derived from actual controller/motion/depot owners.
	SoundID sample; ///< Existing base sound slot; no proprietary samples are copied into native content.
	uint16_t interval; ///< Visual/audio tick sampling cadence, independent of work accumulation.
};

/** Native reuse contract: machinery hum, powered cutting, reversing bell and depot tool strike. */
static constexpr std::array<MultiLayerTBMSound, 4> _multilayer_tbm_sounds{{
	{MultiLayerTBMSoundEvent::Idle, SND_0B_MINE, 192},
	{MultiLayerTBMSoundEvent::Boring, SND_38_LUMBER_MILL_1, 96},
	{MultiLayerTBMSoundEvent::Backing, SND_0E_LEVEL_CROSSING, 96},
	{MultiLayerTBMSoundEvent::Service, SND_20_CONSTRUCTION_RAIL, 192},
}};

/**
 * Query one actual native cutter's audible state without advancing its work.
 * @param train Actual physical front engine; support/hoppers/foreign/Classic/crashed trains receive no native cue.
 * @return Existing base sample and cadence derived from service, backing, unblocked boring or idle state.
 */
std::optional<MultiLayerTBMSound> GetMultiLayerTBMSound(const Train &train);

/**
 * Emit a bounded phase cue through the existing vehicle sound/mixer path.
 * @param train Actual front engine whose ordinary tick already owns its movement/work.
 * @param visual_ticks Read-only native tick sample; it never becomes work, money or production time.
 * @return Whether a native cue was dispatched; audibility remains with viewport, base sound and volume settings.
 */
bool PlayMultiLayerTBMSound(const Train &train, uint64_t visual_ticks);

#endif /* MULTILAYER_SOUND_H */
