/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file asteroid.cpp Persistent finite resources, independent field generation and synchronized survey motion.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups9/11-12/26/28/28B; exit/renewal, sparse lookup, ownerless departure and atomic lifecycle publication. Group29 owns terminal loop activation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group21; opt-in operation diagnostics never govern simulation.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group33; read-only storage and scoped event/forecast counts confer no authority.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md GroupP1
 */
#include "stdafx.h"
#include "space_work_cost.h"
#include "asteroid.h"
#include "core/random_func.hpp"
#include "map_func.h"
#include "multilayer_state.h"
#include "settings_type.h"
#include "space_vehicle.h"
#include "space_transition.h"
#include "space_departure.h"
#include "asteroid_lifecycle.h"
#include "space_mining_target.h"
#include "space_mining_departure.h"
#include "openttd.h"
#include "error_func.h"
#include "company_base.h"
#include "multilayer_space_content.h"
#include "multilayer_asteroid_catalogue.h"
#include "launch_site.h"
#include "gfx_type.h"
#include "safeguards.h"

static AsteroidField _asteroid_field;
static SpaceSpatialIndex _asteroid_index;

/** @return Sole authoritative generated field; callers cannot mutate it through this view. */
const AsteroidField &GetAsteroidField() { return _asteroid_field; }

/** @copydoc ForecastAsteroidExit */
std::optional<AsteroidExitForecast> ForecastAsteroidExit(const AsteroidBody &body, const SpacePlaneBounds &bounds)
{
	RecordSpaceWork(SpaceWorkKind::ExitForecasts);
	return ForecastSpaceMotionExit(body.motion, bounds);
}

/**
 * Hash saved gameplay fields in a stable diagnostic order, excluding derived spatial/render state.
 * @param field Persistent resource, motion and survey records to compare across continuation runs.
 * @return Stable field digest, preserving the legacy stream when no knowledge records exist.
 */
uint64_t ComputeAsteroidFieldDigest(const AsteroidField &field)
{
	uint64_t digest = 14695981039346656037ULL;
	/** Append a value in fixed byte order. @param value Unsigned representation of one saved field. */
	auto hash = [&](uint64_t value) { for (int shift = 56; shift >= 0; shift -= 8) { digest ^= static_cast<uint8_t>(value >> shift); digest *= 1099511628211ULL; } };
	/** Append a projected fixed-point coordinate. @param p Saved Space point, independent of the selected camera. */
	auto point = [&](SpacePoint p) { hash(static_cast<uint32_t>(p.x)); hash(static_cast<uint32_t>(p.y)); };
	hash(field.generation_version); hash(field.width); hash(field.height); hash(field.seed); hash(field.elapsed_ticks); hash(to_underlying(field.density));
	point(field.band.min); point(field.band.max); hash(field.bodies.size()); hash(field.surveys.size());
	// Explicit ID order keeps compact-vector erasure history out of synchronized
	// resource/motion diagnostics and out of later simultaneous event ordering.
	for (const auto *record : GetAsteroidBodiesInIDOrder(field)) {
		const auto &body = *record;
		hash(body.id.base()); hash(to_underlying(body.resource_class)); hash(to_underlying(body.size)); point(body.motion.position);
		hash(static_cast<uint32_t>(body.motion.velocity_x)); hash(static_cast<uint32_t>(body.motion.velocity_y)); hash(body.motion.remainder_x); hash(body.motion.remainder_y);
		hash(body.initial_reserve); hash(body.remaining_reserve); hash(body.survey_slot.base()); hash(body.mining_slot.base());
	}
	// Survey timers and leases affect later decisions even when their current screen positions match.
	for (const auto &survey : field.surveys) {
		hash(survey.id.base()); hash(survey.model); point(survey.position); point(survey.home); hash(survey.target.base());
		hash(to_underlying(survey.phase)); hash(survey.remaining_ticks); hash(survey.travel_remainder); hash(survey.completed_surveys);
	}
	// Preserve the exact legacy diagnostic stream while no company has completed a scan.
	if (!field.knowledge.empty()) {
		hash(0x53504B4E); hash(field.knowledge.size());
		for (const auto &r : field.knowledge) { hash(r.body.base()); hash(r.surveyed_companies); }
	}
	// Legacy digests retain their exact byte stream; new inputs include frozen history rather than current terrain queries.
	if (field.starter_input.kind != AsteroidStarterKind::Legacy) {
		hash(0x41534749); hash(to_underlying(field.starter_input.kind));
		hash(field.starter_input.site_x); hash(field.starter_input.site_y); hash(field.starter_input.rotation);
		point(field.starter_input.preference); hash(field.starter_input.initial_approach_distance_squared);
	}
	// Preserve retained golden digests for pristine dense issuance. Sparse copied
	// owners additionally hash lifetime issuance, including an exhausted cursor.
	if (field.next_body_id != 0 && field.next_body_id != field.bodies.size()) {
		hash(0x41424944); hash(field.next_body_id);
	}
	// Pristine migration has the same next decision as the original seeded
	// field and keeps retained golden diagnostics. Every evolved ledger field,
	// including both words of both streams, affects subsequent native decisions.
	if (field.renewal.initialized && field.renewal != MakeAsteroidRenewalState(field.seed)) {
		hash(0x4152454E455731ULL);
		for (const auto exits : field.renewal.exited_by_size) hash(exits);
		hash(field.renewal.credits); hash(field.renewal.ticks_remaining); hash(field.renewal.last_exit.has_value());
		if (field.renewal.last_exit) { hash(to_underlying(field.renewal.last_exit->size)); hash(to_underlying(field.renewal.last_exit->resource_class)); }
		for (const auto word : field.renewal.timer_rng.state) hash(word);
		for (const auto word : field.renewal.candidate_rng.state) hash(word);
	}
	return digest;
}

/** @copydoc ValidateAsteroidKnowledgeShape */
bool ValidateAsteroidKnowledgeShape(std::span<const AsteroidKnowledgeRecord> records)
{
	if (records.size() > MAX_ORBITAL_BODIES) return false;
	uint32_t previous = 0; bool first = true;
	// Ordered unique receipts permit one authoritative disclosure per body and bounded binary lookup.
	for (const auto &r : records) {
		// Invalid references, empty masks or repeated/out-of-order bodies would grant or hide knowledge on reload.
		if (r.body == INVALID_ORBITAL_BODY || r.surveyed_companies == 0 || (r.surveyed_companies >> MAX_COMPANIES) != 0 || (!first && r.body.base() <= previous)) return false;
		previous = r.body.base(); first = false;
	}
	return true;
}
/** @copydoc ValidateAsteroidKnowledgeRecords(std::span<const AsteroidKnowledgeRecord>, uint32_t) */
bool ValidateAsteroidKnowledgeRecords(std::span<const AsteroidKnowledgeRecord> records, uint32_t body_count)
{
	// This overload describes old dense transport only. A modern receipt must
	// join to actual live/candidate IDs, never merely pass a count comparison.
	if (body_count > MAX_ORBITAL_BODIES || records.size() > body_count || !ValidateAsteroidKnowledgeShape(records)) return false;
	for (const auto &record : records) if (record.body.base() >= body_count) return false;
	return true;
}
/** @copydoc ValidateAsteroidKnowledgeRecords(std::span<const AsteroidKnowledgeRecord>, const AsteroidField &) */
bool ValidateAsteroidKnowledgeRecords(std::span<const AsteroidKnowledgeRecord> records, const AsteroidField &field)
{
	// A bounded receipt set still needs each original identity to survive; a
	// fresh body in a recycled dense slot cannot inherit another body's scan.
	if (records.size() > field.bodies.size() || !ValidateAsteroidKnowledgeShape(records)) return false;
	for (const auto &record : records) if (FindAsteroidBody(field, record.body) == nullptr) return false;
	return true;
}
/** @return Whether live disclosure receipts reference only surviving native companies. */
bool ValidateAsteroidKnowledge()
{
	if (!ValidateAsteroidKnowledgeRecords(_asteroid_field.knowledge, _asteroid_field)) return false;
	// Deleted company IDs must lose their disclosure before those IDs can be reused by another player.
	for (const auto &r : _asteroid_field.knowledge) for (uint8_t company = 0; company < MAX_COMPANIES; ++company)
		// A set bit is meaningful only for a surviving company; unset bits grant no knowledge.
		if ((r.surveyed_companies & (uint64_t{1} << company)) != 0 && !Company::IsValidID(CompanyID{company})) return false;
	return true;
}
/**
 * Publish loaded receipts only after structural and live-company validation.
 * @param records Staged sparse disclosures from the saved knowledge owner.
 * @return Whether validation succeeded and the authoritative field accepted the records.
 */
bool InstallLoadedAsteroidKnowledge(std::span<const AsteroidKnowledgeRecord> records)
{
	if (!ValidateAsteroidKnowledgeRecords(records, _asteroid_field)) return false;
	// Validate every disclosed owner before publishing any receipt, so a failed load cannot partially reveal reserves.
	for (const auto &r : records) for (uint8_t company = 0; company < MAX_COMPANIES; ++company)
		// A disclosure for a missing company could leak into a subsequently reused company ID.
		if ((r.surveyed_companies & (uint64_t{1} << company)) != 0 && !Company::IsValidID(CompanyID{company})) return false;
	_asteroid_field.knowledge.assign(records.begin(), records.end()); return true;
}
/**
 * Reveal reserves only to a real company with a completed native survey receipt.
 * @param company Requesting company; spectators and removed companies receive nothing.
 * @param id Generated body whose initial and remaining reserve are requested.
 * @return Quantities when authorized, otherwise no disclosure.
 */
