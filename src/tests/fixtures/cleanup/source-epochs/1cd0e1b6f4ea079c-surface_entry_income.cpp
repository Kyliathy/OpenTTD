/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_income.cpp Actual commercial purchases and native earnings in the genuine Surface world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BI.2.
 * Original commands and StateGameLoop own every payment, packet, work unit and
 * vehicle position. Period deltas below only observe the original company books.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../industry.h"
#include "../industry_cmd.h"
#include "../industrytype.h"
#include "../layer_industry_processor.h"
#include "../multilayer_recipe.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../vehicle_cmd.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../underground_vehicle.h"
#include "../station_base.h"
#include "../station_func.h"
#include "../town.h"
#include "../misc_cmd.h"
#include "../depot_base.h"
#include "../order_cmd.h"
#include "../economy_func.h"
#include "../openttd.h"
#include "../timer/timer_game_tick.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_fuel_rail_layout.h"
#include <thread>
#include "../safeguards.h"

static std::function<void()> *surface_income_observer = nullptr; ///< Invocation callback, never a money or material owner.
static const std::array<std::string, 3> surface_income_names{"Surface Income farm", "Surface Income goods", "Surface Income coal"};

/** @param args Parameter-free console arguments. @return True after invoking the loaded native observer. */
static bool ObserveSurfaceIncome(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_income_observer != nullptr); (*surface_income_observer)(); return true;
}

