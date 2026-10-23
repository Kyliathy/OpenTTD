/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file underground_gui.cpp Underground entry tools consuming native geology, project and construction owners. @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, Groups 1-5; docs/zz-reach2/upgrades/2026-10/r2df-demo-fixes-interface-movement-and-planning.md, tasks 4.4-4.5 (New area on a busy portal, refused quotes), 10.2-10.3 (approach row), Group 12 (change work area, release and plan new work); docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BD (actual native tool icons). */

#include "stdafx.h"
#include "underground_gui.h"
#include "transport_tube_gui.h"
// Native geothermal inlet/capsule presentation: r2mlvs17-underground-transport-tubes.md Group6BDC.
#include "geothermal_gui.h"
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
#include "tbm_gui.h"
#include "timer/timer.h"
#include "timer/timer_window.h"
#include "tbm_area_preview.h"
#include "tbm_replanning.h"
#include "train.h"
#include "viewport_func.h"
#include "construction_preview.h"
#include "multilayer_sprite.h"
#include "gfx_func.h"
#include "zoom_func.h"
#include "rail_gui.h"
#include "road_gui.h"
#include "toolbar_gui.h"
#include "tilehighlight_func.h"
#include "gui.h"
#include "textbuf_gui.h"
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
	UndergroundTubeControls tubes; ///< Plan17 Group12C: transient fold-out over native construction, service and vehicle owners.
	UndergroundGeothermalControls geothermal; ///< Plan18 Group15CB: transient explicit geometry/issued-plan selections over original native owners.
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
	TBMProjectID area_project{UINT32_MAX}; ///< Existing-machine mode; invalid means the separate portal commissioning flow.
	ExcavationPlanIntent area_intent = ExcavationPlanIntent::Area; ///< Explicit transient work shape, revalidated by the synchronized command.
	std::optional<TBMExtensionPreview> extension_quote; ///< Additional-work quote for the actual paused machine, never another purchase.
	bool area_change = false; ///< Existing-machine mode replaces the unfinished remainder (r2df Group 12) instead of appending after it.
	std::optional<TBMWorkChangePreview> change_quote; ///< Change-work quote for the actual machine, never another purchase.
	TileIndex release_portal = INVALID_TILE; ///< Portal whose finished project the player was asked to release (r2df task 12.5).
	TBMProjectID release_project{UINT32_MAX}; ///< That finished project, revalidated by the Retire command.
	TBMProjectID inspected_project{UINT32_MAX}; ///< Actual clicked project identity for live progress/footprint queries, not saved authority.
	uint64_t inspected_revision = UINT64_MAX; ///< Derived overlay refresh stamp; never work permission or a second progress cursor.
	bool details_visible = false; ///< Optional guidance only; it owns no technology or construction permission.
	/** @param elapsed Unused presentation periods. Refresh the UI without stepping its actual job/controller owners. */
	const IntervalTimer<TimerWindow> project_refresh = {std::chrono::seconds(1), [this]([[maybe_unused]] auto elapsed) { this->SetDirty(); }};

	/** @param desc Widget description. @param layer Requested rock plane. @param policy Accepted display policy. */
	UndergroundConstructionWindow(WindowDesc &desc, MapLayer layer, const MultiLayerProfileSelection &policy) : Window(desc), layer(layer), policy(policy), tubes(layer), geothermal(layer)
	{
		this->status = GetString(STR_UNDERGROUND_SELECT_TOOL);
		this->CreateNestedTree();
		this->GetWidget<NWidgetStacked>(WID_UC_TUBE_STACK)->SetDisplayedPlane(SZSP_NONE);
		this->GetWidget<NWidgetStacked>(WID_UC_GEO_STACK)->SetDisplayedPlane(SZSP_NONE);
		// Hidden geothermal pages must contribute no rows before first layout;
		// the default visible site page keeps only its own native control height.
		for (const auto page : {WID_UC_GEO_UTILITIES_STACK, WID_UC_GEO_TOWN_STACK, WID_UC_GEO_INFORMATION_STACK}) this->GetWidget<NWidgetStacked>(page)->SetDisplayedPlane(SZSP_NONE);
		this->GetWidget<NWidgetStacked>(WID_UC_DETAILS_STACK)->SetDisplayedPlane(SZSP_NONE);
		this->GetWidget<NWidgetStacked>(WID_UC_QUOTE_STACK)->SetDisplayedPlane(SZSP_NONE);
		this->GetWidget<NWidgetStacked>(WID_UC_APPROACH_STACK)->SetDisplayedPlane(SZSP_NONE);
		this->GetWidget<NWidgetStacked>(WID_UC_CHANGE_STACK)->SetDisplayedPlane(SZSP_NONE);
		this->GetWidget<NWidgetStacked>(WID_UC_INTENT_STACK)->SetDisplayedPlane(SZSP_NONE);
		this->GetWidget<NWidgetStacked>(WID_UC_PROJECT_STACK)->SetDisplayedPlane(SZSP_NONE);
		this->FinishInitNested(layer);
	}

	/** Refresh optional information planes only after real selection changes; empty labels cannot consume default map space. */
	void RefreshInformationLayout()
	{
		this->GetWidget<NWidgetStacked>(WID_UC_DETAILS_STACK)->SetDisplayedPlane(this->details_visible ? 0 : SZSP_NONE);
		this->GetWidget<NWidgetStacked>(WID_UC_QUOTE_STACK)->SetDisplayedPlane(this->area_quote.has_value() || this->extension_quote.has_value() || this->change_quote.has_value() ? 0 : SZSP_NONE);
		// A refused quote has no price, material or time to show; its floor row names where planning stopped instead.
		this->GetWidget<NWidgetStacked>(WID_UC_QUOTE_COST_STACK)->SetDisplayedPlane(this->QuoteRefused() ? SZSP_NONE : 0);
		// The approach row appears only when an accepted quote digs beyond its floor, ring and housing (r2df task 10.2).
		const TBMApproachSummary *approach = this->QuoteApproach();
		this->GetWidget<NWidgetStacked>(WID_UC_APPROACH_STACK)->SetDisplayedPlane(approach != nullptr && !approach->tiles.empty() && !this->QuoteRefused() ? 0 : SZSP_NONE);
		// The kept/replaced/abandoned row belongs to an accepted change of work area only (r2df task 12.3).
		this->GetWidget<NWidgetStacked>(WID_UC_CHANGE_STACK)->SetDisplayedPlane(this->change_quote.has_value() && this->change_quote->result.Succeeded() ? 0 : SZSP_NONE);
		this->GetWidget<NWidgetStacked>(WID_UC_PROJECT_STACK)->SetDisplayedPlane(GetTBMProject(this->inspected_project) != nullptr ? 0 : SZSP_NONE);
		this->GetWidget<NWidgetStacked>(WID_UC_INTENT_STACK)->SetDisplayedPlane(this->selected_tool == WID_UC_AREA && _thd.GetCallbackWnd() == this ? 0 : SZSP_NONE); // Only an armed work tool needs an intent control.
		this->ReInit();
	}

	/**
	 * Adopt an actual browser selection and display its geological owner's current footprint.
	 * @param id Actual owned project; stale/foreign identities leave the current tool untouched.
	 */
	void InspectProject(TBMProjectID id)
	{
		const auto *project = GetTBMProject(id);
		// The list may have changed between click and callback. Recheck ownership
		// and actual job layer before giving a presentation cursor to this window.
		if (project == nullptr || project->company != _local_company) return;
		const auto *job = GetExcavationJob(project->job);
		if (job == nullptr || GetTileLayer(job->commissioning_anchor) != this->layer) return;
		SetObjectToPlaceWnd(SPR_CURSOR_QUERY, PAL_NONE, HT_RECT, this);
		this->selected_tool = WID_UC_PROJECT;
		this->inspected_project = id;
		this->inspected_revision = UINT64_MAX;
		this->status = GetString(STR_UNDERGROUND_PROJECT_SELECTED, id.base());
		this->RefreshInspectedFootprint();
		this->RefreshInformationLayout();
	}

	/** @param id Actual owned unfinished controller. Arm additional floor input after centring its accepted endpoint, without changing its route or buying machinery. */
	void PlanExtension(TBMProjectID id)
	{
		const auto *project = GetTBMProject(id);
		// Stale or foreign browser selection must never become a new-machine
		// request merely because the original controller disappeared.
		if (project == nullptr || project->company != _local_company) return;
		const auto *job = GetExcavationJob(project->job);
		if (job == nullptr || job->route.empty() || GetTileLayer(job->commissioning_anchor) != this->layer) return;
		ScrollMainWindowToTile(job->route.back().cutter_position, true);
		SetObjectToPlaceWnd(SPR_CURSOR_QUERY, PAL_NONE, HT_RECT, this);
		this->selected_tool = WID_UC_AREA;
		this->area_project = id;
		this->area_change = false;
		this->area_portal = project->portal;
		this->area_quote.reset();
		this->extension_quote.reset();
		this->change_quote.reset();
		this->inspected_project = id;
		this->LowerWidget(WID_UC_AREA);
		this->status = this->WorkInputPrompt();
		std::vector<TileIndex> working;
		std::ranges::set_difference(job->authorized_tiles, job->requested_tiles, std::back_inserter(working));
		SetConstructionPreview(this, job->requested_tiles, working);
		this->RefreshInformationLayout();
	}

	/**
	 * Arm new-area input that replaces the project's unfinished remainder, centred on the face it works from. Nothing is
	 * paused: the machine keeps working until the player confirms (r2df task 12.3).
	 * @param id Actual owned project with a completed cut; stale or foreign identities change nothing.
	 */
	void PlanWorkChange(TBMProjectID id)
	{
		const auto *project = GetTBMProject(id);
		// A stale or foreign selection must never become a new-machine request or replan another company's work.
		if (project == nullptr || project->company != _local_company || !CanChangeTBMWork(*project)) return;
		const auto *job = GetExcavationJob(project->job);
		if (job == nullptr || job->cursor == 0 || GetTileLayer(job->commissioning_anchor) != this->layer) return;
		ScrollMainWindowToTile(job->route[job->cursor - 1].cutter_position, true);
		SetObjectToPlaceWnd(SPR_CURSOR_QUERY, PAL_NONE, HT_RECT, this);
		this->selected_tool = WID_UC_AREA;
		this->area_project = id;
		this->area_change = true;
		this->area_portal = project->portal;
		this->area_quote.reset();
		this->extension_quote.reset();
		this->change_quote.reset();
		this->inspected_project = id;
		this->LowerWidget(WID_UC_AREA);
		this->status = this->WorkInputPrompt();
		std::vector<TileIndex> working;
		std::ranges::set_difference(job->authorized_tiles, job->requested_tiles, std::back_inserter(working));
		SetConstructionPreview(this, job->requested_tiles, working);
		this->RefreshInformationLayout();
	}

	/**
	 * Select an owned rail portal for the new-machine flow, with its default model, consist and origin.
	 * @param key The portal's pair key.
	 * @return Whether the portal was armed; otherwise the status says what is missing.
	 */
	bool ArmPortal(TileIndex key)
	{
		this->area_portal = key;
		this->area_model = GetLayerPortalDefaultTBMModel(key);
		this->area_consist = GetDefaultTBMConsist(GetTileLayer(GetLayerPortalPairTile(key)));
		this->area_origin = GetLayerPortalDefaultTBMOrigin(key, _local_company);
		if (this->area_model == UINT16_MAX) {
			this->area_portal = INVALID_TILE;
			this->status = GetString(STR_UNDERGROUND_NO_CUTTER);
			return false;
		}
		if (this->area_origin == StationID::Invalid()) {
			this->area_portal = INVALID_TILE;
			this->status = GetString(STR_UNDERGROUND_NO_ORIGIN);
			return false;
		}
		this->status = this->WorkInputPrompt();
		return true;
	}

	/**
	 * Answer to "Release and plan new work" (r2df task 12.5): post Retire for the finished project, then arm the same
	 * portal so the next quote reuses the stored machine. A refused Retire shows its own reason and arms nothing.
	 * @param window This palette.
	 * @param confirmed Whether the player chose to release.
	 */
	static void ReleaseAndPlanCallback(Window *window, bool confirmed)
	{
		auto *palette = static_cast<UndergroundConstructionWindow *>(window);
		const TileIndex portal = palette->release_portal;
		const TBMProjectID project = palette->release_project;
		palette->release_portal = INVALID_TILE;
		palette->release_project = TBMProjectID{UINT32_MAX};
		if (!confirmed || portal == INVALID_TILE) return;
		if (!Command<Commands::ControlTBMProject>::Post(STR_ERROR_CAN_T_CONTROL_TBM_PROJECT, project, TBMProjectAction::Retire)) return;
		// The palette's area tool may have been closed while the question was open; only a still-armed tool continues.
		if (palette->selected_tool == WID_UC_AREA && _thd.GetCallbackWnd() == palette) palette->ArmPortal(portal);
		palette->SetDirty();
	}

	/** @return Next input for the selected intent; a straight endpoint follows the actual housing or accepted route. */
	std::string WorkInputPrompt() const
	{
		if (this->area_portal == INVALID_TILE) return GetString(STR_UNDERGROUND_AREA_NEEDS_PORTAL);
		if (this->area_intent == ExcavationPlanIntent::StraightTunnel) return GetString(STR_TBM_TUNNEL_SELECT_END);
		if (this->area_project == TBMProjectID{UINT32_MAX}) return GetString(STR_UNDERGROUND_AREA_SELECT_FLOOR);
		return GetString(this->area_change ? STR_TBM_CHANGE_SELECT_FLOOR : STR_TBM_EXTEND_SELECT_FLOOR, this->area_project.base());
	}

	/** @return Whether the current fresh or extension quote was refused; a refused quote shows no price, material or time. */
	bool QuoteRefused() const
	{
		if (this->change_quote) return this->change_quote->result.Failed();
		if (this->extension_quote) return this->extension_quote->result.Failed();
		return this->area_quote.has_value() && this->area_quote->purchase.Failed();
	}

	/** @return The current quote's planner result, or nullptr before a floor has been selected. */
	const ExcavationRouteProposal *QuotePlan() const
	{
		if (this->change_quote) return &this->change_quote->plan;
		if (this->extension_quote) return &this->extension_quote->plan;
		return this->area_quote ? &this->area_quote->area.plan : nullptr;
	}

	/** @return The tile where a refused quote's planning stopped, or INVALID_TILE when the refusal has no place. */
	TileIndex QuoteFailureTile() const
	{
		if (!this->QuoteRefused()) return INVALID_TILE;
		// The commissioning preview keeps its own stable failing location; the planner's applies otherwise.
		if (this->area_quote && !this->extension_quote && !this->change_quote && IsValidTile(this->area_quote->area.failure_tile)) return this->area_quote->area.failure_tile;
		const ExcavationRouteProposal *plan = this->QuotePlan();
		return plan == nullptr ? INVALID_TILE : plan->failure_tile;
	}

	/** @return The current quote's approach summary, or nullptr before a floor has been selected. */
	const TBMApproachSummary *QuoteApproach() const
	{
		if (this->change_quote) return &this->change_quote->approach;
		if (this->extension_quote) return &this->extension_quote->approach;
		return this->area_quote ? &this->area_quote->area.approach : nullptr;
	}

	/** @return Current command owner's read-only material/time estimate, or no quote before a floor has been selected. */
	const ExcavationCutResult *AreaTotals() const
	{
		if (this->change_quote) return &this->change_quote->plan.estimate;
		if (this->extension_quote) return &this->extension_quote->plan.estimate;
		return this->area_quote ? &this->area_quote->area.totals : nullptr;
	}

	/** @param widget Control being rendered. @param stringid Default string. @return Localized context or ordinary widget text. */
	std::string GetWidgetString(WidgetID widget, StringID stringid) const override
	{
		if (const auto text = this->tubes.GetWidgetText(widget)) return *text;
		if (const auto text = this->geothermal.GetWidgetText(widget)) return *text;
		switch (widget) {
			case WID_UC_CAPTION: return GetString(STR_UNDERGROUND_CAPTION, GetMapLayerName(this->layer));
			case WID_UC_AREA: return GetString(this->area_project == TBMProjectID{UINT32_MAX} ? STR_UNDERGROUND_AREA : this->area_change ? STR_TBM_PROJECT_CHANGE_WORK : STR_TBM_PROJECT_EXTEND);
			case WID_UC_INTENT: return GetString(this->area_intent == ExcavationPlanIntent::Area ? STR_TBM_INTENT_AREA : STR_TBM_INTENT_TUNNEL);
			case WID_UC_AREA_CONFIRM: return GetString(this->change_quote ? STR_TBM_CHANGE_CONFIRM : this->extension_quote ? STR_TBM_EXTEND_CONFIRM : STR_UNDERGROUND_AREA_CONFIRM);
			case WID_UC_STATUS:
				// Locked tools must explain the actual calendar gate even while
				// optional requirements are collapsed in the compact default view.
				if (this->layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(this->policy, TimerGameCalendar::date)) return this->GetWidgetString(WID_UC_REQUIREMENTS, STR_UNDERGROUND_REQUIREMENTS);
				return this->status;
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
				if (this->QuoteRefused()) return {}; // Hidden: a refused quote buys nothing, so it shows no price.
				if (this->change_quote) {
					const VehicleID cutter = this->change_quote->cutter;
					if (!Train::IsValidID(cutter)) return GetString(STR_ERROR_TBM_EXTEND_REFERENCES);
					return GetString(STR_TBM_CHANGE_PRICE, GetString(STR_VEHICLE_NAME, cutter), this->change_quote->plan.estimate.cost);
				}
				if (this->extension_quote) {
					const VehicleID cutter = this->extension_quote->cutter;
					if (!Train::IsValidID(cutter)) return GetString(STR_ERROR_TBM_EXTEND_REFERENCES);
					return GetString(STR_TBM_EXTEND_PRICE, GetString(STR_VEHICLE_NAME, cutter), this->extension_quote->plan.estimate.cost);
				}
				if (!this->area_quote) return GetString(STR_UNDERGROUND_AREA_EMPTY);
				if (Train::IsValidID(this->area_quote->request.reuse_cutter)) return GetString(STR_TBM_REUSE_PRICE,
					GetString(STR_VEHICLE_NAME, this->area_quote->request.reuse_cutter), this->area_quote->area.totals.cost);
				return GetString(STR_UNDERGROUND_AREA_PRICE, this->area_quote->purchase.GetCost(), this->area_quote->area.totals.cost);
			case WID_UC_AREA_MATERIAL: {
				if (this->QuoteRefused()) return {};
				const auto *totals = this->AreaTotals();
				if (totals == nullptr) return GetString(STR_UNDERGROUND_AREA_EMPTY);
				return GetString(STR_UNDERGROUND_AREA_MATERIAL, totals->earth_t, totals->rock_t, totals->support_units, totals->work_ticks);
			}
			case WID_UC_AREA_TIME: {
				const auto *totals = this->AreaTotals();
				const auto *project = GetTBMProject(this->area_project);
				const auto *model = GetTBMCutterModel(project == nullptr ? this->area_model : project->model_id);
				// No valid owner/rate means no invented completion time; the refusal
				// remains in the status row and confirmation stays unavailable.
				if (totals == nullptr || model == nullptr || this->QuoteRefused()) return {};
				const auto cuts = this->QuotePlan()->route.size();
				const auto ticks = std::max<uint64_t>(cuts, CeilDiv(totals->work_ticks * model->work_rate_denominator, uint64_t{model->work_rate_numerator}));
				return GetString(STR_TBM_WORK_TIME, ticks);
			}
			case WID_UC_AREA_APPROACH: {
				// r2df task 10.2: the approach is its own line, apart from the area totals that already include it.
				const TBMApproachSummary *approach = this->QuoteApproach();
				if (approach == nullptr || approach->tiles.empty() || this->QuoteRefused()) return {};
				return GetString(STR_UNDERGROUND_AREA_APPROACH, approach->tiles.size(), approach->estimate.cost, approach->estimate.earth_t, approach->estimate.rock_t, approach->cuts);
			}
			case WID_UC_AREA_CHANGE: {
				// r2df task 12.3: what the change keeps, replaces and abandons, apart from the new work's totals.
				if (!this->change_quote || this->change_quote->result.Failed()) return {};
				const auto &quote = *this->change_quote;
				if (quote.abandoned_work_ticks == 0) return GetString(STR_TBM_CHANGE_SUMMARY, quote.cursor, quote.dropped_cuts, quote.plan.route.size());
				// Rounded up: any abandoned work shows as at least 1%. Work ticks are 64-bit, so the division stays 64-bit.
				const uint64_t whole = std::max<uint64_t>(1, quote.cut_work_ticks);
				const uint64_t done = std::min<uint64_t>(100, (quote.abandoned_work_ticks * 100 + whole - 1) / whole);
				return GetString(STR_TBM_CHANGE_SUMMARY_ABANDON, quote.cursor, quote.dropped_cuts, quote.plan.route.size(), done);
			}
			case WID_UC_AREA_FLOOR: {
				const ExcavationRouteProposal *plan = this->QuotePlan();
				if (plan == nullptr) return GetString(STR_UNDERGROUND_AREA_EMPTY);
				if (this->QuoteRefused()) {
					// Task 4.5: a refusal states how much of the floor the machine can reach and where planning stopped.
					const size_t reachable = plan->covered_targets.size();
					const size_t total = reachable + plan->remaining_targets.size();
					if (total == 0) return {}; // Refused before any planning (portal, model, consist): the status row says why.
					const TileIndex failure = this->QuoteFailureTile();
					if (IsValidTile(failure)) return GetString(STR_UNDERGROUND_AREA_REFUSED_AT, TileX(failure), TilePlaneY(failure), reachable, total);
					return GetString(STR_UNDERGROUND_AREA_REFUSED, reachable, total);
				}
				// "New solid cells" counts rock and earth actually cut; "cutter positions" counts the route's stops.
				const size_t working = this->change_quote ? this->change_quote->working_space.size() : this->extension_quote ? this->extension_quote->working_space.size() : this->area_quote->area.turn_envelope.size();
				const ExcavationCutResult &totals = this->change_quote || this->extension_quote ? plan->estimate : this->area_quote->area.totals;
				return GetString(STR_UNDERGROUND_AREA_FLOOR, plan->covered_targets.size(), working, totals.new_cells, plan->route.size(), plan->remaining_targets.size());
			}
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
			case WID_UC_TUBES:
			case WID_UC_TUBE_GUIDEWAY: asset = MultiLayerSpriteAsset::ControlTube; break;
			case WID_UC_GEOTHERMAL:
			case WID_UC_GEO_PLAN: asset = MultiLayerSpriteAsset::ControlGeothermal; break;
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
					this->change_quote || this->extension_quote ? this->QuoteRefused() : !this->area_quote || this->area_quote->purchase.Failed(),
					this->change_quote.has_value() || this->extension_quote.has_value() || this->area_quote.has_value());
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
		if (widget == WID_UC_GEO_STATUS) {
			// Native survey/refusal/capital information wraps inside this palette;
			// opening the fold-out must not stretch other map construction controls.
			size.width = std::max<uint>(size.width, ScaleGUITrad(260));
			size.height = std::max(size.height, this->geothermal.GetStatusLines() * GetCharacterHeight(FontSize::Normal) + padding.height);
			return;
		}
		// Tube request/custody and cargo previews both wrap inside the existing
		// palette, so a long cargo list cannot stretch the map's tool window.
		if (widget == WID_UC_TUBE_STATUS || widget == WID_UC_TUBE_PREVIEW) {
			size.width = std::max<uint>(size.width, ScaleGUITrad(260));
			size.height = std::max(size.height, 5 * GetCharacterHeight(FontSize::Normal) + padding.height);
			return;
		}
		if (widget == WID_UC_TUBE_BOUND) {
			size.width = std::max<uint>(size.width, ScaleGUITrad(260));
			size.height = 3 * GetCharacterHeight(FontSize::Normal) + padding.height;
			return;
		}
		// Contextual status wraps inside a bounded palette; a long localized
		// refusal must not permanently stretch every construction button.
		if (widget == WID_UC_STATUS) {
			size.width = std::max<uint>(size.width, ScaleGUITrad(260));
			size.height = std::max(size.height, 3 * GetCharacterHeight(FontSize::Normal) + padding.height);
			return;
		}
		// The template initially carries a small base-set fallback. Reserve the
		// complete native control cell before OnPaint selects its current state,
		// so a disabled/pressed picture cannot overrun the actual button frame.
		// The existing menus remain inspectable when their physical actions are
		// locked. All eight actual image/text buttons reserve their full authored
		// icon and localized caption, independently of enabled/pressed state.
		if (widget == WID_UC_SURVEY || widget == WID_UC_PROJECT || widget == WID_UC_AREA || widget == WID_UC_EXCHANGE ||
			widget == WID_UC_TUBES || widget == WID_UC_TUBE_GUIDEWAY || widget == WID_UC_GEOTHERMAL || widget == WID_UC_GEO_PLAN) {
			if (this->GetWidgetIcon(widget)) {
				const Dimension caption = GetStringBoundingBox(this->GetWidgetString(widget, this->GetWidget<NWidgetCore>(widget)->GetString()));
				const uint picture = ScaleGUITrad(20);
				size.width = std::max(size.width, caption.width + 2 * (picture + WidgetDimensions::scaled.hsep_wide) + padding.width);
				size.height = std::max(size.height, std::max(picture, caption.height) + padding.height);
			}
			return;
		}
		// Optional guidance and quotes wrap instead of letting a long localized
		// sentence stretch the entire construction palette across the map.
		if (widget == WID_UC_REQUIREMENTS || widget == WID_UC_MODEL_DATES || widget == WID_UC_AREA_PRICE || widget == WID_UC_AREA_MATERIAL ||
			widget == WID_UC_AREA_FLOOR || widget == WID_UC_AREA_TIME || widget == WID_UC_AREA_APPROACH || widget == WID_UC_AREA_CHANGE || widget == WID_UC_JOB_PROGRESS) {
			size.width = std::max<uint>(size.width, ScaleGUITrad(260));
			// The approach, change, time and floor lines carry the most terms; three lines keep each one whole (r2df tasks 10.2, 12.3).
			const uint lines = widget == WID_UC_AREA_APPROACH || widget == WID_UC_AREA_CHANGE || widget == WID_UC_AREA_TIME || widget == WID_UC_AREA_FLOOR ? 3 : 2;
			size.height = std::max<uint>(size.height, lines * GetCharacterHeight(FontSize::Normal) + padding.height);
		}
	}

	/**
	 * Draw native status cues beside the existing localized query text.
	 * @param r Actual informational row rectangle.
	 * @param widget Row identity; buttons use the standard image/text renderer.
	 */
	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		if (widget == WID_UC_GEO_STATUS) {
			DrawStringMultiLine(r, this->geothermal.GetWidgetText(widget).value_or(std::string{}), TextColour::Black);
			return;
		}
		// These two passive tube rows share bounded multiline rendering; neither
		// drawing nor refreshing them posts commands or advances utility work.
		if (widget == WID_UC_TUBE_STATUS || widget == WID_UC_TUBE_PREVIEW || widget == WID_UC_TUBE_BOUND) {
			DrawStringMultiLine(r, this->tubes.GetWidgetText(widget).value_or(std::string{}), TextColour::Black);
			return;
		}
		// Other widget families keep their normal renderer and existing input
		// behavior. These four rows explicitly reserve their icon/text regions.
		if (widget != WID_UC_REQUIREMENTS && widget != WID_UC_MODEL_DATES && widget != WID_UC_AREA_PRICE && widget != WID_UC_AREA_MATERIAL &&
			widget != WID_UC_AREA_FLOOR && widget != WID_UC_AREA_TIME && widget != WID_UC_AREA_APPROACH && widget != WID_UC_AREA_CHANGE && widget != WID_UC_STATUS && widget != WID_UC_JOB_PROGRESS) return;
		const bool rtl = _current_text_dir == TD_RTL;
		Rect text_rect = r;
		if (const auto icon = this->GetWidgetIcon(widget)) {
			DrawSpriteIgnorePadding(*icon, PAL_NONE, r.WithWidth(ScaleGUITrad(20), rtl), {AlignmentH::Centre, AlignmentV::Middle});
			text_rect = r.Indent(ScaleGUITrad(24), rtl);
		}
		const std::string text = this->GetWidgetString(widget, this->GetWidget<NWidgetCore>(widget)->GetString());
		DrawStringMultiLine(text_rect, text, TextColour::Black);
	}

	/** Refresh calendar/company availability without modifying geological or vehicle state. */
	void OnPaint() override
	{
		this->RefreshInspectedFootprint();
		const bool locked = this->layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(this->policy, TimerGameCalendar::date);
		const bool spectator = !Company::IsValidID(_local_company);
		this->tubes.Refresh(*this, locked || spectator);
		this->geothermal.Refresh(*this, locked);
		// Deep dates and actual player ownership govern offered tools; all commands still perform their own checks.
		this->SetWidgetsDisabledState(locked || spectator, WID_UC_PROJECT, WID_UC_MINE, WID_UC_EXCHANGE,
			WID_UC_RAIL, WID_UC_ROAD, WID_UC_RAIL_STATION, WID_UC_ROAD_TERMINAL, WID_UC_RAIL_SERVICE, WID_UC_ROAD_SERVICE);
		this->SetWidgetDisabledState(WID_UC_AREA, locked || spectator);
		// Only a complete successful quote offers purchase; commands revalidate date, company and every reference.
		const bool quoted = this->change_quote || this->extension_quote ? !this->QuoteRefused() : this->area_quote && this->area_quote->purchase.Succeeded();
		this->SetWidgetDisabledState(WID_UC_AREA_CONFIRM, locked || spectator || !quoted);
		this->SetWidgetDisabledState(WID_UC_SURVEY, locked);
		// Scenario road tools retain their existing editor owner; company-only purchase/service controls stay unavailable.
		this->SetWidgetDisabledState(WID_UC_ROAD, locked || (spectator && _game_mode != GameMode::Editor));
		// Update only actual control pictures; the standard widget retains the
		// localized caption, tooltip and hit testing for each existing command.
		for (const WidgetID widget : {WID_UC_SURVEY, WID_UC_PROJECT, WID_UC_AREA, WID_UC_EXCHANGE,
			WID_UC_TUBES, WID_UC_TUBE_GUIDEWAY, WID_UC_GEOTHERMAL, WID_UC_GEO_PLAN}) {
			this->GetWidget<NWidgetCore>(widget)->SetSprite(this->GetWidgetIcon(widget).value_or(SPR_IMG_DYNAMITE));
		}
		this->SetWidgetLoweredState(WID_UC_DETAILS, this->details_visible);
		this->DrawWidgets();
	}

	/** @param pt Click position. @param widget Chosen tool. @param click_count Number of clicks, unused. */
	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		if (this->IsWidgetDisabled(widget)) return;
		if (this->tubes.OnClick(*this, widget)) return;
		if (this->geothermal.OnClick(*this, widget)) return;
		if (this->tubes.IsPlacing() || this->geothermal.IsPlacing()) ResetObjectToPlace(); // An existing excavation tool must cancel either optional component's draft before taking the actual cursor.
		if (widget == WID_UC_INTENT) {
			DropDownList list;
			list.push_back(MakeDropDownListStringItem(GetString(STR_TBM_INTENT_AREA), to_underlying(ExcavationPlanIntent::Area)));
			list.push_back(MakeDropDownListStringItem(GetString(STR_TBM_INTENT_TUNNEL), to_underlying(ExcavationPlanIntent::StraightTunnel)));
			ShowDropDownList(this, std::move(list), to_underlying(this->area_intent), widget);
			return;
		}
		if (widget == WID_UC_PROJECT) {
			if (_thd.GetCallbackWnd() == this) ResetObjectToPlace();
			ShowTBMProjectsWindow(this->layer);
			return;
		}
		if (widget == WID_UC_DETAILS) {
			this->details_visible = !this->details_visible;
			this->RefreshInformationLayout();
			return;
		}
		if (_thd.GetCallbackWnd() == this) ClearConstructionPreview();
		this->survey_tile = INVALID_TILE;
		this->inspected_project = TBMProjectID{UINT32_MAX};
		this->inspected_revision = UINT64_MAX;
		switch (widget) {
			case WID_UC_AREA:
				this->selected_tool = widget;
				this->area_portal = INVALID_TILE;
				this->area_quote.reset();
				this->extension_quote.reset();
				this->change_quote.reset();
				this->area_project = TBMProjectID{UINT32_MAX};
				this->area_change = false;
				if (HandlePlacePushButton(this, widget, SPR_CURSOR_QUERY, HT_RECT)) this->status = GetString(STR_UNDERGROUND_AREA_NEEDS_PORTAL);
				this->RefreshInformationLayout();
				this->SetDirty();
				break;
			case WID_UC_AREA_CONFIRM:
				if (this->change_quote) {
					// The quote is bound to the completed-cut count; the command replans and refuses a stale one.
					if (Command<Commands::ChangeTBMWork>::Post(STR_ERROR_TBM_CHANGE_FAILED, this->area_project, this->area_start, this->area_end,
						this->change_quote->cursor, this->change_quote->fingerprint, this->area_intent)) ResetObjectToPlace();
					break;
				}
				if (this->extension_quote) {
					const auto *project = GetTBMProject(this->area_project);
					if (project == nullptr) break;
					const TileIndex portal = project->portal;
					if (Command<Commands::ExtendTBMProject>::Post(STR_ERROR_TBM_EXTEND_FAILED, this->area_project, this->area_start, this->area_end,
						this->extension_quote->job_revision, this->extension_quote->fingerprint, this->area_intent)) {
						ResetObjectToPlace();
						ShowLayerPortalWindow(portal); // The real paused controller supplies the next Resume action, including after asynchronous command dispatch.
					}
					break;
				}
				// A UI quote is only a display snapshot; the owning command supplies current permission and purchase.
				if (!this->area_quote) break;
				if (Command<Commands::ConfirmTBMCommission>::Post(STR_ERROR_CAN_T_COMMISSION_TBM, this->area_portal,
					this->area_model, this->area_consist.earth_wagons, this->area_consist.rock_wagons, this->area_origin, this->area_start, this->area_end,
					this->area_quote->fingerprint, this->area_intent)) ResetObjectToPlace();
				break;
			case WID_UC_SURVEY:
				this->selected_tool = widget;
				if (HandlePlacePushButton(this, widget, SPR_CURSOR_QUERY, HT_RECT)) this->status = GetString(STR_UNDERGROUND_SURVEY_NEXT);
				this->RefreshInformationLayout();
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

	/** @param widget Menu owner. @param index Selected work intent or actual project ID. @param click_result Dropdown detail, unused. */
	void OnDropdownSelect(WidgetID widget, int index, [[maybe_unused]] int click_result) override
	{
		if (this->tubes.OnDropdownSelect(*this, widget, index)) return;
		if (this->geothermal.OnDropdownSelect(*this, widget, index)) return;
		if (widget == WID_UC_INTENT) {
			// A delayed menu response cannot replace a different placement tool or
			// preserve a quote made for another work shape.
			if (index < 0 || index > to_underlying(ExcavationPlanIntent::StraightTunnel) || _thd.GetCallbackWnd() != this || this->selected_tool != WID_UC_AREA) return;
			this->area_intent = static_cast<ExcavationPlanIntent>(index);
			this->area_quote.reset();
			this->extension_quote.reset();
			this->change_quote.reset();
			ClearConstructionPreview();
			this->status = this->WorkInputPrompt();
			this->RefreshInformationLayout();
			return;
		}
		if (widget != WID_UC_EXCHANGE) return;
		this->exchange_project = TBMProjectID{static_cast<uint32_t>(index)};
		this->selected_tool = widget;
		this->RaiseWidget(widget); // A selected menu item is a new placement request, not a second click cancelling the dropdown button.
		HandlePlacePushButton(this, widget, SPR_CURSOR_QUERY, HT_RECT);
		this->status = GetString(STR_UNDERGROUND_EXCHANGE_NEXT, this->exchange_project.base());
		this->SetDirty();
	}

	/** @param pt World position, unused. @param tile Actual clicked world tile supplied by main or extra view. */
	void OnPlaceObject([[maybe_unused]] Point pt, TileIndex tile) override
	{
		if (this->tubes.OnPlaceObject(*this, tile)) return;
		if (this->geothermal.OnPlaceObject(*this, tile)) return;
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
					// Only an owned rail portal can start this new-machine flow;
					// another housing must not silently select incompatible defaults.
					if (!key || GetTileOwner(*key) != _local_company || GetLayerPortalTransportType(*key) != TransportType::Rail) {
						this->status = GetString(STR_UNDERGROUND_AREA_NEEDS_PORTAL);
						break;
					}
					if (GetTileLayer(GetLayerPortalPairTile(*key)) != this->layer) {
						this->status = GetString(STR_UNDERGROUND_WRONG_LAYER);
						break;
					}
					if (const auto controls = GetTBMPairControls(*key, _local_company); controls.project) {
						const TBMProject &project = *controls.project;
						// R6/task 12.5: New area on the company's own busy portal changes that project's work area, without a
						// pause. Before its first cut there is nothing to keep, so it extends the plan instead (task 4.4).
						if (CanChangeTBMWork(project)) {
							this->PlanWorkChange(project.id);
							break;
						}
						if (PauseAndPlanTBMProjectExtension(project.id)) break;
						// A finished project offers release and new work with the same machine; Retire names any refusal.
						if (IsTBMProjectFinished(project.phase) || project.phase == TBMPhase::ReleasePending) {
							this->release_portal = *key;
							this->release_project = project.id;
							ShowQuery(GetEncodedString(STR_TBM_RELEASE_AND_PLAN_CAPTION), GetEncodedString(STR_TBM_RELEASE_AND_PLAN_QUERY, project.id.base()), this, ReleaseAndPlanCallback);
							break;
						}
						this->status = GetString(STR_UNDERGROUND_PORTAL_HAS_PROJECT);
						break;
					}
					this->ArmPortal(*key);
				} else {
					if (this->area_intent == ExcavationPlanIntent::StraightTunnel) {
						TileIndex start = GetLayerPortalPairTile(this->area_portal);
						if (this->area_project != TBMProjectID{UINT32_MAX}) {
							const auto *project = GetTBMProject(this->area_project);
							const auto *job = project == nullptr ? nullptr : GetExcavationJob(project->job);
							if (job == nullptr || job->route.empty()) break; // A removed project cannot fall back to buying a new machine.
							// An extension continues from the accepted route's end; a change from the last completed cut.
							if (this->area_change && (job->cursor == 0 || job->cursor > job->route.size())) break;
							start = this->area_change ? job->route[job->cursor - 1].cutter_position : job->route.back().cutter_position;
						}
						this->QuoteWork(start, tile);
					} else {
						VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_DEMOLISH_AREA);
					}
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
				const StringID hardness = terrain ? GetGeologyHardnessString(GetExcavationCellHardness(geology, tile)) : STR_PORTAL_HARDNESS_UNKNOWN;
				this->status = GetString(STR_UNDERGROUND_SURVEY_RESULT, state, resource, deposit == nullptr ? 0 : deposit->remaining_reserve, hardness);
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
		if (this->area_intent != ExcavationPlanIntent::Area) return; // A delayed rectangular release cannot overwrite a straight endpoint quote.
		this->QuoteWork(start_tile, end_tile);
	}

	/**
	 * Query the actual owner for the selected work shape without changing simulation state.
	 * @param start_tile Rectangle corner, or exact current planned centre for a straight tunnel.
	 * @param end_tile Opposite floor corner, or requested straight centre endpoint.
	 */
	void QuoteWork(TileIndex start_tile, TileIndex end_tile)
	{
		this->area_start = start_tile;
		this->area_end = end_tile;
		if (this->area_project != TBMProjectID{UINT32_MAX} && this->area_change) {
			this->change_quote = PreviewTBMWorkChange(_local_company, this->area_project, start_tile, end_tile, this->area_intent);
			const auto &quote = *this->change_quote;
			this->ShowWorkPreview(quote.plan, quote.working_space, this->QuoteFailureTile());
			this->status = quote.result.Failed() ? GetString(quote.result.GetErrorMessage()) : GetString(STR_TBM_CHANGE_READY);
			this->RefreshInformationLayout();
			this->SetDirty();
			return;
		}
		if (this->area_project != TBMProjectID{UINT32_MAX}) {
			this->extension_quote = PreviewTBMExtension(_local_company, this->area_project, start_tile, end_tile, this->area_intent);
			const auto &quote = *this->extension_quote;
			this->ShowWorkPreview(quote.plan, quote.working_space, this->QuoteFailureTile());
			this->status = quote.result.Failed() ? GetString(quote.result.GetErrorMessage()) : GetString(STR_TBM_EXTEND_READY);
			this->RefreshInformationLayout();
			this->SetDirty();
			return;
		}
		this->area_quote = PreviewTBMCommissionArea(_local_company, this->area_portal, this->area_model, this->area_consist.earth_wagons, this->area_consist.rock_wagons,
			this->area_origin, start_tile, end_tile, false, this->area_intent);
		const auto &quote = *this->area_quote;
		this->ShowWorkPreview(quote.area.plan, quote.area.turn_envelope, this->QuoteFailureTile());
		this->status = quote.purchase.Failed() ? GetString(quote.purchase.GetErrorMessage()) : GetString(STR_UNDERGROUND_AREA_READY);
		this->RefreshInformationLayout();
		this->SetDirty();
	}

	/**
	 * Present canonical route visits and their paid work envelope without granting clearance.
	 * @param plan Actual planner result, including any refused/uncovered prefix.
	 * @param working_space Actual swept cells outside requested final floor.
	 * @param blocked Where a refused plan stopped, marked "!" on the map; INVALID_TILE for an accepted plan.
	 */
	void ShowWorkPreview(const ExcavationRouteProposal &plan, std::span<const TileIndex> working_space, TileIndex blocked)
	{
		std::vector<TileIndex> path;
		// Ordered positions, including revisits, show how the accepted cutter
		// moves through a room instead of implying that its floor is a rail path.
		for (const auto &cut : plan.route) path.push_back(cut.cutter_position);
		SetConstructionPreview(this, plan.covered_targets, working_space, plan.remaining_targets, path, blocked);
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
		this->tubes.Abort();
		this->geothermal.Abort();
		const bool extending = this->area_project != TBMProjectID{UINT32_MAX};
		const bool changing = extending && this->area_change;
		this->RaiseButtons();
		this->area_quote.reset();
		this->extension_quote.reset();
		this->change_quote.reset();
		this->area_project = TBMProjectID{UINT32_MAX};
		this->area_change = false;
		this->area_portal = INVALID_TILE;
		// Cancelling a draft releases only the cursor. An extension's machine remains paused so the player can review
		// another draft or deliberately resume; a change of work area never paused it.
		this->status = GetString(changing ? STR_TBM_CHANGE_CANCELLED : extending ? STR_TBM_EXTEND_CANCELLED : STR_UNDERGROUND_SELECT_TOOL);
		this->survey_tile = INVALID_TILE;
		this->inspected_project = TBMProjectID{UINT32_MAX};
		this->inspected_revision = UINT64_MAX;
		this->RefreshInformationLayout();
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

/** @copydoc GetUndergroundGeothermalSurveyOverlaySprite */
std::optional<SpriteID> GetUndergroundGeothermalSurveyOverlaySprite(TileIndex tile)
{
	const Window *owner = _thd.GetCallbackWnd();
	// The existing cursor owner is the sole presentation lifetime. Closing,
	// hiding, taking another tool or viewing a different plane reveals nothing.
	if (owner == nullptr || owner->window_class != WindowClass::UndergroundConstruction) return std::nullopt;
	return static_cast<const UndergroundConstructionWindow *>(owner)->geothermal.GetSurveySprite(tile);
}

/** Native ground pictures retain actual command IDs, localized captions and existing cursor primitives. */
static constexpr std::initializer_list<NWidgetPart> _underground_widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, Colours::DarkGreen),
		NWidget(WWT_CAPTION, Colours::DarkGreen, WID_UC_CAPTION), SetStringTip(STR_UNDERGROUND_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, Colours::DarkGreen),
		NWidget(NWID_VERTICAL), SetPadding(WidgetDimensions::unscaled.framerect),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_IMGTEXTBTN, Colours::Grey, WID_UC_SURVEY), SetSpriteStringTip(SPR_IMG_DYNAMITE, STR_UNDERGROUND_SURVEY, STR_UNDERGROUND_SURVEY_TOOLTIP), SetFill(1, 0),
				NWidget(WWT_PUSHIMGTEXTBTN, Colours::Grey, WID_UC_PROJECT), SetSpriteStringTip(SPR_IMG_DYNAMITE, STR_UNDERGROUND_PROJECT, STR_UNDERGROUND_PROJECT_TOOLTIP), SetFill(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_IMGTEXTBTN, Colours::Grey, WID_UC_AREA), SetSpriteStringTip(SPR_IMG_DYNAMITE, STR_UNDERGROUND_AREA, STR_UNDERGROUND_AREA_TOOLTIP), SetFill(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_MINE), SetStringTip(STR_UNDERGROUND_MINE, STR_UNDERGROUND_MINE_TOOLTIP), SetFill(1, 0),
			EndContainer(),
			NWidget(WWT_IMGTEXTBTN, Colours::Grey, WID_UC_EXCHANGE), SetSpriteStringTip(SPR_IMG_DYNAMITE, STR_UNDERGROUND_EXCHANGE, STR_UNDERGROUND_EXCHANGE_TOOLTIP), SetFill(1, 0),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_RAIL), SetStringTip(STR_UNDERGROUND_RAIL, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_ROAD), SetStringTip(STR_UNDERGROUND_ROAD, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_RAIL_STATION), SetStringTip(STR_UNDERGROUND_RAIL_STATION, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_ROAD_TERMINAL), SetStringTip(STR_UNDERGROUND_ROAD_TERMINAL, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_RAIL_SERVICE), SetStringTip(STR_UNDERGROUND_RAIL_SERVICE, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_ROAD_SERVICE), SetStringTip(STR_UNDERGROUND_ROAD_SERVICE, STR_UNDERGROUND_FACILITY_TOOLTIP), SetFill(1, 0),
			EndContainer(),
			NWidget(WWT_IMGTEXTBTN, Colours::Grey, WID_UC_TUBES), SetSpriteStringTip(SPR_IMG_DYNAMITE, STR_TUBE_MENU, STR_TUBE_HELP), SetFill(1, 0),
			NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_TUBE_STACK),
				NWidget(NWID_VERTICAL),
					NWidget(NWID_HORIZONTAL),
						NWidget(WWT_PUSHIMGTEXTBTN, Colours::Grey, WID_UC_TUBE_GUIDEWAY), SetSpriteStringTip(SPR_IMG_DYNAMITE, STR_TUBE_GUIDEWAY, STR_TUBE_HELP), SetFill(1, 0),
						NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_TUBE_TERMINAL), SetStringTip(STR_TUBE_TERMINAL, STR_TUBE_HELP), SetFill(1, 0),
						NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_TUBE_DEPOT), SetStringTip(STR_TUBE_DEPOT, STR_TUBE_HELP), SetFill(1, 0),
					EndContainer(),
					NWidget(NWID_HORIZONTAL),
						NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_TUBE_ROUTE), SetStringTip(STR_TUBE_ROUTE, STR_TUBE_PICK_ROUTE), SetFill(1, 0),
						NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_TUBE_GEOTHERMAL), SetStringTip(STR_TUBE_GEOTHERMAL_ROUTE, STR_TUBE_GEOTHERMAL_PICK_PLANT), SetFill(1, 0),
					EndContainer(),
					NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_TUBE_CONNECT), SetStringTip(STR_TUBE_CONNECT, STR_TUBE_HELP), SetFill(1, 0),
					NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_TUBE_SOURCE), SetStringTip(STR_TUBE_SOURCE, STR_TUBE_HELP), SetFill(1, 0),
					NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_TUBE_FLEET), SetStringTip(STR_TUBE_FLEET, STR_TUBE_HELP), SetFill(1, 0),
					NWidget(NWID_HORIZONTAL),
						NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_TUBE_VIEW), SetStringTip(STR_TUBE_VIEW, STR_TUBE_HELP), SetFill(1, 0),
						NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_TUBE_BIND), SetStringTip(STR_TUBE_BIND, STR_TUBE_HELP), SetFill(1, 0),
						NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_TUBE_CLOSE), SetStringTip(STR_TUBE_CLOSE, STR_TUBE_HELP), SetFill(1, 0),
					EndContainer(),
					NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_TUBE_MODELS), SetStringTip(STR_TUBE_MODELS, STR_TUBE_HELP), SetFill(1, 0),
					NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_TUBE_STATUS), SetStringTip(STR_TUBE_HELP, STR_TUBE_HELP), SetMinimalSize(260, 50), SetFill(1, 0),
					NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_TUBE_BOUND), SetStringTip(STR_TUBE_BOUND, STR_TUBE_HELP), SetMinimalSize(260, 24), SetFill(1, 0),
					NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_TUBE_PREVIEW), SetStringTip(STR_TUBE_HELP, STR_TUBE_HELP), SetMinimalSize(260, 50), SetFill(1, 0),
				EndContainer(),
			EndContainer(),
			NWidget(WWT_IMGTEXTBTN, Colours::Grey, WID_UC_GEOTHERMAL), SetSpriteStringTip(SPR_IMG_DYNAMITE, STR_GEO_MENU, STR_GEO_HELP), SetFill(1, 0),
			NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_GEO_STACK),
				NWidget(NWID_VERTICAL),
					NWidget(NWID_HORIZONTAL),
						NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_GEO_PAGE), SetStringTip(STR_GEO_SITES_PAGE, STR_GEO_HELP), SetFill(1, 0),
						NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_GEO_SURVEY), SetStringTip(STR_GEO_SURVEY, STR_GEO_HELP), SetFill(1, 0),
					EndContainer(),
					NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_GEO_ACCESS_STACK),
						NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_GEO_ACCESS), SetStringTip(STR_GEO_DIRECTION_SE, STR_GEO_HELP), SetFill(1, 0),
					EndContainer(),
					NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_GEO_PAGE_STACK),
						NWidget(NWID_VERTICAL),
							NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_GEO_FAMILY), SetStringTip(STR_GEO_COLLECTOR, STR_GEO_HELP), SetFill(1, 0),
							NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_GEO_VENT), SetStringTip(STR_GEO_PICK_VENT, STR_GEO_HELP), SetFill(1, 0),
							NWidget(NWID_HORIZONTAL),
								NWidget(WWT_PUSHIMGTEXTBTN, Colours::Grey, WID_UC_GEO_PLAN), SetSpriteStringTip(SPR_IMG_DYNAMITE, STR_GEO_PLAN, STR_GEO_HELP), SetFill(1, 0),
								NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_GEO_CONFIRM), SetStringTip(STR_GEO_CONFIRM, STR_GEO_HELP), SetFill(1, 0),
							EndContainer(),
							NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_GEO_SITE), SetStringTip(STR_GEO_SELECT_PLAN, STR_GEO_HELP), SetFill(1, 0),
							NWidget(NWID_HORIZONTAL),
								NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_GEO_FUND), SetStringTip(STR_GEO_FUND, STR_GEO_HELP), SetFill(1, 0),
								NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_GEO_CANCEL), SetStringTip(STR_GEO_CANCEL, STR_GEO_HELP), SetFill(1, 0),
							EndContainer(),
						EndContainer(),
					EndContainer(),
					NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_GEO_UTILITIES_STACK),
						NWidget(NWID_VERTICAL),
							NWidget(NWID_HORIZONTAL),
								NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_GEO_HEAT), SetStringTip(STR_GEO_HEAT, STR_GEO_PHYSICAL_HELP), SetFill(1, 0),
								NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_GEO_ELECTRIC), SetStringTip(STR_GEO_ELECTRIC, STR_GEO_PHYSICAL_HELP), SetFill(1, 0),
							EndContainer(),
							NWidget(NWID_HORIZONTAL),
								NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_GEO_REMOVE_HEAT), SetStringTip(STR_GEO_REMOVE_HEAT, STR_GEO_PHYSICAL_HELP), SetFill(1, 0),
								NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_GEO_REMOVE_ELECTRIC), SetStringTip(STR_GEO_REMOVE_ELECTRIC, STR_GEO_PHYSICAL_HELP), SetFill(1, 0),
							EndContainer(),
							NWidget(NWID_HORIZONTAL),
								NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_GEO_RISER), SetStringTip(STR_GEO_BUILD_RISER, STR_GEO_PHYSICAL_HELP), SetFill(1, 0),
								NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_GEO_RISER_CONFIRM), SetStringTip(STR_GEO_FUND, STR_GEO_PHYSICAL_HELP), SetFill(1, 0),
							EndContainer(),
						EndContainer(),
					EndContainer(),
					NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_GEO_TOWN_STACK),
						NWidget(NWID_VERTICAL),
							NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_GEO_GENERATOR), SetStringTip(STR_GEO_SELECT_GENERATOR, STR_GEO_EXPORT_PROMPT), SetFill(1, 0),
							NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_GEO_FIRST_RISER), SetStringTip(STR_GEO_SELECT_FIRST_RISER, STR_GEO_EXPORT_PROMPT), SetFill(1, 0),
							NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_GEO_SECOND_RISER), SetStringTip(STR_GEO_DIRECT_ROUTE, STR_GEO_EXPORT_PROMPT), SetFill(1, 0),
							NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_GEO_TERMINAL), SetStringTip(STR_GEO_SELECT_TERMINAL, STR_GEO_EXPORT_PROMPT), SetFill(1, 0),
							NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_GEO_TOWN), SetStringTip(STR_GEO_SELECT_TOWN, STR_GEO_EXPORT_PROMPT), SetFill(1, 0),
							NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_GEO_CONNECT), SetStringTip(STR_GEO_EXPORT, STR_GEO_EXPORT_PROMPT), SetFill(1, 0),
						EndContainer(),
					EndContainer(),
					NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_GEO_INFORMATION_STACK),
						NWidget(NWID_VERTICAL),
							NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_GEO_INFORMATION), SetStringTip(STR_GEO_BUILD_INFORMATION, STR_GEO_HELP), SetFill(1, 0),
						EndContainer(),
					EndContainer(),
					NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_GEO_STATUS), SetStringTip(STR_GEO_HELP, STR_GEO_HELP), SetMinimalSize(260, 50), SetFill(1, 0),
				EndContainer(),
			EndContainer(),
			NWidget(WWT_TEXTBTN, Colours::Grey, WID_UC_DETAILS), SetStringTip(STR_UNDERGROUND_DETAILS, STR_UNDERGROUND_REQUIREMENTS), SetFill(1, 0),
			NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_DETAILS_STACK),
				NWidget(NWID_VERTICAL),
					NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_REQUIREMENTS), SetStringTip(STR_UNDERGROUND_REQUIREMENTS, STR_UNDERGROUND_REQUIREMENTS), SetMinimalSize(260, 20), SetFill(1, 0),
					NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_MODEL_DATES), SetStringTip(STR_UNDERGROUND_AREA_EMPTY, STR_UNDERGROUND_PROJECT_TOOLTIP), SetMinimalSize(260, 20), SetFill(1, 0),
				EndContainer(),
			EndContainer(),
			NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_INTENT_STACK),
				NWidget(WWT_DROPDOWN, Colours::Grey, WID_UC_INTENT), SetStringTip(STR_TBM_INTENT_AREA, STR_TBM_INTENT_TOOLTIP), SetFill(1, 0),
			EndContainer(),
			NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_QUOTE_STACK),
				NWidget(NWID_VERTICAL),
					NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_QUOTE_COST_STACK),
						NWidget(NWID_VERTICAL),
							NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_AREA_PRICE), SetStringTip(STR_UNDERGROUND_AREA_EMPTY, STR_UNDERGROUND_AREA_TOOLTIP), SetMinimalSize(260, 20), SetFill(1, 0),
							NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_AREA_MATERIAL), SetStringTip(STR_UNDERGROUND_AREA_EMPTY, STR_UNDERGROUND_AREA_TOOLTIP), SetMinimalSize(260, 20), SetFill(1, 0),
							NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_AREA_TIME), SetStringTip(STR_UNDERGROUND_AREA_EMPTY, STR_TBM_INTENT_TOOLTIP), SetMinimalSize(260, 20), SetFill(1, 0),
							NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_APPROACH_STACK),
								NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_AREA_APPROACH), SetStringTip(STR_UNDERGROUND_AREA_EMPTY, STR_UNDERGROUND_AREA_APPROACH_TOOLTIP), SetMinimalSize(260, 20), SetFill(1, 0),
							EndContainer(),
							NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_CHANGE_STACK),
								NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_AREA_CHANGE), SetStringTip(STR_UNDERGROUND_AREA_EMPTY, STR_TBM_CHANGE_SUMMARY_TOOLTIP), SetMinimalSize(260, 20), SetFill(1, 0),
							EndContainer(),
						EndContainer(),
					EndContainer(),
					NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_AREA_FLOOR), SetStringTip(STR_UNDERGROUND_AREA_EMPTY, STR_UNDERGROUND_AREA_TOOLTIP), SetMinimalSize(260, 20), SetFill(1, 0),
					NWidget(WWT_PUSHTXTBTN, Colours::Grey, WID_UC_AREA_CONFIRM), SetStringTip(STR_UNDERGROUND_AREA_CONFIRM, STR_UNDERGROUND_AREA_TOOLTIP), SetFill(1, 0),
				EndContainer(),
			EndContainer(),
			NWidget(NWID_SELECTION, Colours::Invalid, WID_UC_PROJECT_STACK),
				NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_JOB_PROGRESS), SetStringTip(STR_UNDERGROUND_JOB_NONE, STR_UNDERGROUND_PROJECT_TOOLTIP), SetMinimalSize(260, 20), SetFill(1, 0),
			EndContainer(),
			NWidget(WWT_EMPTY, Colours::Invalid, WID_UC_STATUS), SetStringTip(STR_UNDERGROUND_SELECT_TOOL, STR_UNDERGROUND_REQUIREMENTS), SetMinimalSize(260, 30), SetFill(1, 0),
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

