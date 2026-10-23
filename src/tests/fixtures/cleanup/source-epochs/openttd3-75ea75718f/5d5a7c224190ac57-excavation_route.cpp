/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_route.cpp Deterministic bounded advancing coverage with complete cutter/consist sweeps. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Groups 13 and 14B (solid start); docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Groups 8-9 (exact goals, approaches, straight exit from a housing) and 11-12 (remainder replacement planning, approaches after the completed cuts); docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#include "stdafx.h"
#include "excavation.h"
#include "tile_map.h"
#include "rail_map.h"
#include "company_func.h"
#include "company_base.h"
#include <queue>

#include "safeguards.h"

/**
 * Add a conservative bounded rectangle using explicit layer-local coordinates.
 * @param output Complete geometry being constructed.
 * @param layer Explicit physical rock layer.
 * @param min_x First column, inclusive.
 * @param min_y First row, inclusive.
 * @param max_x Last column, inclusive.
 * @param max_y Last row, inclusive.
 * @return Whether every cell is playable, without permitting plane wraparound.
 */
static bool AddPlanningRectangle(std::vector<TileIndex> &output, MapLayer layer, int min_x, int min_y, int max_x, int max_y)
{
	// A conservative envelope cannot wrap a map edge or invent a cell on a neighbouring plane.
	if (min_x < 0 || min_y < 0 || max_x >= static_cast<int>(Map::SizeX()) || max_y >= static_cast<int>(Map::SizeY())) return false;
	for (int y = min_y; y <= max_y; ++y) {
		for (int x = min_x; x <= max_x; ++x) {
			const auto tile = TileXYL(x, y, layer);
			if (!IsValidTile(tile)) return false;
			output.push_back(tile);
		}
	}
	return true;
}

/**
 * Canonicalize complete swept/support geometry so every cell charges/claims at most once.
 * @param tiles Explicit geometry owned by the transient proposal.
 */
static void CanonicalizePlanningTiles(std::vector<TileIndex> &tiles)
{
	std::ranges::sort(tiles);
	tiles.erase(std::unique(tiles.begin(), tiles.end()), tiles.end());
}

/**
 * Advance one real tile with at most one 45-degree bend and complete cutter/consist sweep.
 * @param pose Explicit head-first occupied/support history.
 * @param next_heading Actual advancing heading, never a stationary rotation.
 * @return Complete shape or empty refusal.
 */
ExcavationPlanningStep AdvanceExcavationPlanningPose(const ExcavationPlanningPose &pose, Direction next_heading)
{
	// Required spoil legs have real geometry; a missing/oversized history cannot be replaced by a point-only cutter check.
	if (!IsValidTile(pose.position) || !IsValidDirection(pose.heading) || !IsValidDirection(next_heading) || pose.history.size() < 3 || pose.history.size() > 16 || pose.history.front() != pose.position) return {ExcavationError::InvalidRoute, pose.position};
	const auto layer = GetTileLayer(pose.position);
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return {ExcavationError::MixedLayers, pose.position}; // Cutter geometry belongs only to the explicit rock domain.
	const uint difference = (to_underlying(next_heading) - to_underlying(pose.heading) + 8) % 8;
	if (difference > 1 && difference < 7) return {ExcavationError::IllegalTurn, pose.position}; // Sharp/reversing pivots cannot substitute for two real advancing bends.
	// Every supplied consist centre remains in this layer and follows adjacent occupied cells.
	for (size_t i = 0; i < pose.history.size(); ++i) {
		if (!IsValidTile(pose.history[i])) return {ExcavationError::InvalidTile, pose.history[i]};
		if (GetTileLayer(pose.history[i]) != layer) return {ExcavationError::MixedLayers, pose.history[i]};
		if (i != 0 && DistanceMax(pose.history[i], pose.history[i - 1]) > 1) return {ExcavationError::InvalidRoute, pose.history[i]}; // A detached/nonadjacent consist cannot acquire swept-space permission.
	}
	const auto offset = TileIndexDiffCByDir(next_heading);
	const auto next = TileAddWrap(pose.position, offset.x, offset.y);
	if (!IsValidTile(next)) return {ExcavationError::InvalidTile, next};
	ExcavationPlanningStep result;
	result.next.position = next; result.next.heading = next_heading;
	result.next.history = pose.history;
	// Advancing shifts the real support trail; backing is a separate retreat operation, never a new cut edge.
	for (size_t i = pose.history.size() - 1; i != 0; --i) result.next.history[i] = pose.history[i - 1];
	result.next.history[0] = next;
	result.cut.cutter_position = next; result.cut.heading = next_heading;
	const int x = TileX(pose.position), y = TilePlaneY(pose.position);
	const int nx = TileX(next), ny = TilePlaneY(next);
	// The complete old/new 3x3 bounding sweep deliberately includes both diagonal corner cells for conservative clearance.
	if (!AddPlanningRectangle(result.cut.swept_tiles, layer, std::min(x, nx) - 1, std::min(y, ny) - 1, std::max(x, nx) + 1, std::max(y, ny) + 1)) return {ExcavationError::InvalidTile, next};
	if (!AddPlanningRectangle(result.cut.support_tiles, layer, x - 1, y - 1, x + 1, y + 1)) return {ExcavationError::InvalidTile, pose.position};
	// Each trailing occupied cell has its own complete movement/corner sweep, so a legal head cannot clip wagons through an obstacle.
	for (size_t i = 1; i < pose.history.size(); ++i) {
		const int tx = TileX(pose.history[i]), ty = TilePlaneY(pose.history[i]);
		const int ux = TileX(result.next.history[i]), uy = TilePlaneY(result.next.history[i]);
		if (!AddPlanningRectangle(result.cut.swept_tiles, layer, std::min(tx, ux), std::min(ty, uy), std::max(tx, ux), std::max(ty, uy))) return {ExcavationError::InvalidTile, pose.history[i]};
		result.cut.support_tiles.push_back(pose.history[i]);
	}
	CanonicalizePlanningTiles(result.cut.swept_tiles);
	CanonicalizePlanningTiles(result.cut.support_tiles);
	if (result.cut.swept_tiles.size() > 64 || result.cut.support_tiles.size() > 64) return {ExcavationError::Limit, next}; // The public job/save footprint budget also governs planning geometry.
	return result;
}

