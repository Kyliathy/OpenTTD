/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_parking.cpp Physical resumable TBM withdrawal and actual spoil settlement. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group7; docs/zz-reach2/architecture/vehicles/archi-vehicles.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include "underground_depot_fixture.h"
#include "../tbm_area_preview.h"
#include "../tbm_cmd.h"
#include "../tbm_controls.h"
#include "../vehicle_cmd.h"
#include "../order_cmd.h"

#include "../safeguards.h"

/** @param front Actual native head. @return Every attached part's identity and position, for proving a blocked return cannot move a hidden tail. */
static auto ParkedPartPositions(const Train &front)
{
	std::vector<std::tuple<VehicleID, TileIndex, int, int, int>> result;
	for (const Train *part = &front; part != nullptr; part = part->Next()) result.emplace_back(part->index, part->tile, part->x_pos, part->y_pos, part->z_pos);
	return result;
}

/** A real machine leaves its face, settles an incomplete pass and resumes the same partial cut after actual parking. */
TEST_CASE("TBM physically parks and resumes retained work with whole cargo", "[tbm-parking]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const bool loaded = GENERATE(false, true);
	UndergroundWorkflowWorld world;
	const auto direction = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(world.rock));
	const TileIndex end = TileAddWrap(world.rock, direction.x * 3, direction.y * 3);
	const auto quote = PreviewTBMCommissionArea(_current_company, world.portal, 0, 2, 2, world.origin->index,
		world.rock, end, false, ExcavationPlanIntent::StraightTunnel);
	REQUIRE(quote.purchase.Succeeded());
	REQUIRE(Command<Commands::ConfirmTBMCommission>::Do(DoCommandFlag::Execute, world.portal, uint16_t{0}, uint8_t{2}, uint8_t{2}, world.origin->index,
		world.rock, end, quote.fingerprint, ExcavationPlanIntent::StraightTunnel).Succeeded());
	const TBMProjectID id = CaptureTBMProjects().front().id;
	Train *front = Train::Get(GetTBMProject(id)->cutter);
	const ExcavationJobID job_id = GetTBMProject(id)->job;
	const uint32_t wanted_cursor = loaded ? 1 : 0;
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, front->index, true).Succeeded());
	// The complete real locomotive/parts reach the face under native movement;
	// request parking during unfinished work, optionally after one paid cut.
	for (uint ticks = 0; ticks < 8000; ++ticks) {
		const auto *job = GetExcavationJob(job_id);
		if (GetTBMProject(id)->phase == TBMPhase::Bore && job->cursor == wanted_cursor && job->partial_work_ticks >= 20) break;
		world.Tick();
	}
	const auto before_job = *GetExcavationJob(job_id);
	const auto before_project = *GetTBMProject(id);
	REQUIRE(before_project.phase == TBMPhase::Bore);
	REQUIRE(before_job.cursor == wanted_cursor);
	REQUIRE(before_job.partial_work_ticks >= 20);
	REQUIRE(before_job.cursor < before_job.route.size());
	REQUIRE(Command<Commands::ControlTBMProject>::Do({}, id, TBMProjectAction::ReturnAndPark).Succeeded());
	CHECK(*GetTBMProject(id) == before_project);
	CHECK(*GetExcavationJob(job_id) == before_job);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::ReturnAndPark).Succeeded());
	CHECK(GetTBMProject(id)->phase == TBMPhase::ReturnToPark);
	CHECK(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::ReturnAndPark).Failed());
	CHECK(IsTBMExchangeDue(*GetTBMProject(id)) == loaded); // A partial loaded pass now needs real exchange, independent of its frozen breakpoint.
	const auto positions = ParkedPartPositions(*front);
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, front->index, true).Succeeded());
	for (uint ticks = 0; ticks < 60; ++ticks) world.Tick();
	CHECK(ParkedPartPositions(*front) == positions);
	CHECK(*GetExcavationJob(job_id) == before_job);
	CHECK(GetTBMProject(id)->blocked != TBMBlockedReason::None);
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, front->index, true).Succeeded());
	const auto waiting_phase = loaded ? TBMPhase::WaitToPark : TBMPhase::Parked;
	for (uint ticks = 0; ticks < 8000 && GetTBMProject(id)->phase != waiting_phase; ++ticks) world.Tick(); // Restart the same real train and let its backed return recover.
	INFO("loaded=" << loaded << " blocked=" << to_underlying(GetTBMProject(id)->blocked) << " speed=" << front->cur_speed << " tile=" << front->tile.base() << " bay=" << world.portal.base() << " track=" << front->track.base() << " hold=" << to_underlying(GetTBMTravelHoldReason(*front)));
	REQUIRE(GetTBMProject(id)->phase == waiting_phase);
	CHECK(*GetExcavationJob(job_id) == before_job);
	if (loaded) {
		// No assigned hauler/empties means the loaded TBM remains visibly at
		// exchange. It cannot become parked by deleting or hiding its packets.
		for (uint ticks = 0; ticks < 120; ++ticks) world.Tick();
		CHECK(GetTBMProject(id)->phase == TBMPhase::WaitToPark);
		CHECK(IsTBMExchangeDue(*GetTBMProject(id)));
	}
	Order origin_order;
	origin_order.MakeGoToStation(world.origin->index);
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, world.hauler->index, 0, origin_order).Succeeded());
	REQUIRE(AssignTBMHauler(id, world.hauler->index, true) == TBMExchangeError::None);
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, world.hauler->index, true).Succeeded());
	if (loaded) {
		/** @return Whether the hauler has physically entered the magazine while its tail still occupies the approach. */
		const auto entering = [&]() {
			bool inside = false, outside = false;
			for (const Train *part = world.hauler; part != nullptr; part = part->Next()) {
				inside |= part->tile == world.portal && part->track == Track::Depot;
				outside |= part->tile != world.portal || part->track != Track::Depot;
			}
			return inside && outside;
		};
		for (uint ticks = 0; ticks < 10000 && !entering(); ++ticks) world.Tick(); // Only actual train movement may establish this occupied arrival boundary.
		REQUIRE(entering());
		const auto empty_exchanges = GetTBMExchangeAssignmentForHauler(world.hauler->index)->exchanges;
		for (uint ticks = 0; ticks < 500 && entering(); ++ticks) {
			// A real partly occupied approach refuses exchange throughout native
			// arrival. Braking can finish that short arrival, so no artificial
			// stationary half-train is required to demonstrate the guard.
			CHECK(CheckTBMExchangeParticipant(*world.hauler, GetTBMExchangeBay(*GetTBMProject(id)), _current_company) == TBMExchangeError::NotAtBay);
			CHECK(GetTBMProject(id)->phase == TBMPhase::WaitToPark);
			CHECK(GetTBMExchangeAssignmentForHauler(world.hauler->index)->exchanges == empty_exchanges);
			CHECK(IsTBMExchangeDue(*GetTBMProject(id)));
			world.Tick();
		}
		REQUIRE_FALSE(entering());
	}
	for (uint ticks = 0; ticks < 10000 && GetTBMProject(id)->phase != TBMPhase::Parked; ++ticks) world.Tick(); // Actual wagons exchange only after the hauler reaches the original bay.
	REQUIRE(GetTBMProject(id)->phase == TBMPhase::Parked);
	CHECK(front->vehstatus.Test(VehState::Stopped));
	CHECK(*GetExcavationJob(job_id) == before_job);
	CHECK(GetTBMProject(id)->work_remainder == before_project.work_remainder);
	for (const Train *part = front; part != nullptr; part = part->Next()) {
		CHECK(part->tile == world.portal);
		CHECK(part->track == Track::Depot);
		CHECK(part->cargo.TotalCount() == 0);
	}
	const auto parked_positions = ParkedPartPositions(*front);
	for (uint ticks = 0; ticks < 180; ++ticks) world.Tick(); // Parked work remains stationary and retains its exact geological partial cut.
	CHECK(ParkedPartPositions(*front) == parked_positions);
	CHECK(*GetExcavationJob(job_id) == before_job);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Resume).Succeeded());
	CHECK_FALSE(front->vehstatus.Test(VehState::Stopped));
	// Resume must drive the same real engine back to its accepted work and
	// finish the retained partial cut, rather than replacing the job or cutter.
	for (uint ticks = 0; ticks < 12000 && GetExcavationJob(job_id)->cursor == before_job.cursor; ++ticks) world.Tick();
	CHECK(GetTBMProject(id)->cutter == front->index);
	CHECK(GetTBMProject(id)->job == job_id);
	CHECK(GetExcavationJob(job_id)->cursor == before_job.cursor + 1);
	fmt::print("PARKING loaded={} cursor_before={} cursor_after={} native_ticks={}\n", loaded, before_job.cursor, GetExcavationJob(job_id)->cursor, world.ticks);
	// A lost real destination may invalidate its commissioning binding. That
	// case must hold safely; restoring one owner byte cannot recreate a portal.
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::ReturnAndPark).Succeeded());
	const auto lost_positions = ParkedPartPositions(*front);
	const auto lost_job = *GetExcavationJob(job_id);
	SetTileOwner(world.portal, OWNER_NONE);
	for (uint ticks = 0; ticks < 60; ++ticks) world.Tick();
	CHECK(ParkedPartPositions(*front) == lost_positions);
	CHECK(*GetExcavationJob(job_id) == lost_job);
	CHECK(GetTBMProject(id)->blocked != TBMBlockedReason::None);
}

