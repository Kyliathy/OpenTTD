/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_gateway.cpp Pure complete current launch-entry geometry, custody and funding characterization.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BIEI/15BIEQJ.
 * Actual commands, native buffers, jobs and company books retain ownership.
 * Future assembly prices are definition bounds until a real funded center
 * admits its own commands; no placeholder facility or hull is instantiated.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../command_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../multilayer_space_content.h"
#include "../multilayer_content.h"
#include "../orbital_station.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_vehicle.h"
#include "../roadveh.h"
#include "../road_map.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../depot_base.h"
#include "../order_cmd.h"
#include "../train.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <queue>
#include "../safeguards.h"

static std::function<void()> *surface_gateway_observer = nullptr; ///< Invocation callback, never a gameplay owner.
static constexpr TileIndex surface_gateway_site{2890}, surface_gateway_depot{2443};

/** @param args Original parameter-free console arguments. @return True after this invocation's actual query. */
static bool ObserveSurfaceGateway(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_gateway_observer != nullptr); (*surface_gateway_observer)(); return true;
}

/** @param path Fresh invocation-owned output. Preserve actual original owners without earning work. */
static void SaveSurfaceGateway(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Query one complete original command without changing gameplay owners.
 * @tparam Tcmd Original command. @tparam Targs Native argument types.
 * @param tile Actual action position. @param args Complete native arguments. @return Original quote or refusal.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceGateway(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** One existing genuine reserve carrier; native vehicle and packets remain the only physical owner. */
struct SurfaceGatewayCarrier {
	std::string_view name, role; ///< Native cosmetic discovery name and whitespace-free diagnostic role.
	CargoLabel label; ///< Stable actual cargo meaning, independent of pool-slot reuse.
	uint quantity, source; ///< Previously earned actual useful units and original producing industry.
};

/** Characterize the whole legal current arrangement and funding gap without granting any absent facility or craft. */
static void QuerySurfaceGateway()
{
	static_assert(std::is_same_v<decltype(&ReserveOrbitalConstructionInputs), bool (*)(SpaceStructure &, SpaceVehicle &, bool)>);
	static_assert(std::is_same_v<decltype(&CommitOrbitalConstructionStep), bool (*)(SpaceVehicle &)>);
	REQUIRE(GetLaunchCenterReferences().empty()); REQUIRE(GetSpaceVehicleCount() == 0); REQUIRE(SpaceStructure::GetNumItems() == 0);
	const auto *company = Company::Get(_current_company); const auto area = GetLaunchCenterFootprint(surface_gateway_site, 1); REQUIRE(area.has_value());
	REQUIRE(HasLaunchCenterWaterAccess(*area));
	auto center = QuoteSurfaceGateway<Commands::BuildLaunchCenter>(surface_gateway_site, surface_gateway_site, uint8_t{1}, NEW_STATION, true);
	INFO("Current actual center quote error=" << center.GetErrorMessage().base()); REQUIRE(center.Succeeded());
	for (const auto zone : {LaunchFacilityZone::Pad, LaunchFacilityZone::Workshop, LaunchFacilityZone::Recovery, LaunchFacilityZone::Storage}) {
		const auto extent = GetLaunchFacilityZone(surface_gateway_site, 1, zone); REQUIRE(extent.has_value());
		fmt::print("SURFACE_GATEWAY_ZONE kind={} tile={} width={} height={} prospective_geometry=true\n", to_underlying(zone), extent->tile.base(), extent->w, extent->h);
	}
	const auto *member = Station::Get(StationID{0}); REQUIRE(member->owner == _current_company); REQUIRE(member->facilities.Test(StationFacility::TruckStop));
	REQUIRE_FALSE(GetLaunchHubMembership(member->index).has_value()); const auto anchor = GetLaunchCenterPlaneReference(surface_gateway_site); REQUIRE(anchor.has_value());
	uint physical = 0;
	// Every real station tile must fit the public collection circle. A sign,
	// catchment rectangle or unbuilt center cannot grant actual hub membership.
	for (const auto tile : member->spread) {
		if (!IsTileType(tile, TileType::Station)) continue;
		if (GetStationIndex(tile) != member->index) continue;
		REQUIRE(GetTileOwner(tile) == member->owner); const auto point = GetLaunchCenterPlaneReference(tile); REQUIRE(point.has_value());
		REQUIRE(IsInsideLaunchCenterCollectionArea(*point, *anchor)); ++physical;
	}
	REQUIRE(physical > 0);
	std::map<TileIndex, TileIndex> parent; std::queue<TileIndex> pending;
	parent.emplace(surface_gateway_depot, surface_gateway_depot); pending.push(surface_gateway_depot);
	// Only existing reciprocal Surface roads can carry the actual reserves;
	// no suggested line, map-plane offset or future construction supplies an edge.
	while (!pending.empty()) {
		const auto tile = pending.front(); pending.pop(); if (tile == member->xy) break;
		const auto bits = GetAnyRoadBits(tile, RoadTramType::Road);
		for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			if ((bits & DiagDirToRoadBits(direction)) == RoadBits{}) continue;
			const auto next = AddTileIndexDiffCWrap(tile, TileIndexDiffCByDiagDir(direction));
			// Canonical Surface membership and unique discovery constrain the
			// query to original same-plane connected roads and preserve every owner.
			if (!MultiLayerMap::IsTileInLayer(next, MapLayer::Surface) || parent.contains(next)) continue;
			if ((GetAnyRoadBits(next, RoadTramType::Road) & DiagDirToRoadBits(ReverseDiagDir(direction))) == RoadBits{}) continue;
			parent.emplace(next, tile); pending.push(next);
		}
	}
	REQUIRE(parent.contains(member->xy)); uint edges = 0, crossings = 0;
	for (auto tile = member->xy; tile != surface_gateway_depot; tile = parent.at(tile)) { ++edges; if (IsLevelCrossingTile(tile)) ++crossings; }
	fmt::print("SURFACE_GATEWAY_MEMBER station=0 tile={} physical_tiles={} radius={} geometry_fits=true membership_admitted=false edges={} crossings={} infrastructure=0\n", member->xy.base(), physical, LAUNCH_CENTER_COLLECTION_RADIUS, edges, crossings);
	const std::array<SurfaceGatewayCarrier, 7> carriers{{
		{"Surface Steel reserve 1", "steel1", CargoLabel{"STEL"}, 18, 10}, {"Surface Steel reserve 2", "steel2", CargoLabel{"STEL"}, 18, 10},
		{"Surface Goods reserve", "goods1", CargoLabel{"GOOD"}, 14, 11}, {"Surface Launch Goods reserve 1", "goods2", CargoLabel{"GOOD"}, 18, 11},
		{"Surface Launch Goods reserve 2", "goods3", CargoLabel{"GOOD"}, 18, 11}, {"Surface Propellant reserve", "fuel1", CargoLabel{"PROP"}, 20, 6},
		{"Surface Propellant refill", "fuel2", CargoLabel{"PROP"}, 22, 6}}};
	std::map<CargoLabel, uint64_t> stock, required; uint64_t handling_mass = 0; uint shipment_units = 0;
	// All seven original real carriers quote normal Transfer/NoLoad orders.
	// Receiver acceptance cannot turn the selected construction stock into an
	// accidental industry sale. Existing native packets remain in place here.
	for (const auto &role : carriers) {
		const RoadVehicle *truck = nullptr;
		for (const auto *candidate : RoadVehicle::Iterate()) if (candidate->name == role.name) { REQUIRE(truck == nullptr); truck = candidate; }
		REQUIRE(truck != nullptr); REQUIRE(truck->owner == _current_company); REQUIRE(truck->IsStoppedInDepot()); REQUIRE(truck->tile == surface_gateway_depot);
		REQUIRE(truck->cargo_type == GetCargoTypeByLabel(role.label)); REQUIRE(truck->cargo.TotalCount() == role.quantity);
		for (const auto *packet : *truck->cargo.Packets()) { REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == role.source); }
		Order transfer; transfer.MakeGoToStation(member->index); transfer.SetStopLocation(OrderStopLocation::FarEnd); transfer.SetLoadType(OrderLoadType::NoLoad); transfer.SetUnloadType(OrderUnloadType::Transfer);
		auto admitted = QuoteSurfaceGateway<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, transfer); REQUIRE(admitted.Succeeded()); REQUIRE(admitted.GetCost() == 0);
		Order home; home.MakeGoToDepot(Depot::GetByTile(surface_gateway_depot)->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		auto back = QuoteSurfaceGateway<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, home); REQUIRE(back.Succeeded()); REQUIRE(back.GetCost() == 0);
		stock[role.label] += role.quantity; shipment_units += role.quantity; handling_mass += uint64_t{role.quantity} * CargoSpec::Get(truck->cargo_type)->weight;
		fmt::print("SURFACE_GATEWAY_CARRIER role={} vehicle={} label={} held={} source={} transfer_admitted=true return_admitted=true\n", role.role, truck->index.base(), role.label.AsString(), role.quantity, role.source);
	}
	Money hulls{0}; uint hull_work = 0;
	// Canonical recipes define future material/capital bounds. The actual
	// absent center cannot admit assembly until real funding and stock arrive.
	for (const auto kind : {SpaceBuildKind::Rocket, SpaceBuildKind::ConstructionProbe, SpaceBuildKind::Core}) {
		const auto *definition = GetSpaceConstructionDefinition(kind); REQUIRE(definition != nullptr);
		if (kind != SpaceBuildKind::Core) { hulls += Money{definition->capital}; hull_work += definition->work_ticks; }
		for (const auto &input : definition->inputs) {
			required[input.label] += input.quantity;
			fmt::print("SURFACE_GATEWAY_INPUT kind={} label={} quantity={}\n", to_underlying(kind), input.label.AsString(), input.quantity);
		}
		fmt::print("SURFACE_GATEWAY_RECIPE kind={} model={} capital={} work={} mass={} future_assembly_admitted=false\n", to_underlying(kind), definition->model, definition->capital, definition->work_ticks, definition->dry_mass_sixteenths);
	}
	REQUIRE(required[CargoLabel{"STEL"}] == 32); REQUIRE(required[CargoLabel{"GOOD"}] == 48);
	for (const auto &[label, quantity] : required) REQUIRE(stock.at(label) >= quantity);
	std::vector<TileIndex> core_sites;
	for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Space)) core_sites.push_back(tile);
	/** @param tile Actual Space candidate. @return Stable original distance from the Surface launch reference, then native tile identity. */
	const auto rank = [](TileIndex tile) { return std::make_pair(std::abs(int(TileX(tile)) - int(TileX(surface_gateway_site))) + std::abs(int(TilePlaneY(tile)) - int(TilePlaneY(surface_gateway_site))), tile.base()); };
	std::ranges::sort(core_sites, {}, rank); TileIndex core_tile = INVALID_TILE; CommandCost core = CMD_ERROR;
	// The projected Surface coordinate lies in the protected moving-body
	// strip. Only the original command can admit a nearby complete static
	// footprint and approach; no manual plane offset or painted site bypasses it.
	for (const auto candidate : core_sites) {
		auto admitted = QuoteSurfaceGateway<Commands::BuildSpaceBlueprint>(candidate, candidate, SpaceBuildKind::Core);
		if (admitted.Failed()) continue;
		core = admitted; core_tile = candidate; break;
	}
	REQUIRE(core_tile != INVALID_TILE); REQUIRE(core.Succeeded());
	const auto *core_recipe = GetSpaceConstructionDefinition(SpaceBuildKind::Core); REQUIRE(core.GetCost() == Money{core_recipe->capital});
	for (const uint16_t model : {uint16_t{14}, uint16_t{20}}) {
		auto absent = QuoteSurfaceGateway<Commands::AssembleLaunchCraft>(TileIndex{0}, StationID::Invalid(), uint64_t{0}, model); REQUIRE(absent.Failed());
	}
	const auto &budget = GetMultiLayerSpaceBudget(); const auto day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
	REQUIRE(day > 0);
	const uint fuel = 2 * (uint{budget.outbound_fuel} + budget.recovery_fuel); REQUIRE(stock.at(CargoLabel{"PROP"}) >= fuel);
	const std::array<MultiLayerRecipeCargo, 1> steel_leg{{{CargoLabel{"STEL"}, 16}}}, goods_leg{{{CargoLabel{"GOOD"}, 24}}};
	const auto steel_mass = GetMultiLayerPayloadMass(steel_leg), goods_mass = GetMultiLayerPayloadMass(goods_leg);
	REQUIRE(steel_mass.has_value()); REQUIRE(goods_mass.has_value());
	const auto builder_mass = GetSpaceConstructionDefinition(SpaceBuildKind::ConstructionProbe)->dry_mass_sixteenths;
	REQUIRE(builder_mass + *steel_mass <= budget.rocket_payload_sixteenths); REQUIRE(*steel_mass <= budget.probe_hold_sixteenths);
	REQUIRE(*goods_mass <= budget.rocket_payload_sixteenths); REQUIRE(*goods_mass <= budget.probe_hold_sixteenths); REQUIRE(fuel <= budget.fuel_hold);
	REQUIRE(stock.at(CargoLabel{"STEL"}) <= budget.stock_cap); REQUIRE(stock.at(CargoLabel{"GOOD"}) <= budget.stock_cap); REQUIRE(stock.at(CargoLabel{"PROP"}) <= budget.launch_propellant_cap);
	REQUIRE(MAX_SURFACE_HANDLING_QUANTITY >= 50); REQUIRE(MAX_SURFACE_HANDLING_JOBS >= 3); REQUIRE(MAX_CENTER_CRAFT >= 2);
	fmt::print("SURFACE_GATEWAY_MASS builder={} first_cargo={} second_cargo={} first_payload={} rocket_capacity={} builder_capacity={} fuel_hold={}\n", builder_mass, *steel_mass, *goods_mass, builder_mass + *steel_mass, budget.rocket_payload_sixteenths, budget.probe_hold_sixteenths, budget.fuel_hold);
	// Bounded recipe/budget fields and128real shipment units fit signed native
	// Money; explicit conversions preserve whole-unit fee rounding without a
	// narrowing constructor or another financial balance.
	const Money mission{2 * budget.mission_fee}; const Money pad{static_cast<int64_t>(2 * (uint64_t{budget.prepare_ticks} + budget.turnaround_ticks) * budget.pad_per_day / day)};
	const Money builder_work{static_cast<int64_t>((uint64_t{core_recipe->work_ticks} * budget.active_per_day + day - 1) / day)};
	const Money handling{static_cast<int64_t>(handling_mass / 16)}, reserve{150000};
	const auto planned = center.GetCost() + hulls + core.GetCost() + mission + pad + builder_work + handling + reserve;
	const Money target{((planned.base() + 9999) / 10000) * 10000}; const auto spendable = company->money + company->GetMaxLoan() - company->current_loan;
	// Funding describes actual cash plus unchanged native unused credit. The
	// same pure query must disclose both an unfunded and a fully earned world.
	fmt::print("SURFACE_GATEWAY_PLAN site=2890 rotation=1 width={} height={} core_tile={} center_quote={} hull_bound={} core_quote={} steel={} goods={} flight_fuel={} shipment_units={} handling_fee={} handling_ticks={} handling_jobs=3 hull_work={} mission_fee={} pad_bound={} builder_work_bound={} reserve=150000 planned={} target={} cash={} debt={} ceiling={} spendable={} gap={} year={} complete_capital={}\n",
		area->w, area->h, core_tile.base(), center.GetCost().base(), hulls.base(), core.GetCost().base(), stock.at(CargoLabel{"STEL"}), stock.at(CargoLabel{"GOOD"}), fuel,
		shipment_units, handling.base(), (shipment_units + LAUNCH_HANDLING_THROUGHPUT - 1) / LAUNCH_HANDLING_THROUGHPUT, hull_work, mission.base(), pad.base(), builder_work.base(), planned.base(), target.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base(), spendable.base(), std::max(Money{0}, target - spendable).base(), TimerGameCalendar::year.base(), spendable >= target);
	// Real ages establish the next funding/service boundary; no projected
	// profit, unsold value or hypothetical new engine is counted as capital.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (!vehicle->IsPrimaryVehicle()) continue;
		if (vehicle->owner != _current_company) continue;
		const auto &name = vehicle->name;
		// Audit every actual useful service, including later regional buses.
		// Parked material reserves do not become income vehicles through ID reuse.
		const bool income = name.starts_with("Surface Income ") || name.starts_with("Surface Launch passenger ") || name.starts_with("Surface Regional passenger ") || name == "Surface ore delivery" || name == "Surface Steel delivery";
		if (!income) continue;
		fmt::print("SURFACE_GATEWAY_SERVICE vehicle={} type={} model={} age={} life={} remaining={} orders={}\n", vehicle->index.base(), to_underlying(vehicle->type), vehicle->engine_type.base(), vehicle->age.base(), vehicle->max_age.base(), vehicle->max_age.base() - vehicle->age.base(), vehicle->GetNumOrders());
	}
}

/** Complete pure original current-world query; all native owners remain byte-identical. */
TEST_CASE("Authentic whole launch entry has current geometry stock and explicit funding bounds", "[.surface-entry-gateway]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-gateway"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve original query failures through normal shutdown without compensating gameplay changes. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceGateway(paths.saves / "surface-gateway-before-query-440.sav"); QuerySurfaceGateway();
			SaveSurfaceGateway(paths.saves / "surface-gateway-query-440.sav");
			fmt::print("SURFACE_GATEWAY_COMPLETE supplied_stock=false supplied_work=false future_owner_created=false\n"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_gateway_observer, &inspect); IConsole::CmdRegister("surface_entry_gateway", ObserveSurfaceGateway);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
