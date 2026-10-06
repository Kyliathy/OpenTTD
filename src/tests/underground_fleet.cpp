/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_fleet.cpp Actual purchase, refit, clone, renewal and ordinary fleet membership. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 7B */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_fleet_fixture.h"
#include "../engine_func.h"
#include "../autoreplace_cmd.h"
#include "../train_cmd.h"
#include "../group_cmd.h"
#include "../vehiclelist.h"
#include "../vehicle_gui.h"
#include "../widgets/vehicle_widget.h"
#include "../widgets/build_vehicle_widget.h"
#include "../map_layer_gui.h"
#include "../strings_func.h"
#include "../safeguards.h"

/** Actual ordinary trucks preserve native capability parity across purchase, refit, cloning and renewal, and stay normal manageable vehicles. */
TEST_CASE("Native trucks share command capability and normal fleet owners on both rock planes", "[underground-fleet]")
{
	MapLayer layer = MapLayer::Underground;
	SECTION("Underground") {}
	SECTION("Deep Underground") { layer = MapLayer::DeepUnderground; }
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFleetRig rig;
	AutoRestoreBackup local(_local_company, _current_company);
	const auto depot = rig.BuildDepot(layer, true);
	const auto bulk = GetUndergroundEngine(7);
	const auto copper = GetCargoTypeByLabel(CargoLabel{"CORE"});
	const auto uranium = GetCargoTypeByLabel(CargoLabel{"URAN"});
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto funds = Company::Get(_current_company)->money;
	const auto seeds = _random;
	// Unsupported buy/refit estimates refuse before temporary acquisition can change IDs, ordinary group counts, cash or either RNG stream.
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::AutoReplace}}) {
		CHECK(std::get<0>(CmdBuildVehicle(flags, depot, bulk, false, uranium, ClientID::Invalid)).Failed());
		CHECK(Vehicle::GetNumItems() == 0);
	}
	CHECK(Company::Get(_current_company)->money == funds);
	CHECK(_random.state[0] == seeds.state[0]);
	CHECK(_random.state[1] == seeds.state[1]);
	Vehicle *truck = rig.Buy(depot, 7);
	CHECK(truck->GetRunningCost() > 0);
	CHECK(truck->IsPrimaryVehicle());
	CHECK(truck->owner == _current_company);
	CHECK(std::get<0>(CmdRefitVehicle(DoCommandFlag::Execute, truck->index, copper, 0, false, true, 1)).Succeeded());
	CHECK(truck->cargo_type == copper);
	CHECK(std::get<0>(CmdRefitVehicle(DoCommandFlag::Execute, truck->index, uranium, 0, false, true, 1)).Failed());
	CHECK(truck->cargo_type == copper);
	truck->reliability = 0;
	VehicleServiceInDepot(truck);
	CHECK(truck->reliability == truck->GetEngine()->reliability);
	CHECK(CmdChangeServiceInt(DoCommandFlag::Execute, truck->index, 150, true, false).Succeeded());
	const auto [group_result, group] = CmdCreateGroup(DoCommandFlag::Execute, VehicleType::Road, GroupID::Invalid());
	REQUIRE(group_result.Succeeded());
	REQUIRE(std::get<0>(CmdAddVehicleGroup(DoCommandFlag::Execute, group, truck->index, false, {})).Succeeded());
	VehicleList list;
	REQUIRE(GenerateVehicleSortList(&list, VehicleListIdentifier(VehicleListType::Group, VehicleType::Road, _current_company, group)));
	CHECK(list == VehicleList{truck});
	const auto before_clone = Company::Get(_current_company)->money;
	const auto [clone_result, clone_id] = CmdCloneVehicle(DoCommandFlag::Execute, depot, truck->index, true);
	REQUIRE(clone_result.Succeeded());
	CHECK(Vehicle::Get(clone_id)->engine_type == bulk);
	CHECK(Vehicle::Get(clone_id)->cargo_type == copper);
	CHECK(Vehicle::Get(clone_id)->group_id == group);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == before_clone - clone_result.GetCost()); // Clone's nested native commands charge their actual acquisition/refit once in this command-body fixture.
	// Accepted native acquisition consumes random construction bits; refused requests were checked before this point.
	CHECK(_random.state[0] != seeds.state[0]);
	{
		LayerWindowTestUI ui(WindowClass::VehicleView, _current_company);
		ShowVehicleViewWindow(truck);
		Window *view = FindWindowById(WindowClass::VehicleView, truck->index);
		REQUIRE(view != nullptr);
		CHECK(view->GetWidgetString(WID_VV_CAPTION, STR_VEHICLE_VIEW_CAPTION).find(GetString(GetMapLayerName(layer))) != std::string::npos);
	}
	Company *company = Company::Get(_current_company);
	company->settings.engine_renew = true;
	company->settings.engine_renew_months = 0;
	truck->age = truck->max_age + 1;
	const VehicleID old_id = truck->index;
	REQUIRE(truck->NeedsAutorenewing(company));
	REQUIRE(CmdAutoreplaceVehicle(DoCommandFlag::Execute, old_id).Succeeded());
	CHECK_FALSE(Vehicle::IsValidID(old_id));
	CHECK(Vehicle::GetNumItems() == 2);
	REQUIRE(GenerateVehicleSortList(&list, VehicleListIdentifier(VehicleListType::Group, VehicleType::Road, _current_company, group)));
	CHECK(list.size() == 2);
	// Renewal retains cargo, group and service membership through the ordinary replacement owner, not a parallel project fleet.
	for (const Vehicle *vehicle : list) {
		CHECK(vehicle->engine_type == bulk);
		CHECK(vehicle->cargo_type == copper);
		CHECK(vehicle->group_id == group);
	}
}

