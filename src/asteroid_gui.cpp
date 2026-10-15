/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file asteroid_gui.cpp Read-only Space viewport rendering/picking and live survey inspector.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11; native approach waits.
 */
#include "stdafx.h"
#include "asteroid.h"
#include "space_vehicle.h"
#include "space_vehicle_cmd.h"
#include "space_manifest.h"
#include "space_structure.h"
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

/** Q16 plane center to actual Space world pixels through the existing layer owner. */
static Point SpaceWorldPoint(SpacePoint point)
{
	const TileIndex origin = TileXYL(0, 0, MapLayer::Space);
	constexpr int tile_size = static_cast<int>(TILE_SIZE);
	return {static_cast<int>(static_cast<int64_t>(point.x) * tile_size / SPACE_Q16) + tile_size / 2,
		static_cast<int>(static_cast<int64_t>(point.y) * tile_size / SPACE_Q16) + static_cast<int>(TileY(origin)) * tile_size + tile_size / 2};
}

/** Visible local bounding rectangle is only a derived culling query. */
static std::vector<OrbitalBodyID> VisibleBodies(const DrawPixelInfo &dpi)
{
	SpacePoint min{INT32_MAX, INT32_MAX}, max{INT32_MIN, INT32_MIN};
	for (int x : {dpi.left - 256, dpi.left + dpi.width + 256}) for (int y : {dpi.top - 256, dpi.top + dpi.height + 256}) {
		const Point world = InverseRemapCoords(x, y);
		const int64_t px = (static_cast<int64_t>(world.x) - TILE_SIZE / 2) * SPACE_Q16 / TILE_SIZE;
		const int64_t py = (static_cast<int64_t>(world.y) - TileY(TileXYL(0, 0, MapLayer::Space)) * TILE_SIZE - TILE_SIZE / 2) * SPACE_Q16 / TILE_SIZE;
		min.x = std::min(min.x, static_cast<int32_t>(px)); min.y = std::min(min.y, static_cast<int32_t>(py));
		max.x = std::max(max.x, static_cast<int32_t>(px)); max.y = std::max(max.y, static_cast<int32_t>(py));
	}
	return QueryAsteroidBodies(min, max);
}

void ViewportAddSpaceEntities(const DrawPixelInfo &dpi)
{
	if (_viewport_draw_layer != MapLayer::Space) return;
	const auto &field = GetAsteroidField();
	for (const auto *site : SpaceStructure::Iterate()) {
		if (site->state.phase == SpaceStructurePhase::Cancelled && site->job.inputs[0].TotalCount() == 0 && site->job.inputs[1].TotalCount() == 0) continue;
		const auto &d = *GetSpaceConstructionDefinition(site->state.kind);
		// Native wire footprint is a blueprint overlay, not final module art.
		for (uint y = 0; y <= d.height; ++y) for (uint x = 0; x < (y == d.height ? 1U : d.width); ++x) {
			const Point p = SpaceWorldPoint({static_cast<int32_t>(site->state.x + x) * SPACE_Q16, static_cast<int32_t>(site->state.y + y) * SPACE_Q16});
			AddSortableSpriteToDraw(SPR_SELECT_TILE, PAL_NONE, p.x - TILE_SIZE / 2, p.y - TILE_SIZE / 2, 0, {{0, 0, 0}, {16, 16, 1}, {0, 0, 0}});
		}
	}
	for (const auto id : VisibleBodies(dpi)) {
		const auto &body = field.bodies[id.base()];
		const Point point = SpaceWorldPoint(body.motion.position);
		// Explicit initial preview art, separate from the final Plan13 families.
		const SpriteID sprite = SPR_SPACE_PREVIEW_ASTEROID + to_underlying(body.resource_class) * 3 + to_underlying(body.size);
		AddSortableSpriteToDraw(sprite, PAL_NONE, point.x, point.y, 0, {{-8, -8, 0}, {16, 16, 16}, {0, 0, 0}});
	}
	for (const auto &survey : field.surveys) {
		const Point point = SpaceWorldPoint(survey.position);
		const auto sprite = SPR_SPACE_PREVIEW_PROBE + (survey.phase == SpaceSurveyPhase::Attached ? 1 : 0);
		AddSortableSpriteToDraw(sprite, PAL_NONE, point.x, point.y, 0, {{-4, -4, 0}, {8, 8, 8}, {0, 0, 0}});
	}
}

