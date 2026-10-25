/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_content.h Authoritative gameplay profile selection and immutable content requirements. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#ifndef MULTILAYER_CONTENT_H
#define MULTILAYER_CONTENT_H

#include "multilayer_content_identity.h"
#include "landscape_type.h"
#include "map_layer_type.h"
#include "cargotype.h"
#include "timer/timer_game_calendar.h"
#include <array>

struct Randomizer;

/** Immutable single-cargo finite resources, in saved Metallic/Carbonaceous/Rich order. */
struct MultiLayerAsteroidDefinition {
	CargoLabel cargo; ///< Selected-profile meaning; one tonne of reserve produces one cargo unit.
	uint32_t small_min, small_max; ///< Inclusive whole-tonne bounds before the independent size multiplier.
};

/** Immutable sizes in saved Small/Medium/Large order; artwork never determines resource content. */
struct MultiLayerAsteroidSizeDefinition {
	uint8_t chance_percent; ///< Independent generation weight; the three weights sum to 100.
	uint32_t diameter_q16; ///< Visual body diameter in Q16 tiles, separate from its travel-band centre.
	uint8_t mining_slots; ///< One native extraction lease per body in the initial catalogue.
	uint8_t reserve_multiplier; ///< Applied after sampling the inclusive Small reserve range.
};

/** @return Immutable finite resource records used by the Industry profile. */
const std::array<MultiLayerAsteroidDefinition, 3> &GetMultiLayerAsteroidDefinitions();

/** @return Immutable size records used by the Industry profile. */
const std::array<MultiLayerAsteroidSizeDefinition, 3> &GetMultiLayerAsteroidSizeDefinitions();

/**
 * Resolve one percentile against the catalogue's cumulative size weights.
 * @param percentile Independent uniform generation draw in [0, 100).
 * @return Saved size index; callers must provide a valid percentile.
 */
uint8_t SelectMultiLayerAsteroidSize(uint32_t percentile);

/**
 * Sample finite whole-tonne content without consuming simulation or presentation RNG.
 * @param resource Immutable selected-profile class record with inclusive Small bounds.
 * @param size Immutable independent size record whose multiplier follows sampling.
 * @param random Feature-local content generator; advances exactly once.
 * @return Initial reserve in whole tonnes, also used as the initial remaining reserve.
 */
uint32_t SampleMultiLayerAsteroidReserve(const MultiLayerAsteroidDefinition &resource,
	const MultiLayerAsteroidSizeDefinition &size, Randomizer &random);

/**
 * Look up an exact population rather than deriving a density or silently reducing it.
 * @param width Surface-plane width in tiles, a supported power of two.
 * @param height Surface-plane height in tiles, a supported power of two.
 * @param density Saved Sparse/Normal/Dense column (0/1/2).
 * @return Exact body count, or zero for unsupported geometry or density.
 */
uint32_t GetMultiLayerAsteroidPopulation(uint32_t width, uint32_t height, uint8_t density);

/** Stable layer capability mask, using saved layer IDs rather than UI display order. */
using MultiLayerCapabilities = EnumBitSet<MapLayer, uint8_t>;

/** One declared physical form per supported recipe/refit cargo meaning. */
enum class MultiLayerCargoForm : uint8_t {
	Undefined, ///< No definition may infer an unknown cargo's form from its icon.
	DryBulk, ///< Pourable ores, Earth and Rock.
	CoveredGranules, ///< Covered dry mineral concentrate.
	SealedDrums, ///< Sealed hazardous solid mineral load.
	Ingots, ///< Solid structural metal cargo.
	Liquid, ///< Kilolitres in sealed tankers, never container refits.
	Packaged, ///< Protected manufactured goods, retaining the label's own unit.
	FrozenBulk, ///< Refrigerated Water Ice.
};

