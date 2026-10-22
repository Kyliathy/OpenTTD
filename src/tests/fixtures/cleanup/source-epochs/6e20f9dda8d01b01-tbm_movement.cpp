/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_movement.cpp Real movement of a boring machine and its hauler through their project pair (master Group
 * 7B). The cutter consist leaves its magazine at the Surface key when started, drives through the shaft to its face,
 * advances on its own support, backs into the magazine for each exchange and leaves again ([tbm-movement]). Once its
 * project retires and the pair is opened, ordinary freight drives into the tunnel it bored. An assigned hauler with
 * one order to the origin is steered back to the magazine after every delivery, so the whole route is carried out by
 * real trains, from the rig's project or one commissioned through the player's command ([tbm-loop]). Only the trains'
 * own ticks move them, through the game's vehicle step in the loop and whole-life tests; the TBM controller and the
 * hauler cycle run inside them. A Commissioning pair shows its track to the project's own trains only.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-m-movement-design.md, decisions 1-7
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 19
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 8 and 11 (paid support and bend reload)
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 15
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group4; physical travel holds and lost-target recovery.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "spoil_receiver_fixture.h"
#include "movement_fixture.h"
#include "../vehicle_cmd.h"
#include "../track_func.h"
#include "../signal_func.h"
#include "../pathfinder/yapf/yapf_cache.h"
#include "../town_kdtree.h"
#include "../tbm_controls.h"
#include "../table/strings.h"
#include "../timer/timer_game_economy.h"

#include "../safeguards.h"

/**
 * @param project Live project.
 * @return Its phase, blocked reason and cut cursor.
 */
static std::string DescribeProject(const TBMProject &project)
{
	const ExcavationJob *job = GetExcavationJob(project.job);
	return fmt::format("phase {} blocked {} cursor {}", to_underlying(project.phase), to_underlying(project.blocked), job == nullptr ? std::string{"-"} : fmt::format("{}", job->cursor));
}

/**
 * A fixture town in the town index for one test, so a command that names what it builds after the nearest town (a
 * depot) finds it. Declare it after the rig that owns the town, so it leaves the index before the town is deleted.
 */
class TownIndexEntry {
	TownID id; ///< Indexed town.
public:
	/** @param town Fixture town to index. */
	explicit TownIndexEntry(const Town &town) : id(town.index) { _town_kdtree.Insert(this->id); }

	/** Take the town out of the index again. */
	~TownIndexEntry() { _town_kdtree.Remove(this->id); }
};

/** A published project whose real train drives itself; the controller steps inside the train's own ticks. */
class TBMMovementRig : public TBMProjectRig {
public:
	TileIndex source = this->fixture.request.portal; ///< Surface pair key: the magazine and Source bay.
	TileIndex rock_end = this->proof.destination; ///< Rock end of the pair: the first face approach.

	/**
	 * Publish a project with straight cuts; its consist stands stopped in the magazine, as commissioning leaves it.
	 * @param segments Straight cuts out of the door.
	 */
	explicit TBMMovementRig(size_t segments) : TBMProjectRig(segments) {}

	/**
	 * Publish a project whose route turns as given; its consist stands stopped in the magazine.
	 * @param turns Change of heading at each advance (see TBMProjectRig).
	 */
	explicit TBMMovementRig(const std::vector<DirDiff> &turns) : TBMProjectRig(turns) {}

	/** @return The live project. */
	const TBMProject &Project() const { return *GetTBMProject(this->project.id); }

	/** @return The live job. */
	const ExcavationJob &Job() const { return *GetExcavationJob(this->project.job); }

	/** Start the consist with the player's ordinary start command (R2MLVS3MP-M decision 7). */
	void Start() const
	{
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, this->front->index, true).Succeeded());
	}

	/** @return The project state and every part's tile, track, position, facing and visibility, for a failed drive. */
	std::string Describe() const
	{
		std::string text = fmt::format("source {} rock end {}", this->source.base(), this->rock_end.base());
		return text + "\n" + DescribeProject(this->Project()) + "\n" + DescribeTrain(this->front);
	}

	/** Verify that real trailing parts stay on paid, excavated support throughout movement and backing. */
	void CheckPaidSupport() const
	{
		REQUIRE(IsTrainOnItsTiles(this->front));
		// Check the whole physical chain after each complete train tick: a safe cutter alone cannot prove wagon clearance.
		for (const Train *part = this->front; part != nullptr; part = part->Next()) {
			if (GetTileLayer(part->tile) == MapLayer::Surface) continue;
			// The sealed housing is the controlled shaft exception; every ordinary rock-layer track cell must already be cut.
			if (IsTileType(part->tile, TileType::LayerPortal)) continue;
			REQUIRE(GetGeologyWorldState().terrain.Get(part->tile) == ExcavationState::Excavated);
			REQUIRE(IsTBMSupportUsable(this->Project(), part->tile));
		}
	}

	/**
	 * Reload unchanged controller/support records while the real consist stands on a bend.
	 * This is the producer staging boundary; complete serialized vehicle/file acceptance remains master checkpoint 7.4.
	 */
	void ReloadOnBend() const
	{
		const auto before = this->Digest();
		const auto packets = GetTBMPacketOwners(this->front);
		const auto positions = DescribeTrain(this->front);
		const auto digest = ComputeTBMProjectDigest();
		auto projects = CaptureTBMProjects();
		auto support = CaptureTBMSupport();
		const auto next = GetNextTBMProjectID();
		ResetTBMProjectState();
		StageLoadedTBMProjects(std::move(projects), next);
		StageLoadedTBMSupport(std::move(support));
		REQUIRE(AcceptLoadedTBMProjects());
		CHECK(ComputeTBMProjectDigest() == digest);
		CHECK(this->Digest() == before);
		CHECK(GetTBMPacketOwners(this->front) == packets);
		CHECK(DescribeTrain(this->front) == positions);
		this->CheckPaidSupport();
	}

	/**
	 * Tick the real train until a condition holds; the train must never crash on the way.
	 * @param done Condition on the live project and train.
	 * @param limit Most ticks to try.
	 * @return Ticks taken.
	 */
	uint Drive(const std::function<bool()> &done, uint limit = 20000) const
	{
		uint ticks = 0;
		// One Train::Tick is the game's whole step for this train: the controller step, then the loco handler twice.
		while (!done() && ticks < limit) {
			REQUIRE(this->front->Tick());
			REQUIRE_FALSE(this->front->vehstatus.Test(VehState::Crashed));
			this->CheckPaidSupport();
			++ticks;
		}
		INFO(this->Describe());
		REQUIRE(done());
		return ticks;
	}

	/**
	 * Tick the real train a fixed number of times; it must never crash.
	 * @param ticks Ticks to run.
	 */
	void Run(uint ticks) const
	{
		// Whole train ticks preserve the trailing-part safety check during timed holds as well as travel.
		for (uint tick = 0; tick < ticks; ++tick) {
			REQUIRE(this->front->Tick());
			REQUIRE_FALSE(this->front->vehstatus.Test(VehState::Crashed));
			this->CheckPaidSupport();
		}
	}

	/** @return Whether every part stands in the depot state on the magazine tile. */
	bool IsWhollyInMagazine() const
	{
		for (const Train *part = this->front; part != nullptr; part = part->Next()) {
			if (part->tile != this->source || part->track != Track::Depot) return false;
		}
		return true;
	}
};

/** @param front Actual attached train. @return Every permanent/wagon pose, excluding speed so braking to a safe hold does not imply movement. */
static auto CaptureTBMMovementPositions(const Train *front)
{
	std::vector<std::tuple<VehicleID, TileIndex, TrackBits, int, int, int, Direction>> positions;
	// All attached parts must stay put when work travel is paused or unsafe;
	// checking only the head would miss wagons rolling out of a magazine.
	for (const Train *part = front; part != nullptr; part = part->Next()) {
		positions.emplace_back(part->index, part->tile, part->track, part->x_pos, part->y_pos, part->z_pos, part->direction);
	}
	return positions;
}

/** Restore an explicitly injected fault on an unoccupied paid target, even when a physical assertion fails. */
class TBMTargetTrackFault {
	TileIndex tile; ///< Real vacated target whose native railway fields are changed for recovery characterization.
	TrackBits tracks; ///< Actual laid pieces to restore after the temporary fault.
	RailType railtype; ///< Actual compatible rail type before the fault.
	Owner owner; ///< Actual project company before the fault.
public:
	/** @param tile Real plain-rail target; no train part is moved or edited by this scope. */
	explicit TBMTargetTrackFault(TileIndex tile) : tile(tile), tracks(GetTrackBits(tile)), railtype(GetRailType(tile)), owner(GetTileOwner(tile)) {}
	/** Restore only the injected infrastructure fields and invalidate derived pathfinder data. */
	~TBMTargetTrackFault()
	{
		SetTrackBits(this->tile, this->tracks);
		SetRailType(this->tile, this->railtype);
		SetTileOwner(this->tile, this->owner);
		YapfNotifyTrackLayoutChange(this->tile, Track::Invalid);
	}
};

