/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_heat_conversion_load.cpp Genuine428 shared conversion files and ordinary loader/preview continuation; numerical core fixtures claim no physical plant.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8DB.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "utility_heat_conversion_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../geothermal_generator.h"
#include "../console_internal.h"
#include "../saveload/saveload_func.h"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include "../openttd.h"
#include "../timer/timer_game_tick.h"

/** Independent expected original numerical/cash receipt; actual quantities remain with the native shared owner and company. */
struct ConversionFileExpected {
	uint64_t heat; ///< Lifetime actual original thermal input.
	uint64_t electricity; ///< Lifetime exact electrical publication.
	uint8_t numerator; ///< Sole saved fractional-percent numerator.
	Money cash; ///< Independent actual company balance after complete five-Money boundary invoices.
};

/** @param stage Exact independently retained native boundary. @return Original expected receipts; legacy denotes the genuine427 native powerhouse construction file. */
static ConversionFileExpected ExpectedConversionFile(std::string_view stage)
{
	if (stage == "pending") return {0, 0, 0, Money{1000000}};
	if (stage == "fractional") return {1, 0, 15, Money{999995}};
	if (stage == "electric") return {7, 1, 5, Money{999990}};
	REQUIRE(stage == "legacy"); return {0, 0, 0, Money{820000}};
}

/** @param stage Exact original or continued fixture boundary. Validate every original numerical owner and actual bank receipt without mutation. */
static void CheckConversionFile(std::string_view stage)
{
	const auto expected = ExpectedConversionFile(stage); const auto state = CaptureUtilityPowerState();
	CHECK(Company::Get(_local_company)->money == expected.cash);
	if (stage == "legacy") {
		CHECK_FALSE(HasUtilityHeatConversionState()); CHECK(state.producers.empty());
		const auto plants = CaptureGeothermalGenerators(); REQUIRE(plants.size() == 1); CHECK(plants.front().work_ticks == 296);
		CHECK(ValidateGeothermalGenerators(plants)); return;
	}
	REQUIRE(state.producers.size() == 2); REQUIRE(state.consumers.size() == 1);
	const auto &heat = state.producers[0], &output = state.producers[1]; const auto &input = state.consumers[0];
	CHECK(HasUtilityHeatConversionState()); CHECK(heat.meter.domain == UtilityEnergyDomain::Heat); CHECK(heat.kind == UtilityGenerationKind::Geothermal);
	CHECK(input.effect == UtilityConsumerEffect::HeatGenerator); CHECK(output.kind == UtilityGenerationKind::HeatConversion);
	CHECK(output.conversion_input == input.meter.consumer_id); CHECK(output.conversion_efficiency == 15); CHECK(output.conversion_capacity_w == 150000);
	CHECK(output.conversion_remainder == expected.numerator); CHECK(output.meter.remainder_mj == 0);
	CHECK(output.generated_lifetime_j == expected.electricity); CHECK(output.consumed_lifetime_j == 0);
	CHECK(heat.consumed_lifetime_j == expected.heat); CHECK(heat.generated_lifetime_j == (stage == "electric" ? 54000 : 27000));
	if (expected.heat == 0) CHECK(output.conversion_origins.empty());
	else CHECK(output.conversion_origins == std::vector{UtilityHeatConversionOrigin{heat.meter.producer_id, expected.heat}});
	CHECK(ValidateUtilityPowerState(state, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Genuine native files preserve an unconsumed reservation, sub-joule conversion and actual exact electrical output without private balances or a physical plant substitute. */
TEST_CASE("Shared exact conversion writes genuine pending fractional and electrical files", "[utility-conversion-save-file]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	UtilityConversionRig rig{15, true}; LayerSaveContent content{rig.world.ActivatedContent()};
	AutoRestoreBackup local{_local_company, _current_company}; UtilityConversionBenefit benefit{_current_company};
	const auto paths = GetTestCaseArtifactPaths("geothermal", "atomic-conversion");
	/** @param stage Actual original shared numerical/expense boundary; saving cannot convert, charge, rotate or acquire any identity. */
	const auto save = [&](std::string_view stage) {
		CheckConversionFile(stage); const auto before = ComputeUtilityPowerDigest();
		SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE); AutoRestoreBackup menu{_game_mode, GameMode::Menu};
		AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		const auto path = paths.Output("saves", fmt::format("heat-conversion-{}.sav", stage));
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok); CHECK(ComputeUtilityPowerDigest() == before);
		const auto expected = ExpectedConversionFile(stage);
		fmt::print("UTILITY_CONVERSION_SAVE stage={} heat={} electricity={} numerator={} cash={} path={}\n", stage,
			expected.heat, expected.electricity, expected.numerator, expected.cash.base(), path.string());
	};
	rig.Allocate(1); save("pending"); REQUIRE(CommitUtilityHeatConversion(rig.Prepare(1), benefit).error == UtilityPowerError::None); save("fractional");
	rig.Rotate(); rig.Allocate(6); REQUIRE(CommitUtilityHeatConversion(rig.Prepare(6), benefit).error == UtilityPowerError::None); save("electric");
}

static std::function<void()> *_conversion_file_observer = nullptr; ///< Isolated ordinary-loader observer; never saved as gameplay state.

/** @param arguments Exact native startup command. @return Success after ordinary native loading and all afterload owners have accepted the actual world. */
static bool ObserveConversionFile(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_conversion_file_observer != nullptr); (*_conversion_file_observer)(); return true;
}

