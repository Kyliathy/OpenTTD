/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_power_local_feed_load.cpp Genuine432 local-feed files, independent copied preview and ordinary executable reload/refusal.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group9C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Prepared floor and explicit delivered stock remain separate from paid-TBM/bootstrap and capsule-work acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "utility_power_local_feed_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../saveload/saveload_func.h"
#include "../console_internal.h"
#include "../gfx_func.h"
#include "../table/sprites.h"
#include "../safeguards.h"

/** @param phase Independent connected0, split/released1, cut2 or repaired3 checkpoint. Check real original native identities and shared quantities without creating work or power. */
static void CheckNativeLocalFeedFile(unsigned phase)
{
	const auto plants = CaptureGeothermalGenerators(); REQUIRE(plants.size() == 1);
	const auto &plant = plants.front(); CHECK(plant.model == GeothermalGeneratorModel::MagmaLoop);
	CHECK(plant.active_service_ticks == 2); CHECK(plant.service_remainder == 70);
	REQUIRE(IsGeothermalGeneratorConstructionComplete(plant.industry)); REQUIRE(ValidateGeothermalGenerators(plants));
	const auto *output = GetUtilityPowerProducer(plant.electrical_producer); REQUIRE(output != nullptr);
	CHECK(output->kind == UtilityGenerationKind::HeatConversion); CHECK(output->meter.generation_energy_j == 9450);
	CHECK(output->generated_lifetime_j == 18900); CHECK(output->consumed_lifetime_j == 0);
	CHECK(GetTubePowerConnection(plant.electrical_producer) == nullptr);
	std::vector<UtilityLinkID> feeds;
	// Native role order, not a guessed account/industry number, identifies both
	// original inlets after all ordinary loader fixups have completed.
	for (const auto &link : GetUtilityPowerLinks()) {
		if (link.kind != UtilityLinkKind::ElectricalFeed) continue;
		CHECK(link.active); CHECK(link.generator == plant.industry);
		CHECK(link.source_endpoint == GetGeothermalGeneratorPort(plant.industry));
		REQUIRE(GetTubeTerminal(link.destination_endpoint, plant.owner).has_value()); feeds.push_back(link.id);
	}
	REQUIRE(feeds.size() == 2);
	const auto *first = GetUtilityPowerLink(feeds[0]), *second = GetUtilityPowerLink(feeds[1]);
	CHECK(first->source == second->source); CHECK(first->flow_j == (phase == 0 ? 0 : 3000)); CHECK(second->flow_j == (phase == 0 ? 0 : 2400));
	const auto *supply = GetUtilityPowerConsumer(first->source); REQUIRE(supply != nullptr);
	CHECK(supply->linked_producers == std::vector{plant.electrical_producer}); REQUIRE(supply->shares.size() == 1);
	CHECK(supply->shares.front().transferred_j == (phase == 0 ? 0 : 5400));
	// Every child retains its original generation and parent even after physical
	// loss. Repair never restores the released quantity or a spent cell's rate.
	for (const auto id : feeds) {
		const auto *link = GetUtilityPowerLink(id); const auto *consumer = GetUtilityPowerConsumer(link->destination);
		REQUIRE(consumer != nullptr); CHECK(consumer->linked_producers.empty()); CHECK(consumer->effect == UtilityConsumerEffect::TubeWork);
		if (phase == 0) { CHECK(consumer->shares.empty()); continue; }
		REQUIRE(consumer->shares.size() == 1); const auto &share = consumer->shares.front();
		CHECK(share.producer == plant.electrical_producer); CHECK(share.parent == supply->shares.front().id); CHECK(share.imported_link == id);
		CHECK(share.allocated_j == link->flow_j); CHECK(share.consumed_j == 0);
		CHECK(share.released_j == (phase >= 2 ? share.allocated_j : id == feeds[0] ? 1000 : 0));
	}
	const auto cable = GetGeothermalUtilityLink(TileXYL(35, 24, MapLayer::DeepUnderground), UtilityEnergyDomain::Electricity);
	CHECK(cable.has_value() == (phase != 2)); CHECK(ValidateGeothermalUtilityLinks());
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Real current files retain native inlets, original source shares, released rights and purchased replacement-cell identity. */
TEST_CASE("Native local feed writes genuine ordinary worlds", "[utility-power-local-feed-save-file]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	NativeLocalFeedRig rig{GeothermalGeneratorModel::MagmaLoop, true};
	LayerSaveContent content{rig.ActivatedContent()}; AutoRestoreBackup local{_local_company, _current_company};
	(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
	const auto paths = GetTestCaseArtifactPaths("utilities", "local-feed");
	/** @param phase Actual retained native checkpoint. Save without any owner, simulation-time, fee or RNG mutation. */
	const auto save = [&](unsigned phase) {
		CheckNativeLocalFeedFile(phase); const auto utility = ComputeUtilityPowerDigest(); const auto native = ComputeGeothermalGeneratorDigest();
		SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
		AutoRestoreBackup menu{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		const auto path = paths.Output("saves", fmt::format("local-feed-{}.sav", phase));
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
		CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ComputeGeothermalGeneratorDigest() == native);
		fmt::print("LOCAL_FEED_SAVE phase={} utility={} native={} path={}\n", phase, utility, native, path.string());
	};
	save(0); rig.Transfer(0, 3000);
	REQUIRE(CommitUtilityMutation(PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {rig.recipients[0], GetUtilityPowerInterval()}, 1000)).error == UtilityPowerError::None);
	rig.Transfer(1, 2400); save(1);
	const auto broken = TileXYL(35, 24, MapLayer::DeepUnderground);
	REQUIRE(Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, broken,
		std::make_tuple(broken, UtilityEnergyDomain::Electricity)).Succeeded());
	save(2); rig.Build(broken, UtilityEnergyDomain::Electricity); save(3);
}

