/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_load.cpp Actual dated construction files, copied scalar refusal and ordinary native reload/continuation, including genuine427 compatibility.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8C/8E.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "geothermal_generator_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../geothermal_generator_wire.h"
#include "../saveload/saveload_func.h"
#include "../console_internal.h"
#include "../gfx_func.h"
#include "../table/sprites.h"

/** Copied native field validation cannot normalize bad booleans, receipts or unknown models into paid labour. */
TEST_CASE("Native powerhouse wire retains exact original fields and refuses malformed scalars", "[geothermal-generator-wire]")
{
	GeothermalGeneratorRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(148);
	const auto state = *GetGeothermalGeneratorState(rig.Generator().index);
	const auto wire = EncodeGeothermalGenerator(state);
	REQUIRE(ValidateGeothermalGeneratorWire(wire)); CHECK(DecodeGeothermalGenerator(wire) == state);
	for (unsigned fault = 0; fault < 8; ++fault) {
		auto bad = wire;
		// Exact transport bounds apply before typed booleans or native pointers
		// exist. Every independent malformed field must refuse without live repair.
		switch (fault) {
			case 0: bad.industry = 64000; break;
			case 1: bad.site = UINT32_MAX; break;
			case 2: bad.owner = MAX_COMPANIES; break;
			case 3: bad.model = UINT8_MAX; break;
			case 4: bad.work_ticks = 593; break;
			case 5: bad.materials_committed = 2; break;
			case 6: bad.has_work_tick = 2; break;
			case 7: bad.work_ticks = 0; break;
		}
		CHECK_FALSE(ValidateGeothermalGeneratorWire(bad));
	}
}

/** @param model Actual dated machinery. Preserve its original site, explicit boundary-delivered stock and prepaid work, with no fake carrier or electrical account. */
static void WriteGeothermalConstructionFiles(GeothermalGeneratorModel model)
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	GeothermalGeneratorRig rig{true, model}; REQUIRE(rig.Fund().Succeeded());
	const auto *terms = GetMultiLayerGeothermalConstruction(rig.request.kind); REQUIRE(terms != nullptr);
	const auto steel = terms->inputs.front().quantity; REQUIRE(terms->inputs.front().label == CargoLabel{"STEL"});
	LayerSaveContent content{rig.ActivatedContent()};
	AutoRestoreBackup local{_local_company, _current_company};
	(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
	const auto paths = GetTestCaseArtifactPaths("geothermal", "generator-construction");
	const Money cash = Company::Get(_current_company)->money;
	/** @param stage Explicit independently retained native boundary; no save recreates stock, work or a utility source. */
	const auto save = [&](std::string_view stage) {
		REQUIRE(ValidateGeothermalGenerators(CaptureGeothermalGenerators()));
		const auto before = ComputeGeothermalGeneratorDigest();
		SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
		AutoRestoreBackup menu{_game_mode, GameMode::Menu};
		AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		const auto path = paths.Output("saves", fmt::format("generator-{}.sav", stage));
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
		CHECK(ComputeGeothermalGeneratorDigest() == before); CHECK(Company::Get(_current_company)->money == cash);
		fmt::print("GEOTHERMAL_GENERATOR_SAVE stage={} work={} cash={} path={}\n", stage,
			GetGeothermalGeneratorState(rig.Generator().index)->work_ticks, cash.base(), path.string());
	};
	REQUIRE(rig.Deliver(CargoLabel{"STEL"}, steel) == steel); save("shortage");
	// Complete only the remaining real construction inputs. These explicit
	// receipts are a persistence fixture, not the later driven paid first plant.
	for (const auto &input : terms->inputs) {
		if (input.label != CargoLabel{"STEL"}) REQUIRE(rig.Deliver(input.label, input.quantity) == input.quantity);
	}
	rig.Tick(296); save("partial"); rig.Tick(static_cast<uint16_t>(terms->work_ticks - 296)); save("complete");
	CHECK(IsGeothermalGeneratorConstructionComplete(rig.Generator().index));
	CHECK(CaptureUtilityPowerState().producers.empty());
}

/** Actual first-model construction checkpoints retain existing whole-bill and592-tick native semantics. */
TEST_CASE("Native powerhouse construction writes genuine unfinished and completed files", "[geothermal-generator-save-file]")
{
	WriteGeothermalConstructionFiles(GeothermalGeneratorModel::VentTap);
}

/** Actual430 advanced construction checkpoints retain shortage stock, partial labour and the complete888-tick native receipt. */
TEST_CASE("Native Magma construction writes genuine unfinished and completed files", "[geothermal-magma-construction-save-file]")
{
	WriteGeothermalConstructionFiles(GeothermalGeneratorModel::MagmaLoop);
}

static std::function<void()> *_generator_file_observer = nullptr; ///< Process-local test observer; never saved or used as gameplay authority.

/** @param arguments Exact native startup console invocation. @return Success after ordinary native afterload has accepted the actual world. */
static bool ObserveGeothermalGeneratorFile(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_generator_file_observer != nullptr); (*_generator_file_observer)(); return true;
}

