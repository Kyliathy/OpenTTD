/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_projects_gui.cpp Compact native browser of actual owned excavation projects.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group2
 * @see docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md Group 4 (wait and cargo lines, release prefix, New-area routing), Group 7 (named obstruction), Group 12 (Change work area)
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group 6 (face hauler wording)
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
		// r2df task 7.5: name the train or tile in the cutter's way when one can be found.
		if (const auto text = GetTBMObstructionText(*project); text.has_value()) return GetString(STR_TBM_VEHICLE_WAIT_VEL_TEXT, speed, *text);
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
	return GetString(STR_TBM_VEHICLE_WORK_VEL, speed, GetTBMHaulerPhaseString(*assignment));
}

/** @copydoc CanPlanTBMProjectExtension */
bool CanPlanTBMProjectExtension(const TBMProject &project)
{
	const Train *train = Train::GetIfValid(project.cutter);
	const auto *job = GetExcavationJob(project.job);
	// Final delivery/return must not be paused by a request that cannot extend already completed work; a lost or
	// foreign machine needs actual recovery first.
	return train != nullptr && train->owner == _local_company && job != nullptr && !IsTBMProjectFinished(project.phase) && !IsTBMParking(project.phase) &&
		job->state != ExcavationJobState::Completed && job->state != ExcavationJobState::Cancelled;
}

/** @copydoc CanChangeTBMWork */
bool CanChangeTBMWork(const TBMProject &project)
{
	const Train *train = Train::GetIfValid(project.cutter);
	const auto *job = GetExcavationJob(project.job);
	// Its own live machine, with a completed cut to keep and planned cuts still to replace; finished work and a project
	// being released have no remainder. Working, paused, returning and parked projects all qualify, without a pause.
	return train != nullptr && train->owner == _local_company && job != nullptr && !IsTBMProjectFinished(project.phase) && !IsTBMFinishing(project.phase) &&
		job->state != ExcavationJobState::Completed && job->state != ExcavationJobState::Cancelled && job->cursor > 0;
}

/** @copydoc PauseAndPlanTBMProjectExtension */
bool PauseAndPlanTBMProjectExtension(TBMProjectID id)
{
	const auto *project = GetTBMProject(id);
	if (project == nullptr || project->company != _local_company || !CanPlanTBMProjectExtension(*project)) return false;
	// Pausing uses the same synchronized action as the portal controls; the palette grants no private work/cursor
	// mutation authority. An already paused project goes straight to planning.
	if (project->blocked != TBMBlockedReason::Paused && !Command<Commands::ControlTBMProject>::Post(STR_ERROR_TBM_EXTEND_FAILED, id, TBMProjectAction::Pause)) return false;
	PlanUndergroundProjectExtension(id);
	return true;
}

/**
 * The haulers serving a project, in accepted request order.
 * @param id Project.
 * @return Their front engines.
 */
static std::vector<VehicleID> GetTBMProjectHaulers(TBMProjectID id)
{
	std::vector<VehicleID> haulers;
	// Existing accepted request order determines the displayed hauler order; the view allocates no assignment.
	for (const auto &assignment : CaptureTBMExchangeAssignments()) {
		if (assignment.project == id) haulers.push_back(assignment.hauler);
	}
	return haulers;
}

/** @copydoc GetTBMProjectCargoLines */
std::vector<std::string> GetTBMProjectCargoLines(const TBMProject &project)
{
	std::vector<std::string> lines;
	// R4: what the cutter and each hauler actually carry, separate from what the machine waits for.
	if (const Train *cutter = Train::GetIfValid(project.cutter); cutter != nullptr) {
		const TBMSpoilLoad spoil = SumTBMCutterSpoil(*cutter, project);
		lines.push_back(GetString(STR_TBM_PROJECT_LIST_CARRYING, spoil.earth_loaded_t, spoil.earth_capacity_t, spoil.rock_loaded_t, spoil.rock_capacity_t));
	}
	const auto haulers = GetTBMProjectHaulers(project.id);
	if (haulers.empty()) lines.push_back(GetString(STR_TBM_PROJECT_LIST_NO_HAULER));
	// One line per hauler, in accepted request order; a hauler that no longer exists has nothing to show.
	for (VehicleID id : haulers) {
		const Train *hauler = Train::GetIfValid(id);
		if (hauler == nullptr) continue;
		const TBMSpoilLoad spoil = SumTrainSpoil(*hauler);
		lines.push_back(GetString(STR_TBM_PROJECT_LIST_HAULER_CARRYING, GetString(STR_VEHICLE_NAME, id),
			spoil.earth_loaded_t, spoil.earth_capacity_t, spoil.rock_loaded_t, spoil.rock_capacity_t));
	}
	return lines;
}

