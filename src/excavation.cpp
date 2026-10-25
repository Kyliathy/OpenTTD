/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation.cpp Packed rock authority independent of tile reconstruction. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md */

#include "stdafx.h"
#include "excavation.h"
#include "openttd.h"
#include "excavation_state.h"
#include "tile_map.h"
#include "rail_map.h"
#include "multilayer_content.h"
#include "multilayer_state.h"
#include "debug.h"
#include "company_base.h"
#include "company_func.h"
#include "vehicle_base.h"

#include "safeguards.h"

static GeologyWorldState _geology_world; ///< Sole accepted rock/deposit owner; seed metadata remains with multilayer_state.cpp.
static std::optional<GeologyWorldState> _pending_geology; ///< Validated unaccepted load candidate, not another live reserve owner.

/** @return Accepted geological authority, independently of camera selection. */
const GeologyWorldState &GetGeologyWorldState()
{
	return _geology_world;
}

/** @copydoc ClaimLayerMineDeposit */
GeologyError ClaimLayerMineDeposit(GeologyDepositID deposit, CompanyID company, uint64_t expected_remaining)
{
	const auto *record = _geology_world.deposits.Get(deposit);
	if (record == nullptr) return GeologyError::InvalidID;
	// Exact unclaimed reserve admission prevents both stale quotes and a second mine by the same claimant.
	if (!Company::IsValidID(company) || record->claimant != CompanyID::Invalid()) return GeologyError::ClaimConflict;
	if (record->remaining_reserve != expected_remaining) return GeologyError::InvalidReserve;
	return _geology_world.deposits.Claim(deposit, company);
}

/** @copydoc ReleaseLayerMineDeposit */
GeologyError ReleaseLayerMineDeposit(GeologyDepositID deposit, CompanyID company)
{
	return _geology_world.deposits.ReleaseClaim(deposit, company);
}

/** @copydoc TransferLayerMineDeposit */
GeologyError TransferLayerMineDeposit(GeologyDepositID deposit, CompanyID old_company, CompanyID new_company)
{
	// Company removal releases the geological claim; a takeover must name an existing actual company.
	if (new_company != CompanyID::Invalid() && !Company::IsValidID(new_company)) return GeologyError::ClaimConflict;
	return _geology_world.deposits.TransferMineClaim(deposit, old_company, new_company);
}

/** @copydoc ConsumeLayerMineReserve */
GeologyError ConsumeLayerMineReserve(GeologyDepositID deposit, CompanyID company, uint64_t amount, uint64_t expected_remaining)
{
	const auto *record = _geology_world.deposits.Get(deposit);
	if (record == nullptr) return GeologyError::InvalidID;
	// A failed participant or stale output proposal cannot debit a different balance or create a zero-progress receipt.
	if (amount == 0 || record->remaining_reserve != expected_remaining) return GeologyError::InvalidReserve;
	if (!Company::IsValidID(company)) return GeologyError::ClaimConflict;
	return _geology_world.deposits.ConsumeReserve(deposit, company, amount);
}

/** Clear geological authority at world initialization; old absence never becomes newly generated rock. */
void ResetGeologyWorldState()
{
	_geology_world = {};
	_pending_geology.reset();
	ResetGeologyLoadStaging();
}

/**
 * Stage complete geological transport without publishing gameplay authority.
 * @param state Candidate with actual descriptor/reference validation already complete.
 */
void StageLoadedGeologyState(GeologyWorldState state)
{
	_pending_geology = std::move(state);
}

/** @return Whether the required feature has a fully validated load candidate. */
bool HasStagedGeologyState()
{
	return _pending_geology.has_value();
}

/** @return Read-only actual validated candidate; consumers cannot mutate it or introduce another geological owner. */
const GeologyWorldState *GetStagedGeologyWorldState()
{
	return _pending_geology ? &*_pending_geology : nullptr;
}

/** Accept the validated geological owner after shared profile acceptance; legacy absence retains empty terrain. */
void AcceptLoadedGeologyState()
{
	if (_pending_geology) {
		_geology_world = std::move(*_pending_geology);
		_pending_geology.reset();
	}
	Debug(Facility::Misc, Severity::Info, "MLVS_GEOLOGY_LOADED generator={} deposits={} packed_bytes={} vents={}", _geology_world.generator_version, _geology_world.deposits.Records().size(), _geology_world.terrain.StorageBytes(), _geology_world.vents.size());
	// Log actual accepted geometry/progress after profile and reference acceptance; rendering/route caches are excluded.
	for (const auto &job : _geology_world.jobs) {
		Debug(Facility::Misc, Severity::Info, "MLVS_EXCAVATION_JOB id={} state={} cursor={} partial={} revision={}", job.id.base(), to_underlying(job.state), job.cursor, job.partial_work_ticks, job.revision);
	}
}

/**
 * Publish a complete candidate only after deterministic mandatory generation succeeds.
 * @param world_seed Synchronized new-world generation input.
 * @param hardness_version Current regional rule, or zero for explicitly historical generation evidence.
 * @return Refusal or complete accepted geology; Classic remains allocation-free.
 */
GeologyError GenerateWorldGeology(uint32_t world_seed, uint16_t hardness_version)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return GeologyError::None;
	const uint64_t seed = DeriveGeologySeed(world_seed);
	auto generated = PrepareGeologyGeneration(seed, hardness_version);
	if (generated.error != GeologyError::None) return generated.error;
	_geology_world = std::move(generated.state);
	SetMultiLayerGeologySeed(seed);
	Debug(Facility::Misc, Severity::Info, "MLVS_GEOLOGY generator={} seed={} deposits={} packed_bytes={} vents={}", _geology_world.generator_version, seed, _geology_world.deposits.Records().size(), _geology_world.terrain.StorageBytes(), _geology_world.vents.size());
	return GeologyError::None;
}

/**
 * Apply explicit editor painting without making the override an ordinary gameplay permission.
 * @param footprint Atomic ordered rock-layer floor.
 * @param excavated Desired solid/open state.
 * @param execute Apply only after every cell and the saved policy have passed.
 * @return Stable unchanged refusal or whole-footprint acceptance.
 */
