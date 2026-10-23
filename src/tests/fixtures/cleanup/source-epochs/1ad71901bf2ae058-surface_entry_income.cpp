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
#include "../rail_map.h"
#include "../tunnelbridge_map.h"
#include "../vehicle_cmd.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../underground_vehicle.h"
#include "../station_base.h"
#include "../station_func.h"
#include "../town.h"
#include "../town_cmd.h"
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
#include <sstream>
#include "../safeguards.h"

static std::function<void()> *surface_income_observer = nullptr; ///< Invocation callback, never a money or material owner.
static const std::array<std::string, 3> surface_income_names{"Surface Income farm", "Surface Income goods", "Surface Income coal"};
static const std::array<TileIndex, 3> surface_income_depots{TileIndex{1638}, TileIndex{3224}, TileIndex{3161}};
static const std::array<TileIndex, 3> surface_income_receivers{TileIndex{3280}, TileIndex{2414}, TileIndex{520}};

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

/** Earn a bounded native economic period and observe original rolling books without treating them as lifetime balances.
 * @param settle_coal Stop at the actual ordered empty Coal depot arrival instead of a whole annual period.
 * @param settle_fleet Stop only when all three actual commercial trains have reached their ordered depots.
 */
static void EarnSurfaceIncome(bool settle_coal = false, bool settle_fleet = false)
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
		// A service recall earns only the movement needed for the actual
		// ordered empty depot arrival; ordinary annual periods keep running.
		if (settle_coal && FindSurfaceIncomeTrain(2)->IsStoppedInDepot()) break;
		if (settle_fleet) {
			bool home = true;
			// A fleet service boundary needs every paid train home, rather than
			// treating the first arrival as permission to replace moving assets.
			for (uint role = 0; role < surface_income_names.size(); ++role) home &= FindSurfaceIncomeTrain(role)->IsStoppedInDepot();
			if (home) break;
		}
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
	if (settle_coal) {
		const auto *train = FindSurfaceIncomeTrain(2); REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->tile == TileIndex{3161});
		for (const auto *part = train; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
	}
	if (settle_fleet) {
		// Original empty depot admission protects every useful wagon and packet
		// before a later retirement command can remove either locomotive part.
		for (uint role = 0; role < surface_income_names.size(); ++role) {
			const auto *train = FindSurfaceIncomeTrain(role);
			REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->tile == surface_income_depots[role]);
			for (const auto *part = train; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
		}
	}
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

/** Whole original Coal wagon purchase candidate; no live stock, money or vehicle ownership. */
struct SurfaceIncomeCoalQuote {
	EngineID engine = EngineID::Invalid(); ///< Actually available ordinary conventional Coal wagon.
	Money invoice{0}; ///< Four native whole purchases, excluding unsold book values.
	uint capacity = 0; ///< Actual per-wagon purchase/refit capacity.
};

