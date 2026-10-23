/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_facility_sprite.cpp Actual native tube facility geometry, service demand, packet queues and display-only animation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "transport_tube_meter_fixture.h"
#include "../transport_tube_sprite.h"
#include "../multilayer_sprite.h"
#include "../station_supply.h"
#include "../station_cmd.h"
#include "../station_map.h"
#include "../landscape_cmd.h"
#include "../animated_tile_func.h"
#include "../animated_tile_map.h"
#include "../core/random_func.hpp"
#include "../safeguards.h"

/**
 * Snapshot original electrical, native packet count, company, simulation and RNG owners independently of pixels.
 * @return Comparable actual owners; native viewport dirtiness is intentionally a separate display cache.
 */
static auto CaptureTubeFacilityOwners()
{
	return std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), ComputeTubePowerVehicleDigest(),
		CargoPacket::GetNumItems(), Station::GetNumItems(), Company::Get(_current_company)->money, TimerGameTick::counter,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
}

/**
 * Check actual aligned facility parts against the independently declared native source matrix.
 * @param tile Actual command-built default native tube station/depot.
 * @param asset Original terminal, joined interchange or service source family.
 * @param state Expected original ready/working/blocked condition from real owner actions.
 * @param view Actual two-way platform axis or four-way depot door.
 * @param role Native default StationGfx role0..3, or zero for a one-cell depot.
 */
static void CheckTubeFacilityFrames(TileIndex tile, MultiLayerSpriteAsset asset, uint8_t state, uint8_t view, uint16_t role = 0)
{
	const auto owners = CaptureTubeFacilityOwners();
	// All four lamp phases must retain the same actual geometry/packet owners;
	// frame selection never completes a build, services a capsule or loads cargo.
	for (uint8_t frame = 0; frame < 4; ++frame) {
		const auto art = GetTubeFacilityArt(tile, frame * 8); REQUIRE(art.has_value());
		CHECK(art->sprites.back == *GetMultiLayerSprite(asset, {state, frame, view, role, 0}));
		CHECK(art->sprites.front == *GetMultiLayerSprite(asset, {state, frame, view, role, 1}));
		CHECK(art->front_bounds.origin.x + art->front_bounds.offset.x == 0);
		CHECK(art->front_bounds.origin.y + art->front_bounds.offset.y == 0);
		CHECK(art->front_bounds.origin.z + art->front_bounds.offset.z == 0);
	}
	CHECK(CaptureTubeFacilityOwners() == owners);
}

