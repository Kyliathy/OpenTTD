/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_facility_sprite_load.cpp Genuine native facility scenes and pure loaded-world viewport observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups13CC/13DCC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared geological floor/materials and explicit stock/wear are boundary inputs, separate from bootstrap acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include <charconv>
#include "test_artifact_paths.h"
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "native_art_render_fixture.h"
#include "geothermal_tube_switch_fixture.h"
#include "../transport_tube_sprite.h"
#include "../transport_tube_cmd.h"
#include "../tile_cmd.h"
#include "../vehicle_gui.h"
#include "../multilayer_sprite.h"
#include "../station_cmd.h"
#include "../station_supply.h"
#include "../station_map.h"
#include "../station_func.h"
#include "../rail.h"
#include "../newgrf_station.h"
#include "../openttd.h"
#include "../animated_tile_map.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../safeguards.h"

/**
 * Read one bounded process fixture value without inferring identities from a camera or basename.
 * @param name Explicit decimal native environment field supplied by the owning runner.
 * @return Complete checked unsigned value; malformed text refuses this test.
 */
static uint64_t TubeFacilityExpected(const char *name)
{
	const char *text = std::getenv(name); REQUIRE(text != nullptr);
	uint64_t value = 0; const auto parsed = std::from_chars(text, text + strlen(text), value);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == text + strlen(text)); return value;
}

/** @param scene Actual native boundary. @return Whether ready/service/closed-source0/1/5 uses the original depot rather than a platform. */
static bool IsTubeFacilityDepotScene(uint64_t scene) { return scene <= 1 || scene == 5; }

/**
 * Resolve the declared native scene's original packed family.
 * @param scene Bounded service0/1/5, terminal2/4 or joined interchange3 boundary.
 * @return Exact already packed source identity, with no construction or purchase permission.
 */
static MultiLayerSpriteAsset TubeFacilitySceneAsset(uint64_t scene)
{
	if (IsTubeFacilityDepotScene(scene)) return MultiLayerSpriteAsset::FacilityTubeService;
	return scene == 3 ? MultiLayerSpriteAsset::FacilityTubeInterchange : MultiLayerSpriteAsset::FacilityTubeTerminal;
}

/**
 * Snapshot actual complete source/vehicle/accounting owners independently of the display cache.
 * @return Native/shared digests, counts, money, simulation tick and both original RNG streams.
 */
static auto CaptureTubeFacilityWorldOwners()
{
	return std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), ComputeTubePowerVehicleDigest(),
		ComputeTubeGeothermalPowerDigest(), ComputeGeothermalGeneratorDigest(), CargoPacket::GetNumItems(), Station::GetNumItems(),
		Company::Get(_current_company)->money, TimerGameTick::counter,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
}

/**
 * Write an ordinary complete current native world without advancing any owner.
 * @param path New repository-owned native scene/render-boundary output.
 */
