/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_projects_gui.cpp Compact native browser of actual owned excavation projects.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group2
 * @see docs/zz-reach2/architecture/gui/archi-gui.md
 */

#include "stdafx.h"
#include "tbm_gui.h"
#include "tbm_cmd.h"
#include "tbm_controls.h"
#include "tbm_hangar.h"
#include "depot_base.h"
#include "layer_portal_cmd.h"
#include "dropdown_func.h"
#include "core/backup_type.hpp"
#include "command_func.h"
#include "underground_gui.h"
#include "window_gui.h"
#include "window_func.h"
#include "company_base.h"
#include "company_func.h"
#include "train.h"
#include "vehicle_gui.h"
#include "map_layer_gui.h"
#include "viewport_func.h"
#include "strings_func.h"
#include "gfx_func.h"
#include "palette_func.h"
#include "zoom_func.h"
#include "timer/timer.h"
#include "timer/timer_window.h"
#include "widgets/tbm_widget.h"
#include "table/strings.h"

#include "safeguards.h"

/** @copydoc GetTBMVehicleStatus */
std::optional<std::string> GetTBMVehicleStatus(const Train &front)
{
	const auto problem = GetTBMOrderProblem(front);
	if (!problem) return std::nullopt;
	if (const auto *project = GetTBMProjectForCutter(front.index); project != nullptr && project->phase == TBMPhase::FollowOrders &&
			front.current_order.IsGotoOrder() && project->blocked == TBMBlockedReason::None) return std::nullopt; // Show the actual ordinary station/waypoint destination while its list owns travel.
	// A genuine service visit or station handling keeps its native caption.
	// These orders own that physical action even while a work assignment exists.
	if (front.current_order.IsType(OT_LOADING)) return std::nullopt;
	if (front.current_order.IsType(OT_GOTO_DEPOT) && !IsTBMStagingHangarOrder(front)) {
		const auto *project = GetTBMProjectForCutter(front.index);
		// Native destination captions remain useful during normal travel. A
		// missing/full hangar or blocked route must instead name the actual wait.
		if (project == nullptr || project->phase != TBMPhase::TravelToHangar || project->blocked == TBMBlockedReason::None) return std::nullopt;
	}
	// Native held trains can retain the last travelling speed in their display
	// cache. Cutting, waiting and an actually stationary engine must show zero.
	const bool stationary = front.cur_speed == 0 || IsTBMHoldingTrain(front) || IsTBMHaulerHeld(front);
	const auto speed = PackVelocity(stationary ? 0 : front.GetDisplaySpeed(), VehicleType::Train);
	if (*problem != INVALID_STRING_ID) return GetString(STR_TBM_VEHICLE_WAIT_VEL, speed, *problem);
	if (const auto *project = GetTBMProjectForCutter(front.index); project != nullptr) {
		if (IsTBMStored(project->phase)) return GetString(STR_TBM_VEHICLE_WORK_VEL, speed, GetTBMPhaseString(project->phase));
		const auto state = GetTBMWorkState(project->id);
		if (state.blocked != TBMBlockedReason::None) return GetString(STR_TBM_VEHICLE_WAIT_VEL, speed, GetTBMHoldString(state.blocked));
		return GetString(STR_TBM_VEHICLE_WORK_VEL, speed, GetTBMPhaseString(project->phase));
	}
	const auto *assignment = GetTBMExchangeAssignmentForHauler(front.index);
	assert(assignment != nullptr);
	if (assignment->wait != TBMHaulerWait::None) return GetString(STR_TBM_VEHICLE_WAIT_VEL, speed, GetTBMHaulerWaitString(assignment->wait));
	// Empty return and coupling are driven by the actual assignment. A
	// loaded outbound leg keeps the ordinary station destination when present.
	if ((assignment->phase == TBMHaulerPhase::VisitOrigin || assignment->phase == TBMHaulerPhase::DisposeOrTransfer) &&
		front.current_order.IsType(OT_GOTO_STATION)) return std::nullopt;
	return GetString(STR_TBM_VEHICLE_WORK_VEL, speed, GetTBMHaulerPhaseString(assignment->phase));
}

static constexpr int TBM_MENU_RETURN_BAY = INT_MAX - 1; ///< Transient UI action, outside the actual map tile range.
static constexpr int TBM_MENU_RELEASE = INT_MAX - 2; ///< Transient UI action; never stored as a native destination.

