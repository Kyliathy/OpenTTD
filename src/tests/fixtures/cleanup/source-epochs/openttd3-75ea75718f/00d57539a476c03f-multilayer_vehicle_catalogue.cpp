/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_vehicle_catalogue.cpp Logical model/layer/cargo/commissioning definitions. @see docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md */

#include "stdafx.h"
#include "multilayer_content.h"

#include "safeguards.h"

static constexpr MultiLayerCapabilities _ground_layers{MapLayer::Surface, MapLayer::Underground, MapLayer::DeepUnderground};
static constexpr MultiLayerCapabilities _standard_tbm_layers{MapLayer::Surface, MapLayer::Underground};
static constexpr MultiLayerCapabilities _tube_layers{MapLayer::Underground, MapLayer::DeepUnderground};
static constexpr MultiLayerCapabilities _space_layer{MapLayer::Space};
static constexpr MultiLayerCapabilities _launch_layers{MapLayer::Surface, MapLayer::Space};

static constexpr CargoLabel _earth_cargo[] = {CargoLabel{"ERTH"}};
static constexpr CargoLabel _rock_cargo[] = {CargoLabel{"ROCK"}};
static constexpr CargoLabel _mineral_cargo[] = {CargoLabel{"COAL"}, CargoLabel{"IORE"}, CargoLabel{"CORE"}, CargoLabel{"BAUX"}, CargoLabel{"SALT"}, CargoLabel{"LITH"}, CargoLabel{"NORE"}, CargoLabel{"COBT"}, CargoLabel{"RARE"}, CargoLabel{"RFMT"}, CargoLabel{"STEL"}, CargoLabel{"SCMP"}, CargoLabel{"RBPT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}, CargoLabel{"SMIK"}, CargoLabel{"GOOD"}};
static constexpr CargoLabel _sealed_cargo[] = {CargoLabel{"URAN"}, CargoLabel{"RARE"}};
static constexpr CargoLabel _bulk_cargo[] = {CargoLabel{"COAL"}, CargoLabel{"IORE"}, CargoLabel{"CORE"}, CargoLabel{"BAUX"}, CargoLabel{"SALT"}, CargoLabel{"LITH"}, CargoLabel{"NORE"}, CargoLabel{"COBT"}, CargoLabel{"ERTH"}, CargoLabel{"ROCK"}};
static constexpr CargoLabel _container_cargo[] = {CargoLabel{"BATY"}, CargoLabel{"ELEC"}, CargoLabel{"GOOD"}, CargoLabel{"SMIK"}};
static constexpr CargoLabel _fluid_cargo[] = {CargoLabel{"CHEM"}, CargoLabel{"PROP"}};
static constexpr CargoLabel _return_cargo[] = {CargoLabel{"NORE"}, CargoLabel{"RARE"}, CargoLabel{"WICE"}, CargoLabel{"STEL"}, CargoLabel{"GOOD"}, CargoLabel{"RFMT"}, CargoLabel{"PROP"}};
static constexpr CargoLabel _construction_cargo[] = {CargoLabel{"STEL"}, CargoLabel{"GOOD"}};
static constexpr CargoLabel _heavy_cargo[] = {CargoLabel{"STEL"}, CargoLabel{"RFMT"}, CargoLabel{"SCMP"}, CargoLabel{"RBPT"}};
static constexpr CargoLabel _payload_cargo[] = {CargoLabel{"SCMP"}, CargoLabel{"RBPT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}, CargoLabel{"SMIK"}, CargoLabel{"RFMT"}, CargoLabel{"STEL"}, CargoLabel{"GOOD"}};
static constexpr CargoLabel _asteroid_cargo[] = {CargoLabel{"NORE"}, CargoLabel{"RARE"}, CargoLabel{"WICE"}};
static constexpr CargoLabel _orbital_cargo[] = {CargoLabel{"SCMP"}, CargoLabel{"RBPT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}, CargoLabel{"SMIK"}, CargoLabel{"RFMT"}, CargoLabel{"STEL"}, CargoLabel{"NORE"}, CargoLabel{"RARE"}, CargoLabel{"WICE"}, CargoLabel{"PROP"}, CargoLabel{"GOOD"}};

/* Logical IDs are append-only; revised Space definitions are independently versioned, and are never inserted into original engine tables. */
static const LayerVehicleCapability _vehicle_catalogue[] = {
	{0, "Standard TBM", _standard_tbm_layers, MultiLayerVehicleTransport::Rail, {}, {}, MultiLayerCommissioningFacility::RailPortalBay, MultiLayerServiceFacility::RailService, TimerGameCalendar::Year{1950}, true, true},
	{1, "Deep TBM", _ground_layers, MultiLayerVehicleTransport::Rail, {}, {}, MultiLayerCommissioningFacility::RailPortalBay, MultiLayerServiceFacility::RailService, TimerGameCalendar::Year{1980}, true, true},
	{2, "Earth hopper", _ground_layers, MultiLayerVehicleTransport::Rail, {CargoClass::Bulk}, _earth_cargo, MultiLayerCommissioningFacility::RailDepot, MultiLayerServiceFacility::RailService, TimerGameCalendar::Year{1950}, true, true},
	{3, "Rock hopper", _ground_layers, MultiLayerVehicleTransport::Rail, {CargoClass::Bulk}, _rock_cargo, MultiLayerCommissioningFacility::RailDepot, MultiLayerServiceFacility::RailService, TimerGameCalendar::Year{1950}, true, true},
	{4, "Conventional spoil hauler", _ground_layers, MultiLayerVehicleTransport::Rail, {}, {}, MultiLayerCommissioningFacility::RailDepot, MultiLayerServiceFacility::RailService, TimerGameCalendar::Year{1950}, true, true},
	{5, "Mineral and heavy freight wagon", _ground_layers, MultiLayerVehicleTransport::Rail, {CargoClass::Bulk, CargoClass::PieceGoods, CargoClass::Covered, CargoClass::Oversized}, _mineral_cargo, MultiLayerCommissioningFacility::RailDepot, MultiLayerServiceFacility::RailService, TimerGameCalendar::Year{1950}, true, false},
	{6, "Sealed mineral wagon", _ground_layers, MultiLayerVehicleTransport::Rail, {CargoClass::PieceGoods, CargoClass::Covered, CargoClass::Hazardous}, _sealed_cargo, MultiLayerCommissioningFacility::RailDepot, MultiLayerServiceFacility::RailService, TimerGameCalendar::Year{1980}, true, false},
	{7, "Mine bulk truck", _ground_layers, MultiLayerVehicleTransport::Road, {CargoClass::Bulk}, _bulk_cargo, MultiLayerCommissioningFacility::RoadDepot, MultiLayerServiceFacility::RoadService, TimerGameCalendar::Year{1950}, true, true},
	{8, "Sealed mineral carrier", _ground_layers, MultiLayerVehicleTransport::Road, {CargoClass::PieceGoods, CargoClass::Covered, CargoClass::Hazardous}, _sealed_cargo, MultiLayerCommissioningFacility::RoadDepot, MultiLayerServiceFacility::RoadService, TimerGameCalendar::Year{1980}, true, false},
	{9, "Mole-70 Bulk Capsule", _tube_layers, MultiLayerVehicleTransport::Tube, {CargoClass::Bulk}, _bulk_cargo, MultiLayerCommissioningFacility::TubeDepot, MultiLayerServiceFacility::TubeService, TimerGameCalendar::Year{1970}, false, true},
	{10, "Crate-75 Container Capsule", _tube_layers, MultiLayerVehicleTransport::Tube, {CargoClass::PieceGoods, CargoClass::Covered}, _container_cargo, MultiLayerCommissioningFacility::TubeDepot, MultiLayerServiceFacility::TubeService, TimerGameCalendar::Year{1975}, false, false},
	{11, "Cistern-80 Fluid Capsule", _tube_layers, MultiLayerVehicleTransport::Tube, {CargoClass::Liquid, CargoClass::Hazardous}, _fluid_cargo, MultiLayerCommissioningFacility::TubeDepot, MultiLayerServiceFacility::TubeService, TimerGameCalendar::Year{1980}, false, false},
	{12, "Shield-85 Mineral Capsule", _tube_layers, MultiLayerVehicleTransport::Tube, {CargoClass::PieceGoods, CargoClass::Covered, CargoClass::Hazardous}, _sealed_cargo, MultiLayerCommissioningFacility::TubeDepot, MultiLayerServiceFacility::TubeService, TimerGameCalendar::Year{1985}, false, false},
	{13, "Forge-05 Heavy Capsule", _tube_layers, MultiLayerVehicleTransport::Tube, {CargoClass::PieceGoods, CargoClass::Oversized}, _heavy_cargo, MultiLayerCommissioningFacility::TubeDepot, MultiLayerServiceFacility::TubeService, TimerGameCalendar::Year{2005}, false, false},
	{14, "Surface launch rocket", _launch_layers, MultiLayerVehicleTransport::Rocket, {CargoClass::Express, CargoClass::PieceGoods, CargoClass::Covered, CargoClass::Oversized}, _payload_cargo, MultiLayerCommissioningFacility::LaunchCenter, MultiLayerServiceFacility::LaunchRecovery, TimerGameCalendar::Year{1990}, false, false},
	{15, "Mining probe", _space_layer, MultiLayerVehicleTransport::Orbital, {CargoClass::Bulk, CargoClass::Covered, CargoClass::Refrigerated}, _asteroid_cargo, MultiLayerCommissioningFacility::RocketPayload, MultiLayerServiceFacility::OrbitalService, TimerGameCalendar::Year{1990}, false, false},
	{16, "Heavy construction robot", _space_layer, MultiLayerVehicleTransport::Orbital, {}, {}, MultiLayerCommissioningFacility::OrbitalAssembly, MultiLayerServiceFacility::OrbitalService, TimerGameCalendar::Year{1995}, false, false},
	{17, "Orbital cargo hauler", _space_layer, MultiLayerVehicleTransport::Orbital, {CargoClass::Bulk, CargoClass::Express, CargoClass::PieceGoods, CargoClass::Covered, CargoClass::Liquid, CargoClass::Refrigerated, CargoClass::Oversized}, _orbital_cargo, MultiLayerCommissioningFacility::OrbitalAssembly, MultiLayerServiceFacility::OrbitalService, TimerGameCalendar::Year{1995}, false, false},
	{18, "Return pod", _launch_layers, MultiLayerVehicleTransport::Descent, {CargoClass::Bulk, CargoClass::Covered, CargoClass::Refrigerated, CargoClass::PieceGoods, CargoClass::Express, CargoClass::Liquid}, _return_cargo, MultiLayerCommissioningFacility::RocketPayload, MultiLayerServiceFacility::LaunchRecovery, TimerGameCalendar::Year{1990}, false, false},
	{19, "Survey probe", _space_layer, MultiLayerVehicleTransport::Orbital, {}, {}, MultiLayerCommissioningFacility::RocketPayload, MultiLayerServiceFacility::OrbitalService, TimerGameCalendar::Year{1990}, false, false},
	{20, "Construction probe", _space_layer, MultiLayerVehicleTransport::Orbital, {CargoClass::PieceGoods, CargoClass::Express}, _construction_cargo, MultiLayerCommissioningFacility::RocketPayload, MultiLayerServiceFacility::OrbitalService, TimerGameCalendar::Year{1990}, false, false},
};

/**
 * Publish commissioning, service, refit, calendar and layer policy without allocating vehicles.
 * @return Complete immutable logical catalogue for the selected profile and its Space content revision.
 */
std::span<const LayerVehicleCapability> GetMultiLayerVehicleCapabilities()
{
	return _vehicle_catalogue;
}
