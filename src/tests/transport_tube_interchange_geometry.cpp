/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_interchange_geometry.cpp Native joined facility cells before interchange source authoring.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Group48D.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-underground-transport-tubes.md Group9.
 * Geometry checks do not claim operating transfer, loading reservations or payment acceptance.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_window_fixture.h"
#include "mock_fontcache.h"
#include "transport_tube_fixture.h"
#include "../safeguards.h"

/** Actual separate tube/rail berths and a truck stop sharing one native station on fixture-open floor. */
class TubeInterchangeGeometryRig : public TubePrototypeRig {
public:
	TileIndex tube_platform = TileXYL(16, 16, MapLayer::Underground); ///< First actual two-cell tube berth.
	TileIndex rail_platform = INVALID_TILE; ///< Separate conventional berth, never a mixed track cell.
	TileIndex truck_stop = TileXYL(21, 21, MapLayer::Underground); ///< Actual one-cell freight stop.
	StationID station = StationID::Invalid(); ///< Sole native cargo/order owner for the joined facilities.
	Axis axis; ///< Actual berth axis for both railway modes.

	/**
	 * Build the actual native facility combination through quote and execute commands.
	 * @param axis Actual X/Y berth axis, independent of camera selection.
	 * @param entrance One of the four real one-ended truck-stop entrances.
	 */
	TubeInterchangeGeometryRig(Axis axis, DiagDirection entrance) : axis(axis)
	{
		this->rail_platform = TileAddWrap(this->tube_platform, axis == Axis::X ? 0 : 3, axis == Axis::X ? 3 : 0);
		this->BuildRail(this->tube_platform, this->tube, NEW_STATION);
		this->station = GetStationIndex(this->tube_platform);
		this->BuildRail(this->rail_platform, RAILTYPE_RAIL, this->station);
		const auto delta = TileIndexDiffCByDiagDir(entrance);
		this->Open(std::array{this->truck_stop, TileAddWrap(this->truck_stop, delta.x, delta.y)});
		const auto stations = Station::GetNumItems(), packets = CargoPacket::GetNumItems();
		const Money money = Company::Get(_current_company)->money;
		REQUIRE(CmdBuildRoadStop({}, this->truck_stop, 1, 1, RoadStopType::Truck, false,
			entrance, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, this->station, false).Succeeded());
		CHECK(IsTileType(this->truck_stop, TileType::Clear));
		CHECK(Station::GetNumItems() == stations);
		CHECK(CargoPacket::GetNumItems() == packets);
		CHECK(Company::Get(_current_company)->money == money);
		REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, this->truck_stop, 1, 1, RoadStopType::Truck, false,
			entrance, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, this->station, false).Succeeded());
		CHECK(Station::GetNumItems() == stations);
	}

	/**
	 * Open a precise berth/approaches and build a native two-cell railway platform.
	 * @param tile First actual platform cell.
	 * @param type Tube or conventional native rail type; joining never changes it.
	 * @param join Canonical existing station or NEW_STATION for the first berth.
	 */
	void BuildRail(TileIndex tile, RailType type, StationID join)
	{
		// Only the berth and its two through approaches are fixture-open input.
		// A shared cargo owner does not authorize excavation of a decorative campus.
		for (int along = -1; along <= 2; ++along) {
			this->Open(std::array{TileAddWrap(tile, this->axis == Axis::X ? along : 0, this->axis == Axis::Y ? along : 0)});
		}
		const auto stations = Station::GetNumItems(), packets = CargoPacket::GetNumItems();
		const Money money = Company::Get(_current_company)->money;
		REQUIRE(CmdBuildRailStation({}, tile, type, this->axis, 1, 2, STAT_CLASS_DFLT, 0, join, false).Succeeded());
		CHECK(IsTileType(tile, TileType::Clear));
		CHECK(Station::GetNumItems() == stations);
		CHECK(CargoPacket::GetNumItems() == packets);
		CHECK(Company::Get(_current_company)->money == money);
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, type, this->axis, 1, 2, STAT_CLASS_DFLT, 0, join, false).Succeeded());
	}
};

