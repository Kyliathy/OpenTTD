/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file underground_depot_fixture.h Paid native chamber and separate connected haulage route for recurring depot exchange.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 7BB
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, Group 10R (hauler cycle diagnostics for face/bay meetings)
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md, task 7.3 (fund the route's missing pieces before quoting)
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13D (the further route keeps its breakpoints)
 */

#ifndef TESTS_UNDERGROUND_DEPOT_FIXTURE_H
#define TESTS_UNDERGROUND_DEPOT_FIXTURE_H

#include "underground_workflow_fixture.h"
#include "layer_save_fixture.h"
#include "../economy_func.h"
#include "../track_func.h"
#include "../rail_cmd.h"
#include "../layer_portal_cmd.h"
#include "../tbm_area_preview.h"
#include "../tbm_exchange.h"

/** Declare medium native prices and unit inflation before the world invoices its actual initial logistics. */
class NativeDepotEconomy {
	AutoRestoreBackup<decltype(_price)> prices{_price}; ///< Caller price table.
	AutoRestoreBackup<decltype(_economy)> economy{_economy}; ///< Caller inflation and economic state.
	AutoRestoreBackup<uint8_t> vehicle_costs{_settings_game.difficulty.vehicle_costs, uint8_t{1}}; ///< Medium vehicle price policy.
	AutoRestoreBackup<uint8_t> construction_cost{_settings_game.difficulty.construction_cost, uint8_t{1}}; ///< Medium construction price policy.
	AutoRestoreBackup<uint32_t> clearing_rate{_settings_game.construction.clear_per_64k_frames, 64U << 16}; ///< Native world_settings.ini default company allowance refill.
	AutoRestoreBackup<uint16_t> clearing_burst{_settings_game.construction.clear_frame_burst, uint16_t{4096}}; ///< Native default burst cap; the scenario's initial balance still comes from its company fixture.
	AutoRestoreBackup<TimerGameTick::TickCounter> frames{TimerGameTick::counter, 0}; ///< Declared new-scenario frame origin, restored for the caller after replay.
	AutoRestoreBackup<TimerGameEconomy::DateFract> day_slot{TimerGameEconomy::date_fract, TimerGameEconomy::DateFract{0}}; ///< Declared initial native daily scheduling slot; replay cannot inherit a previous scenario's partial day.
public:
	/** Set the same native base-currency price policy as the complete-project economics fixture. */
	NativeDepotEconomy() { _economy.inflation_prices = _economy.inflation_payment = 1 << 16; }
};

/** Own two paid job stages; construction may use only floor genuinely opened by the first native project. */
class UndergroundDepotWorkflow {
	NativeDepotEconomy economy; ///< Established before the world calculates or charges a price.
public:
	UndergroundWorkflowWorld world{MapLayer::Underground, true, true}; ///< Paid initial Surface logistics, solid target and actual native ticks.
	AutoRestoreBackup<bool> turns{_settings_game.pf.forbid_90_deg, false}; ///< Established after world pathfinder defaults; conventional chamber track allows sharp curves while advancing cutter passes retain their 45-degree guard.
	LayerSaveContent content{this->world.ActivatedContent(), true}; ///< Full-save identity/content transport, never a replacement owner.
	TBMProject first; ///< Read-only retained completed clearing project and its charged job identity.
	TileIndex depot = INVALID_TILE; ///< Actual funded underground depot, on paid floor.
	TileIndex connection = INVALID_TILE; ///< Separate actual Connected Surface/Underground haulage portal.
	Money commands = 0; ///< Exact native construction/control invoices after first clearing; the original machine stays withdrawn for its further assignment.
	Money first_project_cost = 0; ///< Actual cash spent after initial setup through complete clearing/disposal/billing.
	Money first_work_cost = 0; ///< Canonical paid geology/support invoice, retained after project retirement.
	uint64_t first_earth = 0; ///< Actual first-job spoil generation, fully delivered before construction.
	uint64_t first_rock = 0; ///< Actual first-job spoil generation, fully delivered before construction.

	/** Construct the accepted paid world; preparation is an explicit test action with ordinary command invoices. */
	UndergroundDepotWorkflow() { this->world.simulate_company_limits = true; }

	/**
	 * Execute one native owning command and retain its exact invoice.
	 * @tparam Tcmd Public synchronized command identity.
	 * @tparam Targs Its native typed argument types.
	 * @param args Ordinary command arguments; no private world state is supplied.
	 * @return Native result with the same charged cost and identities.
	 */
	template <Commands Tcmd, typename... Targs>
	auto Do(Targs... args)
	{
		auto result = Command<Tcmd>::Do(DoCommandFlag::Execute, args...);
		// The normal command kernel flushes the signal updates an executed command queues, such as new or removed track;
		// a direct call must do the same, or the next train leaving a depot meets a non-empty queue (master task 10R.1).
		UpdateSignalsInBuffer();
		INFO("depot command=" << GetCommandName(Tcmd) << " error=" << ExtractCommandCost(result).GetErrorMessage().base());
		REQUIRE(ExtractCommandCost(result).Succeeded());
		this->commands += ExtractCommandCost(result).GetCost();
		return result;
	}

	/**
	 * Fund a declared edge-adjacent rail route through existing command and terrain guards.
	 * @param path Actual endpoint and intermediate tiles; facility endpoints already exist and need no invented track bits.
	 * @param already_open_only True funds only existing paid work floor; solid future path cells remain exclusively for charged native cutting.
	 */
	void BuildRailPath(const std::vector<TileIndex> &path, bool already_open_only = false)
	{
		REQUIRE(path.size() >= 3);
		// Each intermediate cell receives the native track that joins its real entry/exit edges. Reuse existing
		// company pieces; adding a new branch still pays its own ordinary command invoice and cannot excavate rock.
		for (size_t index = 1; index + 1 < path.size(); ++index) {
			const TileIndex tile = path[index];
			REQUIRE(GetTileLayer(tile) == GetTileLayer(path.front()));
			REQUIRE(DistanceManhattan(tile, path[index - 1]) == 1);
			REQUIRE(DistanceManhattan(tile, path[index + 1]) == 1);
			// The reused project's new solid faces cannot receive ordinary rail until their paid cuts open them.
			if (already_open_only && GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated) continue;
			const DiagDirection entry = DiagdirBetweenTiles(tile, path[index - 1]);
			const DiagDirection exit = DiagdirBetweenTiles(tile, path[index + 1]);
			REQUIRE(IsValidDiagDirection(entry));
			REQUIRE(IsValidDiagDirection(exit));
			Track piece = Track::Invalid;
			// Native track directions, rather than a scratch geometry registry, decide which purchasable piece joins these edges.
			for (Trackdir trackdir : DiagdirReachesTrackdirs(ReverseDiagDir(entry))) {
				if (TrackdirToExitdir(trackdir) == exit) piece = TrackdirToTrack(trackdir);
			}
			REQUIRE(piece != Track::Invalid);
			INFO("rail path tile=" << TileX(tile) << ',' << TilePlaneY(tile) << " layer=" << uint{to_underlying(GetTileLayer(tile))}
				<< " piece=" << uint{to_underlying(piece)} << " type=" << uint{to_underlying(GetTileType(tile))}
				<< " excavated=" << (GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated));
			// A matching existing plain-rail piece is already paid; unrelated facilities are left to the normal command refusal.
			if (IsPlainRailTile(tile) && GetTrackBits(tile).Test(piece)) continue;
			this->Do<Commands::BuildRail>(tile, RAILTYPE_RAIL, piece, false);
		}
	}

	/** Complete actual paid wide clearing, withdraw its machine, convert released support and fund the separate depot network. */
	void Prepare()
	{
		Company *company = Company::Get(_current_company);
		const Money initial = company->money;
		const TBMProjectID project = RunUndergroundExcavationWorkflow(this->world, Ticks::DAY_TICKS, {}, {}, true);
		this->first = *GetTBMProject(project);
		const auto *job = GetExcavationJob(this->first.job);
		REQUIRE(job != nullptr);
		REQUIRE(job->state == ExcavationJobState::Completed);
		this->first_work_cost = job->completed_work_cost;
		this->first_earth = job->completed_earth_t;
		this->first_rock = job->completed_rock_t;
		ChargeTBMProjectCosts(project);
		this->world.hauler->OnNewEconomyDay();
		this->first_project_cost = initial - company->money;
		const Money before_construction = company->money;
		this->Do<Commands::ReleaseTBMHauler>(this->world.hauler->index);
		this->Do<Commands::StartStopVehicle>(this->world.hauler->index, true);
		this->Do<Commands::StartStopVehicle>(this->first.cutter, true);
		this->Do<Commands::ControlTBMProject>(project, TBMProjectAction::Retire);
		// The first finished/withdrawn job no longer protects temporary support; conversion retains each paid rail
		// piece as permanent track before new ordinary facilities may change that footprint.
		for (const auto &support : CaptureTBMSupport()) this->Do<Commands::ConvertTBMSupport>(support.tile);
		const uint x = TileX(this->world.rock), y = TilePlaneY(this->world.rock);
		this->depot = TileXYL(x + 2, y + 3, MapLayer::Underground);
		REQUIRE(GetGeologyWorldState().terrain.Get(this->depot) == ExcavationState::Excavated);
		// Only released existing track may be removed for the bay. No facility opens its own ground or steals active retreat support.
		if (IsPlainRailTile(this->depot)) {
			const TrackBits pieces = GetTrackBits(this->depot);
			for (Track piece : pieces) this->Do<Commands::RemoveRail>(this->depot, piece);
		}
		this->Do<Commands::BuildRailDepot>(this->depot, RAILTYPE_RAIL, DiagDirection::SE);
		this->connection = TileXYL(x + 3, y + 10, MapLayer::Surface);
		this->Do<Commands::BuildLayerPortal>(this->connection, TransportType::Rail, MapLayer::Underground, DiagDirection::NW, RAILTYPE_RAIL, ROADTYPE_ROAD);
		const TileIndex lower = GetLayerPortalPairTile(this->connection);
		REQUIRE(GetGeologyWorldState().terrain.Get(lower) == ExcavationState::Excavated);
		std::vector<TileIndex> underground{lower};
		underground.push_back(GetLayerPortalDoorTile(lower));
		// The first paid chamber supplies a separate eastern freight lane and the complete straight bay approach.
		// Freight turns toward the depot before reaching the next project's solid southern faces.
		for (uint row = y + 8; row > y + 7; --row) underground.push_back(TileXYL(x + 3, row, MapLayer::Underground));
		for (uint column = x + 3; column > x + 2; --column) underground.push_back(TileXYL(column, y + 7, MapLayer::Underground));
		for (uint row = y + 7; row > y + 3; --row) underground.push_back(TileXYL(x + 2, row, MapLayer::Underground));
		underground.push_back(this->depot);
		this->BuildRailPath(underground);
		std::vector<TileIndex> surface{this->connection};
		// Keep the portal's complete door cell straight on its real shaft axis before turning into the outer Surface route.
		surface.push_back(GetLayerPortalDoorTile(this->connection));
		const uint outside = x - 6, around = y + 12, origin_row = TilePlaneY(this->world.origin->train_station.tile), join = origin_row - 2;
		// Route around the western Surface side, away from the real receivers and original commissioning pocket.
		// Existing Surface logistics remains the only disposal destination; the new connection creates no catchment transfer.
		for (uint column = x + 3; column >= outside; --column) surface.push_back(TileXYL(column, around, MapLayer::Surface));
		for (uint row = around - 1; row >= join; --row) surface.push_back(TileXYL(outside, row, MapLayer::Surface));
		for (uint column = outside + 1; column <= x; ++column) surface.push_back(TileXYL(column, join, MapLayer::Surface));
		// Join the existing upstream rail before the left-side receiver footprint; continue through its already paid straight approach.
		for (uint row = join + 1; row < origin_row; ++row) surface.push_back(TileXYL(x, row, MapLayer::Surface));
		surface.push_back(this->world.origin->train_station.tile);
		this->BuildRailPath(surface);
		this->Do<Commands::ChangeLayerPortalConnection>(this->connection, true);
		CHECK(before_construction - company->money == this->commands);
		CHECK(GetLayerPortalWorkState(this->connection) == LayerPortalWorkState::Connected);
		CHECK(CaptureTBMProjects().empty());
	}

	/**
	 * Fund the existing-floor pieces that the further southern route's support needs, by ordinary construction, as the
	 * player does before buying. Until they exist, the quote names the first missing piece (r2df task 7.3); the refused
	 * quote still carries the planned route.
	 */
	void FundFurtherRoute()
	{
		const auto consist = GetDefaultTBMConsist(MapLayer::Underground);
		const auto quote = PreviewTBMCommissionArea(_current_company, this->world.portal, this->first.model_id, consist.earth_wagons, consist.rock_wagons,
			this->world.origin->index, this->world.site, TileAddWrap(this->world.site, 2, 18), false);
		if (quote.purchase.Failed()) REQUIRE(quote.purchase.GetErrorMessage() == STR_ERROR_TBM_COMMISSION_SUPPORT_PIECE);
		const ExcavationJob &route = quote.purchase.Succeeded() ? quote.request.proposal : quote.area.proposal;
		std::vector<size_t> ends;
		const auto path = GetTBMWorkRailPath(route, route.route.size(), ends);
		REQUIRE(ends.size() == route.route.size());
		// Funding the next route's exact existing-floor pieces is ordinary construction, not an unpaid support mutation inside cutting.
		this->BuildRailPath(path, true);
	}

	/**
	 * Fund the cutter's safe return branch and assign a further southern cut to its actual retained machine.
	 * @return Actual new project identity, bound to the same source pair and native underground depot.
	 */
	TBMProjectID StartFurther()
	{
		// Since R2TBMF task 5B.1 halved the yields, the Soft further route froze one breakpoint and finished at its first
		// depot exchange, so no recurring exchange, saved wait or queued turn came after it. Mixed rock restores the
		// former 12 t Rock per cell, and so the route's two breakpoints before its end (r2mlvs5 task 13D.1).
		this->world.AppendRockSeam(GeologyHardness::Mixed, this->world.site, TileAddWrap(this->world.site, 2, 18));
		const uint x = TileX(this->world.rock), y = TilePlaneY(this->world.rock);
		// The cutter returns from the southern faces into the eastern junction below the full straight depot approach.
		// The hauler's separate eastern lane joins there before the active cutter corridor, rather than passing its work face.
		this->BuildRailPath({TileXYL(x, y + 8, MapLayer::Underground), TileXYL(x, y + 7, MapLayer::Underground),
			TileXYL(x + 1, y + 7, MapLayer::Underground), TileXYL(x + 2, y + 7, MapLayer::Underground),
			TileXYL(x + 2, y + 6, MapLayer::Underground), TileXYL(x + 2, y + 5, MapLayer::Underground),
			TileXYL(x + 2, y + 4, MapLayer::Underground), this->depot});
		const auto consist = GetDefaultTBMConsist(MapLayer::Underground);
		this->FundFurtherRoute();
		REQUIRE(PreviewTBMCommissionArea(_current_company, this->world.portal, this->first.model_id, consist.earth_wagons, consist.rock_wagons,
			this->world.origin->index, this->world.site, TileAddWrap(this->world.site, 2, 18), false).purchase.Succeeded());
		this->Do<Commands::CommissionTBM>(this->world.portal, this->first.model_id, consist.earth_wagons, consist.rock_wagons,
			this->world.origin->index, this->world.site, TileAddWrap(this->world.site, 2, 18), false);
		const auto projects = CaptureTBMProjects();
		REQUIRE(projects.size() == 1);
		const TBMProjectID id = projects.front().id;
		CHECK(projects.front().cutter == this->first.cutter);
		this->Do<Commands::SetTBMExchangeBay>(id, this->depot);
		this->Do<Commands::AssignTBMHauler>(id, this->world.hauler->index);
		this->Do<Commands::StartStopVehicle>(this->first.cutter, true);
		this->Do<Commands::StartStopVehicle>(this->world.hauler->index, true);
		return id;
	}

	/**
	 * Drive only native whole-world frames until an actual consumer checkpoint.
	 * @param done Read-only condition on the real project/assignment/physical trains.
	 * @param limit Most physical frames allowed for this focused checkpoint.
	 * @param observe Optional read-only custody/receipt observer after each complete native frame.
	 * @return Frames actually driven; absence of native progress fails with current physical state.
	 */
	uint DriveUntil(const std::function<bool()> &done, uint limit = 50000, const std::function<void()> &observe = {})
	{
		uint frames = 0;
		// A single native frame owns work, physical trains, station disposal and company allowance. No observer
		// changes a phase, position, packet or private order to compensate for a missing depot consumer.
		while (!done() && frames < limit) {
			this->world.Tick();
			if (observe) observe();
			REQUIRE_FALSE(Train::Get(this->first.cutter)->vehstatus.Test(VehState::Crashed));
			REQUIRE_FALSE(this->world.hauler->vehstatus.Test(VehState::Crashed));
			REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
			++frames;
		}
		INFO("depot cutter=" << DescribeTrain(Train::Get(this->first.cutter)));
		INFO("depot hauler=" << DescribeTrain(this->world.hauler));
		INFO("elapsed=" << frames);
		const auto projects = CaptureTBMProjects();
		REQUIRE_FALSE(projects.empty());
		INFO("project phase=" << uint{to_underlying(projects.front().phase)} << " hold=" << uint{to_underlying(projects.front().blocked)});
		// The hauler's own cycle and how it meets the cutter (face or bay, r2df2) explain a stall the train dump cannot.
		std::string cycle = "unassigned";
		if (const auto *assignment = GetTBMExchangeAssignmentForHauler(this->world.hauler->index); assignment != nullptr) {
			const auto *owner = GetTBMProject(assignment->project);
			cycle = "project=" + std::to_string(assignment->project.base()) + " phase=" + std::to_string(to_underlying(assignment->phase)) +
				" wait=" + std::to_string(to_underlying(assignment->wait)) + " bay=" + std::to_string(assignment->bay.base()) +
				" exchanges=" + std::to_string(assignment->exchanges);
			if (owner != nullptr) {
				cycle += " project_phase=" + std::to_string(to_underlying(owner->phase)) + " project_bay=" + std::to_string(owner->exchange_bay.base()) +
					" face=" + std::to_string(GetTBMMeetingKind(*owner) == TBMMeetingKind::Face) + " turn_holder=" + std::to_string(IsTBMFaceTurnHolder(*assignment));
			}
		}
		INFO("hauler cycle " << cycle);
		const auto *job = GetExcavationJob(projects.front().job);
		REQUIRE(job != nullptr);
		const auto cut = EstimateExcavationCut(job->id);
		const auto support = PrepareTBMSupport(projects.front(), *job);
		INFO("cursor=" << job->cursor << " cut_error=" << uint{to_underlying(cut.error)} << " cut_tile=" << cut.failure_tile.base()
			<< " support_error=" << uint{to_underlying(support.error)} << " support_tile=" << support.failure_tile.base());
		const auto goal = GetTBMCutterSteeringTarget(*Train::Get(this->first.cutter));
		const Train *front = Train::Get(this->first.cutter);
		INFO("stuck=" << front->flags.Test(VehicleRailFlag::Stuck) << " lost=" << front->vehicle_flags.Test(VehicleFlag::PathfinderLost)
			<< " held=" << IsTBMHoldingTrain(*front) << " dest=" << front->dest_tile.base());
		INFO("forbid90=" << _settings_game.pf.forbid_90_deg << " backward=" << front->vehicle_flags.Test(VehicleFlag::DrivingBackwards)
			<< " power=" << front->gcache.cached_power << " maxspeed=" << front->gcache.cached_max_track_speed << " curve=" << front->tcache.cached_max_curve_speed);
		if (goal.has_value()) {
			INFO("goal=" << goal->tile.base() << " trackdirs=" << goal->trackdirs.base());
			if (job->cursor != 0) INFO("last heading=" << uint{to_underlying(job->route[job->cursor - 1].heading)});
			REQUIRE(done());
		}
		REQUIRE(done());
		return frames;
	}
};

#endif /* TESTS_UNDERGROUND_DEPOT_FIXTURE_H */
