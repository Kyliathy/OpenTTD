/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_fleet.cpp Paid empty conventional trucks for genuine Surface production.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BD.2.
 * Ordinary purchases, exact cargo capability and legal orders create loading
 * demand. Original game loops must produce every subsequent physical packet.
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
#include "../roadveh.h"
#include "../order_cmd.h"
#include "../order_base.h"
#include "../underground_vehicle.h"
#include "../depot_base.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../cargopacket.h"
#include "../industry.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../safeguards.h"

static std::function<void()> *surface_fleet_observer = nullptr; ///< Original loaded-world callback, not a vehicle or finance authority.

/** Dispatch the current actual loaded game's purchase observer.
 * @param args Parameter-free registered console command.
 * @return True after invoking the current observer.
 */
static bool ObserveSurfaceFleet(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_fleet_observer != nullptr);
	(*surface_fleet_observer)(); return true;
}

/** Save actual paid native hulls/orders without a simulation tick.
 * @param path Fresh invocation-owned output.
 */
static void SaveSurfaceFleet(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Execute one original synchronized command with normal financial settlement.
 * @tparam Tcmd Original command identity.
 * @tparam Targs Original typed command arguments.
 * @param args Native world, vehicle and order identities, never fabricated owners.
 * @return Actual original command result, including any new hull identity.
 */
template <Commands Tcmd, typename... Targs>
static auto SurfaceFreightCommand(Targs... args)
{
	auto result = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false,
		TileIndex{0}, std::make_tuple(args...));
	INFO("freight command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(result).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(result).Succeeded()); return result;
}

/** Query a whole actual truck purchase within the native command kernel's estimate scope.
 * @param depot Original paid company depot.
 * @param engine Actual available original definition.
 * @param cargo Requested supported cargo, including a genuine refit when needed.
 * @return Pure native whole invoice; temporary refit vehicles cannot affect finance.
 */
static CommandCost QuoteSurfaceProductionTruck(TileIndex depot, EngineID engine, CargoType cargo)
{
	const auto *company = Company::Get(_current_company); const auto cash = company->money;
	const auto expenses = company->yearly_expenses;
	// BuildVehicle's refit estimate temporarily creates/sells a native hull.
	// The original command kernel must own nesting so that its internal sale
	// cannot become a top-level refund to this genuine company.
	const auto quote = Command<Commands::BuildVehicle>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true,
		depot, std::make_tuple(depot, engine, false, cargo, ClientID::Server));
	CHECK(company->money == cash); CHECK(company->yearly_expenses == expenses);
	return std::get<0>(quote);
}

/** Select an actually available original truck whose native default cargo matches the required product.
 * @param depot Exact normally paid road depot.
 * @param cargo Current configured original cargo slot.
 * @return Buildable original engine identity, never an altered catalogue/refit mask.
 */
static EngineID SelectOriginalSurfaceTruck(TileIndex depot, CargoType cargo)
{
	// Original Steel/Goods trucks complement the native ore-only bulk model.
	// Exact default cargo, normal availability and a real purchase quote must
	// all agree; a same-class incompatible carrier cannot gain a free refit.
	for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
		if (engine->info.cargo_type != cargo) continue;
		if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
		if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
		if (QuoteSurfaceProductionTruck(depot, engine->index, cargo).Succeeded()) return engine->index;
	}
	return EngineID::Invalid();
}

/** Buy and normally order one actual empty truck.
 * @param depot Actual command-built depot.
 * @param engine Original available native engine identity.
 * @param cargo Exact ordinary cargo meaning carried by this purchased unit.
 * @param pickup Real original loading station.
 * @param receiver Actual useful receiving station, or Invalid to preserve a full load in the depot.
 * @param name Persistent cosmetic discovery name; no secondary saved identity registry.
 * @return Actual normally paid primary vehicle.
 */
