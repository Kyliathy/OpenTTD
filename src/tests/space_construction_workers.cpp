/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_construction_workers.cpp Genuine competing jobs and safe target retirement.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group7.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "space_construction_trace.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../space_approach.h"
#include "../orbital_station.h"
#include "../orbital_service.h"
#include "../asteroid.h"
#include "../launch_center_cmd.h"
#include "../vehicle_cmd.h"
#include "../command_func.h"
#include "../company_func.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../safeguards.h"

/** @copydoc RunSpaceConstructionWorkers */
void RunSpaceConstructionWorkers()
{
	std::vector<SpaceVehicle *> builders;
	std::vector<SpaceStructure *> sites;
	SpaceVehicle *rocket = nullptr;
	// Retain native stable pool order for both real builders; the production
	// arbitration must pick the lower eligible ID regardless of request timing.
	for (auto *craft : SpaceVehicle::Iterate()) {
		if (craft->owner != _local_company) continue;
		if (GetSpaceVehicleModel(*craft) == 14) rocket = craft;
		if (GetSpaceVehicleModel(*craft) == 20) builders.push_back(craft);
	}
	for (auto *site : SpaceStructure::Iterate()) {
		// Both paid empty jobs must be independently approached and supplied. Two
		// hulls at one exclusive berth would test approach contention instead.
		if (site->state.owner == _local_company && site->state.kind == SpaceBuildKind::Storage && site->state.phase == SpaceStructurePhase::Blueprint) sites.push_back(site);
	}
	REQUIRE(builders.size() == 2);
	REQUIRE(sites.size() == 2);
	REQUIRE(rocket != nullptr);
	auto &first = *builders[0];
	auto &second = *builders[1];
	auto &job = *sites[0];
	auto &other_job = *sites[1];
	auto *core = GetOrbitalConstructionCore(job.state);
	REQUIRE(core != nullptr);
	REQUIRE(GetOrbitalConstructionCore(other_job.state) == core);
	REQUIRE_FALSE(HasOrbitalServiceAssignment(second));
	MoveOtherConstructionCraftAway(first);
	OrderConstructionCraft(first, *core, SpaceOrderAction::Visit);
	/** @return Whether the original lower-ID builder has reached the Core's real berth. */
	const auto at_core = [&] { return GetSpaceStructureBerth(first, *core).has_value(); };
	AwaitConstructionBoundary(first, job, at_core, "worker-import-arrival");
	REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core->index, core->generation, true));
	REQUIRE(Command<Commands::ReserveOrbitalModuleInputs>::Post(job.index, job.generation, core->index, core->generation));
	const auto home = rocket->construction.home;
	// Import the second recipe through the actual paid rocket, launch stock and
	// fuel, then unload the real builder hold. No copied packet or fixture grant
	// may make the second builder eligible for the shared tooling slot.
	for (const auto label : {CargoLabel{"STEL"}, CargoLabel{"GOOD"}}) {
		const auto cargo = GetCargoTypeByLabel(label);
		const auto flights = rocket->transition->missions_completed;
		REQUIRE(Command<Commands::MoveLaunchFreight>::Post(home.station, home.generation, rocket->index, rocket->generation, cargo, uint16_t{8}, false));
		REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, uint8_t{0}, uint16_t{15}, false));
		REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, uint32_t{first.index.base()}, first.generation));
		AdvanceConstructionTrace(667, first, job);
		REQUIRE(rocket->transition->missions_completed == flights + 1);
		REQUIRE(first.cargo_type == cargo);
		REQUIRE(first.cargo.TotalCount() == 8);
		REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(core->index, core->generation, first.index, first.generation, cargo, uint16_t{8}, false));
	}
	REQUIRE(Command<Commands::ReserveOrbitalModuleInputs>::Post(other_job.index, other_job.generation, core->index, core->generation));
	OrderConstructionCraft(second, other_job, SpaceOrderAction::Construct);
	OrderConstructionCraft(first, job, SpaceOrderAction::Construct);
	/** @return Whether both independent paid jobs have a physically admitted builder. */
	const auto both_arrived = [&] { return GetSpaceStructureBerth(first, job).has_value() && GetSpaceStructureBerth(second, other_job).has_value(); };
	AwaitConstructionBoundary(first, job, both_arrived, "two-workers-arrived");
	CHECK(job.job.work_ticks == 0);
	CHECK(other_job.job.work_ticks == 0);
	CheckConstructionReadiness(first, OrbitalConstructionBlock::PowerUnavailable);
	CheckConstructionReadiness(second, OrbitalConstructionBlock::BuilderBusy);
	REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core->index, core->generation, false));
	const auto spent = GetUtilityPowerProducer(core->module->producer)->consumed_lifetime_j;
	AdvanceConstructionTrace(1, first, job);
	CHECK(job.job.work_ticks == 1);
	CHECK(other_job.job.work_ticks == 0);
	CHECK(GetUtilityPowerProducer(core->module->producer)->consumed_lifetime_j == spent + 750);
	CheckConstructionReadiness(first, OrbitalConstructionBlock::Working);
	CheckConstructionReadiness(second, OrbitalConstructionBlock::BuilderBusy);
	CHECK_FALSE(GetSpaceConstructionVisualWork(second));
	const auto paused_fleet = ComputeSpaceVehicleDigest(), paused_jobs = ComputeSpaceStructureDigest();
	REQUIRE(_pause_mode.Any());
	StateGameLoop();
	CHECK(ComputeSpaceVehicleDigest() == paused_fleet);
	CHECK(ComputeSpaceStructureDigest() == paused_jobs);
	CheckConstructionReadiness(first, OrbitalConstructionBlock::Paused, false);
	REQUIRE(Command<Commands::StartStopVehicle>::Post(first.tile, first.index, true));
	AdvanceConstructionTrace(5, second, other_job);
	CHECK(job.job.work_ticks == 1);
	CHECK(other_job.job.work_ticks == 5);
	CheckConstructionReadiness(first, OrbitalConstructionBlock::Stopped);
	CheckConstructionReadiness(second, OrbitalConstructionBlock::Working);
	REQUIRE(Command<Commands::StartStopVehicle>::Post(first.tile, first.index, true));
	AdvanceConstructionTrace(1, first, job);
	CHECK(job.job.work_ticks == 2);
	CHECK(other_job.job.work_ticks == 5);
	CheckConstructionReadiness(first, OrbitalConstructionBlock::Working);
	CheckConstructionReadiness(second, OrbitalConstructionBlock::BuilderBusy);
	// Travel away while retaining a later exact Construct instruction. At the
	// Core arrival boundary that instruction becomes current before it can move
	// back, permitting real evacuation/removal with no occupant to teleport.
	OrderConstructionCraft(first, *core, SpaceOrderAction::Visit);
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(first.index, first.generation, SpaceOrderEdit::Insert, uint8_t{1},
		SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, uint32_t{job.index.base()}, job.generation));
	AwaitConstructionBoundary(first, job, at_core, "worker-retirement-arrival");
	const auto removed = GetSpaceStructureReference(job);
	const auto retained_work = job.job.work_ticks;
	REQUIRE(Command<Commands::EvacuateOrbitalModule>::Post(job.index, job.generation, core->index, core->generation));
	CHECK(job.job.work_ticks == retained_work);
	REQUIRE(Command<Commands::RemoveOrbitalModule>::Post(removed.structure, removed.generation));
	CHECK(ResolveSpaceStructure(removed) == nullptr);
	CheckConstructionReadiness(first, OrbitalConstructionBlock::TargetRemoved);
	const auto other_work = other_job.job.work_ticks;
	AdvanceConstructionTrace(5, second, other_job);
	CHECK(other_job.job.work_ticks == other_work + 5);
	CheckConstructionReadiness(first, OrbitalConstructionBlock::TargetRemoved);
	CheckConstructionReadiness(second, OrbitalConstructionBlock::Working);
	CHECK_FALSE(GetSpaceConstructionVisualWork(first));
	CHECK(ValidateSpaceVehicles());
	fmt::print("CONSTRUCTION_WORKERS paid_imports=2 distinct_berths=true stable_lowest_id=true one_750j_step=true pause_stop_resume=true real_target_removed=true other_job_continues=true\n");
}
