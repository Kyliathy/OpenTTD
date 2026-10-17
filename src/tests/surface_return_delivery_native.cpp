/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_return_delivery_native.cpp Paid returned ore reaches a useful native Surface refinery through actual linked rail and road transport.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group19
 * Original returned packets fund both collection members. Normal commands fund infrastructure/vehicles; native ticks earn construction and every trip.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../asteroid.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../launch_center_cmd.h"
#include "../station_base.h"
#include "../station_cmd.h"
#include "../station_map.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../roadveh.h"
#include "../rail_cmd.h"
#include "../road_cmd.h"
#include "../road_map.h"
#include "../track_func.h"
#include "../newgrf_station.h"
#include "../newgrf_roadstop.h"
#include "../underground_vehicle.h"
#include "../vehicle_cmd.h"
#include "../order_base.h"
#include "../order_cmd.h"
#include "../industry.h"
#include "../industry_cmd.h"
#include "../industry_map.h"
#include "../layer_industry_processor.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../console_internal.h"
#include "../core/backup_type.hpp"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include <set>
#include <thread>
#include "../safeguards.h"

static std::function<void()> *surface_delivery_observer = nullptr; ///< Observes native owners without granting stock, progress, terrain or movement.

/** @param args Parameter-free invocation. @return Whether the exact loaded-world observer ran. */
static bool ObserveSurfaceReturnDelivery(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_delivery_observer); (*surface_delivery_observer)(); return true;
}

/** @param start Actual Surface tile. @param direction Actual cardinal map direction. @param steps Bounded proposed route length.
 * @return Adjacent Surface route endpoint, or invalid when the proposal wraps or leaves its original layer.
 */
static TileIndex SurfaceRouteStep(TileIndex start, DiagDirection direction, uint steps)
{
	// Infrastructure planning must never wrap a map edge or use an identically projected Underground tile as a free Surface connection.
	for (uint step = 0; step < steps; ++step) {
		const auto next = AddTileIndexDiffCWrap(start, TileIndexDiffCByDiagDir(direction));
		// Layer admission and local adjacency together reject both stacked-plane escape and same-plane coordinate wrapping before ordinary command quotes.
		if (!MultiLayerMap::IsTileInLayer(next, MapLayer::Surface) || std::abs(static_cast<int>(TileX(next)) - static_cast<int>(TileX(start))) + std::abs(static_cast<int>(TileY(next)) - static_cast<int>(TileY(start))) != 1) return INVALID_TILE;
		start = next;
	}
	return start;
}

/** Actual paid infrastructure identities; no cargo or simulation state is duplicated. */
struct SurfaceReturnRoute {
	TileIndex depot = INVALID_TILE; ///< Original normal depot command's tile.
	StationID receiver = StationID::Invalid(); ///< Actual receiving station beside its useful processor.
	IndustryID factory = IndustryID::Invalid(); ///< Actual paid native refinery inventory owner.
};

/** @param factory Actual normally funded native facility. @return Whether every original industry tile earned construction completion through its native tile loop. */
static bool IsSurfaceReturnRefineryComplete(const Industry &factory)
{
	// Every actual footprint cell must finish before the refinery can accept paid returned ore; a completed anchor cannot stand in for unfinished neighboring cells.
	for (TileIndex tile : factory.location) {
		// Only this industry's own unfinished cells block intake; unrelated holes in its bounding rectangle do not belong to the funded construction job.
		if (factory.TileBelongsToIndustry(tile) && !IsIndustryCompleted(tile)) return false;
	}
	return true;
}

/** Explicitly connect two actual existing paid freight stations through the original synchronized hub command.
 * @param center Original paid recovery center; its current empty hub does not imply a link to nearby station tiles.
 */
