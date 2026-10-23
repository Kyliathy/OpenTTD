/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_obstruction.cpp A later project at a released portal and the "something is in the way" wait (demo report
 * R9). Project 0 cuts three cells out of its pair and is released without converting its support. Project 1,
 * commissioned at the same portal over that corridor, reuses the released support and cuts, also after the orphan
 * records were reloaded. A route whose support would meet existing track is refused in the quote, naming the tile.
 * A train in the next cut's sweep, or on the housing while the cutter waits to leave, is named in the status. The
 * cutter goes on by itself once that train has left.
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 7 (tasks 7.2-7.6)
 * @see docs/zz-reach2/upgrades/2026-10/r2df-a-reproduction-notes.md R9
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "spoil_receiver_fixture.h"
#include "movement_fixture.h"
#include "../vehicle_cmd.h"
#include "../rail_cmd.h"
#include "../track_func.h"
#include "../town_kdtree.h"
#include "../tbm_area_preview.h"
#include "../tbm_controls.h"
#include "../tbm_gui.h"
#include "../strings_func.h"
#include "../table/strings.h"

#include "../safeguards.h"

/**
 * The fixture town in the town index for one test, so commands that name things after the nearest town find it.
 * Declared after the rig that owns the town, so it leaves the index before the town is deleted.
 */
class ObstructionTownEntry {
	TownID id; ///< Indexed town.
public:
	/** @param town Fixture town to index. */
	explicit ObstructionTownEntry(const Town &town) : id(town.index) { _town_kdtree.Insert(this->id); }

	/** Take the town out of the index again. */
	~ObstructionTownEntry() { _town_kdtree.Remove(this->id); }
};

/**
 * Project 0 with three straight cuts at the rig's pair, its receivers and origin, run to completion and released
 * without converting its support; then project 1 at the same portal, over the old corridor and four cells further.
 */
class ReleasedCorridor {
	Randomizer saved_random = _random; ///< The caller's random state, restored at the end.
	/** Load the generated language before anything else, so statuses and refusals can be read as the player reads them. */
	struct Language {
		/** Install the mock environment and the generated test language. */
		Language()
		{
			MockEnvironment::Instance();
			LoadGeneratedWindowTestLanguage();
		}
	} language; ///< Loaded first, before the rig.
public:
	AutoRestoreBackup<uint8_t> indicators{_settings_client.gui.loading_indicators, uint8_t{0}}; ///< No filling text effect without a GUI.
	PathfinderDefaults pathfinder; ///< Real pathfinder settings for moving trains.
	SpoilReceiverRig rig{3}; ///< Project 0: three straight cuts out of the rock end.
	ObstructionTownEntry town{*rig.town}; ///< The origin's town in the town index.
	TileIndex source = INVALID_TILE; ///< Surface pair key: the magazine.
	TileIndex rock_end = INVALID_TILE; ///< Rock end of the pair, where every route starts.
	TileIndex area_start = INVALID_TILE; ///< First cell out of the rock end's door: project 0's first support cell.
	TileIndex area_end = INVALID_TILE; ///< Seventh cell out of the door, four beyond project 0's last cut.
	TBMProjectID second{UINT32_MAX}; ///< Project 1, once commissioned.
	uint8_t earth_wagons = 0; ///< Earth hoppers of the machine, which project 1 reuses.
	uint8_t rock_wagons = 0; ///< Rock hoppers of the machine.

