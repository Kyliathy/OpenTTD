/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file ground_network_active_fixture.h Actual paid active excavation and wagon-wait peer observation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md Group18B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Original projects/wagons remain their native owners;only an explicit public Resume may change player-paused work.
 */
#ifndef TESTS_GROUND_NETWORK_ACTIVE_FIXTURE_H
#define TESTS_GROUND_NETWORK_ACTIVE_FIXTURE_H

#include "ground_network_fixture.h"
#include "../tbm.h"
#include "../tbm_cmd.h"
#include "../tbm_exchange.h"
#include "../excavation.h"
#include "../console_internal.h"
#include <charconv>

inline std::function<void()> *_ground_network_startup_observer = nullptr; ///< Invocation-only native loaded-world hook,not a gameplay registry.

/**
 * Read an explicit original owner/clock observation without allocating a replacement.
 * @param name Wrapper-owned required unsigned environment field.
 * @return Complete unsigned value;malformed fields refuse before any command.
 */
inline uint64_t GroundNetworkUnsigned(const char *name)
{
	const std::string_view input = GroundNetworkArgument(name);
	uint64_t result = 0;
	const auto parsed = std::from_chars(input.data(), input.data() + input.size(), result);
	REQUIRE(parsed.ec == std::errc{});
	REQUIRE(parsed.ptr == input.data() + input.size());
	return result;
}

/**
 * Execute the invocation's actual server startup observation after ordinary afterload.
 * @param arguments Native console command arguments;empty requests help without mutation.
 * @return True after dispatch to the already registered test-only observer.
 */
inline bool GroundNetworkWorkReady(std::span<std::string_view> arguments)
{
	if (arguments.empty()) return true;
	if (_ground_network_startup_observer != nullptr) (*_ground_network_startup_observer)();
	return true;
}