/** A project pause issued during real travel must hold every actual part and resume the same route. */
TEST_CASE("Pausing a travelling TBM freezes the whole real consist", "[tbm-travel-hold]")
{
	PathfinderDefaults pathfinder;
	TBMMovementRig rig(5);
	rig.Start();
	/** @return Whether the actual train has left storage and is moving toward its first face. */
	const auto travelling = [&] { return rig.Project().phase == TBMPhase::TravelToFace && !rig.IsWhollyInMagazine() && rig.front->cur_speed != 0; };
	rig.Drive(travelling);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, rig.project.id, TBMProjectAction::Pause).Succeeded());
	const auto positions = CaptureTBMMovementPositions(rig.front);
	const auto job = rig.Job();
	const auto packets = GetTBMPacketOwners(rig.front);
	rig.Run(64);
	CHECK(CaptureTBMMovementPositions(rig.front) == positions);
	CHECK(rig.front->cur_speed == 0);
	CHECK(rig.Project().blocked == TBMBlockedReason::Paused);
	CHECK(rig.Job() == job);
	CHECK(GetTBMPacketOwners(rig.front) == packets);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, rig.project.id, TBMProjectAction::Resume).Succeeded());
	/** @return Whether ordinary native motion has reached the unchanged accepted face after the player's resume. */
	const auto arrived = [&] { return rig.Project().phase == TBMPhase::Bore; };
	rig.Drive(arrived);
	CHECK(rig.front->tile == rig.rock_end);
}

/** Missing or incompatible target infrastructure must hold travel visibly and recover without moving a train by hand. */
TEST_CASE("A TBM waits for its actual paid target to become usable again", "[tbm-travel-hold]")
{
	PathfinderDefaults pathfinder;
	TBMMovementRig rig(5);
	rig.Start();
	/** @return Whether the first cut is genuinely paid but its new target has not yet been entered by the moving head. */
	const auto advancing = [&] { return rig.Project().phase == TBMPhase::AdvanceSupport && rig.Job().cursor == 1 && rig.front->tile != rig.Job().route.front().cutter_position; };
	rig.Drive(advancing);
	const TileIndex target = rig.Job().route.front().cutter_position;
	REQUIRE(IsPlainRailTile(target));
	for (const Train *part = rig.front; part != nullptr; part = part->Next()) REQUIRE(part->tile != target); // Fault injection may not alter track beneath any part.
	const auto positions = CaptureTBMMovementPositions(rig.front);
	const auto job = rig.Job();
	const auto packets = GetTBMPacketOwners(rig.front);
	{
		TBMTargetTrackFault restore(target);
		SECTION("missing laid piece") { SetTrackBits(target, {}); }
		SECTION("foreign track owner") { SetTileOwner(target, OWNER_TOWN); }
		SECTION("incompatible rail technology") { SetRailType(target, RAILTYPE_MAGLEV); }
		YapfNotifyTrackLayoutChange(target, Track::Invalid);
		CHECK_FALSE(GetTBMCutterSteeringTarget(*rig.front).has_value());
		rig.Run(32);
		CHECK(CaptureTBMMovementPositions(rig.front) == positions);
		CHECK(rig.front->cur_speed == 0);
		CHECK(rig.Project().blocked == TBMBlockedReason::MissingSupport);
		CHECK(rig.Job() == job);
		CHECK(GetTBMPacketOwners(rig.front) == packets);
	}
	/** @return Whether the normal controller recognizes restored support and physically arrives without a progress reset. */
	const auto recovered = [&] { return rig.Project().phase == TBMPhase::Bore && rig.Project().blocked == TBMBlockedReason::None; };
	rig.Drive(recovered);
	CHECK(rig.front->tile == target);
	CHECK(rig.Job().cursor == job.cursor);
}

/** An actual depot order interrupts a partial face safely and rejoins it by physical movement with the same work and cargo. */
TEST_CASE("A TBM leaves its work hold for a real depot visit and returns to the face", "[tbm-travel-service]")
{
	PathfinderDefaults pathfinder;
	TBMMovementRig rig(5);
	const DiagDirection door = GetLayerPortalDoorDirection(rig.source);
	const TileIndex approach = GetLayerPortalDoorTile(rig.source);
	const TileIndex beyond = TileAddByDiagDir(approach, door);
	const TileIndex depot = TileAddByDiagDir(beyond, door);
	// The service destination is an actual constructed depot reached over
	// real Surface rail, never a portal tile cast into a depot identity.
	for (TileIndex tile : {approach, beyond}) REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, DiagDirToDiagTrack(door), false).Succeeded());
	REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, depot, RAILTYPE_RAIL, ReverseDiagDir(door)).Succeeded());
	UpdateSignalsInBuffer();
	rig.Start();
	const bool loaded = GENERATE(false, true);
	/** @return Whether native movement has reached the initial face or the next partially worked face with real spoil aboard. */
	const auto boring = [&] { return rig.Project().phase == TBMPhase::Bore && rig.Job().cursor == (loaded ? 1U : 0U); };
	rig.Drive(boring);
	rig.Run(3);
	const auto job = rig.Job();
	const TileIndex face = rig.front->tile;
	REQUIRE(job.partial_work_ticks != 0);
	const auto packets = GetTBMPacketOwners(rig.front);
	const auto cutter = rig.front->index;
	bool halt = false;
	bool pause = false;
	SECTION("service and continue") {}
	SECTION("halt then start from the depot") { halt = true; }
	SECTION("pause work during service then resume from the depot") { pause = true; halt = true; }
	const auto flags = halt ? DepotCommandFlags{} : DepotCommandFlags{DepotCommandFlag::Service};
	REQUIRE(Command<Commands::SendVehicleToDepot>::Do(DoCommandFlag::Execute, cutter, flags, VehicleListIdentifier{}).Succeeded());
	if (pause) REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, rig.project.id, TBMProjectAction::Pause).Succeeded());
	CHECK_FALSE(IsTBMHoldingTrain(*rig.front));
	/** @return Whether every actual part is in the real depot after its ordinary arrival handling. */
	const auto stored = [&] {
		for (const Train *part = rig.front; part != nullptr; part = part->Next()) {
			if (part->tile != depot || part->track != Track::Depot) return false;
		}
		return !rig.front->current_order.IsType(OT_GOTO_DEPOT);
	};
	/** @param done Actual physical arrival predicate. Drive native vehicle ticks, including ordinary depot arrival/service callbacks. */
	const auto drive = [&](const std::function<bool()> &done) {
		uint ticks = 0;
		while (!done() && ticks < 20000) {
			CallVehicleTicks();
			REQUIRE_FALSE(rig.front->vehstatus.Test(VehState::Crashed));
			rig.CheckPaidSupport();
			++ticks;
		}
		INFO(rig.Describe());
		REQUIRE(done());
	};
	drive(stored);
	CHECK(rig.Job() == job);
	CHECK(GetTBMPacketOwners(rig.front) == packets);
	if (pause) REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, rig.project.id, TBMProjectAction::Resume).Succeeded());
	if (halt) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, cutter, true).Succeeded());
	/** @return Whether the original actual cutter has physically returned to its accepted work face. */
	const auto rejoined = [&] { return rig.front->tile == face && rig.Project().phase == TBMPhase::Bore; };
	drive(rejoined);
	CHECK(rig.front->index == cutter);
	CHECK(rig.Job().id == job.id);
	CHECK(rig.Job().cursor == job.cursor);
	CHECK(rig.Job().partial_work_ticks == job.partial_work_ticks);
	CHECK(GetTBMPacketOwners(rig.front) == packets);
}

