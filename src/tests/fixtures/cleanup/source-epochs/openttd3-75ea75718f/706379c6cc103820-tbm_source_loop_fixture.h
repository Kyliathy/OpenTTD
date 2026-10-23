/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_source_loop_fixture.h The source loop with both trains driving themselves through the game's vehicle step:
 * a project's cutter consist and an assigned hauler with one order to the origin, on a line from the pair's door past an
 * origin platform to the hauler's depot. Moved out of tbm_movement.cpp so the face exchange tests drive the same world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13
 * @see docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-m-movement-design.md, decisions 1-7
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Groups 4-8 (shared by the face exchange tests)
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13D (a face trip may pass through the pair)
 */

#ifndef TESTS_TBM_SOURCE_LOOP_FIXTURE_H
#define TESTS_TBM_SOURCE_LOOP_FIXTURE_H

#include "spoil_receiver_fixture.h"
#include "movement_fixture.h"
#include "../vehicle_cmd.h"
#include "../track_func.h"
#include "../signal_func.h"
#include "../pbs.h"
#include "../pathfinder/yapf/yapf_cache.h"
#include "../town_kdtree.h"
#include "../tbm_controls.h"
#include "../table/strings.h"
#include "../timer/timer_game_economy.h"

/**
 * @param project Live project.
 * @return Its phase, blocked reason and cut cursor.
 */
inline std::string DescribeProject(const TBMProject &project)
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
inline std::string DescribeLoad(const Train *train)
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
	Long, ///< Fourteen straight cuts: the whole consist stands clear of the housing with a face stand behind it (r2df2 Group 4).
};

/**
 * @param start How the loop's project comes to be.
 * @return The turns of the rig's own route: the bend for LoopStart::Bend, the half-circle for LoopStart::UTurn,
 *         fourteen straight cuts for LoopStart::Long, otherwise three straight cuts.
 */
inline std::vector<DirDiff> LoopTurns(LoopStart start)
{
	if (start == LoopStart::Bend) return {DirDiff::Same, DirDiff::Right45, DirDiff::Right45};
	if (start == LoopStart::Long) return std::vector<DirDiff>(14, DirDiff::Same);
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
		// Since r2df2 a hauler drives through the pair on a face trip: from becoming the face turn holder, through the
		// transfer, until it is back out at the origin. A loop may start this run with the hauler already inside.
		bool face_trip = IsTBMHaulerInCorridor(this->Project(), this->hauler->index);
		// The receivers take their intake, then the game's own vehicle step runs: stations load and unload, every vehicle
		// ticks, and a train stopped on entering a depot is started again (VehicleEnteredDepotThisTick).
		while (!this->Delivered() && this->ticks < limit) {
			AdvanceSpoilReceiver(*this->earth);
			AdvanceSpoilReceiver(*this->rock);
			CallVehicleTicks();
			if (this->daily && this->ticks % Ticks::DAY_TICKS == 0) RunTrainDays();
			REQUIRE_FALSE(this->cutter->vehstatus.Test(VehState::Crashed));
			REQUIRE_FALSE(this->hauler->vehstatus.Test(VehState::Crashed));
			// The hauler never passes the cutter: outside a face trip it never reaches the rock end of the pair.
			if (const TBMExchangeAssignment cycle = this->Cycle(); IsTBMFaceTurnHolder(cycle)) {
				face_trip = true;
			} else if (cycle.phase == TBMHaulerPhase::DisposeOrTransfer) {
				face_trip = false;
			}
			for (const Train *part = this->hauler; part != nullptr; part = part->Next()) REQUIRE((face_trip || part->tile != this->rock_end));
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
		// The line's own state explains a hauler that cannot leave its depot: its stuck flag, its target and destination,
		// and the reservation on each line tile from the depot up to the door.
		std::string line;
		const DiagDirection up = ReverseDiagDir(GetLayerPortalDoorDirection(this->source));
		for (TileIndex tile = this->depot; tile != this->source; tile = TileAddByDiagDir(tile, up)) {
			const bool reserved = IsRailDepotTile(tile) ? HasDepotReservation(tile) : GetReservedTrackbits(tile).Any();
			line += fmt::format(" {}:{}", tile.base(), reserved ? "reserved" : "free");
		}
		const auto target = GetTBMHaulerSteeringTarget(*this->hauler);
		INFO("line from the depot:" << line << " | hauler stuck " << this->hauler->flags.Test(VehicleRailFlag::Stuck) << " dest " << this->hauler->dest_tile.base()
			<< " target " << (target.has_value() ? target->tile.base() : 0u) << " reserve_paths " << _settings_game.pf.reserve_paths);
		// Where a reservation towards the door may end: the safe positions on the platform and the approach, and what the
		// key shows this hauler.
		const TileIndex approach = GetLayerPortalDoorTile(this->source);
		const TileIndex platform = TileAddByDiagDir(approach, ReverseDiagDir(up));
		const Trackdir towards_door = DiagDirToDiagTrackdir(up);
		INFO("safe platform " << IsSafeWaitingPosition(this->hauler, platform, towards_door, true, _settings_game.pf.forbid_90_deg)
			<< " safe approach " << IsSafeWaitingPosition(this->hauler, approach, towards_door, true, _settings_game.pf.forbid_90_deg)
			<< " key admits " << GetLayerPortalAdmittedTrackdirs(this->source, *this->hauler, DiagDirection::Invalid).base()
			<< " key status " << GetTileTrackStatus(this->source, TransportType::Rail, RoadTramType::Invalid).trackdirs.base()
			<< " key reserved " << HasLayerPortalReservation(this->source));
		REQUIRE(this->Delivered());
	}
};

/**
 * Check a delivered loop's end state: one exchange at each frozen breakpoint and none elsewhere, spoil committed and
 * every tonne of it received, nothing left aboard either train, and the player's order list untouched.
 * @param loop A loop run until its last load was delivered.
 */
inline void CheckDelivered(const SourceLoop &loop)
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

#endif /* TESTS_TBM_SOURCE_LOOP_FIXTURE_H */
