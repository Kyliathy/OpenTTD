/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_gateway.cpp Original paid launch, physical supply, native orbital construction and expansion admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BIEI/15BIEQJ/15BIEQK/15BIEQL/15BIEQM/15BIEQN/15C/15CB/15CC/15CD/15CE/15CF.
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
#include "../orbital_freight.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_transition.h"
#include "../space_vehicle_cmd.h"
#include "../roadveh.h"
#include "../industry.h"
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
#include <charconv>
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

/** @param allow_core Explicit first-Core consumer; the original launch/mission modes require no orbital site.
 * @return Actual original paid center, admitting only its own genuinely commissioned bodies. */
static Station *SurfaceGatewayAssemblyCenter(bool allow_core = false)
{
	const auto refs = GetLaunchCenterReferences(); REQUIRE(refs.size() == 1); auto *center = ResolveLaunchCenter(refs.front());
	REQUIRE(center != nullptr); REQUIRE(center->owner == _current_company); REQUIRE(center->launch_center->tile == surface_gateway_site);
	REQUIRE(center->launch_center->rotation == 1); REQUIRE(center->launch_operations != nullptr); REQUIRE(center->launch_hub->handling.empty());
	REQUIRE(SpaceStructure::GetNumItems() <= (allow_core ? 1 : 0)); REQUIRE(GetSpaceVehicleCount() <= 2);
	return center;
}

/** @param model Actual selected Earth role. @param allow_core Explicit first-Core consumer; all earlier defaults retain their absent-site bound.
 * @return Its sole existing paid native hull, or nullptr before purchase. */
static const SpaceVehicle *SurfaceGatewayPaidHull(uint16_t model, bool allow_core = false)
{
	const SpaceVehicle *found = nullptr;
	// Only the actual two selected Earth receipts belong to this producer.
	// Their native model, immutable payment and facility generation must agree;
	// a copied definition or a different hull grants no embodied material credit.
	for (const auto *craft : SpaceVehicle::Iterate()) {
		const auto actual = GetSpaceVehicleModel(*craft); REQUIRE((actual == 14 || actual == 20));
		REQUIRE(craft->owner == _current_company); REQUIRE(craft->construction.model == actual);
		REQUIRE(craft->construction.capital_paid == GetSpaceVehicleConstruction(actual)->capital);
		REQUIRE(craft->construction.home == GetLaunchCenterReference(*SurfaceGatewayAssemblyCenter(allow_core)));
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

/** @param allow_construction Explicit later work/activation consumer; all earlier modes retain their zero-work Steel-only blueprint bound.
 * @return The sole actual original paid first Core, or nullptr before the native build command. */
static const SpaceStructure *SurfaceGatewayOriginalCore(bool allow_construction = false)
{
	REQUIRE(SpaceStructure::GetNumItems() <= 1);
	for (const auto *site : SpaceStructure::Iterate()) {
		REQUIRE(site->state.kind == SpaceBuildKind::Core); REQUIRE(site->state.owner == _current_company);
		REQUIRE(site->state.x == 10); REQUIRE(site->state.y == 48); REQUIRE(site->state.capital_paid == 700000);
		if (!allow_construction) {
			REQUIRE(site->state.phase == SpaceStructurePhase::Blueprint); REQUIRE(site->module == nullptr);
			REQUIRE(site->job.work_ticks == 0); REQUIRE(site->job.inputs[1].TotalCount() == 0);
		} else {
			// Only the same paid blueprint's native work/frame progression belongs
			// to this later consumer; cancellation or another module grants none.
			REQUIRE((site->state.phase == SpaceStructurePhase::Blueprint || site->state.phase == SpaceStructurePhase::FrameComplete));
			REQUIRE(site->job.work_ticks <= 740);
		}
		return site;
	}
	return nullptr;
}

/** Freshly quote the real first-Core site and whole remaining paid mission/work/reserve bill before spending. */
static void QuerySurfaceGatewayCore()
{
	REQUIRE(SurfaceGatewayOriginalCore() == nullptr); REQUIRE(SurfaceGatewayFirstMissionComplete());
	const auto quote = QuoteSurfaceGateway<Commands::BuildSpaceBlueprint>(TileIndex{11274}, TileIndex{11274}, SpaceBuildKind::Core);
	REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == Money{700000});
	const auto &budget = GetMultiLayerSpaceBudget(); const auto *company = Company::Get(_current_company);
	const Money mission{budget.mission_fee}, pad{3000}, work{5000}, reserve{150000};
	const auto planned = quote.GetCost() + mission + pad + work + reserve;
	const Money target{((planned.base() + 9999) / 10000) * 10000};
	REQUIRE(company->money + company->GetMaxLoan() - company->current_loan >= target); REQUIRE(company->money >= target);
	// The unpaid target has no issued identity yet. Disclose the real paid
	// constructor now; a generation-bound order quote must follow purchase.
	const auto *constructor = SurfaceGatewayPaidHull(20); REQUIRE(GetSpaceVehicleModel(*constructor) == 20);
	fmt::print("SURFACE_GATEWAY_CORE_PLAN tile=11274 cost={} missions={} pad={} work={} reserve={} planned={} target={} cash={} debt={} credit=0 constructor={} constructor_generation={} model=20 target_exists=false\n",
		quote.GetCost().base(), mission.base(), pad.base(), work.base(), reserve.base(), planned.base(), target.base(), company->money.base(), company->current_loan.base(),
		constructor->index.base(), constructor->generation);
}

/** @return Whether real navigation delivered all16original Steel into the actual native job, still blocked only by missing Goods. */
static bool SurfaceGatewayCoreSteelReady()
{
	const auto *site = SurfaceGatewayOriginalCore();
	const auto *constructor = SurfaceGatewayPaidHull(20, true);
	// Complete physical Steel custody, an empty original constructor hold and
	// the live native MaterialsMissing wait jointly authorize the next resupply.
	return site != nullptr && site->job.inputs[0].TotalCount() == 16 && constructor->cargo.TotalCount() == 0 &&
		constructor->space_orders.blocked == SpaceOrderBlock::MaterialsMissing;
}

/** @param phase Explicit original Core quote, native paid placement/order or bounded synchronized material/service work. */
static void BuildSurfaceGatewayCore(const std::string &phase)
{
	if (phase == "core-query") { QuerySurfaceGatewayCore(); return; }
	if (phase == "core-buy") {
		QuerySurfaceGatewayCore(); PaySurfaceGateway<Commands::BuildSpaceBlueprint>(TileIndex{11274}, TileIndex{11274}, SpaceBuildKind::Core);
		REQUIRE(SurfaceGatewayOriginalCore() != nullptr);
	} else if (phase == "core-order-query" || phase == "core-order") { // Only the actual paid blueprint's issued identity may admit constructor orders; a pure quote cannot create work.
		const auto *site = SurfaceGatewayOriginalCore(); REQUIRE(site != nullptr); const auto *constructor = SurfaceGatewayPaidHull(20, true);
		REQUIRE(constructor->space_orders.entries.empty()); REQUIRE(constructor->cargo.TotalCount() == 16);
		const auto quote = QuoteSurfaceGateway<Commands::EditSpaceOrder>(TileIndex{0}, constructor->index, constructor->generation, SpaceOrderEdit::Insert, uint8_t{0},
			SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, static_cast<uint32_t>(site->index.base()), site->generation);
		REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == Money{0});
		if (phase == "core-order") PaySurfaceGateway<Commands::EditSpaceOrder>(TileIndex{0}, constructor->index, constructor->generation, SpaceOrderEdit::Insert, uint8_t{0},
			SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, static_cast<uint32_t>(site->index.base()), site->generation);
		fmt::print("SURFACE_GATEWAY_CORE_ORDER site={} generation={} constructor={} constructor_generation={} admitted=true executed={}\n",
			site->index.base(), site->generation, constructor->index.base(), constructor->generation, phase == "core-order");
	} else if (phase == "core-steel-work") {
		REQUIRE(!SurfaceGatewayCoreSteelReady()); EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, SurfaceGatewayCoreSteelReady); REQUIRE(SurfaceGatewayCoreSteelReady());
	} else {
		REQUIRE(phase == "core-service"); REQUIRE(SurfaceGatewayCoreSteelReady());
		/** @return False to earn the full ordinary useful-service interval while actual missing Goods prevents all Core construction work. */
		const auto service = [] { return false; };
		EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, service); REQUIRE(SurfaceGatewayCoreSteelReady());
	}
	const auto *site = SurfaceGatewayOriginalCore(); REQUIRE(site != nullptr); const auto *constructor = SurfaceGatewayPaidHull(20, true);
	fmt::print("SURFACE_GATEWAY_CORE_STATE site={} generation={} capital={} work={} steel={} goods={} constructor={} held={} blocked={} paid=true\n",
		site->index.base(), site->generation, site->state.capital_paid, site->job.work_ticks, site->job.inputs[0].TotalCount(), site->job.inputs[1].TotalCount(),
		constructor->index.base(), constructor->cargo.TotalCount(), to_underlying(constructor->space_orders.blocked));
}

