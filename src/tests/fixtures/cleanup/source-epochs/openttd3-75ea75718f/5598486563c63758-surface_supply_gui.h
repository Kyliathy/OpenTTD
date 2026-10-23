/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_gui.h Transient paid-supply controls composed into the ordinary native StationView.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 */
#ifndef SURFACE_SUPPLY_GUI_H
#define SURFACE_SUPPLY_GUI_H

#include "surface_supply.h"
#include "window_type.h"
#include <string>

struct Window;

/** Existing station window's local request state; native commands/history/packets alone own purchases, credits and stock. */
class SurfaceSupplyControls {
	StationID station; ///< Actual parent station; normal StationView deletion closes this component too.
	CargoLabel label{"STEL"}; ///< Draft canonical meaning only, never a newly registered cargo or source stock.
	uint16_t quantity = 1; ///< Complete intended native units; admission never silently reduces a request.
	SurfaceSupplyOrderID order{0}; ///< Selected actual history identity; no row pointer survives a synchronized command.
	bool expanded = false; ///< Transient foldout state, independent of service/date/owner admission.
	bool quantity_query = false; ///< Routes only this component's native quantity editor callback.
	std::string guidance; ///< Last input-format guidance; live price/refund/refusal comes from actual native admission.

	/** @return Whether the native parent has a meaningful owned Surface service or retained refund history to display. */
	bool IsVisible() const;
	/** @return Actual selected same-station/same-player history, or nullptr after removal/takeover/stale selection. */
	const SurfaceSupplyOrder *SelectedOrder() const;
	/** @return Pure complete native purchase quote under the local player's actual company; restores the caller actor. */
	SurfaceSupplyQuote QuotePurchase() const;
	/** @return Pure complete native original-source cancellation quote under the actual local player. */
	SurfaceSupplyCancellation QuoteCancellation() const;

public:
	/** @param station Actual StationView constructor identity; viewing another layer never redirects the request. */
	explicit SurfaceSupplyControls(StationID station);
	/** @param window Existing parent after CreateNestedTree and before FinishInitNested; configure only local layout visibility. */
	void Configure(Window &window) const;
	/** @param window Existing parent; refresh native-backed button admission without buying, cancelling, reserving or advancing time. */
	void Refresh(Window &window) const;
	/** @param window Existing station owner. @param widget Actual clicked control. @return Whether this component handled it. */
	bool OnClick(Window &window, WidgetID widget);
	/** @param window Existing station owner. @param widget Native dropdown. @param index Selected canonical option or real order ID. @return Whether handled. */
	bool OnDropdownSelect(Window &window, WidgetID widget, int index);
	/** @param window Existing native quantity editor parent. @param text Complete input or cancellation. @return Whether this callback belonged to the quantity request. */
	bool OnQueryTextFinished(Window &window, const std::optional<std::string> &text);
	/** @param widget Existing foldout label. @return Native-backed display text, or absence for the parent's unrelated controls. */
	std::optional<std::string> GetWidgetText(WidgetID widget) const;
};

#endif /* SURFACE_SUPPLY_GUI_H */
