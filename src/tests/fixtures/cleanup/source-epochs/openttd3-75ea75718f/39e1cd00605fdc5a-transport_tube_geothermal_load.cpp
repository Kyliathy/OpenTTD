/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_geothermal_load.cpp Genuine433 original inlet files, independent copied preview and ordinary executable reload/refusal.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BBC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor/materials remain separate from paid-TBM/bootstrap and capsule-work acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "geothermal_tube_power_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../saveload/saveload_func.h"
#include "../console_internal.h"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include "../safeguards.h"

/** @param phase Connected0, first root closed1, temporary wire loss2 or original plant demolished3. Check actual identities without renewing grants or repairing original history. */
static void CheckNativeTubeGeothermalFile(unsigned phase)
{
    const auto state = CaptureTubeGeothermalPowerState(); REQUIRE(state.sources.size() == 2);
    CHECK(state.next_source == 2); CHECK(state.sequence >= 2);
    REQUIRE(ValidateTubeGeothermalPowerState(state, CaptureUtilityPowerState()));
    CHECK(state.sources[0].active == (phase == 0)); CHECK(state.sources[1].active == (phase < 3));
    CHECK(CaptureGeothermalGenerators().size() == (phase == 3 ? 0 : 1));
    // Original metadata refers to the same sole output, but each physical
    // inlet retains its own distinct forwarding reservation and native depot.
    CHECK(state.sources[0].producer == state.sources[1].producer);
    CHECK(state.sources[0].supply != state.sources[1].supply); CHECK(state.sources[0].depot != state.sources[1].depot);
    for (const auto &source : state.sources) {
        CHECK(GetTubePowerConnection(source.producer) == nullptr);
        const auto *supply = GetUtilityPowerConsumer(source.supply); REQUIRE(supply != nullptr);
        CHECK(supply->active == source.active); CHECK(supply->owner == source.owner);
        CHECK(GetTubeGeothermalSourcePath(source.id).has_value() == (phase < 2 && source.active));
    }
    CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Real original source files retain active, closed, temporarily disconnected and demolished-source history in the sole native/reference owners. */
TEST_CASE("Native geothermal tube source writes ordinary worlds", "[tube-geothermal-source-save-file]")
{
    MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); GeothermalTubePowerRig rig{GeothermalGeneratorModel::MagmaLoop, true};
    LayerSaveContent content{rig.ActivatedContent()}; AutoRestoreBackup local{_local_company, _current_company};
    (*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
    const auto first = rig.Connect(0), second = rig.Connect(1);
    rig.Allocate(std::array{UtilityPowerRequest{first.supply, 3000}, UtilityPowerRequest{second.supply, 3000}});
    const auto paths = GetTestCaseArtifactPaths("tubes", "geothermal-source-files");
    /** @param phase Actual original-source checkpoint. Save without changing references, money, quantities, time or RNG. */
    const auto save = [&](unsigned phase) {
        CheckNativeTubeGeothermalFile(phase); const auto source = ComputeTubeGeothermalPowerDigest();
        const auto utility = ComputeUtilityPowerDigest(), native = ComputeGeothermalGeneratorDigest();
        SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
        AutoRestoreBackup menu{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
        AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
        const auto path = paths.Output("saves", fmt::format("geothermal-source-{}.sav", phase));
        REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
        CHECK(ComputeTubeGeothermalPowerDigest() == source); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalGeneratorDigest() == native);
        fmt::print("GEOTHERMAL_SOURCE_SAVE phase={} source={} utility={} native={} path={}\n", phase, source, utility, native, path.string());
    };
    save(0); REQUIRE(CloseTubeGeothermalPower(first.source) == TubePowerError::None); save(1);
    const auto broken = TileXYL(35, 24, MapLayer::DeepUnderground);
    REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, broken,
        std::make_tuple(broken, UtilityEnergyDomain::Electricity)).Succeeded());
    save(2);
    REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, rig.plant_port, std::make_tuple(rig.plant_port)).Succeeded());
    save(3);
}