/** Query actual remaining resupply/work/reserve capital without prospective income or another loan. */
static void QuerySurfaceGatewayResupplyBudget()
{
	const auto *company = Company::Get(_current_company); const auto &budget = GetMultiLayerSpaceBudget();
	const Money mission{budget.mission_fee}, pad{1500}, work{5000}, reserve{150000};
	const auto planned = mission + pad + work + reserve; const Money target{((planned.base() + 9999) / 10000) * 10000};
	REQUIRE(company->money >= target); REQUIRE(company->current_loan <= company->GetMaxLoan());
	fmt::print("SURFACE_GATEWAY_RESUPPLY_BUDGET mission={} pad={} work={} reserve={} planned={} target={} cash={} debt={} credit=0\n",
		mission.base(), pad.base(), work.base(), reserve.base(), planned.base(), target.base(), company->money.base(), company->current_loan.base());
}

/** Purely admit real Goods loading/native constructor release; defer flight until those actual owners change. */
static void QuerySurfaceGatewayResupply()
{
	REQUIRE(SurfaceGatewayCoreSteelReady()); QuerySurfaceGatewayResupplyBudget();
	const auto *site = SurfaceGatewayOriginalCore(); const auto *center = SurfaceGatewayAssemblyCenter(true);
	const auto *constructor = SurfaceGatewayPaidHull(20, true), *rocket = SurfaceGatewayPaidHull(14, true);
	REQUIRE(rocket->construction.placement == SpacePlacement::SurfaceStored); REQUIRE(rocket->cargo.TotalCount() == 0);
	REQUIRE(rocket->manifest->supplies[0].TotalCount() == 15); REQUIRE(rocket->manifest->fuel_burned == 15);
	REQUIRE(QuoteSurfaceGateway<Commands::EditSpaceOrder>(TileIndex{0}, constructor->index, constructor->generation, SpaceOrderEdit::Clear, uint8_t{0},
		SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, static_cast<uint32_t>(site->index.base()), site->generation).Succeeded());
	REQUIRE(QuoteSurfaceGateway<Commands::MoveLaunchFreight>(TileIndex{0}, center->index, center->launch_center->generation,
		rocket->index, rocket->generation, GetCargoTypeByLabel(CargoLabel{"GOOD"}), uint16_t{24}, false).Succeeded());
	REQUIRE(GetSpaceResupplyBlock(*rocket, GetSpaceCraftReference(*constructor)) == SpaceFlightBlock::ReceiverBusy);
	fmt::print("SURFACE_GATEWAY_RESUPPLY_PLAN rocket={} generation={} constructor={} constructor_generation={} goods=24 mass=192 capacity={} fuel=15 release_admitted=true load_admitted=true pending_order_busy=true flight_begun=false\n",
		rocket->index.base(), rocket->generation, constructor->index.base(), constructor->generation, GetMultiLayerSpaceBudget().rocket_payload_sixteenths);
}

