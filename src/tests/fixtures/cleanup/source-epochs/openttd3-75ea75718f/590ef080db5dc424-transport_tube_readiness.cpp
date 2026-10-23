/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_readiness.cpp Read-only native tube facility, model/cargo and physical electrical readiness.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups8/6BDC/13CC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.2
 */

#include "stdafx.h"
#include "transport_tube.h"
#include "transport_tube_geothermal.h"
#include "geothermal_generator.h"
#include "station_base.h"
#include "station_map.h"
#include "depot_base.h"
#include "depot_map.h"
#include "rail_map.h"
#include "company_base.h"
#include "multilayer_bootstrap.h"
#include "excavation.h"
#include "excavation_state.h"
#include "timer/timer_game_economy.h"
#include "safeguards.h"

/** @copydoc GetBuiltTubeTerminal */
std::optional<TubeTerminal> GetBuiltTubeTerminal(TileIndex tile, CompanyID company)
{
	// Retained native identity survives a locked date. Actual profile, owner,
	// rock layer and paid geological floor still precede any native pool access;
	// this pure view cannot confer today's construction or operating permission.
	if (!IsValidTile(tile) || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		!Company::IsValidID(company) || GetTileOwner(tile) != company.base() || !IsTubeRailType(GetTileRailType(tile))) return std::nullopt;
	const auto layer = GetTileLayer(tile);
	if (layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) return std::nullopt; // Surface/Space cannot become tube facilities through a retained rail label.
	if (GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated) return std::nullopt;
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

/** @copydoc GetTubeTerminal */
std::optional<TubeTerminal> GetTubeTerminal(TileIndex tile, CompanyID company)
{
	const auto terminal = GetBuiltTubeTerminal(tile, company);
	if (!terminal.has_value()) return std::nullopt;
	if (CheckTubeConstruction(std::array{tile}).Failed()) return std::nullopt;
	return terminal;
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
	// Native sources retain their independent inlet identities even when they
	// share one generator. Physical reach guarantees neither Heat nor a grant.
	for (const auto &source : CaptureTubeGeothermalPowerState().sources) {
		if (!source.active || source.owner != company || !IsTubeGeothermalPowerConnectedTile(source.id, tile)) continue;
		view.geothermal_inlets.push_back({source.id, source.generator, HasGeothermalGeneratorServiceFunds(source.generator, 1)});
	}
	return view;
}
