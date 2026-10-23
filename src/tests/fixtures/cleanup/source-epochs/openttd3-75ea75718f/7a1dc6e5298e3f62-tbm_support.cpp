/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_support.cpp Temporary support track (Group 8): the path through straight and turning advances, refusals,
 * real track with saved metadata, and trailing parts that stay inside opened ground through a legal turn. Group 9:
 * ordinary rail tools refuse support in use, removal after release drops the metadata, conversion, traffic classes.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 8 and 9
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "../tbm.h"
#include "../rail.h"
#include "../rail_map.h"
#include "../track_func.h"
#include "../direction_func.h"
#include "../rail_cmd.h"
#include "../landscape_cmd.h"
#include "../tbm_cmd.h"
#include "../command_func.h"
#include "../landscape.h"

#include "../table/strings.h"

#include "../safeguards.h"

/**
 * Independent oracle: the piece joining two tile edges, from the corner names rather than the trackdir tables.
 * @param a One edge.
 * @param b The other edge.
 * @return Axis track for opposite edges, else the corner piece between the two edges.
 */
static Track GetPieceBetweenEdges(DiagDirection a, DiagDirection b)
{
	if (a == ReverseDiagDir(b)) return DiagDirToDiagTrack(a);
	/**
	 * @param x First edge.
	 * @param y Second edge.
	 * @return Whether {a, b} is exactly {x, y}.
	 */
	const auto is = [a, b](DiagDirection x, DiagDirection y) { return (a == x && b == y) || (a == y && b == x); };
	if (is(DiagDirection::NE, DiagDirection::NW)) return Track::Upper; // North corner.
	if (is(DiagDirection::SE, DiagDirection::SW)) return Track::Lower; // South corner.
	if (is(DiagDirection::SW, DiagDirection::NW)) return Track::Left; // West corner.
	return Track::Right; // East corner, between NE and SE.
}

/**
 * Accepted job with a turning route planned from the housing with the consist's four points.
 * @param rig Project rig whose live job supplies owners and pass terms.
 * @param headings Heading of each advance.
 * @param[out] poses Pose after each advance.
 * @return The job copy (never reserved; support preparation only reads it).
 */
static ExcavationJob MakeTurningJob(const TBMProjectRig &rig, std::span<const Direction> headings, std::vector<ExcavationPlanningPose> &poses)
{
	ExcavationJob job = *GetExcavationJob(rig.project.job);
	job.route.clear();
	ExcavationPlanningPose pose{rig.proof.destination, rig.proof.exit_heading, std::vector<TileIndex>(4, rig.proof.destination)};
	// Each heading is one legal advance of the canonical planner, never a stationary rotation.
	for (Direction heading : headings) {
		const auto step = AdvanceExcavationPlanningPose(pose, heading);
		REQUIRE(step.error == ExcavationError::None);
		job.route.push_back(step.cut);
		pose = step.next;
		poses.push_back(pose);
	}
	return job;
}

/** The first piece leaves the portal mouth as the axis track through the door; preparing changes nothing. */
TEST_CASE("The first support piece is the axis track at the portal mouth", "[tbm-support]")
{
	TBMProjectRig rig;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto &job = *GetExcavationJob(rig.project.job);
	const auto plan = PrepareTBMSupport(rig.project, job);
	REQUIRE(plan.error == TBMBlockedReason::None);
	REQUIRE(plan.pieces.size() == 1);
	CHECK(plan.pieces[0].tile == job.route[0].cutter_position);
	CHECK(plan.pieces[0].track == DiagDirToDiagTrack(GetLayerPortalDoorDirection(rig.proof.destination)));
	CHECK(plan.railtype == GetRailType(rig.proof.destination));
	CHECK(IsTileType(plan.pieces[0].tile, TileType::Clear));
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(CaptureTBMSupport().empty());
}

/**
 * A 90-degree change made of two 45-degree advances: support stays edge-connected with matching pieces, every
 * piece and every trailing consist point lies in ground already opened by the cuts so far, so the turn clips nothing.
 */
