/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_launch.cpp Actual launch-entry quotes, paid civic/service work and native site preparation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BIE/15BIEB/15BIEC.
 * Public native commands alone quote and settle every purchase. No temporary hub,
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
#include "../roadveh.h"
#include "../order_cmd.h"
#include "../water_cmd.h"
#include "../landscape_cmd.h"
#include "../train.h"
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

/** Freshly quote and settle one complete original synchronized purchase.
 * @tparam Tcmd Native command identity. @tparam Targs Original argument types.
 * @param tile Real action tile, or zero for coordinate-free management.
 * @param args Entire native argument list. @return Original paid command result.
 */
template <Commands Tcmd, typename... Targs>
static auto PaySurfaceLaunch(TileIndex tile, Targs... args)
{
	auto quote = QuoteSurfaceLaunch<Tcmd>(tile, args...);
	INFO("Launch service command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(quote).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(quote).Succeeded());
	auto paid = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(args...));
	REQUIRE(ExtractCommandCost(paid).Succeeded()); REQUIRE(ExtractCommandCost(paid).GetCost() == ExtractCommandCost(quote).GetCost());
	fmt::print("SURFACE_PASSENGER_INVOICE command={} tile={} cost={}\n", GetCommandName(Tcmd), tile.base(), ExtractCommandCost(paid).GetCost().base());
	return paid;
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

/** Buy the entire freshly admitted three-bus service through original infrastructure, vehicle and order commands. */
static void BuySurfacePassengerService()
{
	SurfacePassengerLayout layout; layout.Prepare(); const auto passenger = GetCargoTypeByLabel(CargoLabel{"PASS"});
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	const auto bus_quote = QuoteSurfaceLaunch<Commands::BuildVehicle>(TileIndex{2443}, TileIndex{2443}, EngineID{119}, false, passenger, ClientID::Server);
	REQUIRE(std::get<0>(bus_quote).Succeeded()); REQUIRE(std::get<2>(bus_quote) == 40);
	const auto total = layout.invoice + std::get<0>(bus_quote).GetCost() * 3; REQUIRE(cash >= total + Money{20000});
	// Buy both stops while their current civic admission is intact. The depot
	// may later incur its original tree-clearing rating penalty; an earlier
	// quote cannot supply or override that actual native rating owner.
	for (uint index = 0; index < 2; ++index) {
		PaySurfaceLaunch<Commands::BuildRoadStop>(layout.stops[index], layout.stops[index], uint8_t{1}, uint8_t{1}, RoadStopType::Bus,
			true, layout.directions[index], ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, NEW_STATION, true);
	}
	if (GetRoadBits(layout.entrance, RoadTramType::Road) != layout.connections) {
		PaySurfaceLaunch<Commands::BuildRoad>(layout.entrance, layout.entrance, layout.connections, ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid());
	}
	PaySurfaceLaunch<Commands::BuildRoadDepot>(layout.depot, layout.depot, ROADTYPE_ROAD, layout.door);
	const auto first = GetStationIndex(layout.stops[0]), last = GetStationIndex(layout.stops[1]); REQUIRE(first != last);
	// Every empty bus receives two ordinary bidirectional passenger orders.
	// Native loading, route choice and delivery alone can create actual income.
	for (uint index = 0; index < 3; ++index) {
		const auto paid = PaySurfaceLaunch<Commands::BuildVehicle>(layout.depot, layout.depot, EngineID{119}, false, passenger, ClientID::Server);
		const auto *bus = RoadVehicle::Get(std::get<1>(paid)); REQUIRE(bus->IsStoppedInDepot()); REQUIRE(bus->cargo.TotalCount() == 0); REQUIRE(bus->cargo_cap == 40);
		PaySurfaceLaunch<Commands::RenameVehicle>(TileIndex{0}, bus->index, fmt::format("Surface Launch passenger {}", index + 1));
		for (uint stop = 0; stop < 2; ++stop) {
			Order order; order.MakeGoToStation(stop == 0 ? first : last); order.SetStopLocation(OrderStopLocation::FarEnd);
			PaySurfaceLaunch<Commands::InsertOrder>(TileIndex{0}, bus->index, VehicleOrderID{static_cast<uint8_t>(stop)}, order);
		}
		PaySurfaceLaunch<Commands::StartStopVehicle>(TileIndex{0}, bus->index, true);
		fmt::print("SURFACE_PASSENGER_BUS vehicle={} model=119 capacity=40 first={} last={} orders={} empty=true\n", bus->index.base(), first.base(), last.base(), bus->GetNumOrders());
	}
	REQUIRE(company->money == cash - total); REQUIRE(company->current_loan == debt);
	fmt::print("SURFACE_PASSENGER_PURCHASE cost={} cash={} debt={} buses=3 first={} last={} depot={}\n", total.base(), company->money.base(), debt.base(), first.base(), last.base(), layout.depot.base());
}

/** Measure one real annual work interval, including original bus profits across native year rollovers. */
static void EarnSurfacePassengerService()
{
	std::map<VehicleID, Money> previous; Money profit{0}; auto year = TimerGameEconomy::year;
	// Follow the actual commissioned service through native identities; other
	// road freight remains in the independent original company-wide accounts.
	for (const auto *vehicle : Vehicle::Iterate()) {
		// Existing freight may earn another bounded annual interval only
		// within its original engine life; a required renewal becomes a new
		// paid producer instead of letting expired unreliable stock crawl on.
		if (vehicle->type == VehicleType::Train && vehicle->name.starts_with("Surface Income ")) {
			REQUIRE(vehicle->age.base() + 366 < vehicle->max_age.base());
			fmt::print("SURFACE_SAVINGS_FLEET head={} engine={} age={} maximum={} annual_safe=true\n", vehicle->index.base(), vehicle->engine_type.base(), vehicle->age.base(), vehicle->max_age.base());
		}
		if (vehicle->type != VehicleType::Road) continue;
		if (!vehicle->name.starts_with("Surface Launch passenger ")) continue;
		REQUIRE(vehicle->owner == _current_company); REQUIRE(vehicle->GetNumOrders() == 2); previous.emplace(vehicle->index, vehicle->profit_this_year);
	}
	REQUIRE(previous.size() == 3);
	/** @return False while observing native bus accounts; the bounded actual tick limit owns completion. */
	const auto observe = [&] {
		const auto current_year = TimerGameEconomy::year;
		// Preserve each completed native fractional-money profit before the
		// two-year window discards it. Observation never credits company cash.
		for (auto &[id, last] : previous) {
			REQUIRE(Vehicle::IsValidID(id)); const auto *bus = Vehicle::Get(id); REQUIRE(bus->owner == _current_company);
			profit += current_year == year ? bus->profit_this_year - last : bus->profit_last_year - last + bus->profit_this_year;
			last = bus->profit_this_year;
		}
		year = current_year; return false;
	};
	EarnSurfaceEntryWork("SURFACE_PASSENGER", 27000, observe);
	fmt::print("SURFACE_PASSENGER_PROFIT buses={} fraction=256 amount={}\n", previous.size(), profit.base());
}

/** Complete staged native water proposal, never a constructed center or a saved routing owner. */
struct SurfaceLaunchGroundPlan {
	TileIndex anchor = INVALID_TILE; ///< Existing flat whole launch footprint.
	uint8_t rotation = 0; ///< Native quarter-turn footprint orientation.
	TileIndex canal = INVALID_TILE; ///< Actual orthogonal land tile proposed for a paid canal.
	StationID member = StationID::Invalid(); ///< Existing owned freight station whose actual full geometry fits the collection circle.
	Money preparation{0}; ///< Whole original canal command price, including clearing.
	Money center_bound{0}; ///< Definition plus actual clearing bound; full command still requires real water.
	uint distance = UINT_MAX; ///< Actual depot distance for stable selection.
};

/** @param anchor Actual proposed center anchor. @return Existing owned truck station with every physical tile in the public collection circle, or invalid.
 * This checks actual geometry only; no unbuilt center can grant membership or handling admission.
 */
static StationID FindSurfaceLaunchGroundMember(TileIndex anchor)
{
	const auto reference = GetLaunchCenterPlaneReference(anchor); REQUIRE(reference.has_value());
	// Every actual tile of an existing native freight station must qualify.
	// Catchment, sign position and a fabricated center cannot grant a link.
	for (const auto *station : Station::Iterate()) {
		if (station->owner != _current_company) continue;
		if (!station->facilities.Test(StationFacility::TruckStop)) continue;
		if (station->spread.IsEmpty()) continue;
		bool valid = true, freight = false;
		for (const auto tile : station->spread) {
			if (!IsTileType(tile, TileType::Station)) continue;
			if (GetStationIndex(tile) != station->index) continue;
			const auto point = GetLaunchCenterPlaneReference(tile);
			if (!point) { valid = false; break; }
			// Sole physical ownership and the public complete circle, rather
			// than a nearby sign, determine future useful handling geometry.
			if (GetTileOwner(tile) != station->owner || !IsInsideLaunchCenterCollectionArea(*point, *reference)) { valid = false; break; }
			freight |= IsTruckStop(tile);
		}
		if (!valid) continue;
		if (freight) return station->index;
	}
	return StationID::Invalid();
}

/** @return Entire affordable native canal/terrain prerequisite with useful actual member geometry, without installing hypothetical water. */
static SurfaceLaunchGroundPlan QuerySurfaceLaunchGround()
{
	SurfaceLaunchGroundPlan selected; const auto cash = Company::Get(_current_company)->money;
	// Look only at complete flat land footprints accepted by the original
	// per-tile terrain predicate. A missing-water command refusal proves none
	// of the later station/company predicates, which must be queried after work.
	for (const TileIndex anchor : MultiLayerMap::IterateLayer(MapLayer::Surface)) {
		for (uint8_t rotation = 0; rotation < 4; ++rotation) {
			const auto area = GetLaunchCenterFootprint(anchor, rotation); if (!area) continue;
			if (HasLaunchCenterWaterAccess(*area)) continue;
			bool flat = true; Money center{500000}; std::set<TileIndex> edges;
			for (const auto tile : *area) {
				if (GetLaunchCenterTerrainTileBlock(tile, GetTileZ(anchor)).Failed()) { flat = false; break; }
				const auto clearing = QuoteSurfaceLaunch<Commands::LandscapeClear>(tile, tile);
				if (clearing.Failed()) { flat = false; break; }
				center += clearing.GetCost();
				for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
					const auto edge = SurfaceChemicalLayout::Step(tile, direction); if (edge == INVALID_TILE) continue;
					if (!area->Contains(edge)) edges.insert(edge);
				}
			}
			if (!flat) continue;
			const auto member = FindSurfaceLaunchGroundMember(anchor); if (member == StationID::Invalid()) continue;
			// A genuine whole canal command must admit clearing and water on
			// existing adjacent land. No map tile is changed to quote the center.
			for (const auto edge : edges) {
				if (!IsTileType(edge, TileType::Clear) && !IsTileType(edge, TileType::Trees)) continue; // Houses and every existing infrastructure owner survive.
				const auto water = QuoteSurfaceLaunch<Commands::BuildCanal>(edge, edge, edge, WaterClass::Canal, false);
				if (water.Failed()) continue;
				if (water.GetCost() + Money{50000} > cash) continue;
				const uint distance = SurfacePassengerLayout::Distance(anchor, TileIndex{2443});
				const auto rank = std::make_tuple(distance, water.GetCost(), anchor, rotation, edge);
				const auto previous = std::make_tuple(selected.distance, selected.preparation, selected.anchor, selected.rotation, selected.canal);
				if (rank >= previous) continue;
				selected = {anchor, rotation, edge, member, water.GetCost(), center, distance};
			}
		}
	}
	REQUIRE(selected.anchor != INVALID_TILE);
	fmt::print("SURFACE_LAUNCH_GROUND anchor={} rotation={} canal={} member={} preparation={} center_bound={} depot_distance={} reserve=50000 cash={} center_admitted=false\n",
		selected.anchor.base(), selected.rotation, selected.canal.base(), selected.member.base(), selected.preparation.base(), selected.center_bound.base(), selected.distance, cash.base());
	return selected;
}

