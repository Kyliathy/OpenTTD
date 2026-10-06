/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_gui.cpp Underground entry tools consuming native geology, project and construction owners. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Groups 1-5 */

#include "stdafx.h"
#include "underground_gui.h"
#include "window_gui.h"
#include "window_func.h"
#include "strings_func.h"
#include "dropdown_func.h"
#include "dropdown_type.h"
#include "company_func.h"
#include "command_func.h"
#include "excavation.h"
#include "cargotype.h"
#include "map_layer_gui.h"
#include "layer_portal_cmd.h"
#include "tbm_cmd.h"
#include "rail_gui.h"
#include "road_gui.h"
#include "toolbar_gui.h"
#include "tilehighlight_func.h"
#include "gui.h"
#include "widgets/underground_widget.h"
#include "widgets/rail_widget.h"
#include "widgets/road_widget.h"
#include "table/strings.h"
#include "table/sprites.h"
#include "safeguards.h"

/** @copydoc IsUndergroundConstructionLayer */
bool IsUndergroundConstructionLayer(MapLayer layer)
{
	// Both rock planes share excavation tools; Surface and Space keep their own construction entries.
	return layer == MapLayer::Underground || layer == MapLayer::DeepUnderground;
}

/** Explicit menu context and read-only information; simulation state remains in the feature owners. */
struct UndergroundConstructionWindow : Window {
	MapLayer layer; ///< Layer requested by the caller, not a live camera reference.
	MultiLayerProfileSelection policy; ///< Display snapshot; never command permission.
	WidgetID selected_tool = WID_UC_SURVEY; ///< Placement operation armed by this window.
	TBMProjectID exchange_project{UINT32_MAX}; ///< Actual selected project; no UI-allocated identity.
	std::string status; ///< Last read-only survey or visible placement refusal.

	/** @param desc Widget description. @param layer Requested rock plane. @param policy Accepted display policy. */
	UndergroundConstructionWindow(WindowDesc &desc, MapLayer layer, const MultiLayerProfileSelection &policy) : Window(desc), layer(layer), policy(policy)
	{
		this->InitNested(layer);
		this->status = GetString(STR_UNDERGROUND_SELECT_TOOL);
	}

	/** @param widget Control being rendered. @param stringid Default string. @return Localized context or ordinary widget text. */
	std::string GetWidgetString(WidgetID widget, StringID stringid) const override
	{
		switch (widget) {
			case WID_UC_CAPTION: return GetString(STR_UNDERGROUND_CAPTION, GetMapLayerName(this->layer));
			case WID_UC_STATUS: return this->status;
			case WID_UC_REQUIREMENTS: {
				const auto *profile = GetMultiLayerContentProfile(this->policy.id);
				if (profile == nullptr) return GetString(STR_UNDERGROUND_INVALID_CONTEXT);
				// The canonical calendar gate explains every unavailable Deep action, independent of the selected view.
				if (!CheckDeepLayerAvailability(this->policy, TimerGameCalendar::date) && this->layer == MapLayer::DeepUnderground) {
					return GetString(STR_UNDERGROUND_LOCKED, profile->deep_unlock_year.base());
				}
				return GetString(STR_UNDERGROUND_REQUIREMENTS);
			}
			default: return this->Window::GetWidgetString(widget, stringid);
		}
	}

	/** Refresh calendar/company availability without modifying geological or vehicle state. */
	void OnPaint() override
	{
		const bool locked = this->layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(this->policy, TimerGameCalendar::date);
		const bool spectator = !Company::IsValidID(_local_company);
		// Deep dates and actual player ownership govern offered tools; all commands still perform their own checks.
		this->SetWidgetsDisabledState(locked || spectator, WID_UC_PROJECT, WID_UC_MINE, WID_UC_EXCHANGE,
			WID_UC_RAIL, WID_UC_ROAD, WID_UC_RAIL_STATION, WID_UC_ROAD_TERMINAL, WID_UC_RAIL_SERVICE, WID_UC_ROAD_SERVICE);
		this->SetWidgetDisabledState(WID_UC_SURVEY, locked);
		this->DrawWidgets();
	}

