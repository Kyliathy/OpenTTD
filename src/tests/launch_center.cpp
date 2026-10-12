/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_center.cpp Native launch facility placement, ownership, water and save lifecycle. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../launch_center.h"
#include "../asteroid.h"
#include "../launch_center_cmd.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../station_cmd.h"
#include "../station_gui.h"
#include "../station_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../landscape_cmd.h"
#include "../water_cmd.h"
#include "../waypoint_cmd.h"
#include "../terraform_cmd.h"
#include "../clear_map.h"
#include "../water_map.h"
#include "../map_layer.h"
#include "../map_layer_gui.h"
#include "../fios.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../settings_type.h"
#include "../timer/timer_game_calendar.h"
#include "../saveload/saveload_func.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include "../gui.h"
#include "../window_gui.h"
#include "../viewport_func.h"
#include "../tilehighlight_type.h"
#include "../tilehighlight_func.h"
#include "../widgets/airport_widget.h"
#include "../widgets/station_widget.h"
#include "../table/strings.h"
#include "layer_save_fixture.h"
#include "excavation_fixture.h"
#include "mock_environment.h"
#include "native_art_render_fixture.h"
#include <cstdlib>
#include "../safeguards.h"

TEST_CASE("Launch freight links reject duplicate orphanable and unbounded transport", "[launch-hub-records]")
{
	const LaunchHubRecord valid{{StationID{1}, 8}, {{StationID{2}, 1}, {StationID{3}, 2}}};
	CHECK(ValidateLaunchHubRecords({}, 1));
	CHECK_FALSE(ValidateLaunchHubRecords({}, 0));
	CHECK(ValidateLaunchHubRecords(std::array{valid}, 3));
	CHECK_FALSE(ValidateLaunchHubRecords(std::array{valid}, 2));
	CHECK_FALSE(ValidateLaunchHubRecords(std::array{valid, valid}, 3));
	for (auto invalid : std::array{
		LaunchHubRecord{{StationID::Invalid(), 8}, valid.members},
		LaunchHubRecord{{StationID{1}, 0}, valid.members},
		LaunchHubRecord{valid.center, {}},
		LaunchHubRecord{valid.center, {{StationID{1}, 1}}},
		LaunchHubRecord{valid.center, {{StationID::Invalid(), 1}}},
		LaunchHubRecord{valid.center, {{StationID{2}, 0}}},
		LaunchHubRecord{valid.center, {{StationID{2}, 1}, {StationID{2}, 2}}},
		LaunchHubRecord{valid.center, {{StationID{2}, 1}, {StationID{3}, 1}}},
		LaunchHubRecord{valid.center, {{StationID{3}, 2}, {StationID{2}, 1}}}})
		CHECK_FALSE(ValidateLaunchHubRecords(std::array{invalid}, 3));
	CHECK_FALSE(ValidateLaunchHubRecords(std::array{valid, LaunchHubRecord{{StationID{4}, 9}, {{StationID{3}, 3}}}}, 4));
	CHECK_FALSE(ValidateLaunchHubRecords(std::array{valid, LaunchHubRecord{{StationID{2}, 9}, {{StationID{4}, 3}}}}, 4));
	auto maximum = valid; maximum.members.clear();
	for (uint16_t i = 0; i < 32; ++i) maximum.members.push_back({StationID{static_cast<uint16_t>(i + 2)}, uint64_t{i} + 1});
	CHECK(ValidateLaunchHubRecords(std::array{maximum}, 34));
	maximum.members.push_back({StationID{34}, 33});
	CHECK_FALSE(ValidateLaunchHubRecords(std::array{maximum}, 34));
	LoadCheckData preview;
	preview.launch_hubs_present = preview.launch_hubs_active = true;
	preview.Clear(); CHECK_FALSE(preview.launch_hubs_present); CHECK_FALSE(preview.launch_hubs_active);
}

