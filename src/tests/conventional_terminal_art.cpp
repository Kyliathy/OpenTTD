/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file conventional_terminal_art.cpp Actual native truck-stop layouts and complete floor/approach admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group25B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "excavation_fixture.h"
#include "../conventional_terminal_art.h"
#include "../station_cmd.h"
#include "../station_func.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../roadstop_base.h"
#include "../newgrf_roadstop.h"
#include "../road.h"
#include "../town.h"
#include "../table/strings.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

/** Command-ready native truck-stop definitions; explicit fixture floors never claim paid gameplay excavation. */
class ConventionalTerminalArtRig : public ExcavationFixture {
	AutoRestoreBackup<CompanyID> local{_local_company, _current_company}; ///< Restore the caller's presentation company.
public:
	/** Initialize actual ordinary road definitions, naming town and station spatial owner. */
	ConventionalTerminalArtRig()
	{
		ResetRoadTypes();
		RoadStopClass::Reset();
		REQUIRE(Station::GetNumItems() == 0);
		REQUIRE(RoadStop::GetNumItems() == 0);
		REQUIRE(Town::GetNumItems() == 0);
		REQUIRE(Town::CanAllocateItem());
		Town *town = Town::Create(TileXYL(10, 10, MapLayer::Surface));
		town->townnametype = SPECSTR_TOWNNAME_START;
		RebuildTownKdtree();
		RebuildStationKdtree();
	}

	/** Retire real station catchments/stops before the inherited company/map and naming town. */
	~ConventionalTerminalArtRig()
	{
		// Native destruction retires signs and catchment indexes; explicit stop
		// cleanup follows the existing actual-facility fixture's ownership order.
		for (Station *station : Station::Iterate()) delete station;
		_roadstop_pool.CleanPool();
		_town_pool.CleanPool();
		RebuildTownKdtree();
	}
};

/**
 * Emit one actual compiled renderer layout for a complete-success-only source publisher.
 * @param geometry Native default truck-stop value projection; no world mutation.
 */
static void PrintTerminalArtGeometry(const ConventionalRoadTerminalArtGeometry &geometry)
{
	static constexpr std::array<const char *, 6> directions{{"NE", "SE", "SW", "NW", "X", "Y"}};
	fmt::print("TERMINAL_ART_GEOMETRY {{\"graphics\":{},\"direction\":\"{}\",\"drive_through\":{},\"entrance\":",
		geometry.graphics, directions[geometry.graphics], geometry.drive_through);
	if (geometry.entrance) {
		fmt::print("\"{}\"", directions[to_underlying(*geometry.entrance)]);
	} else {
		fmt::print("null");
	}
	fmt::print(",\"axis\":");
	if (geometry.axis) {
		fmt::print("\"{}\"", *geometry.axis == Axis::X ? "X" : "Y");
	} else {
		fmt::print("null");
	}
	fmt::print(",\"parents\":[");
	// Every parent remains in actual renderer sequence order. Through layouts
	// explicitly lack a third owner; no missing original may imitate that fact.
	for (size_t index = 0; index < geometry.parents.size(); ++index) {
		if (index != 0) fmt::print(",");
		if (!geometry.parents[index]) {
			fmt::print("null");
			continue;
		}
		const auto &part = *geometry.parents[index];
		fmt::print("{{\"origin\":[{},{},{}],\"extent\":[{},{},{}],\"offset\":[{},{},{}]}}",
			part.origin.x, part.origin.y, part.origin.z, part.extent.x, part.extent.y, part.extent.z,
			part.offset.x, part.offset.y, part.offset.z);
	}
	fmt::print("]}}\n");
}