static void SaveTubeFacilityArtWorld(const std::filesystem::path &path)
{
	AutoRestoreBackup menu(_game_mode, GameMode::Menu);
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Actual command-built depots/platforms, paid native output and original packets produce independent ordinary scenes. */
TEST_CASE("Native tube facilities write actual service platform and interchange scenes", "[tube-facility-art-write]")
{
	const auto scene = TubeFacilityExpected("OTTD_TUBE_FACILITY_SCENE"); REQUIRE(scene <= 5);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	// Normal native initialization installs the four original rail labels
	// before allocating UTUB. An isolated empty table otherwise gives tubes
	// slot0, so requesting ordinary RAILTYPE_RAIL would build another tube berth.
	ResetRailTypes(); GeothermalTubeSwitchRig rig{true};
	REQUIRE(GetTubeRailType() != RAILTYPE_RAIL);
	AutoRestoreBackup freight(_settings_game.vehicle.freight_trains, uint8_t{1});
	const auto coal = GetCargoTypeByLabel(CT_COAL); REQUIRE(coal != INVALID_CARGO);
	CargoSpec::Get(coal)->weight = 16; CargoSpec::Get(coal)->is_freight = true;
	LayerSaveContent content{rig.ActivatedContent(), true}; AutoRestoreBackup local(_local_company, _current_company);
	AutoRestoreBackup native_work(_pause_mode, PauseModes{});
	REQUIRE(Station::GetNumItems() == 0); StationClass::Reset(); RebuildStationKdtree();
	(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
	const auto platform = TileXYL(32, 29, MapLayer::DeepUnderground);
	// The original one-cell power-switch yard puts a depot roof over the
	// platform's far status glass. Actually clear its unused destination and
	// rebuild a longer paid native route; never move a vehicle, float a lamp
	// through an opaque roof or relax the exact nine-pixel status assertion.
	for (int y : {25, 26}) {
		const auto branch = TileXYL(32, y, MapLayer::DeepUnderground);
		REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, branch, UtilityEnergyDomain::Electricity).Succeeded());
	}
	REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false,
		rig.inlets[1], std::make_tuple(rig.inlets[1])).Succeeded());
	// Explicit fixture floor remains disclosed input. Every actual added rail
	// and destination building goes through the ordinary paid command kernel;
	// the selected original electrical inlet/path stays unchanged at32,24.
	for (int y = 25; y <= 32; ++y) {
		const auto tile = TileXYL(32, y, MapLayer::DeepUnderground); rig.Open(std::array{tile});
		if (y == 32) {
			REQUIRE(IsTileType(tile, TileType::Clear));
			REQUIRE(Command<Commands::BuildRailDepot>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
				std::make_tuple(tile, GetTubeRailType(), DiagDirection::NW)).Succeeded());
			rig.inlets[1] = tile; // Fixture handle follows the actual clear/rebuild result; no physical or saved identity is edited.
		} else if (IsTileType(tile, TileType::Clear)) {
			REQUIRE(Command<Commands::BuildRail>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
				std::make_tuple(tile, GetTubeRailType(), Track::Y, false)).Succeeded());
		} else {
			REQUIRE(IsPlainRailTile(tile)); REQUIRE(GetTrackBits(tile) == TrackBits{Track::Y});
		}
	}
	REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, platform, GetTubeRailType(), Axis::Y, 1, 1, STAT_CLASS_DFLT, 0, NEW_STATION, false).Succeeded());
	Station *station = Station::GetByTile(platform);
	if (scene == 3) {
		AutoRestoreBackup distant(_settings_game.station.distant_join_stations, true);
		const auto berth = TileXYL(28, 29, MapLayer::DeepUnderground);
		REQUIRE(IsTileType(berth, TileType::Clear));
		rig.Open(std::array{berth, TileAddWrap(berth, 0, -1), TileAddWrap(berth, 0, 1)});
		REQUIRE(CmdBuildRailStation(DoCommandFlag::Execute, berth, RAILTYPE_RAIL, Axis::Y, 1, 1, STAT_CLASS_DFLT, 0, station->index, false).Succeeded());
		CHECK(GetStationIndex(berth) == station->index); CHECK(GetRailType(berth) == RAILTYPE_RAIL);
		CHECK_FALSE(IsTubeRailType(GetRailType(berth))); CHECK_FALSE(GetTubeFacilityArt(berth, TimerGameTick::counter).has_value());
	}
	Train *capsule = rig.Buy(); const auto packet = rig.Load(*capsule);
	REQUIRE(BindTubeGeothermalPowerVehicle(capsule->index, rig.geothermal.source).has_value());
	rig.Run(std::array{capsule});
	uint8_t state = 1;
	if (scene == 1) {
		// Declared wear requests a real pending native repair. No train tick
		// follows preparation, so this scene retains the original unused work
		// grant rather than pretending a parked healthy capsule is being serviced.
		capsule->reliability = 100; ++TimerGameTick::counter; PrepareTubePowerTick(); state = 2;
		REQUIRE(GetTubePowerPhase(*capsule) == TubePowerPhase::DepotService);
		REQUIRE(GetTubePowerVehicle(capsule->index)->phase == TubePowerPhase::DepotService);
		REQUIRE(GetTubePowerVehicle(capsule->index)->wait == TubePowerWait::None);
	} else if (scene == 2) {
		Order order; order.MakeGoToStation(station->index);
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, capsule->index, 0, order).Succeeded()); ProcessOrders(capsule);
		REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, capsule->index, true).Succeeded());
		uint ticks = 0;
		// Only actual native arrival may select loading art. Neither the writer
		// nor the later viewport observer rewrites position, order phase or speed.
		while (!capsule->current_order.IsType(OT_LOADING) && ticks < 800) { rig.Run(std::array{capsule}); ++ticks; }
		REQUIRE(capsule->current_order.IsType(OT_LOADING)); REQUIRE(capsule->tile == platform); state = 2;
	} else if (scene == 4) {
		// One genuine native packet declares receiver pressure at the shared
		// bound. This is explicit test stock, not produced bootstrap inventory.
		auto &queue = station->goods[coal].GetOrCreateData().cargo; REQUIRE(CargoPacket::CanAllocateItem());
		queue.Append(CargoPacket::Create(station->index, EXACT_STATION_SUPPLY_CAPACITY, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid()); state = 3;
	} else if (scene == 5) {
		// Real owner commands retire both installed offers before pure capture.
		// The original plant/history/packet survive; no private visual power bit
		// or fabricated empty meter can stand in for a genuine source loss.
		REQUIRE(Command<Commands::CloseTubePower>::Do(DoCommandFlag::Execute, rig.external).Succeeded());
		REQUIRE(Command<Commands::CloseTubeGeothermalPower>::Do(DoCommandFlag::Execute, rig.geothermal.source).Succeeded());
		state = 3;
	}
	const auto tile = IsTubeFacilityDepotScene(scene) ? rig.inlets[0] : platform;
	const uint8_t view = IsTubeFacilityDepotScene(scene) ? static_cast<uint8_t>(GetRailDepotDirection(tile)) : static_cast<uint8_t>(GetRailStationAxis(tile));
	const uint16_t role = IsTubeFacilityDepotScene(scene) ? 0 : GetStationGfx(tile) / 2;
	const uint8_t frame = static_cast<uint8_t>((TimerGameTick::counter / 8) % 4);
	const auto art = GetTubeFacilityArt(tile, TimerGameTick::counter); REQUIRE(art.has_value());
	CHECK(art->sprites.back == *GetMultiLayerSprite(TubeFacilitySceneAsset(scene), {state, frame, view, role, 0}));
	CHECK(capsule->cargo.StoredCount() == 17); CHECK(capsule->cargo.Packets()->front()->index == packet);
	REQUIRE(GetAnimatedTileState(tile) == AnimatedTileState::Animated);
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE); // The ordinary save UI requires its native cursor even in this isolated writer; it owns no simulation state.
	const auto owners = CaptureTubeFacilityWorldOwners();
	const auto paths = GetTestCaseArtifactPaths("tubes", "facility-art");
	const auto path = paths.Output("saves", fmt::format("tube-facility-scene-{}.sav", scene));
	SaveTubeFacilityArtWorld(path); CHECK(CaptureTubeFacilityWorldOwners() == owners);
	fmt::print("TUBE_FACILITY_SAVE scene={} tile={} state={} view={} role={} frame={} back={} front={} tick={} packet={} vehicle={} utility={} invoices={} binding={} geothermal={} generator={} path={}\n",
		scene, tile.base(), state, view, role, frame, art->sprites.back, art->sprites.front, TimerGameTick::counter, packet.base(), capsule->index.base(),
		std::get<0>(owners), std::get<1>(owners), std::get<2>(owners), std::get<3>(owners), std::get<4>(owners), path.string());
}

