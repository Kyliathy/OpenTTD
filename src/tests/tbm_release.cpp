/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_release.cpp Public railway use beside the same retired native machine. @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group7B; docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_depot_fixture.h"
#include "depot_file_fixture.h"
#include "../order_cmd.h"
#include "../landscape_cmd.h"
#include "../signal_func.h"
#include "../core/random_func.hpp"

#include "../safeguards.h"

/** Final cancellation and parked-job release must settle all real cargo before any future geological claim is discarded. */
TEST_CASE("Final TBM release preserves claims until withdrawal and actual haulage settlement", "[tbm-release-settlement]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const bool cancel = GENERATE(false, true);
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const auto id = workflow.StartFurther();
	/** @return Whether actual native boring has produced spoil and begun another partial cut. */
	const auto partial = [&]() {
		const auto *project = GetTBMProject(id);
		const auto *job = GetExcavationJob(project->job);
		return project->phase == TBMPhase::Bore && job->completed_earth_t != 0 && job->partial_work_ticks >= 20;
	};
	workflow.DriveUntil(partial);
	const auto job = *GetExcavationJob(GetTBMProject(id)->job);
	const auto claims = GetGeologyWorldState().job_claims;
	const auto action = cancel ? TBMProjectAction::Cancel : TBMProjectAction::ReturnAndPark;
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, action).Succeeded());
	CHECK(*GetExcavationJob(job.id) == job);
	CHECK(GetGeologyWorldState().job_claims == claims);
	if (cancel) DepotFileRecorder{}.Save("cancel-return"); // Actual pending cancellation retains its work and claims in a native396 file.
	CHECK(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Retire).Failed());
	/** @return Whether the complete real cutter has withdrawn but still awaits final loaded haulage. */
	const auto withdrawn = [&]() { return IsTBMStored(GetTBMProject(id)->phase); };
	workflow.DriveUntil(withdrawn);
	REQUIRE(GetTBMProjectSettlementBlocker(id).has_value());
	CHECK(GetTBMProjectReleaseProblem(id) == STR_ERROR_TBM_RELEASE_HAULAGE);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), exchange = ComputeTBMExchangeDigest(), controller = ComputeTBMProjectDigest();
	const auto money = Company::Get(_current_company)->money;
	const auto rng0 = _random.state[0], rng1 = _random.state[1];
	CHECK(Command<Commands::ControlTBMProject>::Do({}, id, TBMProjectAction::Retire).Failed());
	CHECK(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Retire).Failed());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeTBMExchangeDigest() == exchange);
	CHECK(ComputeTBMProjectDigest() == controller);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(_random.state[0] == rng0);
	CHECK(_random.state[1] == rng1);
	CHECK(GetGeologyWorldState().job_claims == claims);
	/** @return Whether the actual delivered hauler and all train parts have cleared every release prerequisite. */
	const auto settled = [&]() { return GetTBMProjectReleaseProblem(id) == INVALID_STRING_ID; };
	workflow.DriveUntil(settled);
	const auto *settled_job = GetExcavationJob(job.id);
	CHECK(settled_job->completed_earth_t == job.completed_earth_t);
	CHECK(settled_job->completed_rock_t == job.completed_rock_t);
	CHECK(settled_job->cursor == job.cursor);
	if (!cancel) CHECK(*settled_job == job); // Resumable parking never cancels geological work on its own.
	const auto stored = Vehicle::GetNumItems();
	const auto accepted = ComputeGeologyStateDigest(GetGeologyWorldState());
	REQUIRE(Command<Commands::ControlTBMProject>::Do({}, id, TBMProjectAction::Retire).Succeeded());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == accepted);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Retire).Succeeded());
	CHECK(GetTBMProject(id) == nullptr);
	CHECK(GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index) == nullptr);
	CHECK(GetExcavationJob(job.id)->state == ExcavationJobState::Cancelled);
	CHECK(GetExcavationJob(job.id)->partial_work_ticks == 0);
	CHECK(Vehicle::GetNumItems() == stored);
	CHECK(Train::Get(workflow.first.cutter)->IsStoppedInDepot());
	CHECK_FALSE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Retire).Succeeded());
	for (const auto &support : CaptureTBMSupport()) workflow.Do<Commands::ConvertTBMSupport>(support.tile); // Safe final release still pays the ordinary support-conversion invoice.
	fmt::print("RELEASE_SETTLED cancel={} retained_cutter={} partial_before={} native_ticks={}\n", cancel, workflow.first.cutter.base(), job.partial_work_ticks, workflow.world.ticks);
}

