/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file construction_supply_fixture.h Native ordered Surface-to-Deep construction freight on disclosed prepared floor.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_CONSTRUCTION_SUPPLY_FIXTURE_H
#define TESTS_CONSTRUCTION_SUPPLY_FIXTURE_H

#include "excavation_fixture.h"
#include "movement_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../geothermal_collector.h"
#include "../geothermal_riser.h"
#include "../industry_cmd.h"
#include "../industrytype.h"
#include "../industry.h"
#include "../industry_map.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../surface_supply.h"
#include "../surface_supply_cmd.h"
#include "../underground_vehicle.h"
#include "../tbm.h"
#include "../engine_func.h"
#include "../vehicle_cmd.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../station_cmd.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../newgrf_station.h"
#include "../newgrf_commons.h"
#include "../rail_cmd.h"
#include "../rail_map.h"
#include "../depot_base.h"
#include "../town.h"
#include "../news_func.h"
#include "../economy_func.h"
#include "../economy_type.h"
#include "../timer/timer_game_tick.h"
#include "../core/backup_type.hpp"
#include "../table/strings.h"
#include "../safeguards.h"

/** Dispatch ordinary native construction participants after actual delivered stock exists. */
void OnTick_Industry();

/**
 * Advance native movement, orders and station loading without fixture arrivals or inventory writes.
 * @param head Actual command-purchased conventional locomotive.
 * @param origin Original Surface packet owner.
 * @param destination Actual Deep construction catchment.
 * @param station Required observed loading arrival.
 * @param finish Require arrival, empty train and completed departure/payment settlement.
 * @return Actual elapsed synchronized ticks; bounded refusal fails the focused test.
 */
inline uint DriveConstructionSupply(Train &head, Station &origin, Station &destination, StationID station, bool finish)
{
	uint ticks = 0;
	bool arrived = false;
	// The test selects ordinary movement/loading participants explicitly. It
	// freezes calendar/economy-day changes and advances construction separately,
	// so a delivered-but-unbuilt checkpoint retains its exact material boundary.
	for (; ticks < 12000; ++ticks) {
		++TimerGameTick::counter;
		REQUIRE(head.Tick());
		LoadUnloadStation(&origin);
		LoadUnloadStation(&destination);
		REQUIRE_FALSE(head.vehstatus.Test(VehState::Crashed));
		REQUIRE(IsTrainOnItsTiles(&head));
		// An arrival must come from native movement. Empty departure also proves
		// that the native CargoPayment has settled before money is inspected.
		if (head.current_order.IsType(OT_LOADING) && head.last_station_visited == station) arrived = true;
		uint cargo = 0;
		for (const Train *part = &head; part != nullptr; part = part->Next()) cargo += part->cargo.TotalCount();
		// A caller can retain an actual in-flight load or wait for a complete
		// empty visit. Neither policy can synthesize arrival or packet delivery.
		if (arrived && (!finish || (!head.current_order.IsType(OT_LOADING) && head.cargo_payment == nullptr && cargo == 0))) break;
	}
	INFO(DescribeTrain(&head));
	REQUIRE(ticks < 12000);
	return ticks + 1;
}

