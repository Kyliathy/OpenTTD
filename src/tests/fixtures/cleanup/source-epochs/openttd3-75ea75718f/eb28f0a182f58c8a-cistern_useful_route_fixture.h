/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file cistern_useful_route_fixture.h Actual ordinary-loaded Cistern modal construction, native liquid carriers and useful receiver.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15DCB/15DCC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Original source owners remain native; prepared floor/completed machinery/Connected shafts are explicit boundaries.
 */
#ifndef TESTS_CISTERN_USEFUL_ROUTE_FIXTURE_H
#define TESTS_CISTERN_USEFUL_ROUTE_FIXTURE_H
#include "cistern_source_world_fixture.h"
#include "tube_cistern_source_fixture.h"
#include "../transport_tube.h"
#include "../utility_power.h"
#include "../rail_map.h"
#include "../engine_func.h"
#include "../settings_cmd.h"

/** Borrowed same-world native route participants; all original upstream industries, trains and unused inputs remain intact. */
struct CisternUsefulWorld {
	CisternSourceWorld source; ///< Original four parts, three source industries and three stations, without a reconstructed source.
	std::array<Train *, 5> parts{}; ///< Actual new feed/tanker, layer-local Cistern and delivery/tanker.
	std::array<Station *, 3> stations{}; ///< Same-layer joined tube origin/destination and useful Surface PCHM berth.
	Industry *receiver = nullptr; ///< Actual command-funded1980 PCHM processor, never a station acceptance flag.
	UtilityProducerID power{UINT32_MAX}; ///< Sole actual paid external electrical producer.
	RailType tube = INVALID_RAILTYPE; ///< Actual ordinary registered tube identity.
	std::array<TileIndex, 2> feed_depots{TileXYL(32, 20, MapLayer::Surface), TileXYL(4, 20, MapLayer::Underground)}; ///< Real conventional feed home/far service ends.
	std::array<TileIndex, 2> tube_depots{TileXYL(4, 24, MapLayer::Underground), TileXYL(35, 24, MapLayer::Underground)}; ///< Actual model11 service ends.
	std::array<TileIndex, 2> delivery_depots{TileXYL(26, 28, MapLayer::Underground), TileXYL(49, 39, MapLayer::Surface)}; ///< Real final conventional home/far service ends.
};

