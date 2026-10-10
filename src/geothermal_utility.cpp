/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_utility.cpp Paid rock-layer pipe/cable overlays and bounded paths; shared utility records retain installed identity and every resource quantity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups3B-5
 */

#include "stdafx.h"
#include "geothermal_utility.h"
#include <set>
#include "geothermal_utility_cmd.h"
#include "command_func.h"
#include "geothermal_collector.h"
#include "geothermal_riser.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "company_base.h"
#include "company_func.h"
#include "industry.h"
#include "tbm.h"
#include "rail_map.h"
#include "pbs.h"
#include "vehicle_func.h"
#include "economy_func.h"
#include "openttd.h"
#include "viewport_func.h"
#include <deque>
#include "safeguards.h"

static uint64_t _utility_cache_revision = UINT64_MAX; ///< Derived index snapshot; never a saved topology or energy authority.
static uint64_t _utility_cache_builds = 0; ///< Diagnostic only; no simulation decision reads it.
static std::map<std::tuple<CompanyID, UtilityEnergyDomain, TileIndex>, UtilityLinkID> _utility_cells; ///< Derived bounded references, with no copied rates or resource balances.
static std::map<std::pair<TileIndex, UtilityEnergyDomain>, UtilityLinkID> _utility_tile_cells; ///< Derived owner-independent construction lookup; shared records alone retain control/capacity.
static std::map<UtilityLinkID, TileIndex> _utility_components; ///< Derived stable smallest-world-tile component roots, isolated by actual company/domain.
static std::map<std::tuple<CompanyID, UtilityEnergyDomain, TileIndex, TileIndex>, GeothermalUtilityPath> _utility_paths; ///< Bounded unconstrained canonical path references; residual exclusions remain transient and uncached.
static size_t _utility_path_references = 0; ///< Derived stored-reference bound, never a resource quantity.
static uint64_t _utility_path_searches = 0; ///< Diagnostic only; path choice and synchronized state never read it.

/** @copydoc ResetGeothermalUtilityPathCache */
void ResetGeothermalUtilityPathCache()
{
	_utility_cells.clear(); _utility_tile_cells.clear(); _utility_components.clear(); _utility_paths.clear();
	_utility_path_references = 0; _utility_path_searches = 0; _utility_cache_revision = UINT64_MAX; _utility_cache_builds = 0;
}
/** @copydoc InvalidateGeothermalUtilityPathCache */
void InvalidateGeothermalUtilityPathCache() { _utility_cache_revision = UINT64_MAX; }
/** @copydoc GetGeothermalUtilityCacheBuildCount */
uint64_t GetGeothermalUtilityCacheBuildCount() { return _utility_cache_builds; }
/** @copydoc GetGeothermalUtilityPathSearchCount */
uint64_t GetGeothermalUtilityPathSearchCount() { return _utility_path_searches; }

