/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_support.cpp Temporary support track laid by the cut transaction, the saved registry of its metadata, and
 * the rules for its removal, conversion and traversal. The track is ordinary railway of the project's company;
 * geology keeps the excavation state beneath it.
 * The work corridor and its occupancy serve Group 14.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 8, 9 and 14
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Group 9B (canonical cutter steering)
 */

#include "stdafx.h"
#include "tbm.h"
#include "tbm_cmd.h"
#include "tbm_exchange.h"
#include "command_func.h"
#include "train.h"
#include "company_func.h"
#include "rail.h"
#include "rail_map.h"
#include "company_base.h"
#include "company_gui.h"
#include "layer_portal_map.h"
#include "map_func.h"
#include "signal_func.h"
#include "viewport_func.h"
#include "track_func.h"
#include "pathfinder/yapf/yapf_cache.h"
#include "vehicle_func.h"

#include "table/strings.h"

#include "safeguards.h"

static std::vector<TBMSupportTile> _tbm_support; ///< Live registry in strictly increasing tile order.
static std::optional<std::vector<TBMSupportTile>> _pending_tbm_support; ///< Unpublished loaded registry.

/** Classes that may use newly laid support: the consist itself and the haulers that collect its wagons. */
static constexpr TBMSupportClasses TBM_SUPPORT_ALLOWED{TBMSupportClass::Cutter, TBMSupportClass::SpoilWagon, TBMSupportClass::Hauler};

/**
 * @param tile Tile to find.
 * @return Iterator to the tile's entry, or to the position it would take.
 */
static std::vector<TBMSupportTile>::iterator FindTBMSupport(TileIndex tile)
{
	return std::ranges::lower_bound(_tbm_support, tile, {}, &TBMSupportTile::tile);
}

/**
 * @param tile Any tile.
 * @return The support metadata on this tile, or nullptr.
 */
const TBMSupportTile *GetTBMSupportTile(TileIndex tile)
{
	const auto it = FindTBMSupport(tile);
	return it == _tbm_support.end() || it->tile != tile ? nullptr : &*it;
}

/** @return Copy of the registry in increasing tile order. */
std::vector<TBMSupportTile> CaptureTBMSupport()
{
	return _tbm_support;
}

/**
 * Piece that enters a tile through one edge and leaves through another.
 * @param entry Edge the path enters through.
 * @param exit Edge the path leaves through.
 * @return The connecting piece, or Track::Invalid for a reversal.
 */
static Track GetTBMSupportTrack(DiagDirection entry, DiagDirection exit)
{
	// Entering through an edge means moving away from it; one of the reachable pieces leaves through the wanted edge.
	for (Trackdir trackdir : DiagdirReachesTrackdirs(ReverseDiagDir(entry))) {
		if (TrackdirToExitdir(trackdir) == exit) return TrackdirToTrack(trackdir);
	}
	return Track::Invalid;
}

/**
 * Edge-connected support path from the housing through the job's first cutter positions.
 * A diagonal advance passes through the side cell that turns away from the entry edge, except straight after the
 * housing, where it passes straight on first.
 * @param job Accepted job.
 * @param positions Number of route positions to include.
 * @param[out] ends Index in the returned path of each included position.
 * @return Tiles from the housing onward; empty when two positions are not neighbours or a diagonal leaves the housing.
 */
static std::vector<TileIndex> ExpandTBMSupportPath(const ExcavationJob &job, size_t positions, std::vector<size_t> &ends)
{
	std::vector<TileIndex> path{job.commissioning_anchor};
	ends.clear();
	// Each position follows its predecessor; a corner neighbour needs one side cell so every step crosses a tile edge.
	for (size_t i = 0; i < positions && i < job.route.size(); ++i) {
		const TileIndex from = path.back();
		const TileIndex to = job.route[i].cutter_position;
		const int dx = static_cast<int>(TileX(to)) - static_cast<int>(TileX(from));
		const int dy = static_cast<int>(TilePlaneY(to)) - static_cast<int>(TilePlaneY(from));
		if (GetTileLayer(to) != GetTileLayer(from)) return {};
		if (std::abs(dx) == 1 && std::abs(dy) == 1) {
			// The housing has only its door edge, so the first advance must leave straight through it.
			if (path.size() < 2) return {};
			const TileIndex previous = path[path.size() - 2];
			const bool entered_along_x = TilePlaneY(previous) == TilePlaneY(from);
			// Normally turn first, so successive diagonal advances alternate corner pieces. Right after the housing go
			// straight first, because the piece next to a portal mouth must be the axis track that joins it.
			const bool turn_first = path.size() > 2;
			const bool step_along_y = entered_along_x == turn_first;
			const TileIndex side = step_along_y ? TileAddWrap(from, 0, dy) : TileAddWrap(from, dx, 0);
			if (side == INVALID_TILE) return {};
			path.push_back(side);
		} else if (std::abs(dx) + std::abs(dy) != 1) {
			return {};
		}
		path.push_back(to);
		ends.push_back(path.size() - 1);
	}
	return path;
}

