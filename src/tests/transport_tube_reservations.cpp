/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_reservations.cpp Real capsule junction contention, layout recovery and physical turnaround.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-underground-transport-tubes.md Groups3/4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md task10.1
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "transport_tube_fixture.h"
#include "movement_fixture.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../pbs.h"
#include "../signal_func.h"
#include "../pathfinder/follow_track.hpp"

#include "../safeguards.h"

/** Command-built three-arm tube junction; actual train ticks own movement and existing PBS owns every reservation. */
class TubeJunctionRig : public TubePrototypeRig {
	PathfinderDefaults pathfinder; ///< Restore real default routing budgets after the fixture.
	AutoRestoreBackup<bool> paths{_settings_game.pf.reserve_paths, true}; ///< Require ordinary exclusive path reservations.
	AutoRestoreBackup<AccelerationModel> acceleration{_settings_game.vehicle.train_acceleration_model, AccelerationModel::Realistic}; ///< Exercise physical braking.
public:
	TileIndex junction = TileXYL(38, 43, MapLayer::Underground); ///< Shared physical Y/Right conflict cell.
	TileIndex destination = TileXYL(38, 48, MapLayer::Underground); ///< Actual halt/service depot beyond the common branch.
	TileIndex side = TileXYL(34, 43, MapLayer::Underground); ///< Independent second capsule entry depot.
	std::vector<Train *> capsules; ///< Observe existing vehicle owners; this list grants neither movement nor reservations.

	/** Build only fixture-open floor, then real track/depot commands; electrical metering remains a later producer. */
	TubeJunctionRig()
	{
		// One shared output and two approaches exercise a real merging junction, not a second synthetic lease table.
		for (int y = 41; y < 48; ++y) this->BuildTrack(TileXYL(38, y, MapLayer::Underground), this->tube, Track::Y);
		for (int x = 35; x < 38; ++x) this->BuildTrack(TileXYL(x, 43, MapLayer::Underground), this->tube, Track::X);
		this->BuildTrack(this->junction, this->tube, Track::Right);
		this->Open(std::array{this->destination, this->side});
		REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, this->destination, this->tube, DiagDirection::NW).Succeeded());
		REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, this->side, this->tube, DiagDirection::SW).Succeeded());
		UpdateSignalsInBuffer();
	}

	/**
	 * @param source Actual owned tube purchase depot.
	 * @return Command-bought capsule with a normal halt-at-depot order and one real initial cargo packet.
	 */
	Train *BuyForRoute(TileIndex source)
	{
		const auto [result, id, capacity, mail, capacities] = CmdBuildVehicle(DoCommandFlag::Execute,
			source, GetTubeEngine(9), false, INVALID_CARGO, ClientID::Invalid);
		REQUIRE(result.Succeeded());
		Train *capsule = Train::Get(id);
		Order order;
		order.MakeGoToDepot(Depot::GetByTile(this->destination)->index, {OrderDepotTypeFlag::PartOfOrders},
			OrderNonStopFlag::NonStop, {OrderDepotActionFlag::Halt});
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, id, 0, order).Succeeded());
		ProcessOrders(capsule);
		// Initial packet custody is explicit fixture input. Driving and depot service must never synthesize or dispose of it.
		REQUIRE(CargoPacket::CanAllocateItem());
		CargoPacket *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
		packet->UpdateLoadingTile(source);
		capsule->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
		capsule->ConsistChanged(CCF_LOADUNLOAD);
		this->capsules.push_back(capsule);
		return capsule;
	}

	/** @param capsule Real front engine to start through the normal player command. */
	void Start(Train *capsule) const
	{
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule->index, true).Succeeded());
	}

	/** @param ticks Bounded number of whole native train ticks; update every observed capsule in stable purchase order. */
	void Run(uint ticks) const
	{
		UpdateSignalsInBuffer(); // Direct fixture commands must finish the normal command dispatcher's buffered layout updates before train ticks.
		// Stable real owner order gives repeatable competition and checks physical/cargo safety during every hold and crossing.
		for (uint tick = 0; tick < ticks; ++tick) {
			for (Train *capsule : this->capsules) {
				REQUIRE(capsule->Tick());
				REQUIRE_FALSE(capsule->vehstatus.Test(VehState::Crashed));
				REQUIRE(IsTrainOnItsTiles(capsule));
				REQUIRE(capsule->cargo.TotalCount() == 17);
			}
		}
	}

	/**
	 * @param capsule Actual capsule whose physical target is awaited.
	 * @param tile World target reached through real ticks, never fixture relocation.
	 * @param depot Require the complete stopped depot state rather than merely entering its tile.
	 */
	void DriveTo(Train *capsule, TileIndex tile, bool depot = false) const
	{
		uint ticks = 0;
		// A depot arrival includes real entry, hiding and braking; a track arrival permits observing an occupied footprint in motion.
		while (ticks < 6000) {
			if (capsule->tile == tile && (!depot || capsule->IsStoppedInDepot())) break;
			this->Run(1);
			++ticks;
		}
		INFO(DescribeTrain(capsule));
		REQUIRE(ticks < 6000);
	}
};