/** @copydoc InspectUndergroundProject */
void InspectUndergroundProject(TBMProjectID id)
{
	const auto *project = GetTBMProject(id);
	// A removed or foreign controller cannot reopen a palette with stale work
	// ownership merely because its old row was selected a moment earlier.
	if (project == nullptr || project->company != _local_company) return;
	const auto *job = GetExcavationJob(project->job);
	if (job == nullptr) return;
	Window *window = ShowUndergroundConstructionToolbar(GetTileLayer(job->commissioning_anchor), GetCurrentMultiLayerProfileSelection());
	if (window != nullptr) static_cast<UndergroundConstructionWindow *>(window)->InspectProject(id);
}

/** @copydoc PlanUndergroundProjectWorkChange */
void PlanUndergroundProjectWorkChange(TBMProjectID id)
{
	const auto *project = GetTBMProject(id);
	if (project == nullptr || project->company != _local_company) return; // A stale/foreign row cannot arm work for another machine.
	const auto *job = GetExcavationJob(project->job);
	if (job == nullptr) return;
	Window *window = ShowUndergroundConstructionToolbar(GetTileLayer(job->commissioning_anchor), GetCurrentMultiLayerProfileSelection());
	if (window != nullptr) static_cast<UndergroundConstructionWindow *>(window)->PlanWorkChange(id);
}

/** @copydoc PlanUndergroundProjectExtension */
void PlanUndergroundProjectExtension(TBMProjectID id)
{
	const auto *project = GetTBMProject(id);
	if (project == nullptr || project->company != _local_company) return; // A stale/foreign row cannot arm work for another machine.
	const auto *job = GetExcavationJob(project->job);
	if (job == nullptr) return;
	Window *window = ShowUndergroundConstructionToolbar(GetTileLayer(job->commissioning_anchor), GetCurrentMultiLayerProfileSelection());
	if (window != nullptr) static_cast<UndergroundConstructionWindow *>(window)->PlanExtension(id);
}