TEST_CASE("Turning support stays connected and trailing parts stay in opened ground", "[tbm-support]")
{
	TBMProjectRig rig;
	const Direction straight = rig.proof.exit_heading;
	const Direction right = ChangeDir(straight, DirDiff::Right45);
	const Direction across = ChangeDir(right, DirDiff::Right45);
	const std::array headings{straight, straight, right, right, across, across};
	std::vector<ExcavationPlanningPose> poses;
	auto job = MakeTurningJob(rig, headings, poses);

	std::vector<TileIndex> path{rig.proof.destination};
	std::vector<Track> pieces{Track::Invalid};
	std::set<TileIndex> opened;
	// Prepare each cut in turn, as the live job would present it; collect the pieces in path order.
	for (uint32_t cursor = 0; cursor < job.route.size(); ++cursor) {
		job.cursor = cursor;
		opened.insert(job.route[cursor].swept_tiles.begin(), job.route[cursor].swept_tiles.end());
		const auto plan = PrepareTBMSupport(rig.project, job);
		REQUIRE(plan.error == TBMBlockedReason::None);
		REQUIRE_FALSE(plan.pieces.empty());
		CHECK(plan.pieces.back().tile == job.route[cursor].cutter_position);
		// An advance to a corner neighbour lays its side cell and its new position; an axis advance only the position.
		// (Engine "diagonal" directions are the axis ones: NE, SE, SW and NW.)
		const bool corner_step = !IsDiagonalDirection(job.route[cursor].heading);
		CHECK(plan.pieces.size() == (corner_step ? 2U : 1U));
		for (const auto &piece : plan.pieces) {
			CHECK(opened.contains(piece.tile)); // Support never lies outside the ground this cut opens.
			path.push_back(piece.tile);
			pieces.push_back(piece.track);
		}
		// Every trailing consist point after this advance sits on opened ground or in the housing.
		for (TileIndex point : poses[cursor].history) {
			CHECK((opened.contains(point) || point == rig.proof.destination));
		}
	}
	// Neighbouring path tiles share an edge, and each piece joins exactly the edges toward its neighbours.
	for (size_t i = 1; i < path.size(); ++i) {
		const DiagDirection entry = DiagdirBetweenTiles(path[i], path[i - 1]);
		REQUIRE(entry != DiagDirection::Invalid);
		const DiagDirection exit = i + 1 < path.size() ? DiagdirBetweenTiles(path[i], path[i + 1]) : ReverseDiagDir(entry);
		REQUIRE(exit != DiagDirection::Invalid);
		CHECK(pieces[i] == GetPieceBetweenEdges(entry, exit));
	}
	// The pieces at the mouth are axis track; the diagonal stretch uses corner pieces.
	CHECK(IsDiagonalTrack(pieces[1]));
	CHECK_FALSE(IsDiagonalTrack(pieces[pieces.size() - 3]));
}

/** Support never merges with other track, never leaves opened ground and never starts away from the own housing. */
TEST_CASE("Support refuses cells it may not lay on", "[tbm-support]")
{
	TBMWorkRig rig(2);
	const ExcavationJob job = rig.Job();

	/**
	 * @param change Edit to a copy of the job.
	 * @return The prepared plan for the changed job.
	 */
	const auto plan_for = [&](const std::function<void(ExcavationJob &)> &change) {
		ExcavationJob copy = job;
		change(copy);
		return PrepareTBMSupport(rig.Project(), copy);
	};
	// A position that is not a neighbour of the previous one cannot be joined by track.
	auto refused = plan_for([](ExcavationJob &j) { j.route[0].cutter_position = TileAddWrap(j.route[0].cutter_position, 3, 3); });
	CHECK(refused.error == TBMBlockedReason::Obstruction);
	CHECK(refused.pieces.empty());
	// No cut is left to support.
	CHECK(plan_for([](ExcavationJob &j) { j.cursor = static_cast<uint32_t>(j.route.size()); }).error == TBMBlockedReason::Obstruction);
	// A cell that neither this cut opens nor is already open cannot carry support.
	refused = plan_for([](ExcavationJob &j) { j.route[0].swept_tiles.clear(); });
	CHECK(refused.error == TBMBlockedReason::Obstruction);
	CHECK(refused.failure_tile == job.route[0].cutter_position);
	// Another company's project cannot start support from this housing.
	auto foreign = rig.Project();
	foreign.company = CompanyID{static_cast<uint8_t>(foreign.company.base() + 1)};
	CHECK(PrepareTBMSupport(foreign, job).error == TBMBlockedReason::Obstruction);

	/* Once the first cut laid its piece, passing there again reuses it instead of laying it twice. */
	const auto estimate = EstimateExcavationCut(rig.project.job);
	rig.BoreOneCut(); // Cutting duration comes from the actual model, independently of support admission.
	REQUIRE(rig.Job().cursor == 1);
	const auto again = plan_for([](ExcavationJob &j) { j.cursor = 0; });
	CHECK(again.error == TBMBlockedReason::None);
	CHECK(again.pieces.empty());
	const auto next = PrepareTBMSupport(rig.Project(), rig.Job());
	REQUIRE(next.error == TBMBlockedReason::None); // The second cut is still supportable.
	REQUIRE_FALSE(next.pieces.empty());
	// Recurring projects may reuse exact owned, unsignalled track already on
	// excavated floor. Reuse grants no new geology and proposes no duplicate
	// construction charge for that physical piece (Plan5 Group13B).
	const TileIndex second = next.pieces.back().tile;
	MakeRailNormal(second, _current_company, TrackBits{next.pieces.back().track}, GetRailType(rig.proof.destination));
	refused = PrepareTBMSupport(rig.Project(), rig.Job());
	CHECK(refused.error == TBMBlockedReason::None);
	// The existing matching track must stay under its original ordinary owner;
	// newly proposed support pieces cannot overwrite it or charge it again.
	for (const auto &piece : refused.pieces) CHECK(piece.tile != second);
	const Track mismatched = next.pieces.back().track == Track::X ? Track::Y : Track::X;
	MakeRailNormal(second, _current_company, TrackBits{mismatched}, GetRailType(rig.proof.destination));
	refused = PrepareTBMSupport(rig.Project(), rig.Job());
	CHECK(refused.error == TBMBlockedReason::Obstruction);
	CHECK(refused.failure_tile == second);
	MakeRailNormal(second, foreign.company, TrackBits{next.pieces.back().track}, GetRailType(rig.proof.destination));
	refused = PrepareTBMSupport(rig.Project(), rig.Job());
	CHECK(refused.error == TBMBlockedReason::Obstruction);
	CHECK(refused.failure_tile == second);
	MakeRailNormal(second, _current_company, TrackBits{next.pieces.back().track}, RAILTYPE_MONO);
	refused = PrepareTBMSupport(rig.Project(), rig.Job());
	CHECK(refused.error == TBMBlockedReason::Obstruction);
	CHECK(refused.failure_tile == second);
	DoClearSquare(second);
}

