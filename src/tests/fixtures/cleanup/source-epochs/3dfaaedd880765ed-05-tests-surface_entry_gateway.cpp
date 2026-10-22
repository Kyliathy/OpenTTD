/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_gateway.cpp Original launch admission, paid facility, physical supply and native Earth hull/payload producers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BIEI/15BIEQJ/15BIEQK/15BIEQL/15BIEQM/15BIEQN/15C.
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
#include "../misc_cmd.h"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../multilayer_space_content.h"
#include "../multilayer_content.h"
#include "../orbital_station.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../roadveh.h"
#include "../road_map.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../depot_base.h"
#include "../order_cmd.h"
#include "../vehicle_cmd.h"
#include "../train.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_entry_work.h"
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

static const std::array<SurfaceGatewayCarrier, 7> surface_gateway_carriers{{
	{"Surface Steel reserve 1", "steel1", CargoLabel{"STEL"}, 18, 10}, {"Surface Steel reserve 2", "steel2", CargoLabel{"STEL"}, 18, 10},
	{"Surface Goods reserve", "goods1", CargoLabel{"GOOD"}, 14, 11}, {"Surface Launch Goods reserve 1", "goods2", CargoLabel{"GOOD"}, 18, 11},
	{"Surface Launch Goods reserve 2", "goods3", CargoLabel{"GOOD"}, 18, 11}, {"Surface Propellant reserve", "fuel1", CargoLabel{"PROP"}, 20, 6},
	{"Surface Propellant refill", "fuel2", CargoLabel{"PROP"}, 22, 6}}}; ///< Original named carrier roles; never replacement inventory.

/** Original current admission values; copied diagnostics confer no gameplay ownership. */
struct SurfaceGatewayBudget {
	Money center{0}; ///< Complete current paid footprint and native clearing cost.
	Money target{0}; ///< Whole first-Core bill/reserve rounded to native bank steps.
};

/** @return Whole legal current arrangement/capital target without granting any absent facility or craft. */
static SurfaceGatewayBudget QuerySurfaceGateway()
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
	std::map<CargoLabel, uint64_t> stock, required; uint64_t handling_mass = 0; uint shipment_units = 0;
	// All seven original real carriers quote normal Transfer/NoLoad orders.
	// Receiver acceptance cannot turn the selected construction stock into an
	// accidental industry sale. Existing native packets remain in place here.
	for (const auto &role : surface_gateway_carriers) {
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
	return {center.GetCost(), target};
}

/** Quote then execute an actual native paid command once.
 * @tparam Tcmd Original synchronized command. @tparam Targs Native argument types.
 * @param tile Actual command position. @param args Complete original arguments.
 * @return Actual native invoice; normal command accounting owns company books.
 */
template <Commands Tcmd, typename... Targs>
static CommandCost PaySurfaceGateway(TileIndex tile, Targs... args)
{
	const auto quote = QuoteSurfaceGateway<Tcmd>(tile, args...); REQUIRE(quote.Succeeded());
	const auto paid = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(args...));
	REQUIRE(paid.Succeeded()); REQUIRE(paid.GetCost() == quote.GetCost());
	fmt::print("SURFACE_GATEWAY_INVOICE command={} cost={}\n", to_underlying(Tcmd), paid.GetCost().base());
	return paid;
}

/** Admit whole matching credit and optionally build/link only the real center.
 * @param execute False keeps the current world pure; true uses original bank, station and hub commands.
 */
