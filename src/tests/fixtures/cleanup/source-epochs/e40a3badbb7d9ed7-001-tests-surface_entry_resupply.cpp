/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_resupply.cpp Original finite-Salt and chemical-refill admission, transport and settlement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BIEG/15CH/15CIB/15CIC/15CID/15CIE/15CJ/15CK/15CL/15CM/15CN/15CO/15CP/15CQ/15CR/15CS/15CSB.
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
#include "../economy_func.h"
#include "../underground_vehicle.h"
#include "../vehicle_cmd.h"
#include "../roadveh.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../pathfinder/follow_track.hpp"
#include "../order_cmd.h"
#include "../depot_base.h"
#include "../industry.h"
#include "../multilayer_industry.h"
#include "../layer_industry_processor.h"
#include "../layer_portal_cmd.h"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../space_vehicle.h"
#include "../multilayer_space_content.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../rail_map.h"
#include "../rail_cmd.h"
#include "../road_map.h"
#include "../misc_cmd.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_entry_work.h"
#include <queue>
#include <charconv>
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

/** Inspect existing native rail edges without moving a train or granting a route.
 * @param depot Actual owned depot from which a head would depart normally.
 * @param target Actual owned depot or station to reach.
 * @param railtypes Real powered/compatible rail types of the proposed head.
 * @return Whether the native track follower reaches the destination under current geometry and ownership.
 */
static bool SurfaceResupplyRailConnected(TileIndex depot, TileIndex target, RailTypes railtypes)
{
	REQUIRE(IsRailDepotTile(depot)); REQUIRE(GetTileOwner(depot) == _current_company);
	REQUIRE(_settings_game.pf.forbid_90_deg);
	using Position = std::pair<TileIndex, Trackdir>;
	std::set<Position> visited; std::queue<Position> pending;
	const Position first{depot, DiagDirToDiagTrackdir(GetRailDepotDirection(depot))};
	visited.insert(first); pending.push(first);
	// Explore only directional edges accepted by the game's own follower,
	// including real portal pairs, bridges, station exits and depot entrances.
	// A nearby parallel line or an adjacent storage plane supplies no edge.
	while (!pending.empty()) {
		const auto [tile, direction] = pending.front(); pending.pop();
		if (tile == target) return true;
		CFollowTrackRailNo90 follower(_current_company, railtypes);
		if (!follower.Follow(tile, direction)) continue;
		// The native follower can skip a platform. Reaching any tile of the
		// requested original station admits that same physical destination.
		if (IsRailStationTile(target) && IsRailStationTile(follower.new_tile)) {
			if (GetStationIndex(target) == GetStationIndex(follower.new_tile)) return true;
		}
		// Remember each directional state once so loops in paid railway cannot
		// create unbounded planning work or masquerade as a new connection.
		for (const auto next : follower.new_td_bits) {
			const Position position{follower.new_tile, next};
			if (visited.insert(position).second) pending.push(position);
		}
	}
	return false;
}

/** Verify one existing Surface road trip without granting a vehicle or future cargo.
 * @param target Actual owned source, receiver or launch-member road stop.
 * @return Count of existing level crossings that must be cleared by the later real traffic-protection stage.
 */
static uint QuerySurfaceResupplyRoad(TileIndex target)
{
	std::map<TileIndex, TileIndex> parent; std::queue<TileIndex> pending;
	parent.emplace(surface_resupply_road_depot, surface_resupply_road_depot); pending.push(surface_resupply_road_depot);
	// Every supply leg needs a reciprocal real road from the paid depot;
	// station catchment and nearby coordinates cannot supply missing track.
	while (!pending.empty()) {
		const auto tile = pending.front(); pending.pop();
		if (tile == target) break;
		const auto bits = GetAnyRoadBits(tile, RoadTramType::Road);
		// Explore each legal neighboring road once, keeping all existing
		// stations, industries, rails and roads in their original ownership.
		for (const auto direction : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			if ((bits & DiagDirToRoadBits(direction)) == RoadBits{}) continue;
			const auto next = AddTileIndexDiffCWrap(tile, TileIndexDiffCByDiagDir(direction));
			// A new edge must remain on Surface and not repeat a visited road;
			// this bounds the query and forbids camera-plane shortcuts.
			if (!MultiLayerMap::IsTileInLayer(next, MapLayer::Surface) || parent.contains(next)) continue;
			if ((GetAnyRoadBits(next, RoadTramType::Road) & DiagDirToRoadBits(ReverseDiagDir(direction))) == RoadBits{}) continue;
			parent.emplace(next, tile); pending.push(next);
		}
	}
	REQUIRE(parent.contains(target)); uint edges = 0, crossings = 0;
	// Only the chosen existing physical path contributes crossings. Real
	// traffic must later be halted; this quote does not clear those crossings.
	for (auto tile = target; tile != surface_resupply_road_depot; tile = parent.at(tile)) {
		++edges;
		if (IsLevelCrossingTile(tile)) ++crossings;
	}
	fmt::print("SURFACE_RESUPPLY_ROUTE target={} edges={} crossings={} infrastructure=0\n", target.base(), edges, crossings);
	return crossings;
}

/** Quote the complete bounded next fuel window using an admitted alternative head.
 * @param head Actual currently buildable head identity at the original Salt depot.
 * @param head_cost Its whole native purchase quote, with no hypothetical resale credit.
 * @param infrastructure Whole native quotes for a required earlier route-conversion producer; zero for the current route.
 * @param quote_depot Existing compatible depot used for the head quote; a converted Salt depot must be requoted after actual conversion.
 * @return Nothing; explicit remaining permissions and the native bank refusal stay visible.
 */
static void QuerySurfaceResupplyWindow(EngineID head, Money head_cost, Money infrastructure = Money{0}, TileIndex quote_depot = surface_resupply_depot)
{
	const auto salt = GetCargoTypeByLabel(CargoLabel{"SALT"}), chem = GetCargoTypeByLabel(CargoLabel{"CHEM"}), prop = GetCargoTypeByLabel(CargoLabel{"PROP"});
	const auto wagon = GetUndergroundEngine(5);
	const auto hopper = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, wagon, false, salt, ClientID::Server);
	REQUIRE(std::get<0>(hopper).Succeeded()); REQUIRE(std::get<2>(hopper) == 60);
	EngineID replacement = EngineID::Invalid(); Money replacement_cost{0}; uint capacity = 0;
	// Genuine tanker renewal must precede chemical collection. Compare every
	// available whole CHEM purchase, excluding capacities below the actual18need.
	for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
		const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_road_depot, surface_resupply_road_depot, engine->index, false, chem, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		if (std::get<2>(quote) < 18) continue;
		// Retain an earlier candidate unless this whole quote is cheaper;
		// a larger advertised capacity alone cannot justify extra spending.
		if (replacement != EngineID::Invalid() && std::get<0>(quote).GetCost() >= replacement_cost) continue;
		replacement = engine->index; replacement_cost = std::get<0>(quote).GetCost(); capacity = std::get<2>(quote);
	}
	REQUIRE(replacement != EngineID::Invalid()); const uint fuel = 2 * (capacity / 2); REQUIRE(fuel >= 18);
	const auto *chemical = Industry::Get(IndustryID{15}), *processor = Industry::Get(IndustryID{6});
	REQUIRE(IsLayerIndustryComplete(*chemical)); REQUIRE(IsLayerIndustryComplete(*processor));
	REQUIRE(GetLayerProcessorState(chemical->index)->recipe == MultiLayerRecipeID{"CHEM"});
	REQUIRE(GetLayerProcessorState(processor->index)->recipe == MultiLayerRecipeID{"PCHM"});
	const auto oil = std::ranges::find(chemical->accepted, GetCargoTypeByLabel(CargoLabel{"OIL_"}), &Industry::AcceptedCargo::cargo);
	REQUIRE(oil != chemical->accepted.end()); REQUIRE(oil->waiting >= fuel);
	const auto buffered_salt = std::ranges::find(Industry::Get(IndustryID{14})->produced, salt, &Industry::ProducedCargo::cargo);
	REQUIRE(buffered_salt != Industry::Get(IndustryID{14})->produced.end()); REQUIRE(buffered_salt->waiting >= 60);
	uint crossings = 0;
	// Price all real road legs: Source15 collection, PCHM input, PCHM output
	// and the original hub member. Zero new infrastructure is an actual existing
	// route result, never a proposed shortcut or a stock-backed handling claim.
	for (const auto bay : {TileIndex{1825}, TileIndex{1895}, TileIndex{1896}, TileIndex{2504}}) {
		crossings += QuerySurfaceResupplyRoad(bay);
		const auto *station = Station::GetByTile(bay); REQUIRE(station != nullptr); REQUIRE(station->owner == _current_company);
		Order order; order.MakeGoToStation(station->index); order.SetStopLocation(OrderStopLocation::FarEnd);
		order.SetLoadType(OrderLoadType::NoLoad);
		const auto quote = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, VehicleID{9}, VehicleOrderID{0}, order);
		REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == 0);
	}
	const auto membership = GetLaunchHubMembership(StationID{0}); REQUIRE(membership.has_value());
	const auto *center = ResolveLaunchCenter(membership->center), *member = ResolveLaunchHubMember(membership->center, membership->member);
	REQUIRE(center != nullptr); REQUIRE(member != nullptr); REQUIRE(center->index == StationID{21});
	REQUIRE(GetLaunchStockHeadroom(*center, prop) >= fuel); REQUIRE(GetLaunchStockHeadroom(*member, prop) >= fuel);
	const auto *company = Company::Get(_current_company); const Money reserve{100000}, work{18000};
	const Money handling{static_cast<int64_t>((uint64_t{fuel} * CargoSpec::Get(prop)->weight + 15) / 16)};
	const auto target = head_cost + std::get<0>(hopper).GetCost() + replacement_cost + reserve + work + handling + infrastructure;
	const Money credit{((std::max(Money{0}, target - company->money).base() + 9999) / 10000) * 10000};
	const bool bank = credit == 0 || QuoteSurfaceResupply<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit).Succeeded();
	// Every paid purchase, the complete bounded work allowance and unchanged
	// working reserve must fit actual cash plus natively admitted credit. Future
	// income, old-asset resale and the later module reserve supply no money here.
	const bool funded = bank && company->money + credit >= target;
	fmt::print("SURFACE_RESUPPLY_WINDOW head={} head_cost={} wagon={} wagon_cost={} replacement={} replacement_cost={} chemical_capacity={} salt=60 oil_needed={} chemical_feed={} fuel={} handling={} work={} reserve={} target={} cash={} credit={} bank_admitted={} funded={} road_crossings={} infrastructure={} head_quote_depot={} conversion_pending={} unsold_value_counted=false coupling_pending=true handling_stock_missing=true bounded_bill_complete=true fuel_procured=false\n",
		head.base(), head_cost.base(), wagon.base(), std::get<0>(hopper).GetCost().base(), replacement.base(), replacement_cost.base(), capacity,
		fuel, fuel, fuel, handling.base(), work.base(), reserve.base(), target.base(), company->money.base(), credit.base(), bank, funded, crossings, infrastructure.base(), quote_depot.base(), infrastructure > 0);
}

/** Quote the next physically bounded Salt stage without spending a future resale.
 * @param head Actual electric head quoted at the compatible existing depot.
 * @param head_cost Whole native head quote, with no future refund deducted.
 * @param infrastructure Whole already quoted original Salt-route conversion.
 * @param buy_train Include the not-yet-owned train purchase/upkeep; false only after the real consist is paid and observed.
 * @param limit Actual separately admitted short interval, at most the original500ticks; later returns use200without replaying prior work.
 * @return Whether actual liquidity and the native calendar admit this exact first window; no future sale is counted.
 */
static bool QuerySurfaceResupplyPeak(EngineID head, Money head_cost, Money infrastructure, bool buy_train = true, uint32_t limit = 500)
{
	REQUIRE(limit > 0); REQUIRE(limit <= 500);
	const auto *company = Company::Get(_current_company);
	const auto hopper = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, GetUndergroundEngine(5), false, GetCargoTypeByLabel(CargoLabel{"SALT"}), ClientID::Server);
	REQUIRE(std::get<0>(hopper).Succeeded());
	Money work{0}; uint observed = 0;
	constexpr uint64_t denominator = uint64_t{CalendarTime::DAYS_IN_YEAR} * Ticks::DAY_TICKS * 256;
	// Bound current native per-vehicle accrual, including already pending
	// ground running_ticks. Reserve carriers remain stopped; no future fare,
	// asset sale, loan or reduced working reserve can pay this first stage.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->owner != _current_company) continue;
		if (!vehicle->IsPrimaryVehicle()) continue;
		const bool ground = vehicle->type == VehicleType::Train || vehicle->type == VehicleType::Road;
		// Only active ground bodies and the actual orbital idle constructor
		// can accrue costs during this unchanged-order interval. Surface-stored
		// Space hulls have their native zero upkeep and no running_ticks debt.
		if (ground && vehicle->vehstatus.Test(VehState::Stopped)) continue;
		if (!ground && vehicle->type != VehicleType::Space) continue;
		const uint ticks = limit + (ground ? vehicle->running_ticks : 0);
		const auto annual = vehicle->GetRunningCost().base(); REQUIRE(annual >= 0);
		work += Money{static_cast<int64_t>((static_cast<uint64_t>(annual) * ticks + denominator - 1) / denominator) + 1};
		++observed;
	}
	// The proposed new head is not in the world yet. Its full native annual
	// running price is added separately; the unpowered hopper has no upkeep.
	const auto annual = Engine::Get(head)->GetRunningCost().base(); REQUIRE(annual >= 0);
	if (buy_train) work += Money{static_cast<int64_t>((static_cast<uint64_t>(annual) * limit + uint64_t{CalendarTime::DAYS_IN_YEAR} * Ticks::DAY_TICKS - 1) /
		(uint64_t{CalendarTime::DAYS_IN_YEAR} * Ticks::DAY_TICKS)) + 2};
	const uint days = (limit + TimerGameEconomy::date_fract + Ticks::DAY_TICKS - 1) / Ticks::DAY_TICKS;
	bool monthly_boundary = false;
	// Interest and infrastructure maintenance are original monthly charges.
	// Refuse this small work window if even one admitted day can cross that
	// boundary rather than hiding those costs inside an inadequate allowance.
	for (uint offset = 1; offset <= days; ++offset) {
		const auto future = TimerGameEconomy::ConvertDateToYMD(TimerGameEconomy::Date{TimerGameEconomy::date.base() + static_cast<int>(offset)});
		if (future.month != TimerGameEconomy::month) monthly_boundary = true;
	}
	const Money reserve{100000}, wagon_cost = buy_train ? std::get<0>(hopper).GetCost() : Money{0};
	const auto target = reserve + infrastructure + head_cost + wagon_cost + work;
	// Both actual liquidity and a native-calendar-safe work window must fit
	// before any purchase. An actual empty returned train may later be sold,
	// but its unearned refund contributes zero to today's spending permission.
	const bool funded = !monthly_boundary && company->money >= target;
	fmt::print("SURFACE_RESUPPLY_PEAK head={} head_cost={} wagon_cost={} infrastructure={} ticks={} days={} observed_bodies={} work_bound={} reserve={} target={} cash={} shortfall={} monthly_boundary={} funded={} refund_counted=0 chemical_purchase_deferred=true conversion_pending={} salt_trip_unproved=true\n",
		head.base(), head_cost.base(), wagon_cost.base(), infrastructure.base(), limit, days, observed, work.base(), reserve.base(), target.base(),
		company->money.base(), std::max(Money{0}, target - company->money).base(), monthly_boundary, funded, GetRailType(surface_resupply_depot) != RAILTYPE_ELECTRIC);
	return funded;
}

