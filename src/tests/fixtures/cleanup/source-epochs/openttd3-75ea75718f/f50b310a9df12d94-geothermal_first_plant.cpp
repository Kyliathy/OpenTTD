/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant.cpp Actual paid first-plant baseline and canonical native excavation; no supplied floor, construction stock or energy.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FA.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-e-first-plant-bootstrap.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "geothermal_first_plant_fixture.h"
#include "test_artifact_paths.h"
#include "../saveload/saveload_func.h"
#include "../gfx_func.h"
#include "../map_layer_gui.h"
#include "../train_cmd.h"
#include "../landscape_cmd.h"
#include "../tbm_replanning.h"
#include "../table/sprites.h"
#include <set>

/** @param rig Original paid world and actual native identities. @param stage Distinct retained original checkpoint. Save actual owners without supplying any floor, cargo, work, cash or utility quantities. */
static void SaveFirstPlant(GeothermalFirstPlantRig &rig, std::string_view stage)
{
	LayerSaveContent content{rig.world.ActivatedContent(), true}; AutoRestoreBackup local{_local_company, _current_company};
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	AutoRestoreBackup menu{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	const auto path = GetTestCaseArtifactPaths("geothermal", "first-plant").Output("saves", fmt::format("first-plant-{}.sav", stage));
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	fmt::print("FIRST_PLANT_SAVE stage={} cash={} geology={:016X} path={}\n", stage, Company::Get(_current_company)->money.base(),
		ComputeGeologyStateDigest(GetGeologyWorldState()), path.string());
}

/** Exact native Surface setup is paid while all declared Underground/Deep work remains solid and geothermal accounts remain absent. */
TEST_CASE("First plant starts with actual paid Surface logistics and untouched target floors", "[geothermal-first-plant-baseline]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalFirstPlantRig rig;
	CHECK(rig.world.setup_cost > Money{0}); CHECK(Company::Get(_current_company)->money == Money{10000000} - rig.world.setup_cost);
	CHECK(GetGeologyWorldState().hardness_version == GEOLOGY_HARDNESS_VERSION);
	CHECK(GetLayerPortalWorkState(rig.world.portal) == LayerPortalWorkState::Sealed);
	const auto underground = rig.Rectangle(rig.world.site, TileAddWrap(rig.world.site, 4, 8));
	const auto deep = rig.Rectangle(rig.layout.deep_start, rig.layout.deep_end);
	// Neither paid Surface logistics nor pure desired plant geometry grants
	// one excavated target cell. The full declared room must still be earned.
	for (const auto tile : underground) CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Solid);
	for (const auto tile : deep) CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Solid);
	CHECK(CargoPacket::GetNumItems() == 0); CHECK(CaptureGeothermalGenerators().empty()); CHECK(CaptureGeothermalCollectors().empty());
	CHECK(CaptureGeothermalRisers().empty()); CHECK(CaptureUtilityPowerState().producers.empty());
	SaveFirstPlant(rig, "baseline");
	fmt::print("FIRST_PLANT_BASELINE setup={} cash={} vent={}:{}:{} rate={} upper_cells={} deep_cells={} source={}\n",
		rig.world.setup_cost.base(), Company::Get(_current_company)->money.base(), rig.layout.vent.id.base(), rig.layout.vent.x, rig.layout.vent.y,
		rig.layout.vent.thermal_capacity_w, underground.size(), deep.size(), rig.world.portal.base());
}

