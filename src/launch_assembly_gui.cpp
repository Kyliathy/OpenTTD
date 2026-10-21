/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_assembly_gui.cpp Native material-paid Surface workshop and explicit orbital freight controls.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups8B/11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62JD; actual FIFO wait disclosure.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC; explicit paid Earth replacement cancellation.
 */
#include "stdafx.h"
#include "launch_operations.h"
#include "launch_assembly_gui.h"
#include "launch_center_cmd.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "space_transition.h"
#include "space_structure.h"
#include "space_structure_cmd.h"
#include "multilayer_space_content.h"
#include "station_base.h"
#include "company_func.h"
#include "command_func.h"
#include "core/backup_type.hpp"
#include "window_gui.h"
#include "window_func.h"
#include "gfx_func.h"
#include "strings_func.h"
#include "timer/timer.h"
#include "timer/timer_window.h"
#include "table/strings.h"
#include "safeguards.h"

static constexpr uint16_t models[] = {14, 15, 19, 20};
static constexpr StringID names[] = {STR_LAUNCH_ASSEMBLY_ROCKET, STR_LAUNCH_ASSEMBLY_MINER, STR_LAUNCH_ASSEMBLY_SURVEY, STR_LAUNCH_ASSEMBLY_BUILDER};
static constexpr StringID flight_names[] = {STR_LAUNCH_FLIGHT_READY, STR_LAUNCH_FLIGHT_PREPARING, STR_LAUNCH_FLIGHT_ASCENT, STR_LAUNCH_FLIGHT_DELIVERY, STR_LAUNCH_FLIGHT_RETURN, STR_LAUNCH_FLIGHT_TURNAROUND, STR_LAUNCH_FLIGHT_WAITING};
static constexpr StringID flight_blocks[] = {STR_SPACE_ORDER_ACTIVE, STR_SPACE_ORDER_TARGET_LOST, STR_LAUNCH_RESUPPLY_HOME_LOST, STR_LAUNCH_RESUPPLY_PAD_BUSY, STR_LAUNCH_RESUPPLY_BUSY, STR_LAUNCH_RESUPPLY_FULL, STR_LAUNCH_RESUPPLY_APPROACH};
static constexpr std::initializer_list<NWidgetPart> widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, Colours::DarkBlue),
		NWidget(WWT_CAPTION, Colours::DarkBlue), SetStringTip(STR_LAUNCH_ASSEMBLY_CAPTION),
		NWidget(WWT_STICKYBOX, Colours::DarkBlue),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, Colours::DarkBlue, 0), SetMinimalSize(660, 430), SetScrollbar(27), EndContainer(),
		NWidget(NWID_VSCROLLBAR, Colours::DarkBlue, 27),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NWidContainerFlag::EqualSize),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 1), SetStringTip(STR_LAUNCH_ASSEMBLY_ROCKET), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 2), SetStringTip(STR_LAUNCH_ASSEMBLY_MINER), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 3), SetStringTip(STR_LAUNCH_ASSEMBLY_SURVEY), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 4), SetStringTip(STR_LAUNCH_ASSEMBLY_BUILDER), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NWidContainerFlag::EqualSize),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 5), SetStringTip(STR_LAUNCH_ASSEMBLY_PAUSE), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 6), SetStringTip(STR_LAUNCH_ASSEMBLY_RECOVER), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 32), SetStringTip(STR_SPACE_REPLACEMENT_CANCEL, STR_SPACE_REPLACEMENT_CANCEL_HINT), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NWidContainerFlag::EqualSize),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 7), SetStringTip(STR_LAUNCH_LOGISTICS_BUILD_POD), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 8), SetStringTip(STR_LAUNCH_LOGISTICS_LOAD_POD), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 9), SetStringTip(STR_LAUNCH_LOGISTICS_UNLOAD_POD), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 10), SetStringTip(STR_LAUNCH_LOGISTICS_SERVICE), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NWidContainerFlag::EqualSize),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 11), SetStringTip(STR_LAUNCH_LOGISTICS_FUEL), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 12), SetStringTip(STR_LAUNCH_LOGISTICS_METAL), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 13), SetStringTip(STR_LAUNCH_LOGISTICS_ELECTRONICS), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NWidContainerFlag::EqualSize),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 14), SetStringTip(STR_LAUNCH_LOGISTICS_PACK_PROBE), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 15), SetStringTip(STR_LAUNCH_LOGISTICS_UNPACK_PROBE), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 16), SetStringTip(STR_LAUNCH_LOGISTICS_STEEL), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 17), SetStringTip(STR_LAUNCH_LOGISTICS_GOODS), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 18), SetStringTip(STR_LAUNCH_LOGISTICS_UNLOAD_FREIGHT), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NWidContainerFlag::EqualSize),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 19), SetStringTip(STR_LAUNCH_FLIGHT_START), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 20), SetStringTip(STR_LAUNCH_FLIGHT_CANCEL), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 21), SetStringTip(STR_LAUNCH_FLIGHT_RECOVER), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 22), SetStringTip(STR_LAUNCH_DESCENT_RELEASE), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 23), SetStringTip(STR_LAUNCH_DESCENT_RETARGET), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NWidContainerFlag::EqualSize),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 24), SetStringTip(STR_LAUNCH_LOGISTICS_UNLOAD_FUEL), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 25), SetStringTip(STR_LAUNCH_LOGISTICS_UNLOAD_METAL), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 26), SetStringTip(STR_LAUNCH_LOGISTICS_UNLOAD_ELECTRONICS), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NWidContainerFlag::EqualSize),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 28), SetStringTip(STR_LAUNCH_RESUPPLY_SELECT), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 29), SetStringTip(STR_LAUNCH_RESUPPLY_START, STR_LAUNCH_RESUPPLY_HINT), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NWidContainerFlag::EqualSize),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 30), SetStringTip(STR_LAUNCH_MODULE_SELECT, STR_LAUNCH_MODULE_HINT), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 31), SetStringTip(STR_LAUNCH_MODULE_START, STR_LAUNCH_MODULE_HINT), SetFill(1, 0),
	EndContainer(),
};
static WindowDesc desc(WindowPosition::Automatic, {}, 0, 0, WindowClass::LaunchAssembly, WindowClass::StationView, {}, widgets);
/** Explain actual FIFO storage without creating another pad or admission rule.
 * @param center Existing native facility displayed by this inventory window.
 * @param craft Actual paid hull whose current source row is being drawn.
 * @return Whether its real queue entry waits behind another entry or an occupied pad.
 */
