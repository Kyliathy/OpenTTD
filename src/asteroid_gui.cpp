/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file asteroid_gui.cpp Read-only Space viewport rendering/picking and live survey inspector.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups8/13/30; bounded disclosures, sparse-safe selection and checked Mine policy controls.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group31D; initial paid Mine and stopped-controller status retain native flags, original service owners and each role's actual bill.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG; final natural-body and original passive-pod sprites.
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 13; the sticky survey window persists under its own INI key.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 5; that key is the incoming "space_inspector" with a 560x380 default size.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group20; generation-bound synchronized mission admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11; native approach waits.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC; retain the actual inspected incarnation across paid replacement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CD; source-linked heavy mining and original service bills.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K; original per-world-cell module back/front sources.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group13; shared mining recall admission and cargo return region.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14; company-gated composition/motion and original construction/descent progress.
 */
#include "stdafx.h"
#include "space_survey_gui.h"
#include "asteroid.h"
#include "asteroid_inspection.h"
#include "space_vehicle.h"
#include "space_sprite.h"
#include "orbital_sprite.h"
#include "space_vehicle_cmd.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "space_structure.h"
#include "space_construction_gui.h"
#include "orbital_service.h"
#include "space_mining_tuning.h"
#include "space_mining_continuation.h"
#include "widgets/space_survey_widget.h"
#include "core/backup_type.hpp"
#include "command_func.h"
#include "company_func.h"
#include "cargotype.h"
#include "viewport_func.h"
#include "map_func.h"
#include "map_layer_gui.h"
#include "landscape.h"
#include "window_gui.h"
#include "window_func.h"
#include "gfx_func.h"
#include "strings_func.h"
#include "launch_center_cmd.h"
#include "core/backup_type.hpp"
#include "zoom_func.h"
#include "timer/timer.h"
#include "timer/timer_window.h"
#include "table/strings.h"
#include "safeguards.h"

/** Q16 plane center to actual Space world pixels through the existing layer owner.
 * @param point Native orbital position. @return Actual stacked-map pixel location for rendering only.
 */
static Point SpaceWorldPoint(SpacePoint point)
{
	const TileIndex origin = TileXYL(0, 0, MapLayer::Space);
	constexpr int tile_size = static_cast<int>(TILE_SIZE);
	return {static_cast<int>(static_cast<int64_t>(point.x) * tile_size / SPACE_Q16) + tile_size / 2,
		static_cast<int>(static_cast<int64_t>(point.y) * tile_size / SPACE_Q16) + static_cast<int>(TileY(origin)) * tile_size + tile_size / 2};
}

/** Visible local bounding rectangle is only a derived culling query.
 * @param dpi Current viewport draw region. @return Original body IDs intersecting its derived orbital bounds.
 */
static std::vector<OrbitalBodyID> VisibleBodies(const DrawPixelInfo &dpi)
{
	SpacePoint min{INT32_MAX, INT32_MAX}, max{INT32_MIN, INT32_MIN};
	// Transform all four expanded viewport corners so culling never depends on an active camera plane's guessed map offset.
	for (int x : {dpi.left - 256, dpi.left + dpi.width + 256}) for (int y : {dpi.top - 256, dpi.top + dpi.height + 256}) {
		const Point world = InverseRemapCoords(x, y);
		const int64_t px = (static_cast<int64_t>(world.x) - TILE_SIZE / 2) * SPACE_Q16 / TILE_SIZE;
		const int64_t py = (static_cast<int64_t>(world.y) - TileY(TileXYL(0, 0, MapLayer::Space)) * TILE_SIZE - TILE_SIZE / 2) * SPACE_Q16 / TILE_SIZE;
		min.x = std::min(min.x, static_cast<int32_t>(px)); min.y = std::min(min.y, static_cast<int32_t>(py));
		max.x = std::max(max.x, static_cast<int32_t>(px)); max.y = std::max(max.y, static_cast<int32_t>(py));
	}
	return QueryAsteroidBodies(min, max);
}

/** Render original Space identities through their actual final family or retained blueprint/legacy owner.
 * @param dpi Current viewport region used only for culling and drawing.
 */
void ViewportAddSpaceEntities(const DrawPixelInfo &dpi)
{
	if (_viewport_draw_layer != MapLayer::Space) return;
	const auto &field = GetAsteroidField();
	// Every source cell belongs to one actual world tile. Original construction
	// and facility owners select stages; rendering grants no completion or stock.
	for (const auto *site : SpaceStructure::Iterate()) {
		const auto *definition = GetSpaceConstructionDefinition(site->state.kind);
		if (definition == nullptr) continue;
		// Quote original stock/work/actor admission once per site for this draw.
		// Per-cell copies below contain source indices only, not another job owner.
		const auto selected = GetOrbitalModuleArtPose(*site, _local_company, 0, 0);
		if (!selected) continue;
		// Back and genuine near-jaw parts remain independent native parents so a
		// visiting craft sorts between them. Honest absent fronts are transparent.
		for (uint8_t tile = 0; tile < definition->width * definition->height; ++tile) for (uint8_t part = 0; part < 2; ++part) {
			auto pose = *selected; pose.tile = tile; pose.part = part;
			const auto sprite = ResolveOrbitalSprite(pose);
			const auto bounds = GetOrbitalModuleArtBounds(*site, tile, part);
			// Both source resolution and geometry must be declared before drawing;
			// another family or guessed bounds cannot stand in for missing coverage.
			if (!sprite || !bounds) continue;
			const Point p = SpaceWorldPoint({static_cast<int32_t>(site->state.x + tile % definition->width) * SPACE_Q16,
				static_cast<int32_t>(site->state.y + tile / definition->width) * SPACE_Q16});
			AddSortableSpriteToDraw(*sprite, PAL_NONE, p.x - TILE_SIZE / 2, p.y - TILE_SIZE / 2, 0, *bounds);
		}
	}
	// Body appearance reads actual saved class/size and this observer's authorized knowledge. Visible-body lookup remains the existing local spatial query, never an all-body scan.
	for (const auto id : VisibleBodies(dpi)) {
		const auto *body = FindAsteroidBody(field, id);
		if (body == nullptr) continue;
		const Point point = SpaceWorldPoint(body->motion.position);
		const auto pose = GetSpaceAsteroidArtPose(id, _local_company);
		if (!pose) continue;
		const auto sprite = ResolveSpaceSprite(*pose);
		if (!sprite) continue;
		const auto diameter = static_cast<uint8_t>((TILE_SIZE / 2) << to_underlying(body->size));
		const auto radius = static_cast<int8_t>(diameter / 2);
		const auto negative_radius = static_cast<int8_t>(-radius);
		AddSortableSpriteToDraw(*sprite, PAL_NONE, point.x, point.y, 0,
			{{negative_radius, negative_radius, 0}, {diameter, diameter, static_cast<uint8_t>(diameter / 2)}, {0, 0, 0}});
	}
	// Only actual freely visible passive shells draw here; stored/carried references remain inside their original workshop/carrier, and successful recovery has already deleted its spent shell.
	for (const auto *pod : SpacePod::Iterate()) {
		const auto pose = GetSpacePodArtPose(*pod);
		if (!pose) continue;
		const auto sprite = ResolveSpaceSprite(*pose);
		if (!sprite) continue;
		const Point point = SpaceWorldPoint(pod->state.position);
		AddSortableSpriteToDraw(*sprite, PAL_NONE, point.x, point.y, 0, {{-4, -4, 0}, {8, 8, 8}, {0, 0, 0}});
	}
	// Historical survey previews remain visible alongside native paid hulls; this drawing loop earns no mission work.
	for (const auto &survey : field.surveys) {
		const Point point = SpaceWorldPoint(survey.position);
		const auto sprite = SPR_SPACE_PREVIEW_PROBE + (survey.phase == SpaceSurveyPhase::Attached ? 1 : 0);
		AddSortableSpriteToDraw(sprite, PAL_NONE, point.x, point.y, 0, {{-4, -4, 0}, {8, 8, 8}, {0, 0, 0}});
	}
}