std::optional<AsteroidQuantityDisclosure> GetAsteroidQuantityDisclosure(CompanyID company, OrbitalBodyID id)
{
	// Neither a missing player nor a missing body can authorize a quantity lookup.
	const auto *body = FindAsteroidBody(_asteroid_field, id);
	if (!Company::IsValidID(company) || body == nullptr) return std::nullopt;
	/** Extract the sorted receipt key. @param r One saved body disclosure. @return Stable generated body ID. */
	const auto found = std::ranges::lower_bound(_asteroid_field.knowledge, id.base(), {}, [](const auto &r) { return r.body.base(); });
	// Presence of another body's receipt or another company's scan cannot reveal this resource quantity.
	if (found == _asteroid_field.knowledge.end() || found->body != id || (found->surveyed_companies & (uint64_t{1} << company.base())) == 0) return std::nullopt;
	return AsteroidQuantityDisclosure{body->remaining_reserve, body->initial_reserve};
}
/**
 * Record one completed native survey without altering resource custody or creating cargo.
 * @param v Actual orbital survey hull executing the completed work order.
 * @return Whether the active order, target and physical approach authorize disclosure.
 */
bool CompleteAsteroidSurvey(const SpaceVehicle &v)
{
	const auto &q = v.space_orders;
	// Only a running, deployed survey hull that completed its exact work interval may grant knowledge.
	if (GetSpaceVehicleModel(v) != 19 || v.construction.placement != SpacePlacement::Orbital || !Company::IsValidID(v.owner) ||
		v.vehstatus.Test(VehState::Stopped) || !q.started || q.cursor >= q.entries.size() || q.work_ticks != GetMultiLayerSpaceBudget().scan_ticks) return false;
	const auto &order = q.entries[q.cursor];
	// The completed work must still refer to this world's natural body, rather than a structure or stale target.
	if (order.action != SpaceOrderAction::Survey || order.target.kind != SpaceOrderTargetKind::Body || order.target.generation != 0) return false;
	const auto *record = FindAsteroidBody(_asteroid_field, OrbitalBodyID{order.target.id});
	if (record == nullptr) return false;
	const auto &body = *record;
	const SpacePoint approach{body.motion.position.x + v.mission.attachment_offset.x, body.motion.position.y + v.mission.attachment_offset.y};
	if (SpaceDistanceSquared(v.position, approach) > uint64_t{SPACE_Q16 / 4} * (SPACE_Q16 / 4)) return false;
	/** Extract the sorted receipt key. @param r One saved body disclosure. @return Stable generated body ID. */
	auto found = std::ranges::lower_bound(_asteroid_field.knowledge, order.target.id, {}, [](const auto &r) { return r.body.base(); });
	// Create a receipt only when this body lacks one; repeated scans merge company authorization into the same record.
	if (found == _asteroid_field.knowledge.end() || found->body != body.id) found = _asteroid_field.knowledge.insert(found, {body.id, 0});
	found->surveyed_companies |= uint64_t{1} << v.owner.base(); return true;
}
/**
 * Transfer survey knowledge during company lifecycle changes without retaining deleted owner bits.
 * @param old_owner Company relinquishing its completed surveys.
 * @param new_owner Acquiring company, or a non-company owner when knowledge must be removed.
 */
void ChangeAsteroidKnowledgeOwnership(Owner old_owner, Owner new_owner)
{
	// A non-company source or unchanged owner has no disclosure transfer to perform.
	if (old_owner >= CompanyID::End() || old_owner == new_owner) return;
	const uint64_t old_bit = uint64_t{1} << old_owner.base();
	// Touch only receipts belonging to the old company, retaining all unrelated companies' surveys.
	for (auto &r : _asteroid_field.knowledge) if ((r.surveyed_companies & old_bit) != 0) {
		r.surveyed_companies &= ~old_bit;
		if (new_owner < CompanyID::End()) r.surveyed_companies |= uint64_t{1} << new_owner.base();
	}
	/** Remove orphan receipts. @param r Updated disclosure record. @return Whether no company retains its survey. */
	std::erase_if(_asteroid_field.knowledge, [](const auto &r) { return r.surveyed_companies == 0; });
}

/**
 * Split a feature domain without advancing Surface simulation or interactive RNG.
 * @param seed Saved Space feature seed.
 * @param domain Independent content/placement/motion identities 1/2/3 or renewal timer/candidate identities 4/5.
 * @return Locally owned deterministic generator for that domain.
 */
static Randomizer MakeSpaceRandom(uint64_t seed, uint64_t domain)
{
	uint64_t mixed = seed + domain + 0x9E3779B97F4A7C15ULL;
	mixed = (mixed ^ (mixed >> 30)) * 0xBF58476D1CE4E5B9ULL;
	mixed = (mixed ^ (mixed >> 27)) * 0x94D049BB133111EBULL;
	mixed ^= mixed >> 31;
	Randomizer random; random.SetSeed(static_cast<uint32_t>(mixed ^ (mixed >> 32))); return random;
}

/** @copydoc GetAsteroidRenewalCost */
std::optional<uint32_t> GetAsteroidRenewalCost(AsteroidSize size)
{
	static constexpr std::array<uint32_t, 3> costs{1, 3, 5};
	if (to_underlying(size) >= costs.size()) return std::nullopt;
	return costs[to_underlying(size)];
}

