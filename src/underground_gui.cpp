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
#include "company_base.h"
#include "command_func.h"
#include "excavation.h"
#include "cargotype.h"
#include "map_layer_gui.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "tbm_cmd.h"
#include "tbm_controls.h"
#include "timer/timer.h"
#include "timer/timer_window.h"
#include "tbm_area_preview.h"
#include "construction_preview.h"
#include "multilayer_sprite.h"
#include "gfx_func.h"
#include "zoom_func.h"
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

/** @copydoc BuildUndergroundVehicleFilter */
std::vector<const LayerVehicleCapability *> BuildUndergroundVehicleFilter(MapLayer layer, const MultiLayerProfileSelection &policy,
	TimerGameCalendar::Date date, UndergroundFleetKind kind)
{
	std::vector<const LayerVehicleCapability *> result;
	const auto *profile = GetMultiLayerContentProfile(policy.id);
	// Only an accepted native profile on an allocated rock plane can offer these logical construction-domain designs.
	if (!IsUndergroundConstructionLayer(layer) || !MultiLayerMap::IsValidLayer(layer) || profile == nullptr ||
		policy.id != MultiLayerGameplayProfileID::MultilayerIndustry || policy.version != profile->version || policy.legacy) return result;
	// Unknown filter domains must not accidentally fall back to a conventional or tube purchase list.
	if (kind != UndergroundFleetKind::ConventionalFreight && kind != UndergroundFleetKind::TubeCapsules) return result;
	// Model introductions never grant an earlier Deep route; the profile's shared calendar gate applies first.
	if (layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(policy, date)) return result;
	// The immutable catalogue owns every model identity, cargo role, layer and date; this UI only selects display rows.
	for (const auto &model : GetMultiLayerVehicleCapabilities()) {
		// Unsupported layers and future models cannot be offered in the proposed explicit operating context.
		if (!model.supported_layers.Test(layer) || date < TimerGameCalendar::ConvertYMDToDate(model.introduction_year, 0, 1)) continue;
		if (kind == UndergroundFleetKind::TubeCapsules) {
			if (model.transport == MultiLayerVehicleTransport::Tube) result.push_back(&model);
		} else {
			// Conventional freight uses depot-built rail/road designs; a portal-only cutter stays in its project interface.
			if ((model.transport == MultiLayerVehicleTransport::Rail || model.transport == MultiLayerVehicleTransport::Road) &&
				(model.commissioning == MultiLayerCommissioningFacility::RailDepot || model.commissioning == MultiLayerCommissioningFacility::RoadDepot)) result.push_back(&model);
		}
	}
	return result;
}

