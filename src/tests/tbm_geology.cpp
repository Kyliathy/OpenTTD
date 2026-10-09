/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_geology.cpp Actual native cutting and withdrawal against generated regional rock.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group10
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include "depot_file_fixture.h"
#include "../tbm_area_preview.h"
#include "../tbm_cmd.h"
#include "../tbm_replanning.h"
#include "../vehicle_cmd.h"
#include "../order_cmd.h"
#include "../company_cmd.h"

#include "../safeguards.h"

/** Native travel crosses a generated Soft/Mixed boundary, saves live partial work and withdraws with its original identity. */
TEST_CASE("TBM cuts generated regional rock then physically parks", "[tbm-geology]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	NativeDepotEconomy economy;
	UndergroundWorkflowWorld world(MapLayer::Underground, true, true, GEOLOGY_HARDNESS_VERSION);
	LayerSaveContent content(world.ActivatedContent(), true);
	world.simulate_company_limits = true;
	REQUIRE(Command<Commands::RenameCompany>::Do(DoCommandFlag::Execute, "Regional Rock").Succeeded());
	const auto direction = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(world.rock));
	const TileIndex end = TileAddWrap(world.rock, direction.x * 40, direction.y * 40);
	const auto quote = PreviewTBMCommissionArea(_current_company, world.portal, 0, 2, 2, world.origin->index,
		world.rock, end, false, ExcavationPlanIntent::StraightTunnel);
	REQUIRE(quote.purchase.Succeeded());
	REQUIRE(Command<Commands::ConfirmTBMCommission>::Do(DoCommandFlag::Execute, world.portal, uint16_t{0}, uint8_t{2}, uint8_t{2}, world.origin->index,
		world.rock, end, quote.fingerprint, ExcavationPlanIntent::StraightTunnel).Succeeded());
	const auto project = CaptureTBMProjects().front();
	const auto cutter = project.cutter;
	Order origin;
	origin.MakeGoToStation(world.origin->index);
	REQUIRE(Command<Commands::InsertOrder>::Do(DoCommandFlag::Execute, world.hauler->index, VehicleOrderID{0}, origin).Succeeded());
	REQUIRE(Command<Commands::AssignTBMHauler>::Do(DoCommandFlag::Execute, project.id, world.hauler->index).Succeeded());
	DepotFileRecorder recorder;
	recorder.Save("regional-accepted");
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, cutter, true).Succeeded());
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, world.hauler->index, true).Succeeded());
	bool reached_mixed = false;
	// Real cutting and real wagon exchanges alone advance the accepted route.
	// Stop during the first partial Mixed cut, retaining an actual file boundary.
	for (uint ticks = 0; ticks < 65000; ++ticks) {
		const auto *job = GetExcavationJob(project.job);
		if (GetTBMProject(project.id)->phase == TBMPhase::Bore && job->partial_work_ticks >= 20 &&
			GetExcavationCellHardness(GetGeologyWorldState(), job->route[job->cursor].cutter_position) == GeologyHardness::Mixed) {
			reached_mixed = true; break;
		}
		world.Tick();
	}
	REQUIRE(reached_mixed);
	const auto retained = *GetExcavationJob(project.job);
	const auto remainder = GetTBMProject(project.id)->work_remainder;
	recorder.Save("regional-midcut");
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project.id, TBMProjectAction::ReturnAndPark).Succeeded());
	recorder.Save("regional-return");
	for (uint ticks = 0; ticks < 16000 && GetTBMProject(project.id)->phase != TBMPhase::Parked; ++ticks) world.Tick();
	REQUIRE(GetTBMProject(project.id)->phase == TBMPhase::Parked);
	CHECK(*GetExcavationJob(project.job) == retained);
	CHECK(GetTBMProject(project.id)->work_remainder == remainder);
	CHECK(GetTBMProject(project.id)->cutter == cutter);
	for (const Train *part = Train::Get(cutter); part != nullptr; part = part->Next()) {
		CHECK(part->tile == world.portal);
		CHECK(part->track == Track::Depot);
		CHECK(part->cargo.TotalCount() == 0);
	}
	recorder.Save("regional-parked");
	fmt::print("REGIONAL_TBM cursor={} partial={} ticks={} cutter={} parked=true\n", retained.cursor, retained.partial_work_ticks, world.ticks, cutter.base());
}