/** @copydoc GetAsteroidRenewalDelayBounds */
std::pair<uint32_t, uint32_t> GetAsteroidRenewalDelayBounds()
{
	constexpr uint32_t minimum = (20'000 + MILLISECONDS_PER_TICK - 1) / MILLISECONDS_PER_TICK;
	constexpr uint32_t maximum = 60'000 / MILLISECONDS_PER_TICK;
	static_assert(minimum > 0 && minimum <= maximum);
	return {minimum, maximum};
}

/** @copydoc MakeAsteroidRenewalState */
AsteroidRenewalState MakeAsteroidRenewalState(uint64_t seed)
{
	AsteroidRenewalState state;
	state.initialized = true;
	state.timer_rng = MakeSpaceRandom(seed, 4);
	state.candidate_rng = MakeSpaceRandom(seed, 5);
	return state;
}

/** @copydoc ValidateAsteroidRenewalState */
bool ValidateAsteroidRenewalState(const AsteroidRenewalState &state, uint32_t issued_body_ids)
{
	if (!state.initialized) return false;
	uint64_t exits = 0, earned = 0;
	// A body may retire only once. Checking each addition against the original
	// 32-bit issuance bound prevents forged 64-bit history from wrapping; weighted
	// earnings are then bounded by five times that same issuance count.
	for (uint8_t size = 0; size < state.exited_by_size.size(); ++size) {
		const auto count = state.exited_by_size[size];
		if (count > issued_body_ids - exits) return false;
		exits += count;
		earned += count * *GetAsteroidRenewalCost(static_cast<AsteroidSize>(size));
	}
	// Credit is never minted without terminal history. Zero credit disarms the
	// timer, while positive credit with zero ticks is one due interval.
	if (state.credits > earned || state.ticks_remaining > GetAsteroidRenewalDelayBounds().second ||
		(state.credits == 0 && state.ticks_remaining != 0) || state.last_exit.has_value() != (exits != 0)) return false;
	if (state.last_exit) {
		const auto &last = *state.last_exit;
		// Latest metadata must name an actual recorded size and a known mineral;
		// a counter in another size bucket cannot justify an invented last exit.
		if (!GetAsteroidRenewalCost(last.size) || to_underlying(last.resource_class) > to_underlying(AsteroidClass::Rich) ||
			state.exited_by_size[to_underlying(last.size)] == 0) return false;
	}
	return true;
}

/** @param state Copied ledger whose independent timer stream advances exactly once. @return Inclusive normal-speed delay in simulation opportunities. */
static uint32_t SampleAsteroidRenewalDelay(AsteroidRenewalState &state)
{
	const auto [minimum, maximum] = GetAsteroidRenewalDelayBounds();
	return minimum + state.timer_rng.Next(maximum - minimum + 1);
}

/** @copydoc ApplyAsteroidExitCredit */
std::optional<AsteroidRenewalState> ApplyAsteroidExitCredit(const AsteroidRenewalState &state, std::span<const AsteroidExitRecord> exits, uint32_t issued_body_ids)
{
	// A terminal batch is bounded by actual live population; refusal preserves
	// counters, deadline and both random streams rather than partially crediting.
	if (!ValidateAsteroidRenewalState(state, issued_body_ids) || exits.size() > MAX_ORBITAL_BODIES) return std::nullopt;
	std::vector<AsteroidExitRecord> ordered(exits.begin(), exits.end());
	/** @param a First original terminal event. @param b Second event. @return Stable body-ID order, independent of dense storage iteration. */
	std::ranges::sort(ordered, [](const auto &a, const auto &b) { return a.body < b.body; });
	auto next = state;
	OrbitalBodyID previous = INVALID_ORBITAL_BODY;
	// Ascending IDs make the final original body in a simultaneous batch supply
	// the latest size/class. Neither mineral stock nor screen visibility enters it.
	for (const auto &exit : ordered) {
		const auto cost = GetAsteroidRenewalCost(exit.metadata.size);
		if (!cost || to_underlying(exit.metadata.resource_class) > to_underlying(AsteroidClass::Rich) ||
			exit.body.base() >= issued_body_ids || exit.body == previous) return std::nullopt;
		previous = exit.body;
		// Original history already fits 32-bit issuance; explicit arithmetic guards
		// keep this transformation safe even before its complete final validation.
		auto &count = next.exited_by_size[to_underlying(exit.metadata.size)];
		if (count == UINT64_MAX || next.credits > UINT64_MAX - *cost) return std::nullopt;
		++count;
		next.credits += *cost;
		next.last_exit = exit.metadata;
	}
	// Only the empty-to-positive transition starts a new deadline. Additional
	// exits, including those arriving while due, cannot postpone an armed timer.
	if (state.credits == 0 && next.credits != 0) next.ticks_remaining = SampleAsteroidRenewalDelay(next);
	if (!ValidateAsteroidRenewalState(next, issued_body_ids)) return std::nullopt;
	return next;
}

/** @copydoc AdvanceAsteroidRenewalTimer */
std::optional<AsteroidRenewalState> AdvanceAsteroidRenewalTimer(const AsteroidRenewalState &state, uint32_t issued_body_ids)
{
	if (!ValidateAsteroidRenewalState(state, issued_body_ids)) return std::nullopt;
	auto next = state;
	if (next.ticks_remaining != 0) --next.ticks_remaining;
	return next;
}

/** @copydoc BeginAsteroidRenewalAttempt */
std::optional<AsteroidRenewalAttempt> BeginAsteroidRenewalAttempt(const AsteroidRenewalState &state, uint32_t issued_body_ids)
{
	// Only a positive due ledger may propose a candidate. GUI inspection or an
	// armed timer cannot consume another attempt or reserve a future body ID.
	if (!ValidateAsteroidRenewalState(state, issued_body_ids) || state.credits == 0 || state.ticks_remaining != 0) return std::nullopt;
	AsteroidRenewalAttempt attempt;
	attempt.next_candidate_rng = state.candidate_rng;
	attempt.ticket = attempt.next_candidate_rng.Next();
	std::array<AsteroidSize, 3> eligible{};
	uint32_t count = 0;
	// Stable size order makes one fixed candidate draw sufficient. A proposal's
	// later mineral/reserve/entry generation uses its ticket-local RNG, so even
	// allocation refusal cannot consume an unpredictable saved-stream draw count.
	for (uint8_t value = 0; value < eligible.size(); ++value) {
		const auto size = static_cast<AsteroidSize>(value);
		if (size != state.last_exit->size && *GetAsteroidRenewalCost(size) <= state.credits) eligible[count++] = size;
	}
	if (count != 0) attempt.size = eligible[ScaleToLimit(attempt.ticket, count)];
	return attempt;
}

/** @copydoc FinishAsteroidRenewalAttempt */
std::optional<AsteroidRenewalState> FinishAsteroidRenewalAttempt(const AsteroidRenewalState &state, const AsteroidRenewalAttempt &attempt, bool spawned, uint32_t issued_body_ids)
{
	const auto expected = BeginAsteroidRenewalAttempt(state, issued_body_ids);
	// A stale/modified proposal, absent eligible size or exhausted ID domain may
	// not debit credit or publish random state as though a real body was created.
	if (!expected || *expected != attempt || (spawned && (!attempt.size || issued_body_ids == UINT32_MAX))) return std::nullopt;
	auto next = state;
	next.candidate_rng = attempt.next_candidate_rng;
	if (spawned) next.credits -= *GetAsteroidRenewalCost(*attempt.size);
	next.ticks_remaining = next.credits == 0 ? 0 : SampleAsteroidRenewalDelay(next);
	if (!ValidateAsteroidRenewalState(next, issued_body_ids + (spawned ? 1U : 0U))) return std::nullopt;
	return next;
}

/**
 * Apportion exact60/30/10 shares with at least one body of each class.
 * @param count Exact supported population-table count.
 * @return Per-class populations; stable class-ID order breaks largest-remainder ties.
 */
static std::array<uint32_t, 3> InitialAsteroidClassCounts(uint32_t count)
{
	const auto shares = GetMultiLayerAsteroidClassShares(MultiLayerGameplayProfileID::MultilayerIndustry);
	std::array<uint32_t, 3> classes{}, fractions{};
	uint32_t assigned = 0;
	// Keep each resource available on tiny maps before distributing the rounding remainder.
	for (size_t i = 0; i < 3; ++i) { classes[i] = std::max(1U, count * shares[i].percent / 100); fractions[i] = count * shares[i].percent % 100; assigned += classes[i]; }
	// Award leftover bodies to the largest fractional claims without changing the requested total population.
	while (assigned < count) {
		const auto best = std::max_element(fractions.begin(), fractions.end());
		++classes[best - fractions.begin()]; *best = 0; ++assigned;
	}
	return classes;
}

/**
 * Generate saved Industry field records using independent content, placement and motion streams.
 * @param width Surface-plane width in tiles.
 * @param height Surface-plane height in tiles.
 * @param seed Saved feature seed; simulation and presentation RNG remain untouched.
 * @param density Saved exact population-table column.
 * @param starter Verified generation-time input, or none for explicit pure-field centre generation.
 * @return Complete field, or no field when the selected geometry cannot hold the exact count.
 */
std::optional<AsteroidField> GenerateAsteroidField(uint32_t width, uint32_t height, uint64_t seed, AsteroidDensity density,
	std::optional<AsteroidStarterInput> starter)
{
	const uint32_t count = GetMultiLayerAsteroidPopulation(width, height, to_underlying(density));
	if (count == 0) return std::nullopt;
	AsteroidField field;
	const auto &placement_definition = *GetMultiLayerAsteroidPlacementDefinition(MultiLayerGameplayProfileID::MultilayerIndustry);
	field.generation_version = placement_definition.generation_version; field.lifecycle_version = ASTEROID_LIFECYCLE_VERSION; field.width = width; field.height = height;
	field.seed = seed; field.density = density; field.band = GetSpaceTravelBand(width, height);
	field.renewal = MakeAsteroidRenewalState(seed);
	if (starter) field.starter_input = *starter;
	else {
		field.starter_input.kind = AsteroidStarterKind::PureField;
		field.starter_input.preference = {static_cast<int32_t>(width / 2) * SPACE_Q16, static_cast<int32_t>(height / 2) * SPACE_Q16};
	}
	const auto triple = GetAsteroidStarterTriple(width, height, field.starter_input.preference);
	if (!triple) return std::nullopt;
	field.starter_input.initial_approach_distance_squared = triple->approach_distance_squared;
	auto content = MakeSpaceRandom(seed, 1), placement = MakeSpaceRandom(seed, 2), motion = MakeSpaceRandom(seed, 3);
	std::vector<SpacePoint> grid;
	// Four-tile grid cells enforce initial spacing; capacity refusal preserves the exact selected population.
	for (int32_t y = field.band.min.y; y <= field.band.max.y; y += placement_definition.grid_spacing_tiles * SPACE_Q16)
		for (int32_t x = field.band.min.x; x <= field.band.max.x; x += placement_definition.grid_spacing_tiles * SPACE_Q16) { grid.push_back({x, y}); RecordSpaceWork(SpaceWorkKind::GenerationCells); }
	if (grid.size() < count) return std::nullopt;
	// Preserve the established leading-swap order while selecting nearest centres to the saved, explicit world preference.
	const auto center = field.starter_input.preference;
	for (uint32_t i = 0; i < 3; ++i) {
		auto best = grid.begin() + i;
		for (auto it = best; it != grid.end(); ++it) { RecordSpaceWork(SpaceWorkKind::GenerationStarterCandidates); if (SpaceDistanceSquared(*it, center) < SpaceDistanceSquared(*best, center)) best = it; }
		std::iter_swap(grid.begin() + i, best);
	}
	// Shuffle only nonstarter cells without replacement, retaining distinct positions and the central preference.
	for (uint32_t i = 3; i < count; ++i) { std::swap(grid[i], grid[i + placement.Next(static_cast<uint32_t>(grid.size()) - i)]); RecordSpaceWork(SpaceWorkKind::GenerationShuffleSwaps); }
	const auto classes = InitialAsteroidClassCounts(count);
	std::vector<AsteroidClass> labels{AsteroidClass::Metallic, AsteroidClass::Carbonaceous, AsteroidClass::Rich};
	// Reserve one starter of each class, then shuffle the exact remaining class allocation independently of placement.
	for (uint32_t c = 0; c < 3; ++c) for (uint32_t i = 1; i < classes[c]; ++i) labels.push_back(static_cast<AsteroidClass>(c));
	for (uint32_t i = 3; i < count; ++i) { std::swap(labels[i], labels[i + content.Next(count - i)]); RecordSpaceWork(SpaceWorkKind::GenerationShuffleSwaps); }
	const auto headings = GetMultiLayerAsteroidHeadingDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto speeds = GetMultiLayerAsteroidSpeedDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto &profile = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	// Publish each body's composition once in stable ID order; later loading and artwork reuse saved records.
	for (uint32_t i = 0; i < count; ++i) {
		RecordSpaceWork(SpaceWorkKind::GenerationBodies);
		const auto &definition = profile.asteroid_resources[to_underlying(labels[i])];
		const auto size = static_cast<AsteroidSize>(SelectMultiLayerAsteroidSize(content.Next(100)));
		const uint32_t reserve = SampleMultiLayerAsteroidReserve(definition, profile.asteroid_sizes[to_underlying(size)], content);
		const auto &heading = headings[motion.Next(static_cast<uint32_t>(headings.size()))];
		const int32_t speed = speeds[motion.Next(static_cast<uint32_t>(speeds.size()))].distance_q16_per_day;
		field.bodies.push_back({OrbitalBodyID{i}, labels[i], size,
			{grid[i], static_cast<int32_t>(static_cast<int64_t>(speed) * heading.x_q16 / SPACE_Q16),
			static_cast<int32_t>(static_cast<int64_t>(speed) * heading.y_q16 / SPACE_Q16), 0, 0}, reserve, reserve});
	}
	field.next_body_id = count;
	// Generation keeps its original records/RNG order; derive lookup only after
	// complete initial publication, before this owner can be used by callers.
	if (!RebuildAsteroidBodyLookup(field) || !ValidateAsteroidField(field)) return std::nullopt;
	return field;
}

/**
 * Validate historical generation metadata without consulting terrain that may since have changed.
 * @param field Structurally validated body field with bounded positions and dimensions.
 * @return Whether initial input coordinates/provenance and its frozen approach bound are consistent.
 */
static bool ValidateStarterInput(const AsteroidField &field)
{
	const auto &input = field.starter_input;
	if (input.kind == AsteroidStarterKind::Legacy) return input == AsteroidStarterInput{};
	const auto maximum = uint64_t{field.width + field.height} * SPACE_Q16;
	// Unknown provenance, an impossible preference or an excessive bound cannot supply credible initial-access history.
	if (to_underlying(input.kind) > to_underlying(AsteroidStarterKind::Site) || input.preference.x < 0 || input.preference.y < 0 ||
		input.preference.x >= static_cast<int64_t>(field.width) * SPACE_Q16 || input.preference.y >= static_cast<int64_t>(field.height) * SPACE_Q16 ||
		input.initial_approach_distance_squared > maximum * maximum) return false;
	if (input.kind == AsteroidStarterKind::Site) {
		// Recorded sites need a valid native orientation and complete original extent inside the saved world's dimensions.
		if (input.rotation >= 4 || input.site_x >= field.width || input.site_y >= field.height) return false;
		const uint32_t site_width = input.rotation % 2 == 0 ? LAUNCH_CENTER_LONG_EDGE : LAUNCH_CENTER_SHORT_EDGE;
		const uint32_t site_height = input.rotation % 2 == 0 ? LAUNCH_CENTER_SHORT_EDGE : LAUNCH_CENTER_LONG_EDGE;
		// Historical coordinates must contain the whole native extent and reproduce the saved projected anchor preference.
		if (static_cast<uint64_t>(input.site_x) + site_width > field.width || static_cast<uint64_t>(input.site_y) + site_height > field.height ||
			input.preference != SpacePoint{static_cast<int32_t>(input.site_x) * SPACE_Q16, static_cast<int32_t>(input.site_y) * SPACE_Q16}) return false;
	} else {
		// Pure/no-site generation has an explicit central preference and cannot smuggle a fabricated valid anchor into history.
		if (input.site_x != UINT32_MAX || input.site_y != UINT32_MAX || input.rotation != UINT8_MAX ||
			input.preference != SpacePoint{static_cast<int32_t>(field.width / 2) * SPACE_Q16, static_cast<int32_t>(field.height / 2) * SPACE_Q16}) return false;
	}
	if (field.elapsed_ticks == 0) {
		uint64_t actual = 0;
		const auto offset = SpaceMission{}.attachment_offset;
		const auto expected = GetAsteroidStarterTriple(field.width, field.height, input.preference);
		if (!expected) return false;
		// Initial evidence is exact before the first motion step; after that step the saved bound remains historical and immutable.
		for (uint32_t body = 0; body < 3; ++body) {
			const auto *record = FindAsteroidBody(field, OrbitalBodyID{body});
			if (record == nullptr) return false;
			const auto point = record->motion.position;
			if (point != expected->positions[body]) return false;
			actual = std::max(actual, SpaceDistanceSquared({point.x + offset.x, point.y + offset.y}, input.preference));
		}
		if (actual != input.initial_approach_distance_squared) return false;
	}
	return true;
}

/** Validate the size conservation joining original exits to all known live replacement weight.
 * @param initial Original population, independent of current live count. @param originals_live Surviving original identities.
 * @param replacement_weight Actual1/3/5 weight of known live new IDs. @param replacement_counts Known live new IDs by size.
 * @param renewal Original cumulative exits and remaining credit.
 * @return Whether a possible original retired-size distribution funds live replacements plus credit. Unknown retired replacements refund their own spent weight; they cannot mint net weight by claiming a larger exit size.
 */
static bool ValidateAsteroidLiveWeight(uint32_t initial, uint64_t originals_live, uint64_t replacement_weight,
	const std::array<uint64_t, 3> &replacement_counts, const AsteroidRenewalState &renewal)
{
	const uint64_t retired = initial - originals_live, original_weight = replacement_weight + renewal.credits;
	// Only retired originals supply net credit/live replacement weight. Each
	// weighs1/3/5, so bounds and parity must match their exact retained count.
	if (original_weight < retired || original_weight > 5 * retired || ((original_weight - retired) & 1) != 0) return false;
	const uint64_t exited = renewal.exited_by_size[0] + renewal.exited_by_size[1] + renewal.exited_by_size[2];
	// With exits of only one size, that size was never eligible for creation.
	// Thus every such exit must be original and no live replacement may have
	// that size. Unknown history cannot hide forbidden same-size recycling.
	for (uint8_t size = 0; size < renewal.exited_by_size.size(); ++size) {
		if (exited != 0 && renewal.exited_by_size[size] == exited && (exited != retired || replacement_counts[size] != 0)) return false;
	}
	const int64_t count = static_cast<int64_t>(retired), extra_pairs = static_cast<int64_t>((original_weight - retired) / 2);
	const int64_t small = static_cast<int64_t>(renewal.exited_by_size[0]), medium = static_cast<int64_t>(renewal.exited_by_size[1]), large = static_cast<int64_t>(renewal.exited_by_size[2]);
	// For original counts x/y/z, x+y+z=count and y+2z=extra_pairs.
	// Every original size count is bounded by its cumulative exit bucket.
	// Solve the remaining integer z interval without enumerating lifetime IDs.
	const int64_t difference = extra_pairs - medium;
	const int64_t minimum_large = std::max({int64_t{0}, extra_pairs - count, difference > 0 ? (difference + 1) / 2 : difference / 2});
	const int64_t maximum_large = std::min({large, extra_pairs / 2, small + extra_pairs - count});
	return minimum_large <= maximum_large;
}

/** @copydoc ValidateAsteroidField */
bool ValidateAsteroidField(const AsteroidField &field)
{
	if (field.generation_version == 0) return field == AsteroidField{};
	const auto bounds = GetSpacePlaneBounds(field.width, field.height);
	const uint32_t initial_count = GetMultiLayerAsteroidPopulation(field.width, field.height, to_underlying(field.density));
	// Historical placement remains the same contract; live positions may use the
	// entire physical plane, and terminal history may legitimately leave it empty.
	if (field.generation_version != 1 || field.lifecycle_version > ASTEROID_LIFECYCLE_VERSION || !bounds || initial_count == 0 || field.bodies.size() > MAX_ORBITAL_BODIES ||
		field.band != GetSpaceTravelBand(field.width, field.height) || field.surveys.size() > MAX_SPACE_SURVEY_PREVIEWS) return false;
	// Current transport explicitly stores both issuance and initialized renewal;
	// only a genuine legacy candidate may represent their old absent fields.
	if (field.lifecycle_version != 0 && (field.next_body_id == 0 || !field.renewal.initialized)) return false;
	const uint32_t issued = field.next_body_id == 0 ? static_cast<uint32_t>(field.bodies.size()) : field.next_body_id;
	// Zero cursor/uninitialized ledger represent actual old-reader absence only;
	// their pristine effective state is inspected without changing the candidate.
	if (!field.renewal.initialized && field.renewal != AsteroidRenewalState{}) return false;
	const auto renewal = field.renewal.initialized ? field.renewal : MakeAsteroidRenewalState(field.seed);
	if (!ValidateAsteroidRenewalState(renewal, issued)) return false;
	uint64_t exited = 0, earned = 0;
	for (uint8_t size = 0; size < renewal.exited_by_size.size(); ++size) {
		exited += renewal.exited_by_size[size];
		earned += renewal.exited_by_size[size] * *GetAsteroidRenewalCost(static_cast<AsteroidSize>(size));
	}
	// Every issued body is either live or terminal. New issuance additionally
	// requires spent renewal weight, with each real creation costing 1, 3 or 5.
	if (issued < initial_count || exited + field.bodies.size() != issued) return false;
	const uint64_t spawned = issued - initial_count, spent = earned - renewal.credits;
	if (spent < spawned || spent > 5 * spawned || ((spent - spawned) & 1) != 0) return false;
	const bool initial_history = exited == 0;
	if (field.elapsed_ticks == 0 && !initial_history) return false;
	OrbitalBodyID previous = INVALID_ORBITAL_BODY;
	std::array<uint32_t, 3> class_counts{};
	std::array<uint64_t, 3> replacement_counts{};
	uint64_t known_spawned = 0, known_spawn_cost = 0, original_live = 0;
	// Validation must not allocate: native due-attempt allocation refusal still
	// needs a trustworthy original ledger before publishing its failed closure.
	// Bounded stack scratch sorts live pointers only; no second identity owner or
	// lifetime-sized table is created. MAX_ORBITAL_BODIES bounds this frame.
	std::array<const AsteroidBody *, MAX_ORBITAL_BODIES> ordered_storage;
	auto ordered = std::span{ordered_storage}.first(field.bodies.size());
	for (size_t slot = 0; slot < field.bodies.size(); ++slot) ordered[slot] = &field.bodies[slot];
	/** @param a First original live row. @param b Second live row. @return Stable ID order independent of compact storage history. */
	std::ranges::sort(ordered, [](const auto *a, const auto *b) { return a->id < b->id; });
	// Validate every original row before granting any resource or motion authority.
	for (const auto *record : ordered) {
		RecordSpaceWork(SpaceWorkKind::ValidationBodies);
		const auto &body = *record;
		// Unissued/duplicate identities, unknown content, outside motion or reserve
		// growth would break original finite custody and safe target resolution.
		if (body.id.base() >= issued || body.id == previous || to_underlying(body.resource_class) > 2 || to_underlying(body.size) > 2 || !bounds->Contains(body.motion.position) ||
			body.motion.remainder_x >= 74 || body.motion.remainder_y >= 74 || body.remaining_reserve > body.initial_reserve) return false;
		previous = body.id;
		// Monotonic IDs distinguish surviving originals from actual later
		// issuance. A known live replacement already implies its immutable size
		// cost; only retired replacements may have an unknown historical cost.
		if (body.id.base() < initial_count) {
			++original_live;
		} else {
			++known_spawned; known_spawn_cost += *GetAsteroidRenewalCost(body.size);
			++replacement_counts[to_underlying(body.size)];
		}
		// At the exact initial instant, original placement still lies within its
		// generation band; following ticks may legitimately drift beyond it.
		if (field.elapsed_ticks == 0 && !field.band.Contains(body.motion.position)) return false;
		// Native mining leases cannot reference an impossible hull or coexist with ownerless preview surveys.
		if (body.mining_slot != VehicleID::Invalid() && (!field.surveys.empty() || body.mining_slot.base() >= VehicleID::End().base())) return false;
		++class_counts[to_underlying(body.resource_class)];
		// Before any retirement, the original starter IDs still prove bootstrap
		// access. Continuing fields need not retain a departed resource class.
		if (initial_history && body.id.base() < 3 && to_underlying(body.resource_class) != body.id.base()) return false;
		const auto &definition = GetMultiLayerAsteroidDefinitions()[to_underlying(body.resource_class)];
		const uint32_t factor = GetMultiLayerAsteroidSizeDefinitions()[to_underlying(body.size)].reserve_multiplier;
		// Initial content must be an inclusive Small-range sample scaled exactly once by its saved size.
		if (body.initial_reserve < definition.small_min * factor || body.initial_reserve > definition.small_max * factor || body.initial_reserve % factor != 0) return false;
		bool valid_velocity = false;
		// Accept only canonical cardinal/diagonal velocities; arbitrary motion would invalidate interception bounds.
		for (const auto &definition : GetMultiLayerAsteroidSpeedDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry)) {
			const int32_t speed = definition.distance_q16_per_day;
			// All reflected signs are represented by the same eight-heading catalogue, avoiding a second diagonal authority.
			for (const auto &heading : GetMultiLayerAsteroidHeadingDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry)) {
				RecordSpaceWork(SpaceWorkKind::ValidationVelocityTests);
				const int32_t x = static_cast<int64_t>(speed) * heading.x_q16 / SPACE_Q16;
				const int32_t y = static_cast<int64_t>(speed) * heading.y_q16 / SPACE_Q16;
				valid_velocity |= body.motion.velocity_x == x && body.motion.velocity_y == y;
			}
		}
		if (!valid_velocity) return false;
		// A preview lease must point to a retained survey record, never a missing owner.
		if (body.survey_slot != INVALID_SPACE_SURVEY && body.survey_slot.base() >= field.surveys.size()) return false;
	}
	// Renewal cannot bootstrap itself while every original still survives:
	// the first creation requires credit from at least one original terminal exit.
	if (spawned != 0 && original_live == initial_count) return false;
	if (known_spawned > spawned) return false;
	const uint64_t unknown_spawned = spawned - known_spawned;
	// Retired replacements may cost1/3/5, but the surviving known sizes must
	// already be fully paid. Neither an underfunded Large nor an overcharged
	// Small may hide behind aggregate min/max bounds on total issuance.
	if (spent < known_spawn_cost + unknown_spawned || spent > known_spawn_cost + 5 * unknown_spawned) return false;
	if (!ValidateAsteroidLiveWeight(initial_count, original_live, known_spawn_cost, replacement_counts, renewal)) return false;
	// Exact initial class shares constrain generation, not the changing live set.
	if (initial_history && class_counts != InitialAsteroidClassCounts(initial_count)) return false;
	// Cross-check both sides of every preview lease and its bounded timed phase before advancing any survey.
	for (uint32_t i = 0; i < field.surveys.size(); ++i) {
		const auto &survey = field.surveys[i];
		const auto *target = FindAsteroidBody(field, survey.target);
		// Only explicit Return/Waiting may lack a target. An unknown non-sentinel
		// identity cannot borrow another body or authorize pursuit/attachment.
		if (survey.id.base() != i || survey.model != 15 || to_underlying(survey.phase) > 3 || survey.travel_remainder >= 74 || survey.remaining_ticks > 148 ||
			(target == nullptr && (survey.target != INVALID_ORBITAL_BODY || (survey.phase != SpaceSurveyPhase::Return && survey.phase != SpaceSurveyPhase::Waiting)))) return false;
		// Only timed attached/waiting phases retain a countdown; other phases must not inherit hidden work.
		if ((survey.phase == SpaceSurveyPhase::Attached || survey.phase == SpaceSurveyPhase::Waiting) != (survey.remaining_ticks != 0)) return false;
		// Both current and home coordinates must stay in this world's Space plane; no edge wrapping is permitted.
		for (SpacePoint point : {survey.position, survey.home}) if (!bounds->Contains(point)) return false;
		if (target == nullptr) continue; // No-target Return/Waiting owns no body lease; the reverse pass still rejects any orphan.
		const auto &body = *target;
		if ((survey.phase == SpaceSurveyPhase::Attached) != (body.survey_slot == survey.id)) return false;
		// An attached preview follows the actual moving body transform, rather than a stale target tile.
		if (survey.phase == SpaceSurveyPhase::Attached && survey.position != SpacePoint{body.motion.position.x + SPACE_Q16, body.motion.position.y - SPACE_Q16 / 2}) return false;
	}
	// No body may keep a preview lease whose survey points elsewhere or has already departed.
	for (const auto &body : field.bodies) if (body.survey_slot != INVALID_SPACE_SURVEY) {
		const auto &survey = field.surveys[body.survey_slot.base()];
		if (survey.target != body.id || survey.phase != SpaceSurveyPhase::Attached) return false;
	}
	// Both disclosure receipts and frozen generation history must validate before any loaded resource authority is published.
	return ValidateAsteroidKnowledgeRecords(field.knowledge, field) && ValidateStarterInput(field);
}

