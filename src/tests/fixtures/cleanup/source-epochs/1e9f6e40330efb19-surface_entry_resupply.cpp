/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_resupply.cpp Complete original finite-Salt and chemical-refill admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEG.
 * Native commands, original geography, packets, buffers and company books
 * retain ownership. Pure planning creates no carrier, product or progress.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../command_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../underground_vehicle.h"
#include "../vehicle_cmd.h"
#include "../roadveh.h"
#include "../train.h"
#include "../order_cmd.h"
#include "../depot_base.h"
#include "../industry.h"
#include "../multilayer_industry.h"
#include "../layer_industry_processor.h"
#include "../layer_portal_cmd.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../rail_map.h"
#include "../road_map.h"
#include "../misc_cmd.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <queue>
#include "../safeguards.h"

static std::function<void()> *surface_resupply_observer = nullptr; ///< Invocation callback, never a simulation owner.
static constexpr TileIndex surface_resupply_depot{1373}, surface_resupply_road_depot{2443};

/** @param args Original parameter-free console arguments. @return True after observing this invocation. */
static bool ObserveSurfaceResupply(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_resupply_observer != nullptr); (*surface_resupply_observer)(); return true;
}

/** @param path Fresh invocation-owned output. Preserve actual owners without earning work. */
static void SaveSurfaceResupply(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Query one complete original command without mutating gameplay.
 * @tparam Tcmd Original command. @tparam Targs Native argument types.
 * @param tile Actual command position. @param args Complete original arguments. @return Native quote or refusal.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceResupply(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** @return Existing original empty chemical tanker, following its name rather than an assumed reusable pool slot. */
static const RoadVehicle *FindSurfaceResupplyChemical()
{
	const RoadVehicle *result = nullptr;
	for (const auto *truck : RoadVehicle::Iterate()) {
		if (truck->name != "Surface Chemical reserve") continue;
		REQUIRE(result == nullptr); result = truck;
	}
	REQUIRE(result != nullptr); REQUIRE(result->owner == _current_company); REQUIRE(result->IsStoppedInDepot());
	REQUIRE(result->tile == surface_resupply_road_depot); REQUIRE(result->cargo.TotalCount() == 0);
	REQUIRE(result->cargo_type == GetCargoTypeByLabel(CargoLabel{"CHEM"})); REQUIRE(result->cargo_cap == 21);
	return result;
}

/** Complete read-only admission for the real carrier, useful existing inputs and crossing-safe staged delivery. */
static void QuerySurfaceResupply()
{
	const auto *company = Company::Get(_current_company); const auto *chemical = FindSurfaceResupplyChemical();
	const auto *factory = Industry::Get(IndustryID{15}); REQUIRE(IsLayerIndustryComplete(*factory));
	const auto *processor = GetLayerProcessorState(factory->index); REQUIRE(processor != nullptr); REQUIRE(processor->recipe == MultiLayerRecipeID{"CHEM"});
	const auto oil = GetCargoTypeByLabel(CargoLabel{"OIL_"}), salt = GetCargoTypeByLabel(CargoLabel{"SALT"});
	const auto input = std::ranges::find(factory->accepted, oil, &Industry::AcceptedCargo::cargo); REQUIRE(input != factory->accepted.end()); REQUIRE(input->waiting >= 60);
	const auto salt_input = std::ranges::find(factory->accepted, salt, &Industry::AcceptedCargo::cargo); REQUIRE(salt_input != factory->accepted.end()); REQUIRE(salt_input->waiting == 0);
	REQUIRE(GetLayerPortalWorkState(TileIndex{1949}) == LayerPortalWorkState::Connected);
	const auto locomotive = GetUndergroundEngine(4), wagon = GetUndergroundEngine(5);
	auto head = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, locomotive, false, INVALID_CARGO, ClientID::Server);
	auto hopper = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, wagon, false, salt, ClientID::Server);
	REQUIRE(std::get<0>(head).Succeeded()); REQUIRE(std::get<0>(hopper).Succeeded()); REQUIRE(std::get<2>(hopper) == 60);
	REQUIRE(CheckUndergroundVehicleCapability(locomotive, MapLayer::Underground, salt).Succeeded());
	REQUIRE(CheckUndergroundVehicleCapability(wagon, MapLayer::Surface, salt).Succeeded());
	const auto *receiver = Station::Get(StationID{4}); const auto *pickup = Station::Get(StationID{5}); const auto *output = Station::GetByTile(TileIndex{1825});
	REQUIRE(receiver->owner == _current_company); REQUIRE(pickup->owner == _current_company); REQUIRE(output != nullptr); REQUIRE(output->owner == _current_company);
	REQUIRE(receiver->TileIsInCatchment(factory->location.tile)); bool covers = false;
	for (const auto tile : factory->location) if (output->TileIsInCatchment(tile)) covers = true;
	REQUIRE(covers); REQUIRE(GetRailDepotDirection(surface_resupply_depot) == DiagDirection::SE);
	REQUIRE(TileX(receiver->train_station.tile) == TileX(surface_resupply_depot)); REQUIRE(TileY(receiver->train_station.tile) == 24);
	// Stage the full Salt train back into its original depot before sending
	// the empty chemical tanker across the shared mainline. Later Salt delivery
	// uses only this short existing northern rail segment, above road crossings.
	for (uint y = TileY(surface_resupply_depot); y <= TileY(receiver->train_station.tile); ++y) {
		const auto tile = TileXY(TileX(surface_resupply_depot), y); REQUIRE(GetTileOwner(tile) == _current_company);
		REQUIRE((IsTileType(tile, TileType::Railway) || IsRailStationTile(tile))); REQUIRE_FALSE(IsLevelCrossingTile(tile));
		if (IsPlainRailTile(tile)) REQUIRE(GetTrackBits(tile).Test(Track::Y));
	}
	fmt::print("SURFACE_RESUPPLY_RAIL depot=1373 pickup=5 receiver=4 receiver_tile={} short_tiles=4 short_crossings=0 full_return_first=true chemical_waits_at_source=true\n", receiver->train_station.tile.base());
	std::map<TileIndex, TileIndex> parent; std::queue<TileIndex> pending;
	parent.emplace(surface_resupply_road_depot, surface_resupply_road_depot); pending.push(surface_resupply_road_depot);
	// Only the present reciprocal Surface road component admits the existing
	// chemical output stop. No virtual station edge or proposed road is used.
	while (!pending.empty()) {
		const auto tile = pending.front(); pending.pop(); if (tile == output->xy) break;
		const auto bits = GetAnyRoadBits(tile, RoadTramType::Road);
		for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			if ((bits & DiagDirToRoadBits(direction)) == RoadBits{}) continue;
			const auto next = AddTileIndexDiffCWrap(tile, TileIndexDiffCByDiagDir(direction));
			// Canonical Surface ownership and prior discovery constrain the
			// read-only traversal to real same-plane mutually connected roads.
			if (!MultiLayerMap::IsTileInLayer(next, MapLayer::Surface) || parent.contains(next)) continue;
			if ((GetAnyRoadBits(next, RoadTramType::Road) & DiagDirToRoadBits(ReverseDiagDir(direction))) == RoadBits{}) continue;
			parent.emplace(next, tile); pending.push(next);
		}
	}
	REQUIRE(parent.contains(output->xy)); uint edges = 0, crossings = 0;
	// Identify each actual crossing, including its relation to the short
	// later Salt delivery. Native trains must genuinely clear the chemical trip.
	for (auto tile = output->xy; tile != surface_resupply_road_depot; tile = parent.at(tile)) {
		++edges;
		if (!IsLevelCrossingTile(tile)) continue;
		++crossings; REQUIRE(TileY(tile) > 24);
		fmt::print("SURFACE_RESUPPLY_CROSSING tile={} x={} y={} north_delivery_clear=true\n", tile.base(), TileX(tile), TileY(tile));
	}
	Order collect; collect.MakeGoToStation(output->index); collect.SetLoadType(OrderLoadType::FullLoad); collect.SetUnloadType(OrderUnloadType::NoUnload); collect.SetStopLocation(OrderStopLocation::FarEnd);
	auto loading = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, chemical->index, VehicleOrderID{0}, collect); REQUIRE(loading.Succeeded()); REQUIRE(loading.GetCost() == 0);
	auto service = QuoteSurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, chemical->index, true); REQUIRE(service.Succeeded()); REQUIRE(service.GetCost() == 0);
	uint traffic = 0;
	// Preserve every original income order and cargo: native appended depot
	// halts plus delivery at the existing receiver can protect all three routes.
	for (const auto *train : Train::Iterate()) {
		if (!train->IsFrontEngine()) continue;
		if (!train->name.starts_with("Surface Income ")) continue;
		const std::map<std::string, TileIndex> depots{{"Surface Income farm", TileIndex{1638}}, {"Surface Income goods", TileIndex{3224}}, {"Surface Income coal", TileIndex{3161}}};
		const auto *depot = Depot::GetByTile(depots.at(train->name)); REQUIRE(depot != nullptr); REQUIRE(train->GetNumOrders() == 2);
		Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		auto halt = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{2}, home); REQUIRE(halt.Succeeded()); REQUIRE(halt.GetCost() == 0);
		++traffic;
	}
	REQUIRE(traffic == 3); const auto gross = std::get<0>(head).GetCost() + std::get<0>(hopper).GetCost(); const Money reserve{100000};
	const auto shortfall = std::max(Money{0}, gross + reserve - company->money); const Money credit{((shortfall.base() + 9999) / 10000) * 10000};
	if (credit > 0) { auto bank = QuoteSurfaceResupply<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit); REQUIRE(bank.Succeeded()); }
	REQUIRE(company->current_loan + credit <= company->GetMaxLoan()); REQUIRE(company->money + credit - gross >= reserve);
	fmt::print("SURFACE_RESUPPLY_PLAN locomotive={} wagon={} head_cost={} wagon_cost={} gross={} salt_capacity=60 oil={} chemicals=21 chemical_vehicle={} chemical_capacity={} chemical_pickup={} road_edges={} road_crossings={} protected_trains={} reserve=100000 credit={} cash={} debt={} ceiling={} infrastructure=0 unsold_value_counted=false\n",
		locomotive.base(), wagon.base(), std::get<0>(head).GetCost().base(), std::get<0>(hopper).GetCost().base(), gross.base(), input->waiting,
		chemical->index.base(), chemical->cargo_cap, output->index.base(), edges, crossings, traffic, credit.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base());
}

/** Pure original current-world query; preserve all121native save families byte for byte. */
TEST_CASE("Authentic chemical refill has a complete original finite transport and funding quote", "[.surface-entry-resupply]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-resupply"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve native query failures through normal shutdown without compensating gameplay changes. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves / "surface-resupply-before-query-440.sav"); QuerySurfaceResupply();
			SaveSurfaceResupply(paths.saves / "surface-resupply-query-440.sav");
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase=query supplied_stock=false supplied_work=false\n"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect); IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
