/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_bootstrap.cpp First-build ledgers with paid supplies before Deep technology. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#include "stdafx.h"
#include "multilayer_bootstrap.h"

#include "safeguards.h"

static constexpr MultiLayerRecipeCargo _collector_inputs[] = {{CargoLabel{"STEL"}, 8}, {CargoLabel{"RFMT"}, 4}, {CargoLabel{"ELEC"}, 2}};
static constexpr MultiLayerRecipeCargo _powerhouse_inputs[] = {{CargoLabel{"STEL"}, 12}, {CargoLabel{"RFMT"}, 8}, {CargoLabel{"ELEC"}, 4}, {CargoLabel{"BATY"}, 2}};
static constexpr MultiLayerRecipeCargo _riser_inputs[] = {{CargoLabel{"STEL"}, 4}, {CargoLabel{"RFMT"}, 4}, {CargoLabel{"ELEC"}, 2}, {CargoLabel{"BATY"}, 2}};

/** Frozen per-facility split of the existing first-geothermal480,000 capital/material ledger. */
static const MultiLayerGeothermalConstruction _geothermal_construction[] = {
	{MultiLayerGeothermalConstructionKind::Collector, 3, 3, 180000, 296, TimerGameCalendar::Year{1980}, _collector_inputs},
	{MultiLayerGeothermalConstructionKind::Powerhouse, 4, 4, 180000, 592, TimerGameCalendar::Year{1980}, _powerhouse_inputs},
	{MultiLayerGeothermalConstructionKind::Riser, 2, 2, 120000, 296, TimerGameCalendar::Year{1980}, _riser_inputs, 200000},
};

/** @copydoc GetMultiLayerGeothermalConstruction */
const MultiLayerGeothermalConstruction *GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind kind)
{
	const auto found = std::ranges::find(_geothermal_construction, kind, &MultiLayerGeothermalConstruction::kind);
	return found == std::end(_geothermal_construction) ? nullptr : &*found;
}

static const MultiLayerSurfaceSupply _surface_supplies[] = {
	{CargoLabel{"STEL"}, TimerGameCalendar::Year{1950}, 100, 256},
	{CargoLabel{"RFMT"}, TimerGameCalendar::Year{1950}, 180, 128},
	{CargoLabel{"CHEM"}, TimerGameCalendar::Year{1950}, 220, 128},
	{CargoLabel{"ELEC"}, TimerGameCalendar::Year{1965}, 500, 64},
	{CargoLabel{"BATY"}, TimerGameCalendar::Year{1965}, 350, 64},
	{CargoLabel{"SCMP"}, TimerGameCalendar::Year{1980}, 600, 64},
	{CargoLabel{"RBPT"}, TimerGameCalendar::Year{1980}, 750, 32},
	{CargoLabel{"PROP"}, TimerGameCalendar::Year{1980}, 250, 64},
	{CargoLabel{"SMIK"}, TimerGameCalendar::Year{1980}, 1500, 16},
};

static constexpr MultiLayerBootstrapStage _surface_route[] = {MultiLayerBootstrapStage::SurfaceRoute};
static constexpr MultiLayerBootstrapStage _tbm_route[] = {MultiLayerBootstrapStage::SurfaceRoute, MultiLayerBootstrapStage::TBM};
static constexpr MultiLayerBootstrapStage _deep_route[] = {MultiLayerBootstrapStage::SurfaceRoute, MultiLayerBootstrapStage::DeepTBM};
static constexpr MultiLayerBootstrapStage _launch_route[] = {MultiLayerBootstrapStage::SurfaceRoute, MultiLayerBootstrapStage::Launch};
static constexpr MultiLayerBootstrapStage _station_route[] = {MultiLayerBootstrapStage::Launch, MultiLayerBootstrapStage::Station};