/**
 * Publish a validated field and rebuild its disposable spatial index.
 * @param field Complete generated or loaded records transferred to the sole persistent owner.
 * @return Whether validation succeeded; a refused field never replaces live state.
 */
bool InstallAsteroidField(AsteroidField field)
{
	// Seed genuine old absence exactly once from its saved feature seed, without
	// rerolling a body or inferring any past exit. Noncanonical absence is refused.
	if (field.generation_version != 0 && field.lifecycle_version == 0 && !field.renewal.initialized) {
		if (field.renewal != AsteroidRenewalState{}) return false;
		field.renewal = MakeAsteroidRenewalState(field.seed);
	}
	if (!ValidateAsteroidField(field)) return false;
	// Only genuine legacy absence receives the old pristine dense upgrade.
	// Current typed transport preserves its already validated sparse issuance,
	// terminal history and physical-plane positions without guessing past exits.
	if (field.generation_version != 0 && field.lifecycle_version == 0) {
		if (field.renewal != MakeAsteroidRenewalState(field.seed) ||
			(field.next_body_id != 0 && field.next_body_id != field.bodies.size())) return false;
		// An old source never stored sparse identities or off-band positions.
		// Validate those original bounds before assigning its current lifecycle.
		for (size_t slot = 0; slot < field.bodies.size(); ++slot) {
			if (field.bodies[slot].id.base() != slot || !field.band.Contains(field.bodies[slot].motion.position)) return false;
		}
		field.lifecycle_version = ASTEROID_LIFECYCLE_VERSION;
	}
	if (field.next_body_id == 0) field.next_body_id = static_cast<uint32_t>(field.bodies.size());
	if (!RebuildAsteroidBodyLookup(field)) return false;
	SpaceSpatialIndex index;
	index.Reset(field.width, field.height, static_cast<uint32_t>(field.bodies.size()));
	// Populate lookup buckets from saved positions without granting those buckets any resource authority.
	for (const auto &body : field.bodies) if (!index.Insert(body.id, body.motion.position)) return false;
	_asteroid_field = std::move(field);
	_asteroid_index = std::move(index);
	return true;
}

