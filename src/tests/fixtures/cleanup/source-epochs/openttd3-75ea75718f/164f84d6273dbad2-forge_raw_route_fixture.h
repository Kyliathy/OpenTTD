/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file forge_raw_route_fixture.h Paid original-world conventional CORE/Coal routes to actual Surface CFMT.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FCB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor and two Connected shaft pairs remain explicit boundaries; no physical raw delivery is claimed here.
 */
#ifndef TESTS_FORGE_RAW_ROUTE_FIXTURE_H
#define TESTS_FORGE_RAW_ROUTE_FIXTURE_H
#include "forge_source_snapshot_fixture.h"
#include "../layer_portal_map.h"
#include "../layer_portal_cmd.h"
#include "../station_cmd.h"
#include "../station_func.h"
#include "../rail_cmd.h"
#include "../rail_map.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../depot_base.h"

/** Borrowed original raw-source participants and actual paid conventional facilities; native pools own all quantities. */
struct ForgeRawRouteWorld {
	ForgeSourceWorld source; ///< Exact retained two finite mines, original four carrier parts and native CFMT.
	std::array<Station *, 3> stations{}; ///< Actual CORE pickup, Coal pickup and shared Surface CFMT station with distinct platforms.
	std::array<TileIndex, 2> far_depots{TileXYL(26, 27, MapLayer::Surface), TileXYL(36, 25, MapLayer::Surface)}; ///< Actual native far service/reversal ends for CORE and Coal.
};

/** Native command-funded raw routes in the exact original source world, without moving an original participant or advancing a clock. */
class ForgeRawRouteBuilder {
public:
	ForgeRawRouteWorld world; ///< Borrowed source plus actual command-created stations/depots and disclosed shaft boundaries.

