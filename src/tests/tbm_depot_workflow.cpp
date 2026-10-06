/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_depot_workflow.cpp Paid native clearing and real recurring underground-depot logistics.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 7BB
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_depot_fixture.h"
#include "../tbm_area_preview.h"

#include "../safeguards.h"

/** Native trains must execute a real underground-depot exchange before this consumer can claim any recurring loop. */
TEST_CASE("Recommissioned trains physically exchange at the actual underground depot", "[tbm-depot-cycle]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const TBMProjectID project = workflow.StartFurther();
	/** @return Whether the native owner has executed the first actual depot exchange. */
	const auto exchanged = [&]() {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index);
		REQUIRE(assignment != nullptr);
		return assignment->exchanges != 0;
	};
	workflow.DriveUntil(exchanged);
	CHECK(GetTBMProject(project)->exchange_bay == workflow.depot);
	CHECK(GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index)->phase == TBMHaulerPhase::CollectLoaded);
	/** @return Whether the exchanged real hauler has safely left the depot toward the mandatory Surface origin. */
	const auto departed = [&]() { return GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index)->phase == TBMHaulerPhase::VisitOrigin; };
	workflow.DriveUntil(departed);
	/** @return Whether actual disposal and physical empty return permit a second real depot exchange. */
	const auto repeated = [&]() { return GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index)->exchanges >= 2; };
	workflow.DriveUntil(repeated);
	/** @return Whether the full further job and its final mandatory Surface disposal have completed. */
	const auto delivered = [&]() {
		return IsTBMProjectFinished(GetTBMProject(project)->phase) &&
			GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index)->phase == TBMHaulerPhase::ReturnEmpties;
	};
	workflow.DriveUntil(delivered);
	const auto *job = GetExcavationJob(GetTBMProject(project)->job);
	REQUIRE(job != nullptr);
	CHECK(job->cursor == job->route.size());
	const auto *earth = GetSpoilReceiver(workflow.world.earth->index);
	const auto *rock = GetSpoilReceiver(workflow.world.stone->index);
	CHECK(earth->disposed_t + workflow.world.earth->accepted.front().waiting == workflow.first_earth + job->completed_earth_t);
	CHECK(rock->disposed_t + rock->crushed_t + workflow.world.stone->accepted.front().waiting == workflow.first_rock + job->completed_rock_t);
	// Both charged jobs must account for every original physical spoil packet; the final loop cannot leave hidden loaded capacity behind.
	for (const Vehicle *vehicle : Vehicle::Iterate()) CHECK(vehicle->cargo.TotalCount() == 0);
	fmt::print("DEPOT_COMPLETE first_earth={} first_rock={} further_earth={} further_rock={} work={} exchanges={} command_cost={} frames={}\n",
		workflow.first_earth, workflow.first_rock, job->completed_earth_t, job->completed_rock_t, job->completed_work_cost.base(),
		GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index)->exchanges, workflow.commands.base(), workflow.world.ticks);
}

/** Depot and independent ordinary haulage access are funded only after actual wide clearing and complete first-spoil disposal. */
TEST_CASE("Paid clearing prepares an actual underground exchange depot and separate connection", "[tbm-depot-foundation]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const auto *job = GetExcavationJob(workflow.first.job);
	REQUIRE(job != nullptr);
	CHECK(job->completed_work_cost == workflow.first_work_cost);
	CHECK(job->completed_earth_t == workflow.first_earth);
	CHECK(job->completed_rock_t == workflow.first_rock);
	CHECK(workflow.first_project_cost > 0);
	CHECK(workflow.world.setup_cost > 0);
	CHECK(IsRailDepotTile(workflow.depot));
	CHECK(GetTileOwner(workflow.depot) == _current_company);
	// The real withdrawn original consist supplies the approach length; this read-only resolution publishes no substitute project.
	const auto bay = ResolveTBMExchangeBay(workflow.first, workflow.depot);
	REQUIRE(bay.error == TBMExchangeError::None);
	CHECK(bay.kind == TBMExchangeBayKind::Depot);
	CHECK(bay.approach.size() == CeilDiv(Train::Get(workflow.first.cutter)->gcache.cached_total_length, TILE_SIZE));
	fmt::print("DEPOT_FOUNDATION setup={} first_project={} first_work={} earth={} rock={} command_net={} depot={} connection={}\n",
		workflow.world.setup_cost.base(), workflow.first_project_cost.base(), workflow.first_work_cost.base(), workflow.first_earth,
		workflow.first_rock, workflow.commands.base(), workflow.depot.base(), workflow.connection.base());
}