/** All actual default native graphics roles retain their own station cell, through axis and joined packet identity. */
TEST_CASE("Tube terminal and interchange art follows real native platform roles and joins", "[tube-facility-art-geometry]")
{
	LoadGeneratedWindowTestLanguage(); TubePrototypeRig world;
	AutoRestoreBackup clearing(Company::Get(_current_company)->clear_limit, uint32_t{256} << 16);
	std::array<bool, 8> seen{};
	uint layout = 0;
	// The native builder produces bare/building/canopy roles from real berth
	// dimensions. Never set StationGfx or clone a decorative source tile into
	// the map to manufacture missing runtime role coverage.
	for (Axis axis : {Axis::X, Axis::Y}) {
		for (uint8_t tracks : {1, 2}) {
			for (uint8_t length : {1, 2, 5}) {
				CAPTURE(axis, tracks, length);
				const auto first = TileXYL(12 + layout % 4 * 12, 12 + layout / 4 * 12, MapLayer::Underground); ++layout;
				// Only these native berth cells and their actual through approaches
				// are fixture-open input; artwork cannot excavate a larger campus.
				for (uint across = 0; across < tracks; ++across) {
					for (int along = -1; along <= length; ++along) {
						world.Open(std::array{TileAddWrap(first, axis == Axis::X ? along : static_cast<int>(across), axis == Axis::Y ? along : static_cast<int>(across))});
					}
				}
				REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, first, world.tube, axis, tracks, length, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
				const auto station = GetStationIndex(first);
				// Every actual cell must select its complete own role/axis pair;
				// station identity and finite shared packets remain native-owned.
				for (uint across = 0; across < tracks; ++across) {
					for (uint along = 0; along < length; ++along) {
						const auto tile = TileAddWrap(first, axis == Axis::X ? along : across, axis == Axis::Y ? along : across);
						const auto gfx = GetStationGfx(tile); REQUIRE(gfx < seen.size()); seen[gfx] = true;
						CHECK(GetBuiltTubeTerminal(tile, _current_company)->station == station);
						CheckTubeFacilityFrames(tile, MultiLayerSpriteAsset::FacilityTubeTerminal, 3, static_cast<uint8_t>(axis), gfx / 2);
						CHECK(GetAnimatedTileState(tile) == AnimatedTileState::Animated);
					}
				}
			}
		}
	}
	CHECK(std::ranges::all_of(seen, std::identity{}));
	// Four real native depot doors preserve the frozen one-cell service
	// source views and original front sorting; no camera-facing alias is used.
	for (uint8_t view = 0; view < 4; ++view) {
		const auto tile = TileXYL(8 + view * 8, 48, MapLayer::Underground);
		const auto door = static_cast<DiagDirection>(view);
		world.Open(std::array{tile, TileAddByDiagDir(tile, door)});
		REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, tile, world.tube, door).Succeeded());
		CheckTubeFacilityFrames(tile, MultiLayerSpriteAsset::FacilityTubeService, 3, view);
	}
	// A real distant-joined conventional berth changes only presentation
	// identity. An unrelated neighbouring station would keep separate packets.
	AutoRestoreBackup distant_join(_settings_game.station.distant_join_stations, true); // Explicit native join policy is a fixture input, not permission inferred from interchange artwork.
	const auto tube_tile = TileXYL(12, 12, MapLayer::Underground);
	const auto joined = TileAddWrap(tube_tile, 0, 3); world.Open(std::array{joined, TileAddWrap(joined, -1, 0), TileAddWrap(joined, 1, 0)});
	const auto station = GetStationIndex(tube_tile);
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, joined, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, station, false).Succeeded());
	CHECK(GetStationIndex(joined) == station);
	CHECK_FALSE(GetTubeFacilityArt(joined, 0).has_value());
	CheckTubeFacilityFrames(tube_tile, MultiLayerSpriteAsset::FacilityTubeInterchange, 3, 0, GetStationGfx(tube_tile) / 2);
	// A separately owned original tube station joins only its actual truck
	// stop. Freight-road membership is sufficient without a conventional rail
	// berth, and never changes either station's original packet list.
	const auto road_tube = TileXYL(24, 12, MapLayer::Underground);
	const auto road = TileAddWrap(road_tube, 0, 3);
	world.Open(std::array{road, TileAddByDiagDir(road, DiagDirection::SE)});
	REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, road, 1, 1, RoadStopType::Truck, false, DiagDirection::SE,
		ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, GetStationIndex(road_tube), false).Succeeded());
	CheckTubeFacilityFrames(road_tube, MultiLayerSpriteAsset::FacilityTubeInterchange, 3, 0, GetStationGfx(road_tube) / 2);
	AutoRestoreBackup locked(TimerGameCalendar::date, TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1969}, 11, 31));
	CHECK_FALSE(GetTubeTerminal(tube_tile, _current_company).has_value());
	REQUIRE(GetBuiltTubeTerminal(tube_tile, _current_company).has_value());
	CheckTubeFacilityFrames(tube_tile, MultiLayerSpriteAsset::FacilityTubeInterchange, 3, 0, GetStationGfx(tube_tile) / 2);
}

