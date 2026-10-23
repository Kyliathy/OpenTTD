/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_space_loop_native.cpp Actual rail freight, paid original probe/rocket, descent and separate Surface collection.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group18.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_vehicle.h"
#include "../space_transition.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../launch_center.h"
#include "../launch_center_cmd.h"
#include "../launch_operations.h"
#include "../multilayer_space_content.h"
#include "../asteroid.h"
#include "../station_base.h"
#include "../station_cmd.h"
#include "../station_map.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../rail_cmd.h"
#include "../track_func.h"
#include "../newgrf_station.h"
#include "../underground_vehicle.h"
#include "../engine_base.h"
#include "../vehicle_cmd.h"
#include "../order_base.h"
#include "../order_cmd.h"
#include "../command_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../console_internal.h"
#include "../settings_type.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include <filesystem>
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <cstdlib>
#include <thread>
#include <set>
#include "../safeguards.h"

static std::function<void()> *commodity_loop_observer = nullptr; ///< Fixture observer only; actual native vehicles, packets and commands own every transfer.
static constexpr std::string_view SUPPLY_TRAIN_NAME = "Space loop Goods supply"; ///< Cosmetic native name discovers the same saved train, without a second identity registry.

/** @param args Parameter-free loaded-world callback. @return Whether the installed fixture observer ran. */
static bool ObserveCommodityLoop(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(commodity_loop_observer != nullptr); (*commodity_loop_observer)(); return true;
}

/** @param model Delivered Space model role. @return Its unique actual original paid native hull in this retained world. */
static SpaceVehicle *FindLoopCraft(uint16_t model)
{
	SpaceVehicle *result = nullptr;
	// The genuine predecessor has one paid body per role; accidental newly commissioned replacements must fail discovery rather than substitute for an original hull.
	for (auto *craft : SpaceVehicle::Iterate()) if (GetSpaceVehicleModel(*craft) == model) { REQUIRE(result == nullptr); result = craft; }
	REQUIRE(result != nullptr); return result;
}

/** @param center Actual recovery owner. @param facility Required native member loading facility. @return The unique separate current collection member of that kind. */
static LaunchHubMemberRef FindLoopMember(const Station &center, StationFacility facility)
{
	LaunchHubMemberRef result{};
	// Separate physical station membership supplies the collection path; a joined center tile cannot stand in for the distinct freight destination.
	for (const auto ref : center.launch_hub->members) {
		const auto *station = ResolveLaunchHubMember(GetLaunchCenterReference(center), ref);
		if (station != nullptr && station->facilities.Test(facility)) { REQUIRE(result.station == StationID::Invalid()); result = ref; }
	}
	REQUIRE(result.station != StationID::Invalid()); return result;
}

/** Build a paid straight rail route from the actual separate loading platform.
 * @param center Actual native receiving station; joining never changes its orbital anchor.
 * @param source Actual separate Goods loading station with an existing rail platform.
 * @return Command-created depot connected to the real receiving/loading route.
 */
