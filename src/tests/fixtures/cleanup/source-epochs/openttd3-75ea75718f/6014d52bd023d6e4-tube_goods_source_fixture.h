/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_goods_source_fixture.h Actual1975 original Forest/Sawmill source for Crate useful-route acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15CB/15CC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Completed native footprints and a generated town are disclosed boundary inputs; every Wood/Goods unit is earned by original production and driven delivery.
 */
#ifndef TESTS_TUBE_GOODS_SOURCE_FIXTURE_H
#define TESTS_TUBE_GOODS_SOURCE_FIXTURE_H
#include "mine_fixture.h"
#include "movement_fixture.h"
#include "tube_useful_tick_fixture.h"
#include "../station_cmd.h"
#include "../station_func.h"
#include "../rail_cmd.h"
#include "../road.h"
#include "../newgrf_station.h"
#include "../newgrf_roadstop.h"
#include "../order_cmd.h"
#include "../order_func.h"
#include "../depot_base.h"
#include "../town_cmd.h"
#include "../tile_cmd.h"
#include "../group.h"
#include "../underground_vehicle.h"
#include "../space_vehicle.h"
#include "../tbm.h"
#include "../company_manager_face.h"
#include "../spoil_receiver.h"
#include "../layer_industry_processor.h"
#include "../geothermal.h"
#include "../geothermal_riser.h"
#include "../geothermal_generator.h"
#include "../engine_func.h"
#include "../vehicle_cmd.h"
#include "../house.h"
#include "../newgrf_house.h"
#include "../table/strings.h"

/** Advance actual industry production once; public game-loop entry, without a test-specific stock owner. */
void OnTick_Industry();
extern TileIndex _cur_tileloop_tile; ///< Native landscape LFSR cursor, initialized as a normal new world before persistent observations.