/** Refresh actual Space viewports at the bounded native cadence; presentation never advances the field. */
void RefreshSpaceViewports()
{
	// An ungenerated field has nothing to animate, and four-tick throttling limits redraw cost without changing gameplay cadence.
	if ((GetAsteroidField().elapsed_ticks % 4) != 0 || GetAsteroidField().generation_version == 0) return;
	// Surface/underground windows remain outside this orbital refresh boundary.
	for (Window *window : Window::Iterate()) if (window->viewport != nullptr && window->viewport->layer == MapLayer::Space) window->SetDirty();
}

static std::optional<OrbitalBodyID> _inspected_asteroid;
static SpaceCraftRef _inspected_probe;
/** @param v Actual inspected hull. @return Its effective live body, then an actual live original/legacy target. Retired instruction history grants no actionable selection. */
static std::optional<OrbitalBodyID> InspectedProbeTarget(const SpaceVehicle &v)
{
	const auto &q = v.space_orders;
	const auto effective = ResolveEffectiveSpaceOrderTarget(v);
	// Same-mineral continuation owns the current destination. Its old Mine row
	// remains history; selecting the hull must not substitute a dead original ID.
	if (effective && effective->kind == SpaceOrderTargetKind::Body && FindAsteroidBody(GetAsteroidField(), OrbitalBodyID{effective->id}) != nullptr) return OrbitalBodyID{effective->id};
	// Only an existing body-targeting order may redirect the inspector; module and center IDs belong to different owners.
	if (q.cursor < q.entries.size() && q.entries[q.cursor].target.kind == SpaceOrderTargetKind::Body && FindAsteroidBody(GetAsteroidField(), OrbitalBodyID{q.entries[q.cursor].target.id}) != nullptr) return OrbitalBodyID{q.entries[q.cursor].target.id};
	if (FindAsteroidBody(GetAsteroidField(), v.mission.target) != nullptr) return v.mission.target;
	return std::nullopt;
}
static constexpr StringID order_actions[] = {STR_SPACE_ORDER_VISIT, STR_SPACE_ORDER_SURVEY, STR_SPACE_ORDER_MINE, STR_SPACE_ORDER_CONSTRUCT, STR_SPACE_ORDER_UNLOAD, STR_SPACE_ORDER_SERVICE};
static constexpr StringID order_targets[] = {STR_SPACE_TARGET_BODY, STR_SPACE_TARGET_STRUCTURE, STR_SPACE_TARGET_BLUEPRINT, STR_SPACE_TARGET_CENTER, STR_SPACE_TARGET_CRAFT};
// A service block names the wait; the selected hull's actual tuning/job rows
// disclose its bill. The old fixed one-part caption mislabels heavy service.
static constexpr StringID order_blocks[] = {STR_SPACE_ORDER_ACTIVE, STR_SPACE_ORDER_COMPLETED, STR_SPACE_ORDER_TARGET_LOST, STR_SPACE_ORDER_FACILITY_MISSING, STR_SPACE_ORDER_HOLD_INCOMPATIBLE, STR_SPACE_ORDER_TIMED_OUT, STR_SPACE_RETURNS_PENDING, STR_SPACE_CONT_WAIT_SERVICE, STR_SPACE_PODS_MISSING, STR_SPACE_CONSTRUCTION_MISSING, STR_SPACE_CONSTRUCTION_BUSY, STR_GRAPH_INDUSTRY_RANGE_WAITING};
static constexpr StringID mining_policies[] = {STR_SPACE_POLICY_LEGACY, STR_SPACE_POLICY_RETURN, STR_SPACE_POLICY_FULL};
static_assert(std::size(order_blocks) == to_underlying(SpaceOrderBlock::End));
static_assert(std::size(mining_policies) == to_underlying(SpaceMiningPolicy::End));

