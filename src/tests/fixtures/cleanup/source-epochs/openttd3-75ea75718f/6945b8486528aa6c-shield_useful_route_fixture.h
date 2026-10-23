/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file shield_useful_route_fixture.h Paid original-world Shield preparation with conventional Deep/Underground/Surface legs.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15ECB/15ECC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Original source/receiver/carrier remain. Prepared floor and Connected shaft boundaries do not establish actual TBM bootstrap or useful freight.
 */
#ifndef TESTS_SHIELD_USEFUL_ROUTE_FIXTURE_H
#define TESTS_SHIELD_USEFUL_ROUTE_FIXTURE_H
#include "shield_source_world_fixture.h"
#include "../layer_portal_map.h"
#include "../layer_portal_cmd.h"
#include "../station_cmd.h"
#include "../station_func.h"
#include "../rail_cmd.h"
#include "../rail_map.h"
#include "../vehicle_cmd.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../depot_base.h"
#include "../transport_tube_cmd.h"
#include "../utility_power.h"

/** Borrowed original source plus actual purchased modal participants; native pools remain the sole quantity owners. */
struct ShieldUsefulWorld {
	ShieldSourceWorld source; ///< Exact retained original mine/ELEC receiver/Deep carrier and reserve binding.
	std::array<Train *, 3> parts{}; ///< New actual Shield, delivery front and coupled sealed wagon.
	std::array<Station *, 4> stations{}; ///< Deep source, UG joined tube origin/destination and actual Surface ELEC berth.
	UtilityProducerID power{UINT32_MAX}; ///< Original actually paid external service, never an invented electrical balance.
	RailType tube = INVALID_RAILTYPE; ///< Existing dedicated tube domain.
	std::array<TileIndex, 2> feed_depots{INVALID_TILE, TileXYL(4, 29, MapLayer::Underground)}; ///< Original Deep home and real UG far service/reversal end.
	std::array<TileIndex, 2> tube_depots{TileXYL(4, 24, MapLayer::Underground), TileXYL(35, 24, MapLayer::Underground)}; ///< Actual local capsule service ends.
	std::array<TileIndex, 2> delivery_depots{TileXYL(26, 28, MapLayer::Underground), TileXYL(49, 39, MapLayer::Surface)}; ///< Actual final conventional home/far ends.
};