static void LinkSurfaceReturnMembers(Station &center)
{
	const auto home = GetLaunchCenterReference(center); std::set<StationID> chosen;
	// Establish separate real rail/road relationships. Stable native pool order selects the last admitted existing member, giving each facility one explicit command-qualified link.
	for (auto facility : {StationFacility::Train, StationFacility::TruckStop}) {
		Station *candidate = nullptr;
		// Eligibility, original company authority and an unchanged command quote jointly govern membership; nearby same-XY or foreign facilities cannot substitute a real owned Surface link.
		for (auto *station : Station::Iterate()) {
			if (station == &center || station->owner != center.owner || chosen.contains(station->index) || !station->facilities.Test(facility)) continue;
			if (CmdSetLaunchHub({}, station->index, home.station, home.generation).Succeeded()) candidate = station;
		}
		REQUIRE(candidate); REQUIRE(Command<Commands::SetLaunchHub>::Post(candidate->index, home.station, home.generation)); chosen.insert(candidate->index);
	}
}

/** Fund a normal native refinery beside an admitted receiving station site.
 * @param receiver Proposed actual Surface receiving tile.
 * @param axis Straight transport corridor axis, kept clear of the4x4 factory.
 * @return Actual funded refinery, or nullptr when no ordinary construction quote admits the bounded candidates.
 */
static Industry *FundSurfaceReturnRefinery(TileIndex receiver, Axis axis)
{
	const auto type = GetLayerProcessorIndustryType(MultiLayerRecipeID{"CFMT"}); REQUIRE(type != IT_INVALID);
	// Bounded side sites keep the factory inside normal catchment and away from the proposed transport line; every cell and cost remains the ordinary industry's authority.
	for (int side : {-1, 1}) for (int along : {-1, 0, 1}) {
		const int x = static_cast<int>(TileX(receiver)) + (axis == Axis::X ? along : side > 0 ? 2 : -5);
		const int y = static_cast<int>(TileY(receiver)) + (axis == Axis::X ? side > 0 ? 2 : -5 : along);
		// The complete4x4 footprint must remain on the original Surface plane; clipping precedes tile construction and normal paid command admission.
		if (x < 1 || y < 1 || x + 4 >= static_cast<int>(Map::SizeX()) || y + 4 >= static_cast<int>(GetAsteroidField().height)) continue;
		const auto tile = TileXYL(x, y, MapLayer::Surface);
		if (CmdBuildIndustry({}, tile, type, 0, true, 123).Failed()) continue;
		REQUIRE(Command<Commands::BuildIndustry>::Post(tile, type, uint32_t{0}, true, uint32_t{123})); return Industry::GetByTile(tile);
	}
	return nullptr;
}

/** Build a paid straight route and a useful receiver using original public construction commands.
 * @param source Existing generation-qualified separate Surface collection member.
 * @param road True builds a real lorry route; false builds a real train route.
 * @return Actual paid depot, receiving station and native refinery identities.
 */