/** @param craft Original native hull. @return Its actual mining flag/continuation/wait label, or absence when the ordinary mission phase supplies status. Reads no reserve and performs no selection/work. */
static std::optional<StringID> MiningContinuationStatus(const SpaceVehicle &craft)
{
	const auto &intent = craft.mission.continuation; const auto &queue = craft.space_orders;
	// A stopped miner retains pending detachment while its own controller is
	// frozen; field safety still releases an unsafe lease independently. Active
	// onboard service follows its original separate work owner while parked,
	// so that admitted work keeps its service status instead of a false stop.
	const bool onboard_service = craft.manifest && craft.manifest->service_ticks != 0 && !craft.manifest->service_paused && craft.manifest->service_location == SpaceServiceLocation::Onboard;
	if (GetSpaceMiningTuning(GetSpaceVehicleModel(craft)) != nullptr && craft.vehstatus.Test(VehState::Stopped) && !onboard_service) return STR_VEHICLE_STATUS_STOPPED;
	// An original Mine may wait for paid handling before its first admission
	// creates continuation intent. Preserve the concrete native pending/pod/
	// service reason in fleet and inspector rows instead of reporting Idle.
	// Actual departure/return phases retain their movement status until waiting.
	if ((intent.phase == MiningContinuationPhase::None || intent.phase == MiningContinuationPhase::Waiting) &&
		queue.cursor < queue.entries.size() && queue.entries[queue.cursor].action == SpaceOrderAction::Mine) {
		switch (queue.blocked) {
			case SpaceOrderBlock::ReturnsPending: return STR_SPACE_RETURNS_PENDING;
			case SpaceOrderBlock::ServiceRequired: return STR_SPACE_CONT_WAIT_SERVICE;
			case SpaceOrderBlock::PodsMissing: return STR_SPACE_PODS_MISSING;
			default: break;
		}
	}
	// A concrete native refusal takes precedence over the broader phase. A
	// blocked local departure must show its actual safe wait instead of travel.
	switch (intent.waiting) {
		case MiningContinuationWait::NoCompatibleAsteroid: return STR_SPACE_CONT_WAIT_MINERAL;
		case MiningContinuationWait::NoReceiver: return STR_SPACE_CONT_WAIT_RECEIVER;
		case MiningContinuationWait::ServiceUnavailable: return STR_SPACE_CONT_WAIT_SERVICE;
		case MiningContinuationWait::DepartureBlocked: return STR_SPACE_CONT_WAIT_DEPARTURE;
		default: break;
	}
	switch (intent.phase) {
		case MiningContinuationPhase::Detaching: return STR_SPACE_CONT_DETACHING;
		case MiningContinuationPhase::SeekMineral:
			// The saved full-load intent remains SeekMineral after actual lease
			// acquisition. Show the original working phase once attachment has
			// succeeded, rather than claiming an already mining hull is seeking.
			if (craft.mission.phase == SpaceMissionPhase::Mining) return STR_SPACE_MINING;
			// Native extraction publishes Return when it releases a finished
			// lease; configured departure is dispatched on the next opportunity.
			// That saved transition is safe detachment, not another mineral search.
			if (craft.mission.phase == SpaceMissionPhase::Return) return STR_SPACE_CONT_DETACHING;
			return STR_SPACE_CONT_SEEK_MINERAL;
		case MiningContinuationPhase::ReturnCargo: return STR_SPACE_CONT_RETURNING;
		case MiningContinuationPhase::NeedService: return STR_SPACE_CONT_SERVICE;
		case MiningContinuationPhase::Waiting: return STR_SPACE_WAITING;
		default: return std::nullopt;
	}
}
static constexpr std::initializer_list<NWidgetPart> _space_survey_widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, Colours::DarkBlue),
		NWidget(WWT_CAPTION, Colours::DarkBlue), SetStringTip(STR_SPACE_SURVEY_CAPTION),
		NWidget(WWT_STICKYBOX, Colours::DarkBlue),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, Colours::DarkBlue, 0), SetMinimalSize(260, 120), SetResize(1, 1), SetScrollbar(9), EndContainer(),
		NWidget(NWID_VSCROLLBAR, Colours::DarkBlue, 9),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 4), SetStringTip(STR_SPACE_VISIT_SELECTED, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 3), SetStringTip(STR_SPACE_SURVEY_SELECTED, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 1), SetStringTip(STR_SPACE_MINE_SELECTED, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0), SetResize(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 2), SetStringTip(STR_SPACE_RECALL), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 10), SetStringTip(STR_LAUNCH_LOGISTICS_SERVICE), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 5), SetStringTip(STR_SPACE_ORDER_CLEAR, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0), SetResize(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 6), SetStringTip(STR_SPACE_ORDER_ERASE, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 7), SetStringTip(STR_SPACE_ORDER_UP, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 8), SetStringTip(STR_SPACE_ORDER_DOWN, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0), SetResize(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_TEXTBTN, Colours::DarkBlue, WID_SS_POLICY_LEGACY), SetStringTip(STR_SPACE_POLICY_LEGACY, STR_SPACE_POLICY_TIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_TEXTBTN, Colours::DarkBlue, WID_SS_POLICY_RETURN), SetStringTip(STR_SPACE_POLICY_RETURN, STR_SPACE_POLICY_TIP), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_TEXTBTN, Colours::DarkBlue, WID_SS_POLICY_FULL), SetStringTip(STR_SPACE_POLICY_FULL, STR_SPACE_POLICY_TIP), SetFill(1, 0), SetResize(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, Colours::DarkBlue), SetResize(1, 0), SetFill(1, 0), EndContainer(),
		NWidget(WWT_RESIZEBOX, Colours::DarkBlue, 11),
	EndContainer(),
};
// A sticky box needs an INI key (upstream WindowDesc rule, test_window_desc.cpp), so its sticky state and size persist in windows.cfg.
// m2o28 Group 5 keeps the incoming key and default size; the destination's interim "space_survey" key was never released.
static WindowDesc _space_survey_desc(WindowPosition::Automatic, "space_inspector", 560, 380, WindowClass::SpaceSurvey, WindowClass::None, {}, _space_survey_widgets);
/** Native fleet/body inspector with read-only disclosures and synchronized command controls. */
struct SpaceSurveyWindow : Window {
	/** One wrapped row; exact hull/order identities are presentation-only hit targets. */
	struct InspectionLine {
		std::string text;
		TextColour colour;
		SpaceCraftRef craft;
		uint8_t order = UINT8_MAX;
		int top = 0, height = 0;
	};
	std::vector<InspectionLine> lines;
	uint8_t selected_order = UINT8_MAX;
	Scrollbar *scrollbar = nullptr;
	/** @param elapsed Presentation refresh intervals; never simulation work or service progress. */
	const IntervalTimer<TimerWindow> refresh = {std::chrono::milliseconds(250), [this]([[maybe_unused]] auto elapsed) { this->RefreshPresentation(); this->SetDirty(); }};
	/** Initialize native controls; selection resolves actual generations on every redraw. */
	SpaceSurveyWindow() : Window(_space_survey_desc) { this->InitNested(0); this->scrollbar = this->GetScrollbar(9); this->RefreshPresentation(); }
	/**
	 * Keep the three action columns and text viewport within the actual client region.
	 * @param widget Native control identity.
	 * @param size Current measured minimum, including the scaled font.
	 * @param padding Framework padding, already included.
	 * @param fill Existing fill increments.
	 * @param resize Existing resize increments.
	 */
	void UpdateWidgetSize(WidgetID widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		if (widget == 0) size.height = std::min(size.height, static_cast<uint>(std::max(GetCharacterHeight(FontSize::Normal) + ScaleGUITrad(8), (GetMainViewBottom() - GetMainViewTop()) / 8)));
		// Long translations may ellipsize button text, but must not push any
		// original action beyond the screen or enlarge the whole inspector.
		const bool action = widget >= WID_SS_MINE && widget <= WID_SS_POLICY_FULL && widget != WID_SS_SCROLLBAR && widget != WID_SS_RESIZE;
		if (action) size.width = std::min(size.width, static_cast<uint>(std::max(100, _screen.width - ScaleGUITrad(16)) / 3));
	}
	/** Recompute wrapped rows outside paint so scrolling and hit-testing share the same complete contents. */
	void ReflowLines()
	{
		if (this->scrollbar == nullptr) return;
		const auto *panel = this->GetWidget<NWidgetBase>(0);
		const int width = std::max(1, static_cast<int>(panel->current_x) - ScaleGUITrad(8));
		int top = 0;
		// Every detail, order and hull row participates in one scroll coordinate
		// system. A long remedy cannot cover or make later rows unreachable.
		for (auto &line : this->lines) {
			line.top = top;
			line.height = GetStringHeight(line.text, width) + ScaleGUITrad(2);
			top += line.height;
		}
		this->scrollbar->SetCount(top);
		this->scrollbar->SetCapacity(std::max(1, static_cast<int>(panel->current_y) - ScaleGUITrad(8)));
		this->scrollbar->SetStepSize(GetCharacterHeight(FontSize::Normal) + ScaleGUITrad(2));
	}
	/** Fit retained geometry to the current usable client and reflow the existing read-only rows. */
	void OnResize() override
	{
		const int width = std::max<int>(this->nested_root->smallest_x, std::min(this->width, _screen.width));
		const int height = std::max<int>(this->nested_root->smallest_y, std::min(this->height, GetMainViewBottom() - GetMainViewTop()));
		// A resolution or font change must leave all actions reachable; the
		// nested callback converges after this one bounded size correction.
		if (width != this->width || height != this->height) { ResizeWindow(this, width - this->width, height - this->height, false, false); return; }
		this->left = std::clamp(this->left, 0, std::max(0, _screen.width - this->width));
		this->top = std::clamp(this->top, GetMainViewTop(), std::max(GetMainViewTop(), GetMainViewBottom() - this->height));
		this->ReflowLines();
	}
	/** Draw retained presentation only; timers and input own all control/layout refreshes. */
	void OnPaint() override { this->DrawWidgets(); }
	/** @param rect Native panel bounds. @param widget Control being painted; only the details panel draws retained rows. */
	void DrawWidget(const Rect &rect, WidgetID widget) const override
	{
		if (widget != 0) return;
		const Rect inner = rect.Shrink(ScaleGUITrad(4));
		DrawPixelInfo clipped;
		if (!FillDrawPixelInfo(&clipped, inner)) return;
		AutoRestoreBackup drawing(_cur_dpi, &clipped);
		// Clip all text to the panel and draw only rows intersecting its current
		// scroll page. Rendering never updates a hit target or native owner.
		for (const auto &line : this->lines) {
			const int y = line.top - this->scrollbar->GetPosition();
			if (y + line.height <= 0) continue;
			if (y >= inner.Height()) break;
			DrawStringMultiLine(0, inner.Width() - 1, y, y + line.height - 1, line.text, line.colour);
		}
	}
	/** Refresh controls from actual ownership and pure command admission without issuing gameplay mutations. */
	void RefreshPresentation()
	{
		AutoRestoreBackup actor(_current_company, _local_company);
		// Retirement clears only this body's actionable presentation. Keep the
		// original inspected hull/order and never transfer its highlight to a birth.
		if (_inspected_asteroid && FindAsteroidBody(GetAsteroidField(), *_inspected_asteroid) == nullptr) _inspected_asteroid.reset();
		const auto *probe = ResolveSpaceCraft(_inspected_probe);
		const bool owned = probe != nullptr && probe->owner == _local_company && probe->construction.placement == SpacePlacement::Orbital;
		const bool editing = owned && (!probe->manifest || probe->manifest->service_ticks == 0);
		const bool append = editing && probe->space_orders.entries.size() < MAX_SPACE_ORDERS;
		const auto *body = _inspected_asteroid ? GetMutableAsteroidBody(*_inspected_asteroid) : nullptr;
		const auto model = probe == nullptr ? UINT16_MAX : GetSpaceVehicleModel(*probe);
		// Unknown reserves do not leak through a disabled mining button. The command handles actual depletion.
		this->SetWidgetDisabledState(1, !append || GetSpaceMiningTuning(model) == nullptr || body == nullptr);
		this->SetWidgetDisabledState(3, !append || model != 19 || body == nullptr);
		this->SetWidgetDisabledState(4, !append || body == nullptr);
		// Mining recall uses the same admission as the synchronized mission command, including reserved service berths; other roles retain their explicit home Visit action.
		const bool mining_recall = GetSpaceMiningTuning(model) != nullptr;
		const bool recallable = editing && (mining_recall ? CmdSetSpaceMissionChecked({}, probe->index, probe->generation, UINT32_MAX).Succeeded() : probe->construction.home != LaunchCenterRef{});
		this->SetWidgetDisabledState(2, !recallable);
		this->SetWidgetDisabledState(5, !editing || probe->space_orders.entries.empty());
		const bool selected = editing && this->selected_order < probe->space_orders.entries.size();
		this->SetWidgetDisabledState(6, !selected);
		this->SetWidgetDisabledState(7, !selected || this->selected_order == 0);
		this->SetWidgetDisabledState(8, !selected || size_t{this->selected_order} + 1 >= probe->space_orders.entries.size());
		this->SetWidgetDisabledState(10, !owned || CmdServiceLaunchProbe({}, probe->index, probe->generation,
			probe->manifest && probe->manifest->service_ticks != 0 && !probe->manifest->service_paused).Failed());
		const auto mine = probe == nullptr ? std::nullopt : this->SelectedMiningOrder(*probe);
		// Each button inspects the same exact-generation native command admission
		// as execution. Saved policy alone supplies its lowered state; unsupported,
		// foreign, serviced or non-Mine selections cannot optimistically enable it.
		for (uint8_t value = 0; value < to_underlying(SpaceMiningPolicy::End); ++value) {
			const auto policy = static_cast<SpaceMiningPolicy>(value); const WidgetID widget = WID_SS_POLICY_LEGACY + value;
			const bool allowed = mine && CmdSetSpaceMiningPolicy({}, probe->index, probe->generation, *mine, policy).Succeeded();
			this->SetWidgetDisabledState(widget, !allowed);
			this->SetWidgetLoweredState(widget, mine && probe->space_orders.entries[*mine].mining_policy == policy);
		}
		this->RebuildLines();
		this->ReflowLines();
	}
	/** @param craft Original inspected hull. @return Selected existing Mine, or the current Mine when no explicit row is selected. A selected non-Mine never silently edits a different instruction. */
	std::optional<uint8_t> SelectedMiningOrder(const SpaceVehicle &craft) const
	{
		const auto &q = craft.space_orders; const auto index = this->selected_order == UINT8_MAX ? q.cursor : this->selected_order;
		// Policy belongs to one existing instruction. Neither an empty legacy
		// queue nor a future unrelated action can acquire mining behavior here.
		if (index >= q.entries.size() || q.entries[index].action != SpaceOrderAction::Mine) return std::nullopt;
		return index;
	}
	/** @param point Window-local hit position. @param widget Native action identity. @param count Framework click count, unused for these single actions. */
	void OnClick(Point point, WidgetID widget, [[maybe_unused]] int count) override
	{
		if (widget == 0) {
			const auto *panel = this->GetWidget<NWidgetBase>(0);
			const int y = point.y - panel->pos_y - ScaleGUITrad(4) + this->scrollbar->GetPosition();
			// Hit-testing shares wrapped content coordinates with drawing. Only the
			// clicked retained row may select a hull/order; text outside it has no action.
			for (const auto &line : this->lines) {
				if (y < line.top || y >= line.top + line.height) continue;
				if (line.craft != SpaceCraftRef{}) { _inspected_probe = line.craft; this->selected_order = UINT8_MAX; }
				else if (line.order != UINT8_MAX) this->selected_order = line.order;
				this->RefreshPresentation(); this->SetDirty(); return;
			}
		}
		const auto *probe = ResolveSpaceCraft(_inspected_probe);
		// Only the actual locally owned deployed hull may receive commands; stale selections cannot target reused IDs or carried payloads.
		if (probe == nullptr || probe->owner != _local_company || probe->construction.placement != SpacePlacement::Orbital) return;
		const auto ref = _inspected_probe;
		// Policy controls post only the checked native command and retain current
		// work/cargo. Refresh reads the actual saved result, including any refusal;
		// it never selects a target or updates the displayed choice optimistically.
		if (widget >= WID_SS_POLICY_LEGACY && widget <= WID_SS_POLICY_FULL) {
			const auto mine = this->SelectedMiningOrder(*probe); const auto policy = static_cast<SpaceMiningPolicy>(widget - WID_SS_POLICY_LEGACY);
			if (mine && CmdSetSpaceMiningPolicy({}, ref.vehicle, ref.generation, *mine, policy).Succeeded()) {
				Command<Commands::SetSpaceMiningPolicy>::Post(STR_SPACE_ORDER_FAILED, ref.vehicle, ref.generation, *mine, policy);
			}
			this->RefreshPresentation(); this->SetDirty(); return;
		}
		/** @param operation Queue mutation. @param index Existing or insertion slot. @param action Typed action. @param target Original generation-qualified target. */
		auto edit = [&](SpaceOrderEdit operation, uint8_t index, SpaceOrderAction action, SpaceOrderTarget target) {
			Command<Commands::EditSpaceOrder>::Post(STR_SPACE_ORDER_FAILED, ref.vehicle, ref.generation, operation, index, action, target.kind, target.id, target.generation);
		};
		// Appending needs a selected body and queue headroom; native admission still owns role/facility/service/return-route checks. Reordering likewise requires an existing selected slot.
		if (widget == 10) {
			Command<Commands::ServiceLaunchProbe>::Post(STR_SPACE_ORDER_FAILED, ref.vehicle, ref.generation,
				probe->manifest && probe->manifest->service_ticks != 0 && !probe->manifest->service_paused);
		} else if ((widget == 1 || widget == 3 || widget == 4) && _inspected_asteroid && probe->space_orders.entries.size() < MAX_SPACE_ORDERS) {
			edit(SpaceOrderEdit::Insert, static_cast<uint8_t>(probe->space_orders.entries.size()), widget == 1 ? SpaceOrderAction::Mine : widget == 3 ? SpaceOrderAction::Survey : SpaceOrderAction::Visit,
				{SpaceOrderTargetKind::Body, _inspected_asteroid->base(), 0});
		} else if (widget == 2) {
			// A miner's cargo return uses its actual circular release region and retained body departure; an explicit Visit remains the separate precise navigation action for other roles.
			if (GetSpaceMiningTuning(GetSpaceVehicleModel(*probe)) != nullptr) {
				Command<Commands::SetSpaceMissionChecked>::Post(STR_SPACE_MISSION_FAILED, ref.vehicle, ref.generation, UINT32_MAX);
			} else if (probe->construction.home != LaunchCenterRef{}) {
				const auto home = probe->construction.home;
				edit(SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit, {});
				edit(SpaceOrderEdit::Insert, 0, SpaceOrderAction::Visit, {SpaceOrderTargetKind::LaunchCenter, home.station.base(), home.generation});
			}
		} else if (widget == 5) {
			edit(SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit, {}); this->selected_order = UINT8_MAX;
		} else if (widget >= 6 && widget <= 8 && this->selected_order < probe->space_orders.entries.size()) {
			edit(widget == 6 ? SpaceOrderEdit::Erase : widget == 7 ? SpaceOrderEdit::MoveUp : SpaceOrderEdit::MoveDown, this->selected_order, SpaceOrderAction::Visit, {});
			this->selected_order = UINT8_MAX;
		}
		this->RefreshPresentation();
		this->SetDirty();
	}
	/** Collect all owner disclosures and hit identities without drawing or advancing gameplay. */
	void RebuildLines()
	{
		this->lines.clear();
		const auto &field = GetAsteroidField(); const auto *selected = ResolveSpaceCraft(_inspected_probe);
		/** @param text Localized owner disclosure. @param colour Selection colour, never an authority change. */
		auto draw = [&](const std::string &text, TextColour colour = TextColour::White) { this->lines.push_back({text, colour, {}}); };
		uint native_count = 0; for (const auto *probe : SpaceVehicle::Iterate()) if (probe->construction.placement == SpacePlacement::Orbital) ++native_count;
		draw(GetString(native_count == 0 ? STR_SPACE_FIELD_COUNT : STR_SPACE_NATIVE_FIELD_COUNT, field.bodies.size(), native_count == 0 ? field.surveys.size() : native_count));
		draw(GetString(native_count == 0 ? STR_SPACE_SURVEY_BOUNDARY : STR_SPACE_MINING_BOUNDARY));
		static constexpr StringID classes[] = {STR_SPACE_METALLIC, STR_SPACE_CARBONACEOUS, STR_SPACE_RICH};
		const auto *selected_body = _inspected_asteroid ? FindAsteroidBody(field, *_inspected_asteroid) : nullptr;
		// Only a body still present in the actual field may reveal source-authorized survey knowledge.
		if (selected_body != nullptr) {
			const auto &body = *selected_body; const auto inspection = *GetAsteroidInspectionDisclosure(_local_company, body.id); const auto &quantity = inspection.quantities;
			if (quantity) draw(GetString(STR_SPACE_BODY_RESERVE, body.id.base(), classes[to_underlying(body.resource_class)], quantity->remaining, quantity->initial));
			else draw(GetString(STR_SPACE_BODY_UNKNOWN, body.id.base(), classes[to_underlying(body.resource_class)]));
			draw(GetString(STR_SPACE_BODY_LOCATION, body.motion.position.x / SPACE_Q16, body.motion.position.y / SPACE_Q16));
			draw(GetString(STR_SPACE_BODY_COMPOSITION, CargoSpec::Get(inspection.cargo)->name));
			// Explicit signs preserve reflected/sub-resolution direction; the decimal formatter receives nonnegative magnitudes so negative fractions cannot place a sign after the decimal point.
			draw(GetString(STR_SPACE_BODY_TRAJECTORY, std::string{inspection.trajectory.x_negative ? "-" : "+"}, std::abs(inspection.trajectory.x_milli), 3,
				std::string{inspection.trajectory.y_negative ? "-" : "+"}, std::abs(inspection.trajectory.y_milli), 3, inspection.trajectory.speed_milli, 3));
		}
		// The selected builder's real current instruction and next action remain
		// visible independently of the mining controller's idle/holding state.
		if (selected != nullptr && GetSpaceVehicleModel(*selected) == 20) {
			for (const auto &line : GetSpaceConstructionBuilderText(*selected)) draw(line);
		}
		// Queue/progress disclosure requires the actual selected incarnation and an existing action; a stale cursor never creates work.
		if (selected != nullptr && !selected->space_orders.entries.empty()) {
			const auto &q = selected->space_orders;
			// The shared ProducerUnavailable fallback needs the continuation's
			// actual reason; an unsafe/missing mineral is not a missing facility.
			// A stopped controller cannot claim Working/travelling. Keep concrete
			// pod, pending-return and service blocks informative alongside its flag.
			StringID progress = order_blocks[to_underlying(q.blocked)];
			const auto continuation = MiningContinuationStatus(*selected);
			if (q.blocked == SpaceOrderBlock::ProducerUnavailable || (q.blocked == SpaceOrderBlock::None && continuation == STR_VEHICLE_STATUS_STOPPED)) progress = continuation.value_or(progress);
			draw(GetString(STR_SPACE_ORDER_PROGRESS, std::min<size_t>(q.cursor + 1, q.entries.size()), q.entries.size(), progress));
			// Survey work belongs only to the current typed Survey action, independently of mining or service work.
			if (q.cursor < q.entries.size() && q.entries[q.cursor].action == SpaceOrderAction::Survey) draw(GetString(STR_SPACE_SCAN_PROGRESS, q.work_ticks));
			// Current or just-completed construction retains the exact original job.
			// The queue cursor advancing cannot make consumed material look missing.
			const auto construction = GetOrbitalConstructionReadiness(*selected);
			if (construction.target != SpaceStructureRef{}) {
				if (const auto *site = ResolveSpaceStructure(construction.target)) {
					// Both construction views describe the same original packet/work owner.
					for (const auto &line : GetSpaceConstructionMaterialText(*site, selected)) draw(line);
				}
			}
		}
		// The original queue rows remain visible below. Show its actual current
		// destination separately, so a retained Mine/body history cannot masquerade
		// as the live same-mineral receiver or imply that a berth was reached.
		if (selected != nullptr && GetSpaceMiningTuning(GetSpaceVehicleModel(*selected)) != nullptr) {
			if (const auto mine = this->SelectedMiningOrder(*selected)) draw(GetString(STR_SPACE_POLICY_DETAIL, *mine + 1, mining_policies[to_underlying(selected->space_orders.entries[*mine].mining_policy)]));
			else draw(GetString(STR_SPACE_POLICY_MANUAL));
			const auto effective = ResolveEffectiveSpaceOrderTarget(*selected);
			// A retired original target remains truthful queue history, but cannot
			// be disclosed as a current live asteroid destination or click target.
			if (effective && (effective->kind != SpaceOrderTargetKind::Body || FindAsteroidBody(field, OrbitalBodyID{effective->id}) != nullptr)) {
				const bool site = effective->kind == SpaceOrderTargetKind::Structure || effective->kind == SpaceOrderTargetKind::Blueprint;
				draw(GetString(STR_SPACE_EFFECTIVE_TARGET, order_targets[to_underlying(effective->kind)], uint64_t{effective->id} + (site ? 1 : 0)));
			} else {
				draw(GetString(STR_SPACE_EFFECTIVE_NONE));
			}
			if (const auto status = MiningContinuationStatus(*selected)) draw(GetString(STR_SPACE_CONT_STATUS, *status));
		}
		// Actual contained compartments, rather than role defaults, supply loose parts, shells and earned service history.
		if (selected != nullptr && selected->manifest) {
			const auto &m = *selected->manifest;
			draw(GetString(STR_SPACE_PROBE_SERVICE_STATE, m.returns_remaining, m.pending_returns, m.pods.size()));
			draw(GetString(STR_SPACE_PROBE_PARTS, m.supplies[1].TotalCount(), m.supplies[2].TotalCount()));
			// Only this exact sender incarnation's actual descent pods may report retained cargo/countdown; an old numeric vehicle ID cannot reveal another hull's recovery claim.
			for (const auto *pod : SpacePod::Iterate()) {
				if (!pod->descent) continue; const auto &job = *pod->descent;
				if (job.sender != selected->index || job.sender_generation != selected->generation) continue;
				draw(GetString(STR_SPACE_PROBE_DESCENT, pod->index.base() + 1, job.blocked == SpaceDescentBlock::None ? STR_LAUNCH_DESCENT_WORKING : STR_LAUNCH_DESCENT_WAITING,
					job.quantity, CargoSpec::Get(job.cargo)->name, job.remaining_ticks));
			}
			const auto model = GetSpaceVehicleModel(*selected); const auto tuning = GetSpaceServiceTuning(model);
			if (const auto *mining = GetSpaceMiningTuning(model)) {
				draw(GetString(STR_SPACE_MINER_BUDGET, mining->hold_sixteenths / 16, mining->extraction_units, mining->extraction_ticks));
				draw(GetString(STR_SPACE_MINER_WORK, selected->mission.work_ticks, mining->extraction_ticks, selected->extracted_units));
			}
			if (tuning) {
				const StringID role = model == SPACE_HEAVY_MINER_MODEL ? STR_SPACE_MANUFACTURE_HEAVY_MINING : model == 15 ? STR_SPACE_MANUFACTURE_MINING : model == 19 ? STR_SPACE_MANUFACTURE_SURVEY : STR_SPACE_MANUFACTURE_BUILDER;
				draw(GetString(STR_SPACE_SERVICE_BILL, role, tuning->inputs[0], tuning->inputs[1], tuning->ticks, tuning->returns));
				if (m.service_ticks != 0) draw(GetString(STR_SPACE_SERVICE_ONBOARD, m.service_ticks, tuning->ticks, m.service_paused ? STR_CORE_SERVICE_PAUSED : STR_CORE_SERVICE_WORKING));
			}
			// Resolve only the selected hull's actual generation-qualified contained job; an idle berth or another craft's reservation cannot imply service progress.
			for (const auto *site : SpaceStructure::Iterate()) {
				if (!site->module) continue;
				const auto jobs = site->module->GetServiceJobs();
				for (size_t berth = 0; berth < jobs.size(); ++berth) {
					const auto &job = *jobs[berth];
					// Completed/foreign generations carry no work for this inspector; original legacy jobs retain their own immutable one-part budget.
					if (job.remaining_ticks == 0 || job.craft != GetSpaceCraftReference(*selected)) continue;
					if (const auto bill = GetOrbitalServiceJobTuning(job)) draw(GetString(STR_SPACE_SERVICE_CONTAINED, site->index.base() + 1, berth + 1, job.remaining_ticks, bill->ticks));
					draw(GetString(STR_SPACE_SERVICE_RESERVED, job.inputs[0].TotalCount(), job.inputs[1].TotalCount()));
				}
			}
			// Only an actual native refusal deserves an availability warning. Eligibility alone does not mean an idle hull is working or travelling.
			if (GetSpaceMiningTuning(model) != nullptr && selected->space_orders.entries.empty()) {
				const auto block = GetSpaceMiningBlock(*selected);
				if (block != SpaceOrderBlock::None) draw(GetString(order_blocks[to_underlying(block)]));
			}
		}
		draw(GetString(STR_SPACE_SURVEY_HINT));
		// Retain every order/hull row. Pixel scrolling covers the complete details
		// and list together, so additional readiness text cannot hide controls.
		if (selected != nullptr) for (uint8_t index = 0; index < selected->space_orders.entries.size(); ++index) {
			const auto &o = selected->space_orders.entries[index];
			// Site numbering matches the construction selector/current instruction;
			// natural-body and other target domains retain their existing identities.
			const bool site = o.target.kind == SpaceOrderTargetKind::Structure || o.target.kind == SpaceOrderTargetKind::Blueprint;
			draw(GetString(STR_SPACE_ORDER_ROW, index + 1, order_actions[to_underlying(o.action)], order_targets[to_underlying(o.target.kind)], uint64_t{o.target.id} + (site ? 1 : 0)), index == this->selected_order ? TextColour::Yellow : TextColour::White);
			this->lines.back().order = index;
		}
		static constexpr StringID preview_phases[] = {STR_SPACE_APPROACH, STR_SPACE_ATTACHED, STR_SPACE_RETURN, STR_SPACE_WAITING};
		for (const auto &preview : field.surveys) draw(GetString(STR_SPACE_SURVEY_STATUS, preview.id.base() + 1, preview_phases[to_underlying(preview.phase)], preview.target.base(), preview.completed_surveys));
		static constexpr StringID mining_phases[] = {STR_SPACE_IDLE, STR_SPACE_APPROACH, STR_SPACE_MINING, STR_SPACE_RETURN, STR_SPACE_HOLDING};
		for (const auto *probe : SpaceVehicle::Iterate()) {
			// Carried and Surface hulls have no independent orbital list entry.
			if (probe->construction.placement != SpacePlacement::Orbital) continue;
			const auto model = GetSpaceVehicleModel(*probe); const auto &orders = probe->space_orders; StringID status = mining_phases[to_underlying(probe->mission.phase)];
			if (const auto continuation = MiningContinuationStatus(*probe)) status = *continuation;
			// Survey/construction/freight roles keep the mining controller in Holding during real typed work. Their current action or actual wait supplies mission status instead of falsely reporting Idle.
			if (GetSpaceMiningTuning(model) == nullptr && orders.cursor < orders.entries.size()) {
				status = orders.blocked == SpaceOrderBlock::None ? order_actions[to_underlying(orders.entries[orders.cursor].action)] : order_blocks[to_underlying(orders.blocked)];
			}
			if (model == 19) draw(GetString(STR_SPACE_NATIVE_SURVEY_STATUS, probe->index, status), GetSpaceCraftReference(*probe) == _inspected_probe ? TextColour::Yellow : TextColour::White);
			else draw(GetString(STR_SPACE_NATIVE_PROBE_STATUS, probe->index, status, probe->cargo.TotalCount(), probe->cargo_cap, CargoSpec::Get(probe->cargo_type)->name), GetSpaceCraftReference(*probe) == _inspected_probe ? TextColour::Yellow : TextColour::White);
			this->lines.back().craft = GetSpaceCraftReference(*probe);
		}
	}
};
/** Open the native inspector, retaining valid selections or choosing an actual owned orbital hull. */
void ShowSpaceSurveyWindow()
{
	const auto *selected = ResolveSpaceCraft(_inspected_probe);
	// Lost or no-longer-deployed hulls cannot lend a generation to a newly opened orbital inspector.
	if (selected == nullptr || selected->construction.placement != SpacePlacement::Orbital) {
		_inspected_probe = {};
		// Initial selection is one actual local deployed hull; scanning does not create a vehicle or reserve an approach.
		for (const auto *probe : SpaceVehicle::Iterate()) if (probe->owner == _local_company && probe->construction.placement == SpacePlacement::Orbital) { _inspected_probe = GetSpaceCraftReference(*probe); break; }
	}
	auto *window = static_cast<SpaceSurveyWindow *>(BringWindowToFrontById(WindowClass::SpaceSurvey, 0));
	// Preserve independent craft/order context and clear only a retired body.
	// An already open inspector cannot transfer its old highlight to a birth;
	// only a newly opened unselected inspector chooses an initial live default.
	if (_inspected_asteroid) {
		if (FindAsteroidBody(GetAsteroidField(), *_inspected_asteroid) == nullptr) _inspected_asteroid.reset();
	} else if (window == nullptr) {
		_inspected_asteroid = ResolveAsteroidSelection(GetAsteroidField(), std::nullopt);
	}
	if (window == nullptr) window = new SpaceSurveyWindow();
	window->RefreshPresentation(); window->SetDirty();
}