/** Rebuild only after an actual shared geometry/control revision; publication and rendering cannot invent or renew an installed path. */
static void EnsureGeothermalUtilityCache()
{
	if (_utility_cache_revision == GetUtilityLocalLinkRevision()) return;
	_utility_cells.clear();
	_utility_tile_cells.clear();
	_utility_components.clear(); _utility_paths.clear(); _utility_path_references = 0;
	// Iterate bounded installed identity history, never all map tiles. Each
	// active company/domain/cell has one shared reference and one paid capacity.
	for (const auto &link : GetUtilityPowerLinks()) {
		if (link.kind != UtilityLinkKind::LocalCell || !link.active) continue;
		_utility_cells.emplace(std::tuple{link.local_owner, link.local_domain, link.source_endpoint}, link.id);
		_utility_tile_cells.emplace(std::pair{link.source_endpoint, link.local_domain}, link.id);
	}
	// Stable company/domain/world-tile order labels each connected component
	// once on a committed topology change. Only installed cells are visited;
	// later source/consumer pairs can reject disconnection without a BFS.
	for (const auto &[key, id] : _utility_cells) {
		if (_utility_components.contains(id)) continue;
		const auto &[owner, domain, root] = key;
		std::deque<TileIndex> pending{root}; _utility_components.emplace(id, root);
		while (!pending.empty()) {
			const auto tile = pending.front(); pending.pop_front();
			std::array adjacent{TileAddWrap(tile, -1, 0), TileAddWrap(tile, 0, -1), TileAddWrap(tile, 1, 0), TileAddWrap(tile, 0, 1)};
			std::ranges::sort(adjacent);
			for (const auto next : adjacent) {
				if (!IsValidTile(next) || GetTileLayer(next) != GetTileLayer(tile)) continue; // Native plane boundaries cannot connect cables without a separately paid riser.
				const auto cell = _utility_cells.find(std::tuple{owner, domain, next});
				if (cell == _utility_cells.end() || _utility_components.contains(cell->second)) continue; // Only same-owner/domain installed cardinal neighbors enter their component once.
				_utility_components.emplace(cell->second, root); pending.push_back(next);
			}
		}
	}
	_utility_cache_revision = GetUtilityLocalLinkRevision(); ++_utility_cache_builds;
}

/** @copydoc GetGeothermalUtilityLink */
std::optional<GeothermalUtilityLink> GetGeothermalUtilityLink(TileIndex tile, UtilityEnergyDomain domain)
{
	EnsureGeothermalUtilityCache();
	const auto found = _utility_tile_cells.find(std::pair{tile, domain});
	if (found == _utility_tile_cells.end()) return std::nullopt;
	const auto *link = GetUtilityPowerLink(found->second);
	return GeothermalUtilityLink{link->id, link->local_owner, domain, tile, link->local_capacity_w};
}

/** @copydoc GetGeothermalUtilityPortOwner */
std::optional<CompanyID> GetGeothermalUtilityPortOwner(TileIndex tile, UtilityEnergyDomain domain)
{
	if (!IsValidTile(tile)) return std::nullopt;
	if (IsLayerPortalTile(tile)) {
		if (domain != UtilityEnergyDomain::Electricity) return std::nullopt;
		const auto *work = GetLayerPortalWorkRecord(tile);
		// Cable attachment retains paid physical geometry during sealing. Only
		// an operational paid riser may subsequently carry inter-layer power.
		if (work == nullptr || !Company::IsValidID(work->company) || GetTileOwner(tile) != work->company ||
			GetTileOwner(GetLayerPortalPairTile(tile)) != work->company) return std::nullopt;
		return work->company;
	}
	if (!IsTileType(tile, TileType::Industry)) return std::nullopt;
	const auto *industry = Industry::GetByTile(tile);
	if (domain == UtilityEnergyDomain::Electricity) {
		const auto *riser = GetGeothermalRiserState(industry->index);
		// Preparatory cable may attach to funded machinery, but only its exact
		// retained boundary port. The native Industry owner remains neutral;
		// this paid association alone owns the controller, as for collectors.
		if (riser != nullptr && riser->site.port == tile && Company::IsValidID(riser->site.owner) &&
			IsGeothermalRiserDefinition(industry->type)) return riser->site.owner;
	}
	const auto *state = GetGeothermalCollectorState(industry->index);
	if (state == nullptr) return std::nullopt;
	const auto *claim = GetGeothermalSiteClaim(state->site);
	// Only the retained paid machinery's declared boundary port can attach.
	// Construction may still wait for materials; that provides no usable heat.
	if (claim == nullptr || claim->active || claim->site.endpoint != tile || !Company::IsValidID(state->owner) || !IsGeothermalCollectorDefinition(industry->type)) return std::nullopt;
	return state->owner;
}

