/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_admission.cpp Real native truck elevator travel, articulated reference admission and actual rail power checks. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 7C */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_fleet_fixture.h"
#include "movement_fixture.h"
#include "tbm_fixture.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../engine_func.h"
#include "../rail_map.h"
#include "../order_cmd.h"
#include "../safeguards.h"

/** An actual command-built road elevator and short roads connecting a real native truck's depot to Deep. */
class UndergroundElevatorRig : public UndergroundFleetRig {
public:
	PathfinderDefaults pathfinder; ///< Ordinary movement gets the normal pathfinder budget and back-off settings.
	TileIndex source = INVALID_TILE; ///< Real upper elevator housing.
	TileIndex target = INVALID_TILE; ///< Real Deep elevator housing.
	RoadVehicle *truck = nullptr; ///< Actual normal command purchase, with no substituted position or travel state.

	/** @param upper Surface or Underground origin, exercising both legal road elevator pairs. */
	explicit UndergroundElevatorRig(MapLayer upper)
	{
		const auto depot = this->BuildDepot(upper, true);
		this->source = TileAddWrap(depot, 0, 4);
		this->target = MultiLayerMap::TileInLayer(this->source, MapLayer::DeepUnderground);
		std::vector<TileIndex> floor{this->source, this->target};
		// Full road floors and approaches are excavated before their ordinary commands; the elevator opens no free geological room.
		for (int offset = 1; offset <= 3; ++offset) floor.push_back(TileAddWrap(depot, 0, offset));
		for (int offset = 1; offset <= 5; ++offset) floor.push_back(TileAddWrap(this->target, 0, offset));
		std::vector<TileIndex> rock_floor;
		// The geological owner accepts rock-plane cells only; Surface approaches retain ordinary map terrain.
		for (TileIndex tile : floor) {
			if (GetTileLayer(tile) != MapLayer::Surface) rock_floor.push_back(tile);
		}
		this->Open(rock_floor);
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, this->source, TransportType::Road, MapLayer::DeepUnderground,
			DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
		// Both roads continue on the shaft axis; a genuine Deep terminal supplies an ordinary route destination.
		for (int offset = 1; offset <= 3; ++offset) REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, TileAddWrap(depot, 0, offset),
			AxisToRoadBits(Axis::Y), ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded());
		for (int offset = 1; offset <= 4; ++offset) REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, TileAddWrap(this->target, 0, offset),
			AxisToRoadBits(Axis::Y), ROADTYPE_ROAD, {}, TownID::Invalid()).Succeeded());
		// Native housings begin Sealed even on prepared floor. The real connection command revalidates both owned approaches before traffic can use them.
		REQUIRE(CmdChangeLayerPortalConnection(DoCommandFlag::Execute, this->source, true).Succeeded());
		REQUIRE(GetLayerPortalWorkState(this->source) == LayerPortalWorkState::Connected);
		this->truck = RoadVehicle::From(this->Buy(depot, 7));
		const auto stop = TileAddWrap(this->target, 0, 5);
		REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, stop, 1, 1, RoadStopType::Truck, false, DiagDirection::NW,
			ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		Order order;
		order.MakeGoToStation(Station::GetByTile(stop)->index);
		order.SetStopLocation(OrderStopLocation::FarEnd);
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, this->truck->index, 0, order).Succeeded());
	}
};

/** A genuine normal purchase reaches Deep through the actual timed elevator movement rather than a fixture coordinate change. */
TEST_CASE("Native mine trucks physically ride both Deep road elevator pairs", "[underground-admission]")
{
	MapLayer upper = MapLayer::Surface;
	SECTION("Surface to Deep") {}
	SECTION("Underground to Deep") { upper = MapLayer::Underground; }
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundElevatorRig rig(upper);
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, rig.truck->index, true).Succeeded());
	bool saw_wait = false;
	uint ticks = 0;
	// Bounded actual vehicle ticks must observe a timed ride before entering Deep; no camera, cash or position writes grant passage.
	while (GetTileLayer(rig.truck->tile) != MapLayer::DeepUnderground && ticks < 6000) {
		REQUIRE(rig.truck->Tick()); // Isolate actual physical ticks from the day dispatcher; this fixture intentionally holds calendar/economy dates fixed.
		saw_wait |= rig.truck->elevator_wait > 0;
		REQUIRE_FALSE(rig.truck->vehstatus.Test(VehState::Crashed));
		++ticks;
	}
	INFO("ticks " << ticks << ", tile " << rig.truck->tile.base() << ", wait " << rig.truck->elevator_wait << ", frame " << static_cast<uint>(rig.truck->frame)
		<< ", state " << static_cast<uint>(rig.truck->state) << ", speed " << rig.truck->cur_speed << ", destination " << rig.truck->dest_tile.base());
	REQUIRE(GetTileLayer(rig.truck->tile) == MapLayer::DeepUnderground);
	CHECK(saw_wait);
	CHECK(rig.truck->engine_type == GetUndergroundEngine(7));
	CHECK(rig.truck->cargo.TotalCount() == 0);
}

