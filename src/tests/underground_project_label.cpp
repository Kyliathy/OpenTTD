/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_project_label.cpp Real native conventional hauler assignment and visible project identity. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 7B */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "tbm_exchange_fixture.h"
#include "../underground_vehicle.h"
#include "../vehicle_gui.h"
#include "../vehicle_cmd.h"
#include "../widgets/vehicle_widget.h"
#include "../strings_func.h"
#include "../table/strings.h"
#include "../safeguards.h"

/** Actual native haulers display their controller-owned project association while remaining ordinary train identities. */
TEST_CASE("Native conventional hauler shows its actual project label in the normal vehicle window", "[underground-project-label]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	TBMExchangeRig rig;
	REQUIRE(InitialiseUndergroundEngineDefinitions(rig.fixture.package));
	const auto engine = GetUndergroundEngine(4);
	Engine::Get(engine)->company_avail.Set(_current_company);
	const auto [cost, id, capacity, mail, cargo_capacities] = CmdBuildVehicle(DoCommandFlag::Execute, rig.depot, engine, false, INVALID_CARGO, ClientID::Invalid);
	REQUIRE(cost.Succeeded());
	Train *hauler = Train::Get(id);
	// The real exchange admission requires both actual empty hopper kinds; ordinary build and coupling commands supply them.
	for (uint16_t model : {uint16_t{2}, uint16_t{2}, uint16_t{3}, uint16_t{3}}) {
		const auto [wagon_cost, wagon_id, wagon_capacity, wagon_mail, wagon_cargo] = CmdBuildVehicle(DoCommandFlag::Execute, rig.depot,
			GetUndergroundEngine(model), false, INVALID_CARGO, ClientID::Invalid);
		REQUIRE(wagon_cost.Succeeded());
		REQUIRE(CmdMoveRailVehicle(DoCommandFlag::Execute, wagon_id, hauler->index, false).Succeeded());
	}
	REQUIRE(AssignTBMHauler(rig.project.id, hauler->index, true) == TBMExchangeError::None);
	const auto *assignment = GetTBMExchangeAssignmentForHauler(hauler->index);
	REQUIRE(assignment != nullptr);
	CHECK(assignment->project == rig.project.id);
	LayerWindowTestUI ui(WindowClass::VehicleView, _current_company);
	ShowVehicleViewWindow(hauler);
	Window *view = FindWindowById(WindowClass::VehicleView, hauler->index);
	REQUIRE(view != nullptr);
	CHECK(view->GetWidgetString(WID_VV_CAPTION, STR_VEHICLE_VIEW_CAPTION).find(GetString(STR_UNDERGROUND_PROJECT_ENTRY, rig.project.id.base())) != std::string::npos);
}
