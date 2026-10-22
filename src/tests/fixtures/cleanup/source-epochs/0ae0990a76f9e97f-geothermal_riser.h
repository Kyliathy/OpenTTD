/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_riser.h Complete physical admission for a portal's separately funded electrical riser.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group5B
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#ifndef GEOTHERMAL_RISER_H
#define GEOTHERMAL_RISER_H

#include "geothermal.h"

static constexpr uint16_t GEOTHERMAL_RISER_LOCAL_ID = 0x8310; ///< Native IIDS/TIDS family, separate from collector tiles8300..8308; no private runtime ID pool.

/** @param activated Exact admitted native package. @return Whether the complete native riser definition is installed without publishing machinery or electrical flow. */
bool InitialiseGeothermalRiserDefinitions(const GRFConfigList &activated);
/** @return Existing exact native riser IndustryType, or IT_INVALID when absent/malformed; no ID is allocated by this query. */
IndustryType GetGeothermalRiserIndustryType();
/** @param type Native IndustryType. @return Whether the exact package/layout/input-only machinery definition belongs to this riser family. */
bool IsGeothermalRiserDefinition(IndustryType type);
/** @param type Native IndustryType. @return Borrowed rock-layer/1980 capability for the exact riser family, or nullptr. */
const LayerIndustryCapability *GetGeothermalRiserIndustryCapability(IndustryType type);

/** Explicit rock-side machinery floor and its physical connection to one real commissioned portal. */
struct GeothermalRiserRequest {
	CompanyID owner = CompanyID::Invalid(); ///< Company funding both-endpoint access and the complete machinery.
	TileIndex portal = INVALID_TILE; ///< Actual rock-side sending endpoint; its reciprocal housing supplies the receiving layer.
	TileIndex tile = INVALID_TILE; ///< Top-left cell of the complete frozen2x2 machinery footprint.
	TileIndex port = INVALID_TILE; ///< Boundary machinery cell cardinally adjoining the sending portal housing.
	DiagDirection access = DiagDirection::Invalid; ///< Outward construction/loading approach, separately excavated.
};

/** Pure construction admission; a successful quote provides no machinery, materials, electricity or link identity. */
struct GeothermalRiserAdmission {
	GeothermalError error = GeothermalError::Refused; ///< First complete unchanged refusal, or None.
	TileIndex failure_tile = INVALID_TILE; ///< Actual rejected housing/floor where known.
	TileIndex destination = INVALID_TILE; ///< Native reciprocal endpoint, never a computed plane offset.
	uint64_t portal_revision = 0; ///< Exact Connected incarnation to revalidate before actual funding or export.
	const MultiLayerGeothermalConstruction *construction = nullptr; ///< Borrowed frozen capital/material/work terms.
	std::vector<TileIndex> footprint; ///< Complete excavated machinery floor, without modifying the portal's through-route.
};

/**
 * Quote complete riser geometry against both actual housings, calendar, excavation and native occupancy.
 * @param request Explicit company, sending portal, machinery floor, connection port and construction access.
 * @return Pure whole-site permission/refusal; later paid construction must revalidate it and supply real delivered materials.
 */
GeothermalRiserAdmission PrepareGeothermalRiser(const GeothermalRiserRequest &request);

#endif /* GEOTHERMAL_RISER_H */