ExcavationFootprintResult PaintExcavationTerrain(std::span<const TileIndex> footprint, bool excavated, bool execute)
{
	const auto &policy = GetCurrentMultiLayerProfileSelection();
	// The saved override alone cannot create free excavation in normal games, scripts or server commands.
	if (_game_mode != GameMode::Editor || !policy.editor_override || policy.id != MultiLayerGameplayProfileID::MultilayerIndustry || _geology_world.generator_version != 1) return {ExcavationError::InvalidPolicy, INVALID_TILE};
	if (footprint.empty()) return {ExcavationError::InvalidTile, INVALID_TILE};
	MapLayer layer = MapLayer::Invalid;
	// Preflight every cell before touching the first one; painting cannot bury structures or steal active work.
	for (TileIndex tile : footprint) {
		const auto terrain = _geology_world.terrain.Get(tile);
		if (!terrain) return {ExcavationError::InvalidTile, tile};
		const auto current = GetTileLayer(tile);
		if (layer == MapLayer::Invalid) layer = current;
		if (current != layer) return {ExcavationError::MixedLayers, tile};
		if (*terrain == ExcavationState::Excavating || _geology_world.job_claims.contains(tile)) return {ExcavationError::ActiveCut, tile}; // Existing open overlap remains an active job claim and cannot be repainted under its cutter.
		if (GetTileType(tile) != TileType::Clear) return {ExcavationError::Conflict, tile};
	}
	if (!execute) return {};
	const auto desired = excavated ? ExcavationState::Excavated : ExcavationState::Solid;
	// These addresses/states were checked together and Set cannot allocate or fail during this complete commit.
	for (TileIndex tile : footprint) {
		const bool accepted = _geology_world.terrain.Set(tile, desired);
		assert(accepted);
	}
	return {};
}

/**
 * Compare the saved canonical job identity to an exact lookup key.
 * @param job Actual record being searched.
 * @param key Exact typed identity.
 * @return Whether the record precedes the requested identity.
 */
static bool ExcavationJobBeforeID(const ExcavationJob &job, ExcavationJobID key)
{
	return job.id < key;
}

/**
 * Find the mutable producer-owned record without exporting mutable progress.
 * @param id Exact stable identity.
 * @return Actual record or nullptr.
 */
static ExcavationJob *MutableExcavationJob(ExcavationJobID id)
{
	const auto it = std::lower_bound(_geology_world.jobs.begin(), _geology_world.jobs.end(), id, ExcavationJobBeforeID);
	if (it == _geology_world.jobs.end()) return nullptr;
	return it->id == id ? &*it : nullptr;
}