static Vehicle *BuySurfaceProductionTruck(TileIndex depot, EngineID engine, CargoType cargo, StationID pickup, StationID receiver, std::string name)
{
	const auto *company = Company::Get(_current_company); const auto cash = company->money;
	const auto quote = QuoteSurfaceProductionTruck(depot, engine, cargo); REQUIRE(quote.Succeeded());
	const auto [paid, id, capacity, mail, refit] = SurfaceFreightCommand<Commands::BuildVehicle>(depot, engine, false, cargo, ClientID::Server);
	CHECK(paid.GetCost() == quote.GetCost()); CHECK(company->money == cash - paid.GetCost());
	auto *vehicle = Vehicle::GetIfValid(id); REQUIRE(vehicle != nullptr); REQUIRE(vehicle->IsPrimaryVehicle());
	CHECK(vehicle->type == VehicleType::Road); CHECK(vehicle->cargo_type == cargo); CHECK(vehicle->cargo.TotalCount() == 0);
	REQUIRE(vehicle->cargo_cap != 0); CHECK(vehicle->cargo_cap == capacity); CHECK(vehicle->owner == _local_company);
	SurfaceFreightCommand<Commands::RenameVehicle>(id, name);
	Order collect; collect.MakeGoToStation(pickup); collect.SetUnloadType(OrderUnloadType::NoUnload);
	collect.SetLoadType(OrderLoadType::FullLoad); collect.SetStopLocation(OrderStopLocation::FarEnd);
	SurfaceFreightCommand<Commands::InsertOrder>(id, VehicleOrderID{0}, collect);
	Order next;
	if (receiver != StationID::Invalid()) {
		next.MakeGoToStation(receiver); next.SetLoadType(OrderLoadType::NoLoad); next.SetStopLocation(OrderStopLocation::FarEnd);
	} else {
		// Useful newly manufactured products return to the actual depot in
		// physical truck packets. Halting inside clears loading bays for the
		// rest of this chain while preserving stock for later launch entry.
		next.MakeGoToDepot(Depot::GetByTile(depot)->index, OrderDepotTypeFlag::PartOfOrders,
			OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
	}
	SurfaceFreightCommand<Commands::InsertOrder>(id, VehicleOrderID{1}, next);
	SurfaceFreightCommand<Commands::StartStopVehicle>(id, false);
	CHECK(vehicle->cargo.TotalCount() == 0);
	fmt::print("SURFACE_ENTRY_TRUCK_PAID id={} engine={} cargo={} capacity={} invoice={} pickup={} receiver={} name={}\n",
		id.base(), engine.base(), cargo, vehicle->cargo_cap, paid.GetCost().base(), pickup.base(), receiver.base(), name);
	return vehicle;
}

/** Actual empty purchases and legal ordinary orders establish this world's ore/Steel/Goods demand without supplied inventory. */
TEST_CASE("Authentic Surface production buys empty original carriers and legal loading orders", "[.surface-entry-fleet]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-fleet");
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Propagate original native assertions through normal shutdown, without compensating grants. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			const auto *company = Company::Get(_current_company); const auto cash = company->money;
			REQUIRE(cash == 743352); REQUIRE(company->current_loan == 2000000); REQUIRE(CargoPacket::GetNumItems() == 0);
			REQUIRE(Depot::GetNumItems() == 1); const auto depot = (*Depot::Iterate().begin())->xy;
			const auto iron = GetCargoTypeByLabel(CargoLabel{"IORE"}), steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), goods = GetCargoTypeByLabel(CargoLabel{"GOOD"});
			const auto bulk = GetUndergroundEngine(7); REQUIRE(bulk != EngineID::Invalid());
			// Preserve exact native catalogue permissions. Existing original
			// product trucks supply this ordinary Surface route; extending the
			// bulk/sealed mining model to new cargoes is a separate content change.
			CHECK(CheckUndergroundVehicleCapability(bulk, MapLayer::Surface, iron).Succeeded());
			CHECK(CheckUndergroundVehicleCapability(bulk, MapLayer::Surface, steel).Failed());
			CHECK(CheckUndergroundVehicleCapability(GetUndergroundEngine(8), MapLayer::Surface, goods).Failed());
			const auto steel_engine = SelectOriginalSurfaceTruck(depot, steel), goods_engine = SelectOriginalSurfaceTruck(depot, goods);
			REQUIRE(steel_engine != EngineID::Invalid()); REQUIRE(goods_engine != EngineID::Invalid());
			REQUIRE(Station::IsValidID(StationID{0})); REQUIRE(Station::IsValidID(StationID{1})); REQUIRE(Station::IsValidID(StationID{2}));
			SaveSurfaceFleet(paths.saves / "surface-entry-before-fleet-440.sav");
			BuySurfaceProductionTruck(depot, bulk, iron, StationID{0}, StationID{1}, "Surface ore delivery");
			BuySurfaceProductionTruck(depot, steel_engine, steel, StationID{1}, StationID{2}, "Surface Steel delivery");
			BuySurfaceProductionTruck(depot, steel_engine, steel, StationID{1}, StationID::Invalid(), "Surface Steel reserve 1");
			BuySurfaceProductionTruck(depot, steel_engine, steel, StationID{1}, StationID::Invalid(), "Surface Steel reserve 2");
			BuySurfaceProductionTruck(depot, goods_engine, goods, StationID{2}, StationID::Invalid(), "Surface Goods reserve");
			CHECK(CargoPacket::GetNumItems() == 0); CHECK(company->current_loan == 2000000); CHECK(Industry::GetNumItems() == 12);
			SaveSurfaceFleet(paths.saves / "surface-entry-fleet-440.sav");
			fmt::print("SURFACE_ENTRY_FLEET_COMPLETE paid=5 expense={} cash={} debt={} supplied_stock=false\n",
				(cash - company->money).base(), company->money.base(), company->current_loan.base()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_fleet_observer, &inspect); IConsole::CmdRegister("surface_entry_fleet", ObserveSurfaceFleet);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
