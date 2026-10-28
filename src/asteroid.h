/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file asteroid.h Sole persistent asteroid resource/field owner and survey preview.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups9/11-12; exit forecast, renewal accounting and sparse identity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md GroupP1
 */
#ifndef ASTEROID_H
#define ASTEROID_H
#include "space.h"
#include "core/random_func.hpp"
#include <array>
#include <span>
#include <utility>
#include <unordered_map>
#include "asteroid_generation.h"
#include "vehicle_type.h"
#include "multilayer_content.h"
#include "company_type.h"
#include <optional>

struct DrawPixelInfo;
struct Viewport;
struct SpaceAttachmentObstacle;
/** Saved single-resource class order, independent of sprites and runtime cargo slots. */
enum class AsteroidClass : uint8_t { Metallic, Carbonaceous, Rich };
/** Saved visual/resource size order; catalogue records define the immutable multipliers. */
enum class AsteroidSize : uint8_t { Small, Medium, Large };
/** Saved new-world population column; existing fields never change when local settings change. */
enum class AsteroidDensity : uint8_t { Sparse, Normal, Dense };
inline constexpr uint16_t ASTEROID_LIFECYCLE_VERSION = 1; ///< Saved sparse/terminal-history contract; independent of original generation provenance and content revision.

/** Latest terminal exit's size/mineral; it confers no reserve, packet or historical live-body identity. */
struct AsteroidExitMetadata {
	AsteroidSize size = AsteroidSize::Small;
	AsteroidClass resource_class = AsteroidClass::Metallic;
	/** @param other Latest-exit metadata to compare. @return Whether size and resource class agree. */
	bool operator==(const AsteroidExitMetadata &other) const = default;
};
/** Copied terminal event; the field owner must prove that this original body is still live before retiring it. */
struct AsteroidExitRecord {
	OrbitalBodyID body{0};
	AsteroidExitMetadata metadata;
};
/** Contained renewal accounting, distinct from finite cargo reserve and monotonic body identity issuance. */
struct AsteroidRenewalState {
	bool initialized = false; ///< Default is explicit absence, not a seeded RNG stream.
	std::array<uint64_t, 3> exited_by_size{}; ///< Cumulative terminal exits, never decremented when credits are spent.
	uint64_t credits = 0; ///< Pending dedicated size weight, never a cargo quantity or company balance.
	std::optional<AsteroidExitMetadata> last_exit;
	uint32_t ticks_remaining = 0; ///< Zero is disarmed with no credit, or due while credit remains.
	Randomizer timer_rng{}, candidate_rng{}; ///< Both words of each independent stream belong to this saved owner.
	/** @param other Renewal state to compare. @return Whether counters, deadline, metadata and both complete streams agree. */
	bool operator==(const AsteroidRenewalState &other) const
	{
		return this->initialized == other.initialized && this->exited_by_size == other.exited_by_size && this->credits == other.credits &&
			this->last_exit == other.last_exit && this->ticks_remaining == other.ticks_remaining &&
			this->timer_rng.state[0] == other.timer_rng.state[0] && this->timer_rng.state[1] == other.timer_rng.state[1] &&
			this->candidate_rng.state[0] == other.candidate_rng.state[0] && this->candidate_rng.state[1] == other.candidate_rng.state[1];
	}
};
/** One pure due-interval proposal; later body generation uses its ticket without drawing further from the saved stream. */
struct AsteroidRenewalAttempt {
	std::optional<AsteroidSize> size; ///< No size when no different affordable size exists.
	uint32_t ticket = 0; ///< Exactly one candidate-stream draw per due attempt, including unsuccessful attempts.
	Randomizer next_candidate_rng{}; ///< Copied complete stream after that one draw; published only by Finish.
	/** @param other Due proposal to compare. @return Whether selection, ticket and resulting saved stream agree. */
	bool operator==(const AsteroidRenewalAttempt &other) const
	{
		return this->size == other.size && this->ticket == other.ticket && this->next_candidate_rng.state[0] == other.next_candidate_rng.state[0] &&
			this->next_candidate_rng.state[1] == other.next_candidate_rng.state[1];
	}
};
/** @param size Canonical visual/resource size. @return Dedicated renewal cost 1/3/5, or absence for an unknown size. */
std::optional<uint32_t> GetAsteroidRenewalCost(AsteroidSize size);
/** @return Inclusive simulation-tick delay bounds for 20-60 seconds at normal speed, rounded inward using MILLISECONDS_PER_TICK. */
std::pair<uint32_t, uint32_t> GetAsteroidRenewalDelayBounds();
/** @param seed Saved field feature seed. @return Pristine explicitly seeded streams using independent domains 4 and 5; global RNG and generation domains 1/2/3 are untouched. */
AsteroidRenewalState MakeAsteroidRenewalState(uint64_t seed);
/** @param state Copied initialized ledger. @param issued_body_ids Total original IDs issued before this operation, including retired IDs. @return Whether bounded history, credit, timer and metadata are internally admissible. */
bool ValidateAsteroidRenewalState(const AsteroidRenewalState &state, uint32_t issued_body_ids);
/**
 * Plan one terminal batch in stable body-ID order without mutating the input or any field.
 * @param state Original initialized ledger.
 * @param exits Original live bodies preflighted by the caller; duplicates within this batch are refused.
 * @param issued_body_ids Total IDs issued before this operation; the field owner prevents historical double retirement.
 * @return Complete next ledger or refusal. An empty-to-positive balance arms once; later exits preserve its deadline.
 */