static void BuildSurfaceGatewayCenter(bool execute)
{
	const auto budget = QuerySurfaceGateway(); const auto *company = Company::Get(_current_company);
	const auto cash = company->money, debt = company->current_loan;
	const auto shortfall = std::max(Money{0}, budget.target - cash);
	const Money credit{((shortfall.base() + 9999) / 10000) * 10000};
	REQUIRE(cash + company->GetMaxLoan() - debt >= budget.target);
	if (credit > 0) {
		const auto bank = QuoteSurfaceGateway<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit); REQUIRE(bank.Succeeded());
	}
	REQUIRE(debt + credit <= company->GetMaxLoan()); REQUIRE(cash + credit >= budget.target);
	fmt::print("SURFACE_GATEWAY_FACILITY_PLAN anchor={} rotation=1 member=0 center_cost={} target={} credit={} cash={} debt={} ceiling={} future_refund=0\n",
		surface_gateway_site.base(), budget.center.base(), budget.target.base(), credit.base(), cash.base(), debt.base(), company->GetMaxLoan().base());
	if (!execute) return;
	if (credit > 0) PaySurfaceGateway<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit);
	const auto paid = PaySurfaceGateway<Commands::BuildLaunchCenter>(surface_gateway_site, surface_gateway_site, uint8_t{1}, NEW_STATION, true);
	REQUIRE(paid.GetCost() == budget.center); const auto references = GetLaunchCenterReferences(); REQUIRE(references.size() == 1);
	const auto reference = references.front(); const auto *center = ResolveLaunchCenter(reference); REQUIRE(center != nullptr);
	REQUIRE(center->launch_center->tile == surface_gateway_site); REQUIRE(center->launch_center->rotation == 1);
	REQUIRE(center->owner == _current_company); REQUIRE(center->launch_operations != nullptr); REQUIRE(center->launch_hub != nullptr);
	// Resolve the actual newly allocated native Station/generation. Pool slots
	// used by Village or material receiver0 confer no authority to this center.
	REQUIRE(reference.station != StationID{0}); REQUIRE(reference.station != StationID{20}); REQUIRE(reference.generation > 0);
	REQUIRE(IsLaunchHubEligible(*center, *Station::Get(StationID{0})));
	PaySurfaceGateway<Commands::SetLaunchHub>(TileIndex{0}, StationID{0}, reference.station, reference.generation);
	const auto member = GetLaunchHubMembership(StationID{0}); REQUIRE(member.has_value()); REQUIRE_FALSE(member->joined);
	REQUIRE(member->center == reference); REQUIRE(member->member.generation > 0); REQUIRE(member->member.station == StationID{0});
	REQUIRE(ResolveLaunchHubMember(reference, member->member) == Station::Get(StationID{0}));
	REQUIRE(ValidateLaunchCenters()); REQUIRE(ValidateLaunchHubs()); REQUIRE(GetSpaceVehicleCount() == 0); REQUIRE(SpaceStructure::GetNumItems() == 0);
	const auto area = GetLaunchCenterFootprint(surface_gateway_site, 1); REQUIRE(area.has_value()); REQUIRE(HasLaunchCenterWaterAccess(*area));
	// Every admitted footprint cell belongs to the real native facility. No
	// adjacent road, canal or useful station is repainted to manufacture access.
	for (const auto tile : *area) {
		REQUIRE(IsTileType(tile, TileType::Station)); REQUIRE(GetStationIndex(tile) == reference.station); REQUIRE(GetTileOwner(tile) == _current_company);
	}
	REQUIRE(company->money == cash + credit - budget.center); REQUIRE(company->current_loan == debt + credit); REQUIRE(company->money >= 150000);
	fmt::print("SURFACE_GATEWAY_FACILITY_PAID station={} generation={} anchor={} rotation=1 member={} member_generation={} cost={} credit={} cash={} debt={} footprint={} cargo_moved=0 work=0\n",
		reference.station.base(), reference.generation, surface_gateway_site.base(), member->member.station.base(), member->member.generation,
		budget.center.base(), credit.base(), company->money.base(), company->current_loan.base(), area->w * area->h);
}

/** @return The actual paid owned center and its exact exclusive member0generation. */
static LaunchHubMembership SurfaceGatewaySupplyHub()
{
	const auto membership = GetLaunchHubMembership(StationID{0}); REQUIRE(membership.has_value()); REQUIRE_FALSE(membership->joined);
	const auto *center = ResolveLaunchCenter(membership->center); REQUIRE(center != nullptr); REQUIRE(center->owner == _current_company);
	REQUIRE(center->launch_center->tile == surface_gateway_site); REQUIRE(center->launch_center->rotation == 1);
	REQUIRE(ResolveLaunchHubMember(membership->center, membership->member) == Station::Get(StationID{0}));
	REQUIRE(GetSpaceVehicleCount() == 0); REQUIRE(SpaceStructure::GetNumItems() == 0);
	return *membership;
}

/** @param role Original selected material carrier. @return Its sole actual owned body, preserving remaining original packets. */
static const RoadVehicle *SurfaceGatewayTruck(const SurfaceGatewayCarrier &role)
{
	const RoadVehicle *found = nullptr;
	// Names identify original physical carriers independently of unrelated
	// recycled IDs. Every surviving packet retains its actual producing industry.
	for (const auto *truck : RoadVehicle::Iterate()) {
		if (truck->name != role.name) continue;
		REQUIRE(found == nullptr); found = truck;
	}
	REQUIRE(found != nullptr); REQUIRE(found->owner == _current_company); REQUIRE(found->cargo_type == GetCargoTypeByLabel(role.label));
	REQUIRE(found->cargo.TotalCount() <= role.quantity);
	for (const auto *packet : *found->cargo.Packets()) { REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == role.source); }
	return found;
}

/** @return Genuine material-transfer station order; NoLoad prevents replacement pickup or accidental consumption. */
static Order SurfaceGatewayTransferOrder()
{
	Order order; order.MakeGoToStation(StationID{0}); order.SetStopLocation(OrderStopLocation::FarEnd);
	order.SetLoadType(OrderLoadType::NoLoad); order.SetUnloadType(OrderUnloadType::Transfer); return order;
}

