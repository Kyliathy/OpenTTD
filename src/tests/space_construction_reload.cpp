/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_construction_reload.cpp Actual construction checkpoint/control comparisons.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group7.
 */
#include "../stdafx.h"
#include <filesystem>
#include "../3rdparty/catch2/catch.hpp"
#include "space_construction_trace.h"
#include "layer_save_fixture.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_approach.h"
#include "../orbital_station.h"
#include "../orbital_service.h"
#include "../asteroid.h"
#include "../command_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../timer/timer_game_tick.h"
#include "../saveload/saveload_func.h"
#include "../openttd.h"
#include <cstdlib>
#include "../safeguards.h"

/**
 * Emit canonical saved-owner digests and explicit job/accounting fields for a separate-process comparison.
 * @param phase Checkpoint or final continuation boundary.
 * @param builder Actual retained hull; its packets, upkeep and queue remain native.
 * @param site Actual job, including its exact reusable-pool incarnation.
 */
static void ReportConstructionReplay(std::string_view phase, const SpaceVehicle &builder, const SpaceStructure &site)
{
	const auto *core = GetOrbitalConstructionCore(site.state);
	REQUIRE(core != nullptr);
	fmt::print("CONSTRUCTION_REPLAY phase={} fleet={} structures={} power={} field={} manifests={} tick={} elapsed={} random0={} random1={} cash={} builder={}/{} site={}/{} stopped={} work={} fraction={} remainder={} last_tick={} interval={} input0={} input1={} consumed0={} consumed1={} cursor={} profit={} paid_j={}\n",
		phase, ComputeSpaceVehicleDigest(), ComputeSpaceStructureDigest(), ComputeUtilityPowerDigest(), ComputeAsteroidFieldDigest(GetAsteroidField()),
		ComputeSpaceManifestDigest(), TimerGameTick::counter, GetAsteroidField().elapsed_ticks, _random.state[0], _random.state[1],
		static_cast<int64_t>(Company::Get(builder.owner)->money), builder.index.base(), builder.generation, site.index.base(), site.generation,
		builder.vehstatus.Test(VehState::Stopped), site.job.work_ticks, site.job.work_fraction, site.job.work_remainder,
		site.job.last_work_tick, site.job.last_work_interval, site.job.inputs[0].TotalCount(), site.job.inputs[1].TotalCount(),
		site.job.consumed[0], site.job.consumed[1], builder.space_orders.cursor, static_cast<int64_t>(builder.profit_this_year),
		GetUtilityPowerProducer(core->module->producer)->consumed_lifetime_j);
}

/**
 * Write the command-produced world through the real native save writer.
 * @param builder Actual hull whose saved state is recorded for the paired reload.
 * @param site Actual job whose packet identities/receipts must survive.
 */
static void SaveConstructionReplay(const SpaceVehicle &builder, const SpaceStructure &site)
{
	const char *output = std::getenv("OTTD_SPACE_FIXES_OUTPUT");
	REQUIRE(output != nullptr);
	REQUIRE(ValidateSpaceVehicles());
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(std::filesystem::path(output) / "checkpoint.sav"), false) == SaveLoadResult::Ok);
	ReportConstructionReplay("checkpoint", builder, site);
}

/** @copydoc RunSpaceConstructionReplay */
void RunSpaceConstructionReplay(std::string_view scenario)
{
	if (scenario == "workers") { RunSpaceConstructionWorkers(); return; }
	const bool reload = scenario.starts_with("reload-");
	const auto stage = scenario.substr(reload ? 7 : 11);
	REQUIRE((stage == "queued" || stage == "materials" || stage == "power" || stage == "working"));
	SpaceVehicle *builder = nullptr;
	SpaceStructure *site = nullptr;
	// Resolve actual paid assets by role; no pool identity, material, work receipt
	// or company balance is installed directly by this checkpoint fixture.
	for (auto *craft : SpaceVehicle::Iterate()) {
		if (craft->owner == _local_company && GetSpaceVehicleModel(*craft) == 20 && craft->construction.placement == SpacePlacement::Orbital) { builder = craft; break; }
	}
	for (auto *candidate : SpaceStructure::Iterate()) {
		if (candidate->state.owner == _local_company && candidate->state.kind == SpaceBuildKind::Storage && candidate->state.phase == SpaceStructurePhase::Blueprint) { site = candidate; break; }
	}
	REQUIRE(builder != nullptr);
	REQUIRE(site != nullptr);
	auto *core = GetOrbitalConstructionCore(site->state);
	REQUIRE(core != nullptr);
	if (!reload) {
		// The fractional-work source retains another paused service. Cancel that
		// real owner before moving the other builder away, preserving its parts.
		if (core->module->service.remaining_ticks != 0) {
			const auto craft = core->module->service.craft;
			REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(core->index, core->generation, craft.vehicle, craft.generation, OrbitalServiceControl::Cancel));
		}
		MoveOtherConstructionCraftAway(*builder);
		OrderConstructionCraft(*builder, *site, SpaceOrderAction::Construct);
		if (stage != "queued") AdvanceConstructionTrace(148, *builder, *site);
		if (stage == "materials") CheckConstructionReadiness(*builder, OrbitalConstructionBlock::MaterialsMissing);
		if (stage == "power") CheckConstructionReadiness(*builder, OrbitalConstructionBlock::PowerUnavailable);
		if (stage == "working") {
			REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core->index, core->generation, false));
			AdvanceConstructionTrace(5, *builder, *site);
			CheckConstructionReadiness(*builder, OrbitalConstructionBlock::Working);
			REQUIRE(site->job.work_fraction != 0);
		}
		SaveConstructionReplay(*builder, *site);
	} else {
		ReportConstructionReplay("checkpoint", *builder, *site);
	}
	// Apply the identical legal player remedy on each side of the save boundary.
	// Material batches move from the actual Core; partial jobs keep their packets.
	if (!HasOrbitalConstructionInputs(*site)) {
		REQUIRE(Command<Commands::ReserveOrbitalModuleInputs>::Post(site->index, site->generation, core->index, core->generation));
	}
	REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core->index, core->generation, false));
	AdvanceConstructionTrace(400, *builder, *site);
	const auto *recipe = GetSpaceConstructionDefinition(site->state.kind);
	REQUIRE(recipe != nullptr);
	CHECK(site->state.phase == SpaceStructurePhase::FrameComplete);
	CHECK(site->job.work_ticks == recipe->work_ticks);
	CHECK(site->job.work_fraction == 0);
	CHECK(site->job.work_remainder == 0);
	CHECK(site->job.inputs[0].TotalCount() == 0);
	CHECK(site->job.inputs[1].TotalCount() == 0);
	CHECK(site->job.consumed[0] == recipe->inputs[0].quantity);
	CHECK(site->job.consumed[1] == recipe->inputs[1].quantity);
	CHECK(builder->space_orders.cursor == builder->space_orders.entries.size());
	const auto spent = GetUtilityPowerProducer(core->module->producer)->consumed_lifetime_j;
	AdvanceConstructionTrace(5, *builder, *site);
	CHECK(GetUtilityPowerProducer(core->module->producer)->consumed_lifetime_j == spent);
	CHECK(ValidateSpaceVehicles());
	ReportConstructionReplay("final", *builder, *site);
}