/** @copydoc ShowTBMHangarMenu */
void ShowTBMHangarMenu(Window *window, WidgetID button, const Train &front, bool service)
{
	if (front.owner != _local_company || !IsTBMCutterEngine(front.engine_type)) return;
	AutoRestoreBackup actor(_current_company, front.owner);
	DropDownList items;
	if (const auto *project = GetTBMProjectForCutter(front.index); project != nullptr && !service) {
		items.push_back(MakeDropDownListStringItem(STR_TBM_RETURN_PARK, TBM_MENU_RETURN_BAY,
			ControlTBMProject(project->id, TBMProjectAction::ReturnAndPark, false) != TBMProjectError::None));
		items.push_back(MakeDropDownListStringItem(STR_PORTAL_RETIRE, TBM_MENU_RELEASE,
			ControlTBMProject(project->id, TBMProjectAction::Retire, false) != TBMProjectError::None));
		items.push_back(MakeDropDownListDividerItem());
	}
	std::set<TileIndex> candidates;
	for (const Depot *depot : Depot::Iterate()) {
		if (IsValidTile(depot->xy) && GetTileOwner(depot->xy) == front.owner) candidates.insert(depot->xy);
	}
	for (const auto &portal : CaptureLayerPortalWorkRecords()) {
		if (portal.company == front.owner) candidates.insert(portal.pair_key);
	}
	// The native dropdown owns scrolling. Its derived rows allocate no depot,
	// reserve no hangar and never change the currently accepted work/order.
	for (TileIndex tile : candidates) {
		const auto hangar = DescribeTBMHangar(front, tile);
		if (hangar.error != TBMHangarError::None && hangar.error != TBMHangarError::Occupied && hangar.error != TBMHangarError::UnfinishedPortal) continue;
		std::string name = hangar.kind == TBMHangarKind::PortalMagazine ?
			GetString(STR_TBM_HANGAR_NAME, GetMapLayerName(GetTileLayer(hangar.tile)), TileX(hangar.tile), TilePlaneY(hangar.tile)) :
			GetString(STR_DEPOT_NAME, VehicleType::Train, hangar.depot);
		if (hangar.error != TBMHangarError::None) name = GetString(STR_TBM_HANGAR_UNAVAILABLE_CHOICE, name,
			hangar.error == TBMHangarError::Occupied ? STR_TBM_HANGAR_BUSY_SHORT : STR_TBM_HANGAR_UNFINISHED_SHORT);
		items.push_back(MakeDropDownListStringItem(GetString(STR_TBM_HANGAR_CHOICE,
			service ? STR_TBM_HANGAR_SERVICE : STR_TBM_HANGAR_PARK, name), static_cast<int>(hangar.tile.base()), hangar.error != TBMHangarError::None));
	}
	if (items.empty()) items.push_back(MakeDropDownListStringItem(STR_ERROR_TBM_HANGAR_UNAVAILABLE, -1, true));
	ShowDropDownList(window, std::move(items), -1, button);
}

/** @copydoc SelectTBMHangarMenuItem */
void SelectTBMHangarMenuItem(const Train &front, int selection, bool service)
{
	if (selection == TBM_MENU_RETURN_BAY || selection == TBM_MENU_RELEASE) {
		if (const auto *project = GetTBMProjectForCutter(front.index); project != nullptr) {
			Command<Commands::ControlTBMProject>::Post(STR_ERROR_CAN_T_CONTROL_TBM_PROJECT, project->id,
				selection == TBM_MENU_RETURN_BAY ? TBMProjectAction::ReturnAndPark : TBMProjectAction::Retire);
		}
	} else if (selection >= 0 && static_cast<uint32_t>(selection) < Map::Size()) {
		Command<Commands::SendTBMToHangar>::Post(STR_ERROR_CAN_T_CONTROL_TBM_PROJECT, front.index, TileIndex{static_cast<uint32_t>(selection)}, service);
	}
}

