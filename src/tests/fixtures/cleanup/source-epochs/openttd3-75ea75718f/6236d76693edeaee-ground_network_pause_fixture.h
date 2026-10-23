/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file ground_network_pause_fixture.h Real native Pause commands and read-only frozen peer observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Group19B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Wall time selects only diagnostic observations;native agreed commands alone pause or resume gameplay.
 */
#ifndef TESTS_GROUND_NETWORK_PAUSE_FIXTURE_H
#define TESTS_GROUND_NETWORK_PAUSE_FIXTURE_H

#include "ground_network_active_fixture.h"
#include "../timer/timer_game_realtime.h"
#include "../gfx_func.h"
#include "../console_func.h"

/** Invocation-only pause request and two frozen observations,never an alternate simulation clock. */
class GroundNetworkPauseWorld {
	uint64_t original_tick = GroundNetworkUnsigned("OTTD_GROUND_NETWORK_TICK"); ///< Exact saved input clock.
	bool armed = false; ///< Both peers have reached the actual pause-request opportunity.
	std::optional<std::chrono::steady_clock::time_point> first_pause; ///< Diagnostic wall time only.
	std::vector<uint64_t> frozen; ///< Read-only authoritative clock/RNG observations.
	uint samples = 0; ///< Completed local paused snapshots,required before native application exit.

	/** @return Actual saved clock/RNG values;the network frame is deliberately excluded because it advances during pause. */
	static std::vector<uint64_t> Clocks()
	{
		return {TimerGameTick::counter, static_cast<uint64_t>(TimerGameCalendar::date.base()), TimerGameCalendar::date_fract,
			TimerGameCalendar::sub_date_fract, static_cast<uint64_t>(TimerGameEconomy::date.base()), TimerGameEconomy::date_fract,
			_random.state[0], _random.state[1]};
	}

public:
	/**
	 * Request a real administrator pause after the first ordinary peer checkpoint.
	 * @param frame Actual agreed frame at this simulation opportunity.
	 * @param server Whether this is the actual dedicated host permitted to post Pause.
	 */
	void Observe(uint32_t frame, bool server)
	{
		CHECK(_game_speed == 100);
		if (TimerGameTick::counter != this->original_tick + 161) return;
		this->armed = true;
		if (server) {
			// Match ordinary dedicated-console input authority,then restore
			// StateGameLoop's OWNER_NONE actor before simulation continues.
			// The normal console posts the actual agreed server Pause command.
			AutoRestoreBackup actor{_current_company, _local_company};
			IConsoleCmdExec("pause");
		}
		fmt::print("GROUND_NETWORK_PAUSE_REQUEST role={} frame={} tick={}\n", server ? "server" : "client", frame, TimerGameTick::counter);
		std::fflush(stdout);
	}

	/**
	 * Observe actual paused gameplay twice while network frames continue normally.
	 * @param server Actual peer role used to separate native output paths.
	 */
	void ObservePaused(bool server)
	{
		if (!this->armed) return;
		if (!_networking) return;
		if (_game_mode != GameMode::Normal) return;
		if (!(_pause_mode.Test(PauseMode::Normal))) return;
		if (this->samples == 2) return;
		const auto now = std::chrono::steady_clock::now();
		if (!this->first_pause.has_value()) {
			this->first_pause = now;
			this->frozen = Clocks();
		} else {
			// At least half a real second of network processing must leave
			// authoritative gameplay frozen. Wall time never credits work or
			// resources;the wrapper resumes only after both peers finish here.
			if (now - *this->first_pause < std::chrono::milliseconds(500)) return;
			CHECK(Clocks() == this->frozen);
		}
		CHECK(_game_speed == 100);
		const std::string_view stage = this->samples == 0 ? "paused-a" : "paused-b";
		SaveGroundNetworkOpportunity(server, stage);
		this->samples++;
		fmt::print("GROUND_NETWORK_PAUSE role={} stage={} frame={} tick={} calendar={} calendar_fract={} economy={} economy_fract={} rng0={} rng1={} elapsed_ms={} speed={}\n",
			server ? "server" : "client", stage, _frame_counter, TimerGameTick::counter, TimerGameCalendar::date.base(),
			TimerGameCalendar::date_fract, TimerGameEconomy::date.base(), TimerGameEconomy::date_fract, _random.state[0], _random.state[1],
			std::chrono::duration_cast<std::chrono::milliseconds>(now - *this->first_pause).count(), _game_speed);
		std::fflush(stdout);
	}

	/** Require both frozen samples after clean native application shutdown. */
	void Verify() const
	{
		CHECK(this->armed);
		CHECK(this->samples == 2);
	}
};

/** Run actual native pause/resume with wall-time diagnostics and unpaused-opportunity state sampling. */
inline void RunGroundNetworkPausePeer()
{
	GroundNetworkPauseWorld world;
	const bool server = std::string_view{GroundNetworkArgument("OTTD_GROUND_NETWORK_ROLE")} == "server";
	std::exception_ptr failure;
	/** @param count Coalesced real-time notifications;only actual elapsed diagnostic wall time selects the second frozen sample. */
	IntervalTimer<TimerGameRealtime> observe({std::chrono::milliseconds(100), TimerGameRealtime::Trigger::Always}, [&](uint count) {
		(void)count;
		try { world.ObservePaused(server); } catch (...) { failure = std::current_exception(); _exit_game = true; }
	});
	/** @param frame Actual agreed network frame. @param host Actual host versus independent client input authority. */
	RunGroundNetworkPeer([&](uint32_t frame, bool host) { world.Observe(frame, host); }, true, true);
	if (failure) std::rethrow_exception(failure);
	world.Verify();
}

#endif /* TESTS_GROUND_NETWORK_PAUSE_FIXTURE_H */
