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
#include "safeguards.h"

static AsteroidField _asteroid_field;
static SpaceSpatialIndex _asteroid_index;

const AsteroidField &GetAsteroidField() { return _asteroid_field; }

uint64_t ComputeAsteroidFieldDigest(const AsteroidField &field)
{
	uint64_t digest = 14695981039346656037ULL;
	auto hash = [&](uint64_t value) { for (int shift = 56; shift >= 0; shift -= 8) { digest ^= static_cast<uint8_t>(value >> shift); digest *= 1099511628211ULL; } };
	auto point = [&](SpacePoint p) { hash(static_cast<uint32_t>(p.x)); hash(static_cast<uint32_t>(p.y)); };
	hash(field.generation_version); hash(field.width); hash(field.height); hash(field.seed); hash(field.elapsed_ticks); hash(to_underlying(field.density));
	point(field.band.min); point(field.band.max); hash(field.bodies.size()); hash(field.surveys.size());
	for (const auto &body : field.bodies) {
		hash(body.id.base()); hash(to_underlying(body.resource_class)); hash(to_underlying(body.size)); point(body.motion.position);
		hash(static_cast<uint32_t>(body.motion.velocity_x)); hash(static_cast<uint32_t>(body.motion.velocity_y)); hash(body.motion.remainder_x); hash(body.motion.remainder_y);
		hash(body.initial_reserve); hash(body.remaining_reserve); hash(body.survey_slot.base()); hash(body.mining_slot.base());
	}
	for (const auto &survey : field.surveys) {
		hash(survey.id.base()); hash(survey.model); point(survey.position); point(survey.home); hash(survey.target.base());
		hash(to_underlying(survey.phase)); hash(survey.remaining_ticks); hash(survey.travel_remainder); hash(survey.completed_surveys);
	}
	return digest;
}

/** Feature-domain seed split; Surface simulation and interactive RNG never advance. */
static Randomizer MakeSpaceRandom(uint64_t seed, uint64_t domain)
{
	uint64_t mixed = seed + domain + 0x9E3779B97F4A7C15ULL;
	mixed = (mixed ^ (mixed >> 30)) * 0xBF58476D1CE4E5B9ULL;
	mixed = (mixed ^ (mixed >> 27)) * 0x94D049BB133111EBULL;
	mixed ^= mixed >> 31;
	Randomizer random; random.SetSeed(static_cast<uint32_t>(mixed ^ (mixed >> 32))); return random;
}

/** Exact60/30/10 shares, minimum one, stable class-ID largest-remainder tie break. */
static std::array<uint32_t, 3> InitialAsteroidClassCounts(uint32_t count)
{
	constexpr uint32_t shares[] = {6, 3, 1};
	std::array<uint32_t, 3> classes{}, fractions{};
	uint32_t assigned = 0;
	for (size_t i = 0; i < 3; ++i) { classes[i] = std::max(1U, count * shares[i] / 10); fractions[i] = count * shares[i] % 10; assigned += classes[i]; }
	while (assigned < count) {
		const auto best = std::max_element(fractions.begin(), fractions.end());
		++classes[best - fractions.begin()]; *best = 0; ++assigned;
	}
	return classes;
}

