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
#include "surface_entry_work.h"
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

/** Freshly quote and execute one original whole native purchase or management action.
 * @tparam Tcmd Original command identity. @tparam Targs Native argument types.
 * @param tile Actual tiled action, or zero for coordinate-free management.
 * @param args Complete original arguments. @return Settled original command result.
 */
template <Commands Tcmd, typename... Targs>
static auto PaySurfacePropellant(TileIndex tile, Targs... args)
{
	auto quote = QuoteSurfacePropellant<Tcmd>(tile, args...);
	INFO("Propellant quote=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(quote).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(quote).Succeeded());
	auto paid = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(args...));
	REQUIRE(ExtractCommandCost(paid).Succeeded()); REQUIRE(ExtractCommandCost(paid).GetCost() == ExtractCommandCost(quote).GetCost());
	fmt::print("SURFACE_PROPELLANT_INVOICE command={} tile={} cost={}\n", GetCommandName(Tcmd), tile.base(), ExtractCommandCost(paid).GetCost().base());
	return paid;
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

/** @return Cheapest actually admitted useful empty original PROP carrier at the existing real road depot. */
static SurfacePropellantQuote QuoteSurfacePropellantCarrier()
{
	SurfacePropellantQuote result;
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
	return result;
}

/** @return Complete admitted genuine second-factory/transport plan, with native current matching credit. */
static SurfacePropellantQuote QuerySurfacePropellant()
{
	const auto *company = Company::Get(_current_company); const auto *feed = FindSurfacePropellantFeed();
	const auto type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}); REQUIRE(type != IT_INVALID);
	auto factory = QuoteSurfacePropellant<Commands::BuildIndustry>(surface_propellant_site, surface_propellant_site, type, uint32_t{0}, true, uint32_t{123});
	REQUIRE(factory.Succeeded()); auto result = QuoteSurfacePropellantCarrier(); result.factory = factory.GetCost();
	const auto *first = Industry::GetByTile(TileIndex{1566}); REQUIRE(first != nullptr); REQUIRE(first->index == IndustryID{15}); REQUIRE(first->type == type);
	// The existing controlled facility proves this actual definition's recipe
	// availability without changing it. The new actual industry must separately
	// admit its own recipe command after original funding creates its owner.
	auto choice = QuoteSurfacePropellant<Commands::SelectLayerIndustryRecipe>(TileIndex{0}, first->index, uint32_t{0x5043484D}); REQUIRE(choice.Succeeded());
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

/** Fund only the admitted real incomplete second factory and its original selectable PCHM recipe. */
static void FundSurfacePropellant()
{
	const auto quote = QuerySurfacePropellant(); const auto *company = Company::Get(_current_company);
	const auto cash = company->money, debt = company->current_loan;
	PaySurfacePropellant<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, quote.credit);
	REQUIRE(company->money == cash + quote.credit); REQUIRE(company->current_loan == debt + quote.credit);
	const auto paid = PaySurfacePropellant<Commands::BuildIndustry>(surface_propellant_site, surface_propellant_site,
		GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}), uint32_t{0}, true, uint32_t{123});
	REQUIRE(paid.GetCost() == quote.factory);
	const auto *industry = Industry::GetByTile(surface_propellant_site); REQUIRE(industry != nullptr);
	REQUIRE(industry->founder == _current_company); REQUIRE(industry->index != IndustryID{15}); REQUIRE_FALSE(IsLayerIndustryComplete(*industry));
	uint cells = 0;
	// Every actual funded machinery tile must begin incomplete. Neither a
	// side-record flag nor a test work counter may commission this producer.
	for (const auto tile : industry->location) {
		if (!industry->TileBelongsToIndustry(tile)) continue;
		++cells; REQUIRE_FALSE(IsIndustryCompleted(tile)); REQUIRE(GetIndustryConstructionStage(tile) == 0); REQUIRE(GetIndustryConstructionCounter(tile) == 0);
	}
	REQUIRE(cells == 16);
	PaySurfacePropellant<Commands::SelectLayerIndustryRecipe>(TileIndex{0}, industry->index, uint32_t{0x5043484D});
	const auto *state = GetLayerProcessorState(industry->index); REQUIRE(state != nullptr); REQUIRE(state->recipe == MultiLayerRecipeID{"PCHM"});
	REQUIRE(state->work_ticks == 0); REQUIRE(state->dispatched == 0);
	for (const auto &slot : industry->accepted) REQUIRE(slot.waiting == 0);
	for (const auto &slot : industry->produced) REQUIRE(slot.waiting == 0);
	REQUIRE(company->money == cash + quote.credit - paid.GetCost()); REQUIRE(company->current_loan == debt + quote.credit);
	fmt::print("SURFACE_PROPELLANT_FUNDED industry={} site={} cells={} credit={} factory={} cash_before={} debt_before={} cash={} debt={} complete=false empty=true supplied_work=false\n",
		industry->index.base(), surface_propellant_site.base(), cells, quote.credit.base(), paid.GetCost().base(), cash.base(), debt.base(), company->money.base(), company->current_loan.base());
}

