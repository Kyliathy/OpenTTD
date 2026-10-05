/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation_pass.cpp Complete-prefix backing, independent spoil batches and frozen job input. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 14 */

#include "stdafx.h"
#include "excavation.h"
#include "tile_map.h"

#include "safeguards.h"

/**
 * Fold a static actual field into the pass proof, independent of pointer/padding/endian representation.
 * @param hash Current proof, updated by the explicit field.
 * @param value Actual unsigned field/reference bits.
 * @param width Explicit encoded byte count.
 */
static void HashPassValue(uint64_t &hash, uint64_t value, uint width)
{
	for (uint i = width; i != 0; --i) {
		hash ^= (value >> ((i - 1) * 8)) & 255;
		hash *= 1099511628211ULL;
	}
}

/**
 * Hash one canonical geometry vector with its exact cardinality.
 * @param hash Actual static proof accumulator.
 * @param tiles Actual saved/proposed geometry, never a cached owner lookup.
 */
static void HashPassTiles(uint64_t &hash, std::span<const TileIndex> tiles)
{
	HashPassValue(hash, tiles.size(), 4);
	for (TileIndex tile : tiles) HashPassValue(hash, tile.base(), 4);
}

/**
 * Compute a stable static proof that cannot be substituted for another authorized route/capacity plan.
 * @param job Actual canonical geometry.
 * @param terms Actual static return/capacity limits.
 * @return Portable route/input fingerprint, excluding progress and the fingerprint itself.
 */
uint64_t ComputeExcavationPassFingerprint(const ExcavationJob &job, const ExcavationPassTerms &terms)
{
	uint64_t hash = 14695981039346656037ULL;
	HashPassValue(hash, 0x504153535f563031ULL, 8);
	HashPassValue(hash, job.model_id, 2); HashPassValue(hash, job.commissioning_anchor.base(), 4);
	HashPassTiles(hash, job.requested_tiles); HashPassTiles(hash, job.authorized_tiles);
	HashPassValue(hash, job.route.size(), 4);
	// Accepted pose/sweep/support order defines the actual return domain; mutable cursor/receipts cannot create another pass.
	for (const auto &cut : job.route) {
		HashPassValue(hash, cut.cutter_position.base(), 4); HashPassValue(hash, to_underlying(cut.heading), 1);
		HashPassTiles(hash, cut.swept_tiles); HashPassTiles(hash, cut.support_tiles);
	}
	HashPassValue(hash, terms.earth_capacity_t, 8); HashPassValue(hash, terms.rock_capacity_t, 8);
	HashPassTiles(hash, terms.exchange_path); HashPassValue(hash, terms.exchange_after.size(), 2);
	for (uint16_t prefix : terms.exchange_after) HashPassValue(hash, prefix, 2);
	return hash;
}

/**
 * Validate static complete-prefix return and spoil batching before the first reservation.
 * @param job Actual proposed canonical geometry.
 * @param terms Explicit proposed capacities and supported exchange path.
 * @return Complete frozen terms or unchanged refusal.
 */
