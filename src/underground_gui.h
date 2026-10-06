/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_gui.h Explicit underground construction entry context. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md */

#ifndef UNDERGROUND_GUI_H
#define UNDERGROUND_GUI_H

#include "multilayer_content.h"

struct Window;

/**
 * Open the construction interface for an explicitly requested rock layer.
 * @param layer Initial tool/menu layer, independent of any later camera switch.
 * @param policy Accepted profile snapshot used for display; commands revalidate the live owner.
 * @return The actual window, or nullptr for an invalid layer/profile.
 */
Window *ShowUndergroundConstructionToolbar(MapLayer layer, const MultiLayerProfileSelection &policy);

/**
 * Decide whether the airport entry represents underground construction.
 * @param layer Explicit viewport/menu layer, never simulation authority.
 * @return Whether this is one of the two rock layers.
 */
bool IsUndergroundConstructionLayer(MapLayer layer);

#endif /* UNDERGROUND_GUI_H */