/** Immutable cargo traits from the selected content definition source. */
struct MultiLayerCargoContract {
	CargoLabel label; ///< Stable meaning; never a runtime slot.
	MultiLayerCargoForm form; ///< Single physical form used by recipe/refit validation.
	std::string_view unit; ///< Physical packet unit, independently of weight/capacity conversion.
};

/**
 * Resolve a supported recipe/refit cargo's declared physical form.
 * @param label Stable required meaning from the content catalogue.
 * @return Immutable traits, or nullptr when no form is declared for this recipe domain.
 */
const MultiLayerCargoContract *GetMultiLayerCargoContract(CargoLabel label);

/** Dedicated movement domains; tube capsules cannot inherit ordinary rail/portal permission. */
enum class MultiLayerVehicleTransport : uint8_t {
	Invalid, ///< Refuse an unset transport definition.
	Rail, ///< Conventional rail and explicitly permitted inter-layer portals.
	Road, ///< Conventional road, tunnels and permitted elevators.
	Tube, ///< Powered layer-local dedicated tube guideway only.
	Rocket, ///< Saved launch/return mission between Surface and Space.
	Orbital, ///< Layer-local orbital navigation.
	Descent, ///< Saved cargo-return job, not a purchasable routed fleet.
};

/** Facilities permitted to commission or construct a logical vehicle design. */
enum class MultiLayerCommissioningFacility : uint8_t {
	Invalid, ///< Unset definition cannot be purchased.
	RailPortalBay, ///< Exclusive commissioning boundary for TBMs.
	RailDepot, ///< Conventional non-TBM ground fleet.
	RoadDepot, ///< Conventional mine trucks and sealed carriers.
	TubeDepot, ///< Powered excavated tube depot.
	LaunchCenter, ///< Surface rocket assembly/service.
	RocketPayload, ///< Probe/pod deployed by a saved launch mission.
	OrbitalAssembly, ///< Station-assembled robot or hauler.
};

/** Facility kind permitted to maintain a logical vehicle design. */
enum class MultiLayerServiceFacility : uint8_t {
	Invalid, ///< No implicit fallback to a conventional depot.
	RailService, ///< Compatible excavated railway service facilities.
	RoadService, ///< Compatible excavated road service facilities.
	TubeService, ///< Powered tube service depot on the same layer.
	LaunchRecovery, ///< Recovery/maintenance at the owning Surface launch center.
	OrbitalService, ///< Compatible station service inventory in Space.
};

/** Unchanged model15 pursuit defaults; altering these values requires a Space content compatibility revision.
 * Extraction, hold mass, upkeep and service remain in the shared MultiLayerSpaceBudget.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8
 */
struct MultiLayerMiningProbeTuning {
	uint8_t travel_tiles_per_day = 4; ///< Actual native/preview movement rate on the shared economy schedule.
	uint16_t approach_tolerance_q16 = 16384; ///< Maximum authorized arrival/working distance: one quarter tile.
	uint16_t minimum_approach_ticks = 1480; ///< Twenty-day floor, frozen when an attempt begins.
	uint8_t closing_tiles_per_day = 3; ///< Conservative closing rate, below probe speed and above maximum body drift.
	uint16_t approach_margin_ticks = 148; ///< Two additional economy days for the initial distance allowance.
};

/** Read-only logical model capability; engine-pool allocation remains with the feature owner. */
struct LayerVehicleCapability {
	uint16_t model_id = UINT16_MAX; ///< Stable profile-owned model identity, never a runtime EngineID.
	std::string_view name; ///< Visible design identity; detailed tables are published by Group 12.
	MultiLayerCapabilities supported_layers; ///< Saved layer identities supported by the design.
	MultiLayerVehicleTransport transport = MultiLayerVehicleTransport::Invalid; ///< Exclusive movement domain.
	CargoClasses cargo_classes; ///< Broad refit forms; required labels and safety restrictions remain authoritative.
	std::span<const CargoLabel> cargo_labels; ///< Explicit permitted meanings, resolved at activation/refit.
	MultiLayerCommissioningFacility commissioning = MultiLayerCommissioningFacility::Invalid; ///< Required construction facility kind.
	MultiLayerServiceFacility service = MultiLayerServiceFacility::Invalid; ///< Required service facility kind.
	TimerGameCalendar::Year introduction_year{0}; ///< Calendar availability; never an economy year or display-frame count.
	bool conventional_portals = false; ///< Tube/orbital designs cannot use ground inter-layer portals.
	bool mandatory_spoil_wagon_leg = false; ///< Loaded spoil cannot bypass actual wagon exchange/Surface haulage.
	const MultiLayerMiningProbeTuning *mining_tuning = nullptr; ///< Explicit15/21 physical pursuit definition; unrelated roles gain no mining capability.
};

