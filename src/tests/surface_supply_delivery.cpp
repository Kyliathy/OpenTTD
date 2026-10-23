/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_delivery.cpp Paid construction cargo on ordinary three-plane trains.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CD.
 * Initial capital, dry Surface and excavated floor are declared scenario inputs.
 * Subsequent infrastructure, machinery, purchases and physical cargo use native owners.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_depot_fixture.h"
#include "test_artifact_paths.h"
#include "../surface_supply_cmd.h"
#include "../surface_supply_order_runtime.h"
#include "../geothermal_collector.h"
#include "../geothermal_riser.h"
#include "../order_cmd.h"
#include "../vehicle_cmd.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

/** Original station scheduler: paid rights become packets only through this production owner. */
extern void OnTick_Station();
/** Complete the production cargo-definition defaults before rebuilding derived sorting; never creates physical cargo. */
extern void FinaliseCargoArray();

/** Retire this scenario's auxiliary owners after native vehicles/industries and their world are destroyed. */
class SurfaceSupplyDeliveryCleanup {
public:
	/** Clear only test-owned order/construction/utility metadata, including failure-unwind paths; never changes another fixture's physical pools. */
	~SurfaceSupplyDeliveryCleanup()
	{
		ResetSurfaceSupplyOrders();
		ResetGeothermalCollectors();
		ResetGeothermalRisers();
		ResetGeothermalSiteClaims();
		ResetUtilityPowerState();
	}
};

/** Current scheduled material meanings; native slots are always resolved from these labels. */
static constexpr std::array<CargoLabel, 4> DELIVERY_LABELS{
	CargoLabel{"STEL"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}};

/** Reconstruct distributed cargo dimensions before authoring any paid input.
 * The reduced geology fixture installs labels only. Ordinary baseline cargo
 * properties come from their production climate owner; added native labels
 * use the fixed16mass emitted by media/multilayer/build_content.py property0x0F.
 * This initializes definitions only and grants no physical stock or receipts.
 */
static void InitialiseDeliveryCargoDimensions()
{
	std::array<CargoSpec, NUM_CARGO> native;
	for (CargoType cargo{0}; cargo < NUM_CARGO; ++cargo) native[cargo] = *CargoSpec::Get(cargo);
	SetupCargoForClimate(LandscapeType::Temperate);
	// Restore only the actual required package's added cargo definitions. The
	// production climate owner retains every original label, slot and mass.
	for (CargoType cargo{0}; cargo < NUM_CARGO; ++cargo) {
		if (IsDefaultCargo(cargo)) continue;
		if (GetMultiLayerCargoContract(native[cargo].label) == nullptr) continue;
		*CargoSpec::Get(cargo) = native[cargo];
		CargoSpec::Get(cargo)->weight = 16;
		_cargo_mask.Set(cargo);
	}
	BuildCargoLabelMap();
	FinaliseCargoArray();
	InitializeSortedCargoSpecs();
}

/** Execute and invoice one original synchronized command.
 * @tparam Tcmd Native command identity.
 * @tparam Targs Typed original command arguments.
 * @param args Original world/vehicle/material arguments; no private owner writes.
 * @return Complete result, including native created identities and the actual charge.
 */
template <Commands Tcmd, typename... Targs>
static auto DeliveryCommand(Targs... args)
{
	auto result = Command<Tcmd>::template Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false,
		INVALID_TILE, std::make_tuple(args...));
	INFO("delivery command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(result).GetErrorMessage().base());
	REQUIRE(ExtractCommandCost(result).Succeeded());
	return result;
}

/** Save original native owners at a precise physical boundary, without simulation progress.
 * @param path Fresh invocation-owned output; never an immutable predecessor.
 */
