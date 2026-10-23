/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_face_exchange_save.cpp The face exchange's save version, and real face-loop files reloaded natively.
 *
 * The in-process case checks the new version and that the saved project digest tells the face wait apart. The
 * native reader loads a checkpoint that the face-loop observer (demo_feedback_runtime.cpp) wrote through the
 * ordinary saver, and runs it through the ordinary simulation: to its next transition, which must match the
 * uninterrupted run (face_save.py compares the rows); through the exchanges of a legacy file; or, for a malformed
 * file, through the preview and full-load refusals. It only reads state and unpauses; it never moves a vehicle.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 9
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../console_internal.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../misc_cmd.h"
#include "../fios.h"
#include "../timer/timer.h"
#include "../saveload/saveload.h"
#include "../saveload/saveload_func.h"
#include "../core/string_consumer.hpp"
#include "tbm_face_fixture.h"
#include "tbm_face_native.h"
#include <cstdlib>
#include <filesystem>

#include "../safeguards.h"

/** Task 9.1 and 9.3: the face wait has its own save version, and the saved project digest tells it apart. */
TEST_CASE("The face wait has its own save version and digest", "[demo-fix-face-save]")
{
	CHECK(to_underlying(SaveLoadVersion::TBMFaceTransfer) == 425);
	CHECK(SaveLoadVersion::TBMFaceTransfer > SaveLoadVersion::AsteroidStarterInputs);
	TBMFaceTransferRig rig(TBMPhase::AwaitFaceTransfer, TBM_FACE_TRANSFER_TICKS / 2, 40);
	const TBMProject waiting = rig.Project();
	const uint64_t digest = ComputeTBMProjectDigest();
	/**
	 * Reload the same project in another phase, with every other saved field as it was.
	 * @param phase Phase of the reloaded copy.
	 */
	const auto reload = [&rig, &waiting](TBMPhase phase) {
		rig.Reload(phase, [&waiting](TBMProject &project) {
			project.phase_ticks = waiting.phase_ticks;
			project.revision = waiting.revision - 1; // Reload advances the revision of an edited copy once.
		});
	};
	reload(TBMPhase::Bore);
	CHECK(rig.Project().revision == waiting.revision);
	CHECK(ComputeTBMProjectDigest() != digest);
	reload(TBMPhase::AwaitFaceTransfer);
	CHECK(ComputeTBMProjectDigest() == digest);
}

/** Reader of one face-loop file, selected by the runner's environment (tasks 9.2 to 9.5). */
class FaceSaveReader {
public:
	std::string mode; ///< "next": run to the first transition; "exchanges": follow the next exchanges; "refuse": malformed and control files.
	std::string input; ///< Loaded file.
	std::string name; ///< Row name: the file's stem.
	uint exchanges_wanted = 2; ///< Exchanges the "exchanges" mode follows (OTTD_FACE_SAVE_EXCHANGES).
	uint tick_limit = 40000; ///< Ticks after which the run ends unfinished (OTTD_FACE_SAVE_TICKS).
	TBMProjectID project_id{UINT32_MAX}; ///< The followed project: the first one in the file.
	std::optional<FaceLoopSample> before; ///< State after the previous tick, or as loaded.
	uint ticks = 0; ///< Ticks run since the unpause.
	uint exchanges_seen = 0; ///< Exchanges followed so far.
	bool started = false; ///< The loaded world was unpaused.
	bool finished = false; ///< The reader is done; no more observation happens.
	std::exception_ptr failure; ///< First failure, rethrown after native shutdown.

	/** Read the loaded file: log what it holds, then refuse the malformed files or unpause the ordinary simulation. */
	void Start()
	{
		try {
			const auto projects = CaptureTBMProjects();
			REQUIRE_FALSE(projects.empty());
			const TBMProject &project = projects.front();
			this->project_id = project.id;
			extern SaveLoadVersion _sl_version;
			// Exactly what the file holds, before any tick; the writer logged the same owners when it asked for the save.
			fmt::print("FACE_SAVE_LOADED name={} mode={} version={} phase={} {}\n", this->name, this->mode, to_underlying(_sl_version), to_underlying(project.phase),
				DescribeFaceOwners(project));
			if (this->mode == "refuse") {
				this->Refuse(project);
				this->Finish();
				return;
			}
			this->before = SampleFaceLoop(project);
			AutoRestoreBackup actor(_current_company, project.company);
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
			this->started = true;
		} catch (...) {
			this->Fail();
		}
	}