std::optional<AsteroidRenewalState> ApplyAsteroidExitCredit(const AsteroidRenewalState &state, std::span<const AsteroidExitRecord> exits, uint32_t issued_body_ids);
/** @param state Original initialized ledger. @param issued_body_ids Original issuance count. @return One ordinary opportunity's countdown, or refusal; callers skip this function while paused and on the first-credit arming opportunity. */
std::optional<AsteroidRenewalState> AdvanceAsteroidRenewalTimer(const AsteroidRenewalState &state, uint32_t issued_body_ids);
/** @param state Original due ledger with positive credits and zero remaining ticks. @param issued_body_ids Original issuance count. @return One deterministic copied candidate ticket/size, or refusal outside a due interval; repeated queries do not publish RNG changes. */
std::optional<AsteroidRenewalAttempt> BeginAsteroidRenewalAttempt(const AsteroidRenewalState &state, uint32_t issued_body_ids);
/**
 * Close exactly one due attempt without inserting a body or mutating the original ledger.
 * @param state Original due ledger.
 * @param attempt Exact proposal from that ledger; stale or modified proposals are refused.
 * @param spawned True only after the caller admits actual new-body creation; failure retains all credit.
 * @param issued_body_ids Issuance count before the attempted creation; an exhausted cursor forbids success.
 * @return Next ledger with one candidate draw published and, if needed, a freshly sampled timer. Debit occurs only for success.
 */
std::optional<AsteroidRenewalState> FinishAsteroidRenewalAttempt(const AsteroidRenewalState &state, const AsteroidRenewalAttempt &attempt, bool spawned, uint32_t issued_body_ids);

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
/** Pure asteroid exit geometry; size/resource/stock never alter its centre trajectory or earn renewal credit here. */
using AsteroidExitForecast = SpaceMotionExitForecast;
/** @param body Actual or copied asteroid whose saved motion is inspected without mutation. @param bounds Explicit physical plane bounds. @return Exact first exit and last-safe motion, or absence for invalid input; stationary results have no exit tick. */
std::optional<AsteroidExitForecast> ForecastAsteroidExit(const AsteroidBody &body, const SpacePlaneBounds &bounds);

/** One sparse disclosure receipt per body; company removal clears its bit before ID reuse. */
struct AsteroidKnowledgeRecord {
	OrbitalBodyID body{0};
	uint64_t surveyed_companies = 0;
	/** @param other Saved disclosure to compare. @return Whether the same companies know the same body. */
	bool operator==(const AsteroidKnowledgeRecord &other) const = default;
};
/** Authorized quantities from the sole resource record; absent authorization reveals no amounts. */
struct AsteroidQuantityDisclosure { uint32_t remaining = 0, initial = 0; };
/** Detached candidate-file identity/class metadata; it owns no reserve, motion, leases or disclosure. */
struct AsteroidIdentityRecord {
	OrbitalBodyID id{0};
	AsteroidClass resource_class = AsteroidClass::Metallic;
	/** @param other Copied identity row. @return Whether stable body ID and resource class agree. */
	bool operator==(const AsteroidIdentityRecord &other) const = default;
};
struct SpaceVehicle;
struct SpaceCraftRef;

