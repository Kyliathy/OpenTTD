/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_operating_load.cpp Genuine dated native operating references/service with independent copied preview and ordinary executable continuation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8DC-8E.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "geothermal_generator_operating_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../saveload/saveload_func.h"
#include "../console_internal.h"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include "../safeguards.h"

/** @param checkpoint Actual common opportunities0/1/3/4, blocked5 or repaired6. @param model Independent expected dated machinery; no file may silently substitute another model. Check original physical, utility, conversion and service/cash owners after ordinary loading. */
static void CheckNativeGeneratorFile(uint16_t checkpoint, GeothermalGeneratorModel model)
{
	const uint16_t ticks = checkpoint == 5 ? 4 : checkpoint == 6 ? 5 : checkpoint;
	const bool magma = model == GeothermalGeneratorModel::MagmaLoop;
	const uint16_t service = magma ? 35 : 20, efficiency = magma ? 35 : 15;
	const auto states = CaptureGeothermalGenerators(); REQUIRE(states.size() == 1);
	const auto &state = states.front(); CHECK(state.model == model); CHECK(state.work_ticks == (magma ? 888 : 592)); CHECK(state.materials_committed);
	CHECK(state.owner == _current_company); CHECK(state.active_service_ticks == ticks);
	CHECK(state.service_remainder == ticks * service % 74); CHECK(state.has_operating_tick == (ticks != 0));
	REQUIRE(ValidateGeothermalGenerators(states)); REQUIRE(ValidateGeothermalGeneratorPhysicalReferences());
	const auto collectors = CaptureGeothermalCollectors(); REQUIRE(collectors.size() == 1);
	CHECK(IsGeothermalCollectorOperational(collectors.front().industry)); CHECK(IsGeothermalGeneratorConstructionComplete(state.industry));
	uint64_t capital = magma ? 540000 : 360000;
	uint32_t retired = 0;
	// Every original paid cell retains its capital history after removal. The
	// blocked/repaired fixtures also pay its actual removal and replacement;
	// no restored checkpoint refunds either side of that physical repair.
	for (const auto &link : GetUtilityPowerLinks()) {
		REQUIRE(link.kind == UtilityLinkKind::LocalCell);
		const auto *terms = GetMultiLayerLocalUtilityLinkContract(link.local_domain);
		capital += terms->capital_cost;
		if (!link.active) { ++retired; capital += terms->removal_cost; }
	}
	CHECK(retired == (checkpoint >= 5 ? 1 : 0));
	CHECK(Company::Get(_current_company)->money == Money{1000000 - static_cast<int64_t>(capital) - ticks * service / 74});
	if (ticks == 0) {
		CHECK(state.heat_consumer.base() == UINT32_MAX); CHECK(state.electrical_producer.base() == UINT32_MAX);
		CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(CaptureUtilityPowerState().consumers.empty()); return;
	}
	const auto *output = GetUtilityPowerProducer(state.electrical_producer); REQUIRE(output != nullptr);
	const auto *source = GetUtilityPowerProducer(collectors.front().heat_producer); REQUIRE(source != nullptr);
	const uint16_t opportunities = checkpoint >= 5 ? checkpoint : ticks;
	const uint64_t gross = uint64_t{opportunities} * 2700 / 1000;
	const uint64_t heat = gross - (checkpoint >= 5 ? 3 : 0);
	CHECK(source->generated_lifetime_j == gross); CHECK(source->consumed_lifetime_j == heat);
	CHECK(source->meter.remainder_mj == opportunities * 2700 % 1000);
	CHECK(output->kind == UtilityGenerationKind::HeatConversion); CHECK(output->conversion_input == state.heat_consumer);
	CHECK(output->conversion_efficiency == efficiency); CHECK(output->conversion_capacity_w == (magma ? 350000 : 150000));
	CHECK(output->generated_lifetime_j == heat * efficiency / 100); CHECK(output->conversion_remainder == heat * efficiency % 100);
	REQUIRE(output->conversion_origins.size() == 1); CHECK(output->conversion_origins.front().producer == collectors.front().heat_producer);
	CHECK(output->conversion_origins.front().heat_j == heat); CHECK(IsUtilityPowerAllocated()); CHECK(IsUtilityHeatAllocated());
}

