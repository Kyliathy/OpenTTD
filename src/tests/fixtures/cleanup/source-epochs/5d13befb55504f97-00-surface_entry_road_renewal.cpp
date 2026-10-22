/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_road_renewal.cpp Preserve useful genuine ore/Steel road service through paid renewal.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEL.
 * Native commands, actual cargo delivery/depot arrival and original books own every mutation.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../vehicle_cmd.h"
#include "../depot_base.h"
#include "../roadveh.h"
#include "../order_cmd.h"
#include "../misc_cmd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_entry_work.h"
#include "../safeguards.h"

static std::function<void()> *surface_road_renewal_observer = nullptr; ///< Invocation callback, never a game owner.
static constexpr TileIndex surface_road_renewal_depot{2443};

/** @param args Parameter-free original console arguments. @return True after native observation. */
static bool ObserveSurfaceRoadRenewal(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_road_renewal_observer != nullptr); (*surface_road_renewal_observer)(); return true;
}

/** @param path Invocation-owned original save. Preserve all native owners without earning work. */
static void SaveSurfaceRoadRenewal(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Query one complete original synchronized action without mutating the world.
 * @tparam Tcmd Native command. @tparam Targs Original argument types.
 * @param tile Actual tiled action or zero. @param args Complete original arguments. @return Native quote/refusal.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceRoadRenewal(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** Freshly quote and execute one original whole action.
 * @tparam Tcmd Native command. @tparam Targs Original argument types.
 * @param tile Actual tiled action or zero. @param args Complete original arguments. @return Native settled result.
 */
template <Commands Tcmd, typename... Targs>
static auto PaySurfaceRoadRenewal(TileIndex tile, Targs... args)
{
	auto quote = QuoteSurfaceRoadRenewal<Tcmd>(tile, args...);
	INFO("Road renewal=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(quote).GetErrorMessage().base()); REQUIRE(ExtractCommandCost(quote).Succeeded());
	auto paid = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(args...));
	REQUIRE(ExtractCommandCost(paid).Succeeded()); REQUIRE(ExtractCommandCost(paid).GetCost() == ExtractCommandCost(quote).GetCost());
	fmt::print("SURFACE_ROAD_RENEWAL_INVOICE command={} cost={}\n", GetCommandName(Tcmd), ExtractCommandCost(paid).GetCost().base()); return paid;
}

/** @return Actual ore/Steel primary trucks, following their original unique names instead of recycled pool slots. */
static std::array<const RoadVehicle *, 2> SurfaceRoadFreight()
{
	std::array<const RoadVehicle *, 2> result{};
	for (uint index = 0; index < result.size(); ++index) {
		const std::string_view name = index == 0 ? "Surface ore delivery" : "Surface Steel delivery";
		// Retain the genuine cargo-specific service; a reserve carrier or
		// an old historical slot cannot substitute for this useful truck.
		for (const auto *truck : RoadVehicle::Iterate()) {
			if (truck->name != name) continue;
			REQUIRE(result[index] == nullptr); result[index] = truck;
		}
		const auto *truck = result[index]; REQUIRE(truck != nullptr); REQUIRE(truck->IsPrimaryVehicle()); REQUIRE(truck->owner == _current_company);
		REQUIRE(truck->cargo_type == GetCargoTypeByLabel(index == 0 ? CargoLabel{"IORE"} : CargoLabel{"STEL"})); REQUIRE(truck->cargo_cap >= (index == 0 ? 32 : 18));
	}
	return result;
}

/** @param truck Actual ore/Steel truck. @param pickup Required native pickup loading policy. */
static void VerifySurfaceRoadOrders(const RoadVehicle &truck, OrderLoadType pickup)
{
	// The earlier real ore-input expansion moved its receiver to station3.
	// Preserve the actual accepted native route, not the obsolete0to1sketch.
	const bool ore = truck.cargo_type == GetCargoTypeByLabel(CargoLabel{"IORE"});
	const std::array<uint16_t, 2> stops = ore ? std::array<uint16_t, 2>{0, 3} : std::array<uint16_t, 2>{1, 2};
	for (int index = 0; index < 2; ++index) {
		const auto *order = truck.GetOrder(index); REQUIRE(order != nullptr); REQUIRE(order->IsType(OT_GOTO_STATION));
		REQUIRE(order->GetDestination() == DestinationID{stops[index]});
		REQUIRE(order->GetStopLocation() == OrderStopLocation::FarEnd);
		REQUIRE(order->GetLoadType() == (index == 0 ? pickup : OrderLoadType::NoLoad));
		REQUIRE(order->GetUnloadType() == (index == 0 ? OrderUnloadType::NoUnload : OrderUnloadType::UnloadIfPossible));
	}
}

/** One current useful empty native carrier quote; never a future asset or refund owner. */
struct SurfaceRoadRenewalCarrier {
	EngineID engine = EngineID::Invalid(); ///< Cheapest current whole Surface-admitted compatible model.
	Money unit{0}; ///< Native complete current build/refit quote.
	uint capacity = 0; ///< Actual quoted carrying capacity.
};

/** Whole two-truck original admission, excluding future sale value and income. */
struct SurfaceRoadRenewalQuote {
	std::array<SurfaceRoadRenewalCarrier, 2> carriers{}; ///< Actual ore and Steel replacements.
	Money gross{0}, credit{0}; ///< Whole native purchases and necessary matching bank credit.
};

/** @return Original complete current two-truck admission retaining useful capacities and150000cash reserve. */
static SurfaceRoadRenewalQuote QuoteSurfaceRoadFleet()
{
	SurfaceRoadRenewalQuote result; REQUIRE(GetTileOwner(surface_road_renewal_depot) == _current_company); REQUIRE(Depot::GetByTile(surface_road_renewal_depot) != nullptr);
	const auto trucks = SurfaceRoadFreight();
	for (uint index = 0; index < trucks.size(); ++index) {
		const auto *truck = trucks[index]; auto &selected = result.carriers[index];
		// Native depot/layer/refit admission owns compatibility. The existing
		// original bulk model is valid on Surface despite its extra layer role.
		for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
			if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
			auto quote = QuoteSurfaceRoadRenewal<Commands::BuildVehicle>(surface_road_renewal_depot, surface_road_renewal_depot, engine->index, false, truck->cargo_type, ClientID::Server);
			if (std::get<0>(quote).Failed()) continue;
			if (std::get<2>(quote) < truck->cargo_cap) continue;
			fmt::print("SURFACE_ROAD_RENEWAL_MODEL role={} model={} capacity={} unit={}\n", index, engine->index.base(), std::get<2>(quote), std::get<0>(quote).GetCost().base());
			// Cheapest whole native compatible purchase preserves the useful
			// service rather than silently shrinking its original throughput.
			if (selected.engine != EngineID::Invalid() && selected.unit <= std::get<0>(quote).GetCost()) continue;
			selected.engine = engine->index; selected.unit = std::get<0>(quote).GetCost(); selected.capacity = std::get<2>(quote);
		}
		REQUIRE(selected.engine != EngineID::Invalid()); result.gross += selected.unit;
		fmt::print("SURFACE_ROAD_RENEWAL_SELECTED role={} model={} capacity={} unit={}\n", index, selected.engine.base(), selected.capacity, selected.unit.base());
	}
	const auto *company = Company::Get(_current_company); const auto shortfall = std::max(Money{0}, result.gross + Money{150000} - company->money);
	result.credit = Money{((shortfall.base() + 9999) / 10000) * 10000};
	if (result.credit > 0) { auto bank = QuoteSurfaceRoadRenewal<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, result.credit); REQUIRE(bank.Succeeded()); }
	REQUIRE(company->current_loan + result.credit <= company->GetMaxLoan()); REQUIRE(company->money + result.credit - result.gross >= 150000);
	fmt::print("SURFACE_ROAD_RENEWAL_PLAN gross={} credit={} reserve=150000 cash={} debt={} ceiling={} depot={} trucks=2 future_refund=0\n", result.gross.base(), result.credit.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base(), surface_road_renewal_depot.base()); return result;
}

