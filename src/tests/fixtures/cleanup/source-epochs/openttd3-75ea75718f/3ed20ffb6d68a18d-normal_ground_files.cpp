/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_files.cpp Actual normal-start work checkpoints and complete ordinary native readers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6DD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "normal_ground_boring_fixture.h"
#include "../linkgraph/linkgraphschedule.h"
#include <thread>
#include "ground_performance_fixture.h"
#include "../safeguards.h"

/** @return Actual train-held spoil without changing packet ownership or the participant. */
static uint64_t NormalGroundHeld(const Train &front)
{
	uint64_t held = 0;
	for (const auto *part = &front; part != nullptr; part = part->Next()) held += part->cargo.StoredCount();
	return held;
}

/** Save an actual complete frame under public pause;resume only through the same native public control.
 * @param label Invocation-owned working/loaded/next identity.
 */
static void SaveNormalGroundCheckpoint(std::string_view label)
{
	AutoRestoreBackup actor{_current_company, _local_company};
	REQUIRE(_pause_mode.None()); REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
	const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-checkpoints");
	for (const auto suffix : {"", "-repeat"})
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", fmt::format("{}{}.sav", label, suffix))), false) == SaveLoadResult::Ok);
	const auto projects = CaptureTBMProjects(); REQUIRE(projects.size() == 1);
	const auto &project = projects.front(); const auto *job = GetExcavationJob(project.job); REQUIRE(job != nullptr);
	const auto assignments = CaptureTBMExchangeAssignments(); REQUIRE(assignments.size() == 1);
	const auto *hauler = Train::Get(assignments.front().hauler);
	fmt::print("NORMAL_GROUND_CHECKPOINT label={} tick={} cursor={} routes={} earth={} rock={} work_cost={} phase={} cutter_held={} hauler_held={} hauler_tile={} cash={} loan={} returned={} resumed={}\n",
		label, TimerGameTick::counter, job->cursor, job->route.size(), job->completed_earth_t, job->completed_rock_t,
		job->completed_work_cost.base(), to_underlying(project.phase), NormalGroundHeld(*Train::Get(project.cutter)), NormalGroundHeld(*hauler), hauler->tile.base(),
		Company::Get(_current_company)->money.base(), Company::Get(_current_company)->current_loan.base(), label.starts_with("loaded") ? 1 : 0, 0);
	REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false)); std::fflush(stdout);
}