/** The whole route by real movement: out to the face, along the new support, back for each exchange, and out again. */
TEST_CASE("A started cutter drives to its face, works its route and backs into its magazine", "[tbm-movement]")
{
	PathfinderDefaults pathfinder;
	std::vector<DirDiff> turns(3, DirDiff::Same);
	bool bending = false;
	bool reload_bend = false;
	SECTION("on a straight route, exchanged once at its end") {}
	SECTION("on a bending route, exchanged before its end too") {
		// The bend's diagonal cuts overflow one load, so the consist leaves its magazine again with empties.
		turns = {DirDiff::Same, DirDiff::Right45, DirDiff::Right45};
		bending = true;
	}
	SECTION("on a bending route with a saved controller continuation") {
		turns = {DirDiff::Same, DirDiff::Right45, DirDiff::Right45};
		bending = true;
		reload_bend = true;
	}
	TBMMovementRig rig(turns);
	const auto route = rig.Job().route;
	const auto breakpoints = rig.Job().pass_terms->exchange_after;
	const DiagDirection shaft = GetLayerPortalShaftDirection(rig.source);
	// Commissioning leaves the saved heading at the first cut's: out through the door.
	CHECK(rig.Project().heading == route.front().heading);
	REQUIRE(route.size() == 3);
	// Only the bend has an exchange before the end; the straight route's wagons hold all of its spoil.
	REQUIRE((breakpoints.size() >= 2) == bending);
	REQUIRE(rig.Project().phase == TBMPhase::Commissioning);
	REQUIRE(IsTrainStoppedAtPortalSite(*rig.front));

	// A commissioned consist waits, stopped and wholly inside, until the player starts it.
	rig.Run(20);
	CHECK(rig.Project().phase == TBMPhase::Commissioning);
	CHECK(rig.IsWhollyInMagazine());

	rig.Start();
	const uint first_travel = rig.Drive([&rig] { return rig.Project().phase == TBMPhase::Bore; });
	INFO("first travel " << first_travel << " ticks");
	// The cutter leads through the shaft and stops at its first face, the rock end; the rest of it trails behind.
	CHECK(rig.front->tile == rig.rock_end);
	CHECK(rig.front->cur_speed == 0);
	CHECK(IsTBMHoldingTrain(*rig.front));
	for (const Train *part = rig.front; part != nullptr; part = part->Next()) {
		CHECK((part->tile == rig.rock_end || part->tile == rig.source));
	}

	for (uint32_t cut = 0; cut < route.size(); ++cut) {
		// Each cut is worked where the controller stopped the train: the rock end, then the end of the newest support.
		const TileIndex approach = cut == 0 ? rig.rock_end : route[cut - 1].cutter_position;
		INFO("cut " << cut);
		REQUIRE(rig.Project().phase == TBMPhase::Bore);
		CHECK(rig.front->tile == approach);
		// The third cut starts on the actual first bend; preserve partial work, paid support and wagon custody across reload.
		if (reload_bend && cut == 2) {
			REQUIRE(rig.front->tile == route[1].cutter_position);
			REQUIRE(rig.Project().heading == route[1].heading);
			const auto estimate = EstimateExcavationCut(rig.project.job);
			REQUIRE(estimate.error == ExcavationError::None);
			rig.Run(static_cast<uint>(estimate.work_ticks / 2));
			REQUIRE(rig.Job().cursor == cut);
			REQUIRE(rig.Job().partial_work_ticks != 0);
			rig.ReloadOnBend();
		}
		rig.Drive([&rig, cut] { return rig.Job().cursor == cut + 1; });

		const bool exchange = std::ranges::find(breakpoints, static_cast<uint16_t>(cut + 1)) != breakpoints.end();
		if (!exchange) {
			// A cut that is no exchange breakpoint drives the train onto its new support, where it stops to work.
			REQUIRE(rig.Project().phase == TBMPhase::AdvanceSupport);
			CHECK_FALSE(IsTBMHoldingTrain(*rig.front));
			rig.Drive([&rig] { return rig.Project().phase == TBMPhase::Bore; });
			continue;
		}

		// An exchange breakpoint backs the consist along its support and through the shaft into the magazine.
		REQUIRE(rig.Project().phase == TBMPhase::ReturnForExchange);
		// However overdue its service, a working machine never asks for a depot; its bay services it instead.
		const TimerGameEconomy::Date today = TimerGameEconomy::date;
		rig.front->date_of_last_service = today - 1000;
		CHECK_FALSE(rig.front->NeedsAutomaticServicing());
		rig.Drive([&rig] { return rig.Project().phase == TBMPhase::WaitForEmpties; });
		CHECK(rig.IsWhollyInMagazine());
		CHECK(rig.front->date_of_last_service == today);
		CHECK_FALSE(rig.front->vehicle_flags.Test(VehicleFlag::DrivingBackwards));
		CHECK(rig.front->cur_speed == 0);
		// It backed in, so it still faces its face, ready to leave head first. The saved heading is the last committed
		// cut's, which a bend turns; backing never changed it.
		CHECK(rig.front->direction == DiagDirToDir(shaft));
		CHECK(rig.Project().heading == route[cut].heading);

		// Stand-in for the exchange, which the hauler loop (step 2) drives: the wagons are emptied where they stand.
		for (Train *part = rig.front; part != nullptr; part = part->Next()) part->cargo.Truncate();
		if (cut + 1 == route.size()) {
			rig.Drive([&rig] { return rig.Project().phase == TBMPhase::Completed; });
			break;
		}
		// With empties aboard it leaves the magazine through the shaft again and drives to the end of its support.
		rig.Drive([&rig] { return rig.Project().phase == TBMPhase::Bore; });
	}
	CHECK(rig.Project().phase == TBMPhase::Completed);
	CHECK(rig.Job().cursor == route.size());

	// A finished project's consist no longer passes the housing: started or not, it stays in the magazine.
	rig.Run(200);
	CHECK(rig.IsWhollyInMagazine());
}

/**
 * @param train Front engine.
 * @return Every part's world position, in chain order.
 */
static std::vector<std::pair<int, int>> PartPositions(const Train *train)
{
	std::vector<std::pair<int, int>> positions;
	for (const Train *part = train; part != nullptr; part = part->Next()) positions.emplace_back(part->x_pos, part->y_pos);
	return positions;
}

/**
 * A cancellation while the consist straddles the pair takes it back whole into its magazine, with or without spoil, and
 * a player's stop on the way back holds the retreat until the train is started again (Plan 4 14.3).
 */
TEST_CASE("Cancelling a consist that straddles the pair takes it back into its magazine", "[tbm-movement]")
{
	PathfinderDefaults pathfinder;
	TBMMovementRig rig(3);
	rig.Start();
	rig.Drive([&rig] { return rig.Project().phase == TBMPhase::Bore; });
	/** Finish the first cut and drive onto its support, so the second cut starts with spoil aboard. */
	const auto second_cut = [&rig]() {
		rig.Drive([&rig] { return rig.Job().cursor == 1; });
		REQUIRE(rig.Project().phase == TBMPhase::AdvanceSupport);
		rig.Drive([&rig] { return rig.Project().phase == TBMPhase::Bore; });
	};
	bool spoil = false;
	bool stop_on_the_way = false;
	SECTION("in the first cut, at the rock end") {}
	SECTION("in the second cut, with spoil aboard") {
		second_cut();
		spoil = true;
	}
	SECTION("in the second cut, stopped by the player on the way back") {
		second_cut();
		spoil = true;
		stop_on_the_way = true;
	}
	// Part of the current cut, so the cancellation has unfinished work to drop.
	rig.Run(100);
	REQUIRE(rig.Job().partial_work_ticks != 0);
	bool on_key = false, beyond = false;
	// The consist straddles the pair: some parts on the key, the others at the rock end or on the support beyond it.
	for (const Train *part = rig.front; part != nullptr; part = part->Next()) {
		on_key |= part->tile == rig.source;
		beyond |= part->tile != rig.source;
	}
	INFO(rig.Describe());
	REQUIRE(on_key);
	REQUIRE(beyond);

	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, rig.project.id, TBMProjectAction::Cancel).Succeeded());
	CHECK(rig.Job().partial_work_ticks != 0);
	CHECK(rig.Project().phase == TBMPhase::ReturnToRelease);
	if (stop_on_the_way) {
		// Halfway back the player stops the train: the retreat holds, blocked, and nothing moves until it is started.
		rig.Drive([&rig] { return rig.front->vehicle_flags.Test(VehicleFlag::DrivingBackwards) && rig.front->cur_speed > 0; });
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, rig.front->index, true).Succeeded());
		rig.Drive([&rig] { return rig.front->cur_speed == 0; });
		rig.Run(2);
		CHECK(rig.Project().blocked == TBMBlockedReason::Stopped);
		CHECK(rig.Project().phase == TBMPhase::ReturnToRelease);
		CHECK_FALSE(rig.IsWhollyInMagazine());
		const auto held = PartPositions(rig.front);
		rig.Run(200);
		CHECK(PartPositions(rig.front) == held);
		rig.Start();
	}
	rig.Drive([&rig] { return rig.IsWhollyInMagazine() && rig.front->cur_speed == 0; });
	CHECK_FALSE(IsTBMCorridorOccupied(rig.Project()));
	CHECK_FALSE(rig.front->vehicle_flags.Test(VehicleFlag::DrivingBackwards));
	if (spoil) {
		// The spoil of the committed cut is still aboard, so the consist waits in its bay for empties before it ends.
		rig.Drive([&rig] { return rig.Project().phase == TBMPhase::WaitToRelease; });
		CHECK(TBMExchangeRig::Aboard(rig.front) != 0);
		for (Train *part = rig.front; part != nullptr; part = part->Next()) part->cargo.Truncate();
	}
	rig.Drive([&rig] { return rig.Project().phase == TBMPhase::Cancelled; });
	CHECK(rig.Job().state == ExcavationJobState::Cancelled);
	// A cancelled project's consist no longer passes the housing.
	rig.Run(200);
	CHECK(rig.IsWhollyInMagazine());
}