/** Quote an actual powered-rail upgrade before rejecting cheaper electric heads.
 * @param staged Also price the conditional next short Salt window; never count its future empty-train sale as current cash.
 * @return Nothing; every physical track, reciprocal portal end and whole native conversion remains explicit and unmodified.
 */
static void QuerySurfaceResupplyElectrification(bool staged = false)
{
	const TileIndex compatible_depot{1638}, portal{1949};
	const auto pickup = Station::Get(StationID{5})->train_station.tile;
	REQUIRE(GetRailType(compatible_depot) == RAILTYPE_ELECTRIC);
	EngineID selected = EngineID::Invalid(); Money price{0};
	// A quote at the actual already electrified depot establishes the real
	// engine price. It grants no build permission at the still-ordinary Salt
	// depot, which must be converted and freshly quoted before any purchase.
	for (const auto *engine : Engine::IterateType(VehicleType::Train)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Train, _current_company)) continue;
		if (engine->VehInfo<RailVehicleInfo>().railveh_type == RailVehicleType::Wagon) continue;
		if (CheckUndergroundVehicleCapability(engine->index, MapLayer::Underground, INVALID_CARGO).Failed()) continue;
		if (!HasPowerOnRail(engine->VehInfo<RailVehicleInfo>().railtypes, RAILTYPE_ELECTRIC)) continue;
		const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(compatible_depot, compatible_depot, engine->index, false, INVALID_CARGO, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		// Select the cheapest admitted whole electric-depot purchase. Unsold
		// existing heads and their detached wagons contribute no new capital.
		if (selected != EngineID::Invalid() && std::get<0>(quote).GetCost() >= price) continue;
		selected = engine->index; price = std::get<0>(quote).GetCost();
	}
	REQUIRE(selected != EngineID::Invalid()); Money infrastructure{0};
	std::set<TileIndex> quoted; auto tile = surface_resupply_depot;
	// Follow the retained straight Salt route through its canonical portal
	// pair. Quote each real ordinary tile once and the reciprocal housing pair
	// once, so the second end cannot double-charge the conversion budget.
	while (true) {
		REQUIRE(TileX(tile) == TileX(surface_resupply_depot)); REQUIRE(GetTileOwner(tile) == _current_company);
		REQUIRE(GetRailType(tile) == RAILTYPE_RAIL);
		if (!quoted.contains(tile)) {
			const auto quote = QuoteSurfaceResupply<Commands::ConvertRail>(tile, tile, tile, RAILTYPE_ELECTRIC, false);
			fmt::print("SURFACE_RESUPPLY_CONVERSION tile={} cost={} admitted={} error={}\n", tile.base(), quote.GetCost().base(), quote.Succeeded(), quote.GetErrorMessage().base());
			if (quote.Failed()) {
				fmt::print("SURFACE_RESUPPLY_ELECTRIFICATION head={} head_cost={} admitted=false converted=false full_bill=false\n", selected.base(), price.base());
				return;
			}
			infrastructure += quote.GetCost(); quoted.insert(tile);
			if (IsLayerPortalTile(tile)) quoted.insert(GetLayerPortalPairTile(tile));
		}
		if (tile == pickup) break;
		if (tile == portal) tile = GetLayerPortalPairTile(tile);
		else tile = AddTileIndexDiffCWrap(tile, TileIndexDiffCByDiagDir(DiagDirection::SE));
		REQUIRE(quoted.size() <= 20);
	}
	// Normal and electric rail share physical geometry. This existing-route
	// check admits no electrical power until the actual quoted conversion and
	// its fresh native depot, portal and train checks have all completed.
	REQUIRE(SurfaceResupplyRailConnected(surface_resupply_depot, pickup, RailTypes{RAILTYPE_RAIL, RAILTYPE_ELECTRIC}));
	fmt::print("SURFACE_RESUPPLY_ELECTRIFICATION head={} head_cost={} infrastructure={} physical_tiles={} admitted=true converted=false new_depot_quote_pending=true full_bill=true\n",
		selected.base(), price.base(), infrastructure.base(), quoted.size());
	QuerySurfaceResupplyWindow(selected, price, infrastructure, compatible_depot);
	if (staged) QuerySurfaceResupplyPeak(selected, price, infrastructure);
}

/** Quote cheaper finite-Salt heads and prove or refuse reuse of existing income heads.
 * The query owns no rolling stock, future revenue, coupling permission or paid progress.
 * @return Nothing; typed native diagnostics disclose actual route and purchase boundaries.
 */
static void QuerySurfaceResupplyAlternatives()
{
	const auto salt = GetCargoTypeByLabel(CargoLabel{"SALT"});
	const auto *pickup = Station::Get(StationID{5});
	const std::map<std::string, TileIndex> depots{{"Surface Income farm", TileIndex{1638}}, {"Surface Income goods", TileIndex{3224}}, {"Surface Income coal", TileIndex{3161}}};
	uint owned = 0;
	// Reuse can avoid a capital purchase only if the existing income head can
	// physically leave its paid depot for Salt and return to its original line.
	// Capability metadata alone grants neither connectivity nor depot coupling.
	for (const auto *train : Train::Iterate()) {
		if (!train->IsFrontEngine()) continue;
		if (!train->name.starts_with("Surface Income ")) continue;
		const auto depot = depots.at(train->name);
		const bool outward = SurfaceResupplyRailConnected(depot, surface_resupply_depot, train->compatible_railtypes);
		const bool homeward = SurfaceResupplyRailConnected(surface_resupply_depot, depot, train->compatible_railtypes);
		const bool capability = CheckUndergroundVehicleCapability(train->engine_type, MapLayer::Underground, salt).Succeeded();
		REQUIRE(train->owner == _current_company); REQUIRE(train->GetNumOrders() == 2);
		fmt::print("SURFACE_RESUPPLY_REUSE vehicle={} engine={} depot={} capability={} outward={} homeward={} coupled=false admitted=false original_orders=2\n",
			train->index.base(), train->engine_type.base(), depot.base(), capability, outward, homeward);
		++owned;
	}
	REQUIRE(owned == 3);
	EngineID selected = EngineID::Invalid(); Money price{0};
	// Compare whole currently buildable ordinary native heads at the actual
	// Salt depot. A catalogue's expensive dedicated head is not the only option
	// when an original conventional head is legally buildable and powered here.
	for (const auto *engine : Engine::IterateType(VehicleType::Train)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Train, _current_company)) continue;
		if (engine->VehInfo<RailVehicleInfo>().railveh_type == RailVehicleType::Wagon) continue;
		if (CheckUndergroundVehicleCapability(engine->index, MapLayer::Underground, salt).Failed()) continue;
		const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, engine->index, false, INVALID_CARGO, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		const auto rails = GetAllPoweredRailTypes(engine->VehInfo<RailVehicleInfo>().railtypes);
		const bool mine = SurfaceResupplyRailConnected(surface_resupply_depot, pickup->train_station.tile, rails);
		fmt::print("SURFACE_RESUPPLY_HEAD engine={} cost={} depot=1373 mine_route={} capacity={}\n", engine->index.base(), std::get<0>(quote).GetCost().base(), mine, std::get<2>(quote));
		if (!mine) continue;
		// Only an admitted cheaper whole purchase replaces the best candidate;
		// no refund from an unsold head or future commercial income is counted.
		if (selected != EngineID::Invalid() && std::get<0>(quote).GetCost() >= price) continue;
		selected = engine->index; price = std::get<0>(quote).GetCost();
	}
	REQUIRE(selected != EngineID::Invalid());
	fmt::print("SURFACE_RESUPPLY_ALTERNATIVE selected={} head_cost={} reused=false purchased=false coupling_pending=true full_logistics_bill_complete=false\n", selected.base(), price.base());
	QuerySurfaceResupplyWindow(selected, price);
	QuerySurfaceResupplyElectrification();
}

/** Find the actual named chemical carrier without assuming a reusable pool slot.
 * @param renewed Explicit later paid27capacity replacement; default retains the historical21capacity contract.
 * @return Sole original or genuinely renewed named CHEM tanker.
 */
static const RoadVehicle *FindSurfaceResupplyChemical(bool renewed = false)
{
	const RoadVehicle *result = nullptr;
	for (const auto *truck : RoadVehicle::Iterate()) {
		if (truck->name != "Surface Chemical reserve") continue;
		REQUIRE(result == nullptr); result = truck;
	}
	REQUIRE(result != nullptr); REQUIRE(result->owner == _current_company);
	REQUIRE(result->cargo_type == GetCargoTypeByLabel(CargoLabel{"CHEM"})); REQUIRE(result->cargo_cap == (renewed ? 27 : 21));
	return result;
}

/** Complete read-only admission for the real carrier, useful existing inputs and crossing-safe staged delivery.
 * @param admission_only Explicit later funded-Core consumer: report current renewal/work/funding gaps without weakening the original funded query.
 */
static void QuerySurfaceResupply(bool admission_only = false)
{
	const auto *company = Company::Get(_current_company); const auto *chemical = FindSurfaceResupplyChemical();
	REQUIRE(chemical->IsStoppedInDepot()); REQUIRE(chemical->tile == surface_resupply_road_depot); REQUIRE(chemical->cargo.TotalCount() == 0);
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
	if (admission_only) {
		EngineID replacement = EngineID::Invalid(); Money replacement_cost{0}; uint replacement_capacity = 0;
		// The original parked tanker is far beyond useful life. Quote only
		// currently buildable real CHEMcarriers with enough whole input for
		// the missing18fuel; the lowest whole purchase is a prospective price,
		// never a delivered replacement, refunded old asset or fabricated stock.
		for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
			if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
			const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_road_depot, surface_resupply_road_depot, engine->index, false, GetCargoTypeByLabel(CargoLabel{"CHEM"}), ClientID::Server);
			if (std::get<0>(quote).Failed()) continue;
			if (std::get<2>(quote) < 18) continue;
			// Both an already selected candidate and a no-cheaper complete
			// quote are required to discard this option; capacity alone cannot
			// justify a costlier replacement or a claim of affordable supply.
			if (replacement != EngineID::Invalid() && std::get<0>(quote).GetCost() >= replacement_cost) continue;
			replacement = engine->index; replacement_cost = std::get<0>(quote).GetCost(); replacement_capacity = std::get<2>(quote);
		}
		const auto remaining = int64_t{chemical->max_age.base()} - chemical->age.base(); const Money work{18000};
		const uint candidate_fuel = 2 * (replacement_capacity / 2);
		const Money handling{static_cast<int64_t>((uint64_t{candidate_fuel} * CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"PROP"}))->weight + 15) / 16)};
		const auto target = gross + replacement_cost + reserve + work + handling;
		const Money credit{((std::max(Money{0}, target - company->money).base() + 9999) / 10000) * 10000};
		bool bank_admitted = credit == 0;
		if (credit > 0) bank_admitted = QuoteSurfaceResupply<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit).Succeeded();
		// Every prospective purchase, the actual matching bank permission
		// and this bounded work/reserve window must fit before any later paid
		// producer is admitted. Completion duration and fuel return logistics
		// remain explicit missing prices, not projected income or ready stock.
		const bool funded = replacement != EngineID::Invalid() && bank_admitted && company->money + credit >= target;
		fmt::print("SURFACE_RESUPPLY_ADMISSION locomotive={} wagon={} head_cost={} wagon_cost={} rail_gross={} salt_capacity=60 oil={} chemical_vehicle={} chemical_age={} chemical_life={} chemical_remaining={} chemical_capacity={} replacement={} replacement_cost={} replacement_capacity={} candidate_fuel={} minimum_fuel=18 handling_allowance={} work_allowance={} reserve={} target={} cash={} debt={} ceiling={} credit={} bank_admitted={} bounded_window_funded={} replacement_owned=false full_logistics_bill_complete=false\n",
			locomotive.base(), wagon.base(), std::get<0>(head).GetCost().base(), std::get<0>(hopper).GetCost().base(), gross.base(), input->waiting,
			chemical->index.base(), chemical->age.base(), chemical->max_age.base(), remaining, chemical->cargo_cap, replacement.base(), replacement_cost.base(), replacement_capacity,
			candidate_fuel, handling.base(), work.base(), reserve.base(), target.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base(), credit.base(), bank_admitted, funded);
		// Preserve both actual renewed fuel bodies/capacities independently of
		// old20/22role descriptions. No full27unit pickup is implied by18need.
		for (const auto *truck : RoadVehicle::Iterate()) {
			if (truck->name != "Surface Propellant reserve" && truck->name != "Surface Propellant refill") continue;
			fmt::print("SURFACE_RESUPPLY_FUEL_CARRIER vehicle={} capacity={} held={} age={} life={} remaining={} depot={} halted={}\n", truck->index.base(), truck->cargo_cap, truck->cargo.TotalCount(), truck->age.base(), truck->max_age.base(), int64_t{truck->max_age.base()} - truck->age.base(), truck->tile.base(), truck->IsStoppedInDepot());
		}
		return;
	}
	const auto shortfall = std::max(Money{0}, gross + reserve - company->money); const Money credit{((shortfall.base() + 9999) / 10000) * 10000};
	if (credit > 0) { auto bank = QuoteSurfaceResupply<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit); REQUIRE(bank.Succeeded()); }
	REQUIRE(company->current_loan + credit <= company->GetMaxLoan()); REQUIRE(company->money + credit - gross >= reserve);
	fmt::print("SURFACE_RESUPPLY_PLAN locomotive={} wagon={} head_cost={} wagon_cost={} gross={} salt_capacity=60 oil={} chemicals=21 chemical_vehicle={} chemical_capacity={} chemical_pickup={} road_edges={} road_crossings={} protected_trains={} reserve=100000 credit={} cash={} debt={} ceiling={} infrastructure=0 unsold_value_counted=false\n",
		locomotive.base(), wagon.base(), std::get<0>(head).GetCost().base(), std::get<0>(hopper).GetCost().base(), gross.base(), input->waiting,
		chemical->index.base(), chemical->cargo_cap, output->index.base(), edges, crossings, traffic, credit.base(), company->money.base(), company->current_loan.base(), company->GetMaxLoan().base());
}

/** Freshly admit and settle one complete original paid action.
 * @tparam Tcmd Original command. @tparam Targs Native argument types.
 * @param tile Actual action position. @param args Complete native arguments. @return Settled original result.
 */
