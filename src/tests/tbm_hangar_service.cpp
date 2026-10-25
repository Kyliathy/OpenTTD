/* This file is part of OpenTTD, licensed under GNU GPL version 2. */
/** @file tbm_hangar_service.cpp Focused native service interruption and paid-work rejoin, extracted from tbm_hangar.cpp.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical withdrawal and cargo owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group8.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23D.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "underground_depot_fixture.h"
#include "layer_window_fixture.h"
#include "../tbm_hangar.h"
#include "../safeguards.h"

/** The selected service command retains the ordinary native depot visit and the same paid unfinished work. */
TEST_CASE("A TBM visits a selected native depot for service and rejoins its work", "[tbm-hangar-service]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const auto id = workflow.StartFurther();
	/** @return Whether the actual cutter is at an unfinished face away from storage. */
	const auto cutting = [&]() { return GetTBMProject(id)->phase == TBMPhase::Bore && GetExcavationJob(GetTBMProject(id)->job)->partial_work_ticks >= 20; };
	workflow.DriveUntil(cutting);
	const auto before = *GetExcavationJob(GetTBMProject(id)->job);
	Train *front = Train::Get(workflow.first.cutter);
	workflow.Do<Commands::SendTBMToHangar>(front->index, workflow.connection, false); // The explicit service request below must supersede an unfinished selected-hangar trip.
	workflow.Do<Commands::SendTBMToHangar>(front->index, workflow.depot, true);
	REQUIRE_FALSE(front->current_order.GetDepotActionType().Test(OrderDepotActionFlag::Halt));
	bool serviced = false;
	/** @return Whether native depot entry completed the actual service order. */
	const auto entered = [&]() {
		serviced = front->tile == workflow.depot && front->IsChainInDepot() && !front->current_order.IsType(OT_GOTO_DEPOT);
		return serviced;
	};
	workflow.DriveUntil(entered);
	CHECK(*GetExcavationJob(before.id) == before);
	/** @return Whether native departure and approach reach the next real cut of the original job. */
	const auto resumed = [&]() { return GetExcavationJob(before.id)->cursor > before.cursor; };
	workflow.DriveUntil(resumed);
	CHECK(serviced);
	CHECK(GetTBMProject(id)->cutter == front->index);
	CHECK(GetTBMProject(id)->job == before.id);
}