static constexpr std::array<int, 1> STRAIGHT_ONLY = {0}; ///< Turn choices for a cutter leaving a straight portal housing.
static constexpr std::array<int, 3> STRAIGHT_LEFT_RIGHT = {0, -1, 1}; ///< Stable straight, left-45, right-45 expansion order.

/** Full bounded pose key; different trailing histories cannot share an endpoint-only collision result. */
using PlanningKey = std::tuple<TileIndex, Direction, std::array<TileIndex, 16>>;

/**
 * Build one complete history-sensitive key without serializing padding or pointers.
 * @param pose Explicit cutter/consist state.
 * @return Stable ordered search identity.
 */
static PlanningKey KeyForPlanningPose(const ExcavationPlanningPose &pose)
{
	std::array<TileIndex, 16> trail;
	trail.fill(INVALID_TILE);
	std::copy(pose.history.begin(), pose.history.end(), trail.begin());
	return {pose.position, pose.heading, trail};
}

/** One transient bounded search node; parent/support history proves a real advancing path instead of stationary rotations. */
struct ExcavationPlanningNode {
	ExcavationPlanningPose pose; ///< Actual full geometry state.
	ExcavationCutGeometry cut; ///< Complete edge from parent to this pose.
	uint64_t cost = 0; ///< Conservative monetary/time search score, never a charged account.
	uint32_t depth = 0, parent = UINT32_MAX; ///< Bounded real advance count and stable predecessor.
};

/**
 * Lower-bound remaining advances until a 3x3 cutter can cover the target.
 * @param position Explicit candidate head.
 * @param target Stable remaining requested cell.
 * @return Admissible positive transit-step lower bound, independent of RNG/CPU timing.
 */
static uint32_t PlanningDistance(TileIndex position, TileIndex target)
{
	const uint32_t distance = DistanceMax(position, target);
	return distance > 1 ? distance - 1 : 0;
}

/**
 * Keep the refused cell nearest the target as the explanation of a failed search.
 * @param blocker Current explanation; replaced when the new refusal is nearer, or as near with a lower tile index.
 * @param refusal New refusal of one candidate cut.
 * @param fallback Cutter position to name when the refusal has no tile (the cut would leave the map).
 * @param target Cell the search is trying to reach.
 */
static void KeepNearestBlocker(ExcavationFootprintResult &blocker, const ExcavationFootprintResult &refusal, TileIndex fallback, TileIndex target)
{
	// The edge of the working domain is the planner's own boundary, not something in the player's way; a turn the
	// cutter cannot make is a route shape, not a blocker either.
	if (refusal.error == ExcavationError::OutsideAuthorization || refusal.error == ExcavationError::IllegalTurn) return;
	const TileIndex tile = refusal.failure_tile == INVALID_TILE ? fallback : refusal.failure_tile;
	if (blocker.error != ExcavationError::None) {
		const uint distance = DistanceMax(tile, target), current = DistanceMax(blocker.failure_tile, target);
		// Nearer refusals explain the failure better; equal distances keep the lowest tile so every peer agrees.
		if (distance > current || (distance == current && tile >= blocker.failure_tile)) return;
	}
	blocker = {refusal.error, tile};
}

/**
 * Check a complete candidate inside explicitly accepted work and actual terrain/claim/infrastructure authority.
 * @param cut Conservative full cutter/consist envelope.
 * @param authorized Canonical accepted work, including caller-approved approach space.
 * @param continuing_job Actual owned job whose preceding accepted cuts will already be open; invalid for a fresh plan.
 * @return Stable first geometry/ownership refusal, or success.
 */
static ExcavationFootprintResult CheckPlanningCut(const ExcavationCutGeometry &cut, std::span<const TileIndex> authorized, ExcavationJobID continuing_job)
{
	const auto &geology = GetGeologyWorldState();
	// Both cutter and every consist corner must remain authorized and free of another producer's active/owned solid work.
	for (TileIndex tile : cut.swept_tiles) {
		if (!std::binary_search(authorized.begin(), authorized.end(), tile)) return {ExcavationError::OutsideAuthorization, tile};
		const auto claim = geology.job_claims.find(tile);
		// Only the same validated job's already accepted prefix may support its
		// future extension. Another project's reservation never becomes free floor.
		if (claim != geology.job_claims.end() && claim->second != continuing_job) return {ExcavationError::ActiveCut, tile};
		const auto terrain = geology.terrain.Get(tile);
		if (!terrain) return {ExcavationError::InvalidTile, tile};
		if (*terrain == ExcavationState::Excavating && (claim == geology.job_claims.end() || claim->second != continuing_job)) return {ExcavationError::ActiveCut, tile};
		// Real support rail/portal occupancy is validated by the later controller; other structures are not implicit demolition permission.
		if (GetTileType(tile) != TileType::Clear && GetTileType(tile) != TileType::Railway && GetTileType(tile) != TileType::LayerPortal) return {ExcavationError::Conflict, tile};
		// A depot shares Railway's tile type but is a building with a constrained door. Its already open floor
		// cannot make it part of a cutter's full physical sweep; only ordinary plain track may support that sweep.
		if (IsTileType(tile, TileType::Railway) && !IsPlainRailTile(tile)) return {ExcavationError::Conflict, tile};
		if (GetTileType(tile) != TileType::Clear && GetTileOwner(tile) != _current_company) return {ExcavationError::InvalidOwner, tile}; // Planning cannot promise another company's support track or portal as collision-free working space.
		const auto *deposit = geology.deposits.Get(geology.deposits.Find(tile));
		if (deposit != nullptr && deposit->claimant != CompanyID::Invalid() && deposit->claimant != _current_company) return {ExcavationError::Conflict, tile}; // Planning cannot promise another company's recoverable seam.
	}
	return {};
}

