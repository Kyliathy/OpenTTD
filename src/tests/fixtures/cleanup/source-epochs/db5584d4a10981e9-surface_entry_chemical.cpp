/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_chemical.cpp Genuine chemical-chain placement, capital and original paid construction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BH.1-2.
 * Original commands own every invoice, sale, bank loan and new industry/tile.
 * Only later native ticks can earn machinery readiness or physical cargo.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../misc_cmd.h"
#include "../industry_cmd.h"
#include "../industry_map.h"
#include "../industry.h"
#include "../industrytype.h"
#include "../layer_industry_processor.h"
#include "../multilayer_recipe.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../articulated_vehicles.h"
#include "../underground_vehicle.h"
#include "../vehicle_cmd.h"
#include "../vehicle_base.h"
#include "../train.h"
#include "../roadveh.h"
#include "../depot_base.h"
#include "../order_cmd.h"
#include "../station_base.h"
#include "../town.h"
#include "../cargopacket.h"
#include "../tile_map.h"
#include "../openttd.h"
#include "../timer/timer_game_tick.h"
#include "../core/backup_type.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "surface_chemical_layout.h"
#include <thread>
#include "../safeguards.h"

static std::function<void()> *surface_chemical_observer = nullptr; ///< Invocation callback, never a material or finance authority.

/** Dispatch the loaded genuine world's current chemical-chain observer.
 * @param args Parameter-free native console arguments.
 * @return True after the invocation's observer ran.
 */
static bool ObserveSurfaceChemical(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_chemical_observer != nullptr); (*surface_chemical_observer)(); return true;
}

/** Save the actual paused world without earning native work or moving freight.
 * @param path Fresh invocation-owned output, never the original predecessor.
 */
static void SaveSurfaceChemical(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Query one original command inside its native estimate/nesting scope.
 * @tparam Tcmd Original native command identity.
 * @tparam Targs Complete original command argument types.
 * @param tile Actual action location, or zero for coordinate-free actions.
 * @param args Complete native arguments, including tiled command locations.
 * @return Unchanged original successful quote or original refusal.
 */
template <Commands Tcmd, typename... Targs>
static auto QuoteSurfaceChemical(TileIndex tile, Targs... args)
{
	return Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, true, tile, std::make_tuple(args...));
}

/** Execute one original synchronized command with its ordinary invoice settlement.
 * @tparam Tcmd Original native command identity.
 * @tparam Targs Complete original argument types.
 * @param tile Actual action location, or zero for coordinate-free commands.
 * @param args Complete original arguments, including tiled command locations.
 * @return Actual native command result; original refusal remains a failure.
 */
template <Commands Tcmd, typename... Targs>
static auto PaySurfaceChemical(TileIndex tile, Targs... args)
{
	auto result = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(args...));
	INFO("chemical command=" << GetCommandName(Tcmd) << " tile=" << tile.base() << " error=" << ExtractCommandCost(result).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(result).Succeeded());
	fmt::print("SURFACE_CHEMICAL_INVOICE command={} tile={} cost={}\n", GetCommandName(Tcmd), tile.base(), ExtractCommandCost(result).GetCost().base());
	return result;
}