/** @copydoc GetTBMProjectReleaseLine */
std::string GetTBMProjectReleaseLine(TBMProjectID id)
{
	// R6: this is the requirement of Finish/cancel and release only, so it is labelled as such and never reads as a
	// gate for the other actions.
	const StringID reason = GetTBMProjectReleaseProblem(id);
	if (reason == INVALID_STRING_ID) return GetString(STR_TBM_PROJECT_RELEASE_READY);
	std::string requirement = GetString(reason);
	const auto blocker = GetTBMProjectSettlementBlocker(id);
	// Haulage settlement names the vehicle still holding spoil or the bay, so the player knows what to wait for.
	if (reason == STR_ERROR_TBM_RELEASE_HAULAGE && blocker.has_value() && Vehicle::IsValidID(*blocker)) requirement = GetString(STR_VEHICLE_NAME, *blocker) + ": " + requirement;
	return GetString(STR_TBM_PROJECT_RELEASE_REQUIREMENT, requirement);
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
	uint hauler_lines = 1; ///< Hauler cargo lines every row reserves: the most haulers any listed project has, at least one.
	/** @param elapsed Unused presentation periods; repainting does not advance simulation. */
	const IntervalTimer<TimerWindow> refresh = {std::chrono::seconds(1), [this]([[maybe_unused]] auto elapsed) {
		// Rows share one height, so a project gaining or losing a hauler re-lays the whole list.
		const uint lines = this->CountHaulerLines();
		if (lines == this->hauler_lines) {
			this->SetDirty();
			return;
		}
		this->hauler_lines = lines;
		this->ReInit();
	}};

	/** @param desc Native widget definition. @param layer Actual work plane, independent of later camera changes. */
	TBMProjectsWindow(WindowDesc &desc, MapLayer layer) : Window(desc), layer(layer)
	{
		this->CreateNestedTree();
		this->vscroll = this->GetScrollbar(WID_TP_SCROLL);
		this->RebuildRows();
		this->hauler_lines = this->CountHaulerLines();
		this->FinishInitNested(layer);
	}

	/**
	 * A row: the name, two wrapped lines each for progress and the wait, the cutter's cargo line and one cargo line
	 * per hauler, with native separation.
	 * @return Height of every row in pixels.
	 */
	uint RowHeight() const { return (6 + this->hauler_lines) * GetCharacterHeight(FontSize::Normal) + WidgetDimensions::scaled.vsep_normal; }

	/** @return The most haulers serving any listed project, at least one so "No hauler assigned" has its line. */
	uint CountHaulerLines() const
	{
		size_t lines = 1;
		// Every row reserves the busiest project's hauler count, so cargo text cannot overlap adjacent projects.
		for (TBMProjectID id : this->rows) lines = std::max(lines, GetTBMProjectHaulers(id).size());
		return static_cast<uint>(lines);
	}

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
		this->SetWidgetDisabledState(WID_TP_EXTEND, project == nullptr || !CanPlanTBMProjectExtension(*project));
		this->SetWidgetDisabledState(WID_TP_CHANGE, project == nullptr || !CanChangeTBMWork(*project));
		this->DrawWidgets();
	}

	/** @param rect Actual scrollable list rectangle. @param widget Native widget being painted. Draw live phase/progress/hold from the sole simulation owners. */
	void DrawWidget(const Rect &rect, WidgetID widget) const override
	{
		if (widget == WID_TP_RELEASE_REASON) {
			const auto *project = GetTBMProject(this->selected);
			if (project == nullptr) return;
			// The error texts carry their own colour, which the forced black overrides.
			DrawStringMultiLine(rect.Shrink(WidgetDimensions::scaled.framerect), GetTBMProjectReleaseLine(project->id), ExtendedTextColour{TextColour::Black, {ExtendedTextColourFlag::Forced}});
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
			DrawStringMultiLine(area.left, area.right, top + 3 * line_height, top + 5 * line_height - 1, GetTBMWaitLine(*project, state.blocked), colour);
			// The cutter's line, then one line per hauler; the row reserves as many lines as the busiest project needs.
			int line_top = top + 5 * line_height;
			for (const std::string &line : GetTBMProjectCargoLines(*project)) {
				DrawString(area.left, area.right, line_top, line, colour);
				line_top += line_height;
			}
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
				PauseAndPlanTBMProjectExtension(project->id);
				break;
			case WID_TP_CHANGE:
				PlanUndergroundProjectWorkChange(project->id); // No pause: the machine keeps working while the player plans.
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
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_TP_EXTEND), SetStringTip(STR_TBM_PROJECT_PAUSE_EXTEND, STR_TBM_PROJECT_EXTEND_TOOLTIP), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_TP_CHANGE), SetStringTip(STR_TBM_PROJECT_CHANGE_WORK, STR_TBM_PROJECT_CHANGE_WORK_TOOLTIP), SetFill(1, 0),
	EndContainer(),
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