template <Commands Tcmd, typename... Targs>
static auto PaySurfaceResupply(TileIndex tile, Targs... args)
{
	auto quote = QuoteSurfaceResupply<Tcmd>(tile, args...); REQUIRE(ExtractCommandCost(quote).Succeeded());
	auto paid = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(args...));
	REQUIRE(ExtractCommandCost(paid).Succeeded()); REQUIRE(ExtractCommandCost(paid).GetCost() == ExtractCommandCost(quote).GetCost());
	fmt::print("SURFACE_RESUPPLY_INVOICE command={} tile={} cost={}\n", GetCommandName(Tcmd), tile.base(), ExtractCommandCost(paid).GetCost().base());
	return paid;
}

/** @return All three original named income trains and their actual existing paid depots. */
static std::vector<std::pair<const Train *, TileIndex>> SurfaceResupplyTraffic()
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
static bool SurfaceResupplyTrafficProtected()
{
	for (const auto &[train, depot] : SurfaceResupplyTraffic()) {
		// Cargo delivery and true depot arrival both precede protection;
		// a temporary order or nearby stopped train cannot clear the crossing.
		if (!train->IsStoppedInDepot() || train->tile != depot) return false;
		for (const auto *part = train; part != nullptr; part = part->Next()) if (part->cargo.TotalCount() != 0) return false;
	}
	return true;
}

/** @return Actual newly purchased two-part Salt train, or nullptr before purchase/after genuine empty sale. */
static const Train *FindSurfaceResupplySalt()
{
	const Train *result = nullptr;
	// Only the named actual front and its linked paid hopper admit this
	// producer. Cargo label and original source are checked on every observation.
	for (const auto *train : Train::Iterate()) {
		if (train->name != "Surface Salt refill") continue;
		REQUIRE(result == nullptr); result = train;
	}
	if (result == nullptr) return nullptr;
	REQUIRE(result->IsFrontEngine()); REQUIRE(result->owner == _current_company); REQUIRE(result->engine_type == GetUndergroundEngine(4));
	const auto *wagon = result->Next(); REQUIRE(wagon != nullptr); REQUIRE(wagon->Next() == nullptr); REQUIRE(wagon->engine_type == GetUndergroundEngine(5));
	REQUIRE(wagon->cargo_type == GetCargoTypeByLabel(CargoLabel{"SALT"})); REQUIRE(wagon->cargo_cap == 60); REQUIRE(result->gcache.cached_total_length <= TILE_SIZE);
	REQUIRE(result->cargo.TotalCount() == 0);
	for (const auto *packet : *wagon->cargo.Packets()) { REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == 14); }
	return result;
}

/** @return Original explicit halt into the actual paid Surface Salt depot. */
static Order SurfaceResupplySaltHome()
{
	const auto *depot = Depot::GetByTile(surface_resupply_depot); REQUIRE(depot != nullptr);
	Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt); return home;
}

/** Preserve both commercial orders and genuinely recall each original income train through its normal receiver. */
static void RecallSurfaceResupplyTraffic()
{
	for (const auto &[train, tile] : SurfaceResupplyTraffic()) {
		REQUIRE(train->GetNumOrders() == 2); const auto *depot = Depot::GetByTile(tile); REQUIRE(depot != nullptr);
		Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{2}, home);
		if (train->cur_real_order_index != 1) PaySurfaceResupply<Commands::SkipToOrder>(TileIndex{0}, train->index, VehicleOrderID{1});
		fmt::print("SURFACE_RESUPPLY_RECALL head={} depot={} orders=3 supplied_pose=false\n", train->index.base(), tile.base());
	}
}

/** Buy only the admitted real empty Salt pair with freshly necessary matching credit, then issue original pickup/return orders. */
static void BuySurfaceResupplySalt()
{
	REQUIRE(SurfaceResupplyTrafficProtected()); REQUIRE(FindSurfaceResupplySalt() == nullptr);
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	const auto salt = GetCargoTypeByLabel(CargoLabel{"SALT"}); const auto locomotive = GetUndergroundEngine(4), wagon = GetUndergroundEngine(5);
	auto head_quote = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, locomotive, false, INVALID_CARGO, ClientID::Server);
	auto wagon_quote = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, wagon, false, salt, ClientID::Server);
	REQUIRE(std::get<0>(head_quote).Succeeded()); REQUIRE(std::get<0>(wagon_quote).Succeeded());
	const auto gross = std::get<0>(head_quote).GetCost() + std::get<0>(wagon_quote).GetCost();
	const auto shortfall = std::max(Money{0}, gross + Money{100000} - cash); const Money credit{((shortfall.base() + 9999) / 10000) * 10000};
	REQUIRE(debt + credit <= company->GetMaxLoan()); if (credit > 0) PaySurfaceResupply<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, credit);
	auto bought_head = PaySurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, locomotive, false, INVALID_CARGO, ClientID::Server);
	auto bought_wagon = PaySurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, wagon, false, salt, ClientID::Server);
	const auto head_id = std::get<1>(bought_head), wagon_id = std::get<1>(bought_wagon); const auto *train = Train::Get(head_id);
	if (Train::Get(wagon_id)->First() != train) PaySurfaceResupply<Commands::MoveRailVehicle>(TileIndex{0}, wagon_id, train->Last()->index, false);
	PaySurfaceResupply<Commands::RenameVehicle>(TileIndex{0}, head_id, std::string{"Surface Salt refill"}); REQUIRE(FindSurfaceResupplySalt() == train);
	REQUIRE(train->Next()->cargo.TotalCount() == 0); REQUIRE(train->IsStoppedInDepot());
	Order pickup; pickup.MakeGoToStation(StationID{5}); pickup.SetLoadType(OrderLoadType::FullLoad); pickup.SetUnloadType(OrderUnloadType::NoUnload); pickup.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, head_id, VehicleOrderID{0}, pickup);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, head_id, VehicleOrderID{1}, SurfaceResupplySaltHome());
	PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, head_id, true);
	REQUIRE(company->money == cash + credit - gross); REQUIRE(company->current_loan == debt + credit); REQUIRE(company->money >= 100000);
	fmt::print("SURFACE_RESUPPLY_PURCHASE head={} wagon={} gross={} credit={} cash={} debt={} reserve=100000 empty=true\n", head_id.base(), wagon_id.base(), gross.base(), credit.base(), company->money.base(), company->current_loan.base());
}

/** @return True only after the actual finite60Salt load has returned into the original depot. */
static bool SurfaceResupplySaltCollected()
{
	const auto *train = FindSurfaceResupplySalt(); REQUIRE(train != nullptr);
	return train->IsStoppedInDepot() && train->tile == surface_resupply_depot && train->Next()->cargo.TotalCount() == 60;
}

/** Send the original empty chemical tanker through the protected existing road to its actual producer stop. */
static void OrderSurfaceResupplyChemical()
{
	REQUIRE(SurfaceResupplyTrafficProtected()); REQUIRE(SurfaceResupplySaltCollected()); const auto *truck = FindSurfaceResupplyChemical();
	REQUIRE(truck->IsStoppedInDepot()); REQUIRE(truck->tile == surface_resupply_road_depot); REQUIRE(truck->cargo.TotalCount() == 0);
	// Replace only the retired PCHM delivery trip with real Source15pickup
	// and the same actual depot halt; no physical position or cargo is changed.
	while (truck->GetNumOrders() != 0) PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
	Order collect; collect.MakeGoToStation(GetStationIndex(TileIndex{1825})); collect.SetLoadType(OrderLoadType::FullLoad); collect.SetUnloadType(OrderUnloadType::NoUnload); collect.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, collect);
	Order home; home.MakeGoToDepot(Depot::GetByTile(surface_resupply_road_depot)->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, home);
	PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true);
}

/** @return True only when the actual empty tanker has crossed safely and is normally loading at Source15. */
static bool SurfaceResupplyChemicalAtSource()
{
	const auto *truck = FindSurfaceResupplyChemical();
	return truck->tile == TileIndex{1825} && truck->current_order.IsType(OT_LOADING) && truck->cargo.TotalCount() == 0;
}

/** Deliver the physically held Salt over the admitted short northern rail, retaining original empty-return ownership. */
static void OrderSurfaceResupplySaltDelivery()
{
	REQUIRE(SurfaceResupplyTrafficProtected()); REQUIRE(SurfaceResupplySaltCollected()); REQUIRE(SurfaceResupplyChemicalAtSource());
	const auto *train = FindSurfaceResupplySalt();
	// The real full load stays in the same native hopper while normal order
	// commands replace mine pickup with the original Surface chemical receiver.
	while (train->GetNumOrders() != 0) PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, train->index, VehicleOrderID{0});
	Order deliver; deliver.MakeGoToStation(StationID{4}); deliver.SetLoadType(OrderLoadType::NoLoad); deliver.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{0}, deliver);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{1}, SurfaceResupplySaltHome());
	PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, train->index, true);
}

/** @return True only after actual empty Salt return and21new real Source15chemicals in depot custody. */
static bool SurfaceResupplyCollected()
{
	const auto *train = FindSurfaceResupplySalt(); REQUIRE(train != nullptr); const auto *truck = FindSurfaceResupplyChemical();
	return train->IsStoppedInDepot() && train->tile == surface_resupply_depot && train->Next()->cargo.TotalCount() == 0 &&
		truck->IsStoppedInDepot() && truck->tile == surface_resupply_road_depot && truck->cargo.TotalCount() == 21;
}

/** Sell only the genuinely returned empty temporary train and restore each original useful income service. */
static void SettleSurfaceResupply()
{
	REQUIRE(SurfaceResupplyTrafficProtected()); REQUIRE(SurfaceResupplyCollected()); const auto *train = FindSurfaceResupplySalt();
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	const auto refund = PaySurfaceResupply<Commands::SellVehicle>(TileIndex{0}, train->index, true, false, ClientID::Server).GetCost(); REQUIRE(refund < 0);
	REQUIRE(FindSurfaceResupplySalt() == nullptr);
	// The original two commercial orders and all useful wagons survive;
	// deleting only the temporary halt resumes normal native routing and income.
	for (const auto &[income, depot] : SurfaceResupplyTraffic()) {
		REQUIRE(income->GetNumOrders() == 3); PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, income->index, VehicleOrderID{2});
		REQUIRE(income->GetNumOrders() == 2); PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, income->index, true);
	}
	REQUIRE(company->money == cash - refund); REQUIRE(company->current_loan == debt);
	fmt::print("SURFACE_RESUPPLY_SETTLED refund={} cash={} debt={} resumed=3 chemicals=21 empty_sale=true\n", (-refund).base(), company->money.base(), company->current_loan.base());
}

/** Convert only the fully funded original Salt route through native commands.
 * @return Nothing; exact invoices, original cash and real powered-route/depot quotes prove the paid boundary.
 */
static void ConvertSurfaceResupplyRoute()
{
	const TileIndex portal{1949}; const auto pickup = Station::Get(StationID{5})->train_station.tile;
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	std::vector<TileIndex> tiles; std::set<TileIndex> quoted; Money total{0}; auto tile = surface_resupply_depot;
	// Preflight every existing physical tile before the first expense. The
	// reciprocal housing is one native command and must not be charged twice;
	// neither portal conversion nor plain track conversion grants missing floor.
	while (true) {
		REQUIRE(TileX(tile) == TileX(surface_resupply_depot)); REQUIRE(GetTileOwner(tile) == _current_company);
		REQUIRE(GetRailType(tile) == RAILTYPE_RAIL);
		if (!quoted.contains(tile)) {
			const auto quote = QuoteSurfaceResupply<Commands::ConvertRail>(tile, tile, tile, RAILTYPE_ELECTRIC, false);
			REQUIRE(quote.Succeeded()); total += quote.GetCost(); tiles.push_back(tile); quoted.insert(tile);
			if (IsLayerPortalTile(tile)) quoted.insert(GetLayerPortalPairTile(tile));
		}
		if (tile == pickup) break;
		if (tile == portal) tile = GetLayerPortalPairTile(tile);
		else tile = AddTileIndexDiffCWrap(tile, TileIndexDiffCByDiagDir(DiagDirection::SE));
		REQUIRE(quoted.size() <= 20);
	}
	REQUIRE(total == 663); REQUIRE(quoted.size() == 13); REQUIRE(tiles.size() == 12);
	const auto head = QuoteSurfaceResupply<Commands::BuildVehicle>(TileIndex{1638}, TileIndex{1638}, EngineID{24}, false, INVALID_CARGO, ClientID::Server);
	REQUIRE(std::get<0>(head).Succeeded()); REQUIRE(QuerySurfaceResupplyPeak(EngineID{24}, std::get<0>(head).GetCost(), total));
	// Every whole command is freshly admitted immediately before payment.
	// No game tick is earned between invoices, so this route expense cannot
	// consume any of the separately capped500ticks of actual Salt movement.
	for (const auto current : tiles) PaySurfaceResupply<Commands::ConvertRail>(current, current, current, RAILTYPE_ELECTRIC, false);
	for (const auto current : quoted) REQUIRE(GetRailType(current) == RAILTYPE_ELECTRIC);
	REQUIRE(GetLayerPortalWorkState(portal) == LayerPortalWorkState::Connected);
	REQUIRE(SurfaceResupplyRailConnected(surface_resupply_depot, pickup, GetAllPoweredRailTypes(Engine::Get(EngineID{24})->VehInfo<RailVehicleInfo>().railtypes)));
	const auto actual_head = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, EngineID{24}, false, INVALID_CARGO, ClientID::Server);
	const auto actual_wagon = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, GetUndergroundEngine(5), false, GetCargoTypeByLabel(CargoLabel{"SALT"}), ClientID::Server);
	REQUIRE(std::get<0>(actual_head).Succeeded()); REQUIRE(std::get<0>(actual_head).GetCost() == std::get<0>(head).GetCost());
	REQUIRE(std::get<0>(actual_wagon).Succeeded()); REQUIRE(std::get<2>(actual_wagon) == 60);
	REQUIRE(company->money == cash - total); REQUIRE(company->current_loan == debt);
	fmt::print("SURFACE_RESUPPLY_CONVERTED tiles={} commands={} cost={} cash={} debt={} head_cost={} wagon_cost={} wagon_capacity=60 powered_route=true connected_portal=true earned=0\n",
		quoted.size(), tiles.size(), total.base(), company->money.base(), company->current_loan.base(), std::get<0>(actual_head).GetCost().base(), std::get<0>(actual_wagon).GetCost().base());
}

/** @return The exact paid two-part electric Salt consist, or nullptr before its genuine purchase. */
static const Train *FindSurfaceElectricSalt()
{
	const Train *result = nullptr;
	// The discovery name identifies this explicit paid producer only. An old
	// dedicated head, a recycled slot or another useful income consist cannot
	// substitute for its real linked electric head and sealed Salt hopper.
	for (const auto *train : Train::Iterate()) {
		if (train->name != "Surface Salt electric refill") continue;
		REQUIRE(result == nullptr); REQUIRE(train->IsFrontEngine()); REQUIRE(train->owner == _current_company);
		REQUIRE(train->engine_type == EngineID{24}); REQUIRE(train->cargo.TotalCount() == 0);
		const auto *wagon = train->Next(); REQUIRE(wagon != nullptr); REQUIRE(wagon->Next() == nullptr);
		REQUIRE(wagon->engine_type == GetUndergroundEngine(5)); REQUIRE(wagon->cargo_type == GetCargoTypeByLabel(CargoLabel{"SALT"}));
		REQUIRE(wagon->cargo_cap == 60); REQUIRE(train->gcache.cached_total_length <= TILE_SIZE);
		// Native mine packets retain their literal finite Industry14source;
		// bought CompanySupply or another industry's Salt cannot close a trip.
		for (const auto *packet : *wagon->cargo.Packets()) {
			REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == 14);
		}
		result = train;
	}
	return result;
}

