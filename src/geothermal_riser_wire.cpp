/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_riser_wire.cpp Pure copied-coordinate validation and native controller conversion for riser construction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group5B
 */

#include "stdafx.h"
#include "geothermal_riser.h"
#include "geothermal_riser_wire.h"
#include "company_type.h"
#include "safeguards.h"

/** @copydoc ValidateGeothermalRiserWire */
bool ValidateGeothermalRiserWire(const GeothermalRiserWire &row, uint32_t width, uint32_t height, uint32_t layers)
{
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser);
	// Bound copied dimensions before any coordinate arithmetic. Preview cannot
	// borrow a larger live map or an unlocked current company to admit this file.
	if (width < 64 || height < 64 || width > 4096 || height > 4096 || layers != 4 || row.industry >= 64000 ||
		(row.owner >= MAX_COMPANIES && row.owner != CompanyID::Invalid().base()) || row.access >= to_underlying(DiagDirection::End) ||
		row.portal_revision == 0 || row.portal_revision == UINT64_MAX || row.portal_retired > 1 || row.materials_committed > 1 || row.has_work_tick > 1 ||
		row.work_ticks > terms->work_ticks || (row.materials_committed != 0) != (row.work_ticks != 0) || (row.has_work_tick != 0) != (row.work_ticks != 0) ||
		(row.has_work_tick == 0 && row.last_work_tick != 0) || (row.owner == CompanyID::Invalid().base() && row.portal_retired == 0)) return false;
	const auto source = static_cast<MapLayer>(row.layer), destination = static_cast<MapLayer>(row.destination_layer);
	// Only real ground portal plane combinations are meaningful. No Space,
	// self-pair or Surface-side machinery can receive implicit riser capability.
	if ((source != MapLayer::Underground && source != MapLayer::DeepUnderground) ||
		(destination != MapLayer::Surface && destination != MapLayer::Underground && destination != MapLayer::DeepUnderground) || source == destination) return false;
	// Full floor, original reciprocal XY and adjoining port must fit the saved
	// usable plane. Retired machinery retains this original geometry too.
	if (static_cast<uint32_t>(row.x) + terms->width >= width || static_cast<uint32_t>(row.y) + terms->height >= height ||
		row.portal_x >= width - 1 || row.portal_y >= height - 1 || row.portal_x != row.destination_x || row.portal_y != row.destination_y ||
		row.port_x < row.x || row.port_x >= row.x + terms->width || row.port_y < row.y || row.port_y >= row.y + terms->height ||
		abs(static_cast<int>(row.port_x) - row.portal_x) + abs(static_cast<int>(row.port_y) - row.portal_y) != 1 ||
		(row.portal_x >= row.x && row.portal_x < row.x + terms->width && row.portal_y >= row.y && row.portal_y < row.y + terms->height)) return false;
	return true;
}

/** @copydoc EncodeGeothermalRiser */
GeothermalRiserWire EncodeGeothermalRiser(const GeothermalRiserState &state)
{
	return {state.industry.base(), state.site.owner.base(), to_underlying(GetTileLayer(state.site.tile)),
		static_cast<uint16_t>(TileX(state.site.tile)), static_cast<uint16_t>(TilePlaneY(state.site.tile)),
		static_cast<uint16_t>(TileX(state.site.portal)), static_cast<uint16_t>(TilePlaneY(state.site.portal)),
		static_cast<uint16_t>(TileX(state.site.port)), static_cast<uint16_t>(TilePlaneY(state.site.port)), to_underlying(GetTileLayer(state.destination)),
		static_cast<uint16_t>(TileX(state.destination)), static_cast<uint16_t>(TilePlaneY(state.destination)), to_underlying(state.site.access),
		state.portal_revision, state.portal_retired, state.work_ticks, state.materials_committed, state.has_work_tick, state.last_work_tick};
}

/** @copydoc DecodeGeothermalRiser */
GeothermalRiserState DecodeGeothermalRiser(const GeothermalRiserWire &row)
{
	const auto layer = static_cast<MapLayer>(row.layer);
	GeothermalRiserState state;
	state.industry = IndustryID{row.industry};
	state.site = {CompanyID{row.owner}, TileXYL(row.portal_x, row.portal_y, layer), TileXYL(row.x, row.y, layer), TileXYL(row.port_x, row.port_y, layer), static_cast<DiagDirection>(row.access)};
	state.destination = TileXYL(row.destination_x, row.destination_y, static_cast<MapLayer>(row.destination_layer));
	state.portal_revision = row.portal_revision; state.portal_retired = row.portal_retired != 0;
	state.work_ticks = row.work_ticks; state.materials_committed = row.materials_committed != 0;
	state.has_work_tick = row.has_work_tick != 0; state.last_work_tick = row.last_work_tick;
	return state;
}