/** @return Admitted original Coal wagons that remove the actual mineral-wagon speed limit. */
static SurfaceIncomeCoalQuote QuoteSurfaceIncomeCoal()
{
	const TileIndex depot{3161}; const auto coal = GetCargoTypeByLabel(CargoLabel{"COAL"}); SurfaceIncomeCoalQuote result;
	// Whole native purchase quotes at the actual paid depot establish both
	// availability and capacity. Only ordinary Coal wagons with no80speed
	// restriction can improve the existing real200speed locomotive's service.
	for (const auto *engine : Engine::IterateType(VehicleType::Train)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Train, _current_company)) continue;
		if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
		if (engine->VehInfo<RailVehicleInfo>().railveh_type != RailVehicleType::Wagon) continue;
		if (engine->GetDefaultCargoType() != coal) continue;
		const auto speed = engine->GetDisplayMaxSpeed();
		if (speed != 0 && speed < 200) continue; // Slow wagons would retain the same costly service bottleneck.
		auto quote = QuoteSurfaceIncome<Commands::BuildVehicle>(depot, depot, engine->index, false, coal, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		fmt::print("SURFACE_INCOME_COAL_CANDIDATE engine={} cost={} capacity={} speed={} count=4\n", engine->index.base(), std::get<0>(quote).GetCost().base(), std::get<2>(quote), speed);
		if (result.engine != EngineID::Invalid()) continue;
		result = {engine->index, 4 * std::get<0>(quote).GetCost(), std::get<2>(quote)};
	}
	REQUIRE(result.engine != EngineID::Invalid()); REQUIRE(result.capacity >= 30);
	REQUIRE(Company::Get(_current_company)->money >= result.invoice + Money{100000});
	fmt::print("SURFACE_INCOME_COAL_QUOTE engine={} invoice={} capacity={} count=4 old_speed={} depot=3161 cash={} debt={}\n", result.engine.base(), result.invoice.base(), result.capacity,
		FindSurfaceIncomeTrain(2)->GetDisplayMaxSpeed(), Company::Get(_current_company)->money.base(), Company::Get(_current_company)->current_loan.base());
	return result;
}

/** @param train Actual paid Coal front. Replace only its real orders, with no movement or cargo transfer. */
static void RecallSurfaceIncomeCoal(const Train &train)
{
	// Existing Coal must be accepted at the real power station before its
	// empty wagons enter service. Neither a direct sale nor a pose change can
	// substitute for this original delivery and actual halted-depot admission.
	while (train.GetNumOrders() != 0) PaySurfaceIncome<Commands::DeleteOrder>(TileIndex{0}, train.index, VehicleOrderID{0});
	Order deliver; deliver.MakeGoToStation(GetStationIndex(TileIndex{520})); deliver.SetLoadType(OrderLoadType::NoLoad); deliver.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceIncome<Commands::InsertOrder>(TileIndex{0}, train.index, VehicleOrderID{0}, deliver);
	Order home; home.MakeGoToDepot(Depot::GetByTile(TileIndex{3161})->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
	PaySurfaceIncome<Commands::InsertOrder>(TileIndex{0}, train.index, VehicleOrderID{1}, home);
	fmt::print("SURFACE_INCOME_COAL_RECALL head={} receiver={} depot=3161 supplied_pose=false\n", train.index.base(), GetStationIndex(TileIndex{520}).base());
}

/** Normally sell only the four empty mineral wagons and buy/couple the admitted faster original Coal wagons. */
static void RewagonSurfaceIncomeCoal()
{
	const auto quote = QuoteSurfaceIncomeCoal(); const auto head_id = FindSurfaceIncomeTrain(2)->index;
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	const auto *train = Train::Get(head_id); REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->tile == TileIndex{3161});
	std::vector<VehicleID> old;
	for (const auto *part = train; part != nullptr; part = part->Next()) {
		REQUIRE(part->cargo.TotalCount() == 0);
		if (part->engine_type == GetUndergroundEngine(5)) old.push_back(part->index);
	}
	REQUIRE(old.size() == 4); Money invoice{0};
	for (const auto id : old) invoice += PaySurfaceIncome<Commands::SellVehicle>(TileIndex{0}, id, false, false, ClientID::Server).GetCost();
	// Keep the same paid front, rear, route and three-tile platforms. New
	// original wagons are bought empty and normally attached after native sales.
	for (uint index = 0; index < 4; ++index) {
		auto wagon = PaySurfaceIncome<Commands::BuildVehicle>(TileIndex{3161}, TileIndex{3161}, quote.engine, false, GetCargoTypeByLabel(CargoLabel{"COAL"}), ClientID::Server);
		invoice += std::get<0>(wagon).GetCost(); auto *part = Train::Get(std::get<1>(wagon));
		if (part->First() != Train::Get(head_id)) PaySurfaceIncome<Commands::MoveRailVehicle>(TileIndex{0}, part->index, Train::Get(head_id)->Last()->index, false);
	}
	train = Train::Get(head_id); REQUIRE(train->GetDisplayMaxSpeed() == 200); REQUIRE(train->gcache.cached_total_length <= 3 * TILE_SIZE);
	while (train->GetNumOrders() != 0) PaySurfaceIncome<Commands::DeleteOrder>(TileIndex{0}, head_id, VehicleOrderID{0});
	Order collect; collect.MakeGoToStation(GetStationIndex(TileIndex{3158})); collect.SetLoadType(OrderLoadType::FullLoad); collect.SetUnloadType(OrderUnloadType::NoUnload); collect.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceIncome<Commands::InsertOrder>(TileIndex{0}, head_id, VehicleOrderID{0}, collect);
	Order deliver; deliver.MakeGoToStation(GetStationIndex(TileIndex{520})); deliver.SetLoadType(OrderLoadType::NoLoad); deliver.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceIncome<Commands::InsertOrder>(TileIndex{0}, head_id, VehicleOrderID{1}, deliver);
	PaySurfaceIncome<Commands::StartStopVehicle>(TileIndex{0}, head_id, true);
	REQUIRE(company->money == cash - invoice); REQUIRE(company->current_loan == debt);
	fmt::print("SURFACE_INCOME_COAL_REWAGON head={} engine={} invoice={} cash={} debt={} speed={} length={} supplied_stock=false\n", head_id.base(), quote.engine.base(),
		invoice.base(), company->money.base(), debt.base(), train->GetDisplayMaxSpeed(), train->gcache.cached_total_length);
}