/**
 * Select only newly proposed cells for a shared hardness quote.
 * @param cut Full accepted transition.
 * @param cleared Actual/open and previously proposed complete-cut cells.
 * @param previous Parent head, whose 3x3 body is already open at this step.
 * @param parent_open False for the first edge of a solid start, whose parent body is the still-unopened pocket.
 * @return Canonical new cells; this virtual set never writes packed terrain.
 */
static std::vector<TileIndex> NewPlanningCells(const ExcavationCutGeometry &cut, const std::set<TileIndex> &cleared, TileIndex previous, bool parent_open = true)
{
	std::vector<TileIndex> fresh;
	for (TileIndex tile : cut.swept_tiles) {
		if (cleared.contains(tile)) continue;
		if (parent_open && DistanceMax(tile, previous) <= 1) continue; // Parent cutter occupancy is open after the preceding complete edge, including within a target search.
		fresh.push_back(tile);
	}
	return fresh;
}

/**
 * Run one deterministic history-sensitive A* search with stable straight/left/right and queue tie order.
 * @param start Current supported proposal pose.
 * @param target Lowest canonical unresolved requested cell.
 * @param outline Explicit accepted work domain.
 * @param cleared Actual/open and already accepted proposal cells.
 * @param model_id Immutable cutter for hardness scoring.
 * @param limits Exact deterministic operation/depth/storage bounds.
 * @param result Whole-command proposal receiving a legal joined prefix and consumed budget.
 * @param solid_start The start pose is inside an unopened portal pocket: its first edge must go straight out of the door.
 * @param continuing_job Validated existing job whose accepted prefix precedes this additional route.
 * @param exact_goal The cutter centre must stand on the target (a tunnel end); otherwise covering it with the body is enough.
 * @return True if this target is covered; false leaves a visible legal partial/refusal with no world mutation.
 */
static bool SearchExcavationTarget(const ExcavationPlanningPose &start, TileIndex target, const ExcavationOutline &outline, const std::set<TileIndex> &cleared, uint16_t model_id, const ExcavationPlanningLimits &limits, ExcavationRouteProposal &result, bool solid_start, ExcavationJobID continuing_job, bool exact_goal = false)
{
	/* Lower bound of the advances still needed; both forms drop by at most one per advance, so the search stays admissible. */
	const auto remaining = [target, exact_goal](TileIndex position) { return exact_goal ? DistanceMax(position, target) : PlanningDistance(position, target); };
	std::vector<ExcavationPlanningNode> nodes;
	nodes.reserve(limits.states);
	nodes.push_back({start, {}, 0, 0, UINT32_MAX});
	std::map<PlanningKey, uint64_t> best{{KeyForPlanningPose(start), 0}};
	using QueueEntry = std::tuple<uint64_t, uint64_t, uint32_t, uint8_t, uint32_t>;
	std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> queue;
	queue.emplace(remaining(start.position), 0, start.position.base(), to_underlying(start.heading), 0);
	uint32_t closest = 0, found = UINT32_MAX;
	bool limited = false;
	ExcavationFootprintResult failure{ExcavationError::InvalidRoute, target};
	ExcavationFootprintResult blocker;
	// Deterministic operation counts bound work across the command; wall/CPU clocks never choose a multiplayer-dependent route.
	while (!queue.empty()) {
		if (result.expansions >= limits.expansions) { limited = true; break; }
		const uint32_t index = std::get<4>(queue.top());
		queue.pop();
		const auto node = nodes[index]; // Node insertion may reallocate; this complete small value keeps the expanded parent stable.
		if (best.at(KeyForPlanningPose(node.pose)) != node.cost) continue;
		++result.expansions;
		if (remaining(node.pose.position) < remaining(nodes[closest].pose.position)) closest = index;
		// A cutter body covers its target once its 3x3 is open. The root of a solid start is still inside the unopened
		// pocket, so even a pocket target needs the first real advance out of the door.
		if (remaining(node.pose.position) == 0 && !(solid_start && index == 0)) { found = index; break; }
		if (node.depth >= limits.depth || result.route.size() + node.depth >= limits.segments) { limited = true; continue; } // Bounded passes cannot grow another hidden unaccepted route.
		// The root of a solid start sits in the straight housing track: it may only advance straight out of the door,
		// and its unopened pocket is charged as new rock. A root whose whole consist trail still stands on its own tile is
		// in its housing too, an opened one: support rail leaves a housing only through its door, straight on, so it may
		// not turn either (r2df Group 9). Every other node keeps the normal straight/left/right choice.
		const bool leaving_housing = solid_start && index == 0;
		const bool stacked = index == 0 && std::ranges::all_of(node.pose.history, [&node](TileIndex point) { return point == node.pose.position; });
		const std::span<const int> turns = leaving_housing || stacked ? std::span<const int>{STRAIGHT_ONLY} : std::span<const int>{STRAIGHT_LEFT_RIGHT};
		// Forward straight is considered first, then exactly left45/right45; every edge advances a real tile.
		for (int turn : turns) {
			const auto heading = static_cast<Direction>((to_underlying(node.pose.heading) + turn + 8) % 8);
			auto step = AdvanceExcavationPlanningPose(node.pose, heading);
			if (step.error != ExcavationError::None) {
				failure = {step.error, step.failure_tile};
				KeepNearestBlocker(blocker, failure, node.pose.position, target);
				continue;
			}
			const auto legal = CheckPlanningCut(step.cut, outline.authorized_tiles, continuing_job);
			if (legal.error != ExcavationError::None) {
				failure = legal;
				KeepNearestBlocker(blocker, failure, node.pose.position, target);
				continue;
			}
			const auto cells = NewPlanningCells(step.cut, cleared, node.pose.position, !leaving_housing);
			const auto quote = QuoteExcavationCells(cells, model_id, continuing_job);
			if (quote.error != ExcavationError::None) {
				failure = {quote.error, quote.failure_tile};
				KeepNearestBlocker(blocker, failure, node.pose.position, target);
				continue;
			}
			// Frozen active-rate/time plus lining cost gives hardness-sensitive conservative monetary scoring; it charges no account.
			const uint64_t edge_cost = 1 + quote.cost.base() + quote.work_ticks / 74 * (model_id == 0 ? 480 : 960);
			const uint64_t cost = node.cost + edge_cost;
			const auto key = KeyForPlanningPose(step.next);
			const auto old = best.find(key);
			if (old != best.end() && old->second <= cost) continue; // A history-equivalent more expensive pose cannot improve coverage or collision legality.
			if (nodes.size() >= limits.states) { limited = true; continue; }
			best[key] = cost;
			const uint32_t next_index = static_cast<uint32_t>(nodes.size());
			nodes.push_back({std::move(step.next), std::move(step.cut), cost, node.depth + 1, index});
			const auto &next = nodes.back();
			queue.emplace(cost + remaining(next.pose.position), cost, next.pose.position.base(), to_underlying(next.pose.heading), next_index);
		}
	}
	const uint32_t selected = found == UINT32_MAX ? closest : found;
	std::vector<ExcavationCutGeometry> reverse;
	// Parent links prove actual advances; reconstruction cannot replace bends with repeated rotations on one tile.
	for (uint32_t index = selected; nodes[index].parent != UINT32_MAX; index = nodes[index].parent) reverse.push_back(nodes[index].cut);
	std::reverse(reverse.begin(), reverse.end());
	result.route.insert(result.route.end(), reverse.begin(), reverse.end());
	result.end = nodes[selected].pose;
	if (found != UINT32_MAX) return true;
	result.error = limited ? ExcavationError::Limit : failure.error;
	result.failure_tile = limited ? target : failure.failure_tile;
	result.blocker = blocker;
	result.partial = !result.route.empty();
	return false;
}