/** Actual native pool/tile owners in an isolated focused world; command/graphical acceptance follows below. */
struct LaunchHubRig {
	ExcavationFixture world;
	std::vector<std::unique_ptr<Station>> stations;
	Station *first = nullptr, *second = nullptr, *member = nullptr;
	Station *Freight(TileIndex tile, bool lorry = false)
	{
		REQUIRE(Station::CanAllocateItem());
		stations.emplace_back(Station::Create(tile));
		auto *station = stations.back().get(); station->owner = _current_company;
		station->facilities.Set(lorry ? StationFacility::TruckStop : StationFacility::Train);
		station->spread.Add(tile);
		if (lorry) MakeRoadStop(tile, station->owner, station->index, RoadStopType::Truck, ROADTYPE_ROAD, INVALID_ROADTYPE, DiagDirection::SE);
		else { MakeRailStation(tile, station->owner, station->index, Axis::X, 0, RAILTYPE_RAIL); station->train_station.Add(tile); }
		return station;
	}
	Station *Center(uint x)
	{
		const auto tile = TileXYL(x, 20, MapLayer::Surface);
		REQUIRE(Station::CanAllocateItem());
		stations.emplace_back(Station::Create(tile)); auto *station = stations.back().get();
		station->owner = _current_company; station->facilities.Set(StationFacility::LaunchCenter);
		for (const auto cell : *GetLaunchCenterFootprint(tile, 0)) { MakeStation(cell, station->owner, station->index, StationType::LaunchCenter, 0); station->spread.Add(cell); }
		MakeRiver(TileAddXY(tile, 0, -1), 0);
		InstallNewLaunchCenter(*station, tile, 0);
		return station;
	}
	LaunchHubRig()
	{
		ResetLaunchCenters();
		first = Center(20); second = Center(27); member = Freight(TileXYL(24, 22, MapLayer::Surface));
	}
	~LaunchHubRig() { stations.clear(); ResetLaunchCenters(); }
};

