/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file asteroid.cpp Persistent finite resources, independent field generation and synchronized survey motion.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md GroupP1
 */
#include "stdafx.h"
#include "asteroid.h"
#include "core/random_func.hpp"
#include "map_func.h"
#include "multilayer_state.h"
#include "settings_type.h"
#include "space_vehicle.h"
#include "company_base.h"
#include "multilayer_space_content.h"
#include "multilayer_asteroid_catalogue.h"
#include "launch_site.h"
#include "safeguards.h"

static AsteroidField _asteroid_field;
static SpaceSpatialIndex _asteroid_index;

/** @return Sole authoritative generated field; callers cannot mutate it through this view. */
const AsteroidField &GetAsteroidField() { return _asteroid_field; }

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
	// Stable body order includes resource custody and motion, making divergent generation or extraction visible.
	for (const auto &body : field.bodies) {
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
	return digest;
}

/**
 * Validate sparse disclosure structure without consulting live companies.
 * @param records Saved strictly ordered body disclosures.
 * @param body_count Number of generated bodies that may be referenced.
 * @return Whether references, ordering and company-bit bounds are structurally legal.
 */
bool ValidateAsteroidKnowledgeRecords(std::span<const AsteroidKnowledgeRecord> records, uint32_t body_count)
{
	// Oversized fields or receipts exceeding body count cannot represent unique disclosures.
	if (body_count > GetMultiLayerAsteroidPopulation(4096, 4096, 2) || records.size() > body_count) return false;
	uint32_t previous = 0; bool first = true;
	// Ordered unique receipts permit one authoritative disclosure per body and bounded binary lookup.
	for (const auto &r : records) {
		// Invalid references, empty masks or repeated/out-of-order bodies would grant or hide knowledge on reload.
		if (r.body.base() >= body_count || r.surveyed_companies == 0 || (r.surveyed_companies >> MAX_COMPANIES) != 0 || (!first && r.body.base() <= previous)) return false;
		previous = r.body.base(); first = false;
	}
	return true;
}
/** @return Whether live disclosure receipts reference only surviving native companies. */
bool ValidateAsteroidKnowledge()
{
	if (!ValidateAsteroidKnowledgeRecords(_asteroid_field.knowledge, static_cast<uint32_t>(_asteroid_field.bodies.size()))) return false;
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
	if (!ValidateAsteroidKnowledgeRecords(records, static_cast<uint32_t>(_asteroid_field.bodies.size()))) return false;
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
	if (!Company::IsValidID(company) || id.base() >= _asteroid_field.bodies.size()) return std::nullopt;
	/** Extract the sorted receipt key. @param r One saved body disclosure. @return Stable generated body ID. */
	const auto found = std::ranges::lower_bound(_asteroid_field.knowledge, id.base(), {}, [](const auto &r) { return r.body.base(); });
	// Presence of another body's receipt or another company's scan cannot reveal this resource quantity.
	if (found == _asteroid_field.knowledge.end() || found->body != id || (found->surveyed_companies & (uint64_t{1} << company.base())) == 0) return std::nullopt;
	const auto &body = _asteroid_field.bodies[id.base()]; return AsteroidQuantityDisclosure{body.remaining_reserve, body.initial_reserve};
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
	if (order.action != SpaceOrderAction::Survey || order.target.kind != SpaceOrderTargetKind::Body || order.target.generation != 0 || order.target.id >= _asteroid_field.bodies.size()) return false;
	const auto &body = _asteroid_field.bodies[order.target.id];
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
 * @param domain Independent content, placement or motion stream identity.
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
	field.generation_version = placement_definition.generation_version; field.width = width; field.height = height;
	field.seed = seed; field.density = density; field.band = GetSpaceTravelBand(width, height);
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
		for (int32_t x = field.band.min.x; x <= field.band.max.x; x += placement_definition.grid_spacing_tiles * SPACE_Q16) grid.push_back({x, y});
	if (grid.size() < count) return std::nullopt;
	// Preserve the established leading-swap order while selecting nearest centres to the saved, explicit world preference.
	const auto center = field.starter_input.preference;
	for (uint32_t i = 0; i < 3; ++i) {
		auto best = grid.begin() + i;
		for (auto it = best; it != grid.end(); ++it) if (SpaceDistanceSquared(*it, center) < SpaceDistanceSquared(*best, center)) best = it;
		std::iter_swap(grid.begin() + i, best);
	}
	// Shuffle only nonstarter cells without replacement, retaining distinct positions and the central preference.
	for (uint32_t i = 3; i < count; ++i) std::swap(grid[i], grid[i + placement.Next(static_cast<uint32_t>(grid.size()) - i)]);
	const auto classes = InitialAsteroidClassCounts(count);
	std::vector<AsteroidClass> labels{AsteroidClass::Metallic, AsteroidClass::Carbonaceous, AsteroidClass::Rich};
	// Reserve one starter of each class, then shuffle the exact remaining class allocation independently of placement.
	for (uint32_t c = 0; c < 3; ++c) for (uint32_t i = 1; i < classes[c]; ++i) labels.push_back(static_cast<AsteroidClass>(c));
	for (uint32_t i = 3; i < count; ++i) std::swap(labels[i], labels[i + content.Next(count - i)]);
	const auto headings = GetMultiLayerAsteroidHeadingDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto speeds = GetMultiLayerAsteroidSpeedDefinitions(MultiLayerGameplayProfileID::MultilayerIndustry);
	const auto &profile = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	// Publish each body's composition once in stable ID order; later loading and artwork reuse saved records.
	for (uint32_t i = 0; i < count; ++i) {
		const auto &definition = profile.asteroid_resources[to_underlying(labels[i])];
		const auto size = static_cast<AsteroidSize>(SelectMultiLayerAsteroidSize(content.Next(100)));
		const uint32_t reserve = SampleMultiLayerAsteroidReserve(definition, profile.asteroid_sizes[to_underlying(size)], content);
		const auto &heading = headings[motion.Next(static_cast<uint32_t>(headings.size()))];
		const int32_t speed = speeds[motion.Next(static_cast<uint32_t>(speeds.size()))].distance_q16_per_day;
		field.bodies.push_back({OrbitalBodyID{i}, labels[i], size,
			{grid[i], static_cast<int32_t>(static_cast<int64_t>(speed) * heading.x_q16 / SPACE_Q16),
			static_cast<int32_t>(static_cast<int64_t>(speed) * heading.y_q16 / SPACE_Q16), 0, 0}, reserve, reserve});
	}
	if (!ValidateAsteroidField(field)) return std::nullopt;
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
			const auto point = field.bodies[body].motion.position;
			if (point != expected->positions[body]) return false;
			actual = std::max(actual, SpaceDistanceSquared({point.x + offset.x, point.y + offset.y}, input.preference));
		}
		if (actual != input.initial_approach_distance_squared) return false;
	}
	return true;
}

