/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_prototype.cpp Actual native EIDS purchase, junction reservations and semantic route/entry refusals.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group1B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md task10.1
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_fixture.h"
#include "../transport_tube.h"
#include "../pathfinder/follow_track.hpp"
#include "../tile_cmd.h"
#include "../pbs.h"

#include "../safeguards.h"


/** Native identity/refits/physics follow actual EIDS and catalogue rows, preserving prior definitions and initializer idempotence. */
TEST_CASE("Tube capsule prototype is self propelled and retains canonical engine identity", "[transport-tube-prototype]")
{
	TubePrototypeRig world;
	const auto engine = GetTubeEngine(9);
	const auto count = Engine::GetNumItems();
	REQUIRE(engine != EngineID::Invalid());
	CHECK(GetTubeEngine(10) != EngineID::Invalid());
	CHECK(InitialiseTubeEngineDefinitions(world.ActivatedContent()));
	CHECK(Engine::GetNumItems() == count);
	CHECK(GetTubeEngine(9) == engine);
	CHECK_FALSE(InitialiseTubeEngineDefinitions({}));
	CHECK(Engine::GetNumItems() == count);
	const auto capability = GetTubeVehicleCapability(engine);
	REQUIRE(capability.has_value());
	CHECK(capability->catalogue->model_id == 9);
	CHECK_FALSE(capability->catalogue->conventional_portals);
	CHECK(capability->guideway == world.tube);
	CHECK_FALSE(GetTubeVehicleCapability(GetUndergroundEngine(4)).has_value());
	const auto *definition = Engine::Get(engine);
	CHECK(definition->VehInfo<RailVehicleInfo>().power > 0);
	CHECK(definition->VehInfo<RailVehicleInfo>().railveh_type == RailVehicleType::Singlehead);
	// Every refit comes from the existing label owner. A capsule cannot gain
	// passengers or change a liquid into bulk from a compatibility/class mask.
	for (const auto label : capability->catalogue->cargo_labels) CHECK(definition->info.refit_mask.Test(GetCargoTypeByLabel(label)));
	CHECK_FALSE(definition->info.refit_mask.Test(GetCargoTypeByLabel(CargoLabel{"PASS"})));
	Train *capsule = world.BuyCapsule();
	CHECK(capsule->IsFrontEngine());
	CHECK(capsule->cargo_cap == 60);
	CHECK(capsule->gcache.cached_power > 0);
}

/** The normal follower/reservation owners handle real tube track/junction geometry without synthetic rail movement. */
TEST_CASE("Tube capsule follows a real junction and shares ordinary exclusive reservations", "[transport-tube-prototype]")
{
	TubePrototypeRig world;
	Train *capsule = world.BuyCapsule();
	const auto before = TileXYL(42, 40, MapLayer::Underground);
	const auto junction = TileAddWrap(before, 1, 0);
	world.BuildTrack(before, world.tube);
	world.BuildTrack(junction, world.tube);
	world.BuildTrack(junction, world.tube, Track::Upper);
	capsule->tile = before; // Explicit initial test placement, not a claim of actual driven or powered movement.
	CFollowTrackRail follower(capsule);
	REQUIRE(follower.Follow(before, Trackdir::X_SW));
	CHECK(follower.new_tile == junction);
	CHECK(follower.new_td_bits.Any(TrackdirToTrackdirBits(Trackdir::X_SW)));
	REQUIRE(TryReserveRailTrack(junction, Track::X));
	CHECK_FALSE(TryReserveRailTrack(junction, Track::Upper));
	CHECK(GetReservedTrackbits(junction).Test(Track::X));
	UnreserveRailTrack(junction, Track::X);
	CHECK(TryReserveRailTrack(junction, Track::Upper));
	UnreserveRailTrack(junction, Track::Upper);
}

