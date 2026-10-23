/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file tube_tbm_clearance_fixture.h Actual paid non-rectangular clearing and native tube replacement on its earned floor.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md Group21.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_TUBE_TBM_CLEARANCE_FIXTURE_H
#define TESTS_TUBE_TBM_CLEARANCE_FIXTURE_H

#include "underground_depot_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../tbm_replanning.h"
#include "../transport_tube.h"
#include "../saveload/saveload_func.h"
#include "../gfx_func.h"
#include "../table/sprites.h"

/** One generated native world whose L-shaped terminal/corridor floor is earned exclusively by its original paid cutter and spoil trains. */
class TubeTBMClearanceRig {
	NativeDepotEconomy economy; ///< Canonical nonzero native construction and operating prices.
	AutoRestoreBackup<bool> turns{_settings_game.pf.forbid_90_deg, false}; ///< Ordinary chamber rail permits sharp track curves; cutter advances still obey the45-degree authority.
public:
	UndergroundWorkflowWorld world{MapLayer::Underground, true, true}; ///< Generated company/logistics inputs; target rock starts untouched.
	LayerSaveContent content{this->world.ActivatedContent(), true}; ///< Exact original/native identities for genuine complete saves.
	TBMProject original; ///< Copied observation after actual completion, never staged back into a controller.
	ExcavationJob completed; ///< Original earned cuts,quantities and requested non-rectangular outline.
	Money construction_cost = 0; ///< Observed native command invoices after excavation,not another cost authority.
	RailType tube = INVALID_RAILTYPE; ///< Actual canonical dedicated rail domain.
	StationID origin = StationID::Invalid(); ///< Actual native tube loading terminal.
	StationID interchange = StationID::Invalid(); ///< Actual native tube transfer terminal.
	std::array<TileIndex, 2> depots{}; ///< Actual command-built tube service ends.
	uint advancing_bends = 0; ///< Observed legal heading changes in the original accepted job.
	uint extension_count = 0; ///< Actual reviewed native extensions,not a saved planning owner.

	/** Construct only disclosed generated scenario/logistics inputs; no future tube floor is opened. */
	TubeTBMClearanceRig()
	{
		// Complete the initial generated catalogue through the same native
		// startup owner as a new game. Otherwise unbought capsule definitions
		// would be absent from ENGN and reconstructed without saved availability
		// when the actual paid construction world is loaded for useful freight.
		REQUIRE(InitialiseTubeRailType(this->world.ActivatedContent()) != INVALID_RAILTYPE);
		REQUIRE(InitialiseTubeEngineDefinitions(this->world.ActivatedContent()));
		StartupEngines();
		this->world.simulate_company_limits = true;
	}

	/**
	 * Execute one ordinary construction/control action through the paid command kernel.
	 * @tparam Tcmd Actual synchronized native command identity.
	 * @tparam Targs Native command argument types.
	 * @param args Actual arguments on the original earned floor.
	 * @return Native result after successful admission,invoice and signal flush.
	 */
	template <Commands Tcmd, typename... Targs>
	auto Do(Targs... args)
	{
		auto result = Command<Tcmd>::Do(DoCommandFlag::Execute, args...);
		INFO("paid tube command=" << GetCommandName(Tcmd) << " refusal=" << ExtractCommandCost(result).GetErrorMessage().base());
		REQUIRE(ExtractCommandCost(result).Succeeded());
		UpdateSignalsInBuffer();
		this->construction_cost += ExtractCommandCost(result).GetCost();
		return result;
	}

	/**
	 * Select a physical target relative to the unchanged generated seam anchor.
	 * @param x Layer-local horizontal offset from the original target.
	 * @param y Layer-local vertical offset from the original target.
	 * @return Actual same-plane world tile; this lookup opens no floor.
	 */
	TileIndex At(int x, int y) const { return TileAddWrap(this->world.site, x, y); }