ExcavationPassResult PrepareExcavationPass(const ExcavationJob &job, const ExcavationPassTerms &terms)
{
	const auto &geology = GetGeologyWorldState();
	const auto geometry = ValidateExcavationJobGeometry(geology, job);
	if (geometry.error != ExcavationError::None) return {geometry.error, geometry.failure_tile};
	ExcavationPassResult result;
	result.terms = terms;
	result.terms.exchange_after.clear();
	if (result.terms.exchange_path.empty()) result.terms.exchange_path = {job.commissioning_anchor};
	// The actual return starts at this commissioning anchor; a faraway station with matching coordinates is not implied connectivity.
	if (result.terms.exchange_path.size() > 64 || result.terms.exchange_path.front() != job.commissioning_anchor) return {ExcavationError::InvalidRoute, job.commissioning_anchor};
	std::set<TileIndex> cleared;
	const auto layer = GetTileLayer(job.commissioning_anchor);
	// Loaded withdrawal starts with the whole real cutter footprint open; a single open centre cannot substitute for a commissioning room.
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			const auto tile = TileAddWrap(job.commissioning_anchor, x, y);
			if (geology.terrain.Get(tile) != ExcavationState::Excavated) return {ExcavationError::MissingSupport, tile};
		}
	}
	// Initial support is actual open ground; passing later cuts cannot retroactively legalize a solid initial exchange path.
	for (size_t i = 0; i < result.terms.exchange_path.size(); ++i) {
		const auto tile = result.terms.exchange_path[i];
		if (!IsValidTile(tile)) return {ExcavationError::InvalidTile, tile};
		if (GetTileLayer(tile) != layer) return {ExcavationError::MixedLayers, tile};
		if (i != 0 && DistanceMax(tile, result.terms.exchange_path[i - 1]) != 1) return {ExcavationError::InvalidRoute, tile}; // Unsupported gaps/repeated cells cannot be recovered by backing or cross-layer coordinate projection.
		if (geology.terrain.Get(tile) != ExcavationState::Excavated) return {ExcavationError::MissingSupport, tile};
		const auto claim = geology.job_claims.find(tile);
		if (claim != geology.job_claims.end() && claim->second != job.id) return {ExcavationError::Conflict, tile}; // Another accepted job cannot supply this loaded backing approach.
		const auto type = GetTileType(tile);
		if (type != TileType::Clear && type != TileType::Railway && type != TileType::LayerPortal) return {ExcavationError::Conflict, tile}; // Known road/building obstructions must refuse before the reservation, rather than waiting for the first cut.
		if (type != TileType::Clear && GetTileOwner(tile) != job.company) return {ExcavationError::InvalidOwner, tile}; // Owned support infrastructure cannot silently become a different company's excavation logistics.
		cleared.insert(tile);
	}
	// Actual existing cavities can support later complete prefixes, but do not contribute a second spoil yield.
	for (TileIndex tile : job.authorized_tiles) {
		if (geology.terrain.Get(tile) == ExcavationState::Excavated) cleared.insert(tile);
	}
	uint64_t earth = 0, rock = 0;
	TileIndex previous = job.commissioning_anchor;
	// Every prefix opens its whole sweep before backing; independently accumulate only its newly reserved ground.
	for (size_t i = 0; i < job.route.size(); ++i) {
		const auto &cut = job.route[i];
		for (TileIndex tile : cut.support_tiles) {
			if (!cleared.contains(tile)) return {ExcavationError::MissingSupport, tile};
		}
		if (!cleared.contains(previous)) return {ExcavationError::MissingSupport, previous};
		std::vector<TileIndex> fresh;
		for (TileIndex tile : cut.swept_tiles) {
			if (!cleared.contains(tile)) fresh.push_back(tile);
		}
		const auto quote = QuoteExcavationCells(fresh, job.model_id);
		if (quote.error != ExcavationError::None) return {quote.error, quote.failure_tile};
		// A first whole cut that cannot fit either independent wagon group has no recoverable loaded exit and must refuse outright.
		if (quote.earth_t > terms.earth_capacity_t || quote.rock_t > terms.rock_capacity_t) return {ExcavationError::InsufficientCapacity, cut.cutter_position};
		if (earth > terms.earth_capacity_t - quote.earth_t || rock > terms.rock_capacity_t - quote.rock_t) {
			result.terms.exchange_after.push_back(static_cast<uint16_t>(i));
			earth = 0; rock = 0;
		}
		earth += quote.earth_t; rock += quote.rock_t;
		cleared.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
		// The reverse centre path traverses only this opened prefix, never a new backward cut or a stationary heading change.
		for (size_t prefix = i + 1; prefix != 0; --prefix) {
			if (!cleared.contains(job.route[prefix - 1].cutter_position)) return {ExcavationError::MissingSupport, job.route[prefix - 1].cutter_position};
		}
		previous = cut.cutter_position;
	}
	result.terms.exchange_after.push_back(static_cast<uint16_t>(job.route.size()));
	result.terms.fingerprint = ComputeExcavationPassFingerprint(job, result.terms);
	return result;
}

/**
 * Derive the supported reverse route from one frozen actual forward prefix.
 * @param job Actual saved canonical geometry.
 * @param completed_cuts Prefix at which loaded actors return.
 * @return Centre path only; it never creates new work or rotates the cutter heading.
 */
std::vector<TileIndex> GetExcavationRetreatPath(const ExcavationJob &job, uint32_t completed_cuts)
{
	if (!job.pass_terms || completed_cuts > job.route.size() || ValidateExcavationPassTerms(job, *job.pass_terms).error != ExcavationError::None) return {}; // Only this actual bounded frozen input may derive the return domain.
	std::vector<TileIndex> path;
	// Walk the already opened forward history in reverse, then follow the explicitly supported exchange/pocket approach.
	for (uint32_t i = completed_cuts; i != 0; --i) path.push_back(job.route[i - 1].cutter_position);
	path.insert(path.end(), job.pass_terms->exchange_path.begin(), job.pass_terms->exchange_path.end());
	return path;
}

