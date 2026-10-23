/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file portal_signal_boundary.cpp Signal blocks end at a rail pair that is not open to the network (demo report R5).
 * A cutter working below its Commissioning pair no longer occupies the Surface block in front of the pair's door. A
 * train in a Surface depot on that line therefore leaves, whether the line has no signals, a block signal or a path
 * signal, and with path reservation off or on. A train moving on the pair's Surface end still occupies that block. The
 * block signal into it follows the cutter as it leaves its magazine, goes down the shaft, comes back up and is stored
 * again. An ordinary train is still turned back at the door. A Connected pair keeps its one block through both ends;
 * [portal-movement] covers that.
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 6 (tasks 6.2-6.6)
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group22C; native signal-component execution, no original save continuation.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_fixture.h"
#include "movement_fixture.h"
#include "../train_cmd.h"
#include "../rail_cmd.h"
#include "../rail_map.h"
#include "../signal_func.h"
#include "../vehicle_cmd.h"
#include "../command_func.h"
#include "../layer_portal_map.h"
#include "../layer_portal_cmd.h"
#include "../map_layer.h"
#include "../track_func.h"

#include "../safeguards.h"

/** Which signal stands on the line between the depot and the pair's door. */
enum class LineSignals : uint8_t {
	None, ///< No signal: the depot's block runs up to the door, as on the demo line.
	Block, ///< A two-way block signal on the middle tile.
	Path, ///< A two-way path signal on the middle tile.
};

/**
 * A commissioned project whose pair has a short Surface line in front of its door: the door tile, a middle tile that
 * may carry a signal, and a depot facing back up the line with a stopped ordinary train inside. The project's consist
 * stands stopped in its magazine, as commissioning leaves it.
 */
class BoundaryLine {
public:
	PathfinderDefaults pathfinder; ///< Real pathfinder settings for moving trains.
	AutoRestoreBackup<bool> reserve; ///< Path reservation for every train, or only behind path signals.
	TBMProjectRig rig{6}; ///< Project with six straight cuts out of the rock end: room for the whole consist below.
	LineSignals signals; ///< The signal on the middle tile.
	TileIndex source = INVALID_TILE; ///< Surface pair key: the magazine and Source bay.
	TileIndex rock_end = INVALID_TILE; ///< Rock end of the pair.
	TileIndex approach = INVALID_TILE; ///< Door tile: where the line ends for an ordinary train.
	TileIndex middle = INVALID_TILE; ///< Middle tile, where the signal stands.
	TileIndex depot = INVALID_TILE; ///< Depot at the far end of the line.
	Train *freight = nullptr; ///< Ordinary train in the depot.

