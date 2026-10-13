/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_bootstrap.h Paid Surface seeds and acyclic first-build content contracts. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#ifndef MULTILAYER_BOOTSTRAP_H
#define MULTILAYER_BOOTSTRAP_H

#include "multilayer_recipe.h"

/** Bounded external Surface procurement; bought inventory never earns freight income. */
struct MultiLayerSurfaceSupply {
	CargoLabel label; ///< Existing registered cargo meaning.
	TimerGameCalendar::Year introduction_year; ///< Availability independent of inaccessible local manufacturing.
	uint32_t price_per_unit; ///< Internal Money units per declared cargo unit, before inflation.
	uint16_t capacity_per_day; ///< Per-company economy-day purchase limit, shared across supply stations.
};

/** Content-proof nodes only; these ordinals are not saved gameplay/pool identities. */
enum class MultiLayerBootstrapStage : uint8_t {
	SurfaceRoute, TBM, Mine, Tube, DeepTBM, Geothermal, Launch, Probe, Mirror, Station, HeavyRobot, SurveyProbe, ConstructionProbes, Assembly,
};

/** One build milestone, seeded by paid Surface cargo and earlier real transport routes. */
struct MultiLayerBootstrapMilestone {
	MultiLayerBootstrapStage stage; ///< Proof node, not a runtime vehicle/facility ID.
	std::string_view name; ///< Visible milestone description for the contract fixture.
	TimerGameCalendar::Year introduction_year; ///< Earliest proposed calendar date after all dependencies.
	std::span<const MultiLayerBootstrapStage> prerequisites; ///< Required earlier commissioning/access/transport stages.
	std::span<const MultiLayerRecipeCargo> purchases; ///< Bounded paid Surface deliveries, not free starting inventory.
	uint32_t capital_cost; ///< Internal Money units for equipment/construction, excluding listed supplies.
};

/**
 * Read supported external Surface procurement definitions.
 * @return Immutable seed list; feature owners implement payment, bounds and physical delivery.
 */
std::span<const MultiLayerSurfaceSupply> GetMultiLayerSurfaceSupplies();

/**
 * Read the first-build dependency graph and its input ledgers.
 * @return Immutable milestones; none authorizes building without actual route/material validation.
 */
std::span<const MultiLayerBootstrapMilestone> GetMultiLayerBootstrapMilestones();

/** Frozen early tube service prices; runtime invoices and electrical quantities have separate owners. */
struct MultiLayerTubePowerContract {
	uint32_t capacity_w = 100000; ///< Bounded paid external supply, available without geothermal generation.
	uint16_t lease_per_day = 20; ///< Money charged once for an economy day in which service is offered.
	uint16_t joules_per_money = 1000; ///< Used energy tariff; the connection saves the unpaid fractional numerator.
	uint16_t introduction_year = 1970; ///< Calendar gate independent of Deep access and economy speed.
};

/** @return Canonical paid service contract frozen by r2mlvs1-f-utility-and-transfer-contracts.md Group19. */
const MultiLayerTubePowerContract &GetMultiLayerTubePowerContract();

/** Canonical propulsion envelope per capsule; native20-kW mandatory depot service remains the shared TubeWork contract. */
struct MultiLayerTubeEnergyModel {
	uint16_t model_id; ///< Existing immutable catalogue model, independent of EIDS runtime slots.
	uint32_t empty_motion_w; ///< Motoring rate at an empty physical hold; stopping/waiting consumes none.
	uint32_t full_motion_w; ///< Rate at the actual full hold, interpolated with checked integer load accounting.
};

/** @return Frozen relative tube propulsion envelopes; model9's full20-kW reference preserves the bootstrap invoice. */
std::span<const MultiLayerTubeEnergyModel> GetMultiLayerTubeEnergyModels();

/** Frozen first-utility construction families; ordinals select content, never a saved facility or utility identity. */
enum class MultiLayerGeothermalConstructionKind : uint8_t { Collector, Powerhouse, Riser };

/** Canonical transported construction/work contract; a blueprint supplies none of its materials, completed work or electricity. */
struct MultiLayerGeothermalConstruction {
	MultiLayerGeothermalConstructionKind kind; ///< Content-owned first-utility family.
	uint8_t width; ///< Complete excavated facility floor width, excluding its access cells.
	uint8_t height; ///< Complete excavated facility floor height.
	uint32_t capital_cost; ///< Internal Money required for actual equipment/construction, excluding delivered inputs.
	uint32_t work_ticks; ///< Cash-driven completed construction work; no first-build electricity requirement.
	TimerGameCalendar::Year introduction_year; ///< Calendar availability, also constrained by shared Deep/portal policy.
	std::span<const MultiLayerRecipeCargo> inputs; ///< Actual conventionally delivered cargo required before commissioning.
	uint32_t transfer_capacity_w = 0; ///< Frozen directed electrical riser rate; collector/powerhouse construction does not grant export capacity (Plan18 Group5B).
};

/** @param kind First collector/powerhouse/riser family. @return Frozen Plan1 Group19 contract, or nullptr for an unknown family; no runtime construction is authorized by reading it. */
const MultiLayerGeothermalConstruction *GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind kind);

#endif /* MULTILAYER_BOOTSTRAP_H */
