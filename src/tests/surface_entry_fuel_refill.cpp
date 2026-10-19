/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_fuel_refill.cpp Complete admission for additional genuinely produced Propellant.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEH.
 * The original world, commands and native game loop retain every financial,
 * material, construction, routing and vehicle-position owner.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../industry.h"
#include "../industry_cmd.h"
#include "../industry_map.h"
#include "../layer_industry_processor.h"
#include "../multilayer_recipe.h"
#include "../multilayer_industry.h"
#include "../multilayer_space_content.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../underground_vehicle.h"
#include "../vehicle_cmd.h"
#include "../depot_base.h"
#include "../roadveh.h"
#include "../road_map.h"
#include "../train.h"
#include "../station_base.h"
#include "../order_cmd.h"
#include "../misc_cmd.h"
#include "../openttd.h"
#include "../timer/timer_game_tick.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_entry_work.h"
#include <queue>
#include "../safeguards.h"

static std::function<void()> *surface_fuel_refill_observer = nullptr; ///< Invocation callback, never a material or money owner.
static constexpr TileIndex surface_fuel_refill_site{1639}, surface_fuel_refill_depot{2443};

/** @param args Parameter-free original console arguments. @return True after observing the actual loaded world. */
static bool ObserveSurfaceFuelRefill(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_fuel_refill_observer != nullptr); (*surface_fuel_refill_observer)(); return true;
}

/** @param path Fresh invocation-owned output. Preserve original native owners without earning work. */
static void SaveSurfaceFuelRefill(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Query one original complete synchronized command without changing its owners.
 * @tparam Tcmd Original command identity. @tparam Targs Native argument types.
 * @param tile Actual tiled action, or zero for coordinate-free management.
 * @param args Whole original arguments. @return Native quote or refusal.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceFuelRefill(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** Freshly quote and execute one original whole native purchase or management action.
 * @tparam Tcmd Original command identity. @tparam Targs Native argument types.
 * @param tile Actual tiled action, or zero for coordinate-free management.
 * @param args Complete original arguments. @return Settled original command result.
 */
template <Commands Tcmd, typename... Targs>
static auto PaySurfaceFuelRefill(TileIndex tile, Targs... args)
{
	auto quote = QuoteSurfaceFuelRefill<Tcmd>(tile, args...);
	INFO("Propellant quote=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(quote).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(quote).Succeeded());
	auto paid = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(args...));
	REQUIRE(ExtractCommandCost(paid).Succeeded()); REQUIRE(ExtractCommandCost(paid).GetCost() == ExtractCommandCost(quote).GetCost());
	fmt::print("SURFACE_FUEL_REFILL_INVOICE command={} tile={} cost={}\n", GetCommandName(Tcmd), tile.base(), ExtractCommandCost(paid).GetCost().base());
	return paid;
}

/** @return Sole original loaded Source15chemical tanker, following its real name instead of a historical pool slot. */
static const RoadVehicle *FindSurfaceFuelRefillFeed()
{
	const RoadVehicle *result = nullptr;
	// Native identity and packet source protect the paid useful cargo even if
	// ordinary vehicle renewal has legitimately reused another pool slot.
	for (const auto *truck : RoadVehicle::Iterate()) {
		if (truck->name != "Surface Chemical reserve") continue;
		REQUIRE(result == nullptr); REQUIRE(truck->owner == _current_company); result = truck;
	}
	REQUIRE(result != nullptr); REQUIRE(result->IsStoppedInDepot()); REQUIRE(result->tile == surface_fuel_refill_depot);
	REQUIRE(result->cargo_type == GetCargoTypeByLabel(CargoLabel{"CHEM"})); REQUIRE(result->cargo.TotalCount() == 21);
	for (const auto *packet : *result->cargo.Packets()) {
		REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == 15);
	}
	return result;
}

/** Inspect connectivity in the actual existing road geometry, without purchasing or becoming a routing authority.
 * @param target Existing original road bay to reach from the paid road depot.
 * @return Number of level crossings on a connected original physical path.
 */
static uint QuerySurfaceFuelRefillRoad(TileIndex target)
{
	std::map<TileIndex, TileIndex> parent; std::queue<TileIndex> pending;
	parent.emplace(surface_fuel_refill_depot, surface_fuel_refill_depot); pending.push(surface_fuel_refill_depot);
	// Only present mutually connected native road bits can admit this query.
	// Empty land, a proposed road or an adjacent storage plane supplies no edge.
	while (!pending.empty()) {
		const auto tile = pending.front(); pending.pop(); if (tile == target) break;
		const auto bits = GetAnyRoadBits(tile, RoadTramType::Road);
		for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			if ((bits & DiagDirToRoadBits(direction)) == RoadBits{}) continue;
			const auto next = AddTileIndexDiffCWrap(tile, TileIndexDiffCByDiagDir(direction));
			// Every edge must remain on the actual Surface and connect back;
			// this is geometric characterization, not a manufactured vehicle pose.
			if (!MultiLayerMap::IsTileInLayer(next, MapLayer::Surface) || parent.contains(next)) continue;
			if ((GetAnyRoadBits(next, RoadTramType::Road) & DiagDirToRoadBits(ReverseDiagDir(direction))) == RoadBits{}) continue;
			parent.emplace(next, tile); pending.push(next);
		}
	}
	REQUIRE(parent.contains(target)); uint crossings = 0, length = 0;
	// Reconstruct only existing paid/public geometry for observation. The
	// original road pathfinder alone will choose the later actual movement.
	for (auto tile = target; tile != surface_fuel_refill_depot; tile = parent.at(tile)) {
		++length;
		if (IsLevelCrossingTile(tile)) ++crossings;
	}
	fmt::print("SURFACE_FUEL_REFILL_ROUTE target={} edges={} crossings={} infrastructure=0\n", target.base(), length, crossings);
	return crossings;
}