/** A manipulated company-availability flag cannot authorize earlier sealed purchases, Deep cloning/refits or renewal. */
TEST_CASE("Native freight commands refuse stale calendar and layer context without partial allocation", "[underground-fleet]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFleetRig rig;
	const auto depot = rig.BuildDepot(MapLayer::DeepUnderground, true);
	Vehicle *sealed = rig.Buy(depot, 8);
	const auto count = Vehicle::GetNumItems();
	const auto seeds = _random;
	const auto cargo = sealed->cargo_type;
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	CHECK_FALSE(IsEngineBuildable(GetUndergroundEngine(8), VehicleType::Road, _current_company));
	CHECK(std::get<0>(CmdBuildVehicle(DoCommandFlag::Execute, depot, GetUndergroundEngine(7), false, INVALID_CARGO, ClientID::Invalid)).Failed());
	CHECK(std::get<0>(CmdCloneVehicle(DoCommandFlag::Execute, depot, sealed->index, false)).Failed());
	CHECK(std::get<0>(CmdRefitVehicle(DoCommandFlag::Execute, sealed->index, GetCargoTypeByLabel(CargoLabel{"RARE"}), 0, false, true, 1)).Failed());
	CHECK(CmdAutoreplaceVehicle(DoCommandFlag::Execute, sealed->index).Failed());
	CHECK(Vehicle::GetNumItems() == count);
	CHECK(sealed->cargo_type == cargo);
	CHECK(_random.state[0] == seeds.state[0]);
	CHECK(_random.state[1] == seeds.state[1]);
}

/** Whole-train replacement checks every prospective native wagon before any temporary locomotive/wagon allocation. */
TEST_CASE("Conventional trains refuse incompatible sealed replacement and renew compatible mineral wagons", "[underground-fleet]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFleetRig rig;
	const auto depot = rig.BuildDepot(MapLayer::DeepUnderground, false);
	Train *front = Train::From(rig.Buy(depot, 4));
	Train *wagon = Train::From(rig.Buy(depot, 5));
	REQUIRE(CmdMoveRailVehicle(DoCommandFlag::Execute, wagon->index, front->index, false).Succeeded());
	CHECK(front->GetRunningCost() > 0);
	REQUIRE(CmdSetAutoReplace(DoCommandFlag::Execute, ALL_GROUP, GetUndergroundEngine(5), GetUndergroundEngine(6), false).Succeeded());
	const auto count = Vehicle::GetNumItems();
	const auto seeds = _random;
	const auto funds = Company::Get(_current_company)->money;
	CHECK(CmdAutoreplaceVehicle(DoCommandFlag::Execute, front->index).Failed());
	CHECK(Vehicle::GetNumItems() == count);
	CHECK(Company::Get(_current_company)->money == funds);
	CHECK(_random.state[0] == seeds.state[0]);
	CHECK(_random.state[1] == seeds.state[1]);
	CHECK(wagon->engine_type == GetUndergroundEngine(5));
	const auto rare = GetCargoTypeByLabel(CargoLabel{"RARE"});
	REQUIRE(std::get<0>(CmdRefitVehicle(DoCommandFlag::Execute, wagon->index, rare, 0, false, true, 1)).Succeeded());
	REQUIRE(CmdAutoreplaceVehicle(DoCommandFlag::Execute, front->index).Succeeded());
	REQUIRE(front->GetNextUnit() != nullptr);
	CHECK(front->GetNextUnit()->engine_type == GetUndergroundEngine(6));
	CHECK(front->GetNextUnit()->cargo_type == rare);
	CHECK(Vehicle::GetNumItems() == count);
}

/** The actual depot purchase window hides all date/layer-ineligible native rows using the same command predicate. */
TEST_CASE("Deep road purchase UI and command share their calendar gate", "[underground-fleet-ui]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundFleetRig rig;
	const auto depot = rig.BuildDepot(MapLayer::DeepUnderground, true);
	LayerWindowTestUI ui(WindowClass::BuildVehicle, _current_company);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	ShowBuildVehicleWindow(depot, VehicleType::Road);
	Window *window = FindWindowById(WindowClass::BuildVehicle, depot.base());
	REQUIRE(window != nullptr);
	window->OnPaint();
	CHECK(window->IsWidgetDisabled(WID_BV_BUILD));
	CHECK(std::get<0>(CmdBuildVehicle(DoCommandFlag::Execute, depot, GetUndergroundEngine(7), false, INVALID_CARGO, ClientID::Invalid)).Failed());
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	window->OnInvalidateData(0, true);
	window->OnPaint();
	CHECK_FALSE(window->IsWidgetDisabled(WID_BV_BUILD));
	CHECK(std::get<0>(CmdBuildVehicle({}, depot, GetUndergroundEngine(7), false, INVALID_CARGO, ClientID::Invalid)).Succeeded());
	CHECK(Vehicle::GetNumItems() == 0);
}
