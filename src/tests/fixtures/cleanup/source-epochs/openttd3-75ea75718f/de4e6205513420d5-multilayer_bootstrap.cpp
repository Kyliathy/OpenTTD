/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_bootstrap.cpp First-build ledgers with paid supplies before Deep technology. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md; r2mlvs18-geothermal-energy-and-industry.md Groups8C/8E (dated generator terms). */

#include "stdafx.h"
#include "multilayer_bootstrap.h"
#include "geothermal_generator_geometry.h"

#include "safeguards.h"

static constexpr MultiLayerRecipeCargo _collector_inputs[] = {{CargoLabel{"STEL"}, 8}, {CargoLabel{"RFMT"}, 4}, {CargoLabel{"ELEC"}, 2}};
static constexpr MultiLayerRecipeCargo _powerhouse_inputs[] = {{CargoLabel{"STEL"}, 12}, {CargoLabel{"RFMT"}, 8}, {CargoLabel{"ELEC"}, 4}, {CargoLabel{"BATY"}, 2}};
static constexpr MultiLayerRecipeCargo _riser_inputs[] = {{CargoLabel{"STEL"}, 4}, {CargoLabel{"RFMT"}, 4}, {CargoLabel{"ELEC"}, 2}, {CargoLabel{"BATY"}, 2}};
static constexpr MultiLayerRecipeCargo _magma_inputs[] = {{CargoLabel{"STEL"}, 24}, {CargoLabel{"RFMT"}, 16}, {CargoLabel{"ELEC"}, 8}, {CargoLabel{"BATY"}, 4}};

/** Frozen per-facility split of the existing first-geothermal480,000 capital/material ledger. */
static const MultiLayerGeothermalConstruction _geothermal_construction[] = {
	{MultiLayerGeothermalConstructionKind::Collector, 3, 3, 180000, 296, TimerGameCalendar::Year{1980}, _collector_inputs},
	{MultiLayerGeothermalConstructionKind::Powerhouse, 4, 4, 180000, 592, TimerGameCalendar::Year{1980}, _powerhouse_inputs},
	{MultiLayerGeothermalConstructionKind::Riser, 2, 2, 120000, 296, TimerGameCalendar::Year{1980}, _riser_inputs, 200000},
	{MultiLayerGeothermalConstructionKind::MagmaPowerhouse, 4, 4, 360000, 888, TimerGameCalendar::Year{2000}, _magma_inputs},
};

/** Frozen dated operating models share the same original thermal budget; no metadata query creates source, inventory or native identities. */
static constexpr MultiLayerGeothermalGeneratorTerms _geothermal_generator_terms[] = {
	{GeothermalGeneratorModel::VentTap, MultiLayerGeothermalConstructionKind::Powerhouse, 1000000, 150000, 15, 20},
	{GeothermalGeneratorModel::MagmaLoop, MultiLayerGeothermalConstructionKind::MagmaPowerhouse, 1000000, 350000, 35, 35},
};

/** @copydoc GetMultiLayerGeothermalConstruction */
const MultiLayerGeothermalConstruction *GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind kind)
{
	const auto found = std::ranges::find(_geothermal_construction, kind, &MultiLayerGeothermalConstruction::kind);
	return found == std::end(_geothermal_construction) ? nullptr : &*found;
}

/** @copydoc GetMultiLayerGeothermalGeneratorTerms */
const MultiLayerGeothermalGeneratorTerms *GetMultiLayerGeothermalGeneratorTerms(GeothermalGeneratorModel model)
{
	const auto found = std::ranges::find(_geothermal_generator_terms, model, &MultiLayerGeothermalGeneratorTerms::model);
	return found == std::end(_geothermal_generator_terms) ? nullptr : &*found;
}

/** @copydoc GetMultiLayerGeothermalGeneratorModel */
std::optional<GeothermalGeneratorModel> GetMultiLayerGeothermalGeneratorModel(MultiLayerGeothermalConstructionKind construction)
{
	const auto found = std::ranges::find(_geothermal_generator_terms, construction, &MultiLayerGeothermalGeneratorTerms::construction);
	return found == std::end(_geothermal_generator_terms) ? std::nullopt : std::optional{found->model};
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
	{CargoLabel{"GOOD"}, TimerGameCalendar::Year{1950}, 120, 256},
};

static constexpr MultiLayerBootstrapStage _surface_route[] = {MultiLayerBootstrapStage::SurfaceRoute};
static constexpr MultiLayerBootstrapStage _tbm_route[] = {MultiLayerBootstrapStage::SurfaceRoute, MultiLayerBootstrapStage::TBM};
static constexpr MultiLayerBootstrapStage _deep_route[] = {MultiLayerBootstrapStage::SurfaceRoute, MultiLayerBootstrapStage::DeepTBM};
static constexpr MultiLayerBootstrapStage _launch_route[] = {MultiLayerBootstrapStage::SurfaceRoute, MultiLayerBootstrapStage::Launch};
static constexpr MultiLayerBootstrapStage _station_route[] = {MultiLayerBootstrapStage::Launch, MultiLayerBootstrapStage::Station, MultiLayerBootstrapStage::Assembly};
static constexpr MultiLayerBootstrapStage _builders_route[] = {MultiLayerBootstrapStage::SurfaceRoute, MultiLayerBootstrapStage::Launch, MultiLayerBootstrapStage::ConstructionProbes};
static constexpr MultiLayerBootstrapStage _core_route[] = {MultiLayerBootstrapStage::Station};

