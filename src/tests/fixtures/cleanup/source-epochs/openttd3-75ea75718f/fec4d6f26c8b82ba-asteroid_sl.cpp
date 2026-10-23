/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file asteroid_sl.cpp Bounded asteroid/survey transport and424 frozen initial starter input; independent read-only preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../asteroid.h"
#include "../multilayer_state.h"
#include "../map_func.h"
#include "../fios.h"
#include "../safeguards.h"

static std::optional<AsteroidField> _staged_asteroids;
static std::optional<std::vector<AsteroidKnowledgeRecord>> _staged_knowledge;

/** Clear pending field/disclosure authority before another load or preview operation. */
void ResetAsteroidLoadStaging() { _staged_asteroids.reset(); _staged_knowledge.reset(); }
/** @param field Complete validated decoded field, transferred only to staging rather than live gameplay. */
void StageLoadedAsteroidField(AsteroidField field) { _staged_asteroids = std::move(field); }

/** @param value Unsigned saved representation. @param width Exact byte width; fixed big-endian order makes native continuation comparable. */
static void WriteUnsigned(uint64_t value, uint8_t width)
{
	for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift));
}
/** @param width Exact unsigned byte width. @return Bounded-format scalar reconstructed in the same portable byte order. */
static uint64_t ReadUnsigned(uint8_t width)
{
	uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value;
}
/** @param point Signed projected Q16 coordinate; it never stores camera or stacked-world Y. */
static void WritePoint(SpacePoint point) { WriteUnsigned(static_cast<uint32_t>(point.x), 4); WriteUnsigned(static_cast<uint32_t>(point.y), 4); }
/** @return Signed projected Q16 coordinate; gameplay/preview validators subsequently check its saved plane bounds. */
static SpacePoint ReadPoint() { return {static_cast<int32_t>(ReadUnsigned(4)), static_cast<int32_t>(ReadUnsigned(4))}; }

/**
 * Validate field and saved world authority together before publishing orbital state.
 * @param field Staged complete field, or nullptr for an older schema with no asteroid owner.
 * @param width Actual loaded Surface-plane width.
 * @param height Actual loaded Surface-plane height.
 * @param flags Saved world feature flags, independent of current settings/camera selection.
 * @param seed Saved independent Space feature seed.
 */
static void CheckField(const AsteroidField *field, uint32_t width, uint32_t height, uint32_t flags, uint64_t seed)
{
	if (field == nullptr) {
		// A modern schema or enabled feature requires its actual owner; only genuinely older absence may migrate empty.
		if (!IsSavegameVersionBefore(SaveLoadVersion::AsteroidField) || (flags & MULTILAYER_FEATURE_ASTEROIDS) != 0) SlErrorCorrupt("Missing asteroid field");
		return;
	}
	const bool active = field->generation_version != 0;
	// Feature presence, structure and world inputs must agree; mismatched ownership cannot be legitimized by a valid-looking row.
	if (active != ((flags & MULTILAYER_FEATURE_ASTEROIDS) != 0) || !ValidateAsteroidField(*field) ||
		(active && (field->width != width || field->height != height || field->seed != seed))) SlErrorCorrupt("Invalid asteroid field authority");
}

/** @return Whether a validated staged field could be published without deriving new bodies or initial history. */
bool AcceptLoadedAsteroidField()
{
	if (!_staged_asteroids) return IsSavegameVersionBefore(SaveLoadVersion::AsteroidField);
	if (!InstallAsteroidField(std::move(*_staged_asteroids))) return false;
	_staged_asteroids.reset(); return true;
}

/** Check bounded preview owner/feature/seed consistency while leaving the accepted world's resources untouched. */
void ValidateAsteroidLoadPreview()
{
	// Modern saved worlds cannot omit their owner; preview presence is separate from whether any asteroid feature is active.
	if (!_load_check_data.asteroid_present && !IsSavegameVersionBefore(SaveLoadVersion::AsteroidField)) SlErrorCorrupt("Missing asteroid preview family");
	if (_load_check_data.asteroid_active != ((_load_check_data.multilayer_feature_flags & MULTILAYER_FEATURE_ASTEROIDS) != 0)) SlErrorCorrupt("Asteroid preview feature mismatch");
	// An active field must belong to this candidate world's saved seed, never to the running world's previous generation.
	if (_load_check_data.asteroid_active && _load_check_data.asteroid_seed != _load_check_data.orbital_seed) SlErrorCorrupt("Asteroid preview seed mismatch");
}

