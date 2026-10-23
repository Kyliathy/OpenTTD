/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_geothermal_binding_load.cpp Genuine434 native provider/binding files, copied preview and ordinary executable refusal/recovery.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BCC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Actual kernel-paid participants use explicitly prepared floor/materials; powered work, switching and bootstrap remain later acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "geothermal_tube_capsule_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../saveload/saveload_func.h"
#include "../console_internal.h"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include "../safeguards.h"

/** @param phase Connected0, closed source1, demolished plant2, retired/reused capsule3 or actual takeover4. Check only original reference owners; renew no grant, work, source or carry. */
static void CheckNativeGeothermalBindingFile(unsigned phase)
{
    const auto sources = CaptureTubeGeothermalPowerState(); REQUIRE(sources.sources.size() == 1);
    const auto &source = sources.sources.front(); CHECK(source.active == (phase == 0));
    CHECK(CaptureGeothermalGenerators().size() == (phase < 2 ? 1 : 0));
    const auto rows = CaptureTubePowerVehicles(); REQUIRE(rows.size() == 2);
    CHECK(rows[0].active == (phase < 3)); CHECK(rows[1].active == (phase < 4));
    // Both original requests retain one actual source incarnation/output,
    // while each capsule owns its independently issued consumer and feed.
    for (const auto &row : rows) {
        CHECK(row.geothermal_source == source.id); CHECK(row.source == source.producer);
        CHECK(row.remainder_mj == 0); CHECK(row.consumed_tick == UINT64_MAX);
        const auto *feed = GetUtilityPowerLink(row.local_feed); const auto *consumer = GetUtilityPowerConsumer(row.consumer);
        REQUIRE(feed != nullptr); REQUIRE(consumer != nullptr); CHECK(feed->source == source.supply); CHECK(feed->destination == row.consumer);
        CHECK(feed->active == (phase == 0)); CHECK(consumer->active == row.active); CHECK(consumer->linked_producers.empty());
        CHECK(feed->flow_j == 0); CHECK(GetTubePowerConnection(row.source) == nullptr);
    }
    CHECK(rows[0].consumer != rows[1].consumer); CHECK(rows[0].local_feed != rows[1].local_feed);
    REQUIRE(Vehicle::GetNumItems() == 2);
    // The original first capsule/replacement stays manually stopped; the
    // second was explicitly started. Label remapping cannot change either.
    CHECK(Train::Get(rows[0].vehicle)->vehstatus.Test(VehState::Stopped));
    CHECK_FALSE(Train::Get(rows[1].vehicle)->vehstatus.Test(VehState::Stopped));
    CHECK(ValidateTubePowerVehicles(rows) == TubePowerError::None);
    CHECK(ValidateTubeGeothermalPowerState(sources, CaptureUtilityPowerState()));
    CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Real ordinary files retain original source/feed roles through source closure, demolition, native vehicle reuse and actual company lifecycle, without claiming powered work. */
TEST_CASE("Native original geothermal bindings write ordinary worlds", "[tube-geothermal-binding-save-file]")
{
    MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
    std::unique_ptr<Company> successor; GeothermalTubeCapsuleRig rig{true};
    LayerSaveContent content{rig.ActivatedContent(), true}; AutoRestoreBackup local{_local_company, _current_company};
    (*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
    auto *first = rig.Buy(); auto *second = rig.Buy(); const auto source = rig.Connect();
    REQUIRE(BindTubeGeothermalPowerVehicle(first->index, source.source).has_value()); REQUIRE(BindTubeGeothermalPowerVehicle(second->index, source.source).has_value());
    REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, second->index, true).Succeeded());
    const auto paths = GetTestCaseArtifactPaths("tubes", "geothermal-bindings");
    /** @param phase Actual original provider/binding checkpoint. Save without changing any native/quantity/financial owner. */
    const auto save = [&](unsigned phase) {
        CheckNativeGeothermalBindingFile(phase);
        const auto utility = ComputeUtilityPowerDigest(), source_hash = ComputeTubeGeothermalPowerDigest();
        const auto bindings = ComputeTubePowerVehicleDigest(), native = ComputeGeothermalGeneratorDigest();
        SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
        AutoRestoreBackup menu{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
        AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
        const auto path = paths.Output("saves", fmt::format("geothermal-binding-{}.sav", phase));
        REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
        CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeTubeGeothermalPowerDigest() == source_hash);
        CHECK(ComputeTubePowerVehicleDigest() == bindings); CHECK(ComputeGeothermalGeneratorDigest() == native);
        fmt::print("GEOTHERMAL_BINDING_SAVE phase={} source={} binding={} utility={} native={} path={}\n", phase, source_hash, bindings, utility, native, path.string());
    };
    save(0); REQUIRE(CloseTubeGeothermalPower(source.source) == TubePowerError::None); save(1);
    REQUIRE(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, rig.plant_port, std::make_tuple(rig.plant_port)).Succeeded());
    save(2);
    REQUIRE(Command<Commands::SellVehicle>::Do(DoCommandFlag::Execute, first->index, true, false, ClientID::Invalid).Succeeded());
    auto *replacement = rig.Buy(); CHECK(GetTubePowerVehicle(replacement->index) == nullptr); save(3);
    REQUIRE(Company::CanAllocateItem()); successor.reset(Company::Create()); successor->money = 1000000; successor->clear_limit = 64U << 16;
    ChangeOwnershipOfCompanyItems(_current_company, successor->index); save(4);
}