std::optional<AsteroidField> GenerateAsteroidField(uint32_t width, uint32_t height, uint64_t seed, AsteroidDensity density)
{
	const uint32_t count = GetMultiLayerAsteroidPopulation(width, height, to_underlying(density));
	if (count == 0) return std::nullopt;
	AsteroidField field;
	field.generation_version = 1; field.width = width; field.height = height;
	field.seed = seed; field.density = density; field.band = GetSpaceTravelBand(width, height);
	auto content = MakeSpaceRandom(seed, 1), placement = MakeSpaceRandom(seed, 2), motion = MakeSpaceRandom(seed, 3);
	std::vector<SpacePoint> grid;
	for (int32_t y = field.band.min.y; y <= field.band.max.y; y += 4 * SPACE_Q16)
		for (int32_t x = field.band.min.x; x <= field.band.max.x; x += 4 * SPACE_Q16) grid.push_back({x, y});
	if (grid.size() < count) return std::nullopt;
	// The preview's starter cluster is central. Actual waterside launch-site
	// accessibility belongs to Plan9/Plan10 Group2.2 and is not claimed here.
	const SpacePoint center{static_cast<int32_t>(width / 2) * SPACE_Q16, static_cast<int32_t>(height / 2) * SPACE_Q16};
	for (uint32_t i = 0; i < 3; ++i) {
		auto best = grid.begin() + i;
		for (auto it = best; it != grid.end(); ++it) if (SpaceDistanceSquared(*it, center) < SpaceDistanceSquared(*best, center)) best = it;
		std::iter_swap(grid.begin() + i, best);
	}
	for (uint32_t i = 3; i < count; ++i) std::swap(grid[i], grid[i + placement.Next(static_cast<uint32_t>(grid.size()) - i)]);
	const auto classes = InitialAsteroidClassCounts(count);
	std::vector<AsteroidClass> labels{AsteroidClass::Metallic, AsteroidClass::Carbonaceous, AsteroidClass::Rich};
	for (uint32_t c = 0; c < 3; ++c) for (uint32_t i = 1; i < classes[c]; ++i) labels.push_back(static_cast<AsteroidClass>(c));
	for (uint32_t i = 3; i < count; ++i) std::swap(labels[i], labels[i + content.Next(count - i)]);
	constexpr int32_t headings[][2] = {{65536, 0}, {46340, 46340}, {0, 65536}, {-46340, 46340},
		{-65536, 0}, {-46340, -46340}, {0, -65536}, {46340, -46340}};
	constexpr uint32_t speeds[] = {4096, 8192, 16384};
	for (uint32_t i = 0; i < count; ++i) {
		const auto &definition = GetMultiLayerAsteroidDefinitions()[to_underlying(labels[i])];
		const auto roll = content.Next(100);
		const auto size = roll < 50 ? AsteroidSize::Small : roll < 85 ? AsteroidSize::Medium : AsteroidSize::Large;
		const uint32_t reserve = (definition.small_min + content.Next(definition.small_max - definition.small_min + 1)) << to_underlying(size);
		const auto &heading = headings[motion.Next(8)];
		const int32_t speed = speeds[motion.Next(3)];
		field.bodies.push_back({OrbitalBodyID{i}, labels[i], size,
			{grid[i], static_cast<int32_t>(static_cast<int64_t>(speed) * heading[0] / SPACE_Q16),
			static_cast<int32_t>(static_cast<int64_t>(speed) * heading[1] / SPACE_Q16), 0, 0}, reserve, reserve});
	}
	return field;
}

bool ValidateAsteroidField(const AsteroidField &field)
{
	if (field.generation_version == 0) return field == AsteroidField{};
	if (field.generation_version != 1 || field.bodies.size() != GetMultiLayerAsteroidPopulation(field.width, field.height, to_underlying(field.density)) ||
		field.bodies.empty() || field.band != GetSpaceTravelBand(field.width, field.height) || field.surveys.size() > MAX_SPACE_SURVEY_PREVIEWS) return false;
	std::array<uint32_t, 3> class_counts{};
	for (uint32_t i = 0; i < field.bodies.size(); ++i) {
		const auto &body = field.bodies[i];
		if (body.id.base() != i || to_underlying(body.resource_class) > 2 || to_underlying(body.size) > 2 || !field.band.Contains(body.motion.position) ||
			body.motion.remainder_x >= 74 || body.motion.remainder_y >= 74 || body.remaining_reserve > body.initial_reserve) return false;
		if (body.mining_slot != VehicleID::Invalid() && (!field.surveys.empty() || body.mining_slot.base() >= VehicleID::End().base())) return false;
		++class_counts[to_underlying(body.resource_class)];
		if (i < 3 && to_underlying(body.resource_class) != i) return false;
		const auto &definition = GetMultiLayerAsteroidDefinitions()[to_underlying(body.resource_class)];
		const uint32_t factor = 1U << to_underlying(body.size);
		if (body.initial_reserve < definition.small_min * factor || body.initial_reserve > definition.small_max * factor || body.initial_reserve % factor != 0) return false;
		bool valid_velocity = false;
		for (const int32_t speed : {4096, 8192, 16384}) {
			const int32_t diagonal = static_cast<int64_t>(speed) * 46340 / SPACE_Q16;
			const int64_t x = std::abs(static_cast<int64_t>(body.motion.velocity_x)), y = std::abs(static_cast<int64_t>(body.motion.velocity_y));
			valid_velocity |= (x == speed && y == 0) || (x == 0 && y == speed) || (x == diagonal && y == diagonal);
		}
		if (!valid_velocity) return false;
		if (body.survey_slot != INVALID_SPACE_SURVEY && body.survey_slot.base() >= field.surveys.size()) return false;
	}
	if (class_counts != InitialAsteroidClassCounts(static_cast<uint32_t>(field.bodies.size()))) return false;
	for (uint32_t i = 0; i < field.surveys.size(); ++i) {
		const auto &survey = field.surveys[i];
		if (survey.id.base() != i || survey.model != 15 || survey.target.base() >= field.bodies.size() || to_underlying(survey.phase) > 3 || survey.travel_remainder >= 74 || survey.remaining_ticks > 148) return false;
		if ((survey.phase == SpaceSurveyPhase::Attached || survey.phase == SpaceSurveyPhase::Waiting) != (survey.remaining_ticks != 0)) return false;
		for (SpacePoint point : {survey.position, survey.home}) if (point.x < 0 || point.y < 0 || point.x >= static_cast<int32_t>(field.width) * SPACE_Q16 || point.y >= static_cast<int32_t>(field.height) * SPACE_Q16) return false;
		const auto &body = field.bodies[survey.target.base()];
		if ((survey.phase == SpaceSurveyPhase::Attached) != (body.survey_slot == survey.id)) return false;
		if (survey.phase == SpaceSurveyPhase::Attached && survey.position != SpacePoint{body.motion.position.x + SPACE_Q16, body.motion.position.y - SPACE_Q16 / 2}) return false;
	}
	for (const auto &body : field.bodies) if (body.survey_slot != INVALID_SPACE_SURVEY) {
		const auto &survey = field.surveys[body.survey_slot.base()];
		if (survey.target != body.id || survey.phase != SpaceSurveyPhase::Attached) return false;
	}
	return true;
}