static TileIndex BuildLoopRail(Station &center, const Station &source)
{
	const auto platform = source.train_station.tile; REQUIRE(platform != INVALID_TILE);
	const auto axis = GetRailStationAxis(platform); const auto track = AxisToTrack(axis);
	const auto flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRailLong>());
	AutoRestoreBackup distant_join(_settings_game.station.distant_join_stations, true);
	// Search both ends of the real loading platform. A depot behind the source and a joined receiver ahead make a whole physical route without clearing any existing station or changing terrain privately.
	for (const auto direction : {AxisToDiagDir(axis), ReverseDiagDir(AxisToDiagDir(axis))}) {
		const auto entrance = TileAddByDiagDir(platform, direction), depot = TileAddByDiagDir(platform, ReverseDiagDir(direction));
		if (CmdBuildTrainDepot(DoCommandFlag::Auto, depot, RAILTYPE_RAIL, direction).Failed()) continue;
		for (uint distance = 2; distance <= 8; ++distance) {
			auto receiver = entrance;
			for (uint step = 1; step < distance; ++step) receiver = TileAddByDiagDir(receiver, direction); // Walk the bounded proposed straight route using actual tile neighbors, never stacked-layer offsets.
			const auto last = TileAddByDiagDir(receiver, ReverseDiagDir(direction));
			if (CmdBuildRailStation(flags, receiver, RAILTYPE_RAIL, axis, 1, 1, STAT_CLASS_DFLT, 0, center.index, false).Failed()) continue;
			if (CmdBuildRailroadTrack(flags, last, entrance, RAILTYPE_RAIL, track, false, true).Failed()) continue;
			REQUIRE(Command<Commands::BuildRailLong>::Post(last, entrance, RAILTYPE_RAIL, track, false, true));
			REQUIRE(Command<Commands::BuildRailStation>::Post(receiver, RAILTYPE_RAIL, axis, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, center.index, false));
			REQUIRE(Command<Commands::BuildRailDepot>::Post(depot, RAILTYPE_RAIL, direction));
			CHECK(GetStationIndex(receiver) == center.index); return depot;
		}
	}
	FAIL("No native rail route/depot can connect the actual loop stations"); return INVALID_TILE;
}

/** Buy and order a real Surface locomotive and compatible Goods wagon.
 * @param depot Actual paid rail depot connected to both loading points.
 * @param source Actual rail pickup station, unlinked after outbound handling settled.
 * @param center Actual joined receiving station with an ordinary transfer order.
 * @param cargo Delivered GOOD identity from the world's native cargo catalogue.
 * @param shipment Actual available shipment bounded by the wagon and later probe.
 * @return The command-created ordinary primary train that physically delivers this cargo.
 */
static Train *BuyLoopTrain(TileIndex depot, StationID source, StationID center, CargoType cargo, uint16_t shipment)
{
	REQUIRE(shipment <= GetSpaceFreightCapacity(20, cargo)); Train *front = nullptr;
	// Both delivered native designs use current normal company/date/layer/refit admission; no fixture toggles an engine's availability or buys a fabricated substitute.
	for (uint16_t model : {4, 5}) {
		const auto engine = GetUndergroundEngine(model); const auto requested = model == 4 ? INVALID_CARGO : cargo;
		const auto quote = CmdBuildVehicle({DoCommandFlag::QueryCost}, depot, engine, false, requested, ClientID{});
		fmt::print("LOOP_RAIL_QUOTE model={} engine={} error={} capacity={}\n", model, engine.base(), std::get<0>(quote).GetErrorMessage().base(), std::get<2>(quote));
		REQUIRE(std::get<0>(quote).Succeeded()); if (model == 5) REQUIRE(std::get<2>(quote) >= shipment);
		std::set<VehicleID> old;
		for (const auto *vehicle : Train::Iterate()) old.insert(vehicle->index); // Capture actual VEHS identities so this command's newly purchased unit cannot be confused with an old hull.
		const auto money = Company::Get(_current_company)->money;
		REQUIRE(Command<Commands::BuildVehicle>::Post(depot, engine, false, requested, ClientID{})); CHECK(Company::Get(_current_company)->money == money - std::get<0>(quote).GetCost());
		Train *unit = nullptr;
		// Only the actual freshly purchased matching unit is eligible; automatic ordinary depot attachment remains the engine's native behavior.
		for (auto *vehicle : Train::Iterate()) {
			if (!old.contains(vehicle->index) && vehicle->engine_type == engine) { REQUIRE(unit == nullptr); unit = vehicle; }
		}
		REQUIRE(unit != nullptr);
		CHECK(unit->round_trip_time == 0); // Newly purchased locomotive and wagon have no completed unbunching journey; both save this common field even though only the primary owns orders.
		if (model == 4) { REQUIRE(unit->IsPrimaryVehicle()); front = unit; }
		else {
			REQUIRE(front != nullptr);
			if (unit->First() != front) REQUIRE(Command<Commands::MoveRailVehicle>::Post(depot, unit->index, front->index, false));
			CHECK(unit->First() == front); CHECK(unit->cargo_type == cargo); CHECK(unit->cargo_cap >= shipment);
		}
	}
	REQUIRE(front != nullptr); REQUIRE(Command<Commands::RenameVehicle>::Post(front->index, std::string{SUPPLY_TRAIN_NAME}));
	Order pickup; pickup.MakeGoToStation(source); pickup.SetUnloadType(OrderUnloadType::NoUnload); pickup.SetLoadType(OrderLoadType::LoadIfPossible);
	Order transfer; transfer.MakeGoToStation(center); transfer.SetUnloadType(OrderUnloadType::Transfer); transfer.SetLoadType(OrderLoadType::NoLoad);
	REQUIRE(Command<Commands::InsertOrder>::Post(front->tile, front->index, VehicleOrderID{0}, pickup)); REQUIRE(Command<Commands::InsertOrder>::Post(front->tile, front->index, VehicleOrderID{1}, transfer));
	REQUIRE(Command<Commands::StartStopVehicle>::Post(front->tile, front->index, false)); return front;
}