/** Whole original empty carrier quote; owns no vehicle, money, input or product. */
struct SurfaceFuelRefillQuote {
	EngineID engine = EngineID::Invalid(); ///< Cheapest actually admitted original PROP carrier.
	Money vehicle{0}, factory{0}, credit{0}; ///< Fresh native whole costs and matching required bank credit.
	uint capacity = 0; ///< Real quoted useful carrying capacity.
};

/** @return Cheapest actually admitted useful empty original PROP carrier at the existing real road depot. */
static SurfaceFuelRefillQuote QuoteSurfaceFuelRefillCarrier()
{
	SurfaceFuelRefillQuote result;
	const auto prop = GetCargoTypeByLabel(CargoLabel{"PROP"}); REQUIRE(IsValidCargoType(prop));
	for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
		if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
		auto quote = QuoteSurfaceFuelRefill<Commands::BuildVehicle>(surface_fuel_refill_depot, surface_fuel_refill_depot, engine->index, false, prop, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		if (std::get<2>(quote) < 22) continue;
		if (result.engine != EngineID::Invalid()) {
			if (result.vehicle <= std::get<0>(quote).GetCost()) continue;
		}
		result.engine = engine->index; result.vehicle = std::get<0>(quote).GetCost(); result.capacity = std::get<2>(quote);
	}
	REQUIRE(result.engine != EngineID::Invalid());
	return result;
}


