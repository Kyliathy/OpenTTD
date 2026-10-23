/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file tbm_face_deep.cpp Actual Deep acquisition and whole-body clearance at the pure meeting boundary.
	* @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native machinery, geology and paid support.
	* @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group20D.
	* Initial cash/upper floor, source emptying and train poses are explicit components; no driven route or operational face controller is claimed.
	*/
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_commission_fixture.h"
#include "tbm_face_fixture.h"
#include "../safeguards.h"

/** Restore one selected work phase through the same native component boundary as TBMWorkRig.
	* @param id Actual commissioned Deep project, never a relabelled Standard record.
	* @param empty_spoil Whether this declared source-exchange component clears its real hopper packets before more work.
	* This restores original public stores and intentionally clears assignments; the test admits actual haulers only after all cut components finish.
	*/
static void RestoreDeepFaceWork(TBMProjectID id, bool empty_spoil)
{
	const auto *project = GetTBMProject(id); REQUIRE(project != nullptr); auto *front = Train::Get(project->cutter);
	if (empty_spoil) {
	 // Only this acquired train's native packets are removed for the explicit
	 // exchange component. Whole-body lengths and model/part identities stay real.
	 for (Train *part = front; part != nullptr; part = part->Next()) part->cargo.Truncate();
	 front->CargoChanged(); front->UpdateAcceleration();
	}
	auto projects = CaptureTBMProjects(); auto support = CaptureTBMSupport(); const auto next = GetNextTBMProjectID();
	auto at = std::ranges::find(projects, id, &TBMProject::id); REQUIRE(at != projects.end());
	at->phase = TBMPhase::Bore; at->phase_ticks = 0; ++at->revision;
	ResetTBMProjectState(); StageLoadedTBMProjects(std::move(projects), next); StageLoadedTBMSupport(std::move(support)); REQUIRE(AcceptLoadedTBMProjects());
	front->vehstatus.Reset(VehState::Stopped);
}

/** Earn one complete actual Deep support cut at the unchanged model work rate.
	* @param id Existing commissioned Deep project restored to the declared working component phase.
	* Physical spoil/cash/geology/support mutation remains exclusively with the native controller and atomic cut owner.
	*/
static void BoreDeepFaceCut(TBMProjectID id)
{
	const auto *project = GetTBMProject(id); REQUIRE(project != nullptr); const auto *job = GetExcavationJob(project->job); REQUIRE(job != nullptr);
	const auto cursor = job->cursor; const auto estimate = EstimateExcavationCut(project->job); REQUIRE(estimate.error == ExcavationError::None);
	const auto &model = *GetTBMCutterModel(project->model_id); const auto remaining = estimate.work_ticks - job->partial_work_ticks;
	const auto ticks = remaining == 0 ? uint64_t{1} : CeilDiv(remaining * model.work_rate_denominator - project->work_remainder, uint64_t{model.work_rate_numerator});
	// Every model-rate opportunity invokes the real work owner. No geology,
	// cursor, payment, support piece or progress counter is advanced by the fixture.
	for (uint64_t tick = 0; tick < ticks; ++tick) {
	 REQUIRE(AdvanceTBMWork(id) == TBMProjectError::None); REQUIRE(GetTBMProject(id)->blocked == TBMBlockedReason::None);
	}
	REQUIRE(GetExcavationJob(GetTBMProject(id)->job)->cursor == cursor + 1);
}