/** @param execute False quotes every current original order/control; true dispatches only these loaded actual carriers. */
static void DispatchSurfaceGatewaySupply(bool execute)
{
	const auto membership = SurfaceGatewaySupplyHub(); const auto *center = ResolveLaunchCenter(membership.center);
	for (const auto &[label, quantity] : std::array<std::pair<CargoLabel, uint>, 3>{{{CargoLabel{"STEL"}, 36}, {CargoLabel{"GOOD"}, 50}, {CargoLabel{"PROP"}, 42}}}) {
		REQUIRE(GetLaunchStockHeadroom(*center, GetCargoTypeByLabel(label)) >= quantity);
	}
	// Replace only the obsolete pickup order. Preserve the actual second
	// depot halt, including its native timetable, and select the new receiver
	// through original commands before restarting each genuinely loaded truck.
	for (const auto &role : surface_gateway_carriers) {
		const auto *truck = SurfaceGatewayTruck(role); REQUIRE(truck->IsStoppedInDepot()); REQUIRE(truck->tile == surface_gateway_depot);
		REQUIRE(truck->cargo.TotalCount() == role.quantity); REQUIRE(truck->GetNumOrders() == 2);
		const auto *home = truck->GetOrder(1); REQUIRE(home != nullptr); REQUIRE(home->IsType(OT_GOTO_DEPOT));
		REQUIRE(home->GetDestination() == DestinationID{Depot::GetByTile(surface_gateway_depot)->index.base()});
		const auto transfer = SurfaceGatewayTransferOrder();
		REQUIRE(QuoteSurfaceGateway<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}).Succeeded());
		REQUIRE(QuoteSurfaceGateway<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, transfer).Succeeded());
		REQUIRE(QuoteSurfaceGateway<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true).Succeeded());
		fmt::print("SURFACE_GATEWAY_SHIPMENT vehicle={} label={} held={} source={} receiver=0 depot={} admitted=true\n", truck->index.base(), role.label.AsString(), role.quantity, role.source, surface_gateway_depot.base());
		if (!execute) continue;
		PaySurfaceGateway<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
		PaySurfaceGateway<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, transfer);
		// Native insertion may already select order0. SkipToOrder refuses an
		// unchanged selection, so invoke it only when the real cursor differs.
		if (truck->cur_implicit_order_index != VehicleOrderID{0}) PaySurfaceGateway<Commands::SkipToOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
		PaySurfaceGateway<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true);
	}
}

/** @return True only when all seven real carriers returned empty and all original128units physically await at member0. */
static bool SurfaceGatewaySupplyArrived()
{
	for (const auto &role : surface_gateway_carriers) {
		const auto *truck = SurfaceGatewayTruck(role);
		// A destination order alone grants no cargo custody or empty-return
		// completion. Require both the original physical depot and empty body.
		if (!truck->IsStoppedInDepot() || truck->tile != surface_gateway_depot || truck->cargo.TotalCount() != 0) return false;
	}
	const auto *member = Station::Get(StationID{0});
	for (const auto &[label, quantity] : std::array<std::pair<CargoLabel, uint>, 3>{{{CargoLabel{"STEL"}, 36}, {CargoLabel{"GOOD"}, 50}, {CargoLabel{"PROP"}, 42}}}) {
		const auto cargo = GetCargoTypeByLabel(label);
		if (member->goods[cargo].TotalCount() != quantity) return false;
	}
	return true;
}

/** @param execute False quotes current stock-backed native handling; true reserves actual waiting packets in three reverse jobs. */
static void HandleSurfaceGatewaySupply(bool execute)
{
	REQUIRE(SurfaceGatewaySupplyArrived()); const auto membership = SurfaceGatewaySupplyHub();
	// Each actual material label fits one public64unit job. Preflight every
	// complete stock-backed quote before moving any first label into job custody.
	for (const auto &[label, quantity] : std::array<std::pair<CargoLabel, uint16_t>, 3>{{{CargoLabel{"STEL"}, 36}, {CargoLabel{"GOOD"}, 50}, {CargoLabel{"PROP"}, 42}}}) {
		const auto cargo = GetCargoTypeByLabel(label);
		REQUIRE(QuoteSurfaceGateway<Commands::StartSurfaceHandling>(TileIndex{0}, membership.member.station, membership.member.generation,
			membership.center.station, membership.center.generation, cargo, quantity, true).Succeeded());
		fmt::print("SURFACE_GATEWAY_HANDLING label={} quantity={} reverse=true admitted=true\n", label.AsString(), quantity);
	}
	if (!execute) return;
	for (const auto &[label, quantity] : std::array<std::pair<CargoLabel, uint16_t>, 3>{{{CargoLabel{"STEL"}, 36}, {CargoLabel{"GOOD"}, 50}, {CargoLabel{"PROP"}, 42}}}) {
		PaySurfaceGateway<Commands::StartSurfaceHandling>(TileIndex{0}, membership.member.station, membership.member.generation,
			membership.center.station, membership.center.generation, GetCargoTypeByLabel(label), quantity, true);
	}
}

