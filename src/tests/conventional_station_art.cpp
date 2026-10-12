/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file conventional_station_art.cpp Native modular station geometry and complete floor/approach admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group24C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "excavation_fixture.h"
#include "../conventional_station_art.h"
#include "../station_cmd.h"
#include "../station_func.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../newgrf_station.h"
#include "../rail.h"
#include "../town.h"
#include "../table/strings.h"
#include "../transport_tube.h"
#include "../core/backup_type.hpp"
#include "../safeguards.h"

/** Actual profile and command-ready station definitions; opened geology is explicit fixture setup, never free gameplay excavation. */
class ConventionalStationArtRig : public ExcavationFixture {
	AutoRestoreBackup<CompanyID> local{_local_company, _current_company}; ///< Restore caller's local presentation company.
public:
	/** Initialize the ordinary naming town and default rail station definitions, without installing another station owner. */
	ConventionalStationArtRig()
	{
		ResetRailTypes();
		StationClass::Reset();
		REQUIRE(Station::GetNumItems() == 0);
		REQUIRE(Town::GetNumItems() == 0);
		REQUIRE(Town::CanAllocateItem());
		Town *town = Town::Create(TileXYL(10, 10, MapLayer::Surface));
		town->townnametype = SPECSTR_TOWNNAME_START;
		RebuildTownKdtree();
		RebuildStationKdtree();
	}

	/** Destroy actual station spatial/catchment owners before their naming town and inherited company/map setup. */
	~ConventionalStationArtRig()
	{
		// Native destruction retires catchment and signs; a pool wipe alone would leave derived station indexes behind.
		for (Station *station : Station::Iterate()) delete station;
		_town_pool.CleanPool();
		RebuildTownKdtree();
	}
};

/**
 * Emit one compiled native geometry row for a publisher that requires a successful complete test run.
 * @param geometry Read-only projection of the actual default drawing sequence.
 */
static void PrintStationArtGeometry(const ConventionalRailStationArtGeometry &geometry)
{
	fmt::print("STATION_ART_GEOMETRY {{\"graphics\":{},\"axis\":\"{}\",\"role\":{},\"roof_children\":{},\"parts\":[",
		geometry.graphics, geometry.axis == Axis::X ? "X" : "Y", static_cast<uint8_t>(geometry.role), geometry.roof_children);
	// Every sorting parent is exported independently; a missing roof stays null rather than becoming fabricated opaque art.
	for (size_t index = 0; index < geometry.parts.size(); ++index) {
		if (index != 0) fmt::print(",");
		if (!geometry.parts[index]) {
			fmt::print("null");
			continue;
		}
		const auto &part = *geometry.parts[index];
		fmt::print("{{\"origin\":[{},{},{}],\"extent\":[{},{},{}],\"offset\":[{},{},{}]}}",
			part.origin.x, part.origin.y, part.origin.z, part.extent.x, part.extent.y, part.extent.z,
			part.offset.x, part.offset.y, part.offset.z);
	}
	fmt::print("],\"roof_glass\":");
	if (geometry.roof_glass) {
		const auto &glass = *geometry.roof_glass;
		fmt::print("{{\"screen_offset\":[{},{}],\"transparent\":{},\"palette\":\"{}\"}}",
			glass.screen_offset.x, glass.screen_offset.y, glass.transparent,
			glass.palette == PALETTE_TO_TRANSPARENT ? "PALETTE_TO_TRANSPARENT" : "UNSUPPORTED_NATIVE_PALETTE");
	} else {
		fmt::print("null");
	}
	fmt::print("}}\n");
}