/** Query native delivery-preserving empty return for both real useful cargo services. */
static void QuerySurfaceRoadRenewal()
{
	QuoteSurfaceRoadFleet(); const auto *depot = Depot::GetByTile(surface_road_renewal_depot); REQUIRE(depot != nullptr);
	for (const auto *truck : SurfaceRoadFreight()) {
		REQUIRE(truck->GetNumOrders() == 2); VerifySurfaceRoadOrders(*truck, OrderLoadType::FullLoad);
		auto no_load = QuoteSurfaceRoadRenewal<Commands::ModifyOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::NoLoad)); REQUIRE(no_load.Succeeded());
		Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		auto halt = QuoteSurfaceRoadRenewal<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{2}, home); REQUIRE(halt.Succeeded());
		fmt::print("SURFACE_ROAD_RENEWAL_RECALL_PLAN vehicle={} held={} depot={} no_new_load=true return_admitted=true\n", truck->index.base(), truck->cargo.TotalCount(), surface_road_renewal_depot.base());
	}
}

/** Suppress only new pickup loading, preserve actual receiver unload, and append a real owned-depot halt. */
static void RecallSurfaceRoadFreight()
{
	const auto *depot = Depot::GetByTile(surface_road_renewal_depot); REQUIRE(depot != nullptr);
	for (const auto *truck : SurfaceRoadFreight()) {
		REQUIRE(truck->GetNumOrders() == 2); VerifySurfaceRoadOrders(*truck, OrderLoadType::FullLoad);
		PaySurfaceRoadRenewal<Commands::ModifyOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::NoLoad));
		Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		PaySurfaceRoadRenewal<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{2}, home); VerifySurfaceRoadOrders(*truck, OrderLoadType::NoLoad);
	}
}

/** @return True only after genuine cargo delivery and real empty halted arrival in the original owned depot. */
static bool SurfaceRoadFreightEmptyHome()
{
	for (const auto *truck : SurfaceRoadFreight()) {
		// Empty cargo and actual depot arrival jointly authorize sale; a
		// planned halt or loaded vehicle cannot stand in for useful delivery.
		if (!truck->IsStoppedInDepot() || truck->tile != surface_road_renewal_depot || truck->cargo.TotalCount() != 0) return false;
	}
	return true;
}