/** @return True only after real native handling has placed all128original units into this actual center storage. */
static bool SurfaceGatewaySupplyStored()
{
	const auto membership = SurfaceGatewaySupplyHub(); const auto *center = ResolveLaunchCenter(membership.center);
	if (!center->launch_hub->handling.empty()) return false;
	for (const auto &[label, quantity] : std::array<std::pair<CargoLabel, uint>, 3>{{{CargoLabel{"STEL"}, 36}, {CargoLabel{"GOOD"}, 50}, {CargoLabel{"PROP"}, 42}}}) {
		if (center->goods[GetCargoTypeByLabel(label)].TotalCount() != quantity) return false;
	}
	return true;
}

/** @return Actual original paid center; this assembly scope admits its own genuinely commissioned bodies. */
static Station *SurfaceGatewayAssemblyCenter()
{
	const auto refs = GetLaunchCenterReferences(); REQUIRE(refs.size() == 1); auto *center = ResolveLaunchCenter(refs.front());
	REQUIRE(center != nullptr); REQUIRE(center->owner == _current_company); REQUIRE(center->launch_center->tile == surface_gateway_site);
	REQUIRE(center->launch_center->rotation == 1); REQUIRE(center->launch_operations != nullptr); REQUIRE(center->launch_hub->handling.empty());
	REQUIRE(SpaceStructure::GetNumItems() == 0); REQUIRE(GetSpaceVehicleCount() <= 2);
	return center;
}

/** @param model Actual selected Earth role. @return Its sole existing paid native hull, or nullptr before purchase. */
static const SpaceVehicle *SurfaceGatewayPaidHull(uint16_t model)
{
	const SpaceVehicle *found = nullptr;
	// Only the actual two selected Earth receipts belong to this producer.
	// Their native model, immutable payment and facility generation must agree;
	// a copied definition or a different hull grants no embodied material credit.
	for (const auto *craft : SpaceVehicle::Iterate()) {
		const auto actual = GetSpaceVehicleModel(*craft); REQUIRE((actual == 14 || actual == 20));
		REQUIRE(craft->owner == _current_company); REQUIRE(craft->construction.model == actual);
		REQUIRE(craft->construction.capital_paid == GetSpaceVehicleConstruction(actual)->capital);
		REQUIRE(craft->construction.home == GetLaunchCenterReference(*SurfaceGatewayAssemblyCenter()));
		if (actual != model) continue;
		REQUIRE(found == nullptr); found = craft;
	}
	return found;
}

/** Quote current remaining whole first-Core capital and native matching credit.
 * @param center Actual paid workshop/storage owner.
 * @return Necessary original bank credit rounded to10000, excluding future income or resale.
 */
static Money QuerySurfaceGatewayAssemblyBudget(const Station &center)
{
	const auto &budget = GetMultiLayerSpaceBudget(); const auto day = uint64_t{74};
	Money hulls{0};
	for (const auto model : {uint16_t{14}, uint16_t{20}}) if (SurfaceGatewayPaidHull(model) == nullptr) hulls += Money{GetSpaceVehicleConstruction(model)->capital};
	const auto *core = GetSpaceConstructionDefinition(SpaceBuildKind::Core);
	const Money mission{2 * budget.mission_fee};
	const Money pad{static_cast<int64_t>(2 * (uint64_t{budget.prepare_ticks} + budget.turnaround_ticks) * budget.pad_per_day / day)};
	const Money work{static_cast<int64_t>((uint64_t{core->work_ticks} * budget.active_per_day + day - 1) / day)};
	const auto planned = hulls + Money{core->capital} + mission + pad + work + Money{150000};
	const Money target{((planned.base() + 9999) / 10000) * 10000}; const auto *company = Company::Get(_current_company);
	const auto shortfall = std::max(Money{0}, target - company->money); const Money credit{((shortfall.base() + 9999) / 10000) * 10000};
	REQUIRE(company->money + company->GetMaxLoan() - company->current_loan >= target);
	REQUIRE(company->current_loan + credit <= company->GetMaxLoan());
	if (credit > 0) REQUIRE(QuoteSurfaceGateway<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit).Succeeded());
	fmt::print("SURFACE_GATEWAY_ASSEMBLY_BUDGET station={} generation={} hulls={} core={} missions={} pad={} work={} reserve=150000 planned={} target={} credit={} cash={} debt={} ceiling={}\n",
		center.index.base(), center.launch_center->generation, hulls.base(), core->capital, mission.base(), pad.base(), work.base(), planned.base(), target.base(),
		credit.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base());
	return credit;
}

