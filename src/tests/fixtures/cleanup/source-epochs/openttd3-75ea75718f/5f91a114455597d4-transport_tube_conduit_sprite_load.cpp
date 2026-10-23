/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_conduit_sprite_load.cpp Ordinary original-path files and pure native conduit viewport boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13CD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor and commissioned portal references are explicit fixture inputs, never useful-route bootstrap acceptance.
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
#include "../rail.h"
#include "../newgrf_station.h"
#include "../station_func.h"
#include "../openttd.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../safeguards.h"

/** @param name Explicit required decimal expectation. @return Complete checked independent native value. */
static uint64_t TubeConduitExpected(const char *name)
{
	const char *text = std::getenv(name); REQUIRE(text != nullptr);
	uint64_t value = 0; const auto parsed = std::from_chars(text, text + strlen(text), value);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == text + strlen(text)); return value;
}

/** @return Original quantity/reference digests, native packets, cash, tick and both RNG streams; no renderer cache owns these. */
static auto TubeConduitWorldOwners()
{
	return std::tuple{ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), ComputeTubePowerVehicleDigest(),
		ComputeTubeGeothermalPowerDigest(), ComputeGeothermalGeneratorDigest(), CargoPacket::GetNumItems(),
		Company::Get(_current_company)->money, TimerGameTick::counter,
		std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]}};
}

/** @param path New repository-owned ordinary native output. Save every original owner without advancing its opportunity. */
static void SaveTubeConduitWorld(const std::filesystem::path &path)
{
	AutoRestoreBackup menu(_game_mode, GameMode::Menu);
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Actual paid replacement corridors retain their original native plant, capsule packets and complete current file owners. */
TEST_CASE("Native tube conduits write actual dark published and retired path scenes", "[tube-conduit-art-write]")
{
	const auto scene = TubeConduitExpected("OTTD_TUBE_CONDUIT_SCENE"); REQUIRE(scene <= 2);
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); ResetRailTypes(); GeothermalTubeSwitchRig rig{true};
	AutoRestoreBackup freight(_settings_game.vehicle.freight_trains, uint8_t{1});
	const auto coal = GetCargoTypeByLabel(CT_COAL); REQUIRE(coal != INVALID_CARGO);
	CargoSpec::Get(coal)->weight = 16; CargoSpec::Get(coal)->is_freight = true;
	LayerSaveContent content{rig.ActivatedContent(), true}; AutoRestoreBackup local(_local_company, _current_company);
	StationClass::Reset(); RebuildStationKdtree(); (*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
	Train *capsule = rig.Buy(); rig.Load(*capsule);
	REQUIRE(BindTubeGeothermalPowerVehicle(capsule->index, rig.geothermal.source).has_value());
	const auto original = GetTubePowerConnection(rig.external); REQUIRE(original != nullptr);
	const auto portal = original->portal; REQUIRE(CloseTubeExternalPower(rig.external) == TubePowerError::None);
	std::vector<TileIndex> path;
	// An explicit detour leaves clear diagnostic cells five tiles from the
	// depot roof. Only these consecutive cells become the actual paid path.
	for (uint x = 32; x >= 27; --x) path.push_back(TileXYL(x, 23, MapLayer::DeepUnderground));
	path.push_back(TileXYL(27, 24, MapLayer::DeepUnderground));
	for (uint x = 27; x <= 32; ++x) path.push_back(TileXYL(x, 25, MapLayer::DeepUnderground));
	path.push_back(rig.inlets[0]); rig.Open(path);
	// Prepared neighbouring floor exposes original ground pixels without a
	// rock-wall foreground; no source, packet or machinery is moved for capture.
	std::vector<TileIndex> floor;
	for (uint y = 22; y <= 26; ++y) for (uint x = 26; x <= 28; ++x) floor.push_back(TileXYL(x, y, MapLayer::DeepUnderground));
	rig.Open(floor);
	const auto service = ConnectTubeExternalPower(portal, rig.inlets[0], path); REQUIRE(service.error == TubePowerError::None);
	if (scene == 1) {
		// The inherited native plant already allocated its current interval.
		// Normal synchronized preparation advances that owner before the new
		// service publishes; a display fixture cannot reopen allocated output.
		rig.Run(std::array{capsule});
		const auto *publication = GetUtilityPowerProducer(service.producer); REQUIRE(publication != nullptr);
		CHECK(publication->meter.accounting_interval == GetUtilityPowerInterval()); REQUIRE(publication->meter.generation_energy_j != 0);
	}
	if (scene == 2) REQUIRE(CloseTubeExternalPower(service.producer) == TubePowerError::None);
	REQUIRE(capsule->cargo.StoredCount() == 17); REQUIRE(capsule->cargo.Packets()->size() == 1);
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	const auto owners = TubeConduitWorldOwners(); const auto paths = GetTestCaseArtifactPaths("tubes", "conduit-art");
	const auto output = paths.Output("saves", fmt::format("tube-conduit-scene-{}.sav", scene)); SaveTubeConduitWorld(output);
	CHECK(TubeConduitWorldOwners() == owners);
	fmt::print("TUBE_CONDUIT_SAVE scene={} producer={} tick={} packet={} vehicle={} utility={} invoices={} binding={} geothermal={} generator={} path={}\n",
		scene, service.producer.base(), TimerGameTick::counter, capsule->cargo.Packets()->front()->index.base(), capsule->index.base(),
		ComputeUtilityPowerDigest(), ComputeTubePowerDigest(), ComputeTubePowerVehicleDigest(), ComputeTubeGeothermalPowerDigest(), ComputeGeothermalGeneratorDigest(), output.string());
}

static std::function<void()> *_tube_conduit_observer = nullptr; ///< Isolated startup observer only, never a live gameplay owner.

/** @param arguments Exact zero-argument startup invocation. @return Whether the registered pure viewport observer ran. */
static bool ObserveTubeConduitArt(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_tube_conduit_observer != nullptr); (*_tube_conduit_observer)(); return true;
}