/** Recover only the actually retired empty cutter and borrow the minimum matching initial capital. */
static void RecoverSurfaceChemicalCapital()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == 88274); REQUIRE(company->current_loan == 4250000);
	SurfaceChemicalLayout layout; layout.Prepare();
	const auto type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}); REQUIRE(type != IT_INVALID);
	auto factory = QuoteSurfaceChemical<Commands::BuildIndustry>(TileIndex{1566}, TileIndex{1566}, type, uint32_t{0}, true, uint32_t{123}); REQUIRE(factory.Succeeded());
	const TileIndex depot{2443};
	auto oil = QuoteSurfaceChemical<Commands::BuildVehicle>(depot, depot, EngineID{132}, false, GetCargoTypeByLabel(CargoLabel{"OIL_"}), ClientID::Server);
	auto chemical = QuoteSurfaceChemical<Commands::BuildVehicle>(depot, depot, EngineID{132}, false, GetCargoTypeByLabel(CargoLabel{"CHEM"}), ClientID::Server);
	REQUIRE(std::get<0>(oil).Succeeded()); REQUIRE(std::get<0>(chemical).Succeeded());
	const auto capital = factory.GetCost() + layout.invoice + 2 * std::get<0>(oil).GetCost() + std::get<0>(chemical).GetCost(); CHECK(capital == 738253);
	const auto quote = QuoteSurfaceChemical<Commands::SellVehicle>(TileIndex{0}, VehicleID{7}, true, false, ClientID::Server); REQUIRE(quote.Succeeded());
	// Every part really belongs to the retired cutter. Empty-sale admission
	// cannot authorize destroying the separately held useful60Salt packets.
	for (const auto *vehicle = Train::Get(VehicleID{7}); vehicle != nullptr; vehicle = vehicle->Next()) CHECK(vehicle->cargo.TotalCount() == 0);
	const auto cash = company->money; const auto debt = company->current_loan;
	const auto paid = PaySurfaceChemical<Commands::SellVehicle>(TileIndex{0}, VehicleID{7}, true, false, ClientID::Server); CHECK(paid.GetCost() == quote.GetCost());
	CHECK(paid.GetCost() == -350868); CHECK(company->money == cash - paid.GetCost());
	const Money buffer{100000}; const auto shortfall = capital + buffer - company->money; REQUIRE(shortfall > 0);
	const Money amount{((shortfall.base() + 9999) / 10000) * 10000}; CHECK(amount == 400000);
	REQUIRE(debt + amount <= company->GetMaxLoan());
	CHECK(company->money + amount - 10000 < capital + buffer);
	PaySurfaceChemical<Commands::IncreaseLoan>(TileIndex{0}, LoanCommand::Amount, amount);
	CHECK(company->money == cash - paid.GetCost() + amount); CHECK(company->current_loan == debt + amount);
	fmt::print("SURFACE_CHEMICAL_CAPITAL_RECOVERED sale={} credit={} cash={} debt={} required={} buffer={}\n", -paid.GetCost().base(), amount.base(),
		company->money.base(), company->current_loan.base(), capital.base(), buffer.base());
}

