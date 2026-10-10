/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_readiness.cpp Read-only native tube facility, model/cargo and physical electrical readiness.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group8
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.2
 */

#include "stdafx.h"
#include "transport_tube.h"
#include "station_base.h"
#include "station_map.h"
#include "depot_base.h"
#include "depot_map.h"
#include "rail_map.h"
#include "company_base.h"
#include "multilayer_bootstrap.h"
#include "timer/timer_game_economy.h"
#include "safeguards.h"

/** @copydoc GetTubeTerminal */
std::optional<TubeTerminal> GetTubeTerminal(TileIndex tile, CompanyID company)
{
	// Actual current owner, semantic tube rail and geological/calendar admission
	// precede native pool access; coordinates and cached views confer no authority.
	if (!IsValidTile(tile) || !Company::IsValidID(company) || GetTileOwner(tile) != company.base() ||
		!IsTubeRailType(GetTileRailType(tile)) || CheckTubeConstruction(std::array{tile}).Failed()) return std::nullopt;
	TubeTerminal view; view.tile = tile; view.owner = company; view.layer = GetTileLayer(tile); view.guideway = GetTubeRailType();
	if (IsRailStationTile(tile)) {
		const Station *station = Station::GetIfValid(GetStationIndex(tile));
		// A blocked platform or orphan/foreign station ID cannot become an
		// independent stock owner through a presentation readiness query.
		if (station == nullptr || station->owner != company || IsStationTileBlocked(tile)) return std::nullopt;
		view.station = station->index; view.platform_length_tiles = station->GetPlatformLength(tile);
	} else if (IsRailDepotTile(tile)) {
		const Depot *depot = Depot::GetIfValid(GetDepotIndex(tile));
		if (depot == nullptr || depot->xy != tile) return std::nullopt; // The actual pool and map identity must agree before service is advertised.
		view.depot = depot->index;
	} else {
		return std::nullopt;
	}
	return view;
}

/** @copydoc CanTubeTerminalServeVehicle */
bool CanTubeTerminalServeVehicle(TileIndex tile, CompanyID company, EngineID engine, CargoType cargo)
{
	if (!GetTubeTerminal(tile, company).has_value()) return false;
	if (!GetTubeVehicleCapability(engine).has_value()) return false;
	return CheckTubeVehicleAdmission(engine, tile, cargo).Succeeded();
}

/** @copydoc GetTubeNetworkReadiness */
std::optional<TubeNetworkReadiness> GetTubeNetworkReadiness(TileIndex tile, CompanyID company)
{
	const auto terminal = GetTubeTerminal(tile, company);
	if (!terminal.has_value()) return std::nullopt;
	TubeNetworkReadiness view; view.terminal = *terminal;
	const Money cash = Company::Get(company)->money;
	const auto &terms = GetMultiLayerTubePowerContract();
	// Only bounded installed sources participate, in their stable identity order.
	// Native circuit geometry supplies physical reach; same-coordinate layers,
	// foreign tracks and a copied compatibility mask cannot add an inlet/grant.
	for (const auto &connection : CaptureTubePowerConnections()) {
		if (!connection.active || connection.owner != company) continue; // A retired or foreign paid contract cannot advertise readiness for this company.
		if (ValidateTubePowerConnections(std::array{connection}, GetCurrentMultiLayerProfileSelection()) != TubePowerError::None) continue;
		if (!IsTubePowerConnectedTile(connection.producer, tile)) continue;
		const bool current_lease = connection.lease_day == TimerGameEconomy::date.base();
		view.inlets.push_back({connection.producer, current_lease || cash >= Money{terms.lease_per_day}, cash >= Money{1}});
	}
	return view;
}
