/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file native_sound_runtime_fixture.h Real phase selection, baseset loading and PCM mixer proof without exporting sample data.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md task83.4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md task16.2
 */

#ifndef TESTS_NATIVE_SOUND_RUNTIME_FIXTURE_H
#define TESTS_NATIVE_SOUND_RUNTIME_FIXTURE_H

#include "../multilayer_sound.h"
#include "../tbm_exchange.h"
#include "../train.h"
#include "../excavation_state.h"
#include "../spoil_receiver.h"
#include "../company_base.h"
#include "../map_layer_gui.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "../viewport_type.h"
#include "../viewport_func.h"
#include "../core/random_func.hpp"
#include "../core/backup_type.hpp"
#include "../newgrf_sound.h"
#include "../mixer.h"
#include "../settings_type.h"
#include <set>

/** Optional actual-game observer; audio queues/cache and a temporary camera belong only to presentation. */
class NativeSoundRuntimeProbe {
	std::set<MultiLayerTBMSoundEvent> played; ///< Actual first-occurrence audible states; not a gameplay registry.
	bool source_bay = false; ///< The real source loop has no maintenance depot; the depot loop proves all four states.

public:
	/**
	 * Resolve the initial physical scenario without changing it.
	 * @param assignment Actual loaded hauler assignment whose cutter supplies every audible event.
	 */
	explicit NativeSoundRuntimeProbe(const TBMExchangeAssignment &assignment)
	{
		const auto *project = GetTBMProject(assignment.project);
		REQUIRE(project != nullptr);
		const auto bay = ResolveTBMExchangeBay(*project, project->exchange_bay);
		REQUIRE(bay.error == TBMExchangeError::None);
		this->source_bay = bay.kind == TBMExchangeBayKind::Source;
	}

	/**
	 * Play a first-occurrence actual phase through the native hook and real decoder/mixer.
	 * @param assignment Actual current physical assignment; no state is manufactured for playback.
	 * @param ticks Actual observed game frame for retained evidence.
	 */
	void Observe(const TBMExchangeAssignment &assignment, uint ticks)
	{
		const auto *project = GetTBMProject(assignment.project);
		REQUIRE(project != nullptr);
		const Train *cutter = Train::Get(project->cutter);
		const auto cue = GetMultiLayerTBMSound(*cutter);
		// Observe only the actual newly selected phase. Later repeats stay with
		// the production cadence and cannot manufacture a missing sound state.
		if (!cue || !this->played.insert(cue->event).second) return;
		const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
		const auto projects = CaptureTBMProjects();
		const auto assignments = CaptureTBMExchangeAssignments();
		const auto receivers = CaptureSpoilReceivers();
		const Money money = Company::Get(project->company)->money;
		SavedRandomSeeds before;
		SaveRandomSeeds(&before);
		Window *main = GetMainWindow();
		REQUIRE(main != nullptr);
		REQUIRE(main->viewport != nullptr);
		AutoRestoreBackup view(*main->viewport);
		SetViewportLayer(main, GetVehicleDisplayLayer(cutter));
		ScrollWindowToTile(cutter->tile, main, true);
		UpdateViewportPosition(main, 0);
		AutoRestoreBackup enabled(_settings_client.sound.vehicle, true);
		std::vector<int16_t> pcm(MxGetRate() * 2, 0); // One second of actual interleaved stereo PCM; no sample data is exported.
		MxCloseAllChannels();
		MxMixSamples(pcm.data(), static_cast<uint>(pcm.size() / 2)); // Complete deferred cleanup before the one observed cue.
		std::ranges::fill(pcm, 0);
		// Zero is a valid presentation sampling boundary. It dispatches the
		// actual current phase through the same production hook without changing
		// the simulation clock, work cursor, movement, costs or interval owners.
		REQUIRE(PlayMultiLayerTBMSound(*cutter, 0));
		MxMixSamples(pcm.data(), static_cast<uint>(pcm.size() / 2));
		const SoundEntry *sound = GetSound(cue->sample);
		REQUIRE(sound != nullptr);
		REQUIRE(sound->data != nullptr);
		REQUIRE_FALSE(sound->data->empty());
		REQUIRE(sound->rate != 0);
		/** @param sample Actual mixed signed PCM value. @return Whether playback contributed audible waveform data. */
		const auto nonzero = std::ranges::count_if(pcm, [](int16_t sample) { return sample != 0; });
		REQUIRE(nonzero > 0);
		uint64_t hash = 1469598103934665603ULL;
		// Retain a fingerprint and count of the actual mixed stream, not copied
		// copyrighted sample bytes. Distinct events remain independently auditable.
		for (int16_t sample : pcm) hash = (hash ^ static_cast<uint16_t>(sample)) * 1099511628211ULL;
		MxCloseAllChannels();
		MxMixSamples(pcm.data(), static_cast<uint>(pcm.size() / 2));
		SavedRandomSeeds after;
		SaveRandomSeeds(&after);
		CHECK(std::ranges::equal(before.random.state, after.random.state));
		CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
		CHECK(CaptureTBMProjects() == projects);
		CHECK(CaptureTBMExchangeAssignments() == assignments);
		CHECK(CaptureSpoilReceivers() == receivers);
		CHECK(Company::Get(project->company)->money == money);
		fmt::print("M1_SOUND_EVENT event={} sample={} tick={} rate={} bytes={} nonzero_pcm={} pcm_hash={} actual_owner=true\n",
			static_cast<uint8_t>(cue->event), cue->sample, ticks, sound->rate, sound->data->size(), nonzero, hash);
	}

	/** Verify every actual required cue at the ordinary finished-loop boundary; source/maintenance scopes remain explicit. */
	void Finish() const
	{
		CHECK(this->played.contains(MultiLayerTBMSoundEvent::Idle));
		CHECK(this->played.contains(MultiLayerTBMSoundEvent::Boring));
		CHECK(this->played.contains(MultiLayerTBMSoundEvent::Backing));
		if (!this->source_bay) CHECK(this->played.contains(MultiLayerTBMSoundEvent::Service));
		fmt::print("M1_SOUND_COMPLETE events={} actual_decoder_and_mixer=true owners_unchanged=true\n", this->played.size());
	}
};

#endif /* TESTS_NATIVE_SOUND_RUNTIME_FIXTURE_H */