/** Sole asteroid field chunk owner; generated records and original access evidence load without rerolling terrain or RNG. */
struct ASTRChunkHandler : ChunkHandler {
	/** Bind the existing bounded RIFF owner; starter inputs extend its payload rather than creating a duplicate registry. */
	ASTRChunkHandler() : ChunkHandler("ASTR", ChunkType::Riff) {}
	/** Save authoritative records and frozen initial inputs, preserving the established header/body/survey offsets. */
	void Save() const override
	{
		const auto &field = GetAsteroidField();
		CheckField(&field, Map::SizeX(), Map::SizeY(), CaptureMultiLayerStateSchema().feature_flags, CaptureMultiLayerStateSchema().orbital_seed);
		SlSetLength(51 + 40 * field.bodies.size() + 34 * field.surveys.size() + 26);
		WriteUnsigned(field.generation_version, 2); WriteUnsigned(field.width, 4); WriteUnsigned(field.height, 4);
		WriteUnsigned(field.seed, 8); WriteUnsigned(field.elapsed_ticks, 8); WriteUnsigned(to_underlying(field.density), 1);
		WriteUnsigned(field.bodies.size(), 4); WriteUnsigned(field.surveys.size(), 4);
		WritePoint(field.band.min); WritePoint(field.band.max);
		// Stable body order saves finite resource custody and motion exactly once; spatial buckets remain disposable.
		for (const auto &body : field.bodies) {
			WriteUnsigned(body.id.base(), 4); WriteUnsigned(to_underlying(body.resource_class), 1); WriteUnsigned(to_underlying(body.size), 1);
			WritePoint(body.motion.position); WriteUnsigned(static_cast<uint32_t>(body.motion.velocity_x), 4); WriteUnsigned(static_cast<uint32_t>(body.motion.velocity_y), 4);
			WriteUnsigned(body.motion.remainder_x, 1); WriteUnsigned(body.motion.remainder_y, 1);
			WriteUnsigned(body.initial_reserve, 4); WriteUnsigned(body.remaining_reserve, 4); WriteUnsigned(body.survey_slot.base(), 4); WriteUnsigned(body.mining_slot.base(), 4);
		}
		// Preview leases/timers continue from their actual saved phase and never synthesize native mining cargo.
		for (const auto &survey : field.surveys) {
			WriteUnsigned(survey.id.base(), 4); WriteUnsigned(survey.model, 2); WritePoint(survey.position); WritePoint(survey.home);
			WriteUnsigned(survey.target.base(), 4); WriteUnsigned(to_underlying(survey.phase), 1); WriteUnsigned(survey.remaining_ticks, 2);
			WriteUnsigned(survey.travel_remainder, 1); WriteUnsigned(survey.completed_surveys, 4);
		}
		const auto &input = field.starter_input;
		WriteUnsigned(to_underlying(input.kind), 1); WriteUnsigned(input.site_x, 4); WriteUnsigned(input.site_y, 4);
		WriteUnsigned(input.rotation, 1); WritePoint(input.preference); WriteUnsigned(input.initial_approach_distance_squared, 8);
	}
	/** @return Bounded validated field; pre424 records retain explicit unknown initial history without reconstruction. */
	AsteroidField Read() const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::AsteroidField)) SlErrorCorrupt("Unexpected asteroid family in old save");
		const auto length = SlGetFieldLength();
		const uint32_t body_size = IsSavegameVersionBefore(SaveLoadVersion::SpaceMiningVehicles) ? 36 : 40;
		const uint32_t input_size = IsSavegameVersionBefore(SaveLoadVersion::AsteroidStarterInputs) ? 0 : 26;
		// Bound the complete payload before allocation; a new schema cannot omit or append arbitrary initial-input bytes.
		if (length < 51 + input_size || length > 51 + body_size * MAX_ORBITAL_BODIES + 34 * MAX_SPACE_SURVEY_PREVIEWS + input_size) SlErrorCorrupt("Invalid asteroid family length");
		AsteroidField field;
		field.generation_version = ReadUnsigned(2); field.width = ReadUnsigned(4); field.height = ReadUnsigned(4);
		field.seed = ReadUnsigned(8); field.elapsed_ticks = ReadUnsigned(8); field.density = static_cast<AsteroidDensity>(ReadUnsigned(1));
		const auto bodies = ReadUnsigned(4), surveys = ReadUnsigned(4);
		// Declared counts must account for every retained row and the schema-qualified fixed input trailer.
		if (bodies > MAX_ORBITAL_BODIES || surveys > MAX_SPACE_SURVEY_PREVIEWS || length != 51 + bodies * body_size + surveys * 34 + input_size) SlErrorCorrupt("Invalid asteroid record count");
		field.band.min = ReadPoint(); field.band.max = ReadPoint();
		field.bodies.resize(static_cast<size_t>(bodies)); field.surveys.resize(static_cast<size_t>(surveys));
		// Decode the sole stable-ID body records; old native lease formats migrate only their missing lease field.
		for (auto &body : field.bodies) {
			body.id = OrbitalBodyID{static_cast<uint32_t>(ReadUnsigned(4))}; body.resource_class = static_cast<AsteroidClass>(ReadUnsigned(1)); body.size = static_cast<AsteroidSize>(ReadUnsigned(1));
			body.motion.position = ReadPoint(); body.motion.velocity_x = static_cast<int32_t>(ReadUnsigned(4)); body.motion.velocity_y = static_cast<int32_t>(ReadUnsigned(4));
			body.motion.remainder_x = ReadUnsigned(1); body.motion.remainder_y = ReadUnsigned(1);
			body.initial_reserve = ReadUnsigned(4); body.remaining_reserve = ReadUnsigned(4); body.survey_slot = SpaceSurveyID{static_cast<uint32_t>(ReadUnsigned(4))};
			if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceMiningVehicles)) body.mining_slot = VehicleID{static_cast<uint32_t>(ReadUnsigned(4))};
		}
		// Decode actual preview phases; field validation subsequently cross-checks both sides of every lease.
		for (auto &survey : field.surveys) {
			survey.id = SpaceSurveyID{static_cast<uint32_t>(ReadUnsigned(4))}; survey.model = ReadUnsigned(2); survey.position = ReadPoint(); survey.home = ReadPoint();
			survey.target = OrbitalBodyID{static_cast<uint32_t>(ReadUnsigned(4))}; survey.phase = static_cast<SpaceSurveyPhase>(ReadUnsigned(1)); survey.remaining_ticks = ReadUnsigned(2);
			survey.travel_remainder = ReadUnsigned(1); survey.completed_surveys = ReadUnsigned(4);
		}
		if (input_size != 0) {
			auto &input = field.starter_input;
			input.kind = static_cast<AsteroidStarterKind>(ReadUnsigned(1));
			input.site_x = ReadUnsigned(4); input.site_y = ReadUnsigned(4); input.rotation = ReadUnsigned(1);
			input.preference = ReadPoint(); input.initial_approach_distance_squared = ReadUnsigned(8);
		}
		if (!ValidateAsteroidField(field)) SlErrorCorrupt("Invalid asteroid/survey record");
		return field;
	}
	/** Stage one complete validated field; repeated authority cannot overwrite an earlier family during loading. */
	void Load() const override { if (_staged_asteroids) SlErrorCorrupt("Repeated asteroid field"); StageLoadedAsteroidField(this->Read()); }
	/** Check staged field against saved world metadata before any resource authority is published. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		const auto *metadata = GetPendingMultiLayerStateSchema();
		CheckField(_staged_asteroids ? &*_staged_asteroids : nullptr, Map::SizeX(), Map::SizeY(), metadata == nullptr ? 0 : metadata->feature_flags, metadata == nullptr ? 0 : metadata->orbital_seed);
	}
	/** @param length Framing supplied by the dispatcher; Read validates the exact schema-qualified payload before preview publication. */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.asteroid_present) SlErrorCorrupt("Repeated asteroid field preview");
		const auto field = this->Read();
		// Active preview coordinates must match the candidate world's dimensions and Space plane availability.
		if (field.generation_version != 0 && (field.width != _load_check_data.map_size_x || field.height != _load_check_data.map_size_y || _load_check_data.map_layers < 3)) SlErrorCorrupt("Asteroid preview geometry mismatch");
		_load_check_data.asteroid_present = true; _load_check_data.asteroid_active = field.generation_version != 0; _load_check_data.asteroid_seed = field.seed;
		_load_check_data.asteroid_body_count = static_cast<uint32_t>(field.bodies.size());
	}
};
/** @return Whether staged knowledge could be published after native-company/reference validation. */
bool AcceptLoadedAsteroidKnowledge()
{
	if (!_staged_knowledge) return IsSavegameVersionBefore(SaveLoadVersion::SpaceSurveyKnowledge);
	if (!InstallLoadedAsteroidKnowledge(*_staged_knowledge)) return false;
	_staged_knowledge.reset(); return true;
}
/** Validate sparse survey receipts against bounded preview counts without granting current-world disclosure. */
void ValidateAsteroidKnowledgeLoadPreview()
{
	if (!_load_check_data.asteroid_knowledge_present) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceSurveyKnowledge)) SlErrorCorrupt("Missing asteroid survey knowledge family");
		return;
	}
	const auto &rows = _load_check_data.asteroid_knowledge_rows;
	if (!ValidateAsteroidKnowledgeRecords(rows, _load_check_data.asteroid_body_count) || (!rows.empty() &&
		(!_load_check_data.asteroid_active || _load_check_data.gameplay_profile_id != to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry)))) SlErrorCorrupt("Invalid asteroid survey knowledge preview");
	for (const auto &r : rows) for (uint company = 0; company < MAX_COMPANIES; ++company)
		if ((r.surveyed_companies & (uint64_t{1} << company)) != 0 && !_load_check_data.companies.contains(company)) SlErrorCorrupt("Missing company in asteroid survey knowledge preview");
}
/** Sole sparse completed-survey knowledge owner; resource records remain exclusively in ASTR. */
struct SPKNChunkHandler : ChunkHandler {
	/** Bind the existing completed-survey knowledge chunk. */
	SPKNChunkHandler() : ChunkHandler("SPKN", ChunkType::Riff) {}
	/** Save only validated completed-survey disclosures in stable body-ID order. */
	void Save() const override
	{
		if (!ValidateAsteroidKnowledge()) SlErrorCorrupt("Invalid asteroid survey knowledge on save");
		const auto &rows = GetAsteroidField().knowledge; SlSetLength(2 + 12 * rows.size()); WriteUnsigned(rows.size(), 2);
		for (const auto &r : rows) { WriteUnsigned(r.body.base(), 4); WriteUnsigned(r.surveyed_companies, 8); }
	}
	/** @return Bounded structural disclosure rows; missing native company ownership is checked before publication. */
	std::vector<AsteroidKnowledgeRecord> Read() const
	{
		const auto length = SlGetFieldLength(); const uint32_t maximum = GetMultiLayerAsteroidPopulation(4096, 4096, 2);
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceSurveyKnowledge) || length < 2 || length > 2 + 12 * maximum) SlErrorCorrupt("Invalid asteroid survey knowledge length");
		const auto count = ReadUnsigned(2); if (length != 2 + 12 * count || count > maximum) SlErrorCorrupt("Invalid asteroid survey knowledge count");
		std::vector<AsteroidKnowledgeRecord> rows;
		for (uint i = 0; i < count; ++i) rows.push_back({OrbitalBodyID{static_cast<uint32_t>(ReadUnsigned(4))}, ReadUnsigned(8)});
		if (!ValidateAsteroidKnowledgeRecords(rows, maximum)) SlErrorCorrupt("Invalid asteroid survey knowledge transport");
		return rows;
	}
	/** Stage one disclosure owner; a repeated family cannot replace earlier loaded authority. */
	void Load() const override { if (_staged_knowledge) SlErrorCorrupt("Repeated asteroid survey knowledge family"); _staged_knowledge = this->Read(); }
	/** Validate disclosure references against the staged field/profile rather than creating missing companies or body records. */
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		if (!_staged_knowledge) { if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceSurveyKnowledge)) SlErrorCorrupt("Missing asteroid survey knowledge family"); return; }
		const auto *schema = GetPendingMultiLayerStateSchema();
		if (!_staged_knowledge->empty() && (schema == nullptr || schema->profile.id != MultiLayerGameplayProfileID::MultilayerIndustry || !_staged_asteroids ||
			!ValidateAsteroidKnowledgeRecords(*_staged_knowledge, static_cast<uint32_t>(_staged_asteroids->bodies.size())))) SlErrorCorrupt("Invalid asteroid survey knowledge authority");
	}
	/** @param length Dispatcher framing; Read validates exact bounded row counts before preview publication. */
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		if (_load_check_data.asteroid_knowledge_present) SlErrorCorrupt("Repeated asteroid survey knowledge preview");
		_load_check_data.asteroid_knowledge_rows = this->Read(); _load_check_data.asteroid_knowledge_present = true;
	}
};
static const ASTRChunkHandler ASTR;
static const SPKNChunkHandler SPKN;
static const ChunkHandlerRef asteroid_handlers[] = {ASTR, SPKN};
extern const ChunkHandlerTable _asteroid_chunk_handlers(asteroid_handlers);
