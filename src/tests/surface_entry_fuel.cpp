/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_fuel.cpp Pure native Propellant capital and ordinary commercial-freight characterization.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BI.1.
 * Quoted tariffs and unsold values are planning information only. Original
 * company money/debt, packet delivery and industry inventories remain sole owners.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../industry_cmd.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../layer_industry_processor.h"
#include "../multilayer_recipe.h"
#include "../vehicle_base.h"
#include "../vehicle_cmd.h"
#include "../train.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../underground_vehicle.h"
#include "../economy_func.h"
#include "../cargotype.h"
#include "../cargopacket.h"
#include "../station_base.h"
#include "../station_func.h"
#include "../station_map.h"
#include "../town.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_chemical_layout.h"
#include "surface_fuel_rail_layout.h"
#include "../safeguards.h"

static std::function<void()> *surface_fuel_observer = nullptr; ///< Invocation callback, never another source of credit or cargo.

/** Invoke this loaded world's original native financial observer.
 * @param args Parameter-free native console arguments.
 * @return True after invoking the current observer.
 */
static bool ObserveSurfaceFuel(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_fuel_observer != nullptr); (*surface_fuel_observer)(); return true;
}

/** Save native owners without earning work or changing the original predecessor.
 * @param path Fresh invocation-owned output.
 */
