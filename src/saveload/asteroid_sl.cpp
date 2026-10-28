/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file asteroid_sl.cpp Bounded lifecycle field and sparse disclosure transport with independent read-only preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups12/22/26; ID-ordered bodies, complete renewal, same-file disclosure and source-qualified ownerless departure.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
 */
#include "../stdafx.h"
#include "saveload.h"
#include "saveload_error.hpp"
#include "../asteroid.h"
#include "asteroid_field_sl.h"
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
	// Count is only framing. Exact candidate IDs/classes and its own issuance
	// supply membership; an inactive family cannot smuggle retained body metadata.
	if (_load_check_data.asteroid_body_count != _load_check_data.asteroid_body_rows.size() ||
		!ValidateAsteroidIdentityRecords(_load_check_data.asteroid_body_rows, _load_check_data.asteroid_next_body_id) ||
		(!_load_check_data.asteroid_active && (!_load_check_data.asteroid_body_rows.empty() || _load_check_data.asteroid_next_body_id != 0))) SlErrorCorrupt("Invalid asteroid preview identities");
}

/** Sole asteroid field chunk owner; generated records and original access evidence load without rerolling terrain or RNG. */
struct ASTRChunkHandler : ChunkHandler {
	/** Bind the existing bounded RIFF owner; starter inputs extend its payload rather than creating a duplicate registry. */
	ASTRChunkHandler() : ChunkHandler("ASTR", ChunkType::Riff) {}
	/** Save the current original field through its canonical bounded payload adapter. */
	void Save() const override
	{
		const auto &field = GetAsteroidField();
		CheckField(&field, Map::SizeX(), Map::SizeY(), CaptureMultiLayerStateSchema().feature_flags, CaptureMultiLayerStateSchema().orbital_seed);
		const auto encoded = EncodeAsteroidField(field);
		if (!encoded) SlErrorCorrupt("Invalid current asteroid field on save");
		SlSetLength(encoded->size());
		// The complete field was validated before writing. Original scalar byte
		// order is retained; no derived lookup, host struct or optional is copied.
		for (const auto byte : *encoded) SlWriteByte(byte);
	}
	/** @return Complete validated original field; genuinely older layouts upgrade only their absent lifecycle fields. */
	AsteroidField Read() const
	{
		if (IsSavegameVersionBefore(SaveLoadVersion::AsteroidField)) SlErrorCorrupt("Unexpected asteroid family in old save");
		const auto era = IsSavegameVersionBefore(SaveLoadVersion::SpaceMiningVehicles) ? AsteroidFieldWireEra::Preview :
			IsSavegameVersionBefore(SaveLoadVersion::AsteroidStarterInputs) ? AsteroidFieldWireEra::NativeMining :
			IsSavegameVersionBefore(SaveLoadVersion::AsteroidFieldLifecycle) ? AsteroidFieldWireEra::StarterInputs :
			IsSavegameVersionBefore(SaveLoadVersion::SpaceSurveyDeparture) ? AsteroidFieldWireEra::Lifecycle : AsteroidFieldWireEra::SurveyDeparture;
		const auto length = SlGetFieldLength();
		// Bound transport allocation first; the pure decoder then validates exact
		// counts and every row before allocating any body/survey records.
		if (length < 51 || length > GetAsteroidFieldWireMaximum(era)) SlErrorCorrupt("Invalid asteroid family length");
		std::vector<uint8_t> bytes(length);
		for (auto &byte : bytes) byte = SlReadByte();
		auto decoded = DecodeAsteroidField(bytes, era);
		if (!decoded) SlErrorCorrupt("Invalid asteroid field lifecycle or record");
		return std::move(*decoded);
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
		_load_check_data.asteroid_next_body_id = field.next_body_id;
		_load_check_data.asteroid_body_rows = GetAsteroidIdentityRecords(field);
		// Preserve only original occupied leases for the later bidirectional
		// VEHS/SPOR join. Copied identity alone cannot prove active extraction.
		for (const auto *body : GetAsteroidBodiesInIDOrder(field)) {
			if (body->mining_slot != VehicleID::Invalid()) _load_check_data.asteroid_mining_leases.emplace_back(body->id, body->mining_slot);
		}
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
	if (!ValidateAsteroidKnowledgeRecords(rows, _load_check_data.asteroid_body_rows) || (!rows.empty() &&
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
		const auto length = SlGetFieldLength();
		const bool lifecycle = !IsSavegameVersionBefore(SaveLoadVersion::AsteroidFieldLifecycle);
		const uint32_t maximum = lifecycle ? MAX_ORBITAL_BODIES : GetMultiLayerAsteroidPopulation(4096, 4096, 2);
		if (IsSavegameVersionBefore(SaveLoadVersion::SpaceSurveyKnowledge) || length < 2 || length > 2 + 12 * maximum) SlErrorCorrupt("Invalid asteroid survey knowledge length");
		const auto count = ReadUnsigned(2); if (length != 2 + 12 * count || count > maximum) SlErrorCorrupt("Invalid asteroid survey knowledge count");
		std::vector<AsteroidKnowledgeRecord> rows; rows.reserve(static_cast<size_t>(count));
		for (uint i = 0; i < count; ++i) rows.push_back({OrbitalBodyID{static_cast<uint32_t>(ReadUnsigned(4))}, ReadUnsigned(8)});
		// Modern structural admission accepts sorted full-width issued domains;
		// subsequent native/copied joins require each actual same-file live body.
		// Genuine older files retain their original dense transport bound.
		if (lifecycle ? !ValidateAsteroidKnowledgeShape(rows) : !ValidateAsteroidKnowledgeRecords(rows, maximum)) SlErrorCorrupt("Invalid asteroid survey knowledge transport");
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
			!ValidateAsteroidKnowledgeRecords(*_staged_knowledge, *_staged_asteroids))) SlErrorCorrupt("Invalid asteroid survey knowledge authority");
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