/** @param model Actual dated model whose own construction/service and shared conversion must survive all six independent ordinary worlds. */
static void WriteNativeGeneratorFiles(GeothermalGeneratorModel model)
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	GeothermalGeneratorOperatingRig rig{true, true, 100, model};
	LayerSaveContent content{rig.ActivatedContent()}; AutoRestoreBackup local{_local_company, _current_company};
	(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
	const auto paths = GetTestCaseArtifactPaths("geothermal", "generator-operation");
	/** @param ticks Independently retained actual common-driver opportunity count. */
	const auto save = [&](uint16_t ticks) {
		CheckNativeGeneratorFile(ticks, model); const auto utility = ComputeUtilityPowerDigest(); const auto native = ComputeGeothermalGeneratorDigest();
		SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
		AutoRestoreBackup menu{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		const auto path = paths.Output("saves", fmt::format("powerhouse-{}.sav", ticks));
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
		CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalGeneratorDigest() == native);
		fmt::print("GEOTHERMAL_OPERATION_SAVE ticks={} path={}\n", ticks, path.string());
	};
	save(0); rig.Operate(); save(1); rig.Operate(2); save(3); rig.Operate(); save(4);
	const auto broken = rig.pipe[rig.pipe.size() / 2];
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, broken,
		std::make_tuple(broken, UtilityEnergyDomain::Heat)).Succeeded());
	// A real paid path interruption publishes only unused source Heat. Native
	// conversion/service stays unchanged; repairing buys a fresh physical ID.
	rig.Operate(); save(5); rig.Build(broken, UtilityEnergyDomain::Heat); rig.Operate(); save(6);
}

/** Actual first-model worlds preserve the original native construction/service and shared fractional utility quantities. */
TEST_CASE("Native operating powerhouse writes genuine ordinary worlds", "[geothermal-generator-operation-save-file]")
{
	WriteNativeGeneratorFiles(GeothermalGeneratorModel::VentTap);
}

/** Actual advanced worlds retain their appended430 model identity without introducing another material/energy/save owner. */
TEST_CASE("Native Magma generator writes genuine430 ordinary worlds", "[geothermal-magma-save-file]")
{
	WriteNativeGeneratorFiles(GeothermalGeneratorModel::MagmaLoop);
}

static std::function<void()> *_operation_file_observer = nullptr; ///< Process-local observer after actual ordinary native afterload, never gameplay or saved authority.

/** @param arguments Exact native startup observer invocation. @return Success after complete actual world acceptance. */
static bool ObserveNativeGeneratorOperation(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_operation_file_observer != nullptr); (*_operation_file_observer)(); return true;
}