/** Read-only original paid project,job and hauler observations during actual peer game loops. @see r2mlvs15-savegame-determinism-and-lifecycle.md Group19C. */
class GroundNetworkActiveWorld {
	TBMProjectID project_id; ///< Exact original controller identity from the sealed input.
	uint32_t original_cursor; ///< Actual paid progress before the real joining snapshot.
	uint32_t original_phase; ///< Actual active or WaitForEmpties phase,never restaged.
	uint64_t original_tick; ///< Input clock;the first native callback is its next opportunity.
	std::optional<uint32_t> first_frame; ///< Observer-only actual first unpaused frame.
	bool native_tick_schedule; ///< Starting-mode peers use tick-relative observations and the shared initial/next writer.

public:
	/**
	 * Construct observations from independently decoded original input identities;no native world is changed.
	 * @param native_tick_schedule Use Plan15-19C unpaused-opportunity offsets;historical active cases keep their fixed frame schedule.
	 */
	explicit GroundNetworkActiveWorld(bool native_tick_schedule = false) : project_id(static_cast<uint32_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_PROJECT"))),
		original_cursor(static_cast<uint32_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_CURSOR"))),
		original_phase(static_cast<uint32_t>(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_PHASE"))),
		original_tick(GroundNetworkUnsigned("OTTD_GROUND_NETWORK_TICK")), native_tick_schedule(native_tick_schedule) {}

	/** Observe the original server-loaded controller and publicly resume only a retained player pause before joining. */
	void Prepare() const
	{
		REQUIRE(_networking);
		REQUIRE(_network_server);
		const auto *project = GetTBMProject(this->project_id);
		REQUIRE(project != nullptr);
		REQUIRE(project->company == CompanyID{0});
		REQUIRE(to_underlying(project->phase) == this->original_phase);
		const auto *job = GetExcavationJob(project->job);
		REQUIRE(job != nullptr);
		REQUIRE(job->cursor == this->original_cursor);
		REQUIRE(job->cursor < job->route.size());
		if (project->blocked == TBMBlockedReason::Paused) {
			// The retained wagon-wait file was deliberately player-paused by
			// its original producer. A real administrator-owned public command
			// resumes that same project through the agreed server queue before
			// any client snapshot;no saved phase or actor packet is overwritten.
			AutoRestoreBackup actor{_current_company, project->company};
			REQUIRE(Command<Commands::ControlTBMProject>::Post(project->id, TBMProjectAction::Resume));
		} else {
			REQUIRE(project->blocked == TBMBlockedReason::None);
		}
		fmt::print("GROUND_NETWORK_WORK_PREPARE project={} job={} cursor={} phase={} blocked={} tick={}\n",
			project->id.base(), project->job.base(), job->cursor, to_underlying(project->phase),
			to_underlying(project->blocked), TimerGameTick::counter);
		std::fflush(stdout);
	}

	/**
	 * Write an actual initial/next opportunity twice without changing its authoritative owners.
	 * @param server Actual peer role used only to disambiguate evidence destinations.
	 * @param stage Explicit joined or joined-next observation label.
	 */
	void Save(bool server, std::string_view stage) const
	{
		const auto before = std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(),
			ComputeTBMExchangeDigest(), TimerGameTick::counter, _random.state[0], _random.state[1]};
		const auto paths = GetTestCaseArtifactPaths("network", "paid-ground");
		// The ordinary writer is read-only with respect to native gameplay.
		// Complete all126-chunk repeat comparisons supplement owner/RNG checks.
		for (const std::string_view suffix : {"", "-repeat"}) {
			const auto path = paths.Output("saves", fmt::format("ground-{}-{}{}.sav", server ? "server" : "client", stage, suffix));
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
		}
		CHECK((std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(),
			ComputeTBMExchangeDigest(), TimerGameTick::counter, _random.state[0], _random.state[1]} == before));
	}

	/**
	 * Compare real reconstructed references and report their subsequent native progression.
	 * @param frame Actual agreed network frame inside the existing application loop.
	 * @param server Actual host versus independent joining-client role.
	 */
	void Observe(uint32_t frame, bool server)
	{
		const auto *project = GetTBMProject(this->project_id);
		REQUIRE(project != nullptr);
		const auto *job = GetExcavationJob(project->job);
		REQUIRE(job != nullptr);
		const auto *assignment = GetTBMExchangeAssignmentForHauler(VehicleID{0});
		REQUIRE(assignment != nullptr);
		REQUIRE(assignment->project == project->id);
		const auto *cutter = Train::Get(project->cutter);
		const auto *hauler = Train::Get(assignment->hauler);
		REQUIRE(cutter->owner == project->company);
		REQUIRE(hauler->owner == project->company);
		if (!this->first_frame.has_value()) {
			REQUIRE(job->cursor == this->original_cursor);
			REQUIRE(to_underlying(project->phase) == this->original_phase);
			REQUIRE(project->blocked == TBMBlockedReason::None);
			REQUIRE(TimerGameTick::counter == this->original_tick + 1);
			this->first_frame = frame;
			if (!this->native_tick_schedule) this->Save(server, "joined");
		}
		// Tick-relative peers use the shared initial/next writer exactly once;
		// historical fixed-frame cases retain their original evidence layout.
		if (!this->native_tick_schedule && frame == *this->first_frame + 1) this->Save(server, "joined-next");
		constexpr std::array<uint32_t, 4> checkpoints = {256, 384, 512, 640};
		constexpr std::array<uint64_t, 4> tick_offsets = {129, 257, 385, 513};
		const bool later = this->native_tick_schedule ?
			std::ranges::find(tick_offsets, TimerGameTick::counter - this->original_tick) != tick_offsets.end() :
			std::ranges::find(checkpoints, frame) != checkpoints.end();
		// Observe the exact initial/next frames and all later complete native
		// checkpoints;render cadence cannot select or advance any gameplay work.
		// Both starting opportunities and all four later samples retain original
		// work/cargo/reference ledgers at the same shared writer boundaries.
		if (frame == *this->first_frame || frame == *this->first_frame + 1 || later) {
			uint64_t carried = 0;
			// Sum the original actual whole hauler chain,not an independent
			// aggregate cargo owner or a replacement packet registry.
			for (const Train *part = hauler; part != nullptr; part = part->Next()) carried += part->cargo.TotalCount();
			fmt::print("GROUND_NETWORK_WORK role={} frame={} tick={} project={} job={} cursor={} partial={} earth={} rock={} cost={} phase={} blocked={} haul_phase={} haul_wait={} haul_x={} haul_y={} carried={} exchanges={}\n",
				server ? "server" : "client", frame, TimerGameTick::counter, project->id.base(), project->job.base(), job->cursor,
				job->partial_work_ticks, job->completed_earth_t, job->completed_rock_t, job->completed_work_cost.base(),
				to_underlying(project->phase), to_underlying(project->blocked), to_underlying(assignment->phase),
				to_underlying(assignment->wait), hauler->x_pos, hauler->y_pos, carried, assignment->exchanges);
			std::fflush(stdout);
		}
	}
};

/**
 * Run an actual native peer with original paid-work references and a bounded read-only observer.
 * @param native_tick_schedule Use shared initial/next and actual unpaused-tick offsets for legal starting-mode peers.
 */
inline void RunGroundNetworkActivePeer(bool native_tick_schedule = false)
{
	GroundNetworkActiveWorld world{native_tick_schedule};
	std::exception_ptr failure;
	/** Prepare only after ordinary server afterload;defer failures until native application shutdown. */
	std::function<void()> prepare = [&]() {
		try { world.Prepare(); } catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	AutoRestoreBackup hook{_ground_network_startup_observer, &prepare};
	IConsole::CmdRegister("ground_network_work_ready", GroundNetworkWorkReady);
	/** @param frame Actual agreed frame. @param server Actual peer role,not a fabricated command actor. */
	RunGroundNetworkPeer([&](uint32_t frame, bool server) { world.Observe(frame, server); }, native_tick_schedule, native_tick_schedule);
	if (failure) std::rethrow_exception(failure);
}

#endif /* TESTS_GROUND_NETWORK_ACTIVE_FIXTURE_H */
