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
#include "asteroid_generation.h"
#include "vehicle_type.h"
#include "multilayer_content.h"
#include "company_type.h"
#include <optional>

struct DrawPixelInfo;
struct Viewport;
/** Saved single-resource class order, independent of sprites and runtime cargo slots. */
enum class AsteroidClass : uint8_t { Metallic, Carbonaceous, Rich };
/** Saved visual/resource size order; catalogue records define the immutable multipliers. */
enum class AsteroidSize : uint8_t { Small, Medium, Large };
/** Saved new-world population column; existing fields never change when local settings change. */
enum class AsteroidDensity : uint8_t { Sparse, Normal, Dense };

/** Sole saved finite resource record; derived buckets and renderer state have no extraction authority. */
struct AsteroidBody {
	OrbitalBodyID id{0};
	AsteroidClass resource_class = AsteroidClass::Metallic;
	AsteroidSize size = AsteroidSize::Small;
	SpaceMotion motion;
	uint32_t initial_reserve = 0, remaining_reserve = 0;
	SpaceSurveyID survey_slot = INVALID_SPACE_SURVEY;
	VehicleID mining_slot = VehicleID::Invalid(); ///< Sole native probe lease; validated against VEHS after loading.
	/** @param other Saved body to compare. @return Whether resource, motion and lease authority agree. */
	bool operator==(const AsteroidBody &other) const = default;
};

/** One sparse disclosure receipt per body; company removal clears its bit before ID reuse. */
struct AsteroidKnowledgeRecord {
	OrbitalBodyID body{0};
	uint64_t surveyed_companies = 0;
	/** @param other Saved disclosure to compare. @return Whether the same companies know the same body. */
	bool operator==(const AsteroidKnowledgeRecord &other) const = default;
};
/** Authorized quantities from the sole resource record; absent authorization reveals no amounts. */
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
	AsteroidStarterInput starter_input; ///< Historical preference/site/bound; legacy input remains explicitly unknown.
	std::vector<AsteroidBody> bodies;
	std::vector<SpaceSurveyPreview> surveys;
	std::vector<AsteroidKnowledgeRecord> knowledge; ///< Completed native scans only; ownerless preview surveys grant none.
	/** @param other Saved field to compare. @return Whether all gameplay records and original generation inputs agree. */
	bool operator==(const AsteroidField &other) const = default;
};

/** @return Sole persistent field, excluding disposable viewport and spatial authority. */
const AsteroidField &GetAsteroidField();
/** @param id This world's stable generated body ID. @return Mutable owner record, or nullptr for a missing body. */
AsteroidBody *GetMutableAsteroidBody(OrbitalBodyID id);
/** @return Whether retiring ownerless preview leases leaves the real field valid. */
bool RetireSpaceSurveyPreview();
/** @param field Saved field to compare. @return Stable gameplay digest including any recorded initial inputs. */
uint64_t ComputeAsteroidFieldDigest(const AsteroidField &field);
/**
 * Generate exactly the requested field using independent feature streams and an explicit starter preference.
 * @param width Supported Surface-plane width.
 * @param height Supported Surface-plane height.
 * @param seed Saved feature seed; no global RNG draw is permitted.
 * @param density Saved population column.
 * @param starter Optional verified world input; absent input creates a PureField centre preference.
 * @return Complete saved records, or unsupported geometry/input failure.
 */
std::optional<AsteroidField> GenerateAsteroidField(uint32_t width, uint32_t height, uint64_t seed, AsteroidDensity density,
	std::optional<AsteroidStarterInput> starter = std::nullopt);