static constexpr MultiLayerRecipeCargo _tbm_materials[] = {{CargoLabel{"STEL"}, 8}, {CargoLabel{"RFMT"}, 4}};
static constexpr MultiLayerRecipeCargo _tube_materials[] = {{CargoLabel{"STEL"}, 12}, {CargoLabel{"RFMT"}, 8}, {CargoLabel{"ELEC"}, 4}, {CargoLabel{"BATY"}, 2}};
static constexpr MultiLayerRecipeCargo _deep_materials[] = {{CargoLabel{"STEL"}, 12}, {CargoLabel{"RFMT"}, 8}, {CargoLabel{"ELEC"}, 4}};
static constexpr MultiLayerRecipeCargo _geo_materials[] = {{CargoLabel{"STEL"}, 24}, {CargoLabel{"RFMT"}, 16}, {CargoLabel{"ELEC"}, 8}, {CargoLabel{"BATY"}, 4}};
static constexpr MultiLayerRecipeCargo _launch_materials[] = {{CargoLabel{"STEL"}, 24}, {CargoLabel{"RFMT"}, 16}, {CargoLabel{"SCMP"}, 8}, {CargoLabel{"ELEC"}, 8}, {CargoLabel{"PROP"}, 16}, {CargoLabel{"GOOD"}, 16}};
/* Paid embodied hull, two 2t shells and service preload: 14/30t in the first mining payload. */
static constexpr MultiLayerRecipeCargo _probe_materials[] = {{CargoLabel{"STEL"}, 6}, {CargoLabel{"GOOD"}, 12}, {CargoLabel{"RFMT"}, 1}, {CargoLabel{"ELEC"}, 1}};
static constexpr MultiLayerRecipeCargo _survey_materials[] = {{CargoLabel{"STEL"}, 2}, {CargoLabel{"GOOD"}, 4}, {CargoLabel{"RFMT"}, 1}, {CargoLabel{"ELEC"}, 1}, {CargoLabel{"PROP"}, 15}};
/* Two reusable builders plus two service preloads; Core materials are purchased separately. After the mining flight, the two Core flights require 29 extra fuel above the remaining 1. */
static constexpr MultiLayerRecipeCargo _builder_materials[] = {{CargoLabel{"STEL"}, 8}, {CargoLabel{"GOOD"}, 16}, {CargoLabel{"RFMT"}, 2}, {CargoLabel{"ELEC"}, 2}, {CargoLabel{"PROP"}, 29}};
static constexpr MultiLayerRecipeCargo _mirror_materials[] = {{CargoLabel{"STEL"}, 12}, {CargoLabel{"GOOD"}, 4}, {CargoLabel{"ELEC"}, 4}, {CargoLabel{"PROP"}, 15}};
static constexpr MultiLayerRecipeCargo _station_materials[] = {{CargoLabel{"STEL"}, 16}, {CargoLabel{"GOOD"}, 24}};
static constexpr MultiLayerRecipeCargo _assembly_materials[] = {{CargoLabel{"STEL"}, 8}, {CargoLabel{"GOOD"}, 16}, {CargoLabel{"PROP"}, 15}};
static constexpr MultiLayerRecipeCargo _robot_materials[] = {{CargoLabel{"STEL"}, 6}, {CargoLabel{"GOOD"}, 12}, {CargoLabel{"RBPT"}, 4}, {CargoLabel{"ELEC"}, 2}, {CargoLabel{"PROP"}, 15}};

static const MultiLayerBootstrapMilestone _bootstrap_milestones[] = {
	{MultiLayerBootstrapStage::SurfaceRoute, "Surface origin, portal and conventional route", TimerGameCalendar::Year{1950}, {}, {}, 220000},
	{MultiLayerBootstrapStage::TBM, "Standard TBM and spoil train", TimerGameCalendar::Year{1950}, _surface_route, _tbm_materials, 300000},
	{MultiLayerBootstrapStage::Mine, "First excavated mine and loading route", TimerGameCalendar::Year{1950}, _tbm_route, _tbm_materials, 80000},
	{MultiLayerBootstrapStage::Tube, "Powered 1970 tube route", TimerGameCalendar::Year{1970}, _tbm_route, _tube_materials, 160000},
	{MultiLayerBootstrapStage::DeepTBM, "Deep TBM, upgrade and conventional access", TimerGameCalendar::Year{1980}, _tbm_route, _deep_materials, 440000},
	{MultiLayerBootstrapStage::Geothermal, "Collector, powerhouse and commissioned riser", TimerGameCalendar::Year{1980}, _deep_route, _geo_materials, 480000},
	{MultiLayerBootstrapStage::Launch, "Launch center, rocket and Surface fuel", TimerGameCalendar::Year{1990}, _surface_route, _launch_materials, 900000},
	{MultiLayerBootstrapStage::Probe, "First probe deployment", TimerGameCalendar::Year{1990}, _launch_route, _probe_materials, 104000},
	{MultiLayerBootstrapStage::SurveyProbe, "Paid Earth survey probe and launch", TimerGameCalendar::Year{1990}, _launch_route, _survey_materials, 60000},
	{MultiLayerBootstrapStage::Mirror, "First mirror and Surface solar target", TimerGameCalendar::Year{1990}, _launch_route, _mirror_materials, 120000},
	{MultiLayerBootstrapStage::ConstructionProbes, "Two Earth construction probes and Core flights", TimerGameCalendar::Year{1995}, _launch_route, _builder_materials, 200000},
	{MultiLayerBootstrapStage::Station, "Probe-built Surface-supplied Core", TimerGameCalendar::Year{1995}, _builders_route, _station_materials, 700000},
	{MultiLayerBootstrapStage::Assembly, "Delivered completed orbital Assembly", TimerGameCalendar::Year{1995}, _core_route, _assembly_materials, 180000},
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