/** @copydoc RetireAsteroidBatch */
AsteroidLifecycleResult RetireAsteroidBatch(std::span<const OrbitalBodyID> bodies)
{
	if (bodies.empty()) return {AsteroidLifecycleStatus::NoChange};
	try {
		const auto bounds = GetSpacePlaneBounds(_asteroid_field.width, _asteroid_field.height);
		// Only this installed native world's validated owners may publish. A
		// malformed counter or mismatched geometry is not a reason to hold an
		// outward body invisibly; the normal-loop caller must handle refusal.
		if (!bounds || _asteroid_field.width != Map::SizeX() || _asteroid_field.height != Map::SizeY() ||
			!ValidateAsteroidField(_asteroid_field) || _asteroid_index.Size() != _asteroid_field.bodies.size()) return {AsteroidLifecycleStatus::InvalidField};
		if (bodies.size() > _asteroid_field.bodies.size()) return {AsteroidLifecycleStatus::InvalidBatch};
		std::vector<AsteroidExitRecord> exits; exits.reserve(bodies.size()); std::array<bool, 3> changed_minerals{};
		OrbitalBodyID previous = INVALID_ORBITAL_BODY;
		// Capture immutable metadata and verify strict identity/actual outward
		// geometry before copying or erasing any storage. No partial credit is
		// earned for a duplicate, absent body, foreign lease or merely depleted body.
		for (const auto id : bodies) {
			const auto *body = FindAsteroidBody(_asteroid_field, id);
			if (body == nullptr || (previous != INVALID_ORBITAL_BODY && id <= previous)) return {AsteroidLifecycleStatus::InvalidBatch};
			previous = id;
			const auto step = StepSpaceMotionStraight(body->motion, *bounds);
			if (!step || !step->crossed_edges.Any()) return {AsteroidLifecycleStatus::InvalidBatch};
			if (body->mining_slot != VehicleID::Invalid()) return {AsteroidLifecycleStatus::ParticipantsActive};
			exits.push_back({id, {body->size, body->resource_class}}); changed_minerals[to_underlying(body->resource_class)] = true;
		}
		// A target alone supplies no lease. Actual still-active native pursuit is
		// nevertheless a participant that must pass the independent safety owner
		// before removal; it cannot be silently converted by field publication.
		for (const auto *craft : SpaceVehicle::Iterate()) {
			RecordSpaceWork(SpaceWorkKind::RetirementParticipants);
			if ((craft->mission.phase == SpaceMissionPhase::Approach || craft->mission.phase == SpaceMissionPhase::Mining) &&
				std::ranges::binary_search(bodies, craft->mission.target)) return {AsteroidLifecycleStatus::ParticipantsActive};
		}
		const auto renewal = ApplyAsteroidExitCredit(_asteroid_field.renewal, exits, _asteroid_field.next_body_id);
		if (!renewal) return {AsteroidLifecycleStatus::InvalidField};
		auto next = _asteroid_field; auto index = _asteroid_index;
		ForgetSpaceSurveyBodyTargets(next, bodies); next.renewal = *renewal;
		// Sorted receipts and sorted retiring IDs permit one merge pass. Preserve
		// all surviving companies' original receipt bits without repeatedly
		// shifting the whole knowledge vector for a large simultaneous batch.
		size_t retired_cursor = 0, output = 0;
		for (const auto receipt : next.knowledge) {
			while (retired_cursor < bodies.size() && bodies[retired_cursor] < receipt.body) ++retired_cursor;
			if (retired_cursor < bodies.size() && bodies[retired_cursor] == receipt.body) continue;
			next.knowledge[output++] = receipt;
		}
		next.knowledge.resize(output);
		// All structural changes affect only bounded transaction scratch. A
		// failed map/index operation cannot evict a live body or spend its credit.
		for (const auto id : bodies) if (!EraseAsteroidBody(next, id) || !index.Remove(id)) return {AsteroidLifecycleStatus::InvalidField};
		if (!ValidateAsteroidField(next)) return {AsteroidLifecycleStatus::InvalidField};
		static_assert(std::is_nothrow_move_assignable_v<AsteroidField> && std::is_nothrow_move_assignable_v<SpaceSpatialIndex>);
		ForgetSpaceBodySurveyOrders(bodies); // All fallible preparation is complete; no native partial scan can survive the following erasure.
		_asteroid_field = std::move(next); _asteroid_index = std::move(index);
		// Notify only after publication. These original mission-owned retries
		// wake without selection, allocation, movement, extraction or RNG draws.
		for (uint8_t mineral = 0; mineral < changed_minerals.size(); ++mineral) if (changed_minerals[mineral]) NotifySpaceMiningTargetChange(GetMultiLayerAsteroidDefinitions()[mineral].cargo);
		return {AsteroidLifecycleStatus::Retired, static_cast<uint32_t>(bodies.size())};
	} catch (const std::bad_alloc &) {
		return {AsteroidLifecycleStatus::AllocationRefused};
	}
}