/** Two independent real depot approaches compete for one physical merge; clearing the first route permits the second. */
TEST_CASE("Tube junction contenders retain exclusive native reservations and recover", "[transport-tube-reservations]")
{
	MockEnvironment::Instance();
	TubeJunctionRig world;
	Train *first = world.BuyForRoute(world.depot);
	Train *second = world.BuyForRoute(world.side);
	CHECK(first->gcache.cached_total_length == first->gcache.cached_veh_length);
	CHECK(first->gcache.cached_total_length == VEHICLE_LENGTH);
	const CargoPacketID first_packet = first->cargo.Packets()->front()->index;
	const CargoPacketID second_packet = second->cargo.Packets()->front()->index;
	world.Start(first);
	world.DriveTo(first, TileXYL(38, 41, MapLayer::Underground));
	// The train handler requests its departure path while leaving the depot.
	// Observing it on real track avoids manufacturing an ownerless hidden-depot reservation with an out-of-band request.
	REQUIRE(TryPathReserve(first, true));
	CHECK(GetReservedTrackbits(world.junction).Test(Track::Y));
	CHECK(GetTrainForReservation(world.junction, Track::Y) == first);
	CHECK_FALSE(TryPathReserve(second, true));
	CHECK_FALSE(GetReservedTrackbits(world.junction).Test(Track::Right));
	world.Start(second);
	world.DriveTo(first, world.junction);
	CHECK(second->tile == world.side);
	CHECK_FALSE(CmdRemoveSingleRail(DoCommandFlag::Execute, world.junction, Track::Y).Succeeded());
	world.DriveTo(first, world.destination, true);
	CHECK(GetReservedTrackbits(world.junction).None());
	world.DriveTo(second, TileXYL(35, 43, MapLayer::Underground));
	REQUIRE(TryPathReserve(second, true));
	CHECK(GetReservedTrackbits(world.junction).Test(Track::Right));
	CHECK(GetTrainForReservation(world.junction, Track::Right) == second);
	world.DriveTo(second, world.destination, true);
	CHECK(GetReservedTrackbits(world.junction).None());
	CHECK(first->cargo.Packets()->front()->index == first_packet);
	CHECK(second->cargo.Packets()->front()->index == second_packet);
}

/** A real removal releases only the future path, retains the occupied source and blocks departure until the branch is restored. */
TEST_CASE("Tube closed branch releases future reservation and recovers without relocation", "[transport-tube-reservations]")
{
	MockEnvironment::Instance();
	TubeJunctionRig world;
	AutoRestoreBackup<uint8_t> retry(_settings_game.pf.wait_for_pbs_path, uint8_t{1});
	AutoRestoreBackup<bool> reverse_wait(_settings_game.pf.reverse_at_signals, true);
	// Closing the unused alternative arm leaves an actual broken destination branch,
	// so a safe fallback into the side depot cannot be mistaken for a blocked-route test.
	REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, world.junction, Track::Right).Succeeded());
	Train *capsule = world.BuyForRoute(world.depot);
	world.Start(capsule);
	world.DriveTo(capsule, TileXYL(38, 42, MapLayer::Underground));
	REQUIRE(TryPathReserve(capsule, true));
	const auto removed = TileXYL(38, 45, MapLayer::Underground);
	REQUIRE(GetReservedTrackbits(removed).Test(Track::Y));
	REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, removed, Track::Y).Succeeded());
	CHECK(GetReservedTrackbits(removed).None());
	const auto held = capsule->tile;
	CHECK(GetReservedTrackbits(held).Test(Track::Y));
	// Native removal may immediately reserve the still-safe open prefix. The capsule
	// may traverse that prefix, but must stop at the last real cell and never reverse itself or cross the missing tile.
	world.Run(300);
	CHECK(capsule->tile == TileXYL(38, 44, MapLayer::Underground));
	CHECK(capsule->cur_speed == 0);
	CHECK_FALSE(capsule->vehicle_flags.Test(VehicleFlag::DrivingBackwards));
	CHECK(GetReservedTrackbits(world.junction).None());
	CHECK_FALSE(CmdRemoveSingleRail(DoCommandFlag::Execute, capsule->tile, Track::Y).Succeeded());
	CHECK_FALSE(CmdConvertRail(DoCommandFlag::Execute, capsule->tile, capsule->tile, RAILTYPE_MAGLEV, false).Succeeded());
	const bool manual_stop = GENERATE(false, true);
	if (manual_stop) world.Start(capsule); // Toggle the normal manual stop while the closed-network capsule is already motionless.
	const auto stopped_x = capsule->x_pos;
	const auto stopped_y = capsule->y_pos;
	REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, removed, world.tube, Track::Y, false).Succeeded());
	UpdateSignalsInBuffer();
	if (manual_stop) {
		world.Run(100);
		CHECK(capsule->vehstatus.Test(VehState::Stopped));
		CHECK(capsule->x_pos == stopped_x);
		CHECK(capsule->y_pos == stopped_y);
		world.Start(capsule);
	}
	world.DriveTo(capsule, world.destination, true);
}

