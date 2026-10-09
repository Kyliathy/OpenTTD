/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_feedback_runtime.cpp Native input, rendering and bounded loaded-game diagnostics.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Groups1/5B/11
 * @see docs/zz-reach2/architecture/gui/archi-gui.md
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../company_cmd.h"
#include "../construction_preview.h"
#include "../train.h"
#include "../tbm.h"
#include "../tbm_cmd.h"
#include "../tbm_exchange.h"
#include "../tbm_controls.h"
#include "../tbm_replanning.h"
#include "../tbm_area_preview.h"
#include "../layer_portal_map.h"
#include "../map_layer_gui.h"
#include "../order_cmd.h"
#include "../core/random_func.hpp"
#include "../spoil_receiver.h"
#include "../underground_gui.h"
#include "../gui.h"
#include "../toolbar_gui.h"
#include "../rail_map.h"
#include "../track_func.h"
#include "../pbs.h"
#include "../news_gui.h"
#include "../news_func.h"
#include "../tilehighlight_func.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../strings_func.h"
#include "../command_func.h"
#include "../misc_cmd.h"
#include "../vehicle_cmd.h"
#include "../timer/timer.h"
#include "../timer/timer_game_tick.h"
#include "../timer/timer_window.h"
#include "../widgets/toolbar_widget.h"
#include "../widgets/underground_widget.h"
#include "../widgets/tbm_widget.h"
#include "../widgets/viewport_widget.h"
#include "../widgets/layer_portal_widget.h"
#include "../widgets/vehicle_widget.h"
#include "../widgets/rail_widget.h"
#include "../table/strings.h"
#include "native_art_render_fixture.h"
#include "tbm_feedback_haul_fixture.h"
#include "../tbm_gui.h"
#include "../vehicle_gui.h"
#include "layer_save_fixture.h"
#include "../saveload/saveload_func.h"
#include <cstdlib>

#include "../safeguards.h"

/**
 * Feed a real press/release through native hit testing, including modal/cursor ownership.
 * @param point Screen position supplied by the automated input device.
 * @param down Whether the left button remains down (for an actual drag).
 */
static void FeedbackMouse(Point point, bool down)
{
	_cursor.pos = point;
	_cursor.in_window = true;
	_left_button_down = down;
	if (!down) _left_button_clicked = false;
	HandleMouseEvents();
}

/**
 * Click the centre of a real, currently visible widget through the native input dispatcher.
 * @param window Existing owner; no direct OnClick invocation bypasses hit testing.
 * @param widget Widget whose real layout supplies the coordinates.
 */
static void FeedbackClick(Window &window, WidgetID widget)
{
	const Rect rect = window.GetWidget<NWidgetBase>(widget)->GetCurrentRect();
	const Point point{window.left + (rect.left + rect.right) / 2, window.top + (rect.top + rect.bottom) / 2};
	FeedbackMouse(point, false);
	FeedbackMouse(point, true);
	FeedbackMouse(point, false);
}

/**
 * Resolve a physical tile to its visible main-viewport position for ordinary placement input.
 * @param tile Actual explicit-layer world tile, centred before use.
 * @return Screen position of the tile's ground centre.
 */