/** Query both real stock-backed hull commands before committing the first exclusive workshop transaction. */
static void QuerySurfaceGatewayHulls()
{
	const auto *center = SurfaceGatewayAssemblyCenter(); REQUIRE(GetSpaceVehicleCount() == 0);
	QuerySurfaceGatewayAssemblyBudget(*center);
	for (const auto model : {uint16_t{14}, uint16_t{20}}) {
		const auto quote = QuoteSurfaceGateway<Commands::AssembleLaunchCraft>(TileIndex{0}, center->index, center->launch_center->generation, model);
		REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == Money{GetSpaceVehicleConstruction(model)->capital});
		fmt::print("SURFACE_GATEWAY_HULL_PLAN model={} cost={} work={} admitted=true\n", model, quote.GetCost().base(), GetSpaceVehicleConstruction(model)->work_ticks);
	}
}

/** @param model Selected Rocket14orConstructor20. Buy exactly one admitted original paid hull; no work or physical state is supplied. */
static void BuySurfaceGatewayHull(uint16_t model)
{
	auto *center = SurfaceGatewayAssemblyCenter(); REQUIRE(SurfaceGatewayPaidHull(model) == nullptr);
	if (model == 20) {
		const auto *rocket = SurfaceGatewayPaidHull(14); REQUIRE(rocket != nullptr); REQUIRE(rocket->construction.placement == SpacePlacement::SurfaceStored);
	}
	REQUIRE(center->launch_operations->workshop == SpaceCraftRef{});
	const auto credit = QuerySurfaceGatewayAssemblyBudget(*center); const auto before = GetSpaceVehicleCount(); const auto generation = GetNextSpaceVehicleGeneration();
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	std::map<CargoLabel, uint> stock;
	const auto *definition = GetSpaceVehicleConstruction(model);
	for (const auto &input : definition->inputs) stock[input.label] = center->goods[GetCargoTypeByLabel(input.label)].TotalCount();
	if (credit > 0) PaySurfaceGateway<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit);
	const auto paid = PaySurfaceGateway<Commands::AssembleLaunchCraft>(TileIndex{0}, center->index, center->launch_center->generation, model);
	const auto *craft = SurfaceGatewayPaidHull(model); REQUIRE(craft != nullptr); REQUIRE(GetSpaceVehicleCount() == before + 1);
	REQUIRE(craft->generation == generation); REQUIRE(GetNextSpaceVehicleGeneration() == generation + 1);
	REQUIRE(craft->construction.placement == SpacePlacement::Assembling); REQUIRE(craft->construction.remaining_ticks == definition->work_ticks);
	REQUIRE(center->launch_operations->workshop == GetSpaceCraftReference(*craft)); REQUIRE(craft->cargo.TotalCount() == 0);
	// Only the canonical native recipe may consume physical packet quantities;
	// each actually paid hull now embodies its own immutable original inputs.
	for (const auto &input : definition->inputs) REQUIRE(center->goods[GetCargoTypeByLabel(input.label)].TotalCount() + input.quantity == stock.at(input.label));
	REQUIRE(company->money == cash + credit - paid.GetCost()); REQUIRE(company->current_loan == debt + credit); REQUIRE(company->money >= 150000);
	REQUIRE(ValidateSpaceProduction());
	fmt::print("SURFACE_GATEWAY_ASSEMBLED model={} vehicle={} generation={} cost={} credit={} remaining={} cash={} debt={} supplied_work=false\n",
		model, craft->index.base(), craft->generation, paid.GetCost().base(), credit.base(), craft->construction.remaining_ticks, company->money.base(), company->current_loan.base());
}

/** @param model Selected original paid hull. @return True only after genuine workshop completion released this actual body into native storage. */
static bool SurfaceGatewayHullStored(uint16_t model)
{
	const auto *craft = SurfaceGatewayPaidHull(model);
	return craft != nullptr && craft->construction.placement == SpacePlacement::SurfaceStored && craft->construction.remaining_ticks == 0;
}

