/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_command_boundary.cpp Malformed command, station plane isolation and actual cargo-filter parity. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 10 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_fleet_fixture.h"
#include "../vehicle_gui.h"
#include "../widgets/build_vehicle_widget.h"
#include "../safeguards.h"

/** Invalid native command arguments refuse before any facility or specialist part can be allocated. */
TEST_CASE("Malformed underground command arguments preserve actual world allocation and cash", "[underground-command-boundary]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFleetRig rig;
	const auto depot = rig.BuildDepot(MapLayer::Underground, true);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(_current_company)->money;
	const auto seeds = _random;
	// Both command modes reject empty/plane-edge platform geometry and invalid purchase/refit/clone identities without a half-built object.
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(CmdBuildRailStation(flags, TileXYL(32, 63, MapLayer::Underground), RAILTYPE_RAIL, Axis::Y, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Failed());
		CHECK(CmdBuildRailStation(flags, rig.RailSite(), RAILTYPE_RAIL, Axis::X, 0, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Failed());
		CHECK(std::get<0>(CmdBuildVehicle(flags, INVALID_TILE, GetUndergroundEngine(7), false, INVALID_CARGO, ClientID::Invalid)).Failed());
		CHECK(std::get<0>(CmdBuildVehicle(flags, depot, EngineID::Invalid(), false, INVALID_CARGO, ClientID::Invalid)).Failed());
		CHECK(std::get<0>(CmdRefitVehicle(flags, VehicleID::Invalid(), INVALID_CARGO, 0, false, true, 1)).Failed());
		CHECK(std::get<0>(CmdCloneVehicle(flags, depot, VehicleID::Invalid(), false)).Failed());
		CHECK(Station::GetNumItems() == 0);
		CHECK(Vehicle::GetNumItems() == 0);
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(_random.state[0] == seeds.state[0]);
	CHECK(_random.state[1] == seeds.state[1]);
}

/** Storage bands can be close in raw Y but must never authorize one station/cargo identity across physical planes. */
TEST_CASE("Station joining rejects different planes even beside a storage-band boundary", "[underground-command-boundary]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFleetRig rig;
	AutoRestoreBackup distant_join(_settings_game.station.distant_join_stations, true);
	const auto surface = TileXYL(40, 61, MapLayer::Surface);
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, surface, RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	Station *station = Station::GetByTile(surface);
	const auto terminal = TileXYL(44, 1, MapLayer::Underground);
	rig.Open(std::array{terminal, TileAddByDiagDir(terminal, DiagDirection::SE)});
	CHECK(CmdBuildRoadStop(DoCommandFlag::Execute, terminal, 1, 1, RoadStopType::Truck, false, DiagDirection::SE,
		ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, station->index, false).Failed());
	CHECK(GetTileType(terminal) == TileType::Clear);
	CHECK(RoadStop::GetNumItems() == 0);
	CHECK_FALSE(station->facilities.Test(StationFacility::TruckStop));
}

/** Same-XY ordinary stations retain separate identities and catchment on all three ground planes. */
TEST_CASE("Station coordinates cannot transfer catchment between Surface and either rock layer", "[underground-command-boundary]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFleetRig rig;
	std::array<Station *, 3> stations;
	const MapLayer layers[] = {MapLayer::Surface, MapLayer::Underground, MapLayer::DeepUnderground};
	// Build identical numeric locations through actual commands, excavating only each rock plane's full platform and approaches.
	for (size_t index = 0; index < std::size(layers); ++index) {
		const auto layer = layers[index];
		const auto tile = TileXYL(40, 20, layer);
		if (layer != MapLayer::Surface) rig.Open(std::array{tile, TileAddWrap(tile, 1, 0), TileAddWrap(tile, -1, 0), TileAddWrap(tile, 2, 0)});
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		stations[index] = Station::GetByTile(tile);
		REQUIRE(stations[index] != nullptr);
	}
	// Identical X/Y and nearby raw storage rows never share normal StationIDs or reach another layer's cargo owner.
	for (size_t first = 0; first < stations.size(); ++first) {
		for (size_t second = 0; second < stations.size(); ++second) {
			CHECK(stations[first]->TileIsInCatchment(stations[second]->xy) == (first == second));
			CHECK((stations[first]->index == stations[second]->index) == (first == second));
		}
	}
}

/** An unsupported native freight cargo has no usable purchase row, while a supported cargo keeps an actual buy option. */
TEST_CASE("Native cargo filter and real purchase agree when no conventional design carries Water Ice", "[underground-command-boundary]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFleetRig rig;
	const auto depot = rig.BuildDepot(MapLayer::Underground, true);
	AutoRestoreBackup standard_cargo(_standard_cargo_mask, _cargo_mask); // Every registered fixture label is an ordinary cargo; retain the caller's derived UI mask afterward.
	LayerWindowTestUI ui(WindowClass::BuildVehicle, _current_company);
	ShowBuildVehicleWindow(depot, VehicleType::Road);
	Window *window = FindWindowById(WindowClass::BuildVehicle, depot.base());
	REQUIRE(window != nullptr);
	const auto coal = GetCargoTypeByLabel(CargoLabel{"COAL"});
	window->OnDropdownSelect(WID_BV_CARGO_FILTER_DROPDOWN, coal, 0);
	window->OnPaint();
	CHECK_FALSE(window->IsWidgetDisabled(WID_BV_BUILD));
	const auto water_ice = GetCargoTypeByLabel(CargoLabel{"WICE"});
	REQUIRE(IsValidCargoType(water_ice));
	window->OnDropdownSelect(WID_BV_CARGO_FILTER_DROPDOWN, water_ice, 0);
	window->OnPaint();
	CHECK(window->IsWidgetDisabled(WID_BV_BUILD));
	// All actual carrying native designs refuse this label; the zero-capacity locomotive is not a cargo transport option.
	for (uint16_t model = 2; model <= 8; ++model) {
		const auto id = GetUndergroundEngine(model);
		if (!Engine::Get(id)->CanCarryCargo()) continue;
		CHECK(CheckUndergroundVehicleCapability(id, MapLayer::Underground, water_ice).Failed());
	}
	CHECK(std::get<0>(CmdBuildVehicle(DoCommandFlag::Execute, depot, GetUndergroundEngine(7), false, water_ice, ClientID::Invalid)).Failed());
	CHECK(Vehicle::GetNumItems() == 0);
}