/**
 * The whole life of a pair by real movement (Plan 3 15.2): the TBM bores its route and retreats; the retired pair is
 * sealed and turns an ordinary freight train back at its door; once opened, the same train crosses it into the tunnel
 * the TBM bored, to the end of its last cut, and comes back.
 */
TEST_CASE("A freight train crosses the tunnel its TBM bored once the pair is opened", "[tbm-movement]")
{
	PathfinderDefaults pathfinder;
	TBMMovementRig rig(3);
	const auto route = rig.Job().route;
	rig.Start();
	// The whole route by real movement. At each exchange, empties are put aboard where the consist stands, a stand-in
	// for the hauler loop that [tbm-loop] drives.
	rig.Drive([&rig] {
		if (rig.Project().phase == TBMPhase::WaitForEmpties) {
			for (Train *part = rig.front; part != nullptr; part = part->Next()) part->cargo.Truncate();
		}
		return rig.Project().phase == TBMPhase::Completed;
	}, 60000);
	REQUIRE(rig.IsWhollyInMagazine());

	// Retire the finished project: the player stops the consist, the pair is sealed again, and the cutter is sold where
	// it stands. The released support becomes permanent track.
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, rig.front->index, true).Succeeded());
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, rig.project.id, TBMProjectAction::Retire).Succeeded());
	REQUIRE(GetLayerPortalWorkState(rig.source) == LayerPortalWorkState::Sealed);
	REQUIRE(Command<Commands::SellVehicle>::Do(DoCommandFlag::Execute, rig.front->index, true, false, ClientID::Invalid).Succeeded());
	rig.front = nullptr;
	for (const auto &support : CaptureTBMSupport()) REQUIRE(Command<Commands::ConvertTBMSupport>::Do(DoCommandFlag::Execute, support.tile).Succeeded());

	// A line from the pair's door to a depot facing back up it, and a freight train in the depot.
	const DiagDirection door = GetLayerPortalDoorDirection(rig.source);
	const Track axis = DiagDirToDiagTrack(door);
	const TileIndex approach = GetLayerPortalDoorTile(rig.source);
	const TileIndex beyond = TileAddByDiagDir(approach, door);
	const TileIndex depot = TileAddByDiagDir(beyond, door);
	for (const TileIndex tile : {approach, beyond}) {
		REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, axis, false).Succeeded());
	}
	REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, depot, RAILTYPE_RAIL, ReverseDiagDir(door)).Succeeded());
	UpdateSignalsInBuffer();
	rig.fixture.stock->info.cargo_type = GetCargoTypeByLabel(CT_PASSENGERS);
	Train *freight = BuildTrainInDepot(depot, rig.fixture.stock, Engine::Get(GetTBMEngine(2)), 3);
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, freight->index, true).Succeeded());

	/** @return Whether every part of the freight train stands inside its depot. */
	const auto home = [freight, depot]() {
		for (const Train *part = freight; part != nullptr; part = part->Next()) {
			if (part->tile != depot || part->track != Track::Depot) return false;
		}
		return true;
	};
	/**
	 * @param tile Tile to look at.
	 * @return Whether a part of the freight train stands on it.
	 */
	const auto reaches = [freight](TileIndex tile) {
		for (const Train *part = freight; part != nullptr; part = part->Next()) {
			if (part->tile == tile) return true;
		}
		return false;
	};
	/**
	 * Run the game's vehicle step until a condition holds; the train must never crash or leave the tiles it stands on.
	 * @param done Condition.
	 * @param open Whether the pair is open; a closed pair must never take a part of the train.
	 */
	const auto drive = [&rig, freight, &reaches](const std::function<bool()> &done, bool open) {
		uint ticks = 0;
		while (!done() && ticks < 8000) {
			CallVehicleTicks();
			REQUIRE_FALSE(freight->vehstatus.Test(VehState::Crashed));
			REQUIRE(IsTrainOnItsTiles(freight));
			if (!open) REQUIRE_FALSE(reaches(rig.source));
			++ticks;
		}
		INFO("after " << ticks << " ticks: " << DescribeTrain(freight));
		REQUIRE(done());
	};

	// Sealed: the train runs to the door, finds a line end there and turns back to its depot.
	bool at_door = false;
	drive([&at_door, &reaches, &home, approach] {
		at_door |= reaches(approach);
		return at_door && home();
	}, false);

	// Opened with the player's command: the same train crosses the pair into the bored tunnel, to the end of its last
	// cut, and comes back. The signals the connection queued are finished as the command kernel does after a command.
	REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, rig.source, true).Succeeded());
	UpdateSignalsInBuffer();
	REQUIRE(GetLayerPortalWorkState(rig.source) == LayerPortalWorkState::Connected);
	const TileIndex tunnel_end = route.back().cutter_position;
	drive([&reaches, tunnel_end] { return reaches(tunnel_end); }, true);
	drive([&home] { return home(); }, true);
}

/** The pair's track is shown to the project's cutter consist only, at both ends, through the legal sides only. */
TEST_CASE("A Commissioning pair shows its track only to the project's own trains", "[tbm-movement]")
{
	TBMMovementRig rig(1);
	const DiagDirection shaft = GetLayerPortalShaftDirection(rig.source);
	const DiagDirection door = GetLayerPortalDoorDirection(rig.source);
	const TrackdirBits axis = TrackBitsToTrackdirBits(DiagDirToDiagTrack(shaft));

	// The cutter consist sees the axis at the key, through the door or by a shaft step, and at the rock end.
	CHECK(GetLayerPortalAdmittedTrackdirs(rig.source, *rig.front, door) == axis);
	CHECK(GetLayerPortalAdmittedTrackdirs(rig.source, *rig.front, DiagDirection::Invalid) == axis);
	CHECK(GetLayerPortalAdmittedTrackdirs(rig.rock_end, *rig.front, DiagDirection::Invalid) == axis);
	CHECK(GetLayerPortalAdmittedTrackdirs(rig.rock_end, *rig.front, GetLayerPortalDoorDirection(rig.rock_end)) == axis);
	// The shaft side is never an ordinary edge.
	CHECK(GetLayerPortalAdmittedTrackdirs(rig.source, *rig.front, shaft).None());
	// Only the key keeps a magazine, which the cutter consist leaves through the shaft.
	CHECK(GetLayerPortalMagazineExit(rig.source, *rig.front) == shaft);
	CHECK(GetLayerPortalMagazineExit(rig.rock_end, *rig.front) == DiagDirection::Invalid);
	// Ordinary routing still sees no track on the pair.
	CHECK_FALSE(GetTileTrackStatus(rig.source, TransportType::Rail, RoadTramType::Invalid, door).trackdirs.Any());
	CHECK_FALSE(GetTileTrackStatus(rig.rock_end, TransportType::Rail, RoadTramType::Invalid).trackdirs.Any());

	// A second consist of the same company and model, assembled at a second, Sealed pair.
	const TileIndex second = TileXYL(30, 20, MapLayer::Surface);
	REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, second, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
	auto request = rig.fixture.request;
	request.portal = second;
	request.proposal = rig.base;
	request.proposal.commissioning_anchor = GetLayerPortalPairTile(second);
	Train *other = nullptr;
	const std::array<const Engine *, 1> wagons{Engine::Get(GetTBMEngine(2))};
	REQUIRE(BuildTBMConsistAtSite(request, Engine::Get(GetTBMEngine(0)), 0, GetFreeUnitNumber(VehicleType::Train), wagons, &other).Succeeded());
	// It is not this project's consist: no track and no magazine at either end of this pair.
	CHECK(GetLayerPortalAdmittedTrackdirs(rig.source, *other, door).None());
	CHECK(GetLayerPortalAdmittedTrackdirs(rig.rock_end, *other, DiagDirection::Invalid).None());
	CHECK(GetLayerPortalMagazineExit(rig.source, *other) == DiagDirection::Invalid);
	// A Sealed pair shows its track to nobody, its own assembled consist and the other project's cutter included.
	CHECK(GetLayerPortalAdmittedTrackdirs(second, *other, door).None());
	CHECK(GetLayerPortalAdmittedTrackdirs(second, *rig.front, door).None());
	CHECK(GetLayerPortalMagazineExit(second, *other) == DiagDirection::Invalid);
}