static std::function<void()> *_tube_facility_art_observer = nullptr; ///< Process-local startup observation, never a gameplay callback or second owner.

/**
 * Invoke the registered pure native facility viewport observation after ordinary loading.
 * @param arguments Exact zero-argument native startup command.
 * @return Whether the isolated registered observer ran.
 */
static bool ObserveTubeFacilityArt(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_tube_facility_art_observer != nullptr);
	(*_tube_facility_art_observer)(); return true;
}

/**
 * Verify native exposed/hidden body selection without changing a source or a station order.
 * @param capsule Actual native packet owner; depot hiding is respected by the original picker.
 * @param zoom Native screenshot scale, with correct and wrong-layer probes at the same real location.
 */
static void PickTubeFacilityBody(const Train &capsule, ZoomLevel zoom)
{
	Viewport view{}; view.width = 640; view.height = 360; view.zoom = zoom; view.layer = GetVehicleDisplayLayer(&capsule);
	const Point position = RemapCoords(capsule.x_pos, capsule.y_pos, capsule.z_pos);
	const bool hidden = capsule.vehstatus.Test(VehState::Hidden);
	// A hidden depot body can have no rendered bounds. Probe its actual physical
	// position instead of inventing visible coordinates or exposing its sprite.
	view.virtual_left = (hidden ? position.x : (capsule.coord.left + capsule.coord.right) / 2) - ScaleByZoom(320, zoom);
	view.virtual_top = (hidden ? position.y : (capsule.coord.top + capsule.coord.bottom) / 2) - ScaleByZoom(180, zoom);
	const auto *picked = CheckClickOnVehicle(view, 320, 180);
	if (hidden) {
		CHECK((picked == nullptr || picked->First() != &capsule)); // Native depot hiding must prevent selection even at the stored body's true location.
	} else {
		REQUIRE(picked != nullptr); CHECK(picked->First() == &capsule);
	}
	view.layer = MapLayer::Surface; const auto *other = CheckClickOnVehicle(view, 320, 180);
	CHECK((other == nullptr || other->First() != &capsule)); // Matching projection cannot select a Deep body through the Surface view.
}

