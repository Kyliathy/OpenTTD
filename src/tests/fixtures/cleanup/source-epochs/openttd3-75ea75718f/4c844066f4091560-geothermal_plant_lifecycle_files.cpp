/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_plant_lifecycle_files.cpp Ordinary readers and untouched next opportunities for original operating retirement/reconstruction worlds.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1CD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * No fixture builder or lifecycle command participates; complete native afterload/preview/frame owners retain all quantities.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_first_plant_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../misc_cmd.h"
#include "../openttd.h"
#include "../video/null_v.h"
#include "../gfx_func.h"
#include "../window_func.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

static std::function<void()> *_lifecycle_file_start = nullptr; ///< Invocation-only ordinary afterload callback, never a reconstructed participant/account.
static std::function<void()> *_lifecycle_file_frame = nullptr; ///< Observation after every complete ordinary native frame.

/** Explicit-only short native driver for a pure ordinary read or exactly one actual simulation opportunity. */
class PlantLifecycleFileDriver final : public VideoDriver_Null {
public:
	/** Run complete normal frames, allowing native graph-loading pauses without inventing a tick or skipping an owner. */
	void MainLoop() override
	{
		// Ordinary afterload can temporarily wait for graph jobs; observing the
		// actual clock after the whole frame preserves the original next baseline.
		for (uint frame = 0; frame < 105; ++frame) {
			if (_exit_game) break;
			::GameLoop(); ::InputLoop(); ::UpdateWindows();
			if (_exit_game) break;
			if (_lifecycle_file_frame != nullptr) (*_lifecycle_file_frame)();
		}
	}
	/** @return Explicit file observer identity; automatic production video selection stays unchanged. */
	std::string_view GetName() const override { return "plant-lifecycle-file"; }
};

/** Zero-priority test-only factory; no test file/clock observer is linked into the production game. */
class PlantLifecycleFileFactory final : public DriverFactoryBase {
public:
	/** Register the explicitly selected complete-frame reader driver. */
	PlantLifecycleFileFactory() : DriverFactoryBase(Driver::Type::Video, 0, "plant-lifecycle-file", "Original plant lifecycle file reader") {}
	/** @return Ordinary native null driver with one bounded observation point after all native owners. */
	std::unique_ptr<Driver> CreateInstance() const override { return std::make_unique<PlantLifecycleFileDriver>(); }
};
static PlantLifecycleFileFactory _plant_lifecycle_file_factory; ///< Test registration only, with no production selection effect.

/** @param arguments Actual native startup command. @return Success after observing the ordinarily loaded original. */
static bool StartPlantLifecycleFile(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_lifecycle_file_start != nullptr); (*_lifecycle_file_start)(); return true;
}

/** @return Actual authoritative digest/clock/account view for any retired, cold, unfinished or operating stage, without requiring machinery that was legitimately removed. */
static std::string CapturePlantLifecycleFile()
{
	return fmt::format("tick={},year={},cash={},geology={},projects={},exchange={},supply={},risers={},sites={},collectors={},generators={},utility={},industries={},vehicles={},packets={},",
		TimerGameTick::counter, TimerGameCalendar::year.base(), Company::Get(_current_company)->money.base(), ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(),
		ComputeTBMExchangeDigest(), ComputeSurfaceSupplyDigest(), ComputeGeothermalRiserDigest(), ComputeGeothermalSiteDigest(), ComputeGeothermalCollectorDigest(), ComputeGeothermalGeneratorDigest(),
		ComputeUtilityPowerDigest(), Industry::GetNumItems(), Vehicle::GetNumItems(), CargoPacket::GetNumItems());
}

/** @param paths Invocation-owned destinations. @param stage Actual read/repeat/next phase. @return Pure native output path; both world and independent interactive RNG remain unchanged. */
static std::filesystem::path SavePlantLifecycleFile(const TestArtifactPaths &paths, std::string_view stage)
{
	const auto snapshot = CapturePlantLifecycleFile(); const auto random = std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	const char *label = std::getenv("OTTD_PLANT_FILE_LABEL"); REQUIRE(label != nullptr);
	const auto path = paths.Output("saves", fmt::format("plant-lifecycle-file-{}-{}.sav", label, stage));
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup format{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CapturePlantLifecycleFile() == snapshot); CHECK((std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random));
	fmt::print("PLANT_LIFECYCLE_FILE stage={} snapshot={} path={}\n", stage, snapshot, path.string()); return path;
}

/** Every original lifecycle stage must survive independent native preview/read and, when requested, exactly its untouched ordinary next opportunity. */
TEST_CASE("Original geothermal lifecycle worlds retain complete native readers and next opportunities", "[.geothermal-plant-lifecycle-file]")
{
	const char *input = std::getenv("OTTD_PLANT_FILE_INPUT"), *action_text = std::getenv("OTTD_PLANT_FILE_ACTION"); REQUIRE(input != nullptr); REQUIRE(action_text != nullptr);
	const std::string_view action{action_text}; REQUIRE((action == "inspect" || action == "next")); // Pure reads never advance time; continuation permits only the actual original full next opportunity.
	const auto paths = GetTestCaseArtifactPaths("geothermal", "lifecycle-files"); std::exception_ptr failure; bool started = false, completed = false; uint64_t tick = 0;
	/** Ordinary afterload and copied preview retain every original owner; saved read/repeat worlds make purity independently comparable outside the process. */
	std::function<void()> begin = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
			REQUIRE(GetGeologyWorldState().vents.size() == 1); CHECK(GetGeologyWorldState().vents.front().thermal_capacity_w == 1000000);
			const auto before = CapturePlantLifecycleFile(); const auto random = std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(CapturePlantLifecycleFile() == before); CHECK((std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random));
			SavePlantLifecycleFile(paths, "read"); SavePlantLifecycleFile(paths, "repeat");
			if (action == "inspect") { completed = true; _exit_game = true; return; }
			tick = TimerGameTick::counter; REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded()); started = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	/** Observe the first complete native opportunity, preserving all actual movement, construction, source rotation, service and town settlement. */
	std::function<void()> frame = [&] {
		if (!started) return;
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			if (TimerGameTick::counter == tick) return; // A native loading/graph pause has no simulation opportunity to certify.
			REQUIRE(TimerGameTick::counter == tick + 1); SavePlantLifecycleFile(paths, "next"); SavePlantLifecycleFile(paths, "next-repeat");
			completed = true; started = false; _exit_game = true;
		} catch (...) { failure = std::current_exception(); started = false; _exit_game = true; }
	};
	AutoRestoreBackup startup{_lifecycle_file_start, &begin}; AutoRestoreBackup observer{_lifecycle_file_frame, &frame}; IConsole::CmdRegister("plant_lifecycle_file", StartPlantLifecycleFile);
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "plant-lifecycle-file:ticks=105", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(completed);
}