/** Read-only installed live storage diagnostics; capacities describe real containers, not allocator-specific total heap bytes. */
struct AsteroidStorageUsage {
	size_t bodies = 0, body_capacity = 0, lookup_rows = 0, lookup_buckets = 0, knowledge = 0, knowledge_capacity = 0;
	SpaceSpatialIndex::StorageUsage index;
};
/** @return Native live field/lookup/knowledge/index occupancy and capacities; observes no historical catalogue and creates no gameplay state. */
AsteroidStorageUsage InspectAsteroidStorage();

/** Persist generated records, never regenerate from current content defaults on reload. */
struct AsteroidField {
	uint16_t generation_version = 0;
	uint16_t lifecycle_version = 0; ///< Zero is genuine legacy absence/inactive ownership; current active fields use ASTEROID_LIFECYCLE_VERSION.
	uint32_t width = 0, height = 0;
	uint64_t seed = 0, elapsed_ticks = 0;
	AsteroidDensity density = AsteroidDensity::Normal;
	SpaceTravelBand band;
	AsteroidStarterInput starter_input; ///< Historical preference/site/bound; legacy input remains explicitly unknown.
	AsteroidRenewalState renewal; ///< Sole renewal owner; active transitions remain gated on the new persistence contract.
	uint32_t next_body_id = 0; ///< Monotonic issuance cursor; UINT32_MAX means exhausted, never a usable body identity.
	std::vector<AsteroidBody> bodies;
	std::vector<SpaceSurveyPreview> surveys;
	std::vector<AsteroidKnowledgeRecord> knowledge; ///< Completed native scans only; ownerless preview surveys grant none.
	/** @param other Saved field to compare. @return Whether all gameplay records and original generation inputs agree. */
	bool operator==(const AsteroidField &other) const;
private:
	std::unordered_map<uint32_t, size_t> body_slots; ///< Disposable live-only lookup; never serialized, hashed or compared as gameplay state.
	friend const AsteroidBody *FindAsteroidBody(const AsteroidField &, OrbitalBodyID);
	friend bool RebuildAsteroidBodyLookup(AsteroidField &);
	friend bool InsertAsteroidBody(AsteroidField &, AsteroidBody);
	friend bool EraseAsteroidBody(AsteroidField &, OrbitalBodyID);
	friend AsteroidStorageUsage InspectAsteroidStorage();
};