/** Ordinary1975 Surface source and real native Wood train; later Crate routing consumes its existing Sawmill Goods. */
class TubeGoodsSourceRig : public MineAdmissionRig {
	decltype(_engine_mngr.mappings) engines = _engine_mngr.mappings; ///< Caller original/native catalogue scope.
	AutoRestoreBackup<decltype(_price)> native_prices{_price}; ///< Restore all native prices recomputed for ordinary continuation, including recurring costs.
	AutoRestoreBackup<decltype(_economy)> native_economy{_economy}; ///< Restore caller inflation and native economy state after this isolated world.
	AutoRestoreBackup<TileIndex> tile_loop{_cur_tileloop_tile, TileIndex{1}}; ///< Valid native LFSR state; zero would require first-load initialization.
	PathfinderDefaults pathfinder; ///< Native pathfinding budgets and retry defaults.
	AutoRestoreBackup<AccelerationModel> acceleration{_settings_game.vehicle.train_acceleration_model, AccelerationModel::Realistic}; ///< Real Wood mass/braking.
	AutoRestoreBackup<uint8_t> mass{_settings_game.vehicle.freight_trains, uint8_t{1}}; ///< Legal ordinary freight multiplier.
	AutoRestoreBackup<UnitID> train_limit{_settings_game.vehicle.max_trains, UnitID{500}}; ///< Ordinary acquisition limit.
	AutoRestoreBackup<uint8_t> length_limit{_settings_game.vehicle.max_train_length, uint8_t{7}}; ///< Normal two-part train admission.
	AutoRestoreBackup<uint8_t> raw_industry{_settings_game.construction.raw_industry_construction, uint8_t{1}}; ///< Legal player funding option for the original Forest.
	AutoRestoreBackup<TownFounding> town_funding{_settings_game.economy.found_town, TownFounding::CustomLayout}; ///< Legal company-funded native town.
	AutoRestoreBackup<int16_t> fluctuation{_economy.fluct, int16_t{1}}; ///< Explicit ordinary non-recession production.
	AutoRestoreBackup<uint16_t> scale{_settings_game.economy.industry_cargo_scale, uint16_t{100}}; ///< Normal original producer scale.
	AutoRestoreBackup<uint64_t> tick{TimerGameTick::counter, uint64_t{0}}; ///< Isolated native epoch, never a pose/cargo assignment.
	AutoRestoreBackup<TimerGameEconomy::Date> economy_date{TimerGameEconomy::date}; ///< Exact caller economy date.
	AutoRestoreBackup<TimerGameEconomy::DateFract> economy_fraction{TimerGameEconomy::date_fract}; ///< Native daily vehicle scheduling.
	AutoRestoreBackup<TimerGameEconomy::Year> economy_year{TimerGameEconomy::year}; ///< Exact caller cached economy year.
	AutoRestoreBackup<TimerGameEconomy::Month> economy_month{TimerGameEconomy::month}; ///< Exact caller economy month.
	AutoRestoreBackup<uint> economy_days{TimerGameEconomy::days_since_last_month}; ///< Exact caller native month bookkeeping.
	AutoRestoreBackup<TimerGameCalendar::DateFract> calendar_fraction{TimerGameCalendar::date_fract}; ///< Native technology clock fraction.
	AutoRestoreBackup<TimerGameCalendar::Year> calendar_year{TimerGameCalendar::year}; ///< Exact caller cached calendar year.
	AutoRestoreBackup<TimerGameCalendar::Month> calendar_month{TimerGameCalendar::month}; ///< Exact caller calendar month.
	AutoRestoreBackup<Money> train_price{_price[Price::BuildVehicleTrain], Money{10000}}; ///< Positive fixture acquisition price, separate from balance.
	AutoRestoreBackup<Money> wagon_price{_price[Price::BuildVehicleWagon], Money{1000}}; ///< Positive original wagon acquisition price.
	AutoRestoreBackup<Money> rail_price{_price[Price::BuildRail], Money{100}}; ///< Actual paid native route construction.
	AutoRestoreBackup<Money> depot_price{_price[Price::BuildDepotTrain], Money{500}}; ///< Actual paid depot construction.
	AutoRestoreBackup<Money> station_price{_price[Price::BuildStationRail], Money{500}}; ///< Actual paid two-cell platforms.
	AutoRestoreBackup<Money> town_price{_price[Price::BuildTown], Money{1000}}; ///< Positive actual native founding debit.
public:
	Industry *forest = nullptr; ///< Sole original raw Wood producer, with its unchanged native rate.
	Industry *sawmill = nullptr; ///< Sole original one-to-one Wood/Goods conversion owner.
	Station *wood_pickup = nullptr; ///< Native source station/packet owner.
	Station *goods_pickup = nullptr; ///< Real Wood acceptance and Sawmill Goods pickup berth.
	Train *wood_front = nullptr; ///< Native1950 locomotive, never moved outside the real driver.
	Train *wood_wagon = nullptr; ///< Unmodified original Wood wagon, operating only on Surface.
	Town *receiver = nullptr; ///< Actual company-founded native town, not an acceptance flag.
	TileIndex town_berth = INVALID_TILE; ///< Clear native two-cell site with actual Goods acceptance.
	CargoType wood = INVALID_CARGO; ///< Existing original registered Wood meaning.
	CargoType goods = INVALID_CARGO; ///< Existing original registered Goods meaning.
	std::array<TileIndex, 2> depots{TileXYL(4, 20, MapLayer::Surface), TileXYL(32, 20, MapLayer::Surface)}; ///< Real source route ends for ordinary service/reversal.
	uint supplied = 0; ///< Observed original production/dispatch receipt, never a stock owner.

