/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_goods.cpp Additional literal Surface-factory Goods in original paid road carriers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEF.
 * Original commands, vehicles, cargo packets, factory production and company
 * books retain every owner. The observer supplies no material or progress.
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
#include "../road_map.h"
#include "../industry.h"
#include "../station_base.h"
#include "../order_cmd.h"
#include "../misc_cmd.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_entry_work.h"
#include <queue>
#include "../safeguards.h"

static std::function<void()> *surface_goods_observer = nullptr; ///< Invocation callback, never a simulation owner.
static constexpr TileIndex surface_goods_depot{2443};
static constexpr std::string_view surface_goods_prefix = "Surface Launch Goods reserve ";

/** @param args Original parameter-free console arguments. @return True after observing this invocation. */
static bool ObserveSurfaceGoods(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_goods_observer != nullptr); (*surface_goods_observer)(); return true;
}

/** @param path Fresh invocation-owned output. Preserve all actual owners without earning work. */
static void SaveSurfaceGoods(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Query one complete original command without changing its owners.
 * @tparam Tcmd Original command. @tparam Targs Native argument types.
 * @param tile Actual action position. @param args Complete original arguments. @return Original quote or refusal.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceGoods(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** Freshly admit and execute one complete original purchase or management action.
 * @tparam Tcmd Original command. @tparam Targs Native argument types.
 * @param tile Actual action position. @param args Complete original arguments. @return Settled original result.
 */
template <Commands Tcmd, typename... Targs>
static auto PaySurfaceGoods(TileIndex tile, Targs... args)
{
	auto quote = QuoteSurfaceGoods<Tcmd>(tile, args...); REQUIRE(ExtractCommandCost(quote).Succeeded());
	auto paid = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(args...));
	REQUIRE(ExtractCommandCost(paid).Succeeded()); REQUIRE(ExtractCommandCost(paid).GetCost() == ExtractCommandCost(quote).GetCost());
	fmt::print("SURFACE_GOODS_INVOICE command={} tile={} cost={}\n", GetCommandName(Tcmd), tile.base(), ExtractCommandCost(paid).GetCost().base());
	return paid;
}

/** @return Original full-load source2 order; no forced quantity or bypass of factory/station ownership. */
static Order SurfaceGoodsPickupOrder()
{
	Order order; order.MakeGoToStation(StationID{2}); order.SetLoadType(OrderLoadType::FullLoad);
	order.SetUnloadType(OrderUnloadType::NoUnload); order.SetStopLocation(OrderStopLocation::FarEnd); return order;
}

/** @return Original halt order into the existing real paid road depot. */
static Order SurfaceGoodsReturnOrder()
{
	const auto *depot = Depot::GetByTile(surface_goods_depot); REQUIRE(depot != nullptr);
	Order order; order.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt); return order;
}

/** Read-only selected complete purchase; native capacity determines useful surplus. */
struct SurfaceGoodsBudget {
	EngineID engine = EngineID::Invalid(); ///< Actually available compatible original carrier.
	Money unit{0}, gross{0}, credit{0}; ///< Whole native empty purchases and necessary matching bank credit.
	uint capacity = 0, count = 0; ///< Actual quoted carrier capacity and enough full carriers for at least34Goods.
};

