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
	SurfaceRoute, TBM, Mine, Tube, DeepTBM, Geothermal, Launch, Probe, Mirror, Station, HeavyRobot,
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

#endif /* MULTILAYER_BOOTSTRAP_H */
