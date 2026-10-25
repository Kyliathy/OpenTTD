/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_order_preview.h Same-file supply validation without native failure cleanup.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * The existing native load-check wrapper remains the sole corrupt-save error boundary.
 */
#ifndef SURFACE_SUPPLY_ORDER_PREVIEW_H
#define SURFACE_SUPPLY_ORDER_PREVIEW_H

#include <optional>
#include <string>

/** Check the current copied file's complete supply, cargo and reference evidence.
 * Reads the existing load-check candidate and bound lineage. Legacy conversion
 * updates only that candidate; it never installs rights or changes the live world.
 * Component observers may inspect refusals without invoking cleanup for an
 * unrelated full-load attempt. Native callers use ValidateSurfaceSupplyOrderLoadPreview.
 * @return Original corruption reason, or no value when the copied join is valid.
 */
std::optional<std::string> GetSurfaceSupplyOrderLoadPreviewError();

#endif /* SURFACE_SUPPLY_ORDER_PREVIEW_H */
