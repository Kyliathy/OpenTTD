/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file demo_feedback_runtime.cpp Native reproduction of the 2026-10-15 demo reports on the real loaded demo.
 *
 * The observer loads the user's demo world through the ordinary loader, unpauses it and lets the normal simulation
 * run. It only reads state and feeds real input; it never moves vehicles, edits cargo or changes owners itself.
 * Each scenario reproduces one screenshot of the report and logs the facts that confirm or refute its diagnosis.
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Groups 1-3 (Group 3: the "windows" scenario)
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 6 ("hauler-depot": the loaded hauler leaves; checkpoints and their replay)
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 6B ("unload-order": station stops under "Unload all")
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md task 9.7 ("approach": the beside-exit area, bought and dug)
 * and task 5.5 (the "vehicle-spacing" scenario)
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md task 12.6 ("change": the work area changed while
 * boring and while parked, saved just after the change and reloaded)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 8 ("face-loop": the hauler meets the cutter at
 * its face; per-transfer evidence, camera modes for the view equivalence, and the cut rate)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md task 10.4 ("hauler-depot" captures screenshot 03's
 * view after the fix: the loaded hauler leaving its depot)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 9 (face-loop checkpoints and the player's
 * stop, park and release saves; each logs its next transition for the reload in tbm_face_exchange_save.cpp)
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/architecture/pathfinder/archi-pathfinder-rail.md
 */

#include "../stdafx.h"
#include "test_artifact_paths.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../openttd.h"
#include "../console_internal.h"
#include "../console_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../misc_cmd.h"
#include "../vehicle_cmd.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../order_base.h"
#include "../order_cmd.h"
#include "../vehicle_func.h"
#include "../tbm.h"
#include "../tbm_exchange.h"
#include "../tbm_cmd.h"
#include "../tbm_gui.h"
#include "../tbm_area_preview.h"
#include "../tbm_replanning.h"
#include "../tilehighlight_func.h"
#include "../widgets/tbm_widget.h"
#include "../underground_gui.h"
#include "../vehicle_gui.h"
#include "../depot_func.h"
#include "../depot_base.h"
#include "../viewport_func.h"
#include "../multilayer_content.h"
#include "../multilayer_sprite.h"
#include "../gfx_func.h"
#include "../widgets/underground_widget.h"
#include "../smallmap_gui.h"
#include "../widgets/smallmap_widget.h"
#include "../widgets/settings_widget.h"
#include "../querystring_gui.h"
#include "../window_func.h"
#include "../excavation.h"
#include "../layer_portal_map.h"
#include "../layer_portal_cmd.h"
#include "../map_layer_gui.h"
#include "../rail_map.h"
#include "../pbs.h"
#include "../signal_func.h"
#include "../station_base.h"
#include "../cargotype.h"
#include "../strings_func.h"
#include "../blitter/factory.hpp"
#include "../fontcache.h"
#include "../spritecache.h"
#include "../timer/timer.h"
#include "../timer/timer_game_tick.h"
#include "../timer/timer_game_calendar.h"
#include "../timer/timer_game_realtime.h"
#include "../saveload/saveload_func.h"
#include "../core/random_func.hpp"
#include "../table/strings.h"
#include "../core/string_consumer.hpp"
#include "native_art_render_fixture.h"
#include "native_input_fixture.h"
#include "tbm_face_native.h"
#include <cstdlib>
#include <chrono>
#include <set>

#include "../safeguards.h"

/** Artifact owner of every demo-fix reproduction; scenarios are selected by the runner. */
static constexpr std::string_view DEMO_FEATURE = "demo-fixes";

/** Ticks logged after a checkpoint is saved; a replay run of that checkpoint must log the same (task 6.7). */
static constexpr uint REPLAY_TICKS = 400;

/** Window over which the face loop's cut rate is compared with the old bay loop (r2df2 task 8.5). */
static constexpr uint FACE_RATE_TICKS = 20000;

/** Spacing of the face loop's state rows, which the view-equivalence comparison reads (r2df2 task 8.3). */
static constexpr uint FACE_STATE_TICKS = 1000;

/** Progress of the Deep face-loop journey (r2df2 task 8.2). */
enum class FaceBendStage : uint8_t {
	Setup, ///< The loaded project is resumed and its stopped trains started, as a player does.
	Boring, ///< Waiting for the first cut; then the work is changed into a tunnel at 45 degrees.
	Bent, ///< The bent route is accepted; the face loop runs on it, and the diagonal is extended near its end.
};

/** Extensions of the Deep run's diagonal; one change reaches only a few tiles from the machine (task 8.2). */
static constexpr uint FACE_BEND_EXTENSIONS = 4;

/**
 * Describe one train part's physical placement for the trace.
 * @param part Any actual train part.
 * @return "tile(x,y,layer) track" text that distinguishes depot, magazine and running track.
 */
static std::string DescribePlacement(const Train &part)
{
	return fmt::format("{}({},{},{}) track={}", part.tile.base(), TileX(part.tile), TilePlaneY(part.tile), to_underlying(GetTileLayer(part.tile)), part.track.base());
}

/** Side length of the square scratch raster used to measure one vehicle pose. */
static constexpr int POSE_CANVAS = 512;

/**
 * Draw sprites exactly as a viewport does into a square scratch raster centred on the world origin.
 * @param zoom Viewport zoom level used for drawing.
 * @param sprites Sprites with their virtual (viewport) anchor offsets from the raster centre, drawn in order.
 * @return Palette-index raster of POSE_CANVAS x POSE_CANVAS pixels; zero is transparent.
 */
static std::vector<uint8_t> RenderPoses(ZoomLevel zoom, std::span<const std::pair<SpriteID, Point>> sprites)
{
	std::vector<uint8_t> pixels(static_cast<size_t>(POSE_CANVAS) * POSE_CANVAS, 0);
	const int half = ScaleByZoom(POSE_CANVAS / 2, zoom);
	DrawPixelInfo dpi{pixels.data(), -half, -half, ScaleByZoom(POSE_CANVAS, zoom), ScaleByZoom(POSE_CANVAS, zoom), POSE_CANVAS, zoom};
	AutoRestoreBackup screen(_screen, DrawPixelInfo{pixels.data(), 0, 0, POSE_CANVAS, POSE_CANVAS, POSE_CANVAS, ZoomLevel::Min});
	AutoRestoreBackup drawing(_cur_dpi, &dpi);
	AutoRestoreBackup animation(_screen_disable_anim, true);
	for (const auto &[sprite, offset] : sprites) DrawSpriteViewport(sprite, PAL_NONE, offset.x, offset.y);
	return pixels;
}

/**
 * Measure the opaque extent of a rendered raster.
 * @param pixels Raster from RenderPoses.
 * @return Width and height in screen pixels of the smallest box holding every drawn pixel; zero when empty.
 */
static Dimension MeasureOpaque(const std::vector<uint8_t> &pixels)
{
	int left = POSE_CANVAS, right = -1, top = POSE_CANVAS, bottom = -1;
	for (int y = 0; y < POSE_CANVAS; ++y) {
		for (int x = 0; x < POSE_CANVAS; ++x) {
			if (pixels[static_cast<size_t>(y) * POSE_CANVAS + x] == 0) continue;
			left = std::min(left, x);
			right = std::max(right, x);
			top = std::min(top, y);
			bottom = std::max(bottom, y);
		}
	}
	if (right < 0) return {0, 0};
	return {static_cast<uint>(right - left + 1), static_cast<uint>(bottom - top + 1)};
}

/**
 * World step of one movement unit per train direction (N, NE, E, SE, S, SW, W, NW), as GetNewVehiclePos uses it.
 * Consecutive train parts keep their spacing in these steps, so a part trails its leader by its length in steps.
 */
static constexpr std::array<std::pair<int, int>, 8> TRAIN_STEP = {{{-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}}};

/** Safe pose envelope (width, height) per direction at normal zoom, mirroring ENVELOPES in media/multilayer/convert_vehicle_art.py. */
static constexpr std::array<std::pair<uint, uint>, 8> POSE_ENVELOPE = {{{16, 24}, {24, 24}, {32, 24}, {24, 24}, {16, 24}, {24, 24}, {32, 24}, {24, 24}}};

/** One native vehicle family measured by the pose scenario. */
struct PoseFamily {
	std::string_view name; ///< Report name of the family.
	MultiLayerSpriteAsset asset; ///< Published sprite family.
	uint8_t state; ///< Moving or loaded state used when the family is drawn in a consist.
	uint8_t part; ///< Body (0) or permanent support (1) part.
};

/** The families of the demo consists, in the states a moving consist shows them. */
static constexpr std::array<PoseFamily, 7> POSE_FAMILIES = {{
	{"cutter", MultiLayerSpriteAsset::VehicleModel00, 1, 0},
	{"cutter-support", MultiLayerSpriteAsset::VehicleModel00, 1, 1},
	{"deep-cutter", MultiLayerSpriteAsset::VehicleModel01, 1, 0},
	{"deep-cutter-support", MultiLayerSpriteAsset::VehicleModel01, 1, 1},
	{"earth-hopper", MultiLayerSpriteAsset::VehicleModel02, 2, 0},
	{"rock-hopper", MultiLayerSpriteAsset::VehicleModel03, 2, 0},
	{"hauler", MultiLayerSpriteAsset::VehicleModel04, 1, 0},
}};

/** Progress of the task 1.3 journey from a working project to a released portal and a new request. */
enum class AreaStage : uint8_t {
	None, ///< The journey has not started.
	Parking, ///< Return and park was pressed; waiting for the parked machine.
	Releasing, ///< Finish/cancel and release was pressed; waiting for the project to end.
	Released, ///< The beside-exit request was quoted on the released portal.
};

/**
 * Click a widget as a player does after moving the mouse there: the move dismisses any tooltip left by earlier
 * hovering, which would otherwise sit on top of the target and swallow the click.
 * @param window Existing owner of the widget.
 * @param widget Widget to click through the native dispatcher.
 */
static void PlayerClick(Window &window, WidgetID widget)
{
	CloseWindowByClass(WindowClass::ToolTips);
	FeedbackClick(window, widget);
}

/** Progress of the task 12.6 journey: change the work area while boring, then while parked. */
enum class ChangeStage : uint8_t {
	None, ///< Waiting for the demo cutter to bore past its 22nd cut.
	Changed, ///< The work area was changed while boring; waiting for the next completed cut.
	Parking, ///< Return and park was pressed; waiting for the stored machine.
	Resumed, ///< The work area was changed while parked and the project resumed; waiting for its next completed cut.
};

/** Read-only observer of one loaded demo world; it feeds only real input and never edits simulation owners. */
class DemoFeedbackProbe {
public:
	std::string scenario; ///< Runner-selected reproduction, also the artifact scenario name.
	uint tick_limit = 6000; ///< Bounded number of normal simulation ticks to observe after unpausing.
	uint ticks = 0; ///< Normal simulation ticks observed since the unpause.
	bool started = false; ///< The loader finished and the observer issued its setup.
	bool finished = false; ///< The scenario retained its evidence; no more observation happens.
	std::exception_ptr failure; ///< First failure, rethrown only after native shutdown.
	std::map<VehicleID, std::string> previous_rows; ///< Last logged state of each front engine; avoids duplicate trace rows.
	std::map<VehicleID, std::string> previous_depot; ///< Last logged depot-exit decision of each front engine.
	std::map<std::pair<TileIndex, Trackdir>, SignalState> signals; ///< Every company signal and its last observed state.
	std::map<TileIndex, bool> portal_reservations; ///< Every company rail portal end and its last observed reservation bit.
	std::map<VehicleID, uint> full_decisions; ///< Depot-exit evaluations that found a Full segment, per front engine.
	std::string previous_cutter; ///< Last logged cutter travel-decision row.
	TBMProjectID project_id{UINT32_MAX}; ///< The loaded demo project the scenario follows.
	CompanyID company = CompanyID::Invalid(); ///< Owner of the demo project, the acting company of every input.
	TileIndex portal = INVALID_TILE; ///< Upper (key) end of the demo project's portal pair.
	TileIndex anchor = INVALID_TILE; ///< Rock-side housing of the pair, where the corridor starts.
	AreaStage area_stage = AreaStage::None; ///< Progress of the release and new-area journey.
	VehicleID former_hauler = VehicleID::Invalid(); ///< Hauler of project 0, an ordinary train after the release.
	uint forced_at = 0; ///< Tick at which the former hauler was forced out of its depot.
	uint hauler_stop_delay = 120; ///< Ticks after forcing at which the former hauler is stopped (OTTD_DEMO_HAULER_STOP).
	bool hauler_stopped = false; ///< The former hauler was stopped, as in the user's report.
	uint obstruction_ticks = 0; ///< Consecutive ticks in which project 1 reported an obstruction.
	uint area_cursor = 22; ///< Completed cuts of project 0 when the user asked for a new area (screenshot 04 shows 22 of 50).
	std::string release_wait; ///< Last logged release prerequisite while the parked project waits for its hauler.
	std::chrono::steady_clock::time_point started_at; ///< Wall-clock time of the unpause, for the achieved tick rate.
	uint captures = 0; ///< Captures taken so far; bounds the review output.
	std::set<VehicleID> overloaded; ///< Trains already reported with a part above its capacity.
	std::string save_request; ///< Checkpoint to save at the next point between two game ticks; empty when none is due.
	std::string replay_name; ///< Checkpoint whose following ticks are being logged (task 6.7).
	uint replay_left = 0; ///< Ticks still to log after that checkpoint was saved or loaded.
	uint replay_offset = 0; ///< Ticks logged since that checkpoint.
	uint replay_ticks = 0; ///< Replay run (OTTD_DEMO_REPLAY): log this many ticks of the loaded checkpoint, then stop.
	TileIndex hauler_depot = INVALID_TILE; ///< Depot the loaded hauler stood wholly inside, once it has.
	uint hauler_depot_at = 0; ///< Tick at which it stood wholly inside.
	bool departure_saved = false; ///< The loaded hauler's departure was saved.
	bool stored_saved = false; ///< The cutter consist stored in its magazine was saved.
	bool departure_captured = false; ///< The departed hauler was captured on its way (r2df2 task 10.4).
	std::map<VehicleID, bool> last_decision_full; ///< Whether the latest depot-exit evaluation of each front engine found a Full segment.
	bool hauler_loading = false; ///< The hauler stands loading or unloading at a station (task 6B.3).
	StationID stop_station = StationID::Invalid(); ///< Station of the hauler's current stop.
	uint stop_aboard = 0; ///< Spoil aboard the hauler when the stop began.
	uint stop_waiting = 0; ///< Spoil waiting at the station when the stop began.
	uint stops = 0; ///< Completed hauler stops.
	std::set<TileIndex> approach_tiles; ///< Approach cells of the confirmed beside-exit quote (task 9.7).
	uint approach_off_support = 0; ///< Part-ticks in which a part of the approach project's consist stood off its usable rail.
	bool approach_bent = false; ///< The first bend of the approach project was captured.
	ChangeStage change_stage = ChangeStage::None; ///< Progress of the change-work journey (task 12.6).
	uint32_t change_cursor = 0; ///< Completed cuts when the work area was last changed.
	ExcavationCutGeometry change_first; ///< First cut of the last new route: the next cut the machine must complete.
	bool change_next_checked = false; ///< The first cut after the change while boring was checked.
	std::string view = "headless"; ///< Face-loop camera (task 8.3): "headless" never moves or draws a view, "visible" follows the cutter on its layer, "hidden" watches the Surface above it.
	uint face_target = 3; ///< Face transfers the face loop must observe before it ends (OTTD_DEMO_FACE_TRANSFERS).
	std::optional<FaceLoopSample> face_before; ///< Face-loop state after the previous tick.
	uint face_transfers = 0; ///< Exchanges made where the cutter met its hauler at the face.
	uint bay_exchanges = 0; ///< Exchanges made where the cutter met its hauler in its bay.
	uint face_hauler_since = 0; ///< Tick at which the hauler last began waiting on its stand.
	TileIndex face_wait_tile = INVALID_TILE; ///< Cutter tile when its current face wait began.
	bool face_rate_logged = false; ///< The cut cursor after FACE_RATE_TICKS was logged (task 8.5).
	bool face_captured = false; ///< The hauler behind the cutter at the face was captured (task 8.4).
	uint face_next_state = 0; ///< Tick of the next state row.
	bool face_bend = false; ///< Deep run (task 8.2, OTTD_DEMO_FACE_BEND): resume, start and bend the loaded project's route first.
	FaceBendStage face_bend_stage = FaceBendStage::Setup; ///< Progress of that preparation.
	uint face_bent_transfers = 0; ///< Face transfers whose stand lay on a half-tile piece behind a bend.
	uint face_extensions = 0; ///< Times the bent diagonal was extended straight on.
	uint bay_with_stand = 0; ///< Bay exchanges made although a stand behind the cutter existed: a missed face meeting.
	std::set<std::string> face_saves; ///< Face-loop checkpoints still to save (task 9.4, OTTD_DEMO_FACE_SAVES): travelling, held, dwell, leaving, bay-return.
	std::string face_lifecycle; ///< Player action whose result is saved (task 9.5, OTTD_DEMO_FACE_LIFECYCLE): stop, park or release; empty for none.
	bool face_lifecycle_acted = false; ///< The lifecycle action was taken.
	bool face_lifecycle_saved = false; ///< Its checkpoint was requested.
	bool face_allow_bay = false; ///< Bay exchanges are expected (OTTD_DEMO_FACE_ALLOW_BAY): a route starting at its portal.
	std::string face_watch; ///< Checkpoint whose next transition is awaited; empty when none.
	uint face_watch_from = 0; ///< Tick at which that checkpoint was saved.
	std::optional<FaceLoopSample> face_watch_prev; ///< State the watch compares the next tick with: the world as saved, then each watched tick.