	/**
	 * Task 9.3: a pre-425 file that holds the face wait is refused by the preview and by a full load, without touching
	 * the loaded world. The control file, the same version with a phase older versions know, loads normally.
	 * @param project The project as loaded from the valid file.
	 */
	void Refuse(const TBMProject &project)
	{
		const char *corrupt = std::getenv("OTTD_FACE_SAVE_CORRUPT");
		const char *control = std::getenv("OTTD_FACE_SAVE_CONTROL");
		REQUIRE(corrupt != nullptr);
		REQUIRE(control != nullptr);
		const std::string owners = DescribeFaceOwners(project);
		/** @return The followed project's owners now, or a marker when it is gone. */
		const auto current = [this] {
			const TBMProject *now = GetTBMProject(this->project_id);
			return now == nullptr ? std::string{"missing"} : DescribeFaceOwners(*now);
		};
		// The previews read the files' chunks only; the loaded world, its clock and its random state stay as they are.
		const SaveLoadResult corrupt_preview = SaveOrLoad(corrupt, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false);
		CHECK(corrupt_preview == SaveLoadResult::Error);
		CHECK(current() == owners);
		const SaveLoadResult control_preview = SaveOrLoad(control, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false);
		CHECK(control_preview == SaveLoadResult::Ok);
		CHECK(current() == owners);
		// A full load of the malformed file is refused; the valid file then loads again exactly as it did.
		const SaveLoadResult corrupt_load = SaveOrLoad(corrupt, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false);
		CHECK(corrupt_load == SaveLoadResult::ReInit);
		REQUIRE(SaveOrLoad(this->input, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		const bool reloaded = current() == owners;
		CHECK(reloaded);
		// The control file loads in full, with its project in the phase it was saved in.
		const SaveLoadResult control_load = SaveOrLoad(control, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false);
		CHECK(control_load == SaveLoadResult::Ok);
		const TBMProject *loaded = GetTBMProject(this->project_id);
		CHECK(loaded != nullptr);
		fmt::print("FACE_SAVE_REFUSED name={} corrupt_preview={} control_preview={} corrupt_load={} control_load={} control_phase={} reloaded={}\n", this->name,
			to_underlying(corrupt_preview), to_underlying(control_preview), to_underlying(corrupt_load), to_underlying(control_load),
			loaded == nullptr ? -1 : static_cast<int>(to_underlying(loaded->phase)), reloaded ? "same" : "different");
	}

	/**
	 * Observe one ordinary simulation tick of the loaded file.
	 * @param elapsed Ticks since the previous call.
	 */
	void Observe(uint elapsed)
	{
		if (!this->started || this->finished) return;
		this->ticks += elapsed;
		const TBMProject *project = GetTBMProject(this->project_id);
		REQUIRE(project != nullptr);
		const FaceLoopSample now = SampleFaceLoop(*project);
		if (this->mode == "next") {
			// The first transition after the file's tick, or the horizon, with the owners at that tick.
			const std::string transition = DescribeFaceTransition(*this->before, now);
			if (!transition.empty() || this->ticks >= FACE_WATCH_TICKS) {
				fmt::print("FACE_SAVE_NEXT name={} offset={} {} {}\n", this->name, this->ticks, transition.empty() ? "horizon" : transition, DescribeFaceOwners(*project));
				this->Finish();
				return;
			}
		} else if (now.exchanges != this->before->exchanges) {
			// Where each following exchange is met, by the meeting point on the tick before it (rule F1).
			const FaceLoopSample &met = *this->before;
			fmt::print("FACE_SAVE_EXCHANGE name={} n={} tick={} meeting={} cursor={} cutter_phase={} stand={} cutter_aboard={}->{}\n", this->name,
				this->exchanges_seen + 1, this->ticks, met.meeting == TBMMeetingKind::Face ? "face" : "bay", now.cursor, to_underlying(met.phase),
				DescribeTile(met.stand.has_value() ? met.stand->tile : INVALID_TILE), met.cutter_aboard, now.cutter_aboard);
			if (++this->exchanges_seen >= this->exchanges_wanted) {
				this->Finish();
				return;
			}
		}
		this->before = now;
		if (this->ticks >= this->tick_limit) {
			fmt::print("FACE_SAVE_LIMIT name={} ticks={} exchanges={}\n", this->name, this->ticks, this->exchanges_seen);
			FAIL_CHECK("the loaded file did not reach what the reader waits for");
			this->Finish();
		}
	}

	/** Stop observing and end the bounded native run. */
	void Finish()
	{
		this->finished = true;
		fmt::print("FACE_SAVE_COMPLETE name={} mode={} ticks={}\n", this->name, this->mode, this->ticks);
		_exit_game = true;
	}

	/** Retain the current exception and stop the native loop. */
	void Fail()
	{
		if (!this->failure) this->failure = std::current_exception();
		this->finished = true;
		_exit_game = true;
	}
};

static FaceSaveReader *_face_save_reader = nullptr; ///< Process-local reader, absent from production executables.

/**
 * Start the reader once the ordinary startup loader has rebuilt the saved world.
 * @param arguments The registered console command only; the file and mode come from the runner's environment.
 * @return True after dispatching the reader.
 */
static bool StartFaceSaveReader(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1);
	REQUIRE(_face_save_reader != nullptr);
	_face_save_reader->Start();
	return true;
}

/** Tasks 9.2 to 9.5: reload one real face-loop file, selected by OTTD_FACE_SAVE_INPUT and OTTD_FACE_SAVE_MODE. */
TEST_CASE("Face-loop files reload to the same next transition", "[.demo-fix-face-save-file]")
{
	FaceSaveReader reader;
	const char *input = std::getenv("OTTD_FACE_SAVE_INPUT");
	const char *mode = std::getenv("OTTD_FACE_SAVE_MODE");
	REQUIRE(input != nullptr);
	REQUIRE(mode != nullptr);
	reader.mode = mode;
	REQUIRE((reader.mode == "next" || reader.mode == "exchanges" || reader.mode == "refuse"));
	reader.input = ResolveTestInputPath(input).string();
	reader.name = std::filesystem::path(reader.input).stem().string();
	if (const char *wanted = std::getenv("OTTD_FACE_SAVE_EXCHANGES"); wanted != nullptr) reader.exchanges_wanted = ParseInteger<uint>(wanted).value_or(reader.exchanges_wanted);
	if (const char *limit = std::getenv("OTTD_FACE_SAVE_TICKS"); limit != nullptr) reader.tick_limit = ParseInteger<uint>(limit).value_or(reader.tick_limit);
	AutoRestoreBackup observer(_face_save_reader, &reader);
	IConsole::CmdRegister("face_save_check", StartFaceSaveReader);
	/** @param elapsed Real ticks since the previous call; only observation is performed. */
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&reader](uint elapsed) {
		if (reader.failure) return;
		try {
			reader.Observe(elapsed);
		} catch (...) {
			reader.Fail();
		}
	});
	const std::string config = GetTestRuntimeConfig().string();
	const std::string frames = fmt::format("null:ticks={}", reader.tick_limit + 2000);
	std::vector<std::string_view> arguments = {"RelWithDebInfo/openttd_test.exe", "-v", frames, "-b", "8bpp-simple", "-I", "original_dos", "-s", "null", "-m", "null",
		"-x", "-X", "-c", config, "-g", reader.input, "-r", "1280x733"};
	CHECK(openttd_main(arguments) == 0);
	if (reader.failure) std::rethrow_exception(reader.failure);
	REQUIRE(reader.finished);
}