static std::function<void()> *_geothermal_source_observer = nullptr; ///< Process-local ordinary afterload observation, never a simulation/resource owner.

/** @param arguments Actual startup command with no caller-created state. @return Success after the original ordinary loader and observer complete. */
static bool ObserveNativeTubeGeothermal(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_geothermal_source_observer != nullptr); (*_geothermal_source_observer)(); return true;
}

/** @param name Explicit environment field supplied by the owning focused runner. @return Independently expected bounded unsigned scalar; malformed input aborts the test before world mutation. */
static uint64_t TubeGeothermalExpected(const char *name)
{
	const char *text = std::getenv(name); REQUIRE(text != nullptr);
	uint64_t value = 0; const auto parsed = std::from_chars(text, text + strlen(text), value);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == text + strlen(text)); return value;
}

/** Actual executable startup independently verifies copied preview/refusal, exact native shared history and repeated ordinary resaving. */
TEST_CASE("Native geothermal tube sources survive ordinary files and independent preview", "[.tube-geothermal-source-native-file]")
{
	const char *input = std::getenv("OTTD_GEOTHERMAL_SOURCE_INPUT"), *action_text = std::getenv("OTTD_GEOTHERMAL_SOURCE_ACTION");
	REQUIRE(input != nullptr); REQUIRE(action_text != nullptr); const std::string_view action{action_text};
	REQUIRE((action == "inspect" || action == "preview-refusal" || action == "full-refusal"));
	const auto phase = TubeGeothermalExpected("OTTD_GEOTHERMAL_SOURCE_PHASE"); REQUIRE(phase <= 4);
	const auto expected_source = TubeGeothermalExpected("OTTD_GEOTHERMAL_SOURCE_SOURCE");
	const auto expected_utility = TubeGeothermalExpected("OTTD_GEOTHERMAL_SOURCE_UTILITY"), expected_native = TubeGeothermalExpected("OTTD_GEOTHERMAL_SOURCE_NATIVE");
	const auto paths = GetTestCaseArtifactPaths("tubes", "geothermal-source-files"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe accepted native owners before any simulation opportunity; preview and malformed copies cannot change their original history. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; if (phase < 4) { CheckNativeTubeGeothermalFile(static_cast<unsigned>(phase)); } else { CHECK(CaptureTubeGeothermalPowerState().sources.empty()); }
			CHECK(ComputeTubeGeothermalPowerDigest() == expected_source); CHECK(ComputeUtilityPowerDigest() == expected_utility); CHECK(ComputeGeothermalGeneratorDigest() == expected_native);
			const auto cash = Company::Get(_current_company)->money; const auto clock = TimerGameTick::counter;
			const auto random = std::array{_random.state[0], _random.state[1]};
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			if (action != "inspect") {
				const char *bad = std::getenv("OTTD_GEOTHERMAL_SOURCE_CORRUPT"); REQUIRE(bad != nullptr);
				if (action == "preview-refusal") {
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				} else {
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
					REQUIRE(SaveOrLoad(input, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				}
			}
			CHECK(ComputeTubeGeothermalPowerDigest() == expected_source); CHECK(ComputeUtilityPowerDigest() == expected_utility); CHECK(ComputeGeothermalGeneratorDigest() == expected_native);
			CHECK(Company::Get(_current_company)->money == cash); CHECK(TimerGameTick::counter == clock);
			CHECK(std::array{_random.state[0], _random.state[1]} == random);
			if (phase < 4) { CheckNativeTubeGeothermalFile(static_cast<unsigned>(phase)); } else { CHECK(CaptureTubeGeothermalPowerState().sources.empty()); }
			if (action == "inspect") {
				AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
				const auto saved = paths.Output("saves", fmt::format("{}-reload.sav", std::filesystem::path{input}.stem().string()));
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok);
				fmt::print("GEOTHERMAL_SOURCE_READ phase={} path={}\n", phase, saved.string());
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_geothermal_source_observer, &inspect}; IConsole::CmdRegister("geothermal_source_check", ObserveNativeTubeGeothermal);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