/** @return Current whole remaining Core/reserve credit after all original stock-backed loading/carry quotes. */
static Money QuerySurfaceGatewayPayload()
{
	const auto *center = SurfaceGatewayAssemblyCenter(); REQUIRE(SurfaceGatewayHullStored(14)); REQUIRE(SurfaceGatewayHullStored(20));
	const auto *rocket = SurfaceGatewayPaidHull(14), *constructor = SurfaceGatewayPaidHull(20);
	REQUIRE(rocket->cargo.TotalCount() == 0); REQUIRE(constructor->cargo.TotalCount() == 0);
	const auto credit = QuerySurfaceGatewayAssemblyBudget(*center); const auto steel = GetCargoTypeByLabel(CargoLabel{"STEL"});
	const auto core = QuoteSurfaceGateway<Commands::BuildSpaceBlueprint>(TileIndex{11274}, TileIndex{11274}, SpaceBuildKind::Core);
	REQUIRE(core.Succeeded()); REQUIRE(core.GetCost() == Money{GetSpaceConstructionDefinition(SpaceBuildKind::Core)->capital});
	REQUIRE(QuoteSurfaceGateway<Commands::MoveLaunchFreight>(TileIndex{0}, center->index, center->launch_center->generation,
		constructor->index, constructor->generation, steel, uint16_t{16}, false).Succeeded());
	REQUIRE(QuoteSurfaceGateway<Commands::MoveSpaceSupply>(TileIndex{0}, center->index, center->launch_center->generation,
		rocket->index, rocket->generation, to_underlying(SpaceSupply::Propellant), uint16_t{30}, false).Succeeded());
	REQUIRE(QuoteSurfaceGateway<Commands::CarryLaunchProbe>(TileIndex{0}, rocket->index, rocket->generation, constructor->index, constructor->generation, false).Succeeded());
	// Pure current quotes admit each original command. The complete future
	// combined paid body/freight mass is additionally bounded now, and actual
	// packing must freshly quote again after both physical stock moves.
	const auto mass = GetSpaceVehicleConstruction(20)->dry_mass_sixteenths + uint64_t{16} * CargoSpec::Get(steel)->weight;
	REQUIRE(mass == 384); REQUIRE(mass <= GetMultiLayerSpaceBudget().rocket_payload_sixteenths);
	REQUIRE(GetSpaceFreightCapacity(20, steel) >= 16);
	fmt::print("SURFACE_GATEWAY_PAYLOAD_PLAN rocket={} rocket_generation={} constructor={} constructor_generation={} steel=16 fuel=30 mass={} capacity={} core_tile=11274 core_cost={} admitted=true\n",
		rocket->index.base(), rocket->generation, constructor->index.base(), constructor->generation, mass, GetMultiLayerSpaceBudget().rocket_payload_sixteenths, core.GetCost().base());
	return credit;
}

/** @return Whether the actual paid constructor/freight/fuel are held by the same original stored rocket, without starting flight. */
static bool SurfaceGatewayPayloadReady()
{
	const auto *rocket = SurfaceGatewayPaidHull(14), *constructor = SurfaceGatewayPaidHull(20);
	// Reciprocal exact native references, whole first-flight freight and real
	// fuel are jointly required. An empty body, order or numeric ID alone grants
	// no loaded payload/flight authority.
	return rocket != nullptr && constructor != nullptr && rocket->construction.placement == SpacePlacement::SurfaceStored &&
		constructor->construction.placement == SpacePlacement::Carried && rocket->manifest && constructor->manifest &&
		rocket->manifest->passenger == GetSpaceCraftReference(*constructor) && constructor->manifest->carrier == GetSpaceCraftReference(*rocket) &&
		rocket->cargo.TotalCount() == 0 && constructor->cargo_type == GetCargoTypeByLabel(CargoLabel{"STEL"}) && constructor->cargo.TotalCount() == 16 &&
		rocket->manifest->supplies[0].TotalCount() == 30 && rocket->manifest->fuel_burned == 0 && GetSpacePayloadMass(*rocket) == 384;
}

/** Freshly quote the actual generation-qualified first flight; this creates no transition, fee or fuel burn. */
static void QuerySurfaceGatewayFirstFlight()
{
	REQUIRE(SurfaceGatewayPayloadReady()); const auto *center = SurfaceGatewayAssemblyCenter(), *rocket_center = ResolveLaunchCenter(SurfaceGatewayPaidHull(14)->construction.home);
	REQUIRE(center == rocket_center); const auto *rocket = SurfaceGatewayPaidHull(14);
	const auto flight = QuoteSurfaceGateway<Commands::BeginLaunchFlight>(TileIndex{0}, rocket->index, rocket->generation,
		SpaceOrderTargetKind::LaunchCenter, static_cast<uint32_t>(center->index.base()), center->launch_center->generation);
	REQUIRE(flight.Succeeded()); REQUIRE(flight.GetCost() == Money{GetMultiLayerSpaceBudget().mission_fee});
	fmt::print("SURFACE_GATEWAY_FLIGHT_PLAN rocket={} generation={} target={} target_generation={} fee={} mass={} fuel=30 admitted=true begun=false\n",
		rocket->index.base(), rocket->generation, center->index.base(), center->launch_center->generation, flight.GetCost().base(), GetSpacePayloadMass(*rocket));
}

