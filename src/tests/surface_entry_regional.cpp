/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_regional.cpp Genuine paid regional passenger service and measured native earnings.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQA.
 * Public commands and actual game ticks retain every financial, cargo and movement owner.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../command_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../roadveh.h"
#include "../vehicle_cmd.h"
#include "../order_cmd.h"
#include "../station_base.h"
#include "../misc_cmd.h"
#include "../tree_cmd.h"
#include "../town.h"
#include "../water_map.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_entry_work.h"
#include "surface_regional_layout.h"
#include "../safeguards.h"

static std::function<void()> *surface_regional_observer = nullptr; ///< Invocation callback, never a native owner.

/** @param args Parameter-free original console arguments. @return True after inspecting the actual loaded world. */
static bool ObserveSurfaceRegional(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_regional_observer != nullptr); (*surface_regional_observer)(); return true;
}

/** @param path Fresh invocation-owned output. Save actual owners without earning additional work. */
static void SaveSurfaceRegional(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Quote one complete original command without mutation.
 * @tparam Tcmd Native command identity. @tparam Targs Native argument types.
 * @param tile Actual tiled action, or zero for management. @param args Entire native argument list.
 * @return Original whole cost or refusal, never a supplied prerequisite.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceRegional(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** Freshly quote and settle one complete original synchronized command.
 * @tparam Tcmd Native command identity. @tparam Targs Native argument types.
 * @param tile Actual tiled action, or zero for management. @param args Entire native argument list.
 * @return Original settled result with exact fresh invoice.
 */
template <Commands Tcmd, typename... Targs>
static auto PaySurfaceRegional(TileIndex tile, Targs... args)
{
	auto quote = QuoteSurfaceRegional<Tcmd>(tile, args...);
	INFO("Regional command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(quote).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(quote).Succeeded());
	auto paid = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(args...));
	REQUIRE(ExtractCommandCost(paid).Succeeded()); REQUIRE(ExtractCommandCost(paid).GetCost() == ExtractCommandCost(quote).GetCost());
	fmt::print("SURFACE_REGIONAL_INVOICE command={} tile={} cost={}\n", GetCommandName(Tcmd), tile.base(), ExtractCommandCost(paid).GetCost().base()); return paid;
}

/** Fresh whole three-bus funding admission, excluding future fares and unsold assets. */
struct SurfaceRegionalBudget {
	Money unit{0}; ///< Current genuine original model119 PASS purchase quote.
	Money credit{0}; ///< Matching native credit required to preserve the original reserve.
};

/** @param infrastructure Current complete road/stops bill, or zero after that admitted infrastructure is paid.
 * @return Fresh native three-empty-bus and matching whole-service credit admission.
 */
static SurfaceRegionalBudget QuerySurfaceRegionalBudget(Money infrastructure)
{
	const auto *company = Company::Get(_current_company); const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"});
	REQUIRE(IsEngineBuildable(EngineID{119}, VehicleType::Road, _current_company));
	const auto bus = QuoteSurfaceRegional<Commands::BuildVehicle>(SurfaceRegionalLayout::depot, SurfaceRegionalLayout::depot, EngineID{119}, false, passenger, ClientID::Server);
	REQUIRE(std::get<0>(bus).Succeeded()); REQUIRE(std::get<2>(bus) == 40); REQUIRE(RoadVehicle::CanAllocateItem(3));
	SurfaceRegionalBudget result; result.unit = std::get<0>(bus).GetCost();
	const auto total = infrastructure + result.unit * 3; const auto deficit = total + Money{150000} - company->money;
	if (deficit > 0) result.credit = Money{((deficit.base() + 9999) / 10000) * 10000};
	REQUIRE(company->current_loan + result.credit <= company->GetMaxLoan());
	if (result.credit > 0) REQUIRE(QuoteSurfaceRegional<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, result.credit).Succeeded());
	fmt::print("SURFACE_REGIONAL_PLAN buses=3 model=119 capacity=40 unit={} infrastructure={} gross={} reserve=150000 credit={} cash={} debt={} ceiling={} depot=2984 projected_fares=0\n",
		result.unit.base(), infrastructure.base(), total.base(), result.credit.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base()); return result;
}

/** @param execute Whether to pay the independently admitted original civic tree work.
 * Querying a refused future stop never supplies authority approval; actual planting must precede its fresh service quote.
 */
static void PrepareSurfaceRegionalCivic(bool execute)
{
	const auto *company = Company::Get(_current_company); const auto *town = Town::Get(TownID{0});
	const int before = town->ratings[_current_company], target = RATING_VERYPOOR + 100;
	const uint needed = before >= target ? 0 : (target - before + RATING_TREE_UP_STEP - 1) / RATING_TREE_UP_STEP;
	std::vector<TileIndex> trees; Money cost{0};
	// Buy only genuinely clear native-authority land. Existing forests cannot
	// grant a second planting benefit; prepared launch access remains exclusive.
	for (const TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) {
		if (trees.size() == needed) break;
		// The original tree command also admits non-steep natural shore. Its
		// own quote owns that decision; paid canals and existing trees stay intact.
		if (!IsTileType(tile, TileType::Clear) && !(IsTileType(tile, TileType::Water) && IsCoast(tile))) continue;
		if (SurfaceChemicalLayout::FactoryCell(tile)) continue;
		if (tile == TileIndex{2504}) continue;
		if (TileX(tile) >= 10 && TileX(tile) < 13 && TilePlaneY(tile) >= 45 && TilePlaneY(tile) < 49) continue; // Preserve the actual paid flat3x4launch footprint.
		if (ClosestTownFromTile(tile, _settings_game.economy.dist_local_authority) != town) continue;
		const auto quote = QuoteSurfaceRegional<Commands::PlantTree>(tile, tile, tile, uint8_t{0}, false); if (quote.Failed()) continue;
		trees.push_back(tile); cost += quote.GetCost();
		fmt::print("SURFACE_REGIONAL_TREE tile={} cost={}\n", tile.base(), quote.GetCost().base());
	}
	fmt::print("SURFACE_REGIONAL_CIVIC_SEARCH needed={} available={} limit={} cost={}\n", needed, trees.size(), company->tree_limit >> 16, cost.base());
	// A pure characterization may report a real refusal. Payment needs every
	// independent land, native planting-limit and reserve prerequisite to hold.
	const bool admitted = trees.size() == needed && (company->tree_limit >> 16) > needed && company->money - cost >= 150000;
	fmt::print("SURFACE_REGIONAL_CIVIC town=0 rating={} target={} needed={} trees={} cost={} reserve=150000 credit=0 cash={} debt={} limit={} executed={} admitted={}\n",
		before, target, needed, trees.size(), cost.base(), company->money.base(), company->current_loan.base(), company->tree_limit >> 16, execute, admitted);
	if (!execute) return;
	REQUIRE(admitted);
	const auto cash = company->money, debt = company->current_loan;
	for (const auto tile : trees) PaySurfaceRegional<Commands::PlantTree>(tile, tile, tile, uint8_t{0}, false);
	REQUIRE(company->money == cash - cost); REQUIRE(company->current_loan == debt);
	REQUIRE(town->ratings[_current_company] == before + int(trees.size()) * RATING_TREE_UP_STEP);
	fmt::print("SURFACE_REGIONAL_CIVIC_PAID rating={} cost={} cash={} debt={}\n", town->ratings[_current_company], cost.base(), company->money.base(), debt.base());
}

/** Build only the complete freshly admitted native route, preserving all old freight and local passenger service. */
static void BuildSurfaceRegionalInfrastructure()
{
	SurfaceRegionalLayout layout; layout.Prepare(); const auto budget = QuerySurfaceRegionalBudget(layout.invoice);
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	if (budget.credit > 0) PaySurfaceRegional<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, budget.credit);
	// Secure actual accepting catchments before tree clearing can change civic
	// admission. Every purchase still needs its own fresh native command quote.
	for (uint index = 1; index < 2; ++index) {
		PaySurfaceRegional<Commands::BuildRoadStop>(layout.stops[index], layout.stops[index], uint8_t{1}, uint8_t{1}, RoadStopType::Bus,
			true, layout.directions[index], ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, NEW_STATION, true);
		const auto station = GetStationIndex(layout.stops[index]); REQUIRE(Station::Get(station)->owner == _current_company);
		PaySurfaceRegional<Commands::RenameStation>(TileIndex{0}, station, fmt::format("Surface Regional town {}", index));
		fmt::print("SURFACE_REGIONAL_STATION town={} station={} tile={}\n", index, station.base(), layout.stops[index].base());
	}
	// Pay each complete admitted shape once; no raw map cell or invented route
	// owner may replace native construction or erase protected existing traffic.
	for (const auto &[tile, bits] : layout.roads) PaySurfaceRegional<Commands::BuildRoad>(tile, tile, bits, ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid());
	REQUIRE(company->money == cash + budget.credit - layout.invoice); REQUIRE(company->current_loan == debt + budget.credit);
	REQUIRE(company->money - budget.unit * 3 >= 150000);
	fmt::print("SURFACE_REGIONAL_INFRASTRUCTURE cost={} credit={} cash={} debt={} roads={} reused_depot=2984\n", layout.invoice.base(), budget.credit.base(), company->money.base(), company->current_loan.base(), layout.roads.size());
}

