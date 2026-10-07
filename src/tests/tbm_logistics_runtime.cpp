/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_logistics_runtime.cpp Isolated real startup/loader/game-loop logistics continuation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Groups 13A/13C
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 7BB
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 19 (actual viewport/headless pairs)
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md Groups82/83, optional actual native raster observation
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "layer_save_fixture.h"
#include "logistics_crash_fixture.h"
#include "native_art_loop_fixture.h"
#include "native_sound_runtime_fixture.h"
#include "../console_internal.h"
#include "../openttd.h"
#include "../tbm_exchange.h"
#include "../train.h"
#include "../vehicle_cmd.h"
#include "../misc_cmd.h"
#include "../command_func.h"
#include "../station_base.h"
#include "../station_cmd.h"
#include "../newgrf_station.h"
#include "../rail_map.h"
#include "../track_func.h"
#include "../station_map.h"
#include "../layer_portal_map.h"
#include "../map_func.h"
#include "../rail_cmd.h"
#include "../train_cmd.h"
#include "../tbm_cmd.h"
#include "../timer/timer.h"
#include "../timer/timer_game_tick.h"
#include "../saveload/saveload_func.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "../map_layer_gui.h"
#include "../viewport_func.h"
#include <cstdlib>
#include <filesystem>
#include <charconv>

#include "../safeguards.h"

/** Read-only transition observer; all movement and billing run through the normal loaded game loop. */
class LogisticsRuntimeProbe {
public:
	std::filesystem::path output; ///< Fresh repository-local evidence prefix, supplied by the external focused runner.
	VehicleID hauler = VehicleID::Invalid(); ///< Actual loaded assigned front engine.
	uint32_t initial_exchanges = 0; ///< Actual initial cycle count, never changed by the observer.
	uint ticks = 0; ///< Number of real frames observed after the startup command.
	bool started = false; ///< Startup has accepted the actual reconstructed public owners.
	bool completed = false; ///< The next physical exchange has reached its complete atomic boundary.
	bool complete_loop = false; ///< Optional complete-job plus final Surface disposal endpoint for actual visibility pairs.
	std::string presentation; ///< Process-local headless/visible/hidden policy; it never changes native gameplay commands.
	mutable uint draws = 0; ///< Number of actual native viewport render dispatches; observer evidence only.
	mutable uint visible_cutter_draws = 0; ///< Actual draws centered on a non-hidden cutter in the selected physical layer.
	std::unique_ptr<LogisticsCrashLedger> crash; ///< Actual irreversible wreck; the next legal transition is native cleanup.
	std::unique_ptr<NativeArtLoopProbe> art; ///< Optional read-only scene capture; normal logistics checks allocate no art observer.
	std::unique_ptr<NativeSoundRuntimeProbe> sound; ///< Optional actual cue/decoder/mixer proof; normal checks retain their audio policy.

	/**
	 * Save one fully reconstructed native state through the production writer.
	 * @param suffix Distinct owned evidence suffix; previous files are retained.
	 */
	void Save(std::string_view suffix) const
	{
		const auto path = std::filesystem::path(this->output.string() + std::string(suffix) + ".sav");
		REQUIRE_FALSE(std::filesystem::exists(path));
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}