/** @param path Fresh invocation-owned destination. Save original owners without earning work. */
static void SaveSurfaceIncome(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Query one complete original command without changing any native owner.
 * @tparam Tcmd Original command identity. @tparam Targs Original complete argument types.
 * @param tile Actual tiled action, or zero for coordinate-free commands.
 * @param args Complete original arguments. @return Original quote/refusal.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceIncome(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** Freshly quote and execute one original synchronized purchase or management action.
 * @tparam Tcmd Original command identity. @tparam Targs Original complete argument types.
 * @param tile Actual tiled action, or zero for coordinate-free commands.
 * @param args Complete native arguments. @return Original settled command result.
 */
template <Commands Tcmd, typename... Targs>
static auto PaySurfaceIncome(TileIndex tile, Targs... args)
{
	auto quote = QuoteSurfaceIncome<Tcmd>(tile, args...);
	INFO("income quote=" << GetCommandName(Tcmd) << " tile=" << tile.base() << " error=" << ExtractCommandCost(quote).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(quote).Succeeded());
	auto result = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(args...));
	REQUIRE(ExtractCommandCost(result).Succeeded()); REQUIRE(ExtractCommandCost(result).GetCost() == ExtractCommandCost(quote).GetCost());
	fmt::print("SURFACE_INCOME_INVOICE command={} tile={} cost={}\n", GetCommandName(Tcmd), tile.base(), ExtractCommandCost(result).GetCost().base());
	return result;
}

/** @param role Actual named paid route. @return The sole original front engine with that name. */
static const Train *FindSurfaceIncomeTrain(uint role)
{
	const Train *result = nullptr;
	// Native names identify the paid front after free pool slots are reused;
	// neither a historical vehicle ID nor an ambient effect may supply it.
	for (const auto *train : Train::Iterate()) {
		if (train->name != surface_income_names[role]) continue;
		REQUIRE(result == nullptr); REQUIRE(train->IsFrontEngine()); REQUIRE(train->owner == _current_company); result = train;
	}
	REQUIRE(result != nullptr); return result;
}

/** @param role Actual commercial route. @param line Freshly purchased physical line. @return Actual empty fleet invoice. */
static Money BuySurfaceIncomeTrain(uint role, const SurfaceFuelRailLine &line)
{
	const auto depot = line.source.depot;
	const auto cargo = GetCargoTypeByLabel(role == 0 ? CargoLabel{"GRAI"} : role == 1 ? CargoLabel{"GOOD"} : CargoLabel{"COAL"});
	auto bought = PaySurfaceIncome<Commands::BuildVehicle>(depot, depot, EngineID{22}, false, cargo, ClientID::Server);
	auto *front = Train::Get(std::get<1>(bought)); Money invoice = std::get<0>(bought).GetCost();
	const std::vector<std::pair<EngineID, uint>> wagons = role == 0 ? std::vector<std::pair<EngineID, uint>>{{EngineID{31}, 6}, {EngineID{33}, 6}} :
		role == 1 ? std::vector<std::pair<EngineID, uint>>{{EngineID{32}, 12}} : std::vector<std::pair<EngineID, uint>>{{GetUndergroundEngine(5), 4}};
	// Every empty wagon is genuinely bought and normally coupled. Native
	// automatic attachment is accepted only when it joined this actual front.
	for (const auto &[engine, count] : wagons) {
		for (uint index = 0; index < count; ++index) {
			auto wagon = PaySurfaceIncome<Commands::BuildVehicle>(depot, depot, engine, false, INVALID_CARGO, ClientID::Server);
			invoice += std::get<0>(wagon).GetCost(); auto *part = Train::Get(std::get<1>(wagon));
			if (part->First() != front) PaySurfaceIncome<Commands::MoveRailVehicle>(TileIndex{0}, part->index, front->Last()->index, false);
		}
	}
	REQUIRE(front->IsStoppedInDepot()); REQUIRE(front->gcache.cached_total_length <= line.source.length * TILE_SIZE);
	CargoArray capacity{};
	for (const auto *part = front; part != nullptr; part = part->Next()) { REQUIRE(part->cargo.TotalCount() == 0); capacity[part->cargo_type] += part->cargo_cap; }
	PaySurfaceIncome<Commands::RenameVehicle>(TileIndex{0}, front->index, surface_income_names[role]);
	const auto source = GetStationIndex(line.source.anchor), target = GetStationIndex(line.target.anchor);
	Order collect; collect.MakeGoToStation(source); collect.SetLoadType(role == 0 ? OrderLoadType::FullLoadAny : OrderLoadType::FullLoad);
	collect.SetUnloadType(OrderUnloadType::NoUnload); collect.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceIncome<Commands::InsertOrder>(TileIndex{0}, front->index, VehicleOrderID{0}, collect);
	Order deliver; deliver.MakeGoToStation(target); deliver.SetLoadType(OrderLoadType::NoLoad); deliver.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceIncome<Commands::InsertOrder>(TileIndex{0}, front->index, VehicleOrderID{1}, deliver);
	PaySurfaceIncome<Commands::StartStopVehicle>(TileIndex{0}, front->index, true);
	fmt::print("SURFACE_INCOME_TRAIN_PAID role={} head={} source={} target={} depot={} length={} invoice={} empty=true\n", role, front->index.base(),
		source.base(), target.base(), depot.base(), front->gcache.cached_total_length, invoice.base());
	for (CargoType type = CargoType{0}; type < NUM_CARGO; ++type) if (capacity[type] != 0) fmt::print("SURFACE_INCOME_TRAIN_CAPACITY role={} label={} amount={}\n", role, CargoSpec::Get(type)->label.AsString(), capacity[type]);
	return invoice;
}

/** Execute the accepted complete arrangement using only original invoices and normal empty purchases. */
static void FundSurfaceIncome()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == 181985); REQUIRE(company->current_loan == 4650000);
	const auto cash = company->money; SurfaceFuelRailLayout layout; layout.Prepare(); REQUIRE(layout.invoice == 44861);
	const auto *old = Train::Get(VehicleID{18}); REQUIRE(old->IsStoppedInDepot()); REQUIRE(old->tile == TileIndex{1373});
	for (const auto *part = old; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
	const auto sold = PaySurfaceIncome<Commands::SellVehicle>(TileIndex{0}, old->index, true, false, ClientID::Server); REQUIRE(sold.GetCost() == -145769);
	Money infrastructure{0};
	// Fund the town Goods receiver first, before railway tree clearing can
	// reduce authority ratings. All remaining complete platforms precede track.
	for (const std::pair<uint, uint> position : {std::pair<uint, uint>{1, 1}, {1, 0}, {0, 1}, {2, 0}, {0, 0}, {2, 1}}) {
		const auto &line = layout.lines[position.first]; const auto &terminal = position.second == 0 ? line.source : line.target;
		infrastructure += PaySurfaceIncome<Commands::BuildRailStation>(terminal.anchor, terminal.anchor, RAILTYPE_RAIL, terminal.axis,
			uint8_t{1}, terminal.length, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, true).GetCost();
		fmt::print("SURFACE_INCOME_STATION role={} end={} id={} anchor={} owner={}\n", position.first, position.second,
			GetStationIndex(terminal.anchor).base(), terminal.anchor.base(), Station::GetByTile(terminal.anchor)->owner.base());
	}
	// Original depot/track commands revalidate actual adjacent new stations,
	// terrain and existing roads. An earlier quote cannot waive a new refusal.
	for (const auto &line : layout.lines) {
		infrastructure += PaySurfaceIncome<Commands::BuildRailDepot>(line.source.depot, line.source.depot, RAILTYPE_RAIL, line.source.outward).GetCost();
		for (const auto &[tile, track] : line.tracks) infrastructure += PaySurfaceIncome<Commands::BuildRail>(tile, tile, RAILTYPE_RAIL, track, false).GetCost();
	}
	Money bridge_savings{0};
	for (const auto &line : layout.lines) for (const auto &bridge : line.bridges) {
		const auto paid = PaySurfaceIncome<Commands::BuildBridge>(bridge.far, bridge.far, bridge.near, TransportType::Rail, bridge.type, RAILTYPE_RAIL, INVALID_ROADTYPE);
		// Earlier paid plain track already cleared two trees below this span.
		// The original bridge requery must honor that real shared clearing;
		// the complete pre-purchase quote remains a conservative capital bound.
		REQUIRE(paid.GetCost() <= bridge.invoice); bridge_savings += bridge.invoice - paid.GetCost(); infrastructure += paid.GetCost();
	}
	REQUIRE(bridge_savings == 60); REQUIRE(infrastructure + bridge_savings == layout.invoice); Money fleet{0};
	for (uint role = 0; role < layout.lines.size(); ++role) fleet += BuySurfaceIncomeTrain(role, layout.lines[role]);
	REQUIRE(fleet == 126807); REQUIRE(company->money == cash - sold.GetCost() - infrastructure - fleet);
	REQUIRE(company->money == 156146); REQUIRE(company->current_loan == 4650000);
	fmt::print("SURFACE_INCOME_FUNDED sale={} infrastructure={} vehicles={} cash={} debt={} supplied_stock=false\n", -sold.GetCost().base(), infrastructure.base(),
		fleet.base(), company->money.base(), company->current_loan.base());
}

