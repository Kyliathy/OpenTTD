/* This file is part of OpenTTD, licensed under GNU GPL version 2. */
/** @file tbm_change_work.cpp Native unfinished-work command, stable running quotes and physical refusal boundaries.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native command, cargo and geology ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Group23CC.
 * Paid cuts are real. Earlier exchanges and parked/returning poses are explicit component inputs, not driven acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_remainder_fixture.h"
#include "tbm_face_fixture.h"
#include "tbm_face_support_fixture.h"
#include "../tbm_replanning.h"
#include "../tbm_cmd.h"
#include "../table/strings.h"
#include "../safeguards.h"

// Keep the original command prefix and exact wire argument widths while appending the complete native body.
static_assert(to_underlying(Commands::ChangeTBMWork) == 212);
static_assert(to_underlying(Commands::End) > 212);
static_assert(std::is_same_v<CommandTraits<Commands::ChangeTBMWork>::Args,
	std::tuple<TBMProjectID, TileIndex, TileIndex, uint32_t, uint64_t, ExcavationPlanIntent>>);

/** @return First corner of a new floor beside the planned corridor, SW of the cutter's line: reaching it takes a bend. */
static TileIndex ChangeFrom() { return TileXYL(22, 25, MapLayer::Underground); }
/** @return Opposite corner of that floor. Both are computed on the live map, never at static initialization. */
static TileIndex ChangeTo() { return TileXYL(23, 26, MapLayer::Underground); }

/**
 * @param quote A refused or accepted change quote.
 * @return Its refusal text for test output, or nothing when accepted.
 */
static std::string Refusal(const TBMWorkChangePreview &quote)
{
	if (quote.result.Succeeded()) return {};
	return fmt::format("{} (outline error {} at {}, plan error {} at {})", quote.result.GetErrorMessage().base(), to_underlying(quote.outline.error),
		quote.outline.failure_tile.base(), to_underlying(quote.plan.error), quote.plan.failure_tile.base());
}