	/**
	 * Build the line, its signal and the ordinary train.
	 * @param signals Signal on the middle tile.
	 * @param reserve_paths Whether every train reserves its path, as the "reserve_paths" setting does.
	 */
	BoundaryLine(LineSignals signals, bool reserve_paths) : reserve(_settings_game.pf.reserve_paths, reserve_paths), signals(signals)
	{
		this->source = this->rig.fixture.request.portal;
		this->rock_end = GetLayerPortalPairTile(this->source);
		const DiagDirection door = GetLayerPortalDoorDirection(this->source);
		const Track axis = DiagDirToDiagTrack(door);
		this->approach = GetLayerPortalDoorTile(this->source);
		this->middle = TileAddByDiagDir(this->approach, door);
		this->depot = TileAddByDiagDir(this->middle, door);
		for (TileIndex tile : {this->approach, this->middle}) {
			REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, axis, false).Succeeded());
		}
		REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, this->depot, RAILTYPE_RAIL, ReverseDiagDir(door)).Succeeded());
		if (signals != LineSignals::None) {
			const SignalType type = signals == LineSignals::Block ? SignalType::Block : SignalType::Path;
			REQUIRE(CmdBuildSingleSignal(DoCommandFlag::Execute, this->middle, axis, type, SignalVariant::Electric, false, false, false, SignalType::Block, SignalType::Block, 0, 0).Succeeded());
		}
		// The command kernel finishes queued signals after each command.
		UpdateSignalsInBuffer();
		this->rig.fixture.stock->info.cargo_type = GetCargoTypeByLabel(CT_PASSENGERS);
		this->freight = BuildTrainInDepot(this->depot, this->rig.fixture.stock, Engine::Get(GetTBMEngine(2)), 3);
	}

	/** @return The live project. */
	const TBMProject &Project() const { return *GetTBMProject(this->rig.project.id); }

	/**
	 * Start or stop a train with the player's start/stop command.
	 * @param train Front engine.
	 */
	static void StartStop(const Train *train)
	{
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, train->index, true).Succeeded());
	}

	/** @return Whether a part of the cutter consist moves on the pair's Surface end rather than standing aside in its magazine. */
	bool IsCutterOnKey() const
	{
		// Inspect the actual whole chain: one remaining moving part still owns its local block.
		for (const Train *part = this->rig.front; part != nullptr; part = part->Next()) {
			if (part->tile == this->source && part->track != Track::Depot) return true;
		}
		return false;
	}

	/** @return Whether every part of the cutter consist is below the Surface: on the rock end or in the corridor. */
	bool IsCutterBelow() const
	{
		// Inspect the actual whole chain: one remaining moving part still owns its local block.
		for (const Train *part = this->rig.front; part != nullptr; part = part->Next()) {
			if (GetTileLayer(part->tile) == MapLayer::Surface) return false;
		}
		return true;
	}

	/** @return Whether every part of the cutter consist stands aside in its magazine. */
	bool IsCutterStored() const
	{
		// Inspect the actual whole chain: one remaining moving part still owns its local block.
		for (const Train *part = this->rig.front; part != nullptr; part = part->Next()) {
			if (part->tile != this->source || part->track != Track::Depot) return false;
		}
		return true;
	}

	/**
	 * @param tile Tile to look at.
	 * @return Whether a part of the ordinary train stands on it.
	 */
	bool FreightOn(TileIndex tile) const
	{
		// Inspect the actual whole chain: one remaining moving part still owns its local block.
		for (const Train *part = this->freight; part != nullptr; part = part->Next()) {
			if (part->tile == tile) return true;
		}
		return false;
	}

	/** @return Whether every part of the ordinary train stands inside its depot. */
	bool IsFreightHome() const
	{
		// Inspect the actual whole chain: one remaining moving part still owns its local block.
		for (const Train *part = this->freight; part != nullptr; part = part->Next()) {
			if (part->tile != this->depot || part->track != Track::Depot) return false;
		}
		return true;
	}

	/** @return The block signal's state for a train heading towards the pair, as last set; nothing recomputes it here. */
	SignalState PairSignal() const
	{
		const Trackdir td = DiagDirToDiagTrackdir(ReverseDiagDir(GetLayerPortalDoorDirection(this->source)));
		REQUIRE(HasSignalOnTrackdir(this->middle, td));
		return GetSignalStateByTrackdir(this->middle, td);
	}

	/**
	 * Recompute the Surface block in front of the pair's door, from the key's door outwards. Without a signal it is the
	 * depot's block as well, which is what the depot rule evaluates before a train may leave.
	 * @return The block's state; Full when a train is inside it.
	 */
	SigSegState PairBlock() const { return UpdateSignalsOnSegment(this->source, DiagDirection::Invalid, _current_company); }

	/**
	 * @param occupied Whether a train is expected inside the block in front of the door.
	 * @return The state PairBlock must report. A block with a path signal always reports Path: reservations, not
	 *         occupancy, guard it.
	 */
	SigSegState ExpectedBlock(bool occupied) const
	{
		if (this->signals == LineSignals::Path) return SigSegState::Path;
		return occupied ? SigSegState::Full : SigSegState::Free;
	}

	/** At an exchange, empty the cutter consist where it stands: a stand-in for the hauler loop that [tbm-loop] drives. */
	void SupplyEmpties() const
	{
		if (this->Project().phase != TBMPhase::WaitForEmpties) return;
		// Declared component emptying replaces the separate hauler loop only; signal movement itself remains native.
		for (Train *part = this->rig.front; part != nullptr; part = part->Next()) part->cargo.Truncate();
	}

	/**
	 * Run the game's vehicle step until a condition holds. Neither train may crash, and the ordinary train may never
	 * enter the Commissioning pair: its line ends at the door.
	 * @param done Condition.
	 * @param limit Most ticks to try.
	 * @param empties Whether each exchange is served at once (SupplyEmpties), so the cutter works on along its route.
	 */
	void Run(const std::function<bool()> &done, uint limit, bool empties = false) const
	{
		uint ticks = 0;
		// The game's own vehicle step moves both trains; the TBM controller runs inside the cutter's ticks.
		while (!done() && ticks < limit) {
			if (empties) this->SupplyEmpties();
			CallVehicleTicks();
			REQUIRE_FALSE(this->rig.front->vehstatus.Test(VehState::Crashed));
			REQUIRE_FALSE(this->freight->vehstatus.Test(VehState::Crashed));
			REQUIRE_FALSE(this->FreightOn(this->source));
			++ticks;
		}
		INFO("after " << ticks << " ticks\ncutter: " << DescribeTrain(this->rig.front) << "\nfreight: " << DescribeTrain(this->freight));
		REQUIRE(done());
	}
};