/** Read-only identities and transient selection; geological progress, train orders and stock stay with their existing owners. */
struct TBMProjectsWindow : Window {
	MapLayer layer; ///< Explicit work plane selected when opening this browser.
	std::vector<TBMProjectID> rows; ///< Derived stable IDs only, never copied saved controllers or progress.
	TBMProjectID selected{UINT32_MAX}; ///< Actual selected row; retirement/company change revalidates it.
	Scrollbar *vscroll = nullptr; ///< Existing native scroll owner.
	/** @param elapsed Unused presentation periods; repainting does not advance simulation. */
	const IntervalTimer<TimerWindow> refresh = {std::chrono::seconds(1), [this]([[maybe_unused]] auto elapsed) { this->SetDirty(); }};

	/** @param desc Native widget definition. @param layer Actual work plane, independent of later camera changes. */
	TBMProjectsWindow(WindowDesc &desc, MapLayer layer) : Window(desc), layer(layer)
	{
		this->CreateNestedTree();
		this->vscroll = this->GetScrollbar(WID_TP_SCROLL);
		this->RebuildRows();
		this->FinishInitNested(layer);
	}

	/** @return A name plus two wrapped lines each for progress and holds, with native separation. */
	uint RowHeight() const { return 5 * GetCharacterHeight(FontSize::Normal) + WidgetDimensions::scaled.vsep_normal; }

	/** Rebuild presentation IDs from actual same-company, same-layer jobs; retired/foreign rows cannot retain actionable selection. */
	void RebuildRows()
	{
		this->rows.clear();
		// Only a controller with its real geometric owner belongs in this list.
		// Completed projects remain visible until actual retirement removes them.
		for (const auto &project : CaptureTBMProjects()) {
			const auto *job = GetExcavationJob(project.job);
			if (project.company != _local_company || job == nullptr || GetTileLayer(job->commissioning_anchor) != this->layer) continue;
			this->rows.push_back(project.id);
		}
		if (std::ranges::find(this->rows, this->selected) == this->rows.end()) this->selected = this->rows.empty() ? TBMProjectID{UINT32_MAX} : this->rows.front();
		this->vscroll->SetCount(static_cast<uint>(this->rows.size()));
	}

	/** @param widget Requested native caption. @param stringid Default widget text. @return Localized explicit-layer caption or standard action text. */
	std::string GetWidgetString(WidgetID widget, StringID stringid) const override
	{
		if (widget == WID_TP_PARK_DESTINATION) {
			const auto *project = GetTBMProject(this->selected);
			if (project == nullptr) return {};
			if (const Train *front = Train::GetIfValid(project->cutter); front != nullptr) {
				const auto hangar = IsTBMStored(project->phase) ? DescribeTBMHangar(*front, front->tile) : GetTBMOrderedHangar(*front);
				if (hangar.error == TBMHangarError::None || hangar.error == TBMHangarError::Occupied) {
					return GetString(STR_TBM_PARK_DESTINATION, GetMapLayerName(GetTileLayer(hangar.tile)), TileX(hangar.tile), TilePlaneY(hangar.tile));
				}
			}
			const auto bay = GetTBMExchangeBay(*project);
			if (bay.error != TBMExchangeError::None) return GetString(STR_PORTAL_HOLD_MISSING_EXCHANGE);
			return GetString(STR_TBM_PARK_DESTINATION, GetMapLayerName(GetTileLayer(bay.tile)), TileX(bay.tile), TilePlaneY(bay.tile));
		}
		if (widget == WID_TP_CAPTION) return GetString(STR_TBM_PROJECTS_CAPTION, GetMapLayerName(this->layer));
		if (widget == WID_TP_EXTEND) {
			const auto *project = GetTBMProject(this->selected);
			// The label makes the impending pause explicit; an already paused
			// controller simply enters additional-area planning.
			return GetString(project != nullptr && project->blocked == TBMBlockedReason::Paused ? STR_TBM_PROJECT_EXTEND : STR_TBM_PROJECT_PAUSE_EXTEND);
		}
		return this->Window::GetWidgetString(widget, stringid);
	}

	/** @param widget Widget being measured. @param size Minimum dimensions. @param padding Native frame padding. @param fill Native fill steps. @param resize Native resize steps. */
	void UpdateWidgetSize(WidgetID widget, Dimension &size, const Dimension &padding, Dimension &fill, Dimension &resize) override
	{
		if (widget != WID_TP_LIST) return;
		size.width = std::max<uint>(size.width, ScaleGUITrad(260));
		size.height = 2 * this->RowHeight() + padding.height;
		fill.height = 1;
		resize.height = this->RowHeight();
	}

