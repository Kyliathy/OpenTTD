/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_launch.cpp Actual genuine launch-entry placement, material and capital characterization.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIE.
 * Public native commands/definitions alone quote purchases. No temporary hub,
 * supplied storage, money, terrain, work or craft is installed for an estimate.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../command_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../launch_site.h"
#include "../multilayer_space_content.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../underground_vehicle.h"
#include "../vehicle_cmd.h"
#include "../town.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_passenger_layout.h"
#include "../tree_cmd.h"
#include "../tree_map.h"
#include "../clear_map.h"
#include "surface_entry_work.h"
#include "../safeguards.h"

static std::function<void()> *surface_launch_observer = nullptr; ///< Invocation callback; no gameplay or financial authority.

/** @param args Parameter-free original console arguments. @return True after the exact loaded-world observer runs. */
static bool ObserveSurfaceLaunch(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_launch_observer != nullptr); (*surface_launch_observer)(); return true;
}

/** @param path Fresh invocation-owned output. Preserve all native owners without earning work. */
static void SaveSurfaceLaunch(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Query a whole original synchronized command without supplying its missing prerequisite.
 * @tparam Tcmd Original command identity. @tparam Targs Original native argument types.
 * @param tile Actual tiled action, or zero for coordinate-free management.
 * @param args Complete original arguments. @return Original cost or refusal, without mutation.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceLaunch(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** Pure admitted real site quote, not a station, reserved footprint or saved routing owner. */
struct SurfaceLaunchSiteQuote {
	TileIndex anchor; ///< Actual existing Surface land/water candidate.
	uint8_t rotation; ///< Original quarter-turn footprint choice.
	Money invoice; ///< Entire current original command cost.
	uint distance; ///< Stable real distance to the existing material depot, for planning only.
};

/** Query and optionally purchase the complete ordinary civic prerequisite for this actual town.
 * @param execute Whether to execute fresh admitted native tree commands after the full expense/reserve check.
 */
static void PrepareSurfacePassengerTown(bool execute)
{
	const auto *company = Company::Get(_current_company); auto *town = Town::Get(TownID{0});
	const int before = town->ratings[_current_company], target = RATING_VERYPOOR + 70;
	const uint needed = before >= target ? 0 : (target - before + RATING_TREE_UP_STEP - 1) / RATING_TREE_UP_STEP;
	std::vector<TileIndex> trees; Money cost{0};
	// Only genuinely clear land belonging to this actual authority
	// may supply the original tree-plant rating effect. Existing forests cannot
	// yield another rating grant; original whole quotes include any field/rock
	// clearing, while machinery, water and houses remain intact.
	for (const auto tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) {
		if (trees.size() == needed) break;
		if (!IsTileType(tile, TileType::Clear)) continue;
		if (SurfaceChemicalLayout::FactoryCell(tile)) continue;
		if (ClosestTownFromTile(tile, _settings_game.economy.dist_local_authority) != town) continue;
		const auto quote = QuoteSurfaceLaunch<Commands::PlantTree>(tile, tile, tile, uint8_t{0}, false);
		if (quote.Failed()) continue;
		trees.push_back(tile); cost += quote.GetCost();
	}
	REQUIRE(trees.size() == needed); REQUIRE((company->tree_limit >> 16) > needed);
	fmt::print("SURFACE_PASSENGER_CIVIC town=0 rating={} target={} trees={} cost={} cash={} limit={} executed={}\n", before, target, trees.size(), cost.base(), company->money.base(), company->tree_limit >> 16, execute);
	if (!execute) return;
	REQUIRE(company->money >= cost + Money{40000});
	const auto cash = company->money, debt = company->current_loan;
	// Each tree is bought through the same complete synchronized command that
	// was admitted above. Native company books and town rules alone change.
	for (const auto tile : trees) {
		const auto quote = QuoteSurfaceLaunch<Commands::PlantTree>(tile, tile, tile, uint8_t{0}, false); REQUIRE(quote.Succeeded());
		const auto paid = Command<Commands::PlantTree>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile, tile, uint8_t{0}, false));
		REQUIRE(paid.Succeeded()); REQUIRE(paid.GetCost() == quote.GetCost()); REQUIRE(IsTileType(tile, TileType::Trees));
	}
	REQUIRE(company->money == cash - cost); REQUIRE(company->current_loan == debt);
	REQUIRE(town->ratings[_current_company] == before + int(trees.size()) * RATING_TREE_UP_STEP);
	fmt::print("SURFACE_PASSENGER_CIVIC_PAID rating={} cash={} debt={} total={}\n", town->ratings[_current_company], company->money.base(), debt.base(), cost.base());
}