/** A moving reverse command must brake before changing travel direction, including when ordinary trains use original acceleration. */
TEST_CASE("Tube turnaround brakes before backing and preserves physical cargo custody", "[transport-tube-turnaround]")
{
	MockEnvironment::Instance();
	TubeJunctionRig world;
	const auto model = GENERATE(AccelerationModel::Original, AccelerationModel::Realistic);
	AutoRestoreBackup<AccelerationModel> acceleration(_settings_game.vehicle.train_acceleration_model, model);
	Train *capsule = world.BuyForRoute(world.depot);
	const CargoPacketID packet = capsule->cargo.Packets()->front()->index;
	world.Start(capsule);
	world.DriveTo(capsule, TileXYL(38, 42, MapLayer::Underground));
	REQUIRE(capsule->cur_speed > 0);
	const auto speed = capsule->cur_speed;
	const auto facing = capsule->direction;
	const auto tile = capsule->tile;
	const auto x = capsule->x_pos;
	const auto y = capsule->y_pos;
	REQUIRE(CmdReverseTrainDirection(DoCommandFlag::Execute, capsule->index, false).Succeeded());
	CHECK(capsule->cur_speed == speed);
	CHECK(capsule->flags.Test(VehicleRailFlag::Reversing));
	CHECK_FALSE(capsule->vehicle_flags.Test(VehicleFlag::DrivingBackwards));
	CHECK(capsule->direction == facing);
	CHECK(capsule->tile == tile);
	CHECK(capsule->x_pos == x);
	CHECK(capsule->y_pos == y);
	uint ticks = 0;
	// ReverseTrainDirection is reached by the ordinary handler only after braking; there is no replacement movement controller.
	while (!capsule->vehicle_flags.Test(VehicleFlag::DrivingBackwards) && ticks < 1000) {
		world.Run(1);
		++ticks;
	}
	REQUIRE(ticks < 1000);
	CHECK(capsule->direction == facing);
	CHECK(capsule->cargo.Packets()->front()->index == packet);
	Order back;
	back.MakeGoToDepot(Depot::GetByTile(world.depot)->index, {OrderDepotTypeFlag::PartOfOrders},
		OrderNonStopFlag::NonStop, {OrderDepotActionFlag::Halt});
	REQUIRE(CmdDeleteOrder(DoCommandFlag::Execute, capsule->index, 0).Succeeded());
	REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, capsule->index, 0, back).Succeeded());
	ProcessOrders(capsule);
	world.DriveTo(capsule, world.depot, true);
	CHECK(capsule->cargo.Packets()->front()->index == packet);
}