	/** Fit complete readable rows in the current window; additional projects remain reachable through native scrolling. */
	void OnResize() override
	{
		const auto rect = this->GetWidget<NWidgetBase>(WID_TP_LIST)->GetCurrentRect().Shrink(WidgetDimensions::scaled.framerect);
		this->vscroll->SetCapacity(std::max<uint>(1, rect.Height() / this->RowHeight()));
	}

	/** Refresh actual IDs/capabilities before rendering, without allocating a command or altering the selected train. */
	void OnPaint() override
	{
		AutoRestoreBackup actor(_current_company, _local_company); // Read-only control estimates use the same actual owner as the displayed actions.
		this->RebuildRows();
		const auto *project = GetTBMProject(this->selected);
		this->SetWidgetsDisabledState(project == nullptr, WID_TP_SELECT, WID_TP_CENTRE, WID_TP_CONTROLS);
		// A lost or foreign native cutter cannot be opened through a stale row,
		// even while its recoverable geological project remains in the browser.
		const Train *train = project == nullptr ? nullptr : Train::GetIfValid(project->cutter);
		this->SetWidgetDisabledState(WID_TP_VEHICLE, train == nullptr || train->owner != _local_company);
		this->SetWidgetDisabledState(WID_TP_HANGAR, train == nullptr || train->owner != _local_company);
		this->SetWidgetDisabledState(WID_TP_PARK, project == nullptr || ControlTBMProject(project->id, TBMProjectAction::ReturnAndPark, false) != TBMProjectError::None); // Only currently admitted owner/machine/bay combinations may request parking.
		this->SetWidgetDisabledState(WID_TP_RESUME, project == nullptr || ControlTBMProject(project->id, TBMProjectAction::Resume, false) != TBMProjectError::None); // Resume belongs to explicitly paused or actually parked work.
		this->SetWidgetDisabledState(WID_TP_RELEASE, project == nullptr || ControlTBMProject(project->id, TBMProjectAction::Retire, false) != TBMProjectError::None);
		const auto *job = project == nullptr ? nullptr : GetExcavationJob(project->job);
		// Final delivery/return must not be paused by a button that cannot extend
		// already completed work; lost/foreign machinery needs actual recovery.
		this->SetWidgetDisabledState(WID_TP_EXTEND, project == nullptr || train == nullptr || train->owner != _local_company || job == nullptr ||
			IsTBMProjectFinished(project->phase) || IsTBMParking(project->phase) || job->state == ExcavationJobState::Completed || job->state == ExcavationJobState::Cancelled);
		this->DrawWidgets();
	}

	/** @param rect Actual scrollable list rectangle. @param widget Native widget being painted. Draw live phase/progress/hold from the sole simulation owners. */
	void DrawWidget(const Rect &rect, WidgetID widget) const override
	{
		if (widget == WID_TP_RELEASE_REASON) {
			const auto *project = GetTBMProject(this->selected);
			if (project == nullptr) return;
			const StringID reason = GetTBMProjectReleaseProblem(project->id);
			std::string message = GetString(reason == INVALID_STRING_ID ? STR_PORTAL_STEP_RETIRE : reason);
			const auto blocker = GetTBMProjectSettlementBlocker(project->id);
			if (reason == STR_ERROR_TBM_RELEASE_HAULAGE && blocker.has_value() && Vehicle::IsValidID(*blocker)) message = GetString(STR_VEHICLE_NAME, *blocker) + ": " + message;
			DrawStringMultiLine(rect.Shrink(WidgetDimensions::scaled.framerect), message, TextColour::Black);
			return;
		}
		if (widget != WID_TP_LIST) return;
		const Rect area = rect.Shrink(WidgetDimensions::scaled.framerect);
		if (this->rows.empty()) { DrawStringMultiLine(area, STR_UNDERGROUND_NEEDS_PROJECT, TextColour::Black); return; }
		const uint first = this->vscroll->GetPosition();
		const int line_height = GetCharacterHeight(FontSize::Normal);
		// Render only visible real rows; a long company fleet must neither grow
		// the window beyond the map nor trigger a full-map simulation query.
		for (uint row = first; row < this->rows.size() && row - first < this->vscroll->GetCapacity(); ++row) {
			const auto *project = GetTBMProject(this->rows[row]);
			if (project == nullptr) continue;
			const auto *job = GetExcavationJob(project->job);
			if (job == nullptr) continue;
			const int top = area.top + (row - first) * this->RowHeight();
			const bool selected = this->selected == project->id;
			if (selected) GfxFillRect(area.left, top, area.right, top + this->RowHeight() - 1, GetColourGradient(Colours::Grey, Shade::Dark));
			const ExtendedTextColour colour{selected ? TextColour::White : TextColour::Black, {ExtendedTextColourFlag::Forced}};
			const Train *train = Train::GetIfValid(project->cutter);
			const auto name = train == nullptr ? GetString(STR_PORTAL_HOLD_MISSING_CUTTER) : GetString(STR_VEHICLE_NAME, project->cutter);
			DrawString(area.left, area.right, top, GetString(STR_TBM_PROJECT_LIST_NAME, project->id.base(), name), colour);
			DrawStringMultiLine(area.left, area.right, top + line_height, top + 3 * line_height - 1,
				GetString(STR_TBM_PROJECT_LIST_PROGRESS, GetTBMPhaseString(project->phase), job->cursor, job->route.size()), colour);
			const auto state = GetTBMWorkState(project->id);
			const auto hold = state.blocked == TBMBlockedReason::None ? GetString(STR_PORTAL_PROJECT_NO_HOLD) : GetString(STR_PORTAL_PROJECT_HOLD, GetTBMHoldString(state.blocked));
			DrawStringMultiLine(area.left, area.right, top + 3 * line_height, top + 5 * line_height - 1, hold, colour);
		}
	}