/**
 * Validate saved records before publishing them to gameplay or the derived spatial index.
 * @param field Complete staged generated field, including motion, custody and preview leases.
 * @return Whether its generation version and all persistent references obey the shared contract.
 */
bool ValidateAsteroidField(const AsteroidField &field)
{
	if (field.generation_version == 0) return field == AsteroidField{};
	// Unsupported generation, wrong population/band or excessive previews cannot become a playable field.
	if (field.generation_version != 1 || field.bodies.size() != GetMultiLayerAsteroidPopulation(field.width, field.height, to_underlying(field.density)) ||
		field.bodies.empty() || field.band != GetSpaceTravelBand(field.width, field.height) || field.surveys.size() > MAX_SPACE_SURVEY_PREVIEWS) return false;
	std::array<uint32_t, 3> class_counts{};
	// Validate in stable body-ID order before any body reference is used for gameplay or rendering.
	for (uint32_t i = 0; i < field.bodies.size(); ++i) {
		const auto &body = field.bodies[i];
		// Invalid identities/content, out-of-band motion or reserve growth would break finite extraction and pursuit.
		if (body.id.base() != i || to_underlying(body.resource_class) > 2 || to_underlying(body.size) > 2 || !field.band.Contains(body.motion.position) ||
			body.motion.remainder_x >= 74 || body.motion.remainder_y >= 74 || body.remaining_reserve > body.initial_reserve) return false;
		// Native mining leases cannot reference an impossible hull or coexist with ownerless preview surveys.
		if (body.mining_slot != VehicleID::Invalid() && (!field.surveys.empty() || body.mining_slot.base() >= VehicleID::End().base())) return false;
		++class_counts[to_underlying(body.resource_class)];
		// The three starter identities retain one body of each class, preserving tiny-map bootstrap access.
		if (i < 3 && to_underlying(body.resource_class) != i) return false;
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
				const int32_t x = static_cast<int64_t>(speed) * heading.x_q16 / SPACE_Q16;
				const int32_t y = static_cast<int64_t>(speed) * heading.y_q16 / SPACE_Q16;
				valid_velocity |= body.motion.velocity_x == x && body.motion.velocity_y == y;
			}
		}
		if (!valid_velocity) return false;
		// A preview lease must point to a retained survey record, never a missing owner.
		if (body.survey_slot != INVALID_SPACE_SURVEY && body.survey_slot.base() >= field.surveys.size()) return false;
	}
	if (class_counts != InitialAsteroidClassCounts(static_cast<uint32_t>(field.bodies.size()))) return false;
	// Cross-check both sides of every preview lease and its bounded timed phase before advancing any survey.
	for (uint32_t i = 0; i < field.surveys.size(); ++i) {
		const auto &survey = field.surveys[i];
		// Unknown identities, targets, models or phase counters cannot authorize a preview attachment.
		if (survey.id.base() != i || survey.model != 15 || survey.target.base() >= field.bodies.size() || to_underlying(survey.phase) > 3 || survey.travel_remainder >= 74 || survey.remaining_ticks > 148) return false;
		// Only timed attached/waiting phases retain a countdown; other phases must not inherit hidden work.
		if ((survey.phase == SpaceSurveyPhase::Attached || survey.phase == SpaceSurveyPhase::Waiting) != (survey.remaining_ticks != 0)) return false;
		// Both current and home coordinates must stay in this world's Space plane; no edge wrapping is permitted.
		for (SpacePoint point : {survey.position, survey.home}) if (point.x < 0 || point.y < 0 || point.x >= static_cast<int32_t>(field.width) * SPACE_Q16 || point.y >= static_cast<int32_t>(field.height) * SPACE_Q16) return false;
		const auto &body = field.bodies[survey.target.base()];
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
	return ValidateAsteroidKnowledgeRecords(field.knowledge, static_cast<uint32_t>(field.bodies.size())) && ValidateStarterInput(field);
}