static constexpr MultiLayerRecipeCargo _tbm_materials[] = {{CargoLabel{"STEL"}, 8}, {CargoLabel{"RFMT"}, 4}};
static constexpr MultiLayerRecipeCargo _tube_materials[] = {{CargoLabel{"STEL"}, 12}, {CargoLabel{"RFMT"}, 8}, {CargoLabel{"ELEC"}, 4}, {CargoLabel{"BATY"}, 2}};
static constexpr MultiLayerRecipeCargo _deep_materials[] = {{CargoLabel{"STEL"}, 12}, {CargoLabel{"RFMT"}, 8}, {CargoLabel{"ELEC"}, 4}};
static constexpr MultiLayerRecipeCargo _geo_materials[] = {{CargoLabel{"STEL"}, 24}, {CargoLabel{"RFMT"}, 16}, {CargoLabel{"ELEC"}, 8}, {CargoLabel{"BATY"}, 4}};
static constexpr MultiLayerRecipeCargo _launch_materials[] = {{CargoLabel{"STEL"}, 24}, {CargoLabel{"RFMT"}, 16}, {CargoLabel{"SCMP"}, 8}, {CargoLabel{"ELEC"}, 8}, {CargoLabel{"PROP"}, 16}};
/* Probe body (8 t), two expendable pods (2 t) and one service preload (2 t) all travel from Surface. */
static constexpr MultiLayerRecipeCargo _probe_materials[] = {{CargoLabel{"SCMP"}, 4}, {CargoLabel{"RFMT"}, 5}, {CargoLabel{"ELEC"}, 3}};
static constexpr MultiLayerRecipeCargo _mirror_materials[] = {{CargoLabel{"SMIK"}, 4}, {CargoLabel{"RFMT"}, 2}, {CargoLabel{"STEL"}, 8}, {CargoLabel{"ELEC"}, 4}};
static constexpr MultiLayerRecipeCargo _station_materials[] = {{CargoLabel{"SCMP"}, 16}, {CargoLabel{"RFMT"}, 8}, {CargoLabel{"ELEC"}, 4}};
static constexpr MultiLayerRecipeCargo _robot_materials[] = {{CargoLabel{"SCMP"}, 6}, {CargoLabel{"RBPT"}, 4}, {CargoLabel{"ELEC"}, 2}};

static const MultiLayerBootstrapMilestone _bootstrap_milestones[] = {
	{MultiLayerBootstrapStage::SurfaceRoute, "Surface origin, portal and conventional route", TimerGameCalendar::Year{1950}, {}, {}, 220000},
	{MultiLayerBootstrapStage::TBM, "Standard TBM and spoil train", TimerGameCalendar::Year{1950}, _surface_route, _tbm_materials, 300000},
	{MultiLayerBootstrapStage::Mine, "First excavated mine and loading route", TimerGameCalendar::Year{1950}, _tbm_route, _tbm_materials, 80000},
	{MultiLayerBootstrapStage::Tube, "Powered 1970 tube route", TimerGameCalendar::Year{1970}, _tbm_route, _tube_materials, 160000},
	{MultiLayerBootstrapStage::DeepTBM, "Deep TBM, upgrade and conventional access", TimerGameCalendar::Year{1980}, _tbm_route, _deep_materials, 440000},
	{MultiLayerBootstrapStage::Geothermal, "Collector, powerhouse and commissioned riser", TimerGameCalendar::Year{1980}, _deep_route, _geo_materials, 480000},
	{MultiLayerBootstrapStage::Launch, "Launch center, rocket and Surface fuel", TimerGameCalendar::Year{1990}, _surface_route, _launch_materials, 900000},
	{MultiLayerBootstrapStage::Probe, "First probe deployment", TimerGameCalendar::Year{1990}, _launch_route, _probe_materials, 104000},
	{MultiLayerBootstrapStage::Mirror, "First mirror and Surface solar target", TimerGameCalendar::Year{1990}, _launch_route, _mirror_materials, 120000},
	{MultiLayerBootstrapStage::Station, "Surface-supplied station nucleus", TimerGameCalendar::Year{1995}, _launch_route, _station_materials, 700000},
	{MultiLayerBootstrapStage::HeavyRobot, "First station-assembled heavy robot", TimerGameCalendar::Year{1995}, _station_route, _robot_materials, 200000},
};

/**
 * Publish Surface supply pricing/caps without creating cargo packets.
 * @return Immutable first-party purchase catalogue.
 */
std::span<const MultiLayerSurfaceSupply> GetMultiLayerSurfaceSupplies()
{
	return _surface_supplies;
}

/**
 * Publish the milestone dependency graph without changing construction state.
 * @return Immutable input/access/cash proof records.
 */
std::span<const MultiLayerBootstrapMilestone> GetMultiLayerBootstrapMilestones()
{
	return _bootstrap_milestones;
}

/** @copydoc GetMultiLayerTubePowerContract */
const MultiLayerTubePowerContract &GetMultiLayerTubePowerContract()
{
	static constexpr MultiLayerTubePowerContract contract;
	return contract;
}

/** @copydoc GetMultiLayerTubeEnergyModels */
std::span<const MultiLayerTubeEnergyModel> GetMultiLayerTubeEnergyModels()
{
	static constexpr MultiLayerTubeEnergyModel models[] = {
		{9, 10000, 20000}, {10, 12000, 18000}, {11, 12000, 24000}, {12, 10000, 20000}, {13, 20000, 50000},
	};
	return models;
}