/** @copydoc CanConstructOverGeothermalUtility */
bool CanConstructOverGeothermalUtility(TileIndex tile)
{
	// Physical overlays coexist with owned native transport, while ordinary
	// overbuilding cannot appropriate another company's paid network.
	for (const auto domain : {UtilityEnergyDomain::Heat, UtilityEnergyDomain::Electricity}) {
		const auto cell = GetGeothermalUtilityLink(tile, domain);
		if (cell.has_value() && cell->owner != _current_company) return false;
	}
	return true;
}

/** @copydoc HasGeothermalUtilityCell */
bool HasGeothermalUtilityCell(TileIndex tile)
{
	return GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Heat).has_value() || GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity).has_value();
}

/** @copydoc PrepareGeothermalUtilityLink */
GeothermalUtilityAdmission PrepareGeothermalUtilityLink(TileIndex tile, UtilityEnergyDomain domain, bool remove)
{
	GeothermalUtilityAdmission result; result.terms = GetMultiLayerLocalUtilityLinkContract(domain);
	const auto &profile = GetCurrentMultiLayerProfileSelection();
	// Real company/profile/calendar gates precede any physical identity or
	// expense. A viewed Deep plane or editor mode cannot commission a pipe.
	if (_game_mode != GameMode::Normal || profile.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile.legacy ||
		!CheckDeepLayerAvailability(profile, TimerGameCalendar::date)) return result;
	if (!Company::IsValidID(_current_company)) { result.error = GeothermalError::InvalidOwner; return result; }
	if (result.terms == nullptr) { result.error = GeothermalError::InvalidKind; return result; }
	// Heat stays Deep-local; electrical relays also require paid Underground
	// floor. Protected cuts, cold reservations and support are never replaced.
	if (!IsValidTile(tile) || !IsMultiLayerLocalUtilityLayer(domain, GetTileLayer(tile)) || CheckExcavatedFootprint(std::array{tile}).error != ExcavationError::None ||
		GetTBMSupportTile(tile) != nullptr || GetGeothermalSiteClaimForTile(tile) != nullptr) { result.error = GeothermalError::Footprint; return result; }
	if (EnsureNoVehicleOnGround(tile).Failed()) { result.error = GeothermalError::Occupied; return result; }
	if (!CanConstructOverGeothermalUtility(tile)) { result.error = GeothermalError::InvalidOwner; return result; }
	// Native transport keeps all metadata. Only declared machinery ports and
	// electrically attached owned housings admit cable over special tiles.
	if (IsTileType(tile, TileType::Industry) || IsLayerPortalTile(tile)) {
		if (GetGeothermalUtilityPortOwner(tile, domain) != std::optional<CompanyID>{_current_company}) { result.error = GeothermalError::Endpoint; return result; }
		if (IsLayerPortalTile(tile) && GetLayerPortalTransportType(tile) == TransportType::Rail && GetLayerPortalReservationTrackBits(tile).Any()) { result.error = GeothermalError::Occupied; return result; } // A reserved rail housing cannot be rewired beneath its admitted traffic; road housings have no rail reservation field.
	} else if (!IsTileType(tile, TileType::Clear) && !IsTileType(tile, TileType::Railway) && !IsTileType(tile, TileType::Road)) {
		result.error = GeothermalError::Occupied; return result;
	} else if (IsTileType(tile, TileType::Railway) || IsTileType(tile, TileType::Road)) {
		if (GetTileOwner(tile) != _current_company || (IsTileType(tile, TileType::Railway) && GetReservedTrackbits(tile).Any())) { result.error = GeothermalError::Occupied; return result; }
	}
	const auto installed = GetGeothermalUtilityLink(tile, domain);
	if (remove) {
		if (!installed.has_value()) { result.error = GeothermalError::Endpoint; return result; }
		result.id = installed->id;
		if (RemoveUtilityPowerLink(result.id, false) != UtilityPowerError::None) { result.error = GeothermalError::Refused; return result; }
	} else {
		if (installed.has_value()) { result.error = GeothermalError::Occupied; return result; }
		const auto id = RegisterUtilityLocalLink(_current_company, domain, tile, result.terms->capacity_w, false);
		if (!id.has_value()) { result.error = GeothermalError::Limit; return result; }
		result.id = *id;
	}
	const auto cost = remove ? result.terms->removal_cost : result.terms->capital_cost;
	if (GetAvailableMoneyForCommand() < Money{cost}) { result.error = GeothermalError::Refused; return result; }
	result.error = GeothermalError::None; return result;
}