	/** @param source Exact accepted original raw-stock world, with both40-unit sources and whole stopped empty carriers. Prepare only paid rail/facilities/orders while preserving original stocks/claims/poses. */
	explicit ForgeRawRouteBuilder(const ForgeSourceWorld &source)
	{
		this->world.source = source; REQUIRE(_settings_game.station.distant_join_stations);
		REQUIRE(source.homes[0] == TileXYL(45, 13, MapLayer::Underground)); REQUIRE(source.homes[1] == TileXYL(4, 13, MapLayer::Underground));
		REQUIRE(source.industries[2]->location.tile == TileXYL(32, 21, MapLayer::Surface));
		this->world.stations[0] = this->StationAt(TileXYL(49, 13, MapLayer::Underground));
		this->world.stations[1] = this->StationAt(TileXYL(8, 13, MapLayer::Underground));
		this->world.stations[2] = this->StationAt(TileXYL(30, 25, MapLayer::Surface));
		REQUIRE(this->StationAt(TileXYL(30, 27, MapLayer::Surface), this->world.stations[2]->index) == this->world.stations[2]);
		REQUIRE(this->world.stations[2]->goods[source.cargos[0]].status.Test(GoodsEntry::State::Acceptance));
		REQUIRE(this->world.stations[2]->goods[source.cargos[1]].status.Test(GoodsEntry::State::Acceptance));
		this->Portal(TileXYL(59, 13, MapLayer::Underground)); this->Portal(TileXYL(18, 13, MapLayer::Underground));
		this->Depot(this->world.far_depots[0], DiagDirection::SW); this->Depot(this->world.far_depots[1], DiagDirection::NE);
		// Each original Underground home/pickup reaches its own real shaft.
		// Distinct Surface platforms and far service ends keep the two original
		// input trains from meeting head-on on a shared single platform.
		for (uint x = 46; x < 59; ++x) this->TrackAt(TileXYL(x, 13, MapLayer::Underground), Track::X);
		for (uint x = 5; x < 18; ++x) this->TrackAt(TileXYL(x, 13, MapLayer::Underground), Track::X);
		this->TrackAt(TileXYL(60, 13, MapLayer::Surface), Track::X); this->TrackAt(TileXYL(61, 13, MapLayer::Surface), Track::Right);
		for (uint y = 14; y < 27; ++y) this->TrackAt(TileXYL(61, y, MapLayer::Surface), Track::Y);
		this->TrackAt(TileXYL(61, 27, MapLayer::Surface), Track::Upper);
		for (uint x = 27; x < 61; ++x) this->TrackAt(TileXYL(x, 27, MapLayer::Surface), Track::X);
		this->TrackAt(TileXYL(19, 13, MapLayer::Surface), Track::X); this->TrackAt(TileXYL(20, 13, MapLayer::Surface), Track::Right);
		for (uint y = 14; y < 25; ++y) this->TrackAt(TileXYL(20, y, MapLayer::Surface), Track::Y);
		this->TrackAt(TileXYL(20, 25, MapLayer::Surface), Track::Left);
		for (uint x = 21; x < 36; ++x) this->TrackAt(TileXYL(x, 25, MapLayer::Surface), Track::X);
		this->Orders(*source.parts[0], this->world.stations[0]->index, this->world.stations[2]->index, source.homes[0], this->world.far_depots[0]);
		this->Orders(*source.parts[2], this->world.stations[1]->index, this->world.stations[2]->index, source.homes[1], this->world.far_depots[1]);
	}

private:
	/** @param tile Actual two-cell native conventional berth. @param join Optional same-layer native CFMT station identity. @return Real command-funded station owner; no queue, rating or acceptance is assigned by the fixture. */
	Station *StationAt(TileIndex tile, StationID join = NEW_STATION)
	{
		std::array<TileIndex, 4> floor;
		for (int dx = -1; dx <= 2; ++dx) floor[dx + 1] = TileAddWrap(tile, dx, 0);
		OpenShieldSourceFloor(floor);
		const auto cost = Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, join, false);
		INFO(fmt::format("Forge raw berth={},{} layer={} error={}", TileX(tile), TilePlaneY(tile), to_underlying(GetTileLayer(tile)), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
		return Station::GetByTile(tile);
	}

	/** @param tile Actual paid straight/curve gap. @param track Native directed connectivity. Preserve existing original depots/stations and already paid compatible pieces. */
	void TrackAt(TileIndex tile, Track track)
	{
		// Native facilities own their actual cells. Matching existing paid track
		// must not be overwritten or purchased twice when connecting this route.
		if (IsRailStationTile(tile) || IsRailDepotTile(tile)) return;
		if (IsPlainRailTile(tile) && GetRailType(tile) == RAILTYPE_RAIL && GetTrackBits(tile).Test(track)) return;
		OpenShieldSourceFloor(std::array{tile}); const auto cost = Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, track, false);
		INFO(fmt::format("Forge raw track={},{} layer={} error={}", TileX(tile), TilePlaneY(tile), to_underlying(GetTileLayer(tile)), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	}

	/** @param tile Actual native owned far service end. @param direction Real door/exit direction. Pay for the native depot without creating a private arrived order or moving a part. */
	void Depot(TileIndex tile, DiagDirection direction)
	{
		const auto delta = TileIndexDiffCByDiagDir(direction); OpenShieldSourceFloor(std::array{tile, TileAddWrap(tile, delta.x, delta.y)});
		const auto cost = Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, direction);
		INFO(fmt::format("Forge raw far depot={} error={}", tile.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	}

	/** @param lower Disclosed original-world Underground shaft site. Prepare one reciprocal Connected boundary through the public topology/work owner, preserving every existing geological claim and portal. */
	void Portal(TileIndex lower)
	{
		const auto upper = MultiLayerMap::TileInLayer(lower, MapLayer::Surface);
		OpenShieldSourceFloor(std::array{lower, TileAddWrap(lower, -1, 0), upper, TileAddWrap(upper, 1, 0)});
		REQUIRE(IsTileType(lower, TileType::Clear)); REQUIRE(IsTileType(upper, TileType::Clear));
		MakeRailLayerPortal(lower, _current_company, DiagDirection::SW, MapLayer::Surface, RAILTYPE_RAIL);
		MakeRailLayerPortal(upper, _current_company, DiagDirection::NE, MapLayer::Underground, RAILTYPE_RAIL);
		auto records = CaptureLayerPortalWorkRecords(); LayerPortalWorkRecord pair; pair.pair_key = upper; pair.company = _current_company; pair.state = LayerPortalWorkState::Connected;
		records.push_back(pair); std::ranges::sort(records, {}, &LayerPortalWorkRecord::pair_key);
		REQUIRE(ValidateLayerPortalWorkRecords(records, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork(std::move(records)); AcceptLoadedLayerPortalWork();
	}

	/** @param front Actual original native raw front. @param pickup Its original finite source station. @param drop Actual shared Surface CFMT station. @param home Original Underground depot. @param far Real far service/reversal end. Install ordinary pickup/final-delivery/service orders without skipping an arrival. */
	void Orders(Train &front, StationID pickup, StationID drop, TileIndex home, TileIndex far)
	{
		Order load; load.MakeGoToStation(pickup); load.SetUnloadType(OrderUnloadType::NoUnload);
		Order unload; unload.MakeGoToStation(drop); unload.SetLoadType(OrderLoadType::NoLoad); unload.SetUnloadType(OrderUnloadType::UnloadIfPossible);
		Order service; service.MakeGoToDepot(::Depot::GetByTile(far)->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		Order back; back.MakeGoToDepot(::Depot::GetByTile(home)->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		const std::array list{load, unload, service, back};
		// Every actual stop belongs to the native list and must later be earned
		// by normal movement/loading. Preparation cannot set an arrived cursor.
		for (size_t index = 0; index < list.size(); ++index) REQUIRE(Command<Commands::InsertOrder>::Do(DoCommandFlag::Execute, front.index, static_cast<VehicleOrderID>(index), list[index]).Succeeded());
		ProcessOrders(&front);
	}
};

#endif /* TESTS_FORGE_RAW_ROUTE_FIXTURE_H */