	/** Select and draw the actual native main viewport; camera changes own no work, packet or movement step. */
	void Present() const
	{
		if (this->presentation.empty() || this->presentation == "headless") return; // The baseline performs no observer-owned camera selection or draw.
		Window *window = FindWindowById(WindowClass::MainWindow, 0);
		REQUIRE(window != nullptr);
		REQUIRE(window->viewport != nullptr);
		const auto *assignment = GetTBMExchangeAssignmentForHauler(this->hauler);
		REQUIRE(assignment != nullptr);
		const auto *project = GetTBMProject(assignment->project);
		REQUIRE(project != nullptr);
		const Train *cutter = Train::Get(project->cutter);
		const MapLayer selected = this->presentation == "visible" ? GetTileLayer(cutter->tile) : MapLayer::Space;
		SetViewportLayer(window, selected);
		CHECK(window->viewport->layer == selected);
		ScrollWindowToTile(MultiLayerMap::TileInLayer(cutter->tile, selected), window, true);
		UpdateViewportPosition(window, 0);
		const Viewport &viewport = *window->viewport;
		// The null video driver's ordinary loop intentionally skips raster refresh and leaves _cur_dpi unset.
		// Bind its real initialized screen for this explicit native render dispatch, then restore the process's previous draw context.
		AutoRestoreBackup drawing(_cur_dpi, &_screen);
		ViewportDoDraw(viewport, viewport.virtual_left, viewport.virtual_top, viewport.virtual_left + viewport.virtual_width, viewport.virtual_top + viewport.virtual_height);
		++this->draws;
		// A visible pair must actually draw while the physical cutter is outside its hidden magazine/shaft; layer selection alone is insufficient evidence.
		if (selected == GetVehicleDisplayLayer(cutter) && !cutter->vehstatus.Test(VehState::Hidden)) ++this->visible_cutter_draws;
	}

	/** Report actual ordinary tile-dispatch edges for accepted portal pairs, including legacy worlds with no Native work rows. */
	void ReportPortalEdges() const
	{
		std::array<uint32_t, 3> total{}, usable{};
		uint invalid = 0;
		// Read each accepted reciprocal pair once. This observer queries the native track-status owner;
		// it owns no replacement connectivity cache, construction permission or saved portal state.
		for (uint32_t index = 0; index < Map::Size(); ++index) {
			const TileIndex tile{index};
			if (!IsLayerPortalTile(tile)) continue;
			const auto key = GetLayerPortalPairKey(tile);
			if (!key) { ++invalid; continue; }
			if (*key != tile) continue;
			const auto pair = GetLayerPortalPairTile(tile);
			const auto kind = GetLayerPortalKind(tile);
			REQUIRE(to_underlying(kind) < total.size());
			const auto slot = to_underlying(kind);
			++total[slot];
			const auto transport = GetLayerPortalTransportType(tile);
			const auto road = transport == TransportType::Road ? RoadTramType::Road : RoadTramType::Invalid;
			// Both real housings must expose ordinary edges; a stored neighbor cannot establish usable legacy/date/phase admission.
			if (GetTileTrackStatus(tile, transport, road).trackdirs.Any() && GetTileTrackStatus(pair, transport, road).trackdirs.Any()) ++usable[slot];
		}
		fmt::print("PORTAL_NATIVE_EDGES rail={}/{} road={}/{} elevator={}/{} invalid={}\n", usable[0], total[0], usable[1], total[1], usable[2], total[2], invalid);
	}

	/**
	 * Rebuild the lost scenario's owned Surface platform through ordinary construction.
	 * @param project Actual loaded project whose deleted station sign and retained straight rail provide the legal original anchor.
	 */
	void RestoreOrigin(const TBMProject &project) const
	{
		const Station *origin = Station::GetIfValid(project.origin);
		REQUIRE(origin != nullptr);
		REQUIRE(origin->owner == project.company);
		REQUIRE(GetTileLayer(origin->xy) == MapLayer::Surface);
		REQUIRE(IsPlainRailTile(origin->xy));
		const auto track = GetTrackBits(origin->xy);
		// The original single straight platform is restored on its real retained rail; curves or junctions cannot supply an invented station axis.
		REQUIRE((track == TrackBits{Track::X} || track == TrackBits{Track::Y}));
		const Axis axis = track == TrackBits{Track::X} ? Axis::X : Axis::Y;
		const uint length = CeilDiv(Train::Get(this->hauler)->gcache.cached_total_length, TILE_SIZE);
		REQUIRE(length > 0);
		REQUIRE(length <= UINT8_MAX);
		const TileIndex tile = origin->xy;
		const StationID identity = origin->index;
		REQUIRE(Command<Commands::BuildRailStation>::Do(DoCommandFlag::Execute, tile, GetRailType(tile), axis,
			uint8_t{1}, static_cast<uint8_t>(length), STAT_CLASS_DFLT, uint16_t{0}, StationID::Invalid(), false).Succeeded());
		CHECK(Station::GetByTile(tile)->index == identity);
		CHECK(CheckTBMOriginAdmission(project) == TBMExchangeError::None);
	}