	/** @param widget Actual menu anchor. @param index Chosen world tile/action. @param selection Dropdown row, unused. */
	void OnDropdownSelect(WidgetID widget, int index, [[maybe_unused]] int selection) override
	{
		if (widget != WID_TP_HANGAR) return;
		const auto *project = GetTBMProject(this->selected);
		const Train *front = project == nullptr ? nullptr : Train::GetIfValid(project->cutter);
		if (front != nullptr) SelectTBMHangarMenuItem(*front, index, false);
	}

	/** @param point Native window-relative click. @param widget Action or actual list. @param click_count Double-clicking a row opens its real cutter. */
	void OnClick(Point point, WidgetID widget, int click_count) override
	{
		if (this->IsWidgetDisabled(widget)) return;
		if (widget == WID_TP_LIST) {
			const Rect area = this->GetWidget<NWidgetBase>(widget)->GetCurrentRect().Shrink(WidgetDimensions::scaled.framerect);
			if (point.y < area.top) return;
			const uint row = this->vscroll->GetPosition() + (point.y - area.top) / this->RowHeight();
			if (row >= this->rows.size()) return;
			this->selected = this->rows[row];
			this->SetDirty();
			if (click_count < 2) return;
			widget = WID_TP_VEHICLE;
		}
		const auto *project = GetTBMProject(this->selected);
		if (project == nullptr || project->company != _local_company) return; // A stale/foreign row grants no navigation or selection authority.
		const auto *job = GetExcavationJob(project->job);
		if (job == nullptr) return;
		switch (widget) {
			case WID_TP_SELECT: InspectUndergroundProject(project->id); break;
			case WID_TP_CENTRE: {
				const auto work = GetTBMWorkState(project->id);
				ScrollMainWindowToTile(IsValidTile(work.face) ? work.face : job->commissioning_anchor, true);
				break;
			}
			case WID_TP_VEHICLE: {
				const Train *train = Train::GetIfValid(project->cutter);
				if (train != nullptr && train->owner == _local_company) ShowVehicleViewWindow(train); // Actual current ownership must still match after list selection.
				break;
			}
			case WID_TP_CONTROLS: ShowLayerPortalWindow(project->portal); break;
			case WID_TP_HANGAR:
				if (const Train *front = Train::GetIfValid(project->cutter); front != nullptr) ShowTBMHangarMenu(this, widget, *front, false);
				break;
			case WID_TP_PARK:
				Command<Commands::ControlTBMProject>::Post(STR_ERROR_CAN_T_CONTROL_TBM_PROJECT, project->id, TBMProjectAction::ReturnAndPark);
				break;
			case WID_TP_RESUME:
				Command<Commands::ControlTBMProject>::Post(STR_ERROR_CAN_T_CONTROL_TBM_PROJECT, project->id, TBMProjectAction::Resume);
				break;
			case WID_TP_RELEASE:
				Command<Commands::ControlTBMProject>::Post(STR_ERROR_CAN_T_CONTROL_TBM_PROJECT, project->id, TBMProjectAction::Retire);
				break;
			case WID_TP_EXTEND:
				// Pausing uses the same synchronized action as the portal controls;
				// the palette grants no private work/cursor mutation authority.
				if (project->blocked != TBMBlockedReason::Paused) {
					if (!Command<Commands::ControlTBMProject>::Post(STR_ERROR_TBM_EXTEND_FAILED, project->id, TBMProjectAction::Pause)) break;
				}
				PlanUndergroundProjectExtension(project->id);
				break;
			default: break;
		}
	}
};

