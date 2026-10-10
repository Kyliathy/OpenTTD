/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_readiness.cpp Actual native facility identity, model/cargo and pure physical inlet views.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-underground-transport-tubes.md Group8
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group48C actual platform geometry
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "mock_fontcache.h"
#include "transport_tube_meter_fixture.h"
#include "../station_cmd.h"
#include "../safeguards.h"

/** Views resolve actual native depot/capsule/cargo owners and reject invalid, foreign, ordinary and wrong-layer references. */
TEST_CASE("Tube public facility view retains native identity and exact capsule cargo admission", "[transport-tube-readiness]")
{
	TubePrototypeRig rig;
	const auto depot = GetTubeTerminal(rig.depot, _current_company); REQUIRE(depot.has_value());
	CHECK(depot->depot == Depot::GetByTile(rig.depot)->index); CHECK(depot->station == StationID::Invalid());
	CHECK(depot->layer == MapLayer::Underground); CHECK(depot->guideway == rig.tube); CHECK(depot->platform_length_tiles == 0);
	const auto readiness = GetTubeNetworkReadiness(rig.depot, _current_company); REQUIRE(readiness.has_value());
	CHECK(readiness->inlets.empty()); CHECK(CanTubeTerminalServeVehicle(rig.depot, _current_company, GetTubeEngine(9)));
	CHECK(CanTubeTerminalServeVehicle(rig.depot, _current_company, GetTubeEngine(9), GetCargoTypeByLabel(CargoLabel{"COAL"})));
	CHECK_FALSE(CanTubeTerminalServeVehicle(rig.depot, _current_company, GetTubeEngine(9), static_cast<CargoType>(NUM_CARGO)));
	CHECK_FALSE(CanTubeTerminalServeVehicle(rig.depot, _current_company, EngineID::Invalid()));
	CHECK_FALSE(GetTubeTerminal(INVALID_TILE, _current_company).has_value());
	CHECK_FALSE(GetTubeTerminal(rig.depot, CompanyID::Invalid()).has_value());
	CHECK_FALSE(GetTubeTerminal(MultiLayerMap::TileInLayer(rig.depot, MapLayer::Surface), _current_company).has_value());
	const auto ordinary = TileXYL(45, 45, MapLayer::Underground); rig.BuildTrack(ordinary, RAILTYPE_RAIL);
	CHECK_FALSE(GetTubeTerminal(ordinary, _current_company).has_value());
	REQUIRE(Company::CanAllocateItem()); Company *foreign = Company::Create();
	CHECK_FALSE(GetTubeTerminal(rig.depot, foreign->index).has_value());
	CHECK_FALSE(GetTubeNetworkReadiness(rig.depot, foreign->index).has_value()); delete foreign;
}

/** A loading-platform view retains its canonical station ID/geometry and never copies or allocates an alternate packet buffer. */
TEST_CASE("Tube public loading terminal uses actual station platform and cargo owner", "[transport-tube-readiness]")
{
	// Station construction measures its real name sign. Isolated geometry
	// checks need their own font cache rather than relying on an earlier UI
	// case to initialize it before Windows paragraph layout is entered.
	MockFontCache::InitializeFontCaches();
	LoadGeneratedWindowTestLanguage(); TubePrototypeRig rig;
	const auto tile = TileXYL(50, 44, MapLayer::Underground);
	rig.Open(std::array{tile, TileAddWrap(tile, 1, 0), TileAddWrap(tile, -1, 0), TileAddWrap(tile, 2, 0)});
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, rig.tube, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	const auto terminal = GetTubeTerminal(tile, _current_company); REQUIRE(terminal.has_value());
	CHECK(terminal->station == GetStationIndex(tile)); CHECK(terminal->depot == DepotID::Invalid()); CHECK(terminal->platform_length_tiles == 2);
	CHECK(GetTubeTerminal(TileAddWrap(tile, 1, 0), _current_company)->station == terminal->station);
	const auto stations = Station::GetNumItems(), packets = CargoPacket::GetNumItems();
	CHECK(CanTubeTerminalServeVehicle(tile, _current_company, GetTubeEngine(9)));
	CHECK(GetTubeNetworkReadiness(tile, _current_company)->inlets.empty());
	CHECK(Station::GetNumItems() == stations); CHECK(CargoPacket::GetNumItems() == packets);
}

/**
 * Freeze all eight actual default native platform graphics before selecting tube loading source cells.
 * Real command-built axes/platform counts/lengths retain canonical station IDs and berth geometry.
 */