	/** Construct the actual1975 source and its paid complete Surface route; no raw material is supplied by setup. */
	TubeGoodsSourceRig() : MineAdmissionRig(0, true)
	{
		TimerGameCalendar::SetDate(TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1975}, 0, 1), 0);
		TimerGameEconomy::SetDate(TimerGameEconomy::ConvertYMDToDate(TimerGameEconomy::Year{1975}, 0, 1), 0);
		this->PrepareFunding(); Town::Get(TownID{0})->townnametype = SPECSTR_TOWNNAME_START;
		// A newly created fixture company needs the same valid native face
		// identity as a normal company. Otherwise ordinary loading legitimately
		// replaces the missing style and consumes two simulation RNG draws.
		ResetFaces(); SetCompanyManagerFaceStyle(Company::Get(_current_company)->face, 0);
		_game_mode = GameMode::Normal; _pause_mode = {};
		// Full application loads rebuild original cargo properties from the
		// native climate table. Use that same owner before any purchase or work,
		// retaining the already activated custom definitions in their original
		// registered slots. This temporary setup copy is not a cargo registry.
		std::array<CargoSpec, NUM_CARGO> activated_cargo;
		for (uint index = 0; index < NUM_CARGO; ++index) activated_cargo[index] = *CargoSpec::Get(index);
		SetupCargoForClimate(LandscapeType::Temperate);
		for (uint index = 0; index < NUM_CARGO; ++index) {
			if (!activated_cargo[index].IsValid()) continue;
			auto *spec = CargoSpec::Get(index);
			if (spec->IsValid()) {
				REQUIRE(spec->label == activated_cargo[index].label);
			} else {
				*spec = activated_cargo[index]; _cargo_mask.Set(CargoType{static_cast<uint8_t>(index)});
			}
		}
		BuildCargoLabelMap();
		ResetRailTypes(); ResetRoadTypes(); StationClass::Reset(); RebuildStationKdtree(); ResetHouses();
		// Normal GRF finalization resolves original house cargo labels as well
		// as industry labels. Preserve native demand amounts and enabled houses.
		for (auto &house : HouseSpec::Specs()) {
			for (size_t index = 0; index < std::size(house.accepts_cargo_label); ++index) house.accepts_cargo[index] = GetCargoTypeByLabel(house.accepts_cargo_label[index]);
		}
		// Town construction updates both local and global native house counts;
		// reduced unit startup must initialize their real catalogue dimensions.
		ResetHouseClassIDs(); InitializeBuildingCounts();
		_engine_mngr.ResetToDefaultMapping(); SetupEngines();
		// Ordinary content finalization installs these complete native families
		// in this order. Preserve their actual dense EIDS identities from setup,
		// rather than asking the first loader to append missing definitions.
		REQUIRE(InitialiseTBMEngineDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseUndergroundEngineDefinitions(this->ActivatedContent()));
		{
			GRFConfigList caller_content = std::move(_grfconfig);
			CopyGRFConfigList(_grfconfig, this->ActivatedContent(), false);
			const bool space_ready = InitialiseSpaceEngineDefinition();
			_grfconfig = std::move(caller_content);
			REQUIRE(space_ready);
		}
		// Ordinary content loading reconstructs every delivered industry/tile
		// definition. Initialize the same public catalogue before any real source
		// is funded, so save readers need not append missing IIDS/TIDS records.
		REQUIRE(InitialiseSpoilReceiverDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseLayerMineDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseLayerProcessorDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseGeothermalCollectorDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseGeothermalRiserDefinitions(this->ActivatedContent()));
		REQUIRE(InitialiseGeothermalGeneratorDefinitions(this->ActivatedContent()));
		this->wood = GetCargoTypeByLabel(CargoLabel{"WOOD"}); this->goods = GetCargoTypeByLabel(CT_GOODS);
		REQUIRE(this->wood != INVALID_CARGO); REQUIRE(this->goods != INVALID_CARGO);
		// Resolve original label variants against the actual distributed registry.
		// The immutable Forest/Sawmill rates, multipliers, footprints and dates stay unchanged.
		for (const IndustryType type : {IndustryType{3}, IndustryType{2}}) {
			auto *spec = const_cast<IndustrySpec *>(GetIndustrySpec(type));
			for (size_t index = 0; index < spec->accepts_cargo_label.size(); ++index) {
				if (const auto *label = std::get_if<CargoLabel>(&spec->accepts_cargo_label[index])) spec->accepts_cargo[index] = GetCargoTypeByLabel(*label);
			}
			for (size_t index = 0; index < spec->produced_cargo_label.size(); ++index) {
				if (const auto *label = std::get_if<CargoLabel>(&spec->produced_cargo_label[index])) spec->produced_cargo[index] = GetCargoTypeByLabel(*label);
			}
		}
		// Native startup resolves original tile acceptance separately from the
		// industry inventory. Install that same label mapping for these exact
		// original Forest/Sawmill cells; acceptance amounts are never increased.
		for (IndustryGfx gfx = 11; gfx <= 16; ++gfx) {
			auto *spec = const_cast<IndustryTileSpec *>(GetIndustryTileSpec(gfx));
			for (size_t index = 0; index < spec->accepts_cargo_label.size(); ++index) {
				if (const auto *label = std::get_if<CargoLabel>(&spec->accepts_cargo_label[index])) spec->accepts_cargo[index] = GetCargoTypeByLabel(*label);
			}
		}
		this->forest = this->BuildSource(3, TileXYL(7, 14, MapLayer::Surface));
		this->sawmill = this->BuildSource(2, TileXYL(26, 15, MapLayer::Surface));
		REQUIRE(this->forest->produced[0].cargo == this->wood); REQUIRE(this->forest->produced[0].waiting == 0);
		REQUIRE(this->sawmill->produced[0].cargo == this->goods); REQUIRE(this->sawmill->produced[0].waiting == 0);
		this->wood_pickup = this->StationAtSource(TileXYL(8, 20, MapLayer::Surface));
		this->goods_pickup = this->StationAtSource(TileXYL(26, 20, MapLayer::Surface));
		REQUIRE(this->goods_pickup->goods[this->wood].status.Test(GoodsEntry::State::Acceptance));
		REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, this->depots[0], RAILTYPE_RAIL, DiagDirection::SW).Succeeded());
		REQUIRE(Command<Commands::BuildRailDepot>::Do(DoCommandFlag::Execute, this->depots[1], RAILTYPE_RAIL, DiagDirection::NE).Succeeded());
		// Only plain gaps need paid rail; actual platform/depot identities and
		// their native loading/turning behavior remain owned by those facilities.
		for (uint x = 5; x < 32; ++x) {
			const auto tile = TileXYL(x, 20, MapLayer::Surface);
			if (IsRailStationTile(tile)) continue;
			REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Track::X, false).Succeeded());
		}
		REQUIRE(CargoSpec::Get(this->wood)->weight == 16); REQUIRE(CargoSpec::Get(this->wood)->is_freight);
		Engine::Get(GetUndergroundEngine(4))->company_avail.Set(_current_company);
		auto *wood_engine = Engine::Get(EngineID{34});
		// Normal startup resolves each original engine's immutable cargo label
		// after SetupEngines. This is the original Wood wagon, not a new refit or
		// a capability exception; all capacity/length/physics remain native.
		REQUIRE(std::get<CargoLabel>(wood_engine->info.cargo_label) == CargoLabel{"WOOD"});
		wood_engine->info.cargo_type = this->wood; wood_engine->company_avail.Set(_current_company);
		this->wood_front = this->BuySourcePart(GetUndergroundEngine(4)); this->wood_wagon = this->BuySourcePart(EngineID{34});
		REQUIRE(this->wood_wagon->cargo_type == this->wood);
		REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, this->wood_wagon->index, this->wood_front->index, false).Succeeded());
		this->SourceOrders(); this->PrepareTownReceiver();
		// The route must continue under the same native tariffs and recurring
		// costs that ordinary loading reconstructs from saved inflation. Prepared
		// infrastructure retains its disclosed positive construction boundaries;
		// every subsequent delivery and operating cost uses native base currency.
		_economy.inflation_prices = _economy.inflation_payment = uint64_t{1} << 16;
		RecomputePrices();
	}

	/** Retire actual train/packet owners before parent industries and restore the caller's engine catalogue. */
	~TubeGoodsSourceRig()
	{
		// Native whole-consist destruction retires station loading membership and
		// any payment; later fixture teardown must not leave a front referencing it.
		while (Vehicle::GetNumItems() != 0) delete (*Vehicle::Iterate().begin())->First();
		_group_pool.CleanPool(); _engine_pool.CleanPool(); _engine_mngr.mappings = std::move(this->engines);
	}

	/** @param type Existing original Surface definition. @param tile Native footprint anchor. @return Actual command-funded completed industry, with empty original stocks. */
	Industry *BuildSource(IndustryType type, TileIndex tile)
	{
		const auto cost = Command<Commands::BuildIndustry>::Do(DoCommandFlag::Execute, tile, type, 0, true, 123);
		INFO(fmt::format("source type={} tile={} error={}", type, tile.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded());
		auto *industry = Industry::GetByTile(tile);
		// Completion is an explicit machinery boundary, as in the Mole route;
		// it grants neither Wood nor Goods and never changes the production rate.
		for (const auto part : industry->location) if (industry->TileBelongsToIndustry(part)) SetIndustryCompleted(part);
		return industry;
	}

	/** @param tile Actual two-cell native Surface loading berth. @return Command-built sole station/queue owner. */
	Station *StationAtSource(TileIndex tile)
	{
		REQUIRE(Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, tile, RAILTYPE_RAIL, Axis::X, 1, 2, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		return Station::GetByTile(tile);
	}

	/** @param engine Existing native locomotive or original Wood wagon. @return Command-purchased native part in its actual depot. */
	Train *BuySourcePart(EngineID engine)
	{
		const auto [cost, id, capacity, mail, capacities] = Command<Commands::BuildVehicle>::Do(DoCommandFlag::Execute, this->depots[0], engine, false, INVALID_CARGO, ClientID::Invalid);
		INFO(fmt::format("source engine={} error={}", engine.base(), cost.GetErrorMessage())); REQUIRE(cost.Succeeded()); return Train::Get(id);
	}

	/** Install the normal source/load, Sawmill/unload and two actual service depot orders; no arrival or skip is supplied. */
	void SourceOrders()
	{
		Order load; load.MakeGoToStation(this->wood_pickup->index); load.SetUnloadType(OrderUnloadType::NoUnload);
		Order unload; unload.MakeGoToStation(this->goods_pickup->index); unload.SetUnloadType(OrderUnloadType::UnloadIfPossible); unload.SetLoadType(OrderLoadType::NoLoad);
		Order far; far.MakeGoToDepot(Depot::GetByTile(this->depots[1])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		Order home; home.MakeGoToDepot(Depot::GetByTile(this->depots[0])->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {});
		const std::array list{load, unload, far, home};
		for (size_t index = 0; index < list.size(); ++index) REQUIRE(Command<Commands::InsertOrder>::Do(DoCommandFlag::Execute, this->wood_front->index, static_cast<VehicleOrderID>(index), list[index]).Succeeded());
		ProcessOrders(this->wood_front);
	}

	/** Generate a real funded town and locate clear platform cells whose native catchment genuinely accepts Goods. */
	void PrepareTownReceiver()
	{
		const auto [cost, additional, id] = Command<Commands::FoundTown>::Do(DoCommandFlag::Execute, TileXYL(48, 45, MapLayer::Surface), TownSize::Medium, true, TownLayout::Grid3x3, false, 123, "Crate receiver");
		INFO(fmt::format("town error={} extra={}", cost.GetErrorMessage(), additional.base())); REQUIRE(cost.Succeeded()); REQUIRE(id != TownID::Invalid());
		this->receiver = Town::Get(id);
		// Native generated houses alone establish demand. Search a bounded
		// neighbourhood for clear two-cell station space, without clearing houses
		// or setting acceptance flags to manufacture a useful final destination.
		for (uint y = 33; y < 58; ++y) for (uint x = 35; x < 59; ++x) {
			const auto tile = TileXYL(x, y, MapLayer::Surface);
			// Both platform cells must remain genuine clear space; native houses
			// supplying demand cannot be demolished merely to force a chosen site.
			if (!IsTileType(tile, TileType::Clear) || !IsTileType(TileAddWrap(tile, 1, 0), TileType::Clear)) continue;
			const auto [acceptance, always] = GetAcceptanceAroundTiles(tile, 2, 1, 4);
			if (acceptance[this->goods] < 8) continue;
			this->town_berth = tile; break;
		}
		INFO(fmt::format("town houses={} population={}", this->receiver->cache.num_houses, this->receiver->cache.population));
		REQUIRE(this->town_berth != INVALID_TILE);
	}

	/** Advance one complete native vehicle opportunity and verify the original source train's physical integrity. */
	void RunSource()
	{
		RunUsefulTubeNativeOpportunity();
		REQUIRE_FALSE(this->wood_front->vehstatus.Test(VehState::Crashed)); REQUIRE(IsTrainOnItsTiles(this->wood_front));
	}
};

/** @param rig Actual original1975 producer/physical route. Earn native Goods while retaining this same world for the subsequent Crate useful route. */
void EarnTubeGoodsSource(TubeGoodsSourceRig &rig);
#endif /* TESTS_TUBE_GOODS_SOURCE_FIXTURE_H */