/** @copydoc AppendExcavationJob */
ExcavationJobResult AppendExcavationJob(ExcavationJobID id, uint64_t expected_revision, const ExcavationOutline &outline,
	std::span<const ExcavationCutGeometry> route, bool execute)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return {ExcavationError::InvalidPolicy};
	const auto *job = MutableExcavationJob(id);
	// Only the actual company can extend its live reservations; a stale quote
	// or orphaned actor cannot acquire more rock by supplying an old job ID.
	if (job == nullptr || !Company::IsValidID(job->company) || job->company != _current_company) return {ExcavationError::InvalidOwner};
	if (job->revision != expected_revision || job->revision == UINT64_MAX) return {ExcavationError::StaleParticipant};
	// Closed history has already settled its claims. Reusing its machine is a
	// separate controller/portal operation, never an implicit history reset.
	if (job->state == ExcavationJobState::Completed || job->state == ExcavationJobState::Cancelled) return {ExcavationError::AlreadyCompleted};
	if (!ValidateExcavationJobReferences(*job)) return {ExcavationError::InvalidOwner};
	const auto valid = ValidateExcavationJobGeometry(_geology_world, *job);
	if (valid.error != ExcavationError::None) return {valid.error, id, valid.failure_tile};
	// Saved backing/capacity proof and bounded real additions are required;
	// an empty or oversized append cannot merely change an existing footprint.
	if (!job->pass_terms || route.empty() || route.size() > MAX_EXCAVATION_ROUTE - job->route.size()) return {ExcavationError::InvalidRoute};
	if (outline.error != ExcavationError::None) return {outline.error, id, outline.failure_tile};
	if (outline.layer != GetTileLayer(job->commissioning_anchor)) return {ExcavationError::MixedLayers};
	const auto requested = NormalizeExcavationOutline(outline.layer, outline.requested_tiles);
	if (requested.error != ExcavationError::None) return {requested.error, id, requested.failure_tile};
	const auto work = NormalizeExcavationOutline(outline.layer, outline.authorized_tiles);
	if (work.error != ExcavationError::None) return {work.error, id, work.failure_tile};
	ExcavationJob next = *job;
	next.requested_tiles.insert(next.requested_tiles.end(), requested.requested_tiles.begin(), requested.requested_tiles.end());
	next.authorized_tiles.insert(next.authorized_tiles.end(), work.requested_tiles.begin(), work.requested_tiles.end());
	// Canonical unions retain every old accepted cell; extension cannot quietly
	// revoke earlier work, support or the player's requested final floor.
	for (auto *tiles : {&next.requested_tiles, &next.authorized_tiles}) {
		std::ranges::sort(*tiles);
		tiles->erase(std::unique(tiles->begin(), tiles->end()), tiles->end());
	}
	next.route.insert(next.route.end(), route.begin(), route.end());
	// Validate the changed geometry before deriving its new static proof. The
	// live job retains its old proof throughout preparation and every refusal.
	next.pass_terms.reset();
	const auto geometry = ValidateExcavationJobGeometry(_geology_world, next);
	if (geometry.error != ExcavationError::None) return {geometry.error, id, geometry.failure_tile};
	auto terms = *job->pass_terms;
	std::set<TileIndex> cleared, footprint;
	// Existing accepted cuts all precede the addition. Forecast their floor
	// without replaying receipts, advancing the live cursor or clearing rock.
	for (const auto &cut : job->route) cleared.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
	for (TileIndex tile : next.authorized_tiles) {
		if (_geology_world.terrain.Get(tile) == ExcavationState::Excavated) cleared.insert(tile);
	}
	uint64_t earth = 0, rock = 0;
	// The previous final return stays in terms.exchange_after. New work starts
	// after that physical empty-wagon exchange, independent of current cargo.
	for (size_t index = 0; index < route.size(); ++index) {
		const auto &cut = route[index];
		for (TileIndex tile : cut.support_tiles) {
			if (!cleared.contains(tile)) return {ExcavationError::MissingSupport, id, tile};
		}
		std::vector<TileIndex> fresh;
		// Every appended sweep retains normal structure, seam and claim rules,
		// even where the floor is already open and would produce zero spoil.
		for (TileIndex tile : cut.swept_tiles) {
			const auto claim = _geology_world.job_claims.find(tile);
			if (claim != _geology_world.job_claims.end() && claim->second != id) return {ExcavationError::Conflict, id, tile};
			const auto terrain = _geology_world.terrain.Get(tile);
			if (!terrain) return {ExcavationError::InvalidTile, id, tile};
			// An orphaned active bit cannot be adopted as this job's permission.
			if (*terrain == ExcavationState::Excavating && claim == _geology_world.job_claims.end()) return {ExcavationError::ActiveCut, id, tile};
			const auto type = GetTileType(tile);
			if (type != TileType::Clear && type != TileType::Railway && type != TileType::LayerPortal) return {ExcavationError::Conflict, id, tile};
			if (type == TileType::Railway && !IsPlainRailTile(tile)) return {ExcavationError::Conflict, id, tile}; // A depot is a building, not free swept clearance.
			if (type != TileType::Clear && (GetTileOwner(tile) != job->company || !cleared.contains(tile))) return {ExcavationError::InvalidOwner, id, tile}; // Only owned open or already accepted support may precede the new cuts.
			const auto *deposit = _geology_world.deposits.Get(_geology_world.deposits.Find(tile));
			if (deposit != nullptr && deposit->claimant != CompanyID::Invalid() && deposit->claimant != job->company) return {ExcavationError::Conflict, id, tile}; // Another company's seam never becomes this project's working space.
			footprint.insert(tile);
			if (!cleared.contains(tile)) fresh.push_back(tile);
		}
		const auto quote = QuoteExcavationCells(fresh, job->model_id);
		if (quote.error != ExcavationError::None) return {quote.error, id, quote.failure_tile};
		if (quote.earth_t > terms.earth_capacity_t || quote.rock_t > terms.rock_capacity_t) return {ExcavationError::InsufficientCapacity, id, cut.cutter_position}; // One complete cut must fit each independent wagon group.
		if (earth > terms.earth_capacity_t - quote.earth_t || rock > terms.rock_capacity_t - quote.rock_t) {
			// Keep accepted old exchange points and add only necessary future
			// returns; neither cargo group may borrow the other's free space.
			terms.exchange_after.push_back(static_cast<uint16_t>(job->route.size() + index));
			earth = 0;
			rock = 0;
		}
		earth += quote.earth_t;
		rock += quote.rock_t;
		cleared.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
	}
	terms.exchange_after.push_back(static_cast<uint16_t>(next.route.size()));
	terms.fingerprint = ComputeExcavationPassFingerprint(next, terms);
	next.pass_terms = std::move(terms);
	const auto complete = ValidateExcavationJobGeometry(_geology_world, next);
	if (complete.error != ExcavationError::None) return {complete.error, id, complete.failure_tile};
	size_t cells = 0, added_claims = 0;
	// Existing completed history and this append share the same saved-memory
	// budget as fresh jobs; extension must not bypass those world-wide limits.
	for (const auto &existing : _geology_world.jobs) {
		for (const auto &cut : existing.route) cells += cut.swept_tiles.size() + cut.support_tiles.size();
	}
	for (const auto &cut : route) cells += cut.swept_tiles.size() + cut.support_tiles.size();
	for (TileIndex tile : footprint) added_claims += !_geology_world.job_claims.contains(tile);
	if (cells > 262144 || added_claims + _geology_world.job_claims.size() > MAX_EXCAVATION_CLAIMS) return {ExcavationError::Limit};
	if (!execute) return {ExcavationError::None, id};
	++next.revision;
	auto jobs = _geology_world.jobs;
	auto claims = _geology_world.job_claims;
	const auto target = std::lower_bound(jobs.begin(), jobs.end(), id, ExcavationJobBeforeID);
	*target = std::move(next);
	// Complete every allocation before publishing either owner container or
	// any terrain bits, preserving an atomic refusal on preparation failure.
	for (TileIndex tile : footprint) claims.emplace(tile, id);
	_geology_world.jobs.swap(jobs);
	_geology_world.job_claims.swap(claims);
	for (TileIndex tile : footprint) {
		if (_geology_world.terrain.Get(tile) != ExcavationState::Solid) continue;
		const bool accepted = _geology_world.terrain.Set(tile, ExcavationState::Excavating);
		assert(accepted);
	}
	return {ExcavationError::None, id};
}