/** Earn one bounded real construction burst; unfinished actual progress remains available for the next burst. */
static void BuildSurfacePropellant()
{
	const auto *industry = Industry::GetByTile(surface_propellant_site); REQUIRE(industry != nullptr);
	const auto *state = GetLayerProcessorState(industry->index); REQUIRE(state != nullptr); REQUIRE(state->recipe == MultiLayerRecipeID{"PCHM"});
	/** @return Actual native whole-machinery readiness, without changing construction or work counters. */
	const auto ready = [industry] { return IsLayerIndustryComplete(*industry); };
	EarnSurfaceEntryWork("SURFACE_PROPELLANT", 3000, ready);
	fmt::print("SURFACE_PROPELLANT_READINESS industry={} complete={}\n", industry->index.base(), IsLayerIndustryComplete(*industry));
}

/** @param name Actual original road role. @return Sole original owned truck with that native name. */
static const RoadVehicle *FindSurfacePropellantTruck(std::string_view name)
{
	const RoadVehicle *result = nullptr;
	for (const auto *truck : RoadVehicle::Iterate()) {
		if (truck->name != name) continue;
		REQUIRE(result == nullptr); REQUIRE(truck->owner == _current_company); result = truck;
	}
	REQUIRE(result != nullptr); return result;
}

/** @return All three original named freight fronts and their actual paid terminal depots. */
static std::vector<std::pair<const Train *, TileIndex>> SurfacePropellantTraffic()
{
	const std::map<std::string, TileIndex> depots{{"Surface Income farm", TileIndex{1638}}, {"Surface Income goods", TileIndex{3224}}, {"Surface Income coal", TileIndex{3161}}};
	std::vector<std::pair<const Train *, TileIndex>> result;
	// Identify original real routes by name; neither historical pool ordering
	// nor a nearby locomotive can substitute for a missing crossing owner.
	for (const auto *train : Train::Iterate()) {
		if (!train->IsFrontEngine()) continue;
		const auto found = depots.find(train->name); if (found == depots.end()) continue;
		REQUIRE(train->owner == _current_company); result.emplace_back(train, found->second);
	}
	REQUIRE(result.size() == 3); return result;
}

/** @return Whether all real commercial crossing owners are genuinely halted in their original depots. */
static bool SurfacePropellantTrafficHalted()
{
	for (const auto &[train, depot] : SurfacePropellantTraffic()) {
		// Every crossing owner must have completed its own native arrival.
		// An early arrival cannot authorize material movement past another train.
		if (!train->IsStoppedInDepot() || train->tile != depot) return false;
	}
	return true;
}

/** Append a normal halt after each train's original pickup/delivery cycle, without forcing a turn or pose. */
static void OrderSurfacePropellantTraffic()
{
	for (const auto &[train, tile] : SurfacePropellantTraffic()) {
		REQUIRE(train->GetNumOrders() == 2);
		Order home; home.MakeGoToDepot(Depot::GetByTile(tile)->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
		PaySurfacePropellant<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{2}, home);
		fmt::print("SURFACE_PROPELLANT_PARK_ORDER head={} depot={} appended=2 original_orders=2\n", train->index.base(), tile.base());
	}
}