/** @return Complete current original carrier, physical route, source and funding arrangement, without purchases. */
static SurfaceGoodsBudget QuerySurfaceGoods()
{
	const auto cargo = GetCargoTypeByLabel(CargoLabel{"GOOD"}); REQUIRE(IsValidCargoType(cargo));
	const auto *station = Station::Get(StationID{2}); REQUIRE(station->owner == _current_company);
	const auto *factory = Industry::Get(IndustryID{11}); REQUIRE(factory->type == 6);
	bool covers = false;
	// The native paid stop must actually cover the literal Surface factory;
	// a commercial source5station cannot replace this selected production path.
	for (const auto tile : factory->location) if (station->TileIsInCatchment(tile)) covers = true;
	REQUIRE(covers); const TileIndex target = station->xy;
	std::map<TileIndex, TileIndex> parent; std::queue<TileIndex> pending;
	parent.emplace(surface_goods_depot, surface_goods_depot); pending.push(surface_goods_depot);
	// Admit only existing mutually connected Surface road edges. This query
	// never creates infrastructure or overrides the original road pathfinder.
	while (!pending.empty()) {
		const auto tile = pending.front(); pending.pop(); if (tile == target) break;
		const auto bits = GetAnyRoadBits(tile, RoadTramType::Road);
		for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			if ((bits & DiagDirToRoadBits(direction)) == RoadBits{}) continue;
			const auto next = AddTileIndexDiffCWrap(tile, TileIndexDiffCByDiagDir(direction));
			// Both canonical Surface membership and previous discovery constrain
			// this traversal; adjacent packed planes supply no road connection.
			if (!MultiLayerMap::IsTileInLayer(next, MapLayer::Surface) || parent.contains(next)) continue;
			if ((GetAnyRoadBits(next, RoadTramType::Road) & DiagDirToRoadBits(ReverseDiagDir(direction))) == RoadBits{}) continue;
			parent.emplace(next, tile); pending.push(next);
		}
	}
	REQUIRE(parent.contains(target)); uint crossings = 0, length = 0;
	// Existing physical crossings would require an independently admitted
	// traffic-protection producer before irreplaceable Goods can use this path.
	for (auto tile = target; tile != surface_goods_depot; tile = parent.at(tile)) {
		++length; if (IsLevelCrossingTile(tile)) ++crossings;
	}
	REQUIRE(crossings == 0);
	fmt::print("SURFACE_GOODS_ROUTE depot=2443 station=2 tile={} source=11 edges={} crossings={} infrastructure=0 catchment=true\n", target.base(), length, crossings);
	SurfaceGoodsBudget result;
	// Compare whole enough-carrier purchases, rather than assuming an old
	// model or choosing the cheapest unit with an uneconomic useful capacity.
	for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
		if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
		auto quote = QuoteSurfaceGoods<Commands::BuildVehicle>(surface_goods_depot, surface_goods_depot, engine->index, false, cargo, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		const uint capacity = std::get<2>(quote); if (capacity == 0) continue;
		const uint count = (34 + capacity - 1) / capacity; const auto gross = std::get<0>(quote).GetCost() * count;
		if (result.engine != EngineID::Invalid()) if (result.gross <= gross) continue;
		result.engine = engine->index; result.unit = std::get<0>(quote).GetCost(); result.gross = gross; result.capacity = capacity; result.count = count;
	}
	REQUIRE(result.engine != EngineID::Invalid()); REQUIRE(result.count <= 8);
	for (const auto order : {SurfaceGoodsPickupOrder(), SurfaceGoodsReturnOrder()}) {
		auto quote = QuoteSurfaceGoods<Commands::InsertOrder>(TileIndex{0}, VehicleID{6}, VehicleOrderID{0}, order); REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == 0);
	}
	const auto *company = Company::Get(_current_company); const Money reserve{100000};
	const auto shortfall = std::max(Money{0}, result.gross + reserve - company->money);
	result.credit = Money{((shortfall.base() + 9999) / 10000) * 10000};
	if (result.credit > 0) { auto bank = QuoteSurfaceGoods<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, result.credit); REQUIRE(bank.Succeeded()); }
	REQUIRE(company->current_loan + result.credit <= company->GetMaxLoan()); REQUIRE(company->money + result.credit - result.gross >= reserve);
	fmt::print("SURFACE_GOODS_PLAN model={} capacity={} carriers={} unit={} gross={} minimum=34 useful={} reserve=100000 credit={} cash={} debt={} ceiling={} orders_admitted=true full_load=true unsold_value_counted=false\n",
		result.engine.base(), result.capacity, result.count, result.unit.base(), result.gross.base(), result.count * result.capacity, result.credit.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base());
	return result;
}

/** @return All actual additionally bought named Goods trucks; original14Goods remain a separate protected owner. */
static std::vector<const RoadVehicle *> FindSurfaceGoods()
{
	std::vector<const RoadVehicle *> result;
	// Cosmetic role names discover only actually allocated carriers. Their
	// original cargo label and every packet's literal factory source must agree.
	for (const auto *truck : RoadVehicle::Iterate()) {
		if (!truck->name.starts_with(surface_goods_prefix)) continue;
		REQUIRE(truck->owner == _current_company); REQUIRE(truck->cargo_type == GetCargoTypeByLabel(CargoLabel{"GOOD"}));
		for (const auto *packet : *truck->cargo.Packets()) { REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == 11); }
		result.push_back(truck);
	}
	return result;
}