/**
 * The demo's R5 hold, with each kind of signal in front of the door. The cutter on the pair's Surface end occupies the
 * Surface block; once it is wholly below, the block is free and the ordinary train leaves its depot and drives up to
 * the door, even though the cutter is still working in the corridor.
 */
TEST_CASE("A cutter below its Commissioning pair leaves the Surface block in front of the door free", "[tbm-portal-signals]")
{
	const LineSignals signals = GENERATE(LineSignals::None, LineSignals::Block, LineSignals::Path);
	const bool reserve_paths = GENERATE(false, true);
	INFO("signals " << static_cast<int>(to_underlying(signals)) << " reserve_paths " << reserve_paths);
	BoundaryLine line(signals, reserve_paths);
	const bool block_signal = signals == LineSignals::Block;

	// Before anything moves, the block in front of the door is free and its block signal shows green.
	if (block_signal) CHECK(line.PairSignal() == SignalState::Green);
	CHECK(line.PairBlock() == line.ExpectedBlock(false));

	// Leaving its magazine, the cutter moves on the pair's Surface end: it is inside the block, and the block signal
	// turned red when it left, as for a train leaving a depot.
	BoundaryLine::StartStop(line.rig.front);
	line.Run([&line] { return line.IsCutterOnKey(); }, 2000);
	if (block_signal) CHECK(line.PairSignal() == SignalState::Red);
	CHECK(line.PairBlock() == line.ExpectedBlock(true));

	// Once its last part has gone down the shaft, the block is free and the signal turned green again. The corridor's
	// own block, which starts at the rock end's door, holds the cutter, and no reservation is left on the key.
	line.Run([&line] { return line.IsCutterBelow(); }, 60000, true);
	REQUIRE(line.Project().phase != TBMPhase::Completed);
	if (block_signal) CHECK(line.PairSignal() == SignalState::Green);
	CHECK(line.PairBlock() == line.ExpectedBlock(false));
	CHECK(UpdateSignalsOnSegment(line.rock_end, DiagDirection::Invalid, _current_company) == SigSegState::Full);
	CHECK_FALSE(HasLayerPortalReservation(line.source));

	// The ordinary train leaves its depot within the depot rule's wait while the cutter is still below (the R5 hold),
	// passes the signal, reaches the door where its line ends, and comes home.
	BoundaryLine::StartStop(line.freight);
	line.Run([&line] { return line.freight->track != Track::Depot; }, 200);
	CHECK(line.IsCutterBelow());
	line.Run([&line] { return line.FreightOn(line.approach); }, 1500);
	line.Run([&line] { return line.IsFreightHome(); }, 3000);
	BoundaryLine::StartStop(line.freight);
	REQUIRE(line.freight->IsStoppedInDepot());

	// At the end of its route the cutter comes back up the shaft and is stored in its magazine for an exchange. The block
	// is free again, its signal shows green, and the key keeps no reservation.
	line.Run([&line] { return line.IsCutterStored() && line.Project().phase == TBMPhase::WaitForEmpties; }, 60000);
	if (block_signal) CHECK(line.PairSignal() == SignalState::Green);
	CHECK(line.PairBlock() == line.ExpectedBlock(false));
	CHECK_FALSE(HasLayerPortalReservation(line.source));
}