/** Pay only the completely admitted real canal, then quote the original full center on actual resulting geometry. */
static void PrepareSurfaceLaunchGround()
{
	const auto selected = QuerySurfaceLaunchGround(); const auto *company = Company::Get(_current_company);
	const auto cash = company->money, debt = company->current_loan;
	const auto paid = PaySurfaceLaunch<Commands::BuildCanal>(selected.canal, selected.canal, selected.canal, WaterClass::Canal, false);
	REQUIRE(paid.GetCost() == selected.preparation); REQUIRE(company->money == cash - selected.preparation); REQUIRE(company->current_loan == debt);
	REQUIRE(IsCanal(selected.canal)); REQUIRE(GetTileOwner(selected.canal) == _current_company);
	REQUIRE(GetLaunchCenterSiteGeometryBlock(selected.anchor, selected.rotation).Succeeded());
	const auto quote = QuoteSurfaceLaunch<Commands::BuildLaunchCenter>(selected.anchor, selected.anchor, selected.rotation, NEW_STATION, true);
	REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == selected.center_bound);
	REQUIRE(company->money < quote.GetCost()); // This paid prerequisite cannot claim the still-unfunded facility was purchased.
	fmt::print("SURFACE_LAUNCH_GROUND_PAID anchor={} rotation={} canal={} member={} cost={} center={} cash={} debt={} reserve=50000 center_built=false\n",
		selected.anchor.base(), selected.rotation, selected.canal.base(), selected.member.base(), paid.GetCost().base(), quote.GetCost().base(), company->money.base(), debt.base());
}