static Point FeedbackTilePoint(TileIndex tile)
{
	const Viewport &view = *GetMainWindow()->viewport;
	const Point world = RemapCoords(TileX(tile) * TILE_SIZE + TILE_SIZE / 2, TileY(tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(tile));
	return {view.left + UnScaleByZoom(world.x - view.virtual_left, view.zoom), view.top + UnScaleByZoom(world.y - view.virtual_top, view.zoom)};
}

/** Read-only runtime trace plus ordinary UI input; no work, movement or inventory owner is invoked manually. */
class TBMFeedbackProbe {
public:
	std::filesystem::path root; ///< Repository containing retained inputs and review outputs.
	TBMProjectID project_id{UINT32_MAX}; ///< Actual loaded controller, never synthesized.
	bool started = false; ///< Native loader has reconstructed all feature owners.
	uint ticks = 0; ///< Actual simulation frames observed after ordinary unpause.
	uint64_t boring_ticks = 0; ///< Frames actually spent in the Bore phase without a hold.
	std::string previous; ///< Last physical/controller state; avoids unbounded duplicate trace rows.
	std::exception_ptr failure; ///< Report assertion only after native shutdown.
	std::array<uint64_t, to_underlying(TBMPhase::FollowOrders) + 1> phase_frames{}; ///< Native elapsed frames by work phase, including selected hangars and explicit native-list travel.
	std::array<uint64_t, 32> blocked_frames{}; ///< Native elapsed frames by recoverable hold.
	uint32_t last_cut = UINT32_MAX; ///< Previous geological cursor, for complete-cut timing output.
	uint64_t cut_boring_start = 0; ///< Start of current cut's active-work interval.
	uint64_t last_earth = 0; ///< Previous committed statistic; never cargo inventory.
	uint64_t last_rock = 0; ///< Previous committed statistic; never cargo inventory.
	bool long_haul = false; ///< Separate paid detour measurement, preserving the original demo.
	std::string balance; ///< Optional retained whole-unit balance input: haul50 or old-work; skips repeated GUI characterization.
	bool advice = false; ///< Current managed-advice acceptance using the retained native loop and two actual status captures.
	std::string final_output; ///< Fresh Group11 evidence prefix; empty retains historical observer behavior and filenames.
	bool captured_hauler = false; ///< Empty-return caption captured during real movement, independently of cutter work.
	Money initial_train_cost = 0; ///< Native company running-cost ledger at the beginning of this balance interval.
	bool reload_demo = false; ///< Verify the renamed playable copy through the real loader, without repairing it again.
	bool extend_work = false; ///< Native Group3B selection/drag/confirmation journey, separate from unchanged UI baseline captures.
	bool straight_tunnel = false; ///< Select actual straight-intent UI and verify exact endpoint arrival on the same machine.
	bool reload_accepted = false; ///< Load the real paused accepted snapshot, then resume without recreating its work.
	MapLayer fresh_layer = MapLayer::Surface; ///< Optional uncommissioned scenario; Surface selects the original Underground demo.
	uint32_t original_route_size = 0; ///< Original accepted endpoint, used to prove the same physical machine reaches added cuts.
	uint64_t quoted_extension_earth = 0; ///< Additional Earth from the actual reviewed extension quote.
	uint64_t quoted_extension_rock = 0; ///< Additional Rock from the actual reviewed extension quote.
	std::optional<uint64_t> original_earth; ///< Actual receipt total when the native cutter completes its original prefix.
	std::optional<uint64_t> original_rock; ///< Actual Rock receipt total at that same native boundary.
	bool finished = false; ///< Stop the bounded observer after live labels and final actual counts are retained.
	bool captured_running = false; ///< One real moving/boring vehicle status capture, independent of its final parked pose.
	std::unique_ptr<FeedbackHaulBuilder> builder; ///< Optional native paced construction before measurement begins.

	/** @return Stable evidence prefix for this actual layer/intent journey, without overwriting the retained demo captures. */
	std::string EvidencePrefix() const
	{
		if (!this->final_output.empty()) return this->final_output + "-";
		if (this->advice) return "tbm-advice-";
		if (this->fresh_layer != MapLayer::Surface) return fmt::format("tbm-fresh-{}-{}-", this->fresh_layer == MapLayer::DeepUnderground ? "deep" : "underground", this->straight_tunnel ? "tunnel" : "area");
		return this->straight_tunnel ? "tbm-tunnel-" : this->extend_work ? "tbm-extend-" : "tbm-ui-";
	}

	/** @return Accepted snapshot filename belonging only to the current actual layer and work intent. */
	std::string AcceptedFilename() const
	{
		if (!this->final_output.empty()) return this->EvidencePrefix() + "accepted.sav";
		if (this->fresh_layer != MapLayer::Surface) return this->EvidencePrefix() + "393.sav";
		return this->straight_tunnel ? "tbm-extended-tunnel-393.sav" : "tbm-extended-area-393.sav";
	}

	/** @param palette Actual armed work palette. Select straight intent through the native deferred dropdown, preserving cursor ownership. */
	void SelectStraightIntent(Window &palette) const
	{
		this->Capture("intent", [&] {
			FeedbackClick(palette, WID_UC_INTENT);
			Window *menu = FindWindowById(WindowClass::DropdownMenu, 0);
			REQUIRE(menu != nullptr);
			const Point second{menu->left + menu->width / 2, menu->top + 3 * menu->height / 4};
			FeedbackMouse(second, true);
			FeedbackMouse(second, false);
			// Native dropdown selections dispatch after their ordinary deferred
			// mouse-loop passes; no callback is invoked directly by this observer.
			for (uint pass = 0; pass < 5; ++pass) TimerManager<TimerWindow>::Elapsed(std::chrono::milliseconds(30));
		});
		REQUIRE(palette.GetWidgetString(WID_UC_INTENT, STR_TBM_INTENT_AREA) == GetString(STR_TBM_INTENT_TUNNEL));
	}

	/**
	 * Start or stop only the actual cutter/assigned hauler through their ordinary vehicle commands.
	 * @param running Desired manual train state; already matching participants are untouched.
	 */
	void SetParticipantsRunning(bool running) const
	{
		const auto *project = GetTBMProject(this->project_id);
		REQUIRE(project != nullptr);
		std::vector<VehicleID> participants{project->cutter};
		for (const auto &assignment : CaptureTBMExchangeAssignments()) {
			if (assignment.project == project->id) participants.push_back(assignment.hauler); // Only this live job's actual participants belong to the test's start/stop action.
		}
		AutoRestoreBackup actor(_current_company, project->company);
		for (VehicleID id : participants) {
			const Train *train = Train::Get(id);
			if (train->vehstatus.Test(VehState::Stopped) != running) continue;
			REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, id, true).Succeeded());
		}
	}

	/**
	 * Retain the real rendered screen with native windows and current prompt text.
	 * @param name Stable repository-owned review filename suffix.
	 * @param input Optional actual input sequence to execute inside a valid screen buffer.
	 */
	void Capture(std::string_view name, const std::function<void()> &input = {}) const
	{
		const auto path = GetTestCaseArtifactPaths("tbm", "planning").images / fmt::format("{}{}{}.ppm",
			this->EvidencePrefix(), this->reload_accepted ? "reload-" : this->long_haul ? "haul50-" : "", name);
		/** Native drawing and input share the same screen geometry as the supplied screenshot. */
		CaptureNativeArtCanvas(1280, 733, path, [&] {
			// Consecutive test captures have no intervening event-loop frame.
			// Flush native scheduled resizes so picking and rendering see the
			// current viewport rectangle rather than the previous scale's cache.
			for (Window *window : Window::Iterate()) window->ProcessScheduledResize();
			if (input) input();
			for (Window *window : Window::Iterate()) window->ProcessScheduledResize();
			UpdateViewportPosition(GetMainWindow(), 0);
			DrawOverlappedWindowForAll(0, 0, 1280, 733);
		});
	}

	/**
	 * Log the palette's actual localized prompt and quote state after native input.
	 * @param window Real construction palette.
	 * @param step Human-readable interaction name.
	 */
	void Prompt(const Window &window, std::string_view step) const
	{
		fmt::print("FEEDBACK_UI step={} size={}x{} cursor={} confirm_disabled={}\n", step, window.width, window.height,
			_thd.GetCallbackWnd() == &window, window.IsWidgetDisabled(WID_UC_AREA_CONFIRM));
		// Retain the exact user-visible instruction and duplicate-empty rows. Their owner,
		// not test-side inferred state, determines whether a click communicated its next step.
		for (WidgetID widget : {WID_UC_STATUS, WID_UC_JOB_PROGRESS, WID_UC_AREA_PRICE, WID_UC_AREA_MATERIAL, WID_UC_AREA_FLOOR}) {
			fmt::print("FEEDBACK_TEXT widget={} text={}\n", widget, window.GetWidgetString(widget, window.GetWidget<NWidgetCore>(widget)->GetString()));
		}
	}

	/**
	 * Exercise normal layout/input after a real interface-scale change, preserving simulation state.
	 * @param palette Existing native construction palette; reinitialization retains its explicit plane.
	 */
	void PresentationVariants(Window &palette) const
	{
		const int old_scale = _gui_scale_cfg;
		// These two player-sized cases supplement the supplied 1280x733 / 150%
		// baseline. Only presentation scale/window geometry changes between them.
		for (const auto [scale, width, height] : {std::tuple{100, 1024, 600}, std::tuple{200, 1280, 733}}) {
			const auto file = GetTestCaseArtifactPaths("tbm", "planning").images / fmt::format("tbm-ui-scale-{}.ppm", scale);
			CaptureNativeArtCanvas(width, height, file, [&] {
				_gui_scale_cfg = scale;
				if (AdjustGUIZoom(false)) ReInitAllWindows(true);
				RelocateAllWindows(width, height);
				palette.left = width - palette.width;
				palette.top = 50;
				FeedbackClick(palette, WID_UC_DETAILS);
				CHECK(palette.width < width / 2);
				CHECK(palette.top + palette.height < height - 30);
				FeedbackClick(palette, WID_UC_SURVEY);
				REQUIRE(_thd.GetCallbackWnd() == &palette);
				DrawOverlappedWindowForAll(0, 0, width, height);
				fmt::print("FEEDBACK_LAYOUT scale={} screen={}x{} expanded={}x{}\n", scale, width, height, palette.width, palette.height);
				HandleKeypress(WKC_ESC, 0);
				REQUIRE(_thd.place_mode == HT_NONE);
				CHECK(_thd.GetCallbackWnd() != &palette);
				FeedbackClick(palette, WID_UC_DETAILS);
			});
		}
		this->Capture("scale-restored", [&] {
			_gui_scale_cfg = old_scale;
			if (AdjustGUIZoom(false)) ReInitAllWindows(true);
			RelocateAllWindows(1280, 733);
			palette.left = 1280 - palette.width;
			palette.top = 50;
		});
	}

	/**
	 * Verify native layer-menu cancellation and right-button window close without invoking placement callbacks directly.
	 * @param palette Existing compact construction palette.
	 * @param anchor Actual project tile used to open a separate rock-layer viewport.
	 */
	void VerifyInputCancellation(Window &palette, TileIndex anchor) const
	{
		this->Capture("extra-layer-cancel", [&] {
			ShowExtraViewportWindow(anchor);
			Window *extra = FindWindowById(WindowClass::ExtraViewport, 0);
			REQUIRE(extra != nullptr);
			extra->left = 0;
			extra->top = 70;
			FeedbackClick(palette, WID_UC_AREA);
			REQUIRE(_thd.GetCallbackWnd() == &palette);
			FeedbackClick(*extra, WID_EV_MAP_LAYER);
			Window *menu = FindWindowById(WindowClass::DropdownMenu, 0);
			REQUIRE(menu != nullptr);
			const Point first{menu->left + menu->width / 2, menu->top + WidgetDimensions::scaled.framerect.top + ScaleGUITrad(10)};
			FeedbackMouse(first, true);
			FeedbackMouse(first, false);
			// A real dropdown selection is dispatched after its normal deferred
			// input passes, just as the player-facing toolbar menu is above.
			for (uint i = 0; i < 5; ++i) TimerManager<TimerWindow>::Elapsed(std::chrono::milliseconds(30));
			CHECK(extra->viewport->layer == MapLayer::Surface);
			CHECK(GetMainWindow()->viewport->layer == GetTileLayer(anchor));
			REQUIRE(_thd.place_mode == HT_NONE);
			CHECK(_thd.GetCallbackWnd() != &palette);
		});
		CloseWindowByClass(WindowClass::ExtraViewport);
		this->Capture("right-close-cancel", [&] {
			FeedbackClick(palette, WID_UC_SURVEY);
			REQUIRE(_thd.GetCallbackWnd() == &palette);
			AutoRestoreBackup close_setting(_settings_client.gui.right_click_wnd_close, RightClickClose::Yes);
			const Rect title = palette.GetWidget<NWidgetBase>(WID_UC_CAPTION)->GetCurrentRect();
			_cursor.pos = {palette.left + title.left + title.Width() / 2, palette.top + title.top + title.Height() / 2};
			_right_button_clicked = true;
			HandleMouseEvents();
			CHECK(FindWindowById(WindowClass::UndergroundConstruction, GetTileLayer(anchor)) == nullptr);
			CHECK(_thd.place_mode == HT_NONE);
			CHECK(_thd.GetCallbackWnd() != &palette);
		});
		Window::DeleteClosedWindows();
	}

	/** Render actual spectator, narrow-toolbar and editor entry points with the loaded TBM control family. */
	void VerifyToolbarVariants() const
	{
		{
			AutoRestoreBackup spectator(_local_company, COMPANY_SPECTATOR);
			this->Capture("toolbar-disabled");
			Window *toolbar = FindWindowById(WindowClass::MainToolbar, 0);
			REQUIRE(toolbar != nullptr);
			CHECK(toolbar->IsWidgetDisabled(WID_TN_AIR));
			CHECK(toolbar->GetWidget<NWidgetCore>(WID_TN_AIR)->GetToolTip() == STR_UNDERGROUND_MENU);
		}
		const auto narrow = GetTestCaseArtifactPaths("tbm", "planning").images / "tbm-ui-toolbar-split.ppm";
		const int previous_scale = _gui_scale_cfg;
		_gui_scale_cfg = 200; // Fixed player scaling exercises the split layout instead of automatic downscaling to fit every button.
		CaptureNativeArtCanvas(640, 500, narrow, [&] {
			CloseWindowByClass(WindowClass::MainToolbar);
			Window::DeleteClosedWindows();
			ScreenSizeChanged();
			AllocateToolbar();
			Window *toolbar = FindWindowById(WindowClass::MainToolbar, 0);
			REQUIRE(toolbar != nullptr);
			ResizeWindow(toolbar, 640 - toolbar->width, 0, false, false);
			FeedbackClick(*toolbar, WID_TN_SWITCH_BAR);
			CHECK(toolbar->IsWidgetLowered(WID_TN_SWITCH_BAR));
			FeedbackClick(*toolbar, WID_TN_SWITCH_BAR);
			CHECK_FALSE(toolbar->IsWidgetLowered(WID_TN_SWITCH_BAR));
			DrawOverlappedWindowForAll(0, 0, 640, 500);
			CHECK(toolbar->GetWidget<NWidgetCore>(WID_TN_AIR)->GetToolTip() == STR_UNDERGROUND_MENU);
		});
		_gui_scale_cfg = previous_scale;
		{
			AutoRestoreBackup editor(_game_mode, GameMode::Editor);
			this->Capture("toolbar-editor", [&] {
				CloseWindowByClass(WindowClass::MainToolbar);
				Window::DeleteClosedWindows();
				ScreenSizeChanged();
				AllocateToolbar();
			});
			Window *toolbar = FindWindowById(WindowClass::MainToolbar, 0);
			REQUIRE(toolbar != nullptr);
			CHECK(toolbar->GetWidget<NWidgetCore>(WID_TE_INDUSTRY)->GetToolTip() == STR_UNDERGROUND_MENU);
		}
		this->Capture("toolbar-restored", [&] {
			CloseWindowByClass(WindowClass::MainToolbar);
			Window::DeleteClosedWindows();
			ScreenSizeChanged();
			AllocateToolbar();
			RelocateAllWindows(1280, 733);
		});
	}

	/**
	 * Pause, draw, confirm and resume additional work exclusively through native input.
	 * @param palette Existing construction palette reached from the real toolbar.
	 * @param browser Actual owned-project list; its selected row identifies the same loaded cutter.
	 */
	void ExtendThroughUI(Window &palette, Window &browser)
	{
		const auto project = *GetTBMProject(this->project_id);
		const auto job = *GetExcavationJob(project.job);
		this->original_route_size = static_cast<uint32_t>(job.route.size());
		const auto vehicles = Vehicle::GetNumItems();
		const auto money = Company::Get(project.company)->money;
		const auto delta = TileIndexDiffCByDir(job.route.back().heading);
		const TileIndex start = TileAddWrap(job.route.back().cutter_position, delta.x, delta.y);
		const TileIndex end = TileAddWrap(job.route.back().cutter_position, 6 * delta.x, 6 * delta.y);
		this->Capture("select", [&] { FeedbackClick(browser, WID_TP_EXTEND); });
		REQUIRE(GetTBMProject(project.id)->blocked == TBMBlockedReason::Paused);
		REQUIRE(_thd.GetCallbackWnd() == &palette);
		if (this->straight_tunnel) this->SelectStraightIntent(palette);
		/** Draw the same requested floor using real viewport coordinates after any camera recentering. */
		const auto draw = [&] {
			Window *toolbar = FindWindowById(WindowClass::MainToolbar, 0);
			// Expose the six-cell request using the ordinary zoom control; its
			// screen coordinates must be real and outside the overlapping windows.
			for (uint step = 0; step < 4 && GetMainWindow()->viewport->zoom < ZoomLevel::Normal; ++step) FeedbackClick(*toolbar, WID_TN_ZOOM_OUT);
			UpdateViewportPosition(GetMainWindow(), 0);
			const Point first = FeedbackTilePoint(start);
			const Point last = FeedbackTilePoint(end);
			if (this->straight_tunnel) {
				FeedbackMouse(last, false);
				FeedbackMouse(last, true);
				FeedbackMouse(last, false);
				return;
			}
			FeedbackMouse(first, false);
			FeedbackMouse(first, true);
			FeedbackMouse(last, true);
			FeedbackMouse(last, false);
		};
		this->Capture("quote", draw);
		this->Prompt(palette, "extension-quote");
		REQUIRE_FALSE(palette.IsWidgetDisabled(WID_UC_AREA_CONFIRM));
		const auto quoted = PreviewTBMExtension(project.company, project.id, this->straight_tunnel ? job.route.back().cutter_position : start, end,
			this->straight_tunnel ? ExcavationPlanIntent::StraightTunnel : ExcavationPlanIntent::Area);
		REQUIRE(quoted.result.Succeeded());
		if (this->straight_tunnel) {
			CHECK(quoted.plan.route.size() == 6);
			CHECK(quoted.plan.end.position == end);
			// Each exact centre appears in the actual active map preview before
			// confirmation; numbering is derived and cannot replace the route.
			for (size_t index = 0; index < quoted.plan.route.size(); ++index) {
				CHECK(quoted.plan.route[index].heading == job.route.back().heading);
				CHECK(GetConstructionPreviewRouteLabel(quoted.plan.route[index].cutter_position) == fmt::format("{}", index + 1));
			}
		}
		this->quoted_extension_earth = quoted.plan.estimate.earth_t;
		this->quoted_extension_rock = quoted.plan.estimate.rock_t;
		CHECK(palette.GetWidgetString(WID_UC_AREA_MATERIAL, STR_UNDERGROUND_AREA_EMPTY) == GetString(STR_UNDERGROUND_AREA_MATERIAL,
			quoted.plan.estimate.earth_t, quoted.plan.estimate.rock_t, quoted.plan.estimate.support_units, quoted.plan.estimate.work_ticks));
		CHECK(*GetExcavationJob(project.job) == job);
		const auto paused = *GetTBMProject(project.id);
		this->Capture("cancelled", [&] { HandleKeypress(WKC_ESC, 0); });
		CHECK(_thd.GetCallbackWnd() != &palette);
		CHECK(palette.GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_TBM_EXTEND_CANCELLED));
		CHECK(*GetTBMProject(project.id) == paused);
		CHECK(GetConstructionPreviewRouteLabel(end).empty());
		CHECK(*GetExcavationJob(project.job) == job);
		CHECK(Company::Get(project.company)->money == money);
		this->Capture("reselected", [&] { FeedbackClick(browser, WID_TP_EXTEND); });
		this->Capture("requote", draw);
		REQUIRE_FALSE(palette.IsWidgetDisabled(WID_UC_AREA_CONFIRM));
		this->Capture("confirmed", [&] { FeedbackClick(palette, WID_UC_AREA_CONFIRM); });
		const auto *extended = GetTBMProject(project.id);
		const auto *accepted = GetExcavationJob(project.job);
		REQUIRE(extended != nullptr);
		REQUIRE(accepted != nullptr);
		CHECK(extended->cutter == project.cutter);
		CHECK(extended->permanent_parts == project.permanent_parts);
		CHECK(extended->job == project.job);
		CHECK(extended->blocked == TBMBlockedReason::Paused);
		REQUIRE(accepted->route.size() > job.route.size());
		CHECK(std::equal(job.route.begin(), job.route.end(), accepted->route.begin()));
		CHECK(accepted->cursor == job.cursor);
		CHECK(accepted->partial_work_ticks == job.partial_work_ticks);
		CHECK(accepted->completed_earth_t == job.completed_earth_t);
		CHECK(accepted->completed_rock_t == job.completed_rock_t);
		CHECK(Vehicle::GetNumItems() == vehicles);
		CHECK(Company::Get(project.company)->money == money);
		const auto save = GetTestCaseArtifactPaths("tbm", "planning").saves / this->AcceptedFilename();
		if (!this->final_output.empty()) REQUIRE_FALSE(std::filesystem::exists(save));
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(save), false) == SaveLoadResult::Ok);
		fmt::print("FEEDBACK_EXTENSION project={} cutter={} job={} old_cuts={} new_cuts={} cursor={} saved={}\n",
			project.id.base(), project.cutter.base(), project.job.base(), job.route.size(), accepted->route.size(), accepted->cursor, save.string());
		this->Capture("resumed", [&] {
			Window *portal = FindWindowById(WindowClass::LayerPortal, project.portal);
			REQUIRE(portal != nullptr);
			FeedbackClick(*portal, PW_PAUSE);
		});
		CHECK(GetTBMProject(project.id)->blocked != TBMBlockedReason::Paused);
		this->Capture("machine", [&] {
			Window *portal = FindWindowById(WindowClass::LayerPortal, project.portal);
			REQUIRE(portal != nullptr);
			FeedbackClick(*portal, PW_SHOW_MACHINE);
		});
	}

	/** Commission through actual portal/floor input, then extend and drive that same newly acquired machine on the selected rock plane. */
	void StartFresh()
	{
		// The headless driver selects its null renderer during startup; switch
		// to the same actual raster renderer used by the retained demo captures.
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		REQUIRE(CaptureTBMProjects().empty());
		REQUIRE(Company::IsValidID(_local_company));
		AutoRestoreBackup actor(_current_company, _local_company);
		const MapLayer upper = this->fresh_layer == MapLayer::Underground ? MapLayer::Surface : MapLayer::Underground;
		TileIndex portal = INVALID_TILE;
		// Resolve the actual unique owned pair from the loaded scenario. Camera
		// offsets and a fixture's old numeric IDs cannot substitute for its owner.
		for (uint y = 1; y + 1 < Map::SizeY(); ++y) {
			for (uint x = 1; x + 1 < Map::SizeX(); ++x) {
				const TileIndex tile = TileXYL(x, y, upper);
				if (!IsTileType(tile, TileType::LayerPortal) || GetTileOwner(tile) != _current_company || GetLayerPortalTransportType(tile) != TransportType::Rail) continue;
				if (GetTileLayer(GetLayerPortalPairTile(tile)) != this->fresh_layer) continue;
				REQUIRE(portal == INVALID_TILE);
				portal = tile;
			}
		}
		REQUIRE(portal != INVALID_TILE);
		const TileIndex rock = GetLayerPortalPairTile(portal);
		const auto delta = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(rock));
		const TileIndex first = TileAddWrap(rock, delta.x, delta.y);
		const TileIndex end = TileAddWrap(rock, 3 * delta.x, 3 * delta.y);
		const auto origin = GetLayerPortalDefaultTBMOrigin(portal, _current_company);
		const auto model = GetLayerPortalDefaultTBMModel(portal);
		const auto consist = GetDefaultTBMConsist(this->fresh_layer);
		REQUIRE(origin != StationID::Invalid());
		VehicleID hauler = VehicleID::Invalid();
		for (const Train *train : Train::Iterate()) {
			if (train->owner != _current_company || !train->IsFrontEngine()) continue; // Only the actual pre-existing funded freight head can supply empty hoppers.
			REQUIRE(hauler == VehicleID::Invalid());
			hauler = train->index;
		}
		REQUIRE(hauler != VehicleID::Invalid());
		SetViewportLayer(GetMainWindow(), this->fresh_layer);
		ScrollMainWindowToTile(rock, true);
		Window *palette = nullptr;
		this->Capture("fresh-toolbar", [&] {
			RelocateAllWindows(1280, 733);
			FeedbackClick(*FindWindowById(WindowClass::MainToolbar, 0), WID_TN_AIR);
		});
		this->Capture("fresh-palette", [&] {
			Window *menu = FindWindowById(WindowClass::DropdownMenu, 0);
			REQUIRE(menu != nullptr);
			const Point choice{menu->left + menu->width / 2, menu->top + menu->height / 2};
			FeedbackMouse(choice, true);
			FeedbackMouse(choice, false);
			for (uint pass = 0; pass < 5; ++pass) TimerManager<TimerWindow>::Elapsed(std::chrono::milliseconds(30)); // Dispatch the native deferred toolbar choice.
			palette = FindWindowById(WindowClass::UndergroundConstruction, this->fresh_layer);
			REQUIRE(palette != nullptr);
			palette->left = 1280 - palette->width;
			palette->top = 50;
		});
		this->Capture("fresh-armed", [&] { FeedbackClick(*palette, WID_UC_AREA); });
		if (this->straight_tunnel) this->SelectStraightIntent(*palette);
		this->Capture("fresh-portal", [&] {
			Window *toolbar = FindWindowById(WindowClass::MainToolbar, 0);
			for (uint step = 0; step < 4 && GetMainWindow()->viewport->zoom < ZoomLevel::Normal; ++step) FeedbackClick(*toolbar, WID_TN_ZOOM_OUT); // Keep the actual portal and endpoint outside the palette's chrome.
			UpdateViewportPosition(GetMainWindow(), 0);
			const Point point = FeedbackTilePoint(rock);
			FeedbackMouse(point, false);
			FeedbackMouse(point, true);
			FeedbackMouse(point, false);
		});
		const auto money = Company::Get(_current_company)->money;
		const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
		const auto vehicles = Vehicle::GetNumItems();
		this->Capture("fresh-quote", [&] {
			const Point point = FeedbackTilePoint(this->straight_tunnel ? end : first);
			const Point last = FeedbackTilePoint(end);
			FeedbackMouse(point, false);
			FeedbackMouse(point, true);
			if (!this->straight_tunnel) FeedbackMouse(last, true);
			FeedbackMouse(last, false);
		});
		this->Prompt(*palette, "fresh-quote");
		const auto quote = PreviewTBMCommissionArea(_current_company, portal, model, consist.earth_wagons, consist.rock_wagons, origin,
			this->straight_tunnel ? rock : first, end, false, this->straight_tunnel ? ExcavationPlanIntent::StraightTunnel : ExcavationPlanIntent::Area);
		INFO("fresh quote refusal=" << quote.purchase.GetErrorMessage().base());
		REQUIRE(quote.purchase.Succeeded());
		REQUIRE_FALSE(palette->IsWidgetDisabled(WID_UC_AREA_CONFIRM));
		CHECK(Company::Get(_current_company)->money == money);
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
		CHECK(Vehicle::GetNumItems() == vehicles);
		this->Capture("fresh-confirmed", [&] { FeedbackClick(*palette, WID_UC_AREA_CONFIRM); });
		const auto projects = CaptureTBMProjects();
		REQUIRE(projects.size() == 1);
		this->project_id = projects.front().id;
		CHECK(Company::Get(_current_company)->money == money - quote.purchase.GetCost());
		CHECK(GetExcavationJob(projects.front().job)->route == quote.area.plan.route);
		// Logistics use ordinary synchronized commands on the scenario's real
		// hauler. Neither assignment nor its origin order advances a work tick.
		REQUIRE(Command<Commands::AssignTBMHauler>::Do(DoCommandFlag::Execute, this->project_id, hauler).Succeeded());
		Order visit;
		visit.MakeGoToStation(origin);
		REQUIRE(Command<Commands::InsertOrder>::Do(DoCommandFlag::Execute, hauler, VehicleOrderID{0}, visit).Succeeded());
		this->Capture("fresh-projects", [&] { FeedbackClick(*palette, WID_UC_PROJECT); });
		Window *browser = FindWindowById(WindowClass::TBMProjects, this->fresh_layer);
		REQUIRE(browser != nullptr);
		this->ExtendThroughUI(*palette, *browser);
		REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		this->SetParticipantsRunning(true);
		REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
		this->started = true;
	}

	/** Exercise the loaded demo through real button hit testing and world placement input, then start ordinary simulation. */
	void Start()
	{
		try {
			if (this->fresh_layer != MapLayer::Surface && !this->reload_accepted) { // Fresh scenarios first acquire their machine through the actual UI; accepted reloads already own it.
				this->StartFresh();
				return;
			}
			const auto projects = CaptureTBMProjects();
			REQUIRE_FALSE(projects.empty());
			this->project_id = projects.front().id;
			const auto &project = projects.front();
			AutoRestoreBackup actor(_current_company, project.company);
			const auto *job = GetExcavationJob(project.job);
			REQUIRE(job != nullptr);
			const Company *company = Company::Get(project.company);
			if (!this->balance.empty()) {
				// Reuse the actual retained paid route and loaded participants.
				// Save immediately before commands/ticks so the runner can check
				// old receipts and cargo against the original file directly.
				const auto saved = GetTestCaseArtifactPaths("tbm", "planning").saves / fmt::format("tbm-spoil-{}-loaded-394.sav", this->balance);
				if (!this->advice) REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(saved), false) == SaveLoadResult::Ok); // Advice consumes the retained loaded loop without replacing balance evidence.
				// This bounded run can cross New Year. Sum retained year ledgers
				// so their normal annual rotation cannot look like a cost refund.
				for (const auto &year : company->yearly_expenses) this->initial_train_cost += year[ExpensesType::TrainRun];
				fmt::print("BALANCE_INITIAL mode={} cursor={} earth={} rock={} partial={} money={} train_cost={}\n", this->balance,
					job->cursor, job->completed_earth_t, job->completed_rock_t, job->partial_work_ticks, company->money.base(), this->initial_train_cost.base());
				this->last_cut = job->cursor;
				this->last_earth = job->completed_earth_t;
				this->last_rock = job->completed_rock_t;
				this->captured_running = !this->advice; // Only advice acceptance needs new actual controller-status captures.
				if (this->advice) {
					REQUIRE(Command<Commands::RenameCompany>::Do(DoCommandFlag::Execute, "Underground Mines").Succeeded()); // Apply the existing fixture-name repair to this unsaved demonstration, leaving the retained input byte-exact.
					ShowVehicleViewWindow(Train::Get(project.cutter));
					const auto *assignment = GetTBMExchangeAssignment(project.id);
					REQUIRE(assignment != nullptr);
					ShowVehicleViewWindow(Train::Get(assignment->hauler));
				}
				REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
				GfxClearSpriteCache();
				FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				this->SetParticipantsRunning(true);
				REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
				this->started = true;
				return;
			}
			fmt::print("FEEDBACK_SCALE gui={} screen=1280x733 normal_font={}\n", ScaleGUITrad(100), GetCharacterHeight(FontSize::Normal));
			fmt::print("FEEDBACK_WORLD company={} name_id={} name_seed={} custom_name={} status={} date={} cutter={} job={} route={} cursor={}\n",
				company->index.base(), company->name_1, company->name_2, company->name, GetString(STR_STATUSBAR_COMPANY_NAME, company->index),
				TimerGameCalendar::date.base(), project.cutter.base(), project.job.base(), job->route.size(), job->cursor);
			// Repair the malformed historical fixture's public company label using
			// the ordinary company command, then save a new playable copy. The
			// original input bytes and all simulation owners remain available.
			if (!this->reload_demo) REQUIRE(Command<Commands::RenameCompany>::Do(DoCommandFlag::Execute, "Underground Mines").Succeeded());
			REQUIRE(GetString(STR_STATUSBAR_COMPANY_NAME, company->index).find(this->final_output.empty() ? "Underground Mines" : company->name) != std::string::npos);
			// A load-verification run must never repair or rewrite the file under
			// verification; the separate haulage baseline also retains its owner.
			if (!this->long_haul && !this->reload_demo) {
				const auto repaired = GetTestCaseArtifactPaths("tbm", "planning").saves / "underground-demo-tbm-ui-393.sav";
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(repaired), false) == SaveLoadResult::Ok);
			}
			// The accepted geometry is the movement reference. Every segment, including its
			// real advancing bends and paid sweep, is retained before the simulation resumes.
			for (size_t i = 0; i < job->route.size(); ++i) {
				const auto &cut = job->route[i];
				fmt::print("FEEDBACK_PLAN segment={} tile={} x={} y={} heading={} sweep={}\n", i, cut.cutter_position.base(),
					TileX(cut.cutter_position), TileY(cut.cutter_position), to_underlying(cut.heading), cut.swept_tiles.size());
			}
			std::vector<size_t> ends;
			const auto path = GetTBMWorkRailPath(*job, job->route.size(), ends);
			for (TileIndex tile : path) fmt::print("FEEDBACK_SUPPORT tile={}\n", tile.base()); // Ordered real rail path distinguishes coverage revisits from a wrong turn.
			// Sample every authoritative rock cell, independent of the camera or texture;
			// the histogram describes gameplay hardness, not an aesthetic classification.
			for (MapLayer layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
				std::array<uint32_t, 4> hardness{};
				for (uint y = 1; y + 1 < Map::SizeY(); ++y) {
					for (uint x = 1; x + 1 < Map::SizeX(); ++x) ++hardness[to_underlying(GetExcavationCellHardness(GetGeologyWorldState(), TileXYL(x, y, layer)))];
				}
				fmt::print("FEEDBACK_GEOLOGY layer={} soft={} mixed={} hard={} basalt={}\n", to_underlying(layer), hardness[0], hardness[1], hardness[2], hardness[3]);
			}
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			SetViewportLayer(GetMainWindow(), GetTileLayer(job->commissioning_anchor));
			ScrollMainWindowToTile(job->commissioning_anchor, true);
			Window *palette = nullptr;
			/** The main toolbar opens its actual menu; select its actual first row using native mouse input. */
			this->Capture("toolbar", [&] {
				RelocateAllWindows(1280, 733);
				Window *toolbar = FindWindowById(WindowClass::MainToolbar, 0);
				REQUIRE(toolbar != nullptr);
				FeedbackClick(*toolbar, WID_TN_AIR);
			});
			CHECK(FindWindowById(WindowClass::MainToolbar, 0)->IsWidgetLowered(WID_TN_AIR));
			/** Menu selection dispatches the existing layer-aware construction-window owner. */
			this->Capture("palette", [&] {
				Window *menu = FindWindowById(WindowClass::DropdownMenu, 0);
				REQUIRE(menu != nullptr);
				const Point point{menu->left + menu->width / 2, menu->top + menu->height / 2};
				FeedbackMouse(point, true);
				FeedbackMouse(point, false);
				// Native dropdown input is deferred for four ordinary mouse-loop passes.
				// Pump input without another button press rather than calling its callback.
				for (uint i = 0; i < 5; ++i) TimerManager<TimerWindow>::Elapsed(std::chrono::milliseconds(30));
				palette = FindWindowById(WindowClass::UndergroundConstruction, to_underlying(GetTileLayer(job->commissioning_anchor)));
				REQUIRE(palette != nullptr);
				palette->left = 1280 - palette->width;
				palette->top = 50;
			});
			this->Prompt(*palette, "opened");
			/** Projects opens a real list with navigation to the actual train and accepted footprint. */
			this->Capture("projects-click", [&] { FeedbackClick(*palette, WID_UC_PROJECT); });
			this->Prompt(*palette, "projects-click");
			Window *browser = FindWindowById(WindowClass::TBMProjects, GetTileLayer(job->commissioning_anchor));
			REQUIRE(browser != nullptr);
			this->Capture("project-machine", [&] { FeedbackClick(*browser, WID_TP_VEHICLE); });
			REQUIRE(FindWindowById(WindowClass::VehicleView, project.cutter) != nullptr);
			CloseWindowByClass(WindowClass::VehicleView);
			this->Capture("project-selected", [&] { FeedbackClick(*browser, WID_TP_SELECT); });
			REQUIRE(_thd.GetCallbackWnd() == palette);
			CHECK(GetConstructionPreviewTile(job->authorized_tiles.front()) != ConstructionPreviewTile::None);
			if (this->extend_work) {
				if (this->reload_accepted) {
					REQUIRE(project.blocked == TBMBlockedReason::Paused);
					if (this->fresh_layer == MapLayer::Surface) REQUIRE(job->route.size() == (this->straight_tunnel ? 26 : 33));
					this->Capture("resumed", [&] {
						FeedbackClick(*browser, WID_TP_CONTROLS);
						Window *portal = FindWindowById(WindowClass::LayerPortal, project.portal);
						REQUIRE(portal != nullptr);
						FeedbackClick(*portal, PW_PAUSE);
						FeedbackClick(*portal, PW_SHOW_MACHINE);
					});
					CHECK(GetTBMProject(project.id)->blocked != TBMBlockedReason::Paused);
				} else {
					this->ExtendThroughUI(*palette, *browser);
				}
				REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
				GfxClearSpriteCache();
				FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				this->SetParticipantsRunning(true);
				if (!this->final_output.empty()) {
					const auto *assignment = GetTBMExchangeAssignment(project.id);
					REQUIRE(assignment != nullptr);
					ShowVehicleViewWindow(Train::Get(assignment->hauler));
					const auto ready = GetTestCaseArtifactPaths("tbm", "planning").saves / (this->EvidencePrefix() + "ready.sav");
					REQUIRE_FALSE(std::filesystem::exists(ready));
					REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(ready), false) == SaveLoadResult::Ok);
				}
				REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
				this->started = true;
				return;
			}
			browser->Close();
			Window::DeleteClosedWindows();
			this->Capture("cancelled", [&] { HandleKeypress(WKC_ESC, 0); });
			REQUIRE(_thd.GetCallbackWnd() != palette);
			CHECK(GetConstructionPreviewTile(job->authorized_tiles.front()) == ConstructionPreviewTile::None);
			this->PresentationVariants(*palette);
			this->VerifyInputCancellation(*palette, job->commissioning_anchor);
			palette = ShowUndergroundConstructionToolbar(GetTileLayer(job->commissioning_anchor), GetCurrentMultiLayerProfileSelection());
			REQUIRE(palette != nullptr);
			palette->left = 1280 - palette->width;
			palette->top = 50;
			/** Area planning must visibly request a portal before accepting a floor drag. */
			this->Capture("area-click", [&] { FeedbackClick(*palette, WID_UC_AREA); });
			this->Prompt(*palette, "area-click");
			REQUIRE(_thd.GetCallbackWnd() == palette);
			CHECK(palette->IsWidgetLowered(WID_UC_AREA));
			/** Keep the map click outside the wide palette, using actual viewport coordinates. */
			this->Capture("area-portal", [&] {
				palette->left = 1280 - palette->width;
				ScrollMainWindowToTile(job->commissioning_anchor, true);
				UpdateViewportPosition(GetMainWindow(), 0);
				const Point tile = FeedbackTilePoint(job->commissioning_anchor);
				// Moving a window changes presentation only. Expose the actual portal under
				// the cursor even when the baseline window is wider than half the screen.
				palette->top = tile.y + 40;
				FeedbackMouse(tile, true);
				FeedbackMouse(tile, false);
			});
			this->Prompt(*palette, "area-portal");
			CHECK(palette->GetWidgetString(WID_UC_STATUS, STR_UNDERGROUND_SELECT_TOOL) == GetString(STR_UNDERGROUND_PORTAL_HAS_PROJECT));
			/** Drag from the portal's next planned cell to its last face through ordinary placement. */
			this->Capture("area-drag", [&] {
				const Point first = FeedbackTilePoint(job->route.front().cutter_position);
				const Point last = FeedbackTilePoint(job->route.back().cutter_position);
				FeedbackMouse(first, true);
				FeedbackMouse(last, true);
				FeedbackMouse(last, false);
			});
			this->Prompt(*palette, "area-drag");
			ResetObjectToPlace();
			/** The palette's ordinary Rail action opens the actual native construction toolbar. */
			this->Capture("rail-entry", [&] {
				CloseWindowByClass(WindowClass::LayerPortal);
				palette->top = 50;
				FeedbackClick(*palette, WID_UC_RAIL);
			});
			palette->Close();
			Window::DeleteClosedWindows();
			this->VerifyToolbarVariants();
			/** Adding a rail piece on protected work support must surface the normal command refusal. */
			this->Capture("protected-support", [&] {
				Window *rail = FindWindowById(WindowClass::BuildToolbar, TransportType::Rail);
				REQUIRE(rail != nullptr);
				FeedbackClick(*rail, WID_RAT_BUILD_X);
				ScrollMainWindowToTile(job->commissioning_anchor, true);
				UpdateViewportPosition(GetMainWindow(), 0); // Recompute actual tile picking after the toolbar capture changed screen size and scale.
				const Point point = FeedbackTilePoint(job->route.front().cutter_position);
				FeedbackMouse(point, true);
				FeedbackMouse(point, false);
				REQUIRE(FindWindowById(WindowClass::ErrorMessage, 0) != nullptr);
			});
			CloseWindowByClass(WindowClass::ErrorMessage);
			CloseWindowByClass(WindowClass::BuildToolbar);
			ResetObjectToPlace();
			/** Ordinary map input returns to the actual portal controls after the refused rail command. */
			this->Capture("portal-controls", [&] {
				const Point point = FeedbackTilePoint(job->commissioning_anchor);
				FeedbackMouse(point, true);
				FeedbackMouse(point, false);
			});
			/** The actual map click opens the portal; its Machine button opens the native vehicle. */
			this->Capture("machine", [&] {
				Window *portal = FindWindowById(WindowClass::LayerPortal, project.portal.base());
				REQUIRE(portal != nullptr);
				FeedbackClick(*portal, PW_SHOW_MACHINE);
			});
			/** Inspect the ordinary order editor reached from that same real TBM. */
			this->Capture("orders", [&] {
				Window *vehicle = FindWindowById(WindowClass::VehicleView, project.cutter.base());
				REQUIRE(vehicle != nullptr);
				FeedbackClick(*vehicle, WID_VV_SHOW_ORDERS);
			});
			const auto controls = GetTBMPairControls(project.portal, project.company);
			fmt::print("FEEDBACK_RETREAT phase={} cancel={} retire={} connect={} ordinary_orders={}\n", to_underlying(project.phase),
				controls.cancel, controls.retire, controls.connect, Train::Get(project.cutter)->GetNumOrders());
			REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			this->SetParticipantsRunning(!this->long_haul);
			if (this->long_haul) this->builder = std::make_unique<FeedbackHaulBuilder>(project);
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
			this->started = true;
		} catch (...) {
			this->failure = std::current_exception();
			// Null video continues its bounded loop after an exit request. Restore its
			// no-pixel renderer even on assertion failure before that next draw dispatch.
			BlitterFactory::SelectBlitter("null");
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			_exit_game = true;
		}
	}

	/**
	 * Record bounded physical/controller changes after normal synchronized simulation.
	 * @param elapsed Native frame count; never used to advance work manually.
	 */
	void Observe(uint elapsed)
	{
		if (!this->started || this->finished) return; // Loaded setup and finished measurement do not create further observer work.
		if (this->builder != nullptr) {
			if (!this->builder->Tick(elapsed)) return;
			const auto *project = GetTBMProject(this->project_id);
			AutoRestoreBackup actor(_current_company, project->company);
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
			const auto path = GetTestCaseArtifactPaths("tbm", "planning").saves / "tbm-feedback-haul50-393.sav";
			REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
			fmt::print("FEEDBACK_HAUL one_way_cells=50 paid_construction={} saved={}\n", this->builder->cost.base(), path.string());
			this->builder.reset();
			this->SetParticipantsRunning(true);
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
			return;
		}
		this->ticks += elapsed;
		const auto *project = GetTBMProject(this->project_id);
		if (project == nullptr) return;
		const auto *job = GetExcavationJob(project->job);
		// This observed cursor boundary separates the actual pre-existing work
		// from the added cuts; quoted spoil must agree with their real receipts.
		if (this->extend_work && !this->reload_accepted && job->cursor == this->original_route_size && !this->original_earth) {
			this->original_earth = job->completed_earth_t;
			this->original_rock = job->completed_rock_t;
		}
		const Train *front = Train::Get(project->cutter);
		if (!this->final_output.empty() && job->state == ExcavationJobState::Completed) {
			AutoRestoreBackup actor(_current_company, project->company);
			const TileIndex terminal = job->route.back().cutter_position;
			// The ordinary controller backs out after the last paid cut. Stop
			// locally only after its full body clears the future portal pocket;
			// native braking, occupancy and the handover owner decide readiness.
			if (DistanceManhattan(front->tile, terminal) >= 5 && !front->vehstatus.Test(VehState::Stopped)) {
				REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, front->index, true).Succeeded());
			}
			if (front->vehstatus.Test(VehState::Stopped) && front->cur_speed == 0) {
				REQUIRE(job->route.size() == 50);
				REQUIRE(GetTileLayer(front->tile) == MapLayer::Underground);
				REQUIRE(CheckTBMExitHandover(terminal, GetLayerPortalDoorDirection(job->commissioning_anchor), RAILTYPE_RAIL).Succeeded());
				REQUIRE(DistanceManhattan(front->tile, terminal) < 12);
				REQUIRE(this->original_earth.has_value());
				CHECK(job->completed_earth_t - *this->original_earth == this->quoted_extension_earth);
				CHECK(job->completed_rock_t - *this->original_rock == this->quoted_extension_rock);
				const auto *assignment = GetTBMExchangeAssignment(project->id);
				REQUIRE(assignment != nullptr);
				CHECK(assignment->exchanges >= 3);
				this->Finish();
				REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
				const auto stage = GetTestCaseArtifactPaths("tbm", "planning").saves / (this->EvidencePrefix() + "local-stage.sav");
				REQUIRE_FALSE(std::filesystem::exists(stage));
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(stage), false) == SaveLoadResult::Ok);
				fmt::print("FINAL_UI_STAGE cutter={} job={} cuts={} exchanges={} ticks={} earth={} rock={}\n", front->index.base(), job->id.base(), job->cursor,
					assignment->exchanges, this->ticks, job->completed_earth_t, job->completed_rock_t);
				this->finished = true;
				_exit_game = true;
				return;
			}
		}
		const auto target = GetTBMCutterSteeringTarget(*front);
		if (this->advice && !this->captured_hauler) {
			const auto *assignment = GetTBMExchangeAssignment(project->id);
			const Train *hauler = assignment == nullptr ? nullptr : Train::GetIfValid(assignment->hauler);
			// Capture an actual moving empty return. A stationary setup pose
			// cannot prove the vehicle window describes controller-driven travel.
			if (hauler != nullptr && assignment->phase == TBMHaulerPhase::ReturnEmpties && hauler->cur_speed != 0 && hauler->GetDisplaySpeed() > 0) {
				const auto status = GetTBMVehicleStatus(*hauler);
				REQUIRE(status.has_value());
				CHECK(status->find("bringing empty hoppers") != std::string::npos);
				REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
				GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				this->Capture("empty-return");
				REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
				GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
				this->captured_hauler = true;
			}
		}
		// Retain one real work/travel caption while the controller is running;
		// a final parked-state screenshot would conceal misleading "No orders" text.
		if (!this->captured_running && project->phase == TBMPhase::Bore) {
			this->captured_running = true;
			REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			Window *vehicle = FindWindowById(WindowClass::VehicleView, project->cutter.base());
			REQUIRE(vehicle != nullptr);
			fmt::print("FEEDBACK_RUNNING vehicle={} phase={} capture=running\n", project->cutter.base(), to_underlying(project->phase));
			this->Capture("running");
			REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		}
		this->phase_frames[to_underlying(project->phase)] += elapsed;
		this->blocked_frames[to_underlying(project->blocked)] += elapsed;
		// Observe complete transactions at their sole geological cursor. Idle travel and
		// exchange frames never inflate the active-time comparison for the half-rate fix.
		if (job->cursor != this->last_cut) {
			if (this->last_cut != UINT32_MAX) fmt::print("FEEDBACK_CUT cursor={} active_ticks={} earth={} rock={}\n", job->cursor,
				this->boring_ticks - this->cut_boring_start, job->completed_earth_t - this->last_earth, job->completed_rock_t - this->last_rock);
			this->last_cut = job->cursor;
			this->cut_boring_start = this->boring_ticks;
			this->last_earth = job->completed_earth_t;
			this->last_rock = job->completed_rock_t;
		}
		// A held/blocked phase is excluded from cutting-rate evidence: hauling and
		// waiting must not be misreported as geological work speed.
		if (project->phase == TBMPhase::Bore && project->blocked == TBMBlockedReason::None) this->boring_ticks += elapsed;
		std::string row = fmt::format("phase={} block={} cursor={} head={} target={} dirs={} order={} partial={} earth={} rock={}",
			to_underlying(project->phase), to_underlying(project->blocked), job->cursor, to_underlying(project->heading),
			target ? target->tile.base() : UINT32_MAX, target ? target->trackdirs.base() : 0, to_underlying(front->current_order.GetType()),
			job->partial_work_ticks, job->completed_earth_t, job->completed_rock_t);
		// Every physical part and both real trains are included at tile/heading/cargo
		// transitions, so a correct head cannot conceal a wandering or occupied tail.
		for (const Train *train : Train::Iterate()) {
			row += fmt::format(" part={},{},{},{},{},{},{},{}", train->index.base(), train->First()->index.base(), train->tile.base(),
				to_underlying(train->direction), train->track.base(), train->cargo.TotalCount(), train->cargo_cap, train->current_order.GetType());
			row += fmt::format(" pos={},{},{} reserved={}", train->x_pos, train->y_pos, train->z_pos, GetReservedTrackbits(train->tile).base());
		}
		for (const auto &assignment : CaptureTBMExchangeAssignments()) {
			row += fmt::format(" haul={},{},{},{}", assignment.hauler.base(), to_underlying(assignment.phase), to_underlying(assignment.wait), assignment.exchanges);
		}
		if (!this->final_output.empty()) REQUIRE(this->ticks < 160000);
		if (this->final_output.empty() && this->ticks == (this->extend_work ? 40000 : 20000)) {
			if (this->extend_work) {
				CHECK(job->cursor == job->route.size());
				CHECK(IsTBMProjectFinished(project->phase));
				if (!this->reload_accepted) {
					REQUIRE(this->original_earth.has_value());
					CHECK(job->completed_earth_t - *this->original_earth == this->quoted_extension_earth);
					CHECK(job->completed_rock_t - *this->original_rock == this->quoted_extension_rock);
				}
			}
			// The runner compares uninterrupted and real file-reload results at
			// the same native tick, including complete part positions/cargo and
			// every affected owner digest, cash and synchronized random state.
			fmt::print("FEEDBACK_RESULT geology={} projects={} exchange={} receivers={} money={} rng={},{} {}\n",
				ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(), ComputeTBMExchangeDigest(), ComputeSpoilReceiverDigest(),
				Company::Get(project->company)->money.base(), _random.state[0], _random.state[1], row);
			this->Finish();
			this->finished = true;
			AutoRestoreBackup actor(_current_company, project->company);
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
		}
		if (row == this->previous) return;
		this->previous = row;
		fmt::print("FEEDBACK_TRACE tick={} boring={} {}\n", this->ticks, this->boring_ticks, row);
	}

	/** Capture final native news and work/hold measurements without injecting warnings or work. */
	void Finish() const
	{
		uint false_order_warnings = 0;
		if (!this->balance.empty()) {
			const auto *project = GetTBMProject(this->project_id);
			const auto *job = GetExcavationJob(project->job);
			const auto *company = Company::Get(project->company);
			Money total_train_cost = 0;
			for (const auto &year : company->yearly_expenses) total_train_cost += year[ExpensesType::TrainRun]; // Preserve the same cumulative cost across an annual ledger rotation.
			fmt::print("BALANCE_FINAL mode={} cursor={} earth={} rock={} partial={} money={} train_upkeep={}\n", this->balance,
				job->cursor, job->completed_earth_t, job->completed_rock_t, job->partial_work_ticks, company->money.base(),
				(total_train_cost - this->initial_train_cost).base());
		}
		// Read labels before application teardown destroys actual native vehicles;
		// the saved reference distinguishes hauler advice from cutter advice.
		for (const auto &news : GetNews()) {
			if (news.GetStatusText().find("too few orders") != std::string::npos) ++false_order_warnings;
			const auto *vehicle = std::get_if<VehicleID>(&news.ref1);
			fmt::print("FEEDBACK_NEWS vehicle={} name={} text={}\n", vehicle == nullptr ? UINT32_MAX : vehicle->base(),
				vehicle == nullptr ? "none" : GetString(STR_VEHICLE_NAME, *vehicle), news.GetStatusText());
		}
		if (this->advice) {
			CHECK(false_order_warnings == 0);
			CHECK(this->captured_running);
			CHECK(this->captured_hauler);
			fmt::print("ADVICE_RESULT ticks={} too_few={} cutter_capture={} hauler_capture={}\n", this->ticks, false_order_warnings, this->captured_running, this->captured_hauler);
		}
		for (size_t i = 0; i < this->phase_frames.size(); ++i) fmt::print("FEEDBACK_PHASE phase={} ticks={}\n", i, this->phase_frames[i]); // Separate each work/travel/wait owner in balance evidence.
		for (size_t i = 0; i < this->blocked_frames.size(); ++i) {
			if (this->blocked_frames[i] != 0) fmt::print("FEEDBACK_BLOCK reason={} ticks={}\n", i, this->blocked_frames[i]);
		}
	}
};