/** @param rig Actual retained initial world. Earn all Underground access through the real Standard cutter/spoil loop, then release paid floor while retaining the whole machine. */
static void PrepareFirstPlantAccess(GeothermalFirstPlantRig &rig)
{
	SaveFirstPlant(rig, "baseline");
	const auto *company = Company::Get(_current_company); const auto cash = company->money;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()); const auto random = std::array{_random.state[0], _random.state[1]};
	const auto consist = GetDefaultTBMConsist(MapLayer::Underground);
	const auto quote = PreviewTBMCommissionArea(_current_company, rig.world.portal, GetLayerPortalDefaultTBMModel(rig.world.portal),
		consist.earth_wagons, consist.rock_wagons, rig.world.origin->index, rig.world.site, TileAddWrap(rig.world.site, 4, 8), false);
	INFO("area=" << to_underlying(quote.area.error) << " tile=" << quote.area.failure_tile.base());
	REQUIRE(quote.purchase.Succeeded()); REQUIRE(quote.area.error == ExcavationError::None); REQUIRE_FALSE(quote.area.partial);
	CHECK(company->money == cash); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(std::array{_random.state[0], _random.state[1]} == random);
	std::set<TileIndex> swept;
	// Only the canonical advancing sweeps define the charged union. Requested
	// floor and authorized turning room cannot multiply overlap or invent work.
	for (const auto &cut : quote.area.proposal.route) {
		for (const auto tile : cut.swept_tiles) swept.insert(tile);
	}
	uint32_t solid = 0;
	for (const auto tile : swept) solid += GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Solid;
	CHECK(quote.area.totals.new_cells == solid);
	bool partial_saved = false;
	/** @param current Same fully stepped actual native world. Retain the first earned unfinished cut without creating a material/work receipt. */
	const auto observe = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index); REQUIRE(assignment != nullptr);
		const auto *project = GetTBMProject(assignment->project); REQUIRE(project != nullptr);
		const auto *job = GetExcavationJob(project->job); REQUIRE(job != nullptr);
		// A genuine incomplete native job must have earned a positive cut but
		// still retain unfinished work; startup or finished files cannot replace it.
		if (!partial_saved && job->cursor != 0 && job->cursor < job->route.size()) {
			SaveFirstPlant(rig, "standard-partial"); partial_saved = true;
		}
	};
	const auto id = RunUndergroundExcavationWorkflow(rig.world, 0, observe, {}, true);
	REQUIRE(partial_saved); const auto project = *GetTBMProject(id); const auto job = *GetExcavationJob(project.job);
	CHECK(job.completed_earth_t == quote.area.totals.earth_t); CHECK(job.completed_rock_t == quote.area.totals.rock_t);
	CHECK(job.completed_work_cost == quote.area.totals.cost); CHECK(job.cursor == job.route.size());
	CHECK(GetTBMExchangeAssignmentForHauler(rig.world.hauler->index)->exchanges != 0);
	// Every newly required cell must now be opened by its original paid job;
	// no construction command may fill a missing corner with fixture restoration.
	for (const auto tile : rig.Rectangle(rig.world.site, TileAddWrap(rig.world.site, 4, 8))) CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
	for (const auto tile : rig.Rectangle(rig.layout.deep_start, rig.layout.deep_end)) CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Solid);
	CHECK(GetSpoilReceiver(rig.world.earth->index)->fees + GetSpoilReceiver(rig.world.stone->index)->fees == job.completed_earth_t * 2 + job.completed_rock_t * 3);
	CHECK(company->yearly_expenses[0][ExpensesType::TrainRevenue] == Money{0});
	SaveFirstPlant(rig, "standard-cleared");
	rig.Do<Commands::ReleaseTBMHauler>(rig.world.hauler->index);
	// Completed native trains may already be stopped. Stop only still-running
	// owners, avoiding a toggle that would restart the retained paid machine.
	if (!rig.world.hauler->vehstatus.Test(VehState::Stopped)) rig.Do<Commands::StartStopVehicle>(rig.world.hauler->index, true);
	if (!Train::Get(project.cutter)->vehstatus.Test(VehState::Stopped)) rig.Do<Commands::StartStopVehicle>(project.cutter, true);
	rig.Do<Commands::ControlTBMProject>(id, TBMProjectAction::Retire);
	CHECK(GetTBMProject(id) == nullptr); CHECK(IsTrainStoppedAtPortalSite(*Train::Get(project.cutter))); // The portal magazine uses its canonical whole-chain depot-state predicate; ordinary rail-depot tile classification does not describe it.
	// Released support remains actual owned paid track; conversion pays its
	// additional invoice before ordinary access construction can reuse the floor.
	for (const auto &support : CaptureTBMSupport()) rig.Do<Commands::ConvertTBMSupport>(support.tile);
	rig.Do<Commands::ChangeLayerPortalConnection>(rig.world.portal, true);
	CHECK(GetLayerPortalWorkState(rig.world.portal) == LayerPortalWorkState::Connected);
	Money expenses = 0;
	for (const auto amount : company->yearly_expenses[0]) expenses += amount; // The actual native ledger, including every initial setup/purchase/work/haulage/disposal/command invoice, must reconcile to cash.
	CHECK(company->money == Money{10000000} - expenses);
	CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(CargoPacket::GetNumItems() == 0);
	SaveFirstPlant(rig, "standard-released");
	fmt::print("FIRST_PLANT_ACCESS cells={} earth={} rock={} work_support={} purchase={} setup={} commands={} cash={} ticks={} cutter={} job={}\n",
		solid, job.completed_earth_t, job.completed_rock_t, job.completed_work_cost.base(), quote.purchase.GetCost().base(), rig.world.setup_cost.base(),
		rig.commands.base(), company->money.base(), rig.world.ticks, project.cutter.base(), job.id.base());
}