	/**
	 * Retain the real rendered screen, including windows, at the size of the user's screenshots.
	 * @param name Stable review filename stem inside this scenario's images directory.
	 * @param input Optional real input to execute inside the valid screen buffer before drawing.
	 */
	void Capture(std::string_view name, const std::function<void()> &input = {})
	{
		++this->captures;
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		const auto path = GetTestCaseArtifactPaths(DEMO_FEATURE, this->scenario).images / fmt::format("{}.ppm", name);
		CaptureNativeArtCanvas(1280, 733, path, [&] {
			// Consecutive captures have no event-loop frame in between; flush scheduled
			// resizes so drawing and picking see each window's current rectangle.
			for (Window *window : Window::Iterate()) window->ProcessScheduledResize();
			if (input) input();
			for (Window *window : Window::Iterate()) window->ProcessScheduledResize();
			UpdateViewportPosition(GetMainWindow(), 0);
			DrawOverlappedWindowForAll(0, 0, 1280, 733);
		});
		// Simulation between captures runs without pixels, as the null video driver expects.
		REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		fmt::print("DEMO_CAPTURE tick={} name={}\n", this->ticks, name);
	}

	/** Log the world facts that every diagnosis depends on: settings, projects, haulage, trains, portals and rail. */
	void DumpWorld()
	{
		// Every pause reason is listed: 'unpause' clears only the normal one, and any other reason stops ticks too.
		fmt::print("DEMO_PAUSE modes={} timekeeping={} minutes_per_calendar_year={}\n", _pause_mode.base(), to_underlying(_settings_game.economy.timekeeping_units),
			_settings_game.economy.minutes_per_calendar_year);
		fmt::print("DEMO_WORLD scenario={} date={} map={}x{} layers={} reserve_paths={} forbid_90={} default_signal={} height_limit={} money={}\n", this->scenario,
			TimerGameCalendar::date.base(), Map::SizeX(), Map::SizeY(), Map::LayerCount(), _settings_game.pf.reserve_paths, _settings_game.pf.forbid_90_deg,
			to_underlying(_settings_client.gui.default_signal_type), _settings_game.construction.map_height_limit,
			Company::IsValidID(_local_company) ? Company::Get(_local_company)->money.base() : 0);
		for (const auto &project : CaptureTBMProjects()) {
			const ExcavationJob *job = GetExcavationJob(project.job);
			fmt::print("DEMO_PROJECT id={} company={} phase={} blocked={} cutter={} portal={} bay={} origin={} job={} state={} cursor={} route={} anchor={}\n",
				project.id.base(), project.company.base(), to_underlying(project.phase), to_underlying(project.blocked), project.cutter.base(),
				project.portal.base(), project.exchange_bay.base(), project.origin.base(), project.job.base(),
				job == nullptr ? -1 : static_cast<int>(job->state), job == nullptr ? 0 : job->cursor, job == nullptr ? 0 : job->route.size(),
				job == nullptr ? 0 : job->commissioning_anchor.base());
		}
		for (const auto &assignment : CaptureTBMExchangeAssignments()) {
			fmt::print("DEMO_ASSIGN id={} project={} hauler={} bay={} set={} phase={} wait={} exchanges={}\n", assignment.id.base(), assignment.project.base(),
				assignment.hauler.base(), assignment.bay.base(), assignment.set.base(), to_underlying(assignment.phase), to_underlying(assignment.wait), assignment.exchanges);
		}
		for (const auto &set : CaptureTBMWagonSets()) {
			std::string wagons;
			for (VehicleID wagon : set.wagons) wagons += fmt::format("{},", wagon.base());
			fmt::print("DEMO_SET id={} project={} state={} visited_origin={} wagons={}\n", set.id.base(), set.project.base(), to_underlying(set.state), set.visited_origin, wagons);
		}
		for (const Train *front : Train::Iterate()) {
			if (!front->IsFrontEngine()) continue;
			std::string parts;
			for (const Train *part = front; part != nullptr; part = part->Next()) {
				parts += fmt::format(" part={}:{}:cargo={}/{}:{}", part->index.base(), DescribePlacement(*part), part->cargo.TotalCount(), part->cargo_cap,
					part->cargo_type == INVALID_CARGO ? "none" : GetString(CargoSpec::Get(part->cargo_type)->name));
			}
			fmt::print("DEMO_TRAIN id={} unit={} name={} orders={} order={} dest={}{}\n", front->index.base(), front->unitnumber, GetString(STR_VEHICLE_NAME, front->index),
				front->GetNumOrders(), to_underlying(front->current_order.GetType()), front->dest_tile.base(), parts);
			this->LogCargo("dump", *front);
			// Unload and transfer orders send refused spoil down a different cargo path, so each order's flags are recorded.
			if (front->orders != nullptr) {
				for (const Order &order : front->orders->GetOrders()) {
					fmt::print("DEMO_ORDER train={} type={} dest={} unload={} load={}\n", front->index.base(), to_underlying(order.GetType()), order.GetDestination().value,
						to_underlying(order.GetUnloadType()), to_underlying(order.GetLoadType()));
				}
			}
		}
		// Portals, depots and signals are few in the demo; one full scan records the whole network that segments cross.
		for (const TileIndex tile : Map::Iterate()) {
			if (IsLayerPortalTile(tile) && GetLayerPortalTransportType(tile) == TransportType::Rail) {
				this->portal_reservations.try_emplace(tile, HasLayerPortalReservation(tile));
				fmt::print("DEMO_PORTAL tile={}({},{},{}) pair={} shaft={} door={} work={} reserved={} owner={}\n", tile.base(), TileX(tile), TilePlaneY(tile),
					to_underlying(GetTileLayer(tile)), GetLayerPortalPairTile(tile).base(), to_underlying(GetLayerPortalShaftDirection(tile)),
					to_underlying(GetLayerPortalDoorDirection(tile)), to_underlying(GetLayerPortalWorkState(tile)), HasLayerPortalReservation(tile), GetTileOwner(tile).base());
			}
			if (!IsTileType(tile, TileType::Railway)) continue;
			if (IsRailDepotTile(tile)) {
				fmt::print("DEMO_DEPOT_TILE tile={}({},{},{}) dir={} reserved={}\n", tile.base(), TileX(tile), TilePlaneY(tile), to_underlying(GetTileLayer(tile)),
					to_underlying(GetRailDepotDirection(tile)), HasDepotReservation(tile));
				continue;
			}
			fmt::print("DEMO_RAIL tile={}({},{},{}) tracks={} reserved={} signals={}\n", tile.base(), TileX(tile), TilePlaneY(tile), to_underlying(GetTileLayer(tile)),
				GetTrackBits(tile).base(), GetRailReservationTrackBits(tile).base(), HasSignals(tile));
			if (!HasSignals(tile)) continue;
			// Record each signal direction so later state changes can be attributed to a block boundary.
			for (Trackdir trackdir : EnumRange(Trackdir::End)) {
				if (!IsValidTrackdir(trackdir) || !HasTrack(tile, TrackdirToTrack(trackdir)) || !HasSignalOnTrackdir(tile, trackdir)) continue;
				const SignalState state = GetSignalStateByTrackdir(tile, trackdir);
				this->signals[{tile, trackdir}] = state;
				fmt::print("DEMO_SIGNAL tile={}({},{}) trackdir={} type={} state={}\n", tile.base(), TileX(tile), TilePlaneY(tile), to_underlying(trackdir),
					to_underlying(GetSignalType(tile, TrackdirToTrack(trackdir))), to_underlying(state));
			}
		}
	}

	/**
	 * Log one front engine's movement and controller state when it changes.
	 * @param front Actual front engine of any train.
	 */
	void TraceTrain(const Train &front)
	{
		std::string row = fmt::format("speed={} stopped={} stuck={} reversing={} force={} wait={} order={} dest={} head={}", front.cur_speed,
			front.vehstatus.Test(VehState::Stopped), front.flags.Test(VehicleRailFlag::Stuck), front.flags.Test(VehicleRailFlag::Reversing),
			front.force_proceed, front.wait_counter >= 37 ? 37 : front.wait_counter, to_underlying(front.current_order.GetType()), front.dest_tile.base(), DescribePlacement(front));
		const Train *last = front.Last();
		row += fmt::format(" tail={}", DescribePlacement(*last));
		uint cargo = 0;
		for (const Train *part = &front; part != nullptr; part = part->Next()) cargo += part->cargo.TotalCount();
		row += fmt::format(" cargo={}", cargo);
		const auto status = GetTBMVehicleStatus(front);
		if (status.has_value()) row += fmt::format(" status=\"{}\"", *status);
		// Speed, the shown mph figure and the depot wait counter change on almost every tick; the change key keeps
		// only placement, orders, flags, cargo and the status wording, so only real transitions print.
		std::string key = fmt::format("{} {} {} {} {} {} {} {}", front.vehstatus.Test(VehState::Stopped), front.flags.Test(VehicleRailFlag::Stuck),
			front.flags.Test(VehicleRailFlag::Reversing), front.force_proceed, to_underlying(front.current_order.GetType()), front.dest_tile.base(),
			row.substr(row.find(" head=")), front.cur_speed == 0);
		if (status.has_value()) key.erase(key.find(" status=\""), key.find(" - ", key.find(" status=\"")) - key.find(" status=\""));
		if (this->previous_rows[front.index] == key) return;
		this->previous_rows[front.index] = key;
		fmt::print("DEMO_TRACE tick={} train={} {}\n", this->ticks, front.index.base(), row);
	}

	/**
	 * Record the exact input of the native depot-exit decision for a train wholly inside an ordinary rail depot.
	 * CheckTrainStayInDepot evaluates the depot's signal segment every 37 ticks and resets its counter to zero; this
	 * runs right after such an evaluation, when re-evaluating the same segment cannot change any signal.
	 * @param front Actual front engine.
	 */
	void TraceDepotDecision(const Train &front)
	{
		// Only a train whose every part stands in the same ordinary depot is held by the depot rule;
		// portal magazines use the controller's own rule and are traced through the train row instead.
		for (const Train *part = &front; part != nullptr; part = part->Next()) {
			if (part->track != Track::Depot || part->tile != front.tile) {
				// Leaving or entering ends the stay; the next stay counts its own refusals.
				this->full_decisions[front.index] = 0;
				return;
			}
		}
		if (!IsRailDepotTile(front.tile) || front.wait_counter != 0 || front.force_proceed != TFP_NONE) return;
		const SigSegState state = _settings_game.pf.reserve_paths ? SigSegState::Path : UpdateSignalsOnSegment(front.tile, DiagDirection::Invalid, front.owner);
		if (state == SigSegState::Full) ++this->full_decisions[front.index];
		this->last_decision_full[front.index] = state == SigSegState::Full;
		std::string decision = fmt::format("segment={} depot_reserved={}", to_underlying(state), HasDepotReservation(front.tile));
		if (this->previous_depot[front.index] == decision) return;
		this->previous_depot[front.index] = decision;
		// A Full segment needs a train inside it: list every other train that stands outside a depot, by layer,
		// so the occupant that keeps the depot's block red can be named.
		for (const Train *other : Train::Iterate()) {
			if (!other->IsFrontEngine() || other == &front) continue;
			for (const Train *part = other; part != nullptr; part = part->Next()) {
				if (part->track == Track::Depot) continue;
				decision += fmt::format(" occupant={}@{}", other->index.base(), DescribePlacement(*part));
				break;
			}
		}
		fmt::print("DEMO_DEPOT_DECISION tick={} train={} {}\n", this->ticks, front.index.base(), decision);
	}

	/**
	 * Log the inputs of the cutter's travel decision when they change: the hold, the Stuck flag, its steering target
	 * and, for both ends of its portal pair, what the cutter is admitted to, the reservation bit and who stands there.
	 * These are the facts that separate R9's candidates (a) target trackdir, (b) hidden key, (c) stale reservation.
	 * @param project The demo project whose cutter is traced.
	 */
	void TraceCutter(const TBMProject &project)
	{
		const Train *cutter = Train::GetIfValid(project.cutter);
		if (cutter == nullptr || !IsLayerPortalTile(project.portal)) return;
		const auto target = GetTBMCutterSteeringTarget(*cutter);
		std::string row = fmt::format("phase={} blocked={} travel_hold={} stuck={} holding={} target={} target_dirs={}", to_underlying(project.phase),
			to_underlying(project.blocked), to_underlying(GetTBMTravelHoldReason(*cutter)), cutter->flags.Test(VehicleRailFlag::Stuck), IsTBMHoldingTrain(*cutter),
			target.has_value() ? target->tile.base() : 0, target.has_value() ? target->trackdirs.base() : 0);
		for (TileIndex end : {project.portal, GetLayerPortalPairTile(project.portal)}) {
			std::string occupants;
			// Every train part standing on the end is named, so a hauler's tail on the key is visible.
			for (const Train *part : Train::Iterate()) {
				if (part->tile == end) occupants += fmt::format("{}/{}:{},", part->First()->index.base(), part->index.base(), part->track.base());
			}
			row += fmt::format(" end={} work={} reserved={} busy={} door_admit={} shaft_admit={} on={}", end.base(), to_underlying(GetLayerPortalWorkState(end)),
				HasLayerPortalReservation(end), IsLayerPortalMagazineBusy(end, *cutter), GetLayerPortalAdmittedTrackdirs(end, *cutter, GetLayerPortalDoorDirection(end)).base(),
				GetLayerPortalAdmittedTrackdirs(end, *cutter, DiagDirection::Invalid).base(), occupants.empty() ? "-" : occupants);
		}
		if (row == this->previous_cutter) return;
		this->previous_cutter = row;
		fmt::print("DEMO_CUTTER tick={} {}\n", this->ticks, row);
		// An obstruction while cutting comes from the support plan or from another vehicle in the swept envelope; name both.
		const ExcavationJob *job = GetExcavationJob(project.job);
		if (project.blocked != TBMBlockedReason::Obstruction || job == nullptr || job->cursor >= job->route.size()) return;
		const TBMSupportPlan support = PrepareTBMSupport(project, *job);
		const TileIndex failure = support.failure_tile;
		std::string blocker = fmt::format("support_error={} support_tile={}({},{},{})", to_underlying(support.error), failure.base(),
			IsValidTile(failure) ? TileX(failure) : 0, IsValidTile(failure) ? TilePlaneY(failure) : 0, IsValidTile(failure) ? to_underlying(GetTileLayer(failure)) : 0);
		if (IsValidTile(failure)) {
			blocker += fmt::format(" tile_type={} rail_tracks={} has_support={} terrain={}", to_underlying(GetTileType(failure)),
				IsTileType(failure, TileType::Railway) ? GetTrackBits(failure).base() : 0, GetTBMSupportTile(failure) != nullptr,
				GetGeologyWorldState().terrain.Get(failure).has_value() ? static_cast<int>(*GetGeologyWorldState().terrain.Get(failure)) : -1);
		}
		// Every swept cell of the next cut, with any vehicle that is not part of the cutter's own consist.
		for (TileIndex tile : job->route[job->cursor].swept_tiles) {
			for (const Vehicle *v : VehiclesOnTile(tile)) {
				if (v->type != VehicleType::Effect && v->First() != cutter) blocker += fmt::format(" swept_vehicle={}@({},{})", v->First()->index.base(), TileX(tile), TilePlaneY(tile));
			}
		}
		fmt::print("DEMO_CUT_BLOCKER tick={} cursor={} position=({},{}) {}\n", this->ticks, job->cursor, TileX(job->route[job->cursor].cutter_position),
			TilePlaneY(job->route[job->cursor].cutter_position), blocker);
	}

	/**
	 * Log a train's cargo the first time any of its parts holds more than its capacity. The demo autosaves show
	 * 65536 units in 64 t hoppers; this finds the tick and the exchange that produce such a load.
	 * @param front Actual front engine.
	 */
	void TraceOverload(const Train &front)
	{
		bool overloaded = false;
		for (const Train *part = &front; part != nullptr; part = part->Next()) overloaded |= part->cargo.TotalCount() > part->cargo_cap;
		if (!overloaded || this->overloaded.contains(front.index)) return;
		this->overloaded.insert(front.index);
		this->LogCargo("overload", front);
	}

	/** Log every company rail portal whose saved reservation bit changed during the last tick. */
	void TracePortals()
	{
		for (const auto &[tile, reserved] : this->portal_reservations) {
			const bool now = HasLayerPortalReservation(tile);
			if (now == reserved) continue;
			this->portal_reservations[tile] = now;
			fmt::print("DEMO_PORTAL_RESERVATION tick={} tile={}({},{},{}) reserved={} work={}\n", this->ticks, tile.base(), TileX(tile), TilePlaneY(tile),
				to_underlying(GetTileLayer(tile)), now, to_underlying(GetLayerPortalWorkState(tile)));
		}
	}

	/**
	 * Log the actual spoil aboard every part of one train.
	 * @param label Role of the train in the report (cutter or hauler).
	 * @param front Actual front engine.
	 */
	void LogCargo(std::string_view label, const Train &front) const
	{
		uint total = 0;
		std::string parts;
		for (const Train *part = &front; part != nullptr; part = part->Next()) {
			if (part->cargo_cap == 0) continue;
			total += part->cargo.TotalCount();
			parts += fmt::format(" {}:{}={}/{}", part->index.base(), GetString(CargoSpec::Get(part->cargo_type)->name), part->cargo.TotalCount(), part->cargo_cap);
			// The cached count must equal the sum of its packets; listing them separates a bad cache from a bad packet.
			uint packets = 0;
			for (const CargoPacket *packet : *part->cargo.Packets()) {
				packets += packet->Count();
				parts += fmt::format("[packet={}]", packet->Count());
			}
			if (packets != part->cargo.TotalCount()) parts += fmt::format("[packet_sum={}]", packets);
		}
		fmt::print("DEMO_CARGO tick={} role={} train={} total={}{}\n", this->ticks, label, front.index.base(), total, parts);
	}

	/**
	 * Open the excavation projects list through the player's toolbar palette, as the user did.
	 * @param layer Work layer whose palette and project list are shown.
	 * @return The real projects window.
	 */
	Window &OpenProjectsList(MapLayer layer)
	{
		Window *palette = ShowUndergroundConstructionToolbar(layer, GetCurrentMultiLayerProfileSelection());
		REQUIRE(palette != nullptr);
		RelocateAllWindows(1280, 733);
		palette->left = 1280 - palette->width;
		palette->top = 40;
		PlayerClick(*palette, WID_UC_PROJECT);
		Window *browser = FindWindowById(WindowClass::TBMProjects, layer);
		REQUIRE(browser != nullptr);
		browser->left = 40;
		browser->top = 40;
		return *browser;
	}