/** Native model1 machinery and the actual longest hauler must fit beyond the housing, with the same exact inclusive margin as Standard. */
TEST_CASE("Actual commissioned Deep cutter and hauler retain early Bay and exact later Face clearance", "[tbm-face-deep]")
{
	TBMCommissionRig c; AutoRestoreBackup funds(Company::Get(_current_company)->money, Money{50000000});
	AutoRestoreBackup length(_settings_game.vehicle.max_train_length, uint8_t{64});
	AutoRestoreBackup pause_build(_settings_game.construction.command_pause_level, CommandPauseLevel::AllActions);
	const auto portal = TileXYL(44, 30, MapLayer::Underground); std::vector<TileIndex> floor;
	// Only the already-accessible upper assembly footprint is supplied; the
	// entire lower support route stays solid until the actual Deep work owner cuts it.
	for (int dy = -1; dy <= 1; ++dy) {
	 for (int dx = -1; dx <= 1; ++dx) floor.push_back(TileAddWrap(portal, dx, dy));
	}
	c.rig.fixture.world.Open(floor); REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, portal, TransportType::Rail, MapLayer::DeepUnderground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
	const auto rock = GetLayerPortalPairTile(portal); const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(rock));
	const auto start = TileAddWrap(rock, step.x, step.y); const auto end = TileAddWrap(rock, step.x * 24, step.y * 24);
	REQUIRE(IsValidTile(end)); REQUIRE(GetGeologyWorldState().terrain.Get(end) == ExcavationState::Solid);
	const auto quote = CmdCommissionTBM({}, portal, 1, 2, 6, c.rig.fixture.origin->index, start, end, false);
	INFO("Deep commissioning refusal=" << quote.GetErrorMessage().base()); REQUIRE(quote.Succeeded());
	const auto cash = Company::Get(_current_company)->money;
	const auto bought = Command<Commands::CommissionTBM>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, portal,
	 std::make_tuple(portal, uint16_t{1}, uint8_t{2}, uint8_t{6}, c.rig.fixture.origin->index, start, end, false));
	REQUIRE(bought.Succeeded()); CHECK(Company::Get(_current_company)->money == cash - quote.GetCost());
	const auto id = CaptureTBMProjects().back().id; const auto initial = *GetTBMProject(id); auto *front = Train::Get(initial.cutter);
	REQUIRE(initial.model_id == 1); REQUIRE(GetTBMEngineModel(front->engine_type) == 1); REQUIRE(GetTileLayer(GetExcavationJob(initial.job)->commissioning_anchor) == MapLayer::DeepUnderground);
	const auto parts = CountTrainParts(*front); REQUIRE(parts > CountTrainParts(*c.rig.front)); const auto native_length = front->gcache.cached_total_length;
	auto *hauler = c.rig.BuildHauler(c.rig.depot, 2, 6); const uint room = (hauler->gcache.cached_total_length + TILE_SIZE - 1) / TILE_SIZE;
	const uint cuts = static_cast<uint>(room + 2 + parts - 1); REQUIRE(cuts < GetExcavationJob(initial.job)->route.size());
	RestoreDeepFaceWork(id, false);
	// The first short prefix cannot fit the full arriving hauler even with the
	// complete real cutter declared compactly on those same paid cells.
	for (uint cut = 0; cut < 5; ++cut) { BoreDeepFaceCut(id); RestoreDeepFaceWork(id, true); }
	REQUIRE(AssignTBMHauler(id, hauler->index, true) == TBMExchangeError::None);
	std::vector<size_t> ends; auto path = GetTBMWorkRailPath(*GetExcavationJob(initial.job), 5, ends);
	// Deep's wider next-cut sweep may already exclude the short-prefix stand;
	// either that physical exclusion or insufficient hauler room must retain Bay.
	PlaceTrainOnPath(front, path, 5, 3); CHECK(GetTBMMeetingKind(*GetTBMProject(id)) == TBMMeetingKind::Bay);
	RestoreDeepFaceWork(id, true);
	// Earn the remaining support through the same native cut boundary. Each
	// explicit source-exchange reset clears assignments; none is silently copied.
	for (uint cut = 5; cut < cuts; ++cut) { BoreDeepFaceCut(id); RestoreDeepFaceWork(id, true); }
	REQUIRE(AssignTBMHauler(id, hauler->index, true) == TBMExchangeError::None); REQUIRE(GetTBMHaulerRoomCells(id) == room);
	path = GetTBMWorkRailPath(*GetExcavationJob(initial.job), cuts, ends); REQUIRE(path.size() == cuts + 1);
	PlaceTrainOnPath(front, path, cuts, room + 1); REQUIRE(FindTBMFaceStand(*GetTBMProject(id)).has_value()); CHECK(GetTBMMeetingKind(*GetTBMProject(id)) == TBMMeetingKind::Bay);
	PlaceTrainOnPath(front, path, cuts, room + 2); const auto before = c.rig.Digest(); const auto projects = CaptureTBMProjects(); const auto assignments = CaptureTBMExchangeAssignments();
	REQUIRE(FindTBMFaceStand(*GetTBMProject(id)).has_value()); CHECK(GetTBMMeetingKind(*GetTBMProject(id)) == TBMMeetingKind::Face);
	CHECK(c.rig.Digest() == before); CHECK(CaptureTBMProjects() == projects); CHECK(CaptureTBMExchangeAssignments() == assignments);
	CHECK(front->gcache.cached_total_length == native_length); CHECK(CountTrainParts(*front) == parts); CHECK(GetTBMProject(id)->model_id == 1);
	CHECK(GetTBMExchangeBay(*GetTBMProject(id)).kind == TBMExchangeBayKind::Source);
	fmt::print("Deep clearance component: parts={}, native length={}, hauler cells={}, earned cuts={}\n", parts, native_length, room, cuts);
}
