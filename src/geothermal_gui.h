/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_gui.h Transient geothermal controls composed into the actual Underground Construction window.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups15CB/15CC/15CDB.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 */
#ifndef GEOTHERMAL_GUI_H
#define GEOTHERMAL_GUI_H

#include "underground_gui.h"
#include "geothermal_controls.h"
#include "command_type.h"
#include "utility_power.h"

/** Existing window's explicit selection/draft state; native claims, industries, cargo and utilities retain every simulation quantity. */
class UndergroundGeothermalControls {
	/** Only the armed component receives map clicks; selecting a family or opening a fold-out conveys no construction right. */
	enum class Placement : uint8_t { None, Survey, Site, Utility, Riser };
	/** Optional presentation pages share the same native placement owner. */
	enum class Page : uint8_t { Sites, Utilities, Town, Information };
	/** Separate bounded information views read original native quantities without accumulating history. */
	enum class Information : uint8_t { Construction, Energy, Town };
	Information information = Information::Construction; ///< Selected live information only, never a saved facility state.
	Page page = Page::Sites; ///< Visible controls only; changing pages cancels uncommitted placement.
	UtilityEnergyDomain domain = UtilityEnergyDomain::Heat; ///< Explicit installed network, never inferred from a selected producer.
	bool remove_cell = false; ///< Explicit single-cell removal action, with no energy or capital refund.
	TileIndex portal = INVALID_TILE; ///< Clicked original sending housing for a new paid riser.
	GeothermalTownServiceRequest service; ///< Actual machinery/terminal/town choices only; no account IDs or energy quantities.
	MapLayer layer; ///< Explicit construction context, independent of later camera changes.
	bool expanded = false; ///< Optional presentation only.
	Placement placement = Placement::None; ///< Actual parent cursor operation.
	bool input_refused = false; ///< Preserve a last wrong-plane input warning until the next valid click; never construction permission.
	MultiLayerGeothermalConstructionKind kind = MultiLayerGeothermalConstructionKind::Collector; ///< Immutable selected family.
	GeothermalVentID vent{UINT32_MAX}; ///< Explicit existing original source; no copied capacity or discovery state.
	GeothermalSiteID site{UINT32_MAX}; ///< Explicit actually issued plan selected after publication; never a predicted funding identity.
	TileIndex anchor = INVALID_TILE; ///< First clicked world corner of an uncommitted site request.
	TileIndex endpoint = INVALID_TILE; ///< Explicit second clicked boundary connection cell.
	DiagDirection access = DiagDirection::SE; ///< Explicit outward access, independently selectable before confirmation.
	TileIndex inspected = INVALID_TILE; ///< Actual clicked native geology/site/machinery to re-query.
	std::string status; ///< Input guidance or command submission/refusal; never a saved work state.

	/** @return Explicit current request only; this creates no claim, material, work or energy. */
	GeothermalSiteRequest SiteRequest() const;
	/** @return Borrowed actually selected same-company/same-layer plan, including its consumed history, or nullptr. */
	const GeothermalSiteClaim *SelectedSite() const;
	/** @return Complete pure synchronized planning estimate, without reserving an identity. */
	CommandCost QuotePlan() const;
	/** @return Complete pure native machinery funding estimate over the selected actual active plan. */
	CommandCost QuoteFunding() const;
	/** @return Whole pure native riser funding estimate over explicitly clicked original geometry. */
	CommandCost QuoteRiser() const;
	/** @return Whole pure native town connection/recovery estimate over re-queried real participants. */
	CommandCost QuoteService() const;
	/** @return Whether the selected actual source/terminal/town and ordered risers match its retained active native graph; this is inspection, never connection permission. */
	bool IsSelectedServiceConnected() const;
	/** @return Bounded selected information read from current native stock/work/meters; no quantity is retained or renewed. */
	std::string GetInformationText() const;

public:
	/** @param layer Explicit requested rock plane, never a live viewport reference. */
	explicit UndergroundGeothermalControls(MapLayer layer);
	/** @param window Existing construction window. @param locked Shared Deep calendar lock; public inspection needs no company construction authority. Refresh only native-backed button presentation. */
	void Refresh(Window &window, bool locked) const;
	/** @param window Existing owner. @param widget Actual clicked control. @return Whether this component handled the control. */
	bool OnClick(Window &window, WidgetID widget);
	/** @param window Existing owner. @param widget Actual menu. @param index Explicit current native identity/content choice. @return Whether handled, including a cancelled own menu. */
	bool OnDropdownSelect(Window &window, WidgetID widget, int index);
	/** @param window Actual parent placement owner. @param tile Actual explicit clicked world tile. @return Whether this armed component consumed the input. */
	bool OnPlaceObject(Window &window, TileIndex tile);
	/** Cancel only the transient placement geometry; actual selected plans/machinery/stock remain with their original owners. */
	void Abort();
	/** @param widget Actual geothermal label/status row. @return Live native-backed text, or absence for another widget family. */
	std::optional<std::string> GetWidgetText(WidgetID widget) const;
	/** Read only the actual armed natural-vent survey. @param tile Explicit physical survey cell. @return Native vent cue, or absence outside this live unlocked survey. */
	std::optional<SpriteID> GetSurveySprite(TileIndex tile) const;
	/** @return Bounded information height; ordinary action guidance keeps its original short row. */
	uint GetStatusLines() const { return this->page == Page::Information ? 20 : 5; }
	/** @return Whether this component owns the parent's current uncommitted placement. */
	bool IsPlacing() const { return this->placement != Placement::None; }
};

#endif /* GEOTHERMAL_GUI_H */