/**
 * Publish a validated field and rebuild its disposable spatial index.
 * @param field Complete generated or loaded records transferred to the sole persistent owner.
 * @return Whether validation succeeded; a refused field never replaces live state.
 */
bool InstallAsteroidField(AsteroidField field)
{
	if (!ValidateAsteroidField(field)) return false;
	_asteroid_index.Reset(field.width, field.height, static_cast<uint32_t>(field.bodies.size()));
	// Populate lookup buckets from saved positions without granting those buckets any resource authority.
	for (const auto &body : field.bodies) _asteroid_index.Update(body.id, body.motion.position);
	_asteroid_field = std::move(field); return true;
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
	// Give each resource class an ownerless observation craft; these previews neither mine nor reveal quantities.
	for (uint32_t i = 0; i < 3; ++i) {
		const auto target = _asteroid_field.bodies[i].motion.position;
		const SpacePoint home{target.x - static_cast<int32_t>(4 + i) * SPACE_Q16, target.y - 6 * SPACE_Q16};
		_asteroid_field.surveys.push_back({SpaceSurveyID{i}, 15, home, home, OrbitalBodyID{i}});
	}
	return ValidateAsteroidField(_asteroid_field);
}

/**
 * Resolve a generated body for its authoritative gameplay owner.
 * @param id Stable index from this world's persistent field.
 * @return Mutable record, or nullptr when the referenced body does not exist.
 */