/** @param town Actual source-town role. @return The unique originally purchased regional station with that public name. */
static StationID FindSurfaceRegionalStation(uint town)
{
	if (town == 0) { REQUIRE(Station::Get(StationID{17})->owner == _current_company); return StationID{17}; }
	StationID result = StationID::Invalid();
	for (const auto *station : Station::Iterate()) {
		if (station->name != fmt::format("Surface Regional town {}", town)) continue;
		REQUIRE(result == StationID::Invalid()); REQUIRE(station->owner == _current_company); result = station->index;
	}
	REQUIRE(result != StationID::Invalid()); return result;
}

/** Buy three admitted empty ordinary buses and install real commercial two-stop orders through original commands. */
static void BuySurfaceRegionalBuses()
{
	const auto first = FindSurfaceRegionalStation(0), last = FindSurfaceRegionalStation(1); REQUIRE(first != last);
	const auto budget = QuerySurfaceRegionalBudget(Money{0});
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	if (budget.credit > 0) PaySurfaceRegional<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, budget.credit);
	const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"});
	// Native movement, loading and delivery alone produce passenger income;
	// a purchased empty bus contributes neither a passenger packet nor a fare.
	for (uint index = 0; index < 3; ++index) {
		const auto paid = PaySurfaceRegional<Commands::BuildVehicle>(SurfaceRegionalLayout::depot, SurfaceRegionalLayout::depot, EngineID{119}, false, passenger, ClientID::Server);
		const auto *bus = RoadVehicle::Get(std::get<1>(paid)); REQUIRE(bus->IsStoppedInDepot()); REQUIRE(bus->cargo.TotalCount() == 0); REQUIRE(bus->cargo_cap == 40);
		PaySurfaceRegional<Commands::RenameVehicle>(TileIndex{0}, bus->index, fmt::format("Surface Regional passenger {}", index + 1));
		for (uint stop = 0; stop < 2; ++stop) {
			Order order; order.MakeGoToStation(stop == 0 ? first : last); order.SetStopLocation(OrderStopLocation::FarEnd);
			PaySurfaceRegional<Commands::InsertOrder>(TileIndex{0}, bus->index, VehicleOrderID{static_cast<uint8_t>(stop)}, order);
		}
		PaySurfaceRegional<Commands::StartStopVehicle>(TileIndex{0}, bus->index, true);
		fmt::print("SURFACE_REGIONAL_BUS vehicle={} model=119 capacity=40 first={} last={} orders={} empty=true\n", bus->index.base(), first.base(), last.base(), bus->GetNumOrders());
	}
	REQUIRE(company->money == cash + budget.credit - budget.unit * 3); REQUIRE(company->current_loan == debt + budget.credit); REQUIRE(company->money >= 150000);
	fmt::print("SURFACE_REGIONAL_PURCHASE cost={} credit={} cash={} debt={} buses=3\n", (budget.unit * 3).base(), budget.credit.base(), company->money.base(), company->current_loan.base());
}