/** Repay only genuine spare cash, preserving exact spendable capital through matching original bank headroom. */
static void RepaySurfaceIncomeLoan()
{
	const auto *company = Company::Get(_current_company);
	const auto cash = company->money, debt = company->current_loan; const auto ceiling = company->GetMaxLoan();
	const auto expenses = company->yearly_expenses; const auto quarter = company->cur_economy;
	const Money buffer{100000}; REQUIRE(cash >= buffer + Money{10000});
	const Money amount = std::min(debt, Money{((cash - buffer).base() / 10000) * 10000}); REQUIRE(amount > 0);
	const auto before = cash + ceiling - debt;
	// Native DecreaseLoan debits cash and debt together, without a transport
	// expense or grant. Available PCHM capital therefore stays exact while
	// subsequent actual interest falls; the global/company ceiling is untouched.
	PaySurfaceIncome<Commands::DecreaseLoan>(TileIndex{0}, LoanCommand::Amount, amount);
	REQUIRE(company->money == cash - amount); REQUIRE(company->current_loan == debt - amount);
	REQUIRE(company->GetMaxLoan() == ceiling); REQUIRE(company->money + ceiling - company->current_loan == before);
	REQUIRE(company->yearly_expenses == expenses); REQUIRE(company->cur_economy.income == quarter.income); REQUIRE(company->cur_economy.expenses == quarter.expenses);
	fmt::print("SURFACE_INCOME_REPAY amount={} cash_before={} debt_before={} cash={} debt={} ceiling={} spendable={} buffer=100000 annual_interest_reduction={}\n", amount.base(),
		cash.base(), debt.base(), company->money.base(), company->current_loan.base(), ceiling.base(), before.base(), (amount * _economy.interest_rate / 100).base());
}

/** Query actual source-station town associations and complete original permanent service costs without spending.
 * @param require_admitted Require each whole original service quote after legitimate sufficient treasury exists.
 * @return Whole actual cost for unique source towns that do not already have this company's statue.
 */