static SurfaceReturnRoute BuildSurfaceReturnRoute(const Station &source, bool road)
{
	const auto platform = road ? source.xy : source.train_station.tile; REQUIRE(platform != INVALID_TILE);
	const auto axis = road ? DiagDirToAxis(GetBayRoadStopDir(platform)) : GetRailStationAxis(platform);
	const auto first = road ? GetBayRoadStopDir(platform) : AxisToDiagDir(axis);
	const auto rail_flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRailLong>());
	const auto road_flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoadLong>());
	std::map<std::pair<std::string, uint32_t>, uint> refusals;
	/** @param action Actual quoted construction boundary. @param quote Original pure native result.
	 * @return Whether the unchanged native quote admits this candidate; retain exact refusal meanings for a failed bounded search.
	 */
	auto admitted = [&](std::string_view action, const CommandCost &quote) {
		if (quote.Failed()) ++refusals[{std::string{action}, quote.GetErrorMessage().base()}];
		return quote.Succeeded();
	};
	// Try only bounded actual straight corridors. Quotes reject occupied/water/sloped sites; neither terrain nor a vehicle pose is supplied by this fixture.
	for (auto direction : {first, ReverseDiagDir(first)}) for (uint length = 2; length <= 28; ++length) {
		if (road && direction != first) continue; // A bay's actual entrance fixes its road connection; its back wall is not another exit.
		const auto entrance = SurfaceRouteStep(platform, direction, 1), receiver = SurfaceRouteStep(platform, direction, length);
		// All proposed route endpoints must survive clipping before a native quote receives them; invalid planning cannot mutate a wrapped tile.
		if (entrance == INVALID_TILE || receiver == INVALID_TILE) continue;
		const auto last = SurfaceRouteStep(receiver, ReverseDiagDir(direction), 1);
		if (last == INVALID_TILE) continue;
		if (road) {
			if (!admitted("road-stop", CmdBuildRoadStop(road_flags, receiver, 1, 1, RoadStopType::Truck, false, ReverseDiagDir(direction), ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, true))) continue;
			if (!admitted("road-line", CmdBuildLongRoad(road_flags, last, entrance, ROADTYPE_ROAD, axis, {}, false, false, false))) continue;
		} else {
			if (!admitted("rail-station", CmdBuildRailStation(rail_flags, receiver, RAILTYPE_RAIL, axis, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, true))) continue;
			if (!admitted("rail-line", CmdBuildRailroadTrack(rail_flags, last, entrance, RAILTYPE_RAIL, AxisToTrack(axis), false, true))) continue;
		}
		// A rail depot lies behind its loading platform. Road depots search flat paid siding sites along the admitted corridor, rather than assuming the original bay's neighboring slopes can hold a depot.
		for (uint siding = 1; siding < (road ? length : 2); ++siding) for (auto side : {DiagDirection::NE, DiagDirection::SE, DiagDirection::SW, DiagDirection::NW}) {
			if (road && DiagDirToAxis(side) == axis) continue; // A side spur must leave the straight road corridor free for both real loading stops.
			const auto junction = SurfaceRouteStep(platform, direction, siding);
			const auto depot = road ? SurfaceRouteStep(junction, side, 1) : SurfaceRouteStep(platform, ReverseDiagDir(direction), 1);
			if (depot == INVALID_TILE) continue;
			const auto quote = road ? CmdBuildRoadDepot(road_flags, depot, ROADTYPE_ROAD, ReverseDiagDir(side)) : CmdBuildTrainDepot(rail_flags, depot, RAILTYPE_RAIL, direction);
			if (!admitted("depot", quote)) continue;
			if (road && CmdBuildRoad(road_flags, junction, ROAD_ALL, ROADTYPE_ROAD, {}, TownID::Invalid()).Failed()) continue; // The depot's side spur needs an ordinary paid intersection on the actual admitted corridor.
			auto *factory = FundSurfaceReturnRefinery(receiver, axis); if (!factory) continue;
			if (road) {
				REQUIRE(Command<Commands::BuildRoadLong>::Post(last, entrance, ROADTYPE_ROAD, axis, DisallowedRoadDirections{}, false, false, false));
				REQUIRE(Command<Commands::BuildRoad>::Post(junction, ROAD_ALL, ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid()));
				REQUIRE(Command<Commands::BuildRoadStop>::Post(receiver, uint8_t{1}, uint8_t{1}, RoadStopType::Truck, false, ReverseDiagDir(direction), ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, uint16_t{0}, NEW_STATION, true));
				REQUIRE(Command<Commands::BuildRoadDepot>::Post(depot, ROADTYPE_ROAD, ReverseDiagDir(side)));
			} else {
				REQUIRE(Command<Commands::BuildRailLong>::Post(last, entrance, RAILTYPE_RAIL, AxisToTrack(axis), false, true));
				REQUIRE(Command<Commands::BuildRailStation>::Post(receiver, RAILTYPE_RAIL, axis, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, true));
				REQUIRE(Command<Commands::BuildRailDepot>::Post(depot, RAILTYPE_RAIL, direction));
			}
			return {depot, GetStationIndex(receiver), factory->index};
		}
	}
	fmt::print("SURFACE_ROUTE source={} road={} platform={} axis={} direction={}\n", source.index.base(), road, platform.base(), to_underlying(axis), to_underlying(first));
	// Preserve original refusal boundaries for diagnosis; a failed search never clears terrain, relaxes authority or grants an alternate route.
	for (const auto &[key, count] : refusals) fmt::print("SURFACE_ROUTE refusal={} error={} count={}\n", key.first, key.second, count);
	return {};
}

