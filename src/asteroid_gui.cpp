/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file asteroid_gui.cpp Read-only Space viewport rendering/picking and live survey inspector.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG; final natural-body and original passive-pod sprites.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group20; generation-bound synchronized mission admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md GroupP1
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11; native approach waits.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC; retain the actual inspected incarnation across paid replacement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CD; source-linked heavy mining and original service bills.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group13; shared mining recall admission and cargo return region.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14; company-gated composition/motion and original construction/descent progress.
 */
#include "stdafx.h"
#include "asteroid.h"
#include "asteroid_inspection.h"
#include "space_vehicle.h"
#include "space_sprite.h"
#include "space_vehicle_cmd.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "space_structure.h"
#include "orbital_service.h"
#include "space_mining_tuning.h"
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
	// Retain visible unfinished/paid footprints; a cancelled empty blueprint has no remaining physical inventory to disclose.
	for (const auto *site : SpaceStructure::Iterate()) {
		if (site->state.phase == SpaceStructurePhase::Cancelled && site->job.inputs[0].TotalCount() == 0 && site->job.inputs[1].TotalCount() == 0) continue;
		const auto &d = *GetSpaceConstructionDefinition(site->state.kind);
		// Native wire footprint is a blueprint overlay, not final module art.
		for (uint y = 0; y <= d.height; ++y) for (uint x = 0; x < (y == d.height ? 1U : d.width); ++x) {
			const Point p = SpaceWorldPoint({static_cast<int32_t>(site->state.x + x) * SPACE_Q16, static_cast<int32_t>(site->state.y + y) * SPACE_Q16});
			AddSortableSpriteToDraw(SPR_SELECT_TILE, PAL_NONE, p.x - TILE_SIZE / 2, p.y - TILE_SIZE / 2, 0, {{0, 0, 0}, {16, 16, 1}, {0, 0, 0}});
		}
	}
	// Body appearance reads actual saved class/size and this observer's authorized knowledge. Visible-body lookup remains the existing local spatial query, never an all-body scan.
	for (const auto id : VisibleBodies(dpi)) {
		const auto &body = field.bodies[id.base()];
		const Point point = SpaceWorldPoint(body.motion.position);
		const auto pose = GetSpaceAsteroidArtPose(id, _local_company);
		if (!pose) continue;
		const auto sprite = ResolveSpaceSprite(*pose);
		if (!sprite) continue;
		const auto diameter = static_cast<uint8_t>((TILE_SIZE / 2) << to_underlying(body.size));
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
/** @param v Actual inspected hull. @return Its current typed body target, then original legacy mission target, or absence. */
static std::optional<OrbitalBodyID> InspectedProbeTarget(const SpaceVehicle &v)
{
	const auto &q = v.space_orders;
	// Only an existing body-targeting order may redirect the inspector; module and center IDs belong to different owners.
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
		NWidget(WWT_PANEL, Colours::DarkBlue, 0), SetMinimalSize(760, 480), SetResize(1, 1), SetScrollbar(9), EndContainer(),
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
/** Native fleet/body inspector with read-only disclosures and synchronized command controls. */
struct SpaceSurveyWindow : Window {
	mutable std::vector<std::pair<SpaceCraftRef, Rect>> probe_rows;
	mutable std::vector<std::pair<uint8_t, Rect>> order_rows;
	uint8_t selected_order = UINT8_MAX;
	Scrollbar *scrollbar = nullptr;
	/** @param elapsed Presentation refresh intervals; never simulation work or service progress. */
	const IntervalTimer<TimerWindow> refresh = {std::chrono::milliseconds(250), [this]([[maybe_unused]] auto elapsed) { this->SetDirty(); }};
	/** Initialize native controls; selection resolves actual generations on every redraw. */
	SpaceSurveyWindow() : Window(_space_survey_desc) { this->InitNested(0); this->scrollbar = this->GetScrollbar(9); }
	/** Refresh controls from actual ownership and pure command admission without issuing gameplay mutations. */
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
		this->DrawWidgets();
	}
	/** @param point Window-local hit position. @param widget Native action identity. @param count Framework click count, unused for these single actions. */
	void OnClick(Point point, WidgetID widget, [[maybe_unused]] int count) override
	{
		if (widget == 0) {
			// Hit rows retain original craft generations; only points inside their current drawn rectangles may change selection.
			for (const auto &[ref, rect] : this->probe_rows) if (point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom) {
				_inspected_probe = ref; this->selected_order = UINT8_MAX; this->SetDirty(); return;
			}
			// Order hit rectangles select existing rows; clicking unused panel space cannot invent a queue entry.
			for (const auto &[index, rect] : this->order_rows) if (point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom) {
				this->selected_order = index; this->SetDirty(); return;
			}
		}
		const auto *probe = ResolveSpaceCraft(_inspected_probe);
		// Only the actual locally owned deployed hull may receive commands; stale selections cannot target reused IDs or carried payloads.
		if (probe == nullptr || probe->owner != _local_company || probe->construction.placement != SpacePlacement::Orbital) return;
		const auto ref = _inspected_probe;
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
		this->SetDirty();
	}
	/** @param rect Current panel bounds. @param widget Native draw identity; only the inspector panel owns these rows. */
	void DrawWidget(const Rect &rect, WidgetID widget) const override
	{
		if (widget != 0) return;
		this->probe_rows.clear(); this->order_rows.clear();
		const auto &field = GetAsteroidField(); const auto *selected = ResolveSpaceCraft(_inspected_probe);
		const int line = GetCharacterHeight(FontSize::Normal) + 3; int y = rect.top + 8;
		/** @param text Already translated native disclosure. @param colour Selection/readability colour. Advance only the presentation cursor. */
		auto draw = [&](const std::string &text, TextColour colour = TextColour::White) { DrawString(rect.left + 8, rect.right - 8, y, text, colour); y += line; };
		uint native_count = 0; for (const auto *probe : SpaceVehicle::Iterate()) if (probe->construction.placement == SpacePlacement::Orbital) ++native_count;
		draw(GetString(native_count == 0 ? STR_SPACE_FIELD_COUNT : STR_SPACE_NATIVE_FIELD_COUNT, field.bodies.size(), native_count == 0 ? field.surveys.size() : native_count));
		draw(GetString(native_count == 0 ? STR_SPACE_SURVEY_BOUNDARY : STR_SPACE_MINING_BOUNDARY));
		static constexpr StringID classes[] = {STR_SPACE_METALLIC, STR_SPACE_CARBONACEOUS, STR_SPACE_RICH};
		// Only a body still present in the actual field may reveal source-authorized survey knowledge.
		if (_inspected_asteroid && _inspected_asteroid->base() < field.bodies.size()) {
			const auto &body = field.bodies[_inspected_asteroid->base()]; const auto inspection = *GetAsteroidInspectionDisclosure(_local_company, body.id); const auto &quantity = inspection.quantities;
			if (quantity) draw(GetString(STR_SPACE_BODY_RESERVE, body.id.base(), classes[to_underlying(body.resource_class)], quantity->remaining, quantity->initial));
			else draw(GetString(STR_SPACE_BODY_UNKNOWN, body.id.base(), classes[to_underlying(body.resource_class)]));
			draw(GetString(STR_SPACE_BODY_LOCATION, body.motion.position.x / SPACE_Q16, body.motion.position.y / SPACE_Q16));
			draw(GetString(STR_SPACE_BODY_COMPOSITION, CargoSpec::Get(inspection.cargo)->name));
			// Explicit signs preserve reflected/sub-resolution direction; the decimal formatter receives nonnegative magnitudes so negative fractions cannot place a sign after the decimal point.
			draw(GetString(STR_SPACE_BODY_TRAJECTORY, std::string{inspection.trajectory.x_negative ? "-" : "+"}, std::abs(inspection.trajectory.x_milli), 3,
				std::string{inspection.trajectory.y_negative ? "-" : "+"}, std::abs(inspection.trajectory.y_milli), 3, inspection.trajectory.speed_milli, 3));
		}
		// Queue/progress disclosure requires the actual selected incarnation and an existing action; a stale cursor never creates work.
		if (selected != nullptr && !selected->space_orders.entries.empty()) {
			const auto &q = selected->space_orders;
			draw(GetString(STR_SPACE_ORDER_PROGRESS, std::min<size_t>(q.cursor + 1, q.entries.size()), q.entries.size(), order_blocks[to_underlying(q.blocked)]));
			// Survey work belongs only to the current typed Survey action, independently of mining or service work.
			if (q.cursor < q.entries.size() && q.entries[q.cursor].action == SpaceOrderAction::Survey) draw(GetString(STR_SPACE_SCAN_PROGRESS, q.work_ticks));
			// A current Construct instruction identifies the exact original blueprint job. Its retained delivered inputs and earned native work supply this progress, never the queue's presentation timer.
			if (q.cursor < q.entries.size() && q.entries[q.cursor].action == SpaceOrderAction::Construct) {
				const auto &target = q.entries[q.cursor].target;
				if (const auto *site = ResolveSpaceStructure({SpaceStructureID{static_cast<uint16_t>(target.id)}, target.generation})) {
					const auto &recipe = *GetSpaceConstructionDefinition(site->state.kind);
					draw(GetString(STR_SPACE_CONSTRUCTION_STOCK, site->job.inputs[0].TotalCount(), recipe.inputs[0].quantity, site->job.inputs[1].TotalCount(), recipe.inputs[1].quantity, site->job.work_ticks, recipe.work_ticks));
					draw(GetString(STR_SPACE_CONSTRUCTION_CONSUMED, site->job.consumed[0], site->job.consumed[1]));
				}
			}
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
		draw(GetString(STR_SPACE_SURVEY_HINT)); y += 4;
		const uint order_count = selected == nullptr ? 0 : static_cast<uint>(selected->space_orders.entries.size());
		this->scrollbar->SetCount(order_count + native_count + static_cast<uint>(field.surveys.size()));
		this->scrollbar->SetCapacity(std::max(1, (rect.bottom - y + 1) / line));
		uint item = 0; const int first = this->scrollbar->GetPosition(), last = first + this->scrollbar->GetCapacity();
		/** @return Whether the next original row lies inside this scrollbar page; filtering changes no native owner or queue. */
		auto visible = [&] { const int ordinal = static_cast<int>(item++); return ordinal >= first && ordinal < last; };
		// Draw existing orders and hulls only within the page while retaining original hit targets for command selection.
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
			// Carried/Surface hulls do not occupy this orbital list, and clipped rows cannot create clickable selections.
			if (probe->construction.placement != SpacePlacement::Orbital || !visible()) continue;
			const int top = y;
			const auto model = GetSpaceVehicleModel(*probe); const auto &orders = probe->space_orders; StringID status = mining_phases[to_underlying(probe->mission.phase)];
			// Survey/construction/freight roles keep the mining controller in Holding during real typed work. Their current action or actual wait supplies mission status instead of falsely reporting Idle.
			if (GetSpaceMiningTuning(model) == nullptr && orders.cursor < orders.entries.size()) {
				status = orders.blocked == SpaceOrderBlock::None ? order_actions[to_underlying(orders.entries[orders.cursor].action)] : order_blocks[to_underlying(orders.blocked)];
			}
			if (model == 19) draw(GetString(STR_SPACE_NATIVE_SURVEY_STATUS, probe->index, status), GetSpaceCraftReference(*probe) == _inspected_probe ? TextColour::Yellow : TextColour::White);
			else draw(GetString(STR_SPACE_NATIVE_PROBE_STATUS, probe->index, status, probe->cargo.TotalCount(), probe->cargo_cap, CargoSpec::Get(probe->cargo_type)->name), GetSpaceCraftReference(*probe) == _inspected_probe ? TextColour::Yellow : TextColour::White);
			this->probe_rows.push_back({GetSpaceCraftReference(*probe), {rect.left, top, rect.right, y - 1}});
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
	const auto &bodies = GetAsteroidField().bodies;
	// Body selection falls back only when a real body exists and the old selection no longer resolves.
	if (!bodies.empty() && (!_inspected_asteroid || _inspected_asteroid->base() >= bodies.size())) _inspected_asteroid = bodies.front().id;
	if (BringWindowToFrontById(WindowClass::SpaceSurvey, 0) == nullptr) new SpaceSurveyWindow();
}

/** @param craft Original desired hull incarnation; only an actual deployed hull can become the inspector selection. */
void ShowSpaceSurveyWindow(SpaceCraftRef craft)
{
	// A stale or Surface incarnation cannot replace an existing valid orbital selection.
	if (const auto *v = ResolveSpaceCraft(craft); v != nullptr && v->construction.placement == SpacePlacement::Orbital) _inspected_probe = craft;
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
	for (const auto id : VisibleBodies(region)) if (hit(GetAsteroidField().bodies[id.base()].motion.position, 30)) {
		_inspected_asteroid = id; ShowSpaceSurveyWindow(); return true;
	}
	return false;
}