/** @param limit Actual short or annual native tick budget. Observe new-service fractional profit and original company books. */
static void EarnSurfaceRegionalService(uint32_t limit)
{
	std::map<VehicleID, Money> previous, profit; auto year = TimerGameEconomy::year;
	// Include every active useful service in the life gate, while isolating
	// the newly admitted regional buses for measured marginal profitability.
	for (const auto *vehicle : Vehicle::Iterate()) {
		const auto &name = vehicle->name;
		if (name.starts_with("Surface Income ") || name.starts_with("Surface Launch passenger ") || name.starts_with("Surface Regional passenger ") ||
			name == "Surface ore delivery" || name == "Surface Steel delivery") { // Every current commercial service must survive the measured annual interval.
			REQUIRE(vehicle->owner == _current_company); REQUIRE(vehicle->age.base() + 366 < vehicle->max_age.base());
		}
		if (!name.starts_with("Surface Regional passenger ")) continue;
		REQUIRE(vehicle->type == VehicleType::Road); REQUIRE(vehicle->GetNumOrders() == 2);
		previous.emplace(vehicle->index, vehicle->profit_this_year); profit.emplace(vehicle->index, Money{0});
	}
	REQUIRE(previous.size() == 3); uint64_t observations = 0; const auto initial = TimerGameTick::counter;
	/** @return False while recording actual native vehicle profits; the bounded genuine tick limit owns completion. */
	const auto observe = [&] {
		const auto current_year = TimerGameEconomy::year;
		// Retain fractional-money changes before native year rollover discards
		// older profits. Observation never changes company cash or vehicle books.
		for (auto &[id, last] : previous) {
			const auto *bus = Vehicle::Get(id); REQUIRE(bus->owner == _current_company);
			profit[id] += current_year == year ? bus->profit_this_year - last : bus->profit_last_year - last + bus->profit_this_year;
			last = bus->profit_this_year;
		}
		year = current_year;
		if (++observations % 5000 == 0) fmt::print("SURFACE_REGIONAL_PROGRESS earned={} observations={} pause={}\n", TimerGameTick::counter - initial, observations, _pause_mode.base());
		return false;
	};
	EarnSurfaceEntryWork("SURFACE_REGIONAL", limit, observe); Money total{0};
	for (const auto &[id, value] : profit) {
		total += value; fmt::print("SURFACE_REGIONAL_VEHICLE_PROFIT vehicle={} fraction=256 amount={}\n", id.base(), value.base());
	}
	fmt::print("SURFACE_REGIONAL_PROFIT buses=3 fraction=256 amount={}\n", total.base());
}

