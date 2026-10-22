/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file portal_movement.cpp Ordinary freight through an opened (Connected) rail pair, driven by the game's own vehicle
 * step, on both allowed rail pairs: Surface to Underground and Underground to Deep. A train crosses the pair and comes
 * back, reverses while it spans the pair, and occupies the signal block that runs through it.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 15 (15.2, 15.3)
 * @see docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-m-movement-design.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 17 (actual calendar-gated entry)
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
#include "../map_layer.h"
#include "../track_func.h"

#include "../safeguards.h"

/**
 * A freight train in a depot at the end of a line through an opened rail pair, with a block signal on each side of the
 * pair. The upper line runs from the upper end's door: door tile, signal tile, depot. The lower line runs from the lower
 * end's door: door tile, signal tile and two more tiles to a line end.
 */
class OpenedPairLine {
public:
	PathfinderDefaults pathfinder; ///< Real pathfinder settings for moving trains.
	TBMEngineFixture fixture; ///< Real company, profile, the original locomotive and the native hoppers.
	AutoRestoreBackup<UnitID> trains{_settings_game.vehicle.max_trains, UnitID{500}}; ///< Unit tests start with zero limits.
	AutoRestoreBackup<uint8_t> length{_settings_game.vehicle.max_train_length, uint8_t{7}}; ///< Seven tiles, the normal default.
	MapLayer upper; ///< Layer of the pair key.
	MapLayer lower; ///< Layer of the other end.
	TileIndex upper_end = INVALID_TILE; ///< Pair key.
	TileIndex lower_end = INVALID_TILE; ///< Other end of the pair.
	std::vector<TileIndex> upper_line; ///< Door tile, signal tile, depot.
	std::vector<TileIndex> lower_line; ///< Door tile, signal tile, two more tiles; the last is the line end.
	Train *train = nullptr; ///< The freight train: the original locomotive and three empty hoppers.