/** Retain real working/loaded files and their immediate full-native next frames on the exact paid Standard original. */
TEST_CASE("Original normal paid work retains actual cutting and loaded spoil checkpoints", "[.normal-ground-checkpoints]")
{
	REQUIRE(std::getenv("OTTD_NORMAL_GROUND_CAPTURE") != nullptr);
	std::map<std::string, uint64_t> captured; std::exception_ptr failure;
	std::function<void()> after = [&] {
		try {
			if (_game_mode != GameMode::Normal || _pause_mode.Any()) return;
			const auto projects = CaptureTBMProjects(); if (projects.size() != 1) return;
			const auto &project = projects.front(); const auto *job = GetExcavationJob(project.job); REQUIRE(job != nullptr);
			const auto assignments = CaptureTBMExchangeAssignments(); REQUIRE(assignments.size() == 1);
			const auto *hauler = Train::Get(assignments.front().hauler);
			for (const auto label : {"working", "loaded"}) {
				const std::string key{label};
				if (!captured.contains(key)) {
					const bool ready = key == "working" ? job->cursor > 0 && job->cursor < job->route.size() && NormalGroundHeld(*Train::Get(project.cutter)) > 0 :
						NormalGroundHeld(*hauler) > 0 && GetTileLayer(hauler->tile) == MapLayer::Surface;
					if (ready) { SaveNormalGroundCheckpoint(key); captured[key] = TimerGameTick::counter; }
				} else if (!captured.contains(key + "-next") && TimerGameTick::counter > captured.at(key)) {
					REQUIRE(TimerGameTick::counter == captured.at(key) + 1);
					SaveNormalGroundCheckpoint(key + "-next"); captured[key + "-next"] = TimerGameTick::counter;
				}
			}
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	AutoRestoreBackup observer{_normal_ground_complete_frame, &after};
	RunNormalGroundBoring(0, "normal-ground-checkpoints");
	if (failure) std::rethrow_exception(failure);
	REQUIRE(captured.size() == 4);
}

/** @return Read-only native owner/clock/RNG view supplementary to the external full126-chunk comparisons. */
static auto CaptureNormalGroundFile()
{
	return std::tuple{TimerGameTick::counter, Company::Get(_current_company)->money, Company::Get(_current_company)->current_loan,
		ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(), ComputeTBMExchangeDigest(),
		Vehicle::GetNumItems(), CargoPacket::GetNumItems(), _random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
}

/** @param label Explicit invocation identity. @param stage Actual read/repeat/next phase. Save without assigning mode,pause,clock or any saved owner. */
static void SaveNormalGroundFile(std::string_view label, std::string_view stage)
{
	REQUIRE(_pause_mode == PauseModes{PauseMode::Normal});
	const auto before = CaptureNormalGroundFile();
	const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-files");
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", fmt::format("{}-{}.sav", label, stage))), false) == SaveLoadResult::Ok);
	CHECK(CaptureNormalGroundFile() == before);
}

/** Ordinary afterload,preview,pure repeated writing and the next full actual GameLoop frame retain exact original owners. */
TEST_CASE("Original normal-world files preserve every native owner through ordinary readers", "[.normal-ground-file]")
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"), *action_text = std::getenv("OTTD_NORMAL_GROUND_FILE_ACTION"), *label_text = std::getenv("OTTD_NORMAL_GROUND_FILE_LABEL");
	REQUIRE(input != nullptr); REQUIRE(action_text != nullptr); REQUIRE(label_text != nullptr);
	const std::string_view action{action_text}, label{label_text}; ValidateTestArtifactIdentifier(label);
	REQUIRE((action == "baseline" || action == "inspect" || action == "next" || action == "reload-next"));
	bool begin_requested = false, started = false, completed = false; uint64_t tick = 0; std::exception_ptr failure;
	std::function<void()> after;
	std::function<void()> begin = [&] { begin_requested = true; after(); };
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &begin}; IConsole::CmdRegister("normal_ground_file", ObserveNormalGroundNewgame);
	after = [&] {
		if (!begin_requested || completed || failure) return;
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			if (!started) {
				// The native loader may pause while its restored graph worker
				// finishes. Running paused GameLoop frames here would refill saved
				// landscaping allowances. Wait for that worker and let its own
				// canonical pause controller clear the transient load pause before
				// the first ordinary simulation opportunity;never assign a field.
				const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
				while (_pause_mode.Test(PauseMode::LinkGraph)) {
					REQUIRE(std::chrono::steady_clock::now() < deadline);
					StateGameLoop_LinkGraphPauseControl();
					if (_pause_mode.Test(PauseMode::LinkGraph)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				fmt::print("NORMAL_GROUND_FILE_START pause={} tick={} clear_limit={}\n", _pause_mode.base(), TimerGameTick::counter, Company::Get(_current_company)->clear_limit);
				auto pending = _pause_mode; pending.Reset(PauseMode::Normal); pending.Reset(PauseMode::CommandDuringPause);
				REQUIRE(pending.None());
				REQUIRE(_game_mode == GameMode::Normal); REQUIRE_FALSE(_networking); REQUIRE(_pause_mode == PauseModes{PauseMode::Normal});
				SaveNormalGroundFile(label, "read");
				if (action != "baseline") {
					const auto before = CaptureNormalGroundFile();
					REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
					CHECK(CaptureNormalGroundFile() == before);
				}
				SaveNormalGroundFile(label, "repeat");
				if (action == "inspect") {
					completed = true; _exit_game = true;
				} else {
					tick = TimerGameTick::counter; REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false)); started = true;
				}
				return;
			}
			if (TimerGameTick::counter == tick) return;
			REQUIRE(TimerGameTick::counter == tick + 1);
			REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
			SaveNormalGroundFile(label, "next"); SaveNormalGroundFile(label, "next-repeat");
			completed = true; _exit_game = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	AutoRestoreBackup observer{_normal_ground_complete_frame, &after};
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "ground-acceptance-null", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); REQUIRE(completed);
	fmt::print("NORMAL_GROUND_FILE action={} preview={} completed=1\n", action, action == "baseline" ? 0 : 1);
}