/** Ordinary executable startup independently checks preview/refusal/recovery and continues the original actual common driver with no restored cash or purchased replacement identities. */
TEST_CASE("Native operating powerhouse survives ordinary files and independent previews", "[.geothermal-generator-operation-native-file]")
{
	const char *input = std::getenv("OTTD_GEOTHERMAL_OPERATION_INPUT"); REQUIRE(input != nullptr);
	const char *action_text = std::getenv("OTTD_GEOTHERMAL_OPERATION_ACTION"); REQUIRE(action_text != nullptr);
	const char *ticks_text = std::getenv("OTTD_GEOTHERMAL_OPERATION_TICKS"); REQUIRE(ticks_text != nullptr);
	const std::string_view action{action_text};
	const char *model_text = std::getenv("OTTD_GEOTHERMAL_OPERATION_MODEL");
	const auto model = model_text != nullptr && std::string_view{model_text} == "magma-loop" ? GeothermalGeneratorModel::MagmaLoop : GeothermalGeneratorModel::VentTap; // The explicit fixture expectation is independent of the loaded model byte; older first-model runner invocations retain their original default.
	REQUIRE((action == "inspect" || action == "continue" || action == "preview-refusal" || action == "full-refusal"));
	uint16_t ticks = UINT16_MAX; const auto parsed = std::from_chars(ticks_text, ticks_text + strlen(ticks_text), ticks);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == ticks_text + strlen(ticks_text));
	REQUIRE((ticks == 0 || ticks == 1 || ticks == 3 || ticks == 4 || ticks == 5 || ticks == 6));
	const auto paths = GetTestCaseArtifactPaths("geothermal", "generator-operation"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Inspect accepted original native quantities and physical references; copied preview and rejected files cannot change the live world. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; CheckNativeGeneratorFile(ticks, model);
			const auto utility = ComputeUtilityPowerDigest(); const auto native = ComputeGeothermalGeneratorDigest();
			const auto cash = Company::Get(_current_company)->money; const auto clock = TimerGameTick::counter;
			const auto random = std::array{_random.state[0], _random.state[1]};
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalGeneratorDigest() == native);
			CHECK(TimerGameTick::counter == clock); CHECK(Company::Get(_current_company)->money == cash); CHECK(std::array{_random.state[0], _random.state[1]} == random);
			uint16_t result_ticks = ticks;
			// Structural preview and actual full reset/recovery are independent
			// admission guarantees. Neither rejected copy gets a repaired output.
			if (action == "preview-refusal" || action == "full-refusal") {
				const char *bad = std::getenv("OTTD_GEOTHERMAL_OPERATION_CORRUPT"); REQUIRE(bad != nullptr);
				if (action == "preview-refusal") {
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalGeneratorDigest() == native);
					CHECK(TimerGameTick::counter == clock); CHECK(Company::Get(_current_company)->money == cash); CHECK(std::array{_random.state[0], _random.state[1]} == random);
				} else {
					REQUIRE(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
					CHECK(CaptureGeothermalGenerators().empty()); CHECK(CaptureUtilityPowerState().producers.empty());
					REQUIRE(SaveOrLoad(input, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalGeneratorDigest() == native); CheckNativeGeneratorFile(ticks, model);
				}
				REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			} else {
				if (action == "continue") {
					AutoRestoreBackup pause{_pause_mode, PauseModes{}};
					const uint16_t count = ticks == 1 ? 2 : 1; REQUIRE((ticks < 4 || ticks == 5));
					if (ticks == 5) {
						/** @param link Actual saved physical identity. @return The single removed Heat cell whose repair must pay a new capital invoice. */
						const auto lost = std::ranges::find_if(GetUtilityPowerLinks(), [](const auto &link) {
							return !link.active && link.kind == UtilityLinkKind::LocalCell && link.local_domain == UtilityEnergyDomain::Heat;
						});
						REQUIRE(lost != GetUtilityPowerLinks().end()); const auto tile = lost->source_endpoint;
						REQUIRE(Command<Commands::BuildGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
							std::make_tuple(tile, UtilityEnergyDomain::Heat)).Succeeded());
					}
					// Advance original common opportunities only. Native construction,
					// real paid cells and exact utility references all remain retained.
					for (uint16_t index = 0; index < count; ++index) { ++TimerGameTick::counter; PrepareTubePowerTick(); }
					result_ticks += count; CheckNativeGeneratorFile(result_ticks, model);
				}
				AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
				const auto saved = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action));
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok);
				fmt::print("GEOTHERMAL_OPERATION_READ action={} ticks={} path={}\n", action, result_ticks, saved.string());
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_operation_file_observer, &inspect}; IConsole::CmdRegister("geothermal_operation_check", ObserveNativeGeneratorOperation);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