/** Extend an actual linked station with a paid loading platform and a separately paid useful receiving route.
 * @param source Original native rail member whose existing platform is enclosed by paid launch/road facilities.
 * @return Actual command-built depot/receiver/refinery, or invalid identities when every unchanged native quote refuses.
 */
static SurfaceReturnRoute ExtendSurfaceReturnRailRoute(Station &source)
{
	const auto origin = source.train_station.tile; const auto flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRailLong>());
	AutoRestoreBackup distant_join(_settings_game.station.distant_join_stations, true); // Ordinary distant joining, as in the delivered Surface train producer; native spread/terrain/payment rules still govern every added tile.
	// Search only a small neighborhood of this actual station. A new command-funded platform provides physical access without demolishing its neighboring paid launch center or truck bay.
	for (int dy = -4; dy <= 4; ++dy) for (int dx = -4; dx <= 4; ++dx) for (auto axis : {Axis::X, Axis::Y}) {
		const int x = static_cast<int>(TileX(origin)) + dx, y = static_cast<int>(TileY(origin)) + dy;
		// The new platform must remain strictly inside Surface geometry before an ordinary joined-station quote can evaluate its real spread and foundation costs.
		if (x < 1 || y < 1 || x + 1 >= static_cast<int>(Map::SizeX()) || y + 1 >= static_cast<int>(GetAsteroidField().height)) continue;
		const auto platform = TileXYL(x, y, MapLayer::Surface);
		if (CmdBuildRailStation(flags, platform, RAILTYPE_RAIL, axis, 1, 1, STAT_CLASS_DFLT, 0, source.index, true).Failed()) continue;
		// Both real platform ends are candidates, with a normal paid depot behind and a short complete rail route ahead. Quotes retain every existing footprint and reject nonflat depot sites.
		for (auto direction : {AxisToDiagDir(axis), ReverseDiagDir(AxisToDiagDir(axis))}) for (uint length = 2; length <= 6; ++length) {
			const auto depot = SurfaceRouteStep(platform, ReverseDiagDir(direction), 1), entrance = SurfaceRouteStep(platform, direction, 1), receiver = SurfaceRouteStep(platform, direction, length);
			// All transport endpoints must be clipped before native station/depot/track authority sees them; wrapping can never provide a free route.
			if (depot == INVALID_TILE || entrance == INVALID_TILE || receiver == INVALID_TILE) continue;
			const auto last = SurfaceRouteStep(receiver, ReverseDiagDir(direction), 1);
			if (CmdBuildTrainDepot(flags, depot, RAILTYPE_RAIL, direction).Failed()) continue;
			if (CmdBuildRailroadTrack(flags, last, entrance, RAILTYPE_RAIL, AxisToTrack(axis), false, true).Failed()) continue;
			if (CmdBuildRailStation(flags, receiver, RAILTYPE_RAIL, axis, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, true).Failed()) continue;
			auto *factory = FundSurfaceReturnRefinery(receiver, axis); if (!factory) continue;
			REQUIRE(Command<Commands::BuildRailStation>::Post(platform, RAILTYPE_RAIL, axis, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, source.index, true));
			REQUIRE(Command<Commands::BuildRailLong>::Post(last, entrance, RAILTYPE_RAIL, AxisToTrack(axis), false, true));
			REQUIRE(Command<Commands::BuildRailStation>::Post(receiver, RAILTYPE_RAIL, axis, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, true));
			REQUIRE(Command<Commands::BuildRailDepot>::Post(depot, RAILTYPE_RAIL, direction));
			return {depot, GetStationIndex(receiver), factory->index};
		}
	}
	return {};
}