/** @copydoc ReplaceExcavationJobRemainder */
ExcavationJobResult ReplaceExcavationJobRemainder(ExcavationJobID id, uint32_t expected_cursor, const ExcavationOutline &outline,
	std::span<const ExcavationCutGeometry> route, ExcavationLoadStart load, bool execute)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return {ExcavationError::InvalidPolicy};
	const auto *job = MutableExcavationJob(id);
	// Only the actual company can replace its own live work; a copied or stale job ID cannot redirect another project.
	if (job == nullptr || !Company::IsValidID(job->company) || job->company != _current_company) return {ExcavationError::InvalidOwner};
	// Closed history has settled its claims; new work there is a new project, never a reopened job.
	if (job->state == ExcavationJobState::Completed || job->state == ExcavationJobState::Cancelled) return {ExcavationError::AlreadyCompleted};
	// With no completed cut the cutter is still in its housing, whose first cut belongs to the commission. A live job
	// always has a next cut; a cursor at the end is malformed.
	if (job->cursor == 0 || job->cursor >= job->route.size()) return {ExcavationError::InvalidRoute};
	// The quote binds the completed-cut count: a cut completed since then makes it stale.
	if (job->cursor != expected_cursor || job->revision == UINT64_MAX) return {ExcavationError::StaleParticipant};
	if (!ValidateExcavationJobReferences(*job)) return {ExcavationError::InvalidOwner};
	const auto valid = ValidateExcavationJobGeometry(_geology_world, *job);
	if (valid.error != ExcavationError::None) return {valid.error, id, valid.failure_tile};
	const uint32_t cursor = job->cursor;
	// Saved backing/capacity proof and a bounded non-empty new remainder are required.
	if (!job->pass_terms || route.empty() || route.size() > MAX_EXCAVATION_ROUTE - cursor) return {ExcavationError::InvalidRoute};
	if (outline.error != ExcavationError::None) return {outline.error, id, outline.failure_tile};
	if (outline.layer != GetTileLayer(job->commissioning_anchor)) return {ExcavationError::MixedLayers};
	const auto requested = NormalizeExcavationOutline(outline.layer, outline.requested_tiles);
	if (requested.error != ExcavationError::None) return {requested.error, id, requested.failure_tile};
	const auto work = NormalizeExcavationOutline(outline.layer, outline.authorized_tiles);
	if (work.error != ExcavationError::None) return {work.error, id, work.failure_tile};

	ExcavationJob next = *job;
	next.route.resize(cursor);
	// Delivered floor stays requested. Requested cells still unopened belonged to the dropped remainder and leave with it.
	/** @param tile Original requested cell. @return Whether its unfinished floor belonged only to the dropped plan. */
	std::erase_if(next.requested_tiles, [](TileIndex tile) { return _geology_world.terrain.Get(tile) != ExcavationState::Excavated; });
	next.requested_tiles.insert(next.requested_tiles.end(), requested.requested_tiles.begin(), requested.requested_tiles.end());
	// The completed cuts' sweeps and the delivered floor stay authorized, plus the new working space. Authorization that
	// only the dropped cuts used is withdrawn: nothing was dug there.
	next.authorized_tiles = next.requested_tiles;
	for (const auto &cut : next.route) next.authorized_tiles.insert(next.authorized_tiles.end(), cut.swept_tiles.begin(), cut.swept_tiles.end());
	next.authorized_tiles.insert(next.authorized_tiles.end(), work.requested_tiles.begin(), work.requested_tiles.end());
	for (auto *tiles : {&next.requested_tiles, &next.authorized_tiles}) {
		std::ranges::sort(*tiles);
		tiles->erase(std::unique(tiles->begin(), tiles->end()), tiles->end());
	}
	next.route.insert(next.route.end(), route.begin(), route.end());
	// Work done on the cut in progress stays only if the new remainder starts with that very cut; otherwise the face
	// work is abandoned, and the quote states it.
	if (route.front() != job->route[cursor]) next.partial_work_ticks = 0;
	// Validate the changed geometry before deriving its new static proof. The live job keeps its old proof throughout.
	next.pass_terms.reset();
	const auto geometry = ValidateExcavationJobGeometry(_geology_world, next);
	if (geometry.error != ExcavationError::None) return {geometry.error, id, geometry.failure_tile};

	auto terms = *job->pass_terms;
	// Exchanges up to the cursor were planned for the kept cuts; the dropped cuts' later exchanges leave with them.
	/** @param prefix Original exchange boundary. @return Whether it belongs to discarded future cuts. */
	std::erase_if(terms.exchange_after, [cursor](uint16_t prefix) { return prefix > cursor; });
	std::set<TileIndex> cleared, footprint;
	for (size_t index = 0; index < cursor; ++index) cleared.insert(job->route[index].swept_tiles.begin(), job->route[index].swept_tiles.end());
	for (TileIndex tile : next.authorized_tiles) {
		if (_geology_world.terrain.Get(tile) == ExcavationState::Excavated) cleared.insert(tile);
	}
	// With an exchange due at the cursor the wagons leave empty before the first new cut. Otherwise the spoil aboard
	// now fills them first, so a nearly full load makes the exchange due at once.
	const bool exchanging = std::ranges::binary_search(terms.exchange_after, static_cast<uint16_t>(cursor));
	uint64_t earth = exchanging ? 0 : load.earth_t, rock = exchanging ? 0 : load.rock_t;
	// Each new cut keeps the append rules: open support, no other job's claim, no structure or foreign seam, and one
	// whole cut must fit each wagon group. Its sweep becomes this job's footprint.
	for (size_t index = 0; index < route.size(); ++index) {
		const auto &cut = route[index];
		for (TileIndex tile : cut.support_tiles) {
			if (!cleared.contains(tile)) return {ExcavationError::MissingSupport, id, tile};
		}
		std::vector<TileIndex> fresh;
		// Every swept cell, even open floor that yields no spoil, keeps the structure, seam and claim rules.
		for (TileIndex tile : cut.swept_tiles) {
			const auto claim = _geology_world.job_claims.find(tile);
			if (claim != _geology_world.job_claims.end() && claim->second != id) return {ExcavationError::Conflict, id, tile};
			const auto terrain = _geology_world.terrain.Get(tile);
			if (!terrain) return {ExcavationError::InvalidTile, id, tile};
			// An orphaned active bit cannot be adopted as this job's permission.
			if (*terrain == ExcavationState::Excavating && claim == _geology_world.job_claims.end()) return {ExcavationError::ActiveCut, id, tile};
			const auto type = GetTileType(tile);
			if (type != TileType::Clear && type != TileType::Railway && type != TileType::LayerPortal) return {ExcavationError::Conflict, id, tile};
			if (type == TileType::Railway && !IsPlainRailTile(tile)) return {ExcavationError::Conflict, id, tile}; // A depot is a building, not free swept clearance.
			if (type != TileType::Clear && (GetTileOwner(tile) != job->company || !cleared.contains(tile))) return {ExcavationError::InvalidOwner, id, tile}; // Only owned open support may precede the new cuts.
			const auto *deposit = _geology_world.deposits.Get(_geology_world.deposits.Find(tile));
			if (deposit != nullptr && deposit->claimant != CompanyID::Invalid() && deposit->claimant != job->company) return {ExcavationError::Conflict, id, tile}; // Another company's seam never becomes this project's working space.
			footprint.insert(tile);
			if (!cleared.contains(tile)) fresh.push_back(tile);
		}
		// The dropped cuts' unopened cells are still rock: reusing one charges it again.
		const auto quote = QuoteExcavationCells(fresh, job->model_id, id);
		if (quote.error != ExcavationError::None) return {quote.error, id, quote.failure_tile};
		if (quote.earth_t > terms.earth_capacity_t || quote.rock_t > terms.rock_capacity_t) return {ExcavationError::InsufficientCapacity, id, cut.cutter_position};
		if (earth > terms.earth_capacity_t - quote.earth_t || rock > terms.rock_capacity_t - quote.rock_t) {
			// Neither cargo group may borrow the other's free space: the exchange comes before this cut.
			terms.exchange_after.push_back(static_cast<uint16_t>(cursor + index));
			earth = 0;
			rock = 0;
		}
		earth += quote.earth_t;
		rock += quote.rock_t;
		cleared.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
	}
	terms.exchange_after.push_back(static_cast<uint16_t>(next.route.size()));
	terms.fingerprint = ComputeExcavationPassFingerprint(next, terms);
	next.pass_terms = std::move(terms);
	const auto complete = ValidateExcavationJobGeometry(_geology_world, next);
	if (complete.error != ExcavationError::None) return {complete.error, id, complete.failure_tile};
	size_t cells = 0, old_claims = 0;
	// The replaced job shares the world-wide saved-geometry and claim budgets with every other job.
	for (const auto &existing : _geology_world.jobs) {
		for (const auto &cut : existing.id == id ? next.route : existing.route) cells += cut.swept_tiles.size() + cut.support_tiles.size();
	}
	for (const auto &[tile, owner] : _geology_world.job_claims) old_claims += owner == id;
	if (cells > 262144 || _geology_world.job_claims.size() - old_claims + footprint.size() > MAX_EXCAVATION_CLAIMS) return {ExcavationError::Limit};
	if (!execute) return {ExcavationError::None, id};

	++next.revision;
	auto jobs = _geology_world.jobs;
	auto claims = _geology_world.job_claims;
	std::vector<TileIndex> released;
	// This job's claims are exactly its unfinished cuts' sweeps: those outside the new footprint are released.
	for (auto it = claims.begin(); it != claims.end();) {
		if (it->second == id && !footprint.contains(it->first)) {
			released.push_back(it->first);
			it = claims.erase(it);
		} else {
			++it;
		}
	}
	for (TileIndex tile : footprint) claims.emplace(tile, id);
	const auto target = std::lower_bound(jobs.begin(), jobs.end(), id, ExcavationJobBeforeID);
	*target = std::move(next);
	// Every allocation is complete before either owner container or any terrain bit is published.
	_geology_world.jobs.swap(jobs);
	_geology_world.job_claims.swap(claims);
	// Released cells that were never opened go back to solid rock; opened floor never reverts.
	for (TileIndex tile : released) {
		if (_geology_world.terrain.Get(tile) != ExcavationState::Excavating) continue;
		const bool accepted = _geology_world.terrain.Set(tile, ExcavationState::Solid);
		assert(accepted);
	}
	for (TileIndex tile : footprint) {
		if (_geology_world.terrain.Get(tile) != ExcavationState::Solid) continue;
		const bool accepted = _geology_world.terrain.Set(tile, ExcavationState::Excavating);
		assert(accepted);
	}
