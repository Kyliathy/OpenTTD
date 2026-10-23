/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file multilayer_train_art.cpp Actual immutable machine roles, body facing and packet-backed native load poses.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group83
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint8.7
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_sprite.h"
#include "../timer/timer_game_tick.h"
#include "tbm_fixture.h"
#include "tbm_exchange_fixture.h"

#include "../safeguards.h"

/** Authored machinery follows actual fixed roles and body direction; querying cannot change the controller. */
TEST_CASE("Native cutter and permanent support select complete physical views", "[m1-train-art]")
{
	TBMWorkRig rig;
	const auto before = rig.Project();
	REQUIRE(rig.front->Next()->IsArticulatedPart());
	REQUIRE(GetTBMConsistRole(before, *rig.front->Next()) == TBMConsistRole::PermanentSupport);
	AutoRestoreBackup<TimerGameTick::TickCounter> visual_clock(TimerGameTick::counter, 16);
	// Every real eight-way view gets its own body/support poses. Direction is an
	// explicit renderer input, not a camera-derived work heading or engine alias.
	for (uint8_t view = 0; view < 8; ++view) {
		const auto direction = static_cast<Direction>(view);
		CHECK(GetMultiLayerTrainSprite(*rig.front, direction, 16) == GetMultiLayerSprite(MultiLayerSpriteAsset::VehicleModel00, {2, 2, view, 0, 0}));
		CHECK(GetMultiLayerTrainSprite(*rig.front->Next(), direction, 16) == GetMultiLayerSprite(MultiLayerSpriteAsset::VehicleModel00, {2, 2, view, 0, 1}));
		VehicleSpriteSeq sequence;
		rig.front->GetImage(direction, EngineImageType::OnMap, &sequence);
		REQUIRE(sequence.count == 1);
		CHECK(sequence.seq[0].sprite == *GetMultiLayerTrainSprite(*rig.front, direction, 16));
	}
	CHECK_FALSE(GetMultiLayerTrainSprite(*rig.front, Direction::Invalid, 0).has_value());
	CHECK(rig.Project() == before);
	const auto physics = std::tuple(rig.front->acceleration, rig.front->cur_speed, rig.front->gcache.cached_total_length);
	// The real chain includes articulated support and detachable hoppers. A
	// visual refresh must accept every physical part without running the front-
	// only consist/acceleration rebuild or changing the assigned work owner.
	RefreshMultiLayerTrainArt(*rig.front, 16);
	CHECK(std::tuple(rig.front->acceleration, rig.front->cur_speed, rig.front->gcache.cached_total_length) == physics);
	CHECK(rig.Project() == before);
	// The existing whole-project command controls the hold; a renderer must show
	// blocked machinery while leaving paid work, cursor and role assignment alone.
	REQUIRE(ControlTBMProject(rig.project.id, TBMProjectAction::Pause, true) == TBMProjectError::None);
	CHECK(GetMultiLayerTrainSprite(*rig.front, Direction::NE, 8) == GetMultiLayerSprite(MultiLayerSpriteAsset::VehicleModel00, {3, 1, 1, 0, 0}));
}

/** The actual paid cut supplies packets; standing and render clocks cannot hide or animate their load level. */
TEST_CASE("Stopped native spoil wagons preserve actual separate load quantities", "[m1-train-art]")
{
	TBMWorkRig rig;
	const auto earth = rig.Wagons(TBMConsistRole::EarthWagon);
	const auto rock = rig.Wagons(TBMConsistRole::RockWagon);
	REQUIRE_FALSE(earth.empty());
	REQUIRE_FALSE(rock.empty());
	CHECK(GetMultiLayerTrainSprite(*earth.front(), Direction::W, 8) == GetMultiLayerSprite(MultiLayerSpriteAsset::VehicleModel02, {0, 0, 6, 0, 0}));
	const auto cut = rig.BoreOneCut();
	REQUIRE(cut.earth_t > 0);
	REQUIRE(cut.rock_t > 0);
	const auto project = rig.Project();
	const auto material = CaptureTBMMaterial(project);
	// Native wagon art queries every real wagon independently. A mixed cut can
	// fill one wagon and only partly fill another without sharing a display buffer.
	for (auto [wagons, asset] : {std::pair{earth, MultiLayerSpriteAsset::VehicleModel02}, std::pair{rock, MultiLayerSpriteAsset::VehicleModel03}}) {
		for (const Train *wagon : wagons) {
			const uint count = wagon->cargo.StoredCount();
			const uint8_t state = count == 0 ? 0 : count >= wagon->cargo_cap ? 2 : 1;
			CHECK(GetMultiLayerTrainSprite(*wagon, Direction::W, 8) == GetMultiLayerSprite(asset, {state, 0, 6, 0, 0}));
			CHECK(GetMultiLayerTrainSprite(*wagon, Direction::W, 1000) == GetMultiLayerSprite(asset, {state, 0, 6, 0, 0}));
		}
	}
	const auto after = CaptureTBMMaterial(rig.Project());
	CHECK(after.revision == material.revision);
	CHECK(after.earth_space_t == material.earth_space_t);
	CHECK(after.rock_space_t == material.rock_space_t);
	CHECK(after.support_units == material.support_units);
	CHECK(after.available_funds == material.available_funds);
	CHECK(rig.Project() == project);
}