/** Actual funded collector/riser, command-bought native train and two command-built Connected rail transitions; prepared geometry is fixture input. */
class ConstructionSupplyRig : public ExcavationFixture {
	decltype(_industry_mngr.mappings) industries = _industry_mngr.mappings; ///< Caller industry identities.
	decltype(_industile_mngr.mappings) tiles = _industile_mngr.mappings; ///< Caller tile identities.
	decltype(_engine_mngr.mappings) engines = _engine_mngr.mappings; ///< Caller full engine identities.
	AutoRestoreBackup<uint64_t> tick{TimerGameTick::counter}; ///< Restore selected participant clock.
	AutoRestoreBackup<UnitID> limit{_settings_game.vehicle.max_trains, UnitID{500}}; ///< Ordinary purchase allowance.
	AutoRestoreBackup<uint8_t> length{_settings_game.vehicle.max_train_length, uint8_t{7}}; ///< Real consist limit.
	AutoRestoreBackup<Money> engine_price{_price[Price::BuildVehicleTrain], Money{100000}}; ///< Nonzero native purchase price.
	AutoRestoreBackup<Money> wagon_price{_price[Price::BuildVehicleWagon], Money{10000}}; ///< Nonzero native wagon price.
	AutoRestoreBackup<uint64_t> inflation{_economy.inflation_prices, uint64_t{1} << 16}; ///< Frozen seed prices for exact money receipts.
	AutoRestoreBackup<bool> infinite{_settings_game.difficulty.infinite_money, false}; ///< Real money admission.
	PathfinderDefaults pathfinder; ///< Native pathfinder defaults throughout actual driving.
public:
	MultiLayerGeothermalConstructionKind kind; ///< Actual recipient's canonical construction definition.
	Industry *recipient = nullptr; ///< Sole native accepted-stock and construction owner.
	Station *origin = nullptr; ///< Actual owned Surface pickup.
	Station *destination = nullptr; ///< Actual Deep recipient station; catchment never crosses layers.
	Train *head = nullptr; ///< Sole native order/traction owner.
	TileIndex depot = INVALID_TILE; ///< Genuine Surface purchase depot, also used by ordinary reloads.
	TileIndex rock_portal = INVALID_TILE; ///< Actual Deep housing bound to the riser scenario.
	Money initial_cash = 0; ///< Money before actual capital and vehicle command settlement.
	Money setup_charge = 0; ///< Actual funded machinery and purchased native train costs.
	Money seed_charge = 0; ///< Exact source invoice, never a final-delivery tariff.