/** Ordinary native startup, independent preview and full error recovery preserve actual original quantities/carry/history and complete expense receipts. */
TEST_CASE("Shared exact conversion survives ordinary native loading and independent previews", "[.utility-conversion-native-file]")
{
	const char *input = std::getenv("OTTD_UTILITY_CONVERSION_INPUT"), *stage_text = std::getenv("OTTD_UTILITY_CONVERSION_STAGE"), *action_text = std::getenv("OTTD_UTILITY_CONVERSION_ACTION");
	REQUIRE(input != nullptr); REQUIRE(stage_text != nullptr); REQUIRE(action_text != nullptr);
	const std::string_view stage{stage_text}, action{action_text};
	// Each declared operation has its own acceptance contract; a misspelled
	// observer mode cannot silently skip continuation or refusal assertions.
	REQUIRE((action == "inspect" || action == "continue" || action == "legacy" || action == "preview-refusal" || action == "full-refusal"));
	const auto paths = GetTestCaseArtifactPaths("geothermal", "atomic-conversion"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Inspect only accepted native owners; preview and refusal cannot alter their original numerical state, company money, RNG or clock. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; CheckConversionFile(stage);
			const auto before = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_local_company)->money;
			const auto random = std::array{_random.state[0], _random.state[1]}; const auto clock = TimerGameTick::counter;
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(ComputeUtilityPowerDigest() == before); CHECK(Company::Get(_local_company)->money == cash);
			CHECK(std::array{_random.state[0], _random.state[1]} == random); CHECK(TimerGameTick::counter == clock);
			std::string_view result_stage = stage;
			// Structural preview refusal and real full-load reset/recovery are
			// independent guarantees; neither may create a repaired accepted file.
			if (action == "preview-refusal" || action == "full-refusal") {
				const char *bad = std::getenv("OTTD_UTILITY_CONVERSION_CORRUPT"); REQUIRE(bad != nullptr);
				if (action == "preview-refusal") {
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
					CHECK(ComputeUtilityPowerDigest() == before); CHECK(Company::Get(_local_company)->money == cash);
					CHECK(std::array{_random.state[0], _random.state[1]} == random); CHECK(TimerGameTick::counter == clock);
				} else {
					REQUIRE(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
					CHECK_FALSE(HasUtilityHeatConversionState()); CHECK(CaptureUtilityPowerState().producers.empty());
					REQUIRE(SaveOrLoad(input, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(ComputeUtilityPowerDigest() == before); CheckConversionFile(stage);
				}
				REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			} else {
				if (action == "continue") {
					AutoRestoreBackup pause{_pause_mode, PauseModes{}}; UtilityConversionBenefit benefit{_current_company};
					REQUIRE((stage == "pending" || stage == "fractional"));
					if (stage == "fractional") {
						// Only a new original shared epoch can publish again; no saved
						// output/reservation is renewed as the next interval's supply.
						{ AutoRestoreBackup system{_current_company, OWNER_DEITY}; REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), {}) == UtilityPowerError::None); REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None); }
						REQUIRE(PublishUtilityHeatGeneration(UtilityProducerID{0}, GetUtilityPowerInterval(), 1000000, 1) == UtilityPowerError::None);
						{ AutoRestoreBackup system{_current_company, OWNER_DEITY}; REQUIRE(AllocateUtilityHeat(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{UtilityConsumerID{0}, 6}}) == UtilityPowerError::None); }
					}
					const auto plan = PrepareUtilityHeatConversion({{UtilityConsumerID{0}, GetUtilityPowerInterval()}, UtilityProducerID{1}, stage == "pending" ? 1U : 6U, 1});
					REQUIRE(plan.error == UtilityPowerError::None); REQUIRE(CommitUtilityHeatConversion(plan, benefit).error == UtilityPowerError::None);
					result_stage = stage == "pending" ? "fractional" : "electric"; CheckConversionFile(result_stage);
				}
				AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
				const auto saved = paths.Output("saves", fmt::format("{}-{}.sav", std::filesystem::path{input}.stem().string(), action));
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok); const auto expected = ExpectedConversionFile(result_stage);
				fmt::print("UTILITY_CONVERSION_READ action={} stage={} heat={} electricity={} numerator={} cash={} path={}\n", action, result_stage,
					expected.heat, expected.electricity, expected.numerator, expected.cash.base(), saved.string());
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_conversion_file_observer, &inspect}; IConsole::CmdRegister("utility_conversion_check", ObserveConversionFile);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