/**
 * Validate immutable input without charging completed excavation a second time.
 * @param job Actual canonical geometry and saved progress.
 * @param terms Exact saved return/capacity input.
 * @return Complete structural validation or unchanged refusal.
 */
ExcavationFootprintResult ValidateExcavationPassTerms(const ExcavationJob &job, const ExcavationPassTerms &terms)
{
	// A proof requires a bounded actual exchange path and complete strictly increasing return prefixes.
	if (job.route.empty() || job.route.size() > MAX_EXCAVATION_ROUTE || terms.exchange_path.empty() || terms.exchange_path.size() > 64 || terms.exchange_path.front() != job.commissioning_anchor || terms.exchange_after.empty() || terms.exchange_after.size() > job.route.size() || terms.exchange_after.back() != job.route.size() || terms.fingerprint != ComputeExcavationPassFingerprint(job, terms)) return {ExcavationError::InvalidJob};
	uint16_t previous = 0;
	// Strictly increasing whole-cut return counts preserve accepted load-exchange boundaries through saves.
	for (uint16_t prefix : terms.exchange_after) {
		if (prefix <= previous || prefix > job.route.size()) return {ExcavationError::InvalidRoute}; // Empty/repeated passes cannot conceal a capacity exchange.
		previous = prefix;
	}
	// References must resolve to adjacent cells on this actual rock plane, independently of mutable infrastructure.
	for (size_t i = 0; i < terms.exchange_path.size(); ++i) {
		const auto tile = terms.exchange_path[i];
		if (!IsValidTile(tile)) return {ExcavationError::InvalidTile, tile};
		if (GetTileLayer(tile) != GetTileLayer(job.commissioning_anchor)) return {ExcavationError::MixedLayers, tile};
		if (i != 0 && DistanceMax(tile, terms.exchange_path[i - 1]) != 1) return {ExcavationError::InvalidRoute, tile}; // Duplicated/gapped approach cells cannot create supported backing.
	}
	return {};
}

/**
 * Check safe support replacement after both actual actors have withdrawn.
 * @param job Actual accepted route/return terms.
 * @param removed_support Exact support cells proposed for removal.
 * @param alternative_backing Explicit retained replacement path.
 * @param participant Actual cutter/hauler withdrawal owner.
 * @return Stable no-mutation refusal or full replacement permission.
 */
ExcavationFootprintResult CheckExcavationSupportReplacement(const ExcavationJob &job, std::span<const TileIndex> removed_support, std::span<const TileIndex> alternative_backing, const ExcavationWithdrawalParticipant &participant)
{
	if (!job.pass_terms || job.cursor > job.route.size()) return {ExcavationError::InvalidJob}; // Malformed progress cannot address an absent cutter position.
	const auto validation = ValidateExcavationPassTerms(job, *job.pass_terms);
	if (validation.error != ExcavationError::None) return validation;
	if (!participant.HasWithdrawn(job, removed_support)) return {ExcavationError::Conflict};
	const auto current = job.cursor == 0 ? job.commissioning_anchor : job.route[job.cursor - 1].cutter_position;
	// A declared alternative must actually join the current head to this saved exchange; unrelated open chambers are not a retreat route.
	if (alternative_backing.empty() || alternative_backing.size() > MAX_EXCAVATION_JOB_TILES || alternative_backing.front() != current || alternative_backing.back() != job.pass_terms->exchange_path.back()) return {ExcavationError::InvalidRoute, current};
	const auto &geology = GetGeologyWorldState();
	// Every replacement cell must remain connected open ground outside the exact support being removed.
	for (size_t i = 0; i < alternative_backing.size(); ++i) {
		const auto tile = alternative_backing[i];
		if (!IsValidTile(tile)) return {ExcavationError::InvalidTile, tile};
		if (GetTileLayer(tile) != GetTileLayer(current)) return {ExcavationError::MixedLayers, tile};
		if (std::ranges::find(removed_support, tile) != removed_support.end()) return {ExcavationError::MissingSupport, tile};
		if (geology.terrain.Get(tile) != ExcavationState::Excavated) return {ExcavationError::MissingSupport, tile};
		if (i != 0 && DistanceMax(tile, alternative_backing[i - 1]) > 1) return {ExcavationError::InvalidRoute, tile}; // Disconnected replacement support cannot recover a cutter/hauler after tube-track withdrawal.
	}
	return {};
}
