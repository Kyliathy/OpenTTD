/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */

/**
 * @file space_checkpoint_load.h Dedicated paused checkpoint inspection, with no work mode.
 * @see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md.
 * Included only by the existing isolated capital harness. The callback uses its
 * original console/save transport and never calls a query, purchase or work helper.
 */

#ifndef TESTS_SPACE_CHECKPOINT_LOAD_H
#define TESTS_SPACE_CHECKPOINT_LOAD_H

#include "../rev.h"
#include "../saveload/saveload.h"

extern const SaveLoadVersion SAVEGAME_VERSION; ///< Actual native writer version, as recorded by Gamelog::Revision.

/**
 * Encode the native client's gamelog revision identity without whitespace ambiguity.
 * @return Hexadecimal UTF-8 revision text using the existing gamelog version policy.
 */
static std::string SpaceCheckpointRevisionText()
{
	REQUIRE(_openttd_revision_modified < 3);
	const auto text = IsReleasedVersion() ? _openttd_revision : fmt::format("{}{}", "gum"[_openttd_revision_modified], _openttd_revision_hash);
	static constexpr char digits[] = "0123456789abcdef";
	std::string encoded;
	encoded.reserve(text.size() * 2);
	// Version text can contain spaces/non-ASCII bytes. Encode its exact bytes so
	// one terminal field identifies the same revision stored by the native writer.
	for (const unsigned char value : text) {
		encoded += digits[value >> 4];
		encoded += digits[value & 15];
	}
	return encoded;
}

/** Load and serialize the exact selected world twice in one paused observer. */
TEST_CASE("Space checkpoint inspection preserves every owner without earning a tick", "[.space-checkpoint-load]")
{
	const char *source = std::getenv("OTTD_SPACE_CHECKPOINT_SOURCE");
	REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "checkpoint-load");
	const auto config = GetTestRuntimeConfig();
	bool observed = false;
	std::exception_ptr failure;
	/** Inspect real native owners while preserving callback errors for Catch2. */
	std::function<void()> inspect = [&] {
		try {
			REQUIRE(_pause_mode != PauseModes{});
			REQUIRE(Company::IsValidID(_local_company));
			const auto tick = TimerGameTick::counter;
			const auto calendar = TimerGameCalendar::date;
			const auto calendar_fraction = TimerGameCalendar::date_fract;
			const auto economy = TimerGameEconomy::date;
			const auto economy_fraction = TimerGameEconomy::date_fract;
			SaveSurfaceResupply(paths.saves / "checkpoint-before.sav");
			const auto *company = Company::Get(_local_company);
			const auto cash = company->money;
			const auto debt = company->current_loan;
			SaveSurfaceResupply(paths.saves / "checkpoint-after.sav");
			REQUIRE(TimerGameTick::counter == tick);
			REQUIRE(TimerGameCalendar::date == calendar);
			REQUIRE(TimerGameCalendar::date_fract == calendar_fraction);
			REQUIRE(TimerGameEconomy::date == economy);
			REQUIRE(TimerGameEconomy::date_fract == economy_fraction);
			fmt::print("SPACE_CHECKPOINT_NATIVE company={} cash={} debt={} tick_before={} tick_after={} revision_text={} revision_modified={} revision_newgrf={} save_version={}\n",
				_local_company.base(), cash.base(), debt.base(), tick, TimerGameTick::counter, SpaceCheckpointRevisionText(),
				_openttd_revision_modified, _openttd_newgrf_version, to_underlying(SAVEGAME_VERSION));
			fmt::print("SPACE_CHECKPOINT_COMPLETE phase=load-only ticks=0 snapshots=2\n");
			observed = true;
		} catch (...) {
			failure = std::current_exception();
		}
		_pause_mode.Set(PauseMode::Normal);
		_exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect);
	IConsole::CmdRegister("space_checkpoint_load", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_capital_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0);
	if (failure) std::rethrow_exception(failure);
	CHECK(observed);
}

#endif /* TESTS_SPACE_CHECKPOINT_LOAD_H */