	/** Earn the complete L-shaped floor by actual paid work,physical Surface disposal and whole native withdrawal. */
	void Clear()
	{
		/**
		 * Append the reviewed horizontal wing near the real unfinished end of the existing job.
		 * @param current Same physically stepped original world; only public commands may change its work request.
		 */
		const auto extend = [&](const UndergroundWorkflowWorld &current) {
			if (this->extension_count == 1) return;
			const auto projects = CaptureTBMProjects();
			if (projects.empty()) return;
			const auto &project = projects.front();
			const auto *job = GetExcavationJob(project.job);
			// Append only at the final genuinely unfinished cut. Extending during
			// withdrawal or before useful progress would not demonstrate a live
			// paid job retaining its original machine and material history.
			if (IsTBMProjectFinished(project.phase) || job->cursor >= job->route.size() || job->cursor + 1 < job->route.size()) return;
			this->Do<Commands::ControlTBMProject>(project.id, TBMProjectAction::Pause);
			const auto first = TileAddWrap(current.site, 5, 3), last = TileAddWrap(current.site, 12, 8);
			const auto before = std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), Company::Get(_current_company)->money,
				_random.state[0], _random.state[1]};
			const auto quote = PreviewTBMExtension(_current_company, project.id, first, last);
			INFO("paid L-shaped wing refusal=" << quote.result.GetErrorMessage().base() << " plan=" << to_underlying(quote.plan.error));
			REQUIRE(quote.result.Succeeded());
			CHECK((std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), Company::Get(_current_company)->money,
				_random.state[0], _random.state[1]} == before));
			this->Do<Commands::ExtendTBMProject>(project.id, first, last, quote.job_revision, quote.fingerprint, ExcavationPlanIntent::Area);
			CHECK(GetTBMProject(project.id)->cutter == project.cutter);
			CHECK(GetTBMProject(project.id)->job == project.job);
			this->Do<Commands::ControlTBMProject>(project.id, TBMProjectAction::Resume);
			++this->extension_count;
		};
		const auto id = RunUndergroundExcavationWorkflow(this->world, 0, {}, extend, true);
		this->original = *GetTBMProject(id);
		this->completed = *GetExcavationJob(this->original.job);
		REQUIRE(this->extension_count == 1);
		REQUIRE(this->completed.state == ExcavationJobState::Completed);
		REQUIRE(this->completed.completed_work_cost > 0);
		REQUIRE(this->completed.completed_earth_t > 0);
		REQUIRE(this->completed.completed_rock_t > 0);
		// Every actual advancing heading change remains bounded by its original
		// paid route authority. The eventual tube curve cannot stand in for
		// real cutter bends or authorize an uncharged corner.
		for (size_t index = 1; index < this->completed.route.size(); ++index) {
			const auto before = this->completed.route[index - 1].heading, after = this->completed.route[index].heading;
			if (before == after) continue;
			CHECK((ChangeDir(before, DirDiff::Left45) == after || ChangeDir(before, DirDiff::Right45) == after));
			++this->advancing_bends;
		}
		REQUIRE(this->advancing_bends > 0);
		REQUIRE(std::ranges::binary_search(this->completed.requested_tiles, this->At(0, 0)));
		REQUIRE(std::ranges::binary_search(this->completed.requested_tiles, this->At(12, 8)));
		CHECK_FALSE(std::ranges::binary_search(this->completed.requested_tiles, this->At(10, 0)));
		CHECK(GetGeologyWorldState().terrain.Get(this->At(10, 0)) == ExcavationState::Solid);
		ChargeTBMProjectCosts(id);
		this->world.hauler->OnNewEconomyDay();
		this->Do<Commands::ReleaseTBMHauler>(this->world.hauler->index);
		if (!this->world.hauler->vehstatus.Test(VehState::Stopped)) this->Do<Commands::StartStopVehicle>(this->world.hauler->index, true);
		if (!Train::Get(this->original.cutter)->vehstatus.Test(VehState::Stopped)) this->Do<Commands::StartStopVehicle>(this->original.cutter, true);
		this->Do<Commands::ControlTBMProject>(id, TBMProjectAction::Retire);
		REQUIRE(GetTBMProject(id) == nullptr);
		// The native completed workflow already requires every cutter/hauler
		// packet empty and every spoil tonne at its actual Surface receiver.
		// Only now may ordinary removal expose the final non-rail floor.
		for (const auto &support : CaptureTBMSupport()) {
			REQUIRE_FALSE(IsTBMSupportInUse(support.tile));
			const auto tracks = GetTrackBits(support.tile);
			for (const auto track : tracks) this->Do<Commands::RemoveRail>(support.tile, track);
		}
		REQUIRE(CaptureTBMSupport().empty());
	}

	/**
	 * Construct a real native path through already earned same-layer ground.
	 * @param path Consecutive edge-adjacent physical cells; actual facilities already own their endpoints.
	 * @param type Native tube or conventional rail domain.
	 */
	void BuildPath(const std::vector<TileIndex> &path, RailType type)
	{
		REQUIRE(path.size() >= 3);
		// Native track directions derive each purchasable corner from the real
		// neighboring cells. No test-owned track registry creates a route.
		for (size_t index = 1; index + 1 < path.size(); ++index) {
			const auto tile = path[index];
			if (IsRailStationTile(tile)) continue;
			REQUIRE(DistanceManhattan(tile, path[index - 1]) == 1);
			REQUIRE(DistanceManhattan(tile, path[index + 1]) == 1);
			const auto entry = DiagdirBetweenTiles(tile, path[index - 1]), exit = DiagdirBetweenTiles(tile, path[index + 1]);
			Track piece = Track::Invalid;
			for (const auto directed : DiagdirReachesTrackdirs(ReverseDiagDir(entry))) {
				if (TrackdirToExitdir(directed) == exit) piece = TrackdirToTrack(directed);
			}
			REQUIRE(piece != Track::Invalid);
			this->Do<Commands::BuildRail>(tile, type, piece, false);
		}
	}

	/** Build actual terminal/depot owners and a curved guideway entirely on the same paid L-shaped floor. */
	void BuildTubes()
	{
		const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
		const auto cash = Company::Get(_current_company)->money;
		const auto old_cost = this->construction_cost;
		this->tube = InitialiseTubeRailType(this->world.ActivatedContent());
		REQUIRE(this->tube != INVALID_RAILTYPE);
		Company::Get(_current_company)->avail_railtypes.Set(this->tube);
		this->depots = {this->At(4, 0), this->At(12, 8)};
		this->Do<Commands::BuildRailDepot>(this->depots[0], this->tube, DiagDirection::SE);
		this->Do<Commands::BuildRailDepot>(this->depots[1], this->tube, DiagDirection::NE);
		this->Do<Commands::BuildRailStation>(this->At(4, 2), this->tube, Axis::Y, uint8_t{1}, uint8_t{2}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
		this->origin = Station::GetByTile(this->At(4, 2))->index;
		this->Do<Commands::BuildRailStation>(this->At(10, 8), this->tube, Axis::X, uint8_t{1}, uint8_t{2}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
		this->interchange = Station::GetByTile(this->At(10, 8))->index;
		std::vector<TileIndex> path;
		// The vertical original cavern and horizontal paid extension form a
		// real curved route; its source and transfer berths retain their actual
		// complete native footprints and same-layer cargo owners.
		for (int row = 0; row <= 8; ++row) path.push_back(this->At(4, row));
		for (int column = 5; column <= 12; ++column) path.push_back(this->At(column, 8));
		this->BuildPath(path, this->tube);
		CHECK(cash - Company::Get(_current_company)->money == this->construction_cost - old_cost);
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
		CHECK(*GetExcavationJob(this->completed.id) == this->completed);
		fmt::print("TUBE_TBM_CLEAR ticks={} cuts={} bends={} extensions={} earth={} rock={} work={} construction={} origin={} interchange={}\n",
			this->world.ticks, this->completed.cursor, this->advancing_bends, this->extension_count,
			this->completed.completed_earth_t, this->completed.completed_rock_t, this->completed.completed_work_cost,
			this->construction_cost, this->origin.base(), this->interchange.base());
	}

	/** Retain the exact original paid construction world for ordinary loaded useful-route continuation; saving creates no simulation opportunity. */
	void SaveReady() const
	{
		AutoRestoreBackup local{_local_company, _current_company};
		SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
		AutoRestoreBackup menu{_game_mode, GameMode::Menu};
		AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
		AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
		/** @return Original paid geology,work,exchange,clock,cash and native RNG owners; the writer must not advance or repair them. */
		const auto owners = [&]() {
			return std::tuple{ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(), ComputeTBMExchangeDigest(),
				Company::Get(_current_company)->money, TimerGameTick::counter, TimerGameEconomy::date_fract,
				_random.state[0], _random.state[1], Vehicle::GetNumItems()};
		};
		const auto before = owners();
		const auto path = GetTestCaseArtifactPaths("tubes", "tbm-final-use").Output("saves", "tube-tbm-cleared-ready.sav");
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
		CHECK(owners() == before);
		fmt::print("TUBE_TBM_OWNERS site={} portal={} cutter={} hauler={} earth_receiver={} rock_receiver={} job={} depot0={} depot1={} cash={}\n",
			this->world.site.base(), this->world.portal.base(), this->original.cutter.base(), this->world.hauler->index.base(),
			this->world.earth->index.base(), this->world.stone->index.base(), this->completed.id.base(), this->depots[0].base(), this->depots[1].base(),
			Company::Get(_current_company)->money.base());
		fmt::print("TUBE_TBM_SAVE path={}\n", path.string());
	}
};

#endif /* TESTS_TUBE_TBM_CLEARANCE_FIXTURE_H */