/** Designated exchange identity and physical occupancy come from real owners, never a graphics-only bay registry. */
TEST_CASE("Native depot art follows designated exchange and stopped occupancy", "[m1-train-art]")
{
	TBMExchangeRig rig;
	const TileIndex bay = rig.BuildDepotBay(4);
	const uint8_t direction = static_cast<uint8_t>(GetRailDepotDirection(bay));
	CHECK(GetMultiLayerRailDepotSprite(bay, false, 0) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityRailService, {1, 0, direction, 0, 0}));
	REQUIRE(SetTBMProjectExchangeBay(rig.project.id, bay, true) == TBMProjectError::None);
	REQUIRE(GetTBMProjectForExchangeBay(bay) != nullptr);
	CHECK(GetMultiLayerRailDepotSprite(bay, false, 0) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityExchange, {1, 0, direction, 0, 0}));
	rig.Assign();
	rig.PlaceInDepot(rig.hauler, bay);
	const auto assignments = CaptureTBMExchangeAssignments();
	const auto projects = CaptureTBMProjects();
	CHECK(GetMultiLayerRailDepotSprite(bay, false, 8) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityExchange, {2, 1, direction, 0, 0}));
	CHECK(GetMultiLayerRailDepotSprite(bay, true, 8) == GetMultiLayerSprite(MultiLayerSpriteAsset::FacilityExchange, {2, 1, direction, 0, 1}));
	CHECK(CaptureTBMExchangeAssignments() == assignments);
	CHECK(CaptureTBMProjects() == projects);
	CHECK_FALSE(GetMultiLayerRailDepotSprite(rig.depot, false, 0).has_value());
}

/** A real atomic exchange moves the same loaded unit IDs and packets; their art cannot depend on the old train head. */
TEST_CASE("Native spoil load art follows actual wagon IDs through exchange", "[m1-train-art]")
{
	TBMExchangeRig rig;
	rig.BoreAndMeet();
	std::vector<std::tuple<VehicleID, uint, SpriteID>> loaded;
	// Capture the real separately filled Earth/Rock units before the two-way
	// exchange. Empty units are irrelevant to the loaded-custody assertion here.
	for (auto role : {TBMConsistRole::EarthWagon, TBMConsistRole::RockWagon}) {
		for (const Train *wagon : rig.Wagons(role)) {
			if (wagon->cargo.StoredCount() == 0) continue;
			const auto sprite = GetMultiLayerTrainSprite(*wagon, Direction::W, 24);
			REQUIRE(sprite.has_value());
			loaded.emplace_back(wagon->index, wagon->cargo.StoredCount(), *sprite);
		}
	}
	REQUIRE_FALSE(loaded.empty());
	const auto result = ExecuteTBMWagonExchange(rig.Request());
	REQUIRE(result.error == TBMExchangeError::None);
	// Query those same pool-owned unit IDs after genuine coupling: no copied
	// cargo buffer or manufactured display wagon can satisfy this identity check.
	for (auto [identity, count, sprite] : loaded) {
		const Train *wagon = Train::Get(identity);
		CHECK(wagon->First() == rig.hauler);
		CHECK(wagon->cargo.StoredCount() == count);
		CHECK(GetMultiLayerTrainSprite(*wagon, Direction::W, 24) == sprite);
	}
}