/** Laid support is the company's real track, counted as infrastructure, with metadata that allows the work traffic. */
TEST_CASE("Laid support is company track with saved metadata", "[tbm-support]")
{
	TBMWorkRig rig;
	const auto railtype = GetRailType(rig.proof.destination);
	const auto infrastructure = Company::Get(_current_company)->infrastructure.rail[railtype];
	const auto estimate = EstimateExcavationCut(rig.project.job);
	rig.BoreOneCut();
	REQUIRE(rig.Job().cursor == 1);
	const TileIndex cell = rig.Job().route[0].cutter_position;
	CHECK(Company::Get(_current_company)->infrastructure.rail[railtype] == infrastructure + 1);
	REQUIRE(IsPlainRailTile(cell));
	CHECK(GetTileOwner(cell) == _current_company);
	CHECK(GetRailType(cell) == railtype);
	CHECK(GetGeologyWorldState().terrain.Get(cell) == ExcavationState::Excavated); // The cavity stays with geology.
	const auto *support = GetTBMSupportTile(cell);
	REQUIRE(support != nullptr);
	CHECK(support->owner == _current_company);
	CHECK(support->track == GetTrackBits(cell));
	CHECK(support->max_speed == TBM_SUPPORT_MAX_SPEED);
	CHECK(support->conversion_price == RailBuildCost(railtype));
	// The cutter, its wagons and their hauler may use it; Group 14 applies this to moving trains.
	for (const auto vehicle_class : {TBMSupportClass::Cutter, TBMSupportClass::SpoilWagon, TBMSupportClass::Hauler}) {
		CHECK(support->allowed.Test(vehicle_class));
		CHECK(IsTBMSupportTraversable(cell, vehicle_class));
	}
	CHECK(IsTBMSupportTraversable(rig.proof.source, TBMSupportClass::Hauler)); // Ordinary track carries no restriction.
	CHECK(GetTBMSupportTile(rig.proof.destination) == nullptr);
}

/**
 * Bore the rig's only cut so its new cutter cell carries support.
 * @param rig Work rig in Bore.
 * @return The support cell.
 */
static TileIndex BoreFirstCut(const TBMWorkRig &rig)
{
	const auto estimate = EstimateExcavationCut(rig.project.job);
	REQUIRE(estimate.error == ExcavationError::None);
	rig.BoreOneCut();
	REQUIRE(rig.Job().cursor == 1);
	const TileIndex cell = rig.Job().route[0].cutter_position;
	REQUIRE(GetTBMSupportTile(cell) != nullptr);
	return cell;
}