/** Actual ordinary scenes retain every native packet/source/animation owner through repeated full viewport captures. */
TEST_CASE("Native tube facility viewport preserves original complete work owners", "[.tube-facility-art-runtime]")
{
	const char *input = std::getenv("OTTD_TUBE_FACILITY_INPUT"); REQUIRE(input != nullptr);
	const auto scene = TubeFacilityExpected("OTTD_TUBE_FACILITY_SCENE"); REQUIRE(scene <= 5);
	const auto tile_value = TubeFacilityExpected("OTTD_TUBE_FACILITY_TILE"), state = TubeFacilityExpected("OTTD_TUBE_FACILITY_STATE");
	REQUIRE(tile_value < UINT32_MAX); REQUIRE(state < 4);
	const auto tile = TileIndex{static_cast<uint32_t>(tile_value)};
	const auto source = ResolveTestInputPath(std::filesystem::path{input});
	const auto config = GetTestRuntimeConfig(); const auto paths = GetTestArtifactPaths("tubes", "facility-art");
	bool observed = false; std::exception_ptr failure;
	/** Capture actual loaded native parts without any source publication, stock loading or work tick. */
	std::function<void()> capture = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			const auto terminal = GetBuiltTubeTerminal(tile, _current_company); REQUIRE(terminal.has_value());
			const uint8_t view = terminal->depot != DepotID::Invalid() ? static_cast<uint8_t>(GetRailDepotDirection(tile)) : static_cast<uint8_t>(GetRailStationAxis(tile));
			const uint16_t role = terminal->depot != DepotID::Invalid() ? 0 : GetStationGfx(tile) / 2;
			const uint8_t frame = static_cast<uint8_t>((TimerGameTick::counter / 8) % 4);
			const auto art = GetTubeFacilityArt(tile, TimerGameTick::counter); REQUIRE(art.has_value());
			CHECK(art->sprites.back == *GetMultiLayerSprite(TubeFacilitySceneAsset(scene), {static_cast<uint8_t>(state), frame, view, role, 0}));
			CHECK(art->sprites.front == *GetMultiLayerSprite(TubeFacilitySceneAsset(scene), {static_cast<uint8_t>(state), frame, view, role, 1}));
			REQUIRE(GetAnimatedTileState(tile) == AnimatedTileState::Animated);
			const auto owners = CaptureTubeFacilityWorldOwners();
			AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
			fmt::print("TUBE_FACILITY_DISPLAY reservation_cue={} reserved={}\n", _settings_client.gui.show_track_reservation,
				terminal->station != StationID::Invalid() ? HasStationReservation(tile) : false);
			AutoRestoreBackup reservation_cue(_settings_client.gui.show_track_reservation, false); // This art fixture explicitly disables the optional client reservation overlay; its native ownership and separate diagnostic scope remain unchanged.
			const auto before = paths.Output("saves", "tube-facility-before.sav"); SaveTubeFacilityArtWorld(before);
			REQUIRE(AnimateTubeFacilityArt(tile));
			const auto vehicle = TubeFacilityExpected("OTTD_TUBE_FACILITY_VEHICLE"); REQUIRE(vehicle < UINT32_MAX);
			Train *capsule = Train::GetIfValid(VehicleID{static_cast<uint32_t>(vehicle)}); REQUIRE(capsule != nullptr);
			CHECK(capsule->index.base() == TubeFacilityExpected("OTTD_TUBE_FACILITY_VEHICLE"));
			CHECK(capsule->cargo.StoredCount() == 17); REQUIRE(capsule->cargo.Packets()->size() == 1);
			CHECK(capsule->cargo.Packets()->front()->index.base() == TubeFacilityExpected("OTTD_TUBE_FACILITY_PACKET"));
			CHECK(TimerGameTick::counter == TubeFacilityExpected("OTTD_TUBE_FACILITY_TICK"));
			CHECK(ComputeUtilityPowerDigest() == TubeFacilityExpected("OTTD_TUBE_FACILITY_UTILITY"));
			CHECK(ComputeTubePowerDigest() == TubeFacilityExpected("OTTD_TUBE_FACILITY_INVOICES"));
			CHECK(ComputeTubePowerVehicleDigest() == TubeFacilityExpected("OTTD_TUBE_FACILITY_BINDING"));
			CHECK(ComputeTubeGeothermalPowerDigest() == TubeFacilityExpected("OTTD_TUBE_FACILITY_GEOTHERMAL"));
			CHECK(ComputeGeothermalGeneratorDigest() == TubeFacilityExpected("OTTD_TUBE_FACILITY_GENERATOR"));
			// Late accepted power references must select the actual capsule's
			// current image even when this paused load earns no train opportunity.
			RefreshMultiLayerTrainArt(*capsule, TimerGameTick::counter - TimerGameTick::counter % 8);
			const Point origin = RemapCoords(TileX(tile) * TILE_SIZE + TILE_SIZE / 2, TileY(tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(tile));
			const Point position = RemapCoords(capsule->x_pos + capsule->bounds.origin.x + capsule->bounds.offset.x,
				capsule->y_pos + capsule->bounds.origin.y + capsule->bounds.offset.y, capsule->z_pos + capsule->bounds.origin.z + capsule->bounds.offset.z);
			const Direction facing = capsule->flags.Test(VehicleRailFlag::Flipped) ? ReverseDir(capsule->direction) : capsule->direction;
			fmt::print("TUBE_FACILITY_CAPSULE dx={} dy={} view={} frame={} visible={}\n", (position.x - origin.x) / static_cast<int>(ZOOM_BASE),
				(position.y - origin.y) / static_cast<int>(ZOOM_BASE), to_underlying(facing), (TimerGameTick::counter / 8) % 2, !capsule->vehstatus.Test(VehState::Hidden));
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			// Both native zooms use actual depot/station drawing and spatial
			// sorting. Repeated frames cannot advance animation metadata or work.
			for (const auto zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
				for (uint repeat = 0; repeat < 2; ++repeat) {
					CaptureNativeArtViewport(tile, zoom, paths.images / fmt::format("tube-facility-zoom-{}-{}.ppm", to_underlying(zoom), repeat));
				}
				PickTubeFacilityBody(*capsule, zoom);
			}
			// Use the real native tile click after raster capture, so a window
			// cannot contaminate pixels or impersonate the actual selected owner.
			REQUIRE(ClickTile(tile));
			const auto window_class = terminal->station == StationID::Invalid() ? WindowClass::VehicleDepot : WindowClass::StationView;
			const auto number = terminal->station == StationID::Invalid() ? tile.base() : terminal->station.base();
			Window *selected = FindWindowById(window_class, number); REQUIRE(selected != nullptr);
			selected->Close();
			fmt::print("TUBE_FACILITY_PICK scene={} native_window=true body_hidden={} correct_layer=true wrong_layer_refused=true\n", scene, capsule->vehstatus.Test(VehState::Hidden));
			REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			const auto after = paths.Output("saves", "tube-facility-after.sav"); SaveTubeFacilityArtWorld(after);
			CHECK(CaptureTubeFacilityWorldOwners() == owners);
			fmt::print("TUBE_FACILITY_ART scene={} tile={} state={} view={} role={} frame={} back={} front={} before={} after={}\n",
				scene, tile.base(), state, view, role, frame, art->sprites.back, art->sprites.front, before.string(), after.string());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_tube_facility_art_observer, &capture);
	IConsole::CmdRegister("tube_facility_art", ObserveTubeFacilityArt);
	const std::string config_path = config.string(), source_path = source.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
