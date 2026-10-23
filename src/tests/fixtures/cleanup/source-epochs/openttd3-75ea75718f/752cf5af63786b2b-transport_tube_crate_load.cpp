/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_crate_load.cpp Ordinary application loading of genuine Crate route files and original native owner continuation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md tasks15CC.1/15CC.2.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * No source fixture, cargo grant, vehicle relocation, clock adjustment or account normalization runs in this reader.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include <charconv>
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "native_art_render_fixture.h"
#include "crate_useful_world_fixture.h"
#include "tube_useful_tick_fixture.h"
#include "test_artifact_paths.h"
#include "movement_fixture.h"
#include "../underground_vehicle.h"
#include "../console_internal.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../transport_tube_sprite.h"
#include "../vehicle_gui.h"
#include "../rail.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../safeguards.h"

/** @param name Required original writer-observed environment list. @param count Exact declared number of actual participants. @return Complete checked original native IDs; malformed/truncated identities fail before any optional work. */
static std::vector<uint32_t> CrateUsefulIds(const char *name, size_t count)
{
	const char *text = std::getenv(name); REQUIRE(text != nullptr); const char *end = text + strlen(text);
	std::vector<uint32_t> ids;
	// Every original identity must parse completely; a missing suffix or
	// silently truncated native pool ID cannot select a different participant.
	while (text < end) {
		uint32_t id = 0; const auto parsed = std::from_chars(text, end, id);
		REQUIRE(parsed.ec == std::errc{}); ids.push_back(id);
		if (parsed.ptr == end) { text = end; break; }
		REQUIRE(*parsed.ptr == ','); text = parsed.ptr + 1; REQUIRE(text < end);
	}
	REQUIRE(ids.size() == count); return ids;
}

/** @return Borrowed actual original participants after accepted native pool loading, never fixture-created replacement owners. */
static CrateUsefulWorld LoadedCrateUsefulWorld()
{
	CrateUsefulWorld world;
	const auto parts = CrateUsefulIds("OTTD_CRATE_USEFUL_PARTS", 5), references = CrateUsefulIds("OTTD_CRATE_USEFUL_REFERENCES", 8);
	REQUIRE(Vehicle::GetNumItems() == 5);
	// Normal loading must restore each exact original vehicle, its coupled
	// front and physical map membership before a preview or next opportunity.
	for (size_t index = 0; index < parts.size(); ++index) {
		world.parts[index] = Train::GetIfValid(VehicleID{parts[index]}); REQUIRE(world.parts[index] != nullptr);
		REQUIRE_FALSE(world.parts[index]->vehstatus.Test(VehState::Crashed));
	}
	for (uint index : {0, 2, 3}) REQUIRE(IsTrainOnItsTiles(world.parts[index]));
	REQUIRE(world.parts[0]->engine_type == GetUndergroundEngine(4)); REQUIRE(world.parts[3]->engine_type == GetUndergroundEngine(4));
	REQUIRE(world.parts[1]->engine_type == GetUndergroundEngine(5)); REQUIRE(world.parts[4]->engine_type == GetUndergroundEngine(5));
	REQUIRE(world.parts[2]->engine_type == GetTubeEngine(10)); REQUIRE(world.parts[2]->First() == world.parts[2]);
	REQUIRE(world.parts[1]->First() == world.parts[0]); REQUIRE(world.parts[4]->First() == world.parts[3]);
	const auto cargo = GetCargoTypeByLabel(CT_GOODS); REQUIRE(cargo != INVALID_CARGO);
	for (uint index : {1, 2, 4}) REQUIRE(world.parts[index]->cargo_type == cargo);
	for (size_t index = 0; index < 2; ++index) {
		REQUIRE(references[index] < UINT16_MAX);
		world.industries[index] = Industry::GetIfValid(IndustryID{static_cast<uint16_t>(references[index])}); REQUIRE(world.industries[index] != nullptr);
	}
	REQUIRE(world.industries[0]->type == 3); REQUIRE(world.industries[1]->type == 2);
	REQUIRE(references[2] < UINT16_MAX); world.town = Town::GetIfValid(TownID{static_cast<uint16_t>(references[2])}); REQUIRE(world.town != nullptr);
	for (size_t index = 0; index < world.stations.size(); ++index) {
		REQUIRE(references[index + 3] < UINT16_MAX);
		world.stations[index] = Station::GetIfValid(StationID{static_cast<uint16_t>(references[index + 3])}); REQUIRE(world.stations[index] != nullptr);
	}
	world.power = UtilityProducerID{references[7]}; REQUIRE(GetUtilityPowerProducer(world.power) != nullptr);
	const auto *binding = GetTubePowerVehicle(world.parts[2]->index); REQUIRE(binding != nullptr); REQUIRE(binding->source == world.power);
	REQUIRE(world.stations[3]->town == world.town); REQUIRE(world.town->cache.num_houses == 29);
	REQUIRE(world.stations[3]->goods[cargo].status.Test(GoodsEntry::State::Acceptance));
	REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{1975}); REQUIRE(GetTileLayer(world.parts[2]->tile) == MapLayer::Underground);
	// Every surviving original packet retains genuine Sawmill/first-station
	// provenance and ordinary purpose, including native split identities.
	for (const auto *packet : CargoPacket::Iterate()) {
		CHECK(packet->GetSource() == Source{world.industries[1]->index, SourceType::Industry});
		CHECK(packet->GetFirstStation() == world.stations[0]->index); CHECK(packet->GetCompanySupplyOrder() == 0);
	}
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	return world;
}