static Money QuerySurfaceIncomeTownService(bool require_admitted = false)
{
	const auto *company = Company::Get(_current_company); std::set<TownID> towns;
	// The station's actual town owns the permanent service bonus. Industry
	// town assignment or geographic intuition cannot justify buying another
	// town's statue or count its effect on the genuine commercial routes.
	for (const auto station : {StationID{15}, StationID{12}, StationID{14}}) {
		const auto *source = Station::Get(station); REQUIRE(source->owner == _current_company); towns.insert(source->town->index);
		fmt::print("SURFACE_INCOME_SERVICE_SOURCE station={} town={} population={} statue={}\n", station.base(), source->town->index.base(),
			source->town->cache.population, source->town->statues.Test(_current_company));
	}
	Money total{0}; bool admitted = true;
	// One paid town statue serves every original source assigned to that town;
	// deduplicating the actual town owners avoids charging one statue per line.
	for (const auto town : towns) {
		const auto *owner = Town::Get(town); const auto cost = _price[Price::TownAction] * GetTownActionCost(TownAction::BuildStatue) >> 8;
		auto quote = QuoteSurfaceIncome<Commands::TownAction>(TileIndex{0}, town, TownAction::BuildStatue);
		fmt::print("SURFACE_INCOME_SERVICE_QUOTE town={} existing={} admitted={} native_cost={} quote_cost={} rating_bonus=26\n", town.base(),
			owner->statues.Test(_current_company), quote.Succeeded(), cost.base(), quote.GetCost().base());
		if (owner->statues.Test(_current_company)) continue;
		if (require_admitted) REQUIRE(quote.Succeeded());
		total += cost; admitted = admitted && quote.Succeeded();
	}
	const auto shortfall = std::max(Money{0}, total + Money{100000} - company->money);
	const Money credit{((shortfall.base() + 9999) / 10000) * 10000}; bool bank = true;
	if (credit > 0) { auto quote = QuoteSurfaceIncome<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit); bank = quote.Succeeded(); }
	fmt::print("SURFACE_INCOME_SERVICE_CAPITAL towns={} cost={} admitted={} credit={} bank_admitted={} cash={} debt={} spendable_after_costs={} buffer=100000\n",
		towns.size(), total.base(), admitted, credit.base(), bank, company->money.base(), company->current_loan.base(),
		(company->money + company->GetMaxLoan() - company->current_loan - total).base());
	return total;
}

/** Obtain only the original matching bank credit needed for the quoted single-town service and operating buffer. */
static void CreditSurfaceIncomeTownService()
{
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	const auto ceiling = company->GetMaxLoan(); const auto expenses = company->yearly_expenses;
	const auto total = QuerySurfaceIncomeTownService(); const auto shortfall = total + Money{100000} - cash; REQUIRE(shortfall > 0);
	const Money amount{((shortfall.base() + 9999) / 10000) * 10000};
	PaySurfaceIncome<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, amount);
	REQUIRE(company->money == cash + amount); REQUIRE(company->current_loan == debt + amount); REQUIRE(company->GetMaxLoan() == ceiling);
	REQUIRE(company->money + ceiling - company->current_loan == cash + ceiling - debt); REQUIRE(company->yearly_expenses == expenses);
	// Matching credit is reversible capital settlement, not the paid service.
	// Every whole native placement/permission quote must now admit before a
	// later explicit purchase can consume any company money or terrain.
	REQUIRE(QuerySurfaceIncomeTownService(true) == total);
	fmt::print("SURFACE_INCOME_SERVICE_CREDIT amount={} cash_before={} debt_before={} cash={} debt={} ceiling={} spendable={}\n", amount.base(), cash.base(), debt.base(),
		company->money.base(), company->current_loan.base(), ceiling.base(), (cash + ceiling - debt).base());
}

