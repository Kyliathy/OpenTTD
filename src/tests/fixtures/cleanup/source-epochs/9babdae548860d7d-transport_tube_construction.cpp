/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_construction.cpp Pure tube floor, purchase and complete conversion admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-underground-transport-tubes.md Group2
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md authoritative footprint owner
 */

#include "stdafx.h"
#include "transport_tube.h"
#include "command_func.h"
#include "excavation.h"
#include "tbm.h"
#include "rail.h"
#include "road_map.h"
#include "pbs.h"
#include "vehicle_func.h"
#include "layer_portal_map.h"
#include "table/strings.h"

#include "safeguards.h"

/** @copydoc CheckTubeConstruction */
CommandCost CheckTubeConstruction(std::span<const TileIndex> footprint)
{
	if (footprint.empty()) return CMD_ERROR;
	const auto &selection = GetCurrentMultiLayerProfileSelection();
	const auto *profile = GetMultiLayerContentProfile(selection.id);
	const auto tube = GetTubeRailType();
	// Native construction requires the exact live profile and prior admitted
	// registry; Classic/editor legacy policy cannot activate a hidden tube family.
	if (tube == INVALID_RAILTYPE || profile == nullptr || selection.id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		selection.legacy || selection.version != profile->version) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
	if (TimerGameCalendar::date < GetRailTypeInfo(tube)->introduction_date) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
	if (!IsValidTile(footprint.front())) return CMD_ERROR;
	const auto layer = GetTileLayer(footprint.front());
	// All cells share one actual rock plane. Neither a camera nor a wrapped
	// rectangle can place a tube on Surface/Space or join separate layers.
	for (const auto tile : footprint) {
		if (!IsValidTile(tile) || GetTileLayer(tile) != layer ||
			(layer != MapLayer::Underground && layer != MapLayer::DeepUnderground)) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
	}
	// This canonical wrapper calls CheckExcavatedFootprint on every supplied
	// cell and retains the geological owner's precise solid/work/Deep refusal.
	return CheckExcavationConstruction(footprint);
}

/** @copydoc CheckTubeConstructionArea */
CommandCost CheckTubeConstructionArea(TileIndex tile, uint width, uint height, DiagDirection entrance, bool through)
{
	if (CommandCost domain = CheckTubeConstruction(std::array{tile}); domain.Failed()) return domain;
	// The shared owner bounds dimensions and collects real boundary approaches;
	// duplicating its rectangle math would risk a second world-plane authority.
	return CheckExcavationConstructionArea(tile, width, height, entrance, through);
}

/** @copydoc CheckTubeVehicleAdmission */
CommandCost CheckTubeVehicleAdmission(EngineID engine, TileIndex tile, CargoType cargo)
{
	if (!IsValidTile(tile)) return CMD_ERROR;
	const auto capsule = GetTubeVehicleCapability(engine);
	if (capsule.has_value() != IsTubeRailType(GetTileRailType(tile))) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
	if (!capsule) return {};
	if (CommandCost ground = CheckTubeConstruction(std::array{tile}); ground.Failed()) return ground;
	if (TimerGameCalendar::date < TimerGameCalendar::ConvertYMDToDate(capsule->catalogue->introduction_year, 0, 1)) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
	// Explicit labels, not a broadened mutable refit mask, retain the capsule's
	// physical cargo form and mandatory spoil hauling boundary.
	if (cargo != INVALID_CARGO && (cargo >= NUM_CARGO ||
		std::ranges::find(capsule->catalogue->cargo_labels, CargoSpec::Get(cargo)->label) == capsule->catalogue->cargo_labels.end())) return CommandCost(STR_ERROR_UNDERGROUND_VEHICLE_CAPABILITY);
	return {};
}

/** @copydoc CheckTubeRailConversion */
CommandCost CheckTubeRailConversion(std::span<const TileIndex> tiles, RailType target, DoCommandFlags flags)
{
	const bool to_tube = IsTubeRailType(target);
	// Scan the complete selected rail footprint before any command write. A bad
	// later tile must not leave an earlier tile converted or a paired shaft split.
	for (const auto tile : tiles) {
		if (!IsValidTile(tile)) return CMD_ERROR;
		const auto original = GetTileRailType(tile);
		if (original == INVALID_RAILTYPE) continue;
		if (!to_tube && !IsTubeRailType(original)) continue;
		if (IsLayerPortalTile(tile)) return CommandCost(STR_ERROR_MUST_REMOVE_LAYER_PORTAL_FIRST);
		// Underground tubes reuse rail geometry, but not ordinary paired tunnel/
		// bridge transport or level crossings. Dedicated capsules stay layer-local.
		if (IsTileType(tile, TileType::TunnelBridge) || IsLevelCrossingTile(tile)) return CMD_ERROR;
		if (CommandCost ground = CheckTubeConstruction(std::array{tile}); ground.Failed()) return ground;
		// A finished project's temporary record still restricts traffic. Its
		// existing paid support-conversion/removal owner must release that record
		// before the tile changes domain; bankruptcy is not a tube-conversion bypass.
		if (GetTBMSupportTile(tile) != nullptr) return CommandCost(STR_ERROR_TBM_SUPPORT_IN_USE);
		if (CommandCost support = CheckTBMSupportChange(tile, flags); support.Failed()) return support;
		if (CommandCost vehicles = EnsureNoVehicleOnGround(tile); vehicles.Failed()) return vehicles;
		if (GetReservedTrackbits(tile).Any()) return CommandCost(STR_ERROR_TRAIN_IN_THE_WAY);
	}
	return {};
}