/** A released real machine stays stored while ordinary native freight crosses its source portal in both directions. */
TEST_CASE("Released track carries ordinary freight past the retained TBM magazine", "[tbm-release-traffic]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare(); // Actual paid excavation, all spoil delivered, complete withdrawal and support conversion.
	Train *cutter = Train::Get(workflow.first.cutter);
	const auto vehicles = Vehicle::GetNumItems();
	std::vector<std::tuple<VehicleID, TileIndex, TrackBits, int, int>> stored;
	for (const Train *part = cutter; part != nullptr; part = part->Next()) stored.emplace_back(part->index, part->tile, part->track, part->x_pos, part->y_pos);
	REQUIRE(GetTBMProjectForCutter(cutter->index) == nullptr);
	REQUIRE(cutter->vehstatus.Test(VehState::Stopped));
	const uint x = TileX(workflow.world.rock), y = TilePlaneY(workflow.world.rock);
	std::vector<TileIndex> path{workflow.world.rock};
	for (uint row = y + 1; row <= y + 7; ++row) path.push_back(TileXYL(x, row, MapLayer::Underground)); // Ordinary rail only on the first job's actually paid floor.
	path.push_back(TileXYL(x + 1, y + 7, MapLayer::Underground));
	for (uint row = y + 7; row >= y + 4; --row) path.push_back(TileXYL(x + 2, row, MapLayer::Underground));
	path.push_back(workflow.depot);
	workflow.BuildRailPath(path);
	REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do({}, workflow.world.portal, true).Succeeded());
	CHECK(GetLayerPortalWorkState(workflow.world.portal) == LayerPortalWorkState::Sealed);
	workflow.Do<Commands::ChangeLayerPortalConnection>(workflow.world.portal, true);
	workflow.Do<Commands::ChangeLayerPortalConnection>(workflow.connection, false); // Make the retained machine's actual source the sole public route for both physical journeys.
	UpdateSignalsInBuffer(); // Finish the ordinary construction command batch before the first native vehicle step.
	CHECK(GetTBMPairControls(workflow.world.portal, _current_company).step == TBMPlayerStep::Connected);
	CHECK(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, workflow.world.portal).Failed()); // Connection admission never weakens demolition of the real stored machine.
	Order depot_order;
	depot_order.MakeGoToDepot(Depot::GetByTile(workflow.depot)->index, {OrderDepotTypeFlag::PartOfOrders},
		OrderNonStopFlag::NonStop, {OrderDepotActionFlag::Halt});
	Train *freight = workflow.world.hauler;
	REQUIRE(GetTBMExchangeAssignmentForHauler(freight->index) == nullptr);
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, freight->index, 0, depot_order).Succeeded());
	if (freight->cur_real_order_index != 0) REQUIRE(CmdSkipToOrder(DoCommandFlag::Execute, freight->index, 0).Succeeded());
	workflow.Do<Commands::StartStopVehicle>(freight->index, true);
	std::set<VehicleID> entered, returned;
	bool returning = false;
	/** Record every real freight part crossing public rail beside the stationary complete cutter chain. */
	const auto observe = [&]() {
		CHECK_FALSE(freight->vehstatus.Test(VehState::Crashed));
		std::vector<std::tuple<VehicleID, TileIndex, TrackBits, int, int>> current;
		for (const Train *part = cutter; part != nullptr; part = part->Next()) current.emplace_back(part->index, part->tile, part->track, part->x_pos, part->y_pos);
		REQUIRE(current == stored);
		for (const Train *part = freight; part != nullptr; part = part->Next()) {
			if (part->tile == workflow.world.portal) {
				CHECK(part->track != Track::Depot);
				(returning ? returned : entered).insert(part->index);
			}
		}
	};
	/** @param done Actual destination condition. Advance native freight after retirement, without requiring a surviving work controller. */
	const auto drive = [&](const std::function<bool()> &done) {
		for (uint ticks = 0; ticks < 16000 && !done(); ++ticks) {
			workflow.world.Tick();
			observe();
		}
		INFO(DescribeTrain(freight));
		REQUIRE(done());
	};
	/** @return Whether the ordinary freight has physically reached its real underground depot order. */
	const auto at_depot = [&]() { return freight->tile == workflow.depot && freight->IsStoppedInDepot(); };
	drive(at_depot);
	uint parts = 0;
	for (const Train *part = freight; part != nullptr; part = part->Next()) ++parts;
	CHECK(entered.size() == parts);
	returning = true;
	if (freight->cur_real_order_index != 1) REQUIRE(CmdSkipToOrder(DoCommandFlag::Execute, freight->index, 1).Succeeded());
	workflow.Do<Commands::StartStopVehicle>(freight->index, true);
	/** @return Whether the ordinary station order has returned the whole freight through the same source portal. */
	const auto at_origin = [&]() { return returned.size() == parts && freight->tile == workflow.world.origin->train_station.tile; };
	drive(at_origin);
	CHECK(Vehicle::GetNumItems() == vehicles);
	CHECK(Train::Get(workflow.first.cutter) == cutter);
	fmt::print("RELEASE_TRAFFIC parts={} entered={} returned={} retained_cutter={} native_ticks={}\n", parts, entered.size(), returned.size(), cutter->index.base(), workflow.world.ticks);
}
