/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_utility.h Paid native rock-layer utility installation and deterministic physical paths, using shared link identities.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups3B-5
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group7 public facade
 * @see docs/zz-rhr/upgrades/2026-10/r2mlvs1-f-utility-and-transfer-contracts.md paid utility-cell extension
 */

#ifndef GEOTHERMAL_UTILITY_H
#define GEOTHERMAL_UTILITY_H

#include "geothermal_type.h"
#include "utility_power.h"
#include "command_type.h"

/** Read-only installed-cell view; the shared native link record alone owns identity, geometry, controller and capacity. */
struct GeothermalUtilityLink {
	UtilityLinkID id{UINT32_MAX}; ///< Issued shared identity, never a feature-local counter.
	CompanyID owner = CompanyID::Invalid(); ///< Actual active infrastructure controller.
	UtilityEnergyDomain domain = UtilityEnergyDomain::Heat; ///< Separate layer-local network.
	TileIndex tile = INVALID_TILE; ///< Installed excavated rock cell; Heat stays Deep-local.
	uint64_t capacity_w = 0; ///< Frozen physical rate; neither an energy store nor source generation.
};

/** Complete pure per-cell quote; successful preparation creates no installation, energy or grant. */
struct GeothermalUtilityAdmission {
	GeothermalError error = GeothermalError::Unavailable; ///< Whole unchanged refusal or None.
	UtilityLinkID id{UINT32_MAX}; ///< Predicted build identity or actual removal target.
	const MultiLayerLocalUtilityLinkContract *terms = nullptr; ///< Borrowed canonical capital/removal/rate definition.
};

/** Deterministic derived route with explicit installed-cell references; it owns no reservation or consumed energy. */
struct GeothermalUtilityPath {
	uint64_t topology_revision = 0; ///< Exact committed geometry/control snapshot used to find this route.
	uint64_t capacity_w = 0; ///< Minimum actual installed rate along the full path.
	std::vector<UtilityLinkID> cells; ///< Stable cardinal route, including both explicitly installed endpoint cells.
};

/** @param tile Explicit world cell. @param domain Actual network. @return Active installed cell view or absence; proximity/queries never build a connection. */
std::optional<GeothermalUtilityLink> GetGeothermalUtilityLink(TileIndex tile, UtilityEnergyDomain domain);
/** @param tile Actual native machinery or portal cell. @param domain Installed network; only electricity can attach to a riser port or owned portal housing. @return Actual declared port owner or absence; a cold plan/generic industry provides no port. */
std::optional<CompanyID> GetGeothermalUtilityPortOwner(TileIndex tile, UtilityEnergyDomain domain = UtilityEnergyDomain::Heat);
/** @param tile Actual world cell. @param domain Installed network. @param remove Quote removal rather than construction. @return Full date/company/floor/port/cash/occupancy admission without mutation. */
GeothermalUtilityAdmission PrepareGeothermalUtilityLink(TileIndex tile, UtilityEnergyDomain domain, bool remove = false);
/**
 * Find one bounded path through actual installed cells only.
 * @param owner Actual company controlling every cell.
 * @param domain Heat or electricity; no cross-domain or cross-layer adjacency.
 * @param source Explicit installed source port.
 * @param destination Explicit installed consumer port.
 * @param excluded Installed cells with no residual capacity in this transient allocation; no live geometry changes.
 * @return Deterministic shortest cardinal route and bottleneck, or absence; no energy or grant is created.
 */
std::optional<GeothermalUtilityPath> FindGeothermalUtilityPath(CompanyID owner, UtilityEnergyDomain domain, TileIndex source, TileIndex destination, std::span<const UtilityLinkID> excluded = {});
/** Clear derived indices after world reset/accepted load; shared installed geometry and resource authority remain untouched. */
void ResetGeothermalUtilityPathCache();
/** Invalidate derived topology after a nonrefusible lifecycle event, even when the saved revision has saturated; no shared resource or geometry is changed here. */
void InvalidateGeothermalUtilityPathCache();
/** @return Diagnostic rebuild count, independent of gameplay; resource-only operations must not rebuild topology. */
uint64_t GetGeothermalUtilityCacheBuildCount();
/** @return Diagnostic bounded path-search count; unchanged unconstrained paths and disconnected components reuse derived topology without reading this value for gameplay. */
uint64_t GetGeothermalUtilityPathSearchCount();
/** @param tile Native construction cell. @return Whether no active utility domain here belongs to another company; this guard creates no path or ownership. */
bool CanConstructOverGeothermalUtility(TileIndex tile);
/** @param tile Actual native cell. @return Whether either paid utility domain is installed here; building footprints must remove it explicitly before incompatible replacement. */
bool HasGeothermalUtilityCell(TileIndex tile);
/** @return Whether actual active installed cells resolve their native floor/coexistence/declared ports after other feature owners load; no geometry or energy is repaired. */
bool ValidateGeothermalUtilityLinks();

#endif /* GEOTHERMAL_UTILITY_H */