	/** @param collector Select the natural-vent collector instead of the original-portal riser; both use the same native Surface-to-Deep freight path. */
	explicit ConstructionSupplyRig(bool collector) : ExcavationFixture(true), kind(collector ? MultiLayerGeothermalConstructionKind::Collector : MultiLayerGeothermalConstructionKind::Riser)
	{
		REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(Industry::GetNumItems() == 0);
		ResetIndustries(); ResetSurfaceSupplyOrders(); ResetGeothermalCollectors(); ResetGeothermalSiteClaims();
		ResetGeothermalRisers(); ResetUtilityPowerState(); ResetLayerPortalWorkState(); ResetRailTypes();
		StationClass::Reset(); RebuildStationKdtree();
		REQUIRE(InitialiseGeothermalCollectorDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseGeothermalRiserDefinitions(this->ActivatedContent()));
		_engine_mngr.ResetToDefaultMapping(); SetupEngines();
		REQUIRE(InitialiseTBMEngineDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseUndergroundEngineDefinitions(this->ActivatedContent()));
		for (const uint16_t model : {4, 5}) Engine::Get(GetUndergroundEngine(model))->company_avail.Set(_current_company);
		REQUIRE(Town::CanAllocateItem());
		auto *town = Town::Create(TileXYL(10, 10, MapLayer::Surface));
		town->townnametype = SPECSTR_TOWNNAME_START; RebuildTownKdtree();
		// Geological fixture restoration replaces its snapshot. Retain the
		// selected public source identity/coordinates across that replacement.
		const auto vent = GetGeologyWorldState().vents.front();
		const uint x = collector ? vent.x - 1 : 20;
		const uint stop_y = collector ? vent.y - 1 : 26;
		std::vector<TileIndex> floor;
		// All track, housing and building floor is disclosed prepared baseline.
		// This freight proof cannot satisfy the later paid-TBM first-plant test.
		for (uint y = 11; y <= 23; ++y) floor.push_back(TileXYL(x, y, MapLayer::Underground));
		for (uint y = 21; y <= stop_y + 5; ++y) floor.push_back(TileXYL(x, y, MapLayer::DeepUnderground));
		const auto anchor = collector ? TileXYL(vent.x, vent.y, vent.layer) : TileXYL(x + 1, 22, MapLayer::DeepUnderground);
		const uint side = collector ? 3 : 2;
		// Native machinery checks its complete footprint and outward access.
		// Declare the one-cell approach rim as baseline too; funding cannot dig it.
		for (int dy = -1; dy <= static_cast<int>(side); ++dy) {
			for (int dx = -1; dx <= static_cast<int>(side); ++dx) floor.push_back(TileAddWrap(anchor, dx, dy));
		}
		this->Open(floor);
		this->Portal(TileXYL(x, 12, MapLayer::Surface), MapLayer::Underground);
		this->Portal(TileXYL(x, 22, MapLayer::Underground), MapLayer::DeepUnderground);
		this->rock_portal = TileXYL(x, 22, MapLayer::DeepUnderground);
		this->initial_cash = Company::Get(_current_company)->money;
		CommandCost funded;
		if (collector) {
			GeothermalSiteRequest request;
			request.owner = _current_company; request.vent = vent.id; request.tile = request.endpoint = anchor; request.access = DiagDirection::SE;
			const auto claim = ClaimGeothermalSite(request); REQUIRE(claim.error == GeothermalError::None);
			funded = Command<Commands::BuildGeothermalCollector>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, anchor,
				std::make_tuple(anchor, claim.id, uint32_t{123}));
			REQUIRE(funded.Succeeded()); this->recipient = Industry::Get(FindGeothermalCollectorForVent(vent.id));
		} else {
			funded = Command<Commands::BuildGeothermalRiser>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, anchor,
				std::make_tuple(anchor, this->rock_portal, anchor, DiagDirection::SE, uint32_t{123}));
			REQUIRE(funded.Succeeded()); this->recipient = Industry::Get(FindGeothermalRiserForPortal(this->rock_portal));
		}
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, TileXYL(x, 4, MapLayer::Surface), RAILTYPE_RAIL, Axis::Y, 1, 5,
			STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		this->origin = Station::GetByTile(TileXYL(x, 4, MapLayer::Surface));
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, TileXYL(x, stop_y, MapLayer::DeepUnderground), RAILTYPE_RAIL, Axis::Y, 1, 5,
			STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		this->destination = Station::GetByTile(TileXYL(x, stop_y, MapLayer::DeepUnderground));
		this->depot = TileXYL(x, 2, MapLayer::Surface);
		REQUIRE(CmdBuildTrainDepot(DoCommandFlag::Execute, this->depot, RAILTYPE_RAIL, DiagDirection::SE).Succeeded());
		// One unbranched route leaves ordinary pathfinding, station stops and both
		// physical transitions responsible for all cross-layer packet custody.
		for (uint y = 3; y < 12; ++y) {
			const auto tile = TileXYL(x, y, MapLayer::Surface);
			if (!IsRailStationTile(tile)) this->Track(tile);
		}
		for (uint y = 13; y < 22; ++y) this->Track(TileXYL(x, y, MapLayer::Underground));
		for (uint y = 23; y < stop_y; ++y) this->Track(TileXYL(x, y, MapLayer::DeepUnderground));
		this->head = this->Buy(4, INVALID_CARGO);
		// Each cargo keeps its own real 64-unit native wagon. There is no mixed
		// fixture hold or direct industry receipt substituting for final unloading.
		for (const auto &input : GetMultiLayerGeothermalConstruction(this->kind)->inputs) {
			const auto cargo = GetCargoTypeByLabel(input.label);
			REQUIRE(this->destination->goods[cargo].status.Test(GoodsEntry::State::Acceptance));
			auto *wagon = this->Buy(5, cargo);
			REQUIRE(CmdMoveRailVehicle(DoCommandFlag::Execute, wagon->index, this->head->Last()->index, false).Succeeded());
			CargoSpec::Get(cargo)->current_payment = Money{1000}; // A paying-purpose regression cannot hide behind a zero tariff.
		}
		this->setup_charge = this->initial_cash - Company::Get(_current_company)->money;
		Order pickup; pickup.MakeGoToStation(this->origin->index); pickup.SetUnloadType(OrderUnloadType::NoUnload); pickup.SetLoadType(OrderLoadType::LoadIfPossible);
		Order drop; drop.MakeGoToStation(this->destination->index); drop.SetUnloadType(OrderUnloadType::UnloadIfPossible); drop.SetLoadType(OrderLoadType::NoLoad);
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, this->head->index, 0, pickup).Succeeded());
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, this->head->index, 1, drop).Succeeded());
		ProcessOrders(this->head); UpdateSignalsInBuffer();
		_pause_mode = {};
	}

	/** Retire actual vehicles before station, industry, company and definition owners; no caller data is deleted. */
	~ConstructionSupplyRig()
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu};
		while (Vehicle::GetNumItems() != 0) delete (*Vehicle::Iterate().begin())->First();
		for (Industry *industry : Industry::Iterate()) delete industry;
		for (Station *station : Station::Iterate()) delete station;
		for (Depot *item : Depot::Iterate()) delete item;
		InitNewsItemStructs(); _town_pool.CleanPool(); RebuildTownKdtree();
		_group_pool.CleanPool(); _engine_pool.CleanPool();
		ResetSurfaceSupplyOrders(); ResetGeothermalCollectors(); ResetGeothermalSiteClaims(); ResetGeothermalRisers();
		ResetUtilityPowerState(); ResetLayerPortalWorkState(); ResetIndustries();
		_industry_mngr.mappings = std::move(this->industries); _industile_mngr.mappings = std::move(this->tiles); _engine_mngr.mappings = std::move(this->engines);
	}

	/** @param tile Actual prepared ordinary track cell; preserve an already command-built portal approach. */
	void Track(TileIndex tile)
	{
		if (IsPlainRailTile(tile)) return;
		REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Track::Y, false).Succeeded());
	}

	/** @param upper Actual upper housing. @param lower Actual adjacent lower plane; both endpoints and approach tracks are command-built on explicit prepared floor. */
	void Portal(TileIndex upper, MapLayer lower)
	{
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, upper, TransportType::Rail, lower, DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE).Succeeded());
		for (const auto tile : {upper, MultiLayerMap::TileInLayer(upper, lower)}) this->Track(GetLayerPortalDoorTile(tile));
		REQUIRE(ChangeLayerPortalConnection(upper, _current_company, true, true) == LayerPortalWorkError::None);
	}

	/** @param model Actual first-party conventional definition. @param cargo Native wagon refit or locomotive default. @return Command-bought, charged real vehicle in the genuine depot. */
	Train *Buy(uint16_t model, CargoType cargo)
	{
		const auto [result, id, capacity, mail, capacities] = Command<Commands::BuildVehicle>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, this->depot,
			std::make_tuple(this->depot, GetUndergroundEngine(model), false, cargo, ClientID{1})); // Explicit standalone server actor required by the ordinary command kernel.
		REQUIRE(result.Succeeded());
		return Train::Get(id);
	}

	/** Buy the exact complete remaining canonical bill through native synchronized settlement; no destination stock changes. */
	void Purchase()
	{
		for (const auto &input : GetMultiLayerGeothermalConstruction(this->kind)->inputs) {
			const auto quote = PrepareSurfaceSupplyOrder(this->origin->index, input.label, input.quantity);
			REQUIRE(quote.error == SurfaceSupplyError::None);
			const auto before = Company::Get(_current_company)->money;
			REQUIRE(Command<Commands::OrderSurfaceSupply>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, this->origin->xy,
				std::make_tuple(this->origin->index, input.label, static_cast<uint16_t>(input.quantity), quote.next_order)).Succeeded());
			CHECK(Company::Get(_current_company)->money == before - quote.cost);
			this->seed_charge += quote.cost;
			CHECK(this->recipient->GetCargoAccepted(GetCargoTypeByLabel(input.label))->waiting == 0);
		}
	}

	/** @param count Actual industry dispatcher ticks after delivered stock exists; retained native state owns consumption and work. */
	void Work(uint16_t count)
	{
		AutoRestoreBackup pause{_pause_mode, PauseModes{}};
		for (uint16_t index = 0; index < count; ++index) { ++TimerGameTick::counter; OnTick_Industry(); }
	}
};

#endif /* TESTS_CONSTRUCTION_SUPPLY_FIXTURE_H */