/** @copydoc PlanNativeAsteroidRenewal */
AsteroidRenewalProposal PlanNativeAsteroidRenewal()
{
	AsteroidRenewalProposal proposal;
	// Only the actual active world's complete original field may supply a quote;
	// a menu/empty owner or mismatched map cannot borrow another plane's geometry.
	if (_asteroid_field.generation_version == 0 || _asteroid_field.width != Map::SizeX() || _asteroid_field.height != Map::SizeY() ||
		!ValidateAsteroidField(_asteroid_field) || _asteroid_index.Size() != _asteroid_field.bodies.size()) return proposal;
	proposal.source_renewal = _asteroid_field.renewal; proposal.source_next_body_id = _asteroid_field.next_body_id;
	proposal.attempt = BeginAsteroidRenewalAttempt(_asteroid_field.renewal, _asteroid_field.next_body_id);
	if (!proposal.attempt) { proposal.status = AsteroidRenewalProposalStatus::NotDue; return proposal; }
	try {
		auto obstacles = GetSpaceAttachmentObstacles();
		if (!obstacles) return proposal;
		const auto bounds = *GetSpacePlaneBounds(_asteroid_field.width, _asteroid_field.height);
		std::array<SpacePoint, MAX_SPACE_VEHICLES> positions; size_t count = 0;
		/** @param point Original saved waiting/departure point. Retain its one-tile common occupancy envelope while testing an incoming body; acquire no new reservation. */
		auto reserve_point = [&](SpacePoint point) {
			const SpacePoint minimum{std::max(0, point.x - SPACE_Q16), std::max(0, point.y - SPACE_Q16)};
			const SpacePoint maximum{std::min(bounds.max.x, point.x + SPACE_Q16), std::min(bounds.max.y, point.y + SPACE_Q16)};
			obstacles->push_back({minimum, maximum, SpaceAttachmentObstacleKind::ReservedArea});
		};
		// Read actual physical hulls and their original contained reservations.
		// Carried passengers have no independent Space occupancy; a second owner
		// cannot be invented to move them or clear an entry. Fleet capacity bounds
		// hull centres and at most two retained points per original craft.
		for (const auto *craft : SpaceVehicle::Iterate()) {
			if (GetSpaceVehicleLayer(*craft) != MapLayer::Space || craft->construction.placement == SpacePlacement::Carried) continue;
			if (count == positions.size() || !bounds.Contains(craft->position)) return proposal;
			auto centre = craft->position;
			const auto *body = FindAsteroidBody(_asteroid_field, craft->mission.target);
			// During the field's translation phase its exact existing lease already
			// defines the forthcoming attachment transform. Quote that owned centre
			// before final transform publication, including stopped miners; an old
			// hull pose cannot leave a newly admitted body in the translated hull.
			if (craft->construction.placement == SpacePlacement::Orbital && craft->mission.phase == SpaceMissionPhase::Mining &&
				body != nullptr && body->mining_slot == craft->index) centre = {body->motion.position.x + craft->mission.attachment_offset.x, body->motion.position.y + craft->mission.attachment_offset.y};
			if (!bounds.Contains(centre)) return proposal;
			positions[count++] = centre;
			if (craft->mission.continuation.departure_point) reserve_point(*craft->mission.continuation.departure_point);
			if (craft->space_orders.approach) reserve_point(craft->space_orders.approach->staging);
		}
		return PlanAsteroidRenewal(_asteroid_field, *obstacles, std::span{positions}.first(count));
	} catch (const std::bad_alloc &) {
		proposal.status = AsteroidRenewalProposalStatus::AllocationRefused;
		return proposal;
	}
}

/** @copydoc CheckAsteroidRenewalAdmission */
AsteroidLifecycleStatus CheckAsteroidRenewalAdmission(const AsteroidField &field, const AsteroidRenewalAttempt &attempt)
{
	// Only a complete active field can authorize creation; neither empty-world
	// defaults nor malformed counters can provide an affordable replacement.
	if (field.generation_version == 0 || !ValidateAsteroidField(field)) return AsteroidLifecycleStatus::InvalidField;
	const auto expected = BeginAsteroidRenewalAttempt(field.renewal, field.next_body_id);
	if (!expected) return AsteroidLifecycleStatus::NotDue;
	if (*expected != attempt) return AsteroidLifecycleStatus::StaleAttempt;
	if (!attempt.size) return AsteroidLifecycleStatus::NoEligibleSize;
	if (field.bodies.size() >= MAX_ORBITAL_BODIES) return AsteroidLifecycleStatus::Capacity;
	if (field.next_body_id == UINT32_MAX) return AsteroidLifecycleStatus::IssuanceExhausted;
	return AsteroidLifecycleStatus::Ready;
}