static void SaveSurfaceFuel(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Query one whole original command inside its native estimate/nesting scope.
 * @tparam Tcmd Original synchronized command identity.
 * @tparam Targs Complete original argument types.
 * @param tile Actual action tile, or zero for coordinate-free queries.
 * @param args Complete native arguments, including tiled command anchors.
 * @return Unchanged original quote or refusal, without executing the action.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceFuel(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** Export actual original industry/town providers and native tariffs without predicting earned profit. */
static void InspectSurfaceFuelCommercialWorld()
{
	std::set<CargoType> cargoes;
	// Actual generated and paid industries determine available production and
	// receiving roles. Definitions alone cannot supply a missing raw provider.
	for (const auto *industry : Industry::Iterate()) {
		if (GetTileLayer(industry->location.tile) != MapLayer::Surface) continue;
		fmt::print("SURFACE_FUEL_INDUSTRY id={} type={} anchor={} x={} y={} founder={} town={} prod_level={}\n", industry->index.base(), industry->type,
			industry->location.tile.base(), TileX(industry->location.tile), TilePlaneY(industry->location.tile), industry->founder.base(), industry->town->index.base(), industry->prod_level);
		for (const auto &product : industry->produced) {
			if (!IsValidCargoType(product.cargo)) continue;
			cargoes.insert(product.cargo);
			fmt::print("SURFACE_FUEL_PROVIDER industry={} cargo={} label={} rate={} waiting={} last_production={} last_transported={}\n", industry->index.base(),
				product.cargo, CargoSpec::Get(product.cargo)->label.AsString(), product.rate, product.waiting,
				product.history[LAST_MONTH].production, product.history[LAST_MONTH].transported);
		}
		for (const auto &input : industry->accepted) {
			if (!IsValidCargoType(input.cargo)) continue;
			fmt::print("SURFACE_FUEL_RECEIVER industry={} cargo={} label={} waiting={}\n", industry->index.base(), input.cargo,
				CargoSpec::Get(input.cargo)->label.AsString(), input.waiting);
		}
		const auto *spec = GetIndustrySpec(industry->type);
		// Existing original factories can turn both real farm outputs into
		// useful Goods. Export exact native multipliers rather than assuming
		// the previously proven Steel input is the only production alternative.
		for (size_t input = 0; input < spec->accepts_cargo.size(); ++input) {
			if (!IsValidCargoType(spec->accepts_cargo[input])) continue;
			for (size_t output = 0; output < spec->produced_cargo.size(); ++output) {
				if (!IsValidCargoType(spec->produced_cargo[output])) continue;
				if (CargoSpec::Get(spec->produced_cargo[output])->label != CargoLabel{"GOOD"}) continue;
				fmt::print("SURFACE_FUEL_CONVERSION industry={} input={} output=GOOD multiplier={} denominator=256\n", industry->index.base(),
					CargoSpec::Get(spec->accepts_cargo[input])->label.AsString(), spec->input_cargo_multiplier[input][output]);
			}
		}
	}
	for (const auto *town : Town::Iterate()) {
		fmt::print("SURFACE_FUEL_TOWN id={} anchor={} population={} company_rating={}\n", town->index.base(), town->xy.base(),
			town->cache.population, town->ratings[_current_company]);
	}
	for (const auto label : {CargoLabel{"PASS"}, CargoLabel{"MAIL"}, CargoLabel{"SALT"}}) {
		const auto cargo = GetCargoTypeByLabel(label);
		if (IsValidCargoType(cargo)) cargoes.insert(cargo);
	}
	// The original economy quotes planning bounds only. Actual offered stock,
	// transit age, accepted quantity, operating costs and payment determine cash.
	for (const CargoType cargo : cargoes) {
		const auto *spec = CargoSpec::Get(cargo);
		fmt::print("SURFACE_FUEL_TARIFF cargo={} label={} base={} unit_d16={} unit_d32={} unit_d48={} unit_d64={} load14_d40={} load60_d40={}\n", cargo,
			spec->label.AsString(), spec->current_payment.base(), GetTransportedGoodsIncome(1, 16, 16, cargo).base(), GetTransportedGoodsIncome(1, 32, 16, cargo).base(),
			GetTransportedGoodsIncome(1, 48, 16, cargo).base(), GetTransportedGoodsIncome(1, 64, 16, cargo).base(),
			GetTransportedGoodsIncome(14, 40, 16, cargo).base(), GetTransportedGoodsIncome(60, 40, 16, cargo).base());
	}
}

/** Query actually available conventional rail heads and crop/livestock/Goods wagons at the existing paid depot. */
static void QuoteSurfaceFuelRailCarriers()
{
	const TileIndex depot{1373};
	// Whole purchases at this actual ordinary rail depot must admit every
	// locomotive/wagon. Monorail/maglev or foreign-layer availability alone
	// cannot finance a conventional Surface freight arrangement.
	for (const auto *engine : Engine::IterateType(VehicleType::Train)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Train, _current_company)) continue;
		if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
		const auto kind = engine->VehInfo<RailVehicleInfo>().railveh_type;
		const auto cargo = engine->GetDefaultCargoType();
		const auto label = IsValidCargoType(cargo) ? CargoSpec::Get(cargo)->label : CargoLabel{"NONE"};
		if (kind == RailVehicleType::Wagon) {
			// These three separate roles are required for real two-input farm
			// freight and later factory-to-town output, not an invented refit.
			if (label != CargoLabel{"GRAI"} && label != CargoLabel{"LVST"} && label != CargoLabel{"GOOD"}) continue;
		}
		auto quote = QuoteSurfaceFuel<Commands::BuildVehicle>(depot, depot, engine->index, false, INVALID_CARGO, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		fmt::print("SURFACE_FUEL_RAIL_CARRIER engine={} kind={} label={} cost={} capacity={} running_cost={} speed={}\n", engine->index.base(),
			to_underlying(kind), label.AsString(), std::get<0>(quote).GetCost().base(), std::get<2>(quote), engine->GetRunningCost().base(), engine->GetDisplayMaxSpeed());
	}
}

/** Query the complete proposed commercial fleet through original whole purchases.
 * @param railway Complete disjoint original-command infrastructure quote.
 */
static void QuoteSurfaceFuelCommercialFleet(const SurfaceFuelRailLayout &railway)
{
	Money capital{0}; Money annual{0};
	for (size_t role = 0; role < railway.lines.size(); ++role) {
		const auto depot = railway.lines[role].source.depot;
		const auto cargo = GetCargoTypeByLabel(role == 0 ? CargoLabel{"GRAI"} : role == 1 ? CargoLabel{"GOOD"} : CargoLabel{"COAL"});
		auto head = QuoteSurfaceFuel<Commands::BuildVehicle>(depot, depot, EngineID{22}, false, cargo, ClientID::Server);
		// An unbuilt depot cannot authorize vehicle purchase, so use the real
		// existing depot for model/refit admission. The selected future depot's
		// conventional track type is separately admitted by its original quote.
		if (std::get<0>(head).Failed()) head = QuoteSurfaceFuel<Commands::BuildVehicle>(TileIndex{1373}, TileIndex{1373}, EngineID{22}, false, cargo, ClientID::Server);
		REQUIRE(std::get<0>(head).Succeeded()); capital += std::get<0>(head).GetCost(); annual += Engine::Get(EngineID{22})->GetRunningCost();
		fmt::print("SURFACE_FUEL_FLEET role={} engine=22 count=1 label={} cost={} capacity={} running_cost={} speed={}\n", role,
			CargoSpec::Get(cargo)->label.AsString(), std::get<0>(head).GetCost().base(), std::get<2>(head), Engine::Get(EngineID{22})->GetRunningCost().base(), Engine::Get(EngineID{22})->GetDisplayMaxSpeed());
		const std::vector<std::pair<EngineID, uint>> wagons = role == 0 ? std::vector<std::pair<EngineID, uint>>{{EngineID{31}, 6}, {EngineID{33}, 6}} :
			role == 1 ? std::vector<std::pair<EngineID, uint>>{{EngineID{32}, 12}} : std::vector<std::pair<EngineID, uint>>{{GetUndergroundEngine(5), 4}};
		for (const auto &[engine, count] : wagons) {
			auto quote = QuoteSurfaceFuel<Commands::BuildVehicle>(TileIndex{1373}, TileIndex{1373}, engine, false, INVALID_CARGO, ClientID::Server);
			REQUIRE(std::get<0>(quote).Succeeded()); capital += count * std::get<0>(quote).GetCost();
			fmt::print("SURFACE_FUEL_FLEET role={} engine={} count={} label={} cost={} capacity={} running_cost=0 speed=0\n", role, engine.base(), count,
				CargoSpec::Get(Engine::Get(engine)->GetDefaultCargoType())->label.AsString(), std::get<0>(quote).GetCost().base(), std::get<2>(quote));
		}
	}
	fmt::print("SURFACE_FUEL_COMMERCIAL_CAPITAL infrastructure={} vehicles={} whole={} annual_head_running={}\n", railway.invoice.base(), capital.base(), (railway.invoice + capital).base(), annual.base());
	const auto *company = Company::Get(_current_company);
	Money existing_running{0};
	// Include every original company vehicle, even stopped reserve trucks, as
	// a conservative allowance; the actually sold Salt train is excluded once.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->owner != _current_company) continue;
		if (!vehicle->IsPrimaryVehicle()) continue;
		if (vehicle->index == VehicleID{18}) continue;
		existing_running += vehicle->GetDisplayRunningCost();
	}
	/** @param line Actual whole railway. @return Surface Manhattan distance used by native payment planning. */
	const auto distance = [](const SurfaceFuelRailLine &line) {
		return std::abs(int(TileX(line.source.anchor)) - int(TileX(line.target.anchor))) +
			std::abs(int(TilePlaneY(line.source.anchor)) - int(TilePlaneY(line.target.anchor)));
	};
	const auto *farm = Industry::Get(IndustryID{7}); uint monthly = UINT_MAX;
	// The weaker of the two actual last-month farm outputs bounds both input
	// streams; planning cannot count production that the generated world lacks.
	for (const auto &product : farm->produced) if (IsValidCargoType(product.cargo)) monthly = std::min(monthly, uint(product.history[LAST_MONTH].production));
	REQUIRE(monthly != UINT_MAX); const uint captured = monthly * 70 / 100;
	const auto raw = GetTransportedGoodsIncome(captured, distance(railway.lines[0]), 32, GetCargoTypeByLabel(CargoLabel{"GRAI"})) +
		GetTransportedGoodsIncome(captured, distance(railway.lines[0]), 32, GetCargoTypeByLabel(CargoLabel{"LVST"}));
	const auto goods = GetTransportedGoodsIncome(captured * 2 * 70 / 100, distance(railway.lines[1]), 32, GetCargoTypeByLabel(CargoLabel{"GOOD"}));
	const auto *coal_source = Industry::Get(IndustryID{0});
	const uint coal_captured = coal_source->produced.front().history[LAST_MONTH].production * 70 / 100;
	const auto coal = GetTransportedGoodsIncome(coal_captured, distance(railway.lines[2]), 32, GetCargoTypeByLabel(CargoLabel{"COAL"}));
	const auto interest = company->current_loan * _economy.interest_rate / 100;
	const auto overhead = 12 * (_price[Price::StationValue] >> 2); const Money contingency{10000};
	// This world's disabled infrastructure maintenance is observed, never
	// changed. A different setting requires a new complete cost arrangement.
	REQUIRE_FALSE(_settings_game.economy.infrastructure_maintenance);
	const auto forecast = 12 * (raw + goods + coal) - interest - annual - existing_running - overhead - contingency;
	REQUIRE(forecast > 0);
	fmt::print("SURFACE_FUEL_PLANNING_BOUND monthly_each={} capture_percent=70 captured_each={} goods_captured={} coal_captured={} transit_periods=32 annual_gross={} interest={} new_running={} existing_running={} overhead={} contingency={} annual_net={} infrastructure_maintenance={} inflation={} earned_income=0\n",
		monthly, captured, captured * 2 * 70 / 100, coal_captured, (12 * (raw + goods + coal)).base(), interest.base(), annual.base(), existing_running.base(), overhead.base(), contingency.base(), forecast.base(),
		_settings_game.economy.infrastructure_maintenance, _settings_game.economy.inflation);
}