/** @copydoc CheckTBMSupportRailTurns */
bool CheckTBMSupportRailTurns(const ExcavationJob &job)
{
	const RailType railtype = GetRailType(job.commissioning_anchor);
	if (!Rail90DegTurnDisallowed(railtype, railtype)) return true;
	std::vector<size_t> ends;
	const auto path = ExpandTBMSupportPath(job, job.route.size(), ends);
	if (ends.size() != job.route.size()) return false;
	Track previous = DiagDirToDiagTrack(GetLayerPortalDoorDirection(job.commissioning_anchor));
	// Validate every physical corner, including the diagonal side cells: legal cutter heading changes can still form a forbidden ordinary rail bend.
	for (size_t index = 1; index < path.size(); ++index) {
		const DiagDirection entry = DiagdirBetweenTiles(path[index], path[index - 1]);
		const DiagDirection exit = index + 1 < path.size() ? DiagdirBetweenTiles(path[index], path[index + 1]) : ReverseDiagDir(entry);
		// Invalid geometry cannot be presented as traversable, regardless of the route's geological coverage.
		if (entry == DiagDirection::Invalid || exit == DiagDirection::Invalid) return false;
		const Track track = GetTBMSupportTrack(entry, exit);
		// The physical controller masks this same sharp-turn pair; refuse it before buying a machine that would stall there.
		if (track == Track::Invalid || TrackCrossesTracks(previous).Test(track)) return false;
		previous = track;
	}
	return true;
}

/** @copydoc GetTBMCutterSteeringTarget */
std::optional<TBMCutterSteeringTarget> GetTBMCutterSteeringTarget(const Train &front)
{
	const auto *project = GetTBMProjectForCutter(front.First()->index);
	// Ordinary trains and actual service visits follow player/depot orders; only project travel supplies a derived destination.
	if (project == nullptr || front.current_order.IsType(OT_GOTO_DEPOT)) return std::nullopt;
	if (project->phase == TBMPhase::ReturnForExchange) {
		const auto bay = GetTBMExchangeBay(*project);
		if (bay.error != TBMExchangeError::None) return std::nullopt;
		const DiagDirection door = bay.kind == TBMExchangeBayKind::Source ? GetLayerPortalDoorDirection(bay.tile) : GetRailDepotDirection(bay.tile);
		return TBMCutterSteeringTarget{bay.tile, TrackdirToTrackdirBits(DiagDirToDiagTrackdir(ReverseDiagDir(door)))};
	}
	// Work/wait phases hold through their existing train controller; only approach and advancing onto paid support need pathfinding.
	if (project->phase != TBMPhase::TravelToFace && project->phase != TBMPhase::AdvanceSupport) return std::nullopt;
	const auto *job = GetExcavationJob(project->job);
	// Missing or malformed live geometry cannot authorize another route or a fabricated waypoint.
	if (job == nullptr || job->cursor > job->route.size()) return std::nullopt;
	const TileIndex target = job->cursor == 0 ? job->commissioning_anchor : job->route[job->cursor - 1].cutter_position;
	// Match ObserveTBMPhaseInputs' physical arrival condition on the existing face tile. Revisited support can have several valid approaches; only actual laid track supplies directions.
	return TBMCutterSteeringTarget{target, GetTileTrackStatus(target, TransportType::Rail, RoadTramType::Invalid).trackdirs};
}

/**
 * Prepare the support pieces of the job's next cut without changing anything.
 * @param project Assigned project.
 * @param job Its live job, with the cursor at the cut to support.
 * @return The pieces, or a refusal naming the first tile that cannot take support.
 */