/** Six real native layouts retain their actual direction/axis and independent bay/through sorting parents. */
TEST_CASE("Conventional terminal source geometry projects every native truck stop parent", "[conventional-terminal-art]")
{
	// Four directed bays and two undirected through cells are the entire actual
	// native family; fallback graphics must never count as another authored view.
	for (uint8_t graphics = 0; graphics < 6; ++graphics) {
		CAPTURE(graphics);
		const auto geometry = GetConventionalRoadTerminalArtGeometry(graphics);
		REQUIRE(geometry.has_value());
		CHECK(geometry->graphics == graphics);
		CHECK(geometry->drive_through == (graphics >= 4));
		CHECK(geometry->entrance.has_value() == (graphics < 4));
		CHECK(geometry->axis.has_value() == (graphics >= 4));
		if (graphics < 4) CHECK(*geometry->entrance == static_cast<DiagDirection>(graphics));
		if (graphics >= 4) CHECK(*geometry->axis == static_cast<Axis>(graphics - 4));
		size_t parent = 0;
		// Compare every source-export field with the renderer's actual immutable
		// sequence, preserving unusual one-unit fences and offset semantics.
		for (const auto &piece : GetStationTileLayout(StationType::Truck, graphics)->GetSequence()) {
			REQUIRE(piece.IsParentSprite());
			REQUIRE(parent < geometry->parents.size());
			REQUIRE(geometry->parents[parent]);
			const auto &bounds = *geometry->parents[parent++];
			CHECK(bounds.origin == piece.origin);
			CHECK(bounds.extent == piece.extent);
			CHECK(bounds.offset == piece.offset);
			CHECK(bounds.extent.z == (graphics < 4 ? 10 : 16));
		}
		CHECK(parent == (graphics < 4 ? 3 : 2));
		CHECK(geometry->parents[2].has_value() == (graphics < 4));
		PrintTerminalArtGeometry(*geometry);
	}
	CHECK(GetStationTileLayout(StationType::Truck, 6) == GetStationTileLayout(StationType::Truck, 0));
	for (uint8_t graphics : {6, 7, 255}) CHECK_FALSE(GetConventionalRoadTerminalArtGeometry(graphics));
}