static void SaveDrivenSurfaceSupply(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup mode(_game_mode, GameMode::Menu);
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Read the carrier's original physical quantities and require their purchased provenance.
 * @param front Actual ordinary primary train and its native attached wagons.
 * @param source Original owned Surface loading station.
 * @return Whole current units for the four scheduled labels; no parallel gameplay stock is introduced.
 */
static std::array<uint32_t, 4> ObserveDrivenSurfaceSupply(const Train &front, StationID source)
{
	std::array<uint32_t, 4> result{};
	// Inspect every physical wagon and packet because native split/merge can
	// retire the originally issued packet ID while preserving its purpose.
	for (const Vehicle *part = &front; part != nullptr; part = part->Next()) {
		for (const auto *packet : *part->cargo.Packets()) {
			REQUIRE(packet->GetSource().IsCompanySupply());
			CHECK(packet->GetSource().id == _current_company.base());
			CHECK(packet->GetFirstStation() == source);
			CHECK(packet->GetFeederShare() == 0);
			for (size_t index = 0; index < DELIVERY_LABELS.size(); ++index) {
				if (part->cargo_type == GetCargoTypeByLabel(DELIVERY_LABELS[index])) result[index] += packet->Count();
			}
		}
	}
	return result;
}

/** A scoped procurement proof retains declared floor/capital but drives every subsequent material through original owners. */
TEST_CASE("Bought construction materials physically cross both rail portals into bounded collector and riser bills", "[surface-supply-driven]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	NativeDepotEconomy economy;
	SurfaceSupplyDeliveryCleanup cleanup;
	UndergroundWorkflowWorld world(MapLayer::DeepUnderground, true, true);
	LayerSaveContent content(world.ActivatedContent(), true);
	InitialiseDeliveryCargoDimensions();
	REQUIRE(InitialiseGeothermalCollectorDefinitions(world.ActivatedContent()));
	REQUIRE(InitialiseGeothermalRiserDefinitions(world.ActivatedContent()));
	ResetSurfaceSupplyOrders();
	const auto paths = GetTestCaseArtifactPaths("space", "surface-supply-delivery-drive");
	// Construction may replace the geology snapshot; retain only the original
	// selected source value, never a borrowed vector reference across commands.
	const auto vent = GetGeologyWorldState().vents.front();
	const uint x = vent.x - 2, deep_y = vent.y - 4, surface_y = deep_y - 4;
	std::vector<TileIndex> floor;
	// This isolated transport scenario declares its initial foundation/corridor
	// before the saved input. It does not credit these cells to paid TBM work.
	for (MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
		const uint first_y = layer == MapLayer::Underground ? surface_y - 1 : deep_y - 1;
		for (uint row = first_y; row <= deep_y + 8; ++row) {
			for (uint column = x - 1; column <= x + 5; ++column) floor.push_back(TileXYL(column, row, layer));
		}
	}
	world.Open(floor);
	SaveDrivenSurfaceSupply(paths.saves / "delivery-declared-input-440.sav");
	const TileIndex upper = TileXYL(x, surface_y, MapLayer::Surface);
	const TileIndex middle = TileXYL(x, deep_y, MapLayer::Underground);
	DeliveryCommand<Commands::BuildLayerPortal>(upper, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD);
	DeliveryCommand<Commands::BuildLayerPortal>(middle, TransportType::Rail, MapLayer::DeepUnderground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD);
	const TileIndex lower = GetLayerPortalPairTile(middle);
	const TileIndex depot = TileXYL(x, surface_y - 10, MapLayer::Surface);
	const TileIndex pickup = TileXYL(x, surface_y - 7, MapLayer::Surface);
	const TileIndex dropoff = TileXYL(x, deep_y + 2, MapLayer::DeepUnderground);
	DeliveryCommand<Commands::BuildRailDepot>(depot, RAILTYPE_RAIL, DiagDirection::SE);
	// Each plane has its own actual track/facility commands. No same-coordinate
	// catchment or packet move can stand in for either physical portal crossing.
	for (uint row = surface_y - 9; row < surface_y; ++row) {
		const bool platform = row >= surface_y - 7 && row < surface_y - 3;
		if (!platform) DeliveryCommand<Commands::BuildRail>(TileXYL(x, row, MapLayer::Surface), RAILTYPE_RAIL, Track::Y, false);
	}
	for (uint row = surface_y + 1; row < deep_y; ++row) DeliveryCommand<Commands::BuildRail>(TileXYL(x, row, MapLayer::Underground), RAILTYPE_RAIL, Track::Y, false);
	DeliveryCommand<Commands::BuildRail>(TileXYL(x, deep_y + 1, MapLayer::DeepUnderground), RAILTYPE_RAIL, Track::Y, false);
	DeliveryCommand<Commands::BuildRailStation>(pickup, RAILTYPE_RAIL, Axis::Y, uint8_t{1}, uint8_t{4}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	DeliveryCommand<Commands::BuildRailStation>(dropoff, RAILTYPE_RAIL, Axis::Y, uint8_t{1}, uint8_t{4}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	DeliveryCommand<Commands::ChangeLayerPortalConnection>(upper, true);
	DeliveryCommand<Commands::ChangeLayerPortalConnection>(middle, true);
	const StationID origin = Station::GetByTile(pickup)->index;
	const StationID receiver = Station::GetByTile(dropoff)->index;
	GeothermalSiteRequest claim;
	claim.vent = vent.id; claim.owner = _current_company;
	claim.tile = claim.endpoint = TileXYL(vent.x, vent.y, MapLayer::DeepUnderground); claim.access = DiagDirection::SE;
	const auto reserved = ClaimGeothermalSite(claim);
	REQUIRE(reserved.error == GeothermalError::None);
	DeliveryCommand<Commands::BuildGeothermalCollector>(claim.tile, reserved.id, uint32_t{123});
	const TileIndex riser = TileXYL(x + 1, deep_y, MapLayer::DeepUnderground);
	DeliveryCommand<Commands::BuildGeothermalRiser>(riser, lower, riser, DiagDirection::SE, uint32_t{123});
	const IndustryID collector_id = FindGeothermalCollectorForVent(vent.id);
	const IndustryID riser_id = FindGeothermalRiserForPortal(lower);
	Station::Get(receiver)->RecomputeCatchment();
	UpdateStationAcceptance(Station::Get(receiver), false);
	const auto [engine_cost, engine_id, capacity, mail, requested] = DeliveryCommand<Commands::BuildVehicle>(depot, GetUndergroundEngine(4), false, INVALID_CARGO, ClientID::Server);
	Train *front = Train::Get(engine_id);
	// Each scheduled cargo receives a real conventional wagon, bought/refitted
	// by native acquisition and attached through the ordinary consist owner.
	for (CargoLabel label : DELIVERY_LABELS) {
		const auto cargo = GetCargoTypeByLabel(label);
		const auto [cost, id, cap, mail_cap, refit] = DeliveryCommand<Commands::BuildVehicle>(depot, GetUndergroundEngine(5), false, cargo, ClientID::Server);
		Vehicle *wagon = Vehicle::Get(id);
		if (wagon->First() != front) DeliveryCommand<Commands::MoveRailVehicle>(id, front->Last()->index, false);
		REQUIRE(wagon->cargo_type == cargo); REQUIRE(wagon->cargo_cap >= 13);
	}
	Order load; load.MakeGoToStation(origin); load.SetUnloadType(OrderUnloadType::NoUnload);
	Order deliver; deliver.MakeGoToStation(receiver); deliver.SetLoadType(OrderLoadType::NoLoad);
	DeliveryCommand<Commands::InsertOrder>(front->index, VehicleOrderID{0}, load);
	DeliveryCommand<Commands::InsertOrder>(front->index, VehicleOrderID{1}, deliver);
	SaveDrivenSurfaceSupply(paths.saves / "delivery-ready-empty-440.sav");
	const std::array<uint16_t, 4> bought{13, 9, 5, 3};
	const std::array<uint32_t, 4> bill{12, 8, 4, 2};
	Company *company = Company::Get(_current_company);
	const Money before = company->money, expense = company->yearly_expenses[0][ExpensesType::Property];
	Money charged{0};
	// Buy one unit more than the combined bills for each meaning: bounded
	// unloading must preserve that real excess rather than silently deleting it.
	for (size_t index = 0; index < DELIVERY_LABELS.size(); ++index) {
		const auto label = DELIVERY_LABELS[index];
		const uint32_t wire = (uint32_t{label[0]} << 24) | (uint32_t{label[1]} << 16) | (uint32_t{label[2]} << 8) | label[3];
		charged += DeliveryCommand<Commands::PlaceSurfaceSupplyOrder>(origin, wire, bought[index], SurfaceSupplyOrderID{index + 1}).GetCost();
	}
	CHECK(company->money == before - charged);
	CHECK(company->yearly_expenses[0][ExpensesType::Property] == expense + charged);
	SaveDrivenSurfaceSupply(paths.saves / "delivery-paid-pending-440.sav");
	OnTick_Station();
	DeliveryCommand<Commands::StartStopVehicle>(front->index, false);
	AutoRestoreBackup running(_pause_mode, PauseModes{});
	world.simulate_industries = true;
	std::array<bool, 4> loaded_layers{};
	bool loaded_saved = false, delivered = false;
	uint ticks = 0;
	// Only native frame owners advance transport and construction. Observation
	// never writes a pose, packet, remaining bill, work phase or income balance.
	while (ticks < 12000) {
		world.Tick(); ++ticks;
		const auto cargo = ObserveDrivenSurfaceSupply(*front, origin);
		const uint32_t total = std::accumulate(cargo.begin(), cargo.end(), uint32_t{0});
		if (total != 0) loaded_layers[to_underlying(GetTileLayer(front->tile))] = true;
		// Preserve a genuine in-flight Deep checkpoint for later ordinary
		// load/resave/continue; it must precede the first receiver receipt.
		if (!loaded_saved && GetTileLayer(front->tile) == MapLayer::DeepUnderground && total == 30) {
			SaveDrivenSurfaceSupply(paths.saves / "delivery-loaded-deep-440.sav"); loaded_saved = true;
		}
		const auto *collector = GetGeothermalCollectorState(collector_id);
		const auto *relay = GetGeothermalRiserState(riser_id);
		REQUIRE(collector != nullptr); REQUIRE(relay != nullptr);
		// Both independent machinery owners must consume their complete bills;
		// finishing one facility cannot discharge the other's material obligation.
		if (collector->materials_committed && relay->materials_committed) delivered = true;
		// The original complete train must physically return with all overflow;
		// a full receiver cannot destroy remaining non-paying purchased units.
		if (delivered && GetTileLayer(front->tile) == MapLayer::Surface && cargo == std::array<uint32_t, 4>{1, 1, 1, 1}) break;
	}
	INFO("driven ticks=" << ticks << " train=" << DescribeTrain(front));
	REQUIRE(ticks < 12000); REQUIRE(loaded_saved); REQUIRE(delivered);
	CHECK(loaded_layers[to_underlying(MapLayer::Surface)]);
	CHECK(loaded_layers[to_underlying(MapLayer::Underground)]);
	CHECK(loaded_layers[to_underlying(MapLayer::DeepUnderground)]);
	SaveDrivenSurfaceSupply(paths.saves / "delivery-returned-overflow-440.sav");
	bool full_stop = false;
	uint retry_ticks = 0;
	// A further physical round trip offers the same retained overflow to now
	// full receivers. It must neither consume another bill nor create revenue.
	while (retry_ticks < 4000) {
		world.Tick(); ++retry_ticks;
		CHECK((ObserveDrivenSurfaceSupply(*front, origin) == std::array<uint32_t, 4>{1, 1, 1, 1}));
		// Loading at the actual destination proves another receiver visit;
		// merely re-entering Deep would not prove a second full-intake refusal.
		if (front->last_station_visited == receiver && front->current_order.IsType(OT_LOADING)) full_stop = true;
		// Finish only after that full-receiver visit and a real Surface return.
		if (full_stop && GetTileLayer(front->tile) == MapLayer::Surface) break;
	}
	REQUIRE(retry_ticks < 4000); REQUIRE(full_stop);
	// Native accepted histories retain the construction receipt after the
	// complete bill is consumed. Aggregate history cannot exceed real purchases.
	for (size_t index = 0; index < DELIVERY_LABELS.size(); ++index) {
		uint32_t accepted = 0;
		for (IndustryID id : {collector_id, riser_id}) {
			Industry *industry = Industry::Get(id);
			const auto input = industry->GetCargoAccepted(GetCargoTypeByLabel(DELIVERY_LABELS[index]));
			if (input == industry->accepted.end()) continue;
			REQUIRE(input->history != nullptr); accepted += (*input->history)[THIS_MONTH].accepted;
			CHECK(input->waiting == 0);
		}
		CHECK(accepted == bill[index]);
		CHECK(Station::Get(origin)->goods[GetCargoTypeByLabel(DELIVERY_LABELS[index])].TotalCount() == 0);
		CHECK(GetSurfaceSupplyOrder(index + 1)->Native()->state == SurfaceSupplyOrderState::Fulfilled);
		CHECK(CmdCancelSurfaceSupplyOrder({}, index + 1).Failed());
	}
	CHECK((ObserveDrivenSurfaceSupply(*front, origin) == std::array<uint32_t, 4>{1, 1, 1, 1}));
	CHECK(company->yearly_expenses[0][ExpensesType::Property] == expense + charged);
	CHECK(company->yearly_expenses[0][ExpensesType::TrainRevenue] == 0);
	CHECK(company->cur_economy.income == 0);
	REQUIRE(ValidateSurfaceSupplyOrderBook());
	SaveDrivenSurfaceSupply(paths.saves / "delivery-full-refusal-return-440.sav");
	fmt::print("SURFACE_SUPPLY_DRIVEN_COMPLETE ticks={} retry_ticks={} charged={} accepted=12,8,4,2 overflow=1,1,1,1 native_layers=Surface,Underground,Deep prepared_floor=true unassisted=false\n", ticks, retry_ticks, charged.base());
}