void RefreshSpaceViewports()
{
	if ((GetAsteroidField().elapsed_ticks % 4) != 0 || GetAsteroidField().generation_version == 0) return;
	for (Window *window : Window::Iterate()) if (window->viewport != nullptr && window->viewport->layer == MapLayer::Space) window->SetDirty();
}

static std::optional<OrbitalBodyID> _inspected_asteroid;
static SpaceCraftRef _inspected_probe;
static std::optional<OrbitalBodyID> InspectedProbeTarget(const SpaceVehicle &v)
{
	const auto &q = v.space_orders;
	if (q.cursor < q.entries.size() && q.entries[q.cursor].target.kind == SpaceOrderTargetKind::Body) return OrbitalBodyID{q.entries[q.cursor].target.id};
	if (v.mission.target != INVALID_ORBITAL_BODY) return v.mission.target;
	return std::nullopt;
}
static constexpr StringID order_actions[] = {STR_SPACE_ORDER_VISIT, STR_SPACE_ORDER_SURVEY, STR_SPACE_ORDER_MINE, STR_SPACE_ORDER_CONSTRUCT, STR_SPACE_ORDER_UNLOAD, STR_SPACE_ORDER_SERVICE};
static constexpr StringID order_targets[] = {STR_SPACE_TARGET_BODY, STR_SPACE_TARGET_STRUCTURE, STR_SPACE_TARGET_BLUEPRINT, STR_SPACE_TARGET_CENTER, STR_SPACE_TARGET_CRAFT};
static constexpr StringID order_blocks[] = {STR_SPACE_ORDER_ACTIVE, STR_SPACE_ORDER_COMPLETED, STR_SPACE_ORDER_TARGET_LOST, STR_SPACE_ORDER_FACILITY_MISSING, STR_SPACE_ORDER_HOLD_INCOMPATIBLE, STR_SPACE_ORDER_TIMED_OUT, STR_SPACE_RETURNS_PENDING, STR_SPACE_SERVICE_REQUIRED, STR_SPACE_PODS_MISSING, STR_SPACE_CONSTRUCTION_MISSING, STR_SPACE_CONSTRUCTION_BUSY, STR_GRAPH_INDUSTRY_RANGE_WAITING};
static_assert(std::size(order_blocks) == to_underlying(SpaceOrderBlock::End));
static constexpr std::initializer_list<NWidgetPart> _space_survey_widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, Colours::DarkBlue),
		NWidget(WWT_CAPTION, Colours::DarkBlue), SetStringTip(STR_SPACE_SURVEY_CAPTION),
		NWidget(WWT_STICKYBOX, Colours::DarkBlue),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, Colours::DarkBlue, 0), SetMinimalSize(600, 350), SetResize(1, 1), SetScrollbar(9), EndContainer(),
		NWidget(NWID_VSCROLLBAR, Colours::DarkBlue, 9),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 4), SetStringTip(STR_SPACE_VISIT_SELECTED, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 3), SetStringTip(STR_SPACE_SURVEY_SELECTED, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 1), SetStringTip(STR_SPACE_MINE_SELECTED, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 2), SetStringTip(STR_SPACE_RECALL), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 10), SetStringTip(STR_LAUNCH_LOGISTICS_SERVICE), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 5), SetStringTip(STR_SPACE_ORDER_CLEAR, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 6), SetStringTip(STR_SPACE_ORDER_ERASE, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 7), SetStringTip(STR_SPACE_ORDER_UP, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 8), SetStringTip(STR_SPACE_ORDER_DOWN, STR_SPACE_ORDER_EDIT_RESTARTS), SetFill(1, 0),
	EndContainer(),
};
static WindowDesc _space_survey_desc(WindowPosition::Automatic, {}, 0, 0, WindowClass::SpaceSurvey, WindowClass::None, {}, _space_survey_widgets);
struct SpaceSurveyWindow : Window {
	mutable std::vector<std::pair<SpaceCraftRef, Rect>> probe_rows;
	mutable std::vector<std::pair<uint8_t, Rect>> order_rows;
	uint8_t selected_order = UINT8_MAX;
	Scrollbar *scrollbar = nullptr;
	const IntervalTimer<TimerWindow> refresh = {std::chrono::milliseconds(250), [this]([[maybe_unused]] auto elapsed) { this->SetDirty(); }};
	SpaceSurveyWindow() : Window(_space_survey_desc) { this->InitNested(0); this->scrollbar = this->GetScrollbar(9); }
	void OnPaint() override
	{
		AutoRestoreBackup actor(_current_company, _local_company);
		const auto *probe = ResolveSpaceCraft(_inspected_probe);
		const bool owned = probe != nullptr && probe->owner == _local_company && probe->construction.placement == SpacePlacement::Orbital;
		const bool editing = owned && (!probe->manifest || probe->manifest->service_ticks == 0);
		const bool append = editing && probe->space_orders.entries.size() < MAX_SPACE_ORDERS;
		const auto *body = _inspected_asteroid ? GetMutableAsteroidBody(*_inspected_asteroid) : nullptr;
		const auto model = probe == nullptr ? UINT16_MAX : GetSpaceVehicleModel(*probe);
		// Unknown reserves do not leak through a disabled mining button. The command handles actual depletion.
		this->SetWidgetDisabledState(1, !append || model != 15 || body == nullptr);
		this->SetWidgetDisabledState(3, !append || model != 19 || body == nullptr);
		this->SetWidgetDisabledState(4, !append || body == nullptr);
		this->SetWidgetDisabledState(2, !editing || (probe->construction.home == LaunchCenterRef{} && model != 15));
		this->SetWidgetDisabledState(5, !editing || probe->space_orders.entries.empty());
		const bool selected = editing && this->selected_order < probe->space_orders.entries.size();
		this->SetWidgetDisabledState(6, !selected);
		this->SetWidgetDisabledState(7, !selected || this->selected_order == 0);
		this->SetWidgetDisabledState(8, !selected || size_t{this->selected_order} + 1 >= probe->space_orders.entries.size());
		this->SetWidgetDisabledState(10, !owned || CmdServiceLaunchProbe({}, probe->index, probe->generation,
			probe->manifest && probe->manifest->service_ticks != 0 && !probe->manifest->service_paused).Failed());
		this->DrawWidgets();
	}
	void OnClick(Point point, WidgetID widget, [[maybe_unused]] int count) override
	{
		if (widget == 0) {
			for (const auto &[ref, rect] : this->probe_rows) if (point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom) {
				_inspected_probe = ref; this->selected_order = UINT8_MAX; this->SetDirty(); return;
			}
			for (const auto &[index, rect] : this->order_rows) if (point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom) {
				this->selected_order = index; this->SetDirty(); return;
			}
		}
		const auto *probe = ResolveSpaceCraft(_inspected_probe);
		if (probe == nullptr || probe->owner != _local_company || probe->construction.placement != SpacePlacement::Orbital) return;
		const auto ref = _inspected_probe;
		auto edit = [&](SpaceOrderEdit operation, uint8_t index, SpaceOrderAction action, SpaceOrderTarget target) {
			Command<Commands::EditSpaceOrder>::Post(STR_SPACE_ORDER_FAILED, ref.vehicle, ref.generation, operation, index, action, target.kind, target.id, target.generation);
		};
		if (widget == 10) {
			Command<Commands::ServiceLaunchProbe>::Post(STR_SPACE_ORDER_FAILED, ref.vehicle, ref.generation,
				probe->manifest && probe->manifest->service_ticks != 0 && !probe->manifest->service_paused);
		} else if ((widget == 1 || widget == 3 || widget == 4) && _inspected_asteroid && probe->space_orders.entries.size() < MAX_SPACE_ORDERS) {
			edit(SpaceOrderEdit::Insert, static_cast<uint8_t>(probe->space_orders.entries.size()), widget == 1 ? SpaceOrderAction::Mine : widget == 3 ? SpaceOrderAction::Survey : SpaceOrderAction::Visit,
				{SpaceOrderTargetKind::Body, _inspected_asteroid->base(), 0});
		} else if (widget == 2) {
			if (probe->construction.home != LaunchCenterRef{}) {
				const auto home = probe->construction.home;
				edit(SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit, {});
				edit(SpaceOrderEdit::Insert, 0, SpaceOrderAction::Visit, {SpaceOrderTargetKind::LaunchCenter, home.station.base(), home.generation});
			} else if (GetSpaceVehicleModel(*probe) == 15) Command<Commands::SetSpaceMission>::Post(STR_SPACE_MISSION_FAILED, ref.vehicle, UINT32_MAX);
		} else if (widget == 5) {
			edit(SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit, {}); this->selected_order = UINT8_MAX;
		} else if (widget >= 6 && widget <= 8 && this->selected_order < probe->space_orders.entries.size()) {
			edit(widget == 6 ? SpaceOrderEdit::Erase : widget == 7 ? SpaceOrderEdit::MoveUp : SpaceOrderEdit::MoveDown, this->selected_order, SpaceOrderAction::Visit, {});
			this->selected_order = UINT8_MAX;
		}
		this->SetDirty();
	}
	void DrawWidget(const Rect &rect, WidgetID widget) const override
	{
		if (widget != 0) return;
		this->probe_rows.clear(); this->order_rows.clear();
		const auto &field = GetAsteroidField(); const auto *selected = ResolveSpaceCraft(_inspected_probe);
		const int line = GetCharacterHeight(FontSize::Normal) + 3; int y = rect.top + 8;
		auto draw = [&](const std::string &text, TextColour colour = TextColour::White) { DrawString(rect.left + 8, rect.right - 8, y, text, colour); y += line; };
		uint native_count = 0; for (const auto *probe : SpaceVehicle::Iterate()) if (probe->construction.placement == SpacePlacement::Orbital) ++native_count;
		draw(GetString(native_count == 0 ? STR_SPACE_FIELD_COUNT : STR_SPACE_NATIVE_FIELD_COUNT, field.bodies.size(), native_count == 0 ? field.surveys.size() : native_count));
		draw(GetString(native_count == 0 ? STR_SPACE_SURVEY_BOUNDARY : STR_SPACE_MINING_BOUNDARY));
		static constexpr StringID classes[] = {STR_SPACE_METALLIC, STR_SPACE_CARBONACEOUS, STR_SPACE_RICH};
		if (_inspected_asteroid && _inspected_asteroid->base() < field.bodies.size()) {
			const auto &body = field.bodies[_inspected_asteroid->base()]; const auto quantity = GetAsteroidQuantityDisclosure(_local_company, body.id);
			if (quantity) draw(GetString(STR_SPACE_BODY_RESERVE, body.id.base(), classes[to_underlying(body.resource_class)], quantity->remaining, quantity->initial));
			else draw(GetString(STR_SPACE_BODY_UNKNOWN, body.id.base(), classes[to_underlying(body.resource_class)]));
			draw(GetString(STR_SPACE_BODY_LOCATION, body.motion.position.x / SPACE_Q16, body.motion.position.y / SPACE_Q16));
		}
		if (selected != nullptr && !selected->space_orders.entries.empty()) {
			const auto &q = selected->space_orders;
			draw(GetString(STR_SPACE_ORDER_PROGRESS, std::min<size_t>(q.cursor + 1, q.entries.size()), q.entries.size(), order_blocks[to_underlying(q.blocked)]));
			if (q.cursor < q.entries.size() && q.entries[q.cursor].action == SpaceOrderAction::Survey) draw(GetString(STR_SPACE_SCAN_PROGRESS, q.work_ticks));
		}
		if (selected != nullptr && selected->manifest) {
			const auto &m = *selected->manifest;
			draw(GetString(STR_SPACE_PROBE_SERVICE_STATE, m.returns_remaining, m.pending_returns, m.pods.size(), m.service_ticks));
			draw(GetString(STR_SPACE_PROBE_PARTS, m.supplies[1].TotalCount(), m.supplies[2].TotalCount()));
			if (GetSpaceVehicleModel(*selected) == 15 && selected->space_orders.entries.empty()) draw(GetString(order_blocks[to_underlying(GetSpaceMiningBlock(*selected))]));
		}
		draw(GetString(STR_SPACE_SURVEY_HINT)); y += 4;
		const uint order_count = selected == nullptr ? 0 : static_cast<uint>(selected->space_orders.entries.size());
		this->scrollbar->SetCount(order_count + native_count + static_cast<uint>(field.surveys.size()));
		this->scrollbar->SetCapacity(std::max(1, (rect.bottom - y + 1) / line));
		uint item = 0; const int first = this->scrollbar->GetPosition(), last = first + this->scrollbar->GetCapacity();
		auto visible = [&] { const int ordinal = static_cast<int>(item++); return ordinal >= first && ordinal < last; };
		if (selected != nullptr) for (uint8_t index = 0; index < selected->space_orders.entries.size(); ++index) {
			if (!visible()) continue;
			const auto &o = selected->space_orders.entries[index]; const int top = y;
			draw(GetString(STR_SPACE_ORDER_ROW, index + 1, order_actions[to_underlying(o.action)], order_targets[to_underlying(o.target.kind)], o.target.id), index == this->selected_order ? TextColour::Yellow : TextColour::White);
			this->order_rows.push_back({index, {rect.left, top, rect.right, y - 1}});
		}
		static constexpr StringID preview_phases[] = {STR_SPACE_APPROACH, STR_SPACE_ATTACHED, STR_SPACE_RETURN, STR_SPACE_WAITING};
		for (const auto &preview : field.surveys) if (visible()) draw(GetString(STR_SPACE_SURVEY_STATUS, preview.id.base() + 1, preview_phases[to_underlying(preview.phase)], preview.target.base(), preview.completed_surveys));
		static constexpr StringID mining_phases[] = {STR_SPACE_IDLE, STR_SPACE_APPROACH, STR_SPACE_MINING, STR_SPACE_RETURN, STR_SPACE_HOLDING};
		for (const auto *probe : SpaceVehicle::Iterate()) {
			if (probe->construction.placement != SpacePlacement::Orbital || !visible()) continue;
			const int top = y;
			if (GetSpaceVehicleModel(*probe) == 19) draw(GetString(STR_SPACE_NATIVE_SURVEY_STATUS, probe->index, mining_phases[to_underlying(probe->mission.phase)]), GetSpaceCraftReference(*probe) == _inspected_probe ? TextColour::Yellow : TextColour::White);
			else draw(GetString(STR_SPACE_NATIVE_PROBE_STATUS, probe->index, mining_phases[to_underlying(probe->mission.phase)], probe->cargo.TotalCount(), probe->cargo_cap, CargoSpec::Get(probe->cargo_type)->name), GetSpaceCraftReference(*probe) == _inspected_probe ? TextColour::Yellow : TextColour::White);
			this->probe_rows.push_back({GetSpaceCraftReference(*probe), {rect.left, top, rect.right, y - 1}});
		}
	}
};
void ShowSpaceSurveyWindow()
{
	const auto *selected = ResolveSpaceCraft(_inspected_probe);
	if (selected == nullptr || selected->construction.placement != SpacePlacement::Orbital) {
		_inspected_probe = {};
		for (const auto *probe : SpaceVehicle::Iterate()) if (probe->owner == _local_company && probe->construction.placement == SpacePlacement::Orbital) { _inspected_probe = GetSpaceCraftReference(*probe); break; }
	}
	const auto &bodies = GetAsteroidField().bodies;
	if (!bodies.empty() && (!_inspected_asteroid || _inspected_asteroid->base() >= bodies.size())) _inspected_asteroid = bodies.front().id;
	if (BringWindowToFrontById(WindowClass::SpaceSurvey, 0) == nullptr) new SpaceSurveyWindow();
}