/** @param craft Original desired hull incarnation; only an actual deployed hull can become the inspector selection. */
void ShowSpaceSurveyWindow(SpaceCraftRef craft)
{
	// A stale or Surface incarnation cannot replace an existing valid orbital selection.
	if (const auto *v = ResolveSpaceCraft(craft); v != nullptr && v->construction.placement == SpacePlacement::Orbital) {
		_inspected_probe = craft;
		// An explicit hull inspection follows its actual live effective source.
		// Historical Mine IDs remain queue text and never become a new highlight.
		if (const auto target = InspectedProbeTarget(*v)) _inspected_asteroid = *target;
	}
	ShowSpaceSurveyWindow();
}

/** @copydoc RetargetSpaceVehicleInspector */
void RetargetSpaceVehicleInspector(SpaceCraftRef source, SpaceCraftRef product)
{
	if (_inspected_probe != source) return;
	const auto *vehicle = ResolveSpaceCraft(product);
	// The orbital inspector follows only an actual new deployed hull; pending Earth work retains its separate physical center/view owner.
	if (vehicle == nullptr || vehicle->construction.placement != SpacePlacement::Orbital) return;
	_inspected_probe = product;
	if (auto *window = FindWindowById(WindowClass::SpaceSurvey, 0); window != nullptr) window->SetDirty();
}