/** An ordinary train that drives up to the door of a Commissioning pair reaches a line end there and turns back. */
TEST_CASE("An ordinary train is turned back at a Commissioning pair's door", "[tbm-movement]")
{
	PathfinderDefaults pathfinder;
	TBMExchangeRig rig;
	const TileIndex source = rig.fixture.request.portal;
	const DiagDirection door = GetLayerPortalDoorDirection(source);
	const Track axis = DiagDirToDiagTrack(door);
	// A short line from a depot straight up to the pair's door: the door tile, one more tile, then the depot.
	const TileIndex approach = GetLayerPortalDoorTile(source);
	const TileIndex beyond = TileAddByDiagDir(approach, door);
	const TileIndex depot = TileAddByDiagDir(beyond, door);
	REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, approach, RAILTYPE_RAIL, axis, false).Succeeded());
	REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, beyond, RAILTYPE_RAIL, axis, false).Succeeded());
	REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, depot, RAILTYPE_RAIL, ReverseDiagDir(door)).Succeeded());
	UpdateSignalsInBuffer();
	Train *stranger = rig.BuildHauler(depot, 1, 0);
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, stranger->index, true).Succeeded());

	bool reached = false;
	// The train runs to the door, finds a line end, turns back to its depot, and leaves again; it never enters the pair.
	for (uint tick = 0; tick < 3000; ++tick) {
		REQUIRE(stranger->Tick());
		for (const Train *part = stranger; part != nullptr; part = part->Next()) {
			REQUIRE(part->tile != source);
			reached |= part->tile == approach;
		}
	}
	CHECK(reached);
	CHECK_FALSE(stranger->vehstatus.Test(VehState::Crashed));
	// The boring consist held in its magazine was never disturbed.
	for (const Train *part = rig.front; part != nullptr; part = part->Next()) {
		CHECK(part->tile == source);
		CHECK(part->track == Track::Depot);
	}
}

/** Earth and Rock tonnes, kept apart. */
struct SpoilTonnes {
	uint64_t earth = 0; ///< Earth tonnes.
	uint64_t rock = 0; ///< Rock tonnes.

	/** @param other Tonnes to add. @return The sum, kind by kind. */
	SpoilTonnes operator+(const SpoilTonnes &other) const { return {this->earth + other.earth, this->rock + other.rock}; }

	/** @param other Tonnes to compare. @return Whether both kinds match. */
	bool operator==(const SpoilTonnes &other) const = default;
};

/**
 * @param train Front engine.
 * @return Every part's ID in chain order, comma separated, for the loop's trace.
 */
static std::string DescribeLoad(const Train *train)
{
	std::string ids;
	for (VehicleID id : GetTBMChainIDs(train)) ids += fmt::format("{}{}", ids.empty() ? "" : ",", id.base());
	return ids;
}

/** How the source loop's project comes to be. */
enum class LoopStart : uint8_t {
	Straight, ///< The rig's own project of three straight cuts, put back to its commissioned state.
	Bend, ///< The rig's own project on a bend whose spoil overflows one load, put back to its commissioned state.
	UTurn, ///< A full advancing half-circle whose paid envelope is the only traversable underground floor.
	Command, ///< A project commissioned through the player's command at a second Sealed pair, on its own planned route.
};

/**
 * @param start How the loop's project comes to be.
 * @return The turns of the rig's own route: the bend for LoopStart::Bend, otherwise three straight cuts.
 */
static std::vector<DirDiff> LoopTurns(LoopStart start)
{
	if (start == LoopStart::Bend) return {DirDiff::Same, DirDiff::Right45, DirDiff::Right45};
	if (start == LoopStart::UTurn) return {DirDiff::Same, DirDiff::Same, DirDiff::Same, DirDiff::Right45, DirDiff::Same,
		DirDiff::Right45, DirDiff::Same, DirDiff::Right45, DirDiff::Same, DirDiff::Right45, DirDiff::Same, DirDiff::Same};
	return std::vector<DirDiff>(3, DirDiff::Same);
}

/**
 * The source loop of a route with both trains driving themselves through the game's vehicle step. The line runs from
 * the pair's door: plain rail, a platform of the origin, plain rail, and the hauler's depot facing back up it. The
 * hauler has one ordinary order, to the origin. The game's random state is seeded, so two loops replay alike.
 */
class SourceLoop {
	Randomizer saved_random = _random; ///< The caller's random state, restored at the end.
public:
	AutoRestoreBackup<uint8_t> indicators{_settings_client.gui.loading_indicators, uint8_t{0}}; ///< No filling text effect without a GUI.
	PathfinderDefaults pathfinder; ///< Real pathfinder settings for moving trains.
	SpoilReceiverRig rig; ///< Cutter consist, receivers and origin.
	TownIndexEntry town; ///< The origin's town in the town index, so the depot can be named.
	Industry *earth = nullptr; ///< Earth receiver.
	Industry *rock = nullptr; ///< Rock crusher.
	TBMProjectID project{}; ///< The loop's project.
	Train *cutter = nullptr; ///< Its cutter consist.
	TileIndex source = INVALID_TILE; ///< Its Surface pair key: the magazine and Source bay.
	TileIndex rock_end = INVALID_TILE; ///< The rock end of its pair, which the hauler never reaches.
	TileIndex depot = INVALID_TILE; ///< The hauler's depot at the end of the line.
	Train *hauler = nullptr; ///< The assigned hauler.
	std::string trace; ///< One line per transition: phases, wagon IDs and the Earth/Rock ledger, with its tick.
	std::string journal; ///< Every change of both trains' state and the hauler's tile, for a stalled loop.
	std::vector<uint16_t> exchanged_at; ///< The job's cut cursor at each completed exchange, in order.
	bool daily = false; ///< Also run every train's daily step once per game day, as the date timer does in a game.
	uint ticks = 0; ///< Ticks run.

