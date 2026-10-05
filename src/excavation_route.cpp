/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_route.cpp Deterministic bounded advancing coverage with complete cutter/consist sweeps. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Groups 13 and 14B (solid start); docs/zz-reach2/architecture/map-layers/archi-map-layers.md */

#include "stdafx.h"
#include "excavation.h"
#include "tile_map.h"
#include "company_func.h"
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
 * Check a complete candidate inside explicitly accepted work and actual terrain/claim/infrastructure authority.
 * @param cut Conservative full cutter/consist envelope.
 * @param authorized Canonical accepted work, including caller-approved approach space.
 * @return Stable first geometry/ownership refusal, or success.
 */
static ExcavationFootprintResult CheckPlanningCut(const ExcavationCutGeometry &cut, std::span<const TileIndex> authorized)
{
	const auto &geology = GetGeologyWorldState();
	// Both cutter and every consist corner must remain authorized and free of another producer's active/owned solid work.
	for (TileIndex tile : cut.swept_tiles) {
		if (!std::binary_search(authorized.begin(), authorized.end(), tile)) return {ExcavationError::OutsideAuthorization, tile};
		if (geology.job_claims.contains(tile)) return {ExcavationError::ActiveCut, tile};
		const auto terrain = geology.terrain.Get(tile);
		if (!terrain) return {ExcavationError::InvalidTile, tile};
		if (*terrain == ExcavationState::Excavating) return {ExcavationError::ActiveCut, tile};
		// Real support rail/portal occupancy is validated by the later controller; other structures are not implicit demolition permission.
		if (GetTileType(tile) != TileType::Clear && GetTileType(tile) != TileType::Railway && GetTileType(tile) != TileType::LayerPortal) return {ExcavationError::Conflict, tile};
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
 * @return True if this target is covered; false leaves a visible legal partial/refusal with no world mutation.
 */
static bool SearchExcavationTarget(const ExcavationPlanningPose &start, TileIndex target, const ExcavationOutline &outline, const std::set<TileIndex> &cleared, uint16_t model_id, const ExcavationPlanningLimits &limits, ExcavationRouteProposal &result, bool solid_start = false)
{
	std::vector<ExcavationPlanningNode> nodes;
	nodes.reserve(limits.states);
	nodes.push_back({start, {}, 0, 0, UINT32_MAX});
	std::map<PlanningKey, uint64_t> best{{KeyForPlanningPose(start), 0}};
	using QueueEntry = std::tuple<uint64_t, uint64_t, uint32_t, uint8_t, uint32_t>;
	std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> queue;
	queue.emplace(PlanningDistance(start.position, target), 0, start.position.base(), to_underlying(start.heading), 0);
	uint32_t closest = 0, found = UINT32_MAX;
	bool limited = false;
	ExcavationFootprintResult failure{ExcavationError::InvalidRoute, target};
	// Deterministic operation counts bound work across the command; wall/CPU clocks never choose a multiplayer-dependent route.
	while (!queue.empty()) {
		if (result.expansions >= limits.expansions) { limited = true; break; }
		const uint32_t index = std::get<4>(queue.top());
		queue.pop();
		const auto node = nodes[index]; // Node insertion may reallocate; this complete small value keeps the expanded parent stable.
		if (best.at(KeyForPlanningPose(node.pose)) != node.cost) continue;
		++result.expansions;
		if (PlanningDistance(node.pose.position, target) < PlanningDistance(nodes[closest].pose.position, target)) closest = index;
		// A cutter body covers its target once its 3x3 is open. The root of a solid start is still inside the unopened
		// pocket, so even a pocket target needs the first real advance out of the door.
		if (PlanningDistance(node.pose.position, target) == 0 && !(solid_start && index == 0)) { found = index; break; }
		if (node.depth >= limits.depth || result.route.size() + node.depth >= limits.segments) { limited = true; continue; } // Bounded passes cannot grow another hidden unaccepted route.
		// The root of a solid start sits in the straight housing track: it may only advance straight out of the door,
		// and its unopened pocket is charged as new rock. Every other node keeps the normal straight/left/right choice.
		const bool leaving_housing = solid_start && index == 0;
		const std::span<const int> turns = leaving_housing ? std::span<const int>{STRAIGHT_ONLY} : std::span<const int>{STRAIGHT_LEFT_RIGHT};
		// Forward straight is considered first, then exactly left45/right45; every edge advances a real tile.
		for (int turn : turns) {
			const auto heading = static_cast<Direction>((to_underlying(node.pose.heading) + turn + 8) % 8);
			auto step = AdvanceExcavationPlanningPose(node.pose, heading);
			if (step.error != ExcavationError::None) { failure = {step.error, step.failure_tile}; continue; }
			const auto legal = CheckPlanningCut(step.cut, outline.authorized_tiles);
			if (legal.error != ExcavationError::None) { failure = legal; continue; }
			const auto cells = NewPlanningCells(step.cut, cleared, node.pose.position, !leaving_housing);
			const auto quote = QuoteExcavationCells(cells, model_id);
			if (quote.error != ExcavationError::None) { failure = {quote.error, quote.failure_tile}; continue; }
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
			queue.emplace(cost + PlanningDistance(next.pose.position, target), cost, next.pose.position.base(), to_underlying(next.pose.heading), next_index);
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
	result.partial = !result.route.empty();
	return false;
}

/**
 * Cover remaining requested floor in stable order by joining legal bounded advancing passes.
 * @param outline Actual explicitly accepted requested/work domain.
 * @param start Supported cutter and planned consist geometry.
 * @param model_id Immutable cutter capability.
 * @param limits Deterministic work/storage/route bounds.
 * @return Full/visible partial proposal; claims, terrain, funds, cargo and RNG are unchanged.
 */
ExcavationRouteProposal PlanExcavationRoute(const ExcavationOutline &outline, const ExcavationPlanningPose &start, uint16_t model_id, const ExcavationPlanningLimits &limits)
{
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
	const auto model_probe = QuoteExcavationCells(std::array{start.position}, model_id);
	if (model_probe.error != ExcavationError::None) return {model_probe.error, model_probe.failure_tile};
	// A start pose on an unopened portal housing is a solid start. Its issuer must be a current Sealed pair of this
	// company; the cutter must face the door and the whole straddling consist must still be in the shaft.
	ExcavationJob probe;
	probe.commissioning_anchor = start.position;
	probe.company = _current_company;
	const auto starter = ResolveExcavationStarter(probe);
	if (starter.error != ExcavationError::None) return {starter.error, starter.failure_tile};
	if (starter.required) {
		if (start.heading != starter.exit_heading) return {ExcavationError::IllegalTurn, start.position};
		for (TileIndex point : start.history) {
			if (point != start.position) return {ExcavationError::InvalidRoute, point};
		}
	}
	const auto &geology = GetGeologyWorldState();
	std::set<TileIndex> cleared;
	// Actual open work cells form the initial virtual floor; no planning path may pre-excavate its future support in live storage.
	for (TileIndex tile : work.requested_tiles) {
		if (geology.terrain.Get(tile) == ExcavationState::Excavated) cleared.insert(tile);
	}
	// Open starts need open initial support. A solid start's support is its unopened pocket: those cells stay
	// out of the virtual floor so the first edge charges them as new rock.
	for (TileIndex tile : initial.cut.support_tiles) {
		if (geology.terrain.Get(tile) == ExcavationState::Excavated) {
			cleared.insert(tile);
			continue;
		}
		if (!starter.required || !std::binary_search(starter.pocket.begin(), starter.pocket.end(), tile)) return {ExcavationError::MissingSupport, tile};
	}
	ExcavationRouteProposal result;
	result.end = start;
	// Select the lowest remaining final-floor cell, then recompute virtual coverage from complete accepted sweeps, never endpoint centres alone.
	for (TileIndex target : normalized.requested_tiles) {
		if (cleared.contains(target)) continue;
		const size_t before = result.route.size();
		// Only the very first edge of a solid start leaves the housing; later searches start from opened ground.
		const bool reached = SearchExcavationTarget(result.end, target, work, cleared, model_id, limits, result, starter.required && result.route.empty());
		for (size_t i = before; i < result.route.size(); ++i) {
			std::vector<TileIndex> fresh;
			for (TileIndex tile : result.route[i].swept_tiles) {
				if (!cleared.contains(tile)) fresh.push_back(tile);
			}
			const auto quote = QuoteExcavationCells(fresh, model_id);
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