/** Exercise only actual loaded-world native quotes, paid construction or bounded useful service. */
TEST_CASE("Regional passengers use actual paid roads and native fares", "[.surface-entry-regional]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_REGIONAL_PHASE"); REQUIRE(selected != nullptr); const std::string phase{selected};
	// Explicit phases separate admission, infrastructure, empty purchase and
	// earned service; no restore or quote can silently execute another phase.
	REQUIRE((phase == "civic-query" || phase == "civic" || phase == "query" || phase == "infrastructure" || phase == "buy" || phase == "earn" || phase == "short"));
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-regional"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve original failures through shutdown without compensating cash, passengers, roads or work. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceRegional(paths.saves / ("surface-regional-before-" + phase + "-440.sav"));
			if (phase == "civic-query") PrepareSurfaceRegionalCivic(false);
			else if (phase == "civic") PrepareSurfaceRegionalCivic(true);
			else if (phase == "query") { SurfaceRegionalLayout layout; layout.Prepare(); QuerySurfaceRegionalBudget(layout.invoice); }
			else if (phase == "infrastructure") BuildSurfaceRegionalInfrastructure();
			else if (phase == "buy") BuySurfaceRegionalBuses();
			else EarnSurfaceRegionalService(phase == "earn" ? 27000 : 3000);
			SaveSurfaceRegional(paths.saves / ("surface-regional-" + phase + "-440.sav"));
			fmt::print("SURFACE_REGIONAL_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_regional_observer, &inspect); IConsole::CmdRegister("surface_entry_regional", ObserveSurfaceRegional);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