/**
 * Cover remaining requested floor in stable order by joining legal bounded advancing passes.
 * @param outline Actual explicitly accepted requested/work domain.
 * @param start Supported cutter and planned consist geometry.
 * @param model_id Immutable cutter capability.
 * @param limits Deterministic work/storage/route bounds.
 * @param continuation Validated live same-company job, supplied by the extension and replacement wrappers; its whole
 *                     route (for a replacement, a copy cut to the completed prefix) precedes the new work.
 * @param intent Requested area coverage or exact straight advance.
 * @param tunnel_end Exact cutter-centre endpoint for straight intent.
 * @param exact_search For straight intent: reach the endpoint by the bounded search, bends allowed, instead of
 *                     advancing straight on (the approach producer's exact goal, r2df Group 8).
 * @return Full/visible partial proposal; claims, terrain, funds, cargo and RNG are unchanged.
 */
static ExcavationRouteProposal PlanExcavationRouteInternal(const ExcavationOutline &outline, const ExcavationPlanningPose &start, uint16_t model_id, const ExcavationPlanningLimits &limits, const ExcavationJob *continuation, ExcavationPlanIntent intent, TileIndex tunnel_end, bool exact_search = false)
{
	if (intent > ExcavationPlanIntent::StraightTunnel) return {ExcavationError::InvalidRoute};
	// Reject unsafe/unbounded operation settings before allocating any search node or reading caller geometry.
	if (limits.states == 0 || limits.states > 8192 || limits.expansions == 0 || limits.expansions > 32768 || limits.depth == 0 || limits.depth > MAX_EXCAVATION_ROUTE || limits.segments == 0 || limits.segments > MAX_EXCAVATION_ROUTE) return {ExcavationError::Limit};
	if (GetGeologyWorldState().generator_version != 1) return {ExcavationError::InvalidPolicy};
	const auto normalized = NormalizeExcavationOutline(outline.layer, outline.requested_tiles);
	if (normalized.error != ExcavationError::None) return {normalized.error, normalized.failure_tile};
	const auto work = NormalizeExcavationOutline(outline.layer, outline.authorized_tiles);
	if (work.error != ExcavationError::None) return {work.error, work.failure_tile};
	// Requested final floor must remain inside explicitly accepted work; a forged proposal cannot silently widen permission.
	for (TileIndex tile : normalized.requested_tiles) {
		if (!std::binary_search(work.requested_tiles.begin(), work.requested_tiles.end(), tile)) return {ExcavationError::OutsideAuthorization, tile};
	}
	const auto initial = AdvanceExcavationPlanningPose(start, start.heading);
	if (initial.error != ExcavationError::None) return {initial.error, initial.failure_tile};
	if (GetTileLayer(start.position) != outline.layer) return {ExcavationError::MixedLayers, start.position};
	if (continuation == nullptr) {
		const auto model_probe = QuoteExcavationCells(std::array{start.position}, model_id);
		if (model_probe.error != ExcavationError::None) return {model_probe.error, model_probe.failure_tile};
	}
	// A start pose on an unopened portal housing is a solid start. Its issuer must be a current Sealed pair of this
	// company; the cutter must face the door and the whole straddling consist must still be in the shaft.
	ExcavationJob probe;
	probe.commissioning_anchor = start.position;
	probe.company = _current_company;
	const auto starter = continuation == nullptr ? ResolveExcavationStarter(probe) : ExcavationStarterResult{};
	if (starter.error != ExcavationError::None) return {starter.error, starter.failure_tile};
	if (starter.required) {
		if (start.heading != starter.exit_heading) return {ExcavationError::IllegalTurn, start.position};
		for (TileIndex point : start.history) {
			if (point != start.position) return {ExcavationError::InvalidRoute, point};
		}
	}
	const auto &geology = GetGeologyWorldState();
	// The continuing job's own unfinished claims are quoted as the rock they still are. An extension never reaches
	// them (its whole accepted route is virtual floor); a replacement may reuse the cells of its dropped cuts.
	const ExcavationJobID own_job = continuation == nullptr ? ExcavationJobID{UINT32_MAX} : continuation->id;
	std::set<TileIndex> cleared;
	if (continuation != nullptr) {
		// This is a forecast after all existing accepted cuts, not a second
		// terrain store. Old unopened cells retain their live paid-work claims.
		for (const auto &cut : continuation->route) cleared.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
	}
	// Actual open work cells form the initial virtual floor; no planning path may pre-excavate its future support in live storage.
	for (TileIndex tile : work.requested_tiles) {
		if (geology.terrain.Get(tile) == ExcavationState::Excavated) cleared.insert(tile);
	}
	// Open starts need open initial support. A solid start's support is its unopened pocket: those cells stay
	// out of the virtual floor so the first edge charges them as new rock.
	for (TileIndex tile : initial.cut.support_tiles) {
		if (cleared.contains(tile) || geology.terrain.Get(tile) == ExcavationState::Excavated) {
			cleared.insert(tile);
			continue;
		}
		if (!starter.required || !std::binary_search(starter.pocket.begin(), starter.pocket.end(), tile)) return {ExcavationError::MissingSupport, tile};
	}
	ExcavationRouteProposal result;
	result.end = start;
	if (intent == ExcavationPlanIntent::StraightTunnel) {
		// A tunnel means the centre reaches the chosen end. Covering the end with the edge of the body, or turning to
		// cheaper rock, would silently change the player's requested railway alignment.
		if (!IsValidTile(tunnel_end) || GetTileLayer(tunnel_end) != outline.layer) return {ExcavationError::MixedLayers, tunnel_end};
		const int dx = int(TileX(tunnel_end)) - int(TileX(start.position));
		const int dy = int(TilePlaneY(tunnel_end)) - int(TilePlaneY(start.position));
		const int steps = std::max(std::abs(dx), std::abs(dy));
		const auto delta = TileIndexDiffCByDir(start.heading);
		if (steps == 0) return {ExcavationError::IllegalTurn, tunnel_end};
		// An end exactly ahead keeps the straight advance. Any other end (r2df task 9.4: an angled tunnel, whose work
		// the TBM owner built from an exact-goal approach) is reached through the bounded exact-goal search.
		const bool ahead = dx == steps * delta.x && dy == steps * delta.y;
		if (exact_search || !ahead) {
			// The centre reaches the end through advancing 45-degree bends inside the explicit work domain. Each cut is
			// quoted once, against the cells earlier cuts of this route already opened, exactly like the area loop.
			const bool reached = SearchExcavationTarget(result.end, tunnel_end, work, cleared, model_id, limits, result, starter.required,
				continuation == nullptr ? ExcavationJobID{UINT32_MAX} : continuation->id, true);
			for (const auto &cut : result.route) {
				std::vector<TileIndex> fresh;
				for (TileIndex tile : cut.swept_tiles) {
					if (!cleared.contains(tile)) fresh.push_back(tile);
				}
				const auto quote = QuoteExcavationCells(fresh, model_id, own_job);
				if (quote.error != ExcavationError::None) return {quote.error, quote.failure_tile};
				result.estimate.new_cells += quote.new_cells; result.estimate.earth_t += quote.earth_t; result.estimate.rock_t += quote.rock_t;
				result.estimate.support_units += quote.support_units; result.estimate.cost += quote.cost; result.estimate.work_ticks += quote.work_ticks;
				cleared.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
			}
			// Like the straight loop, only arrival on the end with the whole requested floor swept is a full proposal.
			for (TileIndex tile : normalized.requested_tiles) {
				(cleared.contains(tile) ? result.covered_targets : result.remaining_targets).push_back(tile);
			}
			result.partial = !reached || !result.remaining_targets.empty();
			return result;
		}
		if (uint32_t(steps) > limits.segments || uint32_t(steps) > limits.depth || uint32_t(steps) > limits.expansions) return {ExcavationError::Limit, tunnel_end};
		// Every straight edge uses the same complete advancing sweep, ownership
		// checks and paid material quote as area search. No floor is opened here.
		for (int index = 0; index < steps; ++index) {
			auto step = AdvanceExcavationPlanningPose(result.end, start.heading);
			const auto legal = step.error == ExcavationError::None ? CheckPlanningCut(step.cut, work.requested_tiles,
				continuation == nullptr ? ExcavationJobID{UINT32_MAX} : continuation->id) : ExcavationFootprintResult{step.error, step.failure_tile};
			++result.expansions;
			if (legal.error != ExcavationError::None) {
				result.error = legal.error;
				result.failure_tile = legal.failure_tile;
				break;
			}
			std::vector<TileIndex> fresh;
			for (TileIndex tile : step.cut.swept_tiles) {
				if (!cleared.contains(tile)) fresh.push_back(tile);
			}
			const auto quote = QuoteExcavationCells(fresh, model_id, own_job);
			if (quote.error != ExcavationError::None) {
				result.error = quote.error;
				result.failure_tile = quote.failure_tile;
				break;
			}
			result.estimate.new_cells += quote.new_cells; result.estimate.earth_t += quote.earth_t; result.estimate.rock_t += quote.rock_t;
			result.estimate.support_units += quote.support_units; result.estimate.cost += quote.cost; result.estimate.work_ticks += quote.work_ticks;
			cleared.insert(step.cut.swept_tiles.begin(), step.cut.swept_tiles.end());
			result.route.push_back(std::move(step.cut));
			result.end = std::move(step.next);
		}
		result.partial = result.end.position != tunnel_end;
		// The route may explain a refused prefix, but only full endpoint arrival
		// and complete requested-floor coverage can be confirmed by the TBM owner.
		for (TileIndex tile : normalized.requested_tiles) {
			(cleared.contains(tile) ? result.covered_targets : result.remaining_targets).push_back(tile);
		}
		if (!result.remaining_targets.empty()) result.partial = true;
		return result;
	}
	// Select the lowest remaining final-floor cell, then recompute virtual coverage from complete accepted sweeps, never endpoint centres alone.
	for (TileIndex target : normalized.requested_tiles) {
		if (cleared.contains(target)) continue;
		const size_t before = result.route.size();
		// Only the very first edge of a solid start leaves the housing; later searches start from opened ground.
		const bool reached = SearchExcavationTarget(result.end, target, work, cleared, model_id, limits, result, starter.required && result.route.empty(),
			continuation == nullptr ? ExcavationJobID{UINT32_MAX} : continuation->id);
		for (size_t i = before; i < result.route.size(); ++i) {
			std::vector<TileIndex> fresh;
			for (TileIndex tile : result.route[i].swept_tiles) {
				if (!cleared.contains(tile)) fresh.push_back(tile);
			}
			const auto quote = QuoteExcavationCells(fresh, model_id, own_job);
			if (quote.error != ExcavationError::None) return {quote.error, quote.failure_tile};
			result.estimate.new_cells += quote.new_cells; result.estimate.earth_t += quote.earth_t; result.estimate.rock_t += quote.rock_t;
			result.estimate.support_units += quote.support_units; result.estimate.cost += quote.cost; result.estimate.work_ticks += quote.work_ticks;
			cleared.insert(result.route[i].swept_tiles.begin(), result.route[i].swept_tiles.end());
		}
		if (!reached) break;
	}
	// Explicit complete/remaining lists make a bounded partial proposal reviewable without pretending the entire outline was accepted or excavated.
	for (TileIndex tile : normalized.requested_tiles) {
		(cleared.contains(tile) ? result.covered_targets : result.remaining_targets).push_back(tile);
	}
	return result;
}

