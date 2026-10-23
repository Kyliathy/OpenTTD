/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_pacing.cpp Physically reached hardness-specific faces at the halved native cutting rate.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Groups5/5B
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include "movement_fixture.h"
#include "../excavation_state.h"
#include "../tbm_area_preview.h"
#include "../tbm_cmd.h"
#include "../vehicle_cmd.h"

#include "../safeguards.h"

/**
 * Declare one homogeneous geological test patch before any job exists; no work or vehicle position is staged.
 * @param world Actual funded portal/logistics scenario whose surrounding rock remains solid.
 * @param hardness Saved geological difficulty to exercise through the normal quote and cut owners.
 */
static void DeclarePacingRock(const UndergroundWorkflowWorld &world, GeologyHardness hardness)
{
	REQUIRE(CaptureTBMProjects().empty());
	auto snapshot = CaptureGeologyState();
	REQUIRE(snapshot.jobs.empty());
	GeologyDeposit patch;
	patch.id = GeologyDepositID{0};
	patch.layer = world.layer;
	patch.resource = world.resource;
	patch.hardness = hardness;
	patch.initial_reserve = patch.remaining_reserve = 4900;
	// The explicit initial patch covers the complete first swept envelope.
	// Its 49 cells each retain a finite100-tonne mineral quantum, independent
	// of the Earth/Rock receipts under test; no cavity is created by setup.
	for (uint y = TilePlaneY(world.rock) - 3; y <= TilePlaneY(world.rock) + 3; ++y) {
		patch.spans.push_back({static_cast<uint16_t>(y), static_cast<uint16_t>(TileX(world.rock) - 3), static_cast<uint16_t>(TileX(world.rock) + 3)});
	}
	snapshot.deposits = {patch};
	snapshot.next_deposit_id = 1;
	GeologyWorldState restored;
	REQUIRE(PrepareGeologyRestoration(snapshot, restored) == GeologyError::None);
	StageLoadedGeologyState(std::move(restored));
	AcceptLoadedGeologyState();
	CHECK(GetGeologyWorldState().terrain.Get(world.rock) == ExcavationState::Solid);
}

