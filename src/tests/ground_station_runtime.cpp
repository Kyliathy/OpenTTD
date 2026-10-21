/**
 * @file ground_station_runtime.cpp Actual native default station/terminal source and foreground observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79FHCC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Native construction/removal, packet, loading-list and phase boundaries are
 * disclosed isolated art fixtures, without transported or economic acceptance.
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../3rdparty/nlohmann/json.hpp"
#include "test_artifact_paths.h"
#include "native_art_render_fixture.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../underground_vehicle.h"
#include "../engine_func.h"
#include "../engine_base.h"
#include "../vehicle_func.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../timer/timer_game_tick.h"
#include "../core/random_func.hpp"
#include "ground_transport_runtime_fixture.h"
#include "../station_cmd.h"
#include "../station_func.h"
#include "../station_base.h"
#include "../ground_station_sprite.h"
#include "../transparency.h"

#include "../safeguards.h"

static std::function<void()> *ground_station_observer = nullptr; ///< This process's explicit loaded-world callback.

/**
 * Invoke this isolated process's loaded native station observation.
 * @param arguments Parameter-free native startup command.
 * @return True after the supplied observation callback executes.
 */
static bool ObserveGroundStation(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1);
	REQUIRE(ground_station_observer != nullptr);
	(*ground_station_observer)();
	return true;
}

/**
 * Construct a real default module through native placement and selective native removal.
 * @param road Whether the requested module is a freight truck stop rather than rail.
 * @param graphics Native rail role/axis0..7 or truck entrance/through0..5.
 * @return Actual remaining native station tile, retaining its original command-owned graphics.
 */
