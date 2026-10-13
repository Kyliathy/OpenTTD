/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file asteroid.h Sole persistent asteroid resource/field owner and survey preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md GroupP1
 */
#ifndef ASTEROID_H
#define ASTEROID_H
#include "space.h"
#include "vehicle_type.h"
#include "multilayer_content.h"
#include "company_type.h"
#include <optional>

struct DrawPixelInfo;
struct Viewport;
enum class AsteroidClass : uint8_t { Metallic, Carbonaceous, Rich };
enum class AsteroidSize : uint8_t { Small, Medium, Large };
enum class AsteroidDensity : uint8_t { Sparse, Normal, Dense };

struct AsteroidBody {
	OrbitalBodyID id{0};
	AsteroidClass resource_class = AsteroidClass::Metallic;
	AsteroidSize size = AsteroidSize::Small;
	SpaceMotion motion;
	uint32_t initial_reserve = 0, remaining_reserve = 0;
	SpaceSurveyID survey_slot = INVALID_SPACE_SURVEY;
	VehicleID mining_slot = VehicleID::Invalid(); ///< Sole native probe lease; validated against VEHS after loading.
	bool operator==(const AsteroidBody &) const = default;
};

/** One sparse disclosure receipt per body; company removal clears its bit before ID reuse. */
struct AsteroidKnowledgeRecord {
	OrbitalBodyID body{0};
	uint64_t surveyed_companies = 0;
	bool operator==(const AsteroidKnowledgeRecord &) const = default;
};
struct AsteroidQuantityDisclosure { uint32_t remaining = 0, initial = 0; };
struct SpaceVehicle;
struct SpaceCraftRef;

/** Persist generated records, never regenerate from current content defaults on reload. */
struct AsteroidField {
	uint16_t generation_version = 0;
	uint32_t width = 0, height = 0;
	uint64_t seed = 0, elapsed_ticks = 0;
	AsteroidDensity density = AsteroidDensity::Normal;
	SpaceTravelBand band;
	std::vector<AsteroidBody> bodies;
	std::vector<SpaceSurveyPreview> surveys;
	std::vector<AsteroidKnowledgeRecord> knowledge; ///< Completed native scans only; ownerless preview surveys grant none.
	bool operator==(const AsteroidField &) const = default;
};

const AsteroidField &GetAsteroidField();
AsteroidBody *GetMutableAsteroidBody(OrbitalBodyID id);
bool RetireSpaceSurveyPreview();
uint64_t ComputeAsteroidFieldDigest(const AsteroidField &field);
std::optional<AsteroidField> GenerateAsteroidField(uint32_t width, uint32_t height, uint64_t seed, AsteroidDensity density);
bool ValidateAsteroidField(const AsteroidField &field);
bool InstallAsteroidField(AsteroidField field);
void ResetAsteroidField();
bool GenerateWorldAsteroids(uint32_t world_seed);
bool PrepareSpaceSurveyPreview();
void AdvanceAsteroidField(AsteroidField &field);
void TickAsteroidField();
void ViewportAddSpaceEntities(const DrawPixelInfo &dpi);
bool HandleSpaceEntityClick(const Viewport &viewport, int x, int y);
void ShowSpaceSurveyWindow();
void ShowSpaceSurveyWindow(SpaceCraftRef craft);
void RefreshSpaceViewports();
std::vector<OrbitalBodyID> QueryAsteroidBodies(SpacePoint min, SpacePoint max);

/** Load transport hooks; previews never borrow or mutate the running field. */
void StageLoadedAsteroidField(AsteroidField field);
bool AcceptLoadedAsteroidField();
void ValidateAsteroidLoadPreview();
void ResetAsteroidLoadStaging();
/** Sole player-facing quantity query. Invalid/spectator/unsurveyed companies see no amounts. */
std::optional<AsteroidQuantityDisclosure> GetAsteroidQuantityDisclosure(CompanyID company, OrbitalBodyID body);
bool CompleteAsteroidSurvey(const SpaceVehicle &vehicle);
void ChangeAsteroidKnowledgeOwnership(Owner old_owner, Owner new_owner);
bool ValidateAsteroidKnowledgeRecords(std::span<const AsteroidKnowledgeRecord> records, uint32_t body_count);
bool ValidateAsteroidKnowledge();
bool InstallLoadedAsteroidKnowledge(std::span<const AsteroidKnowledgeRecord> records);
bool AcceptLoadedAsteroidKnowledge();
void ValidateAsteroidKnowledgeLoadPreview();

#endif /* ASTEROID_H */
