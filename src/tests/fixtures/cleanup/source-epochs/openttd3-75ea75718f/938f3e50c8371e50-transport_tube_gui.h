/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_gui.h Transient tube controls composed into the existing Underground Construction window.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups12C/6BDC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 */

#ifndef TRANSPORT_TUBE_GUI_H
#define TRANSPORT_TUBE_GUI_H

#include "underground_gui.h"
#include "transport_tube_cmd.h"

static constexpr int TUBE_GEOTHERMAL_SOURCE_MENU_BASE = MAX_UTILITY_INTERVAL_ACCOUNTS; ///< Disjoint transient menu namespace above all shared producer IDs; never transported as an authority ID.

/** Existing underground window's presentation/request state; native vehicles, facilities, commands and power retain authority. */
class UndergroundTubeControls {
	MapLayer layer; ///< Explicit menu layer; a camera switch never redirects a drafted route.
	bool expanded = false; ///< Optional fold-out only, never technology permission.
	bool placing = false; ///< Whether this component currently owns its parent's placement cursor.
	bool geothermal_route = false; ///< Plant/inlet selection instead of drawing an external service route.
	IndustryID generator = IndustryID::Invalid(); ///< Explicit clicked original completed native plant.
	UtilityProducerID generator_output{UINT32_MAX}; ///< Nonreused output pins that exact plant incarnation across pending commands.
	TileIndex geothermal_inlet = INVALID_TILE; ///< Explicit clicked native facility; cable must already be purchased.
	TileIndex portal = INVALID_TILE; ///< Explicitly selected commissioned endpoint for the draft.
	TubePowerCommandPath path; ///< Bounded transient request, not installed utility state.
	VehicleID vehicle = VehicleID::Invalid(); ///< Actual selected capsule identity, rechecked before each action.
	UtilityProducerID source{UINT32_MAX}; ///< Explicit selected existing inlet identity, never inferred from coordinates.
	TubeGeothermalSourceID geothermal_source{UINT32_MAX}; ///< Mutually exclusive original native inlet selection, never cast to a producer ID.
	uint16_t model = 9; ///< Canonical design chosen only for the cargo/date preview.
	std::string status; ///< Last input guidance/refusal; live packets and waits are queried separately.

	/** @return Actual selected owned same-layer capsule, or nullptr after a stale/foreign/domain change. */
	const struct Train *SelectedVehicle() const;
	/** @return Pure current route quote; the source alone owns prices and topology admission. */
	CommandCost QuoteConnection() const;
	/** @return Pure selected capsule/provider bind or reconnect quote with current owner, date and topology admission. */
	CommandCost QuoteBinding() const;
	/** @param external Original external producer or invalid. @param geothermal Original native inlet or invalid. @return Current owned same-layer provider label, or absence for a stale/foreign source. */
	std::optional<std::string> SourceText(UtilityProducerID external, TubeGeothermalSourceID geothermal) const;

public:
	/** @param layer Explicit Underground Construction layer, independent of camera changes. */
	explicit UndergroundTubeControls(MapLayer layer);
	/** @param window Existing underground owner. @param locked Parent company/layer lock. Refresh read-only button admission. */
	void Refresh(Window &window, bool locked) const;
	/** @param window Existing owner. @param widget Clicked control. @return Whether this component handled the widget. */
	bool OnClick(Window &window, WidgetID widget);
	/** @param window Existing owner. @param widget Dropdown control. @param index Actual selected identity. @return Whether handled. */
	bool OnDropdownSelect(Window &window, WidgetID widget, int index);
	/** @param window Existing placement owner. @param tile Actual clicked world tile. @return Whether an armed tube route consumed the input. */
	bool OnPlaceObject(Window &window, TileIndex tile);
	/** Cancel only the transient route request; installed sources, vehicles and orders are untouched. */
	void Abort();
	/** @param widget Existing fold-out label. @return Dynamic native-backed text, or absence for another control. */
	std::optional<std::string> GetWidgetText(WidgetID widget) const;
	/** @return Whether this component owns an uncommitted route placement. */
	bool IsPlacing() const { return this->placing; }
};

#endif /* TRANSPORT_TUBE_GUI_H */
