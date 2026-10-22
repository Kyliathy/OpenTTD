/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_resupply.cpp Original finite-Salt and chemical-refill admission, transport and settlement.
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
#include "../train_cmd.h"
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
#include "surface_entry_work.h"
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
	REQUIRE(result != nullptr); REQUIRE(result->owner == _current_company);
	REQUIRE(result->cargo_type == GetCargoTypeByLabel(CargoLabel{"CHEM"})); REQUIRE(result->cargo_cap == 21);
	return result;
}

/** Complete read-only admission for the real carrier, useful existing inputs and crossing-safe staged delivery. */
static void QuerySurfaceResupply()
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

/** Original current-world quotes, paid transport and bounded real work; no supplied material or position. */
TEST_CASE("Authentic chemical refill has a complete original finite transport and funding quote", "[.surface-entry-resupply]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_RESUPPLY_PHASE"); const std::string phase = selected == nullptr ? "query" : selected;
	const std::array<std::string_view, 11> allowed{"query", "recall", "traffic-work", "buy-salt", "salt-work", "chemical-order", "chemical-source-work", "salt-delivery-order", "refill-work", "settle", "earn"};
	REQUIRE(std::ranges::find(allowed, phase) != allowed.end());
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-resupply"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve native query failures through normal shutdown without compensating gameplay changes. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves / ("surface-resupply-before-" + phase + "-440.sav"));
			if (phase == "query") QuerySurfaceResupply();
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
			const auto *chemical = FindSurfaceResupplyChemical(); const auto *salt_train = FindSurfaceResupplySalt();
			fmt::print("SURFACE_RESUPPLY_CUSTODY chemical_vehicle={} chemicals={} chemical_tile={} chemical_halted={} salt_train={} salt_held={} traffic_protected={}\n", chemical->index.base(), chemical->cargo.TotalCount(), chemical->tile.base(), chemical->IsStoppedInDepot(), salt_train != nullptr, salt_train == nullptr ? 0 : salt_train->Next()->cargo.TotalCount(), SurfaceResupplyTrafficProtected());
			SaveSurfaceResupply(paths.saves / ("surface-resupply-" + phase + "-440.sav"));
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect); IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