/** Buy only admitted empty original trucks and issue their real full-load/return orders. */
static void BuySurfaceGoods()
{
	REQUIRE(FindSurfaceGoods().empty()); const auto budget = QuerySurfaceGoods();
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	if (budget.credit > 0) PaySurfaceGoods<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, budget.credit);
	// Each genuinely paid empty truck acquires its own native orders. Cargo
	// appears later only through factory production and ordinary station loading.
	for (uint index = 0; index < budget.count; ++index) {
		auto paid = PaySurfaceGoods<Commands::BuildVehicle>(surface_goods_depot, surface_goods_depot, budget.engine, false, GetCargoTypeByLabel(CargoLabel{"GOOD"}), ClientID::Server);
		const auto id = std::get<1>(paid); const auto *truck = RoadVehicle::Get(id);
		REQUIRE(truck->cargo_cap == budget.capacity); REQUIRE(truck->cargo.TotalCount() == 0);
		PaySurfaceGoods<Commands::RenameVehicle>(TileIndex{0}, id, fmt::format("{}{}", surface_goods_prefix, index + 1));
		PaySurfaceGoods<Commands::InsertOrder>(TileIndex{0}, id, VehicleOrderID{0}, SurfaceGoodsPickupOrder());
		PaySurfaceGoods<Commands::InsertOrder>(TileIndex{0}, id, VehicleOrderID{1}, SurfaceGoodsReturnOrder());
		PaySurfaceGoods<Commands::StartStopVehicle>(TileIndex{0}, id, true);
		fmt::print("SURFACE_GOODS_TRUCK vehicle={} model={} capacity={} held=0 orders=2\n", id.base(), budget.engine.base(), budget.capacity);
	}
	REQUIRE(company->money == cash + budget.credit - budget.gross); REQUIRE(company->current_loan == debt + budget.credit); REQUIRE(company->money >= 100000);
}

/** @return True only when every actual new truck has genuinely returned full into the paid depot. */
static bool SurfaceGoodsCollected()
{
	const auto trucks = FindSurfaceGoods(); REQUIRE(!trucks.empty()); uint held = 0;
	for (const auto *truck : trucks) {
		// Both full useful custody and actual halted return are required; a
		// loading pose or offered station packet is not launch-ready reserve.
		if (!truck->IsStoppedInDepot() || truck->tile != surface_goods_depot || truck->cargo.TotalCount() != truck->cargo_cap) return false;
		held += truck->cargo.TotalCount();
	}
	return held >= 34;
}

/** Execute only explicit pure quotes, original empty purchases or bounded genuine loading/return work. */
TEST_CASE("Authentic additional Surface Goods remain in actual original road custody", "[.surface-entry-goods]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_GOODS_PHASE"); const std::string phase = selected == nullptr ? "query" : selected;
	// Restrict invocation to these published native boundaries; no debug
	// action may install Goods, money, vehicle positions or elapsed work.
	REQUIRE((phase == "query" || phase == "buy" || phase == "collect" || phase == "earn"));
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-goods"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve original failures through shutdown without compensating changes to gameplay owners. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceGoods(paths.saves / ("surface-goods-before-" + phase + "-440.sav"));
			if (phase == "query") QuerySurfaceGoods();
			else if (phase == "buy") BuySurfaceGoods();
			else if (phase == "collect") EarnSurfaceEntryWork("SURFACE_GOODS", 3000, SurfaceGoodsCollected);
			else {
				REQUIRE(SurfaceGoodsCollected());
				/** @return False; the exact native3000tick interval alone owns replay completion. */
				EarnSurfaceEntryWork("SURFACE_GOODS", 3000, [] { return false; }); REQUIRE(SurfaceGoodsCollected());
			}
			for (const auto *truck : FindSurfaceGoods()) fmt::print("SURFACE_GOODS_CUSTODY vehicle={} capacity={} held={} tile={} halted={}\n", truck->index.base(), truck->cargo_cap, truck->cargo.TotalCount(), truck->tile.base(), truck->IsStoppedInDepot());
			SaveSurfaceGoods(paths.saves / ("surface-goods-" + phase + "-440.sav"));
			fmt::print("SURFACE_GOODS_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_goods_observer, &inspect); IConsole::CmdRegister("surface_entry_goods", ObserveSurfaceGoods);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