static bool IsLaunchQueueWaiting(const Station &center, const SpaceVehicle &craft)
{
    // Only an owned stored rocket in this exact center's native FIFO can wait
    // for its launch pad. Other stored roles and historical homes keep their
    // existing custody labels instead of acquiring imaginary launch authority.
    if (!center.launch_operations || GetSpaceVehicleModel(craft) != 14 || craft.owner != center.owner ||
        craft.construction.home != GetLaunchCenterReference(center) || craft.construction.placement != SpacePlacement::SurfaceStored) return false;
    const auto &operations = *center.launch_operations;
    const auto position = std::ranges::find(operations.pad_queue, GetSpaceCraftReference(craft));
    return position != operations.pad_queue.end() && (position != operations.pad_queue.begin() || ResolveSpaceCraft(operations.pad_active) != nullptr);
}

/** Workshop view posts synchronized commands against actual paid hulls, packets and exact facility incarnations. */
struct LaunchAssemblyWindow : Window {
	LaunchCenterRef center;
	SpaceCraftRef selected;
	SpaceCraftRef carrier;
	SpaceCraftRef receiver;
	SpaceStructureRef module_destination; ///< Explicit UI choice; stale incarnations never silently retarget a flight.
	SpacePodRef selected_pod;
	Scrollbar *scrollbar = nullptr;
	mutable std::vector<std::pair<SpaceCraftRef, Rect>> rows;
	mutable std::vector<std::pair<SpacePodRef, Rect>> pod_rows;
	/** @param elapsed Number of elapsed UI intervals; refresh quotes without advancing any simulation owner. */
	const IntervalTimer<TimerWindow> refresh = {std::chrono::milliseconds(250), [this]([[maybe_unused]] auto elapsed) { this->SetDirty(); }};
	/** @param description Native workshop layout. @param number Station window identity. @param center Exact paid Surface workshop incarnation. */
	LaunchAssemblyWindow(WindowDesc &description, WindowNumber number, LaunchCenterRef center) : Window(description), center(center) { this->InitNested(number); this->scrollbar = this->GetScrollbar(27); }
	/** @return Current owned operational freight modules in native pool order; these transient references own no inventory or berth. */
	std::vector<SpaceStructureRef> GetModuleDestinations() const
	{
		std::vector<SpaceStructureRef> result;
		const auto *station = ResolveLaunchCenter(this->center);
		if (station == nullptr) return result;
		// Offer only real facilities of the workshop's company. Blueprints, inactive frames and lost dependencies cannot promise orbital storage.
		for (const auto *site : SpaceStructure::Iterate()) {
			if (site->state.owner == station->owner && IsOrbitalModuleOperational(*site) && GetOrbitalModuleStockLimit(site->state.kind) != 0) result.push_back(GetSpaceStructureReference(*site));
		}
		return result;
	}
	/** Refresh pure command quotes under the local company; paint never selects a replacement destination or mutates cargo. */
	void OnPaint() override
	{
		AutoRestoreBackup actor(_current_company, _local_company);
		const auto *s = ResolveLaunchCenter(this->center); if (s == nullptr) { this->Close(); return; }
		const auto *v = ResolveSpaceCraft(this->selected);
		const auto *pod = ResolveSpacePod(this->selected_pod);
		// Cancellation is available only for the exact selected unfinished paid Earth replacement body; painting cannot detach or refund a historical source.
		this->SetWidgetDisabledState(32, v == nullptr || CmdCancelEarthSpaceReplacement({}, selected.vehicle, selected.generation).Failed());
		// Quote each canonical recipe independently: another company or a failed native material/slot quote must never enable paid commissioning.
		for (uint i = 0; i < std::size(models); ++i) this->SetWidgetDisabledState(i + 1, s->owner != _local_company || CmdAssembleLaunchCraft({}, center.station, center.generation, models[i]).Failed());
		this->SetWidgetDisabledState(5, pod != nullptr ? CmdPauseLaunchPod({}, selected_pod.pod, selected_pod.generation, !pod->state.paused).Failed() :
			v == nullptr || v->owner != _local_company || v->construction.home != center || v->construction.placement != SpacePlacement::Assembling);
		this->SetWidgetDisabledState(6, pod != nullptr ? CmdRecoverLaunchPod({}, center.station, center.generation, selected_pod.pod, selected_pod.generation).Failed() :
			v == nullptr || v->owner != _local_company || CmdRecoverLaunchCraft({}, center.station, center.generation, selected.vehicle, selected.generation).Failed());
		this->SetWidgetDisabledState(7, CmdAssembleLaunchPod({}, center.station, center.generation).Failed());
		// Pod packing/unpacking is enabled only when both original physical owners survive and the synchronized command admits their whole move.
		for (uint widget : {8, 9}) this->SetWidgetDisabledState(widget, v == nullptr || pod == nullptr ||
			CmdMoveLaunchPod({}, center.station, center.generation, selected.vehicle, selected.generation, selected_pod.pod, selected_pod.generation, widget == 9).Failed());
		this->SetWidgetDisabledState(10, v == nullptr || CmdServiceLaunchProbe({}, selected.vehicle, selected.generation, v->manifest && v->manifest->service_ticks != 0 && !v->manifest->service_paused).Failed());
		// Each real fuel/part compartment keeps its own native capacity and actual Surface stock quote.
		for (uint widget : {11, 12, 13}) this->SetWidgetDisabledState(widget, v == nullptr ||
			CmdMoveSpaceSupply({}, center.station, center.generation, selected.vehicle, selected.generation, widget - 11, widget == 11 ? 15 : 1, false).Failed());
		// Probe packing preserves two existing hull identities; UI selection alone cannot commission or deploy either hull.
		for (uint widget : {14, 15}) this->SetWidgetDisabledState(widget, v == nullptr ||
			CmdCarryLaunchProbe({}, carrier.vehicle, carrier.generation, selected.vehicle, selected.generation, widget == 15).Failed());
		// Freight controls resolve physical labels and current whole holds before quoting a native packet transfer.
		for (uint widget : {16, 17, 18}) {
			const auto cargo = widget == 18 && v != nullptr ? v->cargo_type : GetCargoTypeByLabel(widget == 16 ? CargoLabel{"STEL"} : CargoLabel{"GOOD"});
			this->SetWidgetDisabledState(widget, v == nullptr || CmdMoveLaunchFreight({}, center.station, center.generation, selected.vehicle, selected.generation, cargo, widget == 18 && v != nullptr ? v->cargo.TotalCount() : 1, widget == 18).Failed());
		}
		this->SetWidgetDisabledState(19, v == nullptr || CmdBeginLaunchFlight({}, selected.vehicle, selected.generation, SpaceOrderTargetKind::LaunchCenter, center.station.base(), center.generation).Failed());
		this->SetWidgetDisabledState(20, v == nullptr || CmdCancelLaunchFlight({}, selected.vehicle, selected.generation).Failed());
		this->SetWidgetDisabledState(21, v == nullptr || CmdDivertLaunchRecovery({}, selected.vehicle, selected.generation, center.station, center.generation).Failed());
		this->SetWidgetDisabledState(22, v == nullptr || pod == nullptr || CmdReleaseOrbitalCargo({}, selected.vehicle, selected.generation, center.station, center.generation, selected_pod.pod, selected_pod.generation, static_cast<uint16_t>(v->cargo.TotalCount())).Failed());
		this->SetWidgetDisabledState(23, pod == nullptr || CmdRetargetSpaceDescent({}, selected_pod.pod, selected_pod.generation, center.station, center.generation).Failed());
		// Whole compartment unloading retains actual packets when the selected hull/manifest or destination command cannot accept them.
		for (uint widget : {24, 25, 26}) this->SetWidgetDisabledState(widget, v == nullptr || !v->manifest ||
			CmdMoveSpaceSupply({}, center.station, center.generation, selected.vehicle, selected.generation, widget - 24, static_cast<uint16_t>(v->manifest->supplies[widget - 24].TotalCount()), true).Failed());
		this->SetWidgetDisabledState(28, v == nullptr || v->owner != _local_company || v->construction.placement != SpacePlacement::Orbital);
		this->SetWidgetDisabledState(29, v == nullptr || CmdBeginLaunchFlight({}, selected.vehicle, selected.generation, SpaceOrderTargetKind::Craft, receiver.vehicle.base(), receiver.generation).Failed());
		this->SetWidgetDisabledState(30, s->owner != _local_company || this->GetModuleDestinations().empty());
		this->SetWidgetDisabledState(31, v == nullptr || CmdBeginLaunchFlight({}, selected.vehicle, selected.generation, SpaceOrderTargetKind::Structure, module_destination.structure.base(), module_destination.generation).Failed());
		this->DrawWidgets();
	}
	/** @param r Native panel bounds. @param widget Requested widget; only the inventory panel draws custom rows. */
	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		if (widget != 0) return;
		const auto *s = ResolveLaunchCenter(center); if (s == nullptr) return;
		int y = r.top + 8; const int line = GetCharacterHeight(FontSize::Normal) + 3;
		/** @param text Translated physical status. @param colour Native selection/status colour. */
		auto draw = [&](const std::string &text, TextColour colour = TextColour::White) { DrawString(r.left + 8, r.right - 8, y, text, colour); y += line; };
		// Publish the canonical paid recipes beside current inventory so purchasing decisions use the actual construction definitions.
		for (uint i = 0; i < std::size(models); ++i) {
			const auto &d = *GetSpaceVehicleConstruction(models[i]);
			draw(GetString(STR_LAUNCH_ASSEMBLY_RECIPE, names[i], d.capital, d.inputs[0].quantity, d.inputs[1].quantity));
		}
		const auto &pod_definition = *GetSpaceConstructionDefinition(SpaceBuildKind::Pod);
		draw(GetString(STR_LAUNCH_ASSEMBLY_RECIPE, STR_LAUNCH_LOGISTICS_POD, pod_definition.capital, pod_definition.inputs[0].quantity, pod_definition.inputs[1].quantity));
		draw(GetString(STR_LAUNCH_ASSEMBLY_STATUS));
		if (const auto *v = ResolveSpaceCraft(selected); v != nullptr) {
			draw(GetString(STR_LAUNCH_LOGISTICS_STOCK, v->index.base(), v->manifest ? v->manifest->supplies[0].TotalCount() : 0,
				v->manifest ? v->manifest->supplies[1].TotalCount() : 0, v->manifest ? v->manifest->supplies[2].TotalCount() : 0,
				v->manifest ? v->manifest->pods.size() : 0, v->manifest ? v->manifest->service_ticks : 0));
			draw(GetString(STR_LAUNCH_LOGISTICS_FREIGHT, v->cargo_type, v->cargo.TotalCount()));
		}
        // A live passive descent owns its actual packet list until atomic
        // recovery. Show that original cargo while this exact shell is selected;
        // a stored empty shell or a spent/reused ID cannot invent freight.
        if (const auto *pod = ResolveSpacePod(selected_pod); pod != nullptr && pod->descent) {
            draw(GetString(STR_LAUNCH_LOGISTICS_FREIGHT, pod->descent->cargo, pod->descent->packets.AvailableCount()));
        }
		if (ResolveSpaceCraft(carrier) != nullptr) draw(GetString(STR_LAUNCH_LOGISTICS_CARRIER, carrier.vehicle.base()));
		if (ResolveSpaceCraft(receiver) != nullptr) draw(GetString(STR_LAUNCH_RESUPPLY_RECEIVER, receiver.vehicle));
		else if (receiver != SpaceCraftRef{}) draw(GetString(STR_SPACE_ORDER_TARGET_LOST));
		if (const auto *site = ResolveSpaceStructure(module_destination); site != nullptr) {
			const auto kind = site->state.kind == SpaceBuildKind::Core ? STR_SPACE_MODULE_CORE : site->state.kind == SpaceBuildKind::Dock ? STR_SPACE_MODULE_DOCK : STR_SPACE_MODULE_STORAGE;
			draw(GetString(STR_LAUNCH_MODULE_DESTINATION, kind, site->state.x, site->state.y));
			if (!IsOrbitalModuleOperational(*site)) draw(GetString(STR_LAUNCH_RESUPPLY_BUSY));
		} else if (module_destination != SpaceStructureRef{}) draw(GetString(STR_SPACE_ORDER_TARGET_LOST));
		// Only an existing active flight supplies a phase-specific blocked reason; a stale selection grants no operation or status owner.
		if (const auto *v = ResolveSpaceCraft(selected); v != nullptr && IsSpaceFlightActive(*v)) draw(GetString(flight_blocks[to_underlying(v->transition->blocked)]));
		rows.clear(); const auto area = GetLaunchCenterFootprint(s->launch_center->tile, s->launch_center->rotation);
		/** @param v Actual native hull. @return Whether its home or physical owned recovery/drop geometry belongs in this workshop. */
		auto show_craft = [&](const SpaceVehicle &v) { return v.construction.home == center || (v.owner == s->owner &&
			((IsSpaceCraftAwaitingSurfaceRecovery(v) && area->Contains(v.construction.surface_tile)) ||
			(v.construction.placement == SpacePlacement::Orbital && IsInsideLaunchCenterDropZone(v.position, *GetLaunchCenterPlaneReference(s->launch_center->tile))) ||
			(IsSpaceFlightActive(v) && v.construction.home == LaunchCenterRef{} && IsInsideLaunchCenterDropZone(v.position, *GetLaunchCenterPlaneReference(s->launch_center->tile))))); };
		/** @param pod Actual paid shell. @return Whether its owned custody or explicit descent geometry belongs in this workshop. */
		auto show_pod = [&](const SpacePod &pod) { return pod.state.owner == s->owner && (pod.state.home == center || (selected != SpaceCraftRef{} && pod.state.craft == selected) ||
			(pod.state.custody == SpacePodCustody::Unhomed && area->Contains(pod.state.surface_tile)) ||
			(pod.descent && IsInsideLaunchCenterDropZone(pod.descent->release_point, *GetLaunchCenterPlaneReference(s->launch_center->tile)))); };
		// Use the same actual custody predicate for counting and rendering, so scrolling cannot show unrelated hulls or hide a selectable owned shell.
		uint count = 0; for (const auto *v : SpaceVehicle::Iterate()) if (show_craft(*v)) ++count; for (const auto *pod : SpacePod::Iterate()) if (show_pod(*pod)) ++count;
		this->scrollbar->SetCapacity(std::max(1, (r.bottom - y + 1) / line)); this->scrollbar->SetCount(count);
		int item = 0; const auto first = this->scrollbar->GetPosition(), last = first + this->scrollbar->GetCapacity();
		// Render only the visible native rows, retaining generation-qualified hit targets for the user's explicit selection.
		for (const auto *v : SpaceVehicle::Iterate()) {
			if (!show_craft(*v)) continue; const auto ordinal = item++; if (ordinal < first || ordinal >= last) continue;
			// The finite role catalogue selects the canonical display name; unknown models acquire no workshop commissioning controls.
			uint i = 0; while (i < std::size(models) && models[i] != GetSpaceVehicleModel(*v)) ++i;
			if (i == std::size(models)) continue;
			const StringID status = v->construction.placement == SpacePlacement::Unhomed ? STR_LAUNCH_ASSEMBLY_UNHOMED :
				IsSpaceFlightActive(*v) ? flight_names[to_underlying(v->transition->phase)] : v->construction.placement == SpacePlacement::Orbital ? STR_LAUNCH_FLIGHT_ORBITAL :
				v->construction.placement == SpacePlacement::Carried ? STR_LAUNCH_LOGISTICS_CARRIED :
				v->construction.placement == SpacePlacement::SurfaceStored ? (IsLaunchQueueWaiting(*s, *v) ? STR_LAUNCH_RESUPPLY_BUSY : STR_LAUNCH_ASSEMBLY_STORED) :
                v->construction.paused ? STR_LAUNCH_ASSEMBLY_PAUSED : STR_LAUNCH_ASSEMBLY_WORKING;
			const auto ref = GetSpaceCraftReference(*v); const int top = y;
			draw(GetString(STR_LAUNCH_ASSEMBLY_HULL, names[i], v->index.base(), status, IsSpaceFlightActive(*v) ? v->transition->remaining_ticks : v->construction.remaining_ticks), ref == selected ? TextColour::Yellow : TextColour::White);
			rows.push_back({ref, {r.left, top, r.right, y - 1}});
		}
		pod_rows.clear();
		// Pods continue the same scroll order after hulls, keeping their original shell identity separate from carried cargo and probe selection.
		for (const auto *pod : SpacePod::Iterate()) {
			if (!show_pod(*pod)) continue; const auto ordinal = item++; if (ordinal < first || ordinal >= last) continue;
			const auto status = pod->descent ? pod->descent->blocked == SpaceDescentBlock::None ? STR_LAUNCH_DESCENT_WORKING : STR_LAUNCH_DESCENT_WAITING :
				pod->state.custody == SpacePodCustody::Unhomed ? STR_LAUNCH_ASSEMBLY_UNHOMED : pod->state.custody == SpacePodCustody::Craft ? STR_LAUNCH_LOGISTICS_CARRIED :
				pod->state.custody == SpacePodCustody::CenterStored ? STR_LAUNCH_ASSEMBLY_STORED : pod->state.paused ? STR_LAUNCH_ASSEMBLY_PAUSED : STR_LAUNCH_ASSEMBLY_WORKING;
			const auto ref = GetSpacePodReference(*pod); const int top = y;
			draw(GetString(STR_LAUNCH_ASSEMBLY_HULL, STR_LAUNCH_LOGISTICS_POD, pod->index.base(), status, pod->descent ? pod->descent->remaining_ticks : pod->state.remaining_ticks), ref == selected_pod ? TextColour::Yellow : TextColour::White);
			pod_rows.push_back({ref, {r.left, top, r.right, y - 1}});
		}
	}
	/** @param pt Native pointer position. @param widget Clicked control. @param count Native click count; no repeated simulation operation is implied. */
	void OnClick(Point pt, WidgetID widget, [[maybe_unused]] int count) override
	{
		if (widget == 0) {
			// Hit-test actual visible row bounds; only an explicit hull click changes the selected incarnation and designates an existing rocket as carrier.
			for (const auto &[ref, rect] : rows) if (pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom) {
				selected = ref; selected_pod = {}; if (const auto *v = ResolveSpaceCraft(ref); v != nullptr && GetSpaceVehicleModel(*v) == 14) carrier = ref; this->SetDirty(); return;
			}
			// Shell selection uses its own visible bounds and exact paid identity; it never changes packet custody merely by being clicked.
			for (const auto &[ref, rect] : pod_rows) if (pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom) { selected_pod = ref; this->SetDirty(); return; }
			return;
		}
		if (this->IsWidgetDisabled(widget)) return;
		// Only the bounded four recipe buttons may index the canonical hull catalogue; execution remains synchronized and material-paid.
		if (widget >= 1 && widget <= 4) Command<Commands::AssembleLaunchCraft>::Post(STR_LAUNCH_ASSEMBLY_FAILED, center.station, center.generation, models[widget - 1]);
		const auto *v = ResolveSpaceCraft(selected);
		const auto *pod = ResolveSpacePod(selected_pod);
		if (widget == 5) {
			if (pod != nullptr) Command<Commands::PauseLaunchPod>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected_pod.pod, selected_pod.generation, !pod->state.paused);
			else if (v != nullptr) Command<Commands::PauseLaunchAssembly>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation, !v->construction.paused);
		}
		if (widget == 6) {
			if (pod != nullptr) Command<Commands::RecoverLaunchPod>::Post(STR_LAUNCH_ASSEMBLY_FAILED, center.station, center.generation, selected_pod.pod, selected_pod.generation);
			else Command<Commands::RecoverLaunchCraft>::Post(STR_LAUNCH_ASSEMBLY_FAILED, center.station, center.generation, selected.vehicle, selected.generation);
		}
		if (widget == 7) Command<Commands::AssembleLaunchPod>::Post(STR_LAUNCH_ASSEMBLY_FAILED, center.station, center.generation);
		// Paired pod buttons share the native move while preserving explicit load/unload direction and both existing physical incarnations.
		if (widget == 8 || widget == 9) Command<Commands::MoveLaunchPod>::Post(STR_LAUNCH_ASSEMBLY_FAILED, center.station, center.generation, selected.vehicle, selected.generation, selected_pod.pod, selected_pod.generation, widget == 9);
		// Service acts only on the surviving selected hull; an active unpaused job explicitly pauses rather than consuming another set of parts.
		if (widget == 10 && v != nullptr) Command<Commands::ServiceLaunchProbe>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation, v->manifest && v->manifest->service_ticks != 0 && !v->manifest->service_paused);
		// The finite three packing buttons address existing compartments, with one round-trip fuel load or one physical service part.
		if (widget >= 11 && widget <= 13) Command<Commands::MoveSpaceSupply>::Post(STR_LAUNCH_ASSEMBLY_FAILED, center.station, center.generation, selected.vehicle, selected.generation, static_cast<uint8_t>(widget - 11), static_cast<uint16_t>(widget == 11 ? 15 : 1), false);
		// Paired probe buttons preserve the carrier/passenger relationship through one synchronized pack/unpack command.
		if (widget == 14 || widget == 15) Command<Commands::CarryLaunchProbe>::Post(STR_LAUNCH_ASSEMBLY_FAILED, carrier.vehicle, carrier.generation, selected.vehicle, selected.generation, widget == 15);
		// Freight buttons require an actual selected hold; unloaded type/quantity come from its current physical inventory, never a UI cache.
		if (widget >= 16 && widget <= 18 && v != nullptr) Command<Commands::MoveLaunchFreight>::Post(STR_LAUNCH_ASSEMBLY_FAILED, center.station, center.generation, selected.vehicle, selected.generation,
			widget == 18 ? v->cargo_type : GetCargoTypeByLabel(widget == 16 ? CargoLabel{"STEL"} : CargoLabel{"GOOD"}), static_cast<uint16_t>(widget == 18 ? v->cargo.TotalCount() : 1), widget == 18);
		if (widget == 19) Command<Commands::BeginLaunchFlight>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation, SpaceOrderTargetKind::LaunchCenter, uint32_t{center.station.base()}, center.generation);
		if (widget == 20) Command<Commands::CancelLaunchFlight>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation);
		if (widget == 21) Command<Commands::DivertLaunchRecovery>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation, center.station, center.generation);
		// Cargo descent quotes/releases the entire actual hold into the explicitly selected paid pod; crossing layers alone earns no delivery payment.
		if (widget == 22 && v != nullptr) Command<Commands::ReleaseOrbitalCargo>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation, center.station, center.generation, selected_pod.pod, selected_pod.generation, static_cast<uint16_t>(v->cargo.TotalCount()));
		if (widget == 23) Command<Commands::RetargetSpaceDescent>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected_pod.pod, selected_pod.generation, center.station, center.generation);
		// Each unloading button reads the surviving native manifest's whole compartment so stale UI amounts cannot discard accountable fuel/parts.
		if (widget >= 24 && widget <= 26 && v != nullptr && v->manifest) Command<Commands::MoveSpaceSupply>::Post(STR_LAUNCH_ASSEMBLY_FAILED, center.station, center.generation, selected.vehicle, selected.generation,
			static_cast<uint8_t>(widget - 24), static_cast<uint16_t>(v->manifest->supplies[widget - 24].TotalCount()), true);
		if (widget == 28) { receiver = selected; this->SetDirty(); }
		if (widget == 29) Command<Commands::BeginLaunchFlight>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation, SpaceOrderTargetKind::Craft, receiver.vehicle.base(), receiver.generation);
		if (widget == 30) {
			const auto choices = this->GetModuleDestinations();
			if (choices.empty()) return;
			const auto previous = std::ranges::find(choices, module_destination);
			// Only this explicit selection click may replace a stale choice; painting and command refusal retain the user's exact incarnation.
			module_destination = previous == choices.end() || std::next(previous) == choices.end() ? choices.front() : *std::next(previous);
			this->SetDirty();
		}
		if (widget == 31) Command<Commands::BeginLaunchFlight>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation, SpaceOrderTargetKind::Structure, module_destination.structure.base(), module_destination.generation);
		if (widget == 32) Command<Commands::CancelEarthSpaceReplacement>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation);
	}
};
/** @param center Exact existing paid workshop whose native window should be shown. */
void ShowLaunchAssemblyWindow(LaunchCenterRef center)
{
	if (ResolveLaunchCenter(center) != nullptr) AllocateWindowDescFront<LaunchAssemblyWindow>(desc, center.station.base(), center);
}