/** Buy the admitted permanent service through the original town controller, without writing rating/statue bits. */
static void BuySurfaceIncomeTownService()
{
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	const auto total = QuerySurfaceIncomeTownService(true); std::set<TownID> towns; Money paid{0};
	for (const auto station : {StationID{15}, StationID{12}, StationID{14}}) towns.insert(Station::Get(station)->town->index);
	// Each native owner creates its actual town object/bonus once. No hidden
	// station-rating mutation, inventory, cargo or elapsed work is supplied.
	for (const auto town : towns) {
		if (Town::Get(town)->statues.Test(_current_company)) continue;
		paid += PaySurfaceIncome<Commands::TownAction>(TileIndex{0}, town, TownAction::BuildStatue).GetCost();
		REQUIRE(Town::Get(town)->statues.Test(_current_company));
	}
	REQUIRE(paid == total); REQUIRE(company->money == cash - paid); REQUIRE(company->current_loan == debt); REQUIRE(company->money >= 100000);
	fmt::print("SURFACE_INCOME_SERVICE_BOUGHT towns={} invoice={} cash={} debt={} supplied_stock=false\n", towns.size(), paid.base(), company->money.base(), debt.base());
}

/** @return Bounded actual previously purchased income railway selection, never another routing or map owner. */
static std::set<TileIndex> SurfaceIncomeRailSelection()
{
	const char *text = std::getenv("OTTD_SURFACE_INCOME_RAIL_TILES"); REQUIRE(text != nullptr);
	std::istringstream input{text}; uint32_t raw; std::set<TileIndex> result;
	// Coordinates select the original accepted paid infrastructure. Each real
	// current tile must still be owned railway in the canonical Surface layer;
	// no fixture may supply track, alter a tile or define saved routing.
	while (input >> raw) {
		const TileIndex tile{raw}; REQUIRE(MultiLayerMap::IsTileInLayer(tile, MapLayer::Surface));
		REQUIRE(IsTileOwner(tile, _current_company)); REQUIRE(GetTileRailType(tile) != INVALID_RAILTYPE);
		REQUIRE(result.insert(tile).second);
	}
	REQUIRE(input.eof()); REQUIRE(result.size() == 230); return result;
}