/** Load actual original materials/fuel and carry the same paid constructor using native commands only. */
static void LoadSurfaceGatewayPayload()
{
	const auto credit = QuerySurfaceGatewayPayload(); const auto *center = SurfaceGatewayAssemblyCenter();
	const auto *rocket = SurfaceGatewayPaidHull(14), *constructor = SurfaceGatewayPaidHull(20);
	if (credit > 0) PaySurfaceGateway<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit);
	PaySurfaceGateway<Commands::MoveLaunchFreight>(TileIndex{0}, center->index, center->launch_center->generation,
		constructor->index, constructor->generation, GetCargoTypeByLabel(CargoLabel{"STEL"}), uint16_t{16}, false);
	PaySurfaceGateway<Commands::MoveSpaceSupply>(TileIndex{0}, center->index, center->launch_center->generation,
		rocket->index, rocket->generation, to_underlying(SpaceSupply::Propellant), uint16_t{30}, false);
	PaySurfaceGateway<Commands::CarryLaunchProbe>(TileIndex{0}, rocket->index, rocket->generation, constructor->index, constructor->generation, false);
	REQUIRE(SurfaceGatewayPayloadReady()); QuerySurfaceGatewayFirstFlight();
	fmt::print("SURFACE_GATEWAY_PAYLOAD_LOADED rocket={} constructor={} steel=16 fuel=30 mass={} credit={} begun=false\n", rocket->index.base(), constructor->index.base(), GetSpacePayloadMass(*rocket), credit.base());
}

/** @return Whether the original first mission has deployed its real loaded constructor and recovered the same paid rocket. */
static bool SurfaceGatewayFirstMissionComplete()
{
	const auto *rocket = SurfaceGatewayPaidHull(14), *constructor = SurfaceGatewayPaidHull(20);
	// Only the original completed round trip, retained loaded constructor and
	// accountable fifteen-unit burn authorize the next Core construction step.
	return rocket != nullptr && constructor != nullptr && rocket->transition && rocket->manifest && constructor->manifest &&
		rocket->transition->phase == SpaceFlightPhase::GroundLoading && rocket->transition->missions_started == 1 &&
		rocket->transition->missions_completed == 1 && rocket->transition->missions_canceled == 0 &&
		rocket->construction.placement == SpacePlacement::SurfaceStored && constructor->construction.placement == SpacePlacement::Orbital &&
		constructor->construction.deployed && constructor->cargo.TotalCount() == 16 && rocket->manifest->supplies[0].TotalCount() == 15 &&
		rocket->manifest->fuel_burned == 15 && rocket->manifest->passenger == SpaceCraftRef{} && constructor->manifest->carrier == SpaceCraftRef{};
}

/** @param phase Explicit paid departure, synchronized flight boundary or completed useful-service interval. */
static void FlySurfaceGatewayFirstMission(const std::string &phase)
{
	if (phase == "mission-query") { QuerySurfaceGatewayAssemblyBudget(*SurfaceGatewayAssemblyCenter()); QuerySurfaceGatewayFirstFlight(); return; }
	if (phase == "mission-begin") {
		QuerySurfaceGatewayFirstFlight(); const auto *rocket = SurfaceGatewayPaidHull(14); const auto *center = SurfaceGatewayAssemblyCenter();
		PaySurfaceGateway<Commands::BeginLaunchFlight>(TileIndex{0}, rocket->index, rocket->generation, SpaceOrderTargetKind::LaunchCenter,
			static_cast<uint32_t>(center->index.base()), center->launch_center->generation);
		REQUIRE(rocket->transition); REQUIRE(rocket->transition->phase == SpaceFlightPhase::Preparing);
	} else if (phase == "mission-service") {
		REQUIRE(SurfaceGatewayFirstMissionComplete());
		/** @return False to earn the complete ordinary service interval while keeping the already deployed native constructor idle. */
		const auto service = [] { return false; };
		EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, service); REQUIRE(SurfaceGatewayFirstMissionComplete());
	} else {
		SpaceFlightPhase before = SpaceFlightPhase::Preparing, after = SpaceFlightPhase::Ascent;
		if (phase == "mission-orbit") { before = SpaceFlightPhase::Ascent; after = SpaceFlightPhase::OrbitalDelivery; }
		else if (phase == "mission-deploy") { before = SpaceFlightPhase::OrbitalDelivery; after = SpaceFlightPhase::ReturnRecovery; }
		else if (phase == "mission-recover") { before = SpaceFlightPhase::ReturnRecovery; after = SpaceFlightPhase::GroundLoading; }
		else REQUIRE(phase == "mission-ascent");
		const auto *rocket = SurfaceGatewayPaidHull(14); REQUIRE(rocket->transition); REQUIRE(rocket->transition->phase == before);
		/** @return Whether the actual original transition reached the selected saved phase through synchronized native ticking. */
		const auto reached = [after] { const auto *craft = SurfaceGatewayPaidHull(14); return craft->transition->phase == after; };
		EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, reached); REQUIRE(reached());
		if (phase == "mission-recover") REQUIRE(SurfaceGatewayFirstMissionComplete());
	}
	const auto *rocket = SurfaceGatewayPaidHull(14), *constructor = SurfaceGatewayPaidHull(20); const auto &flight = *rocket->transition;
	fmt::print("SURFACE_GATEWAY_MISSION_STATE rocket={} generation={} phase={} blocked={} layer={} remaining={} fuel={} burned={} started={} completed={} canceled={} constructor={} constructor_generation={} placement={} deployed={} steel={} core=false\n",
		rocket->index.base(), rocket->generation, to_underlying(flight.phase), to_underlying(flight.blocked), to_underlying(flight.layer), flight.remaining_ticks,
		rocket->manifest->supplies[0].TotalCount(), rocket->manifest->fuel_burned, flight.missions_started, flight.missions_completed, flight.missions_canceled,
		constructor->index.base(), constructor->generation, to_underlying(constructor->construction.placement), constructor->construction.deployed, constructor->cargo.TotalCount());
}

