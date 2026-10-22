/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_renewal.cpp Genuine continuing freight renewal with original wagons and company books.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEE.
 * Original commands and StateGameLoop own all orders, arrivals, purchases,
 * refunds, cargo, work and finance. No vehicle pose or useful stock is supplied.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../command_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../vehicle_cmd.h"
#include "../order_cmd.h"
#include "../depot_base.h"
#include "../misc_cmd.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_entry_work.h"
#include "../safeguards.h"

static std::function<void()> *surface_renewal_observer = nullptr; ///< Invocation callback; never a company or vehicle owner.
static constexpr std::array<std::string_view, 3> surface_renewal_names{"Surface Income farm", "Surface Income goods", "Surface Income coal"};
static constexpr std::array<TileIndex, 3> surface_renewal_depots{TileIndex{1638}, TileIndex{3224}, TileIndex{3161}};

/** @param args Parameter-free original console arguments. @return True after inspecting the actual loaded world. */
static bool ObserveSurfaceRenewal(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_renewal_observer != nullptr); (*surface_renewal_observer)(); return true;
}

/** @param path Fresh invocation-owned output. Save actual owners without earning additional work. */
static void SaveSurfaceRenewal(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Quote one complete original synchronized command without mutation.
 * @tparam Tcmd Original command identity. @tparam Targs Native argument types.
 * @param tile Actual tiled action, or zero for management. @param args Entire native argument list.
 * @return Original whole cost or refusal.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceRenewal(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** Freshly quote and settle one original command, preserving its actual financial accounting.
 * @tparam Tcmd Original command identity. @tparam Targs Native argument types.
 * @param tile Actual tiled action, or zero for management. @param args Complete native arguments.
 * @return Original settled result; quote/refusal is never a supplied prerequisite.
 */
template <Commands Tcmd, typename... Targs>
static auto PaySurfaceRenewal(TileIndex tile, Targs... args)
{
	auto quote = QuoteSurfaceRenewal<Tcmd>(tile, args...);
	INFO("Freight renewal command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(quote).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(quote).Succeeded());
	auto paid = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(args...));
	REQUIRE(ExtractCommandCost(paid).Succeeded()); REQUIRE(ExtractCommandCost(paid).GetCost() == ExtractCommandCost(quote).GetCost());
	fmt::print("SURFACE_RENEWAL_INVOICE command={} tile={} cost={}\n", GetCommandName(Tcmd), tile.base(), ExtractCommandCost(paid).GetCost().base()); return paid;
}

/** @param role Actual named original commercial route. @return Its unique real owned front, following native names through normal pool reuse. */
static const Train *FindSurfaceRenewalTrain(uint role)
{
	const Train *result = nullptr;
	for (const auto *train : Train::Iterate()) {
		if (!train->IsFrontEngine()) continue;
		if (train->name != surface_renewal_names[role]) continue;
		REQUIRE(result == nullptr); REQUIRE(train->owner == _current_company); result = train;
	}
	REQUIRE(result != nullptr); return result;
}

/** Pure complete current replacement invoice, excluding unsold assets and any invented headroom. */
struct SurfaceRenewalBudget {
	Money gross{0}; ///< Original three-head purchase cost.
	Money credit{0}; ///< Matching native loan needed to preserve the selected cash reserve.
};

/** @param recall Whether to admit the original explicit recall/order arrangement too.
 * @return Complete actual gross purchase and matching credit, without counting unsold book value.
 */
static SurfaceRenewalBudget QuerySurfaceRenewalBudget(bool recall)
{
	SurfaceRenewalBudget result; const auto *company = Company::Get(_current_company);
	REQUIRE(IsEngineBuildable(EngineID{24}, VehicleType::Train, _current_company));
	// Each actual owned electric depot must admit the chosen original head.
	// Original wagon identities/capacity survive; no speculative sale proceeds
	// reduce the full capital required before any purchase can start.
	for (uint role = 0; role < surface_renewal_names.size(); ++role) {
		const auto *train = FindSurfaceRenewalTrain(role); REQUIRE(train->engine_type == EngineID{26});
		const auto depot = surface_renewal_depots[role];
		const auto quote = QuoteSurfaceRenewal<Commands::BuildVehicle>(depot, depot, EngineID{24}, false, INVALID_CARGO, ClientID::Server);
		REQUIRE(std::get<0>(quote).Succeeded()); result.gross += std::get<0>(quote).GetCost();
		if (recall) {
			REQUIRE(train->GetNumOrders() == 2); REQUIRE(train->GetOrder(0)->IsType(OT_GOTO_STATION)); REQUIRE(train->GetOrder(1)->IsType(OT_GOTO_STATION));
			Order home; home.MakeGoToDepot(Depot::GetByTile(depot)->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
			REQUIRE(QuoteSurfaceRenewal<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{2}, home).Succeeded());
			if (train->cur_implicit_order_index != VehicleOrderID{1}) REQUIRE(QuoteSurfaceRenewal<Commands::SkipToOrder>(TileIndex{0}, train->index, VehicleOrderID{1}).Succeeded());
		}
		fmt::print("SURFACE_RENEWAL_MODEL role={} head={} old_model=26 model=24 depot={} cost={} age={} maximum={} value={} orders={}\n", role, train->index.base(), depot.base(),
			std::get<0>(quote).GetCost().base(), train->age.base(), train->max_age.base(), train->value.base(), train->GetNumOrders());
	}
	const Money reserve{150000}; const auto deficit = result.gross + reserve - company->money;
	if (deficit > 0) result.credit = Money{((deficit.base() + 9999) / 10000) * 10000};
	REQUIRE(company->current_loan + result.credit <= company->GetMaxLoan());
	if (result.credit > 0) REQUIRE(QuoteSurfaceRenewal<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, result.credit).Succeeded());
	fmt::print("SURFACE_RENEWAL_PLAN model=24 heads=3 gross={} reserve=150000 credit={} cash={} debt={} ceiling={} recall={} unsold_value_counted=false\n", result.gross.base(), result.credit.base(),
		company->money.base(), company->current_loan.base(), company->GetMaxLoan().base(), recall);
	return result;
}

/** Append real depot halts, retaining both original commercial orders and every actual held cargo packet. */
static void RecallSurfaceRenewalFleet()
{
	QuerySurfaceRenewalBudget(true);
	// A partly loaded downstream train cannot wait for a feeder that has
	// already parked. The original next-order command sends its actual held
	// freight to the existing receiver, then its new ordinary depot halt.
	for (uint role = 0; role < surface_renewal_names.size(); ++role) {
		const auto *train = FindSurfaceRenewalTrain(role); const auto depot = surface_renewal_depots[role];
		Order home; home.MakeGoToDepot(Depot::GetByTile(depot)->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		PaySurfaceRenewal<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{2}, home);
		if (train->cur_implicit_order_index != VehicleOrderID{1}) PaySurfaceRenewal<Commands::SkipToOrder>(TileIndex{0}, train->index, VehicleOrderID{1});
		fmt::print("SURFACE_RENEWAL_RECALL role={} head={} receiver=1 depot={} orders=3 supplied_pose=false\n", role, train->index.base(), depot.base());
	}
}

/** @return Whether every original train has genuinely delivered and arrived stopped/empty at its own native depot. */
static bool SurfaceRenewalFleetArrived()
{
	for (uint role = 0; role < surface_renewal_names.size(); ++role) {
		const auto *train = FindSurfaceRenewalTrain(role);
		// One arrival cannot authorize selling a still-moving/loaded owner.
		if (!train->IsStoppedInDepot() || train->tile != surface_renewal_depots[role]) return false;
		for (const auto *part = train; part != nullptr; part = part->Next()) if (part->cargo.TotalCount() != 0) return false;
	}
	return true;
}

/** Earn only actual delivery/depot arrival in short resumable native bursts. */
static void WaitSurfaceRenewalFleet()
{
	EarnSurfaceEntryWork("SURFACE_RENEWAL", 3000, SurfaceRenewalFleetArrived);
	for (uint role = 0; role < surface_renewal_names.size(); ++role) {
		const auto *train = FindSurfaceRenewalTrain(role);
		fmt::print("SURFACE_RENEWAL_ARRIVAL role={} head={} tile={} depot={} stopped={}\n", role, train->index.base(), train->tile.base(), surface_renewal_depots[role].base(), train->IsStoppedInDepot());
	}
}

/** @param train Actual newly coupled consist. @param role Original service role. Validate useful native wagon capacity and original platform fit. */
static void CheckSurfaceRenewalCapacity(const Train &train, uint role)
{
	CargoArray capacity{}; uint parts = 0, wagons = 0;
	// Read the actual coupled native chain; no equivalent synthetic capacity
	// may replace an original paid wagon or conceal a missing rear engine.
	for (const auto *part = &train; part != nullptr; part = part->Next()) {
		++parts; capacity[part->cargo_type] += part->cargo_cap; REQUIRE(part->cargo.TotalCount() == 0);
		if (part->GetEngine()->VehInfo<RailVehicleInfo>().railveh_type == RailVehicleType::Wagon) ++wagons;
	}
	REQUIRE(wagons == (role == 2 ? 8 : 12)); REQUIRE(parts == wagons + 1);
	REQUIRE(train.gcache.cached_total_length <= (role == 2 ? 5 : 7) * TILE_SIZE);
	if (role == 0) {
		REQUIRE(capacity[GetCargoTypeByLabel(CargoLabel{"GRAI"})] == 180); REQUIRE(capacity[GetCargoTypeByLabel(CargoLabel{"LVST"})] == 150);
	} else REQUIRE(capacity[GetCargoTypeByLabel(role == 1 ? CargoLabel{"GOOD"} : CargoLabel{"COAL"})] == (role == 1 ? 300 : 240));
}

/** Buy only actual current heads after real empty arrival, retain original wagons and native copied orders, then restore useful service. */
static void BuySurfaceRenewalFleet()
{
	REQUIRE(SurfaceRenewalFleetArrived()); const auto budget = QuerySurfaceRenewalBudget(false); const auto *company = Company::Get(_current_company);
	const auto cash = company->money, debt = company->current_loan; Money invoice{0};
	if (budget.credit > 0) PaySurfaceRenewal<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, budget.credit);
	// Native copy-before-sale preserves original order meaning. Selling only
	// the old engine also retires its native multihead rear; every real wagon
	// is then normally recoupled to the newly purchased single-head body.
	for (uint role = 0; role < surface_renewal_names.size(); ++role) {
		const auto *old = FindSurfaceRenewalTrain(role); const auto old_id = old->index; const auto depot = surface_renewal_depots[role];
		REQUIRE(old->GetNumOrders() == 3); std::vector<VehicleID> wagons;
		for (const auto *part = old; part != nullptr; part = part->Next()) {
			REQUIRE(part->cargo.TotalCount() == 0);
			if (part->GetEngine()->VehInfo<RailVehicleInfo>().railveh_type == RailVehicleType::Wagon) wagons.push_back(part->index);
		}
		REQUIRE(wagons.size() == (role == 2 ? 8 : 12));
		auto bought = PaySurfaceRenewal<Commands::BuildVehicle>(depot, depot, EngineID{24}, false, INVALID_CARGO, ClientID::Server);
		invoice += std::get<0>(bought).GetCost(); const auto head = std::get<1>(bought);
		PaySurfaceRenewal<Commands::CloneOrder>(TileIndex{0}, CO_COPY, head, old_id);
		PaySurfaceRenewal<Commands::DeleteOrder>(TileIndex{0}, head, VehicleOrderID{2});
		invoice += PaySurfaceRenewal<Commands::SellVehicle>(TileIndex{0}, old_id, false, false, ClientID::Server).GetCost();
		for (const auto id : wagons) {
			if (Train::Get(id)->First()->index != head) PaySurfaceRenewal<Commands::MoveRailVehicle>(TileIndex{0}, id, Train::Get(head)->Last()->index, false);
			fmt::print("SURFACE_RENEWAL_WAGON role={} vehicle={} engine={}\n", role, id.base(), Train::Get(id)->engine_type.base());
		}
		PaySurfaceRenewal<Commands::RenameVehicle>(TileIndex{0}, head, std::string{surface_renewal_names[role]});
		const auto *train = Train::Get(head); REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->GetNumOrders() == 2); CheckSurfaceRenewalCapacity(*train, role);
		PaySurfaceRenewal<Commands::StartStopVehicle>(TileIndex{0}, head, true);
		fmt::print("SURFACE_RENEWAL_HEAD role={} old_head={} head={} model=24 speed={} length={} orders=2\n", role, old_id.base(), head.base(), train->GetDisplayMaxSpeed(), train->gcache.cached_total_length);
	}
	REQUIRE(company->money == cash + budget.credit - invoice); REQUIRE(company->current_loan == debt + budget.credit); REQUIRE(company->money >= 150000);
	fmt::print("SURFACE_RENEWAL_PURCHASE credit={} net_cost={} cash_before={} debt_before={} cash={} debt={} ceiling={} reserve=150000\n", budget.credit.base(), invoice.base(), cash.base(), debt.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base());
}