/** Freshly quote the actual loaded Goods rocket and now-idle original Craft receiver before paying the second mission. */
static void QuerySurfaceGatewayResupplyFlight()
{
	QuerySurfaceGatewayResupplyBudget(); const auto *site = SurfaceGatewayOriginalCore(); REQUIRE(site != nullptr);
	const auto *constructor = SurfaceGatewayPaidHull(20, true), *rocket = SurfaceGatewayPaidHull(14, true);
	REQUIRE(site->job.inputs[0].TotalCount() == 16); REQUIRE(constructor->space_orders.entries.empty()); REQUIRE(constructor->cargo.TotalCount() == 0);
	REQUIRE(rocket->cargo_type == GetCargoTypeByLabel(CargoLabel{"GOOD"})); REQUIRE(rocket->cargo.TotalCount() == 24);
	REQUIRE(GetSpacePayloadMass(*rocket) == 192); REQUIRE(rocket->manifest->supplies[0].TotalCount() == 15);
	REQUIRE(GetSpaceResupplyBlock(*rocket, GetSpaceCraftReference(*constructor)) == SpaceFlightBlock::None);
	const auto quote = QuoteSurfaceGateway<Commands::BeginLaunchFlight>(TileIndex{0}, rocket->index, rocket->generation,
		SpaceOrderTargetKind::Craft, static_cast<uint32_t>(constructor->index.base()), constructor->generation);
	REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == Money{5000});
	fmt::print("SURFACE_GATEWAY_RESUPPLY_FLIGHT_PLAN rocket={} generation={} target={} target_generation={} fee={} mass={} fuel=15 admitted=true begun=false\n",
		rocket->index.base(), rocket->generation, constructor->index.base(), constructor->generation, quote.GetCost().base(), GetSpacePayloadMass(*rocket));
}

/** @return Whether the same two paid hulls completed the actual second mission with all24Goods and exact total fuel burn. */
static bool SurfaceGatewayResupplyComplete()
{
	const auto *site = SurfaceGatewayOriginalCore(); const auto *rocket = SurfaceGatewayPaidHull(14, true), *constructor = SurfaceGatewayPaidHull(20, true);
	// Only the real second recovery, full receiver freight and exclusive Steel
	// job authorize restoring Construct; mission metadata alone owns no stock.
	return site != nullptr && site->job.inputs[0].TotalCount() == 16 && rocket->transition &&
		rocket->transition->phase == SpaceFlightPhase::GroundLoading && rocket->transition->missions_started == 2 && rocket->transition->missions_completed == 2 &&
		rocket->transition->missions_canceled == 0 && rocket->construction.placement == SpacePlacement::SurfaceStored && rocket->cargo.TotalCount() == 0 &&
		rocket->manifest->supplies[0].TotalCount() == 0 && rocket->manifest->fuel_burned == 30 && constructor->construction.placement == SpacePlacement::Orbital &&
		constructor->space_orders.entries.empty() && constructor->cargo_type == GetCargoTypeByLabel(CargoLabel{"GOOD"}) && constructor->cargo.TotalCount() == 24;
}

/** Print the actual second mission and current goods/job owners without changing any physical state. */
static void ObserveSurfaceGatewayResupply()
{
	const auto *site = SurfaceGatewayOriginalCore(); const auto *rocket = SurfaceGatewayPaidHull(14, true), *constructor = SurfaceGatewayPaidHull(20, true);
	const auto &flight = *rocket->transition;
	fmt::print("SURFACE_GATEWAY_RESUPPLY_STATE phase={} blocked={} layer={} remaining={} started={} completed={} canceled={} fuel={} burned={} rocket_goods={} constructor_goods={} job_steel={} job_goods={} core_work={}\n",
		to_underlying(flight.phase), to_underlying(flight.blocked), to_underlying(flight.layer), flight.remaining_ticks,
		flight.missions_started, flight.missions_completed, flight.missions_canceled, rocket->manifest->supplies[0].TotalCount(), rocket->manifest->fuel_burned,
		rocket->cargo.TotalCount(), constructor->cargo.TotalCount(), site->job.inputs[0].TotalCount(), site->job.inputs[1].TotalCount(), site->job.work_ticks);
}

/** @param phase Explicit resupply quote, real release/loading, paid flight or completed useful-service interval.
 * @param saves Actual invocation-owned artifact directory for native intermediate phase checkpoints. */
static void ResupplySurfaceGatewayCore(const std::string &phase, const std::filesystem::path &saves)
{
	if (phase == "resupply-query") { QuerySurfaceGatewayResupply(); return; }
	if (phase == "resupply-load") {
		QuerySurfaceGatewayResupply(); const auto *site = SurfaceGatewayOriginalCore(); const auto *center = SurfaceGatewayAssemblyCenter(true);
		const auto *constructor = SurfaceGatewayPaidHull(20, true), *rocket = SurfaceGatewayPaidHull(14, true);
		PaySurfaceGateway<Commands::EditSpaceOrder>(TileIndex{0}, constructor->index, constructor->generation, SpaceOrderEdit::Clear, uint8_t{0},
			SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, static_cast<uint32_t>(site->index.base()), site->generation);
		PaySurfaceGateway<Commands::MoveLaunchFreight>(TileIndex{0}, center->index, center->launch_center->generation,
			rocket->index, rocket->generation, GetCargoTypeByLabel(CargoLabel{"GOOD"}), uint16_t{24}, false);
		QuerySurfaceGatewayResupplyFlight();
	} else if (phase == "resupply-flight-query") QuerySurfaceGatewayResupplyFlight();
	else if (phase == "resupply-flight") {
		QuerySurfaceGatewayResupplyFlight(); const auto *rocket = SurfaceGatewayPaidHull(14, true), *constructor = SurfaceGatewayPaidHull(20, true);
		PaySurfaceGateway<Commands::BeginLaunchFlight>(TileIndex{0}, rocket->index, rocket->generation,
			SpaceOrderTargetKind::Craft, static_cast<uint32_t>(constructor->index.base()), constructor->generation);
		SaveSurfaceGateway(saves / "surface-gateway-resupply-flight-preparing-440.sav"); ObserveSurfaceGatewayResupply();
		const std::array<std::pair<SpaceFlightPhase, std::string_view>, 4> stages{{
			{SpaceFlightPhase::Ascent, "ascent"}, {SpaceFlightPhase::OrbitalDelivery, "orbit"},
			{SpaceFlightPhase::ReturnRecovery, "delivered"}, {SpaceFlightPhase::GroundLoading, "recovered"}}};
		// Each real native phase is separately saved and observed in this one
		// invocation. Native ticking alone earns burns, handover and recovery;
		// no phase timer, position, packet or work receipt is assigned here.
		for (const auto &[target, name] : stages) {
			/** @return Whether the original rocket reached this actual next native phase. */
			const auto reached = [target] { return SurfaceGatewayPaidHull(14, true)->transition->phase == target; };
			EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, reached); REQUIRE(reached());
			SaveSurfaceGateway(saves / fmt::format("surface-gateway-resupply-flight-{}-440.sav", name)); ObserveSurfaceGatewayResupply();
		}
		REQUIRE(SurfaceGatewayResupplyComplete()); return;
	} else {
		REQUIRE(phase == "resupply-service"); REQUIRE(SurfaceGatewayResupplyComplete());
		/** @return False to earn actual ordinary service while the empty constructor queue prevents premature Core delivery/work. */
		const auto service = [] { return false; };
		EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, service); REQUIRE(SurfaceGatewayResupplyComplete());
	}
	ObserveSurfaceGatewayResupply();
}