TEST_CASE("Tube loading artwork follows actual native platform cell roles", "[transport-tube-platform-geometry]")
{
	MockFontCache::InitializeFontCaches();
	LoadGeneratedWindowTestLanguage();
	TubePrototypeRig rig;
	std::array<bool, 8> seen{};
	uint layout_index = 0;
	// Two real axes, one/two parallel platforms and short/long through berths
	// exercise bare, small-building and both canopy-half roles. Layout cells
	// remain native station tiles, never a fixed decorative two-by-two campus.
	for (Axis axis : {Axis::X, Axis::Y}) {
		for (uint8_t tracks : {1, 2}) {
			for (uint8_t length : {1, 2, 5}) {
				CAPTURE(axis, tracks, length);
				// This fixture owns a64x64 plane. Keep every full five-cell
				// berth and both approaches inside its actual boundary, with
				// enough spacing to prevent automatic neighbouring joins.
				const uint origin_x = 12 + (layout_index % 4) * 12;
				const uint origin_y = 12 + (layout_index / 4) * 12;
				const TileIndex tile = TileXYL(origin_x, origin_y, MapLayer::Underground);
				// Open precisely the native platform cells and their two through
				// approaches as explicit fixture input. A builder cannot excavate
				// or acquire an extra stock/capacity cell through its source art.
				for (uint platform = 0; platform < tracks; ++platform) {
					for (int along = -1; along <= length; ++along) {
						const int dx = axis == Axis::X ? along : static_cast<int>(platform);
						const int dy = axis == Axis::X ? static_cast<int>(platform) : along;
						rig.Open(std::array{TileAddWrap(tile, dx, dy)});
					}
				}
				const auto stations = Station::GetNumItems();
				const auto packets = CargoPacket::GetNumItems();
				const Money money = Company::Get(_current_company)->money;
				REQUIRE(CmdBuildRailStation({}, tile, rig.tube, axis, tracks, length, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
				CHECK(Station::GetNumItems() == stations);
				CHECK(Company::Get(_current_company)->money == money);
				REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, rig.tube, axis, tracks, length, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
				CHECK(Station::GetNumItems() == stations + 1);
				const StationID station = GetStationIndex(tile);
				// Every built cell keeps the same canonical packet owner and its
				// actual track-axis/berth length. Graphics roles come from the
				// installed native map, independently of a planned source caption.
				for (uint platform = 0; platform < tracks; ++platform) {
					for (uint along = 0; along < length; ++along) {
						const auto cell = TileAddWrap(tile, axis == Axis::X ? along : platform, axis == Axis::X ? platform : along);
						const auto view = GetTubeTerminal(cell, _current_company);
						REQUIRE(view.has_value());
						CHECK(view->station == station);
						CHECK(view->depot == DepotID::Invalid());
						CHECK(view->platform_length_tiles == length);
						CHECK(GetRailStationAxis(cell) == axis);
						const StationGfx graphics = GetStationGfx(cell);
						REQUIRE(graphics < seen.size());
						seen[graphics] = true;
					}
				}
				CHECK(CargoPacket::GetNumItems() == packets);
				++layout_index;
			}
		}
	}
	CHECK(layout_index == 12);
	CHECK(std::ranges::count(seen, true) == seen.size());
}

/** Source/financial readiness follows actual owned guideways and cash while remaining pure across repeated UI/camera-independent queries. */
TEST_CASE("Tube public power readiness changes only with actual service finance or layout", "[transport-tube-readiness]")
{
	TubeMeterRig rig; rig.Bind();
	const auto ledger = ComputeUtilityPowerDigest(), physical = ComputeTubePowerDigest(), bindings = ComputeTubePowerVehicleDigest();
	const auto revision = CSegmentCostCacheBase::s_rail_change_counter;
	const auto money = Company::Get(_current_company)->money;
	// Repeated downstream inspections cover a remote real depot on this
	// connected guideway without registering another source/consumer or grant.
	for (uint view = 0; view < 20; ++view) {
		const auto readiness = GetTubeNetworkReadiness(rig.destination, _current_company); REQUIRE(readiness.has_value());
		REQUIRE(readiness->inlets.size() == 1); CHECK(readiness->inlets[0].source == rig.source);
		CHECK(readiness->inlets[0].lease_payable); CHECK(readiness->inlets[0].usage_payable);
	}
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == physical); CHECK(ComputeTubePowerVehicleDigest() == bindings);
	CHECK(Company::Get(_current_company)->money == money); CHECK(CSegmentCostCacheBase::s_rail_change_counter == revision);
	TimerGameEconomy::date = TimerGameEconomy::Date{101}; Company::Get(_current_company)->money = Money{0};
	const auto unpaid = GetTubeNetworkReadiness(rig.depot, _current_company); REQUIRE(unpaid.has_value()); REQUIRE(unpaid->inlets.size() == 1);
	CHECK_FALSE(unpaid->inlets[0].lease_payable); CHECK_FALSE(unpaid->inlets[0].usage_payable);
	CHECK(ComputeUtilityPowerDigest() == ledger); CHECK(ComputeTubePowerDigest() == physical);
	Company::Get(_current_company)->money = money;
	const auto segment = TileXYL(38, 41, MapLayer::Underground);
	REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, segment, Track::Y).Succeeded());
	CHECK(GetTubeNetworkReadiness(rig.destination, _current_company)->inlets.empty());
	REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, segment, rig.tube, Track::Y, false).Succeeded());
	CHECK(GetTubeNetworkReadiness(rig.destination, _current_company)->inlets.size() == 1);
	REQUIRE(CloseTubeExternalPower(rig.source) == TubePowerError::None);
	CHECK(GetTubeNetworkReadiness(rig.destination, _current_company)->inlets.empty());
}