	/**
	 * Repair the one interrupted ordinary Surface rail cell and request a fresh physical route choice.
	 * @param project Actual loaded owner/origin/pair; the declared source scenario has one straight owned approach with one missing rail cell.
	 */
	void RestoreLine(const TBMProject &project) const
	{
		const char *declared_tile = std::getenv("OTTD_TBM_LOGISTICS_REPAIR_TILE");
		const char *declared_track = std::getenv("OTTD_TBM_LOGISTICS_REPAIR_TRACK");
		if (declared_tile != nullptr) {
			// The recurring depot has a separate winding freight route. Its producer reports the exact ordinary
			// removed piece; this explicit player repair command cannot infer a straight route from the cutter's Sealed source.
			REQUIRE(declared_track != nullptr);
			uint32_t tile_value = 0, track_value = 0;
			const auto tile_result = std::from_chars(declared_tile, declared_tile + strlen(declared_tile), tile_value);
			const auto track_result = std::from_chars(declared_track, declared_track + strlen(declared_track), track_value);
			REQUIRE(tile_result.ec == std::errc{});
			REQUIRE(tile_result.ptr == declared_tile + strlen(declared_tile));
			REQUIRE(track_result.ec == std::errc{});
			REQUIRE(track_result.ptr == declared_track + strlen(declared_track));
			const TileIndex tile{tile_value};
			REQUIRE(IsValidTile(tile));
			REQUIRE(GetTileLayer(tile) == MapLayer::Surface);
			REQUIRE(IsTileType(tile, TileType::Clear));
			REQUIRE(track_value < to_underlying(Track::End));
			REQUIRE(GetTBMExchangeBay(project).kind == TBMExchangeBayKind::Depot);
			REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, tile, GetRailType(project.portal), static_cast<Track>(track_value), false).Succeeded());
			REQUIRE(Command<Commands::ReverseTrainDirection>::Do(DoCommandFlag::Execute, this->hauler, false).Succeeded());
			return;
		}
		const Station *origin = Station::GetIfValid(project.origin);
		REQUIRE(origin != nullptr);
		REQUIRE(IsRailStationTile(origin->train_station.tile));
		const RailType railtype = GetRailType(origin->train_station.tile);
		const Track track = AxisToTrack(GetRailStationAxis(origin->train_station.tile));
		const TileIndex intent = MultiLayerMap::TileInLayer(project.portal, MapLayer::Surface);
		const DiagDirection direction = DiagdirBetweenTiles(origin->train_station.tile, intent);
		REQUIRE(IsValidDiagDirection(direction));
		TileIndex tile = origin->train_station.tile;
		std::optional<TileIndex> gap;
		bool reached_portal = false;
		// Follow only the actual declared straight approach to its existing owned access portal. Rebuild exactly one
		// removed cell; a different structure, missing portal or multiple gaps cannot turn this probe into arbitrary construction.
		for (uint remaining = Map::SizeX() + Map::SizeY(); remaining != 0; --remaining) {
			tile = TileAddByDiagDir(tile, direction);
			REQUIRE(IsValidTile(tile));
			REQUIRE(GetTileLayer(tile) == MapLayer::Surface);
			if (IsLayerPortalTile(tile)) {
				REQUIRE(GetTileOwner(tile) == project.company);
				reached_portal = true;
				break;
			}
			if (IsTileType(tile, TileType::Clear)) {
				REQUIRE_FALSE(gap.has_value());
				gap = tile;
			} else {
				// Every remaining approach cell must be the same company's existing rail/platform; unrelated terrain cannot be a repair recipe.
				REQUIRE((IsPlainRailTile(tile) || IsRailStationTile(tile)));
				REQUIRE(GetTileOwner(tile) == project.company);
			}
		}
		REQUIRE(reached_portal);
		REQUIRE(gap.has_value());
		REQUIRE(Command<Commands::BuildRail>::Do(DoCommandFlag::Execute, *gap, railtype, track, false).Succeeded());
		// The normal reverse command returns the actual hauler through its paid junction for a new route search;
		// private lost flags, orders, packet lists and physical positions remain untouched by the probe.
		REQUIRE(Command<Commands::ReverseTrainDirection>::Do(DoCommandFlag::Execute, this->hauler, false).Succeeded());
	}

	/**
	 * Reattach the actual assigned set's missing depot units through ordinary rearrangement commands.
	 * @param assignment Loaded public assignment; every recorded unit remains a live exclusive member of its set.
	 */
	void RestoreWagons(const TBMExchangeAssignment &assignment) const
	{
		const auto *set = GetTBMWagonSet(assignment.set);
		REQUIRE(set != nullptr);
		Train *front = Train::Get(this->hauler);
		uint recovered = 0;
		// Only genuine detached units in the same ordinary depot can rejoin this actual hauler. No ID, cargo,
		// capacity or assignment record is replaced to conceal a wrong-consist state after loading.
		for (VehicleID id : set->wagons) {
			const Train *unit = Train::Get(id);
			if (unit->First() == front) continue;
			REQUIRE(unit->track == Track::Depot);
			REQUIRE(front->track == Track::Depot);
			REQUIRE(unit->tile == front->tile);
			REQUIRE(Command<Commands::MoveRailVehicle>::Do(DoCommandFlag::Execute, id, front->Last()->index, false).Succeeded());
			++recovered;
		}
		CHECK(recovered > 0);
	}

	/** Start stopped participants through their actual commands after saving the untouched loaded baseline. */
	void Start()
	{
		REQUIRE_FALSE(this->started);
		if (std::getenv("OTTD_TBM_PORTAL_LOAD_ONLY") != nullptr) {
			// Empty Sealed/Connected and genuine legacy portal fixtures have no hauler assignment. Their accepted
			// native reconstruction is the checkpoint; the observer cannot fabricate a project just to reuse logistics startup.
			REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
			this->Save("-loaded");
			this->ReportPortalEdges();
			fmt::print("PORTAL_LOAD_ONLY pairs={}\n", GetLayerPortalWorkPairCount());
			this->started = this->completed = true;
			_exit_game = true;
			return;
		}
		const auto assignments = CaptureTBMExchangeAssignments();
		REQUIRE_FALSE(assignments.empty());
		// A queued source scenario observes its existing short-empty first turn, not the blocked peer's fictitious exchange.
		// Single-hauler scenarios retain their only actual assignment.
		const auto first_turn = std::ranges::find(assignments, TBMHaulerWait::NoEmpties, &TBMExchangeAssignment::wait);
		const auto &assignment = first_turn == assignments.end() ? assignments.front() : *first_turn;
		const auto *project = GetTBMProject(assignment.project);
		REQUIRE(project != nullptr);
		REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
		this->hauler = assignment.hauler;
		this->initial_exchanges = assignment.exchanges;
		if (std::getenv("OTTD_M1_LOOP_ART") != nullptr) this->art = std::make_unique<NativeArtLoopProbe>(this->output, assignment);
		if (std::getenv("OTTD_M1_SOUND_PROOF") != nullptr) this->sound = std::make_unique<NativeSoundRuntimeProbe>(assignment);
		this->complete_loop = std::getenv("OTTD_TBM_LOGISTICS_COMPLETE_LOOP") != nullptr;
		if (const char *view = std::getenv("OTTD_TBM_LOGISTICS_VIEW"); view != nullptr) {
			this->presentation = view;
			REQUIRE((this->presentation == "headless" || this->presentation == "visible" || this->presentation == "hidden"));
		}
		this->Save("-loaded");
		AutoRestoreBackup actor(_current_company, project->company);
		if (assignment.wait == TBMHaulerWait::LostOrigin) this->RestoreOrigin(*project);
		if (assignment.wait == TBMHaulerWait::BlockedLine) this->RestoreLine(*project);
		if (assignment.wait == TBMHaulerWait::WrongWagons) this->RestoreWagons(assignment);
		if (assignment.wait == TBMHaulerWait::Crashed) {
			this->crash = std::make_unique<LogisticsCrashLedger>(assignment);
			REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project->id, TBMProjectAction::Pause).Succeeded());
		}
		// Insufficient empties cannot continue the same pass. Ordinary cancellation retains all paid geology/spoil
		// and permits the next legal final exchange without inventing additional wagons or accepting unpaid work.
		if (assignment.wait == TBMHaulerWait::NoEmpties) {
			REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, project->id, TBMProjectAction::Cancel).Succeeded());
		}
		// Only stopped real participants receive a start command; toggling an already moving train would alter the saved scenario.
		std::vector<VehicleID> participants{project->cutter};
		// Every actual peer keeps the same native start policy; the queue's recorded request order remains authoritative.
		for (const auto &peer : assignments) {
			REQUIRE(peer.project == project->id);
			participants.push_back(peer.hauler);
		}
		for (VehicleID id : participants) {
			const Train *train = Train::Get(id);
			// A wreck cannot be restarted and its surviving cutter is deliberately paused by the owning command.
			// Ordinary exchange scenarios start only genuinely stopped participants through their native command.
			if (this->crash == nullptr && train->vehstatus.Test(VehState::Stopped)) {
				REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, id, true).Succeeded());
			}
		}
		REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
		this->started = true;
		this->Present();
		if (this->art) this->art->Observe(assignment, this->ticks);
		if (this->sound) this->sound->Observe(assignment, this->ticks);
	}

	/**
	 * Observe the complete previous vehicle step at the next native frame boundary.
	 * @param elapsed Actual timer frames; this observer never calls a work or movement owner.
	 */
	void Observe(uint elapsed)
	{
		// Startup/finished frames are presentation only; they must not create a second transition or overwrite an existing output.
		if (!this->started || this->completed) return;
		this->ticks += elapsed;
		REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
		if (this->crash != nullptr) {
			REQUIRE(this->ticks < 6000);
			if (!this->crash->Observe()) return;
			this->Save("-crash-cleanup");
			fmt::print("LOGISTICS_CRASH_CLEANUP ticks={} destroyed_earth={} destroyed_rock={}\n", this->ticks,
				this->crash->destroyed[0], this->crash->destroyed[1]);
			this->completed = true;
			_exit_game = true; // Finish the isolated presentation loop only after genuine native deletion and its saved ledger.
			return;
		}
		const auto *assignment = GetTBMExchangeAssignmentForHauler(this->hauler);
		REQUIRE(assignment != nullptr);
		REQUIRE(this->ticks < 80000);
		if (this->art) this->art->Observe(*assignment, this->ticks);
		if (this->sound) this->sound->Observe(*assignment, this->ticks);
		if (this->complete_loop) {
			// Daily native view refresh must leave the same physical/cargo/cost result as a process with no observer camera calls.
			if (this->ticks % Ticks::DAY_TICKS == 0) this->Present();
			const auto *project = GetTBMProject(assignment->project);
			REQUIRE(project != nullptr);
			// Completion includes the final ordinary originating Surface visit and actual empty packets, not merely finished excavation.
			if (!IsTBMProjectFinished(project->phase) || assignment->phase != TBMHaulerPhase::ReturnEmpties || assignment->exchanges == 0) return;
			// Native station housekeeping can settle after its assignment reports empty return; observe only real physical packets here.
			for (const Vehicle *vehicle : Vehicle::Iterate()) REQUIRE(vehicle->cargo.TotalCount() == 0);
			this->Save("-complete-loop");
			if (this->art) this->art->Finish();
			if (this->sound) this->sound->Finish();
			if (this->presentation == "visible") CHECK(this->visible_cutter_draws > 0);
			fmt::print("LOGISTICS_COMPLETE_LOOP ticks={} exchanges={} presentation={} draws={} cutter_draws={}\n", this->ticks, assignment->exchanges,
				this->presentation, this->draws, this->visible_cutter_draws);
			this->completed = true;
			_exit_game = true;
			return;
		}
		if (assignment->exchanges == this->initial_exchanges) return;
		CHECK(assignment->exchanges == this->initial_exchanges + 1);
		this->Save("-next-exchange");
		fmt::print("LOGISTICS_NEXT_EXCHANGE ticks={} exchanges={} phase={} wait={}\n", this->ticks,
			assignment->exchanges, to_underlying(assignment->phase), to_underlying(assignment->wait));
		this->completed = true;
		_exit_game = true; // Stop the isolated application's presentation loop after its actual physical result was saved.
	}
};