/** @param world Fully loaded original participants. @param expected Exact original writer-observed named canonical scalars. Require every field without normalizing native load or quarter behavior. */
static void CheckCrateUsefulSnapshot(const CrateUsefulWorld &world, std::string_view expected)
{
	const auto actual = CaptureCrateUsefulWorld(world);
	// A changed field must remain diagnosable in its original native form;
	// neither a partial scalar comparison nor a rewritten expectation can pass.
	if (actual != expected) fmt::print("CRATE_SNAPSHOT actual={}\n", actual);
	REQUIRE(actual == expected);
}

/** @param path New invocation-owned native world output. Save current original owners without advancing the full driver. */
static void SaveLoadedCrateWorld(const std::filesystem::path &path)
{
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** @param world Original native participants at the actual inspection point. @return Complete canonical owner observations and the independent UI RNG, without a second simulation authority. */
static auto CrateRasterOwners(const CrateUsefulWorld &world)
{
	return std::pair{CaptureCrateUsefulWorld(world), std::array{_interactive_random.state[0], _interactive_random.state[1]}};
}

/** @param capsule Actual original loaded capsule. @param zoom Native viewport scale. Verify direct correct-layer selection and identical-coordinate wrong-layer refusal without changing its pose. */
static void PickUsefulCrate(const Train &capsule, ZoomLevel zoom)
{
	Viewport view{}; view.width = 640; view.height = 360; view.zoom = zoom; view.layer = GetVehicleDisplayLayer(&capsule);
	view.virtual_left = (capsule.coord.left + capsule.coord.right) / 2 - ScaleByZoom(320, zoom);
	view.virtual_top = (capsule.coord.top + capsule.coord.bottom) / 2 - ScaleByZoom(180, zoom);
	const auto *picked = CheckClickOnVehicle(view, 320, 180); REQUIRE(picked != nullptr); CHECK(picked->First() == &capsule);
	view.layer = MapLayer::Surface; const auto *other = CheckClickOnVehicle(view, 320, 180);
	// An identical projection on another layer grants no selection of this
	// actual Underground capsule, even if another real vehicle is visible there.
	CHECK((other == nullptr || other->First() != &capsule));
}

/** @param world Complete original native participants, including the loaded moving Crate. @param paths Invocation-owned captures and pure native before/after boundaries. Earn a clear real midpoint through the complete native driver, then inspect its existing packed body. */
static void CaptureUsefulCrate(const CrateUsefulWorld &world, const TestArtifactPaths &paths)
{
	auto &capsule = *world.parts[2]; REQUIRE(capsule.cargo.StoredCount() == 39);
	const auto start = TimerGameTick::counter; const auto *binding = GetTubePowerVehicle(capsule.index); REQUIRE(binding != nullptr);
	const auto source = binding->source; const auto consumed = GetUtilityPowerProducer(source)->consumed_lifetime_j;
	const auto origin = world.stations[1]->xy, interchange = world.stations[2]->xy;
	uint steps = 0;
	// A first tile entry still overlaps a station/depot roof. Earn a true
	// plain-track midpoint away from both facilities with the original39-unit
	// load, never shifting the vehicle, source art or native viewport anchor.
	while (steps < 800) {
		if (IsPlainRailTile(capsule.tile) && !capsule.vehstatus.Test(VehState::Hidden) && capsule.cur_speed > 0 &&
				DistanceManhattan(capsule.tile, origin) >= 3 && DistanceManhattan(capsule.tile, interchange) >= 3 &&
				abs(2 * static_cast<int>(capsule.x_pos % TILE_SIZE) - 16) + abs(2 * static_cast<int>(capsule.y_pos % TILE_SIZE) - 16) <= 4) break;
		AutoRestoreBackup pause{_pause_mode, PauseModes{}}; RunUsefulTubeNativeOpportunity(); ++steps;
		REQUIRE(capsule.cargo.StoredCount() == 39); REQUIRE(IsTrainOnItsTiles(&capsule)); REQUIRE_FALSE(capsule.vehstatus.Test(VehState::Crashed));
		// All original fronts share this native opportunity. The two actual
		// conventional legs must retain physical integrity while the loaded
		// capsule earns its inspection point; rendering cannot isolate its work.
		for (uint index : {0, 3}) { REQUIRE(IsTrainOnItsTiles(world.parts[index])); REQUIRE_FALSE(world.parts[index]->vehstatus.Test(VehState::Crashed)); }
	}
	REQUIRE(steps < 800); REQUIRE(steps > 0); REQUIRE(GetUtilityPowerProducer(source)->consumed_lifetime_j > consumed);
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	const auto facing = capsule.flags.Test(VehicleRailFlag::Flipped) ? ReverseDir(capsule.direction) : capsule.direction;
	const auto view = to_underlying(facing); const auto sprite = GetTubeTrainSprite(capsule, facing, TimerGameTick::counter); REQUIRE(sprite.has_value());
	const auto owners = CrateRasterOwners(world);
	const auto before = paths.Output("saves", "crate-raster-before.sav"); SaveLoadedCrateWorld(before);
	const Point centre = RemapCoords(TileX(capsule.tile) * TILE_SIZE + TILE_SIZE / 2, TileY(capsule.tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(capsule.tile));
	const Point position = RemapCoords(capsule.x_pos + capsule.bounds.origin.x + capsule.bounds.offset.x,
		capsule.y_pos + capsule.bounds.origin.y + capsule.bounds.offset.y, capsule.z_pos + capsule.bounds.origin.z + capsule.bounds.offset.z);
	// Only original foregrounds of real neighbouring guideways may explain
	// body occlusion. Publish their exact native pose for independent palette/
	// anchor checks; arbitrary terrain masks or hidden hull pixels cannot pass.
	for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
		const auto tile = TileAddWrap(capsule.tile, dx, dy); const auto art = GetTubeGuidewayArt(tile);
		if (!art.has_value()) continue;
		REQUIRE(art->count == 1);
		const Point neighbour = RemapCoords(TileX(tile) * TILE_SIZE + TILE_SIZE / 2, TileY(tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(tile));
		fmt::print("TUBE_TRAVEL_RIM view={} mask={} energized={} dx={} dy={} front={}\n", view, GetTrackBits(tile).base(), static_cast<uint>(art->energized),
			(neighbour.x - centre.x) / static_cast<int>(ZOOM_BASE), (neighbour.y - centre.y) / static_cast<int>(ZOOM_BASE), art->parts[0].front);
	}
	// Both native zooms and independent repeats preserve the complete frame,
	// including company remaps/backgrounds; actual native picking is also pure.
	for (const auto zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) {
		for (uint repeat = 0; repeat < 2; ++repeat) CaptureNativeArtViewport(capsule.tile, zoom,
			paths.images / fmt::format("tube-travel-{}-zoom-{}-{}.ppm", view, to_underlying(zoom), repeat));
		PickUsefulCrate(capsule, zoom);
	}
	REQUIRE(capsule.sprite_cache.sprite_seq.count == 1); CHECK(capsule.sprite_cache.sprite_seq.seq[0].sprite == *sprite);
	const auto after = paths.Output("saves", "crate-raster-after.sav"); SaveLoadedCrateWorld(after); CHECK(CrateRasterOwners(world) == owners);
	fmt::print("TUBE_TRAVEL_VIEW model=10 view={} direction={} frame={} sprite={} tile={} mask={} track={} speed={} tick={} dx={} dy={} picked=true wrong_layer_refused=true before={} after={}\n",
		view, to_underlying(capsule.direction), (TimerGameTick::counter / 8) % 2, *sprite, capsule.tile.base(), GetTrackBits(capsule.tile).base(), capsule.track.base(), capsule.cur_speed,
		TimerGameTick::counter, (position.x - centre.x) / static_cast<int>(ZOOM_BASE), (position.y - centre.y) / static_cast<int>(ZOOM_BASE), before.string(), after.string());
	fmt::print("CRATE_RASTER native_steps={} start={} end={} consumed={} original_units=39 full_native_world=true\n", steps, start, TimerGameTick::counter, GetUtilityPowerProducer(source)->consumed_lifetime_j - consumed);
}

static std::function<void()> *_crate_useful_observer = nullptr; ///< Process-local ordinary startup callback; no simulation/quantity authority.

/** @param arguments Actual ordinary application console command tokens. @return Success after inspecting accepted original native owners. */
static bool ObserveCrateUsefulWorld(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_crate_useful_observer != nullptr); (*_crate_useful_observer)(); return true;
}

/** Fully ordinary application loading must preserve all original native participants before a pure resave or one full-native continuation. */
TEST_CASE("Useful Crate files retain original native town cargo history and continuation", "[.tube-useful-crate-native-file]")
{
	const char *input = std::getenv("OTTD_CRATE_USEFUL_INPUT"), *action_text = std::getenv("OTTD_CRATE_USEFUL_ACTION"), *expected_text = std::getenv("OTTD_CRATE_USEFUL_SNAPSHOT");
	REQUIRE(input != nullptr); REQUIRE(action_text != nullptr); REQUIRE(expected_text != nullptr);
	const std::string_view action{action_text}; const std::string expected{expected_text}; REQUIRE((action == "inspect" || action == "step" || action == "raster"));
	const auto paths = GetTestCaseArtifactPaths("tubes", "crate-files"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Inspect only accepted native owners; optional work uses the same complete timers/vehicle loop as the untouched original producer. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			auto world = LoadedCrateUsefulWorld(); CheckCrateUsefulSnapshot(world, expected);
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			world = LoadedCrateUsefulWorld(); CheckCrateUsefulSnapshot(world, expected);
			const auto tick = TimerGameTick::counter;
			if (action == "step") {
				AutoRestoreBackup pause{_pause_mode, PauseModes{}}; RunUsefulTubeNativeOpportunity();
				CHECK(TimerGameTick::counter == tick + 1);
				for (uint index : {0, 2, 3}) CHECK(IsTrainOnItsTiles(world.parts[index]));
			}
			if (action == "raster") CaptureUsefulCrate(world, paths);
			AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
			const auto saved = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action));
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok);
			fmt::print("CRATE_READ action={} fronts=3 original_parts=5 full_native_clock=true snapshot={} path={}\n", action, CaptureCrateUsefulWorld(world), saved.string());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_crate_useful_observer, &inspect}; IConsole::CmdRegister("crate_useful_file_check", ObserveCrateUsefulWorld);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
