/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file forge_useful_raster_fixture.h Actual original moving Forge viewport/picking and complete-owner purity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FEE.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84CB; optional real purchase previews share this world's full owner/RNG boundary.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * The caller supplies the same complete native driver as the accepted useful writer; rendering creates no work.
 */
#ifndef TESTS_FORGE_USEFUL_RASTER_FIXTURE_H
#define TESTS_FORGE_USEFUL_RASTER_FIXTURE_H
#include "forge_useful_world_snapshot.h"
#include "native_art_render_fixture.h"
#include "selected_purchase_view_fixture.h" // Plan14-84CB optional purchase art borrows this actual paid 2005 model world.
#include "layer_save_fixture.h"
#include "../transport_tube_sprite.h"
#include "../vehicle_gui.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../saveload/saveload_func.h"
#include "../station_map.h"

/** @param world Original actual native participants. @return Complete canonical observations and independent unsaved client RNG for pure render/save boundaries. */
inline auto ForgeRasterOwners(const ForgeUsefulWorld &world)
{
	return std::pair{CaptureForgeUsefulWorld(world), std::array{_interactive_random.state[0], _interactive_random.state[1]}};
}

/** @param path Invocation-owned native output. Save the current original world without advancing any simulation opportunity. */
inline void SaveForgeRasterWorld(const std::filesystem::path &path)
{
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** @param capsule Actual original moving loaded model13. @param zoom Native viewport scale. Require correct-layer picking and identical-coordinate Surface refusal without changing the physical pose. */
inline void PickUsefulForge(const Train &capsule, ZoomLevel zoom)
{
	Viewport view{}; view.width = 640; view.height = 360; view.zoom = zoom; view.layer = GetVehicleDisplayLayer(&capsule);
	view.virtual_left = (capsule.coord.left + capsule.coord.right) / 2 - ScaleByZoom(320, zoom);
	view.virtual_top = (capsule.coord.top + capsule.coord.bottom) / 2 - ScaleByZoom(180, zoom);
	const auto *picked = CheckClickOnVehicle(view, 320, 180); REQUIRE(picked != nullptr); CHECK(picked->First() == &capsule);
	view.layer = MapLayer::Surface; const auto *other = CheckClickOnVehicle(view, 320, 180);
	// The same projection on another layer cannot select this Underground
	// capsule, even if an unrelated actual Surface part happens to be visible.
	CHECK((other == nullptr || other->First() != &capsule));
}

/** @param station Original actual joined modal station. @param tube Existing native tube rail identity. @return All actual owned tube platform cells; a conventional berth or bounding-box anchor cannot define capsule roof clearance. */
inline std::vector<TileIndex> ForgeTubePlatforms(const Station &station, RailType tube)
{
	std::vector<TileIndex> found;
	// Joined modal stations retain conventional and tube platforms in one
	// native area. Resolve the actual owned tube tile so its remote conventional
	// anchor cannot falsely make a capsule beside the roof appear clear.
	for (const auto tile : station.train_station) {
		if (!IsRailStationTile(tile) || GetStationIndex(tile) != station.index || GetRailType(tile) != tube) continue;
		found.push_back(tile);
	}
	REQUIRE_FALSE(found.empty()); return found;
}

/** @param world Exact original source/route participants. @param paths Invocation-owned captures and complete native boundaries. @param advance Complete accepted native opportunity, including all original vehicles/effects/clocks/power and both finite mines plus actual CFMT dispatch. Earn a clear actual midpoint, then verify unchanged original packed sprite and pure native viewport/picking. */
inline void CaptureUsefulForge(const ForgeUsefulWorld &world, const TestArtifactPaths &paths, const std::function<void()> &advance)
{
	auto &capsule = *world.parts[2]; REQUIRE(capsule.engine_type == GetTubeEngine(13)); REQUIRE(capsule.cargo.StoredCount() == 32);
	const auto start = TimerGameTick::counter; const auto source = world.power;
	const auto consumed = GetUtilityPowerProducer(source)->consumed_lifetime_j;
	auto platforms = ForgeTubePlatforms(*world.stations[1], world.tube);
	const auto destination = ForgeTubePlatforms(*world.stations[2], world.tube); platforms.insert(platforms.end(), destination.begin(), destination.end()); uint steps = 0;
	// The first moving saved state still overlaps the real source terminal.
	// Earn plain track away from both facilities at the actual native body
	// midpoint; no pose, order cursor, load, source art or anchor is assigned.
	while (steps < 800) {
		uint distance = UINT_MAX;
		// Both native two-cell tube platforms own real roofs. Clearance must
		// hold against the nearest actual cell, including the far end of a
		// berth; neither its station anchor nor its first cell alone suffices.
		for (const auto tile : platforms) distance = std::min(distance, DistanceManhattan(capsule.tile, tile));
		if (IsPlainRailTile(capsule.tile) && !capsule.vehstatus.Test(VehState::Hidden) && capsule.cur_speed > 0 &&
				distance >= 3 &&
				abs(2 * static_cast<int>(capsule.x_pos % TILE_SIZE) - 16) + abs(2 * static_cast<int>(capsule.y_pos % TILE_SIZE) - 16) <= 4) break;
		AutoRestoreBackup pause{_pause_mode, PauseModes{}}; advance(); ++steps;
		REQUIRE(capsule.cargo.StoredCount() == 32); REQUIRE(IsTrainOnItsTiles(&capsule)); REQUIRE_FALSE(capsule.vehstatus.Test(VehState::Crashed));
	}
	REQUIRE(steps < 800); REQUIRE(steps > 0); REQUIRE(GetUtilityPowerProducer(source)->consumed_lifetime_j > consumed);
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	const auto facing = capsule.flags.Test(VehicleRailFlag::Flipped) ? ReverseDir(capsule.direction) : capsule.direction;
	const auto view = to_underlying(facing); const auto sprite = GetTubeTrainSprite(capsule, facing, TimerGameTick::counter); REQUIRE(sprite.has_value());
	// This real shipment carries32 of the native80-unit capacity. Healthy
	// powered plain-track travel selects the original native partial pose;
	// the inspected packet count cannot be changed to match another state.
	REQUIRE(capsule.cargo_cap == 80); REQUIRE(capsule.cargo.StoredCount() > 0); REQUIRE(capsule.cargo.StoredCount() < capsule.cargo_cap);
	const auto *binding = GetTubePowerVehicle(capsule.index); REQUIRE(binding != nullptr); REQUIRE(binding->wait == TubePowerWait::None); REQUIRE_FALSE(capsule.flags.Test(VehicleRailFlag::Stuck));
	const auto owners = ForgeRasterOwners(world); const auto before = paths.Output("saves", "forge-raster-before.sav"); SaveForgeRasterWorld(before);
	const Point centre = RemapCoords(TileX(capsule.tile) * TILE_SIZE + TILE_SIZE / 2, TileY(capsule.tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(capsule.tile));
	const Point position = RemapCoords(capsule.x_pos + capsule.bounds.origin.x + capsule.bounds.offset.x,
		capsule.y_pos + capsule.bounds.origin.y + capsule.bounds.offset.y, capsule.z_pos + capsule.bounds.origin.z + capsule.bounds.offset.z);
	// Only exact original foregrounds of actual neighbouring guideways can
	// explain any hull occlusion. Publish real poses for independent indexed
	// atlas/palette checks; arbitrary scenery or hidden failed pixels cannot pass.
	for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
		const auto tile = TileAddWrap(capsule.tile, dx, dy); const auto art = GetTubeGuidewayArt(tile);
		if (!art.has_value()) continue;
		REQUIRE(art->count == 1);
		const Point neighbour = RemapCoords(TileX(tile) * TILE_SIZE + TILE_SIZE / 2, TileY(tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(tile));
		fmt::print("TUBE_TRAVEL_RIM view={} mask={} energized={} dx={} dy={} front={}\n", view, GetTrackBits(tile).base(), static_cast<uint>(art->energized),
			(neighbour.x - centre.x) / static_cast<int>(ZOOM_BASE), (neighbour.y - centre.y) / static_cast<int>(ZOOM_BASE), art->parts[0].front);
	}
	// Complete repeated frames retain company remaps and background pixels
	// independently of hull/rim accounting. Both zooms and actual selection
	// must leave every original simulation owner and client RNG untouched.
	for (const auto zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
		for (uint repeat = 0; repeat < 2; ++repeat) CaptureNativeArtViewport(capsule.tile, zoom,
			paths.images / fmt::format("tube-travel-{}-zoom-{}-{}.ppm", view, to_underlying(zoom), repeat));
		PickUsefulForge(capsule, zoom);
	}
	const char *purchase = std::getenv("OTTD_SELECTED_PURCHASE_CAPTURE");
	// Only explicit gallery observation adds ordinary purchase preview canvases;
	// the actual 2005 route supplies engine availability and the enclosing full
	// owner/RNG boundary proves that viewing cannot alter its paid useful work.
	if (purchase != nullptr && std::string_view{purchase} == "1") CaptureSelectedPurchaseViews(paths);
	REQUIRE(capsule.sprite_cache.sprite_seq.count == 1); CHECK(capsule.sprite_cache.sprite_seq.seq[0].sprite == *sprite);
	const auto after = paths.Output("saves", "forge-raster-after.sav"); SaveForgeRasterWorld(after); CHECK(ForgeRasterOwners(world) == owners);
	fmt::print("TUBE_TRAVEL_VIEW model=13 state=1 view={} direction={} frame={} sprite={} tile={} mask={} track={} speed={} tick={} dx={} dy={} picked=true wrong_layer_refused=true before={} after={}\n",
		view, to_underlying(capsule.direction), (TimerGameTick::counter / 8) % 2, *sprite, capsule.tile.base(), GetTrackBits(capsule.tile).base(), capsule.track.base(), capsule.cur_speed,
		TimerGameTick::counter, (position.x - centre.x) / static_cast<int>(ZOOM_BASE), (position.y - centre.y) / static_cast<int>(ZOOM_BASE), before.string(), after.string());
	fmt::print("FORGE_RASTER native_steps={} start={} end={} consumed={} original_units=32 full_native_world=true\n", steps, start, TimerGameTick::counter, GetUtilityPowerProducer(source)->consumed_lifetime_j - consumed);
}
#endif /* TESTS_FORGE_USEFUL_RASTER_FIXTURE_H */
