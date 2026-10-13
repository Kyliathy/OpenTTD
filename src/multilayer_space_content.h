/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file multilayer_space_content.h Versioned Space recipes, mass and mission budgets. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md Group12B. */
#ifndef MULTILAYER_SPACE_CONTENT_H
#define MULTILAYER_SPACE_CONTENT_H
#include "multilayer_recipe.h"
#include <optional>

inline constexpr uint16_t MULTILAYER_SPACE_CONTENT_VERSION = 2;
/** Append-only construction identities; natural asteroids are not construction recipes. */
enum class SpaceBuildKind : uint8_t {
	Rocket, MiningProbe, SurveyProbe, ConstructionProbe, Pod, Mirror, Core, Dock, Storage,
	Assembly, Steelworks, Goodsworks, Power, FuelStore, HeavyRobot, Hauler, Refinery, IceProcessor, End,
};

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

std::span<const SpaceConstructionDefinition> GetSpaceConstructionDefinitions();
const SpaceConstructionDefinition *GetSpaceConstructionDefinition(SpaceBuildKind kind);
const MultiLayerSpaceBudget &GetMultiLayerSpaceBudget();
uint64_t GetMultiLayerSpaceContentSignature();
bool IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID profile, uint16_t version, uint64_t signature);
void ResetSpaceContentLoadStaging();
void ValidateSpaceContentLoadPreview();
/** Wide mass sum in sixteenths; missing meanings or arithmetic overflow refuse. No rounding per crate/packet. */
std::optional<uint64_t> GetMultiLayerPayloadMass(std::span<const MultiLayerRecipeCargo> cargo);
/** Canonical commissioning recipe for an existing immutable vehicle model, or nullptr if unsupported. */
const SpaceConstructionDefinition *GetSpaceVehicleConstruction(uint16_t model);
std::span<const MultiLayerRecipeCargo> GetSpaceProbeServiceInputs();
/** Definition-only facility admission; feature owners must additionally prove actual paid, completed physical facilities. */
bool CanCommissionSpaceVehicle(uint16_t model, MultiLayerCommissioningFacility facility, bool completed);
#endif
