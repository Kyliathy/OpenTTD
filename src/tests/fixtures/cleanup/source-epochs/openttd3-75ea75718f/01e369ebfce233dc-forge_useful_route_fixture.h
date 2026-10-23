/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file forge_useful_route_fixture.h Paid original-world Forge modal facilities and genuinely purchased native participants.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15FDB/15FDC/15FEB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Original finite mines/refinery/raw carriers remain; prepared floor/machinery/Connected shafts do not prove paid TBM bootstrap.
 */
#ifndef TESTS_FORGE_USEFUL_ROUTE_FIXTURE_H
#define TESTS_FORGE_USEFUL_ROUTE_FIXTURE_H
#include "forge_raw_delivery_fixture.h"
#include "../vehicle_cmd.h"
#include "../industry_cmd.h"
#include "../industry_map.h"
#include "../transport_tube_cmd.h"
#include "../utility_power.h"

/** Borrowed original manufactured cargo plus actual command-purchased modal owners; all native pools retain their quantities. */
struct ForgeUsefulWorld {
	ForgeRawRouteWorld raw; ///< Original four raw parts, two finite mines, CFMT and all three paid original stations.
	std::array<Train *, 5> parts{}; ///< New conventional RFMT feed front/wagon, Forge capsule and final conventional front/wagon.
	Industry *receiver = nullptr; ///< Actual command-funded Surface ELEC; absence of RARE prevents an Electronics manufacturing claim.
	std::array<Station *, 4> stations{}; ///< Actual original CFMT pickup/UG origin, then new UG destination/Surface ELEC berth.
	UtilityProducerID power{UINT32_MAX}; ///< Actual paid external service owner, never a copied energy budget.
	RailType tube = INVALID_RAILTYPE; ///< Existing dedicated capsule domain.
	std::array<TileIndex, 2> feed_depots{TileXYL(36, 25, MapLayer::Surface), TileXYL(4, 13, MapLayer::Underground)}; ///< Reused real Coal far/home ends, original raw keeper remains stored separately.
	std::array<TileIndex, 2> tube_depots{TileXYL(4, 24, MapLayer::Underground), TileXYL(35, 24, MapLayer::Underground)}; ///< Actual local capsule home/far service ends.
	std::array<TileIndex, 2> delivery_depots{TileXYL(26, 28, MapLayer::Underground), TileXYL(49, 39, MapLayer::Surface)}; ///< Actual conventional final-delivery service ends.
};

/** @return Exact ordinary-loaded original raw and new modal participants; no construction, content, clock, funding, quantity, order or pose repair runs here. */
inline ForgeUsefulWorld LoadForgeUsefulWorld()
{
	ForgeUsefulWorld world; world.raw = LoadForgeRawRouteWorld(false); world.tube = GetTubeRailType(); REQUIRE(world.tube != INVALID_RAILTYPE);
	REQUIRE(Industry::GetNumItems() == 4); REQUIRE(Station::GetNumItems() == 5); uint trains = 0;
	// Moving native scenes also contain saved visual-effect vehicles. Keep
	// those genuine transient owners while requiring exactly nine train parts;
	// another freight fleet or unrelated vehicle class may not hide among them.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->type == VehicleType::Train) { ++trains; } else { REQUIRE(vehicle->type == VehicleType::Effect); }
	}
	REQUIRE(trains == 9);
	const auto parts = ShieldSourceValues("OTTD_FORGE_ROUTE_PARTS", 5), references = ShieldSourceValues("OTTD_FORGE_ROUTE_REFERENCES", 6), portals = ShieldSourceValues("OTTD_FORGE_ROUTE_PORTALS", 4);
	// Native identity, model, coupling and real station ownership must survive.
	// Matching freight totals cannot substitute another capsule or raw carrier.
	for (size_t index = 0; index < world.parts.size(); ++index) {
		REQUIRE(parts[index] < UINT32_MAX); world.parts[index] = Train::GetIfValid(VehicleID{static_cast<uint32_t>(parts[index])}); REQUIRE(world.parts[index] != nullptr); REQUIRE(world.parts[index]->owner == _current_company);
	}
	REQUIRE(references[0] < UINT16_MAX); world.receiver = Industry::GetIfValid(IndustryID{static_cast<uint16_t>(references[0])}); REQUIRE(world.receiver != nullptr);
	REQUIRE(world.receiver->type == GetLayerProcessorIndustryType(MultiLayerRecipeID{"ELEC"})); REQUIRE(GetTileLayer(world.receiver->location.tile) == MapLayer::Surface);
	for (size_t index = 0; index < world.stations.size(); ++index) {
		REQUIRE(references[index + 1] < UINT16_MAX); world.stations[index] = Station::GetIfValid(StationID{static_cast<uint16_t>(references[index + 1])}); REQUIRE(world.stations[index] != nullptr); REQUIRE(world.stations[index]->owner == _current_company);
	}
	REQUIRE(world.stations[0] == world.raw.stations[2]); REQUIRE(world.stations[1] == world.raw.stations[1]);
	REQUIRE(world.parts[2]->engine_type == GetTubeEngine(13)); REQUIRE(world.parts[2]->Next() == nullptr); REQUIRE(world.parts[2]->cargo_cap == 80); REQUIRE(GetTileLayer(world.parts[2]->tile) == MapLayer::Underground);
	for (const size_t index : {size_t{0}, size_t{3}}) {
		REQUIRE(world.parts[index]->engine_type == GetUndergroundEngine(4)); REQUIRE(world.parts[index + 1]->engine_type == GetUndergroundEngine(5));
		REQUIRE(world.parts[index]->Next() == world.parts[index + 1]); REQUIRE(world.parts[index + 1]->First() == world.parts[index]); REQUIRE(world.parts[index + 1]->Next() == nullptr);
	}
	for (const uint index : {1, 2, 4}) REQUIRE(world.parts[index]->cargo_type == world.raw.source.cargos[2]);
	REQUIRE(references[5] < UINT32_MAX); world.power = UtilityProducerID{static_cast<uint32_t>(references[5])}; REQUIRE(GetUtilityPowerProducer(world.power) != nullptr);
	const auto *binding = GetTubePowerVehicle(world.parts[2]->index); REQUIRE(binding != nullptr); REQUIRE(binding->source == world.power); REQUIRE(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	const auto records = CaptureLayerPortalWorkRecords(); REQUIRE(records.size() == portals.size());
	for (size_t index = 0; index < records.size(); ++index) { REQUIRE(records[index].pair_key.base() == portals[index]); REQUIRE(records[index].state == LayerPortalWorkState::Connected); }
	return world;
}