/** @copydoc CommitAsteroidRenewal */
AsteroidLifecycleResult CommitAsteroidRenewal(const AsteroidRenewalProposal &proposal)
{
	// Validate without allocating before recognizing any due attempt. Corrupt
	// original state cannot consume a ticket under the guise of allocation refusal.
	if (_asteroid_field.generation_version == 0 || _asteroid_field.width != Map::SizeX() || _asteroid_field.height != Map::SizeY() ||
		!ValidateAsteroidField(_asteroid_field) || _asteroid_index.Size() != _asteroid_field.bodies.size()) return {AsteroidLifecycleStatus::InvalidField};
	const auto expected = BeginAsteroidRenewalAttempt(_asteroid_field.renewal, _asteroid_field.next_body_id);
	if (!expected) return {AsteroidLifecycleStatus::NotDue};
	RecordSpaceWork(SpaceWorkKind::RenewalAttempts);
	// A coincidentally equal ticket/size is insufficient: intervening exits may
	// have changed credit/latest metadata without drawing the candidate stream.
	if (!proposal.attempt || *proposal.attempt != *expected || proposal.source_renewal != _asteroid_field.renewal ||
		proposal.source_next_body_id != _asteroid_field.next_body_id) return {AsteroidLifecycleStatus::StaleAttempt};
	const auto failed = FinishAsteroidRenewalAttempt(_asteroid_field.renewal, *expected, false, _asteroid_field.next_body_id);
	if (!failed) return {AsteroidLifecycleStatus::InvalidField};
	/** @param status Recognized current attempt's refusal. @return Receipt after publishing only its exact candidate-stream/timer closure; no insertion, ID debit or successful mineral event. */
	auto refuse = [&](AsteroidLifecycleStatus status) {
		_asteroid_field.renewal = *failed;
		return AsteroidLifecycleResult{status, 0, std::nullopt, true};
	};
	const auto admission = CheckAsteroidRenewalAdmission(_asteroid_field, *expected);
	if (admission != AsteroidLifecycleStatus::Ready) return refuse(admission);
	// A read-only quote can itself have encountered an actual allocation fault.
	// Commit recognizes its unchanged original ledger and closes that one failure.
	if (proposal.status == AsteroidRenewalProposalStatus::AllocationRefused) return refuse(AsteroidLifecycleStatus::AllocationRefused);
	try {
		const auto current = PlanNativeAsteroidRenewal();
		if (current.status == AsteroidRenewalProposalStatus::AllocationRefused) return refuse(AsteroidLifecycleStatus::AllocationRefused);
		if (current != proposal) return refuse(AsteroidLifecycleStatus::ProposalChanged);
		// Only a clear complete canonical quote may create. Other recognized
		// placement failures retain all credit and wait for a fresh interval.
		if (current.status == AsteroidRenewalProposalStatus::NoClearEntry) return refuse(AsteroidLifecycleStatus::NoClearEntry);
		if (current.status != AsteroidRenewalProposalStatus::Ready || !current.body) return refuse(AsteroidLifecycleStatus::ProposalChanged);
		const auto renewed = FinishAsteroidRenewalAttempt(_asteroid_field.renewal, *expected, true, _asteroid_field.next_body_id);
		if (!renewed) return refuse(AsteroidLifecycleStatus::InvalidField);
		auto next = _asteroid_field; auto index = _asteroid_index; auto body = *current.body;
		body.id = OrbitalBodyID{next.next_body_id}; const auto issued = body.id; const auto mineral = body.resource_class;
		// Both original storages allocate only in transaction copies. Even if
		// insertion into the second owner fails, the first copy's ID is never
		// published and the original credit closes as an unsuccessful attempt.
		if (!InsertAsteroidBody(next, body) || !index.Insert(issued, body.motion.position)) return refuse(AsteroidLifecycleStatus::AllocationRefused);
		next.renewal = *renewed;
		if (!ValidateAsteroidField(next)) return refuse(AsteroidLifecycleStatus::InvalidField);
		static_assert(std::is_nothrow_move_assignable_v<AsteroidField> && std::is_nothrow_move_assignable_v<SpaceSpatialIndex>);
		_asteroid_field = std::move(next); _asteroid_index = std::move(index);
		NotifySpaceMiningTargetChange(GetMultiLayerAsteroidDefinitions()[to_underlying(mineral)].cargo);
		return {AsteroidLifecycleStatus::Spawned, 0, issued, true};
	} catch (const std::bad_alloc &) {
		return refuse(AsteroidLifecycleStatus::AllocationRefused);
	}
}

/** Clear field, derived index and pending load staging before a different world is initialized. */
void ResetAsteroidField() { _asteroid_field = {}; _asteroid_index = {}; ResetAsteroidLoadStaging(); }

/**
 * Generate a new Industry world's field, leaving Classic worlds without asteroids.
 * @param world_seed New-world seed used to derive the independent saved Space feature seed.
 * @return Whether Classic needs no field or Industry successfully installed its exact population.
 */
bool GenerateWorldAsteroids(uint32_t world_seed)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return true;
	const uint64_t seed = (static_cast<uint64_t>(world_seed) << 32) | (world_seed ^ 0x53504143U);
	const auto starter = FindWorldAsteroidStarterInput(Map::SizeX(), Map::SizeY());
	if (!starter) return false;
	auto field = GenerateAsteroidField(Map::SizeX(), Map::SizeY(), seed, static_cast<AsteroidDensity>(_settings_game.game_creation.asteroid_density), starter);
	// Generation or validation refusal must not publish a seed for a missing or incomplete field.
	if (!field || !InstallAsteroidField(std::move(*field))) return false;
	SetMultiLayerOrbitalSeed(seed); return true;
}

/** @return Whether an empty preview fleet could be attached to the three retained starter targets. */
bool PrepareSpaceSurveyPreview()
{
	// Previews need all three resource classes and cannot duplicate an already installed preview fleet.
	if (_asteroid_field.bodies.size() < 3 || !_asteroid_field.surveys.empty()) return false;
	const auto bounds = GetSpacePlaneBounds(_asteroid_field.width, _asteroid_field.height);
	if (!bounds) return false;
	std::vector<SpaceSurveyPreview> previews;
	// Give each resource class an ownerless observation craft; these previews neither mine nor reveal quantities.
	for (uint32_t i = 0; i < 3; ++i) {
		const auto *body = FindAsteroidBody(_asteroid_field, OrbitalBodyID{i});
		// A surviving count alone cannot prove that the original three starter
		// identities and resource classes still exist after retirement.
		if (body == nullptr || to_underlying(body->resource_class) != i) return false;
		const auto target = body->motion.position;
		const SpacePoint home{target.x - static_cast<int32_t>(4 + i) * SPACE_Q16, target.y - 6 * SPACE_Q16};
		if (!bounds->Contains(home)) return false;
		previews.push_back({SpaceSurveyID{i}, 15, home, home, OrbitalBodyID{i}});
	}
	// Ownerless previews cannot coexist with native extraction leases. Refuse
	// before publishing anything, retaining every original paid hull's custody.
	for (const auto &body : _asteroid_field.bodies) if (body.mining_slot != VehicleID::Invalid()) return false;
	_asteroid_field.surveys = std::move(previews);
	return ValidateAsteroidField(_asteroid_field);
}

/**
 * Resolve a generated body for its authoritative gameplay owner.
 * @param id Stable body identity from this world's persistent field.
 * @return Mutable record, or nullptr when the referenced body does not exist.
 */
AsteroidBody *GetMutableAsteroidBody(OrbitalBodyID id)
{
	RecordSpaceWork(SpaceWorkKind::BodyLookups);
	return FindAsteroidBody(_asteroid_field, id);
}

/** @return Whether retiring all ownerless preview leases leaves the real resource field valid. */
bool RetireSpaceSurveyPreview()
{
	for (auto &body : _asteroid_field.bodies) body.survey_slot = INVALID_SPACE_SURVEY;
	_asteroid_field.surveys.clear();
	return ValidateAsteroidField(_asteroid_field);
}

/** @param field Sole original/copied field. @param survey Affected ownerless controller. Discard only its uncompleted scan/lease; retained coordinates/carry/home/completed count fund ordinary Return. */
static void DetachSpaceSurveyPreview(AsteroidField &field, SpaceSurveyPreview &survey)
{
	auto *body = FindAsteroidBody(field, survey.target);
	if (body != nullptr && body->survey_slot == survey.id) body->survey_slot = INVALID_SPACE_SURVEY;
	survey.target = INVALID_ORBITAL_BODY;
	// A controller already waiting at home retains its actual remaining pause.
	// Every other affected phase returns from its existing bounded point.
	if (survey.phase != SpaceSurveyPhase::Waiting) { survey.phase = SpaceSurveyPhase::Return; survey.remaining_ticks = 0; }
}

/** @copydoc ForgetSpaceSurveyBodyTargets */
void ForgetSpaceSurveyBodyTargets(AsteroidField &field, std::span<const OrbitalBodyID> bodies)
{
	// Scan only the bounded original preview owner. Retiring a target cannot
	// increment completed scans or alter unrelated native cargo/disclosure.
	for (auto &survey : field.surveys) if (std::ranges::find(bodies, survey.target) != bodies.end()) DetachSpaceSurveyPreview(field, survey);
}

/** @copydoc ReleaseUnsafeSpaceSurveyPreviews */
void ReleaseUnsafeSpaceSurveyPreviews(AsteroidField &field, std::span<const SpaceAttachmentObstacle> obstacles)
{
	const auto bounds = GetSpacePlaneBounds(field.width, field.height);
	if (!bounds) return;
	// Release before publishing an unsafe body offset, including ownerless
	// pursuit. These observation models acquire no native berth or cargo rights.
	for (auto &survey : field.surveys) {
		if (survey.phase != SpaceSurveyPhase::Approach && survey.phase != SpaceSurveyPhase::Attached) continue;
		const auto *body = FindAsteroidBody(field, survey.target);
		if (body == nullptr) { DetachSpaceSurveyPreview(field, survey); continue; }
		const auto exit = ForecastAsteroidExit(*body, *bounds);
		if (!exit) continue;
		const auto safety = ForecastSpaceAttachmentSafety(body->motion, *exit, {SPACE_Q16, -SPACE_Q16 / 2}, *bounds, obstacles);
		if (safety && safety->first_unsafe_tick && *safety->first_unsafe_tick <= 1) DetachSpaceSurveyPreview(field, survey);
	}
}