static std::function<void()> *_geothermal_binding_observer = nullptr; ///< Process-local ordinary afterload observation, never a simulation/resource owner.

/** @param arguments Actual startup command with no caller-created state. @return Success after the original ordinary loader and observer complete. */
static bool ObserveNativeGeothermalBinding(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_geothermal_binding_observer != nullptr); (*_geothermal_binding_observer)(); return true;
}

/** @param name Explicit environment field supplied by the owning focused runner. @return Independently expected bounded unsigned scalar; malformed input aborts the test before world mutation. */
static uint64_t GeothermalBindingExpected(const char *name)
{
	const char *text = std::getenv(name); REQUIRE(text != nullptr);
	uint64_t value = 0; const auto parsed = std::from_chars(text, text + strlen(text), value);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == text + strlen(text)); return value;
}

/** Actual executable startup independently verifies copied preview/refusal, exact native shared history and repeated ordinary resaving. */
TEST_CASE("Native geothermal tube bindings survive ordinary files and independent preview", "[.tube-geothermal-binding-native-file]")
{
	const char *input = std::getenv("OTTD_GEOTHERMAL_BINDING_INPUT"), *action_text = std::getenv("OTTD_GEOTHERMAL_BINDING_ACTION");
	REQUIRE(input != nullptr); REQUIRE(action_text != nullptr); const std::string_view action{action_text};
	REQUIRE((action == "inspect" || action == "preview-refusal" || action == "full-refusal"));
	const auto phase = GeothermalBindingExpected("OTTD_GEOTHERMAL_BINDING_PHASE"); REQUIRE((phase <= 4 || phase == 100));
	const auto expected_binding = GeothermalBindingExpected("OTTD_GEOTHERMAL_BINDING_BINDING");
	const auto expected_source = GeothermalBindingExpected("OTTD_GEOTHERMAL_BINDING_SOURCE");
	const auto expected_utility = GeothermalBindingExpected("OTTD_GEOTHERMAL_BINDING_UTILITY"), expected_native = GeothermalBindingExpected("OTTD_GEOTHERMAL_BINDING_NATIVE");
	const auto paths = GetTestCaseArtifactPaths("tubes", "geothermal-bindings"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe accepted native owners before any simulation opportunity; preview and malformed copies cannot change their original history. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; if (phase < 5) { CheckNativeGeothermalBindingFile(static_cast<unsigned>(phase)); } else { CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None); }
			CHECK(ComputeTubePowerVehicleDigest() == expected_binding); CHECK(ComputeTubeGeothermalPowerDigest() == expected_source); CHECK(ComputeUtilityPowerDigest() == expected_utility); CHECK(ComputeGeothermalGeneratorDigest() == expected_native);
			const auto cash = Company::Get(_current_company)->money; const auto clock = TimerGameTick::counter;
			const auto random = std::array{_random.state[0], _random.state[1]};
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			if (action != "inspect") {
				const char *bad = std::getenv("OTTD_GEOTHERMAL_BINDING_CORRUPT"); REQUIRE(bad != nullptr);
				if (action == "preview-refusal") {
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				} else {
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
					REQUIRE(SaveOrLoad(input, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				}
			}
			CHECK(ComputeTubePowerVehicleDigest() == expected_binding); CHECK(ComputeTubeGeothermalPowerDigest() == expected_source); CHECK(ComputeUtilityPowerDigest() == expected_utility); CHECK(ComputeGeothermalGeneratorDigest() == expected_native);
			CHECK(Company::Get(_current_company)->money == cash); CHECK(TimerGameTick::counter == clock);
			CHECK(std::array{_random.state[0], _random.state[1]} == random);
			if (phase < 5) { CheckNativeGeothermalBindingFile(static_cast<unsigned>(phase)); } else { CHECK(ValidateTubePowerVehicles(CaptureTubePowerVehicles()) == TubePowerError::None); }
			if (action == "inspect") {
				AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
				const auto saved = paths.Output("saves", fmt::format("{}-reload.sav", std::filesystem::path{input}.stem().string()));
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok);
				fmt::print("GEOTHERMAL_BINDING_READ phase={} path={}\n", phase, saved.string());
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_geothermal_binding_observer, &inspect}; IConsole::CmdRegister("geothermal_binding_check", ObserveNativeGeothermalBinding);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
