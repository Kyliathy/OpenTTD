/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_sprite_load.cpp Native source/reservation setup and pure guideway/capsule views of retained actual work worlds.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups13B/13CB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "test_artifact_paths.h"
#include "layer_save_fixture.h"
#include "native_art_render_fixture.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../saveload/saveload_func.h"
#include "../transport_tube_sprite.h"
#include "../transport_tube.h"
#include "../transport_tube_cmd.h"
#include "../command_func.h"
#include "../transport_tube_geothermal.h"
#include "../geothermal_generator.h"
#include "../multilayer_sprite.h"
#include "../train.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../pbs.h"
#include "../core/random_func.hpp"
#include "../timer/timer_game_tick.h"
#include "../safeguards.h"

static std::function<void()> *_tube_guideway_art_observer = nullptr; ///< Isolated native startup observation, never a production renderer or simulation callback.

/**
 * Invoke declared native boundary setup and raster observation after ordinary owners have loaded.
 * @param arguments Exact zero-argument native startup command.
 * @return Whether the registered isolated observation ran.
 */
static bool ObserveTubeGuidewayArt(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_tube_guideway_art_observer != nullptr);
	(*_tube_guideway_art_observer)(); return true;
}

/**
 * Drive an original loaded capsule clear of its depot roof before a visible-body observation.
 * @return Actual selected native front on existing plain guideway, with its original packets/orders.
 * The original cable-wait file explicitly recovers through its already purchased external service;
 * this declared boundary action never rewrites position, speed, grants or packet inventories.
 */
static Train *PrepareVisibleTubeCapsule()
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); // Explicit native travel precedes pure paused capture; a saved pause cannot earn source work.
	const auto rows = CaptureTubePowerVehicles();
	const auto selected = std::ranges::find(rows, true, &TubePowerVehicle::active); REQUIRE(selected != rows.end());
	Train *capsule = Train::GetIfValid(selected->vehicle); REQUIRE(capsule != nullptr);
	if (selected->wait != TubePowerWait::None) {
		const auto connections = GetTubePowerConnections();
		const auto service = std::ranges::find(connections, true, &TubePowerConnection::active); REQUIRE(service != connections.end());
		REQUIRE(Command<Commands::SetTubeVehiclePower>::Do(DoCommandFlag::Execute, capsule->index, service->producer, true).Succeeded());
		fmt::print("TUBE_CAPSULE_SETUP explicit_original_external_recovery=true source={}\n", service->producer.base());
	}
	uint ticks = 0;
	// The retained checkpoints stop just beyond the native depot's hidden flag,
	// while its roof still covers the hull. Drive the real ordered front onto
	// the existing one-cell guideway instead of moving it by test coordinates.
	// Its native SE order must also carry the centre into the cell's middle;
	// stopping at the tile edge would still leave the full hull beneath a roof.
	while ((!IsPlainRailTile(capsule->tile) || capsule->y_pos % TILE_SIZE < TILE_SIZE / 2) && ticks < 256) {
		UpdateSignalsInBuffer(); ++TimerGameTick::counter; PrepareTubePowerTick();
		REQUIRE(capsule->Tick()); REQUIRE_FALSE(capsule->vehstatus.Test(VehState::Crashed)); ++ticks;
	}
	REQUIRE(IsPlainRailTile(capsule->tile)); REQUIRE(IsTubeRailType(GetRailType(capsule->tile)));
	REQUIRE_FALSE(capsule->vehstatus.Test(VehState::Hidden));
	fmt::print("TUBE_CAPSULE_SETUP native_travel_ticks={} tile={} speed={} visible=true\n", ticks, capsule->tile.base(), capsule->cur_speed);
	return capsule;
}

