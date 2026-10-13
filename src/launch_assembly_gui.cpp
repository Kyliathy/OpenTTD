/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_assembly_gui.cpp Native material-paid Surface workshop controls; Plan9 Group8B. */
#include "stdafx.h"
#include "launch_operations.h"
#include "launch_center_cmd.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "space_transition.h"
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
};
static WindowDesc desc(WindowPosition::Automatic, {}, 0, 0, WindowClass::LaunchAssembly, WindowClass::StationView, {}, widgets);
struct LaunchAssemblyWindow : Window {
	LaunchCenterRef center;
	SpaceCraftRef selected;
	SpaceCraftRef carrier;
	SpaceCraftRef receiver;
	SpacePodRef selected_pod;
	Scrollbar *scrollbar = nullptr;
	mutable std::vector<std::pair<SpaceCraftRef, Rect>> rows;
	mutable std::vector<std::pair<SpacePodRef, Rect>> pod_rows;
	const IntervalTimer<TimerWindow> refresh = {std::chrono::milliseconds(250), [this]([[maybe_unused]] auto elapsed) { this->SetDirty(); }};
	LaunchAssemblyWindow(WindowDesc &description, WindowNumber number, LaunchCenterRef center) : Window(description), center(center) { this->InitNested(number); this->scrollbar = this->GetScrollbar(27); }
	void OnPaint() override
	{
		AutoRestoreBackup actor(_current_company, _local_company);
		const auto *s = ResolveLaunchCenter(this->center); if (s == nullptr) { this->Close(); return; }
		const auto *v = ResolveSpaceCraft(this->selected);
		const auto *pod = ResolveSpacePod(this->selected_pod);
		for (uint i = 0; i < std::size(models); ++i) this->SetWidgetDisabledState(i + 1, s->owner != _local_company || CmdAssembleLaunchCraft({}, center.station, center.generation, models[i]).Failed());
		this->SetWidgetDisabledState(5, pod != nullptr ? CmdPauseLaunchPod({}, selected_pod.pod, selected_pod.generation, !pod->state.paused).Failed() :
			v == nullptr || v->owner != _local_company || v->construction.home != center || v->construction.placement != SpacePlacement::Assembling);
		this->SetWidgetDisabledState(6, pod != nullptr ? CmdRecoverLaunchPod({}, center.station, center.generation, selected_pod.pod, selected_pod.generation).Failed() :
			v == nullptr || v->owner != _local_company || CmdRecoverLaunchCraft({}, center.station, center.generation, selected.vehicle, selected.generation).Failed());
		this->SetWidgetDisabledState(7, CmdAssembleLaunchPod({}, center.station, center.generation).Failed());
		for (uint widget : {8, 9}) this->SetWidgetDisabledState(widget, v == nullptr || pod == nullptr ||
			CmdMoveLaunchPod({}, center.station, center.generation, selected.vehicle, selected.generation, selected_pod.pod, selected_pod.generation, widget == 9).Failed());
		this->SetWidgetDisabledState(10, v == nullptr || CmdServiceLaunchProbe({}, selected.vehicle, selected.generation, v->manifest && v->manifest->service_ticks != 0 && !v->manifest->service_paused).Failed());
		for (uint widget : {11, 12, 13}) this->SetWidgetDisabledState(widget, v == nullptr ||
			CmdMoveSpaceSupply({}, center.station, center.generation, selected.vehicle, selected.generation, widget - 11, widget == 11 ? 15 : 1, false).Failed());
		for (uint widget : {14, 15}) this->SetWidgetDisabledState(widget, v == nullptr ||
			CmdCarryLaunchProbe({}, carrier.vehicle, carrier.generation, selected.vehicle, selected.generation, widget == 15).Failed());
		for (uint widget : {16, 17, 18}) {
			const auto cargo = widget == 18 && v != nullptr ? v->cargo_type : GetCargoTypeByLabel(widget == 16 ? CargoLabel{"STEL"} : CargoLabel{"GOOD"});
			this->SetWidgetDisabledState(widget, v == nullptr || CmdMoveLaunchFreight({}, center.station, center.generation, selected.vehicle, selected.generation, cargo, widget == 18 && v != nullptr ? v->cargo.TotalCount() : 1, widget == 18).Failed());
		}
		this->SetWidgetDisabledState(19, v == nullptr || CmdBeginLaunchFlight({}, selected.vehicle, selected.generation, SpaceOrderTargetKind::LaunchCenter, center.station.base(), center.generation).Failed());
		this->SetWidgetDisabledState(20, v == nullptr || CmdCancelLaunchFlight({}, selected.vehicle, selected.generation).Failed());
		this->SetWidgetDisabledState(21, v == nullptr || CmdDivertLaunchRecovery({}, selected.vehicle, selected.generation, center.station, center.generation).Failed());
		this->SetWidgetDisabledState(22, v == nullptr || pod == nullptr || CmdReleaseOrbitalCargo({}, selected.vehicle, selected.generation, center.station, center.generation, selected_pod.pod, selected_pod.generation, static_cast<uint16_t>(v->cargo.TotalCount())).Failed());
		this->SetWidgetDisabledState(23, pod == nullptr || CmdRetargetSpaceDescent({}, selected_pod.pod, selected_pod.generation, center.station, center.generation).Failed());
		for (uint widget : {24, 25, 26}) this->SetWidgetDisabledState(widget, v == nullptr || !v->manifest ||
			CmdMoveSpaceSupply({}, center.station, center.generation, selected.vehicle, selected.generation, widget - 24, static_cast<uint16_t>(v->manifest->supplies[widget - 24].TotalCount()), true).Failed());
		this->SetWidgetDisabledState(28, v == nullptr || v->owner != _local_company || v->construction.placement != SpacePlacement::Orbital);
		this->SetWidgetDisabledState(29, v == nullptr || CmdBeginLaunchFlight({}, selected.vehicle, selected.generation, SpaceOrderTargetKind::Craft, receiver.vehicle.base(), receiver.generation).Failed());
		this->DrawWidgets();
	}
	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		if (widget != 0) return;
		const auto *s = ResolveLaunchCenter(center); if (s == nullptr) return;
		int y = r.top + 8; const int line = GetCharacterHeight(FontSize::Normal) + 3;
		auto draw = [&](const std::string &text, TextColour colour = TextColour::White) { DrawString(r.left + 8, r.right - 8, y, text, colour); y += line; };
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
		if (ResolveSpaceCraft(carrier) != nullptr) draw(GetString(STR_LAUNCH_LOGISTICS_CARRIER, carrier.vehicle.base()));
		if (ResolveSpaceCraft(receiver) != nullptr) draw(GetString(STR_LAUNCH_RESUPPLY_RECEIVER, receiver.vehicle));
		else if (receiver != SpaceCraftRef{}) draw(GetString(STR_SPACE_ORDER_TARGET_LOST));
		if (const auto *v = ResolveSpaceCraft(selected); v != nullptr && IsSpaceFlightActive(*v)) draw(GetString(flight_blocks[to_underlying(v->transition->blocked)]));
		rows.clear(); const auto area = GetLaunchCenterFootprint(s->launch_center->tile, s->launch_center->rotation);
		auto show_craft = [&](const SpaceVehicle &v) { return v.construction.home == center || (v.owner == s->owner &&
			((IsSpaceCraftAwaitingSurfaceRecovery(v) && area->Contains(v.construction.surface_tile)) ||
			(v.construction.placement == SpacePlacement::Orbital && IsInsideLaunchCenterDropZone(v.position, *GetLaunchCenterPlaneReference(s->launch_center->tile))) ||
			(IsSpaceFlightActive(v) && v.construction.home == LaunchCenterRef{} && IsInsideLaunchCenterDropZone(v.position, *GetLaunchCenterPlaneReference(s->launch_center->tile))))); };
		auto show_pod = [&](const SpacePod &pod) { return pod.state.owner == s->owner && (pod.state.home == center || (selected != SpaceCraftRef{} && pod.state.craft == selected) ||
			(pod.state.custody == SpacePodCustody::Unhomed && area->Contains(pod.state.surface_tile)) ||
			(pod.descent && IsInsideLaunchCenterDropZone(pod.descent->release_point, *GetLaunchCenterPlaneReference(s->launch_center->tile)))); };
		uint count = 0; for (const auto *v : SpaceVehicle::Iterate()) if (show_craft(*v)) ++count; for (const auto *pod : SpacePod::Iterate()) if (show_pod(*pod)) ++count;
		this->scrollbar->SetCapacity(std::max(1, (r.bottom - y + 1) / line)); this->scrollbar->SetCount(count);
		int item = 0; const auto first = this->scrollbar->GetPosition(), last = first + this->scrollbar->GetCapacity();
		for (const auto *v : SpaceVehicle::Iterate()) {
			if (!show_craft(*v)) continue; const auto ordinal = item++; if (ordinal < first || ordinal >= last) continue;
			uint i = 0; while (i < std::size(models) && models[i] != GetSpaceVehicleModel(*v)) ++i;
			if (i == std::size(models)) continue;
			const StringID status = v->construction.placement == SpacePlacement::Unhomed ? STR_LAUNCH_ASSEMBLY_UNHOMED :
				IsSpaceFlightActive(*v) ? flight_names[to_underlying(v->transition->phase)] : v->construction.placement == SpacePlacement::Orbital ? STR_LAUNCH_FLIGHT_ORBITAL :
				v->construction.placement == SpacePlacement::Carried ? STR_LAUNCH_LOGISTICS_CARRIED :
				v->construction.placement == SpacePlacement::SurfaceStored ? STR_LAUNCH_ASSEMBLY_STORED : v->construction.paused ? STR_LAUNCH_ASSEMBLY_PAUSED : STR_LAUNCH_ASSEMBLY_WORKING;
			const auto ref = GetSpaceCraftReference(*v); const int top = y;
			draw(GetString(STR_LAUNCH_ASSEMBLY_HULL, names[i], v->index.base(), status, IsSpaceFlightActive(*v) ? v->transition->remaining_ticks : v->construction.remaining_ticks), ref == selected ? TextColour::Yellow : TextColour::White);
			rows.push_back({ref, {r.left, top, r.right, y - 1}});
		}
		pod_rows.clear();
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
	void OnClick(Point pt, WidgetID widget, [[maybe_unused]] int count) override
	{
		if (widget == 0) {
			for (const auto &[ref, rect] : rows) if (pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom) {
				selected = ref; selected_pod = {}; if (const auto *v = ResolveSpaceCraft(ref); v != nullptr && GetSpaceVehicleModel(*v) == 14) carrier = ref; this->SetDirty(); return;
			}
			for (const auto &[ref, rect] : pod_rows) if (pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom) { selected_pod = ref; this->SetDirty(); return; }
			return;
		}
		if (this->IsWidgetDisabled(widget)) return;
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
		if (widget == 8 || widget == 9) Command<Commands::MoveLaunchPod>::Post(STR_LAUNCH_ASSEMBLY_FAILED, center.station, center.generation, selected.vehicle, selected.generation, selected_pod.pod, selected_pod.generation, widget == 9);
		if (widget == 10 && v != nullptr) Command<Commands::ServiceLaunchProbe>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation, v->manifest && v->manifest->service_ticks != 0 && !v->manifest->service_paused);
		if (widget >= 11 && widget <= 13) Command<Commands::MoveSpaceSupply>::Post(STR_LAUNCH_ASSEMBLY_FAILED, center.station, center.generation, selected.vehicle, selected.generation, static_cast<uint8_t>(widget - 11), static_cast<uint16_t>(widget == 11 ? 15 : 1), false);
		if (widget == 14 || widget == 15) Command<Commands::CarryLaunchProbe>::Post(STR_LAUNCH_ASSEMBLY_FAILED, carrier.vehicle, carrier.generation, selected.vehicle, selected.generation, widget == 15);
		if (widget >= 16 && widget <= 18 && v != nullptr) Command<Commands::MoveLaunchFreight>::Post(STR_LAUNCH_ASSEMBLY_FAILED, center.station, center.generation, selected.vehicle, selected.generation,
			widget == 18 ? v->cargo_type : GetCargoTypeByLabel(widget == 16 ? CargoLabel{"STEL"} : CargoLabel{"GOOD"}), static_cast<uint16_t>(widget == 18 ? v->cargo.TotalCount() : 1), widget == 18);
		if (widget == 19) Command<Commands::BeginLaunchFlight>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation, SpaceOrderTargetKind::LaunchCenter, uint32_t{center.station.base()}, center.generation);
		if (widget == 20) Command<Commands::CancelLaunchFlight>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation);
		if (widget == 21) Command<Commands::DivertLaunchRecovery>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation, center.station, center.generation);
		if (widget == 22 && v != nullptr) Command<Commands::ReleaseOrbitalCargo>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation, center.station, center.generation, selected_pod.pod, selected_pod.generation, static_cast<uint16_t>(v->cargo.TotalCount()));
		if (widget == 23) Command<Commands::RetargetSpaceDescent>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected_pod.pod, selected_pod.generation, center.station, center.generation);
		if (widget >= 24 && widget <= 26 && v != nullptr && v->manifest) Command<Commands::MoveSpaceSupply>::Post(STR_LAUNCH_ASSEMBLY_FAILED, center.station, center.generation, selected.vehicle, selected.generation,
			static_cast<uint8_t>(widget - 24), static_cast<uint16_t>(v->manifest->supplies[widget - 24].TotalCount()), true);
		if (widget == 28) { receiver = selected; this->SetDirty(); }
		if (widget == 29) Command<Commands::BeginLaunchFlight>::Post(STR_LAUNCH_ASSEMBLY_FAILED, selected.vehicle, selected.generation, SpaceOrderTargetKind::Craft, receiver.vehicle.base(), receiver.generation);
	}
};
void ShowLaunchAssemblyWindow(LaunchCenterRef center)
{
	if (ResolveLaunchCenter(center) != nullptr) AllocateWindowDescFront<LaunchAssemblyWindow>(desc, center.station.base(), center);
}