TBMSupportPlan PrepareTBMSupport(const TBMProject &project, const ExcavationJob &job)
{
	TBMSupportPlan plan;
	/**
	 * Finish a refusal.
	 * @param reason Precise wait.
	 * @param tile Failing tile.
	 * @return The refused plan.
	 */
	const auto refuse = [&plan](TBMBlockedReason reason, TileIndex tile) {
		plan.error = reason;
		plan.failure_tile = tile;
		plan.pieces.clear();
		return plan;
	};
	// Support starts at this project's own rail housing; any other anchor cannot connect to the pair.
	if (job.cursor >= job.route.size() || !IsValidTile(job.commissioning_anchor) || !IsTileType(job.commissioning_anchor, TileType::LayerPortal) || GetLayerPortalTransportType(job.commissioning_anchor) != TransportType::Rail || GetTileOwner(job.commissioning_anchor) != project.company) return refuse(TBMBlockedReason::Obstruction, job.commissioning_anchor);
	plan.railtype = GetRailType(job.commissioning_anchor);

	/* Path through the next planned position too, so the last new piece already points where the train goes next. */
	std::vector<size_t> ends;
	const auto path = ExpandTBMSupportPath(job, job.cursor + 2, ends);
	if (ends.size() <= job.cursor) return refuse(TBMBlockedReason::Obstruction, job.route[job.cursor].cutter_position);
	const size_t first = job.cursor == 0 ? 1 : ends[job.cursor - 1] + 1;
	const size_t last = ends[job.cursor];
	const auto &cut = job.route[job.cursor];
	const auto &geology = GetGeologyWorldState();
	size_t new_tiles = 0;
	// Each path cell of this cut gets the one piece that joins its path neighbours, unless that piece is already there.
	for (size_t index = first; index <= last; ++index) {
		const TileIndex tile = path[index];
		const DiagDirection entry = DiagdirBetweenTiles(tile, path[index - 1]);
		const DiagDirection exit = index + 1 < path.size() ? DiagdirBetweenTiles(tile, path[index + 1]) : ReverseDiagDir(entry);
		if (entry == DiagDirection::Invalid || exit == DiagDirection::Invalid) return refuse(TBMBlockedReason::Obstruction, tile);
		const Track track = GetTBMSupportTrack(entry, exit);
		if (track == Track::Invalid) return refuse(TBMBlockedReason::Obstruction, tile);
		// Support may only lie on ground this very cut opens, or on ground that is already open.
		const bool swept = std::binary_search(cut.swept_tiles.begin(), cut.swept_tiles.end(), tile);
		if (!swept && geology.terrain.Get(tile) != ExcavationState::Excavated) return refuse(TBMBlockedReason::Obstruction, tile);
		if (GetTileSlope(tile) != SLOPE_FLAT) return refuse(TBMBlockedReason::Obstruction, tile);
		const auto *support = GetTBMSupportTile(tile);
		if (IsTileType(tile, TileType::Clear)) {
			++new_tiles; // A fresh cell takes new temporary track.
		} else if (support != nullptr && support->project == project.id && IsPlainRailTile(tile) && !HasSignals(tile) && GetTileOwner(tile) == project.company && GetRailType(tile) == plan.railtype) {
			// A route that passes over its own support again reuses the piece, or adds a junction piece to it.
			if (GetTrackBits(tile).Test(track)) continue;
		} else {
			return refuse(TBMBlockedReason::Obstruction, tile); // Other infrastructure is never merged or replaced.
		}
		plan.pieces.push_back({tile, track});
	}
	if (_tbm_support.size() + new_tiles > MAX_TBM_SUPPORT_TILES) return refuse(TBMBlockedReason::MissingSupport, cut.cutter_position);
	return plan;
}

/**
 * Lay prepared support pieces and register their metadata.
 * @param project Assigned project.
 * @param plan Successful preparation for the current cut.
 */
void PlaceTBMSupport(const TBMProject &project, const TBMSupportPlan &plan)
{
	assert(plan.error == TBMBlockedReason::None);
	Company *company = Company::GetIfValid(project.company);
	// The same side effects as building a single piece of track, without the command's cost or tool checks.
	for (const auto &piece : plan.pieces) {
		/**
		 * Infrastructure pieces of a track layout, counted like ordinary track (overlapping pieces count squared).
		 * @param bits Track layout.
		 * @return Counted pieces.
		 */
		const auto count = [](TrackBits bits) {
			uint pieces = bits.Count();
			return TracksOverlap(bits) ? pieces * pieces : pieces;
		};
		if (IsTileType(piece.tile, TileType::Clear)) {
			MakeRailNormal(piece.tile, project.company, TrackBits{piece.track}, plan.railtype);
			if (company != nullptr) company->infrastructure.rail[plan.railtype] += count(TrackBits{piece.track});
			TBMSupportTile entry;
			entry.tile = piece.tile;
			entry.project = project.id;
			entry.owner = project.company;
			entry.track = TrackBits{piece.track};
			entry.allowed = TBM_SUPPORT_ALLOWED;
			entry.max_speed = TBM_SUPPORT_MAX_SPEED;
			entry.conversion_price = RailBuildCost(plan.railtype);
			_tbm_support.insert(FindTBMSupport(piece.tile), entry);
		} else {
			// The project's own support takes a junction piece; a piece laid earlier in this plan is not laid twice.
			const TrackBits before = GetTrackBits(piece.tile);
			if (before.Test(piece.track)) continue;
			const TrackBits after = before | TrackBits{piece.track};
			SetTrackBits(piece.tile, after);
			if (company != nullptr) company->infrastructure.rail[plan.railtype] += count(after) - count(before);
			auto it = FindTBMSupport(piece.tile);
			assert(it != _tbm_support.end() && it->tile == piece.tile);
			it->track.Set(piece.track);
			it->conversion_price += RailBuildCost(plan.railtype);
		}
		MarkTileDirtyByTile(piece.tile);
		AddTrackToSignalBuffer(piece.tile, piece.track, project.company);
		YapfNotifyTrackLayoutChange(piece.tile, piece.track);
	}
	if (company != nullptr) DirtyCompanyInfrastructureWindows(project.company);
	UpdateSignalsInBuffer(); // Outside a command nothing else flushes the signal buffer.
}