/** Query the complete current original feed, conversion, useful pickup and crossing-protection arrangement. */
static void QuerySurfaceFuelRefill()
{
	const auto *company = Company::Get(_current_company); const auto *feed = FindSurfaceFuelRefillFeed();
	const auto *industry = Industry::GetByTile(surface_fuel_refill_site); REQUIRE(industry != nullptr); REQUIRE(IsLayerIndustryComplete(*industry));
	const auto *state = GetLayerProcessorState(industry->index); REQUIRE(state != nullptr); REQUIRE(state->recipe == MultiLayerRecipeID{"PCHM"});
	REQUIRE(state->consumed.at(CargoLabel{"CHEM"}) == 20); REQUIRE(state->produced.at(CargoLabel{"PROP"}) == 20); REQUIRE(state->dispatched == 20);
	const auto chemical = GetCargoTypeByLabel(CargoLabel{"CHEM"});
	const auto slot = std::ranges::find(industry->accepted, chemical, &Industry::AcceptedCargo::cargo); REQUIRE(slot != industry->accepted.end()); REQUIRE(slot->waiting == 1);
	const auto quote = QuoteSurfaceFuelRefillCarrier(); uint crossings = 0;
	// Original input and output bays retain separate physical roles. Existing
	// catchment/roads and complete native orders must admit both real trips.
	for (const auto bay : {TileIndex{1895}, TileIndex{1896}}) {
		const auto *station = Station::GetByTile(bay); REQUIRE(station != nullptr); REQUIRE(station->owner == _current_company);
		bool covers = false; for (const auto tile : industry->location) if (station->TileIsInCatchment(tile)) covers = true; REQUIRE(covers);
		crossings += QuerySurfaceFuelRefillRoad(bay);
		Order order; order.MakeGoToStation(station->index); order.SetStopLocation(OrderStopLocation::FarEnd);
		order.SetLoadType(bay == TileIndex{1895} ? OrderLoadType::NoLoad : OrderLoadType::LoadIfPossible);
		if (bay == TileIndex{1896}) order.SetUnloadType(OrderUnloadType::NoUnload);
		auto admitted = QuoteSurfaceFuelRefill<Commands::InsertOrder>(TileIndex{0}, feed->index, VehicleOrderID{0}, order); REQUIRE(admitted.Succeeded()); REQUIRE(admitted.GetCost() == 0);
		fmt::print("SURFACE_FUEL_REFILL_BAY tile={} station={} catchment=true order_admitted=true pickup={} full_load=false\n", bay.base(), station->index.base(), bay == TileIndex{1896});
	}
	uint traffic = 0;
	// Actual appended original depot halts preserve both commercial orders;
	// all three trains must really arrive before valuable chemical/fuel trips.
	for (const auto *train : Train::Iterate()) {
		if (!train->IsFrontEngine()) continue;
		if (!train->name.starts_with("Surface Income ")) continue;
		const std::map<std::string, TileIndex> depots{{"Surface Income farm", TileIndex{1638}}, {"Surface Income goods", TileIndex{3224}}, {"Surface Income coal", TileIndex{3161}}};
		const auto *depot = Depot::GetByTile(depots.at(train->name)); REQUIRE(depot != nullptr); REQUIRE(train->GetNumOrders() == 2);
		Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		auto halt = QuoteSurfaceFuelRefill<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{2}, home); REQUIRE(halt.Succeeded()); REQUIRE(halt.GetCost() == 0); ++traffic;
	}
	REQUIRE(traffic == 3); const Money reserve{100000}; const auto shortfall = std::max(Money{0}, quote.vehicle + reserve - company->money);
	const Money credit{((shortfall.base() + 9999) / 10000) * 10000};
	if (credit > 0) { auto bank = QuoteSurfaceFuelRefill<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit); REQUIRE(bank.Succeeded()); }
	REQUIRE(company->current_loan + credit <= company->GetMaxLoan()); REQUIRE(company->money + credit - quote.vehicle >= reserve);
	const auto &budget = GetMultiLayerSpaceBudget(); const uint required = 2 * (uint{budget.outbound_fuel} + budget.recovery_fuel); REQUIRE(required == 30);
	fmt::print("SURFACE_FUEL_REFILL_PLAN industry={} external_chemicals=42 feed=21 input=1 consumed=20 original_fuel=20 expected_additional=22 minimum_additional=10 flight_fuel={} model={} capacity={} gross={} protected_trains={} route_crossings={} reserve=100000 credit={} cash={} debt={} ceiling={} infrastructure=0 full_load=false output_refusal_retains_buffer=true\n",
		industry->index.base(), required, quote.engine.base(), quote.capacity, quote.vehicle.base(), traffic, crossings, credit.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base());
}


/** @return All three original named income trains and their actual existing paid depots. */
static std::vector<std::pair<const Train *, TileIndex>> SurfaceFuelRefillTraffic()
{
	const std::map<std::string, TileIndex> depots{{"Surface Income farm", TileIndex{1638}}, {"Surface Income goods", TileIndex{3224}}, {"Surface Income coal", TileIndex{3161}}};
	std::vector<std::pair<const Train *, TileIndex>> result;
	// Cosmetic discovery names follow the actual native renewed fronts;
	// reused old pool slots cannot become phantom crossing protection.
	for (const auto *train : Train::Iterate()) {
		if (!train->IsFrontEngine()) continue;
		if (!train->name.starts_with("Surface Income ")) continue;
		REQUIRE(train->owner == _current_company); result.emplace_back(train, depots.at(train->name));
	}
	REQUIRE(result.size() == 3); return result;
}