/** Earn one actual renewed-service interval; company accounting alone establishes useful income and operating costs. */
static void EarnSurfaceRenewalFleet()
{
	for (uint role = 0; role < surface_renewal_names.size(); ++role) {
		const auto *train = FindSurfaceRenewalTrain(role); REQUIRE(train->engine_type == EngineID{24}); REQUIRE(train->GetNumOrders() == 2);
		REQUIRE(train->age.base() + 366 < train->max_age.base());
	}
	/** @return False; the declared native27000tick interval, not an installed profit/pose, owns completion. */
	EarnSurfaceEntryWork("SURFACE_RENEWAL", 27000, [] { return false; });
}

/** Preserve actual source-world finance, cargo and useful wagons through original paid conventional replacement. */
TEST_CASE("Authentic continuing freight retains real wagons and paid native service", "[.surface-entry-renewal]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_RENEWAL_PHASE"); const std::string phase = selected == nullptr ? "query" : selected;
	REQUIRE((phase == "query" || phase == "recall" || phase == "arrive" || phase == "buy" || phase == "earn")); // Only explicit original quote, movement and purchase boundaries may touch this real lineage.
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-renewal"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Retain original native failures through shutdown without compensating stock, money, time or vehicle poses. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceRenewal(paths.saves / ("surface-renewal-before-" + phase + "-440.sav"));
			if (phase == "query") QuerySurfaceRenewalBudget(true);
			else if (phase == "recall") RecallSurfaceRenewalFleet();
			else if (phase == "arrive") WaitSurfaceRenewalFleet();
			else if (phase == "buy") BuySurfaceRenewalFleet();
			else EarnSurfaceRenewalFleet();
			SaveSurfaceRenewal(paths.saves / ("surface-renewal-" + phase + "-440.sav"));
			fmt::print("SURFACE_RENEWAL_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_renewal_observer, &inspect); IConsole::CmdRegister("surface_entry_renewal", ObserveSurfaceRenewal);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
