/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_motion_reload.cpp Actual moving/waiting savegames and deterministic native continuation.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group4
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group5; retain genuinely driven pre-rate-change files.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group7; resumable physical parking and native input.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group7B; settled release and retained native machinery.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group8; named native-order field refusals share the real preview/load observer.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../train.h"
#include "../tbm.h"
#include "../tbm_cmd.h"
#include "../tbm_exchange.h"
#include "../tbm_controls.h"
#include "../tbm_area_preview.h"
#include "../spoil_receiver.h"
#include "../layer_portal_map.h"
#include "../rail_map.h"
#include "../track_func.h"
#include "../pbs.h"
#include "../core/random_func.hpp"
#include "../command_func.h"
#include "../misc_cmd.h"
#include "../vehicle_cmd.h"
#include "../window_gui.h"
#include "../tbm_gui.h"
#include "../vehicle_gui.h"
#include "../gui.h"
#include "../widgets/tbm_widget.h"
#include "../widgets/vehicle_widget.h"
#include "../layer_portal_cmd.h"
#include "../table/strings.h"
#include "../timer/timer.h"
#include "../timer/timer_game_tick.h"
#include "../timer/timer_window.h"
#include "../saveload/saveload_func.h"
#include "../fios.h"
#include "layer_save_fixture.h"
#include "native_art_render_fixture.h"
#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../fontcache.h"
#include <cstdlib>
#include <filesystem>
#include <charconv>

#include "../safeguards.h"

/** A native loaded game observed after its vehicle/landscape step, never advanced through fixture work helpers. */
class TBMMotionReplay {
public:
	std::filesystem::path root; ///< Repository-owned input/output boundary.
	std::string replay; ///< Empty for snapshot creation; otherwise one validated snapshot kind.
	bool parking = false; ///< Select the actual partial-work return/park/resume journey instead of historical motion acceptance.
	bool release = false; ///< Continue a parked source through the actual final-release UI and public connection.
	bool release_requested = false; ///< One native final-release click has been posted.
	bool release_saved = false; ///< Actual unbound Sealed file was retained before public connection.
	bool connection_posted = false; ///< The admitted public connection is waiting for its native command boundary.
	VehicleID released_cutter = VehicleID::Invalid(); ///< Actual native identity captured before its controller is released.
	std::optional<ExcavationJob> parked_job; ///< Exact retained job at the player request or loaded return boundary.
	uint16_t parked_remainder = 0; ///< Existing cutting-rate remainder, never cargo fractions.
	uint parked_tick = 0; ///< First physical parking frame; resume after a short stationary observation.
	bool park_resumed = false; ///< One actual native Resume input, avoiding repeated commands.
	std::string pace_seed_layer; ///< Optional real Underground/Deep old-rate partial-work seed; never synthesized progress.
	std::string pace_check_layer; ///< Actual legacy/current mid-cut continuation for the half-speed migration.
	bool pace_resaved = false; ///< Load the current-version migrated copy to prove migration is idempotent.
	bool pace_fraction = false; ///< Load a naturally earned new half/quarter-work fraction after one resumed frame.
	uint64_t pace_partial = 0; ///< Geological work already performed at the real file boundary.
	uint16_t pace_remainder = 0; ///< Exact new-divisor fraction at that same boundary.
	uint64_t pace_duration = 0; ///< Remaining synchronized ticks required by the reviewed work rate.
	ExcavationCutResult pace_quote; ///< Next real cut's material/work requirements, not copied cargo.
	TBMProjectID project{UINT32_MAX}; ///< Actual loaded project selected at startup.
	uint ticks = 0; ///< Full simulation steps since the baseline started; restored from runner evidence.
	bool finished = false; ///< Prevent observation beyond the same final native step.
	std::exception_ptr failure; ///< Deferred assertion, rethrown after the ordinary game shuts down.
	std::array<bool, 3> saved{}; ///< Actual approach, bend and wait snapshots produced once each.
	static constexpr uint LIMIT = 20000; ///< Same bounded ordinary simulation interval for every continuation.
	static constexpr std::array<std::string_view, 3> KINDS{"approach", "bend", "wait"}; ///< Stable retained file names.
	static constexpr std::array<std::string_view, 3> PARK_KINDS{"return", "enter", "parked"}; ///< New files never overwrite historical movement/rate evidence.

