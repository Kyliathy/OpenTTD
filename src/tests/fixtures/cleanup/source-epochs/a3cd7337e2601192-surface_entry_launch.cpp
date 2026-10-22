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
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-launch"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve original failures through native shutdown without compensating stock, money, work or terrain. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceLaunch(paths.saves / "surface-launch-before-query-440.sav"); QuerySurfaceLaunch();
			SaveSurfaceLaunch(paths.saves / "surface-launch-query-440.sav");
			fmt::print("SURFACE_LAUNCH_COMPLETE phase=query supplied_stock=false supplied_work=false\n"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_launch_observer, &inspect); IConsole::CmdRegister("surface_entry_launch", ObserveSurfaceLaunch);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