/** Native command-funded modal preparation in the exact manufactured-RFMT world; original cargo, raw carriers and source/recipe owners are never rebuilt. */
class ForgeUsefulRouteBuilder {
public:
	ForgeUsefulWorld world; ///< Borrowed originals plus actual paid new facilities, receiver and five new native parts.

	/** @param raw Exact accepted original manufactured-cargo world, with both raw chains stopped at home. Pay only for new modal facilities/fleet/orders/service, retaining original mines, CFMT and raw quantities without a clock opportunity. */
	explicit ForgeUsefulRouteBuilder(const ForgeRawRouteWorld &raw)
	{
		this->world.raw = raw; this->world.tube = GetTubeRailType(); REQUIRE(this->world.tube != INVALID_RAILTYPE); REQUIRE(_settings_game.station.distant_join_stations);
		REQUIRE(raw.far_depots[1] == this->world.feed_depots[0]); REQUIRE(raw.source.homes[1] == this->world.feed_depots[1]);
		this->world.stations[0] = raw.stations[2]; this->world.stations[1] = raw.stations[1];
		// The native two-cell Coal berth at y13 and new tube berth at y24
		// require twelve actual station rows. Existing settings/command admission
		// decide whether that real same-layer interchange is possible.
		REQUIRE(_settings_game.station.station_spread >= 12);
		REQUIRE(this->StationAt(TileXYL(8, 24, MapLayer::Underground), this->world.tube, this->world.stations[1]->index) == this->world.stations[1]);
		this->Depot(this->world.tube_depots[0], this->world.tube, DiagDirection::SW); this->Depot(this->world.tube_depots[1], this->world.tube, DiagDirection::NE);
		this->world.stations[2] = this->StationAt(TileXYL(30, 24, MapLayer::Underground), this->world.tube);
		REQUIRE(this->StationAt(TileXYL(30, 28, MapLayer::Underground), RAILTYPE_RAIL, this->world.stations[2]->index) == this->world.stations[2]);
		// Capsules remain local to one excavated Underground tube. Actual new
		// conventional freight owns both cross-layer feed and final delivery.
		for (uint x = 5; x < 35; ++x) this->TrackAt(TileXYL(x, 24, MapLayer::Underground), this->world.tube, Track::X);
		this->Depot(this->world.delivery_depots[0], RAILTYPE_RAIL, DiagDirection::SW); this->Depot(this->world.delivery_depots[1], RAILTYPE_RAIL, DiagDirection::NE);
		this->Portal(TileXYL(35, 28, MapLayer::Underground), DiagDirection::SW);
		for (uint x = 27; x < 35; ++x) this->TrackAt(TileXYL(x, 28, MapLayer::Underground), RAILTYPE_RAIL, Track::X);
		for (uint x = 36; x < 39; ++x) this->TrackAt(TileXYL(x, 28, MapLayer::Surface), RAILTYPE_RAIL, Track::X);
		this->TrackAt(TileXYL(39, 28, MapLayer::Surface), RAILTYPE_RAIL, Track::Right);
		for (uint y = 29; y < 39; ++y) this->TrackAt(TileXYL(39, y, MapLayer::Surface), RAILTYPE_RAIL, Track::Y);
		this->TrackAt(TileXYL(39, 39, MapLayer::Surface), RAILTYPE_RAIL, Track::Left);
		const auto site = TileXYL(44, 34, MapLayer::Surface); const auto type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"ELEC"}); REQUIRE(type != IT_INVALID);
		const auto cost = Command<Commands::BuildIndustry>::Do(DoCommandFlag::Execute, site, type, 0, true, 123);
		INFO(fmt::format("Forge actual ELEC funding error={}", cost.GetErrorMessage())); REQUIRE(cost.Succeeded()); this->world.receiver = Industry::GetByTile(site); REQUIRE(this->world.receiver != nullptr);
		// Initial completion applies only to this real newly funded machinery.
		// It creates no input/output and opens no additional geological floor.
		for (const auto tile : this->world.receiver->location) if (this->world.receiver->TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
		this->world.stations[3] = this->StationAt(TileXYL(44, 39, MapLayer::Surface), RAILTYPE_RAIL);
		for (uint x = 40; x < 49; ++x) this->TrackAt(TileXYL(x, 39, MapLayer::Surface), RAILTYPE_RAIL, Track::X);
		const auto cargo = raw.source.cargos[2]; REQUIRE(this->world.stations[3]->goods[cargo].status.Test(GoodsEntry::State::Acceptance));
		const auto housing = TileXYL(4, 22, MapLayer::Underground); this->Portal(housing, DiagDirection::NW);
		const std::array wire{TileXYL(4, 23, MapLayer::Underground), this->world.tube_depots[0]}; OpenShieldSourceFloor(wire);
		REQUIRE(Command<Commands::ConnectTubePower>::Do(DoCommandFlag::Execute, housing, this->world.tube_depots[0], TubePowerCommandPath{wire}).Succeeded());
		const auto connections = GetTubePowerConnections(); REQUIRE(connections.size() == 1); const auto &connection = connections.front();
		REQUIRE(connection.portal == housing); REQUIRE(connection.interchange == this->world.tube_depots[0]); REQUIRE(connection.owner == _current_company); REQUIRE(connection.active); REQUIRE(connection.invoiced_money > 0);
		this->world.power = connection.producer;
		this->world.parts[0] = this->Buy(this->world.feed_depots[0], GetUndergroundEngine(4)); this->world.parts[1] = this->Buy(this->world.feed_depots[0], GetUndergroundEngine(5), cargo);
		REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, this->world.parts[1]->index, this->world.parts[0]->index, false).Succeeded());
		this->world.parts[2] = this->Buy(this->world.tube_depots[0], GetTubeEngine(13), cargo);
		this->world.parts[3] = this->Buy(this->world.delivery_depots[0], GetUndergroundEngine(4)); this->world.parts[4] = this->Buy(this->world.delivery_depots[0], GetUndergroundEngine(5), cargo);
		REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, this->world.parts[4]->index, this->world.parts[3]->index, false).Succeeded());
		REQUIRE(Command<Commands::SetTubeVehiclePower>::Do(DoCommandFlag::Execute, this->world.parts[2]->index, this->world.power, false).Succeeded());
		this->Orders(*this->world.parts[0], this->world.stations[0]->index, this->world.stations[1]->index, this->world.feed_depots, true);
		this->Orders(*this->world.parts[2], this->world.stations[1]->index, this->world.stations[2]->index, this->world.tube_depots, true);
		this->Orders(*this->world.parts[3], this->world.stations[2]->index, this->world.stations[3]->index, this->world.delivery_depots, false);
	}