/** @return True only when the real60Salt load has returned normally into the original powered depot. */
static bool SurfaceElectricSaltCollected()
{
	const auto *train = FindSurfaceElectricSalt(); REQUIRE(train != nullptr);
	// Full actual depot return, stopped state and physically stored quantity
	// are all required. Reserved loading packets, an order, a full mine buffer
	// or a train near home cannot substitute for actual loaded Salt at home.
	return train->IsStoppedInDepot() && train->tile == surface_resupply_depot &&
		train->Next()->cargo.StoredCount() == 60 && train->Next()->cargo.ReservedCount() == 0;
}

/** @return Explicit independently quoted later return cap in1..200ticks; default200retains earlier query/work behavior. */
static uint32_t SurfaceElectricSaltReturnLimit()
{
	const char *configured = std::getenv("OTTD_SURFACE_SALT_RETURN_LIMIT"); uint32_t limit = 200;
	if (configured != nullptr) {
		const std::string_view value{configured};
		const auto parsed = std::from_chars(value.data(), value.data() + value.size(), limit);
		REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == value.data() + value.size());
	}
	REQUIRE(limit > 0); REQUIRE(limit <= 200); return limit;
}

/** Buy and couple the admitted empty electric Salt consist without earning a tick or changing useful services. */
static void BuySurfaceElectricSalt()
{
	REQUIRE(FindSurfaceElectricSalt() == nullptr); REQUIRE(GetRailType(surface_resupply_depot) == RAILTYPE_ELECTRIC);
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	const auto head_quote = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, EngineID{24}, false, INVALID_CARGO, ClientID::Server);
	REQUIRE(std::get<0>(head_quote).Succeeded()); REQUIRE(QuerySurfaceResupplyPeak(EngineID{24}, std::get<0>(head_quote).GetCost(), Money{0}));
	const auto hopper = GetUndergroundEngine(5); const auto salt = GetCargoTypeByLabel(CargoLabel{"SALT"});
	const auto wagon_quote = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, hopper, false, salt, ClientID::Server);
	REQUIRE(std::get<0>(wagon_quote).Succeeded()); REQUIRE(std::get<2>(wagon_quote) == 60);
	// Establish real paid empty bodies before quoting their actual depot
	// coupling. The complete first-window cash and work reserve already fit;
	// no prospective resale or commercial service head finances the purchase.
	const auto head = std::get<1>(PaySurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, EngineID{24}, false, INVALID_CARGO, ClientID::Server));
	const auto wagon = std::get<1>(PaySurfaceResupply<Commands::BuildVehicle>(surface_resupply_depot, surface_resupply_depot, hopper, false, salt, ClientID::Server));
	if (Train::Get(wagon)->First()->index != head) PaySurfaceResupply<Commands::MoveRailVehicle>(TileIndex{0}, wagon, Train::Get(head)->Last()->index, false);
	PaySurfaceResupply<Commands::RenameVehicle>(TileIndex{0}, head, std::string{"Surface Salt electric refill"});
	const auto *train = FindSurfaceElectricSalt(); REQUIRE(train != nullptr); REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->Next()->cargo.TotalCount() == 0);
	REQUIRE(CheckUndergroundVehicleCapability(train->engine_type, MapLayer::Underground, INVALID_CARGO).Succeeded());
	REQUIRE(SurfaceResupplyRailConnected(surface_resupply_depot, Station::Get(StationID{5})->train_station.tile, train->compatible_railtypes));
	Order pickup; pickup.MakeGoToStation(StationID{5}); pickup.SetLoadType(OrderLoadType::FullLoad); pickup.SetUnloadType(OrderUnloadType::NoUnload); pickup.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, head, VehicleOrderID{0}, pickup);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, head, VehicleOrderID{1}, SurfaceResupplySaltHome());
	// Preserve the exact two commercial orders of all three original trains;
	// this independent Salt trip cannot silently commandeer their useful work.
	for (const auto &[income, depot] : SurfaceResupplyTraffic()) REQUIRE(income->GetNumOrders() == 2);
	PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, head, true);
	const auto gross = std::get<0>(head_quote).GetCost() + std::get<0>(wagon_quote).GetCost();
	REQUIRE(company->money == cash - gross); REQUIRE(company->current_loan == debt); REQUIRE(company->money >= 100000);
	fmt::print("SURFACE_RESUPPLY_ELECTRIC_PURCHASE head={} wagon={} engine=24 wagon_engine={} gross={} cash={} debt={} capacity=60 orders=2 pickup=5 depot=1373 held=0 earned=0\n",
		head.base(), wagon.base(), hopper.base(), gross.base(), company->money.base(), company->current_loan.base());
}

/** Earn only one separately admitted native interval, retaining actual partial or complete finite-Salt return.
 * @param limit First-window500tick cap or later freshly admitted200tick return cap; never replays earlier earned work.
 */
static void WorkSurfaceElectricSalt(uint32_t limit = 500)
{
	const auto *train = FindSurfaceElectricSalt(); REQUIRE(train != nullptr); REQUIRE(train->GetNumOrders() == 2);
	REQUIRE(QuerySurfaceResupplyPeak(EngineID{24}, Money{0}, Money{0}, false, limit));
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, SurfaceElectricSaltCollected);
	REQUIRE(Company::Get(_current_company)->money >= 100000);
	fmt::print("SURFACE_RESUPPLY_ELECTRIC_PROGRESS head={} wagon={} held={} stored={} reserved={} tile={} halted={} earned={} collected={} source=14 new_floor=false supplied_stock=false\n",
		train->index.base(), train->Next()->index.base(), train->Next()->cargo.TotalCount(), train->Next()->cargo.StoredCount(), train->Next()->cargo.ReservedCount(),
		train->tile.base(), train->IsStoppedInDepot(), work.ticks, SurfaceElectricSaltCollected());
}

/** @return Separately admitted delivery cap in1..900; no setting advances native time. */
static uint32_t SurfaceElectricSaltDeliveryLimit()
{
	const char *setting = std::getenv("OTTD_SURFACE_SALT_DELIVERY_LIMIT");
	if (setting == nullptr) return 900;
	const std::string_view text{setting}; uint32_t limit = 0;
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), limit);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == text.data() + text.size()); REQUIRE(limit > 0); REQUIRE(limit <= 900);
	return limit;
}

/** @return Actual newly delivered Salt, whether still in Industry15 or already consumed there. */
static uint64_t SurfaceElectricSaltDelivered()
{
	const auto *factory = Industry::Get(IndustryID{15}); const auto *processor = GetLayerProcessorState(factory->index);
	REQUIRE(processor != nullptr); REQUIRE(processor->recipe == MultiLayerRecipeID{"CHEM"});
	const auto salt = GetCargoTypeByLabel(CargoLabel{"SALT"});
	const auto input = std::ranges::find(factory->accepted, salt, &Industry::AcceptedCargo::cargo); REQUIRE(input != factory->accepted.end());
	const auto consumed = processor->consumed.at(CargoLabel{"SALT"}); REQUIRE(consumed >= 120); REQUIRE(consumed <= 180);
	return input->waiting + consumed - 120;
}

/** @return True only after actual60Salt receiver delivery and fully empty native return to the original depot. */
static bool SurfaceElectricSaltEmptyHome()
{
	const auto *train = FindSurfaceElectricSalt(); REQUIRE(train != nullptr);
	// Actual Industry receipt, an empty hopper and full stopped depot custody
	// jointly authorize capital recovery. No order or reserved packet is a sale.
	return SurfaceElectricSaltDelivered() == 60 && train->Next()->cargo.TotalCount() == 0 &&
		train->IsStoppedInDepot() && train->tile == surface_resupply_depot;
}

/** Price actual Salt delivery/empty return, including the stopped head and monthly charges.
 * @param limit Newly admitted actual ticks, no more than900; prior Salt work is never replayed.
 * @return Whether current cash alone covers the full bound above the scoped90000 working floor.
 */
static bool QuerySurfaceElectricSaltDelivery(uint32_t limit)
{
	REQUIRE(limit > 0); REQUIRE(limit <= 900); REQUIRE(!_settings_game.economy.infrastructure_maintenance);
	const auto *train = FindSurfaceElectricSalt(); REQUIRE(train != nullptr);
	const auto *company = Company::Get(_current_company); REQUIRE(company->money >= 90000);
	const auto receiver = Station::Get(StationID{4})->train_station.tile;
	const auto rails = GetAllPoweredRailTypes(Engine::Get(train->engine_type)->VehInfo<RailVehicleInfo>().railtypes);
	REQUIRE(SurfaceResupplyRailConnected(surface_resupply_depot, receiver, rails));
	if (SurfaceElectricSaltCollected()) {
		// These current native quotes establish that replacing this producer's
		// two orders and starting it spend no cash. Execution still requotes
		// each command after the preceding native order change has completed.
		Order delivery; delivery.MakeGoToStation(StationID{4}); delivery.SetLoadType(OrderLoadType::NoLoad); delivery.SetStopLocation(OrderStopLocation::FarEnd);
		const auto remove = QuoteSurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, train->index, VehicleOrderID{0});
		const auto receive = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{0}, delivery);
		const auto home = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{1}, SurfaceResupplySaltHome());
		const auto start = QuoteSurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, train->index, true);
		REQUIRE(remove.Succeeded()); REQUIRE(receive.Succeeded()); REQUIRE(home.Succeeded()); REQUIRE(start.Succeeded());
		REQUIRE(remove.GetCost() == 0); REQUIRE(receive.GetCost() == 0); REQUIRE(home.GetCost() == 0); REQUIRE(start.GetCost() == 0);
	}
	Money upkeep{0}; uint observed = 0;
	constexpr uint64_t denominator = uint64_t{CalendarTime::DAYS_IN_YEAR} * Ticks::DAY_TICKS * 256;
	// Bound every unchanged useful service and orbital constructor plus this
	// Salt head, which must depart even though it is currently stopped. Pending
	// running_ticks and rounding are charged before any future income is known.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->owner != _current_company) continue;
		if (!vehicle->IsPrimaryVehicle()) continue;
		const bool ground = vehicle->type == VehicleType::Train || vehicle->type == VehicleType::Road;
		// Other stopped ground reserves remain inactive; only this actual Salt
		// head is about to depart. Stored Space hulls keep their native zero cost.
		if (ground && vehicle->vehstatus.Test(VehState::Stopped) && vehicle != train) continue;
		if (!ground && vehicle->type != VehicleType::Space) continue;
		const auto annual = vehicle->GetRunningCost().base(); REQUIRE(annual >= 0);
		const uint ticks = limit + (ground ? vehicle->running_ticks : 0);
		upkeep += Money{static_cast<int64_t>((static_cast<uint64_t>(annual) * ticks + denominator - 1) / denominator) + 1};
		++observed;
	}
	const uint days = (limit + TimerGameEconomy::date_fract + Ticks::DAY_TICKS - 1) / Ticks::DAY_TICKS;
	uint boundaries = 0; auto previous = TimerGameEconomy::month;
	// Every possible charged day is examined, including the current fractional
	// day. Monthly loan interest and the legacy fixed station fee both apply
	// even though improved infrastructure maintenance is disabled here.
	for (uint offset = 1; offset <= days; ++offset) {
		const auto future = TimerGameEconomy::ConvertDateToYMD(TimerGameEconomy::Date{TimerGameEconomy::date.base() + static_cast<int>(offset)});
		if (future.month != previous) { ++boundaries; previous = future.month; }
	}
	REQUIRE(boundaries <= 1);
	const auto yearly = company->current_loan * _economy.interest_rate / 100;
	const Money interest = Money{(yearly.base() + 11) / 12} * boundaries;
	const Money station = (_price[Price::StationValue] >> 2) * boundaries;
	const Money floor{90000}; const auto target = floor + upkeep + interest + station;
	const bool funded = company->money >= target;
	// A real receiver route, complete present cash and bounded calendar costs
	// authorize only this trip. No future sale, fare or credit pays its bills.
	fmt::print("SURFACE_RESUPPLY_DELIVERY_ADMISSION ticks={} days={} observed_bodies={} upkeep={} month_boundaries={} interest={} station_charge={} infrastructure_charge=0 work_bound={} reserve={} target={} cash={} shortfall={} funded={} refund_counted=0 credit_counted=0 receiver=4 receiver_tile={} held={} delivered={} orders_pending={}\n",
		limit, days, observed, upkeep.base(), boundaries, interest.base(), station.base(), (upkeep + interest + station).base(), floor.base(), target.base(),
		company->money.base(), std::max(Money{0}, target - company->money).base(), funded, receiver.base(), train->Next()->cargo.TotalCount(), SurfaceElectricSaltDelivered(), SurfaceElectricSaltCollected());
	return funded;
}

/** Set native receiver/no-load and original depot-halt orders only after fresh funded admission. */
static void OrderSurfaceElectricSaltDelivery()
{
	REQUIRE(SurfaceElectricSaltCollected()); REQUIRE(QuerySurfaceElectricSaltDelivery(SurfaceElectricSaltDeliveryLimit()));
	const auto *train = FindSurfaceElectricSalt(); const auto cash = Company::Get(_current_company)->money;
	// Replace only this temporary producer's mine pickup. All original useful
	// services retain their commercial orders while the real Salt remains held.
	while (train->GetNumOrders() != 0) PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, train->index, VehicleOrderID{0});
	Order delivery; delivery.MakeGoToStation(StationID{4}); delivery.SetLoadType(OrderLoadType::NoLoad); delivery.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{0}, delivery);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{1}, SurfaceResupplySaltHome());
	PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, train->index, true);
	REQUIRE(Company::Get(_current_company)->money == cash);
}

/** Earn only a freshly funded actual delivery interval, preserving incomplete original progress. */
static void WorkSurfaceElectricSaltDelivery()
{
	const auto limit = SurfaceElectricSaltDeliveryLimit(); REQUIRE(QuerySurfaceElectricSaltDelivery(limit));
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, SurfaceElectricSaltEmptyHome);
	const auto *train = FindSurfaceElectricSalt(); REQUIRE(Company::Get(_current_company)->money >= 90000);
	fmt::print("SURFACE_RESUPPLY_DELIVERY_PROGRESS head={} wagon={} held={} stored={} reserved={} tile={} halted={} earned={} delivered={} empty_home={}\n",
		train->index.base(), train->Next()->index.base(), train->Next()->cargo.TotalCount(), train->Next()->cargo.StoredCount(), train->Next()->cargo.ReservedCount(),
		train->tile.base(), train->IsStoppedInDepot(), work.ticks, SurfaceElectricSaltDelivered(), SurfaceElectricSaltEmptyHome());
}