/**
 * @param project Assigned project.
 * @param tile Retreat cell.
 * @return Whether the cell carries rail of the project's company or is its rail housing.
 */
bool IsTBMSupportUsable(const TBMProject &project, TileIndex tile)
{
	if (!IsValidTile(tile)) return false;
	// The pair housing is the mouth of the support path; elsewhere the company's own plain track carries the consist.
	if (IsTileType(tile, TileType::LayerPortal)) return GetLayerPortalTransportType(tile) == TransportType::Rail && GetTileOwner(tile) == project.company;
	return IsPlainRailTile(tile) && GetTileOwner(tile) == project.company && GetTrackBits(tile).Any();
}

/**
 * @param tile Any tile.
 * @param vehicle_class Class of the moving vehicle.
 * @return Whether the class may run on the tile.
 */
bool IsTBMSupportTraversable(TileIndex tile, TBMSupportClass vehicle_class)
{
	const auto *support = GetTBMSupportTile(tile);
	return support == nullptr || support->allowed.Test(vehicle_class);
}

/**
 * @param tile Any tile.
 * @return Whether the laying project still exists and is unfinished.
 */
bool IsTBMSupportInUse(TileIndex tile)
{
	const auto *support = GetTBMSupportTile(tile);
	if (support == nullptr) return false;
	const auto *project = GetTBMProject(support->project);
	return project != nullptr && project->phase != TBMPhase::Completed && project->phase != TBMPhase::Cancelled;
}

/**
 * @param tile Tile an ordinary rail tool changes.
 * @param flags Command flags.
 * @return Success, or the in-use refusal.
 */
CommandCost CheckTBMSupportChange(TileIndex tile, DoCommandFlags flags)
{
	// Company removal clears everything the company owns; the metadata leaves with the track.
	if (flags.Test(DoCommandFlag::Bankrupt)) return CommandCost();
	if (IsTBMSupportInUse(tile)) return CommandCost(STR_ERROR_TBM_SUPPORT_IN_USE);
	return CommandCost();
}

/**
 * @param tile Tile whose piece was removed.
 * @param track Removed piece.
 */
void ReleaseTBMSupportPiece(TileIndex tile, Track track)
{
	auto it = FindTBMSupport(tile);
	if (it == _tbm_support.end() || it->tile != tile) return;
	it->track.Reset(track);
	if (it->track.None()) _tbm_support.erase(it);
}

/**
 * @param tile Tile whose owner changes.
 * @param new_owner New owner.
 */
void ChangeTBMSupportOwner(TileIndex tile, Owner new_owner)
{
	auto it = FindTBMSupport(tile);
	if (it != _tbm_support.end() && it->tile == tile) it->owner = new_owner;
}

/**
 * @param part Any train part.
 * @return Its traffic class on support, or nothing for trains without one.
 */
std::optional<TBMSupportClass> GetTBMSupportClass(const Train &part)
{
	const Train *front = part.First();
	const TBMProject *project = front == nullptr ? nullptr : GetTBMProjectForCutter(front->index);
	if (project == nullptr) return std::nullopt; // Plan 5 haulers will declare the Hauler class here.
	// The machinery runs as the cutter; the dedicated wagons it pulls keep their own class for later haulage.
	switch (GetTBMConsistRole(*project, part)) {
		case TBMConsistRole::Cutter:
		case TBMConsistRole::PermanentSupport:
			return TBMSupportClass::Cutter;
		case TBMConsistRole::EarthWagon:
		case TBMConsistRole::RockWagon:
			return TBMSupportClass::SpoilWagon;
		default:
			return std::nullopt;
	}
}

