/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_claim.cpp Bounded cold collector/plant planning claims, independent of completed construction and every energy/cargo owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group1
 */

#include "stdafx.h"
#include "geothermal_collector.h"
#include "company_base.h"
#include "company_func.h"
#include "tile_map.h"
#include "core/backup_type.hpp"
#include <set>
#include "safeguards.h"

static std::vector<GeothermalSiteClaim> _geothermal_sites; ///< Sole cold planning identity/reservation owner; no capacity, stock, grant or work balance.
static std::optional<std::vector<GeothermalSiteClaim>> _pending_geothermal_sites; ///< Unaccepted parsed transport, invisible to gameplay.

/** @copydoc GetGeothermalSiteClaim */
const GeothermalSiteClaim *GetGeothermalSiteClaim(GeothermalSiteID id)
{
	if (id.base() >= _geothermal_sites.size()) return nullptr;
	return &_geothermal_sites[id.base()];
}

/** @param claim Actual retained planning row. @param tile Valid world cell. @return Exact active same-layer rectangular floor membership; access cells remain owned by ordinary transport. */
static bool IsClaimFloor(const GeothermalSiteClaim &claim, TileIndex tile)
{
	if (!claim.active) return false;
	const auto *terms = GetMultiLayerGeothermalConstruction(claim.site.kind);
	if (terms == nullptr) return false;
	// One active cold floor reservation follows actual world geometry. A
	// coincident upper layer or neighboring approach is never this claim's floor.
	if (GetTileLayer(tile) != GetTileLayer(claim.site.tile)) return false;
	const auto x = TileX(tile), y = TilePlaneY(tile), origin_x = TileX(claim.site.tile), origin_y = TilePlaneY(claim.site.tile);
	return x >= origin_x && y >= origin_y && x < origin_x + terms->width && y < origin_y + terms->height;
}

/** @copydoc GetGeothermalSiteClaimForTile */
const GeothermalSiteClaim *GetGeothermalSiteClaimForTile(TileIndex tile)
{
	if (!IsValidTile(tile)) return nullptr;
	// Bounded retained planning rows suffice for this foundation query; no
	// per-plane scan or coordinate projection can acquire a source reservation.
	for (const auto &claim : _geothermal_sites) {
		if (IsClaimFloor(claim, tile)) return &claim;
	}
	return nullptr;
}

/** @copydoc ClaimGeothermalSite */
GeothermalSiteResult ClaimGeothermalSite(const GeothermalSiteRequest &request, bool execute)
{
	const auto admission = PrepareGeothermalSite(request);
	if (admission.error != GeothermalError::None) return {admission.error};
	if (request.kind == MultiLayerGeothermalConstructionKind::Collector && FindGeothermalCollectorForVent(request.vent) != IndustryID::Invalid()) return {GeothermalError::Claimed}; // Actual unfinished/abandoned machinery retains source exclusivity after its cold reservation was consumed.
	if (_geothermal_sites.size() >= MAX_GEOTHERMAL_SITE_CLAIMS) return {GeothermalError::Limit};
	// A cold floor cannot be reserved twice, and one source's collector cannot
	// be claimed again by either this company or a competitor. Later generators
	// retain separate site identities while sharing the same natural heat budget.
	for (const auto &existing : _geothermal_sites) {
		if (!existing.active) continue;
		if (request.kind == MultiLayerGeothermalConstructionKind::Collector && existing.site.kind == request.kind && existing.site.vent == request.vent) return {GeothermalError::Claimed};
		for (const auto tile : admission.footprint) {
			if (IsClaimFloor(existing, tile)) return {GeothermalError::Claimed};
		}
	}
	const GeothermalSiteID id{static_cast<uint32_t>(_geothermal_sites.size())};
	if (execute) _geothermal_sites.push_back({id, request, true});
	return {GeothermalError::None, id};
}

/** @copydoc CancelGeothermalSite */
GeothermalError CancelGeothermalSite(GeothermalSiteID id, bool execute)
{
	const auto *claim = GetGeothermalSiteClaim(id);
	// Retired IDs and foreign/deity actors cannot release another owner's floor
	// or reuse a historical blueprint as an already paid collector/plant.
	if (claim == nullptr || !claim->active || claim->site.owner != _current_company) return GeothermalError::Refused;
	if (execute) _geothermal_sites[id.base()].active = false;
	return GeothermalError::None;
}

/** @copydoc CaptureGeothermalSiteClaims */
std::vector<GeothermalSiteClaim> CaptureGeothermalSiteClaims() { return _geothermal_sites; }

