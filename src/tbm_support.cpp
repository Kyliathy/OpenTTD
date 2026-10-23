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
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group9 (paid terminal exit handover)
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group20B (derived whole-body face clearance)
 */

#include "stdafx.h"
#include "tbm.h"
#include "tbm_hangar.h"
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
#include "pathfinder/follow_track.hpp"
#include <set>
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
std::vector<TileIndex> GetTBMWorkRailPath(const ExcavationJob &job, size_t positions, std::vector<size_t> &ends)
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
	const auto path = GetTBMWorkRailPath(job, job.route.size(), ends);
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

/**
 * Admit only actual owned, compatible native rail as a work-travel destination.
 * @param front Actual cutter whose cached native rail compatibility governs travel.
 * @param project Existing work assignment and owner.
 * @param tile Proposed physical destination.
 * @param allow_depot Whether a validated exchange bay may be a real rail depot.
 * @return Whether the target can still carry this machine; no track, reservations or orders are changed.
 */
static bool IsTBMTravelTargetUsable(const Train &front, const TBMProject &project, TileIndex tile, bool allow_depot)
{
	if (!IsValidTile(tile)) return false;
	// A stolen/replaced facility or incompatible technology must not become a
	// guessed no-order destination merely because an old tile index still exists.
	const bool rail = IsPlainRailTile(tile) || (allow_depot && IsRailDepotTile(tile)) ||
		(IsLayerPortalTile(tile) && GetLayerPortalTransportType(tile) == TransportType::Rail);
	if (!rail || GetTileOwner(tile) != project.company || !front.compatible_railtypes.Test(GetRailType(tile))) return false;
	return !IsPlainRailTile(tile) || GetTrackBits(tile).Any();
}

/** @copydoc GetTBMCutterSteeringTarget */
std::optional<TBMCutterSteeringTarget> GetTBMCutterSteeringTarget(const Train &front)
{
	const auto *project = GetTBMProjectForCutter(front.First()->index);
	// Ordinary trains and actual service visits follow player/depot orders; only project travel supplies a derived destination.
	if (project == nullptr) return std::nullopt;
	if (project->phase == TBMPhase::FollowOrders) return std::nullopt; // Native orders supply all travel targets while the geological job is suspended.
	if (project->phase == TBMPhase::TravelToHangar) {
		const auto destination = GetTBMOrderedHangar(front);
		if (destination.error != TBMHangarError::None) return std::nullopt;
		const TrackdirBits directions = destination.kind == TBMHangarKind::PortalMagazine ?
			TrackBitsToTrackdirBits(DiagDirToDiagTrack(GetLayerPortalShaftDirection(destination.tile))) :
			TrackdirToTrackdirBits(DiagDirToDiagTrackdir(ReverseDiagDir(GetRailDepotDirection(destination.tile))));
		return TBMCutterSteeringTarget{destination.tile, directions}; // Destination remains authoritative in the actual native depot order.
	}
	if (front.current_order.IsType(OT_GOTO_DEPOT) && !IsTBMStagingHangarOrder(front)) return std::nullopt;
	if (FindTBMProjectReferenceLoss(*project) != TBMBlockedReason::None) return std::nullopt;
	if (IsTBMReturning(project->phase)) {
		const auto bay = GetTBMExchangeBay(*project);
		if (bay.error != TBMExchangeError::None) return std::nullopt;
		if (!IsTBMTravelTargetUsable(front, *project, bay.tile, true)) return std::nullopt;
		const DiagDirection door = bay.kind == TBMExchangeBayKind::Source ? GetLayerPortalDoorDirection(bay.tile) : GetRailDepotDirection(bay.tile);
		return TBMCutterSteeringTarget{bay.tile, TrackdirToTrackdirBits(DiagDirToDiagTrackdir(ReverseDiagDir(door)))};
	}
	// Work/wait phases hold through their existing train controller; only approach and advancing onto paid support need pathfinding.
	if (project->phase != TBMPhase::TravelToFace && project->phase != TBMPhase::AdvanceSupport) return std::nullopt;
	const auto *job = GetExcavationJob(project->job);
	// Missing or malformed live geometry cannot authorize another route or a fabricated waypoint.
	if (job == nullptr || job->cursor > job->route.size()) return std::nullopt;
	const TileIndex target = job->cursor == 0 ? job->commissioning_anchor : job->route[job->cursor - 1].cutter_position;
	if (!IsTBMTravelTargetUsable(front, *project, target, false)) return std::nullopt;
	if (job->cursor == 0) {
		if (!IsLayerPortalTile(target)) return std::nullopt; // The first face still needs its actual housing, not a replacement plain tile.
		return TBMCutterSteeringTarget{target, TrackdirToTrackdirBits(DiagDirToDiagTrackdir(GetLayerPortalDoorDirection(target)))};
	}
	std::vector<size_t> ends;
	const auto path = GetTBMWorkRailPath(*job, job->cursor + 1, ends);
	if (ends.size() < job->cursor) return std::nullopt;
	const size_t index = ends[job->cursor - 1];
	if (index == 0) return std::nullopt;
	const DiagDirection entry = DiagdirBetweenTiles(path[index], path[index - 1]);
	const DiagDirection exit = index + 1 < path.size() ? DiagdirBetweenTiles(path[index], path[index + 1]) : ReverseDiagDir(entry);
	const TrackdirBits actual = GetTileTrackStatus(target, TransportType::Rail, RoadTramType::Invalid).trackdirs;
	// A reused chamber has other paid junction branches. Arriving on any branch of the same tile can send the
	// physical head away from its next planned advance; accept only the actual laid piece joining this route's entry and exit.
	for (Trackdir trackdir : DiagdirReachesTrackdirs(ReverseDiagDir(entry))) {
		if (TrackdirToExitdir(trackdir) == exit && actual.Test(trackdir)) return TBMCutterSteeringTarget{target, TrackdirToTrackdirBits(trackdir)};
	}
	return std::nullopt;
}