/** Fund the real incomplete chemical works and all selected native road/bay infrastructure. */
static void FundSurfaceChemicalWorks()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == 839142); REQUIRE(company->current_loan == 4650000);
	const auto cash = company->money;
	SurfaceChemicalLayout layout; layout.Prepare();
	const auto type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}); REQUIRE(type != IT_INVALID);
	const TileIndex anchor{1566};
	auto quote = QuoteSurfaceChemical<Commands::BuildIndustry>(anchor, anchor, type, uint32_t{0}, true, uint32_t{123}); REQUIRE(quote.Succeeded());
	const auto paid = PaySurfaceChemical<Commands::BuildIndustry>(anchor, anchor, type, uint32_t{0}, true, uint32_t{123}); CHECK(paid.GetCost() == quote.GetCost());
	const auto *industry = Industry::GetByTile(anchor); REQUIRE(industry != nullptr); CHECK(industry->index == IndustryID{15});
	CHECK(industry->founder == _current_company); CHECK_FALSE(IsLayerIndustryComplete(*industry));
	// Actual command-created cells alone establish the incomplete producer.
	// A copied overall completion flag cannot stand in for sixteen machinery cells.
	size_t cells = 0;
	for (TileIndex tile : industry->location) {
		if (!industry->TileBelongsToIndustry(tile)) continue;
		++cells; CHECK_FALSE(IsIndustryCompleted(tile)); CHECK(GetIndustryConstructionStage(tile) == 0); CHECK(GetIndustryConstructionCounter(tile) == 0);
	}
	CHECK(cells == 16);
	const auto *state = GetLayerProcessorState(industry->index); REQUIRE(state != nullptr); CHECK(state->recipe == MultiLayerRecipeID{"CHEM"});
	CHECK(state->work_ticks == 0); CHECK(state->dispatched == 0);
	for (const auto &cargo : industry->accepted) CHECK(cargo.waiting == 0);
	for (const auto &cargo : industry->produced) CHECK(cargo.waiting == 0);
	Money infrastructure{0};
	// Each selected road union is freshly queried against the actual preceding
	// purchases. Existing complete road bits are retained without another bill.
	for (const auto &[tile, bits] : layout.roads) {
		if (IsNormalRoadTile(tile)) {
			if ((GetRoadBits(tile, RoadTramType::Road) | bits) == GetRoadBits(tile, RoadTramType::Road)) continue;
		}
		auto road = QuoteSurfaceChemical<Commands::BuildRoad>(tile, tile, bits, ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid()); REQUIRE(road.Succeeded());
		const auto built = PaySurfaceChemical<Commands::BuildRoad>(tile, tile, bits, ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid());
		CHECK(built.GetCost() == road.GetCost()); infrastructure += built.GetCost();
	}
	// Separate real station objects retain Oil intake and chemical pickup
	// capacity; no station ID or cargo inventory is assigned by this observer.
	for (size_t index = 0; index < layout.bays.size(); ++index) {
		const auto &bay = layout.bays[index]; const auto cost = SurfaceChemicalLayout::QuoteBay(bay.tile, bay.direction); REQUIRE(cost.Succeeded());
		const auto built = PaySurfaceChemical<Commands::BuildRoadStop>(bay.tile, bay.tile, uint8_t{1}, uint8_t{1}, RoadStopType::Truck,
			false, bay.direction, ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, NEW_STATION, true);
		CHECK(built.GetCost() == cost.GetCost()); infrastructure += built.GetCost();
		const auto *station = Station::Get(GetStationIndex(bay.tile)); CHECK(station->owner == _current_company);
		bool serves = false;
		if (index <= 2) {
			const auto *receiver = index == 0 ? Industry::Get(IndustryID{8}) : industry;
			for (TileIndex tile : receiver->location) {
				if (!receiver->TileBelongsToIndustry(tile)) continue;
				if (station->TileIsInCatchment(tile)) serves = true;
			}
			CHECK(serves);
		}
		fmt::print("SURFACE_CHEMICAL_BAY_PAID role={} tile={} station={} current_catchment={}\n", index, bay.tile.base(), station->index.base(), serves);
	}
	// Verify the complete bought network, including retained original roads;
	// successful isolated tile invoices alone do not prove physical continuity.
	for (const auto &[tile, bits] : layout.roads) {
		CHECK(IsNormalRoadTile(tile)); CHECK((GetRoadBits(tile, RoadTramType::Road) | bits) == GetRoadBits(tile, RoadTramType::Road));
	}
	CHECK(infrastructure == layout.invoice); CHECK(company->money == cash - paid.GetCost() - infrastructure); CHECK(company->current_loan == 4650000);
	fmt::print("SURFACE_CHEMICAL_WORKS_FUNDED industry={} anchor={} machinery={} infrastructure={} cash={} debt={} complete=false empty=true\n",
		industry->index.base(), anchor.base(), paid.GetCost().base(), infrastructure.base(), company->money.base(), company->current_loan.base());
}

