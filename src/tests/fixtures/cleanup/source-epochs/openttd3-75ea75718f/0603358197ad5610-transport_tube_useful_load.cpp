/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_useful_load.cpp Ordinary application acceptance of original autonomous Mole cargo, clocks and full-native continuation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15BC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * All actual participants are reacquired only after native loading; no fixture constructor, relocation or stock repair runs in the reader.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include <charconv>
#include "layer_window_fixture.h"
#include "layer_save_fixture.h"
#include "native_art_render_fixture.h"
#include "test_artifact_paths.h"
#include "tube_useful_tick_fixture.h"
#include "movement_fixture.h"
#include "../transport_tube.h"
#include "../transport_tube_sprite.h"
#include "../underground_vehicle.h"
#include "../industry.h"
#include "../station_base.h"
#include "../cargopacket.h"
#include "../console_internal.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../vehicle_gui.h"
#include "../rail.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../safeguards.h"

/** @param name Required independent writer-observed scalar suffix. @return Complete unsigned expectation; malformed/missing values fail before any optional continuation. */
static uint64_t MoleUsefulExpected(const char *name)
{
	const auto key = fmt::format("OTTD_MOLE_USEFUL_{}", name);
	const char *text = std::getenv(key.c_str()); REQUIRE(text != nullptr);
	uint64_t value = 0; const auto parsed = std::from_chars(text, text + strlen(text), value);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == text + strlen(text)); return value;
}

/** @param name Actual original vehicle expectation suffix. @return Reacquired original train part after accepted native pool loading. */
static Train *MoleUsefulLoadedTrain(const char *name)
{
	const auto id = MoleUsefulExpected(name); REQUIRE(id < UINT32_MAX);
	auto *part = Train::GetIfValid(VehicleID{static_cast<uint32_t>(id)}); REQUIRE(part != nullptr);
	REQUIRE(IsTrainOnItsTiles(part->First())); REQUIRE_FALSE(part->vehstatus.Test(VehState::Crashed));
	return part;
}

/** @return Original capsule after all expected native quantity/reference/clock/cargo owners have been accepted, without advancing or repairing a world. */
static Train *CheckLoadedMoleUsefulWorld()
{
	REQUIRE(ComputeUtilityPowerDigest() == MoleUsefulExpected("UTILITY"));
	REQUIRE(ComputeTubePowerVehicleDigest() == MoleUsefulExpected("BINDING"));
	REQUIRE(ComputeTubePowerDigest() == MoleUsefulExpected("INVOICE"));
	auto *capsule = MoleUsefulLoadedTrain("CAPSULE"), *freight = MoleUsefulLoadedTrain("FREIGHT"), *wagon = MoleUsefulLoadedTrain("WAGON");
	REQUIRE(capsule->engine_type == GetTubeEngine(9)); REQUIRE(freight->engine_type == GetUndergroundEngine(4));
	REQUIRE(wagon->engine_type == GetUndergroundEngine(5)); REQUIRE(wagon->First() == freight);
	const auto cargo = GetCargoTypeByLabel(CT_COAL); REQUIRE(cargo != INVALID_CARGO);
	REQUIRE(capsule->cargo_type == cargo); REQUIRE(wagon->cargo_type == cargo);
	REQUIRE(capsule->cargo.StoredCount() == MoleUsefulExpected("CAP_COUNT"));
	REQUIRE(wagon->cargo.StoredCount() == MoleUsefulExpected("WAGON_COUNT"));
	const auto factory_id = MoleUsefulExpected("FACTORY"), mine_id = MoleUsefulExpected("MINE");
	REQUIRE(factory_id < UINT16_MAX); REQUIRE(mine_id < UINT16_MAX);
	auto *factory = Industry::GetIfValid(IndustryID{static_cast<uint16_t>(factory_id)}); REQUIRE(factory != nullptr);
	REQUIRE(Industry::IsValidID(IndustryID{static_cast<uint16_t>(mine_id)}));
	const auto accepted = factory->GetCargoAccepted(cargo); REQUIRE(accepted != factory->accepted.end());
	REQUIRE(accepted->waiting == MoleUsefulExpected("FACTORY_WAIT"));
	const auto *binding = GetTubePowerVehicle(capsule->index); REQUIRE(binding != nullptr);
	REQUIRE(binding->source.base() == MoleUsefulExpected("SOURCE")); REQUIRE(binding->remainder_mj == MoleUsefulExpected("CARRY"));
	REQUIRE(TimerGameTick::counter == MoleUsefulExpected("TICK"));
	REQUIRE(TimerGameCalendar::date.base() == MoleUsefulExpected("CALENDAR")); REQUIRE(TimerGameCalendar::date_fract == MoleUsefulExpected("CALENDAR_FRACTION"));
	REQUIRE(TimerGameEconomy::date.base() == MoleUsefulExpected("ECONOMY")); REQUIRE(TimerGameEconomy::date_fract == MoleUsefulExpected("ECONOMY_FRACTION"));
	REQUIRE(TimerGameCalendar::year == TimerGameCalendar::Year{1970});
	REQUIRE(Company::Get(capsule->owner)->cur_economy.income.base() == MoleUsefulExpected("INCOME"));
	REQUIRE(Company::Get(capsule->owner)->cur_economy.delivered_cargo[cargo] == MoleUsefulExpected("DELIVERED"));
	const auto packet = MoleUsefulExpected("PACKET"); REQUIRE(packet < UINT32_MAX);
	// Before useful intake the original packet belongs to exactly one real
	// physical hold. After native final delivery it is legitimately consumed;
	// acceptance must never recreate it to satisfy an earlier scene label.
	if (accepted->waiting == 0) {
		const auto *holder = capsule->cargo.StoredCount() > 0 ? capsule : wagon;
		REQUIRE(holder->cargo.StoredCount() == 20); REQUIRE(holder->cargo.Packets()->size() == 1);
		const auto *original = holder->cargo.Packets()->front(); REQUIRE(original->index.base() == packet);
		CHECK(original->GetFirstStation().base() == MoleUsefulExpected("ORIGIN"));
		CHECK(original->GetSource() == Source{IndustryID{static_cast<uint16_t>(mine_id)}, SourceType::Industry});
		CHECK(original->GetCompanySupplyOrder() == 0);
	} else {
		REQUIRE(accepted->waiting == 20); CHECK_FALSE(CargoPacket::IsValidID(CargoPacketID{static_cast<uint32_t>(packet)}));
	}
	CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None);
	fmt::print("MOLE_LOADED scene={} tick={} fronts=2 original_packet={} full_native_clock=true\n", MoleUsefulExpected("SCENE"), TimerGameTick::counter, packet);
	return capsule;
}