/** Characterize actual whole launch costs and existing capital before selecting any paid continuation. */
static void QuerySurfaceLaunch()
{
	const auto *company = Company::Get(_current_company); REQUIRE(IsLaunchCenterTechnologyAvailable());
	Money craft_capital{0}; std::map<CargoLabel, uint64_t> inputs;
	// Canonical recipes own every hull/module cost and physical input. A
	// public recipe is a planning bound; an actual workshop and original stock
	// must freshly admit assembly before any dependent purchase or consumption.
	for (const auto kind : {SpaceBuildKind::Rocket, SpaceBuildKind::ConstructionProbe, SpaceBuildKind::Core}) {
		const auto *definition = GetSpaceConstructionDefinition(kind); REQUIRE(definition != nullptr);
		craft_capital += Money{definition->capital};
		fmt::print("SURFACE_LAUNCH_RECIPE kind={} model={} capital={} work={} mass={} width={} height={}\n", to_underlying(kind), definition->model,
			definition->capital, definition->work_ticks, definition->dry_mass_sixteenths, definition->width, definition->height);
		for (const auto &input : definition->inputs) {
			inputs[input.label] += input.quantity;
			fmt::print("SURFACE_LAUNCH_INPUT kind={} label={} quantity={}\n", to_underlying(kind), input.label.AsString(), input.quantity);
		}
	}
	const auto &budget = GetMultiLayerSpaceBudget();
	fmt::print("SURFACE_LAUNCH_CAPACITY rocket_mass={} probe_mass={} outbound_fuel={} recovery_fuel={} mission_fee={} active_daily={} idle_daily={} pad_daily={}\n",
		budget.rocket_payload_sixteenths, budget.probe_hold_sixteenths, budget.outbound_fuel, budget.recovery_fuel,
		budget.mission_fee, budget.active_per_day, budget.idle_per_day, budget.pad_per_day);
	std::vector<SurfaceLaunchSiteQuote> sites;
	// Only original current land beside actual navigable water may admit a
	// whole native site quote. Proposed canals cannot be painted into this query
	// or silently bypass the command's mandatory physical water predicate.
	for (const auto tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) {
		for (uint8_t rotation = 0; rotation < 4; ++rotation) {
			if (GetLaunchCenterSiteGeometryBlock(tile, rotation).Failed()) continue;
			auto quote = QuoteSurfaceLaunch<Commands::BuildLaunchCenter>(tile, tile, rotation, NEW_STATION, true);
			if (quote.Failed()) continue;
			const auto distance = std::abs(int(TileX(tile)) - 11) + std::abs(int(TilePlaneY(tile)) - 38);
			sites.push_back({tile, rotation, quote.GetCost(), static_cast<uint>(distance)});
		}
	}
	/** @param site Actual admitted native site. @return Stable nearest-depot rank, then full invoice and native identity. */
	const auto rank = [](const SurfaceLaunchSiteQuote &site) { return std::make_tuple(site.distance, site.invoice, site.anchor, site.rotation); };
	std::ranges::sort(sites, {}, rank);
	for (size_t index = 0; index < std::min<size_t>(8, sites.size()); ++index) {
		const auto &site = sites[index];
		fmt::print("SURFACE_LAUNCH_SITE anchor={} rotation={} cost={} depot_distance={}\n", site.anchor.base(), site.rotation, site.invoice.base(), site.distance);
	}
	const auto spendable = company->money + company->GetMaxLoan() - company->current_loan;
	fmt::print("SURFACE_LAUNCH_CAPITAL cash={} debt={} ceiling={} spendable={} craft_core={} steel={} goods={} actual_water_sites={} craft_core_affordable={} complete_arrangement=false\n",
		company->money.base(), company->current_loan.base(), company->GetMaxLoan().base(), spendable.base(), craft_capital.base(),
		inputs.at(CargoLabel{"STEL"}), inputs.at(CargoLabel{"GOOD"}), sites.size(), spendable >= craft_capital);
	// These are real original provider/model observations for a necessary
	// financing decision. They do not select an income route or assume that
	// forecast passengers, a quoted tariff or unsold assets supply capital.
	for (const auto *town : Town::Iterate()) fmt::print("SURFACE_LAUNCH_TOWN town={} anchor={} population={}\n", town->index.base(), town->xy.base(), town->cache.population);
	const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"});
	for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
		if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
		auto quote = QuoteSurfaceLaunch<Commands::BuildVehicle>(TileIndex{2443}, TileIndex{2443}, engine->index, false, passenger, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		fmt::print("SURFACE_LAUNCH_PASSENGER engine={} cost={} capacity={} speed={} annual={} reliability={}\n", engine->index.base(), std::get<0>(quote).GetCost().base(),
			std::get<2>(quote), engine->GetDisplayMaxSpeed(), engine->GetRunningCost().base(), engine->reliability);
	}
}