/** Broad native/foreign masks cannot override either route discovery or actual tile entry in either direction. */
TEST_CASE("Tube and conventional trains refuse foreign movement domains despite broad masks", "[transport-tube-prototype]")
{
	TubePrototypeRig world;
	Train *capsule = world.BuyCapsule();
	const auto plain_depot = world.BuildDepot(MapLayer::Underground, false);
	Train *ordinary = Train::From(world.Buy(plain_depot, 4));
	const auto ordinary_tile = TileXYL(44, 42, MapLayer::Underground);
	const auto tube_tile = TileAddWrap(ordinary_tile, 1, 0);
	world.BuildTrack(ordinary_tile, RAILTYPE_RAIL);
	world.BuildTrack(tube_tile, world.tube);
	ordinary->tile = ordinary_tile;
	capsule->tile = tube_tile;
	const RailTypes broad{RAILTYPE_RAIL, RAILTYPE_ELECTRIC, RAILTYPE_MONO, RAILTYPE_MAGLEV, world.tube};
	ordinary->compatible_railtypes = broad;
	capsule->compatible_railtypes = broad;
	// Deliberately hostile mechanical masks exercise the separate domain guard;
	// they do not create an electrical grant or permit a mixed physical consist.
	const_cast<RailTypeInfo *>(GetRailTypeInfo(RAILTYPE_RAIL))->compatible_railtypes = broad;
	const_cast<RailTypeInfo *>(GetRailTypeInfo(world.tube))->compatible_railtypes = broad;
	CHECK(CanTubeTrainEnterTile(*ordinary, ordinary_tile));
	CHECK(CanTubeTrainEnterTile(*capsule, tube_tile));
	CHECK_FALSE(CanTubeTrainEnterTile(*ordinary, tube_tile));
	CHECK_FALSE(CanTubeTrainEnterTile(*capsule, ordinary_tile));
	CFollowTrackRail ordinary_path(ordinary, broad);
	CHECK_FALSE(ordinary_path.Follow(ordinary_tile, Trackdir::X_SW));
	CFollowTrackRail capsule_path(capsule, broad);
	CHECK_FALSE(capsule_path.Follow(tube_tile, Trackdir::X_NE));
	CHECK(VehicleEnterTile(ordinary, tube_tile, 0, 0).Test(VehicleEnterTileState::CannotEnter));
	CHECK(VehicleEnterTile(capsule, ordinary_tile, 0, 0).Test(VehicleEnterTileState::CannotEnter));
	CHECK_FALSE(CanTubeTrainEnterTile(*capsule, TileXYL(45, 42, MapLayer::Surface)));
	CHECK_FALSE(CanTubeTrainEnterTile(*capsule, TileXYL(45, 42, MapLayer::Space)));
}

/** A genuine published cutter/support/spoil consist cannot borrow the tube domain from broadened masks or forced tile entry. */
TEST_CASE("An acquired TBM consist refuses tube guideways independently of compatibility masks", "[transport-tube-prototype]")
{
	TBMProjectRig world;
	const auto tube = InitialiseTubeRailType(world.fixture.package);
	REQUIRE(tube != INVALID_RAILTYPE);
	const auto tile = TileXYL(52, 48, MapLayer::Underground);
	world.fixture.world.Open(std::array{tile});
	Company::Get(_current_company)->avail_railtypes.Set(tube);
	REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, tile, tube, Track::X, false).Succeeded());
	world.front->compatible_railtypes.Set(tube);
	CHECK_FALSE(CanTubeTrainEnterTile(*world.front, tile));
	CHECK(VehicleEnterTile(world.front, tile, 0, 0).Test(VehicleEnterTileState::CannotEnter));
	CHECK(GetTBMConsistRole(world.project, *world.front) == TBMConsistRole::Cutter);
}

/** Required cargo, persisted mapping collisions and registry retirement refuse without allocating or overwriting a definition. */
TEST_CASE("Tube engine preflight preserves definition owners on malformed prerequisites", "[transport-tube-definition-refusal]")
{
	TubePrototypeRig world;
	const auto count = Engine::GetNumItems();
	const auto capsule = GetTubeEngine(9);
	SECTION("missing required refit label") {
		CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"COBT"}))->label = CargoLabel{"MISS"};
		BuildCargoLabelMap();
		CHECK_FALSE(InitialiseTubeEngineDefinitions(world.ActivatedContent()));
		CHECK(GetTubeEngine(9) == capsule);
	}
	SECTION("foreign definition in persisted EIDS slot") {
		const auto &package = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package;
		const auto local = Engine::Get(capsule)->grf_prop.local_id;
		const auto ordinary = GetUndergroundEngine(4);
		_engine_mngr.SetID(VehicleType::Train, local, package.grfid, 0, ordinary);
		CHECK_FALSE(InitialiseTubeEngineDefinitions(world.ActivatedContent()));
		CHECK(GetTubeEngine(9) == EngineID::Invalid());
		CHECK(GetUndergroundEngine(4) == ordinary);
	}
	SECTION("retired canonical rail registry") {
		ResetRailTypes();
		CHECK_FALSE(InitialiseTubeEngineDefinitions(world.ActivatedContent()));
		const auto capability = GetTubeVehicleCapability(capsule);
		REQUIRE(capability.has_value());
		CHECK(capability->guideway == INVALID_RAILTYPE);
	}
	CHECK(Engine::GetNumItems() == count);
}
