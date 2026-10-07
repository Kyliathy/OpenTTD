/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_sound_selection.cpp Actual native work, stopped motion and depot sound-query boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md task83.4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md task16.2
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_sound.h"
#include "tbm_exchange_fixture.h"
#include "../vehicle_cmd.h"

#include "../safeguards.h"

/** Read-only sound sampling follows the actual controller and excludes support/hoppers, preserving paid quantities and both RNG streams. */
TEST_CASE("Native TBM sound queries consume real work and mute owners", "[m1-sound-selection]")
{
	TBMWorkRig rig;
	const auto project = rig.Project();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const Money money = Company::Get(_current_company)->money;
	SavedRandomSeeds before;
	SaveRandomSeeds(&before);
	const auto cue = GetMultiLayerTBMSound(*rig.front);
	REQUIRE(cue.has_value());
	CHECK(cue->event == MultiLayerTBMSoundEvent::Boring);
	CHECK(cue->sample == SND_38_LUMBER_MILL_1);
	CHECK_FALSE(GetMultiLayerTBMSound(*rig.front->Next()).has_value());
	CHECK_FALSE(GetMultiLayerTBMSound(*rig.Wagons(TBMConsistRole::EarthWagon).front()).has_value());
	CHECK_FALSE(PlayMultiLayerTBMSound(*rig.front, 1));
	AutoRestoreBackup muted(_settings_client.sound.vehicle, false);
	CHECK_FALSE(PlayMultiLayerTBMSound(*rig.front, 0));
	CHECK(rig.Project() == project);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == money);
	SavedRandomSeeds after;
	SaveRandomSeeds(&after);
	CHECK(std::ranges::equal(before.random.state, after.random.state));
	CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
	REQUIRE(ControlTBMProject(rig.project.id, TBMProjectAction::Pause, true) == TBMProjectError::None);
	CHECK(GetMultiLayerTBMSound(*rig.front)->event == MultiLayerTBMSoundEvent::Idle);
}

/** A genuine rail depot chooses maintenance sound; the source portal's assembly magazine keeps the ordinary machinery cue. */
TEST_CASE("Native TBM service audio requires actual depot ownership", "[m1-sound-selection]")
{
	TBMExchangeRig rig;
	CHECK(GetMultiLayerTBMSound(*rig.front)->event == MultiLayerTBMSoundEvent::Boring);
	// Existing physical-owner fixture staging places the actual complete cutter
	// chain in the real command-built depot. The audio query cannot create it,
	// move participants, change their packets or perform a service operation.
	rig.PlaceInDepot(rig.front, rig.depot);
	CHECK(GetMultiLayerTBMSound(*rig.front)->event == MultiLayerTBMSoundEvent::Service);
	CHECK(GetMultiLayerTBMSound(*rig.front)->sample == SND_20_CONSTRUCTION_RAIL);
	CHECK_FALSE(GetMultiLayerTBMSound(*rig.hauler).has_value());
}

/** A stationary assembly-magazine direction command cannot manufacture actual backing motion or its warning cue. */
TEST_CASE("Native assembly reversal does not manufacture backing audio", "[m1-sound-selection]")
{
	TBMWorkRig rig;
	const Direction facing = rig.front->direction;
	REQUIRE(Command<Commands::ReverseTrainDirection>::Do(DoCommandFlag::Execute, rig.front->index, false).Succeeded());
	CHECK_FALSE(rig.front->IsDrivingBackwards());
	CHECK(rig.front->direction == facing);
	const auto project = rig.Project();
	CHECK(GetMultiLayerTBMSound(*rig.front)->event != MultiLayerTBMSoundEvent::Backing);
	CHECK(rig.Project() == project);
	CHECK(rig.front->direction == facing);
}