/** @return Seed and first Basalt distance along an actually generated seam's approach, without publishing trial worlds. */
static std::pair<uint32_t, uint> FindRegionalBoundarySeed()
{
	ExcavationFixture map;
	// Seed search is bounded fixture setup. The selected world is then created
	// normally; no seam, material, native train position or paid floor is staged.
	for (uint32_t seed = 1; seed <= 16384; ++seed) {
		const auto generated = PrepareGeologyGeneration(DeriveGeologySeed(seed));
		if (generated.error != GeologyError::None) continue;
		TileIndex source = INVALID_TILE;
		for (const auto &deposit : generated.state.deposits.Records()) {
			if (deposit.resource != CargoLabel{"COAL"}) continue;
			for (const auto &span : deposit.spans) {
				if (span.y < 12 || span.y > 43 || span.first_x > 48 || span.last_x < 12) continue;
				source = TileXYL(std::max<uint>(span.first_x, 12) + 1, span.y - 2, MapLayer::Underground); break;
			}
			if (source != INVALID_TILE) break;
		}
		if (source == INVALID_TILE) continue;
		uint first = 0;
		bool starter_soft = true;
		for (int step = -3; step <= 25 && TilePlaneY(source) + step < 62; ++step) {
			bool basalt = false;
			for (int side = -1; side <= 1; ++side) {
				const auto cell = TileAddWrap(source, side, step);
				const auto material = GetExcavationCellHardness(generated.state, cell);
				basalt |= material == GeologyHardness::Basalt;
				// The fixture's existing two Rock wagons must fit the whole
				// atomic starter pocket. Select suitable real terrain instead of
				// weakening capacity rules to reach the later Basalt boundary.
				if (step <= 3) starter_soft &= material == GeologyHardness::Soft;
			}
			if (basalt) { first = static_cast<uint>(std::max(step, 0)); break; }
		}
		if (starter_soft && first >= 10 && first <= 24 && GetExcavationCellHardness(generated.state, TileAddWrap(source, 0, first)) == GeologyHardness::Basalt) return {seed, first};
	}
	FAIL("No suitable generated Standard-cutter boundary within bounded seed search");
	return {0, 0};
}

/** A Standard cutter reaches an accepted endpoint beside visible Basalt, refuses further work and backs out without cutting it. */
TEST_CASE("Standard cutter retreats safely from a generated Basalt boundary", "[tbm-geology-boundary]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const auto [seed, boundary] = FindRegionalBoundarySeed();
	NativeDepotEconomy economy;
	UndergroundWorkflowWorld world(MapLayer::Underground, true, true, GEOLOGY_HARDNESS_VERSION, seed);
	LayerSaveContent content(world.ActivatedContent(), true);
	world.simulate_company_limits = true;
	REQUIRE(Command<Commands::RenameCompany>::Do(DoCommandFlag::Execute, "Rock Boundary").Succeeded());
	const TileIndex basalt = TileAddWrap(world.rock, 0, boundary);
	// The physical cutter owns its forward body cell as well as its centre.
	// Leave that complete accepted envelope on compatible rock; the extension
	// below deliberately reaches the Basalt beyond the machine's actual face.
	const TileIndex end = TileAddWrap(world.rock, 0, boundary - 2);
	REQUIRE(GetExcavationCellHardness(GetGeologyWorldState(), basalt) == GeologyHardness::Basalt);
	const auto quote = PreviewTBMCommissionArea(_current_company, world.portal, 0, 2, 2, world.origin->index,
		world.rock, end, false, ExcavationPlanIntent::StraightTunnel);
	INFO("seed=" << seed << " boundary=" << boundary << " error=" << quote.purchase.GetErrorMessage().base());
	REQUIRE(quote.purchase.Succeeded());
	REQUIRE(Command<Commands::ConfirmTBMCommission>::Do(DoCommandFlag::Execute, world.portal, uint16_t{0}, uint8_t{2}, uint8_t{2}, world.origin->index,
		world.rock, end, quote.fingerprint, ExcavationPlanIntent::StraightTunnel).Succeeded());
	const auto project = CaptureTBMProjects().front();
	Order origin;
	origin.MakeGoToStation(world.origin->index);
	REQUIRE(Command<Commands::InsertOrder>::Do(DoCommandFlag::Execute, world.hauler->index, VehicleOrderID{0}, origin).Succeeded());
	REQUIRE(Command<Commands::AssignTBMHauler>::Do(DoCommandFlag::Execute, project.id, world.hauler->index).Succeeded());
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, project.cutter, true).Succeeded());
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, world.hauler->index, true).Succeeded());
	bool reached = false;
	// The last accepted face is physically reached before interruption. Its
	// incomplete cut keeps the job resumable for the existing extension command.
	for (uint ticks = 0; ticks < 65000; ++ticks) {
		const auto *job = GetExcavationJob(project.job);
		if (GetTBMProject(project.id)->phase == TBMPhase::Bore && job->cursor + 1 == job->route.size() && job->partial_work_ticks >= 20) { reached = true; break; }
		world.Tick();
	}
	REQUIRE(reached);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project.id, TBMProjectAction::Pause).Succeeded());
	const auto retained = *GetExcavationJob(project.job);
	const auto before = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto extension = PreviewTBMExtension(_current_company, project.id, end, basalt, ExcavationPlanIntent::StraightTunnel);
	CHECK(extension.result.Failed());
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == before);
	DepotFileRecorder recorder;
	recorder.Save("boundary-paused");
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project.id, TBMProjectAction::ReturnAndPark).Succeeded());
	for (uint ticks = 0; ticks < 16000 && GetTBMProject(project.id)->phase != TBMPhase::Parked; ++ticks) world.Tick();
	REQUIRE(GetTBMProject(project.id)->phase == TBMPhase::Parked);
	CHECK(*GetExcavationJob(project.job) == retained);
	CHECK(GetGeologyWorldState().terrain.Get(basalt) == ExcavationState::Solid);
	for (const Train *part = Train::Get(project.cutter); part != nullptr; part = part->Next()) {
		CHECK(part->tile == world.portal);
		CHECK(part->track == Track::Depot);
		CHECK(part->cargo.TotalCount() == 0);
	}
	recorder.Save("boundary-parked");
	fmt::print("REGIONAL_BOUNDARY seed={} distance={} cursor={} ticks={} cutter={} basalt_solid=true parked=true\n", seed, boundary, retained.cursor, world.ticks, project.cutter.base());
}