/** @copydoc CmdBuildGeothermalUtilityLink */
CommandCost CmdBuildGeothermalUtilityLink(DoCommandFlags flags, TileIndex tile, UtilityEnergyDomain domain)
{
	const auto quote = PrepareGeothermalUtilityLink(tile, domain);
	if (quote.error != GeothermalError::None) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		if (RegisterUtilityLocalLink(_current_company, domain, tile, quote.terms->capacity_w) != std::optional<UtilityLinkID>{quote.id}) return CMD_ERROR;
		MarkTileDirtyByTile(tile); // Presentation redraw cannot alter the underlying native transport or metered supply.
	}
	return CommandCost(ExpensesType::Construction, Money{quote.terms->capital_cost});
}

/** @copydoc CmdRemoveGeothermalUtilityLink */
CommandCost CmdRemoveGeothermalUtilityLink(DoCommandFlags flags, TileIndex tile, UtilityEnergyDomain domain)
{
	const auto quote = PrepareGeothermalUtilityLink(tile, domain, true);
	if (quote.error != GeothermalError::None) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		if (RemoveUtilityPowerLink(quote.id) != UtilityPowerError::None) return CMD_ERROR;
		MarkTileDirtyByTile(tile);
	}
	return CommandCost(ExpensesType::Construction, Money{quote.terms->removal_cost});
}