/** Pool-owned front/follower references exercise the existing articulated admission boundary; this does not define an articulated native purchase model. */
TEST_CASE("Road elevator tile admission accepts a truck follower and refuses trams or locked Deep", "[underground-admission]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundElevatorRig rig(MapLayer::Surface);
	REQUIRE(RoadVehicle::CanAllocateItem());
	auto *follower = RoadVehicle::Create();
	follower->SetArticulatedPart();
	follower->owner = _current_company;
	follower->engine_type = rig.truck->engine_type;
	follower->roadtype = ROADTYPE_ROAD;
	follower->direction = rig.truck->direction; // The physical follower reference approaches on the same valid heading as its actual front.
	follower->vehstatus.Set(VehState::Hidden);
	rig.truck->SetNext(follower);
	// Every actual endpoint runs the real tile procedure for both physical roles; no saved controller or vehicle identity is fabricated.
	for (TileIndex end : {rig.source, rig.target}) {
		for (RoadVehicle *part : {rig.truck, follower}) {
			CHECK_FALSE(VehicleEnterTile(part, end, TileX(end) * TILE_SIZE, TileY(end) * TILE_SIZE).Test(VehicleEnterTileState::CannotEnter));
			part->roadtype = ROADTYPE_TRAM;
			CHECK(VehicleEnterTile(part, end, TileX(end) * TILE_SIZE, TileY(end) * TILE_SIZE).Test(VehicleEnterTileState::CannotEnter));
			part->roadtype = ROADTYPE_ROAD;
		}
	}
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CHECK(VehicleEnterTile(rig.truck, rig.source, TileX(rig.source) * TILE_SIZE, TileY(rig.source) * TILE_SIZE).Test(VehicleEnterTileState::CannotEnter));
	CHECK(VehicleEnterTile(follower, rig.target, TileX(rig.target) * TILE_SIZE, TileY(rig.target) * TILE_SIZE).Test(VehicleEnterTileState::CannotEnter));
}

/** A real electric original engine cannot be acquired on unpowered native work/depot rail regardless of shared sprites, while diesel freight is compatible. */
TEST_CASE("Native work and depot rail preserve ordinary electric power restrictions", "[underground-admission]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFleetRig rig;
	const auto depot = rig.BuildDepot(MapLayer::DeepUnderground, false);
	REQUIRE(Engine::CanAllocateItem());
	Engine *electric = Engine::Create(VehicleType::Train, 23); // Actual original SH '30' electric definition, not a sprite-based native capability alias.
	_engine_mngr.SetID(VehicleType::Train, 23, INVALID_GRFID, 23, electric->index);
	electric->info.cargo_type = GetCargoTypeByLabel(CargoLabel{"PASS"});
	electric->company_avail.Set(_current_company);
	Company::Get(_current_company)->avail_railtypes.Set(RAILTYPE_ELECTRIC);
	REQUIRE(IsEngineBuildable(electric->index, VehicleType::Train, _current_company));
	CHECK_FALSE(HasPowerOnRail(electric->VehInfo<RailVehicleInfo>().railtypes, RAILTYPE_RAIL));
	const auto count = Vehicle::GetNumItems();
	CHECK(std::get<0>(CmdBuildVehicle(DoCommandFlag::Execute, depot, electric->index, false, INVALID_CARGO, ClientID::Invalid)).Failed());
	CHECK(Vehicle::GetNumItems() == count);
	Vehicle *freight = rig.Buy(depot, 4);
	CHECK(HasPowerOnRail(freight->GetEngine()->VehInfo<RailVehicleInfo>().railtypes, GetRailType(depot)));
	CHECK(Train::From(freight)->gcache.cached_power > 0);
}

/** Completed native useful work lays real support rail with its housing's railtype; the temporary support appearance cannot create electrical power. */
TEST_CASE("Actual TBM support keeps its ordinary railtype power domain", "[underground-admission]")
{
	MockEnvironment::Instance();
	TBMWorkRig rig;
	rig.BoreOneCut();
	const auto tile = rig.Job().route.front().cutter_position;
	REQUIRE(GetTBMSupportTile(tile) != nullptr);
	REQUIRE(IsPlainRailTile(tile));
	const auto railtype = GetRailType(tile);
	CHECK(railtype == GetRailType(rig.Job().commissioning_anchor));
	CHECK_FALSE(HasPowerOnRail(RAILTYPE_ELECTRIC, railtype));
	CHECK(HasPowerOnRail(rig.front->GetEngine()->VehInfo<RailVehicleInfo>().railtypes, railtype));
}