static std::function<void()> *_local_feed_observer = nullptr; ///< Process-local ordinary afterload observation, never a simulation/resource owner.

/** @param arguments Actual startup command with no caller-created state. @return Success after the original ordinary loader and observer complete. */
static bool ObserveNativeLocalFeed(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_local_feed_observer != nullptr); (*_local_feed_observer)(); return true;
}

/** @param name Explicit environment field supplied by the owning focused runner. @return Independently expected bounded unsigned scalar; malformed input aborts the test before world mutation. */
static uint64_t LocalFeedExpected(const char *name)
{
	const char *text = std::getenv(name); REQUIRE(text != nullptr);
	uint64_t value = 0; const auto parsed = std::from_chars(text, text + strlen(text), value);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == text + strlen(text)); return value;
}

/** Actual executable startup independently verifies copied preview/refusal, exact native shared history and repeated ordinary resaving. */
TEST_CASE("Native local feed survives ordinary files and independent preview", "[.utility-power-local-feed-native-file]")
{
	const char *input = std::getenv("OTTD_LOCAL_FEED_INPUT"), *action_text = std::getenv("OTTD_LOCAL_FEED_ACTION");
	REQUIRE(input != nullptr); REQUIRE(action_text != nullptr); const std::string_view action{action_text};
	REQUIRE((action == "inspect" || action == "preview-refusal" || action == "full-refusal"));
	const auto phase = LocalFeedExpected("OTTD_LOCAL_FEED_PHASE"); REQUIRE(phase <= 3);
	const auto expected_utility = LocalFeedExpected("OTTD_LOCAL_FEED_UTILITY"), expected_native = LocalFeedExpected("OTTD_LOCAL_FEED_NATIVE");
	const auto paths = GetTestCaseArtifactPaths("utilities", "local-feed"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Observe accepted native owners before any simulation opportunity; preview and malformed copies cannot change their original history. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; CheckNativeLocalFeedFile(static_cast<unsigned>(phase));
			CHECK(ComputeUtilityPowerDigest() == expected_utility); CHECK(ComputeGeothermalGeneratorDigest() == expected_native);
			const auto cash = Company::Get(_current_company)->money; const auto clock = TimerGameTick::counter;
			const auto random = std::array{_random.state[0], _random.state[1]};
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			if (action != "inspect") {
				const char *bad = std::getenv("OTTD_LOCAL_FEED_CORRUPT"); REQUIRE(bad != nullptr);
				if (action == "preview-refusal") {
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
				} else {
					CHECK(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
					REQUIRE(SaveOrLoad(input, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
				}
			}
			CHECK(ComputeUtilityPowerDigest() == expected_utility); CHECK(ComputeGeothermalGeneratorDigest() == expected_native);
			CHECK(Company::Get(_current_company)->money == cash); CHECK(TimerGameTick::counter == clock);
			CHECK(std::array{_random.state[0], _random.state[1]} == random);
			CheckNativeLocalFeedFile(static_cast<unsigned>(phase));
			if (action == "inspect") {
				AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
				const auto saved = paths.Output("saves", fmt::format("{}-reload.sav", std::filesystem::path{input}.stem().string()));
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok);
				fmt::print("LOCAL_FEED_READ phase={} path={}\n", phase, saved.string());
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_local_feed_observer, &inspect}; IConsole::CmdRegister("local_feed_check", ObserveNativeLocalFeed);
	const std::string config_path = config.string();
	std::array<std::string_view, 17> arguments = {"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_path, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