/** @param path New invocation-owned native world output. Save current original owners without advancing the full driver. */
static void SaveLoadedMoleWorld(const std::filesystem::path &path)
{
	AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** @param capsule Actual original visible loaded front. @return Source/clock/money/physical/packet/RNG observations, with no second simulation authority. */
static auto MoleRasterOwners(const Train &capsule)
{
	return std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), ComputeTubePowerVehicleDigest(),
		Company::Get(capsule.owner)->money, Company::Get(capsule.owner)->cur_economy.income, CargoPacket::GetNumItems(),
		TimerGameTick::counter, TimerGameCalendar::date, TimerGameCalendar::date_fract, TimerGameEconomy::date, TimerGameEconomy::date_fract,
		capsule.tile, capsule.x_pos, capsule.y_pos, capsule.z_pos, capsule.direction, capsule.cur_speed, capsule.flags, capsule.vehstatus,
		capsule.cargo.StoredCount(), capsule.cargo.Packets()->front()->index,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
}

/** @param capsule Actual original loaded capsule. @param zoom Native viewport scale. Verify direct correct-layer selection and identical-coordinate wrong-layer refusal without changing its pose. */
static void PickUsefulMole(const Train &capsule, ZoomLevel zoom)
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

/** @param capsule Original ordinary-loaded moving front. @param paths Invocation-owned captures and pure native before/after boundaries. Earn a clear real midpoint through the complete native driver, then inspect its existing packed body. */
static void CaptureUsefulMole(Train &capsule, const TestArtifactPaths &paths)
{
	REQUIRE(MoleUsefulExpected("SCENE") == 0); REQUIRE(capsule.cargo.StoredCount() == 20);
	const auto start = TimerGameTick::counter; const auto *binding = GetTubePowerVehicle(capsule.index); REQUIRE(binding != nullptr);
	const auto source = binding->source; const auto consumed = GetUtilityPowerProducer(source)->consumed_lifetime_j;
	const auto origin_id = MoleUsefulExpected("ORIGIN"), interchange_id = MoleUsefulExpected("INTERCHANGE");
	REQUIRE(origin_id < UINT16_MAX); REQUIRE(interchange_id < UINT16_MAX);
	const auto origin = Station::Get(StationID{static_cast<uint16_t>(origin_id)})->xy;
	const auto interchange = Station::Get(StationID{static_cast<uint16_t>(interchange_id)})->xy;
	uint steps = 0;
	// A first tile entry still overlaps a station/depot roof. Earn a true
	// plain-track midpoint away from both facilities with the original20-unit
	// packet, never shifting the vehicle, source art or native viewport anchor.
	while (steps < 800) {
		if (IsPlainRailTile(capsule.tile) && !capsule.vehstatus.Test(VehState::Hidden) && capsule.cur_speed > 0 &&
				DistanceManhattan(capsule.tile, origin) >= 3 && DistanceManhattan(capsule.tile, interchange) >= 3 &&
				abs(2 * static_cast<int>(capsule.x_pos % TILE_SIZE) - 16) + abs(2 * static_cast<int>(capsule.y_pos % TILE_SIZE) - 16) <= 4) break;
		AutoRestoreBackup pause{_pause_mode, PauseModes{}}; RunUsefulTubeNativeOpportunity(); ++steps;
		REQUIRE(capsule.cargo.StoredCount() == 20); REQUIRE(IsTrainOnItsTiles(&capsule)); REQUIRE_FALSE(capsule.vehstatus.Test(VehState::Crashed));
	}
	REQUIRE(steps < 800); REQUIRE(steps > 0); REQUIRE(GetUtilityPowerProducer(source)->consumed_lifetime_j > consumed);
	REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	const auto facing = capsule.flags.Test(VehicleRailFlag::Flipped) ? ReverseDir(capsule.direction) : capsule.direction;
	const auto view = to_underlying(facing); const auto sprite = GetTubeTrainSprite(capsule, facing, TimerGameTick::counter); REQUIRE(sprite.has_value());
	const auto before = paths.Output("saves", "mole-raster-before.sav"); SaveLoadedMoleWorld(before); const auto owners = MoleRasterOwners(capsule);
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
		PickUsefulMole(capsule, zoom);
	}
	REQUIRE(capsule.sprite_cache.sprite_seq.count == 1); CHECK(capsule.sprite_cache.sprite_seq.seq[0].sprite == *sprite);
	const auto after = paths.Output("saves", "mole-raster-after.sav"); SaveLoadedMoleWorld(after); CHECK(MoleRasterOwners(capsule) == owners);
	fmt::print("TUBE_TRAVEL_VIEW model=9 view={} direction={} frame={} sprite={} tile={} mask={} track={} speed={} tick={} dx={} dy={} picked=true wrong_layer_refused=true before={} after={}\n",
		view, to_underlying(capsule.direction), (TimerGameTick::counter / 8) % 2, *sprite, capsule.tile.base(), GetTrackBits(capsule.tile).base(), capsule.track.base(), capsule.cur_speed,
		TimerGameTick::counter, (position.x - centre.x) / static_cast<int>(ZOOM_BASE), (position.y - centre.y) / static_cast<int>(ZOOM_BASE), before.string(), after.string());
	fmt::print("MOLE_RASTER native_steps={} start={} end={} consumed={} original_units=20\n", steps, start, TimerGameTick::counter, GetUtilityPowerProducer(source)->consumed_lifetime_j - consumed);
}