/** Pay admitted empty replacements, preserve native service orders, and sell old trucks only after real empty return. */
static void RenewSurfaceRoadFreight()
{
	REQUIRE(SurfaceRoadFreightEmptyHome()); const auto quote = QuoteSurfaceRoadFleet(); const auto *company = Company::Get(_current_company);
	const auto cash = company->money, debt = company->current_loan; Money total{0};
	if (quote.credit > 0) PaySurfaceRoadRenewal<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, quote.credit);
	const auto trucks = SurfaceRoadFreight();
	for (uint index = 0; index < trucks.size(); ++index) {
		const auto *old = trucks[index]; const auto &selected = quote.carriers[index]; const auto name = old->name; const auto old_id = old->index;
		REQUIRE(old->GetNumOrders() == 3); VerifySurfaceRoadOrders(*old, OrderLoadType::NoLoad);
		const auto paid = PaySurfaceRoadRenewal<Commands::BuildVehicle>(surface_road_renewal_depot, surface_road_renewal_depot, selected.engine, false, old->cargo_type, ClientID::Server);
		total += std::get<0>(paid).GetCost(); const auto *truck = RoadVehicle::Get(std::get<1>(paid)); REQUIRE(truck->IsStoppedInDepot()); REQUIRE(truck->cargo.TotalCount() == 0); REQUIRE(truck->cargo_cap == selected.capacity);
		PaySurfaceRoadRenewal<Commands::CloneOrder>(TileIndex{0}, CO_COPY, truck->index, old_id); PaySurfaceRoadRenewal<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{2});
		PaySurfaceRoadRenewal<Commands::ModifyOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::FullLoad)); VerifySurfaceRoadOrders(*truck, OrderLoadType::FullLoad); REQUIRE(truck->GetNumOrders() == 2);
		auto sold = PaySurfaceRoadRenewal<Commands::SellVehicle>(TileIndex{0}, old_id, false, false, ClientID::Server); total += sold.GetCost();
		PaySurfaceRoadRenewal<Commands::RenameVehicle>(TileIndex{0}, truck->index, name); PaySurfaceRoadRenewal<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true);
		fmt::print("SURFACE_ROAD_RENEWAL_RENEWED old={} vehicle={} model={} capacity={} age={} orders={} empty=true refund={}\n", old_id.base(), truck->index.base(), truck->engine_type.base(), truck->cargo_cap, truck->age.base(), truck->GetNumOrders(), -sold.GetCost().base());
	}
	REQUIRE(company->money == cash + quote.credit - total); REQUIRE(company->current_loan == debt + quote.credit); REQUIRE(company->money >= 150000);
}

/** Genuine useful road renewal and bounded service preserve every native financial/material owner. */
TEST_CASE("Useful road freight renews only after real empty depot return", "[.surface-entry-road-renewal]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_ROAD_PHASE"); const std::string phase = selected == nullptr ? "query" : selected;
	const std::array<std::string_view, 5> allowed{"query", "recall", "work", "renew", "earn"}; REQUIRE(std::ranges::find(allowed, phase) != allowed.end());
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-road-renewal"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve original failures through ordinary shutdown without compensating gameplay changes. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company)); SaveSurfaceRoadRenewal(paths.saves / ("surface-road-before-" + phase + "-440.sav"));
			if (phase == "query") QuerySurfaceRoadRenewal();
			else if (phase == "recall") RecallSurfaceRoadFreight();
			else if (phase == "work") EarnSurfaceEntryWork("SURFACE_ROAD_RENEWAL", 3000, SurfaceRoadFreightEmptyHome);
			else if (phase == "renew") RenewSurfaceRoadFreight();
			else {
				for (const auto *truck : SurfaceRoadFreight()) { REQUIRE(truck->GetNumOrders() == 2); VerifySurfaceRoadOrders(*truck, OrderLoadType::FullLoad); REQUIRE(truck->age < truck->max_age); }
				/** @return False; actual3000native ticks own this bounded continuation. */
				EarnSurfaceEntryWork("SURFACE_ROAD_RENEWAL", 3000, [] { return false; });
			}
			for (const auto *truck : SurfaceRoadFreight()) fmt::print("SURFACE_ROAD_RENEWAL_CUSTODY vehicle={} model={} capacity={} age={} remaining={} held={} tile={} home={} orders={}\n", truck->index.base(), truck->engine_type.base(), truck->cargo_cap, truck->age.base(), (truck->max_age - truck->age).base(), truck->cargo.TotalCount(), truck->tile.base(), truck->IsStoppedInDepot(), truck->GetNumOrders());
			SaveSurfaceRoadRenewal(paths.saves / ("surface-road-" + phase + "-440.sav")); fmt::print("SURFACE_ROAD_RENEWAL_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_road_renewal_observer, &inspect); IConsole::CmdRegister("surface_entry_road_renewal", ObserveSurfaceRoadRenewal);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
