/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_bus_renewal.cpp Renew genuinely empty original passenger service.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEJ.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEQE for explicit regional selection.
 * Native commands, cargo owners, vehicle movement and company books own all work.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../underground_vehicle.h"
#include "../vehicle_cmd.h"
#include "../depot_base.h"
#include "../roadveh.h"
#include "../station_base.h"
#include "../order_cmd.h"
#include "../misc_cmd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_entry_work.h"
#include "../safeguards.h"

static std::function<void()> *surface_bus_observer = nullptr; ///< Invocation callback, never simulation state.
static constexpr TileIndex surface_bus_depot{2984};

/** @param args Parameter-free original console arguments. @return True after native observation. */
static bool ObserveSurfaceBusRenewal(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_bus_observer != nullptr); (*surface_bus_observer)(); return true;
}

/** @param path Invocation-owned save. Retain native owners without earning game work. */
static void SaveSurfaceBusRenewal(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Query a complete original action without changing game owners.
 * @tparam Tcmd Native command. @tparam Targs Original argument types.
 * @param tile Actual tiled action or zero. @param args Complete native arguments.
 * @return Original quote/refusal.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceBus(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** Freshly quote and execute one complete original action.
 * @tparam Tcmd Native command. @tparam Targs Original argument types.
 * @param tile Actual tiled action or zero. @param args Complete native arguments.
 * @return Original settled result.
 */
template <Commands Tcmd, typename... Targs>
static auto PaySurfaceBus(TileIndex tile, Targs... args)
{
	auto quote = QuoteSurfaceBus<Tcmd>(tile, args...);
	INFO("Bus command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(quote).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(quote).Succeeded());
	auto paid = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(args...));
	REQUIRE(ExtractCommandCost(paid).Succeeded()); REQUIRE(ExtractCommandCost(paid).GetCost() == ExtractCommandCost(quote).GetCost());
	fmt::print("SURFACE_BUS_INVOICE command={} cost={}\n", GetCommandName(Tcmd), ExtractCommandCost(paid).GetCost().base()); return paid;
}

/** @return True only for the explicitly selected native regional producer; absent selection preserves original local service. */
static bool SurfaceRegionalBuses()
{
	const char *service = std::getenv("OTTD_SURFACE_BUS_SERVICE");
	if (service == nullptr) return false;
	REQUIRE(std::string_view{service} == "regional"); return true;
}

/** @return The three actual selected named passenger vehicles, following identities rather than recycled slots. */
static std::vector<const RoadVehicle *> SurfaceBuses()
{
	std::vector<const RoadVehicle *> result;
	// Each service name must have exactly one actual owned PASS body. Existing
	// reserve trucks and future pool-slot reuse cannot become a passenger bus.
	for (uint number = 1; number <= 3; ++number) {
		const auto name = fmt::format("{} passenger {}", SurfaceRegionalBuses() ? "Surface Regional" : "Surface Launch", number); const RoadVehicle *found = nullptr;
		for (const auto *bus : RoadVehicle::Iterate()) {
			if (bus->name != name) continue;
			REQUIRE(found == nullptr); found = bus;
		}
		REQUIRE(found != nullptr); REQUIRE(found->owner == _current_company);
		REQUIRE(found->cargo_type == GetCargoTypeByLabel(CargoLabel{"PASS"})); REQUIRE(found->cargo_cap >= 40); result.push_back(found);
	}
	return result;
}

/** Original current whole purchase admission, without owning a future bus or refund. */
struct SurfaceBusQuote {
	EngineID engine = EngineID::Invalid(); ///< Cheapest actually admitted useful passenger model.
	Money unit{0}, credit{0}; ///< Fresh original purchase and necessary matching bank credit.
	uint capacity = 0; ///< Actual quoted capacity, at least the existing40seats.
};

/** @return Whole three-bus current admission with150000cash reserve; future resale/income is excluded. */
static SurfaceBusQuote QuoteSurfaceBusFleet()
{
	SurfaceBusQuote result; const auto *depot = Depot::GetByTile(surface_bus_depot); REQUIRE(depot != nullptr); REQUIRE(GetTileOwner(surface_bus_depot) == _current_company);
	const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"}); REQUIRE(IsValidCargoType(passenger));
	// Native availability and the actual depot/refit command decide which
	// current road models can replace this useful passenger service.
	for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
		if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
		auto quote = QuoteSurfaceBus<Commands::BuildVehicle>(surface_bus_depot, surface_bus_depot, engine->index, false, passenger, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		if (std::get<2>(quote) < 40) continue;
		fmt::print("SURFACE_BUS_MODEL model={} capacity={} unit={}\n", engine->index.base(), std::get<2>(quote), std::get<0>(quote).GetCost().base());
		// Preserve the cheapest admitted complete service, with stable native
		// iteration deciding equal prices rather than a historical engine ID.
		if (result.engine != EngineID::Invalid() && result.unit <= std::get<0>(quote).GetCost()) continue;
		result.engine = engine->index; result.unit = std::get<0>(quote).GetCost(); result.capacity = std::get<2>(quote);
	}
	REQUIRE(result.engine != EngineID::Invalid()); const auto *company = Company::Get(_current_company);
	const auto shortfall = std::max(Money{0}, result.unit * 3 + Money{150000} - company->money);
	result.credit = Money{((shortfall.base() + 9999) / 10000) * 10000};
	if (result.credit > 0) { auto bank = QuoteSurfaceBus<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, result.credit); REQUIRE(bank.Succeeded()); }
	REQUIRE(company->current_loan + result.credit <= company->GetMaxLoan()); REQUIRE(company->money + result.credit - result.unit * 3 >= 150000);
	fmt::print("SURFACE_BUS_PLAN model={} capacity={} unit={} gross={} credit={} reserve=150000 cash={} debt={} ceiling={} depot={} buses=3 future_refund=0\n", result.engine.base(), result.capacity, result.unit.base(), (result.unit * 3).base(), result.credit.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base(), surface_bus_depot.base());
	return result;
}

/** @param bus Actual original passenger vehicle. @param load Required original stop loading policy. */
static void VerifySurfaceBusOrders(const RoadVehicle &bus, OrderLoadType load)
{
	// Both real stops retain normal unload and FarEnd bay access. Suppressing
	// loading alone lets passengers reach a receiver before the depot halt.
	for (int index = 0; index < 2; ++index) {
		const auto *order = bus.GetOrder(index); REQUIRE(order != nullptr); REQUIRE(order->IsType(OT_GOTO_STATION));
		// Only explicit regional service uses the real Village terminal20.
		// Original local buses retain their native17to18commercial route.
		const auto destination = index == 0 ? 17 : (SurfaceRegionalBuses() ? 20 : 18);
		REQUIRE(order->GetDestination() == DestinationID{static_cast<uint16_t>(destination)});
		REQUIRE(order->GetLoadType() == load); REQUIRE(order->GetUnloadType() == OrderUnloadType::UnloadIfPossible); REQUIRE(order->GetStopLocation() == OrderStopLocation::FarEnd);
	}
}

/** Query complete native empty recall and future copied-order restoration before any purchase. */
static void QuerySurfaceBusRenewal()
{
	QuoteSurfaceBusFleet(); const auto *depot = Depot::GetByTile(surface_bus_depot); REQUIRE(depot != nullptr);
	for (const auto *bus : SurfaceBuses()) {
		REQUIRE(bus->GetNumOrders() == 2); VerifySurfaceBusOrders(*bus, OrderLoadType::LoadIfPossible);
		for (uint8_t index = 0; index < 2; ++index) {
			auto quote = QuoteSurfaceBus<Commands::ModifyOrder>(TileIndex{0}, bus->index, VehicleOrderID{index}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::NoLoad)); REQUIRE(quote.Succeeded());
		}
		Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		auto halt = QuoteSurfaceBus<Commands::InsertOrder>(TileIndex{0}, bus->index, VehicleOrderID{2}, home); REQUIRE(halt.Succeeded());
		fmt::print("SURFACE_BUS_RECALL_PLAN vehicle={} held={} first=17 last={} depot={} no_new_load=true return_admitted=true\n", bus->index.base(), bus->cargo.TotalCount(), SurfaceRegionalBuses() ? 20 : 18, surface_bus_depot.base());
	}
}