/** @copydoc FindGeothermalUtilityPath */
std::optional<GeothermalUtilityPath> FindGeothermalUtilityPath(CompanyID owner, UtilityEnergyDomain domain, TileIndex source, TileIndex destination, std::span<const UtilityLinkID> excluded)
{
	const auto &profile = GetCurrentMultiLayerProfileSelection();
	// Physical routing observes actual source technology/companies, never a
	// camera or implicit same-coordinate edge between independent planes.
	if (_game_mode != GameMode::Normal || profile.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile.legacy ||
		!Company::IsValidID(owner) || GetMultiLayerLocalUtilityLinkContract(domain) == nullptr || !CheckDeepLayerAvailability(profile, TimerGameCalendar::date) ||
		!IsValidTile(source) || !IsValidTile(destination) || !IsMultiLayerLocalUtilityLayer(domain, GetTileLayer(source)) || GetTileLayer(destination) != GetTileLayer(source)) return std::nullopt;
	EnsureGeothermalUtilityCache();
	if (!_utility_cells.contains(std::tuple{owner, domain, source}) || !_utility_cells.contains(std::tuple{owner, domain, destination})) return std::nullopt;
	const auto source_id = _utility_cells.at(std::tuple{owner, domain, source}), destination_id = _utility_cells.at(std::tuple{owner, domain, destination});
	if (_utility_components.at(source_id) != _utility_components.at(destination_id)) return std::nullopt;
	const auto key = std::tuple{owner, domain, source, destination};
	if (excluded.empty()) {
		const auto cached = _utility_paths.find(key);
		if (cached != _utility_paths.end()) return cached->second;
	}
	const std::set<UtilityLinkID> unavailable(excluded.begin(), excluded.end());
	// A saturated port cannot be bypassed; every delivered share traverses
	// both paid endpoints even when an alternate interior route exists.
	if (unavailable.contains(_utility_cells.at(std::tuple{owner, domain, source})) || unavailable.contains(_utility_cells.at(std::tuple{owner, domain, destination}))) return std::nullopt;
	++_utility_path_searches;
	std::deque<TileIndex> pending{source}; std::map<TileIndex, TileIndex> previous{{source, INVALID_TILE}};
	// Breadth-first traversal visits each installed cell at most once. Sorted
	// world tile order selects a deterministic shortest route through a cycle.
	while (!pending.empty()) {
		const auto tile = pending.front(); pending.pop_front();
		if (tile == destination) break;
		std::array adjacent{TileAddWrap(tile, -1, 0), TileAddWrap(tile, 0, -1), TileAddWrap(tile, 1, 0), TileAddWrap(tile, 0, 1)};
		std::ranges::sort(adjacent);
		for (const auto next : adjacent) {
			if (!IsValidTile(next) || GetTileLayer(next) != GetTileLayer(tile)) continue; // Local routing cannot cross native planes; only explicit operational riser edges can do so.
			if (!_utility_cells.contains(std::tuple{owner, domain, next}) || previous.contains(next)) continue; // Only installed same-company/domain neighbors enter this route once.
			if (unavailable.contains(_utility_cells.at(std::tuple{owner, domain, next}))) continue; // Full interior cells cannot grant another share; sorted BFS chooses an installed residual alternative.
			previous.emplace(next, tile); pending.push_back(next);
		}
	}
	if (!previous.contains(destination)) return std::nullopt;
	GeothermalUtilityPath result; result.topology_revision = _utility_cache_revision; result.capacity_w = MAX_UTILITY_RATE_W;
	// Reconstruct exact paid identities and rates from shared authority, with
	// the destination-to-source walk bounded by the visited installed cells.
	for (auto tile = destination; tile != INVALID_TILE; tile = previous.at(tile)) {
		const auto id = _utility_cells.at(std::tuple{owner, domain, tile}); const auto *link = GetUtilityPowerLink(id);
		result.cells.push_back(id); result.capacity_w = std::min(result.capacity_w, link->local_capacity_w);
	}
	std::ranges::reverse(result.cells);
	// Cache only geometry-dependent canonical paths within fixed reference
	// bounds. Dynamic residual exclusions never become topology authority or
	// persist another consumer's current capacity decision into a later tick.
	if (excluded.empty() && _utility_paths.size() < MAX_UTILITY_INTERVAL_ACCOUNTS && _utility_path_references + result.cells.size() <= MAX_UTILITY_POWER_SHARES) {
		_utility_paths.emplace(key, result); _utility_path_references += result.cells.size();
	}
	return result;
}

/** @copydoc ValidateGeothermalUtilityLinks */
bool ValidateGeothermalUtilityLinks()
{
	// Resolve installed geometry against actual loaded owners only after the
	// native collector/cold floor has loaded. Vehicle occupancy is legitimate
	// saved traffic on coexisting transport, unlike a new construction action.
	for (const auto &link : GetUtilityPowerLinks()) {
		if (link.kind != UtilityLinkKind::LocalCell || !link.active) continue;
		const auto tile = link.source_endpoint;
		if (!IsValidTile(tile) || GetTileLayer(tile) != link.local_layer || !IsMultiLayerLocalUtilityLayer(link.local_domain, link.local_layer) || CheckExcavatedFootprint(std::array{tile}).error != ExcavationError::None ||
			GetTBMSupportTile(tile) != nullptr || GetGeothermalSiteClaimForTile(tile) != nullptr) return false;
		if (IsTileType(tile, TileType::Industry) || IsLayerPortalTile(tile)) {
			if (GetGeothermalUtilityPortOwner(tile, link.local_domain) != std::optional<CompanyID>{link.local_owner}) return false;
		} else if (IsTileType(tile, TileType::Railway) || IsTileType(tile, TileType::Road)) {
			if (GetTileOwner(tile) != link.local_owner) return false;
		} else if (!IsTileType(tile, TileType::Clear)) return false;
	}
	return true;
}