/**
 * Trackdir of the piece laid on one cell of a work path, in the path's forward direction (r2df2 task 2.2). The piece
 * joins the edge towards the previous cell to the edge towards the next one; at the path's end it runs straight on.
 * Only a piece actually laid on the tile counts: a reused chamber has other junction branches leading elsewhere.
 * @param path Work path from the housing onward.
 * @param index Index of the cell in the path; the housing (0) has no such piece.
 * @return The forward trackdir of the laid piece, or Trackdir::Invalid when the tile does not carry it.
 */
static Trackdir GetTBMPathPieceTrackdir(const std::vector<TileIndex> &path, size_t index)
{
	// Housing and out-of-range positions have no ordinary paid path piece
	// that could authorize a stand or its forward arrival direction.
	if (index == 0 || index >= path.size()) return Trackdir::Invalid;
	const TileIndex tile = path[index];
	const DiagDirection entry = DiagdirBetweenTiles(tile, path[index - 1]);
	const DiagDirection exit = index + 1 < path.size() ? DiagdirBetweenTiles(tile, path[index + 1]) : ReverseDiagDir(entry);
	// Broken neighbour geometry cannot supply a physical stand direction.
	if (entry == DiagDirection::Invalid || exit == DiagDirection::Invalid) return Trackdir::Invalid;
	const TrackdirBits actual = GetTileTrackStatus(tile, TransportType::Rail, RoadTramType::Invalid).trackdirs;
	// Of the trackdirs a train entering from the previous cell can take, the laid one that leaves towards the next cell.
	for (Trackdir trackdir : DiagdirReachesTrackdirs(ReverseDiagDir(entry))) {
		if (TrackdirToExitdir(trackdir) == exit && actual.Test(trackdir)) return trackdir;
	}
	return Trackdir::Invalid;
}

/** Where a cutter consist stands on the work path behind its face (r2df2 Group 2). */
struct TBMConsistOnPath {
	std::vector<TileIndex> path; ///< Work path from the housing through the last completed cutter position.
	size_t front = 0; ///< Path index of the front engine's tile.
	size_t tail = 0; ///< Smallest path index of any part's tile: the rearmost part.
	size_t before = 0; ///< Path index of the approach before the last completed cut; 0, the housing, for the first cut.
};