/** An excavated paid depot remains a physical parking destination for the same reused cutter and retained partial cut. */
TEST_CASE("TBM parks at its actual underground exchange depot and resumes", "[tbm-parking-depot]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const TBMProjectID id = workflow.StartFurther();
	/** @return Whether a real paid loaded cut has been followed by unfinished boring at the next face. */
	const auto partial = [&]() {
		const auto *project = GetTBMProject(id);
		const auto *job = GetExcavationJob(project->job);
		return project->phase == TBMPhase::Bore && job->completed_earth_t != 0 && job->partial_work_ticks >= 20;
	};
	workflow.DriveUntil(partial);
	const auto before = *GetExcavationJob(GetTBMProject(id)->job);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::ReturnAndPark).Succeeded());
	/** @return Whether real native return and whole-wagon settlement have reached parking. */
	const auto parked = [&]() { return GetTBMProject(id)->phase == TBMPhase::Parked; };
	workflow.DriveUntil(parked);
	CHECK(*GetExcavationJob(before.id) == before);
	for (const Train *part = Train::Get(workflow.first.cutter); part != nullptr; part = part->Next()) {
		CHECK(part->tile == workflow.depot);
		CHECK(part->track == Track::Depot);
		CHECK(part->cargo.TotalCount() == 0);
	}
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Resume).Succeeded());
	/** @return Whether the resumed actual machine completed its retained partial cut. */
	const auto resumed = [&]() { return GetExcavationJob(before.id)->cursor > before.cursor; };
	workflow.DriveUntil(resumed);
	CHECK(GetTBMProject(id)->cutter == workflow.first.cutter);
	fmt::print("PARKING_DEPOT cursor_before={} cursor_after={} native_ticks={}\n", before.cursor, GetExcavationJob(before.id)->cursor, workflow.world.ticks);
}