static LogisticsRuntimeProbe *_logistics_runtime_probe = nullptr; ///< Process-local test observer; never present in the normal game executable.

/**
 * Start the test's loaded scenario through the ordinary startup console dispatch.
 * @param argv Actual console command arguments; no hidden gameplay fields are accepted.
 * @return True when the single test command was accepted.
 */
static bool StartLogisticsRuntimeProbe(std::span<std::string_view> argv)
{
	REQUIRE(argv.size() == 1);
	REQUIRE(_logistics_runtime_probe != nullptr);
	_logistics_runtime_probe->Start();
	return true;
}

/** Isolated process only: run the actual application loader and full native game loop to its next physical exchange. */
TEST_CASE("Loaded source logistics reaches its next physical exchange", "[.tbm-logistics-runtime]")
{
	const char *source = std::getenv("OTTD_TBM_LOGISTICS_SOURCE");
	const char *output = std::getenv("OTTD_TBM_LOGISTICS_OUTPUT");
	REQUIRE(source != nullptr);
	REQUIRE(output != nullptr);
	const auto root = std::filesystem::current_path().parent_path();
	const auto family = std::filesystem::weakly_canonical(root / "src/tests/fixtures/haulage");
	const auto portals = std::filesystem::weakly_canonical(root / "src/tests/fixtures/portals");
	// Actual portal-only inputs and logistics worlds stay in their existing owned fixture families; a test environment
	// cannot redirect this native writer into a user-wide save/content directory or an unrelated repository path.
	for (const char *path : {source, output}) {
		const auto parent = std::filesystem::weakly_canonical(std::filesystem::path(path).parent_path());
		REQUIRE((parent == family || parent == portals));
	}
	LogisticsRuntimeProbe probe;
	probe.output = output;
	AutoRestoreBackup observer(_logistics_runtime_probe, &probe);
	IConsole::CmdRegister("logistics_probe", StartLogisticsRuntimeProbe);
	/** @param elapsed Real game frames; timer dispatch does not grant simulation ownership to this observer. */
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&probe](uint elapsed) { probe.Observe(elapsed); });
	// Portal-only acceptance needs startup/reconstruction. Null video otherwise consumes its whole declared
	// tick budget after an exit request, causing unrelated minutes of legacy traffic after the saved checkpoint.
	const std::string_view video = std::getenv("OTTD_TBM_PORTAL_LOAD_ONLY") != nullptr ? "null:ticks=2" : "null:ticks=80000";
	const std::string_view config = std::getenv("OTTD_M1_LOOP_ART") != nullptr ? "native-art.cfg" : "grftest.cfg";
	std::vector<std::string_view> arguments = {"Debug/openttd_test.exe", "-v", video,
		"-s", "null", "-m", "null", "-x", "-X", "-c", config, "-g", source, "-d", "misc=5,sl=2,script=2"};
	if (probe.art) arguments.insert(arguments.end(), {"-I", "original_dos"});
	if (probe.sound) arguments.insert(arguments.end(), {"-S", "original_dos"});
	CHECK(openttd_main(arguments) == 0);
	CHECK(probe.started);
	CHECK(probe.completed);
}