/** Explicit menu context and read-only information; simulation state remains in the feature owners. */
struct UndergroundConstructionWindow : Window {
	MapLayer layer; ///< Layer requested by the caller, not a live camera reference.
	MultiLayerProfileSelection policy; ///< Display snapshot; never command permission.
	WidgetID selected_tool = WID_UC_SURVEY; ///< Placement operation armed by this window.
	TBMProjectID exchange_project{UINT32_MAX}; ///< Actual selected project; no UI-allocated identity.
	std::string status; ///< Last read-only survey or visible placement refusal.
	TileIndex survey_tile = INVALID_TILE; ///< Last explicit survey point; rendering re-queries its live seam without saving information.
	TileIndex area_portal = INVALID_TILE; ///< Selected real source housing, independent of camera changes.
	TileIndex area_start = INVALID_TILE; ///< First world corner of the transient floor quote.
	TileIndex area_end = INVALID_TILE; ///< Opposite world corner on the same rock plane.
	uint16_t area_model = UINT16_MAX; ///< Profile-owned cutter selected by the common portal controls.
	TBMConsistRequest area_consist; ///< Shared native defaults for the actual working rock layer; confirmation reuses the displayed quote's counts.
	StationID area_origin = StationID::Invalid(); ///< Actual Surface disposal origin selected by common controls.
	std::optional<TBMCommissionAreaPreview> area_quote; ///< Read-only canonical quantities and derived drawing geometry.
	TBMProjectID inspected_project{UINT32_MAX}; ///< Actual clicked project identity for live progress/footprint queries, not saved authority.
	uint64_t inspected_revision = UINT64_MAX; ///< Derived overlay refresh stamp; never work permission or a second progress cursor.
	/** @param elapsed Unused presentation periods. Refresh the UI without stepping its actual job/controller owners. */
	const IntervalTimer<TimerWindow> project_refresh = {std::chrono::seconds(1), [this]([[maybe_unused]] auto elapsed) { this->SetDirty(); }};

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
			case WID_UC_JOB_PROGRESS: {
				const auto *project = GetTBMProject(this->inspected_project);
				// Missing/foreign projects cannot expose stale owned progress after
				// retirement, company changes or a replaced placement context.
				if (project == nullptr || project->company != _local_company) return GetString(STR_UNDERGROUND_JOB_NONE);
				const auto *job = GetExcavationJob(project->job);
				if (job == nullptr) return GetString(STR_UNDERGROUND_JOB_NONE);
				const auto work = GetTBMWorkState(project->id);
				return GetString(STR_UNDERGROUND_JOB_PROGRESS, job->id.base(), work.pass_cursor, job->route.size(), work.partial_work_ticks);
			}
			case WID_UC_MODEL_DATES: {
				const auto *profile = GetMultiLayerContentProfile(this->policy.id);
				// Native cutter dates cannot imply native commissioning permission in Classic or an invalid profile.
				if (this->policy.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile == nullptr || this->policy.version != profile->version || this->policy.legacy) return {};
				std::string dates;
				// Show every declared compatible cutter's date, including locked designs, without allocating engine identities.
				for (const auto &model : GetMultiLayerVehicleCapabilities()) {
					// Portal machinery on this rock plane belongs in this calendar; ordinary freight and other movement domains do not.
					if (model.commissioning != MultiLayerCommissioningFacility::RailPortalBay || !model.supported_layers.Test(this->layer)) continue;
					if (!dates.empty()) dates += "; ";
					dates += GetString(STR_UNDERGROUND_MODEL_DATE, std::string(model.name), model.introduction_year.base());
				}
				return GetString(STR_UNDERGROUND_MODEL_CALENDAR, dates);
			}
			case WID_UC_AREA_PRICE:
				if (!this->area_quote) return GetString(STR_UNDERGROUND_AREA_EMPTY);
				return GetString(STR_UNDERGROUND_AREA_PRICE, this->area_quote->purchase.GetCost(), this->area_quote->area.totals.cost);
			case WID_UC_AREA_MATERIAL:
				if (!this->area_quote) return GetString(STR_UNDERGROUND_AREA_EMPTY);
				return GetString(STR_UNDERGROUND_AREA_MATERIAL, this->area_quote->area.totals.earth_t,
					this->area_quote->area.totals.rock_t, this->area_quote->area.totals.support_units, this->area_quote->area.totals.work_ticks);
			case WID_UC_AREA_FLOOR:
				if (!this->area_quote) return GetString(STR_UNDERGROUND_AREA_EMPTY);
				return GetString(STR_UNDERGROUND_AREA_FLOOR, this->area_quote->area.plan.covered_targets.size(),
					this->area_quote->area.turn_envelope.size(), this->area_quote->area.totals.new_cells, this->area_quote->area.plan.remaining_targets.size());
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