	/** Build the receivers and return the rig's consist to its commissioned state, stopped in its magazine. */
	ReleasedCorridor()
	{
		_random.SetSeed(0x5A11);
		this->rig.BuildReceiver(SpoilReceiverKind::Earth, 15, 22);
		this->rig.BuildReceiver(SpoilReceiverKind::Rock, 14, 16);
		this->rig.RefreshOrigin();
		this->rig.front->vehstatus.Set(VehState::Stopped);
		this->rig.Reload(TBMPhase::Commissioning);
		this->source = this->rig.fixture.request.portal;
		this->rock_end = this->rig.proof.destination;
		const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(this->rock_end));
		this->area_start = TileAddWrap(this->rock_end, step.x, step.y);
		this->area_end = TileAddWrap(this->rock_end, step.x * 7, step.y * 7);
		REQUIRE(IsValidTile(this->area_end));
		// The reused machine is quoted with its own wagons; count each hopper once, not its articulated parts.
		const TBMProject &first = *GetTBMProject(this->rig.project.id);
		for (const Train *part = this->rig.front; part != nullptr; part = part->Next()) {
			if (part->IsArticulatedPart()) continue;
			const TBMConsistRole role = GetTBMConsistRole(first, *part);
			if (role == TBMConsistRole::EarthWagon) ++this->earth_wagons;
			if (role == TBMConsistRole::RockWagon) ++this->rock_wagons;
		}
		REQUIRE(this->earth_wagons > 0);
		REQUIRE(this->rock_wagons > 0);
	}

	/** Restore the caller's random state. */
	~ReleasedCorridor() { _random = this->saved_random; }

	/**
	 * Run the game's vehicle step until a condition holds; no train may crash.
	 * @param done Condition.
	 * @param limit Most ticks to try.
	 * @param empties Whether each exchange of the running project is served at once by emptying its consist where it
	 *                stands, a stand-in for the hauler loop that [tbm-loop] drives.
	 */
	void Run(const std::function<bool()> &done, uint limit, bool empties = false) const
	{
		uint ticks = 0;
		// Every train moves through the game's own vehicle step; the controllers run inside the cutters' ticks.
		while (!done() && ticks < limit) {
			if (empties) {
				for (const TBMProject &project : CaptureTBMProjects()) {
					if (project.phase != TBMPhase::WaitForEmpties) continue;
					for (Train *part = Train::Get(project.cutter); part != nullptr; part = part->Next()) part->cargo.Truncate();
				}
			}
			CallVehicleTicks();
			for (const Train *front : Train::Iterate()) REQUIRE_FALSE(front->vehstatus.Test(VehState::Crashed));
			++ticks;
		}
		INFO("after " << ticks << " ticks: " << DescribeTrain(this->rig.front));
		REQUIRE(done());
	}

	/** Run project 0 to the end of its route, then stop its consist and release the project as the player does. */
	void ReleaseFirst()
	{
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, this->rig.front->index, true).Succeeded());
		this->Run([this] { return GetTBMProject(this->rig.project.id)->phase == TBMPhase::Completed; }, 60000, true);
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, this->rig.front->index, true).Succeeded());
		REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, this->rig.project.id, TBMProjectAction::Retire).Succeeded());
		REQUIRE(GetTBMProject(this->rig.project.id) == nullptr);
		// The demo's state: the released project's support records stay on its corridor.
		REQUIRE_FALSE(CaptureTBMSupport().empty());
		REQUIRE(GetTBMSupportTile(this->area_start) != nullptr);
	}

	/** @return The read-only quote for project 1, as the underground window asks for it. */
	TBMCommissionAreaPreview Quote() const
	{
		return PreviewTBMCommissionArea(_current_company, this->source, this->rig.project.model_id, this->earth_wagons, this->rock_wagons,
			this->rig.fixture.origin->index, this->area_start, this->area_end, false);
	}

	/** Commission project 1 with the player's command; it reuses the released machine standing in the magazine. */
	void CommissionSecond()
	{
		const size_t before = CaptureTBMProjects().size();
		const auto bought = Command<Commands::CommissionTBM>::Do(DoCommandFlag::Execute, this->source, this->rig.project.model_id, this->earth_wagons,
			this->rock_wagons, this->rig.fixture.origin->index, this->area_start, this->area_end, false);
		INFO((bought.Failed() ? GetString(bought.GetErrorMessage()) : std::string{"bought"}));
		REQUIRE(bought.Succeeded());
		UpdateSignalsInBuffer(); // The command kernel finishes queued signals after a command.
		REQUIRE(CaptureTBMProjects().size() == before + 1);
		this->second = CaptureTBMProjects().back().id;
		REQUIRE(this->Second().cutter == this->rig.front->index);
	}

	/** @return Project 1. */
	const TBMProject &Second() const { return *GetTBMProject(this->second); }

	/** @return Project 1's job. */
	const ExcavationJob &SecondJob() const { return *GetExcavationJob(this->Second().job); }

	/** @return Whether every part of the cutter consist stands aside in its magazine. */
	bool IsCutterStored() const
	{
		for (const Train *part = this->rig.front; part != nullptr; part = part->Next()) {
			if (part->tile != this->source || part->track != Track::Depot) return false;
		}
		return true;
	}

	/**
	 * Park the rig's ordinary hauler, stopped, at the far side of a tile, as a player would leave a train there.
	 * @param tile Tile to stand on.
	 */
	void ParkHauler(TileIndex tile) const
	{
		Train *hauler = this->rig.hauler;
		const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(this->rock_end));
		// The far side of the tile, away from the cutter coming from the pair, so the two never touch.
		const int x = TileX(tile) * TILE_SIZE + TILE_SIZE / 2 + step.x * 6;
		const int y = TileY(tile) * TILE_SIZE + TILE_SIZE / 2 + step.y * 6;
		for (Train *part = hauler; part != nullptr; part = part->Next()) {
			part->tile = tile;
			part->track = TrackBits{DiagDirToDiagTrack(GetLayerPortalDoorDirection(this->rock_end))};
			part->x_pos = x;
			part->y_pos = y;
			part->vehstatus.Reset(VehState::Hidden);
			part->UpdatePosition();
		}
		hauler->vehstatus.Set(VehState::Stopped);
		hauler->cur_speed = 0;
	}
};