/** @copydoc PlanExcavationRoute */
ExcavationRouteProposal PlanExcavationRoute(const ExcavationOutline &outline, const ExcavationPlanningPose &start, uint16_t model_id, const ExcavationPlanningLimits &limits, ExcavationPlanIntent intent, TileIndex tunnel_end)
{
	return PlanExcavationRouteInternal(outline, start, model_id, limits, nullptr, intent, tunnel_end);
}

/** @copydoc PlanExcavationJobExtension */
ExcavationRouteProposal PlanExcavationJobExtension(ExcavationJobID id, const ExcavationOutline &outline, uint8_t planning_points, const ExcavationPlanningLimits &limits, ExcavationPlanIntent intent, TileIndex tunnel_end)
{
	const auto *job = GetExcavationJob(id);
	// An extension must start from this company's actual accepted history;
	// callers cannot make another claim traversable by presenting a copied ID.
	if (job == nullptr || !Company::IsValidID(job->company) || job->company != _current_company) return {ExcavationError::InvalidOwner};
	// Completed/cancelled work has released its claims; reusing that machine
	// requires a new project through its real portal or hangar owner.
	if (job->state == ExcavationJobState::Completed || job->state == ExcavationJobState::Cancelled) return {ExcavationError::AlreadyCompleted};
	// A bounded native consist and a real accepted endpoint are prerequisites
	// for reconstructing the tail; neither can be inferred from a clicked tile.
	if (planning_points < 3 || planning_points > 16 || job->route.empty()) return {ExcavationError::InvalidRoute};
	const auto valid = ValidateExcavationJobGeometry(GetGeologyWorldState(), *job);
	if (valid.error != ExcavationError::None) return {valid.error, valid.failure_tile};
	if (job->route.size() >= MAX_EXCAVATION_ROUTE) return {ExcavationError::Limit};
	if (limits.segments > MAX_EXCAVATION_ROUTE) return {ExcavationError::Limit}; // Clamping remaining storage must not conceal an invalid caller budget.
	const auto replay = ReplayExcavationRoute(*job, planning_points, job->route.size());
	if (replay.error != ExcavationError::None) return {replay.error, replay.failure_tile};
	auto bounded = limits;
	bounded.segments = std::min<uint32_t>(bounded.segments, MAX_EXCAVATION_ROUTE - job->route.size());
	return PlanExcavationRouteInternal(outline, replay.next, job->model_id, bounded, job, intent, tunnel_end);
}