	/**
	 * Build the line and the hauler, give the loop its commissioned project, and start both trains.
	 * @param start The rig's project on its straight route or its bend, or one commissioned through the player's command.
	 * @param player Leave the machine stopped and the hauler unassigned and stopped, for the player's controls.
	 */
	explicit SourceLoop(LoopStart start = LoopStart::Straight, bool player = false) : rig(LoopTurns(start)), town(*rig.town)
	{
		_random.SetSeed(0x7B3D);
		this->earth = this->rig.BuildReceiver(SpoilReceiverKind::Earth, 15, 22);
		this->rock = this->rig.BuildReceiver(SpoilReceiverKind::Rock, 14, 16);
		// The rig's own consist goes back to its commissioned state: stopped, wholly in its magazine.
		this->rig.front->vehstatus.Set(VehState::Stopped);
		this->rig.Reload(TBMPhase::Commissioning);
		if (start != LoopStart::Command) {
			this->project = this->rig.project.id;
			this->cutter = this->rig.front;
			this->source = this->rig.fixture.request.portal;
			this->rock_end = this->rig.proof.destination;
		} else {
			// A second Sealed pair, built with the player's command; it is commissioned once its line and origin are ready.
			this->source = TileXYL(32, 20, MapLayer::Surface);
			REQUIRE(Command<Commands::BuildLayerPortal>::Do(DoCommandFlag::Execute, this->source, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
			this->rock_end = GetLayerPortalPairTile(this->source);
		}
		const TileIndex source = this->source;
		const DiagDirection door = GetLayerPortalDoorDirection(source);
		const Track axis = DiagDirToDiagTrack(door);
		const TileIndex approach = GetLayerPortalDoorTile(source);
		const TileIndex platform = TileAddByDiagDir(approach, door);
		const TileIndex tail = TileAddByDiagDir(platform, door);
		this->depot = TileAddByDiagDir(tail, door);
		for (const TileIndex tile : {approach, tail}) {
			REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, axis, false).Succeeded());
		}
		REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, this->depot, RAILTYPE_RAIL, ReverseDiagDir(door)).Succeeded());
		MakeRailStation(platform, _current_company, this->rig.fixture.origin->index, DiagDirToAxis(door), 0, RAILTYPE_RAIL);
		this->rig.fixture.origin->train_station = TileArea(this->rig.origin_tile, platform);
		this->rig.fixture.origin->spread.Add(platform);
		UpdateSignalsInBuffer();
		this->rig.RefreshOrigin();
		if (start == LoopStart::Command) this->Commission();

		// The hauler gets one ordinary order, to the origin; the cycle never writes its orders.
		this->hauler = this->rig.BuildHauler(this->depot, 2, 2);
		Order order;
		order.MakeGoToStation(this->rig.fixture.origin->index);
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, this->hauler->index, 0, order).Succeeded());
		// The cutter consist starts as commissioning leaves it: stopped, wholly in its magazine.
		REQUIRE(this->Project().phase == TBMPhase::Commissioning);
		REQUIRE(IsTrainStoppedAtPortalSite(*this->cutter));
		if (player) return; // The player's controls take the remaining steps.
		REQUIRE(AssignTBMHauler(this->project, this->hauler->index, true) == TBMExchangeError::None);
		for (const Train *train : {this->cutter, this->hauler}) {
			REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, train->index, true).Succeeded());
		}
	}

	/** Restore the caller's random state. */
	~SourceLoop() { _random = this->saved_random; }

	/**
	 * Run every train's daily economy and calendar step, as the game's date timer does once per day: service visits,
	 * breakdown checks, order checks, running costs and age.
	 */
	static void RunTrainDays()
	{
		for (Train *train : Train::Iterate()) {
			if (!train->IsFrontEngine()) continue;
			train->OnNewEconomyDay();
			train->OnNewCalendarDay();
		}
	}

	/**
	 * Commission at the second pair with the player's command: two Earth and two Rock hoppers, the origin with its
	 * receivers, and a straight area of seven cells out of the rock end's door, long enough that its spoil overflows one
	 * load. The command plans the route itself.
	 */
	void Commission()
	{
		const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(this->rock_end));
		const TileIndex area_start = TileAddWrap(this->rock_end, step.x, step.y);
		const TileIndex area_end = TileAddWrap(this->rock_end, step.x * 7, step.y * 7);
		REQUIRE(IsValidTile(area_end));
		const size_t before = CaptureTBMProjects().size();
		const Money cash = SpoilReceiverRig::Cash();
		const auto bought = Command<Commands::CommissionTBM>::Do(DoCommandFlag::Execute, this->source, 0, 2, 2, this->rig.fixture.origin->index, area_start, area_end, false);
		INFO(bought.GetErrorMessage().base());
		REQUIRE(bought.Succeeded());
		// The top-level call charged the purchase; the queued signals are finished as the command kernel does.
		UpdateSignalsInBuffer();
		CHECK(SpoilReceiverRig::Cash() == cash - bought.GetCost());
		REQUIRE(CaptureTBMProjects().size() == before + 1);
		const TBMProject commissioned = CaptureTBMProjects().back();
		REQUIRE(commissioned.portal == this->source);
		this->project = commissioned.id;
		this->cutter = Train::Get(commissioned.cutter);
		REQUIRE(GetLayerPortalWorkState(this->source) == LayerPortalWorkState::Commissioning);
	}

	/** @return The loop's live project. */
	const TBMProject &Project() const { return *GetTBMProject(this->project); }

	/** @return Its live job. */
	const ExcavationJob &Job() const { return *GetExcavationJob(this->Project().job); }

	/** @return The hauler's live assignment. */
	TBMExchangeAssignment Cycle() const { return *GetTBMExchangeAssignmentForHauler(this->hauler->index); }

	/** @return Whether the route is finished and its last load delivered. */
	bool Delivered() const { return this->Project().phase == TBMPhase::Completed && this->Cycle().phase == TBMHaulerPhase::ReturnEmpties; }

	/**
	 * @param train Front engine.
	 * @return The Earth and Rock tonnes aboard its parts, wherever they are in their lists.
	 */
	static SpoilTonnes Aboard(const Train *train)
	{
		const CargoType earth = GetCargoTypeByLabel(CargoLabel{"ERTH"});
		const CargoType rock = GetCargoTypeByLabel(CargoLabel{"ROCK"});
		SpoilTonnes tonnes;
		// A part carries one cargo; its whole list counts, whatever its loading state.
		for (const Train *part = train; part != nullptr; part = part->Next()) {
			if (part->cargo_type == earth) tonnes.earth += part->cargo.TotalCount();
			if (part->cargo_type == rock) tonnes.rock += part->cargo.TotalCount();
		}
		return tonnes;
	}

	/** @return The spoil the receivers took: their stock, plus what they disposed of or crushed. */
	SpoilTonnes Received() const
	{
		const SpoilReceiver *earth_state = GetSpoilReceiver(this->earth->index);
		const SpoilReceiver *rock_state = GetSpoilReceiver(this->rock->index);
		return {SpoilReceiverRig::Stock(*this->earth) + earth_state->disposed_t, SpoilReceiverRig::Stock(*this->rock) + rock_state->crushed_t + rock_state->disposed_t};
	}

	/** @return Spoil left on station platforms, which a refused transfer would put there. */
	static SpoilTonnes OnPlatforms()
	{
		const CargoType earth = GetCargoTypeByLabel(CargoLabel{"ERTH"});
		const CargoType rock = GetCargoTypeByLabel(CargoLabel{"ROCK"});
		SpoilTonnes tonnes;
		// Every station's lists count; a goods entry without data holds nothing.
		for (const Station *st : Station::Iterate()) {
			if (st->goods[earth].HasData()) tonnes.earth += st->goods[earth].GetData().cargo.TotalCount();
			if (st->goods[rock].HasData()) tonnes.rock += st->goods[rock].GetData().cargo.TotalCount();
		}
		return tonnes;
	}

	/** @return The tonnes the job has committed. */
	SpoilTonnes Committed() const { return {this->Job().completed_earth_t, this->Job().completed_rock_t}; }

	/** @return Whether every committed tonne is in exactly one place: aboard a train, on a platform or received. */
	bool IsConserved() const { return Aboard(this->cutter) + Aboard(this->hauler) + OnPlatforms() + this->Received() == this->Committed(); }

	/** @return Both phases, the hauler's wait and each train's wagon IDs, as one line that changes only at a transition. */
	std::string Transition() const
	{
		const TBMExchangeAssignment cycle = this->Cycle();
		return fmt::format("cutter {} [{}] | hauler phase {} wait {} [{}]", DescribeProject(this->Project()), DescribeLoad(this->cutter), to_underlying(cycle.phase), to_underlying(cycle.wait), DescribeLoad(this->hauler));
	}

	/**
	 * @param tonnes Earth and Rock tonnes.
	 * @return Them as "earth/rock".
	 */
	static std::string Tonnes(const SpoilTonnes &tonnes) { return fmt::format("{}/{}", tonnes.earth, tonnes.rock); }

	/**
	 * Run the receivers and the game's vehicle step until the last load is delivered. At every transition the trace gets
	 * a line with the wagon IDs and the ledger, and the ledger must balance.
	 * @param limit Most ticks to run.
	 */
	void Run(uint limit = 200000)
	{
		std::string last_transition, last_state;
		// The receivers take their intake, then the game's own vehicle step runs: stations load and unload, every vehicle
		// ticks, and a train stopped on entering a depot is started again (VehicleEnteredDepotThisTick).
		while (!this->Delivered() && this->ticks < limit) {
			AdvanceSpoilReceiver(*this->earth);
			AdvanceSpoilReceiver(*this->rock);
			CallVehicleTicks();
			if (this->daily && this->ticks % Ticks::DAY_TICKS == 0) RunTrainDays();
			REQUIRE_FALSE(this->cutter->vehstatus.Test(VehState::Crashed));
			REQUIRE_FALSE(this->hauler->vehstatus.Test(VehState::Crashed));
			// The hauler never passes the cutter: it never reaches the rock end of the pair.
			for (const Train *part = this->hauler; part != nullptr; part = part->Next()) REQUIRE(part->tile != this->rock_end);
			++this->ticks;
			// Each exchange is recorded with the cut after which it came; two cannot complete in one tick.
			if (const auto exchanges = this->Cycle().exchanges; exchanges != this->exchanged_at.size()) {
				REQUIRE(exchanges == this->exchanged_at.size() + 1);
				this->exchanged_at.push_back(static_cast<uint16_t>(this->Job().cursor));
			}
			if (std::string now = this->Transition(); now != last_transition) {
				// Each transition records where every tonne is; the sum must still be what the job committed.
				this->trace += fmt::format("\n{}: {} | aboard {} {} platforms {} received {} committed {}", this->ticks, now, Tonnes(Aboard(this->cutter)), Tonnes(Aboard(this->hauler)), Tonnes(OnPlatforms()), Tonnes(this->Received()), Tonnes(this->Committed()));
				INFO("tick " << this->ticks << ": " << now);
				CHECK(this->IsConserved());
				last_transition = std::move(now);
			}
			const Train *lead = this->hauler->GetMovingFront();
			if (std::string now = fmt::format("{} | order {} tile {} track {}{}", last_transition, to_underlying(this->hauler->current_order.GetType()), lead->tile.base(), lead->track.base(), this->hauler->vehstatus.Test(VehState::Stopped) ? " stopped" : ""); now != last_state) {
				this->journal += fmt::format("\n{}: {}", this->ticks, now);
				last_state = std::move(now);
			}
		}
		INFO("journal (depot " << this->depot.base() << ", bay " << this->source.base() << "):" << this->journal);
		INFO("cutter " << DescribeTrain(this->cutter) << "\nhauler " << DescribeTrain(this->hauler));
		REQUIRE(this->Delivered());
	}
};

/**
 * Check a delivered loop's end state: one exchange at each frozen breakpoint and none elsewhere, spoil committed and
 * every tonne of it received, nothing left aboard either train, and the player's order list untouched.
 * @param loop A loop run until its last load was delivered.
 */
static void CheckDelivered(const SourceLoop &loop)
{
	INFO("trace:" << loop.trace);
	CHECK(loop.Cycle().exchanges == loop.Job().pass_terms->exchange_after.size());
	CHECK(loop.exchanged_at == loop.Job().pass_terms->exchange_after);
	CHECK(SourceLoop::Aboard(loop.hauler) == SpoilTonnes{});
	CHECK(SourceLoop::Aboard(loop.cutter) == SpoilTonnes{});
	CHECK(loop.Received() == loop.Committed());
	CHECK(loop.Committed() != SpoilTonnes{});
	CHECK(loop.hauler->GetNumOrders() == 1);
}

