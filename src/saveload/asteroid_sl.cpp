/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file asteroid_sl.cpp Bounded404 asteroid/survey transport; independent read-only preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
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

void ResetAsteroidLoadStaging() { _staged_asteroids.reset(); }
void StageLoadedAsteroidField(AsteroidField field) { _staged_asteroids = std::move(field); }

static void WriteUnsigned(uint64_t value, uint8_t width)
{
	for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) SlWriteByte(static_cast<uint8_t>(value >> shift));
}
static uint64_t ReadUnsigned(uint8_t width)
{
	uint64_t value = 0; while (width-- != 0) value = (value << 8) | SlReadByte(); return value;
}
static void WritePoint(SpacePoint point) { WriteUnsigned(static_cast<uint32_t>(point.x), 4); WriteUnsigned(static_cast<uint32_t>(point.y), 4); }
static SpacePoint ReadPoint() { return {static_cast<int32_t>(ReadUnsigned(4)), static_cast<int32_t>(ReadUnsigned(4))}; }

/** Validate candidate metadata/geometry together before publishing any orbital state. */
static void CheckField(const AsteroidField *field, uint32_t width, uint32_t height, uint32_t flags, uint64_t seed)
{
	if (field == nullptr) {
		if (!IsSavegameVersionBefore(SaveLoadVersion::AsteroidField) || (flags & MULTILAYER_FEATURE_ASTEROIDS) != 0) SlErrorCorrupt("Missing asteroid field");
		return;
	}
	const bool active = field->generation_version != 0;
	if (active != ((flags & MULTILAYER_FEATURE_ASTEROIDS) != 0) || !ValidateAsteroidField(*field) ||
		(active && (field->width != width || field->height != height || field->seed != seed))) SlErrorCorrupt("Invalid asteroid field authority");
}

bool AcceptLoadedAsteroidField()
{
	if (!_staged_asteroids) return IsSavegameVersionBefore(SaveLoadVersion::AsteroidField);
	if (!InstallAsteroidField(std::move(*_staged_asteroids))) return false;
	_staged_asteroids.reset(); return true;
}

void ValidateAsteroidLoadPreview()
{
	if (!_load_check_data.asteroid_present && !IsSavegameVersionBefore(SaveLoadVersion::AsteroidField)) SlErrorCorrupt("Missing asteroid preview family");
	if (_load_check_data.asteroid_active != ((_load_check_data.multilayer_feature_flags & MULTILAYER_FEATURE_ASTEROIDS) != 0)) SlErrorCorrupt("Asteroid preview feature mismatch");
	if (_load_check_data.asteroid_active && _load_check_data.asteroid_seed != _load_check_data.orbital_seed) SlErrorCorrupt("Asteroid preview seed mismatch");
}