/** R9: the next project at the portal reuses the released project's support and cuts, live and after a reload. */
TEST_CASE("A project at a released portal cuts on over the old project's support", "[demo-fix-tbm-obstruction]")
{
	ReleasedCorridor corridor;
	corridor.ReleaseFirst();
	SECTION("live") {}
	SECTION("after the orphan records are reloaded") {
		// The registry goes through the loader boundary unchanged; a released record is a valid loaded row.
		const auto support = CaptureTBMSupport();
		ResetTBMSupportState();
		StageLoadedTBMSupport(support);
		REQUIRE(AcceptLoadedTBMSupport(GetNextTBMProjectID()));
		REQUIRE(CaptureTBMSupport() == support);
	}
	const auto quote = corridor.Quote();
	INFO("quote " << (quote.purchase.Failed() ? GetString(quote.purchase.GetErrorMessage()) : std::string{"accepted"}) << " failure " << quote.area.failure_tile.base());
	REQUIRE(quote.purchase.Succeeded());
	CHECK(quote.request.reuse_cutter == corridor.rig.front->index);
	corridor.CommissionSecond();
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, corridor.rig.front->index, true).Succeeded());
	// The first two cuts lie on the old corridor, whose cells carry project 0's records; the third opens new rock.
	corridor.Run([&corridor] { return corridor.SecondJob().cursor >= 3; }, 60000, true);
	CHECK(corridor.Second().blocked != TBMBlockedReason::Obstruction);
	// While project 1 works, the reused records stay protected: neither converted nor removed under the cutter's retreat.
	CHECK(Command<Commands::ConvertTBMSupport>::Do(DoCommandFlag{}, corridor.area_start).Failed());
	CHECK(IsTBMSupportInUse(corridor.area_start));
}

/**
 * A route whose support would meet existing track it may not use is refused in the quote, naming that tile, and the
 * command refuses it too. Owned track that only lacks the needed piece is the player's to complete: once that piece is
 * built, the same quote is accepted. A signal on the track can never carry support.
 */
TEST_CASE("A quote refuses a route whose support would meet existing track", "[demo-fix-tbm-obstruction]")
{
	ReleasedCorridor corridor;
	corridor.ReleaseFirst();
	// The player makes the old support permanent.
	for (const auto &support : CaptureTBMSupport()) REQUIRE(Command<Commands::ConvertTBMSupport>::Do(DoCommandFlag::Execute, support.tile).Succeeded());
	const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(corridor.rock_end));
	const TileIndex blocked = TileAddWrap(corridor.rock_end, step.x * 2, step.y * 2);
	const Track along = DiagDirToDiagTrack(GetLayerPortalDoorDirection(corridor.rock_end));
	const Track across = along == Track::X ? Track::Y : Track::X;
	bool fundable = false;
	SECTION("track lacking the needed piece") {
		// The second cell is relaid across the corridor: the cut would need the straight piece there.
		REQUIRE(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, blocked, along).Succeeded());
		REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, blocked, RAILTYPE_RAIL, across, false).Succeeded());
		fundable = true;
	}
	SECTION("a signal on the track") {
		REQUIRE(CmdBuildSingleSignal(DoCommandFlag::Execute, blocked, along, SignalType::Block, SignalVariant::Electric, false, false, false, SignalType::Block, SignalType::Block, 0, 0).Succeeded());
	}
	// Support never adds a piece to permanent track or shares a signalled one: the quote says so and marks the cell.
	const auto quote = corridor.Quote();
	CHECK(quote.purchase.Failed());
	CHECK(quote.purchase.GetErrorMessage() == (fundable ? STR_ERROR_TBM_COMMISSION_SUPPORT_PIECE : STR_ERROR_TBM_COMMISSION_SUPPORT));
	CHECK(quote.area.failure_tile == blocked);
	// The command refuses the same route, so no project starts and then waits forever.
	const size_t before = CaptureTBMProjects().size();
	CHECK(Command<Commands::CommissionTBM>::Do(DoCommandFlag::Execute, corridor.source, corridor.rig.project.model_id, corridor.earth_wagons,
		corridor.rock_wagons, corridor.rig.fixture.origin->index, corridor.area_start, corridor.area_end, false).Failed());
	CHECK(CaptureTBMProjects().size() == before);
	if (!fundable) return;
	// The player builds the missing piece as ordinary track; the quote is then accepted.
	REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, blocked, RAILTYPE_RAIL, along, false).Succeeded());
	CHECK(corridor.Quote().purchase.Succeeded());
}