/**
 * Read the complete first-party logical vehicle catalogue.
 * @return Append-only immutable designs; feature owners map their model identity to real engine/vehicle records.
 */
std::span<const LayerVehicleCapability> GetMultiLayerVehicleCapabilities();

/** @return Model15's immutable pursuit tuning from the same catalogue used by paid hull commissioning. */
const MultiLayerMiningProbeTuning &GetMultiLayerMiningProbeTuning();

/** Producer identity allocated by the shared utility owner; no feature-local utility registry. */
using UtilityProducerID = StrongType::Typedef<uint32_t, struct UtilityProducerIDTag, StrongType::Compare>;
/** Consumer identity allocated by the shared utility owner. */
using UtilityConsumerID = StrongType::Typedef<uint32_t, struct UtilityConsumerIDTag, StrongType::Compare>;

/** Energy meters distinguish a vent's thermal share from generated electrical joules. */
enum class UtilityEnergyDomain : uint8_t { Electricity, Heat };

/**
 * One utility meter's authoritative interval fields, distinct from paid freight inventory.
 * Rates are watts; energy/demand/use are integer joules. The numerator remainder
 * is millijoules, retained across intervals. Duration derives from synchronized
 * economy ticks at the fixed simulation tick duration, never render or wall time.
 * Groups 15-16 finalize allocation/order/bounds and exclusive grid authority.
 */
struct UtilityPowerAccount {
	UtilityEnergyDomain domain = UtilityEnergyDomain::Electricity; ///< Separate thermal/electric ledgers; heat is never exported through an electrical link.
	UtilityProducerID producer_id{UINT32_MAX}; ///< Stable producer reference; all-ones is unbound/invalid.
	UtilityConsumerID consumer_id{UINT32_MAX}; ///< Stable consumer reference; all-ones is unbound/invalid.
	MapLayer layer = MapLayer::Invalid; ///< Explicit layer-local account; coordinate overlap grants nothing.
	uint64_t accounting_interval = 0; ///< Saved monotonically identified allocation interval.
	uint32_t economy_ticks = 0; ///< Synchronized interval duration in simulation/economy ticks.
	uint64_t generation_rate_w = 0; ///< Available producer rate in watts before allocation.
	uint64_t generation_energy_j = 0; ///< Generated whole joules in this accounting interval.
	uint64_t demand_energy_j = 0; ///< Consumer requested joules for this interval.
	uint64_t granted_energy_j = 0; ///< Joules granted exactly once by the shared allocation owner.
	uint64_t unmet_energy_j = 0; ///< Demand not granted; cannot produce silently while starved.
	uint16_t remainder_mj = 0; ///< Saved fractional generation remainder, in 0..999 millijoules.
};

/** Stable saved profile identities; never reinterpret an existing numeric value. */
enum class MultiLayerGameplayProfileID : uint8_t {
	Classic = 0, ///< Preserve original/legacy content and layer behavior.
	MultilayerIndustry = 1, ///< First-party Temperate economy with explicit feature contracts.
	Invalid = 0xFF, ///< Refuse unknown selections rather than silently using Classic.
};

