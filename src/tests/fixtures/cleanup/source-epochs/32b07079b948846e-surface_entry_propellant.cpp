/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_propellant.cpp Genuine second-factory, physical chemical transfer and Propellant contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BID.
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
#include <queue>
#include "../safeguards.h"

static std::function<void()> *surface_propellant_observer = nullptr; ///< Invocation callback, never a material or money owner.
static constexpr TileIndex surface_propellant_site{1639}, surface_propellant_depot{2443};

/** @param args Parameter-free original console arguments. @return True after observing the actual loaded world. */
static bool ObserveSurfacePropellant(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_propellant_observer != nullptr); (*surface_propellant_observer)(); return true;
}

/** @param path Fresh invocation-owned output. Preserve original native owners without earning work. */
static void SaveSurfacePropellant(const std::filesystem::path &path)
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
static auto QuoteSurfacePropellant(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** @return Sole original loaded Source15chemical tanker, following its real name instead of a historical pool slot. */
static const RoadVehicle *FindSurfacePropellantFeed()
{
	const RoadVehicle *result = nullptr;
	// Native identity and packet source protect the paid useful cargo even if
	// ordinary vehicle renewal has legitimately reused another pool slot.
	for (const auto *truck : RoadVehicle::Iterate()) {
		if (truck->name != "Surface Chemical reserve") continue;
		REQUIRE(result == nullptr); REQUIRE(truck->owner == _current_company); result = truck;
	}
	REQUIRE(result != nullptr); REQUIRE(result->IsStoppedInDepot()); REQUIRE(result->tile == surface_propellant_depot);
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
static uint QuerySurfacePropellantRoad(TileIndex target)
{
	std::map<TileIndex, TileIndex> parent; std::queue<TileIndex> pending;
	parent.emplace(surface_propellant_depot, surface_propellant_depot); pending.push(surface_propellant_depot);
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
	for (auto tile = target; tile != surface_propellant_depot; tile = parent.at(tile)) {
		++length;
		if (IsLevelCrossingTile(tile)) ++crossings;
	}
	fmt::print("SURFACE_PROPELLANT_ROUTE target={} edges={} crossings={} infrastructure=0\n", target.base(), length, crossings);
	return crossings;
}

/** Whole original empty carrier quote; owns no vehicle, money, input or product. */
struct SurfacePropellantQuote {
	EngineID engine = EngineID::Invalid(); ///< Cheapest actually admitted original PROP carrier.
	Money vehicle{0}, factory{0}, credit{0}; ///< Fresh native whole costs and matching required bank credit.
	uint capacity = 0; ///< Real quoted useful carrying capacity.
};

/** @return Complete admitted genuine second-factory/transport plan, with native current matching credit. */
static SurfacePropellantQuote QuerySurfacePropellant()
{
	const auto *company = Company::Get(_current_company); const auto *feed = FindSurfacePropellantFeed();
	const auto type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}); REQUIRE(type != IT_INVALID);
	auto factory = QuoteSurfacePropellant<Commands::BuildIndustry>(surface_propellant_site, surface_propellant_site, type, uint32_t{0}, true, uint32_t{123});
	REQUIRE(factory.Succeeded()); SurfacePropellantQuote result; result.factory = factory.GetCost();
	const auto *first = Industry::GetByTile(TileIndex{1566}); REQUIRE(first != nullptr); REQUIRE(first->index == IndustryID{15}); REQUIRE(first->type == type);
	// The existing controlled facility proves this actual definition's recipe
	// availability without changing it. The new actual industry must separately
	// admit its own recipe command after original funding creates its owner.
	auto choice = QuoteSurfacePropellant<Commands::SelectLayerIndustryRecipe>(TileIndex{0}, first->index, uint32_t{0x5043484D}); REQUIRE(choice.Succeeded());
	const auto prop = GetCargoTypeByLabel(CargoLabel{"PROP"}); REQUIRE(IsValidCargoType(prop));
	for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
		if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
		auto quote = QuoteSurfacePropellant<Commands::BuildVehicle>(surface_propellant_depot, surface_propellant_depot, engine->index, false, prop, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		if (std::get<2>(quote) < 20) continue;
		if (result.engine != EngineID::Invalid()) {
			if (result.vehicle <= std::get<0>(quote).GetCost()) continue;
		}
		result.engine = engine->index; result.vehicle = std::get<0>(quote).GetCost(); result.capacity = std::get<2>(quote);
	}
	REQUIRE(result.engine != EngineID::Invalid());
	uint crossings = 0;
	for (const auto bay : {TileIndex{1895}, TileIndex{1896}}) {
		const auto *station = Station::GetByTile(bay); REQUIRE(station != nullptr); REQUIRE(station->owner == _current_company);
		bool covers = false;
		// A future industry contributes no fake receiver room. Existing native
		// catchment must cover its real proposed machinery before funding it.
		for (const auto tile : TileArea(surface_propellant_site, 4, 4)) if (station->TileIsInCatchment(tile)) covers = true;
		REQUIRE(covers); crossings += QuerySurfacePropellantRoad(bay);
		Order order; order.MakeGoToStation(station->index); order.SetStopLocation(OrderStopLocation::FarEnd);
		order.SetLoadType(bay == TileIndex{1895} ? OrderLoadType::NoLoad : OrderLoadType::LoadIfPossible);
		auto admitted = QuoteSurfacePropellant<Commands::InsertOrder>(TileIndex{0}, feed->index, VehicleOrderID{0}, order); REQUIRE(admitted.Succeeded());
		fmt::print("SURFACE_PROPELLANT_BAY tile={} station={} catchment=true order_admitted=true\n", bay.base(), station->index.base());
	}
	// Native trains must be genuinely halted before the irreplaceable chemical
	// stock crosses their tracks. Original temporary depot orders preserve their
	// commercial orders/cargo, and can be resumed after the material transfer.
	for (const auto *train : Train::Iterate()) {
		if (!train->IsFrontEngine()) continue;
		if (!train->name.starts_with("Surface Income ")) continue;
		const std::map<std::string, TileIndex> depots{{"Surface Income farm", TileIndex{1638}}, {"Surface Income goods", TileIndex{3224}}, {"Surface Income coal", TileIndex{3161}}};
		const auto *depot = Depot::GetByTile(depots.at(train->name)); REQUIRE(depot != nullptr); REQUIRE(train->GetNumOrders() == 2);
		// Closest-depot search can legitimately refuse a train's present
		// loading pose. The player's explicit ordered depot remains a distinct
		// native route: deliver at its existing receiver, then the appended halt.
		Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		auto recall = QuoteSurfacePropellant<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{2}, home); REQUIRE(recall.Succeeded());
		auto deliver = QuoteSurfacePropellant<Commands::SkipToOrder>(TileIndex{0}, train->index, VehicleOrderID{1}); REQUIRE(deliver.Succeeded());
		fmt::print("SURFACE_PROPELLANT_TRAFFIC head={} halt_admitted=true crossings={}\n", train->index.base(), crossings);
	}
	const Money buffer{100000}; const auto required = result.factory + result.vehicle + buffer;
	const auto shortfall = std::max(Money{0}, required - company->money);
	result.credit = Money{((shortfall.base() + 9999) / 10000) * 10000};
	auto bank = QuoteSurfacePropellant<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, result.credit); REQUIRE(bank.Succeeded());
	REQUIRE(company->money + company->GetMaxLoan() - company->current_loan >= required);
	fmt::print("SURFACE_PROPELLANT_PLAN site={} type={} source=15 chemicals=21 expected_product=20 residual_chemical=1 factory={} engine={} tanker={} capacity={} buffer={} requirement={} credit={} cash={} debt={} ceiling={} full_load=false\n",
		surface_propellant_site.base(), type, result.factory.base(), result.engine.base(), result.vehicle.base(), result.capacity, buffer.base(), required.base(), result.credit.base(),
		company->money.base(), company->current_loan.base(), company->GetMaxLoan().base());
	return result;
}

/** Observe only real native contracts in the exact selected funded world. */
TEST_CASE("Authentic Surface Propellant uses real external chemicals and original commands", "[.surface-entry-propellant]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-propellant"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Retain native failures through ordinary shutdown without compensating grants. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfacePropellant(paths.saves / "surface-propellant-before-query-440.sav");
			QuerySurfacePropellant();
			SaveSurfacePropellant(paths.saves / "surface-propellant-query-440.sav");
			fmt::print("SURFACE_PROPELLANT_COMPLETE phase=query supplied_stock=false supplied_work=false\n"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_propellant_observer, &inspect); IConsole::CmdRegister("surface_entry_propellant", ObserveSurfacePropellant);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