/** The Standard cutter earns every future Underground access cell, real wagons deliver all spoil to Surface and release leaves one retained paid machine. */
TEST_CASE("First plant earns Underground access through paid Standard cutting and spoil haulage", "[geothermal-first-plant-access]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalFirstPlantRig rig; PrepareFirstPlantAccess(rig);
}

/** A second actual machine commissions on paid access and retains a bounded ordinary unfinished Deep checkpoint for real file continuation. */
TEST_CASE("First plant retains paid Deep excavation for ordinary bounded continuation", "[geothermal-first-plant-deep]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalFirstPlantRig rig; PrepareFirstPlantAccess(rig);
	// The Standard predecessor owns only two Rock hoppers. Deep's independent
	// four-hopper load must be physically bought/coupled at its genuine depot;
	// another project assignment cannot enlarge the retained empty wagon set.
	rig.Do<Commands::SendVehicleToDepot>(rig.world.hauler->index, DepotCommandFlags{}, VehicleListIdentifier{});
	if (rig.world.hauler->vehstatus.Test(VehState::Stopped)) rig.Do<Commands::StartStopVehicle>(rig.world.hauler->index, true);
	uint withdrawal = 0;
	while (!rig.world.hauler->IsStoppedInDepot() && withdrawal < 12000) { rig.world.Tick(); ++withdrawal; } // Real depot orders withdraw the complete original empty chain; no part is repositioned or hidden by the fixture.
	REQUIRE(rig.world.hauler->IsStoppedInDepot());
	const auto depot = rig.world.hauler->tile;
	const auto standard_consist = GetDefaultTBMConsist(MapLayer::Underground), deep_consist = GetDefaultTBMConsist(MapLayer::DeepUnderground);
	for (uint8_t index = standard_consist.rock_wagons; index < deep_consist.rock_wagons; ++index) {
		const auto [cost, wagon, capacity, mail, capacities] = rig.Do<Commands::BuildVehicle>(depot, GetUndergroundEngine(3), false, INVALID_CARGO, ClientID::Invalid);
		rig.Do<Commands::MoveRailVehicle>(wagon, rig.world.hauler->Last()->index, false);
	}
	// The longer real hauler needs the fourth platform cell used by native
	// Deep workflows. Extend the same owned station and retain its orders/ID.
	// Building over the existing platform lets the ordinary owner identify its
	// station without requesting disabled distant-join construction permissions.
	const auto origin_id = rig.world.origin->index;
	rig.Do<Commands::BuildRailStation>(rig.world.origin->train_station.tile, RAILTYPE_RAIL, Axis::Y, uint8_t{1}, uint8_t{4}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	CHECK(rig.world.origin->index == origin_id);
	CHECK(rig.world.origin->train_station.h == 4);
	// The Deep room creates more Rock than this crusher's finite aggregate
	// store. Choose its existing paid disposal mode before new intake, retaining
	// every earlier crushed tonne/aggregate and paying the actual higher fee.
	rig.Do<Commands::SetSpoilReceiverMode>(rig.world.stone->index, SpoilReceiverMode::SurplusDisposal);
	const auto x = TileX(rig.world.rock), first = TilePlaneY(rig.world.rock);
	// A public route uses only already earned Underground cells. Reusing an
	// existing paid piece is free; any missing straight branch pays its own bill.
	for (uint y = first + 1; y < TilePlaneY(rig.layout.deep_commission); ++y) {
		const auto tile = TileXYL(x, y, MapLayer::Underground);
		REQUIRE(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
		if (IsPlainRailTile(tile) && GetTrackBits(tile).Test(Track::Y)) continue; // Already owned straight access is reused; another orientation cannot impersonate it.
		rig.Do<Commands::BuildRail>(tile, RAILTYPE_RAIL, Track::Y, false);
	}
	// Support conversion retains real permanent track. A new shaft requires
	// an actually vacant housing cell, so its owned rail must be cleared and
	// paid through the ordinary command before construction may replace it.
	if (IsPlainRailTile(rig.layout.deep_commission)) rig.Do<Commands::LandscapeClear>(rig.layout.deep_commission);
	rig.Do<Commands::BuildLayerPortal>(rig.layout.deep_commission, TransportType::Rail, MapLayer::DeepUnderground,
		DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD);
	// These are fixture selectors for the next real visible command. Actual
	// portal, project, train, job and geological state stay with native owners.
	rig.world.portal = rig.layout.deep_commission; rig.world.rock = GetLayerPortalPairTile(rig.world.portal);
	rig.world.layer = MapLayer::DeepUnderground; rig.world.site = rig.layout.deep_start;
	const auto consist = GetDefaultTBMConsist(MapLayer::DeepUnderground); const auto cash = Company::Get(_current_company)->money;
	const auto first_end = TileAddWrap(rig.layout.deep_start, 4, 13);
	const auto quote = PreviewTBMCommissionArea(_current_company, rig.world.portal, GetLayerPortalDefaultTBMModel(rig.world.portal),
		consist.earth_wagons, consist.rock_wagons, rig.world.origin->index, rig.world.site, first_end, false);
	INFO("Deep area=" << static_cast<unsigned>(to_underlying(quote.area.error)) << " tile=" << quote.area.failure_tile.base());
	REQUIRE(quote.purchase.Succeeded()); REQUIRE(quote.area.error == ExcavationError::None); REQUIRE_FALSE(quote.area.partial);
	CHECK(Company::Get(_current_company)->money == cash); SaveFirstPlant(rig, "deep-ready");
	fmt::print("FIRST_PLANT_DEEP_INITIAL_QUOTE cells={} earth={} rock={} work_support={} purchase={}\n",
		quote.area.totals.new_cells, quote.area.totals.earth_t, quote.area.totals.rock_t,
		quote.area.totals.cost.base(), quote.purchase.GetCost().base());
	bool partial_saved = false, checkpointed = false;
	const uint64_t deep_begin = rig.world.ticks;
	/** @param current Same actual native world after a paid step. Preserve the first genuinely earned unfinished Deep work. */
	const auto observe = [&](const UndergroundWorkflowWorld &current) {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index); REQUIRE(assignment != nullptr);
		const auto *project = GetTBMProject(assignment->project); REQUIRE(project != nullptr);
		const auto *job = GetExcavationJob(project->job); REQUIRE(job != nullptr);
		// Startup and completed saves cannot replace an actual partial paid job.
		if (!partial_saved && job->cursor != 0 && job->cursor < job->route.size()) {
			SaveFirstPlant(rig, "deep-partial"); partial_saved = true;
		}
	};
	/** @param current Actual stepped world. Pause only through the native command after the declared bounded writer interval. */
	const auto checkpoint = [&](const UndergroundWorkflowWorld &current) {
		if (current.ticks - deep_begin < 20000) return;
		const auto *assignment = GetTBMExchangeAssignmentForHauler(current.hauler->index); REQUIRE(assignment != nullptr);
		rig.Do<Commands::ControlTBMProject>(assignment->project, TBMProjectAction::Pause);
		SaveFirstPlant(rig, "deep-checkpoint"); checkpointed = true;
	};
	/** @param current Same real world, never advanced here. @return Whether its actual unfinished ordinary checkpoint is retained. */
	const auto stop = [&]([[maybe_unused]] const UndergroundWorkflowWorld &current) { return checkpointed; };
	const auto id = RunUndergroundExcavationWorkflow(rig.world, 0, observe, checkpoint, true, first_end, stop);
	REQUIRE(checkpointed); REQUIRE(partial_saved);
	const auto &project = *GetTBMProject(id); const auto &job = *GetExcavationJob(project.job);
	CHECK(job.cursor > 0); CHECK(job.cursor < job.route.size());
	CHECK(project.blocked == TBMBlockedReason::Paused); CHECK(CaptureGeothermalGenerators().empty());
	CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(Company::Get(_current_company)->yearly_expenses[0][ExpensesType::TrainRevenue] == Money{0});
	fmt::print("FIRST_PLANT_DEEP_CHECKPOINT ticks={} cursor={}/{} earth={} rock={} cost={} cash={}\n", rig.world.ticks - deep_begin,
		job.cursor, job.route.size(), job.completed_earth_t, job.completed_rock_t, job.completed_work_cost.base(), Company::Get(_current_company)->money.base());
}