#ifdef _DEBUG
	{
		// The published state must be exactly what a reload would rebuild.
		GeologyWorldState copy = _geology_world;
		const auto published = copy.job_claims;
		assert(RebuildExcavationJobClaims(copy) == GeologyError::None);
		assert(copy.job_claims == published);
	}
#endif
	return {ExcavationError::None, id};
}

/**
 * Expose only the actual sparse owner record.
 * @param id Exact stable identity.
 * @return Read-only accepted record or nullptr.
 */
const ExcavationJob *GetExcavationJob(ExcavationJobID id)
{
	return MutableExcavationJob(id);
}

/**
 * Prepare all bounded allocations/claims before publishing a fresh reservation.
 * @param proposal Unallocated canonical route owned by the current company.
 * @param execute Publish only on true; estimates leave terrain, IDs, reserves, money and RNG untouched.
 * @return Complete identity/claim result, or unchanged refusal.
 */
ExcavationJobResult ReserveExcavationJob(const ExcavationJob &proposal, bool execute)
{
	const auto &policy = GetCurrentMultiLayerProfileSelection();
	if (policy.id != MultiLayerGameplayProfileID::MultilayerIndustry) return {ExcavationError::InvalidPolicy};
	// Geometric reservations are real company work, not a spectator/deity substitute for a TBM's later binding.
	if (!Company::IsValidID(proposal.company) || proposal.company != _current_company) return {ExcavationError::InvalidOwner};
	// Callers cannot import old progress, paid statistics or an allocated identity as a fresh job.
	if (proposal.id != ExcavationJobID{UINT32_MAX} || proposal.state != ExcavationJobState::Proposed || proposal.cursor != 0 || proposal.partial_work_ticks != 0 || proposal.revision != 0 || proposal.completed_earth_t != 0 || proposal.completed_rock_t != 0 || proposal.completed_work_cost != 0) return {ExcavationError::InvalidJob};
	const auto geometry = ValidateExcavationJobGeometry(_geology_world, proposal);
	if (geometry.error != ExcavationError::None) return {geometry.error, ExcavationJobID{UINT32_MAX}, geometry.failure_tile};
	if (!ValidateExcavationJobReferences(proposal)) return {ExcavationError::InvalidJob};
	const auto layer = GetTileLayer(proposal.commissioning_anchor);
	if (layer == MapLayer::DeepUnderground) {
		if (!CheckDeepLayerAvailability(policy, TimerGameCalendar::date)) return {ExcavationError::LockedDeep, ExcavationJobID{UINT32_MAX}, proposal.commissioning_anchor};
	}
	// Apply the immutable cutter introduction date as well as its rock-layer capability; no calendar gate comes from the camera.
	for (const auto &model : GetMultiLayerVehicleCapabilities()) {
		if (model.model_id != proposal.model_id) continue;
		if (TimerGameCalendar::date < TimerGameCalendar::ConvertYMDToDate(model.introduction_year, 0, 1)) return {ExcavationError::InvalidPolicy};
	}
	// An unopened anchor is only acceptable as a portal-issued solid start whose Sealed pair is current; any other
	// solid anchor would be a free commissioning room, so it keeps the original MissingSupport refusal.
	const auto starter = ResolveExcavationStarter(proposal);
	if (starter.error != ExcavationError::None) return {starter.error, ExcavationJobID{UINT32_MAX}, starter.failure_tile};
	if (!starter.required && _geology_world.terrain.Get(proposal.commissioning_anchor) != ExcavationState::Excavated) return {ExcavationError::MissingSupport, ExcavationJobID{UINT32_MAX}, proposal.commissioning_anchor};
	// Bound history and monotonic identities independently of the number of claimed rock cells.
	if (_geology_world.jobs.size() >= MAX_EXCAVATION_JOBS || _geology_world.next_job_id == UINT32_MAX) return {ExcavationError::Limit};
	std::set<TileIndex> footprint;
	size_t route_cells = 0;
	// Existing saved geometry counts toward the shared transport/memory budget even after cancellation/completion.
	for (const auto &job : _geology_world.jobs) {
		for (const auto &cut : job.route) route_cells += cut.swept_tiles.size() + cut.support_tiles.size();
	}
	// Validate every next-cut cell before reserving the first one; deposits retain their sole company/reserve authority.
	for (const auto &cut : proposal.route) {
		route_cells += cut.swept_tiles.size() + cut.support_tiles.size();
		for (TileIndex tile : cut.swept_tiles) {
			if (_geology_world.job_claims.contains(tile)) return {ExcavationError::Conflict, ExcavationJobID{UINT32_MAX}, tile};
			const auto terrain = _geology_world.terrain.Get(tile);
			if (!terrain) return {ExcavationError::InvalidTile, ExcavationJobID{UINT32_MAX}, tile};
			if (*terrain == ExcavationState::Excavating) return {ExcavationError::ActiveCut, ExcavationJobID{UINT32_MAX}, tile};
			const auto type = GetTileType(tile);
			if (type != TileType::Clear && type != TileType::Railway && type != TileType::LayerPortal) return {ExcavationError::Conflict, ExcavationJobID{UINT32_MAX}, tile}; // Known occupied structures cannot become a valid future sweep merely because the ground is open.
			// The issuing housing is the one structure a solid start may sweep: its rock is reserved, opened and paid
			// like any other cell, while its portal metadata stays untouched. Any other solid structure still refuses.
			const bool housing = starter.required && tile == proposal.commissioning_anchor && GetTileOwner(tile) == proposal.company;
			if (type != TileType::Clear && !housing && (*terrain != ExcavationState::Excavated || GetTileOwner(tile) != proposal.company)) return {ExcavationError::InvalidOwner, ExcavationJobID{UINT32_MAX}, tile}; // Only this company's existing open support can participate; solid infrastructure cannot be overwritten by a cut.
			const auto *deposit = _geology_world.deposits.Get(_geology_world.deposits.Find(tile));
			// A route may cross unclaimed/same-company ore; another company's seam must refuse before any claim change.
			if (deposit != nullptr && deposit->claimant != CompanyID::Invalid() && deposit->claimant != proposal.company) return {ExcavationError::Conflict, ExcavationJobID{UINT32_MAX}, tile};
			footprint.insert(tile);
		}
	}
	if (route_cells > 262144 || footprint.size() + _geology_world.job_claims.size() > MAX_EXCAVATION_CLAIMS) return {ExcavationError::Limit}; // Aggregate bounded memory is part of accepted reservation policy.
	const auto pass = PrepareExcavationPass(proposal, proposal.pass_terms.value_or(ExcavationPassTerms{}));
	if (pass.error != ExcavationError::None) return {pass.error, ExcavationJobID{UINT32_MAX}, pass.failure_tile};
	const ExcavationJobID id{_geology_world.next_job_id};
	if (!execute) return {ExcavationError::None, id};
	auto fresh_jobs = _geology_world.jobs;
	auto fresh_claims = _geology_world.job_claims;
	ExcavationJob accepted = proposal;
	accepted.pass_terms = pass.terms;
	accepted.id = id;
	accepted.state = ExcavationJobState::Reserved;
	accepted.revision = 1;
	fresh_jobs.push_back(std::move(accepted));
	// Allocate every lookup entry before committing packed bits; no partial reservation survives allocation failure.
	for (TileIndex tile : footprint) fresh_claims.emplace(tile, id);
	_geology_world.jobs.swap(fresh_jobs);
	_geology_world.job_claims.swap(fresh_claims);
	++_geology_world.next_job_id;
	// Existing cavities remain open; only newly reserved solid cells acquire Excavating terrain meaning.
	for (TileIndex tile : footprint) {
		if (_geology_world.terrain.Get(tile) != ExcavationState::Solid) continue;
		const bool accepted_cell = _geology_world.terrain.Set(tile, ExcavationState::Excavating);
		assert(accepted_cell);
	}
	return {ExcavationError::None, id};
}