/** Query the actual delivered constructor's Core order and complete remaining native work/reserve bill. */
static void QuerySurfaceGatewayConstruction()
{
	const auto *site = SurfaceGatewayOriginalCore(true); REQUIRE(site != nullptr); REQUIRE(site->job.work_ticks == 0);
	REQUIRE(site->job.inputs[0].TotalCount() == 16); REQUIRE(site->job.inputs[1].TotalCount() == 0); REQUIRE(site->module == nullptr);
	const auto *constructor = SurfaceGatewayPaidHull(20, true); REQUIRE(constructor->space_orders.entries.empty());
	REQUIRE(constructor->cargo_type == GetCargoTypeByLabel(CargoLabel{"GOOD"})); REQUIRE(constructor->cargo.TotalCount() == 24);
	const auto quote = QuoteSurfaceGateway<Commands::EditSpaceOrder>(TileIndex{0}, constructor->index, constructor->generation, SpaceOrderEdit::Insert, uint8_t{0},
		SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, static_cast<uint32_t>(site->index.base()), site->generation);
	REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == Money{0});
	const auto *company = Company::Get(_current_company); const auto *definition = GetSpaceConstructionDefinition(SpaceBuildKind::Core);
	const Money work{static_cast<int64_t>((uint64_t{definition->work_ticks} * GetMultiLayerSpaceBudget().active_per_day + 73) / 74)}, reserve{150000};
	const auto planned = work + reserve; const Money target{((planned.base() + 9999) / 10000) * 10000}; REQUIRE(company->money >= target);
	fmt::print("SURFACE_GATEWAY_CONSTRUCTION_PLAN site={} generation={} constructor={} constructor_generation={} work={} reserve={} planned={} target={} cash={} debt={} credit=0 ticks={} activation={} order_admitted=true\n",
		site->index.base(), site->generation, constructor->index.base(), constructor->generation, work.base(), reserve.base(), planned.base(), target.base(),
		company->money.base(), company->current_loan.base(), definition->work_ticks, GetMultiLayerSpaceBudget().core_activation_ticks);
}

/** Report the actual original job/frame/module/panel receipts without supplying material, progress or utility state. */
static void ObserveSurfaceGatewayConstruction()
{
	const auto *site = SurfaceGatewayOriginalCore(true); const auto *constructor = SurfaceGatewayPaidHull(20, true);
	fmt::print("SURFACE_GATEWAY_CONSTRUCTION_STATE site={} generation={} phase={} work={} steel={} goods={} consumed_steel={} consumed_goods={} held={} module={} activation={} panel={} operational={}\n",
		site->index.base(), site->generation, to_underlying(site->state.phase), site->job.work_ticks, site->job.inputs[0].TotalCount(), site->job.inputs[1].TotalCount(),
		site->job.consumed[0], site->job.consumed[1], constructor->cargo.TotalCount(), site->module != nullptr,
		site->module ? site->module->activation_ticks : 0, site->module ? site->module->producer.base() : UINT32_MAX, IsOrbitalModuleOperational(*site));
}

/** @param phase Explicit original constructor admission, real build/activation or useful-service continuation.
 * @param saves Invocation-owned directory retaining each actual native construction/activation checkpoint. */
static void ConstructSurfaceGatewayCore(const std::string &phase, const std::filesystem::path &saves)
{
	if (phase == "construction-query") { QuerySurfaceGatewayConstruction(); return; }
	if (phase == "construction-build") {
		QuerySurfaceGatewayConstruction(); const auto *site = SurfaceGatewayOriginalCore(true); const auto *constructor = SurfaceGatewayPaidHull(20, true);
		PaySurfaceGateway<Commands::EditSpaceOrder>(TileIndex{0}, constructor->index, constructor->generation, SpaceOrderEdit::Insert, uint8_t{0},
			SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, static_cast<uint32_t>(site->index.base()), site->generation);
		SaveSurfaceGateway(saves / "surface-gateway-construction-ordered-440.sav"); ObserveSurfaceGatewayConstruction();
		/** @return Whether native delivery has reserved the actual whole Goods input, alongside the job's existing16Steel. */
		const auto delivered = [] { return SurfaceGatewayOriginalCore(true)->job.inputs[1].TotalCount() == 24; };
		EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, delivered); REQUIRE(delivered()); REQUIRE(site->job.work_ticks == 0);
		SaveSurfaceGateway(saves / "surface-gateway-construction-reserved-440.sav"); ObserveSurfaceGatewayConstruction();
		/** @return Whether this native portable builder earned half the canonical work, retaining all real reserved inputs. */
		const auto half = [] { return SurfaceGatewayOriginalCore(true)->job.work_ticks >= 370; };
		EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, half); REQUIRE(half()); REQUIRE(site->job.work_ticks == 370);
		SaveSurfaceGateway(saves / "surface-gateway-construction-half-440.sav"); ObserveSurfaceGatewayConstruction();
		/** @return Whether the actual740tick completion consumed both physical inputs and published the paid frame. */
		const auto frame = [] { return SurfaceGatewayOriginalCore(true)->state.phase == SpaceStructurePhase::FrameComplete; };
		EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, frame); REQUIRE(frame()); REQUIRE(site->job.work_ticks == 740);
		SaveSurfaceGateway(saves / "surface-gateway-construction-frame-440.sav"); ObserveSurfaceGatewayConstruction();
		const auto activation = QuoteSurfaceGateway<Commands::ActivateOrbitalCore>(TileIndex{0}, site->index, site->generation);
		REQUIRE(activation.Succeeded()); REQUIRE(activation.GetCost() == Money{0});
		PaySurfaceGateway<Commands::ActivateOrbitalCore>(TileIndex{0}, site->index, site->generation);
		REQUIRE(site->module); REQUIRE(site->module->activation_ticks == GetMultiLayerSpaceBudget().core_activation_ticks);
		SaveSurfaceGateway(saves / "surface-gateway-construction-activating-440.sav"); ObserveSurfaceGatewayConstruction();
		/** @return Whether the real native countdown completed and the actual paid Core now exposes operational functionality. */
		const auto operational = [] { return IsOrbitalModuleOperational(*SurfaceGatewayOriginalCore(true)); };
		EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, operational); REQUIRE(operational());
		SaveSurfaceGateway(saves / "surface-gateway-construction-operational-440.sav"); ObserveSurfaceGatewayConstruction();
		return;
	}
	REQUIRE(phase == "construction-service"); REQUIRE(IsOrbitalModuleOperational(*SurfaceGatewayOriginalCore(true)));
	/** @return False to earn the full ordinary useful-service/panel interval without creating downstream modules or new stock. */
	const auto service = [] { return false; };
	EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, service); REQUIRE(IsOrbitalModuleOperational(*SurfaceGatewayOriginalCore(true))); ObserveSurfaceGatewayConstruction();
}