	/**
	 * Screenshot 03 (R5): wait until the loaded hauler is held in an ordinary depot while the cutter works, then retain it.
	 * In the forced-hauler scenario the hauler is instead forced through the block, as the user did, and the run goes on.
	 * @param project The demo project, whose assigned hauler is observed.
	 */
	void StepHaulerDepot(const TBMProject &project)
	{
		// A forced hauler is followed by the overload trace only; its depot evidence was already logged.
		if (this->forced_at != 0) return;
		const auto *assignment = GetTBMExchangeAssignment(project.id);
		if (assignment == nullptr) return;
		const Train *hauler = Train::GetIfValid(assignment->hauler);
		if (hauler == nullptr) return;
		uint cargo = 0;
		for (const Train *part = hauler; part != nullptr; part = part->Next()) cargo += part->cargo.TotalCount();
		// The report's state is a loaded hauler that the depot rule keeps inside for at least two evaluations (74 ticks).
		if (cargo == 0 || !IsRailDepotTile(hauler->tile) || this->full_decisions[hauler->index] < 2) return;
		const Train *cutter = Train::Get(project.cutter);
		this->LogCargo("hauler", *hauler);
		this->LogCargo("cutter", *cutter);
		fmt::print("DEMO_FINDING report=R5 hauler={} depot={} full_decisions={} cutter={} cutter_phase={}\n", hauler->index.base(),
			DescribePlacement(*hauler), this->full_decisions[hauler->index], DescribePlacement(*cutter), to_underlying(project.phase));
		if (this->scenario == "forced-hauler") {
			// The user then forced the held hauler through the block, as screenshot 03's note says; the run continues
			// so the overload trace can catch what the forced journey does to the spoil aboard.
			AutoRestoreBackup actor(_current_company, hauler->owner);
			const auto forced = Command<Commands::ForceTrainProceed>::Do(DoCommandFlag::Execute, hauler->index);
			this->forced_at = this->ticks;
			fmt::print("DEMO_STEP tick={} name=force-loaded-hauler train={} ok={}\n", this->ticks, hauler->index.base(), forced.Succeeded());
			return;
		}
		this->Capture("03-hauler-held", [&] {
			RelocateAllWindows(1280, 733);
			ShowVehicleViewWindow(hauler);
			ShowVehicleViewWindow(cutter);
		});
		this->Finish();
	}

	/**
	 * Log what a checkpoint must keep across save and reload (task 6.7): every reserved rail piece, the key's
	 * reservation, and the pair's demolition and connection guards.
	 * @param moment Checkpoint name, or "loaded" when a replay run starts from it.
	 */
	void LogGuards(std::string_view moment) const
	{
		std::string pieces;
		uint reserved = 0;
		// Every reservation in the world, so one left behind by a stored or departed train shows up as an extra piece.
		for (const TileIndex tile : Map::Iterate()) {
			const TrackBits bits = GetReservedTrackbits(tile);
			if (bits.None()) continue;
			++reserved;
			pieces += fmt::format(" {}:{}", tile.base(), bits.base());
		}
		const LayerPortalWorkError lifecycle = CheckLayerPortalLifecycle(this->portal);
		const LayerPortalWorkError connect = ChangeLayerPortalConnection(this->portal, this->company, true, false);
		fmt::print("DEMO_GUARDS moment={} key_reserved={} demolish={} connect={} reserved={}{}\n", moment, HasLayerPortalReservation(this->portal),
			to_underlying(lifecycle), to_underlying(connect), reserved, pieces);
	}

	/**
	 * Ask for a checkpoint. The world pauses now and is saved between two game ticks, where a reload resumes it.
	 * @param name Checkpoint name; also the save's file stem.
	 */
	void RequestSave(std::string_view name)
	{
		this->save_request = name;
		AutoRestoreBackup actor(_current_company, this->company);
		REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
	}

