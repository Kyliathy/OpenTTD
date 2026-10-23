/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_exit_replay.h Actual far-exit GUI construction and loaded native movement acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Groups9/11
 * @see docs/zz-reach2/architecture/gui/archi-gui.md
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 */

#ifndef TESTS_TBM_EXIT_REPLAY_H
#define TESTS_TBM_EXIT_REPLAY_H

#include "test_artifact_paths.h"
#include "tbm_hangar_input.h"
#include "../rail_gui.h"
#include "../widgets/rail_widget.h"
#include "../construction_preview.h"
#include "../core/random_func.hpp"
#include "../strings_func.h"
#include "../station_cmd.h"
#include "../station_base.h"
#include "../order_cmd.h"
#include "../signal_func.h"
#include "../rail_cmd.h"
#include "../vehicle_cmd.h"
#include "../tbm_controls.h"
#include "../misc_cmd.h"
#include "../newgrf_station.h"
#include <cstdlib>

/** Observe real loaded terminal handover and parking; commands and native ticks own all gameplay changes. */
class TBMExitReplay {
	TBMProject original; ///< Immutable loaded controller snapshot for identity checks.
	ExcavationJob job; ///< Completed paid geometry and exact integer receipts.
	TileIndex surface = INVALID_TILE; ///< B's actual reciprocal upper endpoint.
	size_t fleet = 0; ///< Actual original vehicle cardinality.
	std::filesystem::path output; ///< Fresh repository-owned save prefix.
	bool saved_motion = false; ///< One actual moving checkpoint, not an invented vehicle pose.
	bool saved_parked = false; ///< Preserve the first actual full arrival while final haulage settlement continues.
	bool final_traffic = false; ///< Group11 continues the same loaded machine through release and ordinary freight.
	uint8_t traffic_stage = 0; ///< Test-observation stage: waiting for release, outward travel, then return; never a vehicle order queue.
	VehicleID hauler = VehicleID::Invalid(); ///< Original actual freight train, retained after the assignment is released.
	StationID destination = StationID::Invalid(); ///< Actual funded public station beyond B.
public:
	bool completed = false; ///< Actual whole-machine arrival was observed.

	/** @param suffix Actual reached stage. Save the native world without replacing any owner. */
	void Save(std::string_view suffix) const
	{
		const auto path = std::filesystem::path(this->output.string() + "-exit-" + std::string(suffix) + ".sav");
		REQUIRE_FALSE(std::filesystem::exists(path));
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}