/**
 * Place every part of a cutter consist on the tail of its work path.
 * @param front Live cutter front engine.
 * @param job Its live job.
 * @return The placement, or nothing before the first cut, for a broken path, or when a part stands off the last path
 *         cells (in the housing's shaft, on another layer, or further back on its way).
 */
static std::optional<TBMConsistOnPath> LocateTBMConsistOnPath(const Train &front, const ExcavationJob &job)
{
	// No completed cut or an invalid cursor has no paid face to locate.
	if (job.cursor == 0 || job.cursor > job.route.size()) return std::nullopt;
	TBMConsistOnPath located;
	std::vector<size_t> ends;
	located.path = GetTBMWorkRailPath(job, job.cursor, ends);
	if (ends.size() != job.cursor) return std::nullopt;
	located.before = job.cursor >= 2 ? ends[job.cursor - 2] : 0;
	size_t parts = 0;
	for (const Train *part = &front; part != nullptr; part = part->Next()) ++parts;
	// N parts cover at most N cells, and the front may still be one diagonal cut (two cells) short of the end, so only
	// the last N + 2 cells are searched: no scan of the whole route on every tick (I-9).
	const auto window = located.path.end() - std::min(located.path.size(), parts + 2);
	located.tail = located.path.size();
	// Every part must stand on that tail; one still in the shaft or travelling elsewhere means the consist is not there.
	for (const Train *part = &front; part != nullptr; part = part->Next()) {
		const auto it = std::find(window, located.path.end(), part->tile);
		if (it == located.path.end()) return std::nullopt;
		const size_t index = static_cast<size_t>(it - located.path.begin());
		if (part == &front) located.front = index;
		located.tail = std::min(located.tail, index);
	}
	return located;
}

/**
 * Preserve one physically clear tile around the complete running cutter.
 * @param cutter Original live machinery and its actual trailing parts.
 * @param tile Candidate hauler support cell on an explicit map layer.
 * @return Whether the whole cell stays outside the cutter and its safety gap;
 *         a chamber shortcut or folded work path cannot reduce that gap.
 */
static bool IsTBMHaulerCellClear(const Train &cutter, TileIndex tile)
{
	// Every bounded actual part protects its own cell and immediate neighbours.
	// Path indices alone do not measure physical clearance in a folded chamber.
	for (const Train *part = &cutter; part != nullptr; part = part->Next()) {
		if (part->track == Track::Depot) continue;
		// Only machinery on the same actual plane can obstruct this cell;
		// nearby coordinates across a shaft never imply physical occupancy.
		if (GetTileLayer(part->tile) == GetTileLayer(tile) &&
				std::abs(static_cast<int>(TileX(part->tile)) - static_cast<int>(TileX(tile))) <= 1 &&
				std::abs(static_cast<int>(TilePlaneY(part->tile)) - static_cast<int>(TilePlaneY(tile))) <= 1) return false;
	}
	return true;
}

/**
 * @param support Support record on a tile.
 * @return Whether the project that laid it no longer works: it was retired, or it finished and awaits release.
 */
static bool IsTBMSupportReleased(const TBMSupportTile &support)
{
	const TBMProject *layer = GetTBMProject(support.project);
	return layer == nullptr || IsTBMProjectFinished(layer->phase);
}

/**
 * Find an actual directed approach around a folded cutter, without selecting a hauler or changing its destination.
 * @tparam Follower Native railway follower with the current90-degree-turn policy.
 * @param project Original live project whose paid corridor bounds the search.
 * @param cutter Actual body whose clearance must remain protected.
 * @param path Canonical paid work history; its first running cell supplies the real shaft approach.
 * @param stand Actual clear stand and its direction towards the cutter.
 * @return Whether native directed rail pieces reach the stand without crossing its body clearance.
 */