/** Actual ordinary work files render native tube casings while retaining complete original vehicle, accounting and packet owners. */
TEST_CASE("Native tube guideway viewport retains original loaded work owners", "[.tube-guideway-art-runtime]")
{
	const char *input = std::getenv("OTTD_TUBE_ART_INPUT"); REQUIRE(input != nullptr);
	const auto source = ResolveTestInputPath(std::filesystem::path{input});
	const auto config = GetTestRuntimeConfig();
	const char *capsule_text = std::getenv("OTTD_TUBE_ART_CAPSULE");
	// The capsule extension owns separate dated evidence; it must never replace
	// the accepted guideway receipt or silently reuse another run's image paths.
	const bool capsule_art = capsule_text != nullptr && std::string_view{capsule_text} == "1";
	const auto paths = GetTestArtifactPaths("tubes", capsule_art ? "capsule-art" : "guideway-art");
	bool observed = false; std::exception_ptr failure;
	/** Prepare requested native source/reservation fixtures, then capture pixels without further work or accounting changes. */
	std::function<void()> capture = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			const auto tile = TileXYL(32, 25, MapLayer::DeepUnderground);
			Train *capsule = capsule_art ? PrepareVisibleTubeCapsule() : nullptr;
			const char *close_text = std::getenv("OTTD_TUBE_ART_CLOSE_EXTERNAL");
			// Only the explicit blackout scene retires its remaining paid external
			// service. The original input stays immutable; closure is a real native
			// owner action before the strictly read-only raster boundary.
			const bool blackout = close_text != nullptr && std::string_view{close_text} == "1";
			if (blackout) {
				REQUIRE(GetTubeGuidewayArt(tile).has_value()); CHECK(GetTubeGuidewayArt(tile)->energized);
				const auto bindings = CaptureTubePowerVehicles();
				const auto active = std::ranges::find(bindings, true, &TubePowerVehicle::active); REQUIRE(active != bindings.end());
				if (!capsule_art) CHECK(active->wait == TubePowerWait::Topology);
				uint retired = 0;
				// A selected native capsule may wait while a separate paid external
				// source still energizes the same guideway. Close only actual active
				// original contracts; never erase a meter or manufacture zero output.
				for (const auto &connection : CaptureTubePowerConnections()) {
					if (!connection.active) continue;
					const auto used = GetUtilityPowerProducer(connection.producer)->consumed_lifetime_j;
					REQUIRE(CloseTubeExternalPower(connection.producer) == TubePowerError::None);
					CHECK(GetUtilityPowerProducer(connection.producer)->consumed_lifetime_j == used); ++retired;
				}
				REQUIRE(retired != 0);
				if (capsule != nullptr) {
					// Visible waiting is a real source-loss transition after the
					// declared recovery/travel, before the read-only render boundary.
					AutoRestoreBackup native_work(_pause_mode, PauseModes{});
					++TimerGameTick::counter; PrepareTubePowerTick(); REQUIRE(capsule->Tick());
					CHECK(capsule->cur_speed == 0); CHECK_FALSE(capsule->vehstatus.Test(VehState::Stopped));
				}
				CHECK(GetTubePowerVehicle(active->vehicle)->wait == TubePowerWait::Topology);
				fmt::print("TUBE_GUIDEWAY_ACTION close_external=true retired={} selected_native_wait=topology original_used_retained=true\n", retired);
			}
			const auto art = GetTubeGuidewayArt(tile); REQUIRE(art.has_value());
			const char *reservation_text = std::getenv("OTTD_TUBE_ART_RESERVATION");
			// Only the explicit diagnostic scene enables the existing client cue.
			// The retained waiting input has no plain-track reservation, so its
			// reservation owner prepares one declared native boundary fixture
			// before the strictly read-only render comparison begins.
			const bool reservation = reservation_text != nullptr && std::string_view{reservation_text} == "1";
			AutoRestoreBackup reservation_setting(_settings_client.gui.show_track_reservation, reservation);
			if (reservation) {
				REQUIRE_FALSE(GetRailReservationTrackBits(tile).Any());
				REQUIRE(TryReserveRailTrack(tile, Track::Y, false));
				const auto tracks = GetRailReservationTrackBits(tile); REQUIRE(tracks == TrackBits{Track::Y});
				fmt::print("TUBE_GUIDEWAY_RESERVATION tracks={} native_reservation_fixture=true\n", static_cast<uint>(tracks.base()));
			}
			const auto utility = ComputeUtilityPowerDigest(), bindings = ComputeTubePowerVehicleDigest();
			const auto native = ComputeGeothermalGeneratorDigest(), sources = ComputeTubeGeothermalPowerDigest(), invoices = ComputeTubePowerDigest();
			const auto cash = Company::Get(_current_company)->money; const auto tick = TimerGameTick::counter;
			SavedRandomSeeds before; SaveRandomSeeds(&before);
			AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
			AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
			const auto before_path = paths.Output("saves", "tube-guideway-before.sav");
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(before_path), false) == SaveLoadResult::Ok);
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			std::optional<SpriteID> capsule_sprite;
			if (capsule_art) {
				const auto rows = CaptureTubePowerVehicles();
				const auto selected = std::ranges::find(rows, true, &TubePowerVehicle::active); REQUIRE(selected != rows.end());
				capsule = Train::GetIfValid(selected->vehicle); REQUIRE(capsule != nullptr);
				const auto capability = GetTubeVehicleCapability(capsule->engine_type); REQUIRE(capability.has_value());
				CHECK(capability->catalogue->model_id == 9); CHECK(capsule->cargo.StoredCount() == 17);
				CHECK_FALSE(capsule->IsInDepot());
				const Direction facing = capsule->flags.Test(VehicleRailFlag::Flipped) ? ReverseDir(capsule->direction) : capsule->direction;
				const uint8_t state = blackout ? 7 : 1;
				const uint8_t frame = static_cast<uint8_t>((tick / 8) % 2);
				capsule_sprite = GetMultiLayerSprite(MultiLayerSpriteAsset::VehicleModel09, {state, frame, static_cast<uint8_t>(facing)});
				REQUIRE(capsule_sprite.has_value());
				CHECK(GetTubeTrainSprite(*capsule, facing, tick) == capsule_sprite);
				VehicleSpriteSeq image; capsule->GetImage(capsule->direction, EngineImageType::OnMap, &image);
				REQUIRE(image.count == 1); CHECK(image.seq[0].sprite == *capsule_sprite);
				// Request the real renderer's existing bounded refresh without a
				// simulation opportunity. GetImage continues using the original tick;
				// the rounded argument only opens the existing eight-tick cache gate.
				RefreshMultiLayerTrainArt(*capsule, tick - tick % 8);
			}
			// Both actual zooms use DrawTile_Rail and the native spatial sorter.
			// Repeated identical views must retain every RGB pixel and native owner;
			// a hidden-layer camera cannot turn presentation into an energy tick.
			for (const auto zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
				for (uint repeat = 0; repeat < 2; ++repeat) {
					CaptureNativeArtViewport(tile, zoom, paths.images / fmt::format("tube-guideway-zoom-{}-{}.ppm", to_underlying(zoom), repeat));
					if (capsule != nullptr) CaptureNativeArtViewport(capsule->tile, zoom, paths.images / fmt::format("tube-capsule-zoom-{}-{}.ppm", to_underlying(zoom), repeat));
				}
			}
			if (capsule != nullptr) {
				REQUIRE(capsule->sprite_cache.sprite_seq.count == 1);
				CHECK(capsule->sprite_cache.sprite_seq.seq[0].sprite == *capsule_sprite);
				CHECK_FALSE(capsule->sprite_cache.revalidate_before_draw);
				const Direction facing = capsule->flags.Test(VehicleRailFlag::Flipped) ? ReverseDir(capsule->direction) : capsule->direction;
				fmt::print("TUBE_CAPSULE_ART vehicle={} model=9 state={} view={} frame={} sprite={} cargo={} tile={} cache_exact=true\n",
					capsule->index.base(), blackout ? 7 : 1, to_underlying(facing), (tick / 8) % 2, *capsule_sprite, capsule->cargo.StoredCount(), capsule->tile.base());
				const Point origin = RemapCoords(TileX(capsule->tile) * TILE_SIZE + TILE_SIZE / 2,
					TileY(capsule->tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(capsule->tile));
				const Point position = RemapCoords(capsule->x_pos + capsule->bounds.origin.x + capsule->bounds.offset.x,
					capsule->y_pos + capsule->bounds.origin.y + capsule->bounds.offset.y, capsule->z_pos + capsule->bounds.origin.z + capsule->bounds.offset.z);
				fmt::print("TUBE_CAPSULE_ANCHOR dx={} dy={} native_bounds=true\n", (position.x - origin.x) / static_cast<int>(ZOOM_BASE), (position.y - origin.y) / static_cast<int>(ZOOM_BASE));
			}
			REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			const auto after_path = paths.Output("saves", "tube-guideway-after.sav");
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(after_path), false) == SaveLoadResult::Ok);
			SavedRandomSeeds after; SaveRandomSeeds(&after);
			CHECK(std::ranges::equal(before.random.state, after.random.state));
			CHECK(std::ranges::equal(before.interactive_random.state, after.interactive_random.state));
			CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubePowerVehicleDigest() == bindings);
			CHECK(ComputeGeothermalGeneratorDigest() == native); CHECK(ComputeTubeGeothermalPowerDigest() == sources); CHECK(ComputeTubePowerDigest() == invoices);
			CHECK(Company::Get(_current_company)->money == cash); CHECK(TimerGameTick::counter == tick);
			fmt::print("TUBE_GUIDEWAY_ART tile={} energized={} count={} back={} front={} before={} after={}\n",
				tile.base(), art->energized, art->count, art->parts[0].back, art->parts[0].front, before_path.string(), after_path.string());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_tube_guideway_art_observer, &capture);
	IConsole::CmdRegister("tube_guideway_art", ObserveTubeGuidewayArt);
	const std::string config_path = config.string(), source_path = source.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}