TEST_CASE("Surface launch membership is explicit exclusive and generation safe", "[launch-hub-native]")
{
	MockEnvironment::Instance(); // Native Station sign teardown requires the standard focused-test font environment.
	LaunchHubRig rig;
	const auto first = GetLaunchCenterReference(*rig.first), second = GetLaunchCenterReference(*rig.second);
	CHECK(rig.first->IsInUse()); // An empty launch facility must not enter the native deleted-station timer.
	REQUIRE(IsLaunchHubEligible(*rig.first, *rig.member));
	REQUIRE(GetEligibleLaunchHubs(*rig.member).size() == 2);
	CHECK_FALSE(GetLaunchHubMembership(rig.member->index)); // Overlap/catchment alone never enrolls a station.
	const auto digest = ComputeLaunchHubDigest(), next = GetNextLaunchHubMemberGeneration();
	const auto money = Company::Get(_current_company)->money;
	const auto seeds = _random;
	REQUIRE(CmdSetLaunchHub({}, rig.member->index, first.station, first.generation).Succeeded());
	CHECK(ComputeLaunchHubDigest() == digest); CHECK(GetNextLaunchHubMemberGeneration() == next);
	CHECK(_random.state[0] == seeds.state[0]); CHECK(_random.state[1] == seeds.state[1]);
	REQUIRE(CmdSetLaunchHub(DoCommandFlag::Execute, rig.member->index, first.station, first.generation).Succeeded());
	const auto link = *GetLaunchHubMembership(rig.member->index);
	CHECK_FALSE(link.joined); CHECK(link.center == first);
	CHECK(ResolveLaunchHubMember(first, link.member) == rig.member);
	CHECK(ResolveLaunchHubMember(second, link.member) == nullptr);
	CHECK(Company::Get(_current_company)->money == money);
	REQUIRE(ValidateLaunchHubs());
	SECTION("reselection and unlink retire the old relationship without moving packets") {
		const auto cargo = GetCargoTypeByLabel(CargoLabel{"NORE"});
		auto &storage = rig.first->goods[cargo].GetOrCreateData().cargo;
		REQUIRE(CargoPacket::CanAllocateItem());
		storage.Append(CargoPacket::Create(rig.first->index, uint16_t{7}, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
		const auto packets = CargoPacket::GetNumItems();
		CHECK(GetLaunchHubStorage(first) == rig.first);
		REQUIRE(CmdSetLaunchHub(DoCommandFlag::Execute, rig.member->index, second.station, second.generation).Succeeded());
		CHECK(rig.first->launch_hub->members.empty()); CHECK(rig.second->launch_hub->members.size() == 1);
		CHECK(ResolveLaunchHubMember(first, link.member) == nullptr);
		CHECK(storage.TotalCount() == 7); CHECK(CargoPacket::GetNumItems() == packets);
		CHECK(rig.member->goods[cargo].TotalCount() == 0); CHECK(rig.second->goods[cargo].TotalCount() == 0);
		REQUIRE(CmdSetLaunchHub(DoCommandFlag::Execute, rig.member->index, StationID::Invalid(), 0).Succeeded());
		CHECK_FALSE(GetLaunchHubMembership(rig.member->index));
		CHECK(storage.TotalCount() == 7); CHECK(CargoPacket::GetNumItems() == packets);
	}
	SECTION("full footprint, other plane, foreign company and absent freight refuse") {
		const auto remote = TileXYL(41, 20, MapLayer::Surface);
		MakeRailStation(remote, rig.member->owner, rig.member->index, Axis::X, 0, RAILTYPE_RAIL); rig.member->spread.Add(remote);
		CHECK_FALSE(IsLaunchHubEligible(*rig.first, *rig.member));
		CHECK(CmdSetLaunchHub({}, rig.member->index, first.station, first.generation).Failed());
		RevalidateLaunchHubMemberships(); CHECK_FALSE(GetLaunchHubMembership(rig.member->index));
		CHECK(ResolveLaunchHubMember(first, link.member) == nullptr);
		rig.member->spread = TileArea(rig.member->xy, 1, 1); MakeClear(remote, ClearGround::Grass, 0);
		REQUIRE(Station::CanAllocateItem());
		std::unique_ptr<Station> below{Station::Create(TileXYL(24, 22, MapLayer::Underground))};
		below->owner = rig.member->owner; below->facilities.Set(StationFacility::Train); below->spread = TileArea(below->xy, 1, 1);
		MakeRailStation(below->xy, below->owner, below->index, Axis::X, 0, RAILTYPE_RAIL);
		CHECK_FALSE(IsLaunchHubEligible(*rig.first, *below)); CHECK(CmdSetLaunchHub({}, below->index, first.station, first.generation).Failed());
		REQUIRE(Company::CanAllocateItem());
		std::unique_ptr<Company> foreign{Company::Create()};
		{ AutoRestoreBackup actor(_current_company, foreign->index); CHECK(CmdSetLaunchHub({}, rig.member->index, first.station, first.generation).Failed()); }
		rig.member->facilities.Reset(StationFacility::Train); CHECK_FALSE(IsLaunchHubEligible(*rig.first, *rig.member));
	}
	SECTION("joined freight shares native storage and cannot link to another center") {
		const auto rail = TileXYL(24, 20, MapLayer::Surface);
		MakeRailStation(rail, rig.first->owner, rig.first->index, Axis::X, 0, RAILTYPE_RAIL);
		rig.first->facilities.Set(StationFacility::Train); rig.first->spread.Add(rail); rig.first->train_station.Add(rail);
		const auto joined = *GetLaunchHubMembership(first.station);
		CHECK(joined.joined); CHECK(joined.center == first);
		CHECK(GetLaunchHubStorage(joined.center) == rig.first);
		CHECK(rig.first->launch_hub->members.size() == 1); // Only the separate station occupies a slot.
		CHECK(CmdSetLaunchHub({}, first.station, second.station, second.generation).Failed());
		CHECK(CmdSetLaunchHub({}, first.station, StationID::Invalid(), 0).Failed());
	}
	SECTION("native station deletion and pool reuse cannot inherit the previous link") {
		const auto id = rig.member->index;
		const auto tile = rig.member->xy;
		MakeClear(tile, ClearGround::Grass, 0); rig.stations.pop_back();
		CHECK(ResolveLaunchHubMember(first, link.member) == nullptr);
		auto *replacement = rig.Freight(tile); REQUIRE(replacement->index == id);
		CHECK_FALSE(GetLaunchHubMembership(id));
		REQUIRE(CmdSetLaunchHub(DoCommandFlag::Execute, id, first.station, first.generation).Succeeded());
		CHECK(GetLaunchHubMembership(id)->member.generation > link.member.generation);
		CHECK(ResolveLaunchHubMember(first, link.member) == nullptr);
	}
	SECTION("full hub refuses before retiring an existing different-center link") {
		REQUIRE(CmdSetLaunchHub(DoCommandFlag::Execute, rig.member->index, second.station, second.generation).Succeeded());
		for (uint y = 10; rig.first->launch_hub->members.size() < MAX_LAUNCH_HUB_MEMBERS; ++y) for (uint x = 10; x < 18 && rig.first->launch_hub->members.size() < MAX_LAUNCH_HUB_MEMBERS; ++x) {
			auto *member = rig.Freight(TileXYL(x, y, MapLayer::Surface));
			REQUIRE(CmdSetLaunchHub(DoCommandFlag::Execute, member->index, first.station, first.generation).Succeeded());
		}
		const auto before = ComputeLaunchHubDigest();
		CHECK(CmdSetLaunchHub(DoCommandFlag::Execute, rig.member->index, first.station, first.generation).GetErrorMessage() == STR_ERROR_LAUNCH_HUB_LIMIT);
		CHECK(ComputeLaunchHubDigest() == before); CHECK(GetLaunchHubMembership(rig.member->index)->center == second);
	}
	CHECK(ValidateLaunchHubs());
}

TEST_CASE("Launch centers preserve native bytes and validate bounded identity transport", "[launch-center-records]")
{
	STATIC_REQUIRE(to_underlying(StationType::RoadWaypoint) == 8);
	STATIC_REQUIRE(to_underlying(StationType::LaunchCenter) == 9);
	STATIC_REQUIRE(to_underlying(StationFacility::LaunchCenter) == 5);
	STATIC_REQUIRE(to_underlying(STATION_FACILITY_GHOST) == 6);
	STATIC_REQUIRE(to_underlying(StationFacility::Waypoint) == 7);
	const LaunchCenterRecord valid{StationID{0}, {TileIndex{10 + 10 * 64}, 1, 0}};
	CHECK(ValidateLaunchCenterRecords({}, 1, 64, 64));
	CHECK(ValidateLaunchCenterRecords(std::array{valid}, 2, 64, 64));
	CHECK_FALSE(ValidateLaunchCenterRecords(std::array{valid}, 1, 64, 64));
	CHECK_FALSE(ValidateLaunchCenterRecords({}, 0, 64, 64));
	CHECK_FALSE(ValidateLaunchCenterRecords(std::array{valid, valid}, 2, 64, 64));
	CHECK_FALSE(ValidateLaunchCenterRecords(std::array{valid, LaunchCenterRecord{StationID{1}, {valid.anchor.tile, 2, 0}}}, 3, 64, 64));
	for (const auto bad : std::array{
		LaunchCenterRecord{StationID::Invalid(), valid.anchor},
		LaunchCenterRecord{StationID{0}, {valid.anchor.tile, 0, 0}},
		LaunchCenterRecord{StationID{0}, {valid.anchor.tile, 1, 4}},
		LaunchCenterRecord{StationID{0}, {TileIndex{4096}, 1, 0}},
		LaunchCenterRecord{StationID{0}, {TileIndex{63 + 63 * 64}, 1, 0}}})
		CHECK_FALSE(ValidateLaunchCenterRecords(std::array{bad}, 2, 64, 64));
	std::vector<LaunchCenterRecord> many;
	for (uint16_t i = 0; i < 65; ++i) many.push_back({StationID{i}, {TileIndex{(i % 8) * 4U + (i / 8) * 4U * 64}, uint64_t{i} + 1, 0}});
	CHECK(ValidateLaunchCenterRecords(std::span{many}.first(64), 66, 64, 64));
	CHECK_FALSE(ValidateLaunchCenterRecords(many, 66, 64, 64));
	LoadCheckData preview;
	preview.asteroid_present = preview.asteroid_active = true;
	preview.asteroid_seed = preview.orbital_seed = 99;
	preview.launch_centers_present = preview.launch_centers_active = true;
	preview.Clear();
	CHECK_FALSE(preview.asteroid_present);
	CHECK_FALSE(preview.asteroid_active);
	CHECK(preview.asteroid_seed == 0);
	CHECK(preview.orbital_seed == 0);
	CHECK_FALSE(preview.launch_centers_present);
	CHECK_FALSE(preview.launch_centers_active);
}

static std::function<void()> *_launch_observer = nullptr;
static bool ObserveLaunchCenter(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1);
	REQUIRE(_launch_observer != nullptr);
	(*_launch_observer)();
	return true;
}

/** Explicit fixture terrain only; never a gameplay free-pad or original-save rewrite. */
static TileIndex PrepareLaunchSiteTerrain()
{
	for (uint y = 2; y + 9 < Map::SizeY(); ++y) for (uint x = 2; x + 9 < Map::SizeX(); ++x) {
		const TileArea region(TileXYL(x, y, MapLayer::Surface), 8, 8);
		bool clear = true;
		for (const auto cell : region) {
			if (!IsTileType(cell, TileType::Clear) && !IsTileType(cell, TileType::Trees) && !(IsWaterTile(cell) && (IsSea(cell) || IsRiver(cell)))) clear = false;
		}
		if (!clear) continue;
		for (const auto cell : region) { MakeClear(cell, ClearGround::Grass, 0); SetTileHeight(cell, 2); }
		return TileXYL(x + 2, y + 2, MapLayer::Surface);
	}
	FAIL("Native fixture has no unoccupied preparation region");
	return INVALID_TILE;
}

/** Scratch terrain for native freight commands; refuses existing buildings or facilities before changing any tile. */
static bool PrepareNativeFreightPatch(TileIndex tile, uint width = 1)
{
	const int x = TileX(tile), y = TilePlaneY(tile);
	if (x < 2 || y < 2 || x + width + 1 >= Map::SizeX() || y + 2 >= Map::SizeY()) return false;
	const TileArea patch(TileXYL(x - 1, y - 1, MapLayer::Surface), width + 2, 3);
	for (const auto cell : patch)
		if (!IsTileType(cell, TileType::Clear) && !IsTileType(cell, TileType::Trees) && !(IsWaterTile(cell) && (IsSea(cell) || IsRiver(cell) || IsCanal(cell)))) return false;
	for (const auto cell : patch) { MakeClear(cell, ClearGround::Grass, 0); SetTileHeight(cell, 2); }
	return true;
}

static TileIndex PrepareNativeFreightTile(const Station &center)
{
	const auto anchor = center.launch_center->tile;
	for (int dy = 2; dy <= 14; ++dy) for (int dx = 6; dx <= 14; ++dx) {
		const int x = TileX(anchor) + dx, y = TilePlaneY(anchor) + dy;
		if (x >= Map::SizeX() || y >= Map::SizeY()) continue;
		const auto tile = TileXYL(x, y, MapLayer::Surface);
		if (PrepareNativeFreightPatch(tile)) return tile;
	}
	FAIL("Native launch hub fixture has no free Surface freight location");
	return INVALID_TILE;
}

/** Native commands, actual station window and actual station extension; no copied station/cargo owner. */
static void PrepareNativeLaunchHub(Station &center, const TestArtifactPaths &paths)
{
	const auto ref = GetLaunchCenterReference(center);
	const auto joined = GetLaunchHubMembership(center.index);
	REQUIRE(joined); CHECK(joined->joined); CHECK(GetLaunchHubStorage(ref) == &center);
	const auto train_tile = PrepareNativeFreightTile(center);
	REQUIRE(Command<Commands::BuildRailStation>::Post(train_tile, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, true));
	auto *train = Station::GetByTile(train_tile);
	CHECK(train->index != center.index); REQUIRE(IsLaunchHubEligible(center, *train));
	CHECK_FALSE(GetLaunchHubMembership(train->index));
	const auto before = ComputeLaunchHubDigest();
	REQUIRE(CmdSetLaunchHub({}, train->index, ref.station, ref.generation).Succeeded());
	CHECK(ComputeLaunchHubDigest() == before);
	REQUIRE(Command<Commands::SetLaunchHub>::Post(train->index, ref.station, ref.generation));
	CHECK(GetLaunchHubMembership(train->index)->center == ref);
	const auto lorry_tile = PrepareNativeFreightTile(center);
	REQUIRE(Command<Commands::BuildRoadStop>::Post(lorry_tile, uint8_t{1}, uint8_t{1}, RoadStopType::Truck, false, DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, NEW_STATION, true));
	auto *lorry = Station::GetByTile(lorry_tile);
	REQUIRE(IsLaunchHubEligible(center, *lorry)); CHECK_FALSE(GetLaunchHubMembership(lorry->index));
	ShowStationViewWindow(lorry->index);
	auto *window = FindWindowById(WindowClass::StationView, lorry->index); REQUIRE(window != nullptr);
	CaptureNativeArtWindow(*window, paths.images / "launch-hub-selector.ppm");
	CHECK_FALSE(window->IsWidgetDisabled(WID_SV_LAUNCH_HUB));
	window->OnClick({}, WID_SV_LAUNCH_HUB, 1);
	window->OnDropdownSelect(WID_SV_LAUNCH_HUB, 1, 0);
	REQUIRE(GetLaunchHubMembership(lorry->index)); CHECK(GetLaunchHubMembership(lorry->index)->center == ref);
	window->OnClick({}, WID_SV_LAUNCH_COLLECTION, 1);
	CHECK(IsViewportLaunchCollection(ref.station, ref.generation));
	CHECK(GetViewportLaunchCollectionTile(lorry_tile) == LaunchCollectionHighlight::Member);
	CHECK(GetViewportLaunchCollectionTile(TileXYL(TileX(lorry_tile), TilePlaneY(lorry_tile), MapLayer::Underground)) == LaunchCollectionHighlight::None);
	const auto anchor = center.launch_center->tile;
	const int ax = TileX(anchor), ay = TilePlaneY(anchor);
	if (ax + 21 < Map::SizeX()) {
		CHECK(GetViewportLaunchCollectionTile(TileXYL(ax + 20, ay, MapLayer::Surface)) == LaunchCollectionHighlight::Area);
		CHECK(GetViewportLaunchCollectionTile(TileXYL(ax + 21, ay, MapLayer::Surface)) == LaunchCollectionHighlight::None);
	}
	CaptureNativeArtViewport(anchor, ZoomLevel::Normal, paths.images / "launch-hub-collection.ppm");
	CaptureNativeArtWindow(*window, paths.images / "launch-hub-linked.ppm");
	SetViewportLaunchCollection(ref.station, ref.generation, false);
	window->CloseChildWindows(WindowClass::DropdownMenu);
	window->Close();
	// A sign/initial tile inside is insufficient once another native tile extends outside.
	bool expanded = false;
	for (const auto offset : std::array{std::array{19, 1}, std::array{19, -1}}) {
		const int x = ax + offset[0], y = ay + offset[1];
		if (x < 0 || y < 0 || x + 1 >= Map::SizeX() || y >= Map::SizeY()) continue;
		const auto inside = TileXYL(x, y, MapLayer::Surface);
		if (!PrepareNativeFreightPatch(inside, 2)) continue;
		const auto outside = TileAddXY(inside, 1, 0);
		REQUIRE(Command<Commands::BuildRailStation>::Post(inside, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, true));
		auto *boundary = Station::GetByTile(inside);
		REQUIRE(Command<Commands::SetLaunchHub>::Post(boundary->index, ref.station, ref.generation));
		const auto old = *GetLaunchHubMembership(boundary->index);
		REQUIRE(Command<Commands::BuildRailStation>::Post(outside, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, boundary->index, false));
		CHECK(boundary->xy == inside); CHECK_FALSE(IsLaunchHubEligible(center, *boundary));
		CHECK_FALSE(GetLaunchHubMembership(boundary->index)); CHECK(ResolveLaunchHubMember(ref, old.member) == nullptr);
		expanded = true; break;
	}
	REQUIRE(expanded);
	REQUIRE(ValidateLaunchHubs()); CHECK(center.launch_hub->members.size() == 2);
}

TEST_CASE("Native launch facility builds joins protects water and retains saved generations", "[.launch-center]")
{
	const char *input = std::getenv("OTTD_LAUNCH_CENTER_SOURCE");
	const bool prepare = input == nullptr;
	const auto source = prepare ? FindTestRepositoryRoot() / "src/tests/fixtures/demos/space/space-survey-approach-1990-404.sav" : std::filesystem::path{input};
	const auto paths = GetTestCaseArtifactPaths("space", "launch-center");
	const auto config = GetTestRuntimeConfig();
	bool observed = false;
	std::exception_ptr failure;
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			AutoRestoreBackup running(_pause_mode, PauseModes{});
			AutoRestoreBackup distant_join(_settings_game.station.distant_join_stations, true);
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			if (prepare) {
				REQUIRE(CaptureLaunchCenters().empty());
				const auto tile = PrepareLaunchSiteTerrain();
				const auto north = TileAddXY(tile, 0, -1), west = TileAddXY(tile, -1, 0), southwest = TileAddXY(tile, -1, 1);
				const auto before = ComputeLaunchCenterDigest();
				CHECK(CmdBuildLaunchCenter({}, tile, 0, NEW_STATION, true).Failed());
				CHECK(ComputeLaunchCenterDigest() == before);
				MakeRiver(north, 0);
				const auto last_corner = TileAddXY(tile, 3, 2);
				SetTileHeight(last_corner, 3);
				CHECK(CmdBuildLaunchCenter(DoCommandFlag::Execute, tile, 0, NEW_STATION, true).Failed());
				CHECK(CaptureLaunchCenters().empty()); CHECK(GetNextLaunchCenterGeneration() == 1);
				for (const auto cell : *GetLaunchCenterFootprint(tile, 0)) CHECK(IsTileType(cell, TileType::Clear));
				SetTileHeight(last_corner, 2);
				{
					AutoRestoreBackup year(TimerGameCalendar::year, TimerGameCalendar::Year{1989});
					CHECK(CmdBuildLaunchCenter({}, tile, 0, NEW_STATION, true).Failed());
				}
				CHECK(CmdBuildLaunchCenter({}, TileXYL(TileX(tile), TilePlaneY(tile), MapLayer::Space), 0, NEW_STATION, true).Failed());
				CHECK(CmdBuildLaunchCenter({}, tile, 4, NEW_STATION, true).Failed());
				CHECK(CmdBuildLaunchCenter(DoCommandFlag::Execute, tile, 0, StationID{63999}, false).Failed());
				const auto random_before = std::array{_random.state[0], _random.state[1]};
				const auto estimate = CmdBuildLaunchCenter({}, tile, 0, NEW_STATION, true);
				REQUIRE(estimate.Succeeded());
				CHECK(estimate.GetCost() == 500000);
				CHECK(ComputeLaunchCenterDigest() == before);
				CHECK((std::array{_random.state[0], _random.state[1]} == random_before));
				auto *company = Company::Get(_local_company);
				const auto money = company->money;
				const auto infrastructure = company->infrastructure.station, airports = company->infrastructure.airport;
				REQUIRE(Command<Commands::BuildLaunchCenter>::Post(tile, uint8_t{0}, NEW_STATION, true));
				CHECK(company->money == money - estimate.GetCost());
				CHECK(company->infrastructure.station == infrastructure + 12);
				CHECK(company->infrastructure.airport == airports);
				auto *station = Station::GetByTile(tile);
				const auto first = GetLaunchCenterReference(*station);
				REQUIRE(ResolveLaunchCenter(first) == station);
				CHECK(station->airport.IsEmpty());
				CHECK_FALSE(station->facilities.Test(StationFacility::Airport));
				CHECK(station->GetCatchmentRadius() == 0);
				REQUIRE(ValidateLaunchCenters());
				CHECK(CmdBuildLaunchCenter({}, tile, 0, station->index, false).Failed());
				const auto stable = ComputeLaunchCenterDigest();
				for (auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
					CHECK(Command<Commands::LandscapeClear>::Do(flags, north).GetErrorMessage() == STR_ERROR_LAUNCH_CENTER_WATER);
					CHECK(CmdBuildBuoy(flags, north).GetErrorMessage() == STR_ERROR_LAUNCH_CENTER_WATER);
				}
				CHECK(ComputeLaunchCenterDigest() == stable);
				MakeRiver(west, 0);
				CHECK_FALSE(CanRemoveLaunchCenterWater([&](TileIndex water) { return water == north || water == west; }));
				for (auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
					CHECK(std::get<0>(CmdClearArea(flags, north, west, false)).GetErrorMessage() == STR_ERROR_LAUNCH_CENTER_WATER);
					CHECK(IsLaunchCenterNavigableWater(north)); CHECK(IsLaunchCenterNavigableWater(west));
				}
				REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, north).Succeeded());
				CHECK_FALSE(IsLaunchCenterNavigableWater(north));
				REQUIRE(ValidateLaunchCenters());
				MakeRiver(southwest, 0);
				for (auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}})
					CHECK(CmdBuildShipDepot(flags, west, Axis::Y).GetErrorMessage() == STR_ERROR_LAUNCH_CENTER_WATER);
				REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, southwest).Succeeded());
				REQUIRE(CmdBuildCanal(DoCommandFlag::Execute, west, west, WaterClass::Canal, false).Succeeded());
				CHECK(IsCanal(west)); REQUIRE(ValidateLaunchCenters());
				const auto rail = TileAddXY(tile, 4, 1);
				REQUIRE(Command<Commands::BuildRailStation>::Post(rail, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, station->index, false));
				CHECK(GetStationIndex(rail) == station->index);
				CHECK(station->GetCatchmentRadius() > 0);
				CHECK(GetLaunchCenterReference(*station) == first);
				CHECK(station->launch_center->tile == tile);
				{
					AutoRestoreBackup foreign(_current_company, OWNER_NONE);
					CHECK(CmdRemoveLaunchCenter({}, tile).Failed());
				}
				REQUIRE(Command<Commands::RemoveLaunchCenter>::Post(tile));
				CHECK(ResolveLaunchCenter(first) == nullptr);
				CHECK(IsRailStationTile(rail)); CHECK(Station::IsValidID(station->index));
				REQUIRE(Command<Commands::BuildLaunchCenter>::Post(tile, uint8_t{1}, station->index, false));
				CHECK(GetLaunchCenterReference(*station).generation > first.generation);
				CHECK(ResolveLaunchCenter(first) == nullptr);
				CHECK(station->launch_center->rotation == 1);
				REQUIRE(ValidateLaunchCenters());
				PrepareNativeLaunchHub(*station, paths);
			}
			const auto records = CaptureLaunchCenters();
			REQUIRE(records.size() == 1);
			auto *station = ResolveLaunchCenter({records[0].station, records[0].anchor.generation});
			REQUIRE(station != nullptr); REQUIRE(ValidateLaunchCenters());
			REQUIRE(ValidateLaunchHubs());
			const auto hubs = CaptureLaunchHubs(); REQUIRE(hubs.size() == 1); CHECK(hubs[0].members.size() == 2);
			for (const auto member : hubs[0].members) CHECK(ResolveLaunchHubMember(hubs[0].center, member) != nullptr);
			if (const char *corrupt = std::getenv("OTTD_LAUNCH_CENTER_CORRUPT")) {
				const auto live = ComputeLaunchCenterDigest(), field = ComputeAsteroidFieldDigest(GetAsteroidField()), hub_digest = ComputeLaunchHubDigest();
				const auto money = Company::Get(_local_company)->money;
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(_load_check_data.launch_centers_present); CHECK(_load_check_data.launch_centers_active);
				CHECK(_load_check_data.launch_hubs_present); CHECK(_load_check_data.launch_hubs_active);
				CHECK(SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(_load_check_data.launch_centers_present); CHECK(_load_check_data.launch_centers_active);
				CHECK(_load_check_data.launch_hubs_present); CHECK(_load_check_data.launch_hubs_active);
				CHECK(ComputeLaunchCenterDigest() == live); CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field); CHECK(ComputeLaunchHubDigest() == hub_digest);
				CHECK(Company::Get(_local_company)->money == money); CHECK(ResolveLaunchCenter({records[0].station, records[0].anchor.generation}) == station);
				fmt::print("LAUNCH_CENTER preview_reject=true authority_unchanged=true\n");
			}
			SetViewportLayer(GetMainWindow(), MapLayer::Surface);
			ScrollMainWindowToTile(records[0].anchor.tile, true);
			CaptureNativeArtViewport(records[0].anchor.tile, ZoomLevel::Normal, paths.images / "launch-center.ppm");
			auto *toolbar = ShowBuildAirToolbar(); REQUIRE(toolbar != nullptr);
			CaptureNativeArtWindow(*toolbar, paths.images / "launch-toolbar.ppm");
			CHECK_FALSE(toolbar->IsWidgetDisabled(WID_AT_LAUNCH_CENTER));
			toolbar->OnClick({}, WID_AT_LAUNCH_CENTER, 1);
			CHECK(_thd.new_size.x == 4 * TILE_SIZE); CHECK(_thd.new_size.y == 3 * TILE_SIZE);
			toolbar->OnClick({}, WID_AT_LAUNCH_ROTATE, 1);
			CHECK(_thd.new_size.x == 3 * TILE_SIZE); CHECK(_thd.new_size.y == 4 * TILE_SIZE);
			toolbar->OnPlaceObjectAbort();
			{
				AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal});
				AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / "launch-center-408.sav"), false) == SaveLoadResult::Ok);
			}
			fmt::print("LAUNCH_CENTER native=true centers=1 generation={} digest={} reload={}\n", records[0].anchor.generation, ComputeLaunchCenterDigest(), !prepare);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_launch_observer, &observe);
	IConsole::CmdRegister("launch_center_test", ObserveLaunchCenter);
	const std::string source_path = source.string(), config_path = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(args) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
