/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_freight_fixture.h Command-built tube/rail/road interchange and real native loading visits.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9.
 * Fixture-open geology and the commissioned inlet are explicit boundary input;
 * no test here claims the later TBM-cleared useful-route acceptance.
 */

#ifndef TESTS_TRANSPORT_TUBE_FREIGHT_FIXTURE_H
#define TESTS_TRANSPORT_TUBE_FREIGHT_FIXTURE_H

#include "transport_tube_power_fixture.h"
#include "../transport_tube_cargo.h"
#include "../economy_base.h"
#include "../economy_func.h"
#include "../station_func.h"
#include "../order_cmd.h"

/** Real native joined station and purchased carriers, with no secondary stock, packet or transfer-payment owner. */
class TubeFreightRig : public TubePowerRig {
	AutoRestoreBackup<bool> distant{_settings_game.station.distant_join_stations, true}; ///< Restore normal station-join preference.
	AutoRestoreBackup<uint8_t> indicators{_settings_client.gui.loading_indicators, uint8_t{0}}; ///< No graphical text effects in this isolated command executable.
	AutoRestoreBackup<Money> rate{CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"COAL"}))->current_payment, Money{10000}}; ///< Explicit nonzero freight-rate boundary; the reduced registry otherwise has zero payments.
	AutoRestoreBackup<uint8_t> feeder{_settings_game.economy.feeder_payment_share, uint8_t{75}}; ///< Exercise real feeder settlement with a declared positive native share.
public:
	TileIndex tube_tile = TileXYL(38, 44, MapLayer::Underground); ///< Actual two-cell Y-axis capsule berth.
	TileIndex rail_tile = TileXYL(41, 44, MapLayer::Underground); ///< Physically separate ordinary berth with the same packet owner.
	TileIndex road_tile = TileXYL(44, 44, MapLayer::Underground); ///< Native joined truck stop, never a traversable capsule cell.
	Station *station = nullptr; ///< Sole actual joined cargo/loading owner.
	CargoType cargo = GetCargoTypeByLabel(CargoLabel{"COAL"}); ///< Canonical fixture-resolved bulk meaning.

	/** Build native separate berths and the actual truck stop on precisely fixture-open floor. */
	TubeFreightRig()
	{
		this->BuildPlatform(this->tube_tile, this->tube, NEW_STATION);
		this->station = Station::GetByTile(this->tube_tile);
		this->BuildPlatform(this->rail_tile, RAILTYPE_RAIL, this->station->index);
		this->Open(std::array{this->road_tile, TileAddWrap(this->road_tile, 0, 1)});
		REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, this->road_tile, 1, 1, RoadStopType::Truck, false,
			DiagDirection::SE, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, this->station->index, false).Succeeded());
	}

	/**
	 * Build a native loading platform and its fully excavated approaches.
	 * @param tile Actual first berth cell.
	 * @param type Canonical tube or ordinary rail identity.
	 * @param join Actual station owner or NEW_STATION for the first facility.
	 */
	void BuildPlatform(TileIndex tile, RailType type, StationID join)
	{
		// Both end approaches are construction prerequisites; opening only the
		// painted footprint must never make a terminal operational in solid rock.
		for (int along = -1; along <= 2; ++along) this->Open(std::array{TileAddWrap(tile, 0, along)});
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, tile, type, Axis::Y, 1, 2,
			STAT_CLASS_DFLT, 0, join, false).Succeeded());
	}

	/** @return Actual command-built conventional source station, including native spatial/catchment registration. */
	Station *BuildSource()
	{
		const auto tile = TileXYL(8, 8, MapLayer::Underground);
		this->BuildPlatform(tile, RAILTYPE_RAIL, NEW_STATION);
		return Station::GetByTile(tile);
	}

	/**
	 * Supply a real customer packet as explicit cargo boundary input.
	 * @param vehicle Actual command-bought compatible carrier, with enough physical hold space.
	 * @param count Whole fixture units to place in that real hold.
	 * @param origin Original native source station.
	 * @return Pool-owned packet; callers must not retain its pointer after a native move/merge.
	 */
	CargoPacket *LoadPacket(Vehicle &vehicle, uint16_t count, StationID origin)
	{
		REQUIRE(vehicle.cargo_type == this->cargo);
		REQUIRE(vehicle.cargo.TotalCount() + count <= vehicle.cargo_cap);
		REQUIRE(CargoPacket::CanAllocateItem());
		CargoPacket *packet = CargoPacket::Create(origin, count, Source{IndustryID::Invalid(), SourceType::Industry});
		packet->UpdateLoadingTile(TileXYL(8, 8, MapLayer::Underground));
		vehicle.cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
		return packet;
	}

	/**
	 * Supply explicit waiting stock through the sole native station packet list.
	 * @param count Whole fixture customer units already waiting at the interchange.
	 * @param origin Original actual station, retained as packet provenance.
	 */
	void AppendStock(uint16_t count, StationID origin)
	{
		REQUIRE(CargoPacket::CanAllocateItem());
		this->station->goods[this->cargo].GetOrCreateData().cargo.Append(
			CargoPacket::Create(origin, count, Source{IndustryID::Invalid(), SourceType::Industry}), StationID::Invalid());
	}

	/**
	 * Position an actual carrier at its own genuine berth as an explicit arrival boundary.
	 * @param vehicle Real purchased capsule, ordinary train or truck.
	 * @param tile Its compatible actual loading tile; no cross-layer movement is claimed by this fixture boundary.
	 */
	void Arrive(Vehicle &vehicle, TileIndex tile)
	{
		vehicle.tile = tile;
		vehicle.x_pos = TileX(tile) * TILE_SIZE + TILE_SIZE / 2;
		vehicle.y_pos = TileY(tile) * TILE_SIZE + TILE_SIZE / 2;
		vehicle.z_pos = 0;
		vehicle.UpdatePosition();
		vehicle.cur_speed = 0;
		vehicle.vehstatus.Reset({VehState::Stopped, VehState::Hidden});
		vehicle.last_station_visited = GetStationIndex(tile);
	}

	/**
	 * Run an ordinary bounded loading stop and settle its one native payment.
	 * @param vehicle Actual carrier already placed at its compatible real facility.
	 * @param unload Native transfer/delivery/no-unload instruction.
	 * @param load Native load/no-load instruction.
	 * @param owner Actual destination station, defaulting to this joined interchange.
	 */
	void Stop(Vehicle &vehicle, OrderUnloadType unload, OrderLoadType load, Station *owner = nullptr)
	{
		if (owner == nullptr) owner = this->station;
		vehicle.last_station_visited = owner->index;
		vehicle.current_order.MakeLoading(false);
		vehicle.current_order.SetUnloadType(unload);
		vehicle.current_order.SetLoadType(load);
		PrepareUnload(&vehicle);
		// Drive the real economy loading owner until it finishes this visit;
		// retained/full-buffer cargo stays in the carrier instead of being erased.
		for (uint step = 0; step < 1000; ++step) {
			LoadUnloadStation(owner);
			if (vehicle.vehicle_flags.Test(VehicleFlag::LoadingFinished)) break;
		}
		REQUIRE(vehicle.vehicle_flags.Test(VehicleFlag::LoadingFinished));
		delete vehicle.cargo_payment;
		owner->loading_vehicles.remove(&vehicle);
	}
};

#endif /* TESTS_TRANSPORT_TUBE_FREIGHT_FIXTURE_H */