/** Quote currently available native heads, the actual owned conversion and useful legal Coal capacity before spending. */
static Money QuerySurfaceIncomeFleet()
{
	const auto *company = Company::Get(_current_company); const auto assets = SurfaceIncomeRailSelection(); Money conversion{0}; uint actions = 0;
	// A bridge's original converter handles both ends together. Select its
	// canonical first end once so the capital bound does not buy the pair twice.
	for (const auto tile : assets) {
		if (IsTileType(tile, TileType::TunnelBridge)) {
			if (tile > GetOtherBridgeEnd(tile)) continue;
		}
		auto quote = QuoteSurfaceIncome<Commands::ConvertRail>(tile, tile, tile, RAILTYPE_ELECTRIC, false);
		INFO("electric conversion tile=" << tile.base() << " error=" << quote.GetErrorMessage().base()); REQUIRE(quote.Succeeded());
		conversion += quote.GetCost(); ++actions;
		fmt::print("SURFACE_INCOME_FLEET_CONVERSION tile={} cost={}\n", tile.base(), quote.GetCost().base());
	}
	for (const auto *engine : Engine::IterateType(VehicleType::Train)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Train, _current_company)) continue;
		if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
		const auto &rail = engine->VehInfo<RailVehicleInfo>(); if (rail.railveh_type == RailVehicleType::Wagon) continue;
		auto quote = QuoteSurfaceIncome<Commands::BuildVehicle>(TileIndex{3161}, TileIndex{3161}, engine->index, false, INVALID_CARGO, ClientID::Server);
		fmt::print("SURFACE_INCOME_FLEET_MODEL engine={} kind={} railtypes={} cost={} running={} speed={} power={} capacity={} default_cargo={} reliability={} current_depot_admitted={}\n",
			engine->index.base(), to_underlying(rail.railveh_type), rail.railtypes.base(), engine->GetCost().base(), engine->GetRunningCost().base(), engine->GetDisplayMaxSpeed(),
			engine->GetPower(), engine->GetDisplayDefaultCapacity(), uint(engine->GetDefaultCargoType()), engine->reliability, std::get<0>(quote).Succeeded());
	}
	Money stations{0}; bool extend = true;
	for (const auto anchor : {TileIndex{3156}, TileIndex{520}}) {
		// These platforms extend their existing station owner. Requesting an
		// independent adjacent station over the old platform must be refused.
		auto quote = QuoteSurfaceIncome<Commands::BuildRailStation>(anchor, anchor, RAILTYPE_ELECTRIC, Axis::X, uint8_t{1}, uint8_t{5}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
		fmt::print("SURFACE_INCOME_FLEET_PLATFORM anchor={} length=5 admitted={} cost={} error={}\n", anchor.base(), quote.Succeeded(), quote.GetCost().base(), quote.GetErrorMessage().base());
		if (quote.Failed()) { extend = false; continue; }
		stations += quote.GetCost();
	}
	auto coal = QuoteSurfaceIncome<Commands::BuildVehicle>(TileIndex{3161}, TileIndex{3161}, EngineID{29}, false, INVALID_CARGO, ClientID::Server); REQUIRE(std::get<0>(coal).Succeeded());
	const auto *selected = Engine::Get(EngineID{26}); REQUIRE(IsEngineBuildable(selected->index, VehicleType::Train, _current_company));
	REQUIRE(selected->VehInfo<RailVehicleInfo>().railtypes.Test(RAILTYPE_ELECTRIC)); REQUIRE(IsValidCargoType(selected->GetDefaultCargoType()));
	const auto rolling = 3 * selected->GetCost(); const auto capacity = extend ? 4 * std::get<0>(coal).GetCost() + stations : Money{0};
	const auto capital = conversion + rolling + capacity; const Money buffer{150000};
	const auto shortfall = std::max(Money{0}, capital + buffer - company->money); const Money credit{((shortfall.base() + 9999) / 10000) * 10000};
	auto loan = QuoteSurfaceIncome<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit); REQUIRE(loan.Succeeded());
	fmt::print("SURFACE_INCOME_FLEET_PLAN tiles={} conversion_actions={} conversion={} head=26 heads=3 rolling={} coal_extension={} capacity_cost={} capital={} buffer={} credit={} cash={} debt={} ceiling={}\n",
		assets.size(), actions, conversion.base(), rolling.base(), extend, capacity.base(), capital.base(), buffer.base(), credit.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base());
	REQUIRE(extend); return credit;
}

/** Recall each genuine commercial train through its final receiver before normal empty depot admission. */
static void RecallSurfaceIncomeFleet()
{
	// Deliver each train's own stock through its original receiver. No cargo
	// transfer, position assignment or destructive loaded sale is permitted.
	for (uint role = 0; role < surface_income_names.size(); ++role) {
		const auto *train = FindSurfaceIncomeTrain(role);
		while (train->GetNumOrders() != 0) PaySurfaceIncome<Commands::DeleteOrder>(TileIndex{0}, train->index, VehicleOrderID{0});
		Order deliver; deliver.MakeGoToStation(GetStationIndex(surface_income_receivers[role]));
		deliver.SetLoadType(OrderLoadType::NoLoad); deliver.SetStopLocation(OrderStopLocation::FarEnd);
		PaySurfaceIncome<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{0}, deliver);
		Order home; home.MakeGoToDepot(Depot::GetByTile(surface_income_depots[role])->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		PaySurfaceIncome<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{1}, home);
		fmt::print("SURFACE_INCOME_FLEET_RECALL role={} head={} receiver={} depot={} supplied_pose=false\n", role, train->index.base(), GetStationIndex(surface_income_receivers[role]).base(), surface_income_depots[role].base());
	}
}