/** @return Fresh whole PCHM, actual PROP tanker and declared operating buffer, without installing credit. */
static Money SurfaceIncomeFuelRequirement()
{
	const TileIndex site{1639}, depot{2443};
	// CHEM is this facility definition's primary recipe; PCHM is its selectable
	// second recipe, not another independently fundable industry definition.
	auto factory = QuoteSurfaceIncome<Commands::BuildIndustry>(site, site, GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}), uint32_t{0}, true, uint32_t{123});
	REQUIRE(factory.Succeeded());
	std::optional<Money> tanker; EngineID selected = EngineID::Invalid(); uint capacity = 0;
	// Actual earned years can retire a purchase model. Requery the cheapest
	// currently available original PROP tanker, retaining its real whole refit
	// and capacity; an obsolete model's old quote cannot define affordability.
	for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
		if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
		auto quote = QuoteSurfaceIncome<Commands::BuildVehicle>(depot, depot, engine->index, false, GetCargoTypeByLabel(CargoLabel{"PROP"}), ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		if (std::get<2>(quote) < 20) continue;
		if (tanker && *tanker <= std::get<0>(quote).GetCost()) continue;
		tanker = std::get<0>(quote).GetCost(); selected = engine->index; capacity = std::get<2>(quote);
	}
	REQUIRE(tanker.has_value());
	fmt::print("SURFACE_INCOME_FUEL_BUDGET factory={} engine={} tanker={} capacity={} buffer=100000 requirement={}\n", factory.GetCost().base(), selected.base(),
		tanker->base(), capacity, (factory.GetCost() + *tanker + Money{100000}).base());
	return factory.GetCost() + *tanker + Money{100000};
}