	/**
	 * Dispatch native cursor movement followed by a real press/release at the current widget layout.
	 * @param window Actual native control window.
	 * @param widget Visible target.
	 */
	void Click(Window &window, WidgetID widget)
	{
		const Rect rect = window.GetWidget<NWidgetBase>(widget)->GetCurrentRect();
		_cursor.UpdateCursorPosition(window.left + (rect.left + rect.right) / 2, window.top + (rect.top + rect.bottom) / 2);
		_cursor.in_window = true;
		_left_button_down = _left_button_clicked = false;
		HandleMouseEvents();
		// Native hover dismissal runs on the window timer after movement. Let it
		// clear the previous button's tooltip before hit-testing the next click.
		for (uint pass = 0; pass < 5; ++pass) TimerManager<TimerWindow>::Elapsed(std::chrono::milliseconds(30));
		REQUIRE(FindWindowFromPt(_cursor.pos.x, _cursor.pos.y) == &window);
		REQUIRE_FALSE(window.IsWidgetDisabled(widget));
		for (bool down : {false, true, false}) {
			_left_button_down = down;
			if (!down) _left_button_clicked = false;
			HandleMouseEvents(); // Exercise normal hit testing; do not call a window's OnClick directly.
		}
	}

	/** @param window Live window. @param name Repository review name. Capture actual widget rendering without changing simulation state. */
	void CaptureParkingWindow(Window &window, std::string_view name)
	{
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		CaptureNativeArtWindow(window, GetTestCaseArtifactPaths("tbm", "motion").images / fmt::format("tbm-{}-{}.ppm", this->release ? "release" : "parking", name));
		REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	}