	/** @param pt Click position. @param widget Chosen tool. @param click_count Number of clicks, unused. */
	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		if (this->IsWidgetDisabled(widget)) return;
		switch (widget) {
			case WID_UC_SURVEY:
			case WID_UC_PROJECT:
				this->selected_tool = widget;
				HandlePlacePushButton(this, widget, SPR_CURSOR_QUERY, HT_RECT);
				break;
			case WID_UC_MINE: ShowBuildIndustryWindow(); break;
			case WID_UC_EXCHANGE: {
				DropDownList list;
				// Offer only real unfinished projects owned by this player on the menu's rock plane.
				for (const auto &project : CaptureTBMProjects()) {
					const auto *job = GetExcavationJob(project.job);
					// A missing geometric owner or a foreign, other-layer or finished project cannot select a bay here.
					if (job == nullptr || !IsValidTile(job->commissioning_anchor) || project.company != _local_company || GetTileLayer(job->commissioning_anchor) != this->layer || IsTBMProjectFinished(project.phase)) continue;
					list.push_back(MakeDropDownListStringItem(GetString(STR_UNDERGROUND_PROJECT_ENTRY, project.id.base()), project.id.base()));
				}
				if (list.empty()) {
					this->status = GetString(STR_UNDERGROUND_NEEDS_PROJECT);
					this->SetDirty();
					return;
				}
				ShowDropDownList(this, std::move(list), -1, widget);
				break;
			}
			case WID_UC_RAIL:
			case WID_UC_RAIL_STATION:
			case WID_UC_RAIL_SERVICE: {
				Window *toolbar = ShowBuildRailToolbar(_last_built_railtype);
				if (toolbar == nullptr) return;
				if (widget == WID_UC_RAIL_STATION) toolbar->OnClick({}, WID_RAT_BUILD_STATION, 1);
				if (widget == WID_UC_RAIL_SERVICE) toolbar->OnClick({}, WID_RAT_BUILD_DEPOT, 1);
				break;
			}
			case WID_UC_ROAD:
			case WID_UC_ROAD_TERMINAL:
			case WID_UC_ROAD_SERVICE: {
				Window *toolbar = ShowBuildRoadToolbar(_last_built_roadtype);
				if (toolbar == nullptr) return;
				if (widget == WID_UC_ROAD_TERMINAL) toolbar->OnClick({}, WID_ROT_TRUCK_STATION, 1);
				if (widget == WID_UC_ROAD_SERVICE) toolbar->OnClick({}, WID_ROT_DEPOT, 1);
				break;
			}
			default: break;
		}
	}

	/** @param widget Menu owner. @param index Actual selected project ID. @param click_result Dropdown detail, unused. */
	void OnDropdownSelect(WidgetID widget, int index, [[maybe_unused]] int click_result) override
	{
		if (widget != WID_UC_EXCHANGE) return;
		this->exchange_project = TBMProjectID{static_cast<uint32_t>(index)};
		this->selected_tool = widget;
		HandlePlacePushButton(this, widget, SPR_CURSOR_QUERY, HT_RECT);
	}

	/** @param pt World position, unused. @param tile Actual clicked world tile supplied by main or extra view. */
	void OnPlaceObject([[maybe_unused]] Point pt, TileIndex tile) override
	{
		// A different-layer click cannot redirect an armed survey/project command; explain and preserve all owners.
		if (!IsValidTile(tile) || GetTileLayer(tile) != this->layer) {
			this->status = GetString(STR_UNDERGROUND_WRONG_LAYER);
			this->SetDirty();
			return;
		}
		switch (this->selected_tool) {
			case WID_UC_SURVEY: {
				const auto &geology = GetGeologyWorldState();
				const auto terrain = geology.terrain.Get(tile);
				StringID state = STR_UNDERGROUND_SURVEY_LEGACY;
				if (terrain) {
					switch (*terrain) {
						case ExcavationState::Solid: state = STR_UNDERGROUND_SURVEY_SOLID; break;
						case ExcavationState::Excavating: state = STR_UNDERGROUND_SURVEY_WORKING; break;
						case ExcavationState::Excavated: state = STR_UNDERGROUND_SURVEY_OPEN; break;
						case ExcavationState::Reserved: state = STR_UNDERGROUND_INVALID_CONTEXT; break;
					}
				}
				const auto *deposit = geology.deposits.Get(geology.deposits.Find(tile));
				const CargoType cargo = deposit == nullptr ? INVALID_CARGO : GetCargoTypeByLabel(deposit->resource);
				const StringID resource = IsValidCargoType(cargo) ? CargoSpec::Get(cargo)->name : STR_UNDERGROUND_SURVEY_NO_DEPOSIT;
				this->status = GetString(STR_UNDERGROUND_SURVEY_RESULT, state, resource, deposit == nullptr ? 0 : deposit->remaining_reserve);
				break;
			}
			case WID_UC_PROJECT: {
				const auto key = GetLayerPortalPairKey(tile);
				if (!key) {
					this->status = GetString(STR_UNDERGROUND_NEEDS_PORTAL);
					break;
				}
				ShowLayerPortalWindow(tile);
				break;
			}
			case WID_UC_EXCHANGE:
				Command<Commands::SetTBMExchangeBay>::Post(STR_ERROR_CAN_T_BUILD_TRAIN_DEPOT, this->exchange_project, tile);
				break;
			default: break;
		}
		this->SetDirty();
	}

	/** Release cursor ownership when the player cancels placement. */
	void OnPlaceObjectAbort() override { this->RaiseButtons(); }
};