/** A valid face target does not authorize a guessed junction branch when the paid path to it is broken. */
TEST_CASE("A TBM holds at a junction until its actual work route is restored", "[tbm-travel-junction]")
{
	SourceLoop loop(LoopStart::Command);
	/** @param done Actual native state to reach. @param limit Maximum physical ticks; every cargo operation stays with its real owner. */
	const auto drive = [&](const std::function<bool()> &done, uint limit = 30000) {
		uint ticks = 0;
		while (!done() && ticks < limit) {
			AdvanceSpoilReceiver(*loop.earth);
			AdvanceSpoilReceiver(*loop.rock);
			CallVehicleTicks();
			REQUIRE_FALSE(loop.cutter->vehstatus.Test(VehState::Crashed));
			REQUIRE_FALSE(loop.hauler->vehstatus.Test(VehState::Crashed));
			REQUIRE(IsTrainOnItsTiles(loop.cutter));
			++ticks;
		}
		INFO(DescribeProject(loop.Project()) << "\n" << DescribeTrain(loop.cutter));
		REQUIRE(done());
	};
	/** @return Whether the real hauler exchange has sent the actual cutter back toward an already paid face. */
	const auto resumed = [&] { return loop.Cycle().exchanges != 0 && loop.Project().phase == TBMPhase::TravelToFace && loop.Job().cursor >= 3; };
	drive(resumed);
	std::vector<size_t> ends;
	const auto path = GetTBMWorkRailPath(loop.Job(), loop.Job().cursor, ends);
	REQUIRE(path.size() >= 4);
	const TileIndex junction = path[1], gap = path[2];
	// Fault injection changes only two vacated, already paid pieces. Both real
	// trains, their cargo and the accepted face target remain where they drove.
	for (const Train *part : Train::Iterate()) REQUIRE((part->tile != junction && part->tile != gap));
	const auto job = loop.Job();
	const auto target = GetTBMCutterSteeringTarget(*loop.cutter);
	REQUIRE(target.has_value());
	{
		TBMTargetTrackFault restore_junction(junction), restore_gap(gap);
		TrackBits branches = DiagdirReachesTracks(DiagdirBetweenTiles(path[0], junction));
		branches.Reset(GetTrackBits(junction));
		REQUIRE(branches.Any());
		SetTrackBits(junction, GetTrackBits(junction) | TrackBits{FindFirstTrack(branches)});
		SetTrackBits(gap, {});
		YapfNotifyTrackLayoutChange(junction, Track::Invalid);
		YapfNotifyTrackLayoutChange(gap, Track::Invalid);
		REQUIRE(GetTBMCutterSteeringTarget(*loop.cutter).has_value());
		CHECK(GetTBMCutterSteeringTarget(*loop.cutter)->tile == target->tile);
		/** @return Whether native pathfinding has refused the broken route and the controller displays that actual wait. */
		const auto held = [&] { return loop.cutter->flags.Test(VehicleRailFlag::Stuck) && loop.Project().blocked == TBMBlockedReason::Obstruction; };
		drive(held);
		const auto positions = CaptureTBMMovementPositions(loop.cutter);
		const auto packets = GetTBMPacketOwners(loop.cutter);
		uint ticks = 0;
		const uint wait = _settings_game.pf.wait_for_pbs_path * Ticks::DAY_TICKS + 2;
		/** @return Whether a full ordinary automatic-turnaround interval has elapsed without moving the cutter. */
		const auto waited = [&] { return ticks++ >= wait; };
		drive(waited, wait + 2);
		CHECK(CaptureTBMMovementPositions(loop.cutter) == positions);
		CHECK(GetTBMPacketOwners(loop.cutter) == packets);
		CHECK(loop.Job() == job);
	}
	/** @return Whether periodic native reservation retry reaches the original paid face after actual track repair. */
	const auto recovered = [&] { return loop.Project().phase == TBMPhase::Bore && loop.Project().blocked == TBMBlockedReason::None; };
	drive(recovered);
	CHECK(loop.cutter->tile == target->tile);
	CHECK(loop.Job().cursor == job.cursor);
}

/**
 * The source loop with both trains driving themselves: one order to the origin carries every load of the route, every
 * transition keeps the Earth/Rock ledger whole, and every committed tonne reaches a receiver (Plan 5 13.1).
 */
TEST_CASE("A hauler steered to its source bay carries every load of a route to the origin", "[tbm-loop]")
{
	SourceLoop loop;
	loop.Run();
	CheckDelivered(loop);
}

/**
 * A bending route whose spoil overflows one load (Plan 5 13.1): the hauler takes the first load partway, the cutter
 * goes back to its face with the empties and bores on, and the hauler, back with empties again, takes the last load.
 */
TEST_CASE("A cutter bores on with the empties of a mid-route exchange", "[tbm-loop]")
{
	SourceLoop loop(LoopStart::Bend);
	REQUIRE(loop.Job().pass_terms->exchange_after.size() >= 2);
	loop.Run();
	CheckDelivered(loop);
}

/** Every real part takes an advancing half-circle and backs along its paid envelope, with actual haulage between passes. */
TEST_CASE("A whole TBM consist drives a constrained U-turn without pivoting or cutting backwards", "[tbm-travel-uturn]")
{
	SourceLoop loop(LoopStart::UTurn);
	const auto route = loop.Job().route;
	REQUIRE(route.back().heading == ReverseDir(route.front().heading));
	std::set<TileIndex> paid;
	uint ticks = 0;
	bool backed = false;
	std::set<VehicleID> turned;
	// The only underground floor is the actual commissioned pocket plus each
	// committed sweep. Keep checking every attached part while the hauler makes
	// real exchanges; no fixture emptying or pose edits can complete this route.
	while (!loop.Delivered() && ticks++ < 30000) {
		const auto poses = CaptureTBMMovementPositions(loop.cutter);
		const auto cursor = loop.Job().cursor;
		const bool backwards = loop.cutter->vehicle_flags.Test(VehicleFlag::DrivingBackwards);
		AdvanceSpoilReceiver(*loop.earth);
		AdvanceSpoilReceiver(*loop.rock);
		CallVehicleTicks();
		REQUIRE_FALSE(loop.cutter->vehstatus.Test(VehState::Crashed));
		REQUIRE(IsTrainOnItsTiles(loop.cutter));
		REQUIRE(loop.IsConserved());
		if (backwards) CHECK(loop.Job().cursor == cursor); // Withdrawal never authorizes a new cut.
		backed |= backwards;
		for (size_t cut = cursor; cut < loop.Job().cursor; ++cut) paid.insert(route[cut].swept_tiles.begin(), route[cut].swept_tiles.end());
		for (const Train *part = loop.cutter; part != nullptr; part = part->Next()) {
			// Storage and the sealed shaft have their own native movement contract;
			// every ordinary underground part must stay on newly paid support.
			if (GetTileLayer(part->tile) == MapLayer::Surface || IsLayerPortalTile(part->tile)) continue;
			REQUIRE(paid.contains(part->tile));
			REQUIRE(GetGeologyWorldState().terrain.Get(part->tile) == ExcavationState::Excavated);
			REQUIRE(IsTBMSupportUsable(loop.Project(), part->tile));
			/** @param pose Previously observed part pose. @return Native part identity, independent of exchanged chain order. */
			const auto identity = [](const auto &pose) { return std::get<0>(pose); };
			const auto previous = std::ranges::find(poses, part->index, identity);
			REQUIRE(previous != poses.end());
			if (part->direction == std::get<6>(*previous)) continue;
			// Bends change facing only as the part physically advances, by at most
			// 45 degrees per tick; a stationary rotation or sharp shortcut fails.
			CHECK((part->x_pos != std::get<3>(*previous) || part->y_pos != std::get<4>(*previous)));
			const uint diff = (to_underlying(part->direction) + 8 - to_underlying(std::get<6>(*previous))) % 8;
			CHECK((diff == 1 || diff == 7));
			turned.insert(part->index);
		}
	}
	INFO(DescribeProject(loop.Project()) << "\n" << DescribeTrain(loop.cutter));
	REQUIRE(loop.Delivered());
	CHECK(loop.Job().cursor == route.size());
	CHECK(backed);
	for (VehicleID part : loop.Project().permanent_parts) CHECK(turned.contains(part)); // Every permanent component must actually negotiate the turn.
	CHECK(loop.Received() == loop.Committed());
}

/**
 * The whole loop from the player's commands (Plan 5 13.1): a pair built and commissioned through them, the route
 * the command planned, then every load carried to the origin by the steered hauler. The route overflows one load, so
 * the cutter also bores on with the empties of a mid-route exchange.
 */