	/**
	 * Build the opened pair, both lines, the signals and the train.
	 * @param upper Layer of the pair key.
	 * @param lower Layer of the other end.
	 */
	OpenedPairLine(MapLayer upper, MapLayer lower) : upper(upper), lower(lower)
	{
		MockEnvironment::Instance(); // Moving trains read their sprite bounds from the sprite cache.
		this->fixture.stock->info.cargo_type = GetCargoTypeByLabel(CT_PASSENGERS);
		Engine::Get(GetTBMEngine(2))->company_avail.Set(_current_company);
		this->upper_end = TileXYL(40, 30, upper);
		this->lower_end = MultiLayerMap::TileInLayer(this->upper_end, lower);
		// The upper end's shaft heads SE, so its door and line face NW; the lower end mirrors it.
		const DiagDirection upper_door = DiagDirection::NW;
		const DiagDirection lower_door = DiagDirection::SE;
		for (TileIndex tile = this->upper_end; this->upper_line.size() < 3;) this->upper_line.push_back(tile = TileAddByDiagDir(tile, upper_door));
		for (TileIndex tile = this->lower_end; this->lower_line.size() < 4;) this->lower_line.push_back(tile = TileAddByDiagDir(tile, lower_door));

		// Rock is opened wherever the line runs below the Surface, both ends included, before anything is built there.
		std::vector<TileIndex> open{this->lower_end};
		open.insert(open.end(), this->lower_line.begin(), this->lower_line.end());
		if (upper != MapLayer::Surface) {
			open.push_back(this->upper_end);
			open.insert(open.end(), this->upper_line.begin(), this->upper_line.end());
		}
		this->fixture.world.Open(open);
		MakeRailLayerPortal(this->upper_end, _current_company, ReverseDiagDir(upper_door), lower, RAILTYPE_RAIL);
		MakeRailLayerPortal(this->lower_end, _current_company, ReverseDiagDir(lower_door), upper, RAILTYPE_RAIL);
		REQUIRE(GetLayerPortalPairTile(this->upper_end) == this->lower_end);
		REQUIRE(GetLayerPortalDoorDirection(this->upper_end) == upper_door);
		REQUIRE(GetLayerPortalDoorDirection(this->lower_end) == lower_door);

		// The pair is opened through the loader boundary, beside the fixture's own sealed pair.
		auto records = CaptureLayerPortalWorkRecords();
		LayerPortalWorkRecord opened;
		opened.pair_key = this->upper_end;
		opened.company = _current_company;
		opened.state = LayerPortalWorkState::Connected;
		records.push_back(opened);
		std::ranges::sort(records, {}, &LayerPortalWorkRecord::pair_key);
		REQUIRE(ValidateLayerPortalWorkRecords(records, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork(std::move(records));
		AcceptLoadedLayerPortalWork();
		REQUIRE(GetLayerPortalWorkState(this->upper_end) == LayerPortalWorkState::Connected);

		// Rail on the shaft axis, then the depot facing back up the upper line, then a two-way block signal each side.
		const Track axis = DiagDirToDiagTrack(upper_door);
		for (TileIndex tile : {this->upper_line[0], this->upper_line[1], this->lower_line[0], this->lower_line[1], this->lower_line[2], this->lower_line[3]}) {
			REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, axis, false).Succeeded());
		}
		REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, this->upper_line[2], RAILTYPE_RAIL, ReverseDiagDir(upper_door)).Succeeded());
		for (TileIndex tile : {this->upper_line[1], this->lower_line[1]}) {
			REQUIRE(CmdBuildSingleSignal(DoCommandFlag::Execute, tile, axis, SignalType::Block, SignalVariant::Electric, false, false, false, SignalType::Block, SignalType::Block, 0, 0).Succeeded());
		}
		UpdateSignalsInBuffer();
		REQUIRE(GetTileTrackStatus(this->upper_end, TransportType::Rail, RoadTramType::Invalid).trackdirs.Any());
		this->train = BuildTrainInDepot(this->upper_line[2], this->fixture.stock, Engine::Get(GetTBMEngine(2)), 3);
	}

	/** Start the train with the player's start command. */
	void Start() const
	{
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, this->train->index, true).Succeeded());
	}

	/** @return Whether the train has parts on both layers of the pair at once. */
	bool Spans() const
	{
		bool on_upper = false, on_lower = false;
		for (const Train *part = this->train; part != nullptr; part = part->Next()) {
			on_upper |= GetTileLayer(part->tile) == this->upper;
			on_lower |= GetTileLayer(part->tile) == this->lower;
		}
		return on_upper && on_lower;
	}

	/** @return Whether every part stands inside the depot. */
	bool IsHome() const
	{
		for (const Train *part = this->train; part != nullptr; part = part->Next()) {
			if (part->tile != this->upper_line[2] || part->track != Track::Depot) return false;
		}
		return true;
	}

	/**
	 * @param tile Signal tile, on the upper or the lower line.
	 * @param towards_pair Heading of the signalled trackdir: towards the pair.
	 * @return The state of the signal that guards entry into the pair's block from that side.
	 */
	static SignalState PairSignal(TileIndex tile, DiagDirection towards_pair)
	{
		const Trackdir td = DiagDirToDiagTrackdir(towards_pair);
		REQUIRE(HasSignalOnTrackdir(tile, td));
		return GetSignalStateByTrackdir(tile, td);
	}

	/** @return The upper signal into the pair's block. */
	SignalState UpperSignal() const { return PairSignal(this->upper_line[1], ReverseDiagDir(GetLayerPortalDoorDirection(this->upper_end))); }

	/** @return The lower signal into the pair's block. */
	SignalState LowerSignal() const { return PairSignal(this->lower_line[1], ReverseDiagDir(GetLayerPortalDoorDirection(this->lower_end))); }

	/**
	 * Run the game's vehicle step until a condition holds; the train must never crash or leave the tiles it stands on.
	 * @param done Condition.
	 * @param limit Most ticks to try.
	 * @return Ticks taken.
	 */
	uint Drive(const std::function<bool()> &done, uint limit = 6000) const
	{
		uint ticks = 0;
		// The game's own vehicle step: stations, every vehicle, and the restart of trains stopped on entering a depot.
		while (!done() && ticks < limit) {
			CallVehicleTicks();
			REQUIRE_FALSE(this->train->vehstatus.Test(VehState::Crashed));
			REQUIRE(IsTrainOnItsTiles(this->train));
			// The same spacing validator used after real loading must accept forward and reversed physical crossings;
			// legitimate shaft continuity cannot trigger the critical broken-train pause.
			CheckTrainsLengths();
			REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
			++ticks;
		}
		INFO("after " << ticks << " ticks: " << DescribeTrain(this->train));
		REQUIRE(done());
		return ticks;
	}
};