private:
	/** @param tile Actual two-cell native berth. @param type Existing conventional/tube domain. @param join Optional actual same-layer owner. @return Real command-funded station, without cargo/service/rating grants. */
	Station *StationAt(TileIndex tile, RailType type, StationID join = NEW_STATION)
	{
		std::array<TileIndex, 4> floor;
		for (int dx = -1; dx <= 2; ++dx) floor[dx + 1] = TileAddWrap(tile, dx, 0);
		OpenShieldSourceFloor(floor); const auto cost = Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, tile, type, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, join, false);
		INFO(fmt::format("Forge modal berth={} layer={} spread={} error={}", tile.base(), to_underlying(GetTileLayer(tile)), _settings_game.station.station_spread, cost.GetErrorMessage())); REQUIRE(cost.Succeeded()); return Station::GetByTile(tile);
	}

	/** @param tile Actual paid gap. @param type Native conventional/tube domain. @param track Exact straight/curve connectivity. Preserve native facilities and already purchased matching track. */
	void TrackAt(TileIndex tile, RailType type, Track track)
	{
		// Native two-cell stations/depots own their cells; matching compatible
		// paid track is retained instead of being overwritten or purchased twice.
		if (IsRailStationTile(tile) || IsRailDepotTile(tile)) return;
		if (IsPlainRailTile(tile) && GetRailType(tile) == type && GetTrackBits(tile).Test(track)) return;
		OpenShieldSourceFloor(std::array{tile}); const auto cost = Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, type, track, false);
		INFO(fmt::format("Forge modal track={},{} layer={} error={}", TileX(tile), TilePlaneY(tile), to_underlying(GetTileLayer(tile)), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	}

	/** @param tile Actual newly paid depot. @param type Existing native gauge. @param direction Actual native door. Prepare only disclosed floor/approach and pay the native command. */
	void Depot(TileIndex tile, RailType type, DiagDirection direction)
	{
		const auto delta = TileIndexDiffCByDiagDir(direction); OpenShieldSourceFloor(std::array{tile, TileAddWrap(tile, delta.x, delta.y)});
		const auto cost = Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, tile, type, direction);
		INFO(fmt::format("Forge modal depot={} error={}", tile.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	}

	/** @param lower Disclosed Underground shaft site. @param direction Real lower passage direction. Publish a reciprocal Connected Surface boundary through the public owner, retaining all original claims/pairs. */
	void Portal(TileIndex lower, DiagDirection direction)
	{
		const auto upper = MultiLayerMap::TileInLayer(lower, MapLayer::Surface); const auto lower_door = TileIndexDiffCByDiagDir(ReverseDiagDir(direction)), upper_door = TileIndexDiffCByDiagDir(direction);
		OpenShieldSourceFloor(std::array{lower, TileAddWrap(lower, lower_door.x, lower_door.y), upper, TileAddWrap(upper, upper_door.x, upper_door.y)});
		REQUIRE(IsTileType(lower, TileType::Clear)); REQUIRE(IsTileType(upper, TileType::Clear));
		MakeRailLayerPortal(lower, _current_company, direction, MapLayer::Surface, RAILTYPE_RAIL); MakeRailLayerPortal(upper, _current_company, ReverseDiagDir(direction), MapLayer::Underground, RAILTYPE_RAIL);
		auto records = CaptureLayerPortalWorkRecords(); LayerPortalWorkRecord pair; pair.pair_key = upper; pair.company = _current_company; pair.state = LayerPortalWorkState::Connected;
		records.push_back(pair); std::ranges::sort(records, {}, &LayerPortalWorkRecord::pair_key);
		REQUIRE(ValidateLayerPortalWorkRecords(records, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None); StageLoadedLayerPortalWork(std::move(records)); AcceptLoadedLayerPortalWork();
	}

	/** @param depot Actual native owned storage. @param engine Exact dated native model. @param cargo Optional RFMT refit. @return Genuine command-purchased native part with its original paid capacity/pose/owner. */
	Train *Buy(TileIndex depot, EngineID engine, CargoType cargo = INVALID_CARGO)
	{
		const auto [cost, id, capacity, mail, capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, depot, engine, false, cargo, ClientID::Invalid);
		INFO(fmt::format("Forge native modal engine={} depot={} error={}", engine.base(), depot.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded()); return Train::Get(id);
	}

	/** @param front Actual new native front. @param pickup Real original/transfer station. @param drop Real transfer/final station. @param depots Actual home/far service ends. @param transfer Preserve freight for the next mode; false permits native ELEC final intake. Install ordinary player orders without an arrived cursor. */
	void Orders(Train &front, StationID pickup, StationID drop, const std::array<TileIndex, 2> &depots, bool transfer)
	{
		Order load; load.MakeGoToStation(pickup); load.SetUnloadType(OrderUnloadType::NoUnload);
		Order unload; unload.MakeGoToStation(drop); unload.SetLoadType(OrderLoadType::NoLoad); unload.SetUnloadType(transfer ? OrderUnloadType::Transfer : OrderUnloadType::UnloadIfPossible);
		Order far; far.MakeGoToDepot(::Depot::GetByTile(depots[1])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		Order home; home.MakeGoToDepot(::Depot::GetByTile(depots[0])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		const std::array list{load, unload, far, home};
		// Every stop belongs to the real native list. Normal movement/loading
		// must later earn arrivals, transfers, payment and complete depot storage.
		for (size_t index = 0; index < list.size(); ++index) REQUIRE(Command<Commands::InsertOrder>::Do(DoCommandFlag::Execute, front.index, static_cast<VehicleOrderID>(index), list[index]).Succeeded());
		ProcessOrders(&front);
	}
};
#endif /* TESTS_FORGE_USEFUL_ROUTE_FIXTURE_H */