/** Sell only the physically empty returned temporary consist and restore the original working reserve. */
static void SellSurfaceElectricSalt()
{
	REQUIRE(SurfaceElectricSaltEmptyHome()); const auto *train = FindSurfaceElectricSalt();
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	const auto quote = QuoteSurfaceResupply<Commands::SellVehicle>(TileIndex{0}, train->index, true, false, ClientID::Server);
	REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() < 0); REQUIRE(cash - quote.GetCost() >= 100000);
	const auto head = train->index, wagon = train->Next()->index;
	const auto paid = PaySurfaceResupply<Commands::SellVehicle>(TileIndex{0}, head, true, false, ClientID::Server);
	REQUIRE(FindSurfaceElectricSalt() == nullptr); REQUIRE(company->money == cash - paid.GetCost()); REQUIRE(company->current_loan == debt);
	fmt::print("SURFACE_RESUPPLY_EMPTY_SALE head={} wagon={} cost={} cash_before={} cash={} debt={} reserve_restored=true delivered=60 held=0\n",
		head.base(), wagon.base(), paid.GetCost().base(), cash.base(), company->money.base(), debt.base());
}

/** Requote only the remaining real chemical/fuel window after actual Salt delivery and banked sale. */
static void QuerySurfaceRemainingFuel()
{
	REQUIRE(FindSurfaceElectricSalt() == nullptr); REQUIRE(SurfaceElectricSaltDelivered() == 60);
	const auto *company = Company::Get(_current_company); REQUIRE(company->money >= 100000);
	const auto *old = FindSurfaceResupplyChemical(); REQUIRE(old->IsStoppedInDepot()); REQUIRE(old->tile == surface_resupply_road_depot); REQUIRE(old->cargo.TotalCount() == 0);
	const auto chemical = GetCargoTypeByLabel(CargoLabel{"CHEM"}), propellant = GetCargoTypeByLabel(CargoLabel{"PROP"});
	EngineID replacement = EngineID::Invalid(); Money replacement_cost{0}; uint capacity = 0;
	// The expired tanker supplies no useful remaining life. Compare real whole
	// build quotes; neither its future disposal nor any further train refund
	// reduces the remaining bill. Salt has already been physically delivered.
	for (const auto *engine : Engine::IterateType(VehicleType::Road)) {
		if (!IsEngineBuildable(engine->index, VehicleType::Road, _current_company)) continue;
		const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_road_depot, surface_resupply_road_depot, engine->index, false, chemical, ClientID::Server);
		if (std::get<0>(quote).Failed()) continue;
		if (std::get<2>(quote) < 18) continue;
		// Keep the cheapest complete admitted purchase; capacity cannot turn
		// a higher quote into affordable capital or already manufactured fuel.
		if (replacement != EngineID::Invalid() && std::get<0>(quote).GetCost() >= replacement_cost) continue;
		replacement = engine->index; replacement_cost = std::get<0>(quote).GetCost(); capacity = std::get<2>(quote);
	}
	REQUIRE(replacement != EngineID::Invalid()); const uint fuel = 2 * (capacity / 2); REQUIRE(fuel >= 18);
	uint fuel_carriers = 0;
	// Both actual renewed fuel carriers remain empty at their existing depot.
	// Their observed capacity and useful life replace stale historical20/22
	// descriptions, while every useful commercial service retains its orders.
	for (const auto *truck : RoadVehicle::Iterate()) {
		if (truck->name != "Surface Propellant reserve" && truck->name != "Surface Propellant refill") continue;
		REQUIRE(truck->cargo_type == propellant); REQUIRE(truck->cargo_cap >= fuel);
		REQUIRE(truck->IsStoppedInDepot()); REQUIRE(truck->tile == surface_resupply_road_depot); REQUIRE(truck->cargo.TotalCount() == 0);
		REQUIRE(truck->age < truck->max_age); ++fuel_carriers;
	}
	REQUIRE(fuel_carriers == 2); uint crossings = 0;
	// All remaining source, processor and hub legs use existing reciprocal road
	// edges. A crossing count discloses traffic exposure; it grants no halt,
	// priority, new geometry or guarantee of trip completion within an allowance.
	for (const auto bay : {TileIndex{1825}, TileIndex{1895}, TileIndex{1896}, TileIndex{2504}}) crossings += QuerySurfaceResupplyRoad(bay);
	const auto membership = GetLaunchHubMembership(StationID{0}); REQUIRE(membership.has_value());
	const auto *center = ResolveLaunchCenter(membership->center), *member = ResolveLaunchHubMember(membership->center, membership->member);
	REQUIRE(center != nullptr); REQUIRE(member != nullptr); REQUIRE(center->index == StationID{21});
	REQUIRE(GetLaunchStockHeadroom(*center, propellant) >= fuel); REQUIRE(GetLaunchStockHeadroom(*member, propellant) >= fuel);
	const Money handling{static_cast<int64_t>((uint64_t{fuel} * CargoSpec::Get(propellant)->weight + 15) / 16)};
	const Money work{18000}, reserve{100000}; const auto target = replacement_cost + handling + work + reserve;
	const auto *factory = Industry::Get(IndustryID{15}); const auto salt = GetCargoTypeByLabel(CargoLabel{"SALT"});
	const auto salt_input = std::ranges::find(factory->accepted, salt, &Industry::AcceptedCargo::cargo); REQUIRE(salt_input != factory->accepted.end());
	const auto chem_output = std::ranges::find(factory->produced, chemical, &Industry::ProducedCargo::cargo); REQUIRE(chem_output != factory->produced.end());
	fmt::print("SURFACE_RESUPPLY_REMAINING replacement={} replacement_cost={} chemical_capacity={} fuel={} minimum_fuel=18 handling={} work_allowance={} reserve={} target={} cash={} debt={} funded={} refund_counted=0 credit_counted=0 salt_delivered=60 salt_input={} chemical_output={} road_crossings={} infrastructure=0 conversion_paid=true salt_train_sold=true chemical_owned=false fuel_procured=false completion_duration_unproved=true\n",
		replacement.base(), replacement_cost.base(), capacity, fuel, handling.base(), work.base(), reserve.base(), target.base(), company->money.base(), company->current_loan.base(),
		company->money >= target, salt_input->waiting, chem_output->waiting, crossings);
}

/** Genuinely buy the admitted chemical replacement before banking the empty expired tanker's sale. */
static void RenewSurfaceResupplyChemical()
{
	QuerySurfaceRemainingFuel();
	const auto *old = FindSurfaceResupplyChemical(); REQUIRE(old->age >= old->max_age);
	const auto old_id = old->index; const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	const auto chemical = GetCargoTypeByLabel(CargoLabel{"CHEM"});
	const auto quote = QuoteSurfaceResupply<Commands::BuildVehicle>(surface_resupply_road_depot, surface_resupply_road_depot, EngineID{134}, false, chemical, ClientID::Server);
	REQUIRE(std::get<0>(quote).Succeeded()); REQUIRE(std::get<2>(quote) == 27);
	const auto gross = std::get<0>(quote).GetCost(); REQUIRE(cash >= gross + Money{118026});
	// Pay the whole new purchase first from already banked cash. The expired
	// empty tanker's later native refund contributes zero to this permission.
	const auto id = std::get<1>(PaySurfaceResupply<Commands::BuildVehicle>(surface_resupply_road_depot, surface_resupply_road_depot, EngineID{134}, false, chemical, ClientID::Server));
	const auto *truck = RoadVehicle::Get(id); REQUIRE(truck->cargo_type == chemical); REQUIRE(truck->cargo_cap == 27);
	REQUIRE(truck->IsStoppedInDepot()); REQUIRE(truck->tile == surface_resupply_road_depot); REQUIRE(truck->cargo.TotalCount() == 0);
	REQUIRE(truck->age.base() == 0); REQUIRE(truck->GetNumOrders() == 0);
	const auto refund = PaySurfaceResupply<Commands::SellVehicle>(TileIndex{0}, old_id, false, false, ClientID::Server).GetCost(); REQUIRE(refund < 0);
	PaySurfaceResupply<Commands::RenameVehicle>(TileIndex{0}, id, std::string{"Surface Chemical reserve"});
	REQUIRE(FindSurfaceResupplyChemical(true) == truck); REQUIRE(truck->engine_type == EngineID{134});
	REQUIRE(company->money == cash - gross - refund); REQUIRE(company->current_loan == debt); REQUIRE(company->money >= 100000);
	fmt::print("SURFACE_RESUPPLY_CHEMICAL_RENEWAL old={} vehicle={} engine=134 capacity=27 held=0 depot=2443 halted=true age=0 gross={} refund={} net={} cash_before={} cash={} debt={} reserve=100000 earned=0 supplied_stock=false\n",
		old_id.base(), id.base(), gross.base(), refund.base(), (gross + refund).base(), cash.base(), company->money.base(), debt.base());
}

/** @return The genuinely renewed CHEM carrier, retaining literal Industry15 packet sources. */
static const RoadVehicle *FindSurfaceRenewedChemical()
{
	const auto *truck = FindSurfaceResupplyChemical(true); REQUIRE(truck->engine_type == EngineID{134}); REQUIRE(truck->age < truck->max_age);
	// Every physical or reserved packet must come from the original paid
	// chemical factory; a bought supply packet cannot complete this pickup.
	for (const auto *packet : *truck->cargo.Packets()) {
		REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == 15);
	}
	return truck;
}

/** @return Separately admitted chemical pickup cap in1..3000ticks, never elapsed work. */
static uint32_t SurfaceChemicalWorkLimit()
{
	const char *setting = std::getenv("OTTD_SURFACE_CHEMICAL_WORK_LIMIT");
	if (setting == nullptr) return 3000;
	const std::string_view text{setting}; uint32_t limit = 0;
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), limit);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == text.data() + text.size()); REQUIRE(limit > 0); REQUIRE(limit <= 3000);
	return limit;
}

/** @return Original explicit road halt into the actual paid depot2443. */
static Order SurfaceChemicalHome()
{
	const auto *depot = Depot::GetByTile(surface_resupply_road_depot); REQUIRE(depot != nullptr);
	Order home; home.MakeGoToDepot(depot->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt); return home;
}

/** Quote all current costs for one bounded chemical-pickup continuation.
 * @param limit Actual new native interval, at most3000ticks; all prior paid work remains retained.
 * @param departing Explicit other paid road carrier for later fuel collection; nullptr retains the renewed CHEM default.
 * @param pickup Actual source road bay for the proposed carrier; default retains original CHEM source1825.
 * @param load Native pickup policy; default full27CHEM, later fuel uses only actual available26in its27capacity body.
 * @param handling Conservative native mass-fee allowance for a later stock-backed reverse transfer; default0grants no handling stock.
 * @return Whether present cash covers every possible charge above the unchanged100000reserve.
 */
static bool QuerySurfaceChemicalWork(uint32_t limit, const RoadVehicle *departing = nullptr, TileIndex pickup = TileIndex{1825}, OrderLoadType load = OrderLoadType::FullLoad, Money handling = Money{0})
{
	REQUIRE(limit > 0); REQUIRE(limit <= 3000); REQUIRE(!_settings_game.economy.infrastructure_maintenance);
	const auto *truck = departing == nullptr ? FindSurfaceRenewedChemical() : departing; const auto *company = Company::Get(_current_company); REQUIRE(truck->owner == _current_company);
	REQUIRE(company->engine_renew_list == nullptr); REQUIRE(handling >= 0);
	// Native NeedsAutorenewing compares age-minus-max_age with signed months.
	// Positive months postpone renewal until after useful life; only a negative
	// setting needs an early lead guard. Still preserve positive useful life for
	// every active service throughout this window, with no settings/age change.
	const int renewal_lead = company->settings.engine_renew ? std::max(0, -int{company->settings.engine_renew_months} * 30) : 0;
	REQUIRE(company->money >= 100000); QuerySurfaceResupplyRoad(pickup);
	const auto *station = Station::GetByTile(pickup); REQUIRE(station != nullptr); REQUIRE(station->owner == _current_company);
	if (truck->GetNumOrders() == 0) {
		Order collect; collect.MakeGoToStation(station->index); collect.SetLoadType(load); collect.SetUnloadType(OrderUnloadType::NoUnload); collect.SetStopLocation(OrderStopLocation::FarEnd);
		const auto source_order = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, collect);
		const auto home = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, SurfaceChemicalHome());
		const auto start = QuoteSurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true);
		REQUIRE(source_order.Succeeded()); REQUIRE(home.Succeeded()); REQUIRE(start.Succeeded());
		REQUIRE(source_order.GetCost() == 0); REQUIRE(home.GetCost() == 0); REQUIRE(start.GetCost() == 0);
	}
	const uint days = (limit + TimerGameEconomy::date_fract + Ticks::DAY_TICKS - 1) / Ticks::DAY_TICKS;
	Money upkeep{0}; uint observed = 0;
	constexpr uint64_t denominator = uint64_t{CalendarTime::DAYS_IN_YEAR} * Ticks::DAY_TICKS * 256;
	// Existing commercial services and the idle orbital constructor keep their
	// real costs. Include the actual departing tanker even while stopped;
	// all other empty stopped reserves remain inactive and cannot supply cash.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->owner != _current_company) continue;
		if (!vehicle->IsPrimaryVehicle()) continue;
		const bool ground = vehicle->type == VehicleType::Train || vehicle->type == VehicleType::Road;
		// Other stopped ground reserves do not enter service. Only active ground
		// and actual Space bodies can accrue costs during this unchanged cohort.
		if (ground && vehicle->vehstatus.Test(VehState::Stopped) && vehicle != truck) continue;
		if (!ground && vehicle->type != VehicleType::Space) continue;
		if (ground) REQUIRE(vehicle->max_age.base() - vehicle->age.base() > static_cast<int>(days) + renewal_lead);
		const auto annual = vehicle->GetRunningCost().base(); REQUIRE(annual >= 0);
		const uint ticks = limit + (ground ? vehicle->running_ticks : 0);
		upkeep += Money{static_cast<int64_t>((static_cast<uint64_t>(annual) * ticks + denominator - 1) / denominator) + 1};
		++observed;
	}
	uint boundaries = 0; auto previous = TimerGameEconomy::month; bool year_boundary = false;
	// Monthly interest and the fixed station charge apply at every possible
	// boundary. Reject an annual price/renewal boundary rather than guessing
	// future prices; actual useful-service life and signed renewal policy were
	// independently checked above rather than assuming an early six-month bill.
	for (uint offset = 1; offset <= days; ++offset) {
		const auto future = TimerGameEconomy::ConvertDateToYMD(TimerGameEconomy::Date{TimerGameEconomy::date.base() + static_cast<int>(offset)});
		if (future.month != previous) { ++boundaries; previous = future.month; }
		if (future.year != TimerGameEconomy::year) year_boundary = true;
	}
	REQUIRE(boundaries <= 2); const auto yearly = company->current_loan * _economy.interest_rate / 100;
	const Money interest = Money{(yearly.base() + 11) / 12} * boundaries;
	const Money station_charge = (_price[Price::StationValue] >> 2) * boundaries;
	const Money reserve{100000}; const auto work_bound = upkeep + interest + station_charge + handling; const auto target = reserve + work_bound;
	// Native cash and a safe cost/life calendar must both admit this bounded
	// movement. Future freight fares, asset refunds and extra credit contribute0.
	const bool funded = !year_boundary && company->money >= target;
	fmt::print("SURFACE_RESUPPLY_CHEMICAL_ADMISSION ticks={} days={} observed_bodies={} upkeep={} month_boundaries={} interest={} station_charge={} work_bound={} reserve={} target={} cash={} shortfall={} year_boundary={} funded={} refund_counted=0 credit_counted=0 capacity=27 held={} depot=2443 source_tile={} handling_allowance={} renew_months={} renewal_lead={}\n",
		limit, days, observed, upkeep.base(), boundaries, interest.base(), station_charge.base(), work_bound.base(), reserve.base(), target.base(), company->money.base(),
		std::max(Money{0}, target - company->money).base(), year_boundary, funded, truck->cargo.TotalCount(), pickup.base(), handling.base(), company->settings.engine_renew_months, renewal_lead);
	return funded;
}