	/**
	 * Select the native icon for a real button or informational row.
	 * @param widget Actual window widget; gameplay availability remains with its existing owner.
	 * @return Authored control/resource icon, or no icon for an unrelated/Classic context.
	 */
	std::optional<SpriteID> GetWidgetIcon(WidgetID widget) const
	{
		MultiLayerSpriteAsset asset;
		switch (widget) {
			case WID_UC_SURVEY: asset = MultiLayerSpriteAsset::ControlSurvey; break;
			case WID_UC_PROJECT: asset = MultiLayerSpriteAsset::ControlExcavation; break;
			case WID_UC_AREA: asset = MultiLayerSpriteAsset::ControlTbmArea; break;
			case WID_UC_EXCHANGE: asset = MultiLayerSpriteAsset::ControlExchange; break;
			case WID_UC_REQUIREMENTS:
				// The date owner's Deep refusal selects the lock cue; a camera or
				// enabled-looking icon never grants permission to enter the layer.
				return GetMultiLayerControlSprite(MultiLayerSpriteAsset::ControlDeepLock,
					this->layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(this->policy, TimerGameCalendar::date), false);
			case WID_UC_MODEL_DATES: {
				bool future = false;
				// Inspect only the compatible portal catalogue rows whose dates this
				// row already displays; the immutable capability owner defines them.
				for (const auto &model : GetMultiLayerVehicleCapabilities()) {
					// Freight and incompatible cutters cannot contribute a misleading
					// future-technology warning to this plane's commissioning row.
					if (model.commissioning != MultiLayerCommissioningFacility::RailPortalBay || !model.supported_layers.Test(this->layer)) continue;
					future |= !IsTBMCutterAvailable(model.model_id, this->layer, this->policy, TimerGameCalendar::date);
				}
				return GetMultiLayerControlSprite(MultiLayerSpriteAsset::ControlFutureLock, future, false);
			}
			case WID_UC_AREA_FLOOR:
				// Turn art describes the canonical swept-envelope quote, including
				// refusals. It adds no pivot command or substitute floor geometry.
				return GetMultiLayerControlSprite(MultiLayerSpriteAsset::ControlTbmTurn45,
					!this->area_quote || this->area_quote->purchase.Failed(), this->area_quote.has_value());
			case WID_UC_STATUS: return GetMultiLayerSurveySprite(this->survey_tile, true);
			default: return std::nullopt;
		}
		return GetMultiLayerControlSprite(asset, this->IsWidgetDisabled(widget), this->IsWidgetLowered(widget));
	}

	/**
	 * Reserve icon room without overlapping localized status text.
	 * @param widget Row being sized; ordinary buttons retain standard image/text sizing.
	 * @param size Minimum size to extend.
	 * @param padding Existing container padding.
	 * @param fill Unchanged fill step.
	 * @param resize Unchanged resize step.
	 */
	void UpdateWidgetSize(WidgetID widget, Dimension &size, const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		// The template initially carries the ordinary query cursor. Reserve the
		// complete native control cell before OnPaint selects its current state,
		// so a disabled/pressed picture cannot overrun the actual button frame.
		if (widget == WID_UC_SURVEY || widget == WID_UC_PROJECT || widget == WID_UC_AREA || widget == WID_UC_EXCHANGE) {
			if (this->GetWidgetIcon(widget)) {
				const Dimension caption = GetStringBoundingBox(this->GetWidgetString(widget, this->GetWidget<NWidgetCore>(widget)->GetString()));
				const uint picture = ScaleGUITrad(20);
				size.width = std::max(size.width, caption.width + 2 * (picture + WidgetDimensions::scaled.hsep_wide) + padding.width);
				size.height = std::max(size.height, std::max(picture, caption.height) + padding.height);
			}
			return;
		}
		// Only custom informational rows need this space; buttons already measure
		// their native picture and localized caption through the ordinary widget.
		if (widget != WID_UC_REQUIREMENTS && widget != WID_UC_MODEL_DATES && widget != WID_UC_AREA_FLOOR && widget != WID_UC_STATUS) return;
		const Dimension text = GetStringBoundingBox(this->GetWidgetString(widget, this->GetWidget<NWidgetCore>(widget)->GetString()));
		size.width = std::max(size.width, text.width + ScaleGUITrad(24) + padding.width);
		size.height = std::max(size.height, std::max<uint>(ScaleGUITrad(20), text.height) + padding.height);
	}