AsteroidBody *GetMutableAsteroidBody(OrbitalBodyID id)
{
	return id.base() < _asteroid_field.bodies.size() ? &_asteroid_field.bodies[id.base()] : nullptr;
}

/** @return Whether retiring all ownerless preview leases leaves the real resource field valid. */
bool RetireSpaceSurveyPreview()
{
	for (auto &body : _asteroid_field.bodies) body.survey_slot = INVALID_SPACE_SURVEY;
	_asteroid_field.surveys.clear();
	return ValidateAsteroidField(_asteroid_field);
}

/**
 * Advance body motion before preview survey decisions on the shared synchronized schedule.
 * @param field Persistent field whose elapsed tick, body transforms and preview phases advance once.
 */
void AdvanceAsteroidField(AsteroidField &field)
{
	if (field.generation_version == 0) return;
	++field.elapsed_ticks;
	// Body records are validated in monotonic ID order; publish all transforms before any attached preview decides work.
	for (auto &body : field.bodies) AdvanceSpaceMotion(body.motion, field.band);
	// Publish all existing observation attachments first; an earlier preview decision must not see a later attachment's old position.
	for (auto &survey : field.surveys) {
		if (survey.phase != SpaceSurveyPhase::Attached) continue;
		const auto point = field.bodies[survey.target.base()].motion.position;
		survey.position = {point.x + SPACE_Q16, point.y - SPACE_Q16 / 2};
	}
	// Stable-ID order resolves simultaneous attachment requests, never bucket order.
	for (auto &survey : field.surveys) {
		auto &body = field.bodies[survey.target.base()];
		const SpacePoint attachment{body.motion.position.x + SPACE_Q16, body.motion.position.y - SPACE_Q16 / 2};
		switch (survey.phase) {
			case SpaceSurveyPhase::Approach:
				// Reaching the current moving approach point permits attachment only while the body lease is free.
				if (MoveSpaceSurveyTowards(survey.position, attachment, survey.travel_remainder) && body.survey_slot == INVALID_SPACE_SURVEY) {
					body.survey_slot = survey.id; survey.phase = SpaceSurveyPhase::Attached; survey.remaining_ticks = 148;
				}
				break;
			case SpaceSurveyPhase::Attached:
				survey.position = attachment;
				if (--survey.remaining_ticks == 0) {
					body.survey_slot = INVALID_SPACE_SURVEY; survey.phase = SpaceSurveyPhase::Return; ++survey.completed_surveys;
				}
				break;
			case SpaceSurveyPhase::Return:
				if (MoveSpaceSurveyTowards(survey.position, survey.home, survey.travel_remainder)) { survey.phase = SpaceSurveyPhase::Waiting; survey.remaining_ticks = 74; }
				break;
			case SpaceSurveyPhase::Waiting:
				if (--survey.remaining_ticks == 0) { survey.target = OrbitalBodyID{(survey.target.base() + 1) % 3}; survey.phase = SpaceSurveyPhase::Approach; }
				break;
		}
	}
}

/** Advance the authoritative field once and refresh derived lookup buckets from its new positions. */
void TickAsteroidField()
{
	AdvanceAsteroidField(_asteroid_field);
	// Disposable buckets must reflect the complete new field before actual attached hulls or any probe decision use it.
	for (const auto &body : _asteroid_field.bodies) _asteroid_index.Update(body.id, body.motion.position);
	UpdateAttachedSpaceCraftTransforms();
}

/**
 * Query disposable spatial data without granting mining or cargo authority.
 * @param min Inclusive minimum projected Space coordinate.
 * @param max Inclusive maximum projected Space coordinate.
 * @return Stable body IDs visible to the renderer/picker in the requested region.
 */
std::vector<OrbitalBodyID> QueryAsteroidBodies(SpacePoint min, SpacePoint max) { return _asteroid_index.Query(min, max); }