/** @return Exact original source plus paid modal participants from an accepted ordinary ready-world load, without constructing a route or changing any owner. */
inline CisternUsefulWorld LoadCisternUsefulWorld()
{
	CisternUsefulWorld world; world.source = LoadCisternSourceWorld(false); world.tube = GetTubeRailType();
	const auto parts = CisternSourceValues("OTTD_CISTERN_ROUTE_PARTS", 5), references = CisternSourceValues("OTTD_CISTERN_ROUTE_REFERENCES", 5);
	REQUIRE(Vehicle::GetNumItems() == 9); REQUIRE(Industry::GetNumItems() == 4); REQUIRE(Station::GetNumItems() == 6);
	REQUIRE(_settings_game.economy.multiple_industry_per_town); REQUIRE(world.tube != INVALID_RAILTYPE);
	// Every original new part and same-layer modal station must survive normal
	// pool reconstruction. A matching cargo count cannot substitute a new train,
	// receiver, power binding or differently joined station for the accepted one.
	for (size_t index = 0; index < parts.size(); ++index) {
		REQUIRE(parts[index] < UINT32_MAX); world.parts[index] = Train::GetIfValid(VehicleID{static_cast<uint32_t>(parts[index])}); REQUIRE(world.parts[index] != nullptr);
	}
	for (size_t index = 0; index < world.stations.size(); ++index) {
		REQUIRE(references[index] < UINT16_MAX); world.stations[index] = Station::GetIfValid(StationID{static_cast<uint16_t>(references[index])}); REQUIRE(world.stations[index] != nullptr);
	}
	REQUIRE(references[3] < UINT16_MAX); world.receiver = Industry::GetIfValid(IndustryID{static_cast<uint16_t>(references[3])}); REQUIRE(world.receiver != nullptr);
	REQUIRE(references[4] < UINT32_MAX); world.power = UtilityProducerID{static_cast<uint32_t>(references[4])};
	const auto *recipe = GetLayerProcessorRecipe(*world.receiver); REQUIRE(recipe != nullptr); REQUIRE(recipe->recipe.id == MultiLayerRecipeID{"PCHM"});
	REQUIRE(world.parts[0]->engine_type == GetUndergroundEngine(4)); REQUIRE(world.parts[3]->engine_type == GetUndergroundEngine(4));
	REQUIRE(world.parts[1]->engine_type == world.source.parts[1]->engine_type); REQUIRE(world.parts[4]->engine_type == world.source.parts[1]->engine_type);
	REQUIRE(world.parts[2]->engine_type == GetTubeEngine(11)); REQUIRE(world.parts[2]->Next() == nullptr);
	REQUIRE(world.parts[1]->First() == world.parts[0]); REQUIRE(world.parts[4]->First() == world.parts[3]);
	for (uint index : {1, 2, 4}) REQUIRE(world.parts[index]->cargo_type == world.source.cargos[2]);
	const auto *binding = GetTubePowerVehicle(world.parts[2]->index); REQUIRE(binding != nullptr); REQUIRE(binding->source == world.power);
	REQUIRE(GetUtilityPowerProducer(world.power) != nullptr); REQUIRE(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	return world;
}

/** Native command construction in the exact original completed chemical-source world; no catalogue, availability, clock or account reset occurs. */
class CisternUsefulRouteBuilder {
public:
	CisternUsefulWorld world; ///< Borrowed original and actual newly purchased/funded native participants.

	/** @param source Exact ordinary-loaded original source. Build a paid still-stopped modal route without dispatching any original Chemicals or earning a useful delivery. */
	explicit CisternUsefulRouteBuilder(const CisternSourceWorld &source)
	{
		this->world.source = source; this->world.tube = GetTubeRailType(); REQUIRE(this->world.tube != INVALID_RAILTYPE);
		REQUIRE(_settings_game.station.distant_join_stations); REQUIRE(CargoPacket::GetNumItems() == 0);
		REQUIRE(source.industries[1]->produced[0].waiting == 20); REQUIRE(CisternSourceInput(*source.industries[1], source.cargos[0]) == 6);
		const auto *capsule = Engine::Get(GetTubeEngine(11)); REQUIRE(capsule != nullptr);
		fmt::print("CISTERN_ROUTE_AVAIL year={} date={} tube_available={} capsule_available={} intro={} base_intro={} cash={}\n",
			TimerGameCalendar::year.base(), TimerGameCalendar::date.base(), Company::Get(_current_company)->avail_railtypes.Test(this->world.tube),
			capsule->company_avail.Test(_current_company), capsule->intro_date.base(), capsule->info.base_intro.base(), Company::Get(_current_company)->money.base());
		this->BuildReceiver();
		this->PreparePortal(TileXYL(14, 20, MapLayer::Underground), DiagDirection::SW);
		this->Depot(this->world.feed_depots[1], RAILTYPE_RAIL, DiagDirection::SW);
		this->world.stations[0] = this->StationAt(TileXYL(8, 20, MapLayer::Underground), RAILTYPE_RAIL);
		REQUIRE(this->StationAt(TileXYL(8, 24, MapLayer::Underground), this->world.tube, this->world.stations[0]->index) == this->world.stations[0]);
		this->world.stations[1] = this->StationAt(TileXYL(30, 24, MapLayer::Underground), this->world.tube);
		REQUIRE(this->StationAt(TileXYL(30, 28, MapLayer::Underground), RAILTYPE_RAIL, this->world.stations[1]->index) == this->world.stations[1]);
		this->Depot(this->world.tube_depots[0], this->world.tube, DiagDirection::SW); this->Depot(this->world.tube_depots[1], this->world.tube, DiagDirection::NE);
		this->Depot(this->world.delivery_depots[0], RAILTYPE_RAIL, DiagDirection::SW);
		this->Depot(this->world.delivery_depots[1], RAILTYPE_RAIL, DiagDirection::NE);
		this->PreparePortal(TileXYL(35, 28, MapLayer::Underground), DiagDirection::SW);
		// Distinct native tracks and same-layer modal joins own all three legs.
		// Conventional liquid wagons alone traverse the two prepared shafts;
		// the Cistern's actual tube guideway remains entirely Underground.
		for (uint x = 5; x < 14; ++x) this->TrackAt(TileXYL(x, 20, MapLayer::Underground), RAILTYPE_RAIL, Track::X);
		for (uint x = 5; x < 35; ++x) this->TrackAt(TileXYL(x, 24, MapLayer::Underground), this->world.tube, Track::X);
		for (uint x = 27; x < 35; ++x) this->TrackAt(TileXYL(x, 28, MapLayer::Underground), RAILTYPE_RAIL, Track::X);
		for (uint x = 36; x < 39; ++x) this->TrackAt(TileXYL(x, 28, MapLayer::Surface), RAILTYPE_RAIL, Track::X);
		this->TrackAt(TileXYL(39, 28, MapLayer::Surface), RAILTYPE_RAIL, Track::Right);
		for (uint y = 29; y < 39; ++y) this->TrackAt(TileXYL(39, y, MapLayer::Surface), RAILTYPE_RAIL, Track::Y);
		this->TrackAt(TileXYL(39, 39, MapLayer::Surface), RAILTYPE_RAIL, Track::Left);
		this->world.stations[2] = this->StationAt(TileXYL(44, 39, MapLayer::Surface), RAILTYPE_RAIL);
		for (uint x = 40; x < 49; ++x) this->TrackAt(TileXYL(x, 39, MapLayer::Surface), RAILTYPE_RAIL, Track::X);
		REQUIRE(this->world.stations[2]->goods[source.cargos[2]].status.Test(GoodsEntry::State::Acceptance));
		const auto housing = TileXYL(4, 22, MapLayer::Underground); this->PreparePortal(housing, DiagDirection::NW);
		const std::array wire{TileXYL(4, 23, MapLayer::Underground), this->world.tube_depots[0]}; OpenCisternSourceFloor(wire);
		const auto power = ConnectTubeExternalPower(housing, this->world.tube_depots[0], wire);
		INFO(fmt::format("Cistern actual paid external connection error={}", to_underlying(power.error))); REQUIRE(power.error == TubePowerError::None); this->world.power = power.producer;
		const auto tanker = source.parts[1]->engine_type;
		this->world.parts[0] = this->Buy(this->world.feed_depots[0], GetUndergroundEngine(4));
		this->world.parts[1] = this->Buy(this->world.feed_depots[0], tanker, source.cargos[2]);
		this->world.parts[2] = this->Buy(this->world.tube_depots[0], GetTubeEngine(11), source.cargos[2]);
		this->world.parts[3] = this->Buy(this->world.delivery_depots[0], GetUndergroundEngine(4));
		this->world.parts[4] = this->Buy(this->world.delivery_depots[0], tanker, source.cargos[2]);
		for (uint index : {0, 3}) REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, this->world.parts[index + 1]->index, this->world.parts[index]->index, false).Succeeded());
		REQUIRE(BindTubePowerVehicle(this->world.parts[2]->index, this->world.power).has_value());
		this->Orders(*this->world.parts[0], source.stations[1]->index, this->world.stations[0]->index, this->world.feed_depots, true);
		this->Orders(*this->world.parts[2], this->world.stations[0]->index, this->world.stations[1]->index, this->world.tube_depots, true);
		this->Orders(*this->world.parts[3], this->world.stations[1]->index, this->world.stations[2]->index, this->world.delivery_depots, false);
	}

	/** Configure the explicit one-town multi-industry scenario through its synchronized owner, fund actual CHEM machinery, select legal1980 PCHM and prepare only completed empty machinery. */
	void BuildReceiver()
	{
		// The original town already owns the source Chemical Works. PCHM is
		// another works of that same native type, so this route scenario uses
		// the ordinary saved multi-industry option through its real command;
		// no town, cash, facility identity or cargo is injected to bypass funding.
		REQUIRE(Command<Commands::ChangeSetting>::Do(DoCommandFlag::Execute, "economy.multiple_industry_per_town", 1).Succeeded());
		REQUIRE(_settings_game.economy.multiple_industry_per_town);
		// PCHM is a legal alternative of the actual Chemical Works definition,
		// not a second independently mapped native facility or catalogue row.
		const auto type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}); REQUIRE(type != IT_INVALID);
		const auto tile = TileXYL(44, 34, MapLayer::Surface);
		const auto cost = Command<Commands::BuildIndustry>::Do(DoCommandFlag::Execute, tile, type, 0, true, 123);
		INFO(fmt::format("Cistern useful PCHM funding error={} cash={}", cost.GetErrorMessage(), Company::Get(_current_company)->money.base())); REQUIRE(cost.Succeeded());
		this->world.receiver = Industry::GetByTile(tile); REQUIRE(this->world.receiver != nullptr);
		// Only actual native machinery cells are completed. No recipe input,
		// product, work, cargo acceptance flag or rate is supplied by this boundary.
		for (const auto cell : this->world.receiver->location) if (this->world.receiver->TileBelongsToIndustry(cell)) SetIndustryCompleted(cell);
		uint32_t recipe = 0;
		for (uint8_t byte : MultiLayerRecipeID{"PCHM"}) recipe = (recipe << 8) | byte;
		REQUIRE(Command<Commands::SelectLayerIndustryRecipe>::Do(DoCommandFlag::Execute, this->world.receiver->index, recipe).Succeeded());
		const auto *selected = GetLayerProcessorRecipe(*this->world.receiver); REQUIRE(selected != nullptr); REQUIRE(selected->recipe.id == MultiLayerRecipeID{"PCHM"});
	}

	/** @param tile Actual native two-cell berth anchor. @param type Conventional or dedicated tube gauge. @param join Existing same-layer modal station. @return Real paid station and native packet/catchment owner. */
	Station *StationAt(TileIndex tile, RailType type, StationID join = NEW_STATION)
	{
		std::array<TileIndex, 4> floor;
		for (int dx = -1; dx <= 2; ++dx) floor[dx + 1] = TileAddWrap(tile, dx, 0);
		OpenCisternSourceFloor(floor);
		const auto cost = Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, tile, type, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, join, false);
		INFO(fmt::format("Cistern modal berth={} type={} error={}", tile.base(), type, cost.GetErrorMessage())); REQUIRE(cost.Succeeded()); return Station::GetByTile(tile);
	}

	/** @param tile Actual native track cell. @param type Dedicated tube or conventional domain. @param track Exact native straight/curve geometry. Preserve every existing facility and compatible paid piece. */
	void TrackAt(TileIndex tile, RailType type, Track track)
	{
		// Existing platforms/depots retain their actual owners. At the original
		// stopped Salt railway, a real ordinary crossing may add only its missing
		// piece; an already paid matching piece must not be cleared or charged twice.
		if (IsRailStationTile(tile) || IsRailDepotTile(tile)) return;
		if (IsPlainRailTile(tile) && GetRailType(tile) == type && GetTrackBits(tile).Test(track)) return;
		OpenCisternSourceFloor(std::array{tile});
		const auto cost = Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, type, track, false);
		INFO(fmt::format("Cistern modal track={},{} layer={} error={}", TileX(tile), TilePlaneY(tile), to_underlying(GetTileLayer(tile)), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	}

	/** @param tile Real paid native depot. @param type Actual conventional/tube gauge. @param direction Actual outward door. Build only on the disclosed complete floor and exit approach. */
	void Depot(TileIndex tile, RailType type, DiagDirection direction)
	{
		const auto delta = TileIndexDiffCByDiagDir(direction); OpenCisternSourceFloor(std::array{tile, TileAddWrap(tile, delta.x, delta.y)});
		const auto cost = Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, tile, type, direction);
		INFO(fmt::format("Cistern modal depot={} error={}", tile.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	}

	/** @param lower Disclosed prepared Underground conventional housing. @param direction Actual passage direction toward Surface. Retain all original portal records and prepare only this reciprocal Connected boundary. */
	void PreparePortal(TileIndex lower, DiagDirection direction)
	{
		const auto delta = TileIndexDiffCByDiagDir(ReverseDiagDir(direction)); OpenCisternSourceFloor(std::array{lower, TileAddWrap(lower, delta.x, delta.y)});
		const auto upper = MultiLayerMap::ProjectToSurface(lower);
		// The original Oil railway owns a paid piece at the new feed housing.
		// Remove it through the real command before preparing that explicit shaft
		// boundary; no occupied train, infrastructure or old portal is overwritten.
		if (IsPlainRailTile(upper)) for (const auto track : GetTrackBits(upper)) REQUIRE(Command<Commands::RemoveRail>::Do(DoCommandFlag::Execute, upper, track).Succeeded());
		REQUIRE(IsTileType(upper, TileType::Clear));
		MakeRailLayerPortal(lower, _current_company, direction, MapLayer::Surface, RAILTYPE_RAIL);
		MakeRailLayerPortal(upper, _current_company, ReverseDiagDir(direction), MapLayer::Underground, RAILTYPE_RAIL);
		auto records = CaptureLayerPortalWorkRecords(); LayerPortalWorkRecord pair; pair.pair_key = upper; pair.company = _current_company; pair.state = LayerPortalWorkState::Connected;
		records.push_back(pair); std::ranges::sort(records, {}, &LayerPortalWorkRecord::pair_key);
		REQUIRE(ValidateLayerPortalWorkRecords(records, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork(std::move(records)); AcceptLoadedLayerPortalWork();
	}

	/** @param depot Real native paid depot. @param engine Exact original tanker or canonical native model. @param cargo Optional actual Liquid CHEM refit. @return Actual command-purchased part in its native stopped depot pose. */
	Train *Buy(TileIndex depot, EngineID engine, CargoType cargo = INVALID_CARGO)
	{
		const auto [cost, id, capacity, mail, capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, depot, engine, false, cargo, ClientID::Invalid);
		INFO(fmt::format("Cistern native modal purchase engine={} depot={} error={}", engine.base(), depot.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded()); return Train::Get(id);
	}

	/** @param front Actual purchased native front. @param pickup Actual source/transfer packet owner. @param drop Actual transfer or useful PCHM berth. @param depots Real home/far service ends. @param transfer Preserve original CHEM for the next modal owner, otherwise use normal final unload. Install only native normal station/service orders. */
	void Orders(Train &front, StationID pickup, StationID drop, const std::array<TileIndex, 2> &depots, bool transfer)
	{
		Order load; load.MakeGoToStation(pickup); load.SetUnloadType(OrderUnloadType::NoUnload);
		Order unload; unload.MakeGoToStation(drop); unload.SetLoadType(OrderLoadType::NoLoad); unload.SetUnloadType(transfer ? OrderUnloadType::Transfer : OrderUnloadType::UnloadIfPossible);
		Order far; far.MakeGoToDepot(::Depot::GetByTile(depots[1])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		Order home; home.MakeGoToDepot(::Depot::GetByTile(depots[0])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		const std::array list{load, unload, far, home};
		// Explicit native service reversals and actual station stops must earn
		// every arrival; selecting an arrived order index cannot complete a leg.
		for (size_t index = 0; index < list.size(); ++index) REQUIRE(Command<Commands::InsertOrder>::Do(DoCommandFlag::Execute, front.index, static_cast<VehicleOrderID>(index), list[index]).Succeeded());
		ProcessOrders(&front);
	}
};
#endif /* TESTS_CISTERN_USEFUL_ROUTE_FIXTURE_H */