	/**
	 * Draw native status cues beside the existing localized query text.
	 * @param r Actual informational row rectangle.
	 * @param widget Row identity; buttons use the standard image/text renderer.
	 */
	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		// Other widget families keep their normal renderer and existing input
		// behavior. These four rows explicitly reserve their icon/text regions.
		if (widget != WID_UC_REQUIREMENTS && widget != WID_UC_MODEL_DATES && widget != WID_UC_AREA_FLOOR && widget != WID_UC_STATUS) return;
		const bool rtl = _current_text_dir == TD_RTL;
		Rect text_rect = r;
		if (const auto icon = this->GetWidgetIcon(widget)) {
			DrawSpriteIgnorePadding(*icon, PAL_NONE, r.WithWidth(ScaleGUITrad(20), rtl), {AlignmentH::Centre, AlignmentV::Middle});
			text_rect = r.Indent(ScaleGUITrad(24), rtl);
		}
		const std::string text = this->GetWidgetString(widget, this->GetWidget<NWidgetCore>(widget)->GetString());
		DrawString(text_rect.left, text_rect.right, r.top + (r.Height() - GetCharacterHeight(FontSize::Normal)) / 2, text, TextColour::Black);
	}

	/** Refresh calendar/company availability without modifying geological or vehicle state. */
	void OnPaint() override
	{
		this->RefreshInspectedFootprint();
		const bool locked = this->layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(this->policy, TimerGameCalendar::date);
		const bool spectator = !Company::IsValidID(_local_company);
		// Deep dates and actual player ownership govern offered tools; all commands still perform their own checks.
		this->SetWidgetsDisabledState(locked || spectator, WID_UC_PROJECT, WID_UC_MINE, WID_UC_EXCHANGE,
			WID_UC_RAIL, WID_UC_ROAD, WID_UC_RAIL_STATION, WID_UC_ROAD_TERMINAL, WID_UC_RAIL_SERVICE, WID_UC_ROAD_SERVICE);
		this->SetWidgetDisabledState(WID_UC_AREA, locked || spectator);
		// Only a complete successful quote offers purchase; commands revalidate date, company and every reference.
		this->SetWidgetDisabledState(WID_UC_AREA_CONFIRM, locked || spectator || !this->area_quote || this->area_quote->purchase.Failed());
		this->SetWidgetDisabledState(WID_UC_SURVEY, locked);
		// Scenario road tools retain their existing editor owner; company-only purchase/service controls stay unavailable.
		this->SetWidgetDisabledState(WID_UC_ROAD, locked || (spectator && _game_mode != GameMode::Editor));
		// Update only actual control pictures; the standard widget retains the
		// localized caption, tooltip and hit testing for each existing command.
		for (const WidgetID widget : {WID_UC_SURVEY, WID_UC_PROJECT, WID_UC_AREA, WID_UC_EXCHANGE}) {
			this->GetWidget<NWidgetCore>(widget)->SetSprite(this->GetWidgetIcon(widget).value_or(SPR_CURSOR_QUERY));
		}
		this->DrawWidgets();
	}

	/** @param pt Click position. @param widget Chosen tool. @param click_count Number of clicks, unused. */
	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		if (this->IsWidgetDisabled(widget)) return;
		if (_thd.GetCallbackWnd() == this) ClearConstructionPreview();
		this->survey_tile = INVALID_TILE;
		this->inspected_project = TBMProjectID{UINT32_MAX};
		this->inspected_revision = UINT64_MAX;
		switch (widget) {
			case WID_UC_AREA:
				this->selected_tool = widget;
				this->area_portal = INVALID_TILE;
				this->area_quote.reset();
				HandlePlacePushButton(this, widget, SPR_CURSOR_QUERY, HT_RECT);
				this->status = GetString(STR_UNDERGROUND_NEEDS_PORTAL);
				this->SetDirty();
				break;
			case WID_UC_AREA_CONFIRM:
				// A UI quote is only a display snapshot; the owning command supplies current permission and purchase.
				if (Command<Commands::CommissionTBM>::Post(STR_ERROR_CAN_T_COMMISSION_TBM, this->area_portal,
					this->area_model, this->area_consist.earth_wagons, this->area_consist.rock_wagons, this->area_origin, this->area_start, this->area_end, false)) ResetObjectToPlace();
				break;
			case WID_UC_SURVEY:
			case WID_UC_PROJECT:
				this->selected_tool = widget;
				HandlePlacePushButton(this, widget, SPR_CURSOR_QUERY, HT_RECT);
				MarkWholeScreenDirty(); // Cursor ownership also governs the read-only seam overlay in every viewport.
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
				Window *toolbar = _game_mode == GameMode::Editor ? ShowBuildRoadScenToolbar(_last_built_roadtype) : ShowBuildRoadToolbar(_last_built_roadtype);
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
			this->survey_tile = INVALID_TILE;
			this->status = GetString(STR_UNDERGROUND_WRONG_LAYER);
			this->SetDirty();
			return;
		}
		switch (this->selected_tool) {
			case WID_UC_AREA:
				if (this->area_portal == INVALID_TILE) {
					const auto key = GetLayerPortalPairKey(tile);
					if (!key) {
						this->status = GetString(STR_UNDERGROUND_NEEDS_PORTAL);
						break;
					}
					this->area_portal = *key;
					this->area_model = GetLayerPortalDefaultTBMModel(*key);
					this->area_consist = GetDefaultTBMConsist(GetTileLayer(GetLayerPortalPairTile(*key)));
					this->area_origin = GetLayerPortalDefaultTBMOrigin(*key, _local_company);
					this->status = GetString(STR_UNDERGROUND_AREA_SELECT_FLOOR);
				} else {
					VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_DEMOLISH_AREA);
				}
				break;
			case WID_UC_SURVEY: {
				this->survey_tile = tile;
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
				const auto controls = GetTBMPairControls(*key, _local_company);
				if (controls.project) {
					this->inspected_project = controls.project->id;
					this->inspected_revision = UINT64_MAX;
					this->RefreshInspectedFootprint();
				}
				break;
			}
			case WID_UC_EXCHANGE:
				Command<Commands::SetTBMExchangeBay>::Post(STR_ERROR_CAN_T_BUILD_TRAIN_DEPOT, this->exchange_project, tile);
				break;
			default: break;
		}
		this->SetDirty();
	}

	/** @param select_method Existing rectangular selection. @param select_proc This window's area process. @param pt Actual world position under either viewport. */
	void OnPlaceDrag(ViewportPlaceMethod select_method, ViewportDragDropSelectionProcess select_proc, Point pt) override
	{
		// Only this window's armed excavation-area operation updates its selection; no ordinary bulldozer runs here.
		if (this->selected_tool != WID_UC_AREA || select_proc != DDSP_DEMOLISH_AREA) return;
		const TileIndex tile = TileVirtXY(pt.x, pt.y);
		// Crossing planes cancels the whole drag before its rectangle could span stacked map bands.
		if (pt.x < 0 || pt.y < 0 || !IsValidTile(tile) || GetTileLayer(tile) != this->layer) {
			ResetObjectToPlace();
			this->status = GetString(STR_UNDERGROUND_WRONG_LAYER);
			this->SetDirty();
			return;
		}
		VpSelectTilesWithMethod(pt.x, pt.y, select_method);
	}

	/** @param select_method Selection geometry, unused. @param select_proc This window's area process. @param pt Release position, unused. @param start_tile Actual starting world corner. @param end_tile Actual final world corner. */
	void OnPlaceMouseUp([[maybe_unused]] ViewportPlaceMethod select_method, ViewportDragDropSelectionProcess select_proc, [[maybe_unused]] Point pt, TileIndex start_tile, TileIndex end_tile) override
	{
		// Only an active area tool with a real portal can obtain a quote; stale release events cannot purchase work.
		if (this->selected_tool != WID_UC_AREA || select_proc != DDSP_DEMOLISH_AREA || this->area_portal == INVALID_TILE || _thd.GetCallbackWnd() != this) return;
		this->area_start = start_tile;
		this->area_end = end_tile;
		this->area_quote = PreviewTBMCommissionArea(_local_company, this->area_portal, this->area_model, this->area_consist.earth_wagons, this->area_consist.rock_wagons,
			this->area_origin, start_tile, end_tile, false);
		const auto &quote = *this->area_quote;
		SetConstructionPreview(this, quote.area.plan.covered_targets, quote.area.turn_envelope, quote.area.plan.remaining_targets);
		this->status = quote.purchase.Failed() ? GetString(quote.purchase.GetErrorMessage()) : GetString(STR_UNDERGROUND_AREA_READY);
		this->SetDirty();
	}

	/** Refresh the actual selected job's derived floor/working-space/uncovered overlay; no cell is reserved or cleared. */
	void RefreshInspectedFootprint()
	{
		// Only the active project-inspection cursor can own these overlays. A
		// background window cannot replace another tool's canonical area quote.
		if (this->selected_tool != WID_UC_PROJECT || _thd.GetCallbackWnd() != this) return;
		const auto *project = GetTBMProject(this->inspected_project);
		// A selected project becoming foreign or unavailable cannot leave an
		// owned construction-looking overlay after takeover or retirement.
		if (project == nullptr || project->company != _local_company) {
			if (this->inspected_revision != UINT64_MAX) ClearConstructionPreview();
			this->inspected_revision = UINT64_MAX;
			return;
		}
		const auto *job = GetExcavationJob(project->job);
		if (job == nullptr) return;
		if (job->revision == this->inspected_revision) return;
		std::vector<TileIndex> floor, working, uncovered;
		// Partition only the geology owner's canonical accepted authorization.
		// Uncovered requested targets remain red and charged approach/turn work
		// remains blue. White floor cues never grant construction or free cutting.
		auto requested = job->requested_tiles;
		auto authorized = job->authorized_tiles;
		std::ranges::sort(requested);
		std::ranges::sort(authorized);
		std::ranges::set_intersection(requested, authorized, std::back_inserter(floor));
		std::ranges::set_difference(authorized, requested, std::back_inserter(working));
		std::ranges::set_difference(requested, authorized, std::back_inserter(uncovered));
		SetConstructionPreview(this, floor, working, uncovered);
		this->inspected_revision = job->revision;
	}

	/** Release cursor ownership when the player cancels placement. */
	void OnPlaceObjectAbort() override
	{
		this->RaiseButtons();
		this->area_quote.reset();
		this->area_portal = INVALID_TILE;
		this->status = GetString(STR_UNDERGROUND_SELECT_TOOL);
		this->survey_tile = INVALID_TILE;
		this->inspected_project = TBMProjectID{UINT32_MAX};
		this->inspected_revision = UINT64_MAX;
		this->SetDirty();
		MarkWholeScreenDirty(); // Cancelling the actual cursor immediately removes its presentation overlay.
	}
};

