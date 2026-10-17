/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_riser.cpp Pure complete-floor and commissioned two-endpoint electrical riser admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5B
 * @see docs/zz-rhr/upgrades/2026-10/r2mlvs1-f-utility-and-transfer-contracts.md Group19
 */

#include "stdafx.h"
#include "geothermal_riser.h"
#include "geothermal_utility.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "company_base.h"
#include "company_func.h"
#include "vehicle_func.h"
#include "bridge_map.h"
#include "landscape.h"
#include "tbm.h"

#include "safeguards.h"

/** @copydoc PrepareGeothermalRiser */
GeothermalRiserAdmission PrepareGeothermalRiser(const GeothermalRiserRequest &request)
{
	GeothermalRiserAdmission result;
	// The actual funding company must own the operation. Spectator/deity queries
	// cannot grant a later command permission to appropriate somebody else's shaft.
	if (!Company::IsValidID(request.owner) || request.owner != _current_company) { result.error = GeothermalError::InvalidOwner; return result; }
	const auto profile = GetCurrentMultiLayerProfileSelection();
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser);
	// All native risers share the1980 technology gate, including an Underground
	// relay to Surface; the reciprocal portal's older date does not unlock the plant.
	if (terms == nullptr || profile.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile.legacy ||
		!CheckDeepLayerAvailability(profile, TimerGameCalendar::date) || TimerGameCalendar::date < TimerGameCalendar::ConvertYMDToDate(terms->introduction_year, 0, 1)) {
		result.error = GeothermalError::Unavailable; return result;
	}
	const auto key = GetLayerPortalPairKey(request.portal);
	if (!key) { result.error = GeothermalError::Endpoint; return result; }
	const auto destination = GetLayerPortalPairTile(request.portal);
	const auto layer = GetTileLayer(request.portal);
	// Export machinery stands on real rock-layer floor. Reciprocal portal policy
	// alone chooses its destination, preserving direct elevator geometry and the
	// prohibition on invented Surface-to-Deep rail or implicit intermediate rooms.
	if ((layer != MapLayer::Underground && layer != MapLayer::DeepUnderground) || !IsValidTile(destination)) {
		result.error = GeothermalError::Endpoint; return result;
	}
	const auto *record = GetLayerPortalWorkRecord(request.portal);
	// Both actual housings and the canonical work record must agree on ownership
	// and Connected incarnation; a copied/stale project label supplies no capacity.
	if (GetTileOwner(request.portal) != request.owner || GetTileOwner(destination) != request.owner || record == nullptr || record->company != request.owner) {
		result.error = GeothermalError::InvalidOwner; return result;
	}
	if (record->state != LayerPortalWorkState::Connected || record->revision == 0 || CheckLayerPortalAvailability(request.portal) != LayerPortalWorkError::None) {
		result.error = GeothermalError::Endpoint; return result;
	}
	// Check each actual endpoint's own plane. Surface needs no geological opening;
	// a direct Deep/Surface elevator does not touch the intervening Underground cell.
	for (const TileIndex endpoint : {request.portal, destination}) {
		if (GetTileLayer(endpoint) == MapLayer::Surface) continue;
		if (CheckExcavatedFootprint(std::array{endpoint}).error != ExcavationError::None) {
			result.error = GeothermalError::Footprint; result.failure_tile = endpoint; return result;
		}
	}
	// Reject clipped or cross-plane footprints before using native coordinate
	// helpers; an invalid rectangle must never wrap into another layer's floor.
	if (!IsValidTile(request.tile) || GetTileLayer(request.tile) != layer || request.access >= DiagDirection::End ||
		TileX(request.tile) + terms->width > Map::MaxX() || TilePlaneY(request.tile) + terms->height > Map::MaxY()) {
		result.error = GeothermalError::Footprint; return result;
	}
	if (CheckExcavationConstructionArea(request.tile, terms->width, terms->height, request.access).Failed()) {
		result.error = GeothermalError::Footprint; return result;
	}
	const auto public_approach = GetLayerPortalDoorTile(request.portal);
	if (!IsValidTile(public_approach)) { result.error = GeothermalError::Endpoint; return result; }
	// All four machinery cells remain separately excavated and unoccupied. Neither
	// funding nor a quote may overwrite paid transport, active retreat, utilities,
	// another blueprint or a vehicle to make room for electrical equipment. The
	// public approach stays reserved even if its track was temporarily removed.
	for (uint y = 0; y < terms->height; ++y) {
		for (uint x = 0; x < terms->width; ++x) {
			const auto tile = TileAddWrap(request.tile, x, y);
			if (tile == public_approach || !IsTileType(tile, TileType::Clear) || GetTileSlope(tile) != SLOPE_FLAT || IsBridgeAbove(tile) || EnsureNoVehicleOnGround(tile).Failed() ||
				GetTBMSupportTile(tile) != nullptr || GetGeothermalSiteClaimForTile(tile) != nullptr || HasGeothermalUtilityCell(tile)) {
				result.error = GeothermalError::Occupied; result.failure_tile = tile; return result;
			}
			result.footprint.push_back(tile);
		}
	}
	if (std::ranges::find(result.footprint, request.port) == result.footprint.end()) { result.error = GeothermalError::Endpoint; return result; }
	const int dx = static_cast<int>(TileX(request.port)) - static_cast<int>(TileX(request.portal));
	const int dy = static_cast<int>(TilePlaneY(request.port)) - static_cast<int>(TilePlaneY(request.portal));
	if (abs(dx) + abs(dy) != 1) { result.error = GeothermalError::Endpoint; return result; }
	result.destination = destination; result.portal_revision = record->revision; result.construction = terms; result.error = GeothermalError::None;
	return result;
}