/** Buy the freshly admitted whole electric renewal, preserving all existing useful freight wagons. */
static void RenewSurfaceIncomeFleet()
{
	const auto credit = QuerySurfaceIncomeFleet(); const auto *company = Company::Get(_current_company);
	const auto cash = company->money, debt = company->current_loan; Money invoice{0};
	// All real routes must first have finished delivery and returned empty.
	// Matching bank credit changes both balances and cannot count as income.
	for (uint role = 0; role < surface_income_names.size(); ++role) {
		const auto *train = FindSurfaceIncomeTrain(role); REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->tile == surface_income_depots[role]);
		for (const auto *part = train; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
	}
	if (credit > 0) PaySurfaceIncome<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit);
	REQUIRE(company->current_loan == debt + credit); REQUIRE(company->money == cash + credit);
	// The native converter owns bridge-pair and station/depot conversion.
	// Only the original selected paid commercial assets are upgraded.
	for (const auto tile : SurfaceIncomeRailSelection()) {
		if (IsTileType(tile, TileType::TunnelBridge)) {
			if (tile > GetOtherBridgeEnd(tile)) continue;
		}
		invoice += PaySurfaceIncome<Commands::ConvertRail>(tile, tile, tile, RAILTYPE_ELECTRIC, false).GetCost();
		REQUIRE(GetTileRailType(tile) == RAILTYPE_ELECTRIC);
	}
	for (const auto anchor : {TileIndex{3156}, TileIndex{520}}) {
		invoice += PaySurfaceIncome<Commands::BuildRailStation>(anchor, anchor, RAILTYPE_ELECTRIC, Axis::X, uint8_t{1}, uint8_t{5}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false).GetCost();
	}
	const std::array<TileIndex, 3> pickups{TileIndex{1702}, TileIndex{3217}, TileIndex{3158}};
	for (uint role = 0; role < surface_income_names.size(); ++role) {
		const auto *old = FindSurfaceIncomeTrain(role); const auto old_id = old->index; std::vector<VehicleID> wagons;
		// Record actual original wagon identities before the front/rear sale;
		// none of their paid capacity is replaced by a synthetic equivalent.
		for (const auto *part = old; part != nullptr; part = part->Next()) {
			if (part->GetEngine()->VehInfo<RailVehicleInfo>().railveh_type == RailVehicleType::Wagon) wagons.push_back(part->index);
		}
		REQUIRE(wagons.size() == (role == 2 ? 4 : 12));
		const auto depot = surface_income_depots[role];
		// The now genuinely converted depot must admit the entire original
		// purchase before any old locomotive is sold or replacement is bought.
		auto admitted = QuoteSurfaceIncome<Commands::BuildVehicle>(depot, depot, EngineID{26}, false, INVALID_CARGO, ClientID::Server);
		REQUIRE(std::get<0>(admitted).Succeeded());
		invoice += PaySurfaceIncome<Commands::SellVehicle>(TileIndex{0}, old_id, false, false, ClientID::Server).GetCost();
		auto bought = PaySurfaceIncome<Commands::BuildVehicle>(depot, depot, EngineID{26}, false, INVALID_CARGO, ClientID::Server);
		invoice += std::get<0>(bought).GetCost(); const auto head = std::get<1>(bought);
		for (const auto id : wagons) {
			auto *part = Train::Get(id);
			if (part->First()->index != head) PaySurfaceIncome<Commands::MoveRailVehicle>(TileIndex{0}, id, Train::Get(head)->Last()->index, false);
			fmt::print("SURFACE_INCOME_FLEET_RETAINED role={} wagon={} engine={}\n", role, id.base(), part->engine_type.base());
		}
		if (role == 2) {
			// Four additional real Coal wagons recover useful line capacity;
			// the longer train uses the two normally extended five-tile platforms.
			for (uint index = 0; index < 4; ++index) {
				auto wagon = PaySurfaceIncome<Commands::BuildVehicle>(depot, depot, EngineID{29}, false, INVALID_CARGO, ClientID::Server);
				invoice += std::get<0>(wagon).GetCost(); auto *part = Train::Get(std::get<1>(wagon));
				if (part->First()->index != head) PaySurfaceIncome<Commands::MoveRailVehicle>(TileIndex{0}, part->index, Train::Get(head)->Last()->index, false);
			}
		}
		PaySurfaceIncome<Commands::RenameVehicle>(TileIndex{0}, head, surface_income_names[role]);
		Order collect; collect.MakeGoToStation(GetStationIndex(pickups[role])); collect.SetLoadType(role == 0 ? OrderLoadType::FullLoadAny : OrderLoadType::FullLoad);
		collect.SetUnloadType(OrderUnloadType::NoUnload); collect.SetStopLocation(OrderStopLocation::FarEnd);
		PaySurfaceIncome<Commands::InsertOrder>(TileIndex{0}, head, VehicleOrderID{0}, collect);
		Order deliver; deliver.MakeGoToStation(GetStationIndex(surface_income_receivers[role])); deliver.SetLoadType(OrderLoadType::NoLoad); deliver.SetStopLocation(OrderStopLocation::FarEnd);
		PaySurfaceIncome<Commands::InsertOrder>(TileIndex{0}, head, VehicleOrderID{1}, deliver);
		const auto *train = Train::Get(head); REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->gcache.cached_total_length <= (role == 2 ? 5 : 7) * TILE_SIZE);
		REQUIRE(train->GetDisplayMaxSpeed() == 264);
		PaySurfaceIncome<Commands::StartStopVehicle>(TileIndex{0}, head, true);
		fmt::print("SURFACE_INCOME_FLEET_RENEWED role={} old_head={} head={} engine=26 speed={} length={} reliability={}\n", role, old_id.base(), head.base(), train->GetDisplayMaxSpeed(), train->gcache.cached_total_length, train->reliability);
	}
	REQUIRE(company->money == cash + credit - invoice); REQUIRE(company->current_loan == debt + credit); REQUIRE(company->money >= 150000);
	fmt::print("SURFACE_INCOME_FLEET_BOUGHT credit={} invoice={} cash_before={} debt_before={} cash={} debt={} ceiling={} supplied_stock=false\n", credit.base(), invoice.base(), cash.base(), debt.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base());
}

