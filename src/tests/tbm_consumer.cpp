/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_consumer.cpp Consumer fixture for r2mlvs5: uses only the public controller boundary in tbm.h, the way the
 * exchange and hauler owners will. It proves the published types, roles, work-state projection and the single
 * controller step. Face work is proven in tbm_work.cpp; driving into the pocket (master Group 7) and exchange
 * (Group 4.2B with Plan 5) are not claimed here.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 5
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"

#include "../safeguards.h"

/** What a consumer can read from an assigned consist: its project, roles, wagon units and capacities. */
TEST_CASE("Consumers read assignment, roles and work state through the public boundary", "[tbm-consumer]")
{
	TBMProjectRig rig;
	const TBMProject *project = GetTBMProjectForCutter(rig.front->index);
	REQUIRE(project != nullptr);
	const auto layout = InspectTBMConsist(*rig.front, *project);
	REQUIRE(layout.error == TBMBlockedReason::None);
	REQUIRE(layout.exchangeable_units.size() == 4);
	// Every exchangeable unit is a whole dedicated wagon with a role the exchange owner can act on.
	for (const auto &unit : layout.exchangeable_units) {
		const auto role = GetTBMConsistRole(*project, *Train::Get(unit.vehicle));
		CHECK(role == unit.role);
		CHECK((role == TBMConsistRole::EarthWagon || role == TBMConsistRole::RockWagon));
	}
	const auto dimensions = MeasureTBMConsist(*rig.front, *project);
	REQUIRE(dimensions.error == TBMAcquisitionError::None);
	CHECK(dimensions.earth_capacity_t == 128);
	CHECK(dimensions.rock_capacity_t == 256);
	CHECK(dimensions.consist_length == 48);
	CHECK(dimensions.planning_points == 4);
	CHECK(dimensions.vehicle_parts == 6);

	const auto state = GetTBMWorkState(project->id);
	CHECK(state.phase == TBMPhase::Commissioning);
	CHECK(state.blocked == TBMBlockedReason::None);
	CHECK(state.pass_cursor == 0);
	CHECK(state.partial_work_ticks == 0);
	CHECK(state.heading == rig.proof.exit_heading);
	REQUIRE(GetExcavationJob(project->job) != nullptr);
	CHECK(state.face == GetExcavationJob(project->job)->route[0].cutter_position);
	CHECK(GetTBMWorkState(TBMProjectID{99}).blocked == TBMBlockedReason::MissingProject);

	// The adapter plans with the measured consist; while this project's own job holds the pocket, a new area here refuses.
	TBMAreaRequest area;
	area.outline = NormalizeExcavationOutline(MapLayer::Underground, rig.plan.job.requested_tiles);
	area.start = {rig.proof.destination, rig.proof.exit_heading, std::vector<TileIndex>(dimensions.planning_points, rig.proof.destination)};
	CHECK(ValidateTBMAreaRoute(*project, area).error != ExcavationError::None);
	auto lost = *project;
	lost.cutter = VehicleID::Invalid();
	CHECK(ValidateTBMAreaRoute(lost, area).consist_error == TBMAcquisitionError::InvalidConsist);
}

/** The single controller step holds a stopped train, then leaves Commissioning and waits for work facts. */
TEST_CASE("The controller step holds stopped trains and waits for work facts", "[tbm-consumer]")
{
	TBMProjectRig rig;
	const auto id = rig.project.id;
	REQUIRE(AdvanceTBMWork(id) == TBMProjectError::None);
	CHECK(GetTBMProject(id)->blocked == TBMBlockedReason::Stopped); // Acquired parts are built stopped.
	CHECK(GetTBMProject(id)->phase == TBMPhase::Commissioning);
	CHECK(GetTBMProject(id)->idle_ticks == 1);
	rig.front->vehstatus.Reset(VehState::Stopped);
	REQUIRE(AdvanceTBMWork(id) == TBMProjectError::None);
	CHECK(GetTBMProject(id)->phase == TBMPhase::TravelToFace);
	// The cutter is still at the source bay, not at the housing, so it holds in TravelToFace until it can drive in.
	for (int tick = 0; tick < 3; ++tick) REQUIRE(AdvanceTBMWork(id) == TBMProjectError::None);
	CHECK(GetTBMProject(id)->phase == TBMPhase::TravelToFace);
	CHECK(GetTBMProject(id)->phase_ticks == 3);
	CHECK(AdvanceTBMWork(TBMProjectID{99}) == TBMProjectError::InvalidProject);
	rig.front->vehstatus.Set(VehState::Stopped);
}