/** @copydoc GetUndergroundConstructionWidgetSprite */
std::optional<SpriteID> GetUndergroundConstructionWidgetSprite(const Window &window, WidgetID widget)
{
	if (window.window_class != WindowClass::UndergroundConstruction) return std::nullopt;
	return static_cast<const UndergroundConstructionWindow &>(window).GetWidgetIcon(widget);
}

/** @copydoc GetUndergroundSurveyOverlaySprite */
std::optional<SpriteID> GetUndergroundSurveyOverlaySprite(TileIndex tile)
{
	const Window *owner = _thd.GetCallbackWnd();
	// Only the actual active underground survey cursor exposes markers. A
	// background window or an unrelated construction cursor cannot leave seams
	// painted across another plane or revive the locked Deep survey control.
	if (owner == nullptr || owner->window_class != WindowClass::UndergroundConstruction || !IsValidTile(tile)) return std::nullopt;
	const auto *survey = static_cast<const UndergroundConstructionWindow *>(owner);
	if (survey->selected_tool != WID_UC_SURVEY || !survey->IsWidgetLowered(WID_UC_SURVEY) || survey->layer != GetTileLayer(tile) ||
		(survey->layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(survey->policy, TimerGameCalendar::date))) return std::nullopt;
	return GetMultiLayerSurveySprite(tile, true); // Existing Native survey/planning is public and free; no saved survey flag changes.
}