/**
 * Pause owned unfinished work while preserving claims and already committed outputs.
 * @param id Exact accepted job.
 * @param reason Non-success visible pause reason.
 * @return Complete pause or unchanged refusal.
 */
ExcavationError PauseExcavationJob(ExcavationJobID id, ExcavationError reason)
{
	auto *job = MutableExcavationJob(id);
	if (job == nullptr) return ExcavationError::InvalidJob;
	if (_current_company != job->company && _current_company != OWNER_DEITY) return ExcavationError::InvalidOwner; // Only the actual owner/deity can alter accepted job control.
	if (job->state == ExcavationJobState::Completed || job->state == ExcavationJobState::Cancelled) return ExcavationError::AlreadyCompleted; // Closed history cannot reacquire live reservations.
	if (reason == ExcavationError::None || to_underlying(reason) > to_underlying(ExcavationError::Limit) || job->revision == UINT64_MAX) return ExcavationError::InvalidJob; // Invalid reasons/stamp wrap cannot conceal a changed transaction.
	job->state = ExcavationJobState::Paused;
	job->pause_reason = reason;
	++job->revision;
	return ExcavationError::None;
}

/**
 * Advance only synchronized partial work, with physical support revalidated by its actual controller.
 * @param id Exact accepted job.
 * @param ticks Controller-supplied elapsed simulation work.
 * @param participant Actual read-only support/binding owner.
 * @return Success or recoverable pause; this never generates cargo, clears a cavity or charges a cut.
 */