static std::function<void()> *_mole_useful_observer = nullptr; ///< Process-local startup observation, never a simulation/quantity owner.

/** @param arguments Actual ordinary startup command arguments. @return Success after inspecting accepted native owners. */
static bool ObserveMoleUsefulWorld(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_mole_useful_observer != nullptr); (*_mole_useful_observer)(); return true;
}

/** Ordinary application loading must retain all original native participants before a pure resave or one complete full-driver continuation. */
TEST_CASE("Useful Mole worlds retain native cargo and full timer continuation", "[.tube-useful-mole-native-file]")
{
	const char *input = std::getenv("OTTD_MOLE_USEFUL_INPUT"), *action_text = std::getenv("OTTD_MOLE_USEFUL_ACTION");
	REQUIRE(input != nullptr); REQUIRE(action_text != nullptr); const std::string_view action{action_text};
	REQUIRE((action == "inspect" || action == "step" || action == "raster"));
	const auto paths = GetTestCaseArtifactPaths("tubes", "useful-files");
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Observe only fully accepted original objects; optional continuation delegates the same complete native clocks/vehicle loop as the untouched writer. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			auto *capsule = CheckLoadedMoleUsefulWorld();
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			capsule = CheckLoadedMoleUsefulWorld();
			if (action == "step") {
				AutoRestoreBackup pause{_pause_mode, PauseModes{}};
				RunUsefulTubeNativeOpportunity();
				CHECK(TimerGameTick::counter == MoleUsefulExpected("TICK") + 1); CHECK(IsTrainOnItsTiles(capsule));
			}
			if (action == "raster") CaptureUsefulMole(*capsule, paths);
			AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
			const auto saved = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action));
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok);
			fmt::print("MOLE_READ action={} path={}\n", action, saved.string());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_mole_useful_observer, &inspect}; IConsole::CmdRegister("mole_useful_file_check", ObserveMoleUsefulWorld);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