/** Query actual adjacent Dock/Assembly sites, original stock and the explicitly incomplete remaining expansion budget. */
static void QuerySurfaceGatewayExpansion()
{
	const auto *core = SurfaceGatewayOriginalCore(true); REQUIRE(core != nullptr); REQUIRE(IsOrbitalModuleOperational(*core));
	const auto *center = SurfaceGatewayAssemblyCenter(true); const auto *company = Company::Get(_current_company);
	const auto *rocket = SurfaceGatewayPaidHull(14, true); REQUIRE(rocket->cargo.TotalCount() == 0); REQUIRE(rocket->manifest->supplies[0].TotalCount() == 0);
	const auto *dock = GetSpaceConstructionDefinition(SpaceBuildKind::Dock), *assembly = GetSpaceConstructionDefinition(SpaceBuildKind::Assembly);
	const TileIndex dock_tile{11277}, assembly_tile{11467};
	const auto dock_quote = QuoteSurfaceGateway<Commands::BuildSpaceBlueprint>(dock_tile, dock_tile, SpaceBuildKind::Dock);
	const auto assembly_quote = QuoteSurfaceGateway<Commands::BuildSpaceBlueprint>(assembly_tile, assembly_tile, SpaceBuildKind::Assembly);
	REQUIRE(dock_quote.Succeeded()); REQUIRE(assembly_quote.Succeeded());
	REQUIRE(dock_quote.GetCost() == Money{dock->capital}); REQUIRE(assembly_quote.GetCost() == Money{assembly->capital});
	// These are inert proposed rectangles, never native pool objects or paid
	// facility identities. Both independent quotes must use the actual Core,
	// and the complete arrangement must also keep its future footprints apart.
	SpaceStructureState proposed_dock; proposed_dock.owner = _current_company; proposed_dock.kind = SpaceBuildKind::Dock;
	proposed_dock.x = TileX(dock_tile); proposed_dock.y = TilePlaneY(dock_tile);
	SpaceStructureState proposed_assembly; proposed_assembly.owner = _current_company; proposed_assembly.kind = SpaceBuildKind::Assembly;
	proposed_assembly.x = TileX(assembly_tile); proposed_assembly.y = TilePlaneY(assembly_tile);
	REQUIRE(GetOrbitalConstructionCore(proposed_dock) == core); REQUIRE(GetOrbitalConstructionCore(proposed_assembly) == core);
	REQUIRE(!SpaceStructureSitesOverlap(proposed_dock, proposed_assembly));
	fmt::print("SURFACE_GATEWAY_EXPANSION_LAYOUT core={} generation={} dock_tile={} dock_x={} dock_y={} dock_cost={} assembly_tile={} assembly_x={} assembly_y={} assembly_cost={} adjacent=true distinct=true admitted=true\n",
		core->index.base(), core->generation, dock_tile.base(), proposed_dock.x, proposed_dock.y, dock_quote.GetCost().base(),
		assembly_tile.base(), proposed_assembly.x, proposed_assembly.y, assembly_quote.GetCost().base());
	std::array<uint, 2> needed{};
	// Aggregate both actual published recipes before quoting stock. Counting
	// only the first module would falsely admit an under-supplied Assembly.
	for (const auto *definition : {dock, assembly}) for (const auto &input : definition->inputs) {
		const uint index = input.label == CargoLabel{"STEL"} ? 0 : 1; REQUIRE((input.label == CargoLabel{"STEL"} || input.label == CargoLabel{"GOOD"}));
		needed[index] += input.quantity;
	}
	/** @param cargo Actual runtime cargo slot. @return Available original Surface station packets; missing native storage contributes zero. */
	const auto held = [center](CargoType cargo) { return center->goods[cargo].HasData() ? center->goods[cargo].GetData().cargo.AvailableCount() : 0U; };
	for (uint index = 0; index < 2; ++index) {
		const auto cargo = GetCargoTypeByLabel(index == 0 ? CargoLabel{"STEL"} : CargoLabel{"GOOD"}); const auto available = held(cargo);
		const auto quote = QuoteSurfaceGateway<Commands::MoveLaunchFreight>(TileIndex{0}, center->index, center->launch_center->generation,
			rocket->index, rocket->generation, cargo, static_cast<uint16_t>(needed[index]), false);
		const auto mass = uint64_t{needed[index]} * CargoSpec::Get(cargo)->weight; REQUIRE(mass <= GetMultiLayerSpaceBudget().rocket_payload_sixteenths);
		REQUIRE(needed[index] <= GetSpaceFreightCapacity(14, cargo));
		fmt::print("SURFACE_GATEWAY_EXPANSION_INPUT index={} needed={} held={} missing={} mass={} capacity={} load_admitted={}\n", index, needed[index], available,
			needed[index] - std::min(needed[index], available), mass, GetSpaceFreightCapacity(14, cargo), quote.Succeeded());
	}
	const auto &budget = GetMultiLayerSpaceBudget(); const uint fuel = 2 * (uint{budget.outbound_fuel} + budget.recovery_fuel), available_fuel = held(GetSpaceSupplyCargo(SpaceSupply::Propellant));
	const auto fuel_quote = QuoteSurfaceGateway<Commands::MoveSpaceSupply>(TileIndex{0}, center->index, center->launch_center->generation,
		rocket->index, rocket->generation, to_underlying(SpaceSupply::Propellant), static_cast<uint16_t>(fuel), false);
	fmt::print("SURFACE_GATEWAY_EXPANSION_FUEL needed={} held={} missing={} fuel_load_admitted={}\n", fuel, available_fuel, fuel - std::min(fuel, available_fuel), fuel_quote.Succeeded());
	const Money modules{dock->capital + assembly->capital}, missions{2 * budget.mission_fee};
	const Money pad{static_cast<int64_t>(2 * (uint64_t{budget.prepare_ticks} + budget.turnaround_ticks) * budget.pad_per_day / 74)};
	const Money work{static_cast<int64_t>(((uint64_t{dock->work_ticks} + assembly->work_ticks) * budget.active_per_day + 73) / 74)}, reserve{150000};
	const auto base = modules + missions + pad + work + reserve, spendable = company->money + company->GetMaxLoan() - company->current_loan;
	const Money credit{((std::max(Money{0}, base - company->money).base() + 9999) / 10000) * 10000};
	const auto bank = QuoteSurfaceGateway<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit);
	// Whole current capital and native rounded bank admission must both pass;
	// projected earnings and unpriced supply logistics cannot fund a purchase.
	const bool funded = spendable >= base && bank.Succeeded();
	fmt::print("SURFACE_GATEWAY_EXPANSION_BUDGET modules={} missions={} pad={} work={} reserve={} base={} cash={} debt={} ceiling={} spendable={} credit={} bank_admitted={} funded={} logistics_costed=false\n",
		modules.base(), missions.base(), pad.base(), work.base(), reserve.base(), base.base(), company->money.base(), company->current_loan.base(),
		company->GetMaxLoan().base(), spendable.base(), credit.base(), bank.Succeeded(), funded);
	const auto freight = GetOrbitalModuleFreightBlock(*rocket, GetSpaceStructureReference(*core), false);
	const auto flight = QuoteSurfaceGateway<Commands::BeginLaunchFlight>(TileIndex{0}, rocket->index, rocket->generation,
		SpaceOrderTargetKind::Structure, static_cast<uint32_t>(core->index.base()), core->generation);
	REQUIRE(freight == SpaceFlightBlock::ReceiverFull); REQUIRE(flight.Failed());
	fmt::print("SURFACE_GATEWAY_EXPANSION_FLIGHT target={} generation={} empty_freight_block={} flight_admitted=false payload_missing=true fuel_missing=true\n",
		core->index.base(), core->generation, to_underlying(freight));
}