template <typename Follower>
static bool HasTBMFaceApproach(const TBMProject &project, const Train &cutter, const std::vector<TileIndex> &path, const TBMFaceStand &stand)
{
	const auto corridor = GetTBMWorkCorridor(project);
	/** @param tile Native candidate cell. @return Whether it is paid owned rail of the allowed class with the cutter's physical clearance. */
	const auto allowed = [&](TileIndex tile) {
		// Only the original paid corridor can supply an approach. Public rails
		// elsewhere and another live project's support cannot make a face viable.
		if (!IsPlainRailTile(tile) || GetTileOwner(tile) != project.company || !std::ranges::binary_search(corridor, tile) ||
				!IsTBMHaulerCellClear(cutter, tile)) return false;
		const auto *support = GetTBMSupportTile(tile);
		return support == nullptr || (support->owner == project.company && support->allowed.Test(TBMSupportClass::Hauler) &&
				(support->project == project.id || IsTBMSupportReleased(*support)));
	};
	const auto start = path[1];
	if (!allowed(start)) return false;
	const auto outward = DiagdirBetweenTiles(path.front(), start);
	if (outward == DiagDirection::Invalid) return false;
	const auto starts = GetTileTrackStatus(start, TransportType::Rail, RoadTramType::Invalid).trackdirs & DiagdirReachesTrackdirs(outward);
	const auto goals = GetTBMFaceStandTrackdirs(stand);
	std::vector<std::pair<TileIndex, Trackdir>> pending;
	for (Trackdir direction : starts) pending.emplace_back(start, direction);
	std::set<std::pair<TileIndex, Trackdir>> visited;
	// Search only actual paid directed rail states, at most12 per corridor
	// cell. A revisited canonical loop need not be followed through the body
	// when its existing native junction offers another genuinely clear branch.
	for (size_t index = 0; index < pending.size(); ++index) {
		const auto [tile, direction] = pending[index];
		if (!visited.emplace(tile, direction).second) continue;
		if (tile == stand.tile && goals.Test(direction)) return true; // The exact arrival towards the cutter matters; reaching another piece of the same junction cannot stop at its stand.
		// Start after the closed housing and use the owner's native follower:
		// this geometry query precedes turn-holder selection and must not ask
		// a portal admission rule that itself depends on the meeting answer.
		Follower follower(project.company, cutter.compatible_railtypes);
		if (!follower.Follow(tile, direction)) continue;
		if (!allowed(follower.new_tile)) continue;
		for (Trackdir next : follower.new_td_bits) pending.emplace_back(follower.new_tile, next);
	}
	return false;
}


/**
 * @param cutter Actual cutter whose whole physical body protects the stand's clearance.
 * @param located Consist placement on its work path.
 * @param job Its live job.
 * @return The stand two cells behind the rearmost part, or nothing (see FindTBMFaceStand).
 */
static std::optional<TBMFaceStand> GetTBMFaceStandBehind(const Train &cutter, const TBMConsistOnPath &located, const ExcavationJob &job)
{
	// path[0] is the housing: the stand needs a cell after it, and one clear cell before the rearmost part.
	if (located.tail < 3) return std::nullopt;
	const size_t stand = located.tail - 2;
	const TileIndex tile = located.path[stand];
	// A path-index gap in a turning chamber may still touch another part of
	// the cutter. A face stand must retain its actual physical safety cell.
	if (!IsPlainRailTile(tile) || !IsTBMHaulerCellClear(cutter, tile)) return std::nullopt;
	const Trackdir trackdir = GetTBMPathPieceTrackdir(located.path, stand);
	if (trackdir == Trackdir::Invalid) return std::nullopt;
	// A safe endpoint can still be isolated behind a folded cutter body.
	// A clear canonical prefix is the cheap usual case. If an earlier loop
	// crosses the body now, native paid junctions may still offer another clear
	// directed approach; only an actually unreachable stand falls back to Bay.
	bool clear = true;
	for (size_t index = 1; index <= stand; ++index) {
		if (!IsTBMHaulerCellClear(cutter, located.path[index])) { clear = false; break; }
	}
	if (!clear) {
		const auto *project = GetTBMProjectForCutter(cutter.index);
		if (project == nullptr) return std::nullopt;
		const TBMFaceStand candidate{tile, trackdir};
		const bool reachable = _settings_game.pf.forbid_90_deg ? HasTBMFaceApproach<CFollowTrackRailNo90>(*project, cutter, located.path, candidate) :
			HasTBMFaceApproach<CFollowTrackRail>(*project, cutter, located.path, candidate);
		if (!reachable) return std::nullopt;
	}
	// The next cut's sweep must stay free of other trains, or that cut would wait for an obstruction.
	if (job.cursor < job.route.size()) {
		const auto &swept = job.route[job.cursor].swept_tiles;
		if (std::ranges::find(swept, tile) != swept.end()) return std::nullopt;
	}
	return TBMFaceStand{tile, trackdir};
}