/** Suppress new pickups and append a real depot halt after both original passenger receivers. */
static void RecallSurfaceBuses()
{
	const auto *depot = Depot::GetByTile(surface_bus_depot); REQUIRE(depot != nullptr);
	for (const auto *bus : SurfaceBuses()) {
		REQUIRE(bus->GetNumOrders() == 2); VerifySurfaceBusOrders(*bus, OrderLoadType::LoadIfPossible);
		for (uint8_t index = 0; index < 2; ++index) PaySurfaceBus<Commands::ModifyOrder>(TileIndex{0}, bus->index, VehicleOrderID{index}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::NoLoad));
		Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		PaySurfaceBus<Commands::InsertOrder>(TileIndex{0}, bus->index, VehicleOrderID{2}, home); VerifySurfaceBusOrders(*bus, OrderLoadType::NoLoad);
	}
}

/** @return True only after all original passengers were delivered and all buses really halted in the owned depot. */
static bool SurfaceBusesEmptyHome()
{
	for (const auto *bus : SurfaceBuses()) {
		// Empty cargo and physical depot arrival jointly authorize sale; an
		// order, an adjacent stopped pose or a loaded halt cannot replace them.
		if (!bus->IsStoppedInDepot() || bus->tile != surface_bus_depot || bus->cargo.TotalCount() != 0) return false;
	}
	return true;
}