/** @return True only after every real income train has arrived empty and halted in its original depot. */
static bool SurfaceFuelRefillTrafficProtected()
{
	for (const auto &[train, depot] : SurfaceFuelRefillTraffic()) {
		// Cargo delivery and true depot arrival both precede protection;
		// a temporary order or nearby stopped train cannot clear the crossing.
		if (!train->IsStoppedInDepot() || train->tile != depot) return false;
		for (const auto *part = train; part != nullptr; part = part->Next()) if (part->cargo.TotalCount() != 0) return false;
	}
	return true;
}

/** Preserve both commercial orders and genuinely recall each original income train through its normal receiver. */
static void RecallSurfaceFuelRefillTraffic()
{
	for (const auto &[train, tile] : SurfaceFuelRefillTraffic()) {
		REQUIRE(train->GetNumOrders() == 2); const auto *depot = Depot::GetByTile(tile); REQUIRE(depot != nullptr);
		Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		PaySurfaceFuelRefill<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{2}, home);
		if (train->cur_real_order_index != 1) PaySurfaceFuelRefill<Commands::SkipToOrder>(TileIndex{0}, train->index, VehicleOrderID{1});
		fmt::print("SURFACE_FUEL_REFILL_RECALL head={} depot={} orders=3 supplied_pose=false\n", train->index.base(), tile.base());
	}
}


/** @param name Actual cosmetic native role. @param required Whether absence is an invalid boundary. @return Original owned road carrier, or nullptr before its paid purchase. */
static const RoadVehicle *FindSurfaceFuelRefillTruck(std::string_view name, bool required = true)
{
	const RoadVehicle *result = nullptr;
	// Native names discover current owners across legitimate pool reuse;
	// a historical ID cannot substitute for another actual carrier role.
	for (const auto *truck : RoadVehicle::Iterate()) {
		if (truck->name != name) continue;
		REQUIRE(result == nullptr); REQUIRE(truck->owner == _current_company); result = truck;
	}
	if (required) REQUIRE(result != nullptr);
	return result;
}
/** @return Current native CHEM waiting amount in the actual second PCHM industry. */
static uint SurfaceFuelRefillChemicalWaiting()
{
	const auto *industry = Industry::GetByTile(surface_fuel_refill_site); REQUIRE(industry != nullptr);
	const auto cargo = GetCargoTypeByLabel(CargoLabel{"CHEM"});
	const auto slot = std::ranges::find(industry->accepted, cargo, &Industry::AcceptedCargo::cargo);
	REQUIRE(slot != industry->accepted.end()); return slot->waiting;
}