	/** Observe public commands and real station arrivals after the controller is released. @param ticks Actual game-loop frame after load. @return None; native commands and train ticks own all mutations. */
	void ObservePublicTraffic(uint ticks)
	{
		REQUIRE(ticks < 80000);
		AutoRestoreBackup actor(_current_company, this->original.company);
		Train *freight = Train::Get(this->hauler);
		if (this->traffic_stage == 1) {
			if (GetTBMProject(this->original.id) != nullptr) return; // The UI-posted release executes at its ordinary command boundary.
			CHECK(GetTBMExchangeAssignmentForHauler(this->hauler) == nullptr);
			for (const auto &support : CaptureTBMSupport()) REQUIRE(Command<Commands::ConvertTBMSupport>::Do(DoCommandFlag::Execute, support.tile).Succeeded());
			REQUIRE(Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, this->original.portal, true).Succeeded());
			REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, this->job.route[25].cutter_position, RAILTYPE_RAIL, Track::Left, false).Succeeded());
			const TileIndex door = GetLayerPortalDoorTile(this->surface);
			Track bend = Track::Invalid;
			// B is near the southern boundary. Use a normal funded bend toward
			// a station with real approaches, preserving the existing public mouth.
			for (Trackdir direction : DiagdirReachesTrackdirs(GetLayerPortalDoorDirection(this->surface))) {
				if (TrackdirToExitdir(direction) == DiagDirection::NE) bend = TrackdirToTrack(direction);
			}
			REQUIRE(bend != Track::Invalid);
			REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, door, RAILTYPE_RAIL, bend, false).Succeeded());
			REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, TileAddWrap(door, -1, 0), RAILTYPE_RAIL, Track::X, false).Succeeded());
			const TileIndex station = TileAddWrap(door, -2, 0);
			REQUIRE(Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, station, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false).Succeeded());
			this->destination = Station::GetByTile(station)->index;
			for (StationID stop : {this->destination, this->job.origin}) {
				Order order;
				order.MakeGoToStation(stop);
				order.SetLoadType(OrderLoadType::NoLoad);
				order.SetUnloadType(OrderUnloadType::NoUnload);
				REQUIRE(Command<Commands::InsertOrder>::Do(DoCommandFlag::Execute, freight->index, freight->GetNumOrders(), order).Succeeded());
			}
			if (freight->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, freight->index, true).Succeeded());
			UpdateSignalsInBuffer();
			this->Save("released");
			this->traffic_stage = 2;
		}
		REQUIRE_FALSE(freight->vehstatus.Test(VehState::Crashed));
		REQUIRE(IsTBMWholeInHangar(*Train::Get(this->original.cutter)));
		REQUIRE(Train::Get(this->original.cutter)->tile == this->surface);
		if (this->traffic_stage == 2 && freight->last_station_visited == this->destination) {
			this->Save("freight-outward");
			this->traffic_stage = 3;
		}
		if (this->traffic_stage != 3 || freight->last_station_visited != this->job.origin) return;
		CHECK(*GetExcavationJob(this->job.id) == this->job);
		CHECK(Vehicle::GetNumItems() == this->fleet);
		REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
		this->Save("traffic");
		fmt::print("FINAL_PUBLIC cutter={} job={} cells={} freight={} destination={} ticks={} both_directions=true parked_b=true\n",
			this->original.cutter.base(), this->job.id.base(), this->job.cursor, this->hauler.base(), this->destination.base(), ticks);
		this->completed = true;
		_exit_game = true;
	}

	/** @param window Actual window. @param widget Actual visible widget whose native rectangle supplies hit testing. */
	static void Click(Window &window, WidgetID widget)
	{
		const Rect rect = window.GetWidget<NWidgetBase>(widget)->GetCurrentRect();
		ClickHangarPoint(window, {(rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2});
		ElapseHangarInputTimers();
	}

	/** @param tile Actual main-viewport world tile. @return Its ground centre in real screen coordinates. */
	static Point TilePoint(TileIndex tile)
	{
		const Viewport &view = *GetMainWindow()->viewport;
		const Point world = RemapCoords(TileX(tile) * TILE_SIZE + TILE_SIZE / 2, TileY(tile) * TILE_SIZE + TILE_SIZE / 2, TilePixelHeight(tile));
		return {view.left + UnScaleByZoom(world.x - view.virtual_left, view.zoom), view.top + UnScaleByZoom(world.y - view.virtual_top, view.zoom)};
	}

	/** @param tile Explicit-layer site. Hover through the normal dispatcher; no placement callback is invoked directly. */
	static void Hover(TileIndex tile)
	{
		const Point point = TilePoint(tile);
		_cursor.UpdateCursorPosition(point.x, point.y);
		_cursor.in_window = true;
		_left_button_down = _left_button_clicked = false;
		HandleMouseEvents();
		ElapseHangarInputTimers(12);
	}

	/** @param picker Actual native picker. Drag its title to the left edge so the centred map site remains reachable. */
	static void MovePicker(Window &picker)
	{
		const Point caption{picker.left + picker.width / 2, picker.top + ScaleGUITrad(7)};
		const Point destination{picker.width / 2, ScaleGUITrad(45)};
		// Only normal mouse drag dispatch changes the window position. Keeping
		// the site visible is part of the ordinary player placement workflow.
		for (const auto &[point, down] : std::array<std::pair<Point, bool>, 4>{{{caption, false}, {caption, true}, {destination, true}, {destination, false}}}) {
			_cursor.UpdateCursorPosition(point.x, point.y);
			_left_button_down = down;
			if (!down) _left_button_clicked = false;
			HandleMouseEvents();
		}
		ElapseHangarInputTimers();
	}

	/** @param tile Actual clicked endpoint. @param shaft Accepted command orientation. Build through native toolbar, picker and viewport input. */
	void BuildThroughInput(TileIndex tile, DiagDirection shaft)
	{
		const bool underground = GetTileLayer(tile) == MapLayer::Underground;
		const auto name = underground ? "exit-underground" : "exit-surface";
		SetViewportLayer(GetMainWindow(), GetTileLayer(tile));
		ScrollMainWindowToTile(tile, true);
		UpdateViewportPosition(GetMainWindow(), 0);
		Window *toolbar = ShowBuildRailToolbar(RAILTYPE_RAIL);
		REQUIRE(toolbar != nullptr);
		Click(*toolbar, WID_RAT_BUILD_LAYER_PORTAL);
		Window *picker = FindWindowById(WindowClass::BuildDepot, TransportType::Rail);
		REQUIRE(picker != nullptr);
		MovePicker(*picker);
		const Point visible = TilePoint(tile);
		INFO("map point=" << visible.x << ',' << visible.y << " picker=" << picker->left << ',' << picker->top << ',' << picker->width << ',' << picker->height);
		REQUIRE(FindWindowFromPt(visible.x, visible.y) == GetMainWindow());
		if (underground) Click(*picker, WID_BRLP_TARGET_SURFACE);
		Click(*picker, WID_BRLP_MOUTH_NE + to_underlying(shaft)); // Opposite to the required mouth: real visible refusal.
		Hover(tile);
		CHECK(GetConstructionPreviewTile(tile) == ConstructionPreviewTile::Uncovered);
		CHECK(picker->GetWidgetString(WID_BRLP_QUOTE, STR_BUILD_RAIL_LAYER_PORTAL_HINT).find(GetString(STR_ERROR_TBM_EXIT_GEOMETRY)) != std::string::npos);
		CaptureHangarWindow(*picker, fmt::format("{}-refused", name));
		Click(*picker, WID_BRLP_MOUTH_NE + to_underlying(ReverseDiagDir(shaft)));
		Hover(tile);
		CHECK(GetConstructionPreviewTile(tile) == ConstructionPreviewTile::Floor);
		CHECK(GetConstructionPreviewTile(MultiLayerMap::TileInLayer(tile, underground ? MapLayer::Surface : MapLayer::Underground)) == ConstructionPreviewTile::Floor);
		CHECK(picker->GetWidgetString(WID_BRLP_QUOTE, STR_BUILD_RAIL_LAYER_PORTAL_HINT).find("Build and hand over support:") != std::string::npos);
		CaptureHangarWindow(*picker, fmt::format("{}-quote", name));
		REQUIRE(BlitterFactory::SelectBlitter("8bpp-simple") != nullptr);
		GfxClearSpriteCache();
		CaptureNativeArtViewport(tile, ZoomLevel::Normal, GetTestCaseArtifactPaths("tbm", "far-exit").images / fmt::format("tbm-{}-footprint.ppm", name));
		REQUIRE(BlitterFactory::SelectBlitter("null") != nullptr);
		GfxClearSpriteCache();
		const Point point = TilePoint(tile);
		Window *main = GetMainWindow();
		ClickHangarPoint(*main, {point.x - main->left, point.y - main->top});
		ElapseHangarInputTimers();
		REQUIRE(IsLayerPortalTile(tile));
		CloseWindowByClass(WindowClass::BuildToolbar);
		fmt::print("EXIT_INPUT side={} tile={} pair={}\n", name, tile.base(), GetLayerPortalPairTile(tile).base());
	}

	/** @param prefix Fresh native output prefix. Continue the genuinely reached stage from the actual file loader. */
	explicit TBMExitReplay(const std::filesystem::path &prefix) : output(prefix)
	{
		const auto projects = CaptureTBMProjects();
		REQUIRE(projects.size() == 1);
		this->original = projects.front();
		this->job = *GetExcavationJob(this->original.job);
		REQUIRE(this->job.state == ExcavationJobState::Completed);
		REQUIRE(this->job.route.size() == 50);
		this->fleet = Vehicle::GetNumItems();
		this->final_traffic = std::getenv("OTTD_TBM_FINAL_TRAFFIC") != nullptr;
		if (this->final_traffic) {
			const auto *assignment = GetTBMExchangeAssignment(this->original.id);
			REQUIRE(assignment != nullptr);
			this->hauler = assignment->hauler;
		}
		const TileIndex rock = this->job.route.back().cutter_position;
		this->surface = MultiLayerMap::TileInLayer(rock, MapLayer::Surface);
		AutoRestoreBackup actor(_current_company, this->original.company);
		REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
		const char *source = std::getenv("OTTD_TBM_LOGISTICS_SOURCE");
		REQUIRE(source != nullptr);
		const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()), controllers = ComputeTBMProjectDigest();
		REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
		CHECK(ComputeTBMProjectDigest() == controllers);
		this->Save("loaded");
		const char *input = std::getenv("OTTD_TBM_EXIT_INPUT");
		if (!IsLayerPortalTile(rock)) {
			const bool underground = input == nullptr || std::string_view(input) == "underground";
			const TileIndex clicked = underground ? rock : this->surface;
			const DiagDirection shaft = underground ? GetLayerPortalDoorDirection(this->job.commissioning_anchor) : GetLayerPortalShaftDirection(this->job.commissioning_anchor);
			if (input != nullptr) {
				BuildThroughInput(clicked, shaft);
			} else {
				REQUIRE(Command<Commands::BuildLayerPortal>::Do(DoCommandFlag::Execute, clicked, TransportType::Rail,
					underground ? MapLayer::Surface : MapLayer::Underground, shaft, RAILTYPE_RAIL, INVALID_ROADTYPE).Succeeded());
			}
			this->Save("built");
		}
		REQUIRE(GetLayerPortalPairTile(this->surface) == rock);
		const TileIndex door = GetLayerPortalDoorTile(this->surface);
		if (!IsPlainRailTile(door)) {
			REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, door, RAILTYPE_RAIL, DiagDirToDiagTrack(GetLayerPortalShaftDirection(this->surface)), false).Succeeded());
		}
		// The existing native connection command validates both real approaches;
		// it never purchases a second cutter or opens unpaid terrain.
		if (!IsTBMWholeInHangar(*Train::Get(this->original.cutter)) || Train::Get(this->original.cutter)->tile != this->surface) {
			if (GetLayerPortalWorkState(this->surface) != LayerPortalWorkState::Connected) {
				const auto connected = Command<Commands::ChangeLayerPortalConnection>::Do(DoCommandFlag::Execute, this->surface, true);
				REQUIRE(connected.Succeeded());
			}
			this->Save("commissioned");
			Train *front = Train::Get(this->original.cutter);
			if (GetTBMOrderedHangar(*front).tile != this->surface) {
				if (input != nullptr) SelectHangarThroughInput(*GetTBMProject(this->original.id), this->surface, false);
				else REQUIRE(Command<Commands::SendTBMToHangar>::Do(DoCommandFlag::Execute, front->index, this->surface, false).Succeeded());
			}
			this->Save("withdrawing");
		}
		CHECK(*GetExcavationJob(this->job.id) == this->job);
		CHECK(Vehicle::GetNumItems() == this->fleet);
		UpdateSignalsInBuffer();
	}

	/** Observe arrival and retain actual moving/parked states before optional public traffic. @param ticks Actual game-loop frames after load. @return None; only an observed whole-machine arrival advances the test stage. */
	void Observe(uint ticks)
	{
		if (this->traffic_stage != 0) {
			this->ObservePublicTraffic(ticks);
			return;
		}
		const auto *work = GetTBMProject(this->original.id);
		REQUIRE(work != nullptr);
		const Train *front = Train::Get(this->original.cutter);
		if (ticks == 10000) fmt::print("EXIT_WAIT phase={} blocked={} tile={} speed={} stopped={}\n", to_underlying(work->phase), to_underlying(work->blocked), front->tile.base(), front->cur_speed, front->vehstatus.Test(VehState::Stopped));
		REQUIRE(ticks < 70000);
		REQUIRE_FALSE(front->vehstatus.Test(VehState::Crashed));
		if (!this->saved_motion && work->phase == TBMPhase::TravelToHangar && !front->IsChainInDepot() && DistanceManhattan(front->tile, this->surface) <= 4) {
			this->Save("moving");
			this->saved_motion = true;
		}
		if (work->phase != TBMPhase::ParkedHangar) return;
		CHECK(IsTBMWholeInHangar(*front));
		CHECK(front->tile == this->surface);
		CHECK(work->permanent_parts == this->original.permanent_parts);
		CHECK(*GetExcavationJob(this->job.id) == this->job);
		CHECK(Vehicle::GetNumItems() == this->fleet);
		if (!this->saved_parked) {
			this->Save("parked");
			this->saved_parked = true;
		}
		if (this->final_traffic) {
			if (GetTBMProjectReleaseProblem(this->original.id) != INVALID_STRING_ID) return;
			AutoRestoreBackup actor(_current_company, this->original.company);
			Window *window = ShowTBMProjectsWindow(GetTileLayer(this->job.commissioning_anchor));
			REQUIRE(window != nullptr);
			CaptureHangarWindow(*window, "final-release-ready");
			Click(*window, WID_TP_RELEASE);
			this->traffic_stage = 1;
			return;
		}
		fmt::print("EXIT_RELOAD cutter={} job={} cells={} target={} ticks={}\n", front->index.base(), this->job.id.base(), this->job.cursor, this->surface.base(), ticks);
		this->completed = true;
		_exit_game = true;
	}
};

#endif /* TESTS_TBM_EXIT_REPLAY_H */