void ShowSpaceSurveyWindow(SpaceCraftRef craft)
{
	if (const auto *v = ResolveSpaceCraft(craft); v != nullptr && v->construction.placement == SpacePlacement::Orbital) _inspected_probe = craft;
	ShowSpaceSurveyWindow();
}

bool HandleSpaceEntityClick(const Viewport &vp, int x, int y)
{
	if (vp.layer != MapLayer::Space || GetAsteroidField().generation_version == 0) return false;
	x = ScaleByZoom(x - vp.left, vp.zoom) + vp.virtual_left;
	y = ScaleByZoom(y - vp.top, vp.zoom) + vp.virtual_top;
	const auto hit = [&](SpacePoint point, int radius) {
		const Point screen = RemapCoords(SpaceWorldPoint(point).x, SpaceWorldPoint(point).y, 0);
		return std::abs(screen.x - x) < radius * static_cast<int>(ZOOM_BASE) && std::abs(screen.y - y) < radius * static_cast<int>(ZOOM_BASE);
	};
	for (const auto *probe : SpaceVehicle::Iterate()) if (probe->construction.placement == SpacePlacement::Orbital && hit(probe->position, 12)) {
		_inspected_probe = GetSpaceCraftReference(*probe);
		if (const auto target = InspectedProbeTarget(*probe)) _inspected_asteroid = *target;
		ShowSpaceSurveyWindow(); return true;
	}
	for (const auto &survey : GetAsteroidField().surveys) if (hit(survey.position, 12)) {
		_inspected_asteroid = survey.target; ShowSpaceSurveyWindow(); return true;
	}
	DrawPixelInfo region{}; region.left = x; region.top = y;
	for (const auto id : VisibleBodies(region)) if (hit(GetAsteroidField().bodies[id.base()].motion.position, 30)) {
		_inspected_asteroid = id; ShowSpaceSurveyWindow(); return true;
	}
	return false;
}