	/** Verify the actual released machine, retain its real file and connect only when ordinary admission succeeds. */
	void ObserveReleased()
	{
		REQUIRE(this->parked_job.has_value());
		const auto &before = *this->parked_job;
		const auto *job = GetExcavationJob(before.id);
		REQUIRE(job != nullptr);
		REQUIRE(job->state == ExcavationJobState::Cancelled);
		CHECK(job->cursor == before.cursor);
		CHECK(job->completed_earth_t == before.completed_earth_t);
		CHECK(job->completed_rock_t == before.completed_rock_t);
		REQUIRE(CaptureTBMProjects().empty());
		REQUIRE(CaptureTBMExchangeAssignments().empty());
		Train *front = Train::Get(this->released_cutter);
		const auto key = GetLayerPortalPairKey(before.commissioning_anchor);
		REQUIRE(key.has_value());
		REQUIRE(front->vehstatus.Test(VehState::Stopped));
		for (const Train *part = front; part != nullptr; part = part->Next()) {
			REQUIRE(part->tile == *key);
			REQUIRE(part->track == Track::Depot);
			REQUIRE(part->cargo.TotalCount() == 0);
		}
		AutoRestoreBackup actor(_current_company, before.company);
		if (!this->release_saved) {
			const auto file = GetTestCaseArtifactPaths("tbm", "motion").saves / "tbm-release-released-396.sav";
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(file), false) == SaveLoadResult::Ok);
			this->release_saved = true;
			for (const auto &support : CaptureTBMSupport()) REQUIRE(Command<Commands::ConvertTBMSupport>::Post(STR_ERROR_TBM_SUPPORT_IN_USE, support.tile)); // Ordinary paid conversion owns every invoice; no free removal of track protection.
			return;
		}
		if (GetLayerPortalWorkState(*key) != LayerPortalWorkState::Connected) {
			if (!this->connection_posted && ChangeLayerPortalConnection(*key, before.company, true, false) == LayerPortalWorkError::None) {
				REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Post(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL, *key, true));
				this->connection_posted = true;
			}
			return; // Ordinary through-track occupancy may still have to clear before a public edge can appear.
		}
		const auto file = GetTestCaseArtifactPaths("tbm", "motion").saves / "tbm-release-connected-396.sav";
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(file), false) == SaveLoadResult::Ok);
		fmt::print("RELEASE_NATIVE tick={} cutter={} job={} cursor={} earth={} rock={} vehicles={}\n", this->ticks, front->index.base(), job->id.base(), job->cursor, job->completed_earth_t, job->completed_rock_t, Vehicle::GetNumItems());
		this->finished = true;
		REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
	}

	/** Observe real return, partial magazine entry, settled parking and the same machine's next retained cut. */
	void ObserveParking()
	{
		const auto &work = *GetTBMProject(this->project);
		const auto &job = *GetExcavationJob(work.job);
		AutoRestoreBackup actor(_current_company, work.company);
		Train *front = Train::Get(work.cutter);
		this->Trace(this->State());
		if (!this->parked_job) {
			if (work.phase != TBMPhase::Bore || job.partial_work_ticks < 20) return;
			uint cargo = 0;
			for (const Train *part = front; part != nullptr; part = part->Next()) cargo += part->cargo.TotalCount(); // Require real spoil still on this incomplete pass, not a just-exchanged empty return.
			if (cargo == 0) return;
			this->parked_job = job;
			this->parked_remainder = work.work_remainder;
			Window *window = ShowTBMProjectsWindow(GetTileLayer(job.commissioning_anchor));
			REQUIRE(window != nullptr);
			this->CaptureParkingWindow(*window, "request");
			this->Click(*window, WID_TP_PARK);
			return; // The ordinary posted command executes at its normal next command boundary.
		}
		if (!this->park_resumed) {
			REQUIRE(job == *this->parked_job);
			REQUIRE(work.work_remainder == this->parked_remainder);
			bool inside = false, outside = false;
			for (const Train *part = front; part != nullptr; part = part->Next()) {
				inside |= part->track == Track::Depot;
				outside |= part->track != Track::Depot;
			}
			const std::array ready{work.phase == TBMPhase::ReturnToPark && front->cur_speed != 0 && !inside,
				work.phase == TBMPhase::ReturnToPark && inside && outside, work.phase == TBMPhase::Parked};
			if (this->replay.empty()) {
				for (size_t kind = 0; kind < ready.size(); ++kind) {
					if (this->saved[kind] || !ready[kind]) continue; // Retain the first real arrival state, never synthesize part positions or phase.
					const auto state = this->State();
					const auto file = GetTestCaseArtifactPaths("tbm", "motion").saves / fmt::format("tbm-{}-{}-396.sav", this->release ? "release" : "parking", PARK_KINDS[kind]);
					REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(file), false) == SaveLoadResult::Ok);
					REQUIRE(this->State() == state);
					this->saved[kind] = true;
					fmt::print("PARK_SNAPSHOT kind={} tick={} {}\n", PARK_KINDS[kind], this->ticks, state);
				}
			}
			if (work.phase == TBMPhase::Parked) {
				REQUIRE(inside);
				REQUIRE_FALSE(outside);
				REQUIRE(front->vehstatus.Test(VehState::Stopped));
				if (this->parked_tick == 0) this->parked_tick = this->ticks;
				if (this->release) {
					if (GetTBMProjectReleaseProblem(this->project) != INVALID_STRING_ID) return; // Preserve the partial job while the real loaded hauler completes its origin visit.
					Window *window = ShowTBMProjectsWindow(GetTileLayer(job.commissioning_anchor));
					REQUIRE(window != nullptr);
					this->CaptureParkingWindow(*window, "ready");
					this->released_cutter = work.cutter;
					this->Click(*window, WID_TP_RELEASE);
					this->release_requested = true;
					return;
				}
				if (this->ticks == this->parked_tick + 20) {
					ShowVehicleViewWindow(front);
					Window *window = FindWindowById(WindowClass::VehicleView, front->index);
					REQUIRE(window != nullptr);
					if (this->replay.empty()) this->CaptureParkingWindow(*window, "parked");
					this->Click(*window, WID_VV_START_STOP);
					this->park_resumed = true;
				}
			}
		} else if (job.cursor > this->parked_job->cursor) {
			CHECK(job.cursor == this->parked_job->cursor + 1);
			CHECK(job.id == this->parked_job->id);
			fmt::print("PARK_RESULT tick={} {}\n", this->ticks, this->State());
			this->finished = true;
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
		}
	}

	/** @return Actual native state, including every part and all accepted support reservations, for exact replay comparison. */
	std::string State() const
	{
		const TBMProject &work = *GetTBMProject(this->project);
		const ExcavationJob &job = *GetExcavationJob(work.job);
		std::string state = fmt::format("clock={} geology={} project={} exchange={} receivers={} cash={} rng={},{} cursor={} heading={} phase={} blocked={} partial={}",
			TimerGameTick::counter, ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(), ComputeTBMExchangeDigest(),
			ComputeSpoilReceiverDigest(), Company::Get(work.company)->money.base(), _random.state[0], _random.state[1], job.cursor,
			to_underlying(work.heading), to_underlying(work.phase), to_underlying(work.blocked), job.partial_work_ticks);
		std::set<TileIndex> rails{work.portal, job.commissioning_anchor};
		std::vector<size_t> ends;
		for (TileIndex tile : GetTBMWorkRailPath(job, job.route.size(), ends)) rails.insert(tile); // Include unoccupied route reservations, not merely the head's tile.
		for (const Train *part : Train::Iterate()) {
			// The retained area route revisits multi-branch track. Audit every
			// actual cutter/wagon position against its accepted path and already
			// paid floor, including real backing and exchanged wagon identities.
			if (part->First()->index == work.cutter && GetTileLayer(part->tile) != MapLayer::Surface && !IsLayerPortalTile(part->tile)) {
				REQUIRE(rails.contains(part->tile));
				REQUIRE(GetGeologyWorldState().terrain.Get(part->tile) == ExcavationState::Excavated);
				REQUIRE(IsTBMSupportUsable(work, part->tile));
			}
			// Every attached engine, permanent part and exchanged hopper retains its
			// identity, pose, speed, order and cargo across the real file boundary.
			state += fmt::format(" part={},{},{},{},{},{},{},{},{},{},{},{},{}", part->index.base(), part->First()->index.base(), part->tile.base(),
				part->track.base(), part->x_pos, part->y_pos, part->z_pos, to_underlying(part->direction), part->cur_speed, part->subspeed,
				part->progress, part->cargo.TotalCount(), part->current_order.GetType());
			rails.insert(part->tile);
			for (const CargoPacket *packet : *part->cargo.Packets()) {
				state += fmt::format(" packet={},{},{}", packet->index.base(), packet->Count(), packet->GetPeriodsInTransit());
			}
		}
		for (TileIndex tile : rails) state += fmt::format(" reservation={},{}", tile.base(), GetReservedTrackbits(tile).base());
		return state;
	}

	/** @param state Exact native state. Emit a compact portable hash; the snapshot and final boundary also retain the complete text. */
	void Trace(const std::string &state) const
	{
		uint64_t hash = 14695981039346656037ULL;
		for (unsigned char byte : state) hash = (hash ^ byte) * 1099511628211ULL; // Hash exact ordered fields without native padding or process-local addresses.
		fmt::print("MOTION_TRACE tick={} hash={} cursor={}\n", this->ticks, hash, GetExcavationJob(GetTBMProject(this->project)->job)->cursor);
	}

	/** @return Whether any actual attached part is negotiating a curved underground piece away from the shaft. */
	bool OnBend() const
	{
		const Train *front = Train::Get(GetTBMProject(this->project)->cutter);
		for (const Train *part = front; part != nullptr; part = part->Next()) {
			// Depot/shaft state is not a bend; require actual native curved rail
			// under a moving part on excavated underground floor.
			if (GetTileLayer(part->tile) != MapLayer::Surface && IsPlainRailTile(part->tile) &&
				part->track != Track::Depot && part->track != Track::Wormhole && !IsDiagonalTrackdir(part->GetVehicleTrackdir())) return true;
		}
		return false;
	}

	/** Retain an actual old-rate partial face and its fractional work before any new tuning or migration is installed. */
	void SavePaceSeed()
	{
		const TBMProject &work = *GetTBMProject(this->project);
		const ExcavationJob &job = *GetExcavationJob(work.job);
		const TBMCutterModel &model = *GetTBMCutterModel(work.model_id);
		const bool deep = this->pace_seed_layer == "deep";
		REQUIRE(model.work_rate_numerator == (deep ? 3 : 1));
		REQUIRE(model.work_rate_denominator == (deep ? 2 : 1));
		// Only ordinary work ticks may create the historical fraction. In
		// particular, Deep's 1/2 remainder must not be fabricated in a save row.
		if (work.phase != TBMPhase::Bore || job.partial_work_ticks == 0 || work.work_remainder != (deep ? 1 : 0)) return;
		const auto state = this->State();
		const auto file = GetTestCaseArtifactPaths("tbm", "motion").saves / fmt::format("tbm-pace-{}-393.sav", this->pace_seed_layer);
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(file), false) == SaveLoadResult::Ok);
		REQUIRE(this->State() == state);
		fmt::print("PACE_SEED layer={} model={} numerator={} denominator={} remainder={} partial={} {}\n", this->pace_seed_layer,
			work.model_id, model.work_rate_numerator, model.work_rate_denominator, work.work_remainder, job.partial_work_ticks, state);
		this->finished = true;
		AutoRestoreBackup actor(_current_company, work.company);
		REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
	}

	/** Verify ordinary native work at every half-speed tick through the next actual atomic cut. */
	void CheckPace()
	{
		const TBMProject &work = *GetTBMProject(this->project);
		const ExcavationJob &job = *GetExcavationJob(work.job);
		const TBMCutterModel &model = *GetTBMCutterModel(work.model_id);
		REQUIRE(work.blocked == TBMBlockedReason::None);
		if (this->ticks == 1 && !this->pace_fraction) {
			// A new-rate Standard half or Deep quarter is produced by the actual
			// first resumed game frame, then persisted without staging a value.
			const auto state = this->State();
			const auto file = GetTestCaseArtifactPaths("tbm", "motion").saves / fmt::format("tbm-pace-{}-fraction-394.sav", this->pace_check_layer);
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(file), false) == SaveLoadResult::Ok);
			REQUIRE(this->State() == state);
			fmt::print("PACE_FRACTION remainder={} {}\n", work.work_remainder, state);
		}
		if (this->ticks < this->pace_duration) {
			REQUIRE(job.cursor == 0);
			REQUIRE(job.partial_work_ticks == this->pace_partial + (this->pace_remainder + this->ticks * model.work_rate_numerator) / model.work_rate_denominator);
			REQUIRE(work.work_remainder == (this->pace_remainder + this->ticks * model.work_rate_numerator) % model.work_rate_denominator);
			return;
		}
		REQUIRE(this->ticks == this->pace_duration);
		CHECK(job.cursor == 1);
		CHECK(job.partial_work_ticks == 0);
		CHECK(job.completed_earth_t == this->pace_quote.earth_t);
		CHECK(job.completed_rock_t == this->pace_quote.rock_t);
		fmt::print("PACE_CUT layer={} ticks={} remainder={} {}\n", this->pace_check_layer, this->ticks, work.work_remainder, this->State());
		this->finished = true;
		AutoRestoreBackup actor(_current_company, work.company);
		REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
	}

	/** Observe after this frame's actual vehicle and landscape ticks; the next loaded frame starts at the same complete simulation boundary. */
	void Observe()
	{
		if (this->finished || this->failure) return; // A failed or completed probe must not create more evidence.
		++this->ticks;
		if (this->release_requested) {
			if (GetTBMProject(this->project) == nullptr) this->ObserveReleased();
			return;
		}
		if (this->parking) {
			this->ObserveParking();
			return;
		}
		if (!this->pace_seed_layer.empty()) {
			this->SavePaceSeed();
			return;
		}
		if (!this->pace_check_layer.empty()) {
			this->CheckPace();
			return;
		}
		const TBMProject &work = *GetTBMProject(this->project);
		const ExcavationJob &job = *GetExcavationJob(work.job);
		const Train *front = Train::Get(work.cutter);
		const auto state = this->State();
		this->Trace(state);
		if (this->replay.empty()) {
			// Snapshot only naturally reached physical states. No project phase,
			// position, reservation, cargo or partial work is staged for saving.
			const std::array ready{work.phase == TBMPhase::TravelToFace && front->cur_speed != 0 && !this->OnBend(),
				front->cur_speed != 0 && this->OnBend(), work.phase == TBMPhase::WaitForEmpties};
			for (size_t kind = 0; kind < ready.size(); ++kind) {
				if (this->saved[kind] || !ready[kind]) continue; // Keep the first actual instance of each distinct reload boundary.
				// Repeat public projections/quotes while travel is active. Existing
				// ownership may refuse purchase, but previewing cannot touch state.
				for (uint repeat = 0; repeat < 3; ++repeat) {
					GetTBMPairControls(work.portal, work.company);
					GetTBMCutterSteeringTarget(*front);
					PreviewTBMCommissionArea(work.company, work.portal, work.model_id, 2, 2, work.origin,
						job.requested_tiles.front(), job.requested_tiles.back(), false);
				}
				REQUIRE(this->State() == state);
				REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
				GfxClearSpriteCache();
				FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				const auto image = GetTestCaseArtifactPaths("tbm", "motion").images / fmt::format("tbm-motion-{}.ppm", KINDS[kind]);
				for (uint repeat = 0; repeat < 3; ++repeat) CaptureNativeArtViewport(front->tile, ZoomLevel::Normal, image); // Repainting the actual moving train cannot advance work, consume RNG or reserve track.
				REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
				GfxClearSpriteCache();
				FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				REQUIRE(this->State() == state);
				const auto file = GetTestCaseArtifactPaths("tbm", "motion").saves / fmt::format("tbm-motion-{}-393.sav", KINDS[kind]);
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(file), false) == SaveLoadResult::Ok);
				REQUIRE(this->State() == state);
				this->saved[kind] = true;
				fmt::print("MOTION_SNAPSHOT kind={} tick={} {}\n", KINDS[kind], this->ticks, state);
			}
		}
		if (this->ticks != LIMIT) return;
		CHECK(job.cursor == job.route.size());
		CHECK(IsTBMProjectFinished(work.phase));
		fmt::print("MOTION_RESULT {}\n", state);
		this->finished = true;
		AutoRestoreBackup actor(_current_company, work.company);
		REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
	}

	/** Initialize only ordinary start/unpause commands after the actual file loader, preserving a replay's saved train state. */
	void Start()
	{
		const auto projects = CaptureTBMProjects();
		REQUIRE(projects.size() == 1);
		this->project = projects.front().id;
		AutoRestoreBackup actor(_current_company, projects.front().company);
		if (this->parking && !this->replay.empty()) {
			const auto &work = *GetTBMProject(this->project);
			this->parked_job = *GetExcavationJob(work.job);
			this->parked_remainder = work.work_remainder;
			if (work.phase == TBMPhase::Parked) this->parked_tick = this->ticks;
		}
		if (const char *bad = std::getenv("OTTD_TBM_PACE_BAD_PREVIEW")) {
			const auto before = this->State();
			const auto result = SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false);
			CHECK(result == SaveLoadResult::Error);
			const char *expected = std::getenv("OTTD_TBM_BAD_EXPECTED");
			CHECK(_load_check_data.error_msg == (expected == nullptr ? "Invalid TBM work fraction" : expected));
			CHECK(this->State() == before);
			fmt::print("PACE_REFUSED preview={} reason={} unchanged={}\n", to_underlying(result), _load_check_data.error_msg, this->State() == before);
			this->finished = true;
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
			return;
		}
		if (!this->pace_seed_layer.empty()) REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, this->project, TBMProjectAction::Resume).Succeeded());
		if (!this->pace_check_layer.empty()) {
			const TBMProject &work = *GetTBMProject(this->project);
			const ExcavationJob &job = *GetExcavationJob(work.job);
			const TBMCutterModel &model = *GetTBMCutterModel(work.model_id);
			const bool deep = this->pace_check_layer == "deep";
			REQUIRE(work.model_id == (deep ? 1 : 0));
			REQUIRE(model.work_rate_numerator == (deep ? 3 : 1));
			REQUIRE(model.work_rate_denominator == (deep ? 4 : 2));
			REQUIRE(work.work_remainder == (this->pace_fraction ? 1 : deep ? 2 : 0));
			REQUIRE(job.partial_work_ticks == (this->pace_fraction && deep ? 2 : 1));
			REQUIRE(job.cursor == 0);
			this->pace_quote = EstimateExcavationCut(work.job);
			REQUIRE(this->pace_quote.error == ExcavationError::None);
			this->pace_partial = job.partial_work_ticks;
			this->pace_remainder = work.work_remainder;
			const uint64_t units = (this->pace_quote.work_ticks - this->pace_partial) * model.work_rate_denominator - this->pace_remainder;
			this->pace_duration = CeilDiv(units, uint64_t{model.work_rate_numerator});
			// The old divisor was exactly half the current divisor, so the same
			// remaining fractional work now takes twice as many ticks, up to one
			// tick of integer rounding. Travel and waits do not enter this budget.
			const uint64_t old_duration = CeilDiv(units, uint64_t{model.work_rate_numerator} * 2);
			CHECK((this->pace_duration == old_duration * 2 || this->pace_duration + 1 == old_duration * 2));
			fmt::print("PACE_LOADED layer={} denominator={} remainder={} old_ticks={} new_ticks={} {}\n", this->pace_check_layer,
				model.work_rate_denominator, work.work_remainder, old_duration, this->pace_duration, this->State());
			if (!this->pace_resaved && !this->pace_fraction) { // Replaying a current fraction must not overwrite the preserved original migration boundary.
				const auto file = GetTestCaseArtifactPaths("tbm", "motion").saves / fmt::format("tbm-pace-{}-394.sav", this->pace_check_layer);
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(file), false) == SaveLoadResult::Ok);
			}
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
			return;
		}
		if (this->replay.empty()) {
			// The retained demo starts paused with manually stopped participants;
			// their real normal vehicle commands begin the baseline journey.
			for (Train *train : Train::Iterate()) {
				if (train->IsFrontEngine() && train->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, train->index, true).Succeeded());
			}
		} else {
			this->Trace(this->State());
			fmt::print("MOTION_LOADED tick={} {}\n", this->ticks, this->State());
		}
		REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
	}
};