/** @copydoc ValidateGeothermalSiteClaims */
GeothermalError ValidateGeothermalSiteClaims(std::span<const GeothermalSiteClaim> rows, bool world_refs)
{
	if (rows.size() > MAX_GEOTHERMAL_SITE_CLAIMS) return GeothermalError::Limit;
	std::set<GeothermalVentID> collectors;
	std::set<TileIndex> floors;
	// Dense retained identities and active source/floor exclusivity preserve
	// one cold reservation without repairing cancelled history or duplicates.
	for (size_t index = 0; index < rows.size(); ++index) {
		const auto &claim = rows[index]; const auto &site = claim.site;
		const auto *terms = GetMultiLayerGeothermalConstruction(site.kind);
		// Scalar transport bounds precede every actual reference or rectangle;
		// hostile rows cannot impersonate a riser or recycle another planning ID.
		if (claim.id.base() != index || site.vent.base() == UINT32_MAX || site.owner.base() >= MAX_COMPANIES || terms == nullptr ||
			site.kind == MultiLayerGeothermalConstructionKind::Riser || site.tile == INVALID_TILE || site.endpoint == INVALID_TILE || site.access >= DiagDirection::End) return GeothermalError::Refused;
		if (world_refs) {
			// Cancellation preserves the original natural/geometry references.
			// Historical rows need no live old company or unchanged empty floor,
			// but cannot introduce orphan sources or an arbitrary remote endpoint.
			if (!IsValidTile(site.tile) || !IsValidTile(site.endpoint) || GetTileLayer(site.tile) != MapLayer::DeepUnderground ||
				GetTileLayer(site.endpoint) != MapLayer::DeepUnderground || TileX(site.tile) + terms->width > Map::MaxX() ||
				TilePlaneY(site.tile) + terms->height > Map::MaxY()) return GeothermalError::Footprint;
			const auto &vents = GetGeologyWorldState().vents;
			if (std::ranges::find(vents, site.vent, &GeothermalVentField::id) == vents.end()) return GeothermalError::InvalidVent;
			const auto x = TileX(site.tile), y = TilePlaneY(site.tile), ex = TileX(site.endpoint), ey = TilePlaneY(site.endpoint);
			if (ex < x || ey < y || ex >= x + terms->width || ey >= y + terms->height ||
				(ex != x && ey != y && ex + 1 != x + terms->width && ey + 1 != y + terms->height)) return GeothermalError::Endpoint;
		}
		if (!claim.active) continue;
		if (site.kind == MultiLayerGeothermalConstructionKind::Collector && !collectors.insert(site.vent).second) return GeothermalError::Claimed; // Two cold collectors cannot each establish ownership of the same natural source.
		if (!world_refs) continue;
		if (!Company::IsValidID(site.owner)) return GeothermalError::InvalidOwner;
		AutoRestoreBackup actor(_current_company, site.owner);
		const auto admission = PrepareGeothermalSite(site);
		if (admission.error != GeothermalError::None) return admission.error;
		// Overlapping active rectangles retain no partial permission during load;
		// the later mutation owner must receive one completely valid cold registry.
		for (const auto tile : admission.footprint) {
			if (!floors.insert(tile).second) return GeothermalError::Claimed;
		}
	}
	return GeothermalError::None;
}

/** @copydoc StageLoadedGeothermalSiteClaims */
void StageLoadedGeothermalSiteClaims(std::vector<GeothermalSiteClaim> rows) { _pending_geothermal_sites = std::move(rows); }
/** @copydoc AcceptLoadedGeothermalSiteClaims */
bool AcceptLoadedGeothermalSiteClaims()
{
	if (!_pending_geothermal_sites.has_value()) return true;
	if (ValidateGeothermalSiteClaims(*_pending_geothermal_sites) != GeothermalError::None) return false;
	_geothermal_sites = std::move(*_pending_geothermal_sites); _pending_geothermal_sites.reset(); return true;
}
/** @copydoc ResetGeothermalSiteClaims */
void ResetGeothermalSiteClaims()
{
	_geothermal_sites.clear(); _pending_geothermal_sites.reset(); ResetGeothermalSiteLoadStaging();
}
/** @copydoc ChangeGeothermalSiteCompanyOwnership */
void ChangeGeothermalSiteCompanyOwnership(Owner old_owner, [[maybe_unused]] Owner new_owner)
{
	// Unbuilt plans lose their old-company reservation before tile/pool reuse;
	// no completed construction, cargo or heat exists in this cold registry.
	for (auto &claim : _geothermal_sites) {
		if (claim.active && claim.site.owner == old_owner) claim.active = false;
	}
}
/** @copydoc ComputeGeothermalSiteDigest */
uint64_t ComputeGeothermalSiteDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Explicit cold planning scalar; hash fixed bytes without padding, camera or host order. */
	const auto field = [&hash](uint64_t value) {
		for (uint byte = 0; byte < 8; ++byte) { hash ^= value & 0xFF; hash *= 1099511628211ULL; value >>= 8; }
	};
	field(_geothermal_sites.size());
	// Canonical original geometry and cancellation history are included once;
	// natural rates, electrical balances, packets and construction keep their owners.
	for (const auto &claim : _geothermal_sites) {
		field(claim.id.base()); field(claim.site.vent.base()); field(claim.site.owner.base()); field(to_underlying(claim.site.kind));
		field(claim.site.tile.base()); field(claim.site.endpoint.base()); field(to_underlying(claim.site.access)); field(claim.active);
	}
	return hash;
}
