/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_structure_gui.cpp Native module selection, paid placement and Core activation controls.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1-2.
 */
#include "stdafx.h"
#include "space_structure.h"
#include "space_structure_cmd.h"
#include "space_vehicle.h"
#include "space_vehicle_cmd.h"
#include "space_manifest.h"
#include "company_func.h"
#include "company_base.h"
#include "core/backup_type.hpp"
#include "dropdown_func.h"
#include "dropdown_type.h"
#include "command_func.h"
#include "map_func.h"
#include "window_gui.h"
#include "window_func.h"
#include "viewport_func.h"
#include "tilehighlight_func.h"
#include "map_layer_gui.h"
#include "gfx_func.h"
#include "strings_func.h"
#include "timer/timer.h"
#include "timer/timer_window.h"
#include "table/strings.h"
#include "safeguards.h"

static constexpr std::initializer_list<NWidgetPart> widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, Colours::DarkBlue),
		NWidget(WWT_CAPTION, Colours::DarkBlue), SetStringTip(STR_SPACE_CONSTRUCTION_CAPTION),
	EndContainer(),
	NWidget(WWT_DROPDOWN, Colours::DarkBlue, 11), SetStringTip(STR_SPACE_CONSTRUCTION_SELECT), SetFill(1, 0),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, Colours::DarkBlue, 0), SetMinimalSize(720, 400), SetResize(1, 1), SetScrollbar(10), EndContainer(),
		NWidget(NWID_VSCROLLBAR, Colours::DarkBlue, 10),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 1), SetStringTip(STR_SPACE_CONSTRUCTION_PLACE), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 2), SetStringTip(STR_SPACE_CONSTRUCTION_NEXT_PROBE), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 3), SetStringTip(STR_SPACE_CONSTRUCTION_QUEUE), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 7), SetStringTip(STR_SPACE_CONSTRUCTION_CLEAR), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 9), SetStringTip(STR_SPACE_ORDER_VISIT), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 4), SetStringTip(STR_SPACE_CONSTRUCTION_CANCEL), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 5), SetStringTip(STR_SPACE_CONSTRUCTION_COLLECT_STEEL), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 6), SetStringTip(STR_SPACE_CONSTRUCTION_COLLECT_GOODS), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 8), SetStringTip(STR_SPACE_CONSTRUCTION_LOCATE), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 12), SetStringTip(STR_SPACE_CONSTRUCTION_ACTIVATE), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 13), SetStringTip(STR_SPACE_CONSTRUCTION_LOAD_STEEL), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 14), SetStringTip(STR_SPACE_CONSTRUCTION_LOAD_GOODS), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 15), SetStringTip(STR_SPACE_CONSTRUCTION_UNLOAD), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 16), SetStringTip(STR_CORE_SERVICE_NEXT), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 17), SetStringTip(STR_CORE_SERVICE_START), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 18), SetStringTip(STR_CORE_SERVICE_PAUSE), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 19), SetStringTip(STR_CORE_SERVICE_RESUME), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 20), SetStringTip(STR_CORE_SERVICE_CANCEL), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 21), SetStringTip(STR_CORE_PANEL_TOGGLE), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 22), SetStringTip(STR_CORE_SERVICE_COMMISSION), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 23), SetStringTip(STR_CORE_SERVICE_UNLOAD_METAL), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 24), SetStringTip(STR_CORE_SERVICE_UNLOAD_ELECTRONICS), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 25), SetStringTip(STR_CORE_SERVICE_LOAD_METAL), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 26), SetStringTip(STR_CORE_SERVICE_LOAD_ELECTRONICS), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, 27), SetStringTip(STR_SPACE_CONSTRUCTION_RESERVE), SetFill(1, 0),
	EndContainer(),
};
static WindowDesc desc(WindowPosition::Automatic, {}, 0, 0, WindowClass::SpaceConstruction, WindowClass::None, {}, widgets);
/** Resolve translated presentation without duplicating recipe quantities or construction authority.
 * @param kind Canonical static recipe. @return Translated module name, or the generic selector label.
 */