/** All eight supported layouts retain exact native parent bounds and elevated roof/glass ownership. */
TEST_CASE("Conventional station source geometry projects all actual default sorting parents", "[conventional-station-art]")
{
	// Default station drawing has four roles on two axes; private replacement tables and the native fallback cannot supply extra views.
	for (uint8_t graphics = 0; graphics < 8; ++graphics) {
		CAPTURE(graphics);
		const auto geometry = GetConventionalRailStationArtGeometry(graphics);
		REQUIRE(geometry.has_value());
		CHECK(geometry->graphics == graphics);
		CHECK(geometry->axis == ((graphics & 1) != 0 ? Axis::Y : Axis::X));
		CHECK(static_cast<uint8_t>(geometry->role) == graphics / 2);
		const auto sequence = GetStationTileLayout(StationType::Rail, graphics)->GetSequence();
		size_t parent = 0;
		// Compare against the renderer's live sequence, including offsets; the source export must not normalize away unusual native bounds.
		for (const auto &piece : sequence) {
			if (!piece.IsParentSprite()) {
				REQUIRE(geometry->roof_glass.has_value());
				CHECK(geometry->roof_glass->screen_offset == Point{piece.origin.x, piece.origin.y});
				CHECK(geometry->roof_glass->transparent == HasBit(piece.image.sprite, PALETTE_MODIFIER_TRANSPARENT));
				CHECK(geometry->roof_glass->palette == piece.image.pal);
				CHECK(geometry->roof_glass->screen_offset == Point{0, 0});
				CHECK(geometry->roof_glass->transparent);
				CHECK(geometry->roof_glass->palette == PALETTE_TO_TRANSPARENT);
				continue;
			}
			REQUIRE(parent < geometry->parts.size());
			REQUIRE(geometry->parts[parent].has_value());
			const auto &bounds = *geometry->parts[parent++];
			CHECK(bounds.origin == piece.origin);
			CHECK(bounds.extent == piece.extent);
			CHECK(bounds.offset == piece.offset);
		}
		CHECK(parent == (graphics >= 4 ? 3 : 2));
		CHECK(geometry->roof_children == (graphics >= 4 ? 1 : 0));
		CHECK(geometry->parts[2].has_value() == (graphics >= 4));
		CHECK(geometry->roof_glass.has_value() == (graphics >= 4));
		if (geometry->parts[2]) {
			CHECK(geometry->parts[2]->origin.z == 16);
			CHECK(geometry->parts[2]->extent == Coord3D<uint8_t>{16, 16, 10});
		}
		PrintStationArtGeometry(*geometry);
	}
	CHECK(GetStationTileLayout(StationType::Rail, 8) == GetStationTileLayout(StationType::Rail, 0));
	for (uint8_t graphics : {8, 9, 255}) CHECK_FALSE(GetConventionalRailStationArtGeometry(graphics));
}

/** Native variable platform dimensions select all four roles on either axis without opening their own floor or end approaches. */
TEST_CASE("Conventional station art follows actual variable platform construction and both approaches", "[conventional-station-art]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const auto layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground);
	const auto axis = GENERATE(Axis::X, Axis::Y);
	const bool missing_near_end = GENERATE(false, true);
	ConventionalStationArtRig rig;
	uint8_t seen = 0;
	const std::array<std::pair<uint8_t, uint8_t>, 6> dimensions{{{1, 1}, {1, 2}, {2, 3}, {3, 3}, {2, 5}, {3, 5}}};
	// Legal short/long and odd/even platform configurations exercise native bare, small-building and both canopy roles; no fixed3x2 building is assumed.
	for (uint index = 0; index < dimensions.size(); ++index) {
		const auto [tracks, length] = dimensions[index];
		CAPTURE(layer, axis, tracks, length, missing_near_end);
		const auto tile = TileXYL(8 + (index % 3) * 16, 16 + (index / 3) * 16, layer);
		const uint width = axis == Axis::X ? length : tracks;
		const uint height = axis == Axis::X ? tracks : length;
		std::vector<TileIndex> floor;
		std::vector<TileIndex> approaches;
		// The real footprint and both end approaches follow the selected track axis, independent of its travel direction.
		for (uint y = 0; y < height; ++y) {
			for (uint x = 0; x < width; ++x) floor.push_back(TileAddWrap(tile, x, y));
		}
		for (uint track = 0; track < tracks; ++track) {
			approaches.push_back(axis == Axis::X ? TileAddWrap(tile, -1, track) : TileAddWrap(tile, track, -1));
			approaches.push_back(axis == Axis::X ? TileAddWrap(tile, length, track) : TileAddWrap(tile, track, length));
		}
		const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
		const size_t stations = Station::GetNumItems();
		CHECK(CmdBuildRailStation({}, tile, RAILTYPE_RAIL, axis, tracks, length, STAT_CLASS_DFLT, 0, NEW_STATION, false).Failed());
		CHECK(CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, axis, tracks, length, STAT_CLASS_DFLT, 0, NEW_STATION, false).Failed());
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
		rig.Open(floor);
		const auto floor_only = ComputeGeologyStateDigest(GetGeologyWorldState());
		CHECK(CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, axis, tracks, length, STAT_CLASS_DFLT, 0, NEW_STATION, false).Failed());
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == floor_only);
		CHECK(Station::GetNumItems() == stations);
		CHECK_FALSE(GetConventionalRailStationTileArtGeometry(tile));
		// Isolate either end in separate native runs. A through station must
		// refuse even when the opposite end and every other platform are open.
		if (missing_near_end) std::ranges::reverse(approaches);
		const auto last_approach = approaches.back();
		approaches.pop_back();
		rig.Open(approaches);
		CHECK(CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, axis, tracks, length, STAT_CLASS_DFLT, 0, NEW_STATION, false).Failed());
		CHECK(Station::GetNumItems() == stations);
		rig.Open(std::array{last_approach});
		REQUIRE(CmdBuildRailStation({}, tile, RAILTYPE_RAIL, axis, tracks, length, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, axis, tracks, length, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		CHECK(Station::GetNumItems() == stations + 1);
		// Every installed platform cell must resolve through its genuine native graphics/axis, including all four alternative cell roles.
		for (const auto cell : floor) {
			REQUIRE(IsRailStationTile(cell));
			const auto geometry = GetConventionalRailStationTileArtGeometry(cell);
			REQUIRE(geometry.has_value());
			CHECK(geometry->axis == GetRailStationAxis(cell));
			CHECK(geometry->graphics == GetStationGfx(cell));
			seen |= 1U << static_cast<uint8_t>(geometry->role);
		}
	}
	CHECK(seen == 0x0F);
}

