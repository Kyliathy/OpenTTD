/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_site.cpp Pure whole-floor natural-source site admission before any collector/plant claim or delivered construction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group1
 */

#include "stdafx.h"
#include "geothermal.h"
#include "company_base.h"
#include "company_func.h"
#include "vehicle_func.h"
#include "tile_map.h"
#include "safeguards.h"

/** @copydoc PrepareGeothermalSite */
GeothermalSiteAdmission PrepareGeothermalSite(const GeothermalSiteRequest &request)
{
	GeothermalSiteAdmission result;
	if (!Company::IsValidID(request.owner) || request.owner != _current_company) { result.error = GeothermalError::InvalidOwner; return result; } // A foreign/deity actor cannot reserve another company's future source/site.
	const auto &profile = GetCurrentMultiLayerProfileSelection();
	const auto *terms = GetMultiLayerGeothermalConstruction(request.kind);
	// A first collector/powerhouse site cannot impersonate a commissioned portal
	// riser; that later facility has both-endpoint transport/export admission.
	if (terms == nullptr || request.kind == MultiLayerGeothermalConstructionKind::Riser) { result.error = GeothermalError::InvalidKind; return result; }
	// Exact Native policy and synchronized technology precede any geological
	// lookup; public discovery grants no legacy/editor or pre1980 construction.
	if (profile.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile.legacy || !CheckDeepLayerAvailability(profile, TimerGameCalendar::date) ||
		TimerGameCalendar::date < TimerGameCalendar::ConvertYMDToDate(terms->introduction_year, 0, 1)) { result.error = GeothermalError::Unavailable; return result; }
	const auto &geology = GetGeologyWorldState();
	const auto vent = std::ranges::find(geology.vents, request.vent, &GeothermalVentField::id);
	if (vent == geology.vents.end()) { result.error = GeothermalError::InvalidVent; return result; }
	// Invalid addressing, wrong planes and a missing real access side cannot
	// become a clipped footprint or a same-coordinate cross-layer utility.
	if (!IsValidTile(request.tile) || GetTileLayer(request.tile) != MapLayer::DeepUnderground || request.access >= DiagDirection::End ||
		TileX(request.tile) + terms->width > Map::MaxX() || TilePlaneY(request.tile) + terms->height > Map::MaxY()) { result.error = GeothermalError::Footprint; return result; }
	if (CheckExcavationConstructionArea(request.tile, terms->width, terms->height, request.access).Failed()) { result.error = GeothermalError::Footprint; return result; }
	bool contacts_source = false;
	// Every physical facility cell must remain excavated and empty. Its explicit
	// source contact is checked across the whole floor rather than trusting one
	// anchor or a blueprint to open rock or overwrite infrastructure/vehicles.
	for (uint y = 0; y < terms->height; ++y) {
		for (uint x = 0; x < terms->width; ++x) {
			const auto tile = TileAddWrap(request.tile, x, y);
			if (!IsTileType(tile, TileType::Clear) || EnsureNoVehicleOnGround(tile).Failed()) { result.error = GeothermalError::Occupied; result.failure_tile = tile; return result; }
			result.footprint.push_back(tile);
			const auto px = TileX(tile), py = TilePlaneY(tile);
			contacts_source |= px >= vent->x && py >= vent->y && px < vent->x + static_cast<uint>(vent->width) && py < vent->y + static_cast<uint>(vent->height);
		}
	}
	// Only collectors require direct natural-field contact. A cold powerhouse
	// may be planned elsewhere, but later actual utility/heat admission must
	// establish its explicit connection rather than paying for proximity.
	if (request.kind == MultiLayerGeothermalConstructionKind::Collector && !contacts_source) { result.error = GeothermalError::InvalidVent; return result; }
	const auto endpoint = std::ranges::find(result.footprint, request.endpoint);
	if (endpoint == result.footprint.end()) { result.error = GeothermalError::Endpoint; return result; }
	const auto dx = TileX(request.endpoint) - TileX(request.tile), dy = TilePlaneY(request.endpoint) - TilePlaneY(request.tile);
	if (dx != 0 && dy != 0 && dx + 1 != terms->width && dy + 1 != terms->height) { result.error = GeothermalError::Endpoint; return result; } // A usable boundary port connects a later real utility edge; an interior arbitrary point cannot imply remote reach.
	result.vent = &*vent; result.construction = terms; result.error = GeothermalError::None;
	return result;
}