/** Task 12.6: the command matches its quote, needs no pause, keeps the phase and the next cut is the new route's first. */
TEST_CASE("Change work area keeps a boring machine working on the new cuts", "[tbm-change-work]")
{
	TBMRemainderRig rig;
	rig.Bore(2);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	const auto project = rig.Project();
	const auto job = rig.Job();
	REQUIRE(project.phase == TBMPhase::Bore);
	REQUIRE(project.blocked != TBMBlockedReason::Paused);
	REQUIRE(job.partial_work_ticks > 0);
	const auto quote = PreviewTBMWorkChange(_current_company, project.id, ChangeFrom(), ChangeTo());
	INFO("refusal: " << Refusal(quote));
	REQUIRE(quote.result.Succeeded());
	CHECK(quote.cursor == job.cursor);
	CHECK(quote.dropped_cuts == job.route.size() - job.cursor);
	REQUIRE(quote.plan.route.front() != job.route[job.cursor]);
	CHECK(quote.abandoned_work_ticks == job.partial_work_ticks); // The cut in progress is abandoned, and the quote says so.
	CHECK(quote.cut_work_ticks >= job.partial_work_ticks);

	// Parity: the command's estimate is the quote and changes nothing.
	const auto before = rig.Digest();
	const auto estimate = CmdChangeTBMWork(DoCommandFlags{}, project.id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area);
	CHECK(estimate.Succeeded());
	CHECK(estimate.GetCost() == quote.result.GetCost());
	CHECK(rig.Digest() == before);
	CHECK(rig.Job() == job);
	CHECK(rig.Project() == project);

	REQUIRE(CmdChangeTBMWork(DoCommandFlag::Execute, project.id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
	const auto after = rig.Project();
	CHECK(after.phase == project.phase);
	CHECK(after.blocked == project.blocked);
	CHECK(after.heading == project.heading);
	CHECK(after.work_remainder == project.work_remainder);
	CHECK(after.revision == project.revision + 1);
	CHECK(rig.Job().cursor == job.cursor);
	CHECK(rig.Job().route[job.cursor] == quote.plan.route.front());
	CHECK(rig.Job().partial_work_ticks == 0);
	CHECK(Company::Get(_current_company)->money == before.money); // Changing the plan costs nothing; the cuts pay as they go.
	// The face's laid track leads on to the new first cut, so the machine can drive back to its face from anywhere.
	std::vector<size_t> ends;
	const auto path = GetTBMWorkRailPath(rig.Job(), job.cursor + 1, ends);
	REQUIRE(ends.size() > job.cursor);
	const size_t face = ends[job.cursor - 1];
	REQUIRE(IsPlainRailTile(path[face]));
	const DiagDirection entry = DiagdirBetweenTiles(path[face], path[face - 1]);
	const DiagDirection exit = DiagdirBetweenTiles(path[face], path[face + 1]);
	bool joined = false;
	// A real laid directed piece must join the retained arrival to the new departure.
	for (Trackdir trackdir : DiagdirReachesTrackdirs(ReverseDiagDir(entry))) {
		joined |= TrackdirToExitdir(trackdir) == exit && GetTrackBits(path[face]).Test(TrackdirToTrack(trackdir));
	}
	CHECK(joined);

	// The machine simply carries on: its next completed cut is the new route's first.
	rig.BoreOneCut();
	CHECK(rig.Job().cursor == job.cursor + 1);
	// The next paid advance must excavate exactly the newly accepted first sweep.
	for (TileIndex tile : quote.plan.route.front().swept_tiles) CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
}

/** Task 12.6: the quote is bound to the completed-cut count, not to the work ticks of the cut in progress. */
TEST_CASE("A change quote goes stale only when another cut completes", "[tbm-change-work]")
{
	TBMRemainderRig rig;
	rig.Bore(2);
	const auto id = rig.Project().id;
	const auto quote = PreviewTBMWorkChange(_current_company, id, ChangeFrom(), ChangeTo());
	INFO("refusal: " << Refusal(quote));
	REQUIRE(quote.result.Succeeded());
	// Work on the cut in progress moves the job revision every work tick; the quote stays valid.
	for (int tick = 0; tick < 4; ++tick) REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	REQUIRE(rig.Job().partial_work_ticks > 0);
	CHECK(CmdChangeTBMWork(DoCommandFlags{}, id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
	// Another completed cut makes it stale.
	rig.BoreOneCut();
	const auto before = rig.Digest();
	const auto stale = CmdChangeTBMWork(DoCommandFlag::Execute, id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area);
	CHECK(stale.Failed());
	CHECK(stale.GetErrorMessage() == STR_ERROR_TBM_CHANGE_STALE);
	CHECK(rig.Digest() == before);
}


/** Eligible live phases keep their existing physical intent when only the unfinished plan is replaced. */
TEST_CASE("Native work change retains working paused returning and parked intent", "[tbm-change-work]")
{
	TBMRemainderRig rig;
	rig.Bore(2);
	const auto id = rig.Project().id;
	SECTION("working") {}
	SECTION("paused") { REQUIRE(ControlTBMProject(id, TBMProjectAction::Pause, true) == TBMProjectError::None); }
	SECTION("returning to park") { REQUIRE(ControlTBMProject(id, TBMProjectAction::ReturnAndPark, true) == TBMProjectError::None); }
	SECTION("returning for exchange") { rig.Reload(TBMPhase::ReturnForExchange); }
	SECTION("parked") { rig.Reload(TBMPhase::Parked); }
	const auto project = rig.Project();
	const auto quote = PreviewTBMWorkChange(_current_company, id, ChangeFrom(), ChangeTo());
	INFO(Refusal(quote));
	REQUIRE(quote.result.Succeeded());
	REQUIRE(CmdChangeTBMWork(DoCommandFlag::Execute, id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
	CHECK(rig.Project().phase == project.phase);
	CHECK(rig.Project().blocked == project.blocked);
	CHECK(rig.Project().heading == project.heading);
	CHECK(rig.Project().work_remainder == project.work_remainder);
	CHECK(rig.Job().route[rig.Job().cursor] == quote.plan.route.front());
}

/** Refusal before the first completed cut is independent of later phase sections, so every section executes. */
TEST_CASE("Native work change refuses a cutter before its first paid cut", "[tbm-change-work]")
{
	TBMRemainderRig rig;
	const auto before = TBMFaceSnapshot::Capture(rig);
	CHECK(PreviewTBMWorkChange(_current_company, rig.Project().id, ChangeFrom(), ChangeTo()).result.GetErrorMessage() == STR_ERROR_TBM_CHANGE_NO_CUT);
	CHECK(CmdChangeTBMWork(DoCommandFlag::Execute, rig.Project().id, ChangeFrom(), ChangeTo(), 0, 0, ExcavationPlanIntent::Area).Failed());
	CHECK(TBMFaceSnapshot::Capture(rig) == before);
}

/** Wrong ownership, withdrawal, malformed intent and stale review all refuse before publishing claims or support. */
TEST_CASE("Native work change refuses invalid or unreviewed work atomically", "[tbm-change-work]")
{
	TBMRemainderRig rig;
	rig.Bore(2);
	const auto id = rig.Project().id;
	const auto quote = PreviewTBMWorkChange(_current_company, id, ChangeFrom(), ChangeTo());
	REQUIRE(quote.result.Succeeded());
	StringID expected = STR_ERROR_TBM_CHANGE_STALE;
	auto intent = ExcavationPlanIntent::Area;
	auto fingerprint = quote.fingerprint;
	auto cursor = quote.cursor;
	CompanyID actor = _current_company;
	SECTION("another company") { actor = CompanyID{1}; expected = STR_ERROR_TBM_EXTEND_REFERENCES; }
	SECTION("being released") {
		REQUIRE(ControlTBMProject(id, TBMProjectAction::Cancel, true) == TBMProjectError::None);
		expected = STR_ERROR_TBM_PROJECT_FINISHED;
	}
	SECTION("malformed intent") { intent = static_cast<ExcavationPlanIntent>(UINT8_MAX); expected = STR_ERROR_TBM_COMMISSION_AREA; }
	SECTION("unreviewed quantities") { fingerprint ^= 1; }
	SECTION("unreviewed completed count") { ++cursor; }
	const auto before = TBMFaceSnapshot::Capture(rig);
	const auto job = rig.Job();
	const auto project = rig.Project();
	CommandCost result;
	{
		// Only the command runs as the foreign actor; physical snapshots still inspect the real owning company.
		AutoRestoreBackup company(_current_company, actor);
		result = CmdChangeTBMWork(DoCommandFlag::Execute, id, ChangeFrom(), ChangeTo(), cursor, fingerprint, intent);
	}
	CHECK(result.Failed());
	CHECK(result.GetErrorMessage() == expected);
	CHECK(TBMFaceSnapshot::Capture(rig) == before);
	CHECK(rig.Job() == job);
	CHECK(rig.Project() == project);
}

/** Actual trains cannot be switched onto a changed face junction by confirming a previously clear quote. */
TEST_CASE("Native work change refuses newly occupied face support", "[tbm-change-work]")
{
	TBMRemainderRig rig;
	rig.Bore(2);
	const auto id = rig.Project().id;
	const auto face = rig.Job().route[rig.Job().cursor - 1].cutter_position;
	// The default new area may keep the face exit. Inject the shared native missing-piece fault to require a junction.
	TBMFacePieceFault fault(face);
	const auto quote = PreviewTBMWorkChange(_current_company, id, ChangeFrom(), ChangeTo());
	INFO(Refusal(quote));
	REQUIRE(quote.result.Succeeded());
	REQUIRE(quote.face_support.pieces.size() == 1);
	REQUIRE(quote.face_support.pieces.front().tile == face);
	rig.PlaceOnTrack(rig.hauler, face); // Explicit obstruction fault, not an arrival claim.
	const auto before = TBMFaceSnapshot::Capture(rig);
	const auto job = rig.Job();
	CHECK(CmdChangeTBMWork(DoCommandFlag::Execute, id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area).GetErrorMessage() == STR_ERROR_TBM_CHANGE_FACE);
	CHECK(TBMFaceSnapshot::Capture(rig) == before);
	CHECK(rig.Job() == job);
	rig.PlaceInDepot(rig.hauler, rig.depot);
	REQUIRE(CmdChangeTBMWork(DoCommandFlag::Execute, id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
}

/** Replanning a genuinely loaded cutter leaves every packet, physical train and already-paid geological prefix intact. */
TEST_CASE("Native work change preserves actual paid spoil and completed cuts", "[tbm-change-work]")
{
	TBMRemainderRig rig;
	rig.Bore(1);
	rig.BoreOneCut(); // Preserve this paid cut's actual packets aboard; no component exchange follows.
	const auto job = rig.Job();
	const auto quote = PreviewTBMWorkChange(_current_company, rig.Project().id, ChangeFrom(), ChangeTo());
	INFO(Refusal(quote));
	REQUIRE(quote.result.Succeeded());
	REQUIRE(quote.load.earth_t + quote.load.rock_t > 0);
	const auto before = TBMFaceSnapshot::Capture(rig);
	REQUIRE(CmdChangeTBMWork(DoCommandFlag::Execute, rig.Project().id, ChangeFrom(), ChangeTo(), quote.cursor, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
	const auto after = TBMFaceSnapshot::Capture(rig);
	CHECK(after.cutter == before.cutter);
	CHECK(after.hauler == before.hauler);
	CHECK(after.packets == before.packets);
	CHECK(after.facts == before.facts);
	CHECK(after.trains == before.trains);
	CHECK(after.assignments == before.assignments);
	CHECK(after.sets == before.sets);
	CHECK(Company::Get(_current_company)->money == before.digest.money);
	CHECK(rig.Job().completed_work_cost == job.completed_work_cost);
	CHECK(rig.Job().cursor == job.cursor);
	// Every paid cut and its physical floor survives; discarded future work cannot restore consumed rock.
	for (uint32_t index = 0; index < job.cursor; ++index) {
		CHECK(rig.Job().route[index] == job.route[index]);
		// Preserve each cell of this paid sweep, including already open overlap.
		for (TileIndex tile : job.route[index].swept_tiles) CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
	}
}

/** Exact tunnel intent reuses the current next cut, preserving already-paid partial face work. */
TEST_CASE("Native straight work change preserves an identical partial next cut", "[tbm-change-work]")
{
	TBMRemainderRig rig;
	rig.Bore(2);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	REQUIRE(rig.Tick().blocked == TBMBlockedReason::None);
	const auto job = rig.Job();
	REQUIRE(job.partial_work_ticks > 0);
	const auto from = job.route[job.cursor - 1].cutter_position;
	const auto to = job.route.back().cutter_position;
	const auto quote = PreviewTBMWorkChange(_current_company, rig.Project().id, from, to, ExcavationPlanIntent::StraightTunnel);
	INFO(Refusal(quote));
	REQUIRE(quote.result.Succeeded());
	REQUIRE(quote.plan.route.front() == job.route[job.cursor]);
	CHECK(quote.plan.route.back().cutter_position == to);
	CHECK(quote.abandoned_work_ticks == 0);
	CHECK(quote.face_support.pieces.empty());
	REQUIRE(CmdChangeTBMWork(DoCommandFlag::Execute, rig.Project().id, from, to, quote.cursor, quote.fingerprint, ExcavationPlanIntent::StraightTunnel).Succeeded());
	CHECK(rig.Job().partial_work_ticks == job.partial_work_ticks);
	CHECK(rig.Job().completed_work_cost == job.completed_work_cost);
}