/** @copydoc PlanExcavationJobReplacement */
ExcavationRouteProposal PlanExcavationJobReplacement(ExcavationJobID id, const ExcavationOutline &outline, uint8_t planning_points, const ExcavationPlanningLimits &limits, ExcavationPlanIntent intent, TileIndex tunnel_end)
{
	const auto *job = GetExcavationJob(id);
	// Only this company's own live job may be replanned; a copied ID cannot make another project's claims traversable.
	if (job == nullptr || !Company::IsValidID(job->company) || job->company != _current_company) return {ExcavationError::InvalidOwner};
	if (job->state == ExcavationJobState::Completed || job->state == ExcavationJobState::Cancelled) return {ExcavationError::AlreadyCompleted};
	// A bounded native consist and at least one completed cut are needed: with none, the cutter is still in its
	// housing, and the first cut out of it belongs to a commission, not to a replacement.
	if (planning_points < 3 || planning_points > 16 || job->cursor == 0 || job->cursor >= job->route.size()) return {ExcavationError::InvalidRoute};
	const auto valid = ValidateExcavationJobGeometry(GetGeologyWorldState(), *job);
	if (valid.error != ExcavationError::None) return {valid.error, valid.failure_tile};
	if (limits.segments > MAX_EXCAVATION_ROUTE) return {ExcavationError::Limit}; // Clamping remaining storage must not conceal an invalid caller budget.
	const auto replay = ReplayExcavationRoute(*job, planning_points, job->cursor);
	if (replay.error != ExcavationError::None) return {replay.error, replay.failure_tile};
	// The new route continues the completed prefix only. The unfinished cuts are dropped: they are not open floor, and
	// their own claims neither block the new route nor make their unopened cells free.
	ExcavationJob kept = *job;
	kept.route.resize(job->cursor);
	auto bounded = limits;
	bounded.segments = std::min<uint32_t>(bounded.segments, MAX_EXCAVATION_ROUTE - job->cursor);
	return PlanExcavationRouteInternal(outline, replay.next, job->model_id, bounded, &kept, intent, tunnel_end);
}