/** Track-layout notifications retire a warmed route; broad mechanics never permit a converted foreign cell or discard the occupied footprint. */
TEST_CASE("Tube cached route cannot enter a converted conventional branch", "[transport-tube-reservations]")
{
	MockEnvironment::Instance();
	TubeJunctionRig world;
	Company::Get(_current_company)->avail_railtypes.Set(RAILTYPE_MAGLEV); // Explicit actual company availability permits testing conversion rather than a locked-tool refusal.
	REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, world.junction, Track::Right).Succeeded());
	Train *capsule = world.BuyForRoute(world.depot);
	world.Start(capsule);
	world.DriveTo(capsule, TileXYL(38, 42, MapLayer::Underground));
	const auto changed = TileXYL(38, 45, MapLayer::Underground);
	REQUIRE(GetReservedTrackbits(changed).Test(Track::Y));
	CHECK_FALSE(CmdConvertRail(DoCommandFlag::Execute, changed, changed, RAILTYPE_MAGLEV, false).Succeeded());
	CHECK(GetRailType(changed) == world.tube);
	FreeTrainTrackReservation(capsule);
	CHECK(GetReservedTrackbits(capsule->tile).Test(Track::Y));
	CHECK(GetReservedTrackbits(changed).None());
	REQUIRE(CmdConvertRail(DoCommandFlag::Execute, changed, changed, RAILTYPE_MAGLEV, false).Succeeded());
	capsule->compatible_railtypes.Set(RAILTYPE_MAGLEV);
	RailTypeInfo *definition = const_cast<RailTypeInfo *>(GetRailTypeInfo(world.tube));
	AutoRestoreBackup<RailTypes> broad(definition->compatible_railtypes, RailTypes{world.tube, RAILTYPE_MAGLEV});
	CFollowTrackRail follower(capsule);
	CHECK_FALSE(follower.Follow(TileXYL(38, 44, MapLayer::Underground), Trackdir::Y_SE));
	world.Run(300);
	CHECK(capsule->tile == TileXYL(38, 44, MapLayer::Underground));
	CHECK(capsule->cur_speed == 0);
	CHECK_FALSE(capsule->vehicle_flags.Test(VehicleFlag::DrivingBackwards));
	CHECK(GetRailType(changed) == RAILTYPE_MAGLEV);
	REQUIRE(CmdConvertRail(DoCommandFlag::Execute, changed, changed, world.tube, false).Succeeded());
	world.DriveTo(capsule, world.destination, true);
}

/** The dedicated tube policy permits straight/45-degree choices and refuses a sharp 90-degree step regardless of ordinary settings. */
TEST_CASE("Tube track geometry is independent of ordinary sharp turn settings", "[transport-tube-geometry]")
{
	TubePrototypeRig world;
	Train *capsule = world.BuyCapsule();
	const bool forbid = GENERATE(false, true);
	AutoRestoreBackup<bool> ordinary_setting(_settings_game.pf.forbid_90_deg, forbid);
	const auto source = TileXYL(44, 44, MapLayer::Underground);
	const auto next = TileXYL(43, 44, MapLayer::Underground);
	world.BuildTrack(source, world.tube, Track::Upper);
	world.BuildTrack(next, world.tube, Track::X);
	world.BuildTrack(next, world.tube, Track::Lower);
	world.BuildTrack(next, world.tube, Track::Left);
	CFollowTrackRail follower(capsule);
	REQUIRE(follower.Follow(source, Trackdir::Upper_E));
	CHECK(follower.new_tile == next);
	CHECK(follower.new_td_bits == TrackdirBits{Trackdir::X_NE, Trackdir::Lower_E});
	CHECK_FALSE(follower.new_td_bits.Test(Trackdir::Left_N));
	CHECK(Rail90DegTurnDisallowed(world.tube, world.tube, false));
	REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, next, Track::X).Succeeded());
	REQUIRE(CmdRemoveSingleRail(DoCommandFlag::Execute, next, Track::Lower).Succeeded());
	CFollowTrackRail sharp(capsule);
	CHECK_FALSE(sharp.Follow(source, Trackdir::Upper_E));
	CHECK(sharp.err == CFollowTrackRail::ErrorCode::SharpTurn);
}

/** A command-bought capsule follows both actual intermediate headings through the merging curve, without a pivot or TBM assignment. */
TEST_CASE("Tube capsule makes a ninety degree change through two traversed bends", "[transport-tube-geometry]")
{
	MockEnvironment::Instance();
	TubeJunctionRig world;
	AutoRestoreBackup<bool> ordinary_setting(_settings_game.pf.forbid_90_deg, false);
	Train *capsule = world.BuyForRoute(world.side);
	world.Start(capsule);
	std::vector<Direction> headings;
	std::set<TileIndex> visited;
	uint ticks = 0;
	// Record only real visible motion: depot hiding/servicing cannot manufacture a traversed heading or count as a curve cell.
	while (ticks < 6000) {
		world.Run(1);
		if (!capsule->vehstatus.Test(VehState::Hidden)) {
			visited.insert(capsule->tile);
			// Keep transitions once so the assertion proves the physical intermediate 45-degree facing, independently of time spent there.
			if (headings.empty() || headings.back() != capsule->direction) headings.push_back(capsule->direction);
		}
		++ticks;
		// The outcome requires actual service-depot entry and a stopped capsule, not a last-track-cell placement.
		if (capsule->tile == world.destination && capsule->IsStoppedInDepot()) break;
	}
	REQUIRE(ticks < 6000);
	CHECK(headings == std::vector<Direction>{Direction::SW, Direction::S, Direction::SE});
	CHECK(visited.contains(world.junction));
	CHECK(visited.contains(TileXYL(37, 43, MapLayer::Underground)));
	CHECK(visited.contains(TileXYL(38, 44, MapLayer::Underground)));
}
