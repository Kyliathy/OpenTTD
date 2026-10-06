/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file portal_fleet_date.cpp Real native trucks obey the Deep calendar gate at both road-elevator origins.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 17
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_fleet_fixture.h"
#include "movement_fixture.h"
#include "../order_cmd.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../town_kdtree.h"
#include "../timer/timer_game_economy.h"

#include "../safeguards.h"

/** A real paid truck, two owned truck stops and a Connected elevator on explicitly prepared initial scenario floor. */
class PortalDateRoadLine {
	PathfinderDefaults pathfinder; ///< Ordinary native route-planning defaults.
	UndergroundFleetRig rig; ///< Owns real company, content, vehicles and facilities.
public:
	MapLayer upper; ///< Surface or Underground elevator origin.
	TileIndex upper_end = INVALID_TILE; ///< Upper elevator endpoint.
	TileIndex deep_end = INVALID_TILE; ///< Deep endpoint at the same plane-local coordinate.
	Station *home = nullptr; ///< Actual upper freight terminal.
	Station *away = nullptr; ///< Actual Deep freight terminal.
	RoadVehicle *truck = nullptr; ///< Command-purchased native bulk truck.

	/**
	 * Construct legal initial infrastructure on the unlocked date without moving vehicles or supplying cargo.
	 * @param upper Surface or Underground origin of the road elevator.
	 */
	explicit PortalDateRoadLine(MapLayer upper) : upper(upper)
	{
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
		this->upper_end = TileXYL(40, 30, upper);
		this->deep_end = MultiLayerMap::TileInLayer(this->upper_end, MapLayer::DeepUnderground);
		const TileIndex depot = TileXYL(39, 28, upper);
		std::vector<TileIndex> floor;
		// Initial scenario floor covers both real terminal approaches and the entire route; construction itself cannot excavate.
		for (uint y = 26; y <= 30; ++y) floor.push_back(TileXYL(40, y, upper));
		floor.push_back(depot);
		for (uint y = 30; y <= 36; ++y) floor.push_back(TileXYL(40, y, MapLayer::DeepUnderground));
		// Surface terrain already belongs to the ordinary map; only actual rock-layer cells enter the geological fixture preparation.
		/** @param tile Candidate initial floor cell. @return Whether ordinary Surface terrain needs no geological opening. */
		std::erase_if(floor, [](TileIndex tile) { return GetTileLayer(tile) == MapLayer::Surface; });
		this->rig.Open(floor);
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, this->upper_end, TransportType::Road, MapLayer::DeepUnderground,
			DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
		// Both owned approaches must physically exist before the pair can become Connected.
		for (const auto [layer, first, last] : {std::tuple{upper, 27U, 29U}, std::tuple{MapLayer::DeepUnderground, 31U, 35U}}) {
			for (uint y = first; y <= last; ++y) {
				REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, TileXYL(40, y, layer), AxisToRoadBits(Axis::Y), ROADTYPE_ROAD,
					DisallowedRoadDirections{}, TownID::Invalid()).Succeeded());
			}
		}
		REQUIRE(CmdBuildRoadDepot(DoCommandFlag::Execute, depot, ROADTYPE_ROAD, DiagDirection::SW).Succeeded());
		// Direct command-body construction omits the UI callback: pay for the real depot junction through the road owner.
		REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, TileXYL(40, 28, upper), RoadBits{RoadBit::NE}, ROADTYPE_ROAD,
			DisallowedRoadDirections{}, TownID::Invalid()).Succeeded());
		REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, TileXYL(40, 26, upper), 1, 1, RoadStopType::Truck, false,
			DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, TileXYL(40, 36, MapLayer::DeepUnderground), 1, 1, RoadStopType::Truck, false,
			DiagDirection::NW, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		this->home = Station::GetByTile(TileXYL(40, 26, upper));
		this->away = Station::GetByTile(TileXYL(40, 36, MapLayer::DeepUnderground));
		REQUIRE(ChangeLayerPortalConnection(this->upper_end, _current_company, true, true) == LayerPortalWorkError::None);
		this->truck = RoadVehicle::From(this->rig.Buy(depot, 7));
		// Actual station orders make both descending and ascending elevator journeys observable in native movement.
		for (Station *station : {this->away, this->home}) {
			Order order;
			order.MakeGoToStation(station->index);
			order.SetStopLocation(OrderStopLocation::FarEnd); // Ordinary road orders have no train-specific platform stop-position choice.
			const auto inserted = CmdInsertOrder(DoCommandFlag::Execute, this->truck->index, this->truck->GetNumOrders(), order);
			INFO("order=" << inserted.GetErrorMessage().base()
				<< " bus=" << this->truck->IsBus() << " roads=" << this->truck->compatible_roadtypes.base());
			REQUIRE(inserted.Succeeded());
		}
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, this->truck->index, true).Succeeded());
	}

	/** Advance one genuine physical vehicle step and require that the real truck survives it. */
	void Tick() const
	{
		CallVehicleTicks();
		REQUIRE_FALSE(this->truck->vehstatus.Test(VehState::Crashed));
	}

	/**
	 * Drive native station/elevator movement to an observed condition with a finite diagnostic limit.
	 * @param done Condition on the real truck's current world state.
	 */
	void Drive(const std::function<bool()> &done) const
	{
		uint ticks = 0;
		// Bounded native stepping detects route/timer failures without teleporting the truck or forcing its elevator phase.
		while (!done() && ticks < 12000) {
			this->Tick();
			++ticks;
		}
		INFO("truck tile=" << this->truck->tile.base() << " state=" << uint{this->truck->state} << " elevator=" << this->truck->elevator_wait);
		REQUIRE(done());
	}
};

/** Both real elevator origins exclude every truck part before 1980, then permit physical descent and the empty return. */
TEST_CASE("Actual road elevator fleets wait for Deep calendar access at both upper origins", "[portal-fleet-date]")
{
	MapLayer upper = MapLayer::Surface;
	SECTION("Surface to Deep elevator") {}
	SECTION("Underground to Deep elevator") { upper = MapLayer::Underground; }
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage(); // Load presentation before the fixture installs its deliberately scoped cargo definitions.
	PortalDateRoadLine line(upper);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	AutoRestoreBackup economy(TimerGameEconomy::date, TimerGameEconomy::ConvertYMDToDate(TimerGameEconomy::Year{2090}, 0, 1));
	// Existing Connected topology and a started native truck cannot turn an economic clock into technology permission.
	for (uint tick = 0; tick < 2000; ++tick) {
		line.Tick();
		// Include articulated parts if the native model ever gains them; no portion of the physical chain may enter early.
		for (const RoadVehicle *part = line.truck; part != nullptr; part = part->Next()) REQUIRE(GetTileLayer(part->tile) == upper);
	}
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	/** @return Whether native station handling has recorded the genuine Deep arrival. */
	line.Drive([&line] { return line.truck->last_station_visited == line.away->index; });
	CHECK(GetTileLayer(line.truck->tile) == MapLayer::DeepUnderground);
	/** @return Whether native station handling has recorded the genuine return to the upper terminal. */
	line.Drive([&line] { return line.truck->last_station_visited == line.home->index; });
	CHECK(GetTileLayer(line.truck->tile) == upper);
}