/** Query actual loan reserve, initial savings goal and complete currently available gross freight renewal without selling assets. */
static void QuerySurfaceLaunchSavings()
{
	const auto *company = Company::Get(_current_company); Money renewal{0}; uint heads = 0;
	EngineID replacement = EngineID::Invalid(); Money replacement_cost{INT64_MAX}; uint available = 0;
	// A retired model remains a valid working asset but is not a new purchase.
	// Inspect all original currently available locomotives in the real electric
	// depot; capability and price, rather than the former model ID, select a
	// planning candidate at least as capable as the earlier paid diesel service.
	for (const auto *engine : Engine::IterateType(VehicleType::Train)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Train, _current_company)) continue;
		const auto &rail = engine->VehInfo<RailVehicleInfo>(); if (rail.railveh_type == RailVehicleType::Wagon) continue;
		const auto quote = QuoteSurfaceLaunch<Commands::BuildVehicle>(TileIndex{1638}, TileIndex{1638}, engine->index, false, INVALID_CARGO, ClientID::Server);
		fmt::print("SURFACE_SAVINGS_AVAILABLE model={} cost={} power={} speed={} admitted={} error={}\n", engine->index.base(), std::get<0>(quote).GetCost().base(), rail.power,
			engine->GetDisplayMaxSpeed(), std::get<0>(quote).Succeeded(), std::get<0>(quote).GetErrorMessage().base());
		if (std::get<0>(quote).Failed()) continue;
		++available;
		if (rail.power < 2000 || engine->GetDisplayMaxSpeed() < 80) continue; // Preserve the proven conventional freight role before considering a cheaper head.
		if (std::get<0>(quote).GetCost() >= replacement_cost) continue;
		replacement = engine->index; replacement_cost = std::get<0>(quote).GetCost();
	}
	// Price every real service depot through original BuildVehicle admission.
	// Unsold head value is disclosed only as a book observation; it cannot
	// reduce this conservative gross purchase requirement or credit cash.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->type != VehicleType::Train) continue;
		if (!vehicle->name.starts_with("Surface Income ")) continue;
		REQUIRE(vehicle->owner == _current_company); REQUIRE(vehicle->engine_type == EngineID{26});
		const TileIndex depot = vehicle->name == "Surface Income farm" ? TileIndex{1638} : vehicle->name == "Surface Income goods" ? TileIndex{3224} : TileIndex{3161};
		Money cost{0};
		if (replacement != EngineID::Invalid()) {
			const auto quote = QuoteSurfaceLaunch<Commands::BuildVehicle>(depot, depot, replacement, false, INVALID_CARGO, ClientID::Server);
			REQUIRE(std::get<0>(quote).Succeeded()); cost = std::get<0>(quote).GetCost(); renewal += cost;
		}
		++heads;
		fmt::print("SURFACE_SAVINGS_MODEL head={} model={} depot={} cost={} age={} maximum={} value={} safe_periods={}\n", vehicle->index.base(), replacement.base(), depot.base(), cost.base(),
			vehicle->age.base(), vehicle->max_age.base(), vehicle->value.base(), (vehicle->max_age.base() - vehicle->age.base()) / 366);
	}
	REQUIRE(heads == 3); const auto spendable = company->money + company->GetMaxLoan() - company->current_loan;
	fmt::print("SURFACE_SAVINGS_PLAN cash={} debt={} ceiling={} spendable={} reserve=150000 milestone=325000 gross_renewal={} available_models={} renewal_selected={} reached={} complete_launch=false\n",
		company->money.base(), company->current_loan.base(), company->GetMaxLoan().base(), spendable.base(), renewal.base(), available, replacement != EngineID::Invalid(), spendable >= Money{325000});
}

