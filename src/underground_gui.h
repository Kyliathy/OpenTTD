/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_gui.h Explicit underground construction entry context. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md */

#ifndef UNDERGROUND_GUI_H
#define UNDERGROUND_GUI_H

#include "multilayer_content.h"
#include "tile_type.h"
#include "gfx_type.h"
#include "window_type.h"
#include "vehicle_type.h"
#include "company_type.h"

struct Window;

/**
 * Open the construction interface for an explicitly requested rock layer.
 * @param layer Initial tool/menu layer, independent of any later camera switch.
 * @param policy Industry profile snapshot that must still match the live accepted policy; commands revalidate construction separately.
 * @return The actual window, or nullptr for a non-rock/unallocated layer, Classic, legacy or stale profile.
 */
Window *ShowUndergroundConstructionToolbar(MapLayer layer, const MultiLayerProfileSelection &policy);

/**
 * Decide whether the airport entry represents underground construction.
 * @param layer Explicit viewport/menu layer, never simulation authority.
 * @return Whether this is one of the two rock layers.
 */
bool IsUndergroundConstructionLayer(MapLayer layer);

/** Separate presentation domains; logical capsule rows never confer ordinary rail or portal permission. */
enum class UndergroundFleetKind : uint8_t { ConventionalFreight, TubeCapsules };

/**
 * Filter immutable logical designs for an explicit construction/fleet context.
 * @param layer Actual requested rock layer, not a live camera reference.
 * @param policy Explicit accepted profile/version; Classic or malformed policy offers no native fleet.
 * @param date Synchronized calendar date for the proposed operation, not economy or wall time.
 * @param kind Conventional rail/road freight or separate tube capsule presentation.
 * @return Profile-owned design pointers in catalogue order; no engine IDs are allocated and commands retain permission.
 */
std::vector<const LayerVehicleCapability *> BuildUndergroundVehicleFilter(MapLayer layer,
	const MultiLayerProfileSelection &policy, TimerGameCalendar::Date date, UndergroundFleetKind kind);

/**
 * Inspect actual owned capsule fronts for the explicit rock-plane fleet control.
 * @param layer Actual native vehicle layer, never a camera projection.
 * @param company Explicit actual company; absent/foreign identities convey no permission.
 * @return Native vehicle identities in pool order; no fleet, packet or electrical registry is created.
 */
std::vector<VehicleID> BuildUndergroundTubeFleetFilter(MapLayer layer, CompanyID company);

/**
 * Query a resource marker only while the real survey window owns the placement cursor.
 * @param tile Explicit world tile being rendered, independent of the current main-camera layer.
 * @return Native public/free seam marker, or no overlay for another tool, layer or locked context.
 */
std::optional<SpriteID> GetUndergroundSurveyOverlaySprite(TileIndex tile);

/** Query a tile-aligned natural vent only while its actual survey cursor is armed.
 * @param tile Explicit physical cell; camera and mineral-icon offsets supply no geometry.
 * @return Existing Ground vent pose, or absence outside the live unlocked survey.
 */
std::optional<SpriteID> GetUndergroundGeothermalSurveyOverlaySprite(TileIndex tile);

/**
 * Inspect the actual underground control/row presentation using its live window state.
 * @param window Actual underground construction window; another window class receives no icon.
 * @param widget Existing control/row identity.
 * @return Selected native icon, or no icon for another context.
 */
std::optional<SpriteID> GetUndergroundConstructionWidgetSprite(const Window &window, WidgetID widget);

#endif /* UNDERGROUND_GUI_H */