ExcavationError AdvanceExcavationWork(ExcavationJobID id, uint64_t ticks, const ExcavationWorkParticipant &participant)
{
	auto *job = MutableExcavationJob(id);
	if (job == nullptr) return ExcavationError::InvalidJob;
	if (job->state == ExcavationJobState::Completed || job->state == ExcavationJobState::Cancelled) return ExcavationError::AlreadyCompleted; // Replaying a closed job cannot accumulate new work.
	if (ticks == 0) return ExcavationError::None;
	// A paused/prepared Deep route never advances merely because its reservation predates a changed calendar/editor policy.
	if (GetTileLayer(job->commissioning_anchor) == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return ExcavationError::LockedDeep;
	if (job->revision == UINT64_MAX) return ExcavationError::Limit;
	if (!Company::IsValidID(job->company)) {
		job->company = CompanyID::Invalid();
		job->state = ExcavationJobState::Paused;
		job->pause_reason = ExcavationError::InvalidOwner;
		++job->revision;
		return ExcavationError::InvalidOwner;
	}
	const auto revision = job->revision;
	const bool supported = participant.IsSupported(*job, job->route[job->cursor]);
	job = MutableExcavationJob(id);
	if (job == nullptr || job->revision != revision) return ExcavationError::StaleParticipant; // Read-only participants cannot invalidate the captured job and still apply elapsed work.
	if (!supported) {
		job->state = ExcavationJobState::Paused;
		job->pause_reason = ExcavationError::MissingSupport;
		++job->revision;
		return ExcavationError::MissingSupport;
	}
	if (ticks > MAX_EXCAVATION_WORK_TICKS - job->partial_work_ticks) return ExcavationError::Limit;
	job->partial_work_ticks += ticks;
	job->state = ExcavationJobState::Working;
	job->pause_reason = ExcavationError::None;
	++job->revision;
	return ExcavationError::None;
}

/**
 * Release only unfinished geometry, preserving historical paid work and existing cavities.
 * @param id Exact owned reservation.
 * @param execute False validates cancellation without changing work, claims or terrain.
 * @return Complete cancellation or unchanged refusal, without any inventory/cash refund.
 */
ExcavationError CancelExcavationJob(ExcavationJobID id, bool execute)
{
	auto *job = MutableExcavationJob(id);
	if (job == nullptr) return ExcavationError::InvalidJob;
	if (_current_company != job->company && _current_company != OWNER_DEITY) return ExcavationError::InvalidOwner; // Orphan cleanup requires the explicit deity lifecycle owner.
	if (job->state == ExcavationJobState::Cancelled) return ExcavationError::None;
	if (job->state == ExcavationJobState::Completed) return ExcavationError::AlreadyCompleted;
	if (job->revision == UINT64_MAX) return ExcavationError::Limit;
	if (!execute) return ExcavationError::None;
	// Claims identify exactly this job's unfinished cells; committed/open ground is never refilled by cancellation.
	for (auto it = _geology_world.job_claims.begin(); it != _geology_world.job_claims.end();) {
		if (it->second != id) { ++it; continue; }
		if (_geology_world.terrain.Get(it->first) == ExcavationState::Excavating) {
			const bool accepted = _geology_world.terrain.Set(it->first, ExcavationState::Solid);
			assert(accepted);
		}
		it = _geology_world.job_claims.erase(it);
	}
	job->state = ExcavationJobState::Cancelled;
	job->pause_reason = ExcavationError::None;
	job->partial_work_ticks = 0;
	++job->revision;
	return ExcavationError::None;
}

/**
 * Retire bounded closed history after its actual controller releases dependent project references.
 * @param id Exact closed job owned by the current company/deity.
 * @return Complete removal or unchanged refusal; the allocator cursor, cavities, cash and cargo stay unchanged.
 */
ExcavationError RetireExcavationJob(ExcavationJobID id)
{
	const auto *job = GetExcavationJob(id);
	if (job == nullptr) return ExcavationError::InvalidJob;
	if (_current_company != job->company && _current_company != OWNER_DEITY) return ExcavationError::InvalidOwner; // History retirement remains with the actual control/lifecycle owner.
	if (job->state != ExcavationJobState::Completed && job->state != ExcavationJobState::Cancelled) return ExcavationError::Conflict; // A live pass cannot lose its saved reservation/progress record.
	const auto it = std::lower_bound(_geology_world.jobs.begin(), _geology_world.jobs.end(), id, ExcavationJobBeforeID);
	_geology_world.jobs.erase(it);
	return ExcavationError::None;
}

void ChangeExcavationJobOwnership(CompanyID old_owner, CompanyID new_owner)
{
	for (auto &job : _geology_world.jobs) {
		if (job.company != old_owner) continue;
		if (new_owner != CompanyID::Invalid()) {
			job.company = new_owner;
		} else {
			job.company = CompanyID::Invalid();
			// The company's trains were deleted first: a cutter binding that no longer resolves is cleared, so it can never
			// name a later vehicle with the same ID.
			if (job.cutter != VehicleID::Invalid() && !Vehicle::IsValidID(job.cutter)) job.cutter = VehicleID::Invalid();
			// Closed history stays closed; live work takes the explicit recoverable orphan pause.
			if (job.state != ExcavationJobState::Completed && job.state != ExcavationJobState::Cancelled) {
				job.state = ExcavationJobState::Paused;
				job.pause_reason = ExcavationError::InvalidOwner;
			}
		}
		++job.revision;
	}
}

/**
 * Commit one complete cut with the actual no-throw external stock/support/funds participant.
 * @param request Exact accepted identity/revision/cursor, checked again after external validation.
 * @param participant Actual controller owner, whose false commit must leave external quantities unchanged.
 * @return Complete once-only receipt, or zero-quantity unchanged refusal/no-progress.
 */
ExcavationCutResult CommitExcavationCut(const ExcavationCutRequest &request, ExcavationMaterialParticipant &participant)
{
	const auto material = participant.Capture();
	const auto prepared = PrepareExcavationCut(request, material);
	if (prepared.error != ExcavationError::None) return prepared;
	const auto *current = GetExcavationJob(request.job);
	if (current->partial_work_ticks < prepared.work_ticks) return {ExcavationError::NoProgress};
	if (!participant.Validate(material, prepared)) return {ExcavationError::StaleParticipant};
	// A validation callback may not leave a changed geological proposal behind and still apply its earlier stock/cash quote.
	const auto revalidated = PrepareExcavationCut(request, material);
	if (revalidated != prepared) return {ExcavationError::StaleParticipant};
	auto *job = MutableExcavationJob(request.job);
	const auto &cut = job->route[job->cursor];
	std::vector<TileIndex> release;
	// Prepare all release bookkeeping before the irreversible participant commit; overlapping future envelopes keep their claims.
	for (TileIndex tile : cut.swept_tiles) {
		bool future = false;
		for (size_t i = job->cursor + 1; i < job->route.size(); ++i) {
			if (std::binary_search(job->route[i].swept_tiles.begin(), job->route[i].swept_tiles.end(), tile)) { future = true; break; }
		}
		if (!future) release.push_back(tile);
	}
	// True commits exactly the revalidated external delta; subsequent geological writes cannot allocate or fail on checked state.
	if (!participant.Commit(material, prepared)) return {ExcavationError::StaleParticipant};
	for (const auto &debit : prepared.reserve_debits) {
		if (debit.tonnes == 0) continue;
		const auto accepted = _geology_world.deposits.DebitExcavationReserve(debit.deposit, job->company, debit.tonnes, debit.expected_remaining);
		assert(accepted == GeologyError::None);
	}
	// Opening every swept cell once preserves independent infrastructure metadata; actual temporary support belongs to the participant.
	for (TileIndex tile : cut.swept_tiles) {
		const bool accepted = _geology_world.terrain.Set(tile, ExcavationState::Excavated);
		assert(accepted);
	}
	for (TileIndex tile : release) _geology_world.job_claims.erase(tile);
	job->completed_earth_t += prepared.earth_t;
	job->completed_rock_t += prepared.rock_t;
	job->completed_work_cost += prepared.cost;
	++job->cursor;
	job->partial_work_ticks = 0;
	++job->revision;
	job->pause_reason = ExcavationError::None;
	job->state = job->cursor == job->route.size() ? ExcavationJobState::Completed : ExcavationJobState::Working;
	return prepared;
}

/**
 * Allocate a bounded two-plane payload with all cells initially solid.
 * @param cells_per_plane Map plane cell count; zero removes optional feature storage.
 * @return Success, or unchanged state for an unsupported size.
 */
bool PackedExcavationState::Reset(uint32_t cells_per_plane)
{
	// Supported map planes fit the saved tile geometry; partial bytes cannot introduce phantom playable cells.
	if (cells_per_plane > MAX_MAP_SIZE * MAX_MAP_SIZE || cells_per_plane % 4 != 0) return false;
	std::vector<uint8_t> fresh(cells_per_plane / 2, 0);
	this->bytes.swap(fresh);
	this->plane_size = cells_per_plane;
	return true;
}

/**
 * Restore only an exact bounded payload with real two-bit encodings.
 * @param cells_per_plane Saved geometry, validated against the map by the snapshot owner.
 * @param payload Complete two-plane transport.
 * @return Complete acceptance or unchanged refusal.
 */
bool PackedExcavationState::Restore(uint32_t cells_per_plane, std::span<const uint8_t> payload)
{
	// Length is checked before allocation so malformed dimensions cannot cause an oversized or partial buffer.
	if (cells_per_plane > MAX_MAP_SIZE * MAX_MAP_SIZE || cells_per_plane % 4 != 0 || payload.size() != cells_per_plane / 2) return false;
	// Every pair represents one real cell; reserved 11 bits must refuse rather than become a free cavity.
	for (uint8_t byte : payload) {
		if ((byte & (byte >> 1) & 0x55) != 0) return false;
	}
	std::vector<uint8_t> fresh(payload.begin(), payload.end());
	this->bytes.swap(fresh);
	this->plane_size = cells_per_plane;
	return true;
}

/**
 * Project only a valid rock-plane world tile to packed local storage.
 * @param tile Explicit world tile, independently of current view.
 * @return Checked cell index or no result.
 */
std::optional<uint32_t> PackedExcavationState::Index(TileIndex tile) const
{
	// Absent or stale geometry is not a permission to treat unrelated memory as a cleared cavity.
	if (this->plane_size == 0 || this->plane_size != Map::PlaneSize()) return std::nullopt;
	if (!IsValidTile(tile)) return std::nullopt;
	const auto layer = GetTileLayer(tile);
	if (!MultiLayerMap::IsValidLayer(layer)) return std::nullopt;
	uint32_t rock_plane;
	if (layer == MapLayer::Underground) {
		rock_plane = 0;
	} else if (layer == MapLayer::DeepUnderground) {
		rock_plane = 1;
	} else {
		return std::nullopt;
	}
	const auto local = MultiLayerMap::ProjectToSurface(tile);
	return rock_plane * this->plane_size + local.base();
}

/**
 * Read terrain independently of track/building metadata.
 * @param tile Explicit playable rock tile.
 * @return Real decoded state, or no result for bad addressing/encoding.
 */
std::optional<ExcavationState> PackedExcavationState::Get(TileIndex tile) const
{
	const auto index = this->Index(tile);
	if (!index) return std::nullopt;
	const auto state = static_cast<ExcavationState>((this->bytes[*index / 4] >> ((*index % 4) * 2)) & 3);
	if (state == ExcavationState::Reserved) return std::nullopt;
	return state;
}

/**
 * Update one validated terrain cell without touching adjacent states or tile m-fields.
 * @param tile Explicit world tile.
 * @param state Accepted real state.
 * @return Success; every refusal leaves the payload unchanged.
 */
bool PackedExcavationState::Set(TileIndex tile, ExcavationState state)
{
	if (to_underlying(state) >= to_underlying(ExcavationState::Reserved)) return false;
	const auto index = this->Index(tile);
	if (!index) return false;
	const uint shift = (*index % 4) * 2;
	this->bytes[*index / 4] = (this->bytes[*index / 4] & ~(3U << shift)) | (to_underlying(state) << shift);
	return true;
}