	/**
	 * Save a requested checkpoint, then resume and log the following ticks. A real-time timer calls this; it runs
	 * between game loops, so the saved world never holds half a tick.
	 */
	void SaveCheckpoint()
	{
		if (this->save_request.empty() || this->finished) return;
		const auto path = GetTestCaseArtifactPaths(DEMO_FEATURE, this->scenario).saves / fmt::format("{}.sav", this->save_request);
		REQUIRE(SaveOrLoad(path.string(), SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		fmt::print("DEMO_SAVE name={} tick={} path={}\n", this->save_request, this->ticks, path.string());
		this->LogGuards(this->save_request);
		this->replay_name = this->save_request;
		this->replay_left = REPLAY_TICKS;
		this->replay_offset = 0;
		// The face loop logs the owners the file holds, and watches for the first transition after it (r2df2 Group 9).
		if (this->scenario == "face-loop") {
			const TBMProject *project = GetTBMProject(this->project_id);
			REQUIRE(project != nullptr);
			fmt::print("DEMO_FACE_CHECKPOINT name={} tick={} {}\n", this->save_request, this->ticks, DescribeFaceOwners(*project));
			this->face_watch_prev = SampleFaceLoop(*project);
			this->face_watch = this->save_request;
			this->face_watch_from = this->ticks;
		}
		this->save_request.clear();
		AutoRestoreBackup actor(_current_company, this->company);
		REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
	}

	/** Log one tick after a checkpoint: each train's place, speed and load, and the project's phase. A reload must log the same. */
	void LogReplay()
	{
		if (this->replay_left == 0) return;
		--this->replay_left;
		++this->replay_offset;
		std::string digest;
		// Pool order is saved with the trains, so both runs list them alike.
		for (const Train *front : Train::Iterate()) {
			if (!front->IsFrontEngine()) continue;
			uint cargo = 0;
			for (const Train *part = front; part != nullptr; part = part->Next()) cargo += part->cargo.TotalCount();
			digest += fmt::format(" {}:{}/{},{}/{}/{}/{}", front->index.base(), front->tile.base(), front->x_pos, front->y_pos, front->cur_speed, front->track.base(), cargo);
		}
		if (const TBMProject *project = GetTBMProject(this->project_id); project != nullptr) {
			const ExcavationJob *job = GetExcavationJob(project->job);
			digest += fmt::format(" phase={} blocked={} cursor={}", to_underlying(project->phase), to_underlying(project->blocked), job->cursor);
			// The next cut, so a reload proves it continues the same (for task 12.6, the changed) plan.
			if (job->cursor < job->route.size()) digest += fmt::format(" next={}h{}", job->route[job->cursor].cutter_position.base(), to_underlying(job->route[job->cursor].heading));
		}
		fmt::print("DEMO_REPLAY name={} offset={}{}\n", this->replay_name, this->replay_offset, digest);
	}

	/**
	 * R5 after Group 6: the loaded hauler enters an ordinary depot while the cutter works below, and leaves it after the
	 * depot rule's ordinary wait. Its departure, and later the cutter stored in its magazine, are saved as checkpoints.
	 * A depot evaluation that still finds the block Full while the whole cutter is below the Surface fails the run.
	 * @param project The demo project, whose assigned hauler is observed.
	 */
	void StepHaulerDeparture(const TBMProject &project)
	{
		// One checkpoint at a time: its save and replay window come first.
		if (!this->save_request.empty() || this->replay_left != 0) return;
		const Train *cutter = Train::Get(project.cutter);
		bool cutter_below = true;
		bool cutter_stored = true;
		// Below means no part on the Surface layer; stored means every part stands aside on the key.
		for (const Train *part = cutter; part != nullptr; part = part->Next()) {
			cutter_below &= GetTileLayer(part->tile) != MapLayer::Surface;
			cutter_stored &= part->tile == project.portal && part->track == Track::Depot;
		}
		if (this->departure_saved && !this->departure_captured) {
			// Screenshot 03 after the fix (r2df2 task 10.4): the loaded hauler has left its depot by the ordinary rules
			// and gathers speed; its own window follows it.
			const auto *assignment = GetTBMExchangeAssignment(project.id);
			const Train *leaving = assignment == nullptr ? nullptr : Train::GetIfValid(assignment->hauler);
			if (leaving != nullptr && leaving->cur_speed >= 20) {
				this->departure_captured = true;
				this->Capture("03-hauler-leaves-depot", [&] {
					CloseWindowByClass(WindowClass::News); // A popup is presentation only; it would cover the train.
					SetViewportLayer(GetMainWindow(), MapLayer::Surface);
					ScrollMainWindowToTile(leaving->tile, true);
					ShowVehicleViewWindow(leaving);
					if (Window *view = FindWindowById(WindowClass::VehicleView, leaving->index); view != nullptr) UpdateViewportPosition(view, 0);
				});
			}
		}
		if (this->departure_saved) {
			if (!this->stored_saved && cutter_stored) {
				this->stored_saved = true;
				fmt::print("DEMO_STEP tick={} name=cutter-stored cutter={}\n", this->ticks, DescribePlacement(*cutter));
				this->RequestSave("stored-consist");
			} else if (this->stored_saved) {
				this->Finish();
			}
			return;
		}
		const auto *assignment = GetTBMExchangeAssignment(project.id);
		const Train *hauler = assignment == nullptr ? nullptr : Train::GetIfValid(assignment->hauler);
		if (hauler == nullptr) return;
		if (this->hauler_depot == INVALID_TILE) {
			uint cargo = 0;
			for (const Train *part = hauler; part != nullptr; part = part->Next()) cargo += part->cargo.TotalCount();
			// The report's state: spoil aboard and the whole train inside an ordinary depot.
			if (cargo == 0 || !IsRailDepotTile(hauler->tile) || !hauler->IsChainInDepot()) return;
			this->hauler_depot = hauler->tile;
			this->hauler_depot_at = this->ticks;
			this->LogCargo("hauler", *hauler);
			fmt::print("DEMO_STEP tick={} name=loaded-hauler-in-depot depot={} cutter={} cutter_below={}\n", this->ticks, this->hauler_depot.base(),
				DescribePlacement(*cutter), cutter_below);
			return;
		}
		if (hauler->IsChainInDepot()) {
			// The R5 hold: a fresh depot evaluation found the block Full although no part of the cutter is on the Surface.
			if (hauler->wait_counter == 0 && this->last_decision_full[hauler->index] && cutter_below) {
				fmt::print("DEMO_FINDING report=R5 fixed=false hauler={} cutter={}\n", hauler->index.base(), DescribePlacement(*cutter));
				FAIL("the loaded hauler is still held while the whole cutter works below the Surface");
			}
			return;
		}
		// Mid-departure: the front has left the depot while a later part still stands inside.
		bool inside = false;
		for (const Train *part = hauler; part != nullptr; part = part->Next()) inside |= part->tile == this->hauler_depot && part->track == Track::Depot;
		if (!inside) return;
		fmt::print("DEMO_FINDING report=R5 fixed=true hauler={} waited={} full_decisions={} cutter={} cutter_below={}\n", hauler->index.base(),
			this->ticks - this->hauler_depot_at, this->full_decisions[hauler->index], DescribePlacement(*cutter), cutter_below);
		this->departure_saved = true;
		this->RequestSave("hauler-departing");
	}

	/**
	 * @param id Station.
	 * @return Earth and Rock waiting at the station, reserved cargo included.
	 */
	static uint WaitingSpoil(StationID id)
	{
		const Station *st = Station::GetIfValid(id);
		if (st == nullptr) return 0;
		uint total = 0;
		for (const CargoLabel label : {CargoLabel{"ERTH"}, CargoLabel{"ROCK"}}) {
			const CargoType cargo = GetCargoTypeByLabel(label);
			if (IsValidCargoType(cargo)) total += st->goods[cargo].TotalCount();
		}
		return total;
	}

	/**
	 * R11 after Group 6B: follow the hauler's stops under "Unload all, No loading". Each completed stop logs the spoil it
	 * carried in and out and the spoil its station holds afterwards: the receivers took what left the train but not into
	 * the station, the rest was transferred. An overloaded part fails the run; three completed stops end it.
	 * @param project The demo project, whose assigned hauler is observed.
	 */
	void StepUnloadOrder(const TBMProject &project)
	{
		const auto *assignment = GetTBMExchangeAssignment(project.id);
		const Train *hauler = assignment == nullptr ? nullptr : Train::GetIfValid(assignment->hauler);
		if (hauler == nullptr) return;
		if (this->overloaded.contains(hauler->index)) FAIL("a hauler part holds more spoil than its capacity");
		const bool loading = hauler->current_order.IsType(OT_LOADING);
		if (loading == this->hauler_loading) return;
		this->hauler_loading = loading;
		uint aboard = 0;
		for (const Train *part = hauler; part != nullptr; part = part->Next()) aboard += part->cargo.TotalCount();
		if (loading) {
			this->stop_station = hauler->last_station_visited;
			this->stop_aboard = aboard;
			this->stop_waiting = WaitingSpoil(this->stop_station);
			return;
		}
		const uint waiting = WaitingSpoil(this->stop_station);
		const uint transferred = waiting > this->stop_waiting ? waiting - this->stop_waiting : 0;
		const uint unloaded = this->stop_aboard > aboard ? this->stop_aboard - aboard : 0;
		fmt::print("DEMO_STOP tick={} hauler={} station={} aboard_in={} aboard_out={} waiting_in={} waiting_out={} delivered={} transferred={}\n",
			this->ticks, hauler->index.base(), this->stop_station.base(), this->stop_aboard, aboard, this->stop_waiting, waiting,
			unloaded > transferred ? unloaded - transferred : 0, transferred);
		this->LogCargo("hauler", *hauler);
		if (++this->stops == 3) this->Finish();
	}

	/**
	 * Screenshot 02 (R4): when the cutter returns with spoil aboard, show the projects list and its actual cargo.
	 * @param project The demo project.
	 */
	void StepHeld(const TBMProject &project)
	{
		const Train *cutter = Train::Get(project.cutter);
		uint cargo = 0;
		for (const Train *part = cutter; part != nullptr; part = part->Next()) cargo += part->cargo.TotalCount();
		if (!IsTBMReturning(project.phase) || cargo == 0) return;
		this->LogCargo("cutter", *cutter);
		if (const auto *assignment = GetTBMExchangeAssignment(project.id); assignment != nullptr) this->LogCargo("hauler", *Train::Get(assignment->hauler));
		// Group 4 replaced "Held: nothing" by the wait line; the projects list capture also shows the carried-cargo lines.
		fmt::print("DEMO_FINDING report=R4 phase={} blocked={} hold_text=\"{}\"\n", to_underlying(project.phase), to_underlying(project.blocked),
			GetTBMWaitLine(project.phase, project.blocked));
		this->Capture("02-held-nothing", [&] {
			SetViewportLayer(GetMainWindow(), GetTileLayer(cutter->tile));
			ScrollMainWindowToTile(cutter->tile, true);
			this->OpenProjectsList(GetTileLayer(GetExcavationJob(project.job)->commissioning_anchor));
		});
		this->Finish();
	}

	/**
	 * Log the simulation state that the view-equivalence comparison reads; nothing presentational is included.
	 * @param project The followed project.
	 * @param reason Why the row is logged: "period" or "transfer".
	 */
	void LogFaceState(const TBMProject &project, std::string_view reason) const
	{
		const ExcavationJob *job = GetExcavationJob(project.job);
		fmt::print("DEMO_FACE_STATE tick={} reason={} phase={} cursor={} random={:08x}{:08x} money={} geology={:016x} portal={:016x} trains={:016x}\n", this->ticks, reason,
			to_underlying(project.phase), job == nullptr ? 0 : job->cursor, _random.state[0], _random.state[1], Company::Get(project.company)->money.base(),
			ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeLayerPortalWorkDigest(), DigestTrains());
	}

	/**
	 * Count the bends behind the cutter: where the heading of a completed cut differs from the cut before it.
	 * @param job The project's job.
	 * @return Heading changes among the completed cuts.
	 */
	static uint CountBendsBehind(const ExcavationJob &job)
	{
		uint bends = 0;
		for (size_t cut = 1; cut < job.cursor && cut < job.route.size(); ++cut) {
			if (job.route[cut].heading != job.route[cut - 1].heading) ++bends;
		}
		return bends;
	}

	/**
	 * Move the camera as the selected view asks (task 8.3). The visible view follows the cutter on its own layer;
	 * the hidden view watches the Surface above it, so the work layer is never drawn. Headless never touches it.
	 * @param cutter The followed cutter.
	 */
	void MoveFaceLoopCamera(const Train &cutter) const
	{
		// Headless never touches the camera; the other views move it every 16 ticks, often enough to keep the cutter in view.
		if (this->view == "headless" || this->ticks % 16 != 0) return;
		const bool visible = this->view == "visible";
		const TileIndex focus = visible ? cutter.tile : TileXYL(TileX(cutter.tile), TilePlaneY(cutter.tile), MapLayer::Surface);
		SetViewportLayer(GetMainWindow(), GetTileLayer(focus));
		ScrollMainWindowToTile(focus, true);
	}

	/**
	 * Log one exchange with what the plan asks for at each transfer: stand, dwell, packets, money and both phases.
	 * @param project The followed project.
	 * @param before Face-loop state on the tick before the exchange.
	 * @param after Face-loop state after it.
	 */
	void LogFaceExchange(const TBMProject &project, const FaceLoopSample &before, const FaceLoopSample &after)
	{
		// The meeting point on the tick before decides where the exchange happened, as the controller saw it.
		const bool at_face = before.meeting == TBMMeetingKind::Face;
		const uint number = at_face ? ++this->face_transfers : ++this->bay_exchanges;
		const ExcavationJob *job = GetExcavationJob(project.job);
		const TileIndex stand = before.stand.has_value() ? before.stand->tile : INVALID_TILE;
		// OpenTTD calls the X and Y pieces "diagonal"; a 45-degree heading of the route lays the half-tile corner pieces instead.
		const bool half_piece = before.stand.has_value() && !IsDiagonalTrackdir(before.stand->trackdir);
		fmt::print("DEMO_FACE_TRANSFER n={} tick={} meeting={} cursor={} stand={} stand_trackdir={} stand_piece={} bends_behind={} dwell={} hauler_waited={} "
			"cutter_phase={}->{} hauler_phase={}->{} cutter_tile={} wait_tile={} hauler={}..{} hauler_trackdir={} cutter_aboard={}->{} hauler_aboard={}->{} "
			"cutter_packets={}->{} hauler_packets={}->{} money={}->{}\n",
			number, this->ticks, at_face ? "face" : "bay", after.cursor, DescribeTile(stand), before.stand.has_value() ? to_underlying(before.stand->trackdir) : -1,
			half_piece ? "diagonal" : "straight", job == nullptr ? 0 : CountBendsBehind(*job), before.phase_ticks, this->ticks - this->face_hauler_since,
			to_underlying(before.phase), to_underlying(after.phase), to_underlying(before.hauler_phase), to_underlying(after.hauler_phase), DescribeTile(after.cutter_tile),
			DescribeTile(this->face_wait_tile), DescribeTile(before.hauler_tile), DescribeTile(before.hauler_last_tile), to_underlying(before.hauler_trackdir),
			before.cutter_aboard, after.cutter_aboard, before.hauler_aboard, after.hauler_aboard, before.cutter_packets, after.cutter_packets,
			before.hauler_packets, after.hauler_packets, before.money.base(), after.money.base());
		const auto *assignment = GetTBMExchangeAssignment(project.id);
		if (Train *cutter = Train::GetIfValid(project.cutter); cutter != nullptr) this->LogCargo("cutter", *cutter);
		if (assignment != nullptr) this->LogCargo("hauler", *Train::Get(assignment->hauler));
		this->LogFaceState(project, "transfer");
		if (!at_face) {
			// Near the portal there is no room for a stand (rule F1), and the bay serves; with a stand, the face should have.
			if (before.stand.has_value()) ++this->bay_with_stand;
			return;
		}
		// A face transfer moves every unit, keeps the total, waits out the dwell and leaves the cutter where it waited.
		CHECK(before.cutter_aboard + before.hauler_aboard == after.cutter_aboard + after.hauler_aboard);
		CHECK(after.cutter_aboard == 0);
		CHECK(before.phase_ticks >= TBM_FACE_TRANSFER_TICKS);
		CHECK(after.hauler_phase == TBMHaulerPhase::CollectLoaded);
		// Task 8.2 asks for a stand on a diagonal section that the route reached through a bend.
		if (half_piece && job != nullptr && CountBendsBehind(*job) > 0) ++this->face_bent_transfers;
		if (this->view != "headless") this->Capture(fmt::format("transfer-{}", number), [] { CloseWindowByClass(WindowClass::News); });
	}

	/**
	 * Retain the hauler behind the cutter at the face, comparable with screenshot 06 (task 8.4), with the projects
	 * list showing both trains' phases. Only the visible view draws the work layer.
	 * @param project The followed project.
	 * @param stand Stand the hauler waits on.
	 * @param cutter Cutter front tile.
	 */
	void CaptureHaulerAtFace(const TBMProject &project, TileIndex stand, TileIndex cutter)
	{
		this->face_captured = true;
		if (this->view != "visible") return;
		// Centre between the hauler's stand and the cutter's front, so both trains and the gap between them are in view.
		const TileIndex centre = TileXYL((TileX(stand) + TileX(cutter)) / 2, (TilePlaneY(stand) + TilePlaneY(cutter)) / 2, GetTileLayer(stand));
		this->Capture("06-hauler-behind-cutter", [&] {
			CloseWindowByClass(WindowClass::News); // A popup is presentation only; it would cover the trains.
			SetViewportLayer(GetMainWindow(), GetTileLayer(centre));
			ScrollMainWindowToTile(centre, true);
			this->OpenProjectsList(GetTileLayer(GetExcavationJob(project.job)->commissioning_anchor));
		});
	}

	/**
	 * Name the face-loop state a checkpoint is wanted in (task 9.4), when the loop is in one now.
	 * @param project The followed project.
	 * @param now State after the last tick.
	 * @return Checkpoint name, or empty when the loop is in none of them.
	 */
	static std::string FindFaceCheckpoint(const TBMProject &project, const FaceLoopSample &now)
	{
		const bool on_stand = now.stand.has_value() && (now.hauler_tile == now.stand->tile || now.hauler_last_tile == now.stand->tile);
		// The hauler drives along the corridor towards a stand at the face.
		if (now.meeting == TBMMeetingKind::Face && now.hauler_phase == TBMHaulerPhase::ReturnEmpties && now.hauler_speed > 0 &&
			FindTBMHaulerInCorridor(project) != VehicleID::Invalid()) return "travelling";
		// It stands still on its stand while the cutter still bores towards its breakpoint.
		if (on_stand && now.hauler_phase == TBMHaulerPhase::AwaitExchange && now.hauler_speed == 0 &&
			(now.phase == TBMPhase::Bore || now.phase == TBMPhase::AdvanceSupport)) return "held";
		// The cutter waits at its face with the hauler on the stand, half-way through the dwell.
		if (on_stand && IsTBMAwaitingFaceTransfer(now.phase) && now.phase_ticks >= TBM_FACE_TRANSFER_TICKS / 2 && now.phase_ticks < TBM_FACE_TRANSFER_TICKS) return "dwell";
		// The loaded hauler crosses the project's portal pair on its way out.
		if (now.hauler_aboard > 0 && now.hauler_on_pair &&
			(now.hauler_phase == TBMHaulerPhase::CollectLoaded || now.hauler_phase == TBMHaulerPhase::VisitOrigin)) return "leaving";
		// The cutter returns to its bay for an exchange met there, because its stand had no room yet (rule F1).
		if (now.phase == TBMPhase::ReturnForExchange && now.meeting == TBMMeetingKind::Bay && now.cursor > 0) return "bay-return";
		return {};
	}

	/**
	 * Task 9.5: take the player's action whose result is saved, then name its checkpoint once the result shows.
	 * "stop" stops the hauler on its way to the stand. "park" and "release" send the cutter back from its face wait
	 * with the hauler on the stand, so rule F7 holds it there; the release then waits while the loaded hauler leaves.
	 * @param project The followed project.
	 * @param now State after the last tick.
	 * @return Checkpoint name once the result shows, or empty.
	 */
	std::string StepFaceLifecycle(const TBMProject &project, const FaceLoopSample &now)
	{
		const TBMExchangeAssignment *assignment = GetTBMExchangeAssignment(project.id);
		if (assignment == nullptr) return {};
		if (!this->face_lifecycle_acted) {
			if (this->face_lifecycle == "stop") {
				// Stopped on its way in, in the corridor, as the player's start/stop button does.
				if (FindFaceCheckpoint(project, now) != "travelling") return {};
				REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, assignment->hauler, false).Succeeded());
			} else {
				// Park or cancel while the cutter waits at its face with the hauler standing on the stand behind it.
				const bool on_stand = now.stand.has_value() && now.hauler_tile == now.stand->tile && now.hauler_speed == 0;
				if (!IsTBMAwaitingFaceTransfer(now.phase) || !on_stand) return {};
				const TBMProjectAction action = this->face_lifecycle == "park" ? TBMProjectAction::ReturnAndPark : TBMProjectAction::Cancel;
				REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project.id, action).Succeeded());
			}
			this->face_lifecycle_acted = true;
			fmt::print("DEMO_STEP tick={} name=face-{} phase={} hauler_phase={} cursor={}\n", this->ticks, this->face_lifecycle,
				to_underlying(GetTBMProject(project.id)->phase), to_underlying(now.hauler_phase), now.cursor);
			return {};
		}
		// The result to save: the stopped hauler standing in the corridor; the return held for the hauler before its
		// transfer; the release waiting while the loaded hauler leaves.
		if (this->face_lifecycle == "stop") return now.hauler_speed == 0 && now.hauler_wait == TBMHaulerWait::Stopped ? "hauler-stopped" : "";
		if (this->face_lifecycle == "park") return IsTBMReturning(now.phase) && now.blocked == TBMBlockedReason::Obstruction && now.cutter_aboard > 0 ? "return-held" : "";
		return now.phase == TBMPhase::ReleasePending && now.hauler_aboard > 0 ? "release-pending" : "";
	}

	/**
	 * Ask for the next face-loop checkpoint when the loop is in a wanted state (tasks 9.4 and 9.5). It is saved
	 * between two game ticks; the uninterrupted run then logs its next transition, which a reload must repeat.
	 * @param project The followed project.
	 * @param now State after the last tick.
	 */
	void RequestFaceCheckpoint(const TBMProject &project, const FaceLoopSample &now)
	{
		// One checkpoint at a time: its save and its watch come first.
		if (!this->save_request.empty() || !this->face_watch.empty()) return;
		const std::string name = this->face_lifecycle.empty() ? FindFaceCheckpoint(project, now) : this->StepFaceLifecycle(project, now);
		if (name.empty()) return;
		if (this->face_lifecycle.empty()) {
			if (!this->face_saves.contains(name)) return;
			this->face_saves.erase(name);
		} else {
			if (this->face_lifecycle_saved) return;
			this->face_lifecycle_saved = true;
		}
		// The owners are logged when the file is written, between two game ticks; inside a tick they are half-way.
		fmt::print("DEMO_FACE_REQUEST name=face-{} tick={} cursor={}\n", name, this->ticks, now.cursor);
		this->RequestSave(fmt::format("face-{}", name));
	}

	/**
	 * Change the work into a straight tunnel from the face. The first change ends at 45 degrees to the current
	 * heading: the farthest such cell the owner accepts whose route bends and then runs on for several cuts. Each
	 * extension ends straight ahead on the diagonal. The change goes through the ordinary command that the palette's
	 * confirm posts, with the quote's cursor and fingerprint.
	 * @param project The followed project.
	 * @param job Its job, with at least one completed cut.
	 * @param extend Whether to extend the diagonal straight on instead of bending.
	 * @return Whether a change was confirmed.
	 */
	bool BendFaceLoop(const TBMProject &project, const ExcavationJob &job, bool extend)
	{
		const ExcavationCutGeometry &last = job.route[job.cursor - 1];
		const auto ahead = TileIndexDiffCByDir(last.heading);
		// Farthest first, to either side for a bend: the longer the diagonal, the more breakpoints fall behind the bend.
		for (int distance = 12; distance >= (extend ? 3 : 5); --distance) {
			for (int turn : {2, 6}) {
				if (extend && turn != 2) continue; // Straight on has a single end; the sides matter only for the bend.
				const auto side = TileIndexDiffCByDir(static_cast<Direction>((to_underlying(last.heading) + turn) % 8));
				const int along_x = extend ? ahead.x : ahead.x + side.x;
				const int along_y = extend ? ahead.y : ahead.y + side.y;
				const TileIndex end = TileAddWrap(last.cutter_position, distance * along_x, distance * along_y);
				if (end == INVALID_TILE) continue;
				const auto quote = PreviewTBMWorkChange(project.company, project.id, last.cutter_position, end, ExcavationPlanIntent::StraightTunnel);
				fmt::print("DEMO_FACE_BEND_CANDIDATE end={} ok={} message=\"{}\" plan_error={} failure={}\n", DescribeTile(end), quote.result.Succeeded(),
					quote.result.Succeeded() ? "" : GetString(quote.result.GetErrorMessage()), to_underlying(quote.plan.error), DescribeTile(quote.plan.failure_tile));
				if (quote.result.Failed()) continue;
				// The first new cut whose heading differs from the cut before it is the bend; several cuts must follow it.
				// An extension needs no bend, only a few more cuts.
				size_t bend = quote.plan.route.size();
				for (size_t cut = 0; cut < quote.plan.route.size() && bend == quote.plan.route.size(); ++cut) {
					const Direction previous = cut == 0 ? last.heading : quote.plan.route[cut - 1].heading;
					if (quote.plan.route[cut].heading != previous) bend = cut;
				}
				if (extend ? quote.plan.route.size() < 3 : bend + 4 > quote.plan.route.size()) continue;
				std::string headings;
				for (const ExcavationCutGeometry &cut : quote.plan.route) headings += fmt::format("{}", to_underlying(cut.heading));
				const CommandCost cost = Command<Commands::ChangeTBMWork>::Do(DoCommandFlag::Execute, project.id, last.cutter_position, end, quote.cursor,
					quote.fingerprint, ExcavationPlanIntent::StraightTunnel);
				const ExcavationJob &changed = *GetExcavationJob(GetTBMProject(project.id)->job);
				std::string breakpoints;
				if (changed.pass_terms.has_value()) {
					for (uint16_t after : changed.pass_terms->exchange_after) breakpoints += fmt::format("{},", after);
				}
				fmt::print("DEMO_FACE_BEND tick={} kind={} end={} cursor={} new={} bend_at={} ok={} route={} breakpoints={} headings={}\n", this->ticks,
					extend ? "extend" : "bend", DescribeTile(end), changed.cursor, quote.plan.route.size(), bend, cost.Succeeded(), changed.route.size(), breakpoints, headings);
				REQUIRE(cost.Succeeded());
				return true;
			}
		}
		return false;
	}

	/**
	 * Task 8.2: prepare the Deep run as a player would. Resume the loaded project and start its stopped trains, then,
	 * once the first cut is done, change the work into a tunnel at 45 degrees, so the hauler meets the cutter behind a
	 * diagonal section after a bend.
	 * @param project The followed project.
	 */
	void StepFaceBend(const TBMProject &project)
	{
		const ExcavationJob *job = GetExcavationJob(project.job);
		if (job == nullptr) return;
		switch (this->face_bend_stage) {
			case FaceBendStage::Setup: {
				if (project.blocked == TBMBlockedReason::Paused) {
					PlayerClick(this->OpenProjectsList(GetTileLayer(job->commissioning_anchor)), WID_TP_RESUME);
					fmt::print("DEMO_STEP tick={} name=resume blocked={}\n", this->ticks, to_underlying(GetTBMProject(project.id)->blocked));
				}
				// A stopped cutter or hauler of the project is started with the ordinary start command.
				std::vector<VehicleID> trains{project.cutter};
				if (const auto *assignment = GetTBMExchangeAssignment(project.id); assignment != nullptr) trains.push_back(assignment->hauler);
				for (VehicleID id : trains) {
					const Train *train = Train::GetIfValid(id);
					if (train == nullptr || !train->vehstatus.Test(VehState::Stopped)) continue;
					const CommandCost started = Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, id, false);
					fmt::print("DEMO_STEP tick={} name=start train={} ok={}\n", this->ticks, id.base(), started.Succeeded());
					CHECK(started.Succeeded());
				}
				this->face_bend_stage = FaceBendStage::Boring;
				return;
			}
			case FaceBendStage::Boring:
				// Like the user's change of work, while the machine really bores its next cut.
				if (job->cursor == 0 || project.phase != TBMPhase::Bore || job->partial_work_ticks == 0) return;
				if (this->BendFaceLoop(project, *job, false)) this->face_bend_stage = FaceBendStage::Bent;
				return;
			case FaceBendStage::Bent:
				// Near the end of the route, while boring, the diagonal is drawn further on, a few times.
				if (this->face_extensions >= FACE_BEND_EXTENSIONS || project.phase != TBMPhase::Bore || job->partial_work_ticks == 0) return;
				if (job->cursor + 2 < job->route.size()) return;
				++this->face_extensions; // One attempt per extension: a refused one is not retried every tick.
				this->BendFaceLoop(project, *job, true);
				return;
		}
	}

	/**
	 * R10 after r2df2 Groups 2-7: follow the real loop and log every cut, phase change and exchange, until the hauler
	 * has met the cutter at its face often enough and the rate window has passed (the Deep run needs no rate).
	 * @param project The followed project.
	 */
	void StepFaceLoop(const TBMProject &project)
	{
		if (this->face_bend) this->StepFaceBend(project);
		const FaceLoopSample now = SampleFaceLoop(project);
		if (const Train *cutter = Train::GetIfValid(project.cutter); cutter != nullptr) this->MoveFaceLoopCamera(*cutter);
		if (this->ticks >= this->face_next_state) {
			this->LogFaceState(project, "period");
			this->face_next_state += FACE_STATE_TICKS;
		}
		if (this->face_before.has_value()) {
			const FaceLoopSample &before = *this->face_before;
			if (now.cursor != before.cursor) fmt::print("DEMO_FACE_CUT tick={} cursor={} aboard={}\n", this->ticks, now.cursor, now.cutter_aboard);
			if (now.phase != before.phase) {
				fmt::print("DEMO_FACE_PHASE tick={} train=cutter phase={}->{} meeting={} stand={}\n", this->ticks, to_underlying(before.phase), to_underlying(now.phase),
					to_underlying(now.meeting), DescribeTile(now.stand.has_value() ? now.stand->tile : INVALID_TILE));
				if (IsTBMAwaitingFaceTransfer(now.phase)) this->face_wait_tile = now.cutter_tile;
			}
			if (now.hauler_phase != before.hauler_phase || now.hauler_wait != before.hauler_wait) {
				fmt::print("DEMO_FACE_PHASE tick={} train=hauler phase={}->{} wait={} meeting={} at={}\n", this->ticks, to_underlying(before.hauler_phase),
					to_underlying(now.hauler_phase), to_underlying(now.hauler_wait), to_underlying(now.meeting), DescribeTile(now.hauler_tile));
				if (now.hauler_phase == TBMHaulerPhase::AwaitExchange) this->face_hauler_since = this->ticks;
			}
			// The first time the hauler stands still on its stand behind a cutter that waits for it at the face is the
			// screenshot-06 view; a hauler still waiting at the bay as the meeting point moves to the face does not count.
			const bool hauler_on_stand = now.stand.has_value() && now.hauler_phase == TBMHaulerPhase::AwaitExchange && now.hauler_speed == 0 &&
				(now.hauler_tile == now.stand->tile || now.hauler_last_tile == now.stand->tile);
			if (!this->face_captured && IsTBMAwaitingFaceTransfer(now.phase) && hauler_on_stand) this->CaptureHaulerAtFace(project, now.stand->tile, now.cutter_tile);
			if (now.exchanges != before.exchanges) this->LogFaceExchange(project, before, now);
		}
		// A saved checkpoint's watch ends at the first transition after it, which a reload must reach the same way. It
		// first compares with the world as saved between two ticks, as the reload does, not with this tick's sample.
		if (!this->face_watch.empty() && this->face_watch_prev.has_value()) {
			const std::string transition = DescribeFaceTransition(*this->face_watch_prev, now);
			const bool horizon = this->ticks - this->face_watch_from >= FACE_WATCH_TICKS;
			if (!transition.empty() || horizon) {
				fmt::print("DEMO_FACE_NEXT name={} offset={} {} {}\n", this->face_watch, this->ticks - this->face_watch_from, transition.empty() ? "horizon" : transition,
					DescribeFaceOwners(project));
				this->face_watch.clear();
			}
			this->face_watch_prev = now;
		}
		this->face_before = now;
		this->RequestFaceCheckpoint(project, now);
		// The cut count after the rate window is logged once, on the first tick past it (task 8.5).
		if (!this->face_rate_logged && this->ticks >= FACE_RATE_TICKS) {
			this->face_rate_logged = true;
			fmt::print("DEMO_FACE_RATE ticks={} cursor={} face_transfers={} bay_exchanges={}\n", this->ticks, now.cursor, this->face_transfers, this->bay_exchanges);
		}
		// Done once enough face transfers were seen and, on the demo route, the rate window has passed too; a lifecycle
		// run is done once its checkpoint is saved. Either way a pending checkpoint and its watch finish first.
		const bool idle = this->save_request.empty() && this->face_watch.empty();
		const bool loop_done = this->face_lifecycle.empty() && this->face_saves.empty() && this->face_transfers >= this->face_target &&
			(this->face_rate_logged || this->face_bend);
		if (idle && (loop_done || this->face_lifecycle_saved)) this->Finish();
	}

	/** Log the face loop's outcome and check it reached its target without a bay exchange. */
	void FinishFaceLoop()
	{
		fmt::print("DEMO_FACE_SUMMARY view={} ticks={} face_transfers={} bay_exchanges={} bay_with_stand={} target={} rate_logged={} bent_transfers={}\n", this->view,
			this->ticks, this->face_transfers, this->bay_exchanges, this->bay_with_stand, this->face_target, this->face_rate_logged, this->face_bent_transfers);
		CHECK(this->face_transfers >= this->face_target);
		CHECK(this->bay_with_stand == 0);
		if (this->face_bend) CHECK(this->face_bent_transfers > 0);
		// The demo route has room for a stand at its first breakpoint, so every exchange there is at the face; the Deep
		// run starts at its portal, where the first breakpoints have no room and the bay serves (rule F1).
		if (!this->face_allow_bay) CHECK(this->bay_exchanges == 0);
		// Every wanted checkpoint was saved and its next transition logged.
		CHECK(this->face_saves.empty());
		CHECK(this->face_watch.empty());
		if (!this->face_lifecycle.empty()) CHECK(this->face_lifecycle_saved);
	}

	/** Log every company signal whose state changed during the last tick. */
	void TraceSignals()
	{
		for (auto &[key, state] : this->signals) {
			const SignalState now = GetSignalStateByTrackdir(key.first, key.second);
			if (now == state) continue;
			state = now;
			fmt::print("DEMO_SIGNAL_CHANGE tick={} tile={}({},{}) trackdir={} state={}\n", this->ticks, key.first.base(), TileX(key.first), TilePlaneY(key.first),
				to_underlying(key.second), to_underlying(now));
		}
	}

	/**
	 * Draw one consist laid out along a direction at the real part pitch into the current drawing area's cell.
	 * @param families Consist from front to back.
	 * @param direction Train direction 0..7 (N..NW).
	 * @param zoom Viewport zoom level of the cell.
	 * @param cell Cell rectangle inside the current 8bpp canvas, in screen pixels.
	 */
	static void DrawConsistCell(std::span<const PoseFamily> families, uint8_t direction, ZoomLevel zoom, const Rect &cell)
	{
		const auto [dx, dy] = TRAIN_STEP[direction];
		// Each trailing part stands its length (8 steps) behind its leader; the consist is centred in the cell.
		const Point span = RemapCoords(-8 * dx * static_cast<int>(families.size() - 1), -8 * dy * static_cast<int>(families.size() - 1), 0);
		std::vector<std::pair<SpriteID, Point>> sprites;
		for (size_t index = 0; index < families.size(); ++index) {
			const auto &family = families[index];
			const auto sprite = GetMultiLayerSprite(family.asset, {family.state, 0, direction, 0, family.part});
			if (!sprite.has_value()) continue;
			const Point at = RemapCoords(-8 * dx * static_cast<int>(index), -8 * dy * static_cast<int>(index), 0);
			sprites.emplace_back(*sprite, Point{at.x - span.x / 2, at.y - span.y / 2});
		}
		// Parts higher on the screen are further away; draw them first, as the viewport's depth sorting would.
		std::ranges::stable_sort(sprites, [](const auto &a, const auto &b) { return a.second.y < b.second.y; });
		DrawPixelInfo *canvas = _cur_dpi;
		uint8_t *origin = static_cast<uint8_t *>(canvas->dst_ptr) + cell.top * canvas->pitch + cell.left;
		const int width = cell.right - cell.left + 1;
		const int height = cell.bottom - cell.top + 1;
		DrawPixelInfo dpi{origin, -ScaleByZoom(width / 2, zoom), -ScaleByZoom(height / 2, zoom), ScaleByZoom(width, zoom), ScaleByZoom(height, zoom), canvas->pitch, zoom};
		AutoRestoreBackup drawing(_cur_dpi, &dpi);
		for (const auto &[sprite, offset] : sprites) DrawSpriteViewport(sprite, PAL_NONE, offset.x, offset.y);
	}

	/**
	 * Task 1.6 (R7): measure every authored pose of the demo's vehicle families at every zoom, compare the normal-zoom
	 * extent with the safe envelope, measure how much adjacent consist parts overlap at the real 8-step pitch, and
	 * capture both demo consists in all eight directions at every zoom.
	 */
	void MeasurePoses()
	{
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		// Widest authored pose per family, direction and zoom, over every state, animation frame and part.
		// The asset enumeration interleaves other assets, so the VehicleModelNN number is the position in this list.
		const std::array<MultiLayerSpriteAsset, 5> models{MultiLayerSpriteAsset::VehicleModel00, MultiLayerSpriteAsset::VehicleModel01,
			MultiLayerSpriteAsset::VehicleModel02, MultiLayerSpriteAsset::VehicleModel03, MultiLayerSpriteAsset::VehicleModel04};
		for (size_t model = 0; model < models.size(); ++model) {
			const MultiLayerSpriteAsset asset = models[model];
			const auto &range = _multilayer_m1_sprite_ranges[static_cast<size_t>(asset)];
			for (uint8_t direction = 0; direction < 8 && direction < range.directions; ++direction) {
				for (int level = to_underlying(ZoomLevel::Min); level <= to_underlying(ZoomLevel::Max); ++level) {
					const ZoomLevel zoom = static_cast<ZoomLevel>(level);
					Dimension widest{0, 0};
					std::string pose;
					for (uint8_t state = 0; state < range.states; ++state) {
						for (uint8_t frame = 0; frame < range.frames; ++frame) {
							for (uint8_t part = 0; part < range.parts; ++part) {
								const auto sprite = GetMultiLayerSprite(asset, {state, frame, direction, 0, part});
								if (!sprite.has_value()) continue;
								const std::array<std::pair<SpriteID, Point>, 1> one{{{*sprite, {0, 0}}}};
								const Dimension size = MeasureOpaque(RenderPoses(zoom, one));
								if (size.width <= widest.width) continue;
								widest = size;
								pose = fmt::format("{}/{}/{}", state, frame, part);
							}
						}
					}
					const auto [envelope_w, envelope_h] = POSE_ENVELOPE[direction];
					// The envelope is defined at normal zoom; other zooms are logged for the review but not judged.
					const bool judged = zoom == ZoomLevel::Normal;
					fmt::print("DEMO_POSE model={} direction={} zoom={} width={} height={} widest_pose={}{}\n", model,
						direction, level, widest.width, widest.height, pose,
						judged ? fmt::format(" envelope={}x{} over_width={} over_height={}", envelope_w, envelope_h, widest.width > envelope_w, widest.height > envelope_h) : "");
				}
			}
		}
		// Adjacent-part overlap at the real pitch: the leader stands at the origin, its trailer 8 steps behind it.
		const std::array<std::pair<size_t, size_t>, 7> pairs{{{0, 1}, {1, 4}, {4, 5}, {5, 4}, {6, 4}, {2, 3}, {3, 4}}};
		for (const auto &[leader, trailer] : pairs) {
			for (uint8_t direction = 0; direction < 8; ++direction) {
				const auto first = GetMultiLayerSprite(POSE_FAMILIES[leader].asset, {POSE_FAMILIES[leader].state, 0, direction, 0, POSE_FAMILIES[leader].part});
				const auto second = GetMultiLayerSprite(POSE_FAMILIES[trailer].asset, {POSE_FAMILIES[trailer].state, 0, direction, 0, POSE_FAMILIES[trailer].part});
				if (!first.has_value() || !second.has_value()) continue;
				const auto [dx, dy] = TRAIN_STEP[direction];
				const std::array<std::pair<SpriteID, Point>, 1> front{{{*first, {0, 0}}}};
				const std::array<std::pair<SpriteID, Point>, 1> back{{{*second, RemapCoords(-8 * dx, -8 * dy, 0)}}};
				const auto a = RenderPoses(ZoomLevel::Normal, front);
				const auto b = RenderPoses(ZoomLevel::Normal, back);
				uint overlap = 0;
				for (size_t i = 0; i < a.size(); ++i) overlap += a[i] != 0 && b[i] != 0;
				fmt::print("DEMO_OVERLAP leader={} trailer={} direction={} zoom={} overlap_pixels={}\n", POSE_FAMILIES[leader].name, POSE_FAMILIES[trailer].name,
					direction, to_underlying(ZoomLevel::Normal), overlap);
			}
		}
		// Review captures: both demo consists in all eight directions, one image per zoom level.
		const std::array<PoseFamily, 5> cutter_consist{POSE_FAMILIES[0], POSE_FAMILIES[1], POSE_FAMILIES[4], POSE_FAMILIES[5], POSE_FAMILIES[4]};
		const std::array<PoseFamily, 3> hauler_consist{POSE_FAMILIES[6], POSE_FAMILIES[4], POSE_FAMILIES[5]};
		for (int level = to_underlying(ZoomLevel::Min); level <= to_underlying(ZoomLevel::Max); ++level) {
			const ZoomLevel zoom = static_cast<ZoomLevel>(level);
			const auto path = GetTestCaseArtifactPaths(DEMO_FEATURE, this->scenario).images / fmt::format("07-consists-zoom{}.ppm", level);
			CaptureNativeArtCanvas(1280, 733, path, [&] {
				// Eight direction cells, four per row; each cell holds the cutter consist above the hauler consist.
				for (uint8_t direction = 0; direction < 8; ++direction) {
					const int left = (direction % 4) * 320;
					const int top = (direction / 4) * 366;
					DrawConsistCell(cutter_consist, direction, zoom, Rect{left, top, left + 319, top + 220});
					DrawConsistCell(hauler_consist, direction, zoom, Rect{left, top + 221, left + 319, top + 365});
				}
			});
			++this->captures;
		}
		REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	}

	/**
	 * Task 5.5: overlap where the track turns by 45 degrees. The leader has just turned while its trailer, 8 steps
	 * behind along the old heading, still points the old way; the composite is the same raster pair as DEMO_OVERLAP.
	 */
	void MeasureCurvePairs()
	{
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
		GfxClearSpriteCache();
		// Cutter and hopper, hopper pairs and hauler and hopper: every coupling of the two demo consists.
		const std::array<std::pair<size_t, size_t>, 5> pairs{{{0, 4}, {4, 5}, {5, 4}, {6, 4}, {2, 4}}};
		for (const auto &[leader, trailer] : pairs) {
			// Every heading, turning left and right, so a hopper pair is checked on each kind of 45-degree curve.
			for (uint8_t direction = 0; direction < 8; ++direction) {
				for (int turn : {-1, 1}) {
					const uint8_t turned = static_cast<uint8_t>((direction + 8 + turn) % 8);
					const auto first = GetMultiLayerSprite(POSE_FAMILIES[leader].asset, {POSE_FAMILIES[leader].state, 0, turned, 0, POSE_FAMILIES[leader].part});
					const auto second = GetMultiLayerSprite(POSE_FAMILIES[trailer].asset, {POSE_FAMILIES[trailer].state, 0, direction, 0, POSE_FAMILIES[trailer].part});
					if (!first.has_value() || !second.has_value()) continue;
					const auto [dx, dy] = TRAIN_STEP[direction];
					const std::array<std::pair<SpriteID, Point>, 1> front{{{*first, {0, 0}}}};
					const std::array<std::pair<SpriteID, Point>, 1> back{{{*second, RemapCoords(-8 * dx, -8 * dy, 0)}}};
					const auto a = RenderPoses(ZoomLevel::Normal, front);
					const auto b = RenderPoses(ZoomLevel::Normal, back);
					uint overlap = 0;
					for (size_t i = 0; i < a.size(); ++i) overlap += a[i] != 0 && b[i] != 0;
					fmt::print("DEMO_CURVE_OVERLAP leader={} trailer={} direction={} turn={} overlap_pixels={}\n", POSE_FAMILIES[leader].name,
						POSE_FAMILIES[trailer].name, direction, turn, overlap);
				}
			}
		}
		REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
		GfxClearSpriteCache();
	}

	/**
	 * Zoom the main view step by step to a level the settings allow.
	 * @param zoom Wanted level.
	 * @return The level actually reached.
	 */
	static ZoomLevel ZoomMainView(ZoomLevel zoom)
	{
		Window *main = GetMainWindow();
		// Each step changes the zoom by one level; a step the settings refuse ends the walk.
		while (main->viewport->zoom > zoom && DoZoomInOutWindow(ZOOM_IN, main)) {}
		while (main->viewport->zoom < zoom && DoZoomInOutWindow(ZOOM_OUT, main)) {}
		return main->viewport->zoom;
	}

	/**
	 * Task 5.5 in the real game: the cutter consist moving on its track at every zoom the settings allow (depth
	 * sorting and coupler gaps as the viewport draws them), then the depot and train list windows showing the same
	 * vehicles at their list widths.
	 * @param cutter The moving cutter front.
	 */
	void CaptureMovingConsist(const Train &cutter)
	{
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		RelocateAllWindows(1280, 733);
		Window *main = GetMainWindow();
		SetViewportLayer(main, GetTileLayer(cutter.tile));
		const ZoomLevel original = main->viewport->zoom;
		for (int level = to_underlying(ZoomLevel::Min); level <= to_underlying(ZoomLevel::Max); ++level) {
			ZoomLevel reached = ZoomLevel::Normal;
			this->Capture(fmt::format("08-moving-zoom{}", level), [&] {
				reached = ZoomMainView(static_cast<ZoomLevel>(level));
				ScrollMainWindowToTile(cutter.tile, true);
			});
			fmt::print("DEMO_SPACING_CAPTURE zoom_wanted={} zoom_reached={} tile=({},{}) speed={}\n", level, to_underlying(reached), TileX(cutter.tile),
				TilePlaneY(cutter.tile), cutter.cur_speed);
		}
		ZoomMainView(original);
		// The hauler's home depot on the Surface and the company train list show both consists at their list widths.
		const Depot *depot = nullptr;
		for (const Depot *candidate : Depot::Iterate()) {
			if (GetTileOwner(candidate->xy) == cutter.owner && GetTileLayer(candidate->xy) == MapLayer::Surface) depot = candidate;
		}
		this->Capture("09-depot-and-list", [&] {
			if (depot != nullptr) ShowDepotWindow(depot->xy, VehicleType::Train);
			ShowVehicleListWindow(cutter.owner, VehicleType::Train);
		});
		REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	}

	/**
	 * Find the open underground construction palette for a layer, opening it through its owner when needed.
	 * @param layer Work layer of the palette.
	 * @return The real palette, placed at the right edge of the screen.
	 */
	Window &Palette(MapLayer layer)
	{
		Window *palette = FindWindowById(WindowClass::UndergroundConstruction, to_underlying(layer));
		if (palette == nullptr) palette = ShowUndergroundConstructionToolbar(layer, GetCurrentMultiLayerProfileSelection());
		REQUIRE(palette != nullptr);
		palette->left = 1280 - palette->width;
		palette->top = 40;
		return *palette;
	}

	/**
	 * Log the palette's visible prompt and quote rows exactly as the player reads them.
	 * @param palette Real construction palette.
	 * @param step Name of the input step just performed.
	 */
	void LogPalette(const Window &palette, std::string_view step) const
	{
		fmt::print("DEMO_PALETTE step={} placing={} confirm_disabled={}\n", step, _thd.GetCallbackWnd() == &palette, palette.IsWidgetDisabled(WID_UC_AREA_CONFIRM));
		for (WidgetID widget : {WID_UC_STATUS, WID_UC_AREA_PRICE, WID_UC_AREA_MATERIAL, WID_UC_AREA_TIME, WID_UC_AREA_APPROACH, WID_UC_AREA_CHANGE, WID_UC_AREA_FLOOR}) {
			fmt::print("DEMO_PALETTE_TEXT step={} widget={} text={}\n", step, widget, palette.GetWidgetString(widget, palette.GetWidget<NWidgetCore>(widget)->GetString()));
		}
	}

	/**
	 * Click a world tile through the main viewport, as a player placing a tool does.
	 * @param tile Explicit-layer world tile, centred on screen first.
	 */
	static void ClickTile(TileIndex tile)
	{
		// The pointer travels to the tile first, which dismisses any tooltip from earlier hovering.
		CloseWindowByClass(WindowClass::ToolTips);
		SetViewportLayer(GetMainWindow(), GetTileLayer(tile));
		ScrollMainWindowToTile(tile, true);
		UpdateViewportPosition(GetMainWindow(), 0);
		const Point point = FeedbackTilePoint(tile);
		FeedbackMouse(point, false);
		FeedbackMouse(point, true);
		FeedbackMouse(point, false);
	}

	/**
	 * Drag a rectangular floor between two world tiles through the main viewport.
	 * @param first Tile where the drag starts.
	 * @param last Tile where the drag ends.
	 */
	static void DragTiles(TileIndex first, TileIndex last)
	{
		// The pointer travels to the tile first, which dismisses any tooltip from earlier hovering.
		CloseWindowByClass(WindowClass::ToolTips);
		// Centre the request so both corners are on screen and outside the palette on the right.
		SetViewportLayer(GetMainWindow(), GetTileLayer(first));
		ScrollMainWindowToTile(first, true);
		UpdateViewportPosition(GetMainWindow(), 0);
		const Point from = FeedbackTilePoint(first);
		const Point to = FeedbackTilePoint(last);
		FeedbackMouse(from, false);
		FeedbackMouse(from, true);
		FeedbackMouse(to, true);
		FeedbackMouse(to, false);
	}

	/**
	 * The screenshot-05 request: one row of cells starting in front of the rock-side housing's door and running
	 * sideways, at a right angle to the exit, so the cutter would need a bent approach to reach it.
	 * @param anchor Rock-side portal housing of the project pair.
	 * @return First and last corner of the requested floor.
	 */
	static std::pair<TileIndex, TileIndex> BesideExitArea(TileIndex anchor)
	{
		const DiagDirection door = GetLayerPortalDoorDirection(anchor);
		const DiagDirection side = ChangeDiagDir(door, DiagDirDiff::Right90);
		const TileIndex first = TileAddByDiagDir(anchor, door);
		TileIndex last = first;
		for (uint step = 0; step < 7; ++step) last = TileAddByDiagDir(last, side);
		return {first, last};
	}

	/**
	 * Log every fact of a commissioning quote: the refusal, the outline, authorized cells, failing tile and plan.
	 * @param step Name of the request.
	 * @param quote Quote returned by the owning preview.
	 */
	static void LogCommissionQuote(std::string_view step, const TBMCommissionAreaPreview &quote)
	{
		const auto &area = quote.area;
		fmt::print("DEMO_QUOTE step={} purchase_ok={} purchase_cost={} message=\"{}\" area_error={} consist_error={} failure_tile={}({},{}) partial={} route={} covered={} remaining={} turn_envelope={} authorized={} requested={} earth={} rock={}\n",
			step, quote.purchase.Succeeded(), quote.purchase.GetCost().base(), quote.purchase.Succeeded() ? "" : GetString(quote.purchase.GetErrorMessage()),
			to_underlying(area.error), to_underlying(area.consist_error), area.failure_tile.base(), IsValidTile(area.failure_tile) ? TileX(area.failure_tile) : 0,
			IsValidTile(area.failure_tile) ? TilePlaneY(area.failure_tile) : 0, area.partial, area.plan.route.size(), area.plan.covered_targets.size(),
			area.plan.remaining_targets.size(), area.turn_envelope.size(), area.proposal.authorized_tiles.size(), area.proposal.requested_tiles.size(),
			area.totals.earth_t, area.totals.rock_t);
		std::string cells;
		for (TileIndex tile : area.proposal.authorized_tiles) cells += fmt::format("({},{})", TileX(tile), TilePlaneY(tile));
		fmt::print("DEMO_QUOTE_AUTHORIZED step={} cells={}\n", step, cells);
		cells.clear();
		for (TileIndex tile : area.plan.remaining_targets) cells += fmt::format("({},{})", TileX(tile), TilePlaneY(tile));
		fmt::print("DEMO_QUOTE_REMAINING step={} cells={}\n", step, cells);
	}

	/**
	 * Task 1.3, at the user's progress: with project 0 still working at 22 of 50 cuts (screenshot 04), record the release
	 * prerequisite and New area on the project portal (R6; since task 4.4 it pauses the project and opens Pause and
	 * extend instead of refusing), quote the beside-exit area as an extension (R8), then press Return and park. Runs
	 * inside an observed tick and returns to pixel-free simulation afterwards.
	 * @param project The demo project.
	 */
	void StartNewArea(const TBMProject &project)
	{
		const ExcavationJob *job = GetExcavationJob(project.job);
		REQUIRE(job != nullptr);
		const TileIndex anchor = job->commissioning_anchor;
		const MapLayer layer = GetTileLayer(anchor);
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		RelocateAllWindows(1280, 733);
		// Show the rock layer first: changing the main view's layer later would cancel an armed placement tool.
		SetViewportLayer(GetMainWindow(), layer);
		Window &palette = this->Palette(layer);
		this->LogPalette(palette, "opened");
		const StringID release = GetTBMProjectReleaseProblem(project.id);
		fmt::print("DEMO_FINDING report=R6 phase={} release_problem=\"{}\"\n", to_underlying(project.phase), release == INVALID_STRING_ID ? "" : GetString(release));
		this->Capture("04-projects-release-text", [&] { this->OpenProjectsList(layer); });
		this->Capture("04-new-area-on-project-portal", [&] {
			PlayerClick(this->Palette(layer), WID_UC_AREA);
			ClickTile(anchor);
		});
		this->LogPalette(this->Palette(layer), "new-area-on-project-portal");
		ResetObjectToPlace();
		const auto [first, last] = BesideExitArea(anchor);
		Window *browser = FindWindowById(WindowClass::TBMProjects, to_underlying(layer));
		REQUIRE(browser != nullptr);
		// The same request through the extension flow: Pause and extend (the project is already paused by New area since
		// task 4.4) arms the area tool.
		this->Capture("05-extension-beside-exit", [&] {
			PlayerClick(*browser, WID_TP_EXTEND);
			DragTiles(first, last);
		});
		this->LogPalette(this->Palette(layer), "extension-beside-exit");
		const auto extension = PreviewTBMExtension(project.company, project.id, first, last);
		fmt::print("DEMO_EXTENSION_QUOTE ok={} message=\"{}\" plan_error={} partial={} failure_tile={} route={} covered={} remaining={}\n", extension.result.Succeeded(),
			extension.result.Succeeded() ? "" : GetString(extension.result.GetErrorMessage()), to_underlying(extension.plan.error), extension.plan.partial,
			extension.plan.failure_tile.base(), extension.plan.route.size(), extension.plan.covered_targets.size(), extension.plan.remaining_targets.size());
		ResetObjectToPlace();
		// Resume the paused project, then ask for the return and park as the user did.
		PlayerClick(*browser, WID_TP_RESUME);
		PlayerClick(*browser, WID_TP_PARK);
		fmt::print("DEMO_STEP tick={} name=return-and-park phase={} blocked={}\n", this->ticks, to_underlying(GetTBMProject(project.id)->phase),
			to_underlying(GetTBMProject(project.id)->blocked));
		this->area_stage = AreaStage::Parking;
		REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	}

	/**
	 * Task 1.3, during simulation: release the parked project as the user did, then plan the beside-exit area at the
	 * same portal for a new machine (screenshot 05) once the release completed.
	 */
	void StepNewArea()
	{
		const TBMProject *project = GetTBMProject(this->project_id);
		if (this->area_stage == AreaStage::Parking && project != nullptr && IsTBMStored(project->phase)) {
			// Finish/cancel and release is only enabled once the whole machine stands parked and empty and the
			// hauler delivered the last spoil; until then the player reads the prerequisite and waits, as the user did.
			const StringID problem = GetTBMProjectReleaseProblem(project->id);
			const std::string wait = fmt::format("phase={} blocked={} problem=\"{}\"", to_underlying(project->phase), to_underlying(project->blocked),
				problem == INVALID_STRING_ID ? "" : GetString(problem));
			if (wait != this->release_wait) {
				this->release_wait = wait;
				fmt::print("DEMO_RELEASE_WAIT tick={} {}\n", this->ticks, wait);
			}
			if (problem != INVALID_STRING_ID) return;
			const MapLayer layer = GetTileLayer(GetExcavationJob(project->job)->commissioning_anchor);
			Window *browser = FindWindowById(WindowClass::TBMProjects, to_underlying(layer));
			if (browser == nullptr) browser = &this->OpenProjectsList(layer);
			this->Capture("04-parked", [] {});
			const Rect button = browser->GetWidget<NWidgetBase>(WID_TP_RELEASE)->GetCurrentRect();
			const Window *under = FindWindowFromPt(browser->left + (button.left + button.right) / 2, browser->top + (button.top + button.bottom) / 2);
			fmt::print("DEMO_RELEASE_CHECK tick={} retire_error={} button_disabled={} under_click={} browser={}\n", this->ticks,
				to_underlying(ControlTBMProject(project->id, TBMProjectAction::Retire, false)), browser->IsWidgetDisabled(WID_TP_RELEASE),
				under == nullptr ? -1 : static_cast<int>(under->window_class), static_cast<int>(browser->window_class));
			PlayerClick(*browser, WID_TP_RELEASE);
			const TBMProject *after = GetTBMProject(this->project_id);
			fmt::print("DEMO_STEP tick={} name=finish-and-release project_left={} phase={}\n", this->ticks, after == nullptr, after == nullptr ? -1 : to_underlying(after->phase));
			this->area_stage = AreaStage::Releasing;
			return;
		}
		if (this->area_stage != AreaStage::Releasing || project != nullptr) return;
		// The machine was released: plan the beside-exit area at the same portal for a new project, as in screenshot 05.
		AutoRestoreBackup actor(_current_company, this->company);
		Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true);
		const MapLayer layer = GetTileLayer(this->anchor);
		const auto [first, last] = BesideExitArea(this->anchor);
		this->Capture("05-armed", [&] {
			PlayerClick(this->Palette(layer), WID_UC_AREA);
			ClickTile(this->anchor);
		});
		this->LogPalette(this->Palette(layer), "portal");
		// Since task 9.7 the request is accepted with its approach; the "approach" scenario keeps that quote as its first capture.
		this->Capture(this->scenario == "approach" ? "05-approach-quote" : "05-cant-dig", [&] { DragTiles(first, last); });
		this->LogPalette(this->Palette(layer), "beside-exit");
		const auto origin = GetLayerPortalDefaultTBMOrigin(this->portal, this->company);
		const auto consist = GetDefaultTBMConsist(layer);
		const auto quote = PreviewTBMCommissionArea(this->company, this->portal, GetLayerPortalDefaultTBMModel(this->portal), consist.earth_wagons, consist.rock_wagons,
			origin, first, last, false, ExcavationPlanIntent::Area);
		LogCommissionQuote("beside-exit", quote);
		fmt::print("DEMO_FINDING report=R8 area=({},{})-({},{}) door={}\n", TileX(first), TilePlaneY(first), TileX(last), TilePlaneY(last),
			to_underlying(GetLayerPortalDoorDirection(this->anchor)));
		this->area_stage = AreaStage::Released;
		if (this->scenario == "new-area") {
			this->Finish();
			return;
		}
		if (this->scenario == "approach") {
			this->StartApproach(quote);
			return;
		}
		this->StartProjectOne(first, last);
	}

	/**
	 * Task 9.7: confirm the beside-exit request, now accepted with its approach cells, through the palette as a player
	 * does; give the new project the released hauler, then let the simulation drive it.
	 * @param quote The beside-exit quote the palette shows.
	 */
	void StartApproach(const TBMCommissionAreaPreview &quote)
	{
		const MapLayer layer = GetTileLayer(this->anchor);
		this->approach_tiles.insert(quote.area.approach.tiles.begin(), quote.area.approach.tiles.end());
		std::string headings;
		for (const auto &cut : quote.area.plan.route) headings += fmt::format("({},{})h{} ", TileX(cut.cutter_position), TilePlaneY(cut.cutter_position), to_underlying(cut.heading));
		fmt::print("DEMO_APPROACH tick={} cuts={} approach_cells={} approach_cost={} total_cost={} forbid_90={} route={}\n", this->ticks, quote.area.plan.route.size(),
			quote.area.approach.tiles.size(), quote.area.approach.estimate.cost.base(), quote.area.totals.cost.base(), _settings_game.pf.forbid_90_deg, headings);
		REQUIRE(quote.purchase.Succeeded());
		REQUIRE_FALSE(quote.area.approach.tiles.empty());
		Window &palette = this->Palette(layer);
		this->LogPalette(palette, "approach-quote");
		REQUIRE_FALSE(palette.IsWidgetDisabled(WID_UC_AREA_CONFIRM));
		PlayerClick(palette, WID_UC_AREA_CONFIRM);
		const auto projects = CaptureTBMProjects();
		REQUIRE(projects.size() == 1);
		this->project_id = projects.front().id;
		AutoRestoreBackup actor(_current_company, this->company);
		// The released hauler is an ordinary train again; the player assigns it to the new project.
		for (const Train *train : Train::Iterate()) {
			if (!train->IsFrontEngine() || train->index == projects.front().cutter || train->owner != this->company) continue;
			const auto assigned = Command<Commands::AssignTBMHauler>::Do(DoCommandFlag::Execute, this->project_id, train->index);
			fmt::print("DEMO_STEP tick={} name=assign-hauler train={} ok={}\n", this->ticks, train->index.base(), assigned.Succeeded());
			if (assigned.Succeeded() && train->vehstatus.Test(VehState::Stopped)) Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, train->index, false);
			if (assigned.Succeeded()) break;
		}
		const Train *cutter = Train::Get(projects.front().cutter);
		if (cutter->vehstatus.Test(VehState::Stopped)) Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, cutter->index, false);
		fmt::print("DEMO_STEP tick={} name=approach-project id={} cutter={} route={}\n", this->ticks, this->project_id.base(), cutter->index.base(),
			GetExcavationJob(projects.front().job)->route.size());
		Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false);
	}

	/**
	 * Task 9.7 during simulation: every part of the approach project's consist on the rock layer must stand on rail it
	 * may use (its housing or its company's track: the paid support). The first bend and the finished area are captured.
	 */
	void StepApproach()
	{
		if (this->area_stage != AreaStage::Released) return;
		const TBMProject *project = GetTBMProject(this->project_id);
		if (project == nullptr) return;
		const ExcavationJob *job = GetExcavationJob(project->job);
		const Train *cutter = Train::GetIfValid(project->cutter);
		if (job == nullptr || cutter == nullptr) return;
		const MapLayer layer = GetTileLayer(this->anchor);
		// Parts still in the shaft or on the Surface are outside the dug corridor; every part below stands on usable rail.
		for (const Train *part = cutter; part != nullptr; part = part->Next()) {
			if (GetTileLayer(part->tile) != layer || IsTBMSupportUsable(*project, part->tile)) continue;
			if (this->approach_off_support++ < 10) fmt::print("DEMO_APPROACH_OFF_SUPPORT tick={} part={}\n", this->ticks, DescribePlacement(*part));
		}
		// The first committed cut whose heading differs from the exit's is the first bend.
		const bool bent = job->cursor > 0 && job->route[job->cursor - 1].heading != job->route.front().heading;
		if (bent && !this->approach_bent) {
			this->approach_bent = true;
			fmt::print("DEMO_STEP tick={} name=approach-bend cursor={} cutter={}\n", this->ticks, job->cursor, DescribePlacement(*cutter));
			this->Capture("06-approach-bend", [&] {
				SetViewportLayer(GetMainWindow(), layer);
				ScrollMainWindowToTile(cutter->tile, true);
			});
		}
		if (job->state != ExcavationJobState::Completed) return;
		// The area is complete: every accepted cut committed. Count the requested floor that is open now.
		size_t open = 0;
		for (TileIndex tile : job->requested_tiles) {
			if (GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated) ++open;
		}
		size_t approach_open = 0;
		for (TileIndex tile : this->approach_tiles) {
			if (GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated) ++approach_open;
		}
		fmt::print("DEMO_FINDING report=R8 approach=1 cuts={} floor_open={}/{} approach_open={}/{} off_support={} bent={} ticks={}\n", job->route.size(), open,
			job->requested_tiles.size(), approach_open, this->approach_tiles.size(), this->approach_off_support, this->approach_bent, this->ticks);
		this->Capture("07-area-done", [&] {
			SetViewportLayer(GetMainWindow(), layer);
			ScrollMainWindowToTile(job->requested_tiles.front(), true);
		});
		this->Finish();
	}

	/**
	 * Task 12.6: find a small floor near the face that the new route reaches by bending away from the planned next cut.
	 * Candidates lie ahead of the last completed cut, to either side, nearest first; the owner's quote decides.
	 * @param project The demo project.
	 * @return The first accepted floor's corners and its quote, or nothing when none is accepted.
	 */
	std::optional<std::pair<std::pair<TileIndex, TileIndex>, TBMWorkChangePreview>> FindChangeArea(const TBMProject &project) const
	{
		const ExcavationJob *job = GetExcavationJob(project.job);
		if (job == nullptr || job->cursor == 0 || job->cursor >= job->route.size()) return std::nullopt;
		const auto &last = job->route[job->cursor - 1];
		const auto ahead = TileIndexDiffCByDir(last.heading);
		// Each candidate is two cells across, some cells ahead and to one side; its quote must turn off the planned path.
		for (int distance : {3, 4, 5}) {
			for (int offset : {2, -2, 3, -3}) {
				const auto side = TileIndexDiffCByDir(static_cast<Direction>((to_underlying(last.heading) + (offset > 0 ? 2 : 6)) % 8));
				const int across = std::abs(offset);
				const TileIndex first = TileAddWrap(last.cutter_position, distance * ahead.x + across * side.x, distance * ahead.y + across * side.y);
				const TileIndex second = first == INVALID_TILE ? INVALID_TILE : TileAddWrap(first, side.x, side.y);
				if (second == INVALID_TILE) continue;
				auto quote = PreviewTBMWorkChange(project.company, project.id, first, second);
				fmt::print("DEMO_CHANGE_CANDIDATE area=({},{})-({},{}) ok={} message=\"{}\" turns={}\n", TileX(first), TilePlaneY(first), TileX(second), TilePlaneY(second),
					quote.result.Succeeded(), quote.result.Succeeded() ? "" : GetString(quote.result.GetErrorMessage()),
					quote.result.Succeeded() && quote.plan.route.front() != job->route[job->cursor]);
				if (quote.result.Succeeded() && quote.plan.route.front() != job->route[job->cursor]) return std::make_pair(std::make_pair(first, second), std::move(quote));
			}
		}
		return std::nullopt;
	}

	/**
	 * Task 12.6: change the demo project's work area as a player does, through Change work area in the projects window
	 * and a drag in the palette, then confirm. No pause is posted, and the machine keeps its phase.
	 * @param project The demo project.
	 * @param name Capture prefix and log name of this change.
	 * @return Whether a floor was found and the change confirmed.
	 */
	bool ChangeWorkArea(const TBMProject &project, std::string_view name)
	{
		const auto found = this->FindChangeArea(project);
		if (!found.has_value()) return false;
		const auto &[area, quote] = *found;
		const ExcavationJob &job = *GetExcavationJob(project.job);
		const MapLayer layer = GetTileLayer(job.commissioning_anchor);
		const TBMPhase phase = project.phase;
		const TBMBlockedReason blocked = project.blocked;
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		RelocateAllWindows(1280, 733);
		SetViewportLayer(GetMainWindow(), layer);
		Window &browser = this->OpenProjectsList(layer);
		this->Capture(fmt::format("{}-armed", name), [&] { PlayerClick(browser, WID_TP_CHANGE); });
		this->Capture(fmt::format("{}-quote", name), [&] { DragTiles(area.first, area.second); });
		Window &palette = this->Palette(layer);
		this->LogPalette(palette, name);
		std::string headings;
		for (const auto &cut : quote.plan.route) headings += fmt::format("({},{})h{} ", TileX(cut.cutter_position), TilePlaneY(cut.cutter_position), to_underlying(cut.heading));
		REQUIRE_FALSE(palette.IsWidgetDisabled(WID_UC_AREA_CONFIRM));
		PlayerClick(palette, WID_UC_AREA_CONFIRM);
		const TBMProject &after = *GetTBMProject(project.id);
		const ExcavationJob &changed = *GetExcavationJob(after.job);
		const bool next_is_new = changed.cursor == quote.cursor && changed.route[changed.cursor] == quote.plan.route.front();
		fmt::print("DEMO_CHANGE name={} tick={} cursor={} kept={} dropped={} new={} abandoned={}/{} phase={}->{} blocked={}->{} next_is_new={} route={}\n", name, this->ticks,
			changed.cursor, quote.cursor, quote.dropped_cuts, quote.plan.route.size(), quote.abandoned_work_ticks, quote.cut_work_ticks, to_underlying(phase),
			to_underlying(after.phase), to_underlying(blocked), to_underlying(after.blocked), next_is_new, headings);
		REQUIRE(next_is_new);
		REQUIRE(after.phase == phase); // The machine keeps its phase: nothing physical is undone.
		REQUIRE(after.blocked != TBMBlockedReason::Paused); // And no pause was posted.
		this->change_cursor = changed.cursor;
		this->change_first = quote.plan.route.front();
		REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		return true;
	}

	/**
	 * Task 12.6 during simulation. While the demo cutter bores past its 22nd cut, change its work area and save a
	 * checkpoint just after; once the next cut completes, check it was the new route's first. Then return and park,
	 * change the area again while parked, resume, and check the cut completed after resuming.
	 * @param project The demo project.
	 */
	void StepChange(const TBMProject &project)
	{
		const ExcavationJob *job = GetExcavationJob(project.job);
		if (job == nullptr) return;
		const MapLayer layer = GetTileLayer(job->commissioning_anchor);
		switch (this->change_stage) {
			case ChangeStage::None:
				// Change while the machine is really boring at its face, as the user wished to (R6).
				if (job->cursor < this->area_cursor || project.phase != TBMPhase::Bore || job->partial_work_ticks == 0) return;
				if (!this->ChangeWorkArea(project, "08-change-boring")) return;
				this->RequestSave("change-work"); // A reload must continue with the same next cut.
				this->change_stage = ChangeStage::Changed;
				return;
			case ChangeStage::Changed: {
				if (job->cursor <= this->change_cursor) return;
				if (!this->change_next_checked) {
					const auto &cut = job->route[this->change_cursor];
					bool open = true;
					for (TileIndex tile : cut.swept_tiles) open &= GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated;
					fmt::print("DEMO_FINDING report=R6 change=boring next_cut_new={} opened={} cursor={} ticks={}\n", cut == this->change_first, open, job->cursor, this->ticks);
					REQUIRE(cut == this->change_first);
					this->change_next_checked = true;
				}
				// The cutter must also drive onto the new track and complete the following cut, before it is parked.
				if (job->cursor < this->change_cursor + 2 && job->state != ExcavationJobState::Completed) return;
				fmt::print("DEMO_FINDING report=R6 change=boring advanced=1 cursor={} ticks={}\n", job->cursor, this->ticks);
				if (job->state == ExcavationJobState::Completed) {
					this->Finish();
					return;
				}
				Window &browser = this->OpenProjectsList(layer);
				PlayerClick(browser, WID_TP_PARK);
				fmt::print("DEMO_STEP tick={} name=return-and-park phase={}\n", this->ticks, to_underlying(GetTBMProject(project.id)->phase));
				this->change_stage = ChangeStage::Parking;
				return;
			}
			case ChangeStage::Parking: {
				if (!IsTBMStored(project.phase)) return;
				if (!this->ChangeWorkArea(project, "09-change-parked")) {
					fmt::print("DEMO_FINDING report=R6 change=parked found=0 cursor={}\n", job->cursor);
					this->Finish();
					return;
				}
				Window &browser = this->OpenProjectsList(layer);
				PlayerClick(browser, WID_TP_RESUME);
				fmt::print("DEMO_STEP tick={} name=resume phase={}\n", this->ticks, to_underlying(GetTBMProject(project.id)->phase));
				this->RequestSave("change-parked"); // A reload must drive back to the same face and cut.
				this->change_stage = ChangeStage::Resumed;
				return;
			}
			case ChangeStage::Resumed: {
				if (job->cursor <= this->change_cursor) return;
				const auto &cut = job->route[this->change_cursor];
				fmt::print("DEMO_FINDING report=R6 change=parked next_cut_new={} cursor={} ticks={}\n", cut == this->change_first, job->cursor, this->ticks);
				REQUIRE(cut == this->change_first);
				this->Capture("10-change-resumed", [&] {
					SetViewportLayer(GetMainWindow(), layer);
					ScrollMainWindowToTile(cut.cutter_position, true);
				});
				this->Finish();
				return;
			}
		}
	}

	/**
	 * Task 1.4, screenshot 06: commission a second project at the same portal through the palette (the released
	 * machine is reused), force the former hauler out of its depot as the user did, then let the simulation run.
	 * @param refused_first First corner of the refused beside-exit request, still shown in the palette.
	 * @param refused_last Last corner of that request.
	 */
	void StartProjectOne(TileIndex refused_first, TileIndex refused_last)
	{
		const MapLayer layer = GetTileLayer(this->anchor);
		const auto origin = GetLayerPortalDefaultTBMOrigin(this->portal, this->company);
		const auto consist = GetDefaultTBMConsist(layer);
		const uint16_t model = GetLayerPortalDefaultTBMModel(this->portal);
		// Candidate requests in the order a player would try them after the refusal: a straight row in front of the
		// door, the refused row moved one cell further out, then floors that start at the door cell (a request must be
		// connected to the portal) and widen beside project 0's corridor on either side, as screenshot 06's outlined
		// block does. The first one the owner accepts is used.
		const DiagDirection door = GetLayerPortalDoorDirection(this->anchor);
		const DiagDirection right = ChangeDiagDir(door, DiagDirDiff::Right90);
		const DiagDirection left = ChangeDiagDir(door, DiagDirDiff::Left90);
		/** @param from Start tile. @param dir Direction. @param steps Cells to walk. @return The tile reached. */
		const auto walk = [](TileIndex from, DiagDirection dir, uint steps) {
			for (uint step = 0; step < steps; ++step) from = TileAddByDiagDir(from, dir);
			return from;
		};
		const TileIndex ahead_first = walk(this->anchor, door, 2);
		const TileIndex door_cell = walk(this->anchor, door, 1);
		const std::array<std::pair<TileIndex, TileIndex>, 8> candidates{{{ahead_first, walk(ahead_first, door, 3)},
			{TileAddByDiagDir(refused_first, door), TileAddByDiagDir(refused_last, door)},
			{door_cell, walk(walk(door_cell, door, 7), left, 3)},
			{door_cell, walk(walk(door_cell, door, 7), right, 3)},
			{door_cell, walk(walk(door_cell, door, 11), left, 7)},
			{door_cell, walk(walk(door_cell, door, 11), right, 7)},
			{walk(door_cell, left, 4), walk(walk(door_cell, door, 9), right, 4)},
			{door_cell, walk(door_cell, door, this->area_cursor + 4)}}};
		std::optional<std::pair<TileIndex, TileIndex>> accepted;
		// Every candidate is quoted and logged, so the refusals of the ones a player would try first are on record too.
		for (const auto &[first, last] : candidates) {
			const auto quote = PreviewTBMCommissionArea(this->company, this->portal, model, consist.earth_wagons, consist.rock_wagons, origin, first, last, false, ExcavationPlanIntent::Area);
			LogCommissionQuote(fmt::format("candidate-({},{})-({},{})", TileX(first), TilePlaneY(first), TileX(last), TilePlaneY(last)), quote);
			if (quote.purchase.Succeeded() && !accepted.has_value()) accepted = std::pair{first, last};
		}
		REQUIRE(accepted.has_value());
		Window &palette = this->Palette(layer);
		// The palette enables Confirm while it paints the quote; the capture draws the frame the player reads first.
		this->Capture("05-project-one-quote", [&] { DragTiles(accepted->first, accepted->second); });
		this->LogPalette(palette, "project-one-quote");
		REQUIRE_FALSE(palette.IsWidgetDisabled(WID_UC_AREA_CONFIRM));
		PlayerClick(palette, WID_UC_AREA_CONFIRM);
		const auto projects = CaptureTBMProjects();
		REQUIRE(projects.size() == 1);
		this->project_id = projects.front().id;
		this->previous_cutter.clear();
		fmt::print("DEMO_STEP tick={} name=project-one id={} cutter={} phase={} blocked={}\n", this->ticks, projects.front().id.base(), projects.front().cutter.base(),
			to_underlying(projects.front().phase), to_underlying(projects.front().blocked));
		AutoRestoreBackup actor(_current_company, this->company);
		// The released hauler is now an ordinary train. When the depot rule still holds it, force it out as the user did;
		// StepObstruction stops it after the configured delay.
		for (const Train *train : Train::Iterate()) {
			if (!train->IsFrontEngine() || train->index == projects.front().cutter) continue;
			this->former_hauler = train->index;
			if (train->vehstatus.Test(VehState::Stopped)) Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, train->index, false);
			if (train->IsInDepot()) {
				const auto forced = Command<Commands::ForceTrainProceed>::Do(DoCommandFlag::Execute, train->index);
				fmt::print("DEMO_STEP tick={} name=force-hauler train={} ok={} at={}\n", this->ticks, train->index.base(), forced.Succeeded(), DescribePlacement(*train));
			}
			this->forced_at = this->ticks;
		}
		const Train *cutter = Train::Get(projects.front().cutter);
		if (cutter->vehstatus.Test(VehState::Stopped)) Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, cutter->index, false);
		Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false);
	}

	/**
	 * Task 1.4 during simulation: stop the forced hauler after the configured delay, and retain screenshot 06 once the
	 * new project's cutter reports an obstruction for 300 consecutive ticks (or report that it reached its face).
	 */
	void StepObstruction()
	{
		if (this->area_stage != AreaStage::Released) return;
		const TBMProject *project = GetTBMProject(this->project_id);
		if (project == nullptr) return;
		AutoRestoreBackup actor(_current_company, this->company);
		const Train *hauler = Train::GetIfValid(this->former_hauler);
		// The user stopped the forced hauler somewhere on its way; the delay selects where it stands when stopped.
		if (hauler != nullptr && !this->hauler_stopped && this->ticks >= this->forced_at + this->hauler_stop_delay) {
			Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, hauler->index, false);
			this->hauler_stopped = true;
			fmt::print("DEMO_STEP tick={} name=stop-hauler train={} front={} tail={}\n", this->ticks, hauler->index.base(), DescribePlacement(*hauler), DescribePlacement(*hauler->Last()));
		}
		this->obstruction_ticks = project->blocked == TBMBlockedReason::Obstruction ? this->obstruction_ticks + 1 : 0;
		const ExcavationJob *job = GetExcavationJob(project->job);
		const Train *cutter = Train::Get(project->cutter);
		if (this->obstruction_ticks >= 300) {
			fmt::print("DEMO_FINDING report=R9 reproduced=1 cutter={} status=\"{}\"\n", DescribePlacement(*cutter), GetTBMVehicleStatus(*cutter).value_or(""));
			this->Capture("06-something-in-the-way", [&] {
				SetViewportLayer(GetMainWindow(), GetTileLayer(cutter->tile));
				ScrollMainWindowToTile(cutter->tile, true);
				ShowVehicleViewWindow(cutter);
			});
			this->Finish();
		} else if (job != nullptr && job->cursor > 0) {
			// Reaching the first cut means this run's hauler position did not block the cutter.
			fmt::print("DEMO_FINDING report=R9 reproduced=0 cursor={} cutter={}\n", job->cursor, DescribePlacement(*cutter));
			this->Finish();
		}
	}

	/**
	 * Log the main view's layer and the small map's caption (which names its own layer) after an input step.
	 * @param step Name of the input step just performed.
	 * @param map The open small map.
	 */
	void LogMapLayers(std::string_view step, const Window &map) const
	{
		// The window system delivers scheduled invalidations at the start of its next frame; do that before reading.
		for (Window *w : Window::Iterate()) w->ProcessScheduledInvalidations();
		fmt::print("DEMO_MAP step={} main_layer={} smallmap=\"{}\" size={}x{} min={}x{}\n", step, to_underlying(GetMainWindow()->viewport->layer),
			map.GetWidgetString(WID_SM_CAPTION, STR_SMALLMAP_CAPTION), map.width, map.height, map.nested_root->smallest_x, map.nested_root->smallest_y);
	}

	/**
	 * Task 1.7 (R1-R3): open the small map on Underground contours, shrink it to its minimum, then press the layer
	 * hotkeys through the native key dispatcher and record which view follows; finally capture the contour legend of
	 * every layer through the small map's own layer button.
	 */
	void ExerciseMap()
	{
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		RelocateAllWindows(1280, 733);
		SetViewportLayer(GetMainWindow(), MapLayer::Underground);
		ShowSmallMap();
		Window *map = FindWindowById(WindowClass::SmallMap, 0);
		REQUIRE(map != nullptr);
		map->left = 0;
		map->top = 30;
		PlayerClick(*map, WID_SM_CONTOUR);
		this->LogMapLayers("opened", *map);
		// Ask for a window far shorter than allowed; the window system clamps it to the real minimum height.
		ResizeWindow(map, 0, -2000);
		this->LogMapLayers("shrunk", *map);
		this->Capture("01-smallmap-min-underground");
		// The layer hotkeys go through HandleKeypress exactly as a key press does: windows, toolbar, then global hotkeys.
		HandleKeypress('N', 0);
		this->LogMapLayers("hotkey-N", *map);
		for (char digit : {'1', '2', '3', '4'}) {
			HandleKeypress(digit | WKC_ALT, 0);
			this->LogMapLayers(fmt::format("hotkey-alt-{}", digit), *map);
		}
		this->Capture("01-smallmap-after-hotkeys");
		// The small map's own layer dropdown previews each allocated layer; choosing an item reaches the window's
		// dropdown callback with the layer's storage value, as a click on the open list does. Capture each legend.
		for (uint step = 0; step < Map::LayerCount(); ++step) {
			PlayerClick(*map, WID_SM_MAP_LAYER);
			map->OnDropdownSelect(WID_SM_MAP_LAYER, static_cast<int>(step), 0);
			CloseWindowByClass(WindowClass::DropdownMenu);
			this->LogMapLayers(fmt::format("own-layer-{}", step), *map);
			this->Capture(fmt::format("01-smallmap-legend-{}", step));
		}
		REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	}

	/**
	 * Log which windows are open, which one Escape or a toolbar key would close, and whether a tool is armed.
	 * @param step Name of the key press just made.
	 */
	void LogWindows(std::string_view step) const
	{
		const Window *front = FindFrontmostClosableWindow(false);
		const Window *options = FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions);
		const QueryString *filter = options == nullptr ? nullptr : options->GetQueryString(WID_GO_FILTER);
		fmt::print("DEMO_WINDOWS step={} front={}/{} options={} smallmap={} palette={} placement={} editbox_focus={} filter=\"{}\"\n", step,
			front == nullptr ? -1 : static_cast<int>(front->window_class), front == nullptr ? -1 : static_cast<int32_t>(front->window_number),
			options != nullptr, FindWindowById(WindowClass::SmallMap, 0) != nullptr,
			FindWindowById(WindowClass::UndergroundConstruction, MapLayer::Underground) != nullptr, IsObjectPlacementActive(), EditBoxInGlobalFocus(),
			filter == nullptr ? std::string_view{} : filter->text.GetText());
	}

	/**
	 * Press one key through the native dispatcher (edit box, windows front to back with the real main window's global
	 * list, main toolbar) and log the result.
	 * @param step Name for the log.
	 * @param keycode Key with modifiers, as the video driver reports it.
	 * @param key Character the key types, for edit boxes.
	 */
	void PressWindowKey(std::string_view step, uint keycode, char32_t key = 0)
	{
		HandleKeypress(keycode, key);
		Window::DeleteClosedWindows();
		this->LogWindows(step);
	}

	/**
	 * Group 3 (Q2, D10) on the real demo game: F2 toggles the game options, a covered small map is raised before its
	 * key closes it, Escape aborts the underground palette's tool before it closes the palette, and a focused filter
	 * box keeps Escape until it is empty and unfocused. Uses the player's real main window and global hotkeys.
	 */
	void ExerciseWindows()
	{
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		RelocateAllWindows(1280, 733);
		fmt::print("DEMO_WINDOWS_SETTINGS hotkey_toggles_window={} escape_closes_window={}\n", _settings_client.gui.hotkey_toggles_window,
			to_underlying(_settings_client.gui.escape_closes_window));
		const auto options_open = [] { return FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions) != nullptr; };

		// Q2: F2 shows the game options and F2 again hides them.
		this->Capture("03-f2-open", [&] { this->PressWindowKey("f2-open", WKC_F2); });
		CHECK(options_open());
		this->PressWindowKey("f2-close", WKC_F2);
		CHECK_FALSE(options_open());

		// A covered window is raised first: the small map behind the town directory comes to the front, then closes.
		ShowSmallMap();
		this->PressWindowKey("f5-over-smallmap", WKC_F5);
		REQUIRE(FindWindowById(WindowClass::TownDirectory, 0) != nullptr);
		this->PressWindowKey("m-raise", 'M');
		CHECK(FindFrontmostClosableWindow(false) == FindWindowById(WindowClass::SmallMap, 0));
		this->PressWindowKey("m-close", 'M');
		CHECK(FindWindowById(WindowClass::SmallMap, 0) == nullptr);
		CHECK(FindWindowById(WindowClass::TownDirectory, 0) != nullptr);
		// Escape through the real global hotkey: the town directory is now the front-most window.
		this->PressWindowKey("escape-towns", WKC_ESC);
		CHECK(FindWindowById(WindowClass::TownDirectory, 0) == nullptr);

		// The game options always stay above the small map, so M cannot raise the map past them: it closes it at once.
		ShowSmallMap();
		this->PressWindowKey("f2-over-smallmap", WKC_F2);
		this->PressWindowKey("m-under-options", 'M');
		CHECK(FindWindowById(WindowClass::SmallMap, 0) == nullptr);
		CHECK(options_open());
		this->PressWindowKey("escape-options", WKC_ESC);
		CHECK_FALSE(options_open());

		// Escape aborts an armed tool and keeps its palette; the next Escape closes the palette.
		Window *palette = ShowUndergroundConstructionToolbar(MapLayer::Underground, GetCurrentMultiLayerProfileSelection());
		REQUIRE(palette != nullptr);
		PlayerClick(*palette, WID_UC_SURVEY);
		this->LogWindows("survey-armed");
		REQUIRE(IsObjectPlacementActive());
		this->Capture("03-escape-aborts-tool", [&] { this->PressWindowKey("escape-tool", WKC_ESC); });
		CHECK_FALSE(IsObjectPlacementActive());
		CHECK(FindWindowById(WindowClass::UndergroundConstruction, MapLayer::Underground) == palette);
		this->PressWindowKey("escape-palette", WKC_ESC);
		CHECK(FindWindowById(WindowClass::UndergroundConstruction, MapLayer::Underground) == nullptr);

		// A focused edit box keeps Escape: the first clears its text, the second leaves it, the third closes the window.
		this->PressWindowKey("f2-filter", WKC_F2);
		Window *options = FindWindowById(WindowClass::GameOptions, GameOptionsWindowNumber::GameOptions);
		REQUIRE(options != nullptr);
		REQUIRE(options->SetFocusedWidget(WID_GO_FILTER));
		SetFocusedWindow(options);
		this->PressWindowKey("type-r", 'R', 'r');
		this->PressWindowKey("type-a", 'A', 'a');
		CHECK(options->GetQueryString(WID_GO_FILTER)->text.GetText() == "ra");
		this->Capture("03-escape-clears-filter", [&] { this->PressWindowKey("escape-filter-text", WKC_ESC); });
		CHECK(options_open());
		this->PressWindowKey("escape-filter-focus", WKC_ESC);
		CHECK(options_open());
		CHECK_FALSE(EditBoxInGlobalFocus());
		this->PressWindowKey("escape-filter-window", WKC_ESC);
		CHECK_FALSE(options_open());

		REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
	}

	/**
	 * Task 1.7 (Q1): after the bounded number of real, wall-clock paced ticks, print the Frame rate window's own
	 * measurements through the 'fps' console command into a console log, with the achieved tick rate beside them.
	 */
	void MeasureFramerate()
	{
		const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - this->started_at).count();
		const auto path = GetTestCaseArtifactPaths(DEMO_FEATURE, this->scenario).logs / "framerate-console.log";
		fmt::print("DEMO_FRAMERATE ticks={} seconds={:.2f} achieved_ticks_per_second={:.2f} console={}\n", this->ticks, seconds,
			seconds > 0 ? this->ticks / seconds : 0.0, path.generic_string());
		// The same report the player gets from the console; the Frame rate window draws these numbers.
		IConsoleCmdExec(fmt::format("script \"{}\"", path.generic_string()));
		IConsoleCmdExec("fps");
		IConsoleCmdExec("script");
	}

	/** Prepare the loaded world: dump it, then unpause the ordinary simulation. */
	void Start()
	{
		try {
			this->DumpWorld();
			if (this->scenario == "framerate") {
				// The real video driver paces ticks and frames; only the observation runs here, never a capture blitter.
				AutoRestoreBackup actor(_current_company, _local_company);
				REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
				this->started_at = std::chrono::steady_clock::now();
				this->started = true;
				return;
			}
			if (this->scenario == "vehicle-spacing") {
				// Task 5.5: measure the re-derived poses first (paused world), then let the cutter move for the captures.
				this->MeasurePoses();
				this->MeasureCurvePairs();
			}
			if (this->scenario == "poses" || this->scenario == "map" || this->scenario == "windows") {
				// Pose measurement and the map and window checks need no simulation tick; they read and drive the paused world.
				this->started = true;
				AutoRestoreBackup player(_current_company, _local_company);
				if (this->scenario == "poses") this->MeasurePoses();
				if (this->scenario == "map") this->ExerciseMap();
				if (this->scenario == "windows") this->ExerciseWindows();
				this->Finish();
				return;
			}
			const auto projects = CaptureTBMProjects();
			REQUIRE_FALSE(projects.empty());
			this->project_id = projects.front().id;
			this->company = projects.front().company;
			this->portal = projects.front().portal;
			this->anchor = GetExcavationJob(projects.front().job)->commissioning_anchor;
			AutoRestoreBackup actor(_current_company, projects.front().company);
			if (this->scenario == "unload-order") {
				// The demo autosaves show that the user changed the hauler's only order to "Unload all" and "No loading";
				// apply the same change through the ordinary order command, then let the hauler deliver to its station.
				const auto *assignment = GetTBMExchangeAssignment(projects.front().id);
				REQUIRE(assignment != nullptr);
				REQUIRE(Command<Commands::ModifyOrder>::Do(DoCommandFlag::Execute, assignment->hauler, 0, MOF_UNLOAD, to_underlying(OrderUnloadType::Unload)).Succeeded());
				REQUIRE(Command<Commands::ModifyOrder>::Do(DoCommandFlag::Execute, assignment->hauler, 0, MOF_LOAD, to_underlying(OrderLoadType::NoLoad)).Succeeded());
				fmt::print("DEMO_STEP name=unload-all-no-loading train={}\n", assignment->hauler.base());
			}
			if (this->replay_ticks != 0) {
				// Task 6.7: the loaded checkpoint must show the guards it was saved with, then the same following ticks.
				this->replay_name = std::filesystem::path(std::getenv("OTTD_DEMO_FEEDBACK_INPUT")).stem().string();
				this->LogGuards("loaded");
				this->replay_left = this->replay_ticks;
			}
			if (this->scenario == "face-loop") {
				// The plan of the followed loop: where its exchanges fall and how the route bends (r2df2 Group 8).
				const ExcavationJob *job = GetExcavationJob(projects.front().job);
				std::string breakpoints;
				if (job->pass_terms.has_value()) {
					for (uint16_t after : job->pass_terms->exchange_after) breakpoints += fmt::format("{},", after);
				}
				std::string headings;
				for (const ExcavationCutGeometry &cut : job->route) headings += fmt::format("{}", to_underlying(cut.heading));
				fmt::print("DEMO_FACE_PLAN view={} project={} layer={} date={} route={} breakpoints={} headings={}\n", this->view, this->project_id.base(),
					to_underlying(GetTileLayer(job->commissioning_anchor)), TimerGameCalendar::date.base(), job->route.size(), breakpoints, headings);
			}
			REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
			GfxClearSpriteCache();
			FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
			this->started = true;
		} catch (...) {
			this->Fail();
		}
	}

	/**
	 * Observe one ordinary simulation tick.
	 * @param elapsed Ticks since the previous call; the observer never advances work itself.
	 */
	void Observe(uint elapsed)
	{
		if (!this->started || this->finished) return;
		this->ticks += elapsed;
		for (const Train *front : Train::Iterate()) {
			if (!front->IsFrontEngine()) continue;
			this->TraceTrain(*front);
			this->TraceDepotDecision(*front);
			this->TraceOverload(*front);
		}
		this->TraceSignals();
		this->TracePortals();
		const TBMProject *project = GetTBMProject(this->project_id);
		if (project != nullptr) this->TraceCutter(*project);
		// Each report scenario retains its evidence once the reported state appears in the ordinary simulation.
		// Inside a game tick the acting company is not the player's; GUI input made by the steps must act as the player.
		AutoRestoreBackup player(_current_company, _local_company);
		this->LogReplay();
		// A replay run only logs the ticks after its loaded checkpoint, then stops.
		if (this->replay_ticks != 0) {
			if (this->replay_left == 0) this->Finish();
			return;
		}
		if (project != nullptr && this->scenario == "hauler-depot") this->StepHaulerDeparture(*project);
		if (project != nullptr && this->scenario == "forced-hauler") this->StepHaulerDepot(*project);
		if (project != nullptr && this->scenario == "unload-order") this->StepUnloadOrder(*project);
		if (project != nullptr && this->scenario == "held") this->StepHeld(*project);
		if (this->scenario == "new-area" || this->scenario == "obstruction" || this->scenario == "approach") {
			// The user asked for a new area once project 0 had completed 22 cuts; the corridor then exists as it did.
			const ExcavationJob *job = project == nullptr ? nullptr : GetExcavationJob(project->job);
			if (this->area_stage == AreaStage::None && job != nullptr && job->cursor >= this->area_cursor) this->StartNewArea(*project);
			this->StepNewArea();
		}
		if (!this->finished && this->scenario == "obstruction") this->StepObstruction();
		if (!this->finished && this->scenario == "approach") this->StepApproach();
		if (!this->finished && this->scenario == "change" && project != nullptr) this->StepChange(*project);
		if (!this->finished && this->scenario == "face-loop" && project != nullptr) this->StepFaceLoop(*project);
		// Task 5.5: the first tick the demo cutter travels (it returns to the exchange bay after its first pass).
		if (!this->finished && this->scenario == "vehicle-spacing" && project != nullptr) {
			const Train *cutter = Train::GetIfValid(project->cutter);
			if (cutter != nullptr && cutter->cur_speed > 0 && IsTBMReturning(project->phase)) {
				this->CaptureMovingConsist(*cutter);
				this->Finish();
			}
		}
		if (!this->finished && this->ticks >= this->tick_limit) {
			if (this->scenario == "framerate") this->MeasureFramerate();
			this->Finish();
		}
	}

	/** Stop observing, pause the world and end the bounded native run. */
	void Finish()
	{
		this->DumpWorld();
		this->finished = true;
		if (this->scenario == "face-loop") this->FinishFaceLoop();
		const auto projects = CaptureTBMProjects();
		if (!projects.empty()) {
			AutoRestoreBackup actor(_current_company, projects.front().company);
			Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true);
		}
		fmt::print("DEMO_COMPLETE scenario={} ticks={} captures={}\n", this->scenario, this->ticks, this->captures);
		_exit_game = true;
	}

	/** Retain the current exception and stop the native loop without drawing through a released canvas. */
	void Fail()
	{
		if (!this->failure) this->failure = std::current_exception();
		BlitterFactory::SelectBlitter("null");
		GfxClearSpriteCache();
		FontCache::ClearFontCaches(FONTSIZES_REQUIRED);
		this->finished = true;
		_exit_game = true;
	}
};