static StringID ModuleName(SpaceBuildKind kind)
{
	switch (kind) {
		case SpaceBuildKind::Core: return STR_SPACE_MODULE_CORE;
		case SpaceBuildKind::Dock: return STR_SPACE_MODULE_DOCK;
		case SpaceBuildKind::Storage: return STR_SPACE_MODULE_STORAGE;
		case SpaceBuildKind::Assembly: return STR_SPACE_MODULE_ASSEMBLY;
		case SpaceBuildKind::Steelworks: return STR_SPACE_MODULE_STEELWORKS;
		case SpaceBuildKind::Goodsworks: return STR_SPACE_MODULE_GOODSWORKS;
		case SpaceBuildKind::Power: return STR_SPACE_MODULE_POWER;
		case SpaceBuildKind::FuelStore: return STR_SPACE_MODULE_FUEL;
		case SpaceBuildKind::Refinery: return STR_SPACE_MODULE_REFINERY;
		case SpaceBuildKind::IceProcessor: return STR_SPACE_MODULE_ICE;
		default: return STR_SPACE_CONSTRUCTION_SELECT;
	}
}
/** Fund Industry-style native selector; all mutations pass through synchronized commands. */
struct SpaceConstructionWindow : Window {
	SpaceBuildKind kind = SpaceBuildKind::Core;
	SpaceStructureRef selected;
	SpaceCraftRef builder;
	SpaceCraftRef service_craft;
	mutable std::vector<std::pair<SpaceStructureRef, Rect>> rows;
	Scrollbar *scroll = nullptr;
	/** Refresh presentation only. @param elapsed Window intervals; never simulation work or cargo age. */
	const IntervalTimer<TimerWindow> refresh = {std::chrono::milliseconds(250), [this]([[maybe_unused]] auto elapsed) { this->SetDirty(); }};
	/** Initialize native controls and select the first owned orbital builder. */
	SpaceConstructionWindow() : Window(desc) { this->InitNested(0); this->scroll = this->GetScrollbar(10); this->NextBuilder(); this->NextServiceCraft(); }
	/** Cycle actual orbital probes independently of construction-builder selection. */
	void NextServiceCraft()
	{
		std::vector<SpaceCraftRef> craft;
		// The service selector admits only actual owned probe hulls; rockets/carried payloads cannot occupy this berth.
		for (const auto *v : SpaceVehicle::Iterate()) {
			const auto model = GetSpaceVehicleModel(*v);
			if (v->owner == _local_company && v->construction.placement == SpacePlacement::Orbital && (model == 15 || model == 19 || model == 20)) craft.push_back(GetSpaceCraftReference(*v));
		}
		const auto it = std::find(craft.begin(), craft.end(), this->service_craft);
		this->service_craft = craft.empty() ? SpaceCraftRef{} : (it == craft.end() || std::next(it) == craft.end() ? craft.front() : *std::next(it));
	}
	/** Cycle actual owned orbital builders in native pool order; payload passengers are not eligible. */
	void NextBuilder()
	{
		std::vector<SpaceCraftRef> builders;
		// Only real company builders already deployed in orbit may receive construction and collection commands.
		for (const auto *v : SpaceVehicle::Iterate()) if (v->owner == _local_company && GetSpaceVehicleModel(*v) == 20 && v->construction.placement == SpacePlacement::Orbital) builders.push_back(GetSpaceCraftReference(*v));
		const auto it = std::find(builders.begin(), builders.end(), this->builder);
		this->builder = builders.empty() ? SpaceCraftRef{} : (it == builders.end() || std::next(it) == builders.end() ? builders.front() : *std::next(it));
	}
	/** Resolve current native identities and update command availability before drawing. */
	void OnPaint() override
	{
		const auto *current = ResolveSpaceStructure(this->selected);
		// Removed/reused or foreign site identities cannot remain selected as actionable construction targets.
		if (current == nullptr || current->state.owner != _local_company) {
			this->selected = {};
			// Choose a surviving owned site without inferring ownership from screen location.
			for (const auto *s : SpaceStructure::Iterate()) if (s->state.owner == _local_company) { this->selected = GetSpaceStructureReference(*s); break; }
		}
		const auto *current_builder = ResolveSpaceCraft(this->builder);
		// Losing a builder or its orbital placement invalidates the UI reference, preserving native command ownership checks.
		if (current_builder == nullptr || current_builder->owner != _local_company || current_builder->construction.placement != SpacePlacement::Orbital) { this->builder = {}; this->NextBuilder(); }
		auto *site = ResolveSpaceStructure(this->selected); const auto *v = ResolveSpaceCraft(this->builder);
		const bool owned = site != nullptr && site->state.owner == _local_company;
		this->SetWidgetDisabledState(3, !owned || v == nullptr || v->owner != _local_company || site->state.phase != SpaceStructurePhase::Blueprint);
		this->SetWidgetDisabledState(4, !owned || site->state.phase != SpaceStructurePhase::Blueprint);
		this->SetWidgetDisabledState(7, v == nullptr || v->owner != _local_company);
		this->SetWidgetDisabledState(9, !owned || v == nullptr || v->owner != _local_company);
		this->SetWidgetDisabledState(8, site == nullptr);
		// Collection is restricted to actual unconsumed cancelled stock; activation never grants a refund path.
		for (uint i = 0; i < 2; ++i) this->SetWidgetDisabledState(5 + i, !owned || v == nullptr || site->state.phase != SpaceStructurePhase::Cancelled || site->job.inputs[i].TotalCount() == 0);
		this->SetWidgetDisabledState(12, !owned || site->state.kind != SpaceBuildKind::Core || site->state.phase != SpaceStructurePhase::FrameComplete || site->module != nullptr);
		const bool exchange = owned && v != nullptr && IsOrbitalModuleOperational(*site) && v->position == GetSpaceStructureApproach(site->state);
		// Module exchange is a precise approach action, independent of the launch center's twenty-tile drop circle.
		for (uint widget = 13; widget <= 15; ++widget) this->SetWidgetDisabledState(widget, !exchange);
		const auto *probe = ResolveSpaceCraft(this->service_craft);
		// Removed/foreign service selections are refreshed without changing the contained job's retained craft reference.
		if (probe == nullptr || probe->owner != _local_company) { this->service_craft = {}; this->NextServiceCraft(); probe = ResolveSpaceCraft(this->service_craft); }
		const bool operational = owned && IsOrbitalModuleOperational(*site);
		const bool busy = operational && site->module->service.remaining_ticks != 0;
		bool can_start = false;
		// The Start button uses the same pure whole admission as execution, including both real parts and the precise paid hull.
		if (operational && probe != nullptr) {
			AutoRestoreBackup actor(_current_company, _local_company);
			can_start = CmdServiceOrbitalProbe({}, site->index, site->generation, probe->index, probe->generation, OrbitalServiceControl::Start).Succeeded();
		}
		this->SetWidgetDisabledState(17, !can_start);
		// Pause/resume/cancel act on the actual occupant even when a different probe is selected or the original hull was lost.
		for (uint widget = 18; widget <= 20; ++widget) this->SetWidgetDisabledState(widget, !busy);
		this->SetWidgetDisabledState(21, !operational || site->module->producer == UtilityProducerID{UINT32_MAX});
		this->SetWidgetDisabledState(22, !operational || site->module->producer != UtilityProducerID{UINT32_MAX});
		// Every part-exchange button also preflights its actual source and destination; empty/full/stopped/remote owners stay visibly disabled.
		for (uint widget = 23; widget <= 26; ++widget) {
			bool allowed = false;
			if (operational && probe != nullptr) {
				AutoRestoreBackup actor(_current_company, _local_company);
				allowed = CmdMoveOrbitalSupply({}, site->index, site->generation, probe->index, probe->generation,
					static_cast<uint8_t>(1 + (widget - 23) % 2), 1, widget >= 25).Succeeded();
			}
			this->SetWidgetDisabledState(widget, !allowed);
		}
		bool can_reserve = false;
		// A paid inert blueprint can reserve only a complete missing-material batch from its actual owned adjacent Core.
		if (owned && site->state.phase == SpaceStructurePhase::Blueprint) {
			if (const auto *core = GetOrbitalConstructionCore(site->state)) {
				AutoRestoreBackup actor(_current_company, _local_company);
				can_reserve = CmdReserveOrbitalModuleInputs({}, site->index, site->generation, core->index, core->generation).Succeeded();
			}
		}
		this->SetWidgetDisabledState(27, !can_reserve);
		this->DrawWidgets();
	}
	/** @param rect Widget bounds. @param widget Native widget identity. */
	void DrawWidget(const Rect &rect, WidgetID widget) const override
	{
		if (widget != 0) return;
		this->rows.clear(); int y = rect.top + 8; const int line = GetCharacterHeight(FontSize::Normal) + 3;
		/** @param text Translated presentation. @param colour Row colour; highlights never change gameplay ownership. */
		auto draw = [&](const std::string &text, TextColour colour = TextColour::Black) { DrawString(rect.left + 8, rect.right - 8, y, text, colour); y += line; };
		const auto &d = *GetSpaceConstructionDefinition(this->kind);
		draw(GetString(STR_SPACE_CONSTRUCTION_COST, ModuleName(this->kind), d.capital, d.inputs[0].quantity, d.inputs[1].quantity, d.work_ticks));
		if (const auto *company = Company::GetIfValid(_local_company)) draw(GetString(STR_SPACE_CONSTRUCTION_CASH, company->money));
		if (this->kind != SpaceBuildKind::Core) draw(GetString(STR_SPACE_CONSTRUCTION_CORE_EDGE));
		const auto *site = ResolveSpaceStructure(this->selected); const auto *v = ResolveSpaceCraft(this->builder);
		uint assigned = 0;
		// Count each actual builder once when its remaining typed queue addresses this exact blueprint incarnation.
		for (const auto *probe : SpaceVehicle::Iterate()) for (size_t i = probe->space_orders.cursor; i < probe->space_orders.entries.size(); ++i) {
			const auto &o = probe->space_orders.entries[i];
			// A body/craft ID with the same numeric value is not an assignment to this structure.
			if (o.action == SpaceOrderAction::Construct && o.target.kind == SpaceOrderTargetKind::Blueprint && o.target.id == this->selected.structure.base() && o.target.generation == this->selected.generation) { ++assigned; break; }
		}
		draw(v == nullptr ? GetString(STR_SPACE_CONSTRUCTION_NO_PROBE) : GetString(STR_SPACE_CONSTRUCTION_PROBE, v->index, assigned));
		if (site != nullptr) {
			const auto &recipe = *GetSpaceConstructionDefinition(site->state.kind);
			draw(GetString(STR_SPACE_CONSTRUCTION_SELECTED, ModuleName(site->state.kind), site->index.base() + 1));
			draw(GetString(STR_SPACE_CONSTRUCTION_STOCK, site->job.inputs[0].TotalCount(), recipe.inputs[0].quantity, site->job.inputs[1].TotalCount(), recipe.inputs[1].quantity, site->job.work_ticks, recipe.work_ticks));
			draw(GetString(STR_SPACE_CONSTRUCTION_REMAINING,
				recipe.inputs[0].quantity - site->job.inputs[0].TotalCount() - site->job.consumed[0],
				recipe.inputs[1].quantity - site->job.inputs[1].TotalCount() - site->job.consumed[1]));
			draw(GetString(STR_SPACE_CONSTRUCTION_CONSUMED, site->job.consumed[0], site->job.consumed[1]));
			if (site->module) {
				if (site->module->activation_ticks != 0) draw(GetString(STR_SPACE_CONSTRUCTION_ACTIVATING, site->module->activation_ticks));
				else draw(GetString(STR_SPACE_CONSTRUCTION_OPERATIONAL, GetMultiLayerSpaceBudget().stock_cap));
				draw(GetString(STR_SPACE_CONSTRUCTION_INVENTORY, site->module->stock[GetCargoTypeByLabel(CargoLabel{"STEL"})].TotalCount(), site->module->stock[GetCargoTypeByLabel(CargoLabel{"GOOD"})].TotalCount()));
				const auto &module = *site->module; const auto &job = module.service;
				draw(GetString(STR_CORE_SERVICE_STOCK, module.stock[GetSpaceSupplyCargo(SpaceSupply::RefinedMetal)].TotalCount(), module.stock[GetSpaceSupplyCargo(SpaceSupply::Electronics)].TotalCount()));
				const auto *probe = ResolveSpaceCraft(job.remaining_ticks == 0 ? this->service_craft : job.craft);
				StringID status = STR_CORE_SERVICE_IDLE;
				// A settled current-epoch receipt remains visible as working after its grant has been consumed; UI never renews that grant.
				const bool worked = job.last_work_interval != 0 && job.last_work_interval == GetUtilityPowerInterval() && module.power_enabled;
				if (job.remaining_ticks != 0) status = job.paused ? STR_CORE_SERVICE_PAUSED : (worked || (probe != nullptr && IsOrbitalServiceWorkTick(*probe)) ? STR_CORE_SERVICE_WORKING : STR_CORE_SERVICE_WAITING);
				draw(GetString(STR_CORE_SERVICE_STATUS, status, job.remaining_ticks, module.services_completed));
				if (probe != nullptr) draw(GetString(STR_CORE_SERVICE_CRAFT, probe->index));
			}
		}
		y = DrawStringMultiLine(rect.left + 8, rect.right - 8, y, y + line * 4, GetString(STR_SPACE_CONSTRUCTION_HINT), TextColour::Black) + line;
		// List actual owned native sites; blueprint presence never synthesizes an operational station.
		std::vector<const SpaceStructure *> sites; for (const auto *s : SpaceStructure::Iterate()) if (s->state.owner == _local_company) sites.push_back(s);
		this->scroll->SetCount(static_cast<uint>(sites.size())); this->scroll->SetCapacity(std::max(1, (rect.bottom - y) / line));
		constexpr StringID phases[] = {STR_SPACE_CONSTRUCTION_BLUEPRINT, STR_SPACE_CONSTRUCTION_FRAME, STR_SPACE_CONSTRUCTION_CANCELLED};
		// Limit selectable rows to the viewport while retaining generation-qualified hit targets.
		for (int i = this->scroll->GetPosition(); i < std::min<int>(static_cast<int>(sites.size()), this->scroll->GetPosition() + this->scroll->GetCapacity()); ++i) {
			const auto *s = sites[i]; this->rows.push_back({GetSpaceStructureReference(*s), {rect.left, y, rect.right, y + line - 1}});
			draw(GetString(STR_SPACE_CONSTRUCTION_SITE, s->index.base() + 1, ModuleName(s->state.kind), s->state.x, s->state.y, IsOrbitalModuleOperational(*s) ? STR_SPACE_CONSTRUCTION_READY : phases[to_underlying(s->state.phase)]), GetSpaceStructureReference(*s) == this->selected ? TextColour::White : TextColour::Black);
		}
	}
	/** @param pt Window-local pointer position. @param widget Native action identity. @param click_count Framework click count. */
	void OnClick(Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		auto *site = ResolveSpaceStructure(this->selected); auto *v = ResolveSpaceCraft(this->builder);
		// Hit-testing chooses a native site reference, never a site inferred from viewport tile coordinates.
		if (widget == 0) for (const auto &[ref, rect] : this->rows) if (pt.y >= rect.top && pt.y <= rect.bottom) this->selected = ref;
		if (widget == 1) {
			const auto &d = *GetSpaceConstructionDefinition(this->kind);
			SetObjectToPlaceWnd(SPR_CURSOR_INDUSTRY, PAL_NONE, HT_RECT, this); SetTileSelectSize(d.width, d.height);
		}
		if (widget == 2) this->NextBuilder();
		// Commands revalidate ownership and incarnation; UI availability is only presentation, never mutation authority.
		if (widget == 3 && site != nullptr && v != nullptr) Command<Commands::EditSpaceOrder>::Post(v->index, v->generation, SpaceOrderEdit::Insert, static_cast<uint8_t>(v->space_orders.entries.size()), SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, uint32_t{site->index.base()}, site->generation);
		if (widget == 7 && v != nullptr) Command<Commands::EditSpaceOrder>::Post(v->index, v->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0});
		if (widget == 9 && site != nullptr && v != nullptr) Command<Commands::EditSpaceOrder>::Post(v->index, v->generation, SpaceOrderEdit::Insert, static_cast<uint8_t>(v->space_orders.entries.size()), SpaceOrderAction::Visit, SpaceOrderTargetKind::Structure, uint32_t{site->index.base()}, site->generation);
		if (widget == 4 && site != nullptr) Command<Commands::CancelSpaceBlueprint>::Post(site->index, site->generation);
		if ((widget == 5 || widget == 6) && site != nullptr && v != nullptr) {
			const uint slot = widget - 5; const auto cargo = GetCargoTypeByLabel(GetSpaceConstructionDefinition(site->state.kind)->inputs[slot].label);
			const auto capacity = GetSpaceFreightCapacity(20, cargo);
			const auto quantity = std::min<uint>(site->job.inputs[slot].TotalCount(), capacity - std::min<uint>(capacity, v->cargo.TotalCount()));
			Command<Commands::CollectSpaceConstructionInputs>::Post(site->index, site->generation, v->index, v->generation, cargo, static_cast<uint16_t>(quantity));
		}
		if (widget == 11) {
			DropDownList list;
			// The authoritative catalogue determines eligible static recipes; mobile hulls and mirrors have no footprint here.
			for (const auto &definition : GetSpaceConstructionDefinitions()) {
				if (definition.width == 0) continue;
				list.push_back(MakeDropDownListStringItem(ModuleName(definition.kind), to_underlying(definition.kind)));
			}
			ShowDropDownList(this, std::move(list), to_underlying(this->kind), 11);
		}
		// Starting activation is an explicit synchronized action on the selected paid frame.
		if (widget == 12 && site != nullptr) Command<Commands::ActivateOrbitalCore>::Post(site->index, site->generation);
		// These controls move actual common construction cargo at a precise approach; no proximity transfer or payment occurs.
		if (widget >= 13 && widget <= 15 && site != nullptr && site->module != nullptr && v != nullptr) {
			const bool load = widget != 15;
			const auto cargo = load ? GetCargoTypeByLabel(widget == 13 ? CargoLabel{"STEL"} : CargoLabel{"GOOD"}) : v->cargo_type;
			if (IsValidCargoType(cargo)) {
				const auto capacity = GetSpaceFreightCapacity(20, cargo);
				const uint quantity = load ? std::min<uint>(site->module->stock[cargo].TotalCount(), capacity - std::min<uint>(capacity, v->cargo.TotalCount())) : v->cargo.TotalCount();
				Command<Commands::MoveOrbitalFreight>::Post(site->index, site->generation, v->index, v->generation, cargo, static_cast<uint16_t>(quantity), load);
			}
		}
		if (widget == 8 && site != nullptr) ScrollMainWindowToTile(TileXYL(site->state.x, site->state.y, MapLayer::Space));
		if (widget == 16) this->NextServiceCraft();
		// Service controls address the saved occupant for existing jobs, preserving cancellation after hull loss.
		if (widget >= 17 && widget <= 20 && site != nullptr && site->module != nullptr) {
			const auto ref = widget == 17 ? this->service_craft : site->module->service.craft;
			Command<Commands::ServiceOrbitalProbe>::Post(site->index, site->generation, ref.vehicle, ref.generation, static_cast<OrbitalServiceControl>(widget - 17));
		}
		if (widget == 21 && site != nullptr && site->module != nullptr) Command<Commands::SetOrbitalPanelPaused>::Post(site->index, site->generation, site->module->power_enabled);
		if (widget == 22 && site != nullptr) Command<Commands::CommissionOrbitalServices>::Post(site->index, site->generation);
		// Only the synchronized command can turn delivered stock into this exact job's exclusive reservation.
		if (widget == 27 && site != nullptr) {
			if (const auto *core = GetOrbitalConstructionCore(site->state)) Command<Commands::ReserveOrbitalModuleInputs>::Post(site->index, site->generation, core->index, core->generation);
		}
		// Move one actual service part per click, through the same synchronized precise-exchange command as native tests.
		if (widget >= 23 && widget <= 26 && site != nullptr) {
			const auto ref = this->service_craft; const uint8_t supply = static_cast<uint8_t>(1 + (widget - 23) % 2);
			Command<Commands::MoveOrbitalSupply>::Post(site->index, site->generation, ref.vehicle, ref.generation, supply, uint16_t{1}, widget >= 25);
		}
		this->SetDirty();
	}
	/** @param widget Dropdown identity. @param index Canonical selected recipe ordinal. @param button Framework selection button. */
	void OnDropdownSelect(WidgetID widget, int index, [[maybe_unused]] int button) override
	{
		const auto *definition = GetSpaceConstructionDefinition(static_cast<SpaceBuildKind>(index));
		// Only a current static catalogue entry may change the placement recipe and footprint preview.
		if (widget != 11 || definition == nullptr || definition->width == 0) return;
		ResetObjectToPlace(); this->kind = definition->kind; this->SetDirty();
	}
	/** @param pt Framework pointer position. @param tile Explicit world tile selected for the current static recipe. */
	void OnPlaceObject([[maybe_unused]] Point pt, TileIndex tile) override { Command<Commands::BuildSpaceBlueprint>::Post(tile, this->kind); }
	/** Cancel cursor presentation without changing native site or cargo ownership. */
	void OnPlaceObjectAbort() override { this->SetDirty(); }
};
/** @copydoc ShowSpaceConstructionWindow */
void ShowSpaceConstructionWindow(SpaceStructureRef site)
{
	SpaceConstructionWindow *window;
	if (BringWindowToFrontById(WindowClass::SpaceConstruction, 0)) window = static_cast<SpaceConstructionWindow *>(FindWindowById(WindowClass::SpaceConstruction, 0));
	else window = new SpaceConstructionWindow();
	const auto *target = ResolveSpaceStructure(site);
	// A caller can focus an actual owned incarnation; stale or foreign references cannot create actionable UI ownership.
	if (target != nullptr && target->state.owner == _local_company) {
		window->selected = site; window->kind = target->state.kind;
	}
	window->SetDirty();
}