/** Real commands require all floors and the actual one-ended/both-ended approaches before allocation. */
TEST_CASE("Conventional terminal source follows real variable road stop construction and approaches", "[conventional-terminal-art]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const auto layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground);
	const uint8_t graphics = GENERATE(0, 1, 2, 3, 4, 5);
	const bool opposite_holdout = GENERATE(false, true);
	ConventionalTerminalArtRig rig;
	const bool through = graphics >= 4;
	const DiagDirection entrance = graphics < 4 ? static_cast<DiagDirection>(graphics) : graphics == 4 ? DiagDirection::NE : DiagDirection::NW;
	const auto step = TileIndexDiffCByDiagDir(entrance);
	const std::array<std::pair<uint8_t, uint8_t>, 4> dimensions{{{1, 1}, {1, 2}, {2, 1}, {2, 3}}};
	// Native commands build one cell per RoadStop and may group a variable
	// rectangle into one Station. A fixed2x2 source campus is not the owner.
	for (uint index = 0; index < dimensions.size(); ++index) {
		const auto [width, height] = dimensions[index];
		const auto tile = TileXYL(12 + (index % 2) * 24, 16 + (index / 2) * 24, layer);
		CAPTURE(layer, graphics, width, height, opposite_holdout);
		std::vector<TileIndex> floor, approaches;
		// Boundary approaches derive independently from the actual entrance.
		// Internal neighbours are paid floor already, and through stops alone
		// need the opposite boundary; avoid counting shared corner cells twice.
		for (uint y = 0; y < height; ++y) {
			for (uint x = 0; x < width; ++x) {
				floor.push_back(TileAddWrap(tile, x, y));
				for (int sign : {1, -1}) {
					if (sign == -1 && !through) continue;
					const int ax = static_cast<int>(x) + sign * step.x;
					const int ay = static_cast<int>(y) + sign * step.y;
					if (ax >= 0 && ay >= 0 && ax < width && ay < height) continue;
					const auto approach = TileAddWrap(tile, ax, ay);
					if (std::ranges::find(approaches, approach) == approaches.end()) approaches.push_back(approach);
				}
			}
		}
		const auto stations = Station::GetNumItems(), stops = RoadStop::GetNumItems();
		const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
		const auto cash = Company::Get(_current_company)->money;
		CHECK(CmdBuildRoadStop({}, tile, width, height, RoadStopType::Truck, through, entrance, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Failed());
		CHECK(CmdBuildRoadStop(DoCommandFlag::Execute, tile, width, height, RoadStopType::Truck, through, entrance, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Failed());
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
		const auto last_floor = floor.back();
		rig.Open(std::span{floor}.first(floor.size() - 1));
		CHECK(CmdBuildRoadStop(DoCommandFlag::Execute, tile, width, height, RoadStopType::Truck, through, entrance, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Failed());
		rig.Open(std::array{last_floor});
		CHECK(CmdBuildRoadStop(DoCommandFlag::Execute, tile, width, height, RoadStopType::Truck, through, entrance, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Failed());
		if (opposite_holdout) std::ranges::reverse(approaches);
		const auto last_approach = approaches.back();
		approaches.pop_back();
		rig.Open(approaches);
		const auto incomplete = ComputeGeologyStateDigest(GetGeologyWorldState());
		CHECK(CmdBuildRoadStop(DoCommandFlag::Execute, tile, width, height, RoadStopType::Truck, through, entrance, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Failed());
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == incomplete);
		CHECK(Station::GetNumItems() == stations);
		CHECK(RoadStop::GetNumItems() == stops);
		CHECK(Company::Get(_current_company)->money == cash);
		rig.Open(std::array{last_approach});
		REQUIRE(CmdBuildRoadStop({}, tile, width, height, RoadStopType::Truck, through, entrance, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, tile, width, height, RoadStopType::Truck, through, entrance, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		CHECK(Station::GetNumItems() == stations + 1);
		CHECK(RoadStop::GetNumItems() == stops + width * height);
		// Installed cells resolve their real native identity; rendering never
		// merges them into another footprint or bypasses physical construction.
		for (const auto cell : floor) {
			REQUIRE(IsTruckStop(cell));
			const auto geometry = GetConventionalRoadTerminalTileArtGeometry(cell);
			REQUIRE(geometry);
			CHECK(geometry->graphics == graphics);
			CHECK(geometry->drive_through == IsDriveThroughStopTile(cell));
			if (through) CHECK(*geometry->axis == GetDriveThroughStopAxis(cell));
			if (!through) CHECK(*geometry->entrance == GetBayRoadStopDir(cell));
		}
	}
}

/** Read-only source selection refuses other native owners and preserves synchronized world state. */
TEST_CASE("Conventional terminal art refuses foreign custom Surface Classic and invalid queries", "[conventional-terminal-art]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ConventionalTerminalArtRig rig;
	const auto tile = TileXYL(20, 20, MapLayer::Underground);
	CHECK_FALSE(GetConventionalRoadTerminalTileArtGeometry(INVALID_TILE));
	CHECK_FALSE(GetConventionalRoadTerminalTileArtGeometry(tile));
	rig.Open(std::array{tile, TileAddWrap(tile, 0, 1)});
	REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, tile, 1, 1, RoadStopType::Truck, false, DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money;
	const auto seeds = _random, interactive = _interactive_random;
	REQUIRE(GetConventionalRoadTerminalTileArtGeometry(tile));
	// Temporary malformed/custom fields characterize query refusal only; they
	// do not claim that an invalid stop was constructed through a normal command.
	SetCustomRoadStopSpecIndex(tile, 1);
	CHECK_FALSE(GetConventionalRoadTerminalTileArtGeometry(tile));
	SetCustomRoadStopSpecIndex(tile, 0);
	SetStationGfx(tile, 6);
	CHECK_FALSE(GetConventionalRoadTerminalTileArtGeometry(tile));
	SetStationGfx(tile, 1);
	const auto policy = GetCurrentMultiLayerProfileSelection();
	ResetMultiLayerProfileSelection();
	CHECK_FALSE(GetConventionalRoadTerminalTileArtGeometry(tile));
	REQUIRE(RestoreMultiLayerProfileSelection(policy, LandscapeType::Temperate, rig.ActivatedContent()) == MultiLayerProfileError::None);
	REQUIRE(GetConventionalRoadTerminalTileArtGeometry(tile));
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(_random.state[0] == seeds.state[0]);
	CHECK(_random.state[1] == seeds.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]);
	CHECK(_interactive_random.state[1] == interactive.state[1]);
	const auto bus = TileXYL(40, 20, MapLayer::Underground);
	rig.Open(std::array{bus, TileAddWrap(bus, 0, 1)});
	REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, bus, 1, 1, RoadStopType::Bus, false, DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	CHECK_FALSE(GetConventionalRoadTerminalTileArtGeometry(bus));
	const auto surface = TileXYL(20, 20, MapLayer::Surface);
	REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, surface, 1, 1, RoadStopType::Truck, false, DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	CHECK_FALSE(GetConventionalRoadTerminalTileArtGeometry(surface));
}