/** @return True only for actual27stored/zeroreserved Source15 chemicals fully stopped at the original depot. */
static bool SurfaceRenewedChemicalCollected()
{
	const auto *truck = FindSurfaceRenewedChemical();
	// Depot custody plus physically stored quantity proves collection; a full
	// reservation at the source stop or a proposed return order proves none.
	return truck->IsStoppedInDepot() && truck->tile == surface_resupply_road_depot && truck->cargo.StoredCount() == 27 && truck->cargo.ReservedCount() == 0;
}

/** Start only the freshly funded renewed carrier's actual source pickup and depot-return orders. */
static void OrderSurfaceRenewedChemicalPickup()
{
	const auto *truck = FindSurfaceRenewedChemical(); REQUIRE(truck->IsStoppedInDepot()); REQUIRE(truck->tile == surface_resupply_road_depot);
	REQUIRE(truck->cargo.TotalCount() == 0); REQUIRE(truck->GetNumOrders() == 0); REQUIRE(QuerySurfaceChemicalWork(SurfaceChemicalWorkLimit()));
	Order collect; collect.MakeGoToStation(GetStationIndex(TileIndex{1825})); collect.SetLoadType(OrderLoadType::FullLoad); collect.SetUnloadType(OrderUnloadType::NoUnload); collect.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, collect);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, SurfaceChemicalHome());
	PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true);
}

/** Earn one separately funded actual chemical pickup interval and retain any incomplete original progress. */
static void WorkSurfaceRenewedChemicalPickup()
{
	REQUIRE(FindSurfaceRenewedChemical()->GetNumOrders() == 2);
	const auto limit = SurfaceChemicalWorkLimit(); REQUIRE(QuerySurfaceChemicalWork(limit));
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, SurfaceRenewedChemicalCollected);
	const auto *truck = FindSurfaceRenewedChemical(); REQUIRE(Company::Get(_current_company)->money >= 100000);
	fmt::print("SURFACE_RESUPPLY_CHEMICAL_PROGRESS vehicle={} held={} stored={} reserved={} tile={} halted={} earned={} collected={} source=15 supplied_stock=false\n",
		truck->index.base(), truck->cargo.TotalCount(), truck->cargo.StoredCount(), truck->cargo.ReservedCount(), truck->tile.base(), truck->IsStoppedInDepot(), work.ticks, SurfaceRenewedChemicalCollected());
}

/** @return Actual newly produced PROP at the original PCHM6, separately from its historical42units. */
static uint64_t SurfaceAdditionalPropellantProduced()
{
	const auto *processor = GetLayerProcessorState(IndustryID{6}); REQUIRE(processor != nullptr); REQUIRE(processor->recipe == MultiLayerRecipeID{"PCHM"});
	const auto produced = processor->produced.at(CargoLabel{"PROP"}); REQUIRE(produced >= 42); REQUIRE(produced <= 68); return produced - 42;
}

/** @return Actual new CHEM received by PCHM6, either still waiting or already consumed in complete batches. */
static uint64_t SurfaceChemicalDeliveredToPchm()
{
	const auto *plant = Industry::Get(IndustryID{6}); const auto *processor = GetLayerProcessorState(plant->index); REQUIRE(processor != nullptr);
	const auto input = std::ranges::find(plant->accepted, GetCargoTypeByLabel(CargoLabel{"CHEM"}), &Industry::AcceptedCargo::cargo); REQUIRE(input != plant->accepted.end());
	const auto consumed = processor->consumed.at(CargoLabel{"CHEM"}); REQUIRE(consumed >= 42); REQUIRE(consumed <= 68);
	return input->waiting + consumed - 42;
}

/** Quote the real PCHM receiver/order permissions in addition to every current work/calendar charge.
 * @param limit Separately admitted actual chemical-delivery interval in1..3000ticks.
 * @return Whether current original cash alone covers the unchanged100000reserve and full bounded charges.
 */
static bool QuerySurfaceChemicalDelivery(uint32_t limit)
{
	const bool funded = QuerySurfaceChemicalWork(limit); const auto *truck = FindSurfaceRenewedChemical();
	QuerySurfaceResupplyRoad(TileIndex{1895}); const auto *station = Station::GetByTile(TileIndex{1895}); REQUIRE(station != nullptr); REQUIRE(station->owner == _current_company);
	Order delivery; delivery.MakeGoToStation(station->index); delivery.SetLoadType(OrderLoadType::NoLoad); delivery.SetStopLocation(OrderStopLocation::FarEnd);
	const auto remove = QuoteSurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
	const auto receive = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, delivery);
	const auto home = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, SurfaceChemicalHome());
	REQUIRE(remove.Succeeded()); REQUIRE(receive.Succeeded()); REQUIRE(home.Succeeded());
	REQUIRE(remove.GetCost() == 0); REQUIRE(receive.GetCost() == 0); REQUIRE(home.GetCost() == 0);
	fmt::print("SURFACE_RESUPPLY_PCHM_ADMISSION held={} delivered={} produced={} receiver={} receiver_tile=1895 capacity=27 expected_produced=26 reserve=100000 funded={} supplied_stock=false\n",
		truck->cargo.StoredCount(), SurfaceChemicalDeliveredToPchm(), SurfaceAdditionalPropellantProduced(), station->index.base(), funded);
	return funded;
}

/** @return True only after actual27CHEM delivery,26native PROP production and empty stopped return to2443. */
static bool SurfaceChemicalPchmComplete()
{
	const auto *truck = FindSurfaceRenewedChemical();
	// Native complete-batch counters prove processing only; an empty returned
	// carrier separately proves delivery/transport. New fuel remains with its
	// actual Industry/station owner until a later genuine pickup and hub leg.
	return SurfaceChemicalDeliveredToPchm() == 27 && SurfaceAdditionalPropellantProduced() == 26 &&
		truck->IsStoppedInDepot() && truck->tile == surface_resupply_road_depot && truck->cargo.TotalCount() == 0;
}

/** Replace only the renewed carrier's pickup with actual no-load PCHM delivery after fresh admission. */
static void OrderSurfaceChemicalDelivery()
{
	REQUIRE(SurfaceRenewedChemicalCollected()); REQUIRE(QuerySurfaceChemicalDelivery(SurfaceChemicalWorkLimit()));
	const auto *truck = FindSurfaceRenewedChemical();
	// The actual27Source15chemicals stay in their paid body. Only its temporary
	// source pickup changes; every useful original service keeps its orders.
	while (truck->GetNumOrders() != 0) PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
	Order delivery; delivery.MakeGoToStation(GetStationIndex(TileIndex{1895})); delivery.SetLoadType(OrderLoadType::NoLoad); delivery.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, delivery);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, SurfaceChemicalHome());
	PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true);
}

/** Earn one freshly funded actual chemical-delivery/processing interval, retaining any partial original. */
static void WorkSurfaceChemicalDelivery()
{
	const auto limit = SurfaceChemicalWorkLimit(); REQUIRE(QuerySurfaceChemicalDelivery(limit));
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, SurfaceChemicalPchmComplete);
	const auto *truck = FindSurfaceRenewedChemical(); REQUIRE(Company::Get(_current_company)->money >= 100000);
	fmt::print("SURFACE_RESUPPLY_PCHM_PROGRESS vehicle={} held={} stored={} reserved={} tile={} halted={} earned={} delivered={} produced={} complete={} new_fuel_collected=false\n",
		truck->index.base(), truck->cargo.TotalCount(), truck->cargo.StoredCount(), truck->cargo.ReservedCount(), truck->tile.base(), truck->IsStoppedInDepot(), work.ticks,
		SurfaceChemicalDeliveredToPchm(), SurfaceAdditionalPropellantProduced(), SurfaceChemicalPchmComplete());
}

/** @return Sole existing renewed original reserve2with literal Source6PROP; no other carrier is commandeered. */
static const RoadVehicle *FindSurfacePropellantReserve()
{
	const RoadVehicle *result = nullptr;
	// This original role/body remains distinct from refill39and newly bought
	// CHEM55. Neither a renamed useful service nor a recycled slot supplies fuel.
	for (const auto *truck : RoadVehicle::Iterate()) {
		if (truck->name != "Surface Propellant reserve") continue;
		REQUIRE(result == nullptr); result = truck;
	}
	REQUIRE(result != nullptr); REQUIRE(result->index == VehicleID{2}); REQUIRE(result->owner == _current_company);
	REQUIRE(result->engine_type == EngineID{134}); REQUIRE(result->cargo_type == GetCargoTypeByLabel(CargoLabel{"PROP"})); REQUIRE(result->cargo_cap == 27);
	REQUIRE(result->age < result->max_age); REQUIRE(result->cargo.TotalCount() <= 26);
	// Only actual packets supplied by the original PCHM industry may extend
	// the twelve original launch units; its produced counter alone supplies0.
	for (const auto *packet : *result->cargo.Packets()) {
		REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == 6);
	}
	return result;
}

/** Quote actual26unit fuel collection without requesting an unavailable full27load.
 * @param limit Newly admitted native work cap in1..3000ticks.
 * @return Whether actual original cash covers its own departing-carrier upkeep and every calendar charge.
 */
static bool QuerySurfacePropellantPickup(uint32_t limit)
{
	const auto *truck = FindSurfacePropellantReserve(); REQUIRE(SurfaceAdditionalPropellantProduced() == 26);
	const bool funded = QuerySurfaceChemicalWork(limit, truck, TileIndex{1896}, OrderLoadType::LoadIfPossible);
	Order pickup; pickup.MakeGoToStation(GetStationIndex(TileIndex{1896})); pickup.SetLoadType(OrderLoadType::LoadIfPossible); pickup.SetUnloadType(OrderUnloadType::NoUnload); pickup.SetStopLocation(OrderStopLocation::FarEnd);
	const auto source = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, pickup);
	const auto home = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, SurfaceChemicalHome());
	const auto start = QuoteSurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true);
	REQUIRE(source.Succeeded()); REQUIRE(home.Succeeded()); REQUIRE(start.Succeeded());
	REQUIRE(source.GetCost() == 0); REQUIRE(home.GetCost() == 0); REQUIRE(start.GetCost() == 0);
	if (truck->GetNumOrders() != 0) {
		const auto remove = QuoteSurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}); REQUIRE(remove.Succeeded()); REQUIRE(remove.GetCost() == 0);
	}
	const auto *plant = Industry::Get(IndustryID{6}); const auto output = std::ranges::find(plant->produced, truck->cargo_type, &Industry::ProducedCargo::cargo); REQUIRE(output != plant->produced.end());
	fmt::print("SURFACE_RESUPPLY_FUEL_ADMISSION vehicle={} capacity=27 held={} stored={} reserved={} buffered={} produced=26 orders={} source_tile=1896 available_load=true impossible_full_load=false funded={} reserve=100000\n",
		truck->index.base(), truck->cargo.TotalCount(), truck->cargo.StoredCount(), truck->cargo.ReservedCount(), output->waiting, truck->GetNumOrders(), funded);
	return funded;
}

/** @return True after a genuine completed pickup round reaches the original depot, including a retained partial load. */
static bool SurfacePropellantRoundHome()
{
	const auto *truck = FindSurfacePropellantReserve();
	// Stop on each actual depot return, even when the first native available-
	// load round is partial. Another fresh admission must pay a later round;
	// standing in the depot cannot manufacture the missing twenty-seventh unit.
	return truck->IsStoppedInDepot() && truck->tile == surface_resupply_road_depot;
}

/** Set only the admitted original reserve's actual available-load/no-unload and depot-halt orders. */
static void OrderSurfacePropellantPickup()
{
	const auto *truck = FindSurfacePropellantReserve(); REQUIRE(SurfacePropellantRoundHome()); REQUIRE(truck->cargo.TotalCount() < 26);
	REQUIRE(QuerySurfacePropellantPickup(SurfaceChemicalWorkLimit()));
	// Real partial cargo stays aboard while this one reserve's old or prior
	// round orders are replaced. All other reserves/useful services keep theirs.
	while (truck->GetNumOrders() != 0) PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
	Order pickup; pickup.MakeGoToStation(GetStationIndex(TileIndex{1896})); pickup.SetLoadType(OrderLoadType::LoadIfPossible); pickup.SetUnloadType(OrderUnloadType::NoUnload); pickup.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, pickup);
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, SurfaceChemicalHome());
	PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true);
}

/** Earn one separately funded actual fuel round, preserving partial or completed stored depot cargo. */
static void WorkSurfacePropellantPickup()
{
	const auto *truck = FindSurfacePropellantReserve(); REQUIRE(!truck->IsStoppedInDepot()); REQUIRE(truck->GetNumOrders() == 2);
	const auto limit = SurfaceChemicalWorkLimit(); REQUIRE(QuerySurfacePropellantPickup(limit));
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, SurfacePropellantRoundHome); REQUIRE(Company::Get(_current_company)->money >= 100000);
	// Fully stored26and zero reservations complete collection; a partial real
	// return is preserved rather than passed off as the missing native quantity.
	const bool collected = SurfacePropellantRoundHome() && truck->cargo.StoredCount() == 26 && truck->cargo.ReservedCount() == 0;
	fmt::print("SURFACE_RESUPPLY_FUEL_PROGRESS vehicle={} held={} stored={} reserved={} tile={} halted={} earned={} round_home={} collected={} source=6 launch_delivered=false\n",
		truck->index.base(), truck->cargo.TotalCount(), truck->cargo.StoredCount(), truck->cargo.ReservedCount(), truck->tile.base(), truck->IsStoppedInDepot(), work.ticks, SurfacePropellantRoundHome(), collected);
}