/** While its project is unfinished, support refuses every ordinary change; once released it can go or be converted. */
TEST_CASE("Support in use refuses ordinary rail tools until released", "[tbm-support]")
{
	TBMWorkRig rig;
	const TileIndex cell = BoreFirstCut(rig);
	const Track laid = *GetTrackBits(cell).begin();
	const Track other = laid == Track::X ? Track::Y : Track::X;
	CHECK(IsTBMSupportInUse(cell));

	/**
	 * @param cost Result of an ordinary tool.
	 * @return Whether it was the in-use refusal.
	 */
	const auto in_use = [](const CommandCost &cost) { return cost.Failed() && cost.GetErrorMessage() == STR_ERROR_TBM_SUPPORT_IN_USE; };
	CHECK(in_use(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, cell, laid)));
	CHECK(in_use(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, cell)));
	CHECK(in_use(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, cell, GetRailType(cell), other, false)));
	CHECK(in_use(Command<Commands::ConvertRail>::Do(DoCommandFlag::Execute, cell, cell, GetRailType(cell), false)));
	CHECK(in_use(Command<Commands::ConvertTBMSupport>::Do(DoCommandFlag::Execute, cell)));
	CHECK(GetTrackBits(cell) == TrackBits{laid});
	REQUIRE(GetTBMSupportTile(cell) != nullptr);
	CHECK(Command<Commands::ConvertTBMSupport>::Do(DoCommandFlag::Execute, rig.proof.destination).GetErrorMessage() == STR_ERROR_TBM_SUPPORT_NONE);

	SECTION("company removal clears the track and its metadata even in use") {
		CHECK(Command<Commands::RemoveRail>::Do({DoCommandFlag::Execute, DoCommandFlag::Bankrupt}, cell, laid).Succeeded());
		CHECK(GetTBMSupportTile(cell) == nullptr);
	}
	SECTION("after the project ends, removal drops the metadata and the cavity stays open") {
		REQUIRE(rig.Job().state == ExcavationJobState::Completed);
		rig.Reload(TBMPhase::Completed);
		CHECK_FALSE(IsTBMSupportInUse(cell));
		CHECK(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, cell, laid).Succeeded());
		CHECK(GetTBMSupportTile(cell) == nullptr);
		CHECK(CaptureTBMSupport().empty());
		CHECK(GetGeologyWorldState().terrain.Get(cell) == ExcavationState::Excavated);
	}
	SECTION("after the project ends, conversion keeps the track for its price") {
		REQUIRE(rig.Job().state == ExcavationJobState::Completed);
		rig.Reload(TBMPhase::Completed);
		const Money price = GetTBMSupportTile(cell)->conversion_price;
		const Money cash = Company::Get(_current_company)->money;
		const auto estimate = Command<Commands::ConvertTBMSupport>::Do(DoCommandFlags{}, cell);
		REQUIRE(estimate.Succeeded());
		CHECK(estimate.GetCost() == price);
		CHECK(GetTBMSupportTile(cell) != nullptr); // An estimate changes nothing.
		REQUIRE(Command<Commands::ConvertTBMSupport>::Do(DoCommandFlag::Execute, cell).Succeeded());
		CHECK(GetTBMSupportTile(cell) == nullptr);
		CHECK(GetTrackBits(cell) == TrackBits{laid});
		CHECK(Company::Get(_current_company)->money == cash); // Command::Do does not pay; the network command path does.
	}
}

/** The project's machinery runs as the cutter and its wagons as spoil wagons; other trains have no class on support. */
TEST_CASE("Support traffic classes follow the consist roles", "[tbm-support]")
{
	TBMWorkRig rig;
	const TileIndex cell = BoreFirstCut(rig);
	CHECK(GetTBMSupportClass(*rig.front) == TBMSupportClass::Cutter);
	CHECK(GetTBMSupportClass(*rig.front->Next()) == TBMSupportClass::Cutter); // Permanent support part.
	for (const Train *wagon : rig.Wagons(TBMConsistRole::EarthWagon)) CHECK(GetTBMSupportClass(*wagon) == TBMSupportClass::SpoilWagon);
	for (const Train *wagon : rig.Wagons(TBMConsistRole::RockWagon)) CHECK(GetTBMSupportClass(*wagon) == TBMSupportClass::SpoilWagon);
	// Every part of the owning consist may run on its support; ordinary tiles carry everything.
	for (const Train *part = rig.front; part != nullptr; part = part->Next()) {
		CHECK(MayTBMSupportCarry(*part, cell));
		CHECK(MayTBMSupportCarry(*part, rig.proof.source));
	}
	// Without a project the same train is ordinary freight, which temporary support does not carry.
	const auto saved = CaptureTBMProjects();
	const auto support = CaptureTBMSupport();
	const auto next = GetNextTBMProjectID();
	ResetTBMProjectState();
	CHECK_FALSE(GetTBMSupportClass(*rig.front).has_value());
	StageLoadedTBMSupport(support);
	StageLoadedTBMProjects(saved, next);
	REQUIRE(AcceptLoadedTBMProjects());
	CHECK(MayTBMSupportCarry(*rig.front, cell));
}