/** Earn only actual traffic settlement before releasing the irreplaceable chemical stock onto its crossings. */
static void WorkSurfacePropellantTraffic()
{
	EarnSurfaceEntryWork("SURFACE_PROPELLANT", 3000, SurfacePropellantTrafficHalted);
	for (const auto &[train, depot] : SurfacePropellantTraffic()) fmt::print("SURFACE_PROPELLANT_PARK_STATE head={} tile={} depot={} halted={}\n",
		train->index.base(), train->tile.base(), depot.base(), train->IsStoppedInDepot());
}

/** Release a real full-load wait through its existing receiver order after the feeder has already parked. */
static void DrainSurfacePropellantTraffic()
{
	uint released = 0;
	// A parked upstream feeder cannot fill the remaining downstream wagon
	// space. Preserve the actual partial load and both original commercial
	// orders; the player's normal next-order command sends it to paid delivery.
	for (const auto &[train, depot] : SurfacePropellantTraffic()) {
		if (train->IsStoppedInDepot()) continue;
		REQUIRE(train->GetNumOrders() == 3); REQUIRE(train->cur_implicit_order_index == VehicleOrderID{0});
		REQUIRE(train->current_order.IsType(OT_LOADING));
		PaySurfacePropellant<Commands::SkipToOrder>(TileIndex{0}, train->index, VehicleOrderID{1}); ++released;
		fmt::print("SURFACE_PROPELLANT_DRAINED head={} receiver_order=1 depot={} original_orders_preserved=true\n", train->index.base(), depot.base());
	}
	REQUIRE(released > 0);
}

/** @return Current native CHEM waiting amount in the actual second PCHM industry. */
static uint SurfacePropellantChemicalWaiting()
{
	const auto *industry = Industry::GetByTile(surface_propellant_site); REQUIRE(industry != nullptr);
	const auto cargo = GetCargoTypeByLabel(CargoLabel{"CHEM"});
	const auto slot = std::ranges::find(industry->accepted, cargo, &Industry::AcceptedCargo::cargo);
	REQUIRE(slot != industry->accepted.end()); return slot->waiting;
}