/** @copydoc GetTBMFaceStandTrackdirs */
TrackdirBits GetTBMFaceStandTrackdirs(const TBMFaceStand &stand)
{
	TrackdirBits result{};
	const DiagDirection exit = TrackdirToExitdir(stand.trackdir);
	// Every laid piece that leaves the stand tile towards the clear cell and the cutter counts, whichever way it came in.
	for (Trackdir trackdir : GetTileTrackStatus(stand.tile, TransportType::Rail, RoadTramType::Invalid).trackdirs) {
		if (TrackdirToExitdir(trackdir) == exit) result.Set(trackdir);
	}
	return result;
}

/** @copydoc FindTBMFaceStand */
std::optional<TBMFaceStand> FindTBMFaceStand(const TBMProject &project)
{
	const Train *front = GetLiveTBMCutter(project);
	const ExcavationJob *job = GetLiveTBMJob(project);
	// Both independent original references must survive before any physical
	// pose can provide a face stand; neither reference repairs the other.
	if (front == nullptr || job == nullptr) return std::nullopt;
	const auto located = LocateTBMConsistOnPath(*front, *job);
	if (!located.has_value()) return std::nullopt;
	return GetTBMFaceStandBehind(*front, *located, *job);
}

/** @copydoc GetTBMMeetingKind */
TBMMeetingKind GetTBMMeetingKind(const TBMProject &project)
{
	const bool working = project.phase == TBMPhase::Bore || project.phase == TBMPhase::AdvanceSupport || IsTBMAwaitingFaceTransfer(project.phase);
	// Only active face work or a physical return may select a new face meeting.
	if (!working && !IsTBMReturning(project.phase)) return TBMMeetingKind::Bay;
	// Reuse the original full reference owner: an orphan origin, broken portal,
	// wrong consist or invalid bay must not invite a hauler via a safe-looking pose.
	if (FindTBMProjectReferenceLoss(project) != TBMBlockedReason::None) return TBMMeetingKind::Bay;
	// After its actual spoil has left, a returning cutter needs no new face
	// visit. Calling each empty hauler back inside would renew the F7 hold and
	// prevent the completed machine from ever withdrawing to its real bay.
	if (!working && !IsTBMExchangeDue(project)) return TBMMeetingKind::Bay;
	// r2mlvs3mp task 10R.2: only a Source bay serves the face. A hauler waiting in an underground depot bay leaves it by
	// the native depot rule, which keeps it inside while another train occupies the depot's signal block; the waiting
	// cutter stands in that same unsignalled chamber block, so each would wait for the other. Such a project meets its
	// hauler at that nearby depot, as it did before r2df2.
	if (const auto bay = GetTBMExchangeBay(project); bay.error == TBMExchangeError::None && bay.kind != TBMExchangeBayKind::Source) return TBMMeetingKind::Bay;
	const Train *front = GetLiveTBMCutter(project);
	const ExcavationJob *job = GetLiveTBMJob(project);
	if (front == nullptr || job == nullptr) return TBMMeetingKind::Bay;
	const auto located = LocateTBMConsistOnPath(*front, *job);
	if (!located.has_value()) return TBMMeetingKind::Bay;
	// A working cutter may still be rolling onto the support of the cut it just made, anywhere from the approach of the
	// cut before; a cutter sent back counts only while its front is still on the face, so no hauler follows it in. A
	// return requested during the face wait, which usually stands one cut short of the face, still meets the hauler
	// already in the corridor there: rule F7 holds it still until that hauler has left (r2df2 Group 7). No new hauler is
	// invited, because the meeting point is the bay until one is inside.
	const size_t face = located->path.size() - 1;
	const bool held_for_hauler = !working && located->front >= located->before && FindTBMHaulerInCorridor(project) != VehicleID::Invalid();
	const bool at_face = working ? located->front >= located->before : (located->front == face || held_for_hauler);
	if (!at_face) return TBMMeetingKind::Bay;
	// Every assigned hauler must fit wholly between the housing and the stand, with a cell to spare for corner pieces, so
	// none stops with a part still in the pair's magazine, where it could not turn round (r2df2 Group 4): with its front
	// on the stand (tail - 2), a hauler of L cells ends L - 1 cells further back, which must not be beyond the housing.
	if (located->tail < GetTBMHaulerRoomCells(project.id) + 2) return TBMMeetingKind::Bay;
	return GetTBMFaceStandBehind(*front, *located, *job).has_value() ? TBMMeetingKind::Face : TBMMeetingKind::Bay;
}