/** Full real controller ticks preserve fractional progress across pauses and use twice the former active time on every supported hardness. */
TEST_CASE("Both native cutters work every supported hardness at half speed", "[tbm-pacing]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	PathfinderDefaults pathfinder;
	const MapLayer layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground);
	const GeologyHardness hardness = GENERATE(GeologyHardness::Soft, GeologyHardness::Mixed, GeologyHardness::Hard, GeologyHardness::Basalt);
	const bool interrupt = GENERATE(false, true);
	UndergroundWorkflowWorld world(layer);
	DeclarePacingRock(world, hardness);
	const uint16_t model_id = layer == MapLayer::Underground ? 0 : 1;
	const TileIndex end = GetLayerPortalDoorTile(world.rock);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	// Use actual paid dedicated wagons large enough for any whole initial cut;
	// otherwise a Basalt capacity refusal would conceal its cutting duration.
	const auto preview = PreviewTBMCommissionArea(_current_company, world.portal, model_id, 2, 6, world.origin->index,
		world.rock, end, false, ExcavationPlanIntent::StraightTunnel);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	if (model_id == 0 && hardness == GeologyHardness::Basalt) {
		// The slower rate does not give Standard a new rock capability.
		CHECK(preview.purchase.Failed());
		CHECK(CaptureTBMProjects().empty());
		return;
	}
	INFO("model=" << model_id << " hardness=" << to_underlying(hardness) << " error=" << preview.purchase.GetErrorMessage().base());
	REQUIRE(preview.purchase.Succeeded());
	REQUIRE(Command<Commands::ConfirmTBMCommission>::Do(DoCommandFlag::Execute, world.portal, model_id, uint8_t{2}, uint8_t{6}, world.origin->index,
		world.rock, end, preview.fingerprint, ExcavationPlanIntent::StraightTunnel).Succeeded());
	const TBMProjectID id = CaptureTBMProjects().front().id;
	const TBMProject *project = GetTBMProject(id);
	const VehicleID cutter = project->cutter;
	const ExcavationJobID job_id = project->job;
	const ExcavationCutResult quote = EstimateExcavationCut(job_id);
	REQUIRE(quote.error == ExcavationError::None);
	const uint days = model_id == 0 ? std::array<uint, 4>{1, 2, 3, 0}[to_underlying(hardness)] : std::array<uint, 4>{1, 1, 2, 4}[to_underlying(hardness)];
	CHECK(quote.work_ticks == quote.new_cells * days * 74);
	// Retain native whole cargo units. Basalt keeps the approved one-tonne
	// Earth minimum while every Rock yield and other Earth yield halves.
	CHECK(quote.earth_t == quote.new_cells * std::array<uint, 4>{3, 2, 1, 1}[to_underlying(hardness)]);
	CHECK(quote.rock_t == quote.new_cells * std::array<uint, 4>{6, 12, 20, 24}[to_underlying(hardness)]);
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, cutter, true).Succeeded());
	uint travel = 0;
	while (GetTBMProject(id)->phase != TBMPhase::Bore && travel++ < 2000) world.Tick(); // Only native movement may reach the work face.
	REQUIRE(GetTBMProject(id)->phase == TBMPhase::Bore);
	REQUIRE(Train::Get(cutter)->tile == world.rock);
	REQUIRE(GetExcavationJob(job_id)->partial_work_ticks == 0);
	const TBMCutterModel &model = *GetTBMCutterModel(model_id);
	const uint64_t duration = CeilDiv(quote.work_ticks * model.work_rate_denominator, uint64_t{model.work_rate_numerator});
	const uint64_t old_duration = CeilDiv(quote.work_ticks * (model_id == 0 ? 1 : 2), uint64_t{model.work_rate_numerator});
	CHECK((duration == old_duration * 2 || duration + 1 == old_duration * 2));
	for (uint64_t tick = 1; tick <= duration; ++tick) {
		// One ordinary vehicle frame earns at most the model's rational work.
		// The ending frame atomically cuts and emits actual packets once.
		world.Tick();
		project = GetTBMProject(id);
		const ExcavationJob &job = *GetExcavationJob(job_id);
		REQUIRE(project->blocked == TBMBlockedReason::None);
		if (tick != duration) {
			REQUIRE(job.cursor == 0);
			REQUIRE(job.partial_work_ticks == tick * model.work_rate_numerator / model.work_rate_denominator);
			REQUIRE(project->work_remainder == tick * model.work_rate_numerator % model.work_rate_denominator);
		}
		if (interrupt && tick == 1) {
			// Pause at a fractional boundary: intervening ordinary frames accrue
			// idle time, but neither erase nor advance the unfinished face.
			const auto saved_job = job;
			const auto remainder = project->work_remainder;
			REQUIRE(remainder != 0);
			REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Pause).Succeeded());
			for (uint idle = 0; idle < 37; ++idle) world.Tick();
			CHECK(*GetExcavationJob(job_id) == saved_job);
			CHECK(GetTBMProject(id)->work_remainder == remainder);
			REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Resume).Succeeded());
		}
	}
	const ExcavationJob &completed = *GetExcavationJob(job_id);
	CHECK(completed.cursor == 1);
	CHECK(completed.partial_work_ticks == 0);
	CHECK(completed.completed_earth_t == quote.earth_t);
	CHECK(completed.completed_rock_t == quote.rock_t);
	CHECK(completed.completed_work_cost == quote.cost);
	uint64_t earth_aboard = 0, rock_aboard = 0;
	// Match the geological receipt to genuine whole cargo packets on the
	// actual native consist immediately after the completed cut.
	for (const Train *part = Train::Get(cutter); part != nullptr; part = part->Next()) {
		const CargoLabel label = CargoSpec::Get(part->cargo_type)->label;
		if (label == CargoLabel{"ERTH"}) earth_aboard += part->cargo.TotalCount();
		if (label == CargoLabel{"ROCK"}) rock_aboard += part->cargo.TotalCount();
	}
	CHECK(earth_aboard == quote.earth_t);
	CHECK(rock_aboard == quote.rock_t);
	fmt::print("PACING model={} hardness={} interrupted={} active_ticks={} former_ticks={} earth={} rock={}\n", model_id,
		to_underlying(hardness), interrupt, duration, old_duration, completed.completed_earth_t, completed.completed_rock_t);
}