static TBMFeedbackProbe *_feedback_probe = nullptr; ///< Process-local observer, absent from production executables.

/**
 * Run the probe only after the ordinary startup loader reconstructed the demo.
 * @param arguments Single registered console command, without gameplay parameters.
 * @return True after dispatching the isolated observer.
 */
static bool StartTBMFeedback(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1);
	REQUIRE(_feedback_probe != nullptr);
	_feedback_probe->Start();
	return true;
}

/** Preserve the loaded native demo while exercising real input and observing a bounded normal simulation. */
TEST_CASE("TBM feedback reproduces native input and loaded movement", "[.tbm-feedback]")
{
	TBMFeedbackProbe probe;
	probe.root = FindTestRepositoryRoot();
	probe.long_haul = std::getenv("OTTD_TBM_FEEDBACK_LONG_HAUL") != nullptr;
	probe.reload_demo = std::getenv("OTTD_TBM_FEEDBACK_RELOAD") != nullptr;
	probe.extend_work = std::getenv("OTTD_TBM_FEEDBACK_EXTEND") != nullptr;
	probe.straight_tunnel = std::getenv("OTTD_TBM_FEEDBACK_TUNNEL") != nullptr;
	probe.reload_accepted = std::getenv("OTTD_TBM_FEEDBACK_ACCEPTED") != nullptr;
	probe.advice = std::getenv("OTTD_TBM_FEEDBACK_ADVICE") != nullptr;
	const char *final_source = std::getenv("OTTD_TBM_FINAL_SOURCE");
	if (final_source != nullptr) {
		const char *output = std::getenv("OTTD_TBM_FINAL_OUTPUT");
		REQUIRE(output != nullptr);
		probe.final_output = output;
		REQUIRE(std::filesystem::path(output).filename().string() == probe.final_output);
		probe.reload_demo = probe.extend_work = probe.straight_tunnel = probe.advice = true;
	}
	if (const char *balance = std::getenv("OTTD_TBM_FEEDBACK_BALANCE"); balance != nullptr) {
		probe.balance = balance;
		REQUIRE((probe.balance == "haul50" || probe.balance == "old-work")); // Select only retained repository fixtures, never an arbitrary input/output path.
	}
	const char *fresh = std::getenv("OTTD_TBM_FEEDBACK_FRESH");
	if (fresh != nullptr) {
		REQUIRE((std::string_view{fresh} == "deep" || std::string_view{fresh} == "underground"));
		probe.fresh_layer = std::string_view{fresh} == "deep" ? MapLayer::DeepUnderground : MapLayer::Underground;
	}
	AutoRestoreBackup observer(_feedback_probe, &probe);
	IConsole::CmdRegister("tbm_feedback", StartTBMFeedback);
	/** @param elapsed Real frames after loader startup; only observation is performed. */
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&probe](uint elapsed) {
		if (probe.failure) return;
		try {
			probe.Observe(elapsed);
		} catch (...) {
			probe.failure = std::current_exception();
			BlitterFactory::SelectBlitter("null"); // The null driver's remaining bounded frames must never draw through a released capture canvas.
			GfxClearSpriteCache(); FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		}
	});
	const std::string source_name = !probe.balance.empty() ? (probe.balance == "haul50" ? "tbm-feedback-haul50-393.sav" : "tbm-motion-wait-393.sav") : probe.reload_accepted ? probe.AcceptedFilename() : probe.fresh_layer != MapLayer::Surface ?
		fmt::format("tbm-planning-{}-393.sav", fresh) : probe.reload_demo ? "underground-demo-tbm-ui-393.sav" : "underground-demo-390.sav";
	const auto runtime_config = GetTestRuntimeConfig().string();
	const char *accepted_source = std::getenv("OTTD_TBM_FEEDBACK_ACCEPTED_SOURCE");
	// A continued feedback run loads its explicitly selected predecessor's save.
	// The fixture fallback remains for direct legacy compatibility observations.
	const auto selected = accepted_source == nullptr ? probe.root / "src/tests/fixtures/haulage" / source_name : std::filesystem::path{accepted_source};
	const auto source = final_source == nullptr ? ResolveTestInputPath(selected).string() : std::string(final_source);
	std::array<std::string_view, 19> arguments = {"RelWithDebInfo/openttd_test.exe", "-v", final_source != nullptr ? "null:ticks=160002" : probe.extend_work ? "null:ticks=40002" : probe.long_haul ? "null:ticks=40000" : "null:ticks=20002", "-b", "8bpp-simple", "-I", "original_dos",
		"-s", "null", "-m", "null", "-x", "-X", "-c", runtime_config, "-g", source, "-r", "1280x733"};
	CHECK(openttd_main(arguments) == 0);
	if (probe.failure) std::rethrow_exception(probe.failure);
	REQUIRE(probe.started);
	REQUIRE(probe.finished);
	fmt::print("FEEDBACK_COMPLETE ticks={} boring_ticks={}\n", probe.ticks, probe.boring_ticks);
}