/** Genuine ordinary startup validates original native machinery/stock/work, refuses malformed copies and continues only retained construction. */
TEST_CASE("Native powerhouse construction survives ordinary loading and copied previews", "[.geothermal-generator-native-file]")
{
	const char *input = std::getenv("OTTD_GEOTHERMAL_GENERATOR_INPUT"); REQUIRE(input != nullptr);
	const char *action_text = std::getenv("OTTD_GEOTHERMAL_GENERATOR_ACTION"); REQUIRE(action_text != nullptr);
	const char *expected_text = std::getenv("OTTD_GEOTHERMAL_GENERATOR_EXPECTED"); REQUIRE(expected_text != nullptr);
	const std::string_view action{action_text};
	const char *model_text = std::getenv("OTTD_GEOTHERMAL_GENERATOR_MODEL");
	const auto model = model_text != nullptr && std::string_view{model_text} == "magma-loop" ? GeothermalGeneratorModel::MagmaLoop : GeothermalGeneratorModel::VentTap; // An independently declared expected model catches substitution; historical427 reader invocations preserve the first-model default.
	const auto *terms = GetMultiLayerGeothermalConstruction(GetMultiLayerGeothermalGeneratorTerms(model)->construction);
	const Money expected_cash{1000000 - terms->capital_cost};
	// A typo in a harness operation must refuse instead of silently skipping
	// the intended continuation or malformed-file admission check.
	REQUIRE((action == "inspect" || action == "continue" || action == "legacy" || action == "preview-refusal" || action == "full-refusal" || action == "native-refusal"));
	uint16_t expected = 0; const auto parsed = std::from_chars(expected_text, expected_text + strlen(expected_text), expected);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == expected_text + strlen(expected_text));
	const auto paths = GetTestCaseArtifactPaths("geothermal", "generator-construction");
	const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Inspect accepted native owners; independent preview never borrows their stock, work, RNG or clock. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			const auto before = ComputeGeothermalGeneratorDigest();
			const auto clock = TimerGameTick::counter; const auto random = std::array{_random.state[0], _random.state[1]};
			const auto cash = Company::Get(_local_company)->money;
			const auto states = CaptureGeothermalGenerators();
			if (action == "legacy") {
				CHECK(states.empty());
			} else {
				REQUIRE(states.size() == 1); REQUIRE(ValidateGeothermalGenerators(states));
				// Loading preserves the actual once-only dated construction invoice;
				// neither a shortage nor a partial receipt recreates that money.
				CHECK(cash == expected_cash); CHECK(states.front().model == model);
				CHECK(states.front().work_ticks == expected);
				REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				CHECK(ComputeGeothermalGeneratorDigest() == before); CHECK(TimerGameTick::counter == clock);
				CHECK(std::array{_random.state[0], _random.state[1]} == random); CHECK(Company::Get(_local_company)->money == cash);
				const auto id = states.front().industry;
				if (action == "preview-refusal" || action == "full-refusal" || action == "native-refusal") {
					const char *bad = std::getenv("OTTD_GEOTHERMAL_GENERATOR_CORRUPT"); REQUIRE(bad != nullptr);
					if (action == "preview-refusal") {
						CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
						CHECK(ComputeGeothermalGeneratorDigest() == before); CHECK(TimerGameTick::counter == clock);
						CHECK(std::array{_random.state[0], _random.state[1]} == random); CHECK(Company::Get(_local_company)->money == cash);
						REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					} else {
						if (action == "native-refusal") REQUIRE(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
						REQUIRE(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
						CHECK(CaptureGeothermalGenerators().empty());
						REQUIRE(SaveOrLoad(input, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
						CHECK(ComputeGeothermalGeneratorDigest() == before);
						CHECK(Company::Get(_local_company)->money == expected_cash);
					}
				} else {
					auto &industry = *Industry::Get(id);
					CHECK(IsGeothermalGeneratorConstructionComplete(id) == (expected == terms->work_ticks));
					CHECK(industry.GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting == (expected == 0 ? terms->inputs.front().quantity : 0));
					CHECK(CaptureUtilityPowerState().producers.empty());
					if (action == "continue") {
						AutoRestoreBackup pause{_pause_mode, PauseModes{}};
						// Continue the retained boundary-delivered bill, supplying only
						// missing native receipt units; no source invoice is recreated.
						for (const auto &term : terms->inputs) {
							const auto cargo = GetCargoTypeByLabel(term.label); const auto room = GetGeothermalGeneratorInputRoom(industry, cargo);
							if (room != 0) REQUIRE(ReceiveGeothermalGeneratorCargo(industry, cargo, room) == room);
						}
						for (uint16_t tick = expected; tick < terms->work_ticks; ++tick) { ++TimerGameTick::counter; OnTick_Industry(); }
						CHECK(IsGeothermalGeneratorConstructionComplete(id)); CHECK(GetGeothermalGeneratorState(id)->work_ticks == terms->work_ticks);
						CHECK(Company::Get(_local_company)->money == cash); CHECK(CaptureUtilityPowerState().producers.empty());
					}
				}
			}
			// A refusal produces no repaired file that could be mistaken for an
			// accepted checkpoint; successful original/continued loads get new names.
			if (action == "inspect" || action == "continue" || action == "legacy") {
				AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
				AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
				const auto saved = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action));
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok);
				fmt::print("GEOTHERMAL_GENERATOR_READ action={} work={} cash={} path={}\n", action,
					CaptureGeothermalGenerators().empty() ? 0 : CaptureGeothermalGenerators().front().work_ticks, Company::Get(_local_company)->money.base(), saved.string());
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_generator_file_observer, &inspect}; IConsole::CmdRegister("geothermal_generator_check", ObserveGeothermalGeneratorFile);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