/** @param train Actual owned Surface train. Replace its old orders with a real halt at the original rail depot. */
static void OrderSurfaceChemicalDepot(Train &train)
{
	const TileIndex depot{1373};
	// The retired work route must release the shared loading platform. Only
	// ordinary order commands may redirect the actual existing paid hauler.
	while (train.GetNumOrders() != 0) PaySurfaceChemical<Commands::DeleteOrder>(TileIndex{0}, train.index, VehicleOrderID{0});
	Order home; home.MakeGoToDepot(Depot::GetByTile(depot)->index, OrderDepotTypeFlag::PartOfOrders,
		OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
	PaySurfaceChemical<Commands::InsertOrder>(TileIndex{0}, train.index, VehicleOrderID{0}, home);
	if (train.vehstatus.Test(VehState::Stopped)) PaySurfaceChemical<Commands::StartStopVehicle>(TileIndex{0}, train.index, true);
}

/** Recall the empty retired hauler before dispatching the loaded Salt train onto its shared track. */
static void RecallSurfaceChemicalHauler()
{
	REQUIRE(IsLayerIndustryComplete(*Industry::Get(IndustryID{15})));
	auto *train = Train::Get(VehicleID{13}); REQUIRE(train->owner == _current_company);
	for (const auto *part = train; part != nullptr; part = part->Next()) REQUIRE(part->cargo.TotalCount() == 0);
	OrderSurfaceChemicalDepot(*train);
	fmt::print("SURFACE_CHEMICAL_HAULER_RECALLED vehicle=13 depot=1373 supplied_pose=false\n");
}

/** Earn real movement until the recalled empty hauler is legally stopped in its actual depot, then sell it normally. */
static void SettleSurfaceChemicalHauler()
{
	auto *train = Train::Get(VehicleID{13}); const auto debt = Company::Get(_current_company)->current_loan;
	if (_pause_mode.Test(PauseMode::Normal)) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
	const auto initial = TimerGameTick::counter; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(85);
	// The actual train must travel and halt before its original sale can admit
	// payment. Native routing pauses retain their owner and never grant a pose.
	while (!train->IsStoppedInDepot()) {
		REQUIRE(TimerGameTick::counter - initial < 3000); REQUIRE(std::chrono::steady_clock::now() < deadline);
		StateGameLoop();
		if (_pause_mode.Any()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	CHECK(train->tile == TileIndex{1373});
	for (const auto *part = train; part != nullptr; part = part->Next()) CHECK(part->cargo.TotalCount() == 0);
	auto quote = QuoteSurfaceChemical<Commands::SellVehicle>(TileIndex{0}, train->index, true, false, ClientID::Server); REQUIRE(quote.Succeeded());
	const auto paid = PaySurfaceChemical<Commands::SellVehicle>(TileIndex{0}, train->index, true, false, ClientID::Server); CHECK(paid.GetCost() == quote.GetCost());
	CHECK(Company::Get(_current_company)->current_loan == debt);
	fmt::print("SURFACE_CHEMICAL_HAULER_SETTLED earned={} sale={} cash={} debt={} depot=1373\n", TimerGameTick::counter - initial,
		-paid.GetCost().base(), Company::Get(_current_company)->money.base(), debt.base());
}

/** Buy and normally order one exact empty original road tanker.
 * @param cargo Actual Oil or CHEM meaning admitted by the original engine's refit.
 * @param pickup Real native loading station.
 * @param receiver Actual receiving station, or Invalid for a depot-halted reserve.
 * @param name Persistent cosmetic discovery name, never another inventory registry.
 * @return Actual native whole purchase invoice including ordinary initial refit.
 */
static Money BuySurfaceChemicalTanker(CargoType cargo, StationID pickup, StationID receiver, std::string name)
{
	const TileIndex depot{2443}; const EngineID engine{132};
	auto quote = QuoteSurfaceChemical<Commands::BuildVehicle>(depot, depot, engine, false, cargo, ClientID::Server); REQUIRE(std::get<0>(quote).Succeeded());
	const auto bought = PaySurfaceChemical<Commands::BuildVehicle>(depot, depot, engine, false, cargo, ClientID::Server);
	CHECK(std::get<0>(bought).GetCost() == std::get<0>(quote).GetCost()); const auto id = std::get<1>(bought);
	const auto *vehicle = Vehicle::Get(id); CHECK(vehicle->type == VehicleType::Road); CHECK(vehicle->cargo_type == cargo);
	CHECK(vehicle->cargo.TotalCount() == 0); CHECK(vehicle->cargo_cap == 21);
	PaySurfaceChemical<Commands::RenameVehicle>(TileIndex{0}, id, name);
	Order collect; collect.MakeGoToStation(pickup); collect.SetLoadType(OrderLoadType::FullLoad); collect.SetUnloadType(OrderUnloadType::NoUnload);
	PaySurfaceChemical<Commands::InsertOrder>(TileIndex{0}, id, VehicleOrderID{0}, collect);
	Order next;
	if (receiver == StationID::Invalid()) {
		next.MakeGoToDepot(Depot::GetByTile(depot)->index, OrderDepotTypeFlag::PartOfOrders,
			OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
	} else { next.MakeGoToStation(receiver); next.SetLoadType(OrderLoadType::NoLoad); }
	PaySurfaceChemical<Commands::InsertOrder>(TileIndex{0}, id, VehicleOrderID{1}, next);
	PaySurfaceChemical<Commands::StartStopVehicle>(TileIndex{0}, id, false);
	fmt::print("SURFACE_CHEMICAL_TANKER_PAID vehicle={} engine=132 cargo={} capacity=21 cost={} pickup={} receiver={}\n", id.base(), cargo,
		std::get<0>(bought).GetCost().base(), pickup.base(), receiver.base());
	return std::get<0>(bought).GetCost();
}

/** Buy empty Oil/CHEM tankers and dispatch the original loaded Salt train over the newly freed real rail platform. */
static void BuySurfaceChemicalFleet()
{
	REQUIRE(IsLayerIndustryComplete(*Industry::Get(IndustryID{15}))); REQUIRE_FALSE(Vehicle::IsValidID(VehicleID{13}));
	const auto *company = Company::Get(_current_company); const auto cash = company->money; REQUIRE(company->current_loan == 4650000);
	const auto packets = CargoPacket::GetNumItems(), vehicles = Vehicle::GetNumItems();
	const auto oil = GetCargoTypeByLabel(CargoLabel{"OIL_"}), chemical = GetCargoTypeByLabel(CargoLabel{"CHEM"}), salt = GetCargoTypeByLabel(CargoLabel{"SALT"});
	const auto pickup = GetStationIndex(TileIndex{2675}), input = GetStationIndex(TileIndex{1824}), output = GetStationIndex(TileIndex{1825});
	CHECK(pickup == StationID{6}); CHECK(input == StationID{7}); CHECK(output == StationID{8});
	Money paid{0};
	paid += BuySurfaceChemicalTanker(oil, pickup, input, "Surface Oil delivery 1");
	paid += BuySurfaceChemicalTanker(oil, pickup, input, "Surface Oil delivery 2");
	paid += BuySurfaceChemicalTanker(chemical, output, StationID::Invalid(), "Surface Chemical reserve");
	auto *train = Train::Get(VehicleID{18}); REQUIRE(train->IsStoppedInDepot()); REQUIRE(train->tile == TileIndex{1373});
	REQUIRE(train->Next() != nullptr); CHECK(train->Next()->index == VehicleID{19}); CHECK(train->Next()->cargo.TotalCount() == 60);
	CHECK(GetLayerIndustryReceiverRoomAt(*Station::Get(StationID{4}), salt) >= 60);
	// Replace the old mine pickup with one real delivery of its already held
	//60Salt. No stock is transferred by these order edits or by a vehicle pose.
	while (train->GetNumOrders() != 0) PaySurfaceChemical<Commands::DeleteOrder>(TileIndex{0}, train->index, VehicleOrderID{0});
	Order deliver; deliver.MakeGoToStation(StationID{4}); deliver.SetLoadType(OrderLoadType::NoLoad); deliver.SetStopLocation(OrderStopLocation::FarEnd);
	PaySurfaceChemical<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{0}, deliver);
	Order home; home.MakeGoToDepot(Depot::GetByTile(TileIndex{1373})->index, OrderDepotTypeFlag::PartOfOrders,
		OrderNonStopFlag::NonStop, OrderDepotActionFlag::Halt);
	PaySurfaceChemical<Commands::InsertOrder>(TileIndex{0}, train->index, VehicleOrderID{1}, home);
	PaySurfaceChemical<Commands::StartStopVehicle>(TileIndex{0}, train->index, true);
	CHECK(paid == 13615); CHECK(company->money == cash - paid); CHECK(CargoPacket::GetNumItems() == packets); CHECK(Vehicle::GetNumItems() == vehicles + 3);
	fmt::print("SURFACE_CHEMICAL_FLEET_PAID invoice={} cash={} debt={} empty_tankers=3 held_original_salt=60 supplied_pose=false\n", paid.base(), company->money.base(), company->current_loan.base());
}

/** Characterize real nonoverlapping factory candidates and their actual nearest-town constraint.
 * @param type Installed native Chemical works definition, shared by CHEM and PCHM.
 */
static void QuoteSurfaceChemicalSites(IndustryType type)
{
	const auto *spec = GetIndustrySpec(type); REQUIRE(spec->layouts.size() == 1); REQUIRE(spec->layouts.front().size() == 16);
	const auto *station = Station::Get(StationID{4});
	std::vector<TileIndex> candidates;
	for (TileIndex tile : MultiLayerMap::IterateLayer(MapLayer::Surface)) candidates.push_back(tile);
	/** Rank by the existing real Salt unloading station, then actual tile identity.
	 * @param tile Candidate Surface anchor.
	 * @return Stable distance and tile ordering, without changing terrain.
	 */
	const auto rank = [station](TileIndex tile) {
		return std::make_pair(std::abs(int(TileX(tile)) - int(TileX(station->xy))) +
			std::abs(int(TileY(tile)) - int(TileY(station->xy))), tile.base());
	};
	std::ranges::sort(candidates, {}, rank);
	std::map<TownID, uint> admitted;
	// Joint planning must retain whole16cell machinery and town identity. Two
	// independent successful quotes do not permit overlapping factories or a
	// second same-type factory in one town when the actual setting forbids it.
	for (TileIndex anchor : candidates) {
		if (TileX(anchor) + 4 >= Map::SizeX()) continue;
		if (TilePlaneY(anchor) + 4 >= Map::SizeY()) continue;
		const auto *town = ClosestTownFromTile(anchor, UINT_MAX); REQUIRE(town != nullptr);
		if (admitted[town->index] >= 8) continue;
		bool clear = true, catchment = false;
		// Native layout offsets own the footprint; empty-looking anchor tiles
		// cannot authorize clearance of a paid route or another actual facility.
		for (const auto &cell : spec->layouts.front()) {
			const auto tile = TileAddWrap(anchor, cell.ti.x, cell.ti.y);
			clear = clear && IsTileType(tile, TileType::Clear);
			catchment = catchment || station->TileIsInCatchment(tile);
		}
		if (!clear) continue;
		auto quote = QuoteSurfaceChemical<Commands::BuildIndustry>(anchor, anchor, type, uint32_t{0}, true, uint32_t{123});
		if (quote.Failed()) continue;
		++admitted[town->index];
		fmt::print("SURFACE_CHEMICAL_SITE anchor={} x={} y={} town={} cost={} salt_station_catchment={}\n",
			anchor.base(), TileX(anchor), TilePlaneY(anchor), town->index.base(), quote.GetCost().base(), catchment);
	}
	CHECK_FALSE(admitted.empty());
}

/** Query actual conventional carriers without extending any cargo/refit permission.
 * @param label Required original Oil, chemical or Propellant cargo meaning.
 */
static void QuoteSurfaceChemicalCarriers(CargoLabel label)
{
	const CargoType cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo));
	// Exact refit masks and whole BuildVehicle estimates must both admit a
	// useful positive-capacity carrier. A liquid class alone grants no hull.
	for (const auto kind : {VehicleType::Road, VehicleType::Train}) {
		const TileIndex depot = kind == VehicleType::Road ? TileIndex{2443} : TileIndex{1373};
		uint available = 0;
		for (const auto *engine : Engine::IterateType(kind)) {
			if (!engine->CanCarryCargo()) continue;
			if (!IsEngineBuildable(engine->index, kind, _current_company)) continue;
			if (GetUndergroundEngineCapability(engine->index) != nullptr) continue;
			if (!GetUnionOfArticulatedRefitMasks(engine->index, true).Test(cargo)) continue;
			auto quote = QuoteSurfaceChemical<Commands::BuildVehicle>(depot, depot, engine->index, false, cargo, ClientID::Server);
			if (std::get<0>(quote).Failed()) continue;
			if (std::get<2>(quote) == 0) continue;
			++available;
			fmt::print("SURFACE_CHEMICAL_CARRIER cargo={} kind={} engine={} cost={} capacity={}\n", label.AsString(),
				static_cast<uint>(kind), engine->index.base(), std::get<0>(quote).GetCost().base(), std::get<2>(quote));
		}
		fmt::print("SURFACE_CHEMICAL_CARRIER_COUNT cargo={} kind={} available={}\n", label.AsString(), static_cast<uint>(kind), available);
	}
}

/** Record actual facility, natural Oil, bank and recoverable-asset terms without spending. */
static void CharacterizeSurfaceChemical()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->current_loan == 4250000); REQUIRE(company->money == 88274);
	const auto cash = company->money; const auto expenses = company->yearly_expenses;
	const auto vehicles = Vehicle::GetNumItems(), industries = Industry::GetNumItems(), packets = CargoPacket::GetNumItems();
	const IndustryType type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}); REQUIRE(type != IT_INVALID);
	REQUIRE(IsLayerProcessorDefinition(type));
	fmt::print("SURFACE_CHEMICAL_WORLD type={} year={} cash={} debt={} ceiling={} towns={} multiple_per_town={}\n", type,
		TimerGameCalendar::year.base(), cash.base(), company->current_loan.base(), company->GetMaxLoan().base(), Town::GetNumItems(),
		_settings_game.economy.multiple_industry_per_town);
	// Both alternatives use one installed facility family but distinct physical
	// source identities are required by the original own-source delivery guard.
	for (const auto &record : GetMultiLayerManufacturingRecipes()) {
		if (record.facility != "Chemical works") continue;
		fmt::print("SURFACE_CHEMICAL_RECIPE id={} year={} work={} batches={} input_capacity={} output_capacity={}\n", record.recipe.id.AsString(),
			record.introduction_year.base(), record.recipe.work_economy_ticks, record.recipe.max_batches, record.input_capacity, record.output_capacity);
	}
	const CargoType oil = GetCargoTypeByLabel(CargoLabel{"OIL_"}); REQUIRE(IsValidCargoType(oil));
	// Only a genuinely generated ordinary producer can supply the selected Oil;
	// an input acceptance slot or a processor output cannot stand in for it.
	for (const auto *industry : Industry::Iterate()) {
		if (GetTileLayer(industry->location.tile) != MapLayer::Surface) continue;
		for (const auto &product : industry->produced) {
			if (product.cargo != oil) continue;
			fmt::print("SURFACE_CHEMICAL_OIL industry={} type={} anchor={} founder={} rate={} waiting={}\n", industry->index.base(),
				industry->type, industry->location.tile.base(), industry->founder.base(), product.rate, product.waiting);
		}
	}
	QuoteSurfaceChemicalSites(type);
	SurfaceChemicalLayout layout; layout.Prepare();
	const auto first = QuoteSurfaceChemical<Commands::BuildIndustry>(TileIndex{1566}, TileIndex{1566}, type, uint32_t{0}, true, uint32_t{123});
	const auto second = QuoteSurfaceChemical<Commands::BuildIndustry>(TileIndex{1639}, TileIndex{1639}, type, uint32_t{0}, true, uint32_t{123});
	REQUIRE(first.Succeeded()); REQUIRE(second.Succeeded());
	const auto oil_truck = QuoteSurfaceChemical<Commands::BuildVehicle>(TileIndex{2443}, TileIndex{2443}, EngineID{132}, false, oil, ClientID::Server);
	const auto chemical_truck = QuoteSurfaceChemical<Commands::BuildVehicle>(TileIndex{2443}, TileIndex{2443}, EngineID{132}, false,
		GetCargoTypeByLabel(CargoLabel{"CHEM"}), ClientID::Server);
	REQUIRE(std::get<0>(oil_truck).Succeeded()); REQUIRE(std::get<0>(chemical_truck).Succeeded());
	const auto recovery = QuoteSurfaceChemical<Commands::SellVehicle>(TileIndex{0}, VehicleID{7}, true, false, ClientID::Server); REQUIRE(recovery.Succeeded());
	const auto capital = first.GetCost() + layout.invoice + 2 * std::get<0>(oil_truck).GetCost() + std::get<0>(chemical_truck).GetCost();
	const Money buffer{100000}; const auto shortfall = capital + buffer - (cash - recovery.GetCost());
	const Money credit{((shortfall.base() + 9999) / 10000) * 10000};
	REQUIRE(company->current_loan + credit <= company->GetMaxLoan());
	fmt::print("SURFACE_CHEMICAL_CAPITAL first={} second={} infrastructure={} carriers={} initial_total={} cutter_recovery={} buffer={} credit={} future_headroom={}\n",
		first.GetCost().base(), second.GetCost().base(), layout.invoice.base(),
		(2 * std::get<0>(oil_truck).GetCost() + std::get<0>(chemical_truck).GetCost()).base(), capital.base(), -recovery.GetCost().base(),
		buffer.base(), credit.base(), (company->GetMaxLoan() - company->current_loan - credit).base());
	for (const auto label : {CargoLabel{"OIL_"}, CargoLabel{"CHEM"}, CargoLabel{"PROP"}}) QuoteSurfaceChemicalCarriers(label);
	// Idle assets are capital only after an admitted original sale. Book values
	// and machinery hidden outside a legal depot cannot become free cash.
	for (const auto id : {VehicleID{7}, VehicleID{13}}) {
		const auto *vehicle = Vehicle::Get(id); REQUIRE(vehicle->cargo.TotalCount() == 0);
		auto quote = QuoteSurfaceChemical<Commands::SellVehicle>(TileIndex{0}, id, true, false, ClientID::Server);
		fmt::print("SURFACE_CHEMICAL_RECOVERY vehicle={} tile={} admitted={} cost={} error={}\n", id.base(), vehicle->tile.base(),
			quote.Succeeded(), quote.GetCost().base(), quote.GetErrorMessage().base());
	}
	CHECK(company->money == cash); CHECK(company->current_loan == 4250000); CHECK(company->yearly_expenses == expenses);
	CHECK(Vehicle::GetNumItems() == vehicles); CHECK(Industry::GetNumItems() == industries); CHECK(CargoPacket::GetNumItems() == packets);
}