/** @param label Actual selected factory output. @return The existing paid pickup, excluding the busy Goods input bay. */
static const Station *SurfaceGatewayComponentPickup(CargoLabel label)
{
	if (label == CargoLabel{"STEL"}) return Station::Get(StationID{1});
	REQUIRE(label == CargoLabel{"GOOD"}); const Station *found = nullptr;
	// The separate paid bay keeps full-load reserve trucks out of the two
	// loading positions used by useful Steel deliveries into Goods production.
	for (const auto *station : Station::Iterate()) {
		if (station->name != "Surface Launch Goods pickup") continue;
		REQUIRE(found == nullptr); found = station;
	}
	REQUIRE(found != nullptr); return found;
}

/** @param role Existing actual component carrier. @return Native full-load/no-unload order for its genuine factory. */
static Order SurfaceGatewayComponentOrder(const SurfaceGatewayCarrier &role)
{
	Order order; order.MakeGoToStation(SurfaceGatewayComponentPickup(role.label)->index);
	order.SetLoadType(OrderLoadType::FullLoad); order.SetUnloadType(OrderUnloadType::NoUnload); order.SetStopLocation(OrderStopLocation::FarEnd); return order;
}

/** @param station Existing paid pickup. @param source Original producing industry. @return Crossings on one actual reciprocal Surface road path. */
static uint QuerySurfaceGatewayComponentRoad(const Station &station, uint source)
{
	REQUIRE(station.owner == _current_company); REQUIRE(source <= UINT16_MAX);
	const auto *factory = Industry::Get(IndustryID{static_cast<uint16_t>(source)}); bool covers = false;
	// Actual catchment must reach the literal producing factory. Another
	// industry's output or a declared pickup name cannot replace that owner.
	for (const auto tile : factory->location) if (station.TileIsInCatchment(tile)) covers = true;
	REQUIRE(covers); std::map<TileIndex, TileIndex> parent; std::queue<TileIndex> pending;
	parent.emplace(surface_gateway_depot, surface_gateway_depot); pending.push(surface_gateway_depot);
	// Traverse only existing reciprocal Surface road edges; this read-only
	// admission cannot build a shortcut or override the native pathfinder.
	while (!pending.empty()) {
		const auto tile = pending.front(); pending.pop(); if (tile == station.xy) break;
		const auto bits = GetAnyRoadBits(tile, RoadTramType::Road);
		for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			if ((bits & DiagDirToRoadBits(direction)) == RoadBits{}) continue;
			const auto next = AddTileIndexDiffCWrap(tile, TileIndexDiffCByDiagDir(direction));
			// Canonical layer membership and first discovery prevent crossing
			// packed planes or revisiting a road while checking real connectivity.
			if (!MultiLayerMap::IsTileInLayer(next, MapLayer::Surface) || parent.contains(next)) continue;
			if ((GetAnyRoadBits(next, RoadTramType::Road) & DiagDirToRoadBits(ReverseDiagDir(direction))) == RoadBits{}) continue;
			parent.emplace(next, tile); pending.push(next);
		}
	}
	REQUIRE(parent.contains(station.xy)); uint edges = 0, crossings = 0;
	// Count physical mainline crossings before any traffic is dispatched;
	// a nonzero result requires its own real protection prerequisite.
	for (auto tile = station.xy; tile != surface_gateway_depot; tile = parent.at(tile)) { ++edges; if (IsLevelCrossingTile(tile)) ++crossings; }
	fmt::print("SURFACE_GATEWAY_COMPONENT_ROUTE station={} tile={} source={} edges={} crossings={} catchment=true infrastructure=0\n", station.index.base(), station.xy.base(), source, edges, crossings);
	return crossings;
}

/** @return Three original renewed income fronts paired with their actual paid depots. */
static std::vector<std::pair<const Train *, TileIndex>> SurfaceGatewayComponentTraffic()
{
	const std::map<std::string, TileIndex> depots{{"Surface Income farm", TileIndex{1638}}, {"Surface Income goods", TileIndex{3224}}, {"Surface Income coal", TileIndex{3161}}};
	std::vector<std::pair<const Train *, TileIndex>> result;
	// Discovery follows actual renewed fronts rather than recycled pool IDs;
	// every original commercial consist must survive temporary protection.
	for (const auto *train : Train::Iterate()) {
		if (!train->IsFrontEngine()) continue;
		if (!train->name.starts_with("Surface Income ")) continue;
		REQUIRE(train->owner == _current_company); result.emplace_back(train, depots.at(train->name));
	}
	REQUIRE(result.size() == 3); return result;
}