/** Repay only actual cash above the selected reserve through the original matching bank command. */
static void RepaySurfaceLaunchSavings()
{
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan, ceiling = company->GetMaxLoan();
	const auto expenses = company->yearly_expenses; const auto quarter = company->cur_economy;
	REQUIRE(cash >= Money{160000}); const Money amount = std::min(debt, Money{((cash - Money{150000}).base() / 10000) * 10000}); REQUIRE(amount > 0);
	const auto spendable = cash + ceiling - debt;
	PaySurfaceLaunch<Commands::DecreaseLoan>(TileIndex{0}, LoanCommand::Amount, amount);
	REQUIRE(company->money == cash - amount); REQUIRE(company->current_loan == debt - amount); REQUIRE(company->GetMaxLoan() == ceiling);
	REQUIRE(company->money + ceiling - company->current_loan == spendable); REQUIRE(company->yearly_expenses == expenses);
	REQUIRE(company->cur_economy.income == quarter.income); REQUIRE(company->cur_economy.expenses == quarter.expenses);
	fmt::print("SURFACE_SAVINGS_REPAY amount={} cash_before={} debt_before={} cash={} debt={} ceiling={} spendable={} reserve=150000\n", amount.base(), cash.base(), debt.base(), company->money.base(), company->current_loan.base(), ceiling.base(), spendable.base());
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
	REQUIRE((phase == "query" || phase == "passenger-query" || phase == "civic-query" || phase == "civic" || phase == "finance" || phase == "buy" || phase == "earn" || phase == "ground-query" || phase == "ground" || phase == "savings-query" || phase == "repay")); // Explicit phases distinguish pure quotes, real work and original paid/bank boundaries.
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
			else if (phase == "buy") BuySurfacePassengerService();
			else if (phase == "earn") EarnSurfacePassengerService();
			else if (phase == "ground-query") QuerySurfaceLaunchGround();
			else if (phase == "ground") PrepareSurfaceLaunchGround();
			else if (phase == "savings-query") QuerySurfaceLaunchSavings();
			else if (phase == "repay") RepaySurfaceLaunchSavings();
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