/** Actual owned commerce funds the next chemical factory without supplied material, work or capital. */
TEST_CASE("Authentic Surface commerce buys original trains and earns ordinary Propellant capital", "[.surface-entry-income]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *mode = std::getenv("OTTD_SURFACE_INCOME_PHASE"); REQUIRE(mode != nullptr); const std::string phase{mode};
	REQUIRE((phase == "fund" || phase == "earn" || phase == "coal-query" || phase == "coal-recall" || phase == "coal-settle" || phase == "coal-rewagon" || phase == "repay" || phase == "town-query" || phase == "town-credit" || phase == "town-buy" || phase == "fleet-query" || phase == "fleet-recall" || phase == "fleet-settle" || phase == "fleet-buy")); // Each explicitly selected purchase, pure query or earned service/bank boundary retains its original owners.
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-income"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve original failures through native shutdown; no compensating inventory, work or credit is supplied. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceIncome(paths.saves / ("surface-income-before-" + phase + "-440.sav"));
			if (phase == "fund") FundSurfaceIncome();
			else if (phase == "coal-query") QuoteSurfaceIncomeCoal();
			else if (phase == "coal-recall") RecallSurfaceIncomeCoal(*FindSurfaceIncomeTrain(2));
			else if (phase == "coal-rewagon") RewagonSurfaceIncomeCoal();
			else if (phase == "repay") RepaySurfaceIncomeLoan();
			else if (phase == "town-query") QuerySurfaceIncomeTownService();
			else if (phase == "town-credit") CreditSurfaceIncomeTownService();
			else if (phase == "town-buy") BuySurfaceIncomeTownService();
			else if (phase == "fleet-query") QuerySurfaceIncomeFleet();
			else if (phase == "fleet-recall") RecallSurfaceIncomeFleet();
			else if (phase == "fleet-buy") RenewSurfaceIncomeFleet();
			else EarnSurfaceIncome(phase == "coal-settle", phase == "fleet-settle");
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