/** Rendering eligibility is read-only and rejects other domains even when their numeric station graphics appear compatible. */
TEST_CASE("Conventional station art refuses custom tube Surface Classic and invalid queries without mutations", "[conventional-station-art]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	ConventionalStationArtRig rig;
	const auto tile = TileXYL(20, 20, MapLayer::Underground);
	CHECK_FALSE(GetConventionalRailStationTileArtGeometry(INVALID_TILE));
	CHECK_FALSE(GetConventionalRailStationTileArtGeometry(tile));
	rig.Open(std::array{tile, TileAddWrap(tile, -1, 0), TileAddWrap(tile, 1, 0)});
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money;
	const auto seeds = _random;
	const auto interactive_seeds = _interactive_random;
	const auto stations = Station::GetNumItems();
	REQUIRE(GetConventionalRailStationTileArtGeometry(tile));
	// These temporary native fields are explicit malformed/foreign guard inputs; they do not claim successful custom or tube construction.
	SetCustomStationSpecIndex(tile, 1);
	CHECK_FALSE(GetConventionalRailStationTileArtGeometry(tile));
	SetCustomStationSpecIndex(tile, 0);
	const RailType tube = InitialiseTubeRailType(rig.ActivatedContent());
	REQUIRE(tube != INVALID_RAILTYPE);
	SetRailType(tile, tube);
	CHECK_FALSE(GetConventionalRailStationTileArtGeometry(tile));
	SetRailType(tile, RAILTYPE_RAIL);
	const auto graphics = GetStationGfx(tile);
	SetStationGfx(tile, 8);
	CHECK_FALSE(GetConventionalRailStationTileArtGeometry(tile));
	SetStationGfx(tile, graphics);
	const auto policy = GetCurrentMultiLayerProfileSelection();
	ResetMultiLayerProfileSelection();
	CHECK_FALSE(GetConventionalRailStationTileArtGeometry(tile));
	REQUIRE(RestoreMultiLayerProfileSelection(policy, LandscapeType::Temperate, rig.ActivatedContent()) == MultiLayerProfileError::None);
	REQUIRE(GetConventionalRailStationTileArtGeometry(tile));
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(Station::GetNumItems() == stations);
	CHECK(_random.state[0] == seeds.state[0]);
	CHECK(_random.state[1] == seeds.state[1]);
	CHECK(_interactive_random.state[0] == interactive_seeds.state[0]);
	CHECK(_interactive_random.state[1] == interactive_seeds.state[1]);
	const auto surface = TileXYL(20, 20, MapLayer::Surface);
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, surface, RAILTYPE_RAIL, Axis::X, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	CHECK_FALSE(GetConventionalRailStationTileArtGeometry(surface));
}