/** Query actual goods-accepting town bay candidates and original buildable product trucks. */
static void QuoteSurfaceFuelGoodsRoutes()
{
	const CargoType goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}); REQUIRE(IsValidCargoType(goods));
	const auto origin = Industry::Get(IndustryID{11})->location.tile;
	std::vector<TileIndex> candidates;
	for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) candidates.push_back(tile);
	/** @param tile Real receiving-bay candidate. @return Farthest real Surface distance from the actual Goods factory, then native tile ID. */
	const auto rank = [origin](TileIndex tile) {
		return std::make_pair(-(std::abs(int(TileX(tile)) - int(TileX(origin))) +
			std::abs(int(TilePlaneY(tile)) - int(TilePlaneY(origin)))), tile.base());
	};
	std::ranges::sort(candidates, {}, rank);
	std::map<TownID, uint> selected; SurfaceChemicalLayout geometry;
	// Native house acceptance, original station placement and legal physical
	// road entrances must all agree. A town population is not an acceptance flag.
	for (TileIndex tile : candidates) {
		if (!IsTileType(tile, TileType::Clear)) continue;
		if (SurfaceChemicalLayout::FactoryCell(tile)) continue;
		const auto *town = ClosestTownFromTile(tile, UINT_MAX); REQUIRE(town != nullptr);
		if (selected[town->index] >= 4) continue;
		const auto [acceptance, always] = GetAcceptanceAroundTiles(tile, 1, 1, CA_TRUCK);
		if (acceptance[goods] < 8) continue;
		if (!always.Test(goods)) continue;
		for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			const auto entrance = SurfaceChemicalLayout::Step(tile, direction);
			if (!geometry.CanRoad(entrance, ROAD_ALL)) continue;
			const auto quote = SurfaceChemicalLayout::QuoteBay(tile, direction);
			if (quote.Failed()) continue;
			++selected[town->index];
			fmt::print("SURFACE_FUEL_GOODS_BAY town={} tile={} entrance={} direction={} acceptance={} cost={} source_distance={}\n", town->index.base(),
				tile.base(), entrance.base(), to_underlying(direction), acceptance[goods], quote.GetCost().base(), -rank(tile).first);
			break;
		}
	}
	// Only original actually available whole purchases count as useful carriers.
	// Refits must pass the native command kernel, without a temporary-sale refund.
	for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
		if (engine->GetDefaultCargoType() != goods) continue;
		if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
		if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
		auto quote = QuoteSurfaceFuel<Commands::BuildVehicle>(TileIndex{2443}, TileIndex{2443}, engine->index, false, goods, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		fmt::print("SURFACE_FUEL_GOODS_TRUCK engine={} cost={} capacity={} running_cost={} speed={}\n", engine->index.base(),
			std::get<0>(quote).GetCost().base(), std::get<2>(quote), engine->GetRunningCost().base(), engine->GetDisplayMaxSpeed());
	}
}

