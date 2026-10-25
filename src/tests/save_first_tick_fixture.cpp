/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_first_tick_fixture.cpp Single native tick observer implementation, avoiding duplicated heavy Catch/owner templates across native test translation units.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 * Only isolated component copies advance; protected Space continuation windows remain closed.
 */
#include "../stdafx.h"
#include "save_first_tick_fixture.h"
#include "save_candidate_lifecycle_fixture.h"
#include "layer_save_fixture.h"
#include "../multilayer_state.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../core/format.hpp"
#include <chrono>
#include <thread>
#include "../safeguards.h"

/** @copydoc CheckFirstNativeTick */
void CheckFirstNativeTick(const char *source, const std::optional<SaveOriginHint> &origin, const std::filesystem::path &output)
{
	REQUIRE(source != nullptr); REQUIRE(_game_mode == GameMode::Normal);
	REQUIRE(SaveOrLoad(source, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false, origin) == SaveLoadResult::Ok);
	/** @param path Invocation-owned new file. Preserve the paused native state without a command, charge or work opportunity. */
	const auto save = [](const std::filesystem::path &path) {
		AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	};
	/** Advance exactly one real synchronized tick; GUI pacing and a paused callback cannot masquerade as production work. */
	const auto step = [] {
		const auto before = TimerGameTick::counter; AutoRestoreBackup pause(_pause_mode, PauseModes{});
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
		// Background link-graph work can legitimately pause a native callback.
		// Count only the first actual simulation tick, never bypass that owner or
		// count wall-clock/paused callbacks as production, dispatch or service.
		while (TimerGameTick::counter == before) {
			REQUIRE(std::chrono::steady_clock::now() < deadline); StateGameLoop();
			if (TimerGameTick::counter == before) std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		REQUIRE(TimerGameTick::counter == before + 1);
	};
	const auto before = output.string() + ".tick-before.sav";
	const auto direct = output.string() + ".tick-direct.sav", reloaded = output.string() + ".tick-reloaded.sav";
	save(before); step();
	const auto expected = CaptureCandidateLiveState(); const auto canonical = ComputeLiveMultiLayerStateDigest(CaptureMultiLayerStateSchema()); REQUIRE(canonical);
	save(direct);
	REQUIRE(SaveOrLoad(before, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	step(); const auto actual = CaptureCandidateLiveState();
	CHECK(actual.tick == expected.tick); CHECK(actual.random[0] == expected.random[0]); CHECK(actual.random[1] == expected.random[1]);
	CHECK(actual.book == expected.book); CHECK(actual.packets == expected.packets); CHECK(actual.accounts == expected.accounts); CHECK(actual.domains == expected.domains);
	CHECK(ComputeLiveMultiLayerStateDigest(CaptureMultiLayerStateSchema()) == canonical);
	save(reloaded);
	fmt::print("MERGED_FIRST_TICK_NATIVE tick={} original_and_reloaded=true real_accounts_packets_rng_equal=true\n", actual.tick);
}