struct ASTRChunkHandler : ChunkHandler {
	ASTRChunkHandler() : ChunkHandler("ASTR", ChunkType::Riff) {}
	void Save() const override
	{
		const auto &field = GetAsteroidField();
		CheckField(&field, Map::SizeX(), Map::SizeY(), CaptureMultiLayerStateSchema().feature_flags, CaptureMultiLayerStateSchema().orbital_seed);
		SlSetLength(51 + 40 * field.bodies.size() + 34 * field.surveys.size());
		WriteUnsigned(field.generation_version, 2); WriteUnsigned(field.width, 4); WriteUnsigned(field.height, 4);
		WriteUnsigned(field.seed, 8); WriteUnsigned(field.elapsed_ticks, 8); WriteUnsigned(to_underlying(field.density), 1);
		WriteUnsigned(field.bodies.size(), 4); WriteUnsigned(field.surveys.size(), 4);
		WritePoint(field.band.min); WritePoint(field.band.max);
		for (const auto &body : field.bodies) {
			WriteUnsigned(body.id.base(), 4); WriteUnsigned(to_underlying(body.resource_class), 1); WriteUnsigned(to_underlying(body.size), 1);
			WritePoint(body.motion.position); WriteUnsigned(static_cast<uint32_t>(body.motion.velocity_x), 4); WriteUnsigned(static_cast<uint32_t>(body.motion.velocity_y), 4);
			WriteUnsigned(body.motion.remainder_x, 1); WriteUnsigned(body.motion.remainder_y, 1);
			WriteUnsigned(body.initial_reserve, 4); WriteUnsigned(body.remaining_reserve, 4); WriteUnsigned(body.survey_slot.base(), 4); WriteUnsigned(body.mining_slot.base(), 4);
		}
		for (const auto &survey : field.surveys) {
			WriteUnsigned(survey.id.base(), 4); WriteUnsigned(survey.model, 2); WritePoint(survey.position); WritePoint(survey.home);
			WriteUnsigned(survey.target.base(), 4); WriteUnsigned(to_underlying(survey.phase), 1); WriteUnsigned(survey.remaining_ticks, 2);
			WriteUnsigned(survey.travel_remainder, 1); WriteUnsigned(survey.completed_surveys, 4);
		}
	}
	AsteroidField Read() const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::AsteroidField)) SlErrorCorrupt("Unexpected asteroid family in old save");
		const auto length = SlGetFieldLength();
		const uint32_t body_size = IsSavegameVersionBefore(SaveLoadVersion::SpaceMiningVehicles) ? 36 : 40;
		if (length < 51 || length > 51 + body_size * MAX_ORBITAL_BODIES + 34 * MAX_SPACE_SURVEY_PREVIEWS) SlErrorCorrupt("Invalid asteroid family length");
		AsteroidField field;
		field.generation_version = ReadUnsigned(2); field.width = ReadUnsigned(4); field.height = ReadUnsigned(4);
		field.seed = ReadUnsigned(8); field.elapsed_ticks = ReadUnsigned(8); field.density = static_cast<AsteroidDensity>(ReadUnsigned(1));
		const auto bodies = ReadUnsigned(4), surveys = ReadUnsigned(4);
		if (bodies > MAX_ORBITAL_BODIES || surveys > MAX_SPACE_SURVEY_PREVIEWS || length != 51 + bodies * body_size + surveys * 34) SlErrorCorrupt("Invalid asteroid record count");
		field.band.min = ReadPoint(); field.band.max = ReadPoint();
		field.bodies.resize(static_cast<size_t>(bodies)); field.surveys.resize(static_cast<size_t>(surveys));
		for (auto &body : field.bodies) {
			body.id = OrbitalBodyID{static_cast<uint32_t>(ReadUnsigned(4))}; body.resource_class = static_cast<AsteroidClass>(ReadUnsigned(1)); body.size = static_cast<AsteroidSize>(ReadUnsigned(1));
			body.motion.position = ReadPoint(); body.motion.velocity_x = static_cast<int32_t>(ReadUnsigned(4)); body.motion.velocity_y = static_cast<int32_t>(ReadUnsigned(4));
			body.motion.remainder_x = ReadUnsigned(1); body.motion.remainder_y = ReadUnsigned(1);
			body.initial_reserve = ReadUnsigned(4); body.remaining_reserve = ReadUnsigned(4); body.survey_slot = SpaceSurveyID{static_cast<uint32_t>(ReadUnsigned(4))};
			if (!IsSavegameVersionBefore(SaveLoadVersion::SpaceMiningVehicles)) body.mining_slot = VehicleID{static_cast<uint32_t>(ReadUnsigned(4))};
		}
		for (auto &survey : field.surveys) {
			survey.id = SpaceSurveyID{static_cast<uint32_t>(ReadUnsigned(4))}; survey.model = ReadUnsigned(2); survey.position = ReadPoint(); survey.home = ReadPoint();
			survey.target = OrbitalBodyID{static_cast<uint32_t>(ReadUnsigned(4))}; survey.phase = static_cast<SpaceSurveyPhase>(ReadUnsigned(1)); survey.remaining_ticks = ReadUnsigned(2);
			survey.travel_remainder = ReadUnsigned(1); survey.completed_surveys = ReadUnsigned(4);
		}
		if (!ValidateAsteroidField(field)) SlErrorCorrupt("Invalid asteroid/survey record");
		return field;
	}
	void Load() const override { StageLoadedAsteroidField(this->Read()); }
	void FixPointers() const override
	{
		if (!SlIsFixingPointers()) return;
		const auto *metadata = GetPendingMultiLayerStateSchema();
		CheckField(_staged_asteroids ? &*_staged_asteroids : nullptr, Map::SizeX(), Map::SizeY(), metadata == nullptr ? 0 : metadata->feature_flags, metadata == nullptr ? 0 : metadata->orbital_seed);
	}
	void LoadCheck([[maybe_unused]] size_t length) const override
	{
		const auto field = this->Read();
		if (field.generation_version != 0 && (field.width != _load_check_data.map_size_x || field.height != _load_check_data.map_size_y || _load_check_data.map_layers < 3)) SlErrorCorrupt("Asteroid preview geometry mismatch");
		_load_check_data.asteroid_present = true; _load_check_data.asteroid_active = field.generation_version != 0; _load_check_data.asteroid_seed = field.seed;
	}
};
static const ASTRChunkHandler ASTR;
static const ChunkHandlerRef asteroid_handlers[] = {ASTR};
extern const ChunkHandlerTable _asteroid_chunk_handlers(asteroid_handlers);