/** Native ground pictures retain actual command IDs, localized captions and existing cursor primitives. */
static constexpr std::initializer_list<NWidgetPart> _underground_widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, Colours::DarkGreen),
		NWidget(WWT_CAPTION, Colours::DarkGreen, WID_UC_CAPTION), SetStringTip(STR_UNDERGROUND_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, Colours::DarkGreen),
		NWidget(NWID_VERTICAL), SetPadding(WidgetDimensions::unscaled.framerect),
			NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_REQUIREMENTS), SetStringTip(STR_UNDERGROUND_REQUIREMENTS, STR_UNDERGROUND_REQUIREMENTS), SetMinimalSize(520, 20), SetFill(1, 0),
			NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_MODEL_DATES), SetStringTip(STR_UNDERGROUND_AREA_EMPTY, STR_UNDERGROUND_PROJECT_TOOLTIP), SetMinimalSize(520, 20), SetFill(1, 0),
			NWidget(WWT_PUSHIMGTEXTBTN, Colours::Grey, WID_UC_SURVEY), SetSpriteStringTip(SPR_CURSOR_QUERY, STR_UNDERGROUND_SURVEY, STR_UNDERGROUND_SURVEY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHIMGTEXTBTN, Colours::Grey, WID_UC_PROJECT), SetSpriteStringTip(SPR_CURSOR_QUERY, STR_UNDERGROUND_PROJECT, STR_UNDERGROUND_PROJECT_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHIMGTEXTBTN, Colours::Grey, WID_UC_AREA), SetSpriteStringTip(SPR_CURSOR_QUERY, STR_UNDERGROUND_AREA, STR_UNDERGROUND_AREA_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_MINE), SetStringTip(STR_UNDERGROUND_MINE, STR_UNDERGROUND_MINE_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHIMGTEXTBTN, Colours::Grey, WID_UC_EXCHANGE), SetSpriteStringTip(SPR_CURSOR_QUERY, STR_UNDERGROUND_EXCHANGE, STR_UNDERGROUND_EXCHANGE_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_RAIL), SetStringTip(STR_UNDERGROUND_RAIL, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_ROAD), SetStringTip(STR_UNDERGROUND_ROAD, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_RAIL_STATION), SetStringTip(STR_UNDERGROUND_RAIL_STATION, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_ROAD_TERMINAL), SetStringTip(STR_UNDERGROUND_ROAD_TERMINAL, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_RAIL_SERVICE), SetStringTip(STR_UNDERGROUND_RAIL_SERVICE, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_ROAD_SERVICE), SetStringTip(STR_UNDERGROUND_ROAD_SERVICE, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_TEXT, Colours::Invalid, WID_UC_AREA_PRICE), SetStringTip(STR_UNDERGROUND_AREA_EMPTY, STR_UNDERGROUND_AREA_TOOLTIP), SetMinimalSize(520, 20), SetFill(1, 0),
			NWidget(WWT_TEXT, Colours::Invalid, WID_UC_AREA_MATERIAL), SetStringTip(STR_UNDERGROUND_AREA_EMPTY, STR_UNDERGROUND_AREA_TOOLTIP), SetMinimalSize(520, 20), SetFill(1, 0),
			NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_AREA_FLOOR), SetStringTip(STR_UNDERGROUND_AREA_EMPTY, STR_UNDERGROUND_AREA_TOOLTIP), SetMinimalSize(520, 20), SetFill(1, 0),
			NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_AREA_CONFIRM), SetStringTip(STR_UNDERGROUND_AREA_CONFIRM, STR_UNDERGROUND_AREA_TOOLTIP), SetFill(1, 0),
			NWidget(WWT_TEXT, Colours::Invalid, WID_UC_JOB_PROGRESS), SetStringTip(STR_UNDERGROUND_JOB_NONE, STR_UNDERGROUND_PROJECT_TOOLTIP), SetMinimalSize(520, 20), SetFill(1, 0),
			NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_STATUS), SetStringTip(STR_UNDERGROUND_SELECT_TOOL, STR_UNDERGROUND_REQUIREMENTS), SetMinimalSize(520, 20), SetFill(1, 0),
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