/** A train in the next cut's sweep is named, and the cutter cuts on by itself once the train has left. */
TEST_CASE("A train in the cut's way is named and the cutter cuts on once it has left", "[demo-fix-tbm-obstruction]")
{
	ReleasedCorridor corridor;
	corridor.ReleaseFirst();
	corridor.CommissionSecond();
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, corridor.rig.front->index, true).Succeeded());
	corridor.Run([&corridor] { return corridor.SecondJob().cursor == 1 && corridor.Second().phase == TBMPhase::Bore; }, 30000, true);
	// A train left on the cell the next cut sweeps.
	const TileIndex next = corridor.SecondJob().route[1].cutter_position;
	corridor.ParkHauler(next);
	corridor.Run([&corridor] { return corridor.Second().blocked == TBMBlockedReason::Obstruction; }, 200);
	const std::string named = GetString(STR_TBM_HOLD_TRAIN_IN_WAY, corridor.rig.hauler->index);
	CHECK(GetTBMObstructionText(corridor.Second()) == named);
	const auto status = GetTBMVehicleStatus(*corridor.rig.front);
	REQUIRE(status.has_value());
	CHECK(status->find(named) != std::string::npos);
	CHECK(GetTBMWaitLine(corridor.Second(), corridor.Second().blocked).find(named) != std::string::npos);
	// The train goes back into its depot; nobody touches the cutter, which cuts on.
	corridor.rig.PlaceInDepot(corridor.rig.hauler, corridor.rig.depot);
	corridor.Run([&corridor] { return corridor.SecondJob().cursor == 2; }, 30000, true);
	CHECK_FALSE(GetTBMObstructionText(corridor.Second()).has_value());
}

/** The guard the demo's forced-out hauler made: a train on the key keeps the cutter in its magazine, named, until it leaves. */
TEST_CASE("A train on the housing keeps the cutter in its magazine until it leaves", "[demo-fix-tbm-obstruction]")
{
	ReleasedCorridor corridor;
	corridor.ReleaseFirst();
	corridor.CommissionSecond();
	REQUIRE(corridor.IsCutterStored());
	// A stopped train on the key's housing: the magazine lets one train move there at a time.
	corridor.rig.PlaceOnTrack(corridor.rig.hauler, corridor.source);
	corridor.rig.hauler->vehstatus.Set(VehState::Stopped);
	REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, corridor.rig.front->index, true).Succeeded());
	corridor.Run([&corridor] { return corridor.Second().phase == TBMPhase::TravelToFace; }, 200);
	for (uint tick = 0; tick < 200; ++tick) CallVehicleTicks();
	CHECK(corridor.IsCutterStored());
	CHECK(GetTBMObstructionText(corridor.Second()) == GetString(STR_TBM_HOLD_TRAIN_IN_WAY, corridor.rig.hauler->index));
	// Once the train has gone, the cutter leaves by itself.
	corridor.rig.PlaceInDepot(corridor.rig.hauler, corridor.rig.depot);
	corridor.Run([&corridor] { return !corridor.IsCutterStored(); }, 500);
	CHECK_FALSE(GetTBMObstructionText(corridor.Second()).has_value());
}