/** Buy and order one real terrestrial carrier with its normal native depot/refit authority.
 * @param route Actual command-built depot and useful receiver.
 * @param source Original linked collection station supplying exactly ten returned ore units.
 * @param road True selects native model7; false buys native4+5 and uses normal consist attachment.
 * @param cargo Original NORE meaning from the saved world.
 * @param name Persistent cosmetic name observing the actual saved primary vehicle.
 * @return The actual paid primary carrier; no direct pose or cargo assignment occurs.
 */
static Vehicle *BuySurfaceReturnCarrier(const SurfaceReturnRoute &route, StationID source, bool road, CargoType cargo, const std::string &name)
{
	Vehicle *front = nullptr;
	// Existing designs, normal payment and native automatic depot attachment create the complete carrier. A fresh unit cannot substitute an old native vehicle's cargo or orders.
	for (uint16_t model : road ? std::vector<uint16_t>{7} : std::vector<uint16_t>{4, 5}) {
		const auto engine = GetUndergroundEngine(model); const auto requested = model == 4 ? INVALID_CARGO : cargo; std::set<VehicleID> old;
		for (const auto *vehicle : Vehicle::Iterate()) old.insert(vehicle->index); // Retain actual pool identities solely to discover this normal purchase's fresh unit.
		REQUIRE(std::get<0>(CmdBuildVehicle({}, route.depot, engine, false, requested, ClientID{})).Succeeded());
		REQUIRE(Command<Commands::BuildVehicle>::Post(route.depot, engine, false, requested, ClientID{})); Vehicle *unit = nullptr;
		// Only this purchase's actual matching native unit may satisfy discovery; no private pool insertion or borrowed primary is allowed.
		for (auto *vehicle : Vehicle::Iterate()) if (!old.contains(vehicle->index) && vehicle->engine_type == engine) { REQUIRE(unit == nullptr); unit = vehicle; }
		REQUIRE(unit);
		if (model == 5) {
			REQUIRE(front); if (unit->First() != front) REQUIRE(Command<Commands::MoveRailVehicle>::Post(route.depot, unit->index, front->index, false)); REQUIRE(unit->First() == front);
		} else front = unit;
		if (model != 4) { REQUIRE(unit->cargo_type == cargo); REQUIRE(unit->cargo_cap >= 10); }
	}
	REQUIRE(front); REQUIRE(front->IsPrimaryVehicle()); REQUIRE(Command<Commands::RenameVehicle>::Post(front->index, name));
	Order pickup; pickup.MakeGoToStation(source); pickup.SetUnloadType(OrderUnloadType::NoUnload); pickup.SetLoadType(OrderLoadType::LoadIfPossible);
	Order delivery; delivery.MakeGoToStation(route.receiver); delivery.SetLoadType(OrderLoadType::NoLoad); // Default ordinary accepted unloading owns final delivery and payment; this is not a transfer order.
	// A road order must use the native generic stopping position; the default train-platform Middle position is rejected by ordinary road command validation.
	if (road) { pickup.SetStopLocation(OrderStopLocation::FarEnd); delivery.SetStopLocation(OrderStopLocation::FarEnd); }
	REQUIRE(Command<Commands::InsertOrder>::Post(front->tile, front->index, VehicleOrderID{0}, pickup)); REQUIRE(Command<Commands::InsertOrder>::Post(front->tile, front->index, VehicleOrderID{1}, delivery));
	REQUIRE(Command<Commands::StartStopVehicle>::Post(front->tile, front->index, false)); return front;
}