/** @copydoc IsTBMReadyForFaceTransfer */
bool IsTBMReadyForFaceTransfer(const TBMProject &project)
{
	// Actual cutting/travel/storage cannot start the transfer transaction;
	// only the explicit wait or a loaded return can offer physical spoil.
	if (!IsTBMAwaitingFaceTransfer(project.phase) && !IsTBMReturning(project.phase)) return false;
	return GetTBMMeetingKind(project) == TBMMeetingKind::Face && IsTBMExchangeDue(project);
}

/** @copydoc GetTBMTravelHoldReason */
TBMBlockedReason GetTBMTravelHoldReason(const Train &front)
{
	const auto *project = GetTBMProjectForCutter(front.First()->index);
	// Finished machinery and ordinary trains follow normal orders. A real
	// depot order owns travel while service is in progress, even at a work face.
	if (project == nullptr || IsTBMProjectFinished(project->phase)) return TBMBlockedReason::None;
	if (front.current_order.IsType(OT_GOTO_DEPOT) && !IsTBMStagingHangarOrder(front) && project->phase != TBMPhase::TravelToHangar) return TBMBlockedReason::None; // Existing ordinary service visits retain their precedence over a paused geological job.
	if (project->blocked == TBMBlockedReason::Paused) return TBMBlockedReason::Paused;
	if (project->phase == TBMPhase::FollowOrders) {
		return front.current_order.IsGotoOrder() || front.current_order.IsType(OT_LOADING) || front.current_order.IsType(OT_LEAVESTATION) ?
			TBMBlockedReason::None : TBMBlockedReason::MissingOrders;
	}
	if (project->phase == TBMPhase::TravelToHangar) {
		const auto destination = GetTBMOrderedHangar(front);
		if (destination.error == TBMHangarError::Occupied) return TBMBlockedReason::OccupiedHangar;
		return destination.error == TBMHangarError::None ? TBMBlockedReason::None : TBMBlockedReason::MissingHangar;
	}
	const auto loss = FindTBMProjectReferenceLoss(*project);
	if (loss != TBMBlockedReason::None) return loss;
	// Rule F7 (r2df2 task 4.5): a cutter sent back never moves while a hauler of its project is in its corridor.
	if (IsTBMReturning(project->phase) && FindTBMHaulerInCorridor(*project) != VehicleID::Invalid()) return TBMBlockedReason::Obstruction;
	// Plan18 8FA: once the face transfer is complete, keep the cutter's whole
	// body still until the departing hauler clears its corridor. In a folded
	// chamber, advancing while the load exits can close its only physical path
	// back to Surface even though the stand itself was clear at the transfer.
	if (const auto *assignment = GetTBMExchangeAssignmentForHauler(FindTBMHaulerInCorridor(*project)); assignment != nullptr) {
		if (assignment->phase == TBMHaulerPhase::CollectLoaded || assignment->phase == TBMHaulerPhase::VisitOrigin ||
				assignment->phase == TBMHaulerPhase::DisposeOrTransfer) return TBMBlockedReason::Obstruction;
	}
	// Work and magazine waits already have physical holds. Only real project
	// travel needs a destination; an absent one may never invoke ordinary wandering.
	if (project->phase != TBMPhase::TravelToFace && project->phase != TBMPhase::AdvanceSupport && !IsTBMReturning(project->phase)) return TBMBlockedReason::None;
	return GetTBMCutterSteeringTarget(front) ? TBMBlockedReason::None : TBMBlockedReason::MissingSupport;
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
	const auto path = GetTBMWorkRailPath(job, job.cursor + 2, ends);
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
		// A further project may traverse already paid permanent rail. Reuse only the exact owned compatible
		// piece on open floor; the cut transaction neither adds free ordinary track nor creates temporary metadata for it.
		if (support == nullptr && IsPlainRailTile(tile) && !HasSignals(tile) && GetTileOwner(tile) == project.company &&
			GetRailType(tile) == plan.railtype && geology.terrain.Get(tile) == ExcavationState::Excavated && GetTrackBits(tile).Test(track)) continue;
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
 * @return Whether a real train, geological claim or unfinished project's retreat still requires this support.
 */
bool IsTBMSupportInUse(TileIndex tile)
{
	const auto *support = GetTBMSupportTile(tile);
	if (support == nullptr) return false;
	if (HasVehicleOnTile(tile, [](const Vehicle *vehicle) { return vehicle->type == VehicleType::Train; }) || GetGeologyWorldState().job_claims.contains(tile)) return true; // Neither a hidden tail nor another job's claimed floor becomes convertible when the laying project ends.
	const auto *project = GetTBMProject(support->project);
	if (project != nullptr && !IsTBMProjectFinished(project->phase)) return true;
	// Reused old support can carry a later job's return route. Query that
	// existing route owner only after the common original-owner guard clears.
	for (const auto &other : CaptureTBMProjects()) {
		if (IsTBMProjectFinished(other.phase)) continue;
		if (std::ranges::binary_search(GetTBMWorkCorridor(other), tile)) return true;
	}
	return false;
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

/** @copydoc CheckTBMExitHandover */
CommandCost CheckTBMExitHandover(TileIndex tile, DiagDirection shaft, RailType railtype)
{
	const auto *support = GetTBMSupportTile(tile);
	if (support == nullptr || !IsPlainRailTile(tile)) return CommandCost(STR_ERROR_TBM_SUPPORT_NONE);
	if (support->owner != _current_company || GetTileOwner(tile) != _current_company) return CommandCost(STR_ERROR_AREA_IS_OWNED_BY_ANOTHER);
	const auto *project = GetTBMProject(support->project);
	const auto *job = project == nullptr ? nullptr : GetExcavationJob(project->job);
	if (job == nullptr || project->company != _current_company || job->state != ExcavationJobState::Completed || job->cursor != job->route.size()) return CommandCost(STR_ERROR_TBM_EXIT_UNFINISHED);
	// A running hauler must leave before the terminal support changes into an exit.
	if (FindTBMHaulerInCorridor(*project) != VehicleID::Invalid()) return CommandCost(STR_ERROR_TBM_EXIT_HAULER);
	std::vector<size_t> ends;
	const auto path = GetTBMWorkRailPath(*job, job->route.size(), ends);
	const auto door_step = TileIndexDiffCByDiagDir(ReverseDiagDir(shaft));
	const TileIndex door = TileAddWrap(tile, door_step.x, door_step.y);
	// Replacing the terminal piece cannot sever the cutter from A. A branch,
	// a reused middle tile or the wrong mouth direction is a different design
	// and must not inherit this narrowly bounded handover.
	if (path.size() < 4 || path.back() != tile || path[path.size() - 2] != door || std::ranges::count(path, tile) != 1 ||
			GetRailType(tile) != railtype || HasSignals(tile) || GetTrackBits(tile) != TrackBits{DiagDirToDiagTrack(shaft)} || support->track != GetTrackBits(tile)) return CommandCost(STR_ERROR_TBM_EXIT_GEOMETRY);
	const auto pocket = GetExcavationCutterEnvelope(tile);
	if (pocket.empty()) return CommandCost(STR_ERROR_TBM_EXIT_GEOMETRY);
	if (const auto floor = CheckExcavationConstruction(pocket); floor.Failed()) return floor;
	if (GetRailReservationTrackBits(tile).Any() || EnsureNoVehicleOnGround(tile).Failed() || EnsureNoVehicleOnGround(door).Failed()) return CommandCost(STR_ERROR_TBM_EXIT_STAGE);
	const Train *front = Train::GetIfValid(project->cutter);
	if (front == nullptr || !front->vehstatus.Test(VehState::Stopped) || front->cur_speed != 0 ||
			InspectTBMConsist(*front, *project).error != TBMBlockedReason::None) return CommandCost(STR_ERROR_TBM_EXIT_STAGE);
	// Every real part must be locally staged on the retained approach. A
	// stopped icon or an already withdrawn head cannot conceal an outside tail.
	for (const Train *part = front; part != nullptr; part = part->Next()) {
		const auto position = std::ranges::find(path, part->tile);
		if (part->vehstatus.Test(VehState::Crashed) || position == path.end() || position >= path.end() - 2 || part->track == Track::Depot) return CommandCost(STR_ERROR_TBM_EXIT_STAGE);
	}
	for (const auto &other : CaptureTBMProjects()) {
		// Another active route may still need this paid terminal for retreat.
		if (other.id == project->id || IsTBMProjectFinished(other.phase)) continue;
		if (std::ranges::binary_search(GetTBMWorkCorridor(other), tile)) return CommandCost(STR_ERROR_TBM_SUPPORT_IN_USE);
	}
	return CommandCost(ExpensesType::Construction, support->conversion_price + RailClearCost(railtype));
}

/** @copydoc CompleteTBMExitHandover */
void CompleteTBMExitHandover(TileIndex tile)
{
	auto support = FindTBMSupport(tile);
	assert(support != _tbm_support.end() && support->tile == tile && support->track.Count() == 1);
	assert(IsLayerPortalTile(tile) && GetTileOwner(tile) == support->owner);
	// The portal coordinator has already installed and published both real
	// housings. Only the replaced single track piece leaves infrastructure;
	// all remaining temporary support and the geological job stay protected.
	Company::Get(support->owner)->infrastructure.rail[GetRailType(tile)] -= 1;
	_tbm_support.erase(support);
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
	if (project == nullptr) {
		// Every part of an assigned hauler runs as the Hauler class (r2df2 task 3.1); any other train has no class.
		if (front != nullptr && GetTBMExchangeAssignmentForHauler(front->index) != nullptr) return TBMSupportClass::Hauler;
		return std::nullopt;
	}
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
	// Company, physical class and the registered permission must all agree; assignment alone never opens private support.
	if (part.owner != support->owner || !vehicle_class.has_value() || !support->allowed.Test(*vehicle_class)) return false;
	if (*vehicle_class != TBMSupportClass::Hauler) return true;
	// A hauler serves one project: its support, or released support that route reuses, as its cutter does. Another
	// live project's corridor stays closed to it (r2df2 task 3.1).
	const auto *assignment = GetTBMExchangeAssignmentForHauler(part.First()->index);
	if (assignment == nullptr || (assignment->project != support->project && !IsTBMSupportReleased(*support))) return false;
	const auto *project = GetTBMProject(assignment->project);
	if (project == nullptr) return false;
	// Plan18 8FA: chamber junctions can offer a shorter route to the stand
	// across the cutter's occupied body. Both path discovery and native front
	// admission use this same support rule, so an assigned hauler must route
	// around actual machinery instead of treating its own project as empty rail.
	const auto *cutter = Train::GetIfValid(project->cutter);
	return cutter != nullptr && IsTBMHaulerCellClear(*cutter, tile);
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