/** Buy fresh empty bodies, copy actual orders, sell only empty old buses, and restore original paid service. */
static void RenewSurfaceBuses()
{
	REQUIRE(SurfaceBusesEmptyHome()); const auto quote = QuoteSurfaceBusFleet(); const auto *company = Company::Get(_current_company);
	const auto cash = company->money, debt = company->current_loan; Money total{0};
	if (quote.credit > 0) PaySurfaceBus<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, quote.credit);
	const auto original = SurfaceBuses();
	for (const auto *old : original) {
		REQUIRE(old->GetNumOrders() == 3); VerifySurfaceBusOrders(*old, OrderLoadType::NoLoad);
		const auto name = old->name; const auto old_id = old->index;
		const auto paid = PaySurfaceBus<Commands::BuildVehicle>(surface_bus_depot, surface_bus_depot, quote.engine, false, GetCargoTypeByLabel(CargoLabel{"PASS"}), ClientID::Server);
		total += std::get<0>(paid).GetCost(); const auto *bus = RoadVehicle::Get(std::get<1>(paid)); REQUIRE(bus->IsStoppedInDepot()); REQUIRE(bus->cargo.TotalCount() == 0); REQUIRE(bus->cargo_cap == quote.capacity);
		PaySurfaceBus<Commands::CloneOrder>(TileIndex{0}, CO_COPY, bus->index, old_id);
		PaySurfaceBus<Commands::DeleteOrder>(TileIndex{0}, bus->index, VehicleOrderID{2});
		for (uint8_t index = 0; index < 2; ++index) PaySurfaceBus<Commands::ModifyOrder>(TileIndex{0}, bus->index, VehicleOrderID{index}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::LoadIfPossible));
		VerifySurfaceBusOrders(*bus, OrderLoadType::LoadIfPossible); REQUIRE(bus->GetNumOrders() == 2);
		auto sold = PaySurfaceBus<Commands::SellVehicle>(TileIndex{0}, old_id, false, false, ClientID::Server); total += sold.GetCost();
		PaySurfaceBus<Commands::RenameVehicle>(TileIndex{0}, bus->index, name); PaySurfaceBus<Commands::StartStopVehicle>(TileIndex{0}, bus->index, true);
		fmt::print("SURFACE_BUS_RENEWED old={} vehicle={} model={} capacity={} age={} orders={} empty=true refund={}\n", old_id.base(), bus->index.base(), bus->engine_type.base(), bus->cargo_cap, bus->age.base(), bus->GetNumOrders(), -sold.GetCost().base());
	}
	REQUIRE(company->money == cash + quote.credit - total); REQUIRE(company->current_loan == debt + quote.credit); REQUIRE(company->money >= 150000);
}

/** Genuine passenger renewal and bounded native service, with no supplied money, cargo or movement. */
TEST_CASE("Original passenger service renews only after real empty depot return", "[.surface-entry-bus-renewal]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_BUS_PHASE"); const std::string phase = selected == nullptr ? "query" : selected;
	const std::array<std::string_view, 5> allowed{"query", "recall", "work", "renew", "earn"}; REQUIRE(std::ranges::find(allowed, phase) != allowed.end());
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-bus-renewal"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve original failures through ordinary shutdown without compensating gameplay changes. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company)); SaveSurfaceBusRenewal(paths.saves / ("surface-bus-before-" + phase + "-440.sav"));
			if (phase == "query") QuerySurfaceBusRenewal();
			else if (phase == "recall") RecallSurfaceBuses();
			else if (phase == "work") EarnSurfaceEntryWork("SURFACE_BUS", 3000, SurfaceBusesEmptyHome);
			else if (phase == "renew") RenewSurfaceBuses();
			else {
				for (const auto *bus : SurfaceBuses()) { REQUIRE(bus->GetNumOrders() == 2); VerifySurfaceBusOrders(*bus, OrderLoadType::LoadIfPossible); REQUIRE(bus->age < bus->max_age); }
				/** @return False; the actual3000native ticks own this bounded replay interval. */
				EarnSurfaceEntryWork("SURFACE_BUS", 3000, [] { return false; });
			}
			for (const auto *bus : SurfaceBuses()) fmt::print("SURFACE_BUS_CUSTODY vehicle={} model={} capacity={} age={} remaining={} held={} tile={} home={} orders={}\n", bus->index.base(), bus->engine_type.base(), bus->cargo_cap, bus->age.base(), (bus->max_age - bus->age).base(), bus->cargo.TotalCount(), bus->tile.base(), bus->IsStoppedInDepot(), bus->GetNumOrders());
			SaveSurfaceBusRenewal(paths.saves / ("surface-bus-" + phase + "-440.sav")); fmt::print("SURFACE_BUS_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_bus_observer, &inspect); IConsole::CmdRegister("surface_entry_bus_renewal", ObserveSurfaceBusRenewal);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
