/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_crate_routes.cpp Actual1975 original Goods through two conventional shafts and a layer-local Crate route.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15CBB/15CC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared rock floor/commissioned shafts and original machinery/town remain boundary inputs; native production and driven transport own every Goods unit.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "tube_goods_source_fixture.h"
#include "crate_useful_world_fixture.h"
#include "test_artifact_paths.h"
#include "../saveload/saveload_func.h"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include "../transport_tube.h"
#include "../layer_portal_map.h"
#include "../layer_portal_cmd.h"
#include "../safeguards.h"

/** Real source-world consumer; the original Sawmill stock and native town are retained without a recreated shipment. */
class CrateUsefulRouteRig : public TubeGoodsSourceRig {
	AutoRestoreBackup<bool> distant_join{_settings_game.station.distant_join_stations, true}; ///< Ordinary same-layer joined modal berths.
	AutoRestoreBackup<bool> crossing{_settings_game.construction.crossing_with_competitor, true}; ///< Legal native town-road level crossings preserve roads and the houses supplying demand.
public:
	RailType tube = INVALID_RAILTYPE; ///< Dedicated native tube rail type, never a conventional shaft gauge.
	UtilityProducerID power{UINT32_MAX}; ///< Sole original paid external source.
	Train *feed = nullptr; ///< Actual Surface/Underground conventional feed front.
	Train *feed_wagon = nullptr; ///< Sole conventional inbound Goods hold.
	Train *capsule = nullptr; ///< Original1975 Crate, confined to Underground.
	Train *delivery = nullptr; ///< Actual Underground/Surface final conventional front.
	Train *delivery_wagon = nullptr; ///< Sole final conventional Goods hold.
	Station *tube_origin = nullptr; ///< Joined Underground receiving/Crate loading owner.
	Station *tube_destination = nullptr; ///< Joined Underground Crate unload/final conventional loading owner.
	Station *town_station = nullptr; ///< Actual native town catchment and final delivery owner.
	TileIndex town_route_berth = INVALID_TILE; ///< Native estimate-selected town platform, preserving all actual houses.
	std::array<TileIndex, 2> feed_depots{TileXYL(32, 20, MapLayer::Surface), TileXYL(4, 20, MapLayer::Underground)}; ///< Native feed home/far reversal depots.
	std::array<TileIndex, 2> tube_depots{TileXYL(4, 24, MapLayer::Underground), TileXYL(35, 24, MapLayer::Underground)}; ///< Real paid Crate route/service ends.
	std::array<TileIndex, 2> delivery_depots{TileXYL(26, 28, MapLayer::Underground), INVALID_TILE}; ///< Actual Underground home; the real town platform is a terminal without a far depot.