/** @return Borrowed exact original ordinary-loaded modal participants; no builder, clock, cargo registry or owner repair runs here. */
inline ShieldUsefulWorld LoadShieldUsefulWorld()
{
	ShieldUsefulWorld world; world.source = LoadShieldSourceWorld(false); world.feed_depots[0] = world.source.home; world.tube = GetTubeRailType();
	const auto parts = ShieldSourceValues("OTTD_SHIELD_ROUTE_PARTS", 3), references = ShieldSourceValues("OTTD_SHIELD_ROUTE_REFERENCES", 5);
	REQUIRE(Vehicle::GetNumItems() == 5); REQUIRE(Industry::GetNumItems() == 2); REQUIRE(Station::GetNumItems() == 4); REQUIRE(world.tube != INVALID_RAILTYPE);
	// Exact original new parts and joined native stations must survive loading.
	// A matching cargo count cannot substitute another capsule, trailer or berth.
	for (size_t index = 0; index < world.parts.size(); ++index) {
		REQUIRE(parts[index] < UINT32_MAX); world.parts[index] = Train::GetIfValid(VehicleID{static_cast<uint32_t>(parts[index])}); REQUIRE(world.parts[index] != nullptr);
	}
	for (size_t index = 0; index < world.stations.size(); ++index) {
		REQUIRE(references[index] < UINT16_MAX); world.stations[index] = Station::GetIfValid(StationID{static_cast<uint16_t>(references[index])}); REQUIRE(world.stations[index] != nullptr);
	}
	REQUIRE(references[4] < UINT32_MAX); world.power = UtilityProducerID{static_cast<uint32_t>(references[4])};
	REQUIRE(world.parts[0]->engine_type == GetTubeEngine(12)); REQUIRE(world.parts[0]->Next() == nullptr);
	REQUIRE(world.parts[1]->engine_type == GetUndergroundEngine(4)); REQUIRE(world.parts[2]->engine_type == GetUndergroundEngine(6)); REQUIRE(world.parts[2]->First() == world.parts[1]);
	for (uint index : {0, 2}) REQUIRE(world.parts[index]->cargo_type == world.source.cargo);
	const auto *binding = GetTubePowerVehicle(world.parts[0]->index); REQUIRE(binding != nullptr); REQUIRE(binding->source == world.power);
	REQUIRE(GetUtilityPowerProducer(world.power) != nullptr); REQUIRE(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	REQUIRE(GetTileLayer(world.parts[0]->tile) == MapLayer::Underground); return world;
}

/** Command-funded native preparation in the exact accepted source world; every original physical and historical owner is retained. */
class ShieldUsefulRouteBuilder {
public:
	ShieldUsefulWorld world; ///< Borrowed originals and actual command-purchased new participants.

	/** @param source Exact accepted original native source, still whole stopped/empty with undispatched stock. Prepare paid modal topology/orders/service without advancing a source opportunity. */
	explicit ShieldUsefulRouteBuilder(const ShieldSourceWorld &source)
	{
		this->world.source = source; this->world.feed_depots[0] = source.home; this->world.tube = GetTubeRailType();
		REQUIRE(this->world.tube != INVALID_RAILTYPE); REQUIRE(_settings_game.station.distant_join_stations);
		REQUIRE(CargoPacket::GetNumItems() == 0); REQUIRE(source.industries[0]->produced[0].waiting == 20);
		const uint x = TileX(source.industries[0]->location.tile), row = TilePlaneY(source.home), column = x + 8;
		REQUIRE(x == 9); REQUIRE(row == 37); REQUIRE(column == 17); REQUIRE(source.home == TileXYL(x - 4, row, MapLayer::DeepUnderground));
		this->world.stations[0] = this->StationAt(TileXYL(x, row, MapLayer::DeepUnderground), RAILTYPE_RAIL);
		this->Portal(TileXYL(x + 6, row, MapLayer::DeepUnderground), MapLayer::Underground, DiagDirection::SW);
		this->Depot(this->world.feed_depots[1], RAILTYPE_RAIL, DiagDirection::SW);
		this->world.stations[1] = this->StationAt(TileXYL(8, 29, MapLayer::Underground), RAILTYPE_RAIL);
		REQUIRE(this->StationAt(TileXYL(8, 24, MapLayer::Underground), this->world.tube, this->world.stations[1]->index) == this->world.stations[1]);
		// The actual retained sealed carrier alone crosses Deep/Underground.
		// Both native two-cell platforms retain their owners; paid connecting
		// gaps and real two-bend geometry cannot be replaced by a remote arrival.
		for (uint rail_x = x - 3; rail_x < x + 6; ++rail_x) this->TrackAt(TileXYL(rail_x, row, MapLayer::DeepUnderground), RAILTYPE_RAIL, Track::X);
		this->TrackAt(TileXYL(x + 7, row, MapLayer::Underground), RAILTYPE_RAIL, Track::X);
		this->TrackAt(TileXYL(column, row, MapLayer::Underground), RAILTYPE_RAIL, Track::Upper);
		for (uint y = 30; y < row; ++y) this->TrackAt(TileXYL(column, y, MapLayer::Underground), RAILTYPE_RAIL, Track::Y);
		this->TrackAt(TileXYL(column, 29, MapLayer::Underground), RAILTYPE_RAIL, Track::Right);
		for (uint rail_x = 5; rail_x < column; ++rail_x) this->TrackAt(TileXYL(rail_x, 29, MapLayer::Underground), RAILTYPE_RAIL, Track::X);
		this->Depot(this->world.tube_depots[0], this->world.tube, DiagDirection::SW); this->Depot(this->world.tube_depots[1], this->world.tube, DiagDirection::NE);
		this->world.stations[2] = this->StationAt(TileXYL(30, 24, MapLayer::Underground), this->world.tube);
		REQUIRE(this->StationAt(TileXYL(30, 28, MapLayer::Underground), RAILTYPE_RAIL, this->world.stations[2]->index) == this->world.stations[2]);
		// The capsule remains on one Underground tube. Only actual conventional
		// freight uses the separate Underground/Surface shaft and receiver berth.
		for (uint rail_x = 5; rail_x < 35; ++rail_x) this->TrackAt(TileXYL(rail_x, 24, MapLayer::Underground), this->world.tube, Track::X);
		this->Depot(this->world.delivery_depots[0], RAILTYPE_RAIL, DiagDirection::SW);
		this->Depot(this->world.delivery_depots[1], RAILTYPE_RAIL, DiagDirection::NE);
		this->Portal(TileXYL(35, 28, MapLayer::Underground), MapLayer::Surface, DiagDirection::SW);
		for (uint rail_x = 27; rail_x < 35; ++rail_x) this->TrackAt(TileXYL(rail_x, 28, MapLayer::Underground), RAILTYPE_RAIL, Track::X);
		for (uint rail_x = 36; rail_x < 39; ++rail_x) this->TrackAt(TileXYL(rail_x, 28, MapLayer::Surface), RAILTYPE_RAIL, Track::X);
		this->TrackAt(TileXYL(39, 28, MapLayer::Surface), RAILTYPE_RAIL, Track::Right);
		for (uint y = 29; y < 39; ++y) this->TrackAt(TileXYL(39, y, MapLayer::Surface), RAILTYPE_RAIL, Track::Y);
		this->TrackAt(TileXYL(39, 39, MapLayer::Surface), RAILTYPE_RAIL, Track::Left);
		this->world.stations[3] = this->StationAt(TileXYL(44, 39, MapLayer::Surface), RAILTYPE_RAIL);
		for (uint rail_x = 40; rail_x < 49; ++rail_x) this->TrackAt(TileXYL(rail_x, 39, MapLayer::Surface), RAILTYPE_RAIL, Track::X);
		REQUIRE(this->world.stations[3]->goods[source.cargo].status.Test(GoodsEntry::State::Acceptance));
		// A third disclosed housing supplies the actual commissioned portal-side
		// external service. The synchronized command alone settles its native fee;
		// neither a copied budget nor the remote freight shaft provides free power.
		const auto housing = TileXYL(4, 22, MapLayer::Underground); this->Portal(housing, MapLayer::Surface, DiagDirection::NW);
		const std::array wire{TileXYL(4, 23, MapLayer::Underground), this->world.tube_depots[0]}; OpenShieldSourceFloor(wire);
		REQUIRE(Command<Commands::ConnectTubePower>::Do(DoCommandFlag::Execute, housing, this->world.tube_depots[0], TubePowerCommandPath{wire}).Succeeded());
		const auto connections = GetTubePowerConnections(); REQUIRE(connections.size() == 1);
		const auto &connection = connections.front(); REQUIRE(connection.portal == housing); REQUIRE(connection.interchange == this->world.tube_depots[0]);
		REQUIRE(connection.owner == _current_company); REQUIRE(connection.active); REQUIRE(connection.invoiced_money > 0); this->world.power = connection.producer;
		this->world.parts[0] = this->Buy(this->world.tube_depots[0], GetTubeEngine(12), source.cargo);
		this->world.parts[1] = this->Buy(this->world.delivery_depots[0], GetUndergroundEngine(4));
		this->world.parts[2] = this->Buy(this->world.delivery_depots[0], GetUndergroundEngine(6), source.cargo);
		REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, this->world.parts[2]->index, this->world.parts[1]->index, false).Succeeded());
		REQUIRE(Command<Commands::SetTubeVehiclePower>::Do(DoCommandFlag::Execute, this->world.parts[0]->index, this->world.power, false).Succeeded());
		this->Orders(*source.parts[0], this->world.stations[0]->index, this->world.stations[1]->index, this->world.feed_depots, true);
		this->Orders(*this->world.parts[0], this->world.stations[1]->index, this->world.stations[2]->index, this->world.tube_depots, true);
		this->Orders(*this->world.parts[1], this->world.stations[2]->index, this->world.stations[3]->index, this->world.delivery_depots, false);
	}

	/** @param tile Actual native two-cell station anchor. @param type Existing conventional/tube domain. @param join Optional actual same-layer station. @return Command-funded native owner; no cargo or service eligibility is granted. */
	Station *StationAt(TileIndex tile, RailType type, StationID join = NEW_STATION)
	{
		std::array<TileIndex, 4> floor;
		for (int dx = -1; dx <= 2; ++dx) floor[dx + 1] = TileAddWrap(tile, dx, 0);
		OpenShieldSourceFloor(floor);
		const auto cost = Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, tile, type, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, join, false);
		INFO(fmt::format("Shield modal berth={} layer={} error={}", tile.base(), to_underlying(GetTileLayer(tile)), cost.GetErrorMessage())); REQUIRE(cost.Succeeded()); return Station::GetByTile(tile);
	}

	/** @param tile Actual paid native gap. @param type Original conventional/tube gauge. @param track Exact straight/curve. Retain every existing facility and already paid compatible piece. */
	void TrackAt(TileIndex tile, RailType type, Track track)
	{
		// Two-cell stations/depots retain native ownership. Matching paid pieces
		// must not be overwritten or purchased twice while connecting the route.
		if (IsRailStationTile(tile) || IsRailDepotTile(tile)) return;
		if (IsPlainRailTile(tile) && GetRailType(tile) == type && GetTrackBits(tile).Test(track)) return;
		OpenShieldSourceFloor(std::array{tile}); const auto cost = Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, type, track, false);
		INFO(fmt::format("Shield modal track={},{} layer={} error={}", TileX(tile), TilePlaneY(tile), to_underlying(GetTileLayer(tile)), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	}

	/** @param tile Actual paid depot. @param type Existing gauge. @param direction Native door/exit direction. Prepare only its disclosed floor and real approach. */
	void Depot(TileIndex tile, RailType type, DiagDirection direction)
	{
		const auto delta = TileIndexDiffCByDiagDir(direction); OpenShieldSourceFloor(std::array{tile, TileAddWrap(tile, delta.x, delta.y)});
		const auto cost = Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, tile, type, direction);
		INFO(fmt::format("Shield modal depot={} error={}", tile.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	}

	/** @param lower Disclosed lower rock-layer housing. @param upper_layer Actual adjacent native upper layer. @param direction Real passage direction from the lower end. Prepare a complete reciprocal Connected boundary while retaining every original claim/portal. */
	void Portal(TileIndex lower, MapLayer upper_layer, DiagDirection direction)
	{
		const auto upper = MultiLayerMap::TileInLayer(lower, upper_layer); const auto lower_layer = GetTileLayer(lower);
		const auto lower_door = TileIndexDiffCByDiagDir(ReverseDiagDir(direction)), upper_door = TileIndexDiffCByDiagDir(direction);
		OpenShieldSourceFloor(std::array{lower, TileAddWrap(lower, lower_door.x, lower_door.y), upper, TileAddWrap(upper, upper_door.x, upper_door.y)});
		REQUIRE(IsTileType(lower, TileType::Clear)); REQUIRE(IsTileType(upper, TileType::Clear));
		MakeRailLayerPortal(lower, _current_company, direction, upper_layer, RAILTYPE_RAIL);
		MakeRailLayerPortal(upper, _current_company, ReverseDiagDir(direction), lower_layer, RAILTYPE_RAIL);
		auto records = CaptureLayerPortalWorkRecords(); LayerPortalWorkRecord pair; pair.pair_key = upper; pair.company = _current_company; pair.state = LayerPortalWorkState::Connected;
		records.push_back(pair); std::ranges::sort(records, {}, &LayerPortalWorkRecord::pair_key);
		REQUIRE(ValidateLayerPortalWorkRecords(records, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork(std::move(records)); AcceptLoadedLayerPortalWork();
	}

	/** @param depot Actual native owned depot. @param engine Exact dated native model. @param cargo Optional original RARE refit. @return Real command-purchased part, retaining native position/capacity/owner. */
	Train *Buy(TileIndex depot, EngineID engine, CargoType cargo = INVALID_CARGO)
	{
		const auto [cost, id, capacity, mail, capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, depot, engine, false, cargo, ClientID::Invalid);
		INFO(fmt::format("Shield native modal engine={} depot={} error={}", engine.base(), depot.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded()); return Train::Get(id);
	}

	/** @param front Actual native front. @param pickup Original source/transfer station. @param drop Actual transfer/final station. @param depots Genuine home/far service ends. @param transfer Retain cargo for the next actual modal owner; false permits final native ELEC intake. Install only ordinary player orders, never an arrived cursor. */
	void Orders(Train &front, StationID pickup, StationID drop, const std::array<TileIndex, 2> &depots, bool transfer)
	{
		Order load; load.MakeGoToStation(pickup); load.SetUnloadType(OrderUnloadType::NoUnload);
		Order unload; unload.MakeGoToStation(drop); unload.SetLoadType(OrderLoadType::NoLoad); unload.SetUnloadType(transfer ? OrderUnloadType::Transfer : OrderUnloadType::UnloadIfPossible);
		Order far; far.MakeGoToDepot(::Depot::GetByTile(depots[1])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		Order home; home.MakeGoToDepot(::Depot::GetByTile(depots[0])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		const std::array list{load, unload, far, home};
		// Every real pickup/unload/service stop is retained by the native list.
		// Normal ticks must earn every arrival/departure before useful acceptance.
		for (size_t index = 0; index < list.size(); ++index) REQUIRE(Command<Commands::InsertOrder>::Do(DoCommandFlag::Execute, front.index, static_cast<VehicleOrderID>(index), list[index]).Succeeded());
		ProcessOrders(&front);
	}
};
#endif /* TESTS_SHIELD_USEFUL_ROUTE_FIXTURE_H */