static TBMMotionReplay *_motion_replay = nullptr; ///< Test-process observer; absent from production execution.
static const NWidgetPart _motion_widgets[]{NWidget(WWT_PANEL, Colours::Grey), SetMinimalSize(1, 1), EndContainer()}; ///< No gameplay control or input interception.
static WindowDesc _motion_desc(WindowPosition::Manual, {}, 1, 1, WindowClass::ScriptDebug, WindowClass::None, {}, _motion_widgets); ///< Test-only end-of-simulation callback owner.

/** Minimal native window supplies the post-vehicle game-tick event; TimerGameTick fires too early for a complete-frame save. */
class TBMMotionWindow : public Window {
public:
	/** Register a harmless one-pixel observer after loader startup. */
	TBMMotionWindow() : Window(_motion_desc) { this->InitNested(UINT32_MAX); }
	/** Observe complete simulation work and retain failures until ordinary shutdown. */
	void OnGameTick() override
	{
		try { _motion_replay->Observe(); }
		catch (...) {
			_motion_replay->failure = std::current_exception();
			BlitterFactory::SelectBlitter("null"); // A failed capture must restore the null driver's no-pixel renderer before its next draw.
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		}
	}
};

/** @param args Registered console command only. @return Whether the actual startup loader initialized the observer. */
static bool StartTBMMotion(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1);
	_motion_replay->Start();
	new TBMMotionWindow();
	return true;
}