/** @copydoc ReplayExcavationRoute */
ExcavationPlanningStep ReplayExcavationRoute(const ExcavationJob &job, uint8_t planning_points, size_t cuts)
{
	if (planning_points < 3 || planning_points > 16 || job.route.empty() || cuts > job.route.size()) return {ExcavationError::InvalidRoute};
	ExcavationPlanningStep result;
	result.next = {job.commissioning_anchor, job.route.front().heading, std::vector<TileIndex>(planning_points, job.commissioning_anchor)};
	// Replaying each accepted sweep proves the complete real consist's history. A shorter fake train may not squeeze a
	// new bend into less space, and a cut the replay does not reproduce exactly is not this consist's route.
	for (size_t index = 0; index < cuts; ++index) {
		const auto &cut = job.route[index];
		auto step = AdvanceExcavationPlanningPose(result.next, cut.heading);
		if (step.error != ExcavationError::None || step.cut != cut) return {ExcavationError::InvalidRoute, cut.cutter_position};
		result.next = std::move(step.next);
		result.cut = cut;
	}
	return result;
}

/**
 * Cells around the start and the floor in which approach and turning space may be planned: room for the tightest
 * U-turn. Four 45-degree bends on consecutive advances move the cutter centre 3 cells sideways, plus 1 for its body.
 */
static constexpr int APPROACH_MARGIN = 4;