/** Saved policy fields, with absence-of-profile migration defined as Classic. */
struct MultiLayerProfileSelection {
	MultiLayerGameplayProfileID id = MultiLayerGameplayProfileID::Classic; ///< Stable selected profile identity.
	uint16_t version = 0; ///< Exact profile contract version; Classic uses zero.
	bool legacy = true; ///< Explicit preserved legacy behavior, never inferred from the camera.
	bool editor_override = false; ///< Saved scenario-author exception, separate from normal technology dates.
};

/** Exclusive owner of electricity-related Surface town effects; adapter approval is a separate feasibility gate. */
enum class MultiLayerTownPowerAuthority : uint8_t {
	Native = 0, ///< First-party useful power works with no Game Script.
	ETIGameScript = 1, ///< Reserved for a pinned tested named adapter; native town effects must be disabled.
	Invalid = 0xFF, ///< Unknown authority refuses effects.
};

/** Shared immutable Q16 coordinate, economy-step and body-centre envelope contract.
 * Asteroid headings/speeds remain in their catalogue; static structures and
 * moving bodies consume this single geometry authority.
 */
struct MultiLayerSpaceTuning {
	uint32_t coordinate_units_per_tile = 65536; ///< Saved Q16 precision, never a camera scale.
	uint16_t ticks_per_economy_day = 74; ///< One synchronized movement step per simulation tick.
	uint8_t body_center_margin_tiles = 2; ///< Translation envelope reserved on each travel-strip edge.
	uint8_t travel_start_quarters = 1; ///< Inclusive middle-half strip starts at floor(height/4).
	uint8_t travel_end_quarters = 3; ///< Exclusive strip ends at floor(3*height/4).
};

/** @return Shared Industry geometry defaults; definition changes require profile compatibility handling. */
const MultiLayerSpaceTuning &GetMultiLayerSpaceTuning();

/** Immutable definition contract; later capability queries extend this same public owner. */
struct MultiLayerGameplayProfile {
	MultiLayerGameplayProfileID id; ///< Stable profile identity.
	uint16_t version; ///< Exact contract version, separate from the package version and save schema.
	const RequiredMultiLayerContent *required_content; ///< Pinned package/labels, or nullptr for Classic.
	MultiLayerCapabilities supported_layers{MapLayer::Surface, MapLayer::Underground, MapLayer::Space, MapLayer::DeepUnderground}; ///< Supported storage layers, independent of technology/date availability.
	TimerGameCalendar::Year deep_unlock_year{1980}; ///< First-profile calendar boundary; Classic preserves its legacy behavior.
	MultiLayerTownPowerAuthority town_power_authority = MultiLayerTownPowerAuthority::Native; ///< One authority; no approved ETI adapter in version 1.
	uint16_t orbital_release_radius = 20; ///< Required circular Space release radius in tiles, with its own mission eligibility.
	uint16_t surface_collection_radius = 20; ///< Independent circular Surface station membership radius, never inferred from orbital eligibility.
	std::span<const MultiLayerAsteroidDefinition> asteroid_resources; ///< Industry's immutable resource definitions; Classic has none.
	std::span<const MultiLayerAsteroidSizeDefinition> asteroid_sizes; ///< Industry's independent size probabilities and reserve multipliers.
	const MultiLayerSpaceTuning *space_tuning = nullptr; ///< Industry's shared geometry/step authority; Classic has no orbital tuning.
};

/** Specific reasons why a new world cannot select a gameplay profile. */
enum class MultiLayerProfileError : uint8_t {
	None, ///< Candidate meets the selected profile contract.
	UnknownProfile, ///< Selection is not a known stable identity.
	UnsupportedClimate, ///< Only Temperate has the first-party mapping.
	UnsupportedContent, ///< External packages have no named tested adapter yet.
	MissingPackage, ///< The distributed first-party package could not be found.
	ChangedPackage, ///< Selected package digest or definition version is not the pinned content.
	AmbiguousCargo, ///< Required cargo labels are missing or duplicated after activation.
};