/** @param field Copied or installed owner. @param id Stable body identity, never a dense slot. @return Existing body or nullptr; no lookup inserts, rebuilds or changes any record. Borrowed pointers expire after insertion/erasure. */
const AsteroidBody *FindAsteroidBody(const AsteroidField &field, OrbitalBodyID id);
/** @param field Mutable original owner. @param id Stable body identity. @return Existing resource record or nullptr, with the same pointer lifetime as the const overload. */
AsteroidBody *FindAsteroidBody(AsteroidField &field, OrbitalBodyID id);
/** @param field Copied owner whose raw IDs are inspected independently of its disposable map. @return Whether bounded live IDs are unique, issued and distinct from the exhausted sentinel. */
bool ValidateAsteroidBodyIdentities(const AsteroidField &field);
/** @param field Decoded, copied or generated owner. @return Whether raw identities validate and a complete derived map was rebuilt; refusal leaves the old map untouched. */
bool RebuildAsteroidBodyLookup(AsteroidField &field);
/** @param field Owner with a rebuilt map; direct structural edits to public decoded records require RebuildAsteroidBodyLookup first. @param body Complete proposed body whose ID must equal the unissued cursor. @return Whether insertion succeeded and consumed exactly that ID; refusal preserves all records and issuance. Does not update the installed spatial index or ledger. */
bool InsertAsteroidBody(AsteroidField &field, AsteroidBody body);
/** @param field Owner with a rebuilt map, whose caller already released leases and dependent references. @param id Original live identity to erase. @return Whether compact swap/pop erased it without reusing its ID or changing renewal accounting; borrowed body pointers expire. */
bool EraseAsteroidBody(AsteroidField &field, OrbitalBodyID id);
/** @param field Copied or installed owner. @return Borrowed live body pointers sorted by original ID for wire records, digests and simultaneous events; structural mutations invalidate them. */
std::vector<const AsteroidBody *> GetAsteroidBodiesInIDOrder(const AsteroidField &field);
/** @param field Read-only current or copied field. @param selected Previously inspected identity, if any. @return That surviving identity, otherwise the lowest live ID or absence for an empty field; dense storage order never selects a successor. */
std::optional<OrbitalBodyID> ResolveAsteroidSelection(const AsteroidField &field, std::optional<OrbitalBodyID> selected);
/** @param field Original or staged owner. @param id Retiring body's identity. Remove only its disclosure receipt before body erasure; no reserve, credit or other company's records are changed. */
void RemoveAsteroidKnowledge(AsteroidField &field, OrbitalBodyID id);
/** @param field Valid decoded or current field. @return Independent identity/class copies in strict ID order, for candidate-only joins. */
std::vector<AsteroidIdentityRecord> GetAsteroidIdentityRecords(const AsteroidField &field);
/** @param records Candidate rows in serialized ID order. @param next_body_id Candidate lifetime issuance cursor, including its exhausted sentinel. @return Whether rows are bounded, strictly ordered, issued and class-valid without consulting any live world. */
bool ValidateAsteroidIdentityRecords(std::span<const AsteroidIdentityRecord> records, uint32_t next_body_id);
/** @param records Previously validated sorted candidate rows. @param id Exact referenced original identity. @return Matching candidate row or nullptr; no installed field lookup or record creation is performed. */
const AsteroidIdentityRecord *FindAsteroidIdentity(std::span<const AsteroidIdentityRecord> records, OrbitalBodyID id);

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
/** @param field Original or copied field. @param bodies Exact prevalidated natural identities being retired. Release affected ownerless leases/partial scans before erasure and retain current position/home/completed counts. Return/Waiting uses the explicit no-target sentinel; no native craft or company knowledge is created. */
void ForgetSpaceSurveyBodyTargets(AsteroidField &field, std::span<const OrbitalBodyID> bodies);
/** @param field Original pre-translation field. @param obstacles Same world's original static geometry. Release ownerless approach/attachments before their next unsafe offset transform, retaining the existing valid point for ordinary homeward Return. Group29 owns normal-loop activation. */
void ReleaseUnsafeSpaceSurveyPreviews(AsteroidField &field, std::span<const SpaceAttachmentObstacle> obstacles);
/** @param field Original field whose body transforms were already published once. Advance only its original ownerless preview controllers, including no-target home Return/Waiting and deterministic surviving-ID selection. */
void AdvanceSpaceSurveyPreviews(AsteroidField &field);
/** Offer exactly one normal unpaused field opportunity: pre-release unsafe participants, retire sorted outward IDs, move/index survivors, advance previously armed time, commit at most one due replacement, then publish preview/native attachments. Editor/menu/paused calls preserve all owners. New bodies first move next opportunity; fatal storage/invariant refusal cannot silently freeze an outward body. Shared approach/power preparation remains in CallVehicleTicks. */
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
/** @param records Saved strictly ordered disclosures. @return Bounded row shape, valid company-mask width and non-sentinel identities, without any membership or live-company lookup. */
bool ValidateAsteroidKnowledgeShape(std::span<const AsteroidKnowledgeRecord> records);
/** @param records Legacy dense-format disclosures. @param body_count Legacy ID bound, used only by old transport readers/tests. @return Dense legacy structural validity; new live/candidate joins must use membership instead. */
bool ValidateAsteroidKnowledgeRecords(std::span<const AsteroidKnowledgeRecord> records, uint32_t body_count);
/** @param records Saved strictly ordered disclosures. @param field Current or copied owner whose actual live IDs confer membership. @return Whether every well-formed receipt belongs to a surviving body; company existence is checked by the native/candidate owner separately. */
bool ValidateAsteroidKnowledgeRecords(std::span<const AsteroidKnowledgeRecord> records, const AsteroidField &field);
/** @param records Saved sparse disclosure rows. @param bodies Validated sorted identities from the same candidate file. @return Whether each well-formed receipt joins to that candidate's live IDs, independently of current-world records. */
bool ValidateAsteroidKnowledgeRecords(std::span<const AsteroidKnowledgeRecord> records, std::span<const AsteroidIdentityRecord> bodies);
/** @return Whether live disclosures reference only surviving native companies. */
bool ValidateAsteroidKnowledge();
/** @param records Fully decoded disclosures. @return Whether structural/live-company validation allowed publication. */
bool InstallLoadedAsteroidKnowledge(std::span<const AsteroidKnowledgeRecord> records);
/** @return Whether staged survey knowledge was accepted without leaking disclosure to missing companies. */
bool AcceptLoadedAsteroidKnowledge();
/** Check decoded survey knowledge in read-only preview state, without publishing it to gameplay. */
void ValidateAsteroidKnowledgeLoadPreview();

#endif /* ASTEROID_H */