/**
 * @param section Index of the pair under test.
 * @return Upper and lower layer of each allowed rail pair.
 */
static std::pair<MapLayer, MapLayer> AllowedRailPair(int section)
{
	return section == 0 ? std::pair{MapLayer::Surface, MapLayer::Underground} : std::pair{MapLayer::Underground, MapLayer::DeepUnderground};
}

/** An existing actual fleet must wait outside Deep before the calendar unlock, then cross and return normally. */
TEST_CASE("Actual Deep rail traffic waits for the calendar gate then crosses both ways", "[portal-fleet-date]")
{
	OpenedPairLine line(MapLayer::Underground, MapLayer::DeepUnderground);
	line.Start();
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	// A real started train repeatedly tries its route; neither faster economic time nor existing Connected topology unlocks Deep.
	AutoRestoreBackup economy(TimerGameEconomy::date, TimerGameEconomy::ConvertYMDToDate(TimerGameEconomy::Year{2090}, 0, 1));
	for (uint tick = 0; tick < 1500; ++tick) {
		CallVehicleTicks();
		REQUIRE_FALSE(line.train->vehstatus.Test(VehState::Crashed));
		// Check the whole chain so a trailing wagon crossing early cannot hide behind the front engine's refusal.
		for (const Train *part = line.train; part != nullptr; part = part->Next()) REQUIRE(GetTileLayer(part->tile) == MapLayer::Underground);
	}
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	line.Drive([&line] { return GetTileLayer(line.train->tile) == MapLayer::DeepUnderground; });
	line.Drive([&line] { return line.IsHome(); });
}

/** Freight crosses an opened pair to the end of the lower line and comes back up to its depot. */
TEST_CASE("Freight crosses an opened rail pair and comes back", "[portal-movement]")
{
	int section = 0;
	SECTION("Surface to Underground") { section = 0; }
	SECTION("Underground to Deep") { section = 1; }
	const auto [upper, lower] = AllowedRailPair(section);
	OpenedPairLine line(upper, lower);
	line.Start();
	// Down through the shaft to the line end below, where the train turns round.
	line.Drive([&line] {
		for (const Train *part = line.train; part != nullptr; part = part->Next()) {
			if (part->tile == line.lower_line.back()) return true;
		}
		return false;
	});
	// Back up through the pair and into its depot.
	line.Drive([&line] { return line.IsHome(); });
}

/** A train that reverses while it spans an opened pair backs out whole on the side it came from. */
TEST_CASE("A train reversed while it spans an opened rail pair stays whole", "[portal-movement]")
{
	int section = 0;
	SECTION("Surface to Underground") { section = 0; }
	SECTION("Underground to Deep") { section = 1; }
	const auto [upper, lower] = AllowedRailPair(section);
	OpenedPairLine line(upper, lower);
	line.Start();
	line.Drive([&line] { return line.Spans(); });
	REQUIRE(Command<Commands::ReverseTrainDirection>::Do(DoCommandFlag::Execute, line.train->index, false).Succeeded());
	// Every part follows the new front back up through the shaft; none is left behind below.
	line.Drive([&line] { return line.IsHome(); });
	for (const Train *part = line.train; part != nullptr; part = part->Next()) CHECK(GetTileLayer(part->tile) == upper);
}

/** The pair and the lines beside it form one signal block: a train anywhere in it turns both signals into it red. */
TEST_CASE("A train spanning an opened rail pair occupies the block through it", "[portal-movement]")
{
	int section = 0;
	SECTION("Surface to Underground") { section = 0; }
	SECTION("Underground to Deep") { section = 1; }
	const auto [upper, lower] = AllowedRailPair(section);
	OpenedPairLine line(upper, lower);
	CHECK(line.UpperSignal() == SignalState::Green);
	CHECK(line.LowerSignal() == SignalState::Green);
	line.Start();
	line.Drive([&line] { return line.Spans(); });
	CHECK(line.UpperSignal() == SignalState::Red);
	CHECK(line.LowerSignal() == SignalState::Red);
	// Once the train is home again the block is free on both sides.
	line.Drive([&line] { return line.IsHome(); });
	CHECK(line.UpperSignal() == SignalState::Green);
	CHECK(line.LowerSignal() == SignalState::Green);
}