/** Pick actual Space identities without changing gameplay.
 * @param vp Clicked viewport. @param x Screen horizontal position. @param y Screen vertical position.
 * @return Whether an actual visible Space entity opened its inspector.
 */
bool HandleSpaceEntityClick(const Viewport &vp, int x, int y)
{
	// Other layers and an ungenerated field have no orbital entities to inspect.
	if (vp.layer != MapLayer::Space || GetAsteroidField().generation_version == 0) return false;
	x = ScaleByZoom(x - vp.left, vp.zoom) + vp.virtual_left;
	y = ScaleByZoom(y - vp.top, vp.zoom) + vp.virtual_top;
	/** @param point Actual orbital position. @param radius Screen-space picking tolerance. @return Whether this entity contains the transformed click. */
	const auto hit = [&](SpacePoint point, int radius) {
		const Point screen = RemapCoords(SpaceWorldPoint(point).x, SpaceWorldPoint(point).y, 0);
		return std::abs(screen.x - x) < radius * static_cast<int>(ZOOM_BASE) && std::abs(screen.y - y) < radius * static_cast<int>(ZOOM_BASE);
	};
	// Native deployed hulls take picking priority; carried payloads have no independent orbital position to select.
	for (const auto *probe : SpaceVehicle::Iterate()) if (probe->construction.placement == SpacePlacement::Orbital && hit(probe->position, 12)) {
		_inspected_probe = GetSpaceCraftReference(*probe);
		if (const auto target = InspectedProbeTarget(*probe)) _inspected_asteroid = *target;
		ShowSpaceSurveyWindow(); return true;
	}
	// Retained preview probes and visible bodies open read-only selections after native hulls have been considered.
	for (const auto &survey : GetAsteroidField().surveys) if (hit(survey.position, 12)) {
		_inspected_asteroid = survey.target; ShowSpaceSurveyWindow(); return true;
	}
	DrawPixelInfo region{}; region.left = x; region.top = y;
	// A queried ID is only a candidate. Retirement between retained query data
	// and lookup cannot redirect a click to the body now occupying its old slot.
	for (const auto id : VisibleBodies(region)) {
		const auto *body = FindAsteroidBody(GetAsteroidField(), id);
		if (body == nullptr) continue;
		if (hit(body->motion.position, 30)) {
			_inspected_asteroid = id; ShowSpaceSurveyWindow(); return true;
		}
	}
	return false;
}

/** @copydoc GetSpaceSurveySelection */
SpaceCraftRef GetSpaceSurveySelection()
{
    const auto *vehicle = ResolveSpaceCraft(_inspected_probe);
    // A GUI query reports only its surviving actual deployed hull; a reused
    // pool ID or a stored/passenger role cannot become an orbital selection.
    return vehicle != nullptr && vehicle->construction.placement == SpacePlacement::Orbital ? _inspected_probe : SpaceCraftRef{};
}