/** @param truck Actual owned original road carrier. @param pickup True for normal non-full PROP collection, false for external CHEM delivery. */
static void OrderSurfaceFuelRefillTruck(const RoadVehicle &truck, bool pickup)
{
	REQUIRE(SurfaceFuelRefillTrafficProtected()); REQUIRE(truck.IsStoppedInDepot()); REQUIRE(truck.tile == surface_fuel_refill_depot);
	// The two real stops serve distinct transfer directions. Original orders
	// alone unload external CHEM or collect already produced PROP; neither a
	// handoff helper nor a full-load wait can manufacture the missing seven units.
	while (truck.GetNumOrders() != 0) PaySurfaceFuelRefill<Commands::DeleteOrder>(TileIndex{0}, truck.index, VehicleOrderID{0});
	Order serve; serve.MakeGoToStation(GetStationIndex(pickup ? TileIndex{1896} : TileIndex{1895})); serve.SetStopLocation(OrderStopLocation::FarEnd);
	serve.SetLoadType(pickup ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad);
	if (pickup) serve.SetUnloadType(OrderUnloadType::NoUnload);
	PaySurfaceFuelRefill<Commands::InsertOrder>(TileIndex{0}, truck.index, VehicleOrderID{0}, serve);
	Order home; home.MakeGoToDepot(Depot::GetByTile(surface_fuel_refill_depot)->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
	PaySurfaceFuelRefill<Commands::InsertOrder>(TileIndex{0}, truck.index, VehicleOrderID{1}, home);
	PaySurfaceFuelRefill<Commands::StartStopVehicle>(TileIndex{0}, truck.index, true);
	fmt::print("SURFACE_FUEL_REFILL_ROAD_ORDER vehicle={} pickup={} station={} starting_held={}\n", truck.index.base(), pickup, serve.GetDestination().base(), truck.cargo.TotalCount());
}

/** @return Actual empty returned chemical carrier and all42external units received/consumed by the new factory. */
static bool SurfaceFuelRefillChemicalsDelivered()
{
	const auto *truck = FindSurfaceFuelRefillTruck("Surface Chemical reserve");
	const auto *industry = Industry::GetByTile(surface_fuel_refill_site); const auto *state = GetLayerProcessorState(industry->index);
	// Delivery is complete only after the real truck has unloaded and returned;
	// processor consumption cannot hide a missing original external input.
	return truck->IsStoppedInDepot() && truck->tile == surface_fuel_refill_depot && truck->cargo.TotalCount() == 0 &&
		SurfaceFuelRefillChemicalWaiting() + state->consumed.at(CargoLabel{"CHEM"}) == 42 && state->produced.at(CargoLabel{"PROP"}) == 42;
}

/** Buy only the fresh admitted empty carrier after actual useful product exists. */
static void BuySurfaceFuelRefillCarrier()
{
	REQUIRE(SurfaceFuelRefillTrafficProtected());
	const auto *industry = Industry::GetByTile(surface_fuel_refill_site); const auto *state = GetLayerProcessorState(industry->index);
	REQUIRE(state->produced.at(CargoLabel{"PROP"}) == 42); REQUIRE(state->consumed.at(CargoLabel{"CHEM"}) == 42);
	const auto quote = QuoteSurfaceFuelRefillCarrier(); const auto cash = Company::Get(_current_company)->money;
	REQUIRE(Company::Get(_current_company)->money - quote.vehicle >= 100000);
	auto paid = PaySurfaceFuelRefill<Commands::BuildVehicle>(surface_fuel_refill_depot, surface_fuel_refill_depot, quote.engine, false, GetCargoTypeByLabel(CargoLabel{"PROP"}), ClientID::Server);
	const auto *truck = RoadVehicle::Get(std::get<1>(paid)); REQUIRE(truck->cargo.TotalCount() == 0); REQUIRE(truck->IsStoppedInDepot());
	REQUIRE(truck->cargo_cap == quote.capacity); REQUIRE(std::get<0>(paid).GetCost() == quote.vehicle);
	PaySurfaceFuelRefill<Commands::RenameVehicle>(TileIndex{0}, truck->index, std::string{"Surface Propellant refill"});
	REQUIRE(Company::Get(_current_company)->money == cash - quote.vehicle);
	fmt::print("SURFACE_FUEL_REFILL_CARRIER vehicle={} engine={} invoice={} capacity={} empty=true\n", truck->index.base(), quote.engine.base(), quote.vehicle.base(), truck->cargo_cap);
}

/** @return Whether at least10additional genuine Source6PROP units have reached actual halted depot custody. */
static bool SurfaceFuelRefillCollected()
{
	const auto *truck = FindSurfaceFuelRefillTruck("Surface Propellant refill");
	if (!truck->IsStoppedInDepot()) return false;
	if (truck->tile != surface_fuel_refill_depot) return false;
	if (truck->cargo.TotalCount() < 10) return false;
	const auto *industry = Industry::GetByTile(surface_fuel_refill_site);
	for (const auto *packet : *truck->cargo.Packets()) {
		REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == industry->index.base());
	}
	return true;
}

/** Remove only the temporary depot orders and normally resume all original commercial freight after safe fuel custody. */
static void ResumeSurfaceFuelRefillTraffic()
{
	REQUIRE(SurfaceFuelRefillCollected()); REQUIRE(SurfaceFuelRefillTrafficProtected());
	for (const auto &[train, depot] : SurfaceFuelRefillTraffic()) {
		REQUIRE(train->GetNumOrders() == 3); REQUIRE(train->tile == depot);
		PaySurfaceFuelRefill<Commands::DeleteOrder>(TileIndex{0}, train->index, VehicleOrderID{2});
		REQUIRE(train->GetNumOrders() == 2);
		PaySurfaceFuelRefill<Commands::StartStopVehicle>(TileIndex{0}, train->index, true);
		fmt::print("SURFACE_FUEL_REFILL_RESUMED head={} original_orders=2\n", train->index.base());
	}
}