/** @return Actual generation-qualified original member0/center21hub, preserving all paid Space owners. */
static LaunchHubMembership SurfaceFuelLaunchHub()
{
	const auto membership = GetLaunchHubMembership(StationID{0}); REQUIRE(membership.has_value()); REQUIRE(!membership->joined);
	const auto *center = ResolveLaunchCenter(membership->center), *member = ResolveLaunchHubMember(membership->center, membership->member);
	REQUIRE(center != nullptr); REQUIRE(member != nullptr); REQUIRE(center->owner == _current_company); REQUIRE(center->index == StationID{21});
	REQUIRE(member == Station::Get(StationID{0})); REQUIRE(member->owner == _current_company); REQUIRE(GetStationIndex(TileIndex{2504}) == member->index);
	return *membership;
}

/** @param station Actual original native station. @return Its physical/reserved PROPpacket quantity, never a production counter. */
static uint SurfaceFuelStationStock(StationID station)
{
	return Station::Get(station)->goods[GetCargoTypeByLabel(CargoLabel{"PROP"})].TotalCount();
}

/** @return Conservative actual native mass-fee allowance for26PROP, without granting a stock-backed handling quote. */
static Money SurfaceFuelHandlingAllowance()
{
	return Money{static_cast<int64_t>((uint64_t{26} * CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"PROP"}))->weight + 15) / 16)};
}

/** @return Explicit no-load transfer into original member0; ordinary acceptance cannot substitute for launch supply. */
static Order SurfaceFuelMemberOrder()
{
	Order order; order.MakeGoToStation(StationID{0}); order.SetLoadType(OrderLoadType::NoLoad); order.SetUnloadType(OrderUnloadType::Transfer); order.SetStopLocation(OrderStopLocation::FarEnd); return order;
}

/** @return True only after26actual member stock, unchanged center12and fully empty returned original reserve2. */
static bool SurfaceFuelMemberReady()
{
	const auto *truck = FindSurfacePropellantReserve();
	// Real member inventory and depot custody jointly authorize the separate
	// reverse-transfer stage. A receiver order or empty truck alone does not.
	return SurfacePropellantRoundHome() && truck->cargo.TotalCount() == 0 && SurfaceFuelStationStock(StationID{0}) == 26 && SurfaceFuelStationStock(StationID{21}) == 12;
}

/** Quote complete delivery costs and disclose the still-conditional native handling stock gate.
 * @param limit Newly admitted native transport interval in1..3000ticks.
 * @return Whether current cash covers every charged day plus the26handling allowance above100000reserve.
 */
static bool QuerySurfaceFuelDelivery(uint32_t limit)
{
	const auto *truck = FindSurfacePropellantReserve(); const auto hub = SurfaceFuelLaunchHub();
	const bool funded = QuerySurfaceChemicalWork(limit, truck, TileIndex{2504}, OrderLoadType::NoLoad, SurfaceFuelHandlingAllowance());
	const auto remove = QuoteSurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
	const auto delivery = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, SurfaceFuelMemberOrder());
	const auto home = QuoteSurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, SurfaceChemicalHome());
	const auto start = QuoteSurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true);
	REQUIRE(remove.Succeeded()); REQUIRE(delivery.Succeeded()); REQUIRE(home.Succeeded()); REQUIRE(start.Succeeded());
	REQUIRE(remove.GetCost() == 0); REQUIRE(delivery.GetCost() == 0); REQUIRE(home.GetCost() == 0); REQUIRE(start.GetCost() == 0);
	const auto handling = QuoteSurfaceResupply<Commands::StartSurfaceHandling>(TileIndex{0}, hub.member.station, hub.member.generation,
		hub.center.station, hub.center.generation, truck->cargo_type, uint16_t{26}, true);
	fmt::print("SURFACE_RESUPPLY_HUB_ADMISSION vehicle=2 held={} member={} member_generation={} member_stock={} center={} center_generation={} center_stock={} receiver_tile=2504 handling_allowance={} handling_stock_admitted={} funded={} reserve=100000 refund_counted=0 credit_counted=0\n",
		truck->cargo.StoredCount(), hub.member.station.base(), hub.member.generation, SurfaceFuelStationStock(hub.member.station), hub.center.station.base(), hub.center.generation,
		SurfaceFuelStationStock(hub.center.station), SurfaceFuelHandlingAllowance().base(), handling.Succeeded(), funded);
	return funded;
}

/** Dispatch only the genuinely collected26unit reserve into original member0after fresh admission. */
static void OrderSurfaceFuelDelivery()
{
	const auto *truck = FindSurfacePropellantReserve(); REQUIRE(SurfacePropellantRoundHome()); REQUIRE(truck->cargo.StoredCount() == 26); REQUIRE(truck->cargo.ReservedCount() == 0);
	REQUIRE(SurfaceFuelStationStock(StationID{0}) == 0); REQUIRE(SurfaceFuelStationStock(StationID{21}) == 12);
	REQUIRE(QuerySurfaceFuelDelivery(SurfaceChemicalWorkLimit()));
	// Keep actual Source6packets in the same original body while replacing only
	// its pickup with a native transfer. Every other reserve/service stays owned.
	while (truck->GetNumOrders() != 0) PaySurfaceResupply<Commands::DeleteOrder>(TileIndex{0}, truck->index, VehicleOrderID{0});
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{0}, SurfaceFuelMemberOrder());
	PaySurfaceResupply<Commands::InsertOrder>(TileIndex{0}, truck->index, VehicleOrderID{1}, SurfaceChemicalHome());
	PaySurfaceResupply<Commands::StartStopVehicle>(TileIndex{0}, truck->index, true);
}

/** Earn only one newly admitted actual member-delivery interval, preserving incomplete original progress. */
static void WorkSurfaceFuelDelivery()
{
	const auto limit = SurfaceChemicalWorkLimit(); REQUIRE(QuerySurfaceFuelDelivery(limit));
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, SurfaceFuelMemberReady); const auto *truck = FindSurfacePropellantReserve();
	REQUIRE(Company::Get(_current_company)->money >= 100000);
	fmt::print("SURFACE_RESUPPLY_HUB_PROGRESS vehicle=2 held={} stored={} reserved={} tile={} halted={} earned={} member_stock={} center_stock={} member_ready={} handling_started=false\n",
		truck->cargo.TotalCount(), truck->cargo.StoredCount(), truck->cargo.ReservedCount(), truck->tile.base(), truck->IsStoppedInDepot(), work.ticks,
		SurfaceFuelStationStock(StationID{0}), SurfaceFuelStationStock(StationID{21}), SurfaceFuelMemberReady());
}

/** Quote only actual generation-bound member stock and separately bounded timed fees.
 * @param limit Fresh handling work cap in1..50ticks;26units require four actual native throughput ticks.
 * @return Whether real stock/command authority and present cash admit the reverse transfer.
 */
static bool QuerySurfaceFuelHandling(uint32_t limit)
{
	REQUIRE(limit > 0); REQUIRE(limit <= 50); REQUIRE(SurfaceFuelMemberReady()); const auto hub = SurfaceFuelLaunchHub();
	const auto *truck = FindSurfacePropellantReserve();
	const bool funded = QuerySurfaceChemicalWork(limit, truck, TileIndex{2504}, OrderLoadType::NoLoad, SurfaceFuelHandlingAllowance());
	const auto quote = QuoteSurfaceResupply<Commands::StartSurfaceHandling>(TileIndex{0}, hub.member.station, hub.member.generation,
		hub.center.station, hub.center.generation, truck->cargo_type, uint16_t{26}, true);
	REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == 0);
	fmt::print("SURFACE_RESUPPLY_HANDLING_ADMISSION quantity=26 member={} member_generation={} center={} center_generation={} member_stock=26 center_stock=12 command_cost=0 fee_bound={} ticks={} funded={} reserve=100000 stock_admitted=true reverse=true\n",
		hub.member.station.base(), hub.member.generation, hub.center.station.base(), hub.center.generation, SurfaceFuelHandlingAllowance().base(), limit, funded);
	return funded;
}

/** Start only the freshly quoted actual26unit reverse job; native job custody replaces member custody once. */
static void StartSurfaceFuelHandling()
{
	REQUIRE(QuerySurfaceFuelHandling(SurfaceChemicalWorkLimit())); const auto hub = SurfaceFuelLaunchHub();
	PaySurfaceResupply<Commands::StartSurfaceHandling>(TileIndex{0}, hub.member.station, hub.member.generation,
		hub.center.station, hub.center.generation, GetCargoTypeByLabel(CargoLabel{"PROP"}), uint16_t{26}, true);
	REQUIRE(SurfaceFuelStationStock(StationID{0}) == 0); REQUIRE(SurfaceFuelStationStock(StationID{21}) == 12);
}

/** @return True only when all38real Source6PROP is in original center21and member/job/road custody is empty. */
static bool SurfaceFuelStoredAtLaunch()
{
	const auto hub = SurfaceFuelLaunchHub(); const auto *center = ResolveLaunchCenter(hub.center); const auto *truck = FindSurfacePropellantReserve();
	// The original twelve plus physically delivered26must all reach actual
	// launch storage. Completed job counters or an empty member supply no fuel.
	return SurfacePropellantRoundHome() && truck->cargo.TotalCount() == 0 && SurfaceFuelStationStock(StationID{0}) == 0 &&
		SurfaceFuelStationStock(StationID{21}) == 38 && center->launch_hub->handling.empty();
}

/** Earn only the newly admitted actual handling interval, charging native throughput fees once. */
static void WorkSurfaceFuelHandling()
{
	const auto limit = SurfaceChemicalWorkLimit(); REQUIRE(limit > 0); REQUIRE(limit <= 50); const auto hub = SurfaceFuelLaunchHub();
	REQUIRE(!ResolveLaunchCenter(hub.center)->launch_hub->handling.empty());
	REQUIRE(QuerySurfaceChemicalWork(limit, FindSurfacePropellantReserve(), TileIndex{2504}, OrderLoadType::NoLoad, SurfaceFuelHandlingAllowance()));
	const auto work = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, SurfaceFuelStoredAtLaunch); REQUIRE(Company::Get(_current_company)->money >= 100000);
	fmt::print("SURFACE_RESUPPLY_HANDLING_PROGRESS earned={} member_stock={} center_stock={} stored={} fee_bound={} supplied_stock=false\n",
		work.ticks, SurfaceFuelStationStock(StationID{0}), SurfaceFuelStationStock(StationID{21}), SurfaceFuelStoredAtLaunch(), SurfaceFuelHandlingAllowance().base());
}

#include "surface_entry_module_capital.h"
#include "surface_entry_capital_income.h"
#include "surface_entry_capital_renewals.h"
#include "surface_entry_capital_recovery.h"
#include "surface_entry_freight_efficiency.h"
#include "surface_entry_income_capitalization.h"
#include "surface_entry_passenger_coverage.h"
#include "surface_entry_goods_income.h"
#include "surface_entry_goods_trade.h"
#include "surface_entry_productive_capital.h"
#include "surface_entry_farm_capacity.h"
#include "surface_entry_farm_upgrade.h"
#include "surface_entry_farm_fleet.h"
#include "surface_entry_farm_balance.h"
#include "surface_entry_farm_rebalance.h"
#include "surface_entry_farm_cycle.h"
#include "surface_entry_paid_recovery.h"
#include "surface_entry_intertown.h"
#include "surface_entry_road_shortcut.h"
#include "surface_entry_farm_road.h"
#include "surface_entry_farm_road_work.h"
#include "surface_entry_farm_feeder.h"
#include "surface_entry_farm_feeder_work.h"
#include "surface_entry_farm_feeder_rail.h"
#include "surface_entry_factory_goods.h"
#include "surface_entry_factory_goods_work.h"
#include "surface_entry_factory_goods_settlement.h"
#include "surface_entry_return_passenger.h"
#include "surface_entry_goods_rail_feeder.h"
#include "surface_entry_refill_capital.h"