/** Complete opposite-build loading and repeated actual ground captures preserve every original native owner. */
TEST_CASE("Native tube conduit viewport preserves complete original path and work owners", "[.tube-conduit-art-runtime]")
{
	const char *input = std::getenv("OTTD_TUBE_CONDUIT_INPUT"); REQUIRE(input != nullptr);
	const auto scene = TubeConduitExpected("OTTD_TUBE_CONDUIT_SCENE"); REQUIRE(scene <= 2);
	const auto source = ResolveTestInputPath(std::filesystem::path{input}); const auto config = GetTestRuntimeConfig();
	const auto paths = GetTestArtifactPaths("tubes", "conduit-art"); bool observed = false; std::exception_ptr failure;
	/** Capture only original display pixels after ordinary loading; all simulation expectations come from the exact original writer. */
	std::function<void()> capture = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			CHECK(ComputeUtilityPowerDigest() == TubeConduitExpected("OTTD_TUBE_CONDUIT_UTILITY"));
			CHECK(ComputeTubePowerDigest() == TubeConduitExpected("OTTD_TUBE_CONDUIT_INVOICES"));
			CHECK(ComputeTubePowerVehicleDigest() == TubeConduitExpected("OTTD_TUBE_CONDUIT_BINDING"));
			CHECK(ComputeTubeGeothermalPowerDigest() == TubeConduitExpected("OTTD_TUBE_CONDUIT_GEOTHERMAL"));
			CHECK(ComputeGeothermalGeneratorDigest() == TubeConduitExpected("OTTD_TUBE_CONDUIT_GENERATOR"));
			CHECK(TimerGameTick::counter == TubeConduitExpected("OTTD_TUBE_CONDUIT_TICK"));
			const auto vehicle = TubeConduitExpected("OTTD_TUBE_CONDUIT_VEHICLE"); REQUIRE(vehicle < UINT32_MAX);
			const Train *capsule = Train::GetIfValid(VehicleID{static_cast<uint32_t>(vehicle)}); REQUIRE(capsule != nullptr);
			CHECK(capsule->cargo.StoredCount() == 17); REQUIRE(capsule->cargo.Packets()->size() == 1);
			CHECK(capsule->cargo.Packets()->front()->index.base() == TubeConduitExpected("OTTD_TUBE_CONDUIT_PACKET"));
			const auto owners = TubeConduitWorldOwners(); const auto before = paths.Output("saves", "tube-conduit-before.sav"); SaveTubeConduitWorld(before);
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			// Real saved consecutive edges form the two corners and middle. Full
			// repeated views, not cropped mocks, exercise the common ground hook.
			for (uint y : {23U, 24U, 25U}) {
				const auto tile = TileXYL(27, y, MapLayer::DeepUnderground); const auto art = GetTubeConduitArt(tile);
				CHECK(art.has_value() == (scene != 2));
				for (const auto zoom : {ZoomLevel::Normal, ZoomLevel::In2x}) for (uint repeat = 0; repeat < 2; ++repeat) {
					CaptureNativeArtViewport(tile, zoom, paths.images / fmt::format("tube-conduit-{}-zoom-{}-{}.ppm", y, to_underlying(zoom), repeat));
				}
			}
			REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr); GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			const auto after = paths.Output("saves", "tube-conduit-after.sav"); SaveTubeConduitWorld(after); CHECK(TubeConduitWorldOwners() == owners);
			fmt::print("TUBE_CONDUIT_ART scene={} before={} after={}\n", scene, before.string(), after.string());
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer(_tube_conduit_observer, &capture); IConsole::CmdRegister("tube_conduit_art", ObserveTubeConduitArt);
	const std::string config_path = config.string(), source_path = source.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", config_path, "-g", source_path};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