/** Compact bounded list with explicit selection/navigation and native resize/scroll affordances. */
static constexpr std::initializer_list<NWidgetPart> _tbm_projects_widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, Colours::DarkGreen),
		NWidget(WWT_CAPTION, Colours::DarkGreen, WID_TP_CAPTION), SetStringTip(STR_TBM_PROJECTS_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_RESIZEBOX, Colours::DarkGreen),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, Colours::Grey, WID_TP_LIST), SetMinimalSize(260, 90), SetFill(1, 1), SetResize(1, 1), SetScrollbar(WID_TP_SCROLL), EndContainer(),
		NWidget(NWID_VSCROLLBAR, Colours::Grey, WID_TP_SCROLL),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_TP_SELECT), SetStringTip(STR_TBM_PROJECT_SELECT, STR_UNDERGROUND_PROJECT_TOOLTIP), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_TP_VEHICLE), SetStringTip(STR_TBM_PROJECT_VEHICLE, STR_UNDERGROUND_PROJECT_TOOLTIP), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_TP_CENTRE), SetStringTip(STR_TBM_PROJECT_CENTRE, STR_UNDERGROUND_PROJECT_TOOLTIP), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_TP_CONTROLS), SetStringTip(STR_TBM_PROJECT_CONTROLS, STR_UNDERGROUND_PROJECT_TOOLTIP), SetFill(1, 0),
	EndContainer(),
	NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_TP_EXTEND), SetStringTip(STR_TBM_PROJECT_PAUSE_EXTEND, STR_TBM_PROJECT_EXTEND_TOOLTIP), SetFill(1, 0),
	NWidget(WWT_PANEL, Colours::Grey), SetFill(1, 0),
		NWidget(WWT_TEXT, Colours::Invalid, WID_TP_PARK_DESTINATION), SetStringTip(STR_TBM_PARK_DESTINATION), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_TP_PARK), SetStringTip(STR_TBM_RETURN_PARK, STR_TBM_RETURN_PARK_TOOLTIP), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_TP_RESUME), SetStringTip(STR_PORTAL_RESUME, STR_TBM_RESUME_PARK_TOOLTIP), SetFill(1, 0),
	EndContainer(),
	NWidget(WWT_PANEL, Colours::Grey, WID_TP_RELEASE_REASON), SetMinimalSize(260, 38), SetFill(1, 0), EndContainer(),
	NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_TP_HANGAR), SetStringTip(STR_TBM_CHOOSE_HANGAR, STR_TBM_CHOOSE_HANGAR_TOOLTIP), SetFill(1, 0),
	NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_TP_RELEASE), SetStringTip(STR_PORTAL_RETIRE, STR_PORTAL_RETIRE_TOOLTIP), SetFill(1, 0),
};

/** One presentation browser per explicit work plane; its selection has no saved simulation authority. */
static WindowDesc _tbm_projects_desc(WindowPosition::Automatic, {}, 0, 0, WindowClass::TBMProjects, WindowClass::None, {}, _tbm_projects_widgets);

/** @copydoc ShowTBMProjectsWindow */
Window *ShowTBMProjectsWindow(MapLayer layer)
{
	// Invalid planes and spectators cannot acquire a company-looking project
	// action list, while an owned empty list remains an informative real window.
	if (!IsUndergroundConstructionLayer(layer) || !MultiLayerMap::IsValidLayer(layer) || !Company::IsValidID(_local_company)) return nullptr;
	if (Window *window = BringWindowToFrontById(WindowClass::TBMProjects, layer)) return window;
	return new TBMProjectsWindow(_tbm_projects_desc, layer);
}