/** Original current-world quotes, paid transport and bounded real work; no supplied material or position. */
TEST_CASE("Authentic chemical refill has a complete original finite transport and funding quote", "[.surface-entry-resupply]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_RESUPPLY_PHASE"); const std::string phase = selected == nullptr ? "query" : selected;
	const std::array<std::string_view, 40> allowed{"query", "admission", "current-admission", "staged-admission", "staged-convert", "staged-buy", "staged-work", "staged-return-query", "staged-return-work", "staged-delivery-query", "staged-delivery-order", "staged-delivery-work", "staged-empty-sale", "staged-remaining-query", "staged-chemical-renew", "staged-chemical-query", "staged-chemical-order", "staged-chemical-work", "staged-chemical-delivery-query", "staged-chemical-delivery-order", "staged-chemical-delivery-work", "staged-chemical-fuel-query", "staged-chemical-fuel-order", "staged-chemical-fuel-work", "staged-chemical-hub-query", "staged-chemical-hub-order", "staged-chemical-hub-work", "staged-chemical-handling-query", "staged-chemical-handling-start", "staged-chemical-handling-work", "recall", "traffic-work", "buy-salt", "salt-work", "chemical-order", "chemical-source-work", "salt-delivery-order", "refill-work", "settle", "earn"};
	if (phase.starts_with("staged-chemical-capital-")) {
		const std::array<std::string_view, 67> capital{"staged-chemical-capital-query", "staged-chemical-capital-buy", "staged-chemical-capital-work",
			"staged-chemical-capital-renewal-query", "staged-chemical-capital-renewal-recall", "staged-chemical-capital-renewal-work", "staged-chemical-capital-renewal-pay",
			"staged-chemical-capital-recovery-query", "staged-chemical-capital-recovery-work", "staged-chemical-capital-throughput-query",
			"staged-chemical-capital-throughput-change", "staged-chemical-capital-throughput-work", "staged-chemical-capital-income-query",
			"staged-chemical-capital-coverage-buy", "staged-chemical-capital-coverage-work", "staged-chemical-capital-goods-query",
			"staged-chemical-capital-trade-start", "staged-chemical-capital-trade-work", "staged-chemical-capital-productive-query", "staged-chemical-capital-farm-query",
			"staged-chemical-capital-farm-recall", "staged-chemical-capital-farm-return", "staged-chemical-capital-farm-pay", "staged-chemical-capital-farm-service", "staged-chemical-capital-farm-fleet-query", "staged-chemical-capital-farm-balance-query",
			"staged-chemical-capital-farm-rebalance-recall", "staged-chemical-capital-farm-rebalance-return", "staged-chemical-capital-farm-rebalance-park", "staged-chemical-capital-farm-rebalance-pay", "staged-chemical-capital-farm-rebalance-service",
			"staged-chemical-capital-farm-cycle-query", "staged-chemical-capital-farm-cycle-work",
			"staged-chemical-capital-paid-recovery-query", "staged-chemical-capital-paid-recovery-work", "staged-chemical-capital-paid-recovery-short-query",
			"staged-chemical-capital-intertown-query", "staged-chemical-capital-intertown-change", "staged-chemical-capital-intertown-work", "staged-chemical-capital-road-shortcut-query", "staged-chemical-capital-farm-road-query",
			"staged-chemical-capital-farm-road-stops", "staged-chemical-capital-farm-road-buy", "staged-chemical-capital-farm-road-work", "staged-chemical-capital-farm-feeder-query",
			"staged-chemical-capital-farm-feeder-road", "staged-chemical-capital-farm-feeder-stops", "staged-chemical-capital-farm-feeder-orders", "staged-chemical-capital-farm-feeder-work", "staged-chemical-capital-farm-feeder-follow", "staged-chemical-capital-farm-feeder-rail", "staged-chemical-capital-factory-goods-query", "staged-chemical-capital-factory-goods-mixed-query", "staged-chemical-capital-factory-goods-orders", "staged-chemical-capital-factory-goods-work", "staged-chemical-capital-factory-goods-settlement", "staged-chemical-capital-factory-goods-liquid-work", "staged-chemical-capital-return-passenger-query", "staged-chemical-capital-return-passenger-orders", "staged-chemical-capital-return-passenger-work", "staged-chemical-capital-return-passenger-delivery", "staged-chemical-capital-goods-rail-feeder-query", "staged-chemical-capital-goods-rail-feeder-infrastructure", "staged-chemical-capital-goods-rail-feeder-orders", "staged-chemical-capital-goods-rail-feeder-work", "staged-chemical-capital-refill-capital-query", "staged-chemical-capital-refill-capital-sell"};
		REQUIRE(std::ranges::find(capital, phase) != capital.end());
	} else {
		REQUIRE(std::ranges::find(allowed, phase) != allowed.end());
	}
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-resupply"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve native query failures through normal shutdown without compensating gameplay changes. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves / ("surface-resupply-before-" + phase + "-440.sav"));
			if (phase == "staged-chemical-capital-query") QuerySurfaceModuleCapital();
			else if (phase == "staged-chemical-capital-buy") BuySurfaceCapitalIncome();
			else if (phase == "staged-chemical-capital-work") WorkSurfaceCapitalIncome();
			else if (phase == "staged-chemical-capital-renewal-query") QuerySurfaceCapitalRenewalReturn();
			else if (phase == "staged-chemical-capital-renewal-recall") RecallSurfaceCapitalRenewals();
			else if (phase == "staged-chemical-capital-renewal-work") WorkSurfaceCapitalRenewals();
			else if (phase == "staged-chemical-capital-renewal-pay") PaySurfaceCapitalRenewals();
			else if (phase == "staged-chemical-capital-recovery-query") QuerySurfaceCapitalRecovery();
			else if (phase == "staged-chemical-capital-recovery-work") WorkSurfaceCapitalRecovery();
			else if (phase == "staged-chemical-capital-throughput-query") QuerySurfaceFreightEfficiency();
			else if (phase == "staged-chemical-capital-throughput-change") ApplySurfaceFreightEfficiency();
			else if (phase == "staged-chemical-capital-throughput-work") WorkSurfaceFreightEfficiency();
			else if (phase == "staged-chemical-capital-income-query") QuerySurfaceIncomeCapitalization();
			else if (phase == "staged-chemical-capital-coverage-buy") BuySurfacePassengerCoverage();
			else if (phase == "staged-chemical-capital-coverage-work") WorkSurfacePassengerCoverage();
			else if (phase == "staged-chemical-capital-goods-query") QuerySurfaceGoodsIncome();
			else if (phase == "staged-chemical-capital-trade-start") StartSurfaceGoodsTrade();
			else if (phase == "staged-chemical-capital-trade-work") WorkSurfaceGoodsTrade();
			else if (phase == "staged-chemical-capital-productive-query") QuerySurfaceProductiveCapital();
			else if (phase == "staged-chemical-capital-farm-query") QuerySurfaceFarmCapacity();
			else if (phase == "staged-chemical-capital-farm-recall") RecallSurfaceFarmUpgrade();
			else if (phase == "staged-chemical-capital-farm-return") WorkSurfaceFarmReturn();
			else if (phase == "staged-chemical-capital-farm-pay") PaySurfaceFarmUpgrade();
			else if (phase == "staged-chemical-capital-farm-service") WorkSurfaceFarmUpgrade();
			else if (phase == "staged-chemical-capital-farm-fleet-query") QuerySurfaceFarmFleet();
			else if (phase == "staged-chemical-capital-farm-balance-query") QuerySurfaceFarmBalance();
			else if (phase == "staged-chemical-capital-farm-rebalance-recall") RecallSurfaceFarmRebalance();
			else if (phase == "staged-chemical-capital-farm-rebalance-return") WorkSurfaceFarmRebalanceReturn();
			else if (phase == "staged-chemical-capital-farm-rebalance-park") ParkSurfaceFarmRebalance();
			else if (phase == "staged-chemical-capital-farm-rebalance-pay") PaySurfaceFarmRebalance();
			else if (phase == "staged-chemical-capital-farm-rebalance-service") WorkSurfaceFarmRebalance();
			else if (phase == "staged-chemical-capital-farm-cycle-query") QuerySurfaceFarmCycle();
			else if (phase == "staged-chemical-capital-farm-cycle-work") WorkSurfaceFarmCycle();
			else if (phase == "staged-chemical-capital-paid-recovery-query") QuerySurfacePaidRecovery();
			else if (phase == "staged-chemical-capital-paid-recovery-short-query") QuerySurfacePaidRecovery(3000);
			else if (phase == "staged-chemical-capital-paid-recovery-work") WorkSurfacePaidRecovery();
			else if (phase == "staged-chemical-capital-intertown-query") QuerySurfaceIntertown();
			else if (phase == "staged-chemical-capital-intertown-change") ChangeSurfaceIntertown();
			else if (phase == "staged-chemical-capital-intertown-work") WorkSurfaceIntertown();
			else if (phase == "staged-chemical-capital-road-shortcut-query") QuerySurfaceRoadShortcut();
			else if (phase == "staged-chemical-capital-farm-road-query") QuerySurfaceFarmRoad();
			else if (phase == "staged-chemical-capital-farm-road-stops") PaySurfaceFarmRoadStops();
			else if (phase == "staged-chemical-capital-farm-road-buy") BuySurfaceFarmRoadIncome();
			else if (phase == "staged-chemical-capital-farm-road-work") WorkSurfaceFarmRoadIncome();
			else if (phase == "staged-chemical-capital-farm-feeder-query") QuerySurfaceFarmFeeder();
			else if (phase == "staged-chemical-capital-farm-feeder-road") PaySurfaceFarmFeederRoad();
			else if (phase == "staged-chemical-capital-farm-feeder-stops") PaySurfaceFarmFeederStops();
			else if (phase == "staged-chemical-capital-farm-feeder-orders") OrderSurfaceFarmFeeder();
			else if (phase == "staged-chemical-capital-farm-feeder-work") WorkSurfaceFarmFeeder();
			else if (phase == "staged-chemical-capital-farm-feeder-follow") WorkSurfaceFarmFeeder(true);
			else if (phase == "staged-chemical-capital-farm-feeder-rail") WorkSurfaceFarmFeederRail();
			else if (phase == "staged-chemical-capital-factory-goods-query") QuerySurfaceFactoryGoods();
			else if (phase == "staged-chemical-capital-factory-goods-mixed-query") QuerySurfaceFactoryGoods(true);
			else if (phase == "staged-chemical-capital-factory-goods-orders") OrderSurfaceFactoryGoods();
			else if (phase == "staged-chemical-capital-factory-goods-work") WorkSurfaceFactoryGoods();
			else if (phase == "staged-chemical-capital-factory-goods-settlement") SettleSurfaceFactoryGoods();
			else if (phase == "staged-chemical-capital-factory-goods-liquid-work") WorkSurfaceFactoryGoodsLiquidity(SurfaceChemicalWorkLimit());
			else if (phase == "staged-chemical-capital-return-passenger-query") QuerySurfaceReturnPassenger();
			else if (phase == "staged-chemical-capital-return-passenger-orders") OrderSurfaceReturnPassenger();
			else if (phase == "staged-chemical-capital-return-passenger-work") WorkSurfaceFactoryGoodsLiquidity(SurfaceChemicalWorkLimit(), true, true);
			else if (phase == "staged-chemical-capital-return-passenger-delivery") WorkSurfaceFactoryGoodsLiquidity(SurfaceChemicalWorkLimit(), true, false, true);
			else if (phase == "staged-chemical-capital-goods-rail-feeder-query") QuerySurfaceGoodsRailFeeder();
			else if (phase == "staged-chemical-capital-goods-rail-feeder-infrastructure") PaySurfaceGoodsRailFeederInfrastructure();
			else if (phase == "staged-chemical-capital-goods-rail-feeder-orders") OrderSurfaceGoodsRailFeeder();
			else if (phase == "staged-chemical-capital-goods-rail-feeder-work") WorkSurfaceGoodsRailFeeder(SurfaceChemicalWorkLimit());
			else if (phase == "staged-chemical-capital-refill-capital-query") QuerySurfaceRefillCapital();
			else if (phase == "staged-chemical-capital-refill-capital-sell") SellSurfaceRefillCapital();
			else if (phase == "query") QuerySurfaceResupply();
			else if (phase == "admission") QuerySurfaceResupply(true);
			else if (phase == "current-admission") { QuerySurfaceResupply(true); QuerySurfaceResupplyAlternatives(); }
			else if (phase == "staged-admission") QuerySurfaceResupplyElectrification(true);
			else if (phase == "staged-convert") ConvertSurfaceResupplyRoute();
			else if (phase == "staged-buy") BuySurfaceElectricSalt();
			else if (phase == "staged-work") WorkSurfaceElectricSalt();
			else if (phase == "staged-return-query") {
				const auto *train = FindSurfaceElectricSalt(); REQUIRE(train != nullptr); REQUIRE(train->Next()->cargo.TotalCount() == 60);
				QuerySurfaceResupplyPeak(EngineID{24}, Money{0}, Money{0}, false, SurfaceElectricSaltReturnLimit());
			}
			else if (phase == "staged-return-work") {
				const auto *train = FindSurfaceElectricSalt(); REQUIRE(train != nullptr); REQUIRE(train->Next()->cargo.TotalCount() == 60);
				WorkSurfaceElectricSalt(SurfaceElectricSaltReturnLimit());
			}
			else if (phase == "staged-delivery-query") QuerySurfaceElectricSaltDelivery(SurfaceElectricSaltDeliveryLimit());
			else if (phase == "staged-delivery-order") OrderSurfaceElectricSaltDelivery();
			else if (phase == "staged-delivery-work") WorkSurfaceElectricSaltDelivery();
			else if (phase == "staged-empty-sale") SellSurfaceElectricSalt();
			else if (phase == "staged-remaining-query") QuerySurfaceRemainingFuel();
			else if (phase == "staged-chemical-renew") RenewSurfaceResupplyChemical();
			else if (phase == "staged-chemical-query") QuerySurfaceChemicalWork(SurfaceChemicalWorkLimit());
			else if (phase == "staged-chemical-order") OrderSurfaceRenewedChemicalPickup();
			else if (phase == "staged-chemical-work") WorkSurfaceRenewedChemicalPickup();
			else if (phase == "staged-chemical-delivery-query") QuerySurfaceChemicalDelivery(SurfaceChemicalWorkLimit());
			else if (phase == "staged-chemical-delivery-order") OrderSurfaceChemicalDelivery();
			else if (phase == "staged-chemical-delivery-work") WorkSurfaceChemicalDelivery();
			else if (phase == "staged-chemical-fuel-query") QuerySurfacePropellantPickup(SurfaceChemicalWorkLimit());
			else if (phase == "staged-chemical-fuel-order") OrderSurfacePropellantPickup();
			else if (phase == "staged-chemical-fuel-work") WorkSurfacePropellantPickup();
			else if (phase == "staged-chemical-hub-query") QuerySurfaceFuelDelivery(SurfaceChemicalWorkLimit());
			else if (phase == "staged-chemical-hub-order") OrderSurfaceFuelDelivery();
			else if (phase == "staged-chemical-hub-work") WorkSurfaceFuelDelivery();
			else if (phase == "staged-chemical-handling-query") QuerySurfaceFuelHandling(SurfaceChemicalWorkLimit());
			else if (phase == "staged-chemical-handling-start") StartSurfaceFuelHandling();
			else if (phase == "staged-chemical-handling-work") WorkSurfaceFuelHandling();
			else if (phase == "recall") RecallSurfaceResupplyTraffic();
			else if (phase == "traffic-work") EarnSurfaceEntryWork("SURFACE_RESUPPLY", 3000, SurfaceResupplyTrafficProtected);
			else if (phase == "buy-salt") BuySurfaceResupplySalt();
			else if (phase == "salt-work") { REQUIRE(SurfaceResupplyTrafficProtected()); EarnSurfaceEntryWork("SURFACE_RESUPPLY", 3000, SurfaceResupplySaltCollected); }
			else if (phase == "chemical-order") OrderSurfaceResupplyChemical();
			else if (phase == "chemical-source-work") { REQUIRE(SurfaceResupplyTrafficProtected()); REQUIRE(SurfaceResupplySaltCollected()); EarnSurfaceEntryWork("SURFACE_RESUPPLY", 3000, SurfaceResupplyChemicalAtSource); }
			else if (phase == "salt-delivery-order") OrderSurfaceResupplySaltDelivery();
			else if (phase == "refill-work") { REQUIRE(SurfaceResupplyTrafficProtected()); EarnSurfaceEntryWork("SURFACE_RESUPPLY", 3000, SurfaceResupplyCollected); }
			else if (phase == "settle") SettleSurfaceResupply();
			else {
				REQUIRE(FindSurfaceResupplySalt() == nullptr); REQUIRE(FindSurfaceResupplyChemical()->cargo.TotalCount() == 21);
				/** @return False; the declared actual3000tick interval owns replay completion. */
				EarnSurfaceEntryWork("SURFACE_RESUPPLY", 3000, [] { return false; });
			}
			const auto *chemical = FindSurfaceResupplyChemical(phase.starts_with("staged-chemical-")); const auto *salt_train = FindSurfaceResupplySalt();
			fmt::print("SURFACE_RESUPPLY_CUSTODY chemical_vehicle={} chemicals={} chemical_tile={} chemical_halted={} salt_train={} salt_held={} traffic_protected={}\n", chemical->index.base(), chemical->cargo.TotalCount(), chemical->tile.base(), chemical->IsStoppedInDepot(), salt_train != nullptr, salt_train == nullptr ? 0 : salt_train->Next()->cargo.TotalCount(), SurfaceResupplyTrafficProtected());
			SaveSurfaceResupply(paths.saves / ("surface-resupply-" + phase + "-440.sav"));
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) {
			failure = std::current_exception();
			// A failed new paid boundary must retain its actual world before
			// shutdown. Recovery can inspect partial invoices/progress rather
			// than repeat successful commands against the old input snapshot.
			if (phase.starts_with("staged-")) {
				try { SaveSurfaceResupply(paths.saves / ("surface-resupply-failed-" + phase + "-440.sav")); }
				catch (...) { fmt::print("SURFACE_RESUPPLY_FAILURE_SNAPSHOT unavailable=true phase={}\n", phase); }
			}
		}
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect); IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