/** Inspect genuine Propellant funding, existing paid infrastructure and real ordinary earning opportunities. */
static void CharacterizeSurfaceFuel()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == 181985); REQUIRE(company->current_loan == 4650000);
	const auto cash = company->money, debt = company->current_loan; const auto expenses = company->yearly_expenses;
	const auto count = Vehicle::GetNumItems(), industries = Industry::GetNumItems(), packets = CargoPacket::GetNumItems();
	const auto type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}); REQUIRE(type != IT_INVALID);
	const TileIndex site{1639};
	auto factory = QuoteSurfaceFuel<Commands::BuildIndustry>(site, site, type, uint32_t{0}, true, uint32_t{123}); REQUIRE(factory.Succeeded());
	const auto *train = Train::Get(VehicleID{18}); REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->tile == TileIndex{1373});
	for (const auto *part = train; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
	auto sale = QuoteSurfaceFuel<Commands::SellVehicle>(TileIndex{0}, train->index, true, false, ClientID::Server); REQUIRE(sale.Succeeded());
	const auto propellant = GetCargoTypeByLabel(CargoLabel{"PROP"}); REQUIRE(IsValidCargoType(propellant));
	auto tanker = QuoteSurfaceFuel<Commands::BuildVehicle>(TileIndex{2443}, TileIndex{2443}, EngineID{132}, false, propellant, ClientID::Server);
	REQUIRE(std::get<0>(tanker).Succeeded());
	CHECK(GetStationIndex(TileIndex{1895}) == StationID{9}); CHECK(GetStationIndex(TileIndex{1896}) == StationID{10});
	const Money buffer{100000}; const auto capital = factory.GetCost() + std::get<0>(tanker).GetCost();
	const auto headroom = company->GetMaxLoan() - debt;
	const auto available_after_sale = cash + headroom - sale.GetCost();
	const auto shortfall = std::max(Money{0}, capital + buffer - available_after_sale);
	fmt::print("SURFACE_FUEL_CAPITAL site={} factory={} tanker={} initial_total={} buffer={} cash={} debt={} ceiling={} headroom={} empty_train_sale={} available_after_sale={} shortfall={} interest_rate={}\n",
		site.base(), factory.GetCost().base(), std::get<0>(tanker).GetCost().base(), capital.base(), buffer.base(), cash.base(), debt.base(), company->GetMaxLoan().base(),
		headroom.base(), -sale.GetCost().base(), available_after_sale.base(), shortfall.base(), _economy.interest_rate);
	const auto coal = GetCargoTypeByLabel(CargoLabel{"COAL"}); REQUIRE(IsValidCargoType(coal));
	auto refit = QuoteSurfaceFuel<Commands::RefitVehicle>(TileIndex{0}, VehicleID{19}, coal, uint8_t{0}, false, true, uint8_t{1});
	auto wagon = QuoteSurfaceFuel<Commands::BuildVehicle>(TileIndex{1373}, TileIndex{1373}, GetUndergroundEngine(5), false, coal, ClientID::Server);
	fmt::print("SURFACE_FUEL_EXISTING_TRAIN coal_refit_admitted={} coal_refit_cost={} coal_wagon_admitted={} coal_wagon_cost={} coal_capacity={} locomotive_running_cost={}\n",
		std::get<0>(refit).Succeeded(), std::get<0>(refit).GetCost().base(), std::get<0>(wagon).Succeeded(), std::get<0>(wagon).GetCost().base(),
		std::get<2>(wagon), train->GetDisplayRunningCost().base());
	InspectSurfaceFuelCommercialWorld(); QuoteSurfaceFuelGoodsRoutes(); QuoteSurfaceFuelRailCarriers();
	SurfaceFuelRailLayout railway; railway.Prepare(); QuoteSurfaceFuelCommercialFleet(railway);
	CHECK(company->money == cash); CHECK(company->current_loan == debt); CHECK(company->yearly_expenses == expenses);
	CHECK(Vehicle::GetNumItems() == count); CHECK(Industry::GetNumItems() == industries); CHECK(CargoPacket::GetNumItems() == packets);
}

/** Original native quotes preserve every actual physical/financial owner before any new fuel investment. */
TEST_CASE("Authentic Surface fuel finance retains pure original capital and commercial quotes", "[.surface-entry-fuel]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-fuel"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve native failures through original shutdown without inventing compensating material or finance. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceFuel(paths.saves / "surface-fuel-before-query-440.sav"); CharacterizeSurfaceFuel();
			SaveSurfaceFuel(paths.saves / "surface-fuel-query-440.sav");
			fmt::print("SURFACE_FUEL_COMPLETE pure=true supplied_stock=false\n"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_fuel_observer, &inspect); IConsole::CmdRegister("surface_entry_fuel", ObserveSurfaceFuel);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