/** @param front Actual command-built ordinary train. @return Its unique physical freight wagon; the locomotive's empty list cannot stand in for delivered cargo. */
static Train *FindLoopWagon(Train &front)
{
	Train *result = nullptr;
	// The delivered two-unit consist has one powered locomotive and one real freight owner. Reject extra cargo bodies instead of silently summing a different fixture.
	for (auto *unit = &front; unit != nullptr; unit = unit->Next()) if (unit->cargo_cap != 0) { REQUIRE(result == nullptr); result = unit; }
	REQUIRE(result != nullptr); return result;
}

/** Execute one bounded actual Surface/Space commodity stage, retaining all original paid identities and normal physical transport. */
TEST_CASE("An actual Surface train feeds paid rocket resupply and the same Goods return through collection", "[.surface-space-loop]")
{
	const char *input = std::getenv("OTTD_COMMODITY_LOOP_SOURCE"), *action = std::getenv("OTTD_COMMODITY_LOOP_STAGE"); REQUIRE(input != nullptr); REQUIRE(action != nullptr); const std::string stage{action};
	const auto paths = GetTestCaseArtifactPaths("space", "surface-space-loop"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Advance only the loaded original paid world and propagate native assertion failure after orderly shutdown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AutoRestoreBackup running_world(_pause_mode, PauseModes{}); REQUIRE(ValidateSpaceVehicles());
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			auto *rocket = FindLoopCraft(14), *builder = FindLoopCraft(20); const auto rocket_ref = GetSpaceCraftReference(*rocket), builder_ref = GetSpaceCraftReference(*builder);
			const auto home = rocket->construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center != nullptr); REQUIRE(center->launch_hub != nullptr);
			const auto good = GetCargoTypeByLabel(CargoLabel{"GOOD"}); REQUIRE(IsValidCargoType(good)); const auto fleet = GetSpaceVehicleCount();
			auto *company = Company::Get(rocket->owner); const auto delivered = company->cur_economy.delivered_cargo[good]; CHECK(delivered == 0);
			/** @param suffix Before or after this exact stage; use only the normal native writer after ownership validation. */
			auto save = [&](std::string_view suffix) {
				REQUIRE(ValidateSpaceVehicles()); REQUIRE(ValidateSurfaceHandling()); AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("commodity-loop-{}-{}-423.sav", stage, suffix)), false) == SaveLoadResult::Ok);
			};
			/** @param done Actual physical/command completion predicate. @param limit Maximum authoritative opportunities. @param reason Native operation for bounded diagnostics. */
			auto advance_until = [&](const std::function<bool()> &done, uint limit, std::string_view reason) {
				AutoRestoreBackup running(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(85); uint ticks = 0;
				// Ordinary vehicle travel/loading and existing Space/handling controllers get genuine synchronized opportunities; no observer edits their positions, phases, cargo or timers.
				while (!done()) {
					INFO(reason); REQUIRE(ticks < limit); REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop();
					if (GetAsteroidField().elapsed_ticks == before) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
					REQUIRE(GetAsteroidField().elapsed_ticks == before + 1); ++ticks;
				}
			};
			/** @return The actual saved rail vehicle commissioned in preparation, identified by its native cosmetic name. */
			auto find_train = [&]() -> Train * {
				Train *result = nullptr;
				// Only the persistent ordinary primary owns the transfer orders; this label observes VEHS rather than creating another cargo or vehicle registry.
				for (auto *vehicle : Train::Iterate()) if (vehicle->name == SUPPLY_TRAIN_NAME) { REQUIRE(result == nullptr); result = vehicle; }
				REQUIRE(result != nullptr); return result;
			};
			save("before"); uint quantity = 0; Train *train = nullptr; Train *wagon = nullptr;
			if (stage == "prepare") {
				REQUIRE(rocket->construction.placement == SpacePlacement::SurfaceStored); REQUIRE(builder->construction.placement == SpacePlacement::SurfaceStored);
				// Unpack the original builder's historical freight before choosing/staging the physical Surface shipment; later deployment must not mix that legacy inventory with newly delivered Goods.
				if (builder->cargo.TotalCount() != 0) REQUIRE(Command<Commands::MoveLaunchFreight>::Post(home.station, home.generation, builder->index, builder->generation, builder->cargo_type, static_cast<uint16_t>(builder->cargo.TotalCount()), true));
				const auto source_ref = FindLoopMember(*center, StationFacility::Train); auto *source = ResolveLaunchHubMember(home, source_ref); REQUIRE(source != nullptr);
				const auto stock = center->goods[good].TotalCount(); REQUIRE(stock != 0); REQUIRE(stock <= 1024); REQUIRE(source->goods[good].TotalCount() == 0);
				quantity = std::min<uint>({20, stock, GetSpaceFreightCapacity(20, good)}); REQUIRE(quantity != 0);
				const auto overflow_ref = FindLoopMember(*center, StationFacility::TruckStop);
				// Empty the real receiving inventory through timed outbound handling, keeping its original Goods in the separate source loading point before the actual train trip.
				while (center->goods[good].TotalCount() != 0) {
					const auto destination = source->goods[good].TotalCount() == 0 ? source_ref : overflow_ref;
					const auto remaining = center->goods[good].TotalCount(); const auto count = static_cast<uint16_t>(std::min<uint>(MAX_SURFACE_HANDLING_QUANTITY, destination == source_ref ? quantity : remaining));
					REQUIRE(Command<Commands::StartSurfaceHandling>::Post(destination.station, destination.generation, home.station, home.generation, good, count, false));
					advance_until([&] { return !HasSurfaceHandlingForStation(destination.station); }, 100, "outbound source staging");
				}
				REQUIRE(source->goods[good].TotalCount() == quantity); REQUIRE(source->goods[good].TotalCount() + Station::Get(overflow_ref.station)->goods[good].TotalCount() == stock);
				REQUIRE(Command<Commands::SetLaunchHub>::Post(source->index, StationID::Invalid(), uint64_t{0})); CHECK_FALSE(GetLaunchHubMembership(source->index));
				const auto depot = BuildLoopRail(*center, *source); train = BuyLoopTrain(depot, source->index, center->index, good, static_cast<uint16_t>(quantity)); const auto train_ref = train->index; wagon = FindLoopWagon(*train);
				advance_until([&] { return wagon->cargo.TotalCount() == quantity; }, 4500, "actual whole staged Goods pickup"); REQUIRE(quantity != 0);
				CHECK(wagon->cargo_type == good); CHECK(train->tile != depot);
				// Complete only after the original whole load leaves the physical train and appears once at the actual joined receiver; partial loading/unloading is not an arrival receipt.
				advance_until([&] { return wagon->cargo.TotalCount() == 0 && center->goods[good].TotalCount() == quantity; }, 4500, "actual Goods transfer delivery");
				REQUIRE(Command<Commands::StartStopVehicle>::Post(train->tile, train->index, false)); CHECK(train->vehstatus.Test(VehState::Stopped)); CHECK(train->index == train_ref);
				CHECK(center->goods[good].TotalCount() == quantity); CHECK(source->goods[good].TotalCount() == 0);
				fmt::print("COMMODITY_LOOP surface_vehicle={} delivered={} physical_travel=true transfer_order=true\n", train->index.base(), quantity);
			} else {
				train = find_train(); wagon = FindLoopWagon(*train); REQUIRE(train->vehstatus.Test(VehState::Stopped)); REQUIRE(wagon->cargo.TotalCount() == 0);
				if (stage == "deploy") {
					quantity = center->goods[good].TotalCount(); REQUIRE(quantity != 0); REQUIRE(builder->construction.placement == SpacePlacement::SurfaceStored);
					REQUIRE(builder->cargo.TotalCount() == 0); // Preparation already settled its historical Goods before the actual train arrival; deployment receives no extra legacy shipment.
					REQUIRE(builder->manifest != nullptr);
					if (builder->manifest->returns_remaining == 0) {
						// A zero historical quota must complete real paid Earth service; incoming inputs and a timer cannot be replaced by a fabricated return allowance.
						for (uint8_t supply : {1, 2}) if (builder->manifest->supplies[supply].TotalCount() == 0) REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, builder->index, builder->generation, supply, uint16_t{1}, false));
						REQUIRE(Command<Commands::ServiceLaunchProbe>::Post(builder->index, builder->generation, false));
						advance_until([&] { return builder->manifest->service_ticks == 0; }, 200, "paid Earth return service");
					}
					SpacePodRef shell{};
					// The existing paid stored shells are selected through actual command admission; incomplete, foreign or already assigned shells cannot be substituted.
					for (auto *pod : SpacePod::Iterate()) if (CmdMoveLaunchPod({}, home.station, home.generation, builder->index, builder->generation, pod->index, pod->generation, false).Succeeded()) { shell = {pod->index, pod->generation}; break; }
					REQUIRE(shell != SpacePodRef{}); REQUIRE(Command<Commands::MoveLaunchPod>::Post(home.station, home.generation, builder->index, builder->generation, shell.pod, shell.generation, false));
					const auto fuel = static_cast<uint16_t>(GetMultiLayerSpaceBudget().outbound_fuel + GetMultiLayerSpaceBudget().recovery_fuel);
					REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, uint8_t{0}, fuel, false));
					REQUIRE(Command<Commands::CarryLaunchProbe>::Post(rocket->index, rocket->generation, builder->index, builder->generation, false));
					const auto quote = CmdBeginLaunchFlight({}, rocket->index, rocket->generation, SpaceOrderTargetKind::LaunchCenter, home.station.base(), home.generation); REQUIRE(quote.Succeeded()); const auto money = company->money;
					REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::LaunchCenter, home.station.base(), home.generation)); CHECK(company->money == money - quote.GetCost());
					advance_until([&] { return !IsSpaceFlightActive(*rocket); }, 800, "paid original builder deployment");
					CHECK(builder->construction.placement == SpacePlacement::Orbital); CHECK(builder->manifest->pods.front() == shell); CHECK(center->goods[good].TotalCount() == quantity);
				} else if (stage == "resupply") {
					quantity = center->goods[good].TotalCount(); REQUIRE(quantity != 0); REQUIRE(builder->cargo.TotalCount() == 0); REQUIRE(builder->construction.placement == SpacePlacement::Orbital);
					REQUIRE(Command<Commands::MoveLaunchFreight>::Post(home.station, home.generation, rocket->index, rocket->generation, good, static_cast<uint16_t>(quantity), false)); REQUIRE(center->goods[good].TotalCount() == 0);
					const auto fuel = static_cast<uint16_t>(GetMultiLayerSpaceBudget().outbound_fuel + GetMultiLayerSpaceBudget().recovery_fuel);
					REQUIRE(Command<Commands::MoveSpaceSupply>::Post(home.station, home.generation, rocket->index, rocket->generation, uint8_t{0}, fuel, false));
					const auto quote = CmdBeginLaunchFlight({}, rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, builder->index.base(), builder->generation); REQUIRE(quote.Succeeded()); const auto money = company->money;
					REQUIRE(Command<Commands::BeginLaunchFlight>::Post(rocket->index, rocket->generation, SpaceOrderTargetKind::Craft, builder->index.base(), builder->generation)); CHECK(company->money == money - quote.GetCost());
					advance_until([&] { return !IsSpaceFlightActive(*rocket); }, 800, "actual original craft Goods resupply");
					CHECK(rocket->cargo.TotalCount() == 0); CHECK(builder->cargo_type == good); CHECK(builder->cargo.TotalCount() == quantity); CHECK(center->goods[good].TotalCount() == 0);
				} else {
					REQUIRE(stage == "return"); quantity = builder->cargo.TotalCount(); REQUIRE(quantity != 0); REQUIRE(builder->cargo_type == good); REQUIRE(builder->manifest->pods.size() == 1);
					const auto shell = builder->manifest->pods.front(); REQUIRE(CanReleaseOrbitalCargo(builder_ref, home, shell, static_cast<uint16_t>(quantity)));
					REQUIRE(Command<Commands::ReleaseOrbitalCargo>::Post(builder->index, builder->generation, home.station, home.generation, shell.pod, shell.generation, static_cast<uint16_t>(quantity)));
					CHECK(builder->cargo.TotalCount() == 0); advance_until([&] { return !HasSpaceDescentForCenter(home); }, 300, "actual Goods descent");
					REQUIRE(center->goods[good].TotalCount() == quantity); CHECK(ResolveSpacePod(shell) == nullptr);
					const auto member_ref = FindLoopMember(*center, StationFacility::TruckStop); auto *member = ResolveLaunchHubMember(home, member_ref); REQUIRE(member != nullptr); const auto previous = member->goods[good].TotalCount();
					REQUIRE(Command<Commands::StartSurfaceHandling>::Post(member_ref.station, member_ref.generation, home.station, home.generation, good, static_cast<uint16_t>(quantity), false));
					advance_until([&] { return !HasSurfaceHandlingForStation(member->index); }, 100, "separate lorry collection");
					CHECK(center->goods[good].TotalCount() == 0); CHECK(member->goods[good].TotalCount() == previous + quantity);
					fmt::print("COMMODITY_LOOP collected_station={} returned={} same_paid_pod=true separate_lorry=true\n", member->index.base(), quantity);
				}
			}
			CHECK(GetSpaceCraftReference(*rocket) == rocket_ref); CHECK(GetSpaceCraftReference(*builder) == builder_ref); CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetLaunchCenterReference(*center) == home);
			CHECK(company->cur_economy.delivered_cargo[good] == delivered); save("after");
			fmt::print("COMMODITY_LOOP native=true stage={} quantity={} rocket={} builder={} train={} final_goods_deliveries=0\n", stage, quantity, rocket_ref.vehicle.base(), builder_ref.vehicle.base(), train->index.base());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup hook(commodity_loop_observer, &observe); IConsole::CmdRegister("commodity_loop_test", ObserveCommodityLoop);
	const std::string source{input}, configuration = config.string();
	std::array<std::string_view, 17> args = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", configuration, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