/** @return True only after all three original income consists have delivered and halted empty in their own depots. */
static bool SurfaceGatewayComponentTrafficProtected()
{
	// Genuine depot arrival of every body clears mainline traffic; appended
	// orders or nearby stationary trains alone cannot protect the road crossing.
	for (const auto &[train, depot] : SurfaceGatewayComponentTraffic()) {
		// Both actual halt and original depot position are required to clear
		// all crossing traffic without altering any native vehicle pose.
		if (!train->IsStoppedInDepot() || train->tile != depot) return false;
		for (const auto *part = train; part != nullptr; part = part->Next()) if (part->cargo.TotalCount() != 0) return false;
	}
	return true;
}

/** @param execute False purely quotes matching native credit; true funds only the next bounded work/reserve window.
 * @return Actual quoted whole bank-step credit, with no projected income or increased ceiling.
 */
static Money FundSurfaceGatewayComponentWork(bool execute)
{
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	const Money target{158061}; const Money credit{((std::max(Money{0}, target - cash).base() + 9999) / 10000) * 10000};
	REQUIRE(cash + company->GetMaxLoan() - debt >= target);
	if (credit > 0) {
		REQUIRE(QuoteSurfaceGateway<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit).Succeeded());
		if (execute) PaySurfaceGateway<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit);
	}
	fmt::print("SURFACE_GATEWAY_COMPONENT_FUND target={} credit={} executed={} cash_before={} debt_before={} cash={} debt={} ceiling={}\n", target.base(), credit.base(), execute, cash.base(), debt.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base());
	return credit;
}

/** @param execute False purely quotes all five actual order changes; true dispatches only the admitted empty originals. */
static void DispatchSurfaceGatewayComponents(bool execute)
{
	REQUIRE(IsOrbitalModuleOperational(*SurfaceGatewayOriginalCore(true))); SurfaceGatewayAssemblyCenter(true);
	REQUIRE(SurfaceGatewayPaidHull(14, true)->transition->missions_completed == 2);
	uint crossings = 0;
	for (const auto &[label, source] : std::array<std::pair<CargoLabel, uint>, 2>{{{CargoLabel{"STEL"}, 10}, {CargoLabel{"GOOD"}, 11}}}) crossings += QuerySurfaceGatewayComponentRoad(*SurfaceGatewayComponentPickup(label), source);
	// The complete current query must also admit the crossing prerequisite.
	// Dispatch instead requires its genuinely arrived three-order consists.
	for (const auto &[train, tile] : SurfaceGatewayComponentTraffic()) {
		REQUIRE((train->GetNumOrders() == 2 || train->GetNumOrders() == 3)); const auto *depot = Depot::GetByTile(tile); REQUIRE(depot != nullptr);
		if (train->GetNumOrders() == 2) {
			REQUIRE(!execute);
			Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
			const auto quote = QuoteSurfaceGateway<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{2}, home);
			REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == 0);
		} else REQUIRE(SurfaceGatewayComponentTrafficProtected());
		fmt::print("SURFACE_GATEWAY_COMPONENT_TRAFFIC vehicle={} depot={} orders={} protected={} recall_cost=0\n", train->index.base(), tile.base(), train->GetNumOrders(), SurfaceGatewayComponentTrafficProtected());
	}
	const auto *company = Company::Get(_current_company); const Money work{8000}, handling{61}, reserve{150000};
	FundSurfaceGatewayComponentWork(false);
	fmt::print("SURFACE_GATEWAY_COMPONENT_BUDGET carriers=5 steel=36 goods=50 orders=0 infrastructure=0 handling={} work={} reserve={} target={} cash={} debt={} funded=true crossings={}\n", handling.base(), work.base(), reserve.base(), (work + handling + reserve).base(), company->money.base(), company->current_loan.base(), crossings);
	// Preflight the whole five-carrier arrangement before the first mutation;
	// depot halts and paid bodies survive unchanged, and fuel carriers stay put.
	for (uint index = 0; index < 5; ++index) {
		const auto &role = surface_gateway_carriers[index]; const auto *truck = SurfaceGatewayTruck(role);
		REQUIRE(truck->IsStoppedInDepot()); REQUIRE(truck->tile == surface_gateway_depot); REQUIRE(truck->cargo.TotalCount() == 0);
		REQUIRE(truck->cargo_cap == role.quantity); REQUIRE(truck->GetNumOrders() == 2); REQUIRE(truck->GetOrder(1)->IsType(OT_GOTO_DEPOT));
		REQUIRE(truck->GetOrder(1)->GetDestination() == DestinationID{Depot::GetByTile(surface_gateway_depot)->index.base()});
		REQUIRE(QuoteSurfaceGateway<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}).Succeeded());
		REQUIRE(QuoteSurfaceGateway<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, SurfaceGatewayComponentOrder(role)).Succeeded());
		REQUIRE(QuoteSurfaceGateway<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true).Succeeded());
		fmt::print("SURFACE_GATEWAY_COMPONENT_CARRIER role={} vehicle={} source={} station={} capacity={} held=0 admitted=true\n", role.role, truck->index.base(), role.source, SurfaceGatewayComponentPickup(role.label)->index.base(), role.quantity);
	}
	if (!execute) return;
	REQUIRE(SurfaceGatewayComponentTrafficProtected());
	FundSurfaceGatewayComponentWork(true);
	// Only the fully protected road admits these native restarts. Original
	// source/depot orders change, while every vehicle and packet owner survives.
	for (uint index = 0; index < 5; ++index) {
		const auto &role = surface_gateway_carriers[index]; const auto *truck = SurfaceGatewayTruck(role);
		PaySurfaceGateway<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
		PaySurfaceGateway<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, SurfaceGatewayComponentOrder(role));
		if (truck->cur_implicit_order_index != VehicleOrderID{0}) PaySurfaceGateway<Commands::SkipToOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
		PaySurfaceGateway<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true);
	}
}

/** @param count First original component roles to verify; two clears Steel crossing traffic, five completes collection.
 * @return True only when each selected original carrier is genuinely full and halted at its real depot.
 */
static bool SurfaceGatewayComponentsCollected(uint count = 5)
{
	// Completion requires physical whole native capacities and actual arrival;
	// a historical output counter or a truck still loading cannot close custody.
	for (uint index = 0; index < count; ++index) {
		const auto &role = surface_gateway_carriers[index]; const auto *truck = SurfaceGatewayTruck(role);
		if (truck->cargo.TotalCount() != role.quantity) return false;
		if (!truck->IsStoppedInDepot()) return false;
		if (truck->tile != surface_gateway_depot) return false;
	}
	return true;
}