bool InstallAsteroidField(AsteroidField field)
{
	if (!ValidateAsteroidField(field)) return false;
	_asteroid_index.Reset(field.width, field.height, static_cast<uint32_t>(field.bodies.size()));
	for (const auto &body : field.bodies) _asteroid_index.Update(body.id, body.motion.position);
	_asteroid_field = std::move(field); return true;
}

void ResetAsteroidField() { _asteroid_field = {}; _asteroid_index = {}; ResetAsteroidLoadStaging(); }

bool GenerateWorldAsteroids(uint32_t world_seed)
{
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return true;
	const uint64_t seed = (static_cast<uint64_t>(world_seed) << 32) | (world_seed ^ 0x53504143U);
	auto field = GenerateAsteroidField(Map::SizeX(), Map::SizeY(), seed, static_cast<AsteroidDensity>(_settings_game.game_creation.asteroid_density));
	if (!field || !InstallAsteroidField(std::move(*field))) return false;
	SetMultiLayerOrbitalSeed(seed); return true;
}

bool PrepareSpaceSurveyPreview()
{
	if (_asteroid_field.bodies.size() < 3 || !_asteroid_field.surveys.empty()) return false;
	for (uint32_t i = 0; i < 3; ++i) {
		const auto target = _asteroid_field.bodies[i].motion.position;
		const SpacePoint home{target.x - static_cast<int32_t>(4 + i) * SPACE_Q16, target.y - 6 * SPACE_Q16};
		_asteroid_field.surveys.push_back({SpaceSurveyID{i}, 15, home, home, OrbitalBodyID{i}});
	}
	return ValidateAsteroidField(_asteroid_field);
}

AsteroidBody *GetMutableAsteroidBody(OrbitalBodyID id)
{
	return id.base() < _asteroid_field.bodies.size() ? &_asteroid_field.bodies[id.base()] : nullptr;
}

bool RetireSpaceSurveyPreview()
{
	for (auto &body : _asteroid_field.bodies) body.survey_slot = INVALID_SPACE_SURVEY;
	_asteroid_field.surveys.clear();
	return ValidateAsteroidField(_asteroid_field);
}

void AdvanceAsteroidField(AsteroidField &field)
{
	if (field.generation_version == 0) return;
	++field.elapsed_ticks;
	for (auto &body : field.bodies) AdvanceSpaceMotion(body.motion, field.band);
	// Stable-ID order resolves simultaneous attachment requests, never bucket order.
	for (auto &survey : field.surveys) {
		auto &body = field.bodies[survey.target.base()];
		const SpacePoint attachment{body.motion.position.x + SPACE_Q16, body.motion.position.y - SPACE_Q16 / 2};
		switch (survey.phase) {
			case SpaceSurveyPhase::Approach:
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

void TickAsteroidField()
{
	AdvanceAsteroidField(_asteroid_field);
	for (const auto &body : _asteroid_field.bodies) _asteroid_index.Update(body.id, body.motion.position);
}

/** Only the renderer/picker consumes this derived index. */
std::vector<OrbitalBodyID> QueryAsteroidBodies(SpacePoint min, SpacePoint max) { return _asteroid_index.Query(min, max); }