/** Actual original additional-fuel commands and bounded native work preserve all material and financial owners. */
TEST_CASE("Authentic additional Propellant has complete original carrier and conversion admission", "[.surface-entry-fuel-refill]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_FUEL_REFILL_PHASE"); const std::string phase = selected == nullptr ? "query" : selected;
	const std::array<std::string_view, 10> allowed{"query", "recall", "traffic-work", "chemical-order", "chemical-work", "carrier", "collect-order", "collect-work", "resume", "earn"};
	REQUIRE(std::ranges::find(allowed, phase) != allowed.end());
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-fuel-refill"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve original failures through normal shutdown without compensating gameplay changes. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceFuelRefill(paths.saves / ("surface-fuel-refill-before-" + phase + "-440.sav"));
			if (phase == "query") QuerySurfaceFuelRefill();
			else if (phase == "recall") RecallSurfaceFuelRefillTraffic();
			else if (phase == "traffic-work") EarnSurfaceEntryWork("SURFACE_FUEL_REFILL", 3000, SurfaceFuelRefillTrafficProtected);
			else if (phase == "chemical-order") OrderSurfaceFuelRefillTruck(*FindSurfaceFuelRefillTruck("Surface Chemical reserve"), false);
			else if (phase == "chemical-work") {
				REQUIRE(SurfaceFuelRefillTrafficProtected()); EarnSurfaceEntryWork("SURFACE_FUEL_REFILL", 3000, SurfaceFuelRefillChemicalsDelivered); REQUIRE(SurfaceFuelRefillTrafficProtected());
			}
			else if (phase == "carrier") { REQUIRE(SurfaceFuelRefillChemicalsDelivered()); BuySurfaceFuelRefillCarrier(); }
			else if (phase == "collect-order") OrderSurfaceFuelRefillTruck(*FindSurfaceFuelRefillTruck("Surface Propellant refill"), true);
			else if (phase == "collect-work") {
				REQUIRE(SurfaceFuelRefillTrafficProtected()); EarnSurfaceEntryWork("SURFACE_FUEL_REFILL", 3000, SurfaceFuelRefillCollected); REQUIRE(SurfaceFuelRefillTrafficProtected());
			}
			else if (phase == "resume") { REQUIRE(SurfaceFuelRefillChemicalsDelivered()); ResumeSurfaceFuelRefillTraffic(); }
			else {
				REQUIRE(SurfaceFuelRefillCollected()); REQUIRE(SurfaceFuelRefillChemicalsDelivered());
				/** @return False; the exact original3000tick interval, not an installed profit or state, owns replay completion. */
				EarnSurfaceEntryWork("SURFACE_FUEL_REFILL", 3000, [] { return false; }); REQUIRE(SurfaceFuelRefillCollected());
			}
			const auto *feed = FindSurfaceFuelRefillTruck("Surface Chemical reserve"), *carrier = FindSurfaceFuelRefillTruck("Surface Propellant refill", false);
			const auto *state = GetLayerProcessorState(Industry::GetByTile(surface_fuel_refill_site)->index); REQUIRE(state != nullptr);
			fmt::print("SURFACE_FUEL_REFILL_CUSTODY feed={} feed_tile={} feed_halted={} input={} consumed={} produced={} carrier={} fuel={} fuel_tile={} fuel_halted={} traffic_protected={}\n", feed->cargo.TotalCount(), feed->tile.base(), feed->IsStoppedInDepot(), SurfaceFuelRefillChemicalWaiting(), state->consumed.at(CargoLabel{"CHEM"}), state->produced.at(CargoLabel{"PROP"}), carrier != nullptr, carrier == nullptr ? 0 : carrier->cargo.TotalCount(), carrier == nullptr ? 0 : carrier->tile.base(), carrier != nullptr && carrier->IsStoppedInDepot(), SurfaceFuelRefillTrafficProtected());
			SaveSurfaceFuelRefill(paths.saves / ("surface-fuel-refill-" + phase + "-440.sav"));
			fmt::print("SURFACE_FUEL_REFILL_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_fuel_refill_observer, &inspect); IConsole::CmdRegister("surface_entry_fuel_refill", ObserveSurfaceFuelRefill);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
