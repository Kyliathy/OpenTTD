/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file excavation.cpp Packed rock authority independent of tile reconstruction. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md */

#include "stdafx.h"
#include "excavation.h"
#include "openttd.h"
#include "excavation_state.h"
#include "tile_map.h"
#include "multilayer_content.h"
#include "multilayer_state.h"
#include "debug.h"
#include "company_base.h"
#include "company_func.h"

#include "safeguards.h"

static GeologyWorldState _geology_world; ///< Sole accepted rock/deposit owner; seed metadata remains with multilayer_state.cpp.
static std::optional<GeologyWorldState> _pending_geology; ///< Validated unaccepted load candidate, not another live reserve owner.

/** @return Accepted geological authority, independently of camera selection. */
const GeologyWorldState &GetGeologyWorldState()
{
	return _geology_world;
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
 * @return Refusal or complete accepted geology; Classic remains allocation-free.
 */
GeologyError GenerateWorldGeology(uint32_t world_seed)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return GeologyError::None;
	const uint64_t seed = DeriveGeologySeed(world_seed);
	auto generated = PrepareGeologyGeneration(seed);
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
	if (_geology_world.terrain.Get(proposal.commissioning_anchor) != ExcavationState::Excavated) return {ExcavationError::MissingSupport, ExcavationJobID{UINT32_MAX}, proposal.commissioning_anchor};
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
			if (type != TileType::Clear && (*terrain != ExcavationState::Excavated || GetTileOwner(tile) != proposal.company)) return {ExcavationError::InvalidOwner, ExcavationJobID{UINT32_MAX}, tile}; // Only this company's existing open support can participate; solid infrastructure cannot be overwritten by a cut.
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
 * @return Complete cancellation or unchanged refusal, without any inventory/cash refund.
 */
ExcavationError CancelExcavationJob(ExcavationJobID id)
{
	auto *job = MutableExcavationJob(id);
	if (job == nullptr) return ExcavationError::InvalidJob;
	if (_current_company != job->company && _current_company != OWNER_DEITY) return ExcavationError::InvalidOwner; // Orphan cleanup requires the explicit deity lifecycle owner.
	if (job->state == ExcavationJobState::Cancelled) return ExcavationError::None;
	if (job->state == ExcavationJobState::Completed) return ExcavationError::AlreadyCompleted;
	if (job->revision == UINT64_MAX) return ExcavationError::Limit;
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