/** @param phase Explicit pure admission, native dispatch or bounded real collection/service. */
static void CollectSurfaceGatewayComponents(std::string_view phase)
{
	if (phase == "components-query") { DispatchSurfaceGatewayComponents(false); return; }
	if (phase == "components-dispatch-query") { REQUIRE(SurfaceGatewayComponentTrafficProtected()); DispatchSurfaceGatewayComponents(false); return; }
	if (phase == "components-recall") {
		DispatchSurfaceGatewayComponents(false);
		// Append one temporary native depot halt after each unchanged useful
		// receiver. Existing cargo is delivered before actual depot protection.
		for (const auto &[train, tile] : SurfaceGatewayComponentTraffic()) {
			Order home; home.MakeGoToDepot(Depot::GetByTile(tile)->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
			PaySurfaceGateway<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{2}, home);
			if (train->cur_real_order_index != 1) PaySurfaceGateway<Commands::SkipToOrder>(TileIndex{0}, train->index, VehicleOrderID{1});
		}
		return;
	}
	if (phase == "components-traffic-work") {
		EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, SurfaceGatewayComponentTrafficProtected);
		fmt::print("SURFACE_GATEWAY_COMPONENT_PROTECTION complete={}\n", SurfaceGatewayComponentTrafficProtected()); return;
	}
	if (phase == "components-dispatch") { DispatchSurfaceGatewayComponents(true); return; }
	if (phase == "components-restore") {
		REQUIRE(SurfaceGatewayComponentsCollected(2)); REQUIRE(SurfaceGatewayComponentTrafficProtected());
		// Both full Steel carriers have cleared the only crossing. Remaining
		// Goods use the five-edge crossing-free bay road, so remove temporary
		// halt2 and resume the preserved commercial orders immediately.
		for (const auto &[train, tile] : SurfaceGatewayComponentTraffic()) {
			REQUIRE(train->GetNumOrders() == 3); PaySurfaceGateway<Commands::DeleteOrder>(TileIndex{0}, train->index, VehicleOrderID{2});
			REQUIRE(train->GetNumOrders() == 2); PaySurfaceGateway<Commands::StartStopVehicle>(TileIndex{0}, train->index, true);
			fmt::print("SURFACE_GATEWAY_COMPONENT_RESTORED vehicle={} orders=2 resumed=true\n", train->index.base());
		}
		return;
	}
	if (phase == "components-service") {
		REQUIRE(SurfaceGatewayComponentsCollected()); const auto initial = TimerGameTick::counter;
		FundSurfaceGatewayComponentWork(true);
		/** @return True after exactly3000ordinary ticks without moving the halted actual stock. */
		const auto service = [initial] { return TimerGameTick::counter - initial >= 3000; };
		EarnSurfaceEntryWork("SURFACE_GATEWAY", 3000, service); REQUIRE(SurfaceGatewayComponentsCollected()); return;
	}
	REQUIRE(phase == "components-work"); REQUIRE(!SurfaceGatewayComponentsCollected());
	if (!SurfaceGatewayComponentsCollected(2)) REQUIRE(SurfaceGatewayComponentTrafficProtected());
	FundSurfaceGatewayComponentWork(true);
	uint limit = 3000;
	if (const auto *selected = std::getenv("OTTD_SURFACE_COMPONENT_TICKS"); selected != nullptr) {
		const std::string_view value{selected}; const auto parsed = std::from_chars(value.data(), value.data() + value.size(), limit);
		REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == value.data() + value.size());
	}
	REQUIRE(limit > 0); REQUIRE(limit <= 3000);
	/** @return True on actual collection or the reserve boundary; a finance pause retains genuine partial progress. */
	const auto bounded = [] { return SurfaceGatewayComponentsCollected() || Company::Get(_current_company)->money < 154061; };
	EarnSurfaceEntryWork("SURFACE_GATEWAY", limit, bounded);
	fmt::print("SURFACE_GATEWAY_COMPONENT_PROGRESS complete={} supplied_cargo=false\n", SurfaceGatewayComponentsCollected());
}

/** Complete original current-world query or explicitly selected paid native gateway/mission/Core boundary. */
TEST_CASE("Authentic whole launch entry has current geometry stock and explicit funding bounds", "[.surface-entry-gateway]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_GATEWAY_PHASE"); const std::string phase = selected == nullptr ? "query" : selected;
	// Only named pure/current paid boundaries may execute; the absent selector
	// keeps the original whole-arrangement query entirely read-only.
	REQUIRE((phase == "query" || phase == "center-query" || phase == "center" || phase == "shipment-query" || phase == "dispatch" || phase == "ship-work" || phase == "handling-query" || phase == "handling" || phase == "handle-work" || phase == "service" || phase == "assembly-query" || phase == "rocket" || phase == "constructor" || phase == "rocket-work" || phase == "constructor-work" || phase == "assembly-service" || phase == "payload-query" || phase == "payload" || phase == "flight-query" || phase == "payload-service" || phase == "mission-query" || phase == "mission-begin" || phase == "mission-ascent" || phase == "mission-orbit" || phase == "mission-deploy" || phase == "mission-recover" || phase == "mission-service" || phase == "core-query" || phase == "core-buy" || phase == "core-order-query" || phase == "core-order" || phase == "core-steel-work" || phase == "core-service" || phase == "resupply-query" || phase == "resupply-load" || phase == "resupply-flight-query" || phase == "resupply-flight" || phase == "resupply-service" || phase == "construction-query" || phase == "construction-build" || phase == "construction-service" || phase == "expansion-query" || phase == "components-query" || phase == "components-dispatch" || phase == "components-work" || phase == "components-service" || phase == "components-recall" || phase == "components-traffic-work" || phase == "components-restore" || phase == "components-dispatch-query"));
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
			else if (phase.starts_with("core-")) BuildSurfaceGatewayCore(phase);
			else if (phase.starts_with("resupply-")) ResupplySurfaceGatewayCore(phase, paths.saves);
			else if (phase.starts_with("construction-")) ConstructSurfaceGatewayCore(phase, paths.saves);
			else if (phase == "expansion-query") QuerySurfaceGatewayExpansion();
			else if (phase.starts_with("components-")) CollectSurfaceGatewayComponents(phase);
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