/** Complete original current-world query or explicitly selected paid native gateway/mission boundary. */
TEST_CASE("Authentic whole launch entry has current geometry stock and explicit funding bounds", "[.surface-entry-gateway]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_GATEWAY_PHASE"); const std::string phase = selected == nullptr ? "query" : selected;
	// Only named pure/current paid boundaries may execute; the absent selector
	// keeps the original whole-arrangement query entirely read-only.
	REQUIRE((phase == "query" || phase == "center-query" || phase == "center" || phase == "shipment-query" || phase == "dispatch" || phase == "ship-work" || phase == "handling-query" || phase == "handling" || phase == "handle-work" || phase == "service" || phase == "assembly-query" || phase == "rocket" || phase == "constructor" || phase == "rocket-work" || phase == "constructor-work" || phase == "assembly-service" || phase == "payload-query" || phase == "payload" || phase == "flight-query" || phase == "payload-service" || phase == "mission-query" || phase == "mission-begin" || phase == "mission-ascent" || phase == "mission-orbit" || phase == "mission-deploy" || phase == "mission-recover" || phase == "mission-service"));
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-gateway"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve original query failures through normal shutdown without compensating gameplay changes. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceGateway(paths.saves / fmt::format("surface-gateway-before-{}-440.sav", phase));
			if (phase == "query") QuerySurfaceGateway();
			else if (phase == "center-query" || phase == "center") BuildSurfaceGatewayCenter(phase == "center"); // Only explicit real facility phases may create the paid center; all supply phases require its existing native identity.
			else if (phase == "shipment-query" || phase == "dispatch") DispatchSurfaceGatewaySupply(phase == "dispatch"); // Query preserves all stock; dispatch changes only original orders/control and requires actual full carriers.
			else if (phase == "ship-work") EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, SurfaceGatewaySupplyArrived);
			else if (phase == "handling-query" || phase == "handling") HandleSurfaceGatewaySupply(phase == "handling"); // Only actual waiting stock admits reverse handling; queries cannot create job custody.
			else if (phase == "handle-work") EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, SurfaceGatewaySupplyStored);
			else if (phase == "service") {
				REQUIRE(SurfaceGatewaySupplyStored());
				/** @return False so ordinary proof earns the full bounded useful-service interval with original stored stock. */
				const auto continue_service = [] { return false; };
				EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, continue_service); REQUIRE(SurfaceGatewaySupplyStored());
			}
			else if (phase == "assembly-query") QuerySurfaceGatewayHulls();
			else if (phase == "rocket") BuySurfaceGatewayHull(14);
			else if (phase == "constructor") BuySurfaceGatewayHull(20);
			else if (phase == "rocket-work" || phase == "constructor-work") { // Actual exclusive workshop progress is earned separately for each originally paid hull, never inferred from another body.
				const uint16_t model = phase == "rocket-work" ? 14 : 20;
				/** @return Whether this actual selected native hull completed its own paid workshop work. */
				const auto stored = [model] { return SurfaceGatewayHullStored(model); };
				EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, stored);
			}
			else if (phase == "assembly-service") {
				REQUIRE(SurfaceGatewayHullStored(14)); REQUIRE(SurfaceGatewayHullStored(20));
				/** @return False to earn the full ordinary useful-service proof while both genuine paid hulls remain stored. */
				const auto service = [] { return false; };
				EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, service);
			}
			else if (phase == "payload-query") QuerySurfaceGatewayPayload();
			else if (phase == "payload") LoadSurfaceGatewayPayload();
			else if (phase == "flight-query") QuerySurfaceGatewayFirstFlight();
			else if (phase == "payload-service") {
				REQUIRE(SurfaceGatewayPayloadReady());
				/** @return False so ground proof earns the full original service interval without beginning flight or burning fuel. */
				const auto service = [] { return false; };
				EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, service); REQUIRE(SurfaceGatewayPayloadReady());
			}
			else if (phase.starts_with("mission-")) FlySurfaceGatewayFirstMission(phase);
			SaveSurfaceGateway(paths.saves / fmt::format("surface-gateway-{}-440.sav", phase));
			fmt::print("SURFACE_GATEWAY_COMPLETE supplied_stock=false supplied_work=false center_created={}\n", phase == "center"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_gateway_observer, &inspect); IConsole::CmdRegister("surface_entry_gateway", ObserveSurfaceGateway);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