/** One bounded normal-game process writes or genuinely reloads a moving/waiting file, with no fixture reconstruction. */
TEST_CASE("TBM motion snapshots replay actual native frames", "[.tbm-motion-reload]")
{
	TBMMotionReplay probe;
	probe.root = FindTestRepositoryRoot();
	probe.parking = std::getenv("OTTD_TBM_PARKING") != nullptr;
	probe.release = std::getenv("OTTD_TBM_FINAL_RELEASE") != nullptr;
	if (probe.release) probe.parking = true;
	if (const char *layer = std::getenv("OTTD_TBM_PACE_SEED")) {
		probe.pace_seed_layer = layer;
		REQUIRE((probe.pace_seed_layer == "underground" || probe.pace_seed_layer == "deep"));
	}
	if (const char *layer = std::getenv("OTTD_TBM_PACE_CHECK")) {
		probe.pace_check_layer = layer;
		REQUIRE((probe.pace_check_layer == "underground" || probe.pace_check_layer == "deep"));
		probe.pace_resaved = std::getenv("OTTD_TBM_PACE_RESAVED") != nullptr;
		probe.pace_fraction = std::getenv("OTTD_TBM_PACE_FRACTION") != nullptr;
	}
	if (const char *kind = std::getenv("OTTD_TBM_MOTION_REPLAY")) {
		probe.replay = kind;
		const auto &kinds = probe.parking ? TBMMotionReplay::PARK_KINDS : TBMMotionReplay::KINDS;
		REQUIRE(std::ranges::find(kinds, probe.replay) != kinds.end());
		const char *tick = std::getenv("OTTD_TBM_MOTION_TICK");
		REQUIRE(tick != nullptr);
		const auto parsed = std::from_chars(tick, tick + strlen(tick), probe.ticks);
		REQUIRE(parsed.ec == std::errc{});
		REQUIRE(*parsed.ptr == '\0');
		REQUIRE(probe.ticks < TBMMotionReplay::LIMIT);
	}
	AutoRestoreBackup observer(_motion_replay, &probe);
	IConsole::CmdRegister("tbm_motion", StartTBMMotion);
	const char *bad_load = std::getenv("OTTD_TBM_PACE_BAD_LOAD");
	const auto source_name = probe.parking ? (probe.replay.empty() ? "tbm-feedback-haul50-393.sav" : fmt::format("tbm-parking-{}-396.sav", probe.replay)) :
		probe.pace_fraction ? fmt::format("tbm-pace-{}-fraction-394.sav", probe.pace_check_layer) :
		!probe.pace_check_layer.empty() ? fmt::format("tbm-pace-{}-{}.sav", probe.pace_check_layer, probe.pace_resaved ? 394 : 393) :
		!probe.pace_seed_layer.empty() ? fmt::format("tbm-fresh-{}-area-393.sav", probe.pace_seed_layer) :
		probe.replay.empty() ? "underground-demo-tbm-ui-393.sav" : fmt::format("tbm-motion-{}-393.sav", probe.replay);
	const auto runtime_config = GetTestRuntimeConfig().string();
	const char *replay_source = std::getenv("OTTD_TBM_REPLAY_SOURCE");
	// A continuation consumes its caller's exact generated snapshot. Historical
	// compatibility seeds remain explicit defaults for independent migration tests.
	const auto selected = replay_source != nullptr ? std::filesystem::path{replay_source} : probe.root / "src/tests/fixtures/haulage" / source_name;
	const auto source = ResolveTestInputPath(bad_load != nullptr ? std::filesystem::path{bad_load} : selected).string();
	const auto video = fmt::format("null:ticks={}", bad_load != nullptr ? 3 : TBMMotionReplay::LIMIT - probe.ticks + 2);
	std::array<std::string_view, 21> args{"RelWithDebInfo/openttd_test.exe", "-v", video, "-b", "null", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", runtime_config, "-g", source, "-r", "1280x733", "-d", "sl=1"};
	CHECK(openttd_main(args) == 0);
	if (probe.failure) std::rethrow_exception(probe.failure);
	if (bad_load != nullptr) {
		// A malformed startup file must never reach OnStartGame or dispatch its
		// game-start script. The runner also requires the precise loader error.
		CHECK(probe.project == TBMProjectID{UINT32_MAX});
		CHECK_FALSE(probe.finished);
		fmt::print("PACE_REFUSED full_load=true startup_dispatched={}\n", probe.project != TBMProjectID{UINT32_MAX});
		return;
	}
	REQUIRE(probe.finished);
	// A movement baseline must reach all three states; a rate seed has its own
	// explicit partial-work proof and never substitutes for that baseline.
	if (probe.replay.empty() && probe.pace_seed_layer.empty() && probe.pace_check_layer.empty()) for (bool saved : probe.saved) REQUIRE(saved);
}