/** Inspect the exact physically produced-fuel world without creating a launch center, cargo, company balance or craft. */
TEST_CASE("Authentic launch entry uses actual native facilities recipes and original capital", "[.surface-entry-launch]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_LAUNCH_PHASE"); const std::string phase = selected == nullptr ? "query" : selected;
	REQUIRE((phase == "query" || phase == "passenger-query" || phase == "civic-query" || phase == "civic" || phase == "finance")); // Explicit phases distinguish pure quotes, original freight work and paid town work.
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-launch"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve original failures through native shutdown without compensating stock, money, work or terrain. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceLaunch(paths.saves / ("surface-launch-before-" + phase + "-440.sav"));
			if (phase == "query") QuerySurfaceLaunch();
			else if (phase == "civic-query") PrepareSurfacePassengerTown(false);
			else if (phase == "civic") PrepareSurfacePassengerTown(true);
			else if (phase == "finance") {
				/** @return Whether actual original freight payments now cover the bounded next civic/service budget. */
				const auto funded = [] { return Company::Get(_current_company)->money >= Money{90000}; };
				EarnSurfaceEntryWork("SURFACE_PASSENGER", 27000, funded);
			}
			else {
				SurfacePassengerLayout layout; layout.Prepare();
				const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"});
				const auto bus = QuoteSurfaceLaunch<Commands::BuildVehicle>(TileIndex{2443}, TileIndex{2443}, EngineID{119}, false, passenger, ClientID::Server);
				REQUIRE(std::get<0>(bus).Succeeded()); REQUIRE(std::get<2>(bus) == 40);
				const Money reserve{20000}; const auto total = layout.invoice + std::get<0>(bus).GetCost() * 3;
				fmt::print("SURFACE_PASSENGER_CAPITAL buses=3 model=119 capacity=40 bus_cost={} total={} reserve={} cash={} affordable={}\n", std::get<0>(bus).GetCost().base(), total.base(), reserve.base(),
					Company::Get(_current_company)->money.base(), Company::Get(_current_company)->money >= total + reserve);
			}
			SaveSurfaceLaunch(paths.saves / ("surface-launch-" + phase + "-440.sav"));
			fmt::print("SURFACE_LAUNCH_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_launch_observer, &inspect); IConsole::CmdRegister("surface_entry_launch", ObserveSurfaceLaunch);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
