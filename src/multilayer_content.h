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

/** Immutable single-cargo finite resource definitions; class order is saved. */
struct MultiLayerAsteroidDefinition {
	CargoLabel cargo;
	uint32_t small_min, small_max;
};
const std::array<MultiLayerAsteroidDefinition, 3> &GetMultiLayerAsteroidDefinitions();
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
};

/**
 * Read the complete first-party logical vehicle catalogue.
 * @return Nineteen immutable designs; feature owners map their model identity to real engine/vehicle records.
 */
std::span<const LayerVehicleCapability> GetMultiLayerVehicleCapabilities();

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