/** Real returned packets pass both native links and the selected physical carrier into a useful original factory input owner. */
TEST_CASE("Returned asteroid ore reaches a useful Surface refinery through real linked train and lorry trips", "[.surface-return-delivery-native]")
{
	const char *input = std::getenv("OTTD_SURFACE_RETURN_SOURCE"), *mode = std::getenv("OTTD_SURFACE_RETURN_MODE"), *selected = std::getenv("OTTD_SURFACE_RETURN_PHASE"); REQUIRE(input); REQUIRE(mode); REQUIRE(selected);
	const std::string_view phase{selected}; const bool road = std::string_view{mode} == "road"; const std::string name = road ? "Returned ore lorry" : "Returned ore train";
	const auto paths = GetTestCaseArtifactPaths("space", "surface-return-delivery"); const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Advance only original native owners; propagate all assertions after orderly application teardown. */
	std::function<void()> observe = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); AutoRestoreBackup running(_pause_mode, PauseModes{}); const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(85);
			const auto cargo = GetCargoTypeByLabel(CargoLabel{"NORE"}); REQUIRE(IsValidCargoType(cargo)); auto *miner = SpaceVehicle::GetIfValid(VehicleID{16}); REQUIRE(miner); REQUIRE(miner->generation == 2); REQUIRE(miner->extracted_units == 20); REQUIRE(miner->manifest); REQUIRE(miner->manifest->returns_remaining == 1);
			const auto home = miner->construction.home; auto *center = ResolveLaunchCenter(home); REQUIRE(center); REQUIRE(center->launch_hub); const auto fleet = GetSpaceVehicleCount(); const auto reserve = GetAsteroidField().bodies[0].remaining_reserve;
			/** @param predicate Actual native completion condition. @param limit Bounded opportunities. @param label Semantic failure context. */
			auto advance = [&](const std::function<bool()> &predicate, uint limit, std::string_view label) {
				// Actual game opportunities must earn construction, handling, loading and travel. Wall-clock pacing or estimated distance cannot replace a native receipt.
				for (uint count = 0; !predicate(); ++count) { INFO(label); REQUIRE(count < limit); REQUIRE(std::chrono::steady_clock::now() < deadline); const auto before = GetAsteroidField().elapsed_ticks; StateGameLoop(); if (GetAsteroidField().elapsed_ticks == before) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
			};
			if (phase == "setup") LinkSurfaceReturnMembers(*center);
			Station *member = nullptr;
			// Both original generation-qualified links retain their separate physical station owners; neither a joined center tile nor a coincident other layer qualifies.
			for (const auto ref : center->launch_hub->members) {
				auto *station = ResolveLaunchHubMember(home, ref); REQUIRE(station);
				if (station->facilities.Test(road ? StationFacility::TruckStop : StationFacility::Train)) { REQUIRE(member == nullptr); member = station; }
			}
			REQUIRE(member); Vehicle *carrier = nullptr;
			if (phase == "setup") {
				REQUIRE(center->goods[cargo].TotalCount() == 20); REQUIRE(CaptureLayerProcessors().empty());
				// Each real link receives half the actual returned hold before any carrier demand. Emptying the center prevents automatic demand from adding a second load during pickup.
				for (const auto ref : center->launch_hub->members) REQUIRE(Command<Commands::StartSurfaceHandling>::Post(ref.station, ref.generation, home.station, home.generation, cargo, uint16_t{10}, false));
				advance([&] { return center->launch_hub->handling.empty(); }, 200, "original timed collection"); REQUIRE(member->goods[cargo].TotalCount() == 10); REQUIRE(center->goods[cargo].TotalCount() == 0);
				auto route = BuildSurfaceReturnRoute(*member, road);
				// An enclosed original rail platform needs actual new paid access before it can serve a useful physical trip; this fallback funds ordinary infrastructure and creates no cargo or movement.
				if (!road && route.depot == INVALID_TILE) route = ExtendSurfaceReturnRailRoute(*member);
				REQUIRE(route.depot != INVALID_TILE); auto *factory = Industry::Get(route.factory);
				advance([&] { return IsSurfaceReturnRefineryComplete(*factory); }, 20000, "actual funded refinery construction");
				REQUIRE(Command<Commands::SelectLayerIndustryRecipe>::Post(factory->index, uint32_t{0x4E464D54})); REQUIRE(GetLayerProcessorInputRoom(*factory, cargo) >= 10);
				carrier = BuySurfaceReturnCarrier(route, member->index, road, cargo, name);
			} else {
				// Cosmetic name observes the exact command-built native primary saved by setup, without introducing another vehicle identity registry.
				for (auto *vehicle : Vehicle::Iterate()) if (vehicle->name == name) { REQUIRE(carrier == nullptr); carrier = vehicle; }
				REQUIRE(carrier);
			}
			const auto processors = CaptureLayerProcessors(); REQUIRE(processors.size() == 1); auto *factory = Industry::Get(processors.front().industry); REQUIRE(GetLayerProcessorState(factory->index)->recipe == MultiLayerRecipeID{"NFMT"});
			/** @return Actual physical cargo units across this paid primary's native consist; every body participates so an empty locomotive cannot hide its wagon's returned packets. */
			auto held = [&] { uint result = 0; for (auto *unit = carrier; unit; unit = unit->Next()) result += unit->cargo.TotalCount(); return result; };
			if (phase == "collect") { advance([&] { return held() == 10; }, 20000, "actual returned-ore pickup"); CHECK(member->goods[cargo].TotalCount() == 0); }
			else if (phase == "deliver") {
				REQUIRE(held() == 10);
				advance([&] { const auto *state = GetLayerProcessorState(factory->index); return factory->GetCargoAccepted(cargo)->waiting + state->consumed.at(CargoLabel{"NORE"}) == 10; }, 20000, "actual useful refinery delivery");
				// Final acceptance books a native CargoPayment first. Its actual normal destructor must settle company/vehicle income before this scenario claims completed delivery and an unchanged final resave.
				advance([&] { return carrier->cargo_payment == nullptr; }, 2000, "actual native final payment settlement");
				CHECK(held() == 0); CHECK(Company::Get(miner->owner)->cur_economy.delivered_cargo[cargo] == 10); CHECK(GetLayerProcessorState(factory->index)->consumed.at(CargoLabel{"NORE"}) == 0); CHECK(factory->GetCargoAccepted(cargo)->waiting == 10);
			} else if (phase != "setup") { REQUIRE(phase == "own"); REQUIRE(factory->GetCargoAccepted(cargo)->waiting == 10); REQUIRE(held() == 0); }
			CHECK(GetSpaceVehicleCount() == fleet); CHECK(GetAsteroidField().bodies[0].remaining_reserve == reserve); CHECK(miner->extracted_units == 20); CHECK(miner->cargo.TotalCount() == 0); CHECK(miner->manifest->returns_remaining == 1); REQUIRE(ValidateSpaceVehicles());
			{ AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"}); REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.saves / fmt::format("surface-return-{}-{}-438.sav", road ? "road" : "rail", phase)), false) == SaveLoadResult::Ok); }
			fmt::print("SURFACE_RETURN native=true mode={} phase={} carrier={} factory={} added_stock=false\n", road ? "road" : "rail", phase, carrier->index.base(), factory->index.base()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_exit_game = true;
	};
	AutoRestoreBackup hook(surface_delivery_observer, &observe); IConsole::CmdRegister("surface_return_delivery_test", ObserveSurfaceReturnDelivery); const std::string runtime = config.string();
	std::array<std::string_view, 17> args{"openttd_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", runtime, "-g", input};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