/** @copydoc ShowLaunchCraftWindow */
bool ShowLaunchCraftWindow(SpaceCraftRef craft)
{
    const auto *vehicle = ResolveSpaceCraft(craft);
    // The orbital inspector owns deployed roles. This native workshop can
    // disclose only its actual supported Earth/stored/carried/flight hulls;
    // a stale incarnation never changes the surviving current GUI selection.
    if (vehicle == nullptr || vehicle->construction.placement == SpacePlacement::Orbital ||
        std::ranges::find(models, GetSpaceVehicleModel(*vehicle)) == std::end(models)) return false;
    const auto home = vehicle->construction.home;
    const auto *center = ResolveLaunchCenter(home);
    // A reused station ID or another company's home is not this original
    // hull's workshop. Opening a window must never silently retarget recovery.
    if (center == nullptr || center->owner != vehicle->owner) return false;
    auto *window = static_cast<LaunchAssemblyWindow *>(FindWindowById(WindowClass::LaunchAssembly, home.station.base()));
    // Window numbers use station IDs, whereas native homes include generations.
    // Replace only an obsolete GUI incarnation before selecting the real home;
    // no facility, inventory, craft or reservation is created by this refresh.
    if (window != nullptr && window->center != home) { window->Close(); window = nullptr; }
    ShowLaunchAssemblyWindow(home);
    window = static_cast<LaunchAssemblyWindow *>(FindWindowById(WindowClass::LaunchAssembly, home.station.base()));
    if (window == nullptr) return false;
    window->selected = craft; window->selected_pod = {};
    if (GetSpaceVehicleModel(*vehicle) == 14) window->carrier = craft;
    window->SetDirty();
    return true;
}

/** @copydoc GetLaunchCraftWindowSelection */
SpaceCraftRef GetLaunchCraftWindowSelection(LaunchCenterRef center)
{
    const auto *window = static_cast<const LaunchAssemblyWindow *>(FindWindowById(WindowClass::LaunchAssembly, center.station.base()));
    // GUI window numbers omit generations. Only the actual surviving center's
    // current native selection may answer a cross-window presentation query.
    if (ResolveLaunchCenter(center) == nullptr || window == nullptr || window->center != center) return {};
    return ResolveSpaceCraft(window->selected) != nullptr ? window->selected : SpaceCraftRef{};
}

/** @copydoc GetLaunchPodWindowSelection */
SpacePodRef GetLaunchPodWindowSelection(LaunchCenterRef center)
{
    const auto *window = static_cast<const LaunchAssemblyWindow *>(FindWindowById(WindowClass::LaunchAssembly, center.station.base()));
    // A recycled station/window number never answers for another incarnation;
    // successful recovery has already consumed the exact selected live shell.
    if (ResolveLaunchCenter(center) == nullptr || window == nullptr || window->center != center) return {};
    return ResolveSpacePod(window->selected_pod) != nullptr ? window->selected_pod : SpacePodRef{};
}