/** The actual saved world authorizes only native pure producer/route/capital quotes. */
TEST_CASE("Authentic Surface chemicals use original placement freight and capital", "[.surface-entry-chemical]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *mode = std::getenv("OTTD_SURFACE_CHEMICAL_PHASE"); const std::string_view phase = mode == nullptr ? "query" : mode;
	// Each explicit phase owns a distinct financial/construction boundary;
	// a pure query cannot silently sell machinery or fund a new industry.
	REQUIRE((phase == "query" || phase == "capital" || phase == "fund" || phase == "recall" || phase == "settle" || phase == "fleet"));
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-chemical"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve native assertions through ordinary shutdown without compensating grants. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceChemical(paths.saves / (phase == "query" ? "surface-chemical-before-440.sav" : fmt::format("surface-chemical-before-{}-440.sav", phase)));
			if (phase == "query") CharacterizeSurfaceChemical();
			else if (phase == "capital") RecoverSurfaceChemicalCapital();
			else if (phase == "fund") FundSurfaceChemicalWorks();
			else if (phase == "recall") RecallSurfaceChemicalHauler();
			else if (phase == "settle") SettleSurfaceChemicalHauler();
			else BuySurfaceChemicalFleet();
			SaveSurfaceChemical(paths.saves / fmt::format("surface-chemical-{}-440.sav", phase));
			fmt::print("SURFACE_CHEMICAL_COMPLETE phase={} pure={} supplied_stock=false\n", phase, phase == "query"); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_chemical_observer, &inspect); IConsole::CmdRegister("surface_entry_chemical", ObserveSurfaceChemical);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