/** @copydoc PlanExcavationApproach */
ExcavationApproachProposal PlanExcavationApproach(const ExcavationPlanningPose &start, const ExcavationOutline &target, uint16_t model_id, const ExcavationPlanningLimits &limits, ExcavationJobID continuing_job, bool solid_start, bool exact_goal, uint32_t kept_cuts)
{
	// The same unsafe/unbounded settings, generator, layer and Deep refusals as a route, before any search allocates.
	if (limits.states == 0 || limits.states > 8192 || limits.expansions == 0 || limits.expansions > 32768 || limits.depth == 0 || limits.depth > MAX_EXCAVATION_ROUTE || limits.segments == 0 || limits.segments > MAX_EXCAVATION_ROUTE) return {ExcavationError::Limit};
	if (GetGeologyWorldState().generator_version != 1) return {ExcavationError::InvalidPolicy};
	const auto floor = NormalizeExcavationOutline(target.layer, target.requested_tiles);
	if (floor.error != ExcavationError::None) return {floor.error, floor.failure_tile};
	// The existing authorization is the floor plus what the caller already accepted (ring, housing, earlier work). It
	// may still be in pieces, a floor away from its housing: joining them is what the approach is for.
	if (target.authorized_tiles.size() > MAX_EXCAVATION_JOB_TILES * 4) return {ExcavationError::Limit};
	std::vector<TileIndex> base(target.authorized_tiles.begin(), target.authorized_tiles.end());
	base.insert(base.end(), floor.requested_tiles.begin(), floor.requested_tiles.end());
	CanonicalizePlanningTiles(base);
	if (base.size() > MAX_EXCAVATION_JOB_TILES) return {ExcavationError::Limit, base.front()};
	// Canonical order names the lowest invalid or wrong-layer cell, whatever order the caller listed them in.
	for (TileIndex tile : base) {
		if (!IsValidTile(tile)) return {ExcavationError::InvalidTile, tile};
		if (GetTileLayer(tile) != target.layer) return {ExcavationError::MixedLayers, tile};
	}
	if (!IsValidTile(start.position) || GetTileLayer(start.position) != target.layer) return {ExcavationError::MixedLayers, start.position};

	const ExcavationJob *job = nullptr;
	ExcavationJob kept;
	auto bounded = limits;
	if (continuing_job != ExcavationJobID{UINT32_MAX}) {
		// The same checks as an extension: this company's unfinished, valid job with route capacity left.
		job = GetExcavationJob(continuing_job);
		if (job == nullptr || !Company::IsValidID(job->company) || job->company != _current_company) return {ExcavationError::InvalidOwner};
		if (job->state == ExcavationJobState::Completed || job->state == ExcavationJobState::Cancelled) return {ExcavationError::AlreadyCompleted};
		const auto valid = ValidateExcavationJobGeometry(GetGeologyWorldState(), *job);
		if (valid.error != ExcavationError::None) return {valid.error, valid.failure_tile};
		// A remainder replacement continues the completed cuts only, exactly as PlanExcavationJobReplacement plans it.
		if (kept_cuts < job->route.size()) {
			if (kept_cuts == 0) return {ExcavationError::InvalidRoute, start.position};
			kept = *job;
			kept.route.resize(kept_cuts);
			job = &kept;
		}
		if (job->route.size() >= MAX_EXCAVATION_ROUTE) return {ExcavationError::Limit};
		// A continuing job starts at the open end of its accepted route, never inside an unopened housing.
		if (solid_start) return {ExcavationError::InvalidRoute, start.position};
		bounded.segments = std::min<uint32_t>(bounded.segments, MAX_EXCAVATION_ROUTE - static_cast<uint32_t>(job->route.size()));
	} else if (!solid_start) {
		// The caller promised open ground; an unopened housing would need its pocket paid for, so it refuses.
		ExcavationJob probe;
		probe.commissioning_anchor = start.position;
		probe.company = _current_company;
		const auto starter = ResolveExcavationStarter(probe);
		if (starter.error != ExcavationError::None) return {starter.error, starter.failure_tile};
		if (starter.required) return {ExcavationError::MissingSupport, start.position};
	}

	// Goal: the requested cell nearest the start. Canonical order keeps the lowest tile among equally near cells.
	TileIndex goal = INVALID_TILE;
	uint goal_distance = UINT_MAX;
	for (TileIndex tile : floor.requested_tiles) {
		const uint distance = exact_goal ? DistanceMax(start.position, tile) : PlanningDistance(start.position, tile);
		if (distance < goal_distance) {
			goal_distance = distance;
			goal = tile;
		}
	}

	ExcavationApproachProposal result;
	result.end = start;
	if (!exact_goal) {
		// Most floors are covered inside the floor, its ring and the housing: then no extra cell is proposed and the
		// quote stays exactly what it was before approaches existed.
		ExcavationOutline existing;
		existing.layer = target.layer;
		existing.requested_tiles = floor.requested_tiles;
		existing.authorized_tiles = base; // A base in pieces refuses here and goes on to the wider box.
		const auto inside = PlanExcavationRouteInternal(existing, start, model_id, bounded, job, ExcavationPlanIntent::Area, INVALID_TILE);
		result.expansions += inside.expansions;
		if (inside.error == ExcavationError::None && !inside.partial && inside.remaining_targets.empty()) return result;
	}

	// The box around the consist trail and the floor, grown by the margin and clipped to the map.
	int min_x = INT_MAX, min_y = INT_MAX, max_x = INT_MIN, max_y = INT_MIN;
	const auto widen = [&](TileIndex tile) {
		min_x = std::min<int>(min_x, TileX(tile)); max_x = std::max<int>(max_x, TileX(tile));
		min_y = std::min<int>(min_y, TilePlaneY(tile)); max_y = std::max<int>(max_y, TilePlaneY(tile));
	};
	for (TileIndex tile : start.history) widen(tile);
	widen(start.position);
	for (TileIndex tile : floor.requested_tiles) widen(tile);
	min_x = std::max(0, min_x - APPROACH_MARGIN); min_y = std::max(0, min_y - APPROACH_MARGIN);
	max_x = std::min<int>(Map::MaxX(), max_x + APPROACH_MARGIN); max_y = std::min<int>(Map::MaxY(), max_y + APPROACH_MARGIN);
	const auto refuse = [&result](ExcavationError error, TileIndex tile) {
		ExcavationApproachProposal refused;
		refused.error = error;
		refused.failure_tile = tile;
		refused.expansions = result.expansions;
		return refused;
	};
	// A box larger than one job's authorized area cannot be accepted anyway: the floor is too far from the start.
	if (static_cast<uint64_t>(max_x - min_x + 1) * static_cast<uint64_t>(max_y - min_y + 1) > MAX_EXCAVATION_JOB_TILES) return refuse(ExcavationError::Limit, goal);
	std::vector<TileIndex> domain = base;
	// Every playable cell of the box; the void border of the map is never working space.
	for (int y = min_y; y <= max_y; ++y) {
		for (int x = min_x; x <= max_x; ++x) {
			const TileIndex tile = TileXYL(x, y, target.layer);
			if (IsValidTile(tile)) domain.push_back(tile);
		}
	}
	CanonicalizePlanningTiles(domain);
	if (domain.size() > MAX_EXCAVATION_JOB_TILES) return refuse(ExcavationError::Limit, goal);

	ExcavationOutline wide;
	wide.layer = target.layer;
	wide.requested_tiles = floor.requested_tiles;
	wide.authorized_tiles = std::move(domain);
	const auto planned = exact_goal ?
		PlanExcavationRouteInternal(wide, start, model_id, bounded, job, ExcavationPlanIntent::StraightTunnel, goal, true) :
		PlanExcavationRouteInternal(wide, start, model_id, bounded, job, ExcavationPlanIntent::Area, INVALID_TILE);
	result.expansions += planned.expansions;
	// An approach is all or nothing: a full arrival (exact) or full coverage (area), otherwise a refusal with no cells.
	const bool complete = planned.error == ExcavationError::None && !planned.partial && planned.remaining_targets.empty();
	if (!complete) {
		// The budget, then the refusal nearest the unreached target (building, claim, seam, map edge), explain it. A search
		// stopped only by the box edge needs more room than the margin allows: too far to plan, shown at the goal.
		if (planned.error == ExcavationError::Limit) return refuse(ExcavationError::Limit, planned.failure_tile == INVALID_TILE ? goal : planned.failure_tile);
		if (planned.blocker.error != ExcavationError::None) return refuse(planned.blocker.error, planned.blocker.failure_tile);
		if (planned.error == ExcavationError::OutsideAuthorization) return refuse(ExcavationError::Limit, goal);
		if (planned.error != ExcavationError::None) return refuse(planned.error, planned.failure_tile);
		return refuse(ExcavationError::InvalidRoute, planned.remaining_targets.empty() ? goal : planned.remaining_targets.front());
	}
	result.route = planned.route;
	result.end = planned.end;
	std::vector<TileIndex> swept;
	for (const auto &cut : result.route) swept.insert(swept.end(), cut.swept_tiles.begin(), cut.swept_tiles.end());
	CanonicalizePlanningTiles(swept);
	std::set_difference(swept.begin(), swept.end(), base.begin(), base.end(), std::back_inserter(result.extra_tiles));
	return result;
}