/** Earn a bounded native economic period and observe original rolling books without treating them as lifetime balances. */
static void EarnSurfaceIncome()
{
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	const auto initial = TimerGameTick::counter; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(85);
	Expenses accrued{}; CargoArray delivered{}; Money income{0}; uint year_rolls = 0, quarter_rolls = 0;
	for (uint role = 0; role < surface_income_names.size(); ++role) FindSurfaceIncomeTrain(role);
	if (_pause_mode.Test(PauseMode::Normal)) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
	bool affordable = false; Money requirement = SurfaceIncomeFuelRequirement();
	// Native work alone advances all trains, stock, production, station ratings
	// and payment. Every fiscal/quarter rollover is accounted at the actual tick
	// before its original historical window can drop out of a later save.
	while (TimerGameTick::counter - initial < 27000) {
		REQUIRE(std::chrono::steady_clock::now() < deadline);
		const auto year = TimerGameEconomy::year; const auto month = TimerGameEconomy::month;
		const auto expenses = company->yearly_expenses[0]; const auto quarter = company->cur_economy;
		StateGameLoop();
		const bool rolled_year = TimerGameEconomy::year != year;
		const bool rolled_quarter = rolled_year || TimerGameEconomy::month / 3 != month / 3;
		year_rolls += rolled_year; quarter_rolls += rolled_quarter;
		for (uint index = 0; index < to_underlying(ExpensesType::End); ++index) {
			const auto type = static_cast<ExpensesType>(index);
			accrued[type] += company->yearly_expenses[rolled_year ? 1 : 0][type] - expenses[type];
			if (rolled_year) accrued[type] += company->yearly_expenses[0][type];
		}
		const auto &completed = rolled_quarter ? company->old_economy[0] : company->cur_economy;
		income += completed.income - quarter.income;
		if (rolled_quarter) income += company->cur_economy.income;
		for (CargoType type = CargoType{0}; type < NUM_CARGO; ++type) {
			delivered[type] += completed.delivered_cargo[type] - quarter.delivered_cargo[type];
			if (rolled_quarter) delivered[type] += company->cur_economy.delivered_cargo[type];
		}
		// Fresh affordability uses original cash plus genuinely available credit,
		// never the conditional tariff forecast or unsold vehicle book values.
		if ((TimerGameTick::counter - initial) % 74 == 0 && company->money + company->GetMaxLoan() - company->current_loan >= requirement) {
			requirement = SurfaceIncomeFuelRequirement(); affordable = company->money + company->GetMaxLoan() - company->current_loan >= requirement;
			if (affordable) break;
		}
		if (company->money < 30000) break;
		if (_pause_mode.Any()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	Money expense{0};
	for (uint index = 0; index < to_underlying(ExpensesType::End); ++index) {
		const auto type = static_cast<ExpensesType>(index); expense += accrued[type];
		fmt::print("SURFACE_INCOME_EXPENSE type={} amount={}\n", index, accrued[type].base());
	}
	REQUIRE(company->current_loan == debt); REQUIRE(company->money == cash - expense);
	fmt::print("SURFACE_INCOME_EARNED earned={} cash_before={} cash={} debt={} income={} net_expense={} year_rolls={} quarter_rolls={} requirement={} affordable={} year={} month={}\n",
		TimerGameTick::counter - initial, cash.base(), company->money.base(), debt.base(), income.base(), expense.base(), year_rolls, quarter_rolls,
		requirement.base(), affordable, TimerGameEconomy::year.base(), TimerGameEconomy::month);
	for (CargoType type = CargoType{0}; type < NUM_CARGO; ++type) if (delivered[type] != 0) fmt::print("SURFACE_INCOME_DELIVERED label={} amount={}\n", CargoSpec::Get(type)->label.AsString(), delivered[type]);
	for (uint role = 0; role < surface_income_names.size(); ++role) {
		const auto *train = FindSurfaceIncomeTrain(role);
		fmt::print("SURFACE_INCOME_POSITION role={} head={} tile={} stopped={} speed={} last_station={}\n", role, train->index.base(), train->tile.base(),
			train->vehstatus.Test(VehState::Stopped), train->cur_speed, train->last_station_visited.base());
	}
}

/** Actual owned commerce funds the next chemical factory without supplied material, work or capital. */
TEST_CASE("Authentic Surface commerce buys original trains and earns ordinary Propellant capital", "[.surface-entry-income]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *mode = std::getenv("OTTD_SURFACE_INCOME_PHASE"); REQUIRE(mode != nullptr); const std::string phase{mode};
	REQUIRE((phase == "fund" || phase == "earn")); // Only the explicitly selected purchase or real work boundary may run.
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-income"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve original failures through native shutdown; no compensating inventory, work or credit is supplied. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceIncome(paths.saves / ("surface-income-before-" + phase + "-440.sav"));
			if (phase == "fund") FundSurfaceIncome(); else EarnSurfaceIncome();
			SaveSurfaceIncome(paths.saves / ("surface-income-" + phase + "-440.sav"));
			fmt::print("SURFACE_INCOME_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_income_observer, &inspect); IConsole::CmdRegister("surface_entry_income", ObserveSurfaceIncome);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
