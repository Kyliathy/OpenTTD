/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file multilayer_space_content.h Versioned Space recipes, mass and mission budgets. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md Group12B. */
#ifndef MULTILAYER_SPACE_CONTENT_H
#define MULTILAYER_SPACE_CONTENT_H
#include "multilayer_recipe.h"
#include <optional>

inline constexpr uint16_t MULTILAYER_SPACE_LEGACY_CONTENT_VERSION = 2; ///< Exact original0..20 recipes and paid receipt provenance remain supported.
inline constexpr uint16_t MULTILAYER_SPACE_CONTENT_VERSION = 3; ///< Appends heavy miner21; actual saved admission is versioned by Plan11 Group9B.
/** Append-only construction identities; natural asteroids are not construction recipes. */
enum class SpaceBuildKind : uint8_t {
	Rocket, MiningProbe, SurveyProbe, ConstructionProbe, Pod, Mirror, Core, Dock, Storage,
	Assembly, Steelworks, Goodsworks, Power, FuelStore, HeavyRobot, Hauler, Refinery, IceProcessor, HeavyMiner, End,
};

/** Immutable full recipe and physical construction contract; native jobs/hulls retain consumed inputs and capital provenance. */
struct SpaceConstructionDefinition {
	SpaceBuildKind kind;
	uint16_t model; ///< Logical model including nonrouted pod18; UINT16_MAX for static structures/mirrors.
	std::span<const MultiLayerRecipeCargo> inputs;
	uint32_t capital;
	uint32_t work_ticks;
	uint16_t dry_mass_sixteenths;
	TimerGameCalendar::Year introduction_year;
	uint8_t width = 0, height = 0; ///< Complete static footprint; zero for carried/mobile equipment.
};

/** Shared immutable defaults. Runtime inventory, receipts and timers remain with their feature owners. */
struct MultiLayerSpaceBudget {
	uint16_t rocket_payload_sixteenths = 30 * 16;
	uint16_t probe_hold_sixteenths = 20 * 16;
	uint16_t fuel_hold = 40, outbound_fuel = 10, recovery_fuel = 5;
	uint16_t prepare_ticks = 148, turnaround_ticks = 74, flight_ticks = 222, descent_ticks = 148;
	uint16_t service_ticks = 148, scan_ticks = 148;
	uint16_t active_per_day = 500, idle_per_day = 100, pad_per_day = 500;
	uint16_t mission_fee = 5000;
	uint8_t returns_per_service = 2, starting_pods = 2;
	uint16_t mining_interval = 74, mining_units = 2;
	uint16_t stock_cap = 1024;
	uint16_t launch_propellant_cap = 128, launch_metal_cap = 64, launch_electronics_cap = 32, launch_components_cap = 64;
	uint8_t launch_pod_cap = 8, probe_metal_cap = 8, probe_electronics_cap = 4;
	uint16_t core_activation_ticks = 74;
	uint32_t core_panel_w = 30000, module_panel_w = 60000;
	uint16_t work_j_per_tick = 750;
};

/** @return Every immutable construction definition in stable append-only identity order; this creates no paid asset. */
std::span<const SpaceConstructionDefinition> GetSpaceConstructionDefinitions();
/** @param kind Canonical construction identity. @return Immutable full recipe, or nullptr for an unsupported kind. */
const SpaceConstructionDefinition *GetSpaceConstructionDefinition(SpaceBuildKind kind);
/** @return Delivered common Space budget; per-miner operating differences are published by space_mining_tuning.h. */
const MultiLayerSpaceBudget &GetMultiLayerSpaceBudget();
/** @return Current canonical Space content fingerprint, including every appended role and its immutable operating tuning. */
uint64_t GetMultiLayerSpaceContentSignature();
/** Compute a supported content revision using precisely its original definition/model set.
 * @param version Legacy2 or current3; unsupported revisions do not acquire compatibility through current definitions.
 * @return Exact revision fingerprint, or zero for an unsupported version.
 */
uint64_t GetMultiLayerSpaceContentSignatureForVersion(uint16_t version);
/** @param version Original paid construction provenance. @return Whether a legacy2 or current3 recipe revision is recognized; callers still validate the full actual recipe/owner. */
bool IsSupportedSpaceConstructionVersion(uint16_t version);
/** Preserve older paid receipts without granting a new role to an older construction revision.
 * @param version Original paid receipt revision. @param model Actual canonical hull/payload identity.
 * @return Whether that exact supported model/revision exists; heavy21 requires3. This creates no commissioning right.
 */
bool IsSupportedSpaceVehicleContentVersion(uint16_t version, uint16_t model);
/** @param profile Actual selected/same-file gameplay profile. @param version Its saved Space revision. @param signature Exact saved immutable fingerprint.
 * @return Whether profile and revision/fingerprint agree; save readers additionally gate old/new source versions.
 */
bool IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID profile, uint16_t version, uint64_t signature);
/** Clear only pending content metadata before an independent load/check; paid native assets are untouched. */
void ResetSpaceContentLoadStaging();
/** Validate same-file copied content metadata without publishing it to the open world. */
void ValidateSpaceContentLoadPreview();
/** Sum wide native physical mass without per-crate/packet rounding.
 * @param cargo Actual label/whole-unit recipe or payload view. @return Mass in sixteenths, or no value after missing meanings/overflow.
 */
std::optional<uint64_t> GetMultiLayerPayloadMass(std::span<const MultiLayerRecipeCargo> cargo);
/** @param model Canonical logical hull/payload identity. @return Full immutable commissioning recipe, or nullptr if unsupported; this allocates no Engine/Vehicle. */
const SpaceConstructionDefinition *GetSpaceVehicleConstruction(uint16_t model);
/** @return Existing model15/19/20 service recipe; per-miner operating definitions supply the explicit heavy quantities. */
std::span<const MultiLayerRecipeCargo> GetSpaceProbeServiceInputs();
/** Query definition-only commissioning rights; native producers additionally prove actual materials, berth, paid work and funds.
 * @param model Canonical logical vehicle role. @param facility Explicit physical facility class. @param completed Whether that actual facility has completed activation.
 * @return Whether the role permits that completed facility; this query creates no assets or readiness.
 */
bool CanCommissionSpaceVehicle(uint16_t model, MultiLayerCommissioningFacility facility, bool completed);
#endif