/** A healthy parked capsule is ready; actual repair demand and real source loss select separate native states. */
TEST_CASE("Tube depot art reads genuine service demand without earning repairs", "[tube-facility-art-service]")
{
	TubeMeterRig world; world.Bind(); world.Run();
	CheckTubeFacilityFrames(world.depot, MultiLayerSpriteAsset::FacilityTubeService, 1, static_cast<uint8_t>(DiagDirection::SE));
	world.capsule->reliability = 100;
	++TimerGameTick::counter; PrepareTubePowerTick();
	const auto used = GetUtilityPowerProducer(world.source)->consumed_lifetime_j;
	CheckTubeFacilityFrames(world.depot, MultiLayerSpriteAsset::FacilityTubeService, 2, static_cast<uint8_t>(DiagDirection::SE));
	CHECK(world.capsule->reliability == 100); CHECK(world.capsule->IsStoppedInDepot());
	CHECK(GetUtilityPowerProducer(world.source)->consumed_lifetime_j == used);
	REQUIRE(world.capsule->Tick()); CHECK(GetUtilityPowerProducer(world.source)->consumed_lifetime_j == used + 540);
	CheckTubeFacilityFrames(world.depot, MultiLayerSpriteAsset::FacilityTubeService, 1, static_cast<uint8_t>(DiagDirection::SE));
	REQUIRE(CloseTubeExternalPower(world.source) == TubePowerError::None);
	CheckTubeFacilityFrames(world.depot, MultiLayerSpriteAsset::FacilityTubeService, 3, static_cast<uint8_t>(DiagDirection::SE));
	CHECK(world.capsule->IsStoppedInDepot());
	// Existing native animation registration/callbacks only dirty pixels. They
	// retain current electrical, packet, stop and money owners exactly.
	const auto owners = CaptureTubeFacilityOwners();
	REQUIRE(GetAnimatedTileState(world.depot) == AnimatedTileState::Animated);
	RegisterTubeFacilityArtAnimation(world.depot); AnimateAnimatedTiles();
	CHECK(CaptureTubeFacilityOwners() == owners);
	REQUIRE(CmdLandscapeClear(DoCommandFlag::Execute, world.destination).Succeeded());
	AnimateAnimatedTiles(); CHECK(GetAnimatedTileState(world.destination) == AnimatedTileState::None);
}

/** Actual native station capacity pressure changes a lamp without becoming another packet store. */
TEST_CASE("Tube platform art warns on the exact original full native packet queue", "[tube-facility-art-stock]")
{
	LoadGeneratedWindowTestLanguage(); TubeMeterRig world; world.Bind();
	const auto tile = TileXYL(38, 44, MapLayer::Underground);
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, world.tube, Axis::Y, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	world.Run(); REQUIRE(IsTubeTileEnergized(tile));
	const auto role = GetStationGfx(tile) / 2;
	CheckTubeFacilityFrames(tile, MultiLayerSpriteAsset::FacilityTubeTerminal, 1, 1, role);
	Station *station = Station::GetByTile(tile);
	// Ordinary native orders and ticks, rather than a forged loading phase or
	// fixture teleport, bring this same capsule to the new actual platform.
	REQUIRE(CmdDeleteOrder(DoCommandFlag::Execute, world.capsule->index, 0).Succeeded());
	Order order; order.MakeGoToStation(station->index);
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, world.capsule->index, 0, order).Succeeded());
	ProcessOrders(world.capsule); world.Start();
	uint ticks = 0;
	while (!world.capsule->current_order.IsType(OT_LOADING) && ticks < 800) { world.Run(); ++ticks; } // A bounded real arrival must precede working art; no waiting-state write can satisfy it.
	REQUIRE(world.capsule->current_order.IsType(OT_LOADING)); REQUIRE(world.capsule->tile == tile);
	CheckTubeFacilityFrames(tile, MultiLayerSpriteAsset::FacilityTubeTerminal, 2, 1, role);
	const auto cargo = GetCargoTypeByLabel(CT_COAL);
	auto &queue = station->goods[cargo].GetOrCreateData().cargo;
	REQUIRE(CargoPacket::CanAllocateItem());
	// Declared native receiver pressure is one actual station packet; no
	// graphics-owned buffer or forged capacity flag stands in for real stock.
	queue.Append(CargoPacket::Create(station->index, EXACT_STATION_SUPPLY_CAPACITY, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
	const auto packet = queue.Packets()->begin()->second.front()->index;
	CheckTubeFacilityFrames(tile, MultiLayerSpriteAsset::FacilityTubeTerminal, 3, 1, role);
	CHECK(queue.TotalCount() == EXACT_STATION_SUPPLY_CAPACITY);
	CHECK(queue.Packets()->begin()->second.front()->index == packet);
}