TEST_CASE("A project commissioned by command carries its whole route to the origin", "[tbm-loop]")
{
	SourceLoop loop(LoopStart::Command);
	REQUIRE(loop.Project().id != loop.rig.project.id);
	INFO("route of " << loop.Job().route.size() << " cuts");
	REQUIRE(loop.Job().pass_terms->exchange_after.size() >= 2);
	loop.Run();
	CheckDelivered(loop);
	// The rig's own project never moved: it still waits, stopped, in its own magazine.
	CHECK(loop.rig.Project().phase == TBMPhase::Commissioning);
	CHECK(IsTrainStoppedAtPortalSite(*loop.rig.front));
}

/**
 * The same commands from the same fixture give the same trace, ledger and wagon IDs included (Plan 5 13.3). The
 * project is commissioned by command, so the replay covers the commissioning too.
 */
TEST_CASE("The source loop replays the same trace from the same fixture", "[tbm-loop]")
{
	std::string first;
	{
		SourceLoop loop(LoopStart::Command);
		loop.Run();
		first = loop.trace;
	}
	SourceLoop again(LoopStart::Command);
	again.Run();
	REQUIRE_FALSE(first.empty());
	CHECK(again.trace == first);
}

/**
 * The whole life of a project played only through the portal window's controls (R2MLVS3MP master 7.9). Every step the
 * controls name is taken with the command the window posts. Nothing is offered before it is accepted, every refusal
 * names its reason, and a finished project never draws its hauler back into the portal.
 */
TEST_CASE("A project's whole life can be played from the portal window's controls", "[tbm-controls]")
{
	SourceLoop loop(LoopStart::Command, true);
	const CompanyID company = _current_company;
	const TBMProjectID project = loop.project;
	// The game's daily steps run too, with a real service interval and every train already overdue for service.
	loop.daily = true;
	AutoRestoreBackup service_interval(Company::Get(company)->settings.vehicle.servint_trains, uint16_t{150});
	for (Train *train : Train::Iterate()) train->date_of_last_service = TimerGameEconomy::date - 1000;
	/** @return The pair's controls, as the window sees them now. */
	const auto controls = [&loop, company]() { return GetTBMPairControls(loop.source, company); };
	/**
	 * Run the receivers and the game's vehicle step until the controls name a step; no train may crash.
	 * @param step Step to wait for.
	 * @param limit Most ticks to run.
	 */
	const auto run_until = [&loop, &controls](TBMPlayerStep step, uint limit) {
		uint ticks = 0;
		while (controls().step != step && ticks < limit) {
			AdvanceSpoilReceiver(*loop.earth);
			AdvanceSpoilReceiver(*loop.rock);
			CallVehicleTicks();
			if (++loop.ticks % Ticks::DAY_TICKS == 0) SourceLoop::RunTrainDays();
			REQUIRE_FALSE(loop.cutter->vehstatus.Test(VehState::Crashed));
			REQUIRE_FALSE(loop.hauler->vehstatus.Test(VehState::Crashed));
			++ticks;
		}
		INFO("after " << ticks << " ticks, step " << to_underlying(controls().step) << ": " << DescribeProject(loop.Project()));
		REQUIRE(controls().step == step);
	};

	// Commissioned: the machine stands stopped in its portal, and the controls say so and what a hauler needs.
	TBMPairControls c = controls();
	REQUIRE(c.project.has_value());
	CHECK(c.project->id == project);
	CHECK(c.step == TBMPlayerStep::StartMachine);
	CHECK(c.cuts_done == 0);
	CHECK(c.cuts_total == loop.Job().route.size());
	CHECK(c.earth_hoppers == 2);
	CHECK(c.rock_hoppers == 2);
	CHECK(c.assign);
	CHECK(c.cancel);
	CHECK_FALSE(c.retire);
	CHECK_FALSE(c.release);
	CHECK(c.convertible.empty());
	CHECK(c.sellable == VehicleID::Invalid());
	// The refusals name their reasons: the machine can't haul its own spoil, an unfinished project can't retire, and
	// a train that serves no project can't be released.
	CHECK(Command<Commands::AssignTBMHauler>::Do(DoCommandFlag::Execute, project, loop.cutter->index).GetErrorMessage() == STR_ERROR_TBM_HAULER_TRAIN);
	CHECK(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project, TBMProjectAction::Retire).GetErrorMessage() == STR_ERROR_TBM_RELEASE_RETURN);
	CHECK(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project, TBMProjectAction::Resume).GetErrorMessage() == STR_ERROR_TBM_PROJECT_NOT_PAUSED);
	CHECK(Command<Commands::ReleaseTBMHauler>::Do(DoCommandFlag::Execute, loop.hauler->index).GetErrorMessage() == STR_ERROR_TBM_HAULER_NOT_ASSIGNED);

	// Start: the machine goes to work, and asks for a hauler once its hoppers need emptying.
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, loop.cutter->index, false).Succeeded());
	run_until(TBMPlayerStep::Working, 10);
	run_until(TBMPlayerStep::AssignHauler, 20000);
	c = controls();
	CHECK(c.wait == TBMLogisticsWait::NoHauler);
	CHECK(c.cuts_done != 0);
	CHECK(c.earth_t != 0);
	CHECK(c.rock_t != 0);

	// Pause and resume through the controls; a paused project asks to be resumed.
	REQUIRE(c.pause);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project, TBMProjectAction::Pause).Succeeded());
	c = controls();
	CHECK(c.step == TBMPlayerStep::Resume);
	CHECK(c.resume);
	CHECK_FALSE(c.pause);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project, TBMProjectAction::Resume).Succeeded());

	// Assign the hauler, as the window does for the clicked train, and start it; the loop then runs by itself.
	REQUIRE(Command<Commands::AssignTBMHauler>::Do(DoCommandFlag::Execute, project, loop.hauler->index).Succeeded());
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, loop.hauler->index, false).Succeeded());
	c = controls();
	REQUIRE(c.hauler.has_value());
	CHECK(c.hauler->hauler == loop.hauler->index);
	loop.Run();
	CheckDelivered(loop);
	// Overdue all along, the machine was never sent to a depot; its bay serviced it at every visit.
	CHECK_FALSE(loop.cutter->current_order.IsType(OT_GOTO_DEPOT));
	CHECK(loop.cutter->date_of_last_service == TimerGameEconomy::date);

	// Delivered: the finished project never draws its hauler back into the portal, and offers its release.
	c = controls();
	CHECK(c.step == TBMPlayerStep::ReleaseHauler);
	CHECK(c.release);
	CHECK_FALSE(c.assign);
	CHECK_FALSE(c.cancel);
	CHECK_FALSE(IsTBMHaulerAdmittedAtBay(*loop.hauler, loop.source));
	CHECK(MayTBMTrainLeaveMagazine(*loop.hauler));
	for (uint tick = 0; tick < 3000; ++tick) {
		AdvanceSpoilReceiver(*loop.earth);
		AdvanceSpoilReceiver(*loop.rock);
		CallVehicleTicks();
		if (++loop.ticks % Ticks::DAY_TICKS == 0) SourceLoop::RunTrainDays();
		for (const Train *part = loop.hauler; part != nullptr; part = part->Next()) REQUIRE(part->tile != loop.source);
	}
	REQUIRE(Command<Commands::ReleaseTBMHauler>::Do(DoCommandFlag::Execute, loop.hauler->index).Succeeded());

	// The released temporary track is offered for conversion, then the machine is stopped and the project retired.
	c = controls();
	CHECK(c.step == TBMPlayerStep::ConvertTrack);
	REQUIRE_FALSE(c.convertible.empty());
	for (const TileIndex tile : c.convertible) REQUIRE(Command<Commands::ConvertTBMSupport>::Do(DoCommandFlag::Execute, tile).Succeeded());
	c = controls();
	CHECK(c.convertible.empty());
	CHECK(c.step == TBMPlayerStep::Retire);
	CHECK(c.retire); // Finish can stop an already stationary whole stored machine as part of the admitted release.
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, loop.cutter->index, false).Succeeded());
	c = controls();
	CHECK(c.step == TBMPlayerStep::Retire);
	REQUIRE(c.retire);
	REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project, TBMProjectAction::Retire).Succeeded());

	// Retired: the actual machine remains selectable in its native magazine;
	// opening completed public track no longer requires selling it.
	c = controls();
	CHECK_FALSE(c.project.has_value());
	CHECK(c.step == TBMPlayerStep::Connect);
	REQUIRE(c.sellable == loop.cutter->index);

	// Both approaches are dug and built, so the pair connects for ordinary trains.
	c = controls();
	CHECK(c.step == TBMPlayerStep::Connect);
	REQUIRE(c.connect);
	REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, loop.source, true).Succeeded());
	UpdateSignalsInBuffer();
	c = controls();
	CHECK(c.step == TBMPlayerStep::Connected);
	CHECK(GetLayerPortalWorkState(loop.source) == LayerPortalWorkState::Connected);
}