/** Joined native cells preserve one station/packet owner while capsules remain on their own guideway. */
TEST_CASE("Tube interchange geometry uses native joined rail and road facilities", "[transport-tube-interchange-geometry]")
{
	MockFontCache::InitializeFontCaches();
	LoadGeneratedWindowTestLanguage();
	AutoRestoreBackup<bool> distant{_settings_game.station.distant_join_stations, true};
	// Both physical berth axes and all four genuine road entrances must remain
	// independently buildable. Source art cannot collapse them into four fake
	// rotations of a fixed six-cell campus.
	for (Axis axis : {Axis::X, Axis::Y}) {
		for (DiagDirection entrance : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			CAPTURE(axis, entrance);
			TubeInterchangeGeometryRig rig(axis, entrance);
			Station *station = Station::Get(rig.station);
			CHECK(Station::GetByTile(rig.rail_platform) == station);
			CHECK(Station::GetByTile(rig.truck_stop) == station);
			CHECK(GetRailType(rig.tube_platform) == rig.tube);
			CHECK(GetRailType(rig.rail_platform) == RAILTYPE_RAIL);
			CHECK(GetRailStationAxis(rig.tube_platform) == axis);
			CHECK(GetRailStationAxis(rig.rail_platform) == axis);
			REQUIRE(station->truck_stops != nullptr);
			CHECK(station->truck_stops->xy == rig.truck_stop);
			const auto view = GetTubeTerminal(rig.tube_platform, _current_company);
			REQUIRE(view.has_value());
			CHECK(view->station == station->index);
			CHECK(view->platform_length_tiles == 2);
			CHECK_FALSE(GetTubeTerminal(rig.rail_platform, _current_company).has_value());
			CHECK_FALSE(GetTubeTerminal(rig.truck_stop, _current_company).has_value());
			Train *capsule = rig.BuyCapsule();
			CHECK(CanTubeTrainEnterTile(*capsule, rig.tube_platform));
			CHECK_FALSE(CanTubeTrainEnterTile(*capsule, rig.rail_platform));
			CHECK_FALSE(CanTubeTrainEnterTile(*capsule, rig.truck_stop));
		}
	}
}

/** Cross-layer joins and nearby independent facilities cannot acquire the joined station's cargo identity. */
TEST_CASE("Tube interchange geometry refuses cross-layer station identity", "[transport-tube-interchange-geometry]")
{
	MockFontCache::InitializeFontCaches();
	LoadGeneratedWindowTestLanguage();
	AutoRestoreBackup<bool> distant{_settings_game.station.distant_join_stations, true};
	TubeInterchangeGeometryRig rig(Axis::X, DiagDirection::SE);
	const auto stations = Station::GetNumItems(), packets = CargoPacket::GetNumItems();
	const Money money = Company::Get(_current_company)->money;
	const auto upper_rail = MultiLayerMap::TileInLayer(rig.tube_platform, MapLayer::Surface);
	const auto upper_road = MultiLayerMap::TileInLayer(rig.truck_stop, MapLayer::Surface);
	// Estimates and attempted execution must both reject physical layer mixing
	// before a tile, station, packet owner or company balance changes.
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}}) {
		CHECK(CmdBuildRailStation(flags, upper_rail, RAILTYPE_RAIL, Axis::X, 1, 2,
			STAT_CLASS_DFLT, 0, rig.station, false).GetErrorMessage() == STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER);
		CHECK(CmdBuildRoadStop(flags, upper_road, 1, 1, RoadStopType::Truck, false, DiagDirection::SE,
			ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, rig.station, false).GetErrorMessage() == STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER);
		CHECK(IsTileType(upper_rail, TileType::Clear));
		CHECK(IsTileType(upper_road, TileType::Clear));
		CHECK(Station::GetNumItems() == stations);
		CHECK(CargoPacket::GetNumItems() == packets);
		CHECK(Company::Get(_current_company)->money == money);
	}
	const auto separate = TileXYL(16, 22, MapLayer::Underground);
	rig.Open(std::array{separate, TileAddWrap(separate, 0, 1)});
	REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, separate, 1, 1, RoadStopType::Truck, false, DiagDirection::SE,
		ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, true).Succeeded());
	CHECK(GetStationIndex(separate) != rig.station);
	CHECK(&Station::GetByTile(separate)->goods != &Station::Get(rig.station)->goods);
	CHECK(Station::GetNumItems() == stations + 1);
	CHECK(CargoPacket::GetNumItems() == packets);
}