/** @param truck Actual owned original road carrier. @param pickup True for normal non-full PROP collection, false for external CHEM delivery. */
static void OrderSurfacePropellantTruck(const RoadVehicle &truck, bool pickup)
{
	REQUIRE(SurfacePropellantTrafficHalted()); REQUIRE(truck.IsStoppedInDepot()); REQUIRE(truck.tile == surface_propellant_depot);
	// The two real stops serve distinct transfer directions. Original orders
	// alone unload external CHEM or collect already produced PROP; neither a
	// handoff helper nor a full-load wait can manufacture the missing seven units.
	while (truck.GetNumOrders() != 0) PaySurfacePropellant<Commands::DeleteOrder>(TileIndex{0}, truck.index, VehicleOrderID{0});
	Order serve; serve.MakeGoToStation(GetStationIndex(pickup ? TileIndex{1896} : TileIndex{1895})); serve.SetStopLocation(OrderStopLocation::FarEnd);
	serve.SetLoadType(pickup ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad);
	if (pickup) serve.SetUnloadType(OrderUnloadType::NoUnload);
	PaySurfacePropellant<Commands::InsertOrder>(TileIndex{0}, truck.index, VehicleOrderID{0}, serve);
	Order home; home.MakeGoToDepot(Depot::GetByTile(surface_propellant_depot)->index, OrderDepotTypeFlag::PartOfOrders, OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
	PaySurfacePropellant<Commands::InsertOrder>(TileIndex{0}, truck.index, VehicleOrderID{1}, home);
	PaySurfacePropellant<Commands::StartStopVehicle>(TileIndex{0}, truck.index, true);
	fmt::print("SURFACE_PROPELLANT_ROAD_ORDER vehicle={} pickup={} station={} starting_held={}\n", truck.index.base(), pickup, serve.GetDestination().base(), truck.cargo.TotalCount());
}

/** @return Actual empty returned chemical carrier and exact21external units received/consumed by the new factory. */
static bool SurfacePropellantChemicalsDelivered()
{
	const auto *truck = FindSurfacePropellantTruck("Surface Chemical reserve");
	const auto *industry = Industry::GetByTile(surface_propellant_site); const auto *state = GetLayerProcessorState(industry->index);
	// Delivery is complete only after the real truck has unloaded and returned;
	// processor consumption cannot hide a missing original external input.
	return truck->IsStoppedInDepot() && truck->tile == surface_propellant_depot && truck->cargo.TotalCount() == 0 &&
		SurfacePropellantChemicalWaiting() + state->consumed.at(CargoLabel{"CHEM"}) == 21;
}

/** Earn real road movement or processing without releasing the original stopped crossing protection.
 * @param processing Wait for native20PROP output rather than the original chemical carrier's actual empty return.
 */
static void WorkSurfacePropellantChemicals(bool processing)
{
	REQUIRE(SurfacePropellantTrafficHalted());
	/** @return Actual native recipe output, never a supplied batch or private work increment. */
	const auto produced = [] { return GetLayerProcessorState(Industry::GetByTile(surface_propellant_site)->index)->produced.at(CargoLabel{"PROP"}) == 20; };
	EarnSurfaceEntryWork("SURFACE_PROPELLANT", 3000, processing ? std::function<bool()>{produced} : std::function<bool()>{SurfacePropellantChemicalsDelivered});
	REQUIRE(SurfacePropellantTrafficHalted());
}

/** Buy only the fresh admitted empty carrier after actual useful product exists. */
static void BuySurfacePropellantCarrier()
{
	REQUIRE(SurfacePropellantTrafficHalted());
	const auto *industry = Industry::GetByTile(surface_propellant_site); const auto *state = GetLayerProcessorState(industry->index);
	REQUIRE(state->produced.at(CargoLabel{"PROP"}) == 20); REQUIRE(state->consumed.at(CargoLabel{"CHEM"}) == 20);
	const auto quote = QuoteSurfacePropellantCarrier(); const auto cash = Company::Get(_current_company)->money;
	auto paid = PaySurfacePropellant<Commands::BuildVehicle>(surface_propellant_depot, surface_propellant_depot, quote.engine, false, GetCargoTypeByLabel(CargoLabel{"PROP"}), ClientID::Server);
	const auto *truck = RoadVehicle::Get(std::get<1>(paid)); REQUIRE(truck->cargo.TotalCount() == 0); REQUIRE(truck->IsStoppedInDepot());
	REQUIRE(truck->cargo_cap == quote.capacity); REQUIRE(std::get<0>(paid).GetCost() == quote.vehicle);
	PaySurfacePropellant<Commands::RenameVehicle>(TileIndex{0}, truck->index, std::string{"Surface Propellant reserve"});
	REQUIRE(Company::Get(_current_company)->money == cash - quote.vehicle);
	fmt::print("SURFACE_PROPELLANT_CARRIER vehicle={} engine={} invoice={} capacity={} empty=true\n", truck->index.base(), quote.engine.base(), quote.vehicle.base(), truck->cargo_cap);
}

/** @return Whether exactly20genuine Source6PROP units have reached actual halted depot custody. */
static bool SurfacePropellantCollected()
{
	const auto *truck = FindSurfacePropellantTruck("Surface Propellant reserve");
	if (!truck->IsStoppedInDepot()) return false;
	if (truck->tile != surface_propellant_depot) return false;
	if (truck->cargo.TotalCount() != 20) return false;
	const auto *industry = Industry::GetByTile(surface_propellant_site);
	for (const auto *packet : *truck->cargo.Packets()) {
		REQUIRE(packet->GetSource().type == SourceType::Industry); REQUIRE(packet->GetSource().id == industry->index.base());
	}
	return true;
}

/** Earn only real collection/travel; a premature partial return remains a resumable ordinary pickup. */
static void WorkSurfacePropellantCollection()
{
	REQUIRE(SurfacePropellantTrafficHalted());
	EarnSurfaceEntryWork("SURFACE_PROPELLANT", 3000, SurfacePropellantCollected);
	REQUIRE(SurfacePropellantTrafficHalted());
}

/** Remove only the temporary depot orders and normally resume all original commercial freight after safe fuel custody. */
static void ResumeSurfacePropellantTraffic()
{
	REQUIRE(SurfacePropellantCollected()); REQUIRE(SurfacePropellantTrafficHalted());
	for (const auto &[train, depot] : SurfacePropellantTraffic()) {
		REQUIRE(train->GetNumOrders() == 3); REQUIRE(train->tile == depot);
		PaySurfacePropellant<Commands::DeleteOrder>(TileIndex{0}, train->index, VehicleOrderID{2});
		REQUIRE(train->GetNumOrders() == 2);
		PaySurfacePropellant<Commands::StartStopVehicle>(TileIndex{0}, train->index, true);
		fmt::print("SURFACE_PROPELLANT_RESUMED head={} original_orders=2\n", train->index.base());
	}
}

/** Observe the actual second industry's sole physical stocks and lifetime committed history. */
static void ObserveSurfacePropellantState()
{
	const auto *industry = Industry::GetByTile(surface_propellant_site); const auto *state = GetLayerProcessorState(industry->index);
	const auto prop = GetCargoTypeByLabel(CargoLabel{"PROP"}); const auto output = std::ranges::find(industry->produced, prop, &Industry::ProducedCargo::cargo);
	REQUIRE(output != industry->produced.end());
	fmt::print("SURFACE_PROPELLANT_STATE industry={} complete={} chemicals={} consumed={} produced={} buffered={} dispatched={} batches={}\n",
		industry->index.base(), IsLayerIndustryComplete(*industry), SurfacePropellantChemicalWaiting(), state->consumed.at(CargoLabel{"CHEM"}),
		state->produced.at(CargoLabel{"PROP"}), output->waiting, state->dispatched, state->batches.at(MultiLayerRecipeID{"PCHM"}));
}

/** Observe only real native contracts in the exact selected funded world. */
TEST_CASE("Authentic Surface Propellant uses real external chemicals and original commands", "[.surface-entry-propellant]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *mode = std::getenv("OTTD_SURFACE_PROPELLANT_PHASE"); const std::string phase = mode == nullptr ? "query" : mode;
	// Pure quote, original paid funding and earned construction are separate
	// native boundaries; none can silently supply the next producer's progress.
	REQUIRE((phase == "query" || phase == "fund" || phase == "build" || phase == "traffic-order" || phase == "traffic-work" || phase == "traffic-drain" ||
		phase == "chemical-order" || phase == "chemical-work" || phase == "process" || phase == "carrier" ||
		phase == "collect-order" || phase == "collect-work" || phase == "resume"));
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-propellant"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Retain native failures through ordinary shutdown without compensating grants. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfacePropellant(paths.saves / ("surface-propellant-before-" + phase + "-440.sav"));
			if (phase == "query") QuerySurfacePropellant();
			else if (phase == "fund") FundSurfacePropellant();
			else if (phase == "build") BuildSurfacePropellant();
			else if (phase == "traffic-order") OrderSurfacePropellantTraffic();
			else if (phase == "traffic-work") WorkSurfacePropellantTraffic();
			else if (phase == "traffic-drain") DrainSurfacePropellantTraffic();
			else if (phase == "chemical-order") OrderSurfacePropellantTruck(*FindSurfacePropellantFeed(), false);
			else if (phase == "chemical-work") WorkSurfacePropellantChemicals(false);
			else if (phase == "process") WorkSurfacePropellantChemicals(true);
			else if (phase == "carrier") BuySurfacePropellantCarrier();
			else if (phase == "collect-order") OrderSurfacePropellantTruck(*FindSurfacePropellantTruck("Surface Propellant reserve"), true);
			else if (phase == "collect-work") WorkSurfacePropellantCollection();
			else ResumeSurfacePropellantTraffic();
			if (phase != "query") ObserveSurfacePropellantState();
			SaveSurfacePropellant(paths.saves / ("surface-propellant-" + phase + "-440.sav"));
			fmt::print("SURFACE_PROPELLANT_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_propellant_observer, &inspect); IConsole::CmdRegister("surface_entry_propellant", ObserveSurfacePropellant);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