/**
 * Resolve one immutable profile without reading or changing the current world.
 * @param id Stable identity stored by a selector or saved profile record.
 * @return Authoritative immutable profile, or nullptr for unknown/invalid identities.
 */
const MultiLayerGameplayProfile *GetMultiLayerContentProfile(MultiLayerGameplayProfileID id);

/** Resolve legacy cargo meanings from the candidate's exact immutable definition identity.
 * @param id Original saved profile; Industry requires its exact pinned package, Classic permits unchanged native baseline definitions only.
 * @param version Original saved profile revision.
 * @param landscape Original saved climate; Industry permits Temperate only, vanilla Classic uses its original native climate.
 * @param candidate This file's resolved NGRF entries, including exact package digest/version and system entries.
 * @return Original slots projected from the same package producer and native climate definitions, or no mapping for unsupported content. Never reads live CargoSpec.
 */
std::optional<std::array<CargoLabel, NUM_CARGO>> GetLegacyMultiLayerCargoLabels(MultiLayerGameplayProfileID id, uint16_t version,
	LandscapeType landscape, const GRFConfigList &candidate);

/**
 * Check Deep access from an explicit saved policy and a typed calendar date.
 * This read-only predicate does not select a camera, inspect a frame counter, or
 * grant vehicle/facility capabilities; feature commands check those independently.
 * @param selection Authoritative saved profile/version and explicit editor/legacy policy.
 * @param calendar_date Calendar date of the simulated operation, not an economy date.
 * @return True for Classic legacy access or an eligible Multilayer date/explicit override; false for invalid policy.
 */
bool CheckDeepLayerAvailability(const MultiLayerProfileSelection &selection, TimerGameCalendar::Date calendar_date);

/**
 * Read the currently accepted world's policy without permitting direct mutation.
 * @return Current accepted selection; reset/load without a profile uses Classic legacy policy.
 */
const MultiLayerProfileSelection &GetCurrentMultiLayerProfileSelection();

/** Reset world policy at initialization before new-world activation or save record loading. */
void ResetMultiLayerProfileSelection();

/**
 * Prepare exact first-party content after new-game settings/GRF selection has resolved.
 * Validation changes only candidate configuration, never packets, money, timers or RNG.
 * @param id Requested profile identity.
 * @param landscape Requested climate, independent of view selection.
 * @param candidate Configuration to be passed to the staged loader.
 * @return Refusal cause, or None when the candidate is ready to load.
 */
MultiLayerProfileError PrepareMultiLayerProfileContent(MultiLayerGameplayProfileID id, LandscapeType landscape, GRFConfigList &candidate);

/**
 * Accept a new-world policy only after its activated definitions validate.
 * @param id Requested stable identity, read from authoritative new-game settings.
 * @param landscape Requested world climate.
 * @param activated Actual staged-loader configuration and cargo definitions.
 * @return Refusal cause; failed validation retains the initialized Classic policy.
 */
MultiLayerProfileError ActivateMultiLayerProfile(MultiLayerGameplayProfileID id, LandscapeType landscape, const GRFConfigList &activated);

/**
 * Restore an explicit saved policy after schema and activated content validation.
 * @param selection Validated saved identity/version and explicit legacy/editor flags.
 * @param landscape Actual loaded climate, not pending new-game selection.
 * @param activated Actual loaded definitions; missing/changed meanings refuse.
 * @return Refusal or None; only success publishes the saved policy through its one owner.
 */
MultiLayerProfileError RestoreMultiLayerProfileSelection(const MultiLayerProfileSelection &selection, LandscapeType landscape, const GRFConfigList &activated);

/**
 * Provide a specific user-facing refusal reason for a failed selection.
 * @param error Validation result returned by profile preparation or activation.
 * @return Stable English diagnostic; UI localization belongs to the caller.
 */
std::string_view GetMultiLayerProfileErrorMessage(MultiLayerProfileError error);

#endif /* MULTILAYER_CONTENT_H */
