/**
 * @file ground_transport_runtime_fixture.h Actual command-built carriers for native art observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHC.
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md.
 * Floor, packet loads, positions and movement/hold boundaries are explicit
 * isolated art fixtures; they establish no earned transport or route operation.
 */

#ifndef TESTS_GROUND_TRANSPORT_RUNTIME_FIXTURE_H
#define TESTS_GROUND_TRANSPORT_RUNTIME_FIXTURE_H

#include "ground_industry_runtime_fixture.h"
#include "../vehicle_cmd.h"
#include "../vehicle_gui.h"
#include "../train_cmd.h"
#include "../rail_cmd.h"
#include "../road_cmd.h"
#include "../train.h"
#include "../roadveh.h"
#include "../cargopacket.h"
#include "../depot_map.h"
#include "../rail_map.h"
#include "../road_map.h"
#include "../ground_train_sprite.h"
#include "../ground_road_sprite.h"
#include "../timer/timer_game_calendar.h"

/**
 * Snapshot actual packet, route, movement, company, calendar and RNG ownership around drawing.
 * @param vehicle Real command-created carrier; no presentation state is returned as authority.
 * @return Equality-comparable native owner fields that rendering must preserve exactly.
 */
static auto GroundTransportDrawOwnership(const Vehicle &vehicle)
{
	return std::tuple{*vehicle.cargo.Packets(), vehicle.cargo.StoredCount(), vehicle.cargo_type,
		vehicle.orders, vehicle.current_order.GetType(), vehicle.current_order.GetDestination(),
		vehicle.cur_real_order_index, vehicle.dest_tile, vehicle.cur_speed, vehicle.vehstatus,
		Company::Get(_current_company)->money, TimerGameCalendar::date, TimerGameTick::counter,
		_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
}

/**
 * Find and open a complete clear native fixture area without replacing retained facilities.
 * @return Actual Underground anchor with an eight-by-eight disclosed fixture floor.
 */
static TileIndex FindGroundTransportFixtureFloor()
{
	// Search real map ownership before staging floor. Complete enclosing space
	// keeps every observed vehicle away from retained mines, collectors and portals.
	for (uint y = 8; y + 8 < Map::SizeY(); y += 8) {
		for (uint x = 8; x + 8 < Map::SizeX(); x += 8) {
			std::vector<TileIndex> floor;
			bool clear = true;
			for (uint dy = 0; dy < 8; ++dy) {
				for (uint dx = 0; dx < 8; ++dx) {
					const auto tile = TileXYL(x + dx, y + dy, MapLayer::Underground);
					if (!IsTileType(tile, TileType::Clear)) clear = false;
					floor.push_back(tile);
				}
			}
			if (!clear) continue;
			OpenGroundIndustryFixtureFloor(floor);
			return TileXYL(x, y, MapLayer::Underground);
		}
	}
	FAIL("No complete unoccupied native transport fixture floor");
	return INVALID_TILE;
}

/**
 * Stage actual native packet custody without creating a presentation inventory.
 * @param vehicle Actual command-created carrier whose cargo list owns every packet.
 * @param count Whole cargo units for this explicit empty/partial/full boundary.
 * @param depot Actual owned depot used as the disclosed loading origin.
 */
static void LoadGroundTransportFixture(Vehicle &vehicle, uint16_t count, TileIndex depot)
{
	vehicle.cargo.Truncate();
	if (count == 0) return;
	REQUIRE(CargoPacket::CanAllocateItem());
	auto *packet = CargoPacket::Create(StationID::Invalid(), count, Source{Source::Invalid, SourceType::Industry});
	packet->UpdateLoadingTile(depot);
	vehicle.cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
}

/**
 * Position one actual native carrier for a draw-only boundary without simulating a journey.
 * @param vehicle Actual command-created vehicle; its native position and viewport hashes remain authoritative.
 * @param tile Actual fixture floor or depot tile.
 * @param direction Authored native world view, independent of cargo capability.
 * @param state Source boundary: empty0, partial1, full2, service3 or blocked4.
 */
static void PlaceGroundTransportFixture(Vehicle &vehicle, TileIndex tile, Direction direction, uint8_t state)
{
	vehicle.tile = tile;
	vehicle.direction = direction;
	vehicle.x_pos = TileX(tile) * TILE_SIZE + TILE_SIZE / 2;
	vehicle.y_pos = TileY(tile) * TILE_SIZE + TILE_SIZE / 2;
	vehicle.z_pos = TilePixelHeight(tile);
	vehicle.vehstatus.Reset(VehState::Hidden);
	vehicle.vehstatus.Reset(VehState::Stopped);
	vehicle.cur_speed = state < 3 ? 8 : 0;
	if (vehicle.type == VehicleType::Train) {
		auto &train = *Train::From(&vehicle);
		train.track = state == 3 ? Track::Depot : Track::X;
		train.flags.Reset(VehicleRailFlag::Flipped);
		if (state == 4) train.flags.Set(VehicleRailFlag::Stuck);
		else train.flags.Reset(VehicleRailFlag::Stuck);
	} else {
		auto &road = *RoadVehicle::From(&vehicle);
		road.state = state == 3 ? RVSB_IN_DEPOT : to_underlying(Trackdir::X_NE);
		road.blocked_ctr = state == 4 ? 5 : 0;
	}
	vehicle.UpdatePosition();
	if (vehicle.type == VehicleType::Train) Train::From(&vehicle)->UpdateViewport(true, true);
	else RoadVehicle::From(&vehicle)->UpdateViewport(true, true);
	vehicle.sprite_cache.revalidate_before_draw = true;
}

/**
 * Draw actual body map/list boundaries for one real EIDS design and native packet load.
 * @param paths This actual process's isolated artifact homes.
 * @param road Whether the selected command-built carriers are road7/8 or rail4/5/6.
 * @return Actual native frame metadata; source pixel qualification follows independently.
 */
static nlohmann::json CaptureGroundTransportBodies(const TestArtifactPaths &paths, bool road)
{
	const auto floor = FindGroundTransportFixtureFloor();
	const auto depot = floor;
	const auto display = TileAddWrap(floor, 4, 4);
	REQUIRE((road ? CmdBuildRoadDepot(DoCommandFlag::Execute, depot, ROADTYPE_ROAD, DiagDirection::SE) :
		CmdBuildTrainDepot(DoCommandFlag::Execute, depot, RAILTYPE_RAIL, DiagDirection::SE)).Succeeded());
	REQUIRE((road ? CmdBuildRoad(DoCommandFlag::Execute, display, ROAD_X, ROADTYPE_ROAD, {}, TownID::Invalid()) :
		CmdBuildSingleRail(DoCommandFlag::Execute, display, RAILTYPE_RAIL, Track::X, false)).Succeeded());
	nlohmann::json frames = nlohmann::json::array();
	AutoRestoreBackup<TimerGameTick::TickCounter> clock(TimerGameTick::counter);
	const auto phase_origin = (TimerGameTick::counter / 16) * 16;
	// Native definitions, company/date admission and command construction own
	// every physical vehicle. No local IDs or synthetic stand-in vehicles are used.
	for (uint16_t model = road ? 7 : 4; model <= (road ? 8 : 6); ++model) {
		const auto engine = GetUndergroundEngine(model);
		REQUIRE(Engine::IsValidID(engine));
		AutoRestoreBackup availability(Engine::Get(engine)->company_avail);
		Engine::Get(engine)->company_avail.Set(_current_company);
		const auto [cost, id, capacity, mail, capacities] = CmdBuildVehicle(DoCommandFlag::Execute, depot, engine, false, INVALID_CARGO, ClientID::Invalid);
		INFO(cost.GetErrorMessage().base());
		REQUIRE(cost.Succeeded());
		REQUIRE(Vehicle::IsValidID(id));
		auto *vehicle = Vehicle::Get(id);
		REQUIRE(vehicle->First() == vehicle);
		REQUIRE(vehicle->Next() == nullptr);
		const auto palette_id = GetVehiclePalette(vehicle);
		nlohmann::json remap = nlohmann::json::array();
		const auto *colours = GetNonSprite(palette_id, SpriteType::Recolour) + 1;
		for (uint index = 0; index < 256; ++index) remap.push_back(colours[index]);
		std::vector<CargoLabel> labels;
		if (model == 4) labels = {CargoSpec::Get(vehicle->cargo_type)->label};
		if (model == 5) labels = {CargoLabel{"COAL"}, CargoLabel{"IORE"}, CargoLabel{"CORE"}, CargoLabel{"BAUX"}, CargoLabel{"SALT"},
			CargoLabel{"LITH"}, CargoLabel{"NORE"}, CargoLabel{"COBT"}, CargoLabel{"RARE"}, CargoLabel{"RFMT"}, CargoLabel{"STEL"},
			CargoLabel{"SCMP"}, CargoLabel{"RBPT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}, CargoLabel{"SMIK"}, CargoLabel{"GOOD"}};
		if (model == 7) labels = {CargoLabel{"COAL"}, CargoLabel{"IORE"}, CargoLabel{"CORE"}, CargoLabel{"BAUX"}, CargoLabel{"SALT"},
			CargoLabel{"LITH"}, CargoLabel{"NORE"}, CargoLabel{"COBT"}, CargoLabel{"ERTH"}, CargoLabel{"ROCK"}};
		if (model == 6) labels = {CargoLabel{"URAN"}, CargoLabel{"RARE"}};
		if (model == 8) labels = {CargoLabel{"URAN"}, CargoLabel{"RARE"}};
		// Every accepted source cargo meaning is admitted by the actual depot
		// refit command. Extra variants need partial/full map/list samples; the
		// first variant additionally exhausts all state, direction and phase axes.
		for (size_t variant = 0; variant < labels.size(); ++variant) {
			vehicle->cargo.Truncate();
			PlaceGroundTransportFixture(*vehicle, depot, Direction::W, 3);
			vehicle->vehstatus.Set(VehState::Stopped);
			const auto cargo = GetCargoTypeByLabel(labels[variant]);
			REQUIRE(IsValidCargoType(cargo));
			// The retained model4 is the zero-capacity powered hauler. Native
			// wagons own its freight; refitting or loading the engine itself
			// would fabricate custody that the actual definition does not grant.
			if (model != 4) {
				REQUIRE(vehicle->cargo_cap > 1);
				REQUIRE(std::get<0>(CmdRefitVehicle(DoCommandFlag::Execute, id, cargo, 0, false, true, 1)).Succeeded());
			} else {
				REQUIRE(vehicle->cargo_cap == 0);
			}
			REQUIRE(vehicle->cargo_type == cargo);
			const auto label = CargoSpec::Get(cargo)->label;
			const std::string cargo_label(label.begin(), label.end());
			// Five reachable presentation states span both authored phases and all
			// eight native directions. Native packet counts retain priority over holds.
			for (uint8_t state = variant == 0 ? 0 : 1; state < (variant == 0 ? 5 : 3); ++state) {
				// A powered zero-capacity hauler has motion/service/idle artwork;
				// only the real freight carriers have partial/full packet states.
				if (model == 4 && (state == 1 || state == 2)) continue;
				LoadGroundTransportFixture(*vehicle, state == 1 ? 1 : state == 2 ? vehicle->cargo_cap : 0, depot);
				const auto count = vehicle->cargo.StoredCount();
				for (uint8_t phase = 0; phase < (variant == 0 ? 2 : 1); ++phase) {
					TimerGameTick::counter = phase_origin + phase * 8;
					for (uint8_t view = variant == 0 ? 0 : 6; view < (variant == 0 ? 8 : 7); ++view) {
						const auto tile = state == 3 ? depot : display;
						PlaceGroundTransportFixture(*vehicle, tile, static_cast<Direction>(view), state);
						const auto basename = fmt::format("body-model-{:02}-c{}-s{}-p{}-d{}", model, variant, state, phase, view);
						const auto money = Company::Get(_current_company)->money;
						const auto random = _random;
						const auto interactive = _interactive_random;
						const auto owners = GroundTransportDrawOwnership(*vehicle);
						// Native sprite placement includes the existing physical sorting
						// origin and contact offset. Retain that geometry explicitly;
						// tile-centre coordinates alone are not a vehicle draw baseline.
						const auto contact = RemapCoords(vehicle->bounds.origin.x + vehicle->bounds.offset.x,
							vehicle->bounds.origin.y + vehicle->bounds.offset.y, vehicle->bounds.origin.z + vehicle->bounds.offset.z);
						CaptureNativeArtViewport(tile, ZoomLevel::Normal, paths.images / (basename + ".ppm"), paths.images / (basename + ".idx"));
						CHECK(GroundTransportDrawOwnership(*vehicle) == owners);
						CHECK(vehicle->cargo.StoredCount() == count);
						CHECK(Company::Get(_current_company)->money == money);
						CHECK(_random.state[0] == random.state[0]);
						CHECK(_random.state[1] == random.state[1]);
						CHECK(_interactive_random.state[0] == interactive.state[0]);
						CHECK(_interactive_random.state[1] == interactive.state[1]);
						CHECK(TimerGameTick::counter == phase_origin + phase * 8);
						frames.push_back({{"asset_id", fmt::format("vehicle.model-{:02}", model)}, {"model", model},
							{"vehicle", id.base()}, {"engine", engine.base()}, {"cargo", vehicle->cargo_type}, {"cargo_label", cargo_label}, {"load", count},
							{"state_index", state}, {"frame", phase}, {"direction", view}, {"basename", basename},
							{"palette", palette_id}, {"remap", remap}, {"baseline", {320 + contact.x / static_cast<int>(ZOOM_BASE), 180 + contact.y / static_cast<int>(ZOOM_BASE)}},
							{"native_sorting_origin", {vehicle->bounds.origin.x, vehicle->bounds.origin.y, vehicle->bounds.origin.z}},
							{"native_contact_offset", {vehicle->bounds.offset.x, vehicle->bounds.offset.y, vehicle->bounds.offset.z}}, {"tile", tile.base()}});
					}
				}
				TimerGameTick::counter = phase_origin;
				PlaceGroundTransportFixture(*vehicle, state == 3 ? depot : display, Direction::W, state);
				Point offset;
				if (road) RoadVehicle::From(vehicle)->GetDisplayImageWidth(&offset);
				else Train::From(vehicle)->GetDisplayImageWidth(&offset);
				const auto basename = fmt::format("list-model-{:02}-c{}-s{}", model, variant, state);
				/** Draw the real list chain renderer; its clipping and native baseline remain authoritative. */
				const std::function<void()> paint = [&] {
					DrawVehicleImage(vehicle, Rect{64, 0, 223, 79}, VehicleID::Invalid(), EngineImageType::InList, 0);
				};
				const auto packets = *vehicle->cargo.Packets();
				const auto owners = GroundTransportDrawOwnership(*vehicle);
				CaptureNativeArtCanvas(256, 80, paths.images / (basename + ".ppm"), paint);
				CHECK(GroundTransportDrawOwnership(*vehicle) == owners);
				CHECK(*vehicle->cargo.Packets() == packets);
				CHECK(vehicle->cargo.StoredCount() == count);
				frames.push_back({{"asset_id", fmt::format("vehicle.model-{:02}", model)}, {"model", model},
					{"vehicle", id.base()}, {"engine", engine.base()}, {"cargo", vehicle->cargo_type}, {"cargo_label", cargo_label}, {"load", count},
					{"state_index", state}, {"frame", 0}, {"direction", 6}, {"basename", basename}, {"context", "list"},
					{"palette", palette_id}, {"remap", remap}, {"baseline", {64 + offset.x, 40 + offset.y}}});
			}
		}
		delete vehicle;
	}
	return frames;
}

#endif /* TESTS_GROUND_TRANSPORT_RUNTIME_FIXTURE_H */