/** Native controls; stock icons remain with the ordinary rail/road pickers until the art owner delivers assets. */
static constexpr std::initializer_list<NWidgetPart> _underground_widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, Colours::DarkGreen),
		NWidget(WWT_CAPTION, Colours::DarkGreen, WID_UC_CAPTION), SetStringTip(STR_UNDERGROUND_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, Colours::DarkGreen),
		NWidget(NWID_VERTICAL), SetPadding(WidgetDimensions::unscaled.framerect),
			NWidget(WWT_TEXT, Colours::Invalid, WID_UC_REQUIREMENTS), SetStringTip(STR_UNDERGROUND_REQUIREMENTS, STR_UNDERGROUND_REQUIREMENTS), SetMinimalSize(520, 20), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_SURVEY), SetStringTip(STR_UNDERGROUND_SURVEY, STR_UNDERGROUND_SURVEY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_PROJECT), SetStringTip(STR_UNDERGROUND_PROJECT, STR_UNDERGROUND_PROJECT_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_MINE), SetStringTip(STR_UNDERGROUND_MINE, STR_UNDERGROUND_MINE_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_EXCHANGE), SetStringTip(STR_UNDERGROUND_EXCHANGE, STR_UNDERGROUND_EXCHANGE_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_RAIL), SetStringTip(STR_UNDERGROUND_RAIL, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_ROAD), SetStringTip(STR_UNDERGROUND_ROAD, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_RAIL_STATION), SetStringTip(STR_UNDERGROUND_RAIL_STATION, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_ROAD_TERMINAL), SetStringTip(STR_UNDERGROUND_ROAD_TERMINAL, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_RAIL_SERVICE), SetStringTip(STR_UNDERGROUND_RAIL_SERVICE, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_ROAD_SERVICE), SetStringTip(STR_UNDERGROUND_ROAD_SERVICE, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_TEXT, Colours::Invalid, WID_UC_STATUS), SetStringTip(STR_UNDERGROUND_SELECT_TOOL, STR_UNDERGROUND_REQUIREMENTS), SetMinimalSize(520, 20), SetFill(1, 0),
		EndContainer(),
	EndContainer(),
};

/** One explicit-layer interface per rock plane; no saved simulation fields belong to the window. */
static WindowDesc _underground_desc(WindowPosition::Automatic, {}, 0, 0, WindowClass::UndergroundConstruction, WindowClass::None, {}, _underground_widgets);

/** @copydoc ShowUndergroundConstructionToolbar */
Window *ShowUndergroundConstructionToolbar(MapLayer layer, const MultiLayerProfileSelection &policy)
{
	const auto *profile = GetMultiLayerContentProfile(policy.id);
	// A bad profile or nonexistent/non-rock plane cannot open a tool that appears to authorize construction there.
	if (!IsUndergroundConstructionLayer(layer) || !MultiLayerMap::IsValidLayer(layer) || profile == nullptr || policy.version != profile->version) return nullptr;
	if (Window *window = BringWindowToFrontById(WindowClass::UndergroundConstruction, layer)) return window;
	return new UndergroundConstructionWindow(_underground_desc, layer, policy);
}