	/** Earn the verified source, retire its actual empty train and construct every native route participant at1975. */
	CrateUsefulRouteRig()
	{
		EarnTubeGoodsSource(*this);
		REQUIRE(this->wood_front->IsChainInDepot()); REQUIRE(this->wood_wagon->cargo.StoredCount() == 0);
		// The source's ordinary service cycle restarts automatically. Stop the
		// actual complete empty stored train through its native command before
		// selling it; no hidden tail, pose assignment or deletion earns a return.
		if (!this->wood_front->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, this->wood_front->index, true).Succeeded());
		const auto sold = Command<Commands::SellVehicle>::Do(DoCommandFlag::Execute, this->wood_front->index, true, false, ClientID::Invalid);
		INFO(fmt::format("wood sale error={}", sold.GetErrorMessage())); REQUIRE(sold.Succeeded());
		this->wood_front = nullptr; this->wood_wagon = nullptr;
		REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(CargoPacket::GetNumItems() == 0);
		ResetTubePowerState(); ResetUtilityPowerState(); ResetLayerPortalWorkState();
		this->tube = InitialiseTubeRailType(this->ActivatedContent()); REQUIRE(this->tube != INVALID_RAILTYPE);
		REQUIRE(InitialiseTubeEngineDefinitions(this->ActivatedContent()));
		Company::Get(_current_company)->avail_railtypes.Set(this->tube); Company::Get(_current_company)->clear_limit = 512U << 16;
		Engine::Get(GetTubeEngine(10))->company_avail.Set(_current_company); Engine::Get(GetUndergroundEngine(5))->company_avail.Set(_current_company);
		this->CratePortal(TileXYL(14, 20, MapLayer::Underground), DiagDirection::SW);
		this->CrateDepot(this->feed_depots[1], RAILTYPE_RAIL, DiagDirection::SW);
		this->tube_origin = this->CrateStation(TileXYL(8, 20, MapLayer::Underground), RAILTYPE_RAIL);
		REQUIRE(this->CrateStation(TileXYL(8, 24, MapLayer::Underground), this->tube, this->tube_origin->index) == this->tube_origin);
		this->tube_destination = this->CrateStation(TileXYL(30, 24, MapLayer::Underground), this->tube);
		REQUIRE(this->CrateStation(TileXYL(30, 28, MapLayer::Underground), RAILTYPE_RAIL, this->tube_destination->index) == this->tube_destination);
		this->CrateDepot(this->tube_depots[0], this->tube, DiagDirection::SW); this->CrateDepot(this->tube_depots[1], this->tube, DiagDirection::NE);
		this->CrateDepot(this->delivery_depots[0], RAILTYPE_RAIL, DiagDirection::SW);
		this->CratePortal(TileXYL(35, 28, MapLayer::Underground), DiagDirection::SW);
		// Three physical domains retain distinct tracks and same-layer modal
		// joins. A shaft is crossed by conventional freight alone; no rock-plane
		// coordinate gap is bridged by painted rail or catchment proximity.
		for (uint x = 5; x < 14; ++x) this->CrateTrack(TileXYL(x, 20, MapLayer::Underground), RAILTYPE_RAIL, Track::X);
		for (uint x = 5; x < 35; ++x) this->CrateTrack(TileXYL(x, 24, MapLayer::Underground), this->tube, Track::X);
		for (uint x = 27; x < 35; ++x) this->CrateTrack(TileXYL(x, 28, MapLayer::Underground), RAILTYPE_RAIL, Track::X);
		this->town_route_berth = this->FindTownRouteBerth();
		const uint town_row = TilePlaneY(this->town_route_berth);
		for (uint x = 36; x < 39; ++x) this->CrateTrack(TileXYL(x, 28, MapLayer::Surface), RAILTYPE_RAIL, Track::X);
		this->CrateTrack(TileXYL(39, 28, MapLayer::Surface), RAILTYPE_RAIL, Track::Right);
		for (uint y = 29; y < town_row; ++y) this->CrateTrack(TileXYL(39, y, MapLayer::Surface), RAILTYPE_RAIL, Track::Y);
		this->CrateTrack(TileXYL(39, town_row, MapLayer::Surface), RAILTYPE_RAIL, Track::Left);
		this->town_station = this->CrateStation(this->town_route_berth, RAILTYPE_RAIL);
		// Stop the railway at the real town platform. A through depot would
		// demolish the houses supplying this small native town's Goods demand;
		// ordinary terminal reversal must earn the empty conventional return.
		for (uint x = 40; x < TileX(this->town_route_berth); ++x) this->CrateTrack(TileXYL(x, town_row, MapLayer::Surface), RAILTYPE_RAIL, Track::X);
		UpdateStationAcceptance(this->town_station, false);
		REQUIRE(this->town_station->goods[this->goods].status.Test(GoodsEntry::State::Acceptance));
		REQUIRE(this->town_station->town == this->receiver);
		fmt::print("CRATE_TOWN berth={},{} houses={} preserved=true\n", TileX(this->town_route_berth), town_row, this->receiver->cache.num_houses);
		const auto housing = TileXYL(4, 22, MapLayer::Underground); this->CratePortal(housing, DiagDirection::NW);
		const std::array wire{TileXYL(4, 23, MapLayer::Underground), this->tube_depots[0]}; this->Open(wire);
		const auto connection = ConnectTubeExternalPower(housing, this->tube_depots[0], wire);
		REQUIRE(connection.error == TubePowerError::None); this->power = connection.producer;
		// Actual native Goods metadata determines consist mass, capacity and
		// town effects in both the producer and ordinary application reader.
		// Native tariffs were computed from this world's saved inflation.
		REQUIRE(CargoSpec::Get(this->goods)->weight == 8); REQUIRE(CargoSpec::Get(this->goods)->multiplier == 0x200);
		REQUIRE(CargoSpec::Get(this->goods)->is_freight); REQUIRE(CargoSpec::Get(this->goods)->town_acceptance_effect == TownAcceptanceEffect::Goods);
		REQUIRE(CargoSpec::Get(this->goods)->current_payment > Money{0});
		this->feed = this->CrateBuy(this->feed_depots[0], GetUndergroundEngine(4));
		this->feed_wagon = this->CrateBuy(this->feed_depots[0], GetUndergroundEngine(5), this->goods);
		this->capsule = this->CrateBuy(this->tube_depots[0], GetTubeEngine(10), this->goods);
		this->delivery = this->CrateBuy(this->delivery_depots[0], GetUndergroundEngine(4));
		this->delivery_wagon = this->CrateBuy(this->delivery_depots[0], GetUndergroundEngine(5), this->goods);
		REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, this->feed_wagon->index, this->feed->index, false).Succeeded());
		REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, this->delivery_wagon->index, this->delivery->index, false).Succeeded());
		REQUIRE(BindTubePowerVehicle(this->capsule->index, this->power).has_value());
		this->CrateOrders(*this->feed, this->goods_pickup->index, this->tube_origin->index, this->feed_depots, true);
		this->CrateOrders(*this->capsule, this->tube_origin->index, this->tube_destination->index, this->tube_depots, true);
		this->CrateOrders(*this->delivery, this->tube_destination->index, this->town_station->index, this->delivery_depots, false, true);
	}

	/** @return Actual two-cell clear Goods berth whose complete paid L-shaped approach preserves native demand buildings. */
	TileIndex FindTownRouteBerth() const
	{
		/** @param tile Actual proposed Surface track cell. @param track Native straight/curve geometry. @return Pure native estimate permits construction without demolishing a demand house. */
		const auto may_build = [](TileIndex tile, Track track) {
			if (IsTileType(tile, TileType::House)) return false;
			return Command<Commands::BuildRail>::Do({}, tile, RAILTYPE_RAIL, track, false).Succeeded();
		};
		// Original source acceptance establishes real town demand, but a chosen
		// footprint may have houses in its approaches. Search only this bounded
		// neighbourhood, using actual estimates for every required native cell;
		// no clearing, acceptance flags, vehicle pose or RNG changes select a site.
		for (uint y = 33; y < 58; ++y) for (uint x = 40; x < 58; ++x) {
			const auto tile = TileXYL(x, y, MapLayer::Surface);
			// Both physical platform cells must remain clear and genuine native
			// catchment must accept Goods before any construction is committed.
			if (!IsTileType(tile, TileType::Clear) || !IsTileType(TileAddWrap(tile, 1, 0), TileType::Clear)) continue;
			const auto [acceptance, always] = GetAcceptanceAroundTiles(tile, 2, 1, 4);
			if (acceptance[this->goods] < 8) continue;
			bool permitted = may_build(TileXYL(39, y, MapLayer::Surface), Track::Left);
			for (uint row = 29; row < y; ++row) permitted &= may_build(TileXYL(39, row, MapLayer::Surface), Track::Y);
			for (uint col = 40; col < x; ++col) permitted &= may_build(TileXYL(col, y, MapLayer::Surface), Track::X);
			if (permitted) return tile;
		}
		FAIL("No native Goods berth has a house-preserving ordinary approach"); return INVALID_TILE;
	}

	/** @param tile Native rock-floor command footprint. Open only the explicit prepared terrain boundary, never an industry inventory. */
	void CrateFloor(TileIndex tile)
	{
		if (GetTileLayer(tile) != MapLayer::Surface) this->Open(std::array{tile});
	}

	/** @param tile Actual native route cell. @param type Dedicated tube or conventional gauge. @param track Exact native track geometry. */
	void CrateTrack(TileIndex tile, RailType type, Track track)
	{
		// Native facilities keep their identities; only actual plain gap cells
		// require track commands, including real paid clearing of any obstruction.
		if (IsRailStationTile(tile) || IsRailDepotTile(tile)) return;
		// Real houses supply the native useful destination. Construction may
		// cross existing streets but cannot clear those houses to force a route.
		REQUIRE_FALSE(IsTileType(tile, TileType::House));
		this->CrateFloor(tile);
		const auto cost = Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, type, track, false);
		INFO(fmt::format("crate track {},{} layer={} error={}", TileX(tile), TilePlaneY(tile), to_underlying(GetTileLayer(tile)), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	}

	/** @param tile Real purchase/service cell. @param type Native gauge. @param facing Actual outward depot door. */
	void CrateDepot(TileIndex tile, RailType type, DiagDirection facing)
	{
		this->CrateFloor(tile); const auto delta = TileIndexDiffCByDiagDir(facing); this->CrateFloor(TileAddWrap(tile, delta.x, delta.y));
		const auto cost = Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, tile, type, facing);
		INFO(fmt::format("crate depot {} error={}", tile.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
	}

	/** @param tile Actual two-cell berth. @param type Native modal gauge. @param join Same-layer existing station or NEW_STATION. @return Real native station/packet owner. */
	Station *CrateStation(TileIndex tile, RailType type, StationID join = NEW_STATION)
	{
		// The ordinary station requires both platform cells and its two real
		// end approaches open. Declare that complete prepared floor explicitly;
		// a station command must never excavate the missing approach for free.
		for (int dx = -1; dx <= 2; ++dx) this->CrateFloor(TileAddWrap(tile, dx, 0));
		const auto cost = Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, tile, type, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, join, false);
		INFO(fmt::format("crate berth {} error={}", tile.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded()); return Station::GetByTile(tile);
	}

	/** @param lower Prepared Underground conventional housing. @param direction Actual travel direction toward Surface. Publish only reciprocal native commissioned topology, never a capsule transition. */
	void CratePortal(TileIndex lower, DiagDirection direction)
	{
		const auto door = TileIndexDiffCByDiagDir(ReverseDiagDir(direction)); this->CrateFloor(lower); this->CrateFloor(TileAddWrap(lower, door.x, door.y));
		const auto upper = MultiLayerMap::ProjectToSurface(lower);
		MakeRailLayerPortal(lower, _current_company, direction, MapLayer::Surface, RAILTYPE_RAIL);
		MakeRailLayerPortal(upper, _current_company, ReverseDiagDir(direction), MapLayer::Underground, RAILTYPE_RAIL);
		auto records = CaptureLayerPortalWorkRecords(); LayerPortalWorkRecord pair; pair.pair_key = upper; pair.company = _current_company; pair.state = LayerPortalWorkState::Connected;
		records.push_back(pair); std::ranges::sort(records, {}, &LayerPortalWorkRecord::pair_key);
		REQUIRE(ValidateLayerPortalWorkRecords(records, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork(std::move(records)); AcceptLoadedLayerPortalWork();
	}

	/** @param depot Real native owned depot. @param engine Canonical original/native identity. @param cargo Optional actual Goods refit. @return Command-purchased part in its genuine depot pose. */
	Train *CrateBuy(TileIndex depot, EngineID engine, CargoType cargo = INVALID_CARGO)
	{
		const auto [cost, id, capacity, mail, capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, depot, engine, false, cargo, ClientID::Invalid);
		INFO(fmt::format("crate purchase engine={} error={}", engine.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded()); return Train::Get(id);
	}

	/** @param front Actual native front. @param pickup Original source station. @param drop Native transfer/final town berth. @param depots Real home and optional far service ends. @param transfer Preserve actual Goods for the next modal owner instead of final payment. @param terminal Native town terminal returns directly to home through ordinary end-of-line reversal. */
	void CrateOrders(Train &front, StationID pickup, StationID drop, const std::array<TileIndex, 2> &depots, bool transfer, bool terminal = false)
	{
		Order load; load.MakeGoToStation(pickup); load.SetUnloadType(OrderUnloadType::NoUnload);
		Order unload; unload.MakeGoToStation(drop); unload.SetUnloadType(transfer ? OrderUnloadType::Transfer : OrderUnloadType::UnloadIfPossible); unload.SetLoadType(OrderLoadType::NoLoad);
		std::vector<Order> list{load, unload};
		// Modal through berths retain native service-depot reversals; the town
		// terminal instead preserves demand buildings beyond its actual buffer.
		if (!terminal) {
			Order far; far.MakeGoToDepot(Depot::GetByTile(depots[1])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {}); list.push_back(far);
		}
		Order home; home.MakeGoToDepot(Depot::GetByTile(depots[0])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		list.push_back(home);
		for (size_t index = 0; index < list.size(); ++index) REQUIRE(Command<Commands::InsertOrder>::Do(DoCommandFlag::Execute, front.index, static_cast<VehicleOrderID>(index), list[index]).Succeeded());
		ProcessOrders(&front);
	}

	/** Advance all three original fronts through one complete native opportunity and check their physical/cargo provenance. */
	void RunCrate()
	{
		RunUsefulTubeNativeOpportunity();
		// Stable native pool order owns all movement/loading, including empty
		// returns. Every physical Goods hold retains Sawmill/first-station/purpose
		// provenance even when native loading legitimately splits a packet.
		for (const auto *front : {this->feed, this->capsule, this->delivery}) {
			REQUIRE_FALSE(front->vehstatus.Test(VehState::Crashed)); REQUIRE(IsTrainOnItsTiles(front));
			for (const auto *part = front; part != nullptr; part = part->Next()) for (const auto *packet : *part->cargo.Packets()) {
				CHECK(packet->GetSource() == Source{this->sawmill->index, SourceType::Industry});
				CHECK(packet->GetFirstStation() == this->goods_pickup->index); CHECK(packet->GetCompanySupplyOrder() == 0);
			}
		}
		CHECK(GetTileLayer(this->capsule->tile) == MapLayer::Underground);
	}
};

/** @param rig Actual original producer participants. @return Borrowed native view shared with the ordinary application observer; no owner or stock is copied. */
static CrateUsefulWorld CrateUsefulParticipants(const CrateUsefulRouteRig &rig)
{
	return {{rig.feed, rig.feed_wagon, rig.capsule, rig.delivery, rig.delivery_wagon}, {rig.forest, rig.sawmill}, rig.receiver,
		{rig.goods_pickup, rig.tube_origin, rig.tube_destination, rig.town_station}, rig.power};
}

/** @param rig Original actual native participants. @param paths Invocation-owned native output locations. @param scene Actual loaded feed Surface/Underground, Crate, delivery Underground/Surface or final useful world0..5. @param step Original boundary0 or its untouched next complete native opportunity1. */
static void SaveCrateUsefulWorld(const CrateUsefulRouteRig &rig, const TestArtifactPaths &paths, uint scene, uint step)
{
	const auto world = CrateUsefulParticipants(rig); const auto before = CaptureCrateUsefulWorld(world);
	const std::array interactive{_interactive_random.state[0], _interactive_random.state[1]};
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	const auto path = paths.Output("saves", fmt::format("crate-useful-{}-{}.sav", scene, step));
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	CHECK(CaptureCrateUsefulWorld(world) == before); CHECK((std::array{_interactive_random.state[0], _interactive_random.state[1]} == interactive));
	fmt::print("CRATE_WORLD scene={} step={} tick={} parts={},{},{},{},{} references={},{},{},{},{},{},{},{} snapshot={} path={}\n", scene, step, TimerGameTick::counter,
		rig.feed->index.base(), rig.feed_wagon->index.base(), rig.capsule->index.base(), rig.delivery->index.base(), rig.delivery_wagon->index.base(),
		rig.forest->index.base(), rig.sawmill->index.base(), rig.receiver->index.base(), rig.goods_pickup->index.base(), rig.tube_origin->index.base(),
		rig.tube_destination->index.base(), rig.town_station->index.base(), rig.power.base(), before, path.string());
}

/** @param save_files True additionally writes genuine ordinary cargo-state files and untouched next full-native opportunities; false retains the original route-only acceptance. */
static void RunCrateUsefulRoute(bool save_files)
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); CrateUsefulRouteRig rig;
	std::optional<LayerSaveContent> content; std::optional<TestArtifactPaths> paths;
	AutoRestoreBackup local{_local_company, _current_company};
	if (save_files) { content.emplace(rig.ActivatedContent(), true); paths.emplace(GetTestCaseArtifactPaths("tubes", "crate-files")); }
	std::array<bool, 5> saved{}; std::optional<uint> pending;
	REQUIRE(rig.supplied == 39); REQUIRE(rig.sawmill->produced[0].waiting == rig.supplied);
	rig.goods_pickup->goods[rig.goods].rating = UINT8_MAX; rig.goods_pickup->goods[rig.goods].last_speed = 100;
	// Native original output dispatch creates the one real source packet;
	// subsequent native loading may split it but never changes its provenance.
	for (const auto tile : rig.sawmill->location) if (rig.sawmill->TileBelongsToIndustry(tile)) _tile_type_procs[TileType::Industry]->tile_loop_proc(tile);
	REQUIRE(rig.goods_pickup->goods[rig.goods].TotalCount() == rig.supplied); REQUIRE(rig.sawmill->produced[0].waiting == 0);
	const auto income = CrateNativeAccounts(*Company::Get(_current_company), rig.goods).first;
	for (auto *front : {rig.feed, rig.capsule, rig.delivery}) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, front->index, true).Succeeded());
	bool fed = false, carried = false, crossed = false; std::array<bool, 3> returned{}; uint ticks = 0;
	// Conserve complete original material across all real queues/holds and the
	// useful town receipt at every opportunity. Transfer income cannot satisfy
	// a final delivery, and a loaded final leg must genuinely return to Surface.
	while (ticks++ < 24000) {
		rig.RunCrate();
		// A pending scene's baseline is exactly this next ordinary shared
		// opportunity, including all fronts, loading, quarter history and power.
		// Saving never inserts an extra hidden movement step between boundaries.
		if (pending.has_value()) { SaveCrateUsefulWorld(rig, *paths, *pending, 1); pending.reset(); }
		const auto [earned, delivered] = CrateNativeAccounts(*Company::Get(_current_company), rig.goods);
		uint physical = rig.sawmill->produced[0].waiting;
		for (const auto *station : Station::Iterate()) physical += station->goods[rig.goods].TotalCount();
		for (const auto *part : Vehicle::Iterate()) if (part->cargo_type == rig.goods) physical += part->cargo.StoredCount();
		CHECK(physical + delivered == rig.supplied);
		// Modal transfer accounting cannot pay company freight revenue before
		// any genuine final town intake, even when a vehicle shows feeder credit.
		if (delivered == 0) CHECK(earned == income);
		// These observations describe earned native movement, not a diagnostic
		// scene label that can relocate a front or replace its current order.
		if (rig.feed_wagon->cargo.StoredCount() > 0 && GetTileLayer(rig.feed_wagon->tile) == MapLayer::Underground) fed = true;
		if (rig.capsule->cargo.StoredCount() > 0 && rig.capsule->cur_speed > 0) carried = true;
		if (rig.delivery_wagon->cargo.StoredCount() > 0 && GetTileLayer(rig.delivery_wagon->tile) == MapLayer::Surface) crossed = true;
		if (save_files) {
			// Scene labels only observe physically earned loaded movement. They
			// cannot move a part, assign an order or replace a source packet.
			const std::array eligible{
				rig.feed_wagon->cargo.StoredCount() > 0 && rig.feed->cur_speed > 0 && GetTileLayer(rig.feed_wagon->tile) == MapLayer::Surface,
				rig.feed_wagon->cargo.StoredCount() > 0 && rig.feed->cur_speed > 0 && GetTileLayer(rig.feed_wagon->tile) == MapLayer::Underground,
				rig.capsule->cargo.StoredCount() > 0 && rig.capsule->cur_speed > 0 && IsPlainRailTile(rig.capsule->tile),
				rig.delivery_wagon->cargo.StoredCount() > 0 && rig.delivery->cur_speed > 0 && GetTileLayer(rig.delivery_wagon->tile) == MapLayer::Underground,
				rig.delivery_wagon->cargo.StoredCount() > 0 && rig.delivery->cur_speed > 0 && GetTileLayer(rig.delivery_wagon->tile) == MapLayer::Surface};
			for (uint scene = 0; scene < eligible.size(); ++scene) {
				// Preserve each first genuine scene once; an already pending
				// baseline takes precedence over a later concurrently eligible leg.
				if (saved[scene] || !eligible[scene] || pending.has_value()) continue;
				SaveCrateUsefulWorld(rig, *paths, scene, 0); saved[scene] = true; pending = scene;
			}
		}
		// Only genuinely empty home arrivals after the final receipt earn the
		// three return markers; initial depot presence cannot complete a leg.
		if (delivered == rig.supplied) {
			const std::array fronts{rig.feed, rig.capsule, rig.delivery}; const std::array homes{rig.feed_depots[0], rig.tube_depots[0], rig.delivery_depots[0]};
			for (size_t index = 0; index < fronts.size(); ++index) if (fronts[index]->IsChainInDepot() && fronts[index]->tile == homes[index] && fronts[index]->cargo_payment == nullptr) returned[index] = true;
			if (std::ranges::all_of(returned, std::identity{})) break;
		}
	}
	INFO(DescribeTrain(rig.feed)); INFO(DescribeTrain(rig.capsule)); INFO(DescribeTrain(rig.delivery));
	REQUIRE(ticks < 24000); CHECK(fed); CHECK(carried); CHECK(crossed); CHECK(std::ranges::all_of(returned, std::identity{}));
	const auto final_accounts = CrateNativeAccounts(*Company::Get(_current_company), rig.goods);
	CHECK(final_accounts.second == rig.supplied); REQUIRE(final_accounts.first > income);
	REQUIRE(Company::Get(_current_company)->num_valid_stat_ent > 0);
	const auto *source = GetUtilityPowerProducer(rig.power); REQUIRE(source != nullptr); REQUIRE(source->consumed_lifetime_j > 0);
	// Two further native economy days with empty original routes cannot replay
	// useful Goods, final income or a retired source shipment.
	for (uint tick = 0; tick < 148; ++tick) rig.RunCrate();
	CHECK(CrateNativeAccounts(*Company::Get(_current_company), rig.goods) == final_accounts);
	CHECK(TimerGameCalendar::year == TimerGameCalendar::Year{1975}); CHECK(CargoPacket::GetNumItems() == 0);
	if (save_files) {
		REQUIRE(std::ranges::all_of(saved, std::identity{})); REQUIRE_FALSE(pending.has_value());
		SaveCrateUsefulWorld(rig, *paths, 5, 0); rig.RunCrate(); SaveCrateUsefulWorld(rig, *paths, 5, 1);
	}
	fmt::print("CRATE_USEFUL year=1975 delivered={} source=original_sawmill native_wood=39 fed=true carried=true surfaced=true returns=3 income={} consumed={} ticks={} tick={} quarter_history={}\n",
		rig.supplied, (final_accounts.first - income).base(), source->consumed_lifetime_j, ticks, TimerGameTick::counter, Company::Get(_current_company)->num_valid_stat_ent);
}

/** All39 genuinely produced Goods must physically change modes before one useful town receipt and complete empty returns. */
TEST_CASE("Crate1975 delivers original Sawmill Goods through conventional modal transfers", "[tube-useful-crate-route]") { RunCrateUsefulRoute(false); }

/** Ordinary native files retain every actual cargo phase and uninterrupted next full-native opportunity of the same genuine route. */
TEST_CASE("Crate1975 writes original useful route worlds and native baselines", "[tube-useful-crate-save-file]") { RunCrateUsefulRoute(true); }