/** A retired real machine takes a further paid job at the same source without losing parts or paying another purchase. */
TEST_CASE("A withdrawn native machine recommissions unchanged for an actual depot job", "[tbm-depot-recommission]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const auto consist = GetDefaultTBMConsist(MapLayer::Underground);
	const TileIndex end = TileAddWrap(workflow.world.site, 2, 18);
	const Money money = Company::Get(_current_company)->money;
	const auto vehicles = Vehicle::GetNumItems();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto projects = ComputeTBMProjectDigest();
	std::vector<VehicleID> chain;
	// Freeze the actual whole original chain before quoting; recommissioning must preserve every machinery and hopper identity.
	for (const Train *part = Train::Get(workflow.first.cutter); part != nullptr; part = part->Next()) chain.push_back(part->index);
	const auto quote = PreviewTBMCommissionArea(_current_company, workflow.world.portal, workflow.first.model_id,
		consist.earth_wagons, consist.rock_wagons, workflow.world.origin->index, workflow.world.site, end, false);
	INFO("recommission quote error=" << quote.purchase.GetErrorMessage().base() << " area=" << uint{to_underlying(quote.area.error)}
		<< " plan=" << uint{to_underlying(quote.area.plan.error)} << " failure=" << quote.area.failure_tile.base());
	REQUIRE(quote.purchase.Succeeded());
	CHECK(quote.request.reuse_cutter == workflow.first.cutter);
	CHECK(quote.purchase.GetCost() == 0);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(Vehicle::GetNumItems() == vehicles);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeTBMProjectDigest() == projects);
	const auto plan = PrepareTBMAcquisition(quote.request, {consist.earth_wagons, consist.rock_wagons});
	REQUIRE(plan.error == TBMAcquisitionError::None);
	// The occupancy exemption is tied to this healthy native machine, not its company's ordinary hauler or a different requested length.
	auto wrong = quote.request;
	wrong.reuse_cutter = workflow.world.hauler->index;
	CHECK(ValidateLayerPortalCommissioning(wrong).error == LayerPortalWorkError::InvalidReference);
	CHECK(PreviewTBMCommissionArea(_current_company, workflow.world.portal, workflow.first.model_id, consist.earth_wagons + 1,
		consist.rock_wagons, workflow.world.origin->index, workflow.world.site, end, false).purchase.Failed());
	Vehicle *none = nullptr;
	CHECK(BuildRailVehicleForTBM(DoCommandFlags{}, quote.request, Engine::Get(plan.cutter_engine), &none).Failed());
	CHECK(none == nullptr);
	{
		TBMAcquisition held;
		REQUIRE(AcquireTBMConsist(quote.request, plan, held) == TBMAcquisitionError::None);
		CHECK(held.GetFront()->index == workflow.first.cutter);
	} // Failed/unpublished reassignment must release only the prospective owner; actual paid parts remain untouched.
	CHECK(Vehicle::GetNumItems() == vehicles);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeTBMProjectDigest() == projects);
	workflow.Do<Commands::CommissionTBM>(workflow.world.portal, workflow.first.model_id, consist.earth_wagons, consist.rock_wagons,
		workflow.world.origin->index, workflow.world.site, end, false);
	const auto assigned = CaptureTBMProjects();
	REQUIRE(assigned.size() == 1);
	CHECK(assigned.front().cutter == workflow.first.cutter);
	CHECK(assigned.front().job != workflow.first.job);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK(Vehicle::GetNumItems() == vehicles);
	std::vector<VehicleID> retained;
	// Reusing the command preserves actual wagon IDs and empty packets; no reconstruction or replacement train may impersonate assignment.
	for (const Train *part = Train::Get(assigned.front().cutter); part != nullptr; part = part->Next()) {
		retained.push_back(part->index);
		CHECK(part->cargo.TotalCount() == 0);
	}
	CHECK(retained == chain);
	workflow.Do<Commands::SetTBMExchangeBay>(assigned.front().id, workflow.depot);
	workflow.Do<Commands::AssignTBMHauler>(assigned.front().id, workflow.world.hauler->index);
	CHECK(GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index)->bay == workflow.depot);
	CHECK(Command<Commands::CommissionTBM>::Do(DoCommandFlag::Execute, workflow.world.portal, workflow.first.model_id,
		consist.earth_wagons, consist.rock_wagons, workflow.world.origin->index, workflow.world.site, end, false).Failed());
}