/**
 * @param part Moving train part.
 * @param tile Tile it enters.
 * @return Whether the part may run there.
 */
bool MayTBMSupportCarry(const Train &part, TileIndex tile)
{
	const auto *support = GetTBMSupportTile(tile);
	if (support == nullptr) return true;
	const auto vehicle_class = GetTBMSupportClass(part);
	return part.owner == support->owner && vehicle_class.has_value() && support->allowed.Test(*vehicle_class);
}

/**
 * @param project Assigned project.
 * @return Its retreat path and laid support tiles.
 */
std::vector<TileIndex> GetTBMWorkCorridor(const TBMProject &project)
{
	std::vector<TileIndex> corridor;
	const auto *job = project.job == ExcavationJobID{UINT32_MAX} ? nullptr : GetExcavationJob(project.job);
	if (job != nullptr) corridor = GetExcavationRetreatPath(*job, job->cursor);
	// Every tile this project laid belongs to it, including support behind a later turn of the route.
	for (const auto &support : _tbm_support) {
		if (support.project == project.id) corridor.push_back(support.tile);
	}
	std::ranges::sort(corridor);
	corridor.erase(std::unique(corridor.begin(), corridor.end()), corridor.end());
	return corridor;
}

/**
 * @param project Assigned project.
 * @return Whether any train part stands on a corridor tile.
 */
bool IsTBMCorridorOccupied(const TBMProject &project)
{
	// Any train counts, the project's own consist included: release waits until everything has withdrawn.
	for (TileIndex tile : GetTBMWorkCorridor(project)) {
		if (HasVehicleOnTile(tile, [](const Vehicle *v) { return v->type == VehicleType::Train; })) return true;
	}
	return false;
}

/**
 * Convert released support on one tile into permanent track.
 * @param flags Command estimate/execute policy.
 * @param tile Support tile.
 * @return The conversion price, or a refusal.
 */
CommandCost CmdConvertTBMSupport(DoCommandFlags flags, TileIndex tile)
{
	if (!IsValidTile(tile)) return CMD_ERROR;
	const auto *support = GetTBMSupportTile(tile);
	if (support == nullptr) return CommandCost(STR_ERROR_TBM_SUPPORT_NONE);
	if (CommandCost owner = CheckTileOwnership(tile); owner.Failed()) return owner;
	if (IsTBMSupportInUse(tile)) return CommandCost(STR_ERROR_TBM_SUPPORT_IN_USE);
	CommandCost cost(ExpensesType::Construction, support->conversion_price);
	if (flags.Test(DoCommandFlag::Execute)) {
		_tbm_support.erase(FindTBMSupport(tile));
		MarkTileDirtyByTile(tile);
	}
	return cost;
}

/** Clear the registry and its load staging. */
void ResetTBMSupportState()
{
	_tbm_support.clear();
	_pending_tbm_support.reset();
}

/**
 * @param rows Strictly increasing loaded tiles.
 */
void StageLoadedTBMSupport(std::vector<TBMSupportTile> rows)
{
	_pending_tbm_support = std::move(rows);
}

/**
 * Validate staged support against the loaded map and publish it.
 * @param next_project Project allocation cursor; every row was laid by an earlier project.
 * @return False when a row refuses; the live registry is then left empty.
 */
bool AcceptLoadedTBMSupport(TBMProjectID next_project)
{
	_tbm_support.clear();
	if (!_pending_tbm_support.has_value()) return true;
	auto rows = std::move(*_pending_tbm_support);
	_pending_tbm_support.reset();
	if (rows.size() > MAX_TBM_SUPPORT_TILES) return false;
	TileIndex previous = INVALID_TILE;
	// Each row must still describe real track of its owner on a rock-layer cell, with known metadata.
	for (const auto &row : rows) {
		if (!IsValidTile(row.tile) || (previous != INVALID_TILE && row.tile <= previous)) return false;
		previous = row.tile;
		const auto layer = GetTileLayer(row.tile);
		if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return false;
		if (row.project.base() >= next_project.base() || row.owner.base() >= MAX_COMPANIES) return false;
		// The laid pieces must still be on the tile; another owner or a missing piece means the save is inconsistent.
		if (!IsPlainRailTile(row.tile) || GetTileOwner(row.tile) != row.owner || !row.track.Any() || !row.track.IsValid() || (GetTrackBits(row.tile) & row.track) != row.track) return false;
		if (!row.allowed.Any() || !row.allowed.IsValid() || row.max_speed == 0 || row.conversion_price < 0) return false;
	}
	_tbm_support = std::move(rows);
	return true;
}
