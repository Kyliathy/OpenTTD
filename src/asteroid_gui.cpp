/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file asteroid_gui.cpp Read-only Space viewport rendering/picking and live survey inspector.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
 */
#include "stdafx.h"
#include "asteroid.h"
#include "space_vehicle.h"
#include "space_vehicle_cmd.h"
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
static VehicleID _inspected_probe = VehicleID::Invalid();

static constexpr std::initializer_list<NWidgetPart> _space_survey_widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, Colours::DarkBlue),
		NWidget(WWT_CAPTION, Colours::DarkBlue), SetStringTip(STR_SPACE_SURVEY_CAPTION),
		NWidget(WWT_STICKYBOX, Colours::DarkBlue),
	EndContainer(),
	NWidget(WWT_PANEL, Colours::DarkBlue, 0), SetMinimalSize(460, 270), SetResize(1, 1), EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 1), SetStringTip(STR_SPACE_MINE_SELECTED), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 2), SetStringTip(STR_SPACE_RECALL), SetFill(1, 0),
	EndContainer(),
};
static WindowDesc _space_survey_desc(WindowPosition::Automatic, {}, 0, 0, WindowClass::SpaceSurvey, WindowClass::None, {}, _space_survey_widgets);

struct SpaceSurveyWindow : Window {
	mutable std::vector<std::pair<VehicleID, Rect>> probe_rows; ///< Derived picking rectangles, never simulation state.
	const IntervalTimer<TimerWindow> refresh = {std::chrono::milliseconds(250), [this]([[maybe_unused]] auto elapsed) { this->SetDirty(); }};
	SpaceSurveyWindow() : Window(_space_survey_desc) { this->InitNested(0); }
	void OnPaint() override
	{
		const auto *probe = SpaceVehicle::GetIfValid(_inspected_probe);
		const bool owned = probe != nullptr && probe->owner == _local_company;
		const auto &bodies = GetAsteroidField().bodies;
		const auto *body = _inspected_asteroid && _inspected_asteroid->base() < bodies.size() ? &bodies[_inspected_asteroid->base()] : nullptr;
		const bool can_mine = owned && body != nullptr && body->remaining_reserve != 0 && probe->cargo.TotalCount() < probe->cargo_cap &&
			(probe->cargo.TotalCount() == 0 || probe->cargo_type == GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(body->resource_class)].cargo));
		this->SetWidgetDisabledState(1, !can_mine);
		this->SetWidgetDisabledState(2, !owned);
		this->DrawWidgets();
	}
	void OnClick([[maybe_unused]] Point point, WidgetID widget, [[maybe_unused]] int count) override
	{
		if (widget == 0) for (const auto &[id, rect] : this->probe_rows) if (point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom) {
			_inspected_probe = id; this->SetDirty(); return;
		}
		const auto *probe = SpaceVehicle::GetIfValid(_inspected_probe);
		if (probe == nullptr || probe->owner != _local_company) return;
		if (widget == 1 && _inspected_asteroid) Command<Commands::SetSpaceMission>::Post(STR_SPACE_MISSION_FAILED, probe->index, _inspected_asteroid->base());
		if (widget == 2) Command<Commands::SetSpaceMission>::Post(STR_SPACE_MISSION_FAILED, probe->index, UINT32_MAX);
	}
	void DrawWidget(const Rect &rect, [[maybe_unused]] WidgetID widget) const override
	{
		if (widget != 0) return;
		this->probe_rows.clear();
		const auto &field = GetAsteroidField();
		const int line = GetCharacterHeight(FontSize::Normal) + 3;
		int y = rect.top + 8;
		auto draw = [&](const std::string &text, TextColour colour = TextColour::White) { DrawString(rect.left + 8, rect.right - 8, y, text, colour); y += line; };
		uint native_count = 0; for ([[maybe_unused]] const auto *probe : SpaceVehicle::Iterate()) ++native_count;
		draw(GetString(native_count == 0 ? STR_SPACE_FIELD_COUNT : STR_SPACE_NATIVE_FIELD_COUNT, field.bodies.size(), native_count == 0 ? field.surveys.size() : native_count));
		draw(GetString(native_count == 0 ? STR_SPACE_SURVEY_BOUNDARY : STR_SPACE_MINING_BOUNDARY));
		y += 6;
		static constexpr StringID classes[] = {STR_SPACE_METALLIC, STR_SPACE_CARBONACEOUS, STR_SPACE_RICH};
		static constexpr StringID phases[] = {STR_SPACE_APPROACH, STR_SPACE_ATTACHED, STR_SPACE_RETURN, STR_SPACE_WAITING};
		if (_inspected_asteroid && _inspected_asteroid->base() < field.bodies.size()) {
			const auto &body = field.bodies[_inspected_asteroid->base()];
			draw(GetString(STR_SPACE_BODY_RESERVE, body.id.base(), classes[to_underlying(body.resource_class)], body.remaining_reserve, body.initial_reserve));
			draw(GetString(STR_SPACE_BODY_LOCATION, body.motion.position.x / SPACE_Q16, body.motion.position.y / SPACE_Q16));
			y += 6;
		}
		for (const auto &survey : field.surveys) {
			if (y + line * 2 > rect.bottom) break;
			draw(GetString(STR_SPACE_SURVEY_STATUS, survey.id.base() + 1, phases[to_underlying(survey.phase)], survey.target.base(), survey.completed_surveys));
			y += 4;
		}
		static constexpr StringID mining_phases[] = {STR_SPACE_IDLE, STR_SPACE_APPROACH, STR_SPACE_MINING, STR_SPACE_RETURN, STR_SPACE_HOLDING};
		for (const auto *probe : SpaceVehicle::Iterate()) {
			if (y + line * 2 > rect.bottom) break;
			const int row_top = y;
			draw(GetString(STR_SPACE_NATIVE_PROBE_STATUS, probe->index, mining_phases[to_underlying(probe->mission.phase)], probe->cargo.TotalCount(), probe->cargo_cap, CargoSpec::Get(probe->cargo_type)->name), probe->index == _inspected_probe ? TextColour::Yellow : TextColour::White);
			if (probe->index == _inspected_probe && probe->mission.target != INVALID_ORBITAL_BODY) draw(GetString(STR_SPACE_NATIVE_SELECTED, probe->mission.target.base(), probe->mission.work_ticks));
			this->probe_rows.push_back({probe->index, {rect.left, row_top, rect.right, y - 1}});
			y += 4;
		}
		draw(GetString(STR_SPACE_SURVEY_HINT));
	}
};

void ShowSpaceSurveyWindow()
{
	if (SpaceVehicle::GetIfValid(_inspected_probe) == nullptr) for (const auto *probe : SpaceVehicle::Iterate()) { _inspected_probe = probe->index; break; }
	const auto &bodies = GetAsteroidField().bodies;
	if (!bodies.empty() && (!_inspected_asteroid || _inspected_asteroid->base() >= bodies.size())) _inspected_asteroid = bodies.front().id;
	if (BringWindowToFrontById(WindowClass::SpaceSurvey, 0) == nullptr) new SpaceSurveyWindow();
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
	for (const auto *probe : SpaceVehicle::Iterate()) if (hit(probe->position, 12)) {
		_inspected_probe = probe->index;
		if (probe->mission.target != INVALID_ORBITAL_BODY) _inspected_asteroid = probe->mission.target;
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