/** @param field Original live/candidate field. @param previous Last surveyed surviving ID or explicit no-target sentinel. @return Next live ID in ascending order with wrap, or absence for an empty field; compact storage order never supplies identity. */
static std::optional<OrbitalBodyID> NextSpaceSurveyTarget(const AsteroidField &field, OrbitalBodyID previous)
{
	std::optional<OrbitalBodyID> first, next;
	// One bounded live-body pass avoids allocation/sorting for each preview.
	// The original target acts only as a stable ordering key, never an offset.
	for (const auto &body : field.bodies) {
		if (!first || body.id < *first) first = body.id;
		if (body.id > previous && (!next || body.id < *next)) next = body.id;
	}
	return next ? next : first;
}

/** @copydoc AdvanceSpaceSurveyPreviews */
void AdvanceSpaceSurveyPreviews(AsteroidField &field)
{
	const auto bounds = GetSpacePlaneBounds(field.width, field.height);
	if (!bounds) return;
	// Publish all existing observation attachments first; an earlier preview decision must not see a later attachment's old position.
	for (auto &survey : field.surveys) {
		if (survey.phase != SpaceSurveyPhase::Attached) continue;
		const auto *body = FindAsteroidBody(field, survey.target);
		if (body == nullptr) { DetachSpaceSurveyPreview(field, survey); continue; }
		const auto point = body->motion.position;
		const SpacePoint attachment{point.x + SPACE_Q16, point.y - SPACE_Q16 / 2};
		// Normal-loop pre-release prevents this case. A stale copied/controller
		// call still retains its existing point instead of publishing an unsafe
		// offset or repairing it with a teleport/clamp.
		if (!bounds->Contains(attachment)) { DetachSpaceSurveyPreview(field, survey); continue; }
		survey.position = attachment;
	}
	// Stable-ID order resolves simultaneous attachment requests, never bucket order.
	for (auto &survey : field.surveys) {
		auto *target = FindAsteroidBody(field, survey.target);
		// Return/Waiting advances without a live resource. A lost pursuit also
		// returns instead of freezing forever or indexing the next compact slot.
		if (target == nullptr && (survey.target != INVALID_ORBITAL_BODY || survey.phase == SpaceSurveyPhase::Approach)) DetachSpaceSurveyPreview(field, survey);
		switch (survey.phase) {
			case SpaceSurveyPhase::Approach: {
				auto &body = *target;
				const SpacePoint attachment{body.motion.position.x + SPACE_Q16, body.motion.position.y - SPACE_Q16 / 2};
				if (!bounds->Contains(attachment)) { DetachSpaceSurveyPreview(field, survey); break; }
				// Reaching the current moving approach point permits attachment only while the body lease is free.
				if (MoveSpaceSurveyTowards(survey.position, attachment, survey.travel_remainder) && body.survey_slot == INVALID_SPACE_SURVEY) {
					body.survey_slot = survey.id; survey.phase = SpaceSurveyPhase::Attached; survey.remaining_ticks = 148;
				}
				break;
			}
			case SpaceSurveyPhase::Attached:
				if (--survey.remaining_ticks == 0) {
					target->survey_slot = INVALID_SPACE_SURVEY; survey.phase = SpaceSurveyPhase::Return; ++survey.completed_surveys;
				}
				break;
			case SpaceSurveyPhase::Return:
				if (MoveSpaceSurveyTowards(survey.position, survey.home, survey.travel_remainder)) { survey.phase = SpaceSurveyPhase::Waiting; survey.remaining_ticks = 74; }
				break;
			case SpaceSurveyPhase::Waiting:
				if (--survey.remaining_ticks == 0) {
					const auto next = NextSpaceSurveyTarget(field, survey.target);
					if (next) { survey.target = *next; survey.phase = SpaceSurveyPhase::Approach; }
					else { survey.target = INVALID_ORBITAL_BODY; survey.remaining_ticks = 74; }
				}
				break;
		}
	}
}

/** @copydoc InspectAsteroidStorage */
AsteroidStorageUsage InspectAsteroidStorage()
{
	return {_asteroid_field.bodies.size(), _asteroid_field.bodies.capacity(), _asteroid_field.body_slots.size(), _asteroid_field.body_slots.bucket_count(),
		_asteroid_field.knowledge.size(), _asteroid_field.knowledge.capacity(), _asteroid_index.InspectStorage()};
}

/** @copydoc TickAsteroidField */
void TickAsteroidField()
{
	// A field opportunity belongs only to normal running simulation. Direct
	// editor/menu/paused calls cannot earn motion, exit credit or renewal time.
	if (_game_mode != GameMode::Normal || _pause_mode.Any() || _asteroid_field.generation_version == 0) return;
	try {
		const auto bounds = GetSpacePlaneBounds(_asteroid_field.width, _asteroid_field.height);
		// Native tile projection belongs to this Map, not merely to a syntactically
		// valid copied field extent. Refuse mismatched owners before any movement.
		if (!bounds || _asteroid_field.width != Map::SizeX() || _asteroid_field.height != Map::SizeY()) FatalError("Invalid asteroid plane during synchronized field tick");
		const bool timer_was_armed = _asteroid_field.renewal.credits != 0;
		std::array<OrbitalBodyID, MAX_ORBITAL_BODIES> exit_storage; size_t exit_count = 0;
		if (_asteroid_field.bodies.size() > exit_storage.size()) FatalError("Invalid live asteroid count during synchronized field tick");
		// Gather only copied stable exit identities. The same constant-time pure
		// step is recomputed for survivors after compact erasure, avoiding pointers
		// across publication and avoiding a population-sized allocation each tick.
		for (const auto &body : _asteroid_field.bodies) {
			const auto step = StepSpaceMotionStraight(body.motion, *bounds);
			if (!step) FatalError("Invalid motion for asteroid {}", body.id.base());
			if (step->crossed_edges.Any()) exit_storage[exit_count++] = body.id;
		}
		auto exits = std::span{exit_storage}.first(exit_count); std::ranges::sort(exits);
		const auto obstacles = GetSpaceAttachmentObstacles();
		if (!obstacles) FatalError("Invalid original Space geometry during asteroid safety preparation");
		ReleaseUnsafeSpaceMiningAttachments();
		ReleaseUnsafeSpaceSurveyPreviews(_asteroid_field, *obstacles);
		if (_asteroid_field.elapsed_ticks == UINT64_MAX) FatalError("Asteroid elapsed tick domain exhausted");
		++_asteroid_field.elapsed_ticks;
		// Retire against original last-contained geometry before moving survivors.
		// This preserves valid preview attachment joins throughout the atomic
		// retirement preflight; no outside centre is ever stored or indexed.
		if (!exits.empty()) {
			const auto result = RetireAsteroidBatch(exits);
			if (result.status != AsteroidLifecycleStatus::Retired) FatalError("Asteroid retirement refused during synchronized field tick: {}", to_underlying(result.status));
		}
		// Publish each surviving original motion exactly once and refresh its
		// existing derived membership before any attachment or native decision.
		// Refused native storage cannot be hidden by freezing an outward body or
		// continuing with an index belonging to a different geometry checkpoint.
		for (auto &body : _asteroid_field.bodies) {
			const auto step = StepSpaceMotionStraight(body.motion, *bounds);
			if (!step || step->crossed_edges.Any()) FatalError("Unexpected survivor exit for asteroid {}", body.id.base());
			body.motion = step->proposed; RecordSpaceWork(SpaceWorkKind::BodySteps);
			if (!_asteroid_index.Update(body.id, body.motion.position)) FatalError("Asteroid spatial update allocation refused");
		}
		// Ownerless attachments/controllers now consume the complete surviving
		// field once. Their exact transform joins must be valid before a due
		// creation validates this same original field, including an attached scan.
		AdvanceSpaceSurveyPreviews(_asteroid_field);
		if (timer_was_armed) {
			const auto next = AdvanceAsteroidRenewalTimer(_asteroid_field.renewal, _asteroid_field.next_body_id);
			if (!next) FatalError("Invalid asteroid renewal ledger during synchronized countdown");
			_asteroid_field.renewal = *next;
		}
		// First credit this opportunity keeps its complete fresh delay. Existing
		// due intervals see this opportunity's sorted exits/latest size. Exactly
		// one commit closes the interval; the new body first moves next tick.
		if (_asteroid_field.renewal.credits != 0 && _asteroid_field.renewal.ticks_remaining == 0) {
			const auto result = CommitAsteroidRenewal(PlanNativeAsteroidRenewal());
			if (!result.attempt_closed) FatalError("Asteroid renewal failed to close its synchronized due interval: {}", to_underlying(result.status));
		}
		UpdateAttachedSpaceCraftTransforms();
	} catch (const std::bad_alloc &) {
		// Following safety publication, continuing an incomplete opportunity
		// would mix field/lease/index checkpoints. Use the engine's terminal error
		// handling instead of inventing rollback, duplicate time or hidden waits.
		FatalError("Allocation refused during synchronized asteroid field preparation");
	}
}

/**
 * Query disposable spatial data without granting mining or cargo authority.
 * @param min Inclusive minimum projected Space coordinate.
 * @param max Inclusive maximum projected Space coordinate.
 * @return Stable body IDs visible to the renderer/picker in the requested region.
 */
std::vector<OrbitalBodyID> QueryAsteroidBodies(SpacePoint min, SpacePoint max) { return _asteroid_index.Query(min, max); }