static TileIndex BuildGroundStationModule(bool road, uint8_t graphics)
{
	const auto floor = FindGroundTransportFixtureFloor();
	const auto anchor = TileAddWrap(floor, 1, 1);
	if (road) {
		const bool through = graphics >= 4;
		const auto entrance = static_cast<DiagDirection>(through ? graphics - 4 : graphics);
		REQUIRE(CmdBuildRoadStop(DoCommandFlag::Execute, anchor, 1, 1, RoadStopType::Truck, through, entrance,
			ROADTYPE_ROAD, ROADSTOP_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
		REQUIRE(GetStationGfx(anchor) == graphics);
		return anchor;
	}
	const auto axis = static_cast<Axis>(graphics & 1);
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, anchor, RAILTYPE_RAIL, axis, 3, 5, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	const auto *station = Station::GetByTile(anchor);
	std::vector<TileIndex> tiles;
	TileIndex selected = INVALID_TILE;
	// The ordinary3x5 layout supplies all four native roles. Select a real
	// command-built module, then use ordinary demolition for its neighbours;
	// no private tile graphics or local replacement station identity is invented.
	for (const auto tile : station->train_station) {
		tiles.push_back(tile);
		if (GetStationGfx(tile) == graphics) selected = tile;
	}
	REQUIRE(selected != INVALID_TILE);
	for (const auto tile : tiles) {
		if (tile == selected) continue;
		REQUIRE(CmdRemoveFromRailStation(DoCommandFlag::Execute, tile, tile, true).Succeeded());
	}
	REQUIRE(IsRailStationTile(selected));
	REQUIRE(GetStationGfx(selected) == graphics);
	return selected;
}

/**
 * Acquire the actual conventional carrier used by one type's foreground fixture.
 * @param road Whether to use the native open truck7 instead of mineral wagon5.
 * @return Command-created carrier and its actual owned native depot tile.
 */
static std::pair<Vehicle *, TileIndex> BuyGroundStationCarrier(bool road)
{
	const auto depot = FindGroundTransportFixtureFloor();
	REQUIRE((road ? CmdBuildRoadDepot(DoCommandFlag::Execute, depot, ROADTYPE_ROAD, DiagDirection::SE) :
		CmdBuildTrainDepot(DoCommandFlag::Execute, depot, RAILTYPE_RAIL, DiagDirection::SE)).Succeeded());
	const auto engine = GetUndergroundEngine(road ? 7 : 5);
	REQUIRE(Engine::IsValidID(engine));
	AutoRestoreBackup availability(Engine::Get(engine)->company_avail);
	Engine::Get(engine)->company_avail.Set(_current_company);
	const auto [cost, id, capacity, mail, capacities] = CmdBuildVehicle(DoCommandFlag::Execute, depot, engine, false, INVALID_CARGO, ClientID::Invalid);
	REQUIRE(cost.Succeeded());
	REQUIRE(Vehicle::IsValidID(id));
	auto *vehicle = Vehicle::Get(id);
	REQUIRE(vehicle->cargo_cap > 1);
	REQUIRE(vehicle->Next() == nullptr);
	LoadGroundTransportFixture(*vehicle, 1, depot);
	return {vehicle, depot};
}

/**
 * Capture actual current station roles and native transparency/vehicle loading boundaries.
 * @param paths This process's isolated review directories.
 * @return Complete native scene metadata, without asserting independent pixel qualification yet.
 */
static nlohmann::json CaptureGroundStationModules(const TestArtifactPaths &paths)
{
	fmt::print(stderr, "GROUND_STATION_PROGRESS begin_modules\n");
	nlohmann::json frames = nlohmann::json::array();
	AutoRestoreBackup transparency(_transparency_opt);
	AutoRestoreBackup invisibility(_invisibility_opt);
	AutoRestoreBackup display(_display_opt);
	_display_opt.Reset(DisplayOption::ShowStationNames); // Native label preference keeps signs from covering the source-pixel observation.
	AutoRestoreBackup clock(TimerGameTick::counter);
	const auto origin = (TimerGameTick::counter / 32) * 32;
	// Each family uses one actual carrier. A real station loading list and its
	// matching physical tile alone select Working; phase sampling performs no work.
	for (const bool road : {false, true}) {
		fmt::print(stderr, "GROUND_STATION_PROGRESS buy_carrier road={}\n", road);
		const auto [vehicle, depot] = BuyGroundStationCarrier(road);
		for (uint8_t graphics = 0; graphics < (road ? 6 : 8); ++graphics) {
			fmt::print(stderr, "GROUND_STATION_PROGRESS build_module road={} graphics={}\n", road, graphics);
			const auto tile = BuildGroundStationModule(road, graphics);
			fmt::print(stderr, "GROUND_STATION_PROGRESS module_ready road={} graphics={}\n", road, graphics);
			auto *station = Station::GetByTile(tile);
			REQUIRE(station->loading_vehicles.empty());
			for (const bool working : {false, true}) {
				station->loading_vehicles.clear();
				const auto direction = road ? DiagDirToDir(static_cast<DiagDirection>(graphics < 4 ? graphics : graphics - 4)) :
					(graphics & 1 ? Direction::SE : Direction::NE);
				PlaceGroundTransportFixture(*vehicle, working ? tile : depot, direction, 0);
				// A source-qualified rail position near the platform's far end
				// exercises real glass/vehicle overlap on both canopy-front axes.
				// Retained ordinary rail underneath neighbouring removed modules
				// supports the carrier's physical overhang; no movement is earned.
				if (working && !road) {
					vehicle->x_pos = TileX(tile) * TILE_SIZE + (graphics & 1 ? 8 : 2);
					vehicle->y_pos = TileY(tile) * TILE_SIZE + (graphics & 1 ? 2 : 8);
					vehicle->UpdatePositionAndViewport();
				}
				vehicle->cur_speed = 0;
				if (working) station->loading_vehicles.push_back(vehicle);
				else vehicle->vehstatus.Set(VehState::Hidden);
				// Atomic native construction has only Ready and Working installed
				// states. Construction/blocked source poses remain preview-only.
				for (uint8_t phase = 0; phase < (working ? 4 : 1); ++phase) {
					TimerGameTick::counter = origin + phase * 8;
					const auto layout = GetGroundStationTileLayout(tile, TimerGameTick::counter);
					REQUIRE(layout.has_value());
					nlohmann::json parts = nlohmann::json::array();
					// Retain every actual parent/child and its native geometry; source
					// qualification must preserve roof attachment and absent parents.
					for (uint8_t index = 0; index < layout->count; ++index) {
						const auto &part = layout->sequence[index];
						parts.push_back({{"index", index}, {"parent", part.IsParentSprite()}, {"image", part.image.sprite}, {"palette", part.image.pal},
							{"opaque", HasBit(part.image.sprite, SPRITE_MODIFIER_OPAQUE)}, {"transparent", HasBit(part.image.sprite, PALETTE_MODIFIER_TRANSPARENT)},
							{"native_sprite_offset", (part.image.sprite & SPRITE_MASK) - SPR_MAP_LAYER_BASE},
							{"origin", {part.origin.x, part.origin.y, part.origin.z}}, {"extent", {part.extent.x, part.extent.y, part.extent.z}},
							{"offset", {part.offset.x, part.offset.y, part.offset.z}}});
					}
					const auto vehicle_palette = GetVehiclePalette(vehicle);
					nlohmann::json vehicle_remap = nlohmann::json::array();
					const auto *vehicle_colours = GetNonSprite(vehicle_palette, SpriteType::Recolour) + 1;
					for (uint index = 0; index < 256; ++index) vehicle_remap.push_back(vehicle_colours[index]);
					const auto &bounds = vehicle->bounds;
					const int local_x = vehicle->x_pos - static_cast<int>(TileX(tile) * TILE_SIZE) + bounds.origin.x + bounds.offset.x;
					const int local_y = vehicle->y_pos - static_cast<int>(TileY(tile) * TILE_SIZE) + bounds.origin.y + bounds.offset.y;
					const int local_z = bounds.origin.z + bounds.offset.z;
					const auto label = CargoSpec::Get(vehicle->cargo_type)->label;
					const nlohmann::json body{{"asset_id", road ? "vehicle.model-07" : "vehicle.model-05"},
						{"direction", to_underlying(direction)}, {"frame", 0}, {"state", "partial"}, {"cargo_label", std::string(label.begin(), label.end())},
						{"load", vehicle->cargo.StoredCount()}, {"palette", vehicle_palette}, {"remap", vehicle_remap},
						{"baseline", {320 + 2 * (local_y - local_x), 164 + local_x + local_y - local_z}},
						{"local_position", {vehicle->x_pos - static_cast<int>(TileX(tile) * TILE_SIZE), vehicle->y_pos - static_cast<int>(TileY(tile) * TILE_SIZE)}},
						{"origin", {bounds.origin.x, bounds.origin.y, bounds.origin.z}}, {"extent", {bounds.extent.x, bounds.extent.y, bounds.extent.z}},
						{"offset", {bounds.offset.x, bounds.offset.y, bounds.offset.z}}};
					for (uint8_t appearance = 0; appearance < 3; ++appearance) {
						_transparency_opt.Reset(TransparencyOption::Buildings);
						_invisibility_opt.Reset(TransparencyOption::Buildings);
						if (appearance != 0) _transparency_opt.Set(TransparencyOption::Buildings);
						if (appearance == 2) _invisibility_opt.Set(TransparencyOption::Buildings);
						const auto basename = fmt::format("{}-g{}-{}-p{}-a{}", road ? "terminal" : "station", graphics, working ? "working" : "ready", phase, appearance);
						const auto owners = GroundTransportDrawOwnership(*vehicle);
						const auto loading = station->loading_vehicles;
						const auto stock = station->goods[vehicle->cargo_type].TotalCount();
						CaptureNativeArtViewport(tile, ZoomLevel::Normal, paths.images / (basename + ".ppm"), paths.images / (basename + ".idx"));
						CHECK(GroundTransportDrawOwnership(*vehicle) == owners);
						CHECK(station->loading_vehicles == loading);
						CHECK(station->goods[vehicle->cargo_type].TotalCount() == stock);
						frames.push_back({{"asset_id", road ? "facility.road-terminal" : "facility.rail-station"}, {"graphics", graphics},
							{"state", working ? "working" : "ready"}, {"frame", phase}, {"appearance", appearance}, {"basename", basename},
							{"tile", tile.base()}, {"station", station->index.base()}, {"vehicle", vehicle->index.base()},
							{"vehicle_visible", working}, {"body", body}, {"parts", parts}, {"fixture_native_neighbour_removal", !road},
							{"native_rail_preserved_under_removed_modules", !road}});
					}
				}
			}
			station->loading_vehicles.clear();
		}
		delete vehicle;
	}
	return frames;
}

/** Actual current native station construction and loading owners produce the complete fourteen-module scene matrix. */
TEST_CASE("Current default station observer captures real native loading and foreground boundaries", "[.ground-station-runtime]")
{
	const char *source_value = std::getenv("OTTD_GROUND_STATION_SOURCE");
	REQUIRE(source_value != nullptr);
	const auto source = ResolveTestInputPath(std::filesystem::path{source_value});
	const auto paths = GetTestArtifactPaths("selected-ground", "stations");
	const auto config = GetTestRuntimeConfig();
	bool observed = false;
	std::exception_ptr failure;
	/** Observe actual current modules while retaining headless cleanup on every assertion failure. */
	std::function<void()> capture = [&] {
		try {
			fmt::print(stderr, "GROUND_STATION_PROGRESS startup_callback\n");
			REQUIRE(GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry);
			REQUIRE(Company::IsValidID(_local_company));
			AutoRestoreBackup actor(_current_company, _local_company);
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			const auto frames = CaptureGroundStationModules(paths);
			REQUIRE(frames.size() == 210);
			nlohmann::json palette = nlohmann::json::array();
			for (const auto &colour : _cur_palette.palette) palette.push_back({colour.r, colour.g, colour.b});
			nlohmann::json transparent_remap = nlohmann::json::array();
			const auto *transparent_colours = GetNonSprite(PALETTE_TO_TRANSPARENT, SpriteType::Recolour) + 1;
			for (uint index = 0; index < 256; ++index) transparent_remap.push_back(transparent_colours[index]);
			nlohmann::json report{{"_documentation", "@see r2mlvs14-remaining-assets-and-runtime-integration.md79FHCC; actual native modules; source pixels pending"},
				{"frames", frames}, {"palette", palette}, {"transparent_remap", transparent_remap},
				{"drawing_owners_unchanged", true}, {"fixture_floor_packets_position_loading_phase", true},
				{"source_pixel_verification", "pending"}, {"transported_bootstrap_claimed", false}};
			std::ofstream output(paths.reports / "station-modules.json", std::ios::binary);
			output << report.dump(2) << '\n';
			output.close();
			REQUIRE(output.good());
			fmt::print("GROUND_STATION_MODULES rail_graphics=8 terminal_graphics=6 frames=210 actual_loading_owner=true\n");
		} catch (...) {
			failure = std::current_exception();
		}
		BlitterFactory::SelectBlitter("null");
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		observed = true;
		_exit_game = true;
	};
	AutoRestoreBackup observer(ground_station_observer, &capture);
	IConsole::CmdRegister("ground_station_art", ObserveGroundStation);
	const std::string config_path = config.string();
	const std::string source_path = source.string();
	std::array<std::string_view, 17> arguments{"openttd_ground_viewport_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple",
		"-I", "original_dos", "-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", source_path};
	fmt::print(stderr, "GROUND_STATION_PROGRESS start_main\n");
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