static DemoFeedbackProbe *_demo_probe = nullptr; ///< Process-local observer, absent from production executables.

/**
 * Start the observer once the ordinary startup loader has rebuilt the demo world.
 * @param arguments The registered console command only; gameplay parameters come from the runner's environment.
 * @return True after dispatching the observer.
 */
static bool StartDemoFeedback(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1);
	REQUIRE(_demo_probe != nullptr);
	_demo_probe->Start();
	return true;
}

/** Reproduce one 2026-10-15 demo report natively, selected by OTTD_DEMO_FEEDBACK_SCENARIO and OTTD_DEMO_FEEDBACK_INPUT. */
TEST_CASE("Demo feedback reproduces the reported demo states", "[.demo-feedback]")
{
	DemoFeedbackProbe probe;
	const char *scenario = std::getenv("OTTD_DEMO_FEEDBACK_SCENARIO");
	const char *input = std::getenv("OTTD_DEMO_FEEDBACK_INPUT");
	REQUIRE(scenario != nullptr);
	REQUIRE(input != nullptr);
	probe.scenario = ValidateTestArtifactIdentifier(scenario);
	if (const char *limit = std::getenv("OTTD_DEMO_FEEDBACK_TICKS"); limit != nullptr) probe.tick_limit = ParseInteger<uint>(limit).value_or(probe.tick_limit);
	if (const char *delay = std::getenv("OTTD_DEMO_HAULER_STOP"); delay != nullptr) probe.hauler_stop_delay = ParseInteger<uint>(delay).value_or(probe.hauler_stop_delay);
	if (const char *replay = std::getenv("OTTD_DEMO_REPLAY"); replay != nullptr) probe.replay_ticks = ParseInteger<uint>(replay).value_or(0);
	if (const char *view = std::getenv("OTTD_DEMO_VIEW"); view != nullptr) probe.view = view;
	REQUIRE((probe.view == "headless" || probe.view == "visible" || probe.view == "hidden"));
	if (const char *target = std::getenv("OTTD_DEMO_FACE_TRANSFERS"); target != nullptr) probe.face_target = ParseInteger<uint>(target).value_or(probe.face_target);
	if (const char *bend = std::getenv("OTTD_DEMO_FACE_BEND"); bend != nullptr) probe.face_bend = std::string_view{bend} == "1";
	if (const char *bay = std::getenv("OTTD_DEMO_FACE_ALLOW_BAY"); bay != nullptr) probe.face_allow_bay = std::string_view{bay} == "1";
	if (const char *saves = std::getenv("OTTD_DEMO_FACE_SAVES"); saves != nullptr) {
		// A comma-separated list of the checkpoint states the face loop saves (task 9.4).
		std::string_view list{saves};
		while (!list.empty()) {
			const size_t comma = list.find(',');
			const std::string_view name = list.substr(0, comma);
			REQUIRE((name == "travelling" || name == "held" || name == "dwell" || name == "leaving" || name == "bay-return"));
			probe.face_saves.emplace(name);
			list = comma == std::string_view::npos ? std::string_view{} : list.substr(comma + 1);
		}
	}
	if (const char *lifecycle = std::getenv("OTTD_DEMO_FACE_LIFECYCLE"); lifecycle != nullptr) probe.face_lifecycle = lifecycle;
	REQUIRE((probe.face_lifecycle.empty() || probe.face_lifecycle == "stop" || probe.face_lifecycle == "park" || probe.face_lifecycle == "release"));
	AutoRestoreBackup observer(_demo_probe, &probe);
	IConsole::CmdRegister("demo_feedback", StartDemoFeedback);
	/** @param elapsed Real ticks since the previous call; only observation is performed. */
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&probe](uint elapsed) {
		if (probe.failure) return;
		try {
			probe.Observe(elapsed);
		} catch (...) {
			probe.Fail();
		}
	});
	/** Save a requested checkpoint; real-time timers run between game loops, never inside a tick. */
	const IntervalTimer<TimerGameRealtime> saver({std::chrono::milliseconds(1), TimerGameRealtime::Trigger::Always}, [&probe](auto) {
		if (probe.failure) return;
		try {
			probe.SaveCheckpoint();
		} catch (...) {
			probe.Fail();
		}
	});
	const auto source = ResolveTestInputPath(input).string();
	// The frame-rate reading needs the player's own config, a real video driver and its own blitter choice;
	// every other scenario uses the isolated runtime config with the pixel-free null driver.
	const char *config_override = std::getenv("OTTD_DEMO_FEEDBACK_CONFIG");
	const char *video_override = std::getenv("OTTD_DEMO_FEEDBACK_VIDEO");
	const auto runtime_config = config_override != nullptr ? std::string{config_override} : GetTestRuntimeConfig().string();
	// The null video driver runs a bounded number of frames; the observer ends earlier through _exit_game.
	const std::string frames = video_override != nullptr ? std::string{video_override} : fmt::format("null:ticks={}", probe.tick_limit + 2000);
	std::vector<std::string_view> arguments = {"RelWithDebInfo/openttd_test.exe", "-v", frames};
	if (video_override == nullptr) arguments.insert(arguments.end(), {"-b", "8bpp-simple", "-I", "original_dos"});
	arguments.insert(arguments.end(), {"-s", "null", "-m", "null", "-x", "-X", "-c", runtime_config, "-g", source, "-r", "1280x733"});
	CHECK(openttd_main(arguments) == 0);
	if (probe.failure) std::rethrow_exception(probe.failure);
	REQUIRE(probe.started);
	REQUIRE(probe.finished);
}