/** @param field Complete staged field. @return Whether resource, motion, lease and original-input authority are valid. */
bool ValidateAsteroidField(const AsteroidField &field);
/** @param field Complete records transferred to the sole owner. @return Whether validation and derived-index rebuilding succeeded. */
bool InstallAsteroidField(AsteroidField field);
/** Clear field, derived index and pending load staging before initializing another world. */
void ResetAsteroidField();
/** @param world_seed New-world seed used to derive the independent feature seed. @return Whether Classic needs no field or Industry installed its exact field. */
bool GenerateWorldAsteroids(uint32_t world_seed);
/** @return Whether the three starter resources accepted ownerless observation previews. */
bool PrepareSpaceSurveyPreview();
/** @param field Persistent field advancing by one synchronized body/preview work tick. */
void AdvanceAsteroidField(AsteroidField &field);
/** Advance the live field and refresh disposable lookup buckets from its new positions. */
void TickAsteroidField();
/** Publish every actual mining lease's translation after body/index motion and before any native craft work. */
void UpdateAttachedSpaceCraftTransforms();
/** @param dpi Actual viewport drawing region; drawing grants no resource or transfer permission. */
void ViewportAddSpaceEntities(const DrawPixelInfo &dpi);
/** @param viewport Actual clicked viewport. @param x Screen x coordinate. @param y Screen y coordinate. @return Whether a real Space entity handled the click. */
bool HandleSpaceEntityClick(const Viewport &viewport, int x, int y);
/** Show the ownerless observation preview inspector; it cannot reveal native survey-gated quantities. */
void ShowSpaceSurveyWindow();
/** @param craft Actual generation-qualified native hull to inspect. */
void ShowSpaceSurveyWindow(SpaceCraftRef craft);
/** Retarget an already selected native hull after actual paid replacement; opening/selection of unrelated inspectors remains with their current UI owner.
 * @param source Exact retired original incarnation, including its generation.
 * @param product Actual newly paid deployed incarnation to inspect.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC
 */
void RetargetSpaceVehicleInspector(SpaceCraftRef source, SpaceCraftRef product);
/** Invalidate Space viewports after changed derived render state without advancing simulation. */
void RefreshSpaceViewports();
/** @param min Inclusive projected minimum. @param max Inclusive projected maximum. @return Stable body IDs from disposable lookup buckets. */
std::vector<OrbitalBodyID> QueryAsteroidBodies(SpacePoint min, SpacePoint max);

/** @param field Complete decoded records; staging does not mutate the running field or regenerate bodies. */
void StageLoadedAsteroidField(AsteroidField field);
/** @return Whether complete loaded field authority was accepted without rerolling generated content. */
bool AcceptLoadedAsteroidField();
/** Validate read-only preview metadata without changing the accepted world. */
void ValidateAsteroidLoadPreview();
/** Clear pending decoded field/knowledge records before a different load operation. */
void ResetAsteroidLoadStaging();
/** @param company Requesting native company. @param body Stable generated resource ID. @return Quantities only for a completed authorized survey. */
std::optional<AsteroidQuantityDisclosure> GetAsteroidQuantityDisclosure(CompanyID company, OrbitalBodyID body);
/** @param vehicle Actual deployed survey hull with completed native work. @return Whether it gained a disclosure receipt. */
bool CompleteAsteroidSurvey(const SpaceVehicle &vehicle);
/** @param old_owner Company relinquishing completed surveys. @param new_owner Acquirer, or a non-company identity to remove knowledge. */
void ChangeAsteroidKnowledgeOwnership(Owner old_owner, Owner new_owner);
/** @param records Saved strictly ordered disclosures. @param body_count Maximum referenced body count. @return Structural validity without live-company lookup. */
bool ValidateAsteroidKnowledgeRecords(std::span<const AsteroidKnowledgeRecord> records, uint32_t body_count);
/** @return Whether live disclosures reference only surviving native companies. */
bool ValidateAsteroidKnowledge();
/** @param records Fully decoded disclosures. @return Whether structural/live-company validation allowed publication. */
bool InstallLoadedAsteroidKnowledge(std::span<const AsteroidKnowledgeRecord> records);
/** @return Whether staged survey knowledge was accepted without leaking disclosure to missing companies. */
bool AcceptLoadedAsteroidKnowledge();
/** Check decoded survey knowledge in read-only preview state, without publishing it to gameplay. */
void ValidateAsteroidKnowledgeLoadPreview();

#endif /* ASTEROID_H */
